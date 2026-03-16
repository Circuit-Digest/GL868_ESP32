/*
 * GL868_ESP32_JSON.cpp
 * JSON payload builder implementation using ArduinoJson
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#include "GL868_ESP32_JSON.h"
#include "GL868_ESP32_Logger.h"

GL868_ESP32_JSON::GL868_ESP32_JSON() : _pointCount(0), _payloadCount(0) {
  _deviceId[0] = '\0';
}

void GL868_ESP32_JSON::begin(const char *deviceId) {
  strncpy(_deviceId, deviceId, GL868_ESP32_MAX_DEVICE_ID_LEN - 1);
  _deviceId[GL868_ESP32_MAX_DEVICE_ID_LEN - 1] = '\0';
  clear();
  GL868_ESP32_LOG_D("JSON builder initialized (device: %s)", _deviceId);
}

void GL868_ESP32_JSON::clear() {
  _pointCount = 0;
  // Note: We don't clear payload fields here, only data points
  GL868_ESP32_LOG_V("JSON data points cleared");
}

bool GL868_ESP32_JSON::addDataPoint(const GPSData &gps, uint8_t battery,
                                    int signalStrength) {
  if (_pointCount >= GL868_ESP32_MAX_BATCH_POINTS) {
    GL868_ESP32_LOG_E("JSON data point buffer full");
    return false;
  }

  _gpsPoints[_pointCount] = gps;
  _batteryPoints[_pointCount] = battery;
  _signalPoints[_pointCount] = signalStrength;
  _pointCount++;

  GL868_ESP32_LOG_V("Added data point %d: %.6f, %.6f (signal: %d)", _pointCount,
                    gps.latitude, gps.longitude, signalStrength);
  return true;
}

void GL868_ESP32_JSON::setPayloadFields(PayloadField *fields, uint8_t count) {
  // Clear existing fields
  _payloadCount = 0;

  if (fields && count > 0) {
    uint8_t toCopy = min(count, (uint8_t)MAX_PAYLOAD_KEYS);

    for (uint8_t i = 0; i < toCopy; i++) {
      // Copy entire PayloadField struct (includes type and all values)
      _payloadFields[i] = fields[i];
    }

    _payloadCount = toCopy;
    GL868_ESP32_LOG_V("Set %d payload fields", _payloadCount);
  }
}

size_t GL868_ESP32_JSON::getEstimatedSize() const {
  // Base structure: ~50 bytes
  // Device ID: ~40 bytes
  // Per data point: ~200 bytes (timestamp, lat, lon, altitude, speed, heading,
  // accuracy, satellites, battery) Per payload field: ~40 bytes per point

  size_t size = 100;         // Base overhead
  size += _pointCount * 200; // Core data per point (with extended GPS fields)
  size +=
      _pointCount * _payloadCount * 40; // Payload data (increased for strings)

  return size;
}

bool GL868_ESP32_JSON::build(char *buffer, size_t maxLen) {
  if (_pointCount == 0) {
    GL868_ESP32_LOG_E("Cannot build JSON: no data points");
    return false;
  }

  // Calculate required size for JsonDocument
  // ArduinoJson v7 uses JsonDocument without pre-allocation by default
  JsonDocument doc;

  // Set device_id
  doc["device_id"] = _deviceId;

  // Create arrays for core fields
  JsonArray timestampArr = doc["timestamp"].to<JsonArray>();
  JsonArray latArr = doc["lat"].to<JsonArray>();
  JsonArray longArr = doc["long"].to<JsonArray>();
  JsonArray altitudeArr = doc["altitude"].to<JsonArray>();
  JsonArray speedArr = doc["speed"].to<JsonArray>();
  JsonArray headingArr = doc["heading"].to<JsonArray>();
  JsonArray accuracyArr = doc["accuracy"].to<JsonArray>();
  JsonArray satellitesArr = doc["satellites"].to<JsonArray>();
  JsonArray signalArr = doc["signal_strength"].to<JsonArray>();
  JsonArray batteryArr = doc["battery"].to<JsonArray>();

  // Add data points
  for (uint8_t i = 0; i < _pointCount; i++) {
    timestampArr.add(_gpsPoints[i].timestamp);
    latArr.add(_gpsPoints[i].latitude);
    longArr.add(_gpsPoints[i].longitude);
    altitudeArr.add(_gpsPoints[i].altitude);
    speedArr.add(_gpsPoints[i].speed);
    headingArr.add(_gpsPoints[i].heading);
    accuracyArr.add(_gpsPoints[i].accuracy);
    satellitesArr.add(_gpsPoints[i].satellites);
    signalArr.add(_signalPoints[i]);
    batteryArr.add(_batteryPoints[i]);
  }

  // Add payload array if we have payload fields
  if (_payloadCount > 0) {
    JsonArray payloadArr = doc["payload"].to<JsonArray>();

    // Add one payload object for each data point
    for (uint8_t i = 0; i < _pointCount; i++) {
      JsonObject payloadObj = payloadArr.add<JsonObject>();

      for (uint8_t j = 0; j < _payloadCount; j++) {
        // Add value based on type
        switch (_payloadFields[j].type) {
        case PAYLOAD_FLOAT:
          payloadObj[_payloadFields[j].key] = _payloadFields[j].floatVal;
          break;
        case PAYLOAD_INT:
          payloadObj[_payloadFields[j].key] = _payloadFields[j].intVal;
          break;
        case PAYLOAD_STRING:
          payloadObj[_payloadFields[j].key] = _payloadFields[j].stringVal;
          break;
        case PAYLOAD_BOOL:
          payloadObj[_payloadFields[j].key] = _payloadFields[j].boolVal;
          break;
        }
      }
    }
  }

  // Serialize to buffer
  size_t written = serializeJson(doc, buffer, maxLen);

  if (written == 0) {
    GL868_ESP32_LOG_E("Failed to serialize JSON");
    return false;
  }

  if (written >= maxLen - 1) {
    GL868_ESP32_LOG_E("JSON buffer overflow (size: %u, max: %u)", written,
                      maxLen);
    return false;
  }

  GL868_ESP32_LOG_D("JSON built: %u bytes, %d points", written, _pointCount);
  GL868_ESP32_LOG_V("JSON: %s", buffer);

  return true;
}
