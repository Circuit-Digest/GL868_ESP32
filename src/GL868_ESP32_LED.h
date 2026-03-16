/*
 * GL868_ESP32_LED.h
 * Non-blocking LED manager for WS2812B (system indicator) and Status LED (user)
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_LED_H
#define GL868_ESP32_LED_H

#include "GL868_ESP32_Config.h"
#include "GL868_ESP32_Types.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class GL868_ESP32_LED {
public:
  GL868_ESP32_LED();

  /**
   * Initialize LED pins and WS2812B
   */
  void begin();

  /**
   * Update LED patterns (called by background task)
   */
  void update();

  /**
   * Set LED state based on system state
   */
  void setState(LEDState state);

  /**
   * Set error state with blink count
   */
  void setError(ErrorCode code);

  /**
   * Turn off RGB LED (for sleep). Status LED is NOT touched — user-owned.
   */
  void off();

  /**
   * Stop background task and power off RGB
   */
  void end();

  /**
   * Get current LED state
   */
  LEDState getState() const { return _state; }

  /**
   * Enable/disable WS2812B power (via TPS22919DCKR)
   */
  void setPowerEnabled(bool enabled);

  /**
   * Check if WS2812B power is enabled
   */
  bool isPowerEnabled() const { return _powerEnabled; }

  /**
   * Set the overall brightness of the RGB LED
   * @param percentage 0-100 (default 25)
   */
  void setBrightness(uint8_t percentage);

  // -------------------------------------------------------------------------
  // User-controlled Status LED (GPIO GL868_ESP32_LED_STATUS)
  // The library never drives this LED after begin(). It is freely available
  // to the user application.
  // -------------------------------------------------------------------------

  /**
   * Turn the user status LED ON
   */
  void turnOnLED();

  /**
   * Turn the user status LED OFF
   */
  void turnOffLED();

  // -------------------------------------------------------------------------
  // Error loop tracking (used by sleep manager to wait for pattern to finish)
  // -------------------------------------------------------------------------

  /**
   * Returns true once all GL868_ESP32_ERROR_BLINK_LOOPS have completed.
   * Only meaningful after setError() has been called.
   */
  bool errorBlinksComplete() const { return _errorBlinksComplete; }

  /**
   * Reset error-complete flag (called at the start of each wake cycle)
   */
  void resetErrorBlinks() {
    _errorLoopCount = 0;
    _errorBlinksComplete = false;
  }

private:
  void setRGB(uint8_t r, uint8_t g, uint8_t b);
  void sendWS2812B(uint8_t r, uint8_t g, uint8_t b);
  static void ledTask(void *pvParameters);

  // Colour for current state
  void getStateColour(uint8_t &r, uint8_t &g, uint8_t &b) const;

  LEDState _state;
  ErrorCode _errorCode;
  uint32_t _lastUpdate;

  // State blink FSM
  bool _stateBlinkOn; // true = currently showing colour

  // Error blink FSM
  uint8_t _errBlinksDone;    // blinks done in current loop
  bool _errBlinkOn;          // RGB on/off within a blink
  uint8_t _errorLoopCount;   // how many full loops have completed
  bool _errorBlinksComplete; // set after all loops done

  bool _rgbOn;
  bool _powerEnabled;
  TaskHandle_t _taskHandle;

  // Current RGB output values
  uint8_t _currentR;
  uint8_t _currentG;
  uint8_t _currentB;

  // Global brightness (0-100 percentage)
  uint8_t _brightness;

  // Target blink count for current error
  uint8_t _targetBlinks;
};

#endif // GL868_ESP32_LED_H
