/*
 * GL868_ESP32_Sleep.h
 * Sleep manager for deep sleep and wake source detection
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_SLEEP_H
#define GL868_ESP32_SLEEP_H

#include "GL868_ESP32_Config.h"
#include "GL868_ESP32_Types.h"
#include <Arduino.h>

class GL868_ESP32_Sleep {
public:
  GL868_ESP32_Sleep();

  /**
   * Initialize sleep manager
   */
  void begin();

  /**
   * Set send interval (seconds)
   */
  void setSendInterval(uint32_t seconds) { _sendInterval = seconds; }

  /**
   * Get send interval (seconds)
   */
  uint32_t getSendInterval() const { return _sendInterval; }

  /**
   * Set threshold for full power off vs low power mode (seconds)
   */
  void setFullPowerOffThreshold(uint32_t seconds) {
    _fullOffThreshold = seconds;
  }

  /**
   * Enable/disable motion wake
   */
  void enableMotionWake(bool enabled) { _motionWakeEnabled = enabled; }

  /**
   * Check if motion wake is enabled
   */
  bool isMotionWakeEnabled() const { return _motionWakeEnabled; }

  /**
   * Enable/disable call monitoring (keeps modem in low power)
   */
  void enableCallMonitoring(bool enabled) { _callMonitorEnabled = enabled; }

  /**
   * Enable/disable SMS monitoring (keeps modem in low power)
   */
  void enableSMSMonitoring(bool enabled) { _smsMonitorEnabled = enabled; }

  /**
   * Set SIM868 sleep mode preference
   */
  void setSIM868SleepMode(SIM868SleepMode mode) { _sim868Mode = mode; }

  /**
   * Get SIM868 sleep mode preference
   */
  SIM868SleepMode getSIM868SleepMode() const { return _sim868Mode; }

  /**
   * Enable/disable heartbeat
   */
  void enableHeartbeat(bool enabled) { _heartbeatEnabled = enabled; }

  /**
   * Set heartbeat interval (seconds)
   */
  void setHeartbeatInterval(uint32_t seconds) { _heartbeatInterval = seconds; }

  /**
   * Get heartbeat interval (seconds)
   */
  uint32_t getHeartbeatInterval() const { return _heartbeatInterval; }

  /**
   * Check if heartbeat is due
   */
  bool isHeartbeatDue();

  /**
   * Set RI pin for call/SMS wake detection
   */
  void setRIPin(uint8_t pin) { _riPin = pin; }

  /**
   * Get RI pin
   */
  uint8_t getRIPin() const { return _riPin; }

  /**
   * Enter deep sleep
   */
  void enterSleep();

  /**
   * Get wake source (call after wake)
   */
  WakeSource getWakeSource() const;

  /**
   * Check if this is a scheduled (timer) wake
   */
  bool isScheduledWake() const;

  /**
   * Check if this is a motion wake
   */
  bool isMotionWake() const;

  /**
   * Check if modem should be fully powered off
   */
  bool shouldFullPowerOff() const;

  /**
   * Check if call/SMS monitoring is enabled
   */
  bool isCallSMSMonitoringEnabled() const {
    return _callMonitorEnabled || _smsMonitorEnabled;
  }

  /**
   * Reset last send time (for rate limiting)
   */
  void resetLastSendTime() { _lastSendTime = millis(); }

  /**
   * Check if rate limit allows sending
   */
  bool canSendNow() const;

  /**
   * Record heartbeat sent
   */
  void recordHeartbeat() { _lastHeartbeatTime = millis(); }

private:
  void configureWakeSources();
  void saveWakeReason();

  uint32_t _sendInterval;
  uint32_t _fullOffThreshold;
  uint32_t _heartbeatInterval;
  uint32_t _lastSendTime;
  uint32_t _lastHeartbeatTime;

  bool _motionWakeEnabled;
  bool _callMonitorEnabled;
  bool _smsMonitorEnabled;
  bool _heartbeatEnabled;
  bool _fullPowerOffEnabled;

  SIM868SleepMode _sim868Mode;
  uint8_t _riPin;

  WakeSource _detectedWakeSource;
};

#endif // GL868_ESP32_SLEEP_H
