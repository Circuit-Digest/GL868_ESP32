/*
 * GL868_ESP32_Sleep.cpp
 * Sleep manager implementation
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#include "GL868_ESP32_Sleep.h"
#include "GL868_ESP32_Logger.h"
#include <driver/rtc_io.h>
#include <esp_sleep.h>

// RTC memory to store wake reason
RTC_DATA_ATTR static WakeSource _rtcWakeSource = WAKE_UNKNOWN;
RTC_DATA_ATTR static uint32_t _rtcBootCount = 0;
RTC_DATA_ATTR static uint32_t _rtcLastHeartbeatSec = 0;

GL868_ESP32_Sleep::GL868_ESP32_Sleep()
    : _sendInterval(GL868_ESP32_DEFAULT_SEND_INTERVAL),
      _fullOffThreshold(GL868_ESP32_FULL_POWEROFF_THRESHOLD_SEC),
      _heartbeatInterval(GL868_ESP32_DEFAULT_HEARTBEAT_INTERVAL),
      _lastSendTime(0), _lastHeartbeatTime(0), _motionWakeEnabled(false),
      _callMonitorEnabled(false), _smsMonitorEnabled(false),
      _heartbeatEnabled(false), _fullPowerOffEnabled(true),
      _sim868Mode(SIM868_FULL_OFF), _riPin(GL868_ESP32_RI_PIN_DEFAULT),
      _detectedWakeSource(WAKE_UNKNOWN) {}

void GL868_ESP32_Sleep::begin() {
  _rtcBootCount++;

  // Determine wake source from ESP32 wake reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
  case ESP_SLEEP_WAKEUP_TIMER:
    _detectedWakeSource = WAKE_TIMER;
    break;

  case ESP_SLEEP_WAKEUP_EXT0:
    // EXT0 is exclusively used for RI pin (call/SMS wake).
    // esp_sleep_get_ext1_wakeup_status() does NOT work for EXT0 — it only
    // returns valid data for EXT1. So we know directly: EXT0 = RI = Call/SMS.
    _detectedWakeSource = WAKE_CALL;
    GL868_ESP32_LOG_I("Wake detected: RI pin (EXT0) -> Call/SMS");
    break;

  case ESP_SLEEP_WAKEUP_EXT1:
    // EXT1 is used for motion sensor interrupt
    {
      uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
      if (wakeup_pin_mask & (1ULL << GL868_ESP32_MOTION_INT)) {
        _detectedWakeSource = WAKE_MOTION;
        GL868_ESP32_LOG_I("Wake detected: Motion sensor (EXT1)");
      } else {
        _detectedWakeSource = WAKE_UNKNOWN;
        GL868_ESP32_LOG_W("EXT1 wake but unknown pin mask: 0x%llx",
                          wakeup_pin_mask);
      }
    }
    break;

  default:
    _detectedWakeSource = WAKE_UNKNOWN;
    break;
  }

  GL868_ESP32_LOG_I("Sleep manager initialized (boot #%lu, wake: %s)",
                    _rtcBootCount,
                    GL868_ESP32_WakeSourceToString(_detectedWakeSource));
}

WakeSource GL868_ESP32_Sleep::getWakeSource() const {
  return _detectedWakeSource;
}

bool GL868_ESP32_Sleep::isScheduledWake() const {
  return _detectedWakeSource == WAKE_TIMER ||
         _detectedWakeSource == WAKE_UNKNOWN;
}

bool GL868_ESP32_Sleep::isMotionWake() const {
  return _detectedWakeSource == WAKE_MOTION;
}

bool GL868_ESP32_Sleep::shouldFullPowerOff() const {
  // Don't power off if call/SMS monitoring enabled
  if (_callMonitorEnabled || _smsMonitorEnabled) {
    return false;
  }

  // Check if sleep interval exceeds threshold
  if (_sendInterval >= _fullOffThreshold) {
    return _fullPowerOffEnabled;
  }

  return false;
}

bool GL868_ESP32_Sleep::canSendNow() const {
  if (_lastSendTime == 0)
    return true;

  return (millis() - _lastSendTime) >= GL868_ESP32_API_MIN_INTERVAL_MS;
}

bool GL868_ESP32_Sleep::isHeartbeatDue() {
  if (!_heartbeatEnabled)
    return false;

  // Use RTC memory for cross-sleep tracking
  uint32_t currentTimeSec = millis() / 1000 + (_rtcBootCount * _sendInterval);

  if (_rtcLastHeartbeatSec == 0) {
    // First boot, heartbeat due
    return true;
  }

  return (currentTimeSec - _rtcLastHeartbeatSec) >= _heartbeatInterval;
}

void GL868_ESP32_Sleep::configureWakeSources() {
  // Configure timer wake
  esp_sleep_enable_timer_wakeup(_sendInterval * 1000000ULL);
  GL868_ESP32_LOG_D("Timer wake enabled: %lu seconds", _sendInterval);

  // Configure EXT1 for motion interrupt (active HIGH)
  uint64_t ext1_high_mask = 0;

  // Motion interrupt triggers HIGH
  if (_motionWakeEnabled) {
    ext1_high_mask |= (1ULL << GL868_ESP32_MOTION_INT);
    GL868_ESP32_LOG_D("Motion wake enabled (GPIO%d)", GL868_ESP32_MOTION_INT);
  }

  // RI pin for call/SMS (only if monitoring enabled and modem not fully off)
  // SIM868 RI pin is ACTIVE LOW: HIGH in standby, goes LOW on call/SMS
  // Use EXT0 for RI since it supports LOW level trigger
  if ((_callMonitorEnabled || _smsMonitorEnabled) && !shouldFullPowerOff()) {
    // Enable internal pull-up on RI pin to prevent floating when disconnected
    rtc_gpio_pullup_en((gpio_num_t)_riPin);
    rtc_gpio_pulldown_dis((gpio_num_t)_riPin);

    // Use EXT0 for RI pin with LOW level trigger (active LOW)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)_riPin, 0); // 0 = wake on LOW
    GL868_ESP32_LOG_D("RI wake enabled (GPIO%d, active LOW with pull-up)",
                      _riPin);
  }

  // Enable EXT1 for motion if needed
  if (ext1_high_mask != 0) {
    esp_sleep_enable_ext1_wakeup(ext1_high_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
  }
}

void GL868_ESP32_Sleep::enterSleep() {
  GL868_ESP32_LOG_I("Entering deep sleep for %lu seconds...", _sendInterval);

  // Configure wake sources
  configureWakeSources();

  // Save state to RTC memory
  _rtcWakeSource = WAKE_UNKNOWN;

  // Small delay to allow log output
  delay(100);

  // Enter deep sleep
  esp_deep_sleep_start();

  // Should never reach here
}
