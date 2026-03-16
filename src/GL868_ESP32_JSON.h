/*
 * GL868_ESP32_JSON.h
 * JSON payload builder using ArduinoJson
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_JSON_H
#define GL868_ESP32_JSON_H

#include "GL868_ESP32_Config.h"
#include "GL868_ESP32_Types.h"
#include <Arduino.h>
#include <ArduinoJson.h>

class GL868_ESP32_JSON {
public:
  GL868_ESP32_JSON();

  /**
   * Initialize with device ID
   * @param deviceId Device identifier
   */
  void begin(const char *deviceId);

  /**
   * Clear all data points
   */
  void clear();

  /**
   * Add a GPS data point with battery level and signal strength
   * @param gps GPS data
   * @param battery Battery percentage
   * @param signalStrength GSM signal strength (0-31, 99=unknown)
   * @return true if added successfully
   */
  bool addDataPoint(const GPSData &gps, uint8_t battery,
                    int signalStrength = 99);

  /**
   * Set custom payload fields (clears existing)
   * @param fields Array of payload fields
   * @param count Number of fields
   */
  void setPayloadFields(PayloadField *fields, uint8_t count);

  /**
   * Build JSON string
   * @param buffer Output buffer
   * @param maxLen Maximum buffer length
   * @return true if successful
   */
  bool build(char *buffer, size_t maxLen);

  /**
   * Get estimated JSON size
   * @return Estimated size in bytes
   */
  size_t getEstimatedSize() const;

  /**
   * Get number of data points
   */
  uint8_t getPointCount() const { return _pointCount; }

  /**
   * Check if any data points are stored
   */
  bool hasData() const { return _pointCount > 0; }

private:
  char _deviceId[GL868_ESP32_MAX_DEVICE_ID_LEN];

  // Data point storage
  GPSData _gpsPoints[GL868_ESP32_MAX_BATCH_POINTS];
  uint8_t _batteryPoints[GL868_ESP32_MAX_BATCH_POINTS];
  int _signalPoints[GL868_ESP32_MAX_BATCH_POINTS];
  uint8_t _pointCount;

  // Payload fields (same for all points)
  PayloadField _payloadFields[MAX_PAYLOAD_KEYS];
  uint8_t _payloadCount;
};

#endif // GL868_ESP32_JSON_H
