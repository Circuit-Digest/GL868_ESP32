/*
 * GL868_ESP32_Types.h
 * Type definitions and enumerations for GL868_ESP32 library
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_TYPES_H
#define GL868_ESP32_TYPES_H

#include <Arduino.h>
#include <string.h>

// ============================================================================
// System States
// ============================================================================

enum SystemState {
  STATE_BOOT = 0,
  STATE_MODEM_POWER_ON,
  STATE_GSM_INIT,
  STATE_GSM_REGISTER,
  STATE_GPS_POWER_ON,
  STATE_GPS_WAIT_FIX,
  STATE_BUILD_JSON,
  STATE_GPRS_ATTACH,
  STATE_SEND_HTTP,
  STATE_SEND_OFFLINE_QUEUE,
  STATE_SLEEP_PREPARE,
  STATE_SLEEP,
  STATE_IDLE // Listening for SMS/Call (non-data modes)
};

// ============================================================================
// LED States
// ============================================================================

enum LEDState {
  LED_OFF = 0,
  LED_BOOT,
  LED_MODEM_POWER_ON,
  LED_GSM_INIT,
  LED_GSM_REGISTER,
  LED_GPS_POWER_ON,
  LED_GPS_WAIT_FIX,
  LED_BUILD_JSON,
  LED_GPRS_ATTACH,
  LED_SEND_HTTP,
  LED_SEND_OFFLINE,
  LED_WAIT_API_KEY,  // Green blink - waiting for API key (Factory Firmware)
  LED_SLEEP_PREPARE,
  LED_SLEEP,
  LED_IDLE,          // Dim white slow blink - listening for SMS/Call
  LED_SMS_RECEIVED,  // Orange flash - SMS received & processing
  LED_SMS_SENDING,   // Orange fast blink - sending SMS reply
  LED_CALL_INCOMING, // Yellow slow blink - incoming call
  LED_CALL_ACTIVE,   // Yellow solid - call in progress
  LED_ERROR
};

// ============================================================================
// Error Codes
// ============================================================================

enum ErrorCode {
  ERROR_NONE = 0,
  ERROR_NO_SIM,
  ERROR_NO_NETWORK,
  ERROR_NO_GPRS,
  ERROR_NO_GPS,
  ERROR_HTTP_FAIL,
  ERROR_MODEM,
  ERROR_MOTION_SENSOR,
  ERROR_QUEUE_FULL,
  ERROR_JSON_BUILD,
  ERROR_TIMEOUT
};

// ============================================================================
// Wake Sources
// ============================================================================

enum WakeSource {
  WAKE_UNKNOWN = 0,
  WAKE_TIMER,
  WAKE_MOTION,
  WAKE_CALL,
  WAKE_SMS
};

// ============================================================================
// Modem Power Modes
// ============================================================================

enum ModemPowerMode {
  POWER_OFF = 0, // Full power off via PWRKEY
  POWER_CFUN0,   // Minimum functionality (AT+CFUN=0)
  POWER_CFUN4,   // Disable RF (AT+CFUN=4)
  POWER_FULL     // Normal operation (AT+CFUN=1)
};

// ============================================================================
// SIM868 Sleep Modes
// ============================================================================

enum SIM868SleepMode {
  SIM868_FULL_OFF = 0, // PWRKEY power off
  SIM868_CFUN0,        // AT+CFUN=0
  SIM868_CFUN4         // AT+CFUN=4
};

// ============================================================================
// Queue Storage Types
// ============================================================================

enum QueueStorage { QUEUE_RAM = 0, QUEUE_LITTLEFS, QUEUE_SPIFFS };

// ============================================================================
// Call Actions
// ============================================================================

enum CallAction { CALL_HANGUP = 0, CALL_SEND_GPS, CALL_CUSTOM };

// ============================================================================
// Battery Source
// ============================================================================

enum BatterySource {
  BATTERY_SOURCE_ADC = 0, // Use ESP32 ADC (GPIO35)
  BATTERY_SOURCE_MODEM    // Use SIM868 AT+CBC command
};

// ============================================================================
// Operating Modes
// ============================================================================

/**
 * Operating mode flags (can be combined with bitwise OR)
 * Examples:
 *   MODE_DATA_ONLY  - GPS tracking + HTTP only
 *   MODE_SMS_ONLY   - SMS commands only (no data transmission)
 *   MODE_CALL_ONLY  - Call actions only
 *   MODE_DATA | MODE_SMS - Data + SMS
 *   MODE_FULL       - All features enabled
 */
enum OperatingModeFlags {
  MODE_DATA = 0x01, // GPS tracking + HTTP data transmission
  MODE_SMS = 0x02,  // SMS command processing
  MODE_CALL = 0x04  // Incoming call handling
};

