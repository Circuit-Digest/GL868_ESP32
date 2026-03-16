/*
 * GL868_ESP32_Call.h
 * Call control plane
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_CALL_H
#define GL868_ESP32_CALL_H

#include "GL868_ESP32_Config.h"
#include "GL868_ESP32_Modem.h"
#include "GL868_ESP32_Types.h"
#include "GL868_ESP32_Whitelist.h"
#include <Arduino.h>

// Forward declarations
class GL868_ESP32_Modem;
class GL868_ESP32_Whitelist;

class GL868_ESP32_Call {
public:
  GL868_ESP32_Call();

  /**
   * Set dependencies
   */
  void setModem(GL868_ESP32_Modem *modem) { _modem = modem; }
  void setWhitelist(GL868_ESP32_Whitelist *whitelist) {
    _whitelist = whitelist;
  }

  /**
   * Initialize call system
   */
  void begin();

  /**
   * Disable call system
   */
  void end();

  /**
   * Enable/disable call monitoring
   */
  void enable(bool enabled);

  /**
   * Check if calls are enabled
   */
  bool isEnabled() const { return _enabled; }

  /**
   * Set default action on incoming call
   */
  void setDefaultAction(CallAction action) { _defaultAction = action; }

  /**
   * Register custom call handler
   * @param handler Callback function
   * @return true if registered
   */
  bool registerHandler(CallHandler handler);

  /**
   * Check for incoming call (call in loop)
   */
  void update();

  /**
   * Check if call ring detected (for wake source detection)
   */
  bool hasPendingCall() const { return _pendingCall; }

  /**
   * Clear pending call flag
   */
  void clearPendingCall() { _pendingCall = false; }

  /**
   * Make an outgoing call
   * @param number Phone number to call
   * @param timeout Timeout in seconds to wait for connection (0 = no wait)
   * @return true if call initiated/connected
   */
  bool makeCall(const char *number, uint8_t timeout = 30);

  /**
   * Answer an incoming call
   * @return true if answered successfully
   */
  bool answer();

  /**
   * Hang up current call (incoming or outgoing)
   */
  void hangup();

  /**
   * Check if call is in progress
   */
  bool isCallActive();

  /**
   * Callback to set for requesting GPS send
   */
  typedef void (*ActionCallback)(const char *action, const char *caller);
  void setActionCallback(ActionCallback callback) {
    _actionCallback = callback;
  }

  /**
   * Set LED callback for visual feedback
   */
  void setLEDCallback(LEDCallback callback);

private:
  void processIncomingCall(const char *caller);
  void checkRING();

  GL868_ESP32_Modem *_modem;
  GL868_ESP32_Whitelist *_whitelist;

  bool _enabled;
  bool _pendingCall;
  CallAction _defaultAction;

  // Custom handlers
  CallHandler _customHandlers[GL868_ESP32_MAX_CALL_HANDLERS];
  uint8_t _handlerCount;

  // Action callback
  ActionCallback _actionCallback;

  // LED callback
  LEDCallback _ledCallback;

  uint32_t _lastCheck;
};

#endif // GL868_ESP32_CALL_H
