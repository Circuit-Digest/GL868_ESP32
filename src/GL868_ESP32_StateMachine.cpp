/*
 * GL868_ESP32_StateMachine.cpp
 * Non-blocking state machine implementation
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#include "GL868_ESP32_StateMachine.h"
#include "GL868_ESP32_Logger.h"

GL868_ESP32_StateMachine::GL868_ESP32_StateMachine()
    : _state(STATE_BOOT), _previousState(STATE_BOOT), _stateEnteredAt(0),
      _retryCount(0), _sleepRequested(false), _wakeRequested(false),
      _stateChanged(false), _stateChangeCallback(nullptr),
      _errorCallback(nullptr), _dataSentCallback(nullptr) {}

void GL868_ESP32_StateMachine::begin() {
  _state = STATE_BOOT;
  _previousState = STATE_BOOT;
  _stateEnteredAt = millis();
  _retryCount = 0;
  _sleepRequested = false;
  _wakeRequested = false;
  _stateChanged = true;

  GL868_ESP32_LOG_I("State machine initialized");
}

void GL868_ESP32_StateMachine::update() {
  // Clear state changed flag at start of update
  _stateChanged = false;

  // Check for forced sleep request
  if (_sleepRequested && _state != STATE_SLEEP &&
      _state != STATE_SLEEP_PREPARE) {
    transitionTo(STATE_SLEEP_PREPARE);
    _sleepRequested = false;
  }

  // Check for wake request (after sleep)
  if (_wakeRequested && _state == STATE_SLEEP) {
    transitionTo(STATE_BOOT);
    _wakeRequested = false;
  }
}

void GL868_ESP32_StateMachine::transitionTo(SystemState newState) {
  if (_state == newState)
    return;

  _previousState = _state;
  _state = newState;
  _stateEnteredAt = millis();
  _retryCount = 0;
  _stateChanged = true;

  GL868_ESP32_LOG_I("State: %s -> %s",
                    GL868_ESP32_StateToString(_previousState),
                    GL868_ESP32_StateToString(_state));

  // Invoke callback
  if (_stateChangeCallback) {
    _stateChangeCallback(_previousState, _state);
  }
}

void GL868_ESP32_StateMachine::forceState(SystemState state) {
  transitionTo(state);
}

bool GL868_ESP32_StateMachine::incrementRetries(uint8_t maxRetries) {
  _retryCount++;

  if (_retryCount >= maxRetries) {
    GL868_ESP32_LOG_E("Max retries reached (%d)", maxRetries);
    return false;
  }

  GL868_ESP32_LOG_D("Retry %d/%d", _retryCount, maxRetries);
  return true;
}

void GL868_ESP32_StateMachine::reportError(ErrorCode code) {
  _lastError = code;
  GL868_ESP32_LOG_E("Error reported: %s", GL868_ESP32_ErrorToString(code));

  if (_errorCallback) {
    _errorCallback(code);
  }
}

void GL868_ESP32_StateMachine::reportDataSent(bool success, int httpCode) {
  if (success) {
    GL868_ESP32_LOG_I("Data sent successfully (HTTP %d)", httpCode);
  } else {
    GL868_ESP32_LOG_E("Data send failed (HTTP %d)", httpCode);
  }

  if (_dataSentCallback) {
    _dataSentCallback(success, httpCode);
  }
}