// Pre-defined mode combinations for convenience
enum OperatingMode {
  MODE_DATA_ONLY = MODE_DATA,                  // 0x01
  MODE_SMS_ONLY = MODE_SMS,                    // 0x02
  MODE_CALL_ONLY = MODE_CALL,                  // 0x04
  MODE_DATA_SMS = MODE_DATA | MODE_SMS,        // 0x03
  MODE_DATA_CALL = MODE_DATA | MODE_CALL,      // 0x05
  MODE_SMS_CALL = MODE_SMS | MODE_CALL,        // 0x06
  MODE_FULL = MODE_DATA | MODE_SMS | MODE_CALL // 0x07 (default)
};

// ============================================================================
// Log Levels
// ============================================================================

enum LogLevel {
  LOG_OFF = 0,
  LOG_ERROR = 1,
  LOG_INFO = 2,
  LOG_DEBUG = 3,
  LOG_VERBOSE = 4
};

// ============================================================================
// Data Structures
// ============================================================================

/**
 * GPS Data structure
 */
struct GPSData {
  bool valid;
  double latitude;
  double longitude;
  float altitude;
  float speed;
  float heading;
  float accuracy;
  uint8_t satellites;
  char timestamp[20]; // "YYYY-MM-DD HH:MM:SS"

  GPSData()
      : valid(false), latitude(0.0), longitude(0.0), altitude(0.0), speed(0.0),
        heading(0.0), accuracy(0.0), satellites(0) {
    timestamp[0] = '\0';
  }
};

/**
 * Custom payload field with multi-type support
 */
#define MAX_PAYLOAD_KEYS 5
#define MAX_PAYLOAD_KEY_LEN 16
#define MAX_PAYLOAD_STR_LEN 32

/**
 * Payload value type enumeration
 */
enum PayloadType { PAYLOAD_FLOAT, PAYLOAD_INT, PAYLOAD_STRING, PAYLOAD_BOOL };

struct PayloadField {
  char key[MAX_PAYLOAD_KEY_LEN];
  PayloadType type;

  // Union for different value types
  union {
    float floatVal;
    int32_t intVal;
    bool boolVal;
  };
  char stringVal[MAX_PAYLOAD_STR_LEN]; // Separate for string (can't be in union
                                       // with non-trivial types)

  // Default constructor
  PayloadField() : type(PAYLOAD_FLOAT), floatVal(0.0f) {
    key[0] = '\0';
    stringVal[0] = '\0';
  }

  // Constructor for float: {"temperature", 25.5f}
  PayloadField(const char *k, float v) : type(PAYLOAD_FLOAT), floatVal(v) {
    strncpy(key, k, MAX_PAYLOAD_KEY_LEN - 1);
    key[MAX_PAYLOAD_KEY_LEN - 1] = '\0';
    stringVal[0] = '\0';
  }

  // Constructor for double (converts to float): {"temperature", 25.5}
  PayloadField(const char *k, double v)
      : type(PAYLOAD_FLOAT), floatVal((float)v) {
    strncpy(key, k, MAX_PAYLOAD_KEY_LEN - 1);
    key[MAX_PAYLOAD_KEY_LEN - 1] = '\0';
    stringVal[0] = '\0';
  }

  // Constructor for int: {"count", 42}
  PayloadField(const char *k, int v) : type(PAYLOAD_INT), intVal(v) {
    strncpy(key, k, MAX_PAYLOAD_KEY_LEN - 1);
    key[MAX_PAYLOAD_KEY_LEN - 1] = '\0';
    stringVal[0] = '\0';
  }

  // Constructor for string: {"status", "active"}
  PayloadField(const char *k, const char *v)
      : type(PAYLOAD_STRING), floatVal(0.0f) {
    strncpy(key, k, MAX_PAYLOAD_KEY_LEN - 1);
    key[MAX_PAYLOAD_KEY_LEN - 1] = '\0';
    strncpy(stringVal, v, MAX_PAYLOAD_STR_LEN - 1);
    stringVal[MAX_PAYLOAD_STR_LEN - 1] = '\0';
  }

  // Constructor for bool: {"enabled", true}
  PayloadField(const char *k, bool v) : type(PAYLOAD_BOOL), boolVal(v) {
    strncpy(key, k, MAX_PAYLOAD_KEY_LEN - 1);
    key[MAX_PAYLOAD_KEY_LEN - 1] = '\0';
    stringVal[0] = '\0';
  }
};

/**
 * Queue entry structure
 */
struct QueueEntry {
  GPSData gps;
  uint8_t battery;
  PayloadField payload[MAX_PAYLOAD_KEYS];
  uint8_t payloadCount;
  uint32_t storedTime;

  QueueEntry() : battery(0), payloadCount(0), storedTime(0) {}
};

// ============================================================================
// Callback Types
// ============================================================================

typedef void (*SMSHandler)(const char *sender, const char *command,
                           const char *args);
typedef void (*CallHandler)(const char *caller);
typedef void (*StateChangeCallback)(SystemState from, SystemState to);
typedef void (*ErrorCallback)(ErrorCode code);
typedef void (*DataSentCallback)(bool success, int httpCode);
typedef void (*LEDCallback)(LEDState state);

#endif // GL868_ESP32_TYPES_H
