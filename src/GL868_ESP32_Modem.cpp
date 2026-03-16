/*
 * GL868_ESP32_Modem.cpp
 * SIM868 modem driver implementation
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#include "GL868_ESP32_Modem.h"
#include "GL868_ESP32_Logger.h"

GL868_ESP32_Modem::GL868_ESP32_Modem()
    : _powered(false), _powerMode(POWER_OFF), _lastCommandTime(0) {}

void GL868_ESP32_Modem::begin() {
  // IMPORTANT: Set HIGH BEFORE setting as OUTPUT to avoid a brief LOW glitch
  // that would toggle modem power via the PDTC114EU transistor
  digitalWrite(GL868_ESP32_PWRKEY_PIN, HIGH); // Pre-set output register to HIGH
  pinMode(GL868_ESP32_PWRKEY_PIN, OUTPUT);    // Now safe to enable output

  // Initialize UART2 for SIM868
  Serial2.begin(GL868_ESP32_SIM868_BAUD, SERIAL_8N1, GL868_ESP32_SIM868_RX,
                GL868_ESP32_SIM868_TX);

  GL868_ESP32_LOG_I("Modem driver initialized (UART2: TX=%d, RX=%d, BAUD=%d)",
                    GL868_ESP32_SIM868_TX, GL868_ESP32_SIM868_RX,
                    GL868_ESP32_SIM868_BAUD);
}

void GL868_ESP32_Modem::pulseKey(uint32_t duration) {
  // PWRKEY pulse (active LOW via PDTC114EU transistor)
  GL868_ESP32_LOG_D("PWRKEY pulse: %lums", duration);

  digitalWrite(GL868_ESP32_PWRKEY_PIN,
               LOW); // Activate (transistor ON, PWRKEY pulled low)

  // Non-blocking delay using millis
  uint32_t start = millis();
  while (millis() - start < duration) {
    yield(); // Allow other tasks
  }

  digitalWrite(GL868_ESP32_PWRKEY_PIN, HIGH); // Release (transistor OFF, idle)
}

bool GL868_ESP32_Modem::waitForBoot() {
  GL868_ESP32_LOG_D("Waiting for modem boot...");

  uint32_t start = millis();
  while (millis() - start < GL868_ESP32_MODEM_BOOT_TIMEOUT) {
    if (isResponsive()) {
      GL868_ESP32_LOG_I("Modem booted in %lums", millis() - start);
      return true;
    }

    // Small delay between checks
    uint32_t delayStart = millis();
    while (millis() - delayStart < 500) {
      yield();
    }
  }

  GL868_ESP32_LOG_E("Modem boot timeout");
  return false;
}

bool GL868_ESP32_Modem::powerOn() {
  GL868_ESP32_LOG_I("Powering on modem...");

  // First, check if modem responds to AT command (might be on in any CFUN
  // state)
  if (isResponsive()) {
    GL868_ESP32_LOG_I("Modem already powered on (responds to AT)");
    _powered = true;

    // Modem is on - restore to full functionality (CFUN=1)
    // This handles wake from CFUN=0 or CFUN=4 sleep modes
    GL868_ESP32_LOG_D("Restoring CFUN=1 (full functionality)...");
    if (!sendAT("+CFUN=1", 15000)) {
      GL868_ESP32_LOG_W("AT+CFUN=1 command failed, but modem is responsive");
    }

    _powerMode = POWER_FULL;

    // Wait for modem to fully wake up from low power mode
    delay(500);

    // Flush any leftover data (e.g., from previous power down messages)
    flushSerial();

    // Disable echo
    sendAT("E0");

    // Final flush to clear any remaining garbage
    flushSerial();

    GL868_ESP32_LOG_I("Modem restored to full functionality");
    return true;
  }

  // Modem not responding - it's truly OFF, use PWRKEY to turn it on
  GL868_ESP32_LOG_D("Modem not responding, using PWRKEY to power on...");

  // Try up to 2 PWRKEY pulses (in case first one toggles modem OFF)
  for (int attempt = 0; attempt < 2; attempt++) {
    GL868_ESP32_LOG_D("PWRKEY power on attempt %d", attempt + 1);

    // Pulse PWRKEY to toggle power
    pulseKey(GL868_ESP32_PWRKEY_PULSE_MS);

    // Wait for modem to boot
    if (waitForBoot()) {
      _powered = true;
      _powerMode = POWER_FULL;

      // Disable echo
      sendAT("E0");

      return true;
    }

    // If first attempt failed, modem might have been ON and we turned it OFF
    // Second pulse should turn it back ON
    if (attempt == 0) {
      GL868_ESP32_LOG_W("First PWRKEY pulse failed, trying second pulse...");
    }
  }

  GL868_ESP32_LOG_E("Failed to power on modem after 2 attempts");
  return false;
}

bool GL868_ESP32_Modem::powerOff() {
  GL868_ESP32_LOG_I("Powering off modem...");

  // If not responsive, assume already off
  if (!isResponsive()) {
    GL868_ESP32_LOG_I("Modem already off");
    _powered = false;
    _powerMode = POWER_OFF;
    return true;
  }

  // Try graceful shutdown with AT+CPOWD=1
  // NOTE: We send manually instead of using sendAT() because modem responds
  // with "NORMAL POWER DOWN" instead of "OK", which would make sendAT() fail
  flushSerial();
  Serial2.println("AT+CPOWD=1");

  // Wait for "NORMAL POWER DOWN" or "POWER DOWN" response
  uint32_t start = millis();
  bool gotPowerDown = false;
  while (millis() - start < 5000) {
    if (Serial2.available()) {
      String line = Serial2.readStringUntil('\n');
      GL868_ESP32_LOG_V("<< %s", line.c_str());

      if (line.indexOf("POWER DOWN") >= 0) {
        gotPowerDown = true;
        GL868_ESP32_LOG_I("Modem powered down gracefully");
        _powered = false;
        _powerMode = POWER_OFF;

        // Wait for modem to fully shut down before returning
        delay(1000);
        return true;
      }
    }
    yield();
  }

  // Only pulse PWRKEY if we didn't get POWER DOWN response
  if (!gotPowerDown) {
    GL868_ESP32_LOG_W(
        "Graceful shutdown failed - forcing power off with PWRKEY");
    pulseKey(GL868_ESP32_PWRKEY_PULSE_MS);

    // Wait and verify
    uint32_t delayStart = millis();
    while (millis() - delayStart < GL868_ESP32_PWRKEY_IDLE_MS) {
      yield();
    }

    if (!isResponsive()) {
      _powered = false;
      _powerMode = POWER_OFF;
      GL868_ESP32_LOG_I("Modem powered off");
      return true;
    }

    GL868_ESP32_LOG_E("Failed to power off modem");
    return false;
  }

  return true;
}

bool GL868_ESP32_Modem::setFunctionality(ModemPowerMode mode) {
  const char *cmd = nullptr;

  switch (mode) {
  case POWER_OFF:
    return powerOff();

  case POWER_CFUN0:
    cmd = "+CFUN=0";
    break;

  case POWER_CFUN4:
    cmd = "+CFUN=4";
    break;

  case POWER_FULL:
    cmd = "+CFUN=1";
    break;
  }

  if (cmd && sendAT(cmd, 10000)) {
    _powerMode = mode;
    GL868_ESP32_LOG_I("Modem functionality set to %d", mode);
    return true;
  }

  return false;
}

bool GL868_ESP32_Modem::isResponsive() {
  flushSerial();

  Serial2.println("AT");

  uint32_t start = millis();
  while (millis() - start < 1000) {
    if (Serial2.available()) {
      String line = Serial2.readStringUntil('\n');
      line.trim();
      if (line == "OK" || line == "AT") {
        // Read remaining OK if we got AT echo
        if (line == "AT") {
          uint32_t waitStart = millis();
          while (millis() - waitStart < 500) {
            if (Serial2.available()) {
              line = Serial2.readStringUntil('\n');
              line.trim();
              if (line == "OK")
                break;
            }
            yield();
          }
        }
        return true;
      }
    }
    yield();
  }

  return false;
}

void GL868_ESP32_Modem::flushSerial() {
  while (Serial2.available()) {
    Serial2.read();
  }
}

bool GL868_ESP32_Modem::sendAT(const char *cmd, uint32_t timeout) {
  char fullCmd[64];
  snprintf(fullCmd, sizeof(fullCmd), "AT%s", cmd);
  return sendCommand(fullCmd, timeout);
}

bool GL868_ESP32_Modem::sendCommand(const char *cmd, uint32_t timeout) {
  GL868_ESP32_LOG_V(">> %s", cmd);

  flushSerial();
  Serial2.println(cmd);
  _lastCommandTime = millis();

  return waitOK(timeout);
}

bool GL868_ESP32_Modem::waitResponse(const char *expected, uint32_t timeout) {
  _response = "";

  uint32_t start = millis();
  while (millis() - start < timeout) {
    if (Serial2.available()) {
      char c = Serial2.read();
      _response += c;

      // Check for expected response
      if (_response.indexOf(expected) >= 0) {
        GL868_ESP32_LOG_V("<< %s (found: %s)", _response.c_str(), expected);
        return true;
      }
    }
    yield();
  }

  GL868_ESP32_LOG_D("<< %s (timeout waiting for: %s)", _response.c_str(),
                    expected);
  return false;
}

bool GL868_ESP32_Modem::waitOK(uint32_t timeout) {
  _response = "";

  uint32_t start = millis();
  while (millis() - start < timeout) {
    if (Serial2.available()) {
      String line = Serial2.readStringUntil('\n');
      line.trim();
      _response += line + "\n";

      GL868_ESP32_LOG_V("<< %s", line.c_str());

      if (line == "OK") {
        return true;
      }
      if (line == "ERROR" || line.startsWith("+CME ERROR") ||
          line.startsWith("+CMS ERROR")) {
        GL868_ESP32_LOG_D("Command error: %s", line.c_str());
        return false;
      }
    }
    yield();
  }

  GL868_ESP32_LOG_D("Command timeout");
  return false;
}

bool GL868_ESP32_Modem::sendATGetResponse(const char *cmd, char *buffer,
                                          size_t len, uint32_t timeout) {
  GL868_ESP32_LOG_V(">> AT%s", cmd);

  flushSerial();

  char fullCmd[64];
  snprintf(fullCmd, sizeof(fullCmd), "AT%s", cmd);
  Serial2.println(fullCmd);

  _response = "";
  size_t pos = 0;

  uint32_t start = millis();
  while (millis() - start < timeout) {
    if (Serial2.available()) {
      char c = Serial2.read();
      _response += c;

      // Store in buffer
      if (pos < len - 1) {
        buffer[pos++] = c;
        buffer[pos] = '\0';
      }

      // Check for OK
      if (_response.indexOf("\nOK") >= 0 || _response.endsWith("OK\r")) {
        GL868_ESP32_LOG_V("<< %s", _response.c_str());
        return true;
      }

      // Check for ERROR
      if (_response.indexOf("ERROR") >= 0) {
        GL868_ESP32_LOG_D("Command error");
        return false;
      }
    }
    yield();
  }

  GL868_ESP32_LOG_D("Command timeout");
  return false;
}
