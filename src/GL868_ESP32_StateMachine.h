/*
 * GL868_ESP32_StateMachine.h
 * Non-blocking state machine for system control
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_STATEMACHINE_H
#define GL868_ESP32_STATEMACHINE_H

#include "GL868_ESP32_Config.h"
#include "GL868_ESP32_Types.h"
#include <Arduino.h>

class GL868_ESP32_StateMachine {
public:
  GL868_ESP32_StateMachine();

  /**
   * Initialize state machine
   */
  void begin();

  /**
   * Update state machine (call every loop, non-blocking)
   */
  void update();

  /**
   * Get current state
   */
  SystemState getState() const { return _state; }

  /**
   * Get previous state
   */
  SystemState getPreviousState() const { return _previousState; }

  /**
   * Request transition to sleep
   */
  void requestSleep() { _sleepRequested = true; }

  /**
   * Request wake (typically after sleep)
   */
  void requestWake() { _wakeRequested = true; }

  /**
   * Force specific state (use with caution)
   */
  void forceState(SystemState state);

  /**
   * Check if state just changed
   */
  bool stateChanged() const { return _stateChanged; }

  /**
   * Clear state changed flag
   */
  void clearStateChanged() { _stateChanged = false; }

  /**
   * Get time in current state (ms)
   */
  uint32_t getTimeInState() const { return millis() - _stateEnteredAt; }

  /**
   * Get retry count for current operation
   */
  uint8_t getRetryCount() const { return _retryCount; }

  /**
   * Reset retry counter
   */
  void resetRetries() { _retryCount = 0; }

  /**
   * Increment retry counter
   * @return true if retries remaining
   */
  bool incrementRetries(uint8_t maxRetries);

  /**
   * Set state change callback
   */
  void onStateChange(StateChangeCallback callback) {
    _stateChangeCallback = callback;
  }

  /**
   * Set error callback
   */
  void onError(ErrorCallback callback) { _errorCallback = callback; }

  /**
   * Set data sent callback
   */
  void onDataSent(DataSentCallback callback) { _dataSentCallback = callback; }

  /**
   * Report error
   */
  void reportError(ErrorCode code);

  /**
   * Get the last reported error
   */
  ErrorCode getLastError() const { return _lastError; }

  /**
   * Report data sent result
   */
  void reportDataSent(bool success, int httpCode);

  /**
   * Check if sleep was requested
   */
  bool isSleepRequested() const { return _sleepRequested; }

  /**
   * Clear sleep request
   */
  void clearSleepRequest() { _sleepRequested = false; }

private:
  void transitionTo(SystemState newState);

  SystemState _state;
  SystemState _previousState;
  ErrorCode _lastError;
  uint32_t _stateEnteredAt;
  uint8_t _retryCount;
  bool _sleepRequested;
  bool _wakeRequested;
  bool _stateChanged;

  // Callbacks
  StateChangeCallback _stateChangeCallback;
  ErrorCallback _errorCallback;
  DataSentCallback _dataSentCallback;
};

#endif // GL868_ESP32_STATEMACHINE_H
