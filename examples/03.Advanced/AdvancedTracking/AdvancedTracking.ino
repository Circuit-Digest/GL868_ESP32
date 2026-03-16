/*
 * AdvancedTracking.ino
 * Advanced GPS tracking example for GL868_ESP32 library
 *
 * This example demonstrates ALL available configuration options including:
 * - Basic configuration (send interval, GPS timeout, timezone)
 * - Battery configuration (source, voltage range)
 * - Custom payload fields using simple syntax
 * - SMS command control with custom handlers
 * - Incoming call handling with custom handlers
 * - Motion detection and wake
 * - Sleep & power management
 * - Heartbeat mechanism
 * - Offline data queuing
 * - Event callbacks
 * - Logging configuration
 *
 * Hardware:
 * - ESP32
 * - SIM868 module
 * - LIS3DHTR accelerometer
 * - SIM card with data plan
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#include <GL868_ESP32.h>

// ============================================================================
// Configuration - EDIT THESE
// ============================================================================

const char *DEVICE_ID = "GPS_TRACKER_001";
const char *API_KEY = "YOUR_API_KEY_HERE"; // Replace with your API key

// ============================================================================
// Sensor Reading Functions (Example - implement for your hardware)
// ============================================================================

float readExternalTemperature() {
  // Example: Read from external temperature sensor
  return 25.5;
}

float readExternalHumidity() {
  // Example: Read from humidity sensor
  return 65.0;
}

float readExternalPressure() {
  // Example: Read from pressure sensor
  return 1013.25;
}

// ============================================================================
// Custom SMS Command Handler
// ============================================================================

void handleAlertCommand(const char *sender, const char *command,
                        const char *args) {
  Serial.printf("ALERT command from %s\n", sender);
  Serial.printf("Arguments: %s\n", args);

  // Example: Send acknowledgment
  GeoLinker.sendSMS(sender, "Alert received!");

  // Example: Trigger immediate GPS send
  GeoLinker.forceSend();
}

void handleRebootCommand(const char *sender, const char *command,
                         const char *args) {
  Serial.printf("REBOOT command from %s\n", sender);

  // Send acknowledgment first
  GeoLinker.sendSMS(sender, "Rebooting...");
  delay(1000);

  // Reboot ESP32
  ESP.restart();
}

void handleConfigCommand(const char *sender, const char *command,
                         const char *args) {
  Serial.printf("CONFIG command from %s: %s\n", sender, args);

  // Example: Parse config value
  String argStr = args;
  if (argStr.startsWith("GPS=")) {
    int timeout = argStr.substring(4).toInt();
    GeoLinker.setGPSTimeout(timeout);
    GeoLinker.sendSMS(sender, "GPS timeout updated");
  } else if (argStr.startsWith("MOTION=")) {
    float threshold = argStr.substring(7).toFloat();
    GeoLinker.setMotionThreshold(threshold);
    GeoLinker.sendSMS(sender, "Motion threshold updated");
  }
}

// ============================================================================
// Custom Call Handler
// ============================================================================

void handleIncomingCall(const char *caller) {
  Serial.printf("Incoming call from %s\n", caller);

  // Example: Toggle a GPIO when called
  pinMode(14, OUTPUT);
  digitalWrite(14, !digitalRead(14));

  Serial.println("GPIO14 toggled");
}

// ============================================================================
// Event Callbacks
// ============================================================================

void onStateChanged(SystemState from, SystemState to) {
  Serial.printf("State changed: %s -> %s\n", GL868_ESP32_StateToString(from),
                GL868_ESP32_StateToString(to));
}

void onErrorOccurred(ErrorCode code) {
  Serial.printf("ERROR: %s\n", GL868_ESP32_ErrorToString(code));

  // Example: You could send an SMS alert on critical errors
  // GeoLinker.sendSMS("+1234567890", "Device error!");
}

void onDataSentResult(bool success, int httpCode) {
  if (success) {
    Serial.printf("Data sent successfully (HTTP %d)\n", httpCode);
  } else {
    Serial.printf("Data send failed (HTTP %d)\n", httpCode);
  }
}

// ============================================================================
// Setup
// ============================================================================

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println("==========================================");
  Serial.println("GL868_ESP32 Advanced Tracking Example");
  Serial.println("All Configuration Options Demonstrated");
  Serial.println("==========================================");

  // Initialize the library
  GeoLinker.begin(DEVICE_ID, API_KEY);

  // ==========================================================
  // APN CONFIGURATION (can also be set before begin())
  // ==========================================================

  // If not called, uses compile-time default from Config.h
  // GeoLinker.setAPN("internet");                    // Simple APN
  // GeoLinker.setAPN("airtelgprs.com", "user", "");  // With auth

  // ==========================================================
  // OPERATING MODE (should be set before begin())
  // ==========================================================

  // GeoLinker.setOperatingMode(MODE_FULL);  // Default: all features
  // Options: MODE_DATA_ONLY, MODE_SMS_ONLY, MODE_CALL_ONLY,
  //          MODE_DATA_SMS, MODE_DATA_CALL, MODE_SMS_CALL, MODE_FULL

  // ==========================================================
  // BASIC CONFIGURATION
  // ==========================================================

  // Set GPS data send interval (5 minutes = 300 seconds)
  GeoLinker.setSendInterval(300);

  // Set GPS fix timeout (2 minutes = 120 seconds)
  GeoLinker.setGPSTimeout(120);

  // Set timezone offset for timestamps (IST = +5:30)
  GeoLinker.setTimeOffset(5, 30);

  // ==========================================================
  // BATTERY CONFIGURATION
  // ==========================================================

  // Set battery voltage source:
  // - BATTERY_SOURCE_ADC: Use ESP32 ADC on GPIO35 (default)
  // - BATTERY_SOURCE_MODEM: Use SIM868 AT+CBC command
  GeoLinker.setBatterySource(BATTERY_SOURCE_ADC);

  // Set battery voltage range (optional - default is 3200mV to 4200mV)
  // For LiPo: 3000mV (empty) to 4200mV (full)
  // For Li-ion: 3200mV (empty) to 4200mV (full)
  GeoLinker.setBatteryRange(3200, 4200);

  // ==========================================================
  // LED CONFIGURATION
  // ==========================================================

  // WS2812B LED power control (via TPS22919DCKR switch)
  GeoLinker.setLEDPower(true);

  // ==========================================================
  // MOTION SENSOR CONFIGURATION
  // ==========================================================

  // Enable motion-triggered sending (sends data when motion detected)
  GeoLinker.enableMotionTrigger(true);

  // Configure motion detection threshold in milligrams
  // Recommended values:
  //   300-500mg: Asset in transit (detect handling)
  //   200-400mg: Parked vehicle (detect towing/theft)
  //   400-800mg: Equipment (detect usage)
  GeoLinker.setMotionThreshold(300.0); // 300mg threshold

  // Enable motion wake from deep sleep
  GeoLinker.enableMotionWake(true);

  // ==========================================================
  // SMS CONTROL CONFIGURATION
  // ==========================================================

  // Enable SMS command processing
  GeoLinker.enableSMS(false);

  // Enable SMS monitoring during sleep (keeps modem in low power mode)
  // This allows wake on incoming SMS
  GeoLinker.enableSMSMonitoring(false);

  // Register custom SMS command handlers
  // Format: command string, callback function
  GeoLinker.registerSMSHandler("ALERT", handleAlertCommand);
  GeoLinker.registerSMSHandler("REBOOT", handleRebootCommand);
  GeoLinker.registerSMSHandler("CONFIG", handleConfigCommand);

  // Built-in SMS commands (always available when SMS enabled):
  // - STATUS: Reply with device status
  // - LOC: Reply with GPS coordinates
  // - SEND: Force immediate data send
  // - SLEEP: Enter sleep mode
  // - INTERVAL=xx: Set send interval (minutes)
  // - WL ADD <num>: Add number to whitelist
  // - WL DEL <num>: Remove from whitelist
  // - WL LIST: List whitelist
  // - WL CLEAR: Clear whitelist

  // ==========================================================
  // CALL CONTROL CONFIGURATION
  // ==========================================================

  // Enable incoming call processing
  GeoLinker.enableCalls(false);

  // Enable call monitoring during sleep (keeps modem in low power mode)
  // This allows wake on incoming call
  GeoLinker.enableCallMonitoring(false);

  // Set default action on incoming call:
  // - CALL_HANGUP: Just hang up (default)
  // - CALL_SEND_GPS: Hang up and send GPS location
  // - CALL_CUSTOM: Only run custom handlers
  GeoLinker.setCallAction(CALL_SEND_GPS);

  // Register custom call handler
  GeoLinker.registerCallHandler(handleIncomingCall);

  // ==========================================================
  // SLEEP & POWER CONFIGURATION
  // ==========================================================

  // Enable/disable full modem power off during sleep
  // When true and sleep > 5min, modem fully powers off
  // When false, modem stays in low power mode (CFUN=0 or CFUN=4)
  GeoLinker.enableFullPowerOff(false); // Keep modem on for SMS/calls

  // Set SIM868 low power mode (when not fully off):
  // - SIM868_CFUN0: Minimum functionality
  // - SIM868_CFUN4: Disable RF (airplane mode)
  GeoLinker.setSIM868SleepMode(SIM868_CFUN0);

  // Set RI pin for call/SMS wake detection
  // Default: GPIO15
  GeoLinker.setRIPin(15);

  // ==========================================================
  // HEARTBEAT CONFIGURATION
  // ==========================================================

  // Enable periodic heartbeat (sends data even without motion)
  GeoLinker.enableHeartbeat(false);

  // Set heartbeat interval (1 hour = 3600 seconds)
  GeoLinker.setHeartbeatInterval(3600);

  // ==========================================================
  // OFFLINE QUEUE CONFIGURATION
  // ==========================================================

  // Configure offline data queue:
  // - QUEUE_RAM: Store in RAM (lost on power off) - fast
  // - QUEUE_LITTLEFS: Store in LittleFS filesystem - persistent
  // - QUEUE_SPIFFS: Store in SPIFFS filesystem - persistent
  // Second parameter: max entries to store
  GeoLinker.setQueueStorage(QUEUE_LITTLEFS, 50);

  // ==========================================================
  // LOGGING CONFIGURATION
  // ==========================================================

  // Set runtime log level:
  // - LOG_OFF: No logging
  // - LOG_ERROR: Only errors
  // - LOG_INFO: Info and errors (default)
  // - LOG_DEBUG: Debug, info, and errors
  // - LOG_VERBOSE: All messages including verbose
  GeoLinker.setLogLevel(LOG_VERBOSE);

  // ==========================================================
  // EVENT CALLBACKS
  // ==========================================================

  // Set state change callback
  GeoLinker.onStateChange(onStateChanged);

  // Set error callback
  GeoLinker.onError(onErrorOccurred);

  // Set data sent callback
  GeoLinker.onDataSent(onDataSentResult);

  Serial.println("Setup complete - entering main loop");
}

// ============================================================================
// Main Loop
// ============================================================================

void loop() {
  // ==========================================================
  // CUSTOM PAYLOAD USING SIMPLE SYNTAX
  // ==========================================================

  // Set payloads using simple initializer list syntax
  // Supports multiple types: float, int, string, bool
  // Each call clears existing payloads and sets new ones
  GeoLinker.setPayloads({
      {"temperature", readExternalTemperature()}, // float
      {"humidity", readExternalHumidity()},       // float
      {"pressure", readExternalPressure()},       // float
      {"count", 42},                              // int
      {"status", "active"}                        // string (max 32 chars)
      // {"enabled", true}                         // bool - uncomment if needed
  });

  // Alternative: Clear payloads
  // GeoLinker.setPayloads({});
  // GeoLinker.clearPayloads();  // Also works

  // Call update() every loop - this handles all operations non-blocking
  GeoLinker.update();

  // ==========================================================
  // STATUS MONITORING (Optional)
  // ==========================================================

  static uint32_t lastStatus = 0;
  if (millis() - lastStatus > 10000) {
    lastStatus = millis();

    Serial.println("------- Status -------");
    Serial.printf("State: %s\n",
                  GL868_ESP32_StateToString(GeoLinker.getState()));
    Serial.printf("Wake source: %s\n",
                  GL868_ESP32_WakeSourceToString(GeoLinker.getWakeSource()));
    Serial.printf("Scheduled wake: %s\n",
                  GeoLinker.isScheduledWake() ? "Yes" : "No");
    Serial.printf("Motion wake: %s\n", GeoLinker.isMotionWake() ? "Yes" : "No");
    Serial.printf("Battery: %d%% (%dmV)\n", GeoLinker.getBatteryPercent(),
                  GeoLinker.getBatteryVoltageMV());
    Serial.printf("Signal: %d\n", GeoLinker.getSignalStrength());
    Serial.printf("Satellites: %d\n", GeoLinker.getSatelliteCount());
    Serial.printf("Queue: %d entries\n", GeoLinker.queue.count());
    Serial.printf("Operator: %s\n", GeoLinker.getOperator().c_str());

    // Access last GPS data
    const GPSData &gps = GeoLinker.getLastGPS();
    if (gps.valid) {
      Serial.printf("GPS: %.6f, %.6f\n", gps.latitude, gps.longitude);
      Serial.printf("Time: %s\n", gps.timestamp);
    }
    Serial.println("----------------------");
  }
}

// ============================================================================
// CONFIGURATION REFERENCE
// ============================================================================
/*

ALL AVAILABLE CONFIGURATION METHODS:
====================================

BASIC:
- begin(deviceId, apiKey)         - Initialize library
- update()                        - Call in loop()
- setSendInterval(seconds)        - Set data send interval (default: 300)
- setGPSTimeout(seconds)          - Set GPS fix timeout (default: 180)
- setTimeOffset(hours, minutes)   - Set timezone offset (e.g., 5, 30 for IST)

BATTERY:
- setBatterySource(source)        - Options: BATTERY_SOURCE_ADC (default)
                                            BATTERY_SOURCE_MODEM (AT+CBC)
- setBatteryRange(minMV, maxMV)   - Optional, default 3200-4200mV

PAYLOAD (Multi-Type Support):
- setPayloads({...})              - Set payloads with initializer list
                                    Supports: float, int, string, bool
                                    Max 5 fields, 16 char keys, 32 char strings
- setPayloadFields(array, count)  - Legacy method
- clearPayloads()                 - Clear all payloads

LED:
- setLEDPower(bool)               - WS2812B power control (via TPS22919)

APN:
- setAPN(apn)                     - Set APN (before or after begin())
- setAPN(apn, user, pass)         - APN with authentication
- getAPN()                        - Get current APN string

OPERATING MODES (before begin()):
- setOperatingMode(mode)          - Options: MODE_DATA_ONLY (0x01)
                                            MODE_SMS_ONLY  (0x02)
                                            MODE_CALL_ONLY (0x04)
                                            MODE_DATA_SMS  (0x03)
                                            MODE_DATA_CALL (0x05)
                                            MODE_SMS_CALL  (0x06)
                                            MODE_FULL      (0x07, default)
- getOperatingMode()              - Returns: uint8_t mode flags
- isModeEnabled(flag)             - Check MODE_DATA/MODE_SMS/MODE_CALL

AT COMMANDS (direct modem access):
- sendATCommand(cmd, timeout)     - Send AT command, check for OK
- sendATCommand(cmd, buf, len)    - Send and capture response
- getATResponse()                 - Get last AT response
- getModemSerial()                - Direct serial access (advanced)

ON-DEMAND GPS (all modes):
- gpsOn()                         - Power on GPS manually
- gpsOff()                        - Power off GPS
- isGpsPowered()                  - Check GPS power state
- getLocation(&data, timeout)     - Blocking GPS read (powers on/off)
- getLocationNow(&data)           - Non-blocking read (GPS must be on)

HTTP POST (custom endpoints):
- httpPost(url, body)             - POST to any URL
- httpPost(url, body, headers)    - POST with custom headers
- httpPost(url, body, hdrs, resp, len) - POST with response capture
                                    Returns: HTTP status or -1 on error

USER SMS/CALL:
- sendSMS(number, message)        - Send SMS (max 160 chars)
- makeCall(number, timeout)       - Make outgoing call
- answerCall()                    - Answer incoming call
- hangupCall()                    - Hang up
- isCallActive()                  - Check if call in progress

SMS:
- enableSMS(bool)                 - Enable/disable SMS processing
- enableSMSMonitoring(bool)       - Enable SMS wake from sleep
- registerSMSHandler(cmd, fn)     - Register custom SMS command
                                    Built-in: STATUS, LOC, SEND, SLEEP,
                                              INTERVAL=xx, WL ADD/DEL/LIST/CLEAR

CALLS:
- enableCalls(bool)               - Enable/disable call processing
- enableCallMonitoring(bool)      - Enable call wake from sleep
- setCallAction(action)           - Options: CALL_HANGUP  (just hang up)
                                            CALL_SEND_GPS (send GPS, default)
                                            CALL_CUSTOM   (only custom handlers)
- registerCallHandler(fn)         - Register custom call handler

MOTION (LIS3DHTR Accelerometer):
- enableMotionTrigger(bool)       - Enable motion-triggered sending
- setMotionThreshold(mg)          - Threshold in milligrams (16-2032)
                                    Recommended: 300-500mg transit,
                                    200-400mg parked, 400-800mg equipment
- setMotionSensitivity(value)     - Raw register: 0-127 (lower = more sensitive)
- enableMotionWake(bool)          - Enable motion wake from sleep

POWER:
- enableFullPowerOff(bool)        - Full modem power off in sleep (default:
true)
- setSIM868SleepMode(mode)        - Options: SIM868_FULL_OFF (default)
                                            SIM868_CFUN0 (minimal function)
                                            SIM868_CFUN4 (RF disabled)
- setRIPin(pin)                   - Set RI pin for wake (default: 15)

HEARTBEAT:
- enableHeartbeat(bool)           - Enable periodic heartbeat (default: false)
- setHeartbeatInterval(seconds)   - Set heartbeat interval (default: 3600)

QUEUE:
- setQueueStorage(type, max)      - Options: QUEUE_RAM      (max 10, default)
                                            QUEUE_LITTLEFS (max 100)
                                            QUEUE_SPIFFS   (max 100)

LOGGING:
- setLogLevel(level)              - Options: LOG_OFF
                                            LOG_ERROR
                                            LOG_INFO (default)
                                            LOG_DEBUG
                                            LOG_VERBOSE

STATUS:
- getState()                      - Returns: SystemState enum
                                    States: STATE_BOOT, STATE_MODEM_POWER_ON,
                                            STATE_GSM_INIT, STATE_GSM_REGISTER,
                                             STATE_GPS_POWER_ON,
                                             STATE_GPS_WAIT_FIX,
                                             STATE_BUILD_JSON,
                                             STATE_GPRS_ATTACH,
                                             STATE_SEND_HTTP,
                                             STATE_SEND_OFFLINE_QUEUE,
                                             STATE_SLEEP_PREPARE,
                                             STATE_SLEEP,
                                             STATE_IDLE
- getWakeSource()                 - Returns: WakeSource enum
                                    Sources: WAKE_UNKNOWN, WAKE_TIMER,
                                             WAKE_MOTION, WAKE_CALL, WAKE_SMS
- isScheduledWake()               - Returns: bool (true if timer wake)
- isMotionWake()                  - Returns: bool (true if motion wake)
- getBatteryPercent()             - Returns: uint8_t (0-100%)
- getBatteryVoltageMV()           - Returns: uint16_t (mV)
- getSignalStrength()             - Returns: int (GSM signal 0-31, 99=unknown)
                                    Quality: 0-9 Poor, 10-14 Fair,
                                             15-19 Good, 20-31 Excellent
- getSatelliteCount()             - Returns: uint8_t (GPS satellites in view)
- getOperator()                   - Returns: String (network operator name)
- getLastGPS()                    - Returns: const GPSData& struct
                                    Fields: valid, latitude, longitude,
                                            altitude, speed, heading,
                                            accuracy, satellites, timestamp

SIM/MODEM INFO:
- getIMEI()                       - Returns: String (15 digit modem IMEI)
- getIMSI()                       - Returns: String (SIM subscriber ID)
- getICCID()                      - Returns: String (19 digits, no Luhn
checksum)
- getPhoneNumber()                - Returns: String (phone if stored, else "")

CONTROL:
- forceSend()                     - Force immediate data send
- forceSleep()                    - Force immediate deep sleep

CALLBACKS:
- onStateChange(fn)               - Signature: void fn(SystemState from,
SystemState to)
- onError(fn)                     - Signature: void fn(ErrorCode code)
                                     Errors: ERROR_NONE, ERROR_NO_SIM,
                                             ERROR_NO_NETWORK, ERROR_NO_GPRS,
                                             ERROR_NO_GPS, ERROR_HTTP_FAIL,
                                             ERROR_MODEM, ERROR_MOTION_SENSOR,
                                             ERROR_QUEUE_FULL, ERROR_JSON_BUILD,
                                             ERROR_TIMEOUT
- onDataSent(fn)                  - Signature: void fn(bool success, int
httpCode)

*/
