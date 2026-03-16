/*
 * GL868_ESP32_GPS.h
 * GPS layer for GNSS control and data parsing
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_GPS_H
#define GL868_ESP32_GPS_H

#include "GL868_ESP32_Config.h"
#include "GL868_ESP32_Modem.h"
#include "GL868_ESP32_Types.h"
#include <Arduino.h>

// Forward declaration
class GL868_ESP32_Modem;

class GL868_ESP32_GPS {
public:
  GL868_ESP32_GPS();

  /**
   * Set modem reference
   */
  void setModem(GL868_ESP32_Modem *modem) { _modem = modem; }

  /**
   * Power on GPS module
   * @return true if successful
   */
  bool powerOn();

  /**
   * Power off GPS module
   * @return true if successful
   */
  bool powerOff();

  /**
   * Check if GPS is powered on
   */
  bool isPowered() const { return _powered; }

  /**
   * Get GPS fix (blocks until fix or timeout)
   * @param data Pointer to GPSData structure to fill
   * @param timeout Maximum wait time in ms
   * @return true if valid fix obtained
   */
  bool getFix(GPSData *data, uint32_t timeout = GL868_ESP32_GPS_FIX_TIMEOUT);

  /**
   * Get single GPS reading (non-blocking)
   * @param data Pointer to GPSData structure to fill
   * @return true if data is valid
   */
  bool getReading(GPSData *data);

  /**
   * Set timezone offset for timestamp conversion
   * @param hours Hour offset (-12 to +14)
   * @param minutes Minute offset (0, 15, 30, 45)
   */
  void setTimeOffset(int8_t hours, int8_t minutes);

  /**
   * Get current timezone offset
   */
  void getTimeOffset(int8_t &hours, int8_t &minutes) const {
    hours = _tzHours;
    minutes = _tzMinutes;
  }

private:
  bool parseGNSINF(const char *response, GPSData *data);
  void applyTimeOffset(GPSData *data);
  bool isValidCoordinate(double lat, double lon);

  GL868_ESP32_Modem *_modem;
  bool _powered;
  int8_t _tzHours;
  int8_t _tzMinutes;
};

#endif // GL868_ESP32_GPS_H
