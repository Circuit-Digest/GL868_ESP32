/*
 * GL868_ESP32_Call.cpp
 * Call control plane implementation
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#include "GL868_ESP32_Call.h"
#include "GL868_ESP32_Logger.h"

GL868_ESP32_Call::GL868_ESP32_Call()
    : _modem(nullptr), _whitelist(nullptr), _enabled(false),
      _pendingCall(false), _defaultAction(CALL_SEND_GPS), _handlerCount(0),
      _actionCallback(nullptr), _ledCallback(nullptr), _lastCheck(0) {}

void GL868_ESP32_Call::setLEDCallback(LEDCallback callback) {
  _ledCallback = callback;
}

void GL868_ESP32_Call::begin() {
  if (!_modem)
    return;

  // Enable caller ID
  _modem->sendAT("+CLIP=1", 2000);

  GL868_ESP32_LOG_I("Call system initialized");
}

void GL868_ESP32_Call::end() {
  _enabled = false;
  GL868_ESP32_LOG_D("Call system disabled");
}

void GL868_ESP32_Call::enable(bool enabled) {
  _enabled = enabled;
  if (enabled) {
    begin();
  }
  GL868_ESP32_LOG_I("Calls %s", enabled ? "enabled" : "disabled");
}

bool GL868_ESP32_Call::registerHandler(CallHandler handler) {
  if (_handlerCount >= GL868_ESP32_MAX_CALL_HANDLERS) {
    GL868_ESP32_LOG_E("Call handler limit reached");
    return false;
  }

  _customHandlers[_handlerCount] = handler;
  _handlerCount++;

  GL868_ESP32_LOG_D("Registered call handler");
  return true;
}

void GL868_ESP32_Call::update() {
  if (!_enabled || !_modem)
    return;

  // Check more frequently for calls
  if (millis() - _lastCheck < 500)
    return;
  _lastCheck = millis();

  checkRING();
}

void GL868_ESP32_Call::checkRING() {
  // Check for unsolicited RING or +CLIP
  while (_modem->getSerial().available()) {
    String line = _modem->getSerial().readStringUntil('\n');
    line.trim();

    if (line == "RING") {
      GL868_ESP32_LOG_I("Incoming call detected");
      _pendingCall = true;
      // Signal LED: incoming call
      if (_ledCallback)
        _ledCallback(LED_CALL_INCOMING);
      // Wait for CLIP with caller ID
      continue;
    }

    // +CLIP: "number",type
    if (line.startsWith("+CLIP:")) {
      int firstQuote = line.indexOf('"');
      int secondQuote = line.indexOf('"', firstQuote + 1);

      if (firstQuote >= 0 && secondQuote > firstQuote) {
        String caller = line.substring(firstQuote + 1, secondQuote);
        GL868_ESP32_LOG_I("Caller ID: %s", caller.c_str());

        processIncomingCall(caller.c_str());
      }
      break;
    }
  }
}

void GL868_ESP32_Call::processIncomingCall(const char *caller) {
  // Always hang up first
  hangup();

  // Check whitelist
  if (_whitelist && !_whitelist->isAllowed(caller)) {
    GL868_ESP32_LOG_I("Call from non-whitelisted number: %s", caller);
    return;
  }

  GL868_ESP32_LOG_I("Processing call from: %s", caller);

  // Execute custom handlers first
  for (uint8_t i = 0; i < _handlerCount; i++) {
    _customHandlers[i](caller);
  }

  // Execute default action
  switch (_defaultAction) {
  case CALL_HANGUP:
    // Already hung up
    break;

  case CALL_SEND_GPS:
    // Request GPS send via callback
    if (_actionCallback) {
      _actionCallback("SEND_GPS", caller);
    }
    break;

  case CALL_CUSTOM:
    // Only custom handlers, no built-in action
    break;
  }

  _pendingCall = true;

  // Restore LED to idle after call processing
  if (_ledCallback)
    _ledCallback(LED_IDLE);
}

void GL868_ESP32_Call::hangup() {
  if (!_modem)
    return;

  _modem->sendAT("H", 1000); // ATH - Hang up
  GL868_ESP32_LOG_D("Call hung up");
}

bool GL868_ESP32_Call::makeCall(const char *number, uint8_t timeout) {
  if (!_modem || !number || strlen(number) == 0) {
    GL868_ESP32_LOG_E("Invalid call parameters");
    return false;
  }

  GL868_ESP32_LOG_I("Dialing: %s", number);

  // Dial the number using ATD command
  char cmd[64];
  snprintf(cmd, sizeof(cmd), "D%s;", number); // ATD<number>; for voice call

  if (!_modem->sendAT(cmd, 5000)) {
    GL868_ESP32_LOG_E("Dial command failed");
    return false;
  }

  // If timeout specified, wait for connection or timeout
  if (timeout > 0) {
    uint32_t start = millis();
    while ((millis() - start) < (timeout * 1000UL)) {
      delay(500);
      // Check call status using AT+CLCC
      if (_modem->sendAT("+CLCC", 1000)) {
        String resp = _modem->getResponse();
        // If active call (state=0 means active)
        if (resp.indexOf(",0,") >= 0) {
          GL868_ESP32_LOG_I("Call connected");
          return true;
        }
        // If NO CARRIER or disconnected
        if (resp.indexOf("NO CARRIER") >= 0 || resp.indexOf("BUSY") >= 0 ||
            resp.indexOf("NO ANSWER") >= 0) {
          GL868_ESP32_LOG_W("Call failed: %s", resp.c_str());
          return false;
        }
      }
    }
    GL868_ESP32_LOG_W("Call timeout");
    hangup();
    return false;
  }

  return true; // Call initiated (no wait)
}

bool GL868_ESP32_Call::answer() {
  if (!_modem)
    return false;

  GL868_ESP32_LOG_I("Answering call");
  if (_modem->sendAT("A", 2000)) { // ATA - Answer call
    GL868_ESP32_LOG_I("Call answered");
    _pendingCall = false;
    // Signal LED: call active
    if (_ledCallback)
      _ledCallback(LED_CALL_ACTIVE);
    return true;
  }

  GL868_ESP32_LOG_E("Failed to answer call");
  return false;
}

bool GL868_ESP32_Call::isCallActive() {
  if (!_modem)
    return false;

  // Query call list with AT+CLCC
  if (_modem->sendAT("+CLCC", 1000)) {
    String resp = _modem->getResponse();
    // If there's any +CLCC response, there's an active/alerting call
    return resp.indexOf("+CLCC:") >= 0;
  }

  return false;
}
