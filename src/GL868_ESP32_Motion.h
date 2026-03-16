/*
 * GL868_ESP32_Motion.h
 * LIS3DHTR accelerometer driver with motion interrupt support
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_MOTION_H
#define GL868_ESP32_MOTION_H

#include "GL868_ESP32_Config.h"
#include <Arduino.h>
#include <Wire.h>

class GL868_ESP32_Motion {
public:
  GL868_ESP32_Motion();

  /**
   * Initialize the motion sensor
   * @return true if successful
   */
  bool begin();

  /**
   * Disable sensor before sleep
   */
  void end();

  /**
   * Check if sensor is connected
   * @return true if sensor responds correctly
   */
  bool isConnected();

  /**
   * Set motion detection sensitivity (raw register value)
   * @param threshold Threshold value (0-127), higher = less sensitive
   */
  void setSensitivity(uint8_t threshold);

  /**
   * Set motion detection threshold in milligrams
   * At ±2g scale: 1 LSB = 16mg, range ~16mg to ~2032mg
   * Recommended values:
   *   300-500mg: Asset in transit (detect handling)
   *   200-400mg: Parked vehicle (detect towing/theft)
   *   400-800mg: Equipment (detect usage)
   * @param mg Threshold in milligrams (16-2032)
   */
  void setThresholdMg(float mg);

  /**
   * Enable motion interrupt on INT1
   */
  void enableInterrupt();

  /**
   * Disable motion interrupt
   */
  void disableInterrupt();

  /**
   * Check if motion was detected (interrupt triggered)
   * @return true if motion detected
   */
  bool motionDetected();

  /**
   * Clear interrupt flag
   */
  void clearInterrupt();

  /**
   * Read temperature from built-in sensor
   * @return Temperature in degrees Celsius
   */
  float readTemperature();

  /**
   * Get current sensitivity threshold
   */
  uint8_t getSensitivity() const { return _threshold; }

private:
  void writeRegister(uint8_t reg, uint8_t value);
  uint8_t readRegister(uint8_t reg);

  uint8_t _threshold;
  bool _initialized;
  bool _interruptEnabled;
};

#endif // GL868_ESP32_MOTION_H
