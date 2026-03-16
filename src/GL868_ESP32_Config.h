/*
 * GL868_ESP32_Config.h
 * Configuration constants and defaults for GL868_ESP32 library
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_CONFIG_H
#define GL868_ESP32_CONFIG_H

// ============================================================================
// Hardware Pins - SIM868 UART (Fixed, cannot be changed)
// ============================================================================

#define GL868_ESP32_SIM868_TX 17
#define GL868_ESP32_SIM868_RX 18
#define GL868_ESP32_SIM868_BAUD 115200

// ============================================================================
// Hardware Pins - SIM868 Control
// ============================================================================

#define GL868_ESP32_PWRKEY_PIN 42
#define GL868_ESP32_RI_PIN_DEFAULT 16

// ============================================================================
// Hardware Pins - LED (WS2812B)
// ============================================================================

#define GL868_ESP32_WS2812B_PIN 15
#define GL868_ESP32_WS2812B_POWER_PIN 14 // TPS22919DCKR power switch
#define GL868_ESP32_LED_STATUS 47
#define GL868_ESP32_WS2812B_COUNT 1

// ============================================================================
// Hardware Pins - Motion Sensor (LIS3DHTR)
// ============================================================================

#define GL868_ESP32_MOTION_SDA 8
#define GL868_ESP32_MOTION_SCL 9
#define GL868_ESP32_MOTION_INT 2
#define GL868_ESP32_MOTION_ADDR 0x19

// ============================================================================
// Hardware Pins - Battery
// ============================================================================

#define GL868_ESP32_BATTERY_PIN 1
#define GL868_ESP32_BATTERY_MIN_MV 3000
#define GL868_ESP32_BATTERY_MAX_MV 4200

// ============================================================================
// Timeouts (ms) - Library internal, NOT user configurable
// ============================================================================

#define GL868_ESP32_MODEM_BOOT_TIMEOUT 10000
#define GL868_ESP32_AT_RESPONSE_TIMEOUT 3000
#define GL868_ESP32_SIM_READY_TIMEOUT 15000
#define GL868_ESP32_NETWORK_REG_TIMEOUT 60000
#define GL868_ESP32_GPRS_ATTACH_TIMEOUT 30000
#define GL868_ESP32_GPS_FIX_TIMEOUT 120000
#define GL868_ESP32_HTTP_RESPONSE_TIMEOUT 30000
#define GL868_ESP32_PWRKEY_PULSE_MS 1200
#define GL868_ESP32_PWRKEY_IDLE_MS 2000

// ============================================================================
// Retry Limits
// ============================================================================

#define GL868_ESP32_AT_RETRY_COUNT 5
#define GL868_ESP32_GPS_RETRY_COUNT 3
#define GL868_ESP32_HTTP_RETRY_COUNT 3

// ============================================================================
// API Configuration
// ============================================================================

#define GL868_ESP32_API_MIN_INTERVAL_MS 10000 // 10 seconds rate limit
#define GL868_ESP32_API_ENDPOINT                                               \
  "https://www.circuitdigest.cloud/api/v1/geolinker"
#define GL868_ESP32_API_ENDPOINT_HTTP                                          \
  "http://www.circuitdigest.cloud/api/v1/geolinker"
#define GL868_ESP32_APN "iot.com"

// Default connection method: 1 = HTTPS (SSL), 0 = HTTP (no SSL)
// Set to 0 if your SIM module has SSL connection issues
#define GL868_ESP32_USE_HTTPS 0

// Optional: Uncomment and set if your carrier requires APN authentication
// #define GL868_ESP32_APN_USER "username"
// #define GL868_ESP32_APN_PASS "password"

// ============================================================================
// Queue Configuration
// ============================================================================

#define GL868_ESP32_RAM_QUEUE_SIZE 10
#define GL868_ESP32_FS_QUEUE_SIZE 100
#define GL868_ESP32_QUEUE_FILENAME "/glesp_queue.dat"

// ============================================================================
// Power Configuration
// ============================================================================

#define GL868_ESP32_FULL_POWEROFF_THRESHOLD_SEC 300 // 5 minutes

// ============================================================================
// SMS Configuration
// ============================================================================

#define GL868_ESP32_MAX_SMS_HANDLERS 10
#define GL868_ESP32_MAX_SMS_CMD_LEN 16
#define GL868_ESP32_MAX_SMS_MSG_LEN 160

// ============================================================================
// Call Configuration
// ============================================================================

#define GL868_ESP32_MAX_CALL_HANDLERS 5

// ============================================================================
// Whitelist Configuration
// ============================================================================

#define GL868_ESP32_MAX_WHITELIST_NUMBERS 20
#define GL868_ESP32_MAX_PHONE_LEN 16
#define GL868_ESP32_NVS_NAMESPACE "glesp868"

// ============================================================================
// JSON Configuration
// ============================================================================

#define GL868_ESP32_MAX_DEVICE_ID_LEN 32
#define GL868_ESP32_MAX_API_KEY_LEN 32
#define GL868_ESP32_MAX_JSON_SIZE 2048
#define GL868_ESP32_MAX_BATCH_POINTS 10

// ============================================================================
// Motion Sensor Configuration
// ============================================================================

#define GL868_ESP32_MOTION_DEFAULT_THRESHOLD 40

// ============================================================================
// Default Values
// ============================================================================

#define GL868_ESP32_DEFAULT_SEND_INTERVAL 300       // 5 minutes
#define GL868_ESP32_DEFAULT_GPS_TIMEOUT 120         // 2 minutes
#define GL868_ESP32_DEFAULT_HEARTBEAT_INTERVAL 3600 // 1 hour

// ============================================================================
// LIS3DHTR Registers
// ============================================================================

#define LIS3DH_REG_WHO_AM_I 0x0F
#define LIS3DH_REG_CTRL_REG1 0x20
#define LIS3DH_REG_CTRL_REG2 0x21
#define LIS3DH_REG_CTRL_REG3 0x22
#define LIS3DH_REG_CTRL_REG4 0x23
#define LIS3DH_REG_CTRL_REG5 0x24
#define LIS3DH_REG_CTRL_REG6 0x25
#define LIS3DH_REG_INT1_CFG 0x30
#define LIS3DH_REG_INT1_SRC 0x31
#define LIS3DH_REG_INT1_THS 0x32
#define LIS3DH_REG_INT1_DURATION 0x33
#define LIS3DH_REG_OUT_TEMP_L 0x0C
#define LIS3DH_REG_OUT_TEMP_H 0x0D
#define LIS3DH_WHO_AM_I_VALUE 0x33

// ============================================================================
// LED Configuration
// ============================================================================

// State blink: RGB on for STATE_BLINK_ON_MS, off for STATE_BLINK_OFF_MS
#define GL868_ESP32_STATE_BLINK_ON_MS 250   // ms RGB is ON per blink
#define GL868_ESP32_STATE_BLINK_OFF_MS 1750 // ms RGB is OFF between blinks

// Error blink: RGB on for ERROR_BLINK_ON_MS, then off for ERROR_BLINK_OFF_MS
// Repeats N times (N = blink count for error code), then pauses
// ERROR_BLINK_PAUSE_MS
#define GL868_ESP32_ERROR_BLINK_ON_MS 200  // ms RGB is ON per blink
#define GL868_ESP32_ERROR_BLINK_OFF_MS 200 // ms RGB is OFF between blinks
#define GL868_ESP32_ERROR_BLINK_PAUSE_MS                                       \
  1000 // ms pause after N blinks before repeating

// Modem error (solid pattern): ON for ERROR_SOLID_ON_MS, off for
// ERROR_SOLID_OFF_MS
#define GL868_ESP32_ERROR_SOLID_ON_MS 2000  // ms RGB solid ON
#define GL868_ESP32_ERROR_SOLID_OFF_MS 1000 // ms RGB OFF

// How many times to repeat the error blink pattern before sleep
#define GL868_ESP32_ERROR_BLINK_LOOPS 5

#endif // GL868_ESP32_CONFIG_H
