/*
 * GL868_ESP32_SMS.h
 * SMS control plane for command handling
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_SMS_H
#define GL868_ESP32_SMS_H

#include "GL868_ESP32_Config.h"
#include "GL868_ESP32_Modem.h"
#include "GL868_ESP32_Types.h"
#include "GL868_ESP32_Whitelist.h"
#include <Arduino.h>

// Forward declarations
class GL868_ESP32_Modem;
class GL868_ESP32_Whitelist;

class GL868_ESP32_SMS {
public:
  GL868_ESP32_SMS();

  /**
   * Set dependencies
   */
  void setModem(GL868_ESP32_Modem *modem) { _modem = modem; }
  void setWhitelist(GL868_ESP32_Whitelist *whitelist) {
    _whitelist = whitelist;
  }

  /**
   * Initialize SMS system
   */
  void begin();

  /**
   * Disable SMS system
   */
  void end();

  /**
   * Enable/disable SMS monitoring
   */
  void enable(bool enabled);

  /**
   * Check if SMS is enabled
   */
  bool isEnabled() const { return _enabled; }

  /**
   * Enable/disable built-in commands
   */
  void enableBuiltinCommands(bool enabled) { _builtinEnabled = enabled; }

  /**
   * Register custom SMS command handler
   * @param command Command string (e.g., "ALERT")
   * @param handler Callback function
   * @return true if registered
   */
  bool registerHandler(const char *command, SMSHandler handler);

  /**
   * Check for incoming SMS (call in loop)
   */
  void update();

  /**
   * Send SMS
   * @param number Recipient phone number
   * @param message Message text
   * @return true if sent
   */
  bool send(const char *number, const char *message);

  /**
   * Check if SMS ring detected (for wake source detection)
   */
  bool hasPendingSMS() const { return _pendingSMS; }

  /**
   * Clear pending SMS flag
   */
  void clearPendingSMS() { _pendingSMS = false; }

  /**
   * Callback to set for requesting actions
   */
  typedef void (*ActionCallback)(const char *action, const char *args);
  void setActionCallback(ActionCallback callback) {
    _actionCallback = callback;
  }

  /**
   * Set LED callback for visual feedback
   */
  void setLEDCallback(LEDCallback callback);

  /**
   * Delete all SMS messages from SIM memory
   */
  void deleteAllSMS();

private:
  void processIncoming(const char *sender, const char *message);
  bool handleBuiltinCommand(const char *sender, const char *cmd,
                            const char *args);
  void parseIncomingSMS();
  bool deleteSMS(int index);

  GL868_ESP32_Modem *_modem;
  GL868_ESP32_Whitelist *_whitelist;

  bool _enabled;
  bool _builtinEnabled;
  bool _pendingSMS;

  // Custom handlers
  SMSHandler _customHandlers[GL868_ESP32_MAX_SMS_HANDLERS];
  char _customCommands[GL868_ESP32_MAX_SMS_HANDLERS]
                      [GL868_ESP32_MAX_SMS_CMD_LEN];
  uint8_t _handlerCount;

  // Action callback
  ActionCallback _actionCallback;

  // LED callback
  LEDCallback _ledCallback;

  uint32_t _lastCheck;
};

#endif // GL868_ESP32_SMS_H
