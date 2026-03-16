/*
 * GL868_ESP32_Logger.h
 * Debug logging macros for GL868_ESP32 library
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_LOGGER_H
#define GL868_ESP32_LOGGER_H

#include "GL868_ESP32_Types.h"
#include <Arduino.h>

// ============================================================================
// Compile-time log level (can be overridden before including this header)
// ============================================================================

#ifndef GL868_ESP32_LOG_LEVEL
#define GL868_ESP32_LOG_LEVEL LOG_INFO
#endif

// ============================================================================
// Runtime log level support
// ============================================================================

extern LogLevel _glesp868_runtime_log_level;

inline void GL868_ESP32_SetLogLevel(LogLevel level) {
  _glesp868_runtime_log_level = level;
}

inline LogLevel GL868_ESP32_GetLogLevel() {
  return _glesp868_runtime_log_level;
}

// ============================================================================
// Logging Macros
// ============================================================================

#if GL868_ESP32_LOG_LEVEL >= LOG_ERROR
#define GL868_ESP32_LOG_E(fmt, ...)                                            \
  do {                                                                         \
    if (_glesp868_runtime_log_level >= LOG_ERROR)                              \
      Serial.printf("[E][GLESP] " fmt "\n", ##__VA_ARGS__);                    \
  } while (0)
#else
#define GL868_ESP32_LOG_E(fmt, ...)                                            \
  do {                                                                         \
  } while (0)
#endif

#if GL868_ESP32_LOG_LEVEL >= LOG_INFO
#define GL868_ESP32_LOG_I(fmt, ...)                                            \
  do {                                                                         \
    if (_glesp868_runtime_log_level >= LOG_INFO)                               \
      Serial.printf("[I][GLESP] " fmt "\n", ##__VA_ARGS__);                    \
  } while (0)
#define GL868_ESP32_LOG_W(fmt, ...)                                            \
  do {                                                                         \
    if (_glesp868_runtime_log_level >= LOG_INFO)                               \
      Serial.printf("[W][GLESP] " fmt "\n", ##__VA_ARGS__);                    \
  } while (0)
#else
#define GL868_ESP32_LOG_I(fmt, ...)                                            \
  do {                                                                         \
  } while (0)
#define GL868_ESP32_LOG_W(fmt, ...)                                            \
  do {                                                                         \
  } while (0)
#endif

#if GL868_ESP32_LOG_LEVEL >= LOG_DEBUG
#define GL868_ESP32_LOG_D(fmt, ...)                                            \
  do {                                                                         \
    if (_glesp868_runtime_log_level >= LOG_DEBUG)                              \
      Serial.printf("[D][GLESP] " fmt "\n", ##__VA_ARGS__);                    \
  } while (0)
#else
#define GL868_ESP32_LOG_D(fmt, ...)                                            \
  do {                                                                         \
  } while (0)
#endif

#if GL868_ESP32_LOG_LEVEL >= LOG_VERBOSE
#define GL868_ESP32_LOG_V(fmt, ...)                                            \
  do {                                                                         \
    if (_glesp868_runtime_log_level >= LOG_VERBOSE)                            \
      Serial.printf("[V][GLESP] " fmt "\n", ##__VA_ARGS__);                    \
  } while (0)
#else
#define GL868_ESP32_LOG_V(fmt, ...)                                            \
  do {                                                                         \
  } while (0)
#endif

// ============================================================================
// Helper macros for state/error logging
// ============================================================================

inline const char *GL868_ESP32_StateToString(SystemState state) {
  switch (state) {
  case STATE_BOOT:
    return "BOOT";
  case STATE_MODEM_POWER_ON:
    return "MODEM_POWER_ON";
  case STATE_GSM_INIT:
    return "GSM_INIT";
  case STATE_GSM_REGISTER:
    return "GSM_REGISTER";
  case STATE_GPS_POWER_ON:
    return "GPS_POWER_ON";
  case STATE_GPS_WAIT_FIX:
    return "GPS_WAIT_FIX";
  case STATE_BUILD_JSON:
    return "BUILD_JSON";
  case STATE_GPRS_ATTACH:
    return "GPRS_ATTACH";
  case STATE_SEND_HTTP:
    return "SEND_HTTP";
  case STATE_SEND_OFFLINE_QUEUE:
    return "SEND_OFFLINE_QUEUE";
  case STATE_SLEEP_PREPARE:
    return "SLEEP_PREPARE";
  case STATE_SLEEP:
    return "SLEEP";
  case STATE_IDLE:
    return "IDLE";
  default:
    return "UNKNOWN";
  }
}

inline const char *GL868_ESP32_ErrorToString(ErrorCode code) {
  switch (code) {
  case ERROR_NONE:
    return "NONE";
  case ERROR_NO_SIM:
    return "NO_SIM";
  case ERROR_NO_NETWORK:
    return "NO_NETWORK";
  case ERROR_NO_GPRS:
    return "NO_GPRS";
  case ERROR_NO_GPS:
    return "NO_GPS";
  case ERROR_HTTP_FAIL:
    return "HTTP_FAIL";
  case ERROR_MODEM:
    return "MODEM";
  case ERROR_MOTION_SENSOR:
    return "MOTION_SENSOR";
  case ERROR_QUEUE_FULL:
    return "QUEUE_FULL";
  case ERROR_JSON_BUILD:
    return "JSON_BUILD";
  case ERROR_TIMEOUT:
    return "TIMEOUT";
  default:
    return "UNKNOWN";
  }
}

inline const char *GL868_ESP32_WakeSourceToString(WakeSource source) {
  switch (source) {
  case WAKE_TIMER:
    return "TIMER";
  case WAKE_MOTION:
    return "MOTION";
  case WAKE_CALL:
    return "CALL";
  case WAKE_SMS:
    return "SMS";
  case WAKE_UNKNOWN:
    return "UNKNOWN";
  default:
    return "UNKNOWN";
  }
}

#endif // GL868_ESP32_LOGGER_H
