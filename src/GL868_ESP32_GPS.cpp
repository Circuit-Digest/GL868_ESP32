/*
 * GL868_ESP32_GPS.cpp
 * GPS layer implementation
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#include "GL868_ESP32_GPS.h"
#include "GL868_ESP32_Logger.h"
#include <stdlib.h>

GL868_ESP32_GPS::GL868_ESP32_GPS()
    : _modem(nullptr), _powered(false), _tzHours(0), _tzMinutes(0) {}

bool GL868_ESP32_GPS::powerOn() {
  if (!_modem)
    return false;

  GL868_ESP32_LOG_I("Powering on GPS...");

  // Reset _powered flag first to ensure fresh state
  _powered = false;

  // SIM868 GPS holds the last valid fix in volatile RAM even across sleep
  // cycles. There is no "flush cache" AT command — the only way to clear stale
  // data is to power-cycle the GPS engine (AT+CGNSPWR=0 then AT+CGNSPWR=1).
  // This ONLY affects the GPS subsystem; GSM/network stays fully registered.
  _modem->flushSerial();
  _modem->getSerial().println("AT+CGNSINF");

  uint32_t checkStart = millis();
  while (millis() - checkStart < 2000) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');

      if (line.startsWith("+CGNSINF:")) {
        int colonPos = line.indexOf(':');
        if (colonPos > 0) {
          String data = line.substring(colonPos + 1);
          data.trim();
          int runStatus = data.toInt();

          if (runStatus == 1) {
            GL868_ESP32_LOG_I(
                "GPS running - power-cycling to clear cached fix");
            _modem->sendAT("+CGNSPWR=0", 3000); // Stop GPS engine (clears RAM)
            delay(500);                         // Let engine fully shut down
          }
        }
        break;
      }
    }
    yield();
  }

  // GPS not running, try to power it on
  GL868_ESP32_LOG_D("GPS not running, sending power on command...");

  // Try GPS power on up to 3 times
  for (int attempt = 0; attempt < 3; attempt++) {
    if (attempt > 0) {
      GL868_ESP32_LOG_W("GPS power on retry %d", attempt + 1);
    }

    // Power on GNSS
    if (!_modem->sendAT("+CGNSPWR=1", 5000)) {
      GL868_ESP32_LOG_E("Failed to send GPS power command");
      continue;
    }

    // Disable NMEA raw data output to prevent serial clutter
    _modem->sendAT("+CGNSTST=0", 2000);

    // Poll for GPS running (run=1) with 5 second timeout
    // GPS engine takes time to initialize even after AT+CGNSPWR=1 OK
    uint32_t pollStart = millis();
    while (millis() - pollStart < 5000) {
      // Wait a bit before checking
      uint32_t delayStart = millis();
      while (millis() - delayStart < 500)
        yield();

      // Check if GPS is running by parsing CGNSINF
      _modem->flushSerial();
      _modem->getSerial().println("AT+CGNSINF");

      uint32_t readStart = millis();
      while (millis() - readStart < 2000) {
        if (_modem->getSerial().available()) {
          String line = _modem->getSerial().readStringUntil('\n');

          if (line.startsWith("+CGNSINF:")) {
            int colonPos = line.indexOf(':');
            if (colonPos > 0) {
              String data = line.substring(colonPos + 1);
              data.trim();
              int runStatus = data.toInt();

              if (runStatus == 1) {
                _powered = true;
                GL868_ESP32_LOG_I("GPS power on verified (run=1)");
                return true;
              }
              // run=0, keep polling
              break;
            }
          }
        }
        yield();
      }
    }

    GL868_ESP32_LOG_W("GPS run=1 not detected within timeout");
  }

  GL868_ESP32_LOG_E("GPS power on failed after retries");
  return false;
}

bool GL868_ESP32_GPS::powerOff() {
  if (!_modem)
    return false;

  if (_modem->sendAT("+CGNSPWR=0", 3000)) {
    _powered = false;
    GL868_ESP32_LOG_I("GPS powered off");
    return true;
  }

  return false;
}

bool GL868_ESP32_GPS::getFix(GPSData *data, uint32_t timeout) {
  if (!_modem || !data)
    return false;

  // Make sure GPS is powered on
  if (!_powered) {
    if (!powerOn())
      return false;
  }

  GL868_ESP32_LOG_I("Waiting for GPS fix (timeout: %lums)", timeout);

  // SIM868 hot-starts from NVRAM even after AT+CGNSPWR=0, immediately
  // returning the last cached fix with its old timestamp.
  // With satellites visible the engine updates every ~1 second.
  // We skip the first valid reading and only accept once the timestamp
  // advances — identical strategy to handleGPSWaitFix().
  char firstTimestamp[20] = {0};

  uint32_t start = millis();
  while (millis() - start < timeout) {
    if (getReading(data)) {
      if (data->valid && isValidCoordinate(data->latitude, data->longitude)) {

        if (firstTimestamp[0] == '\0') {
          // First valid reading — record timestamp, keep waiting
          strncpy(firstTimestamp, data->timestamp, sizeof(firstTimestamp) - 1);
          firstTimestamp[sizeof(firstTimestamp) - 1] = '\0';
          GL868_ESP32_LOG_I("Hot-start fix (stale): %s — waiting for current",
                            firstTimestamp);
        } else if (strcmp(data->timestamp, firstTimestamp) != 0) {
          // Timestamp has advanced — this is a fresh satellite fix
          GL868_ESP32_LOG_I("GPS fix obtained: %.6f, %.6f (sats: %d)",
                            data->latitude, data->longitude, data->satellites);
          return true;
        }
        // Same timestamp as first reading — still stale, keep polling
      }
    }

    // Wait between readings (1 second)
    uint32_t delayStart = millis();
    while (millis() - delayStart < 1000)
      yield();
  }

  GL868_ESP32_LOG_E("GPS fix timeout");
  return false;
}

bool GL868_ESP32_GPS::getReading(GPSData *data) {
  if (!_modem || !data)
    return false;

  char buffer[256];

  _modem->flushSerial();
  _modem->getSerial().println("AT+CGNSINF");

  // Read response
  size_t pos = 0;
  bool gotData = false;

  uint32_t start = millis();
  while (millis() - start < 2000) {
    if (_modem->getSerial().available()) {
      char c = _modem->getSerial().read();
      if (pos < sizeof(buffer) - 1) {
        buffer[pos++] = c;
        buffer[pos] = '\0';
      }

      // Check for complete response
      if (strstr(buffer, "OK") != nullptr) {
        gotData = true;
        break;
      }
    }
    yield();
  }

  if (!gotData) {
    GL868_ESP32_LOG_V("No GPS data received");
    return false;
  }

  // Find +CGNSINF line
  char *infoStart = strstr(buffer, "+CGNSINF:");
  if (!infoStart) {
    GL868_ESP32_LOG_V("GPS response format error");
    return false;
  }

  return parseGNSINF(infoStart, data);
}

bool GL868_ESP32_GPS::parseGNSINF(const char *response, GPSData *data) {
  // Format: +CGNSINF:
  // run,fix,UTC,lat,lon,alt,speed,heading,fixmode,reserved,HDOP,PDOP,VDOP,reserved,sats,reserved,HPA,VPA
  // Example: +CGNSINF:
  // 1,1,20240105100000.000,11.010915,77.013209,920.500,0.00,135.0,1,,1.2,1.4,0.9,,12,,5.0,7.0

  data->valid = false;

  // Skip "+CGNSINF: "
  const char *p = strchr(response, ':');
  if (!p)
    return false;
  p++;
  while (*p == ' ')
    p++;

  // Parse run status
  int runStatus = atoi(p);
  p = strchr(p, ',');
  if (!p)
    return false;
  p++;

  // Parse fix status
  int fixStatus = atoi(p);
  p = strchr(p, ',');
  if (!p)
    return false;
  p++;

  // Parse UTC timestamp (YYYYMMDDHHMMSS.sss)
  char utc[20];
  size_t utcLen = 0;
  while (*p && *p != ',' && utcLen < sizeof(utc) - 1) {
    utc[utcLen++] = *p++;
  }
  utc[utcLen] = '\0';

  if (*p == ',')
    p++;

  // Parse latitude
  data->latitude = atof(p);
  p = strchr(p, ',');
  if (!p)
    return false;
  p++;

  // Parse longitude
  data->longitude = atof(p);
  p = strchr(p, ',');
  if (!p)
    return false;
  p++;

  // Parse altitude
  data->altitude = atof(p);
  p = strchr(p, ',');
  if (!p)
    return false;
  p++;

  // Parse speed
  data->speed = atof(p);
  p = strchr(p, ',');
  if (!p)
    return false;
  p++;

  // Parse heading
  data->heading = atof(p);
  p = strchr(p, ',');
  if (!p)
    return false;
  p++;

  // Skip fixmode
  p = strchr(p, ',');
  if (!p)
    return false;
  p++;

  // Skip reserved
  p = strchr(p, ',');
  if (!p)
    return false;
  p++;

  // Parse HDOP (accuracy proxy)
  data->accuracy = atof(p);
  p = strchr(p, ',');
  if (!p)
    return false;
  p++;

  // Skip PDOP, VDOP, reserved
  for (int i = 0; i < 3; i++) {
    p = strchr(p, ',');
    if (!p)
      return false;
    p++;
  }

  // Parse satellites
  data->satellites = atoi(p);

  // Convert UTC timestamp to formatted string
  // Input: YYYYMMDDHHMMSS.sss -> Output: YYYY-MM-DD HH:MM:SS
  if (strlen(utc) >= 14) {
    snprintf(data->timestamp, sizeof(data->timestamp),
             "%.4s-%.2s-%.2s %.2s:%.2s:%.2s", utc, utc + 4, utc + 6, utc + 8,
             utc + 10, utc + 12);

    // Apply timezone offset
    applyTimeOffset(data);
  } else {
    data->timestamp[0] = '\0';
  }

  // Set validity based on fix status and coordinates
  data->valid = (runStatus == 1 && fixStatus == 1 &&
                 isValidCoordinate(data->latitude, data->longitude));

  GL868_ESP32_LOG_V(
      "GPS: run=%d, fix=%d, lat=%.6f, lon=%.6f, sats=%d, valid=%d", runStatus,
      fixStatus, data->latitude, data->longitude, data->satellites,
      data->valid);

  return true;
}

void GL868_ESP32_GPS::setTimeOffset(int8_t hours, int8_t minutes) {
  _tzHours = hours;
  _tzMinutes = minutes;
  GL868_ESP32_LOG_D("Timezone set to UTC%+d:%02d", hours, abs(minutes));
}

void GL868_ESP32_GPS::applyTimeOffset(GPSData *data) {
  // Parse current timestamp
  int year, month, day, hour, minute, second;
  if (sscanf(data->timestamp, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour,
             &minute, &second) != 6) {
    return;
  }

  // Apply offset
  minute += _tzMinutes;
  if (minute >= 60) {
    minute -= 60;
    hour++;
  } else if (minute < 0) {
    minute += 60;
    hour--;
  }

  hour += _tzHours;

  // Handle day rollover
  if (hour >= 24) {
    hour -= 24;
    day++;

    // Simple month length check (not accounting for leap years precisely)
    int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
      daysInMonth[2] = 29;
    }

    if (day > daysInMonth[month]) {
      day = 1;
      month++;
      if (month > 12) {
        month = 1;
        year++;
      }
    }
  } else if (hour < 0) {
    hour += 24;
    day--;

    if (day < 1) {
      month--;
      if (month < 1) {
        month = 12;
        year--;
      }

      int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
      if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        daysInMonth[2] = 29;
      }
      day = daysInMonth[month];
    }
  }

  // Format new timestamp
  snprintf(data->timestamp, sizeof(data->timestamp),
           "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute,
           second);
}

bool GL868_ESP32_GPS::isValidCoordinate(double lat, double lon) {
  // Check for 0,0 (invalid fix)
  if (lat == 0.0 && lon == 0.0)
    return false;

  // Check valid ranges
  if (lat < -90.0 || lat > 90.0)
    return false;
  if (lon < -180.0 || lon > 180.0)
    return false;

  return true;
}
