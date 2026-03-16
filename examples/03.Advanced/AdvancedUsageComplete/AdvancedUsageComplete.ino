/*
 * AdvancedUsageComplete.ino
 * GL868_ESP32 Library - Advanced Usage with All Options
 *
 * This example demonstrates ALL available library features with detailed
 * comments explaining each option and its purpose.
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#include <GL868_ESP32.h>

// ============================================================================
// SECTION 1: Basic Configuration
// ============================================================================

// Device identification
#define DEVICE_ID "YOUR_DEVICE_ID" // Unique ID for this device
#define API_KEY "YOUR_API_KEY"     // API key from GeoLinker cloud

// Phone number for SMS/Call features
#define PHONE_NUMBER "+91XXXXXXXXXX"

// ============================================================================
// SECTION 2: Callback Functions (Optional)
// ============================================================================

// Custom SMS command handler
void onCustomSMS(const char *sender, const char *command, const char *args) {
  Serial.printf("[SMS] From: %s, Cmd: %s, Args: %s\n", sender, command, args);
  GeoLinker.sendSMS(sender, "Command received!");
}

// Called when an incoming call is detected
void onCallReceived(const char *callerNumber) {
  Serial.printf("[CALL] Incoming from: %s\n", callerNumber);
}

// ============================================================================
// SECTION 3: Setup with All Configuration Options
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n================================================");
  Serial.println("   GL868_ESP32 Advanced Usage - All Options");
  Serial.println("================================================\n");

  // --------------------------------------------------------------------------
  // 3.1 Operating Mode (BEFORE begin())
  // --------------------------------------------------------------------------

  // MODE_DATA_ONLY  - GPS tracking + HTTP only
  // MODE_SMS_ONLY   - SMS commands only (no GPS/HTTP)
  // MODE_CALL_ONLY  - Call handling only (no GPS/HTTP)
  // MODE_DATA_SMS   - Data + SMS
  // MODE_DATA_CALL  - Data + Call
  // MODE_SMS_CALL   - SMS + Call (no GPS/HTTP)
  // MODE_FULL       - All features (default)
  GeoLinker.setOperatingMode(MODE_FULL);

  // --------------------------------------------------------------------------
  // 3.2 APN Configuration (BEFORE begin())
  // --------------------------------------------------------------------------

  // If not called, uses compile-time default from Config.h ("iot.com")
  // GeoLinker.setAPN("internet");                            // Simple APN
  // GeoLinker.setAPN("airtelgprs.com", "user", "password");  // With auth

  // --------------------------------------------------------------------------
  // 3.3 Logging Configuration (before begin())
  // --------------------------------------------------------------------------

  // LOG_OFF, LOG_ERROR, LOG_INFO (default), LOG_DEBUG, LOG_VERBOSE
  GeoLinker.setLogLevel(LOG_INFO);

  // --------------------------------------------------------------------------
  // 3.4 Initialize the Library
  // --------------------------------------------------------------------------

  GeoLinker.begin(DEVICE_ID, API_KEY);

  // --------------------------------------------------------------------------
  // 3.5 Data Transmission Configuration
  // --------------------------------------------------------------------------

  // Send interval in seconds (default: 300 = 5 minutes)
  GeoLinker.setSendInterval(300);

  // GPS fix timeout in seconds (default: 120)
  GeoLinker.setGPSTimeout(120);

  // Timezone offset for timestamps (IST = +5:30)
  GeoLinker.setTimeOffset(5, 30);

  // --------------------------------------------------------------------------
  // 3.6 Battery Configuration
  // --------------------------------------------------------------------------

  // Source: BATTERY_SOURCE_ADC (default) or BATTERY_SOURCE_MODEM
  GeoLinker.setBatterySource(BATTERY_SOURCE_ADC);

  // Voltage range (default: 3200mV-4200mV)
  GeoLinker.setBatteryRange(3200, 4200);

  // --------------------------------------------------------------------------
  // 3.7 Motion Sensor (LIS3DHTR)
  // --------------------------------------------------------------------------

  // Motion-triggered data send
  GeoLinker.enableMotionTrigger(true);

  // Motion threshold in milligrams (16-2032mg)
  // Recommended: 300-500mg transit, 200-400mg parked, 400-800mg equipment
  GeoLinker.setMotionThreshold(300.0); // 300mg threshold

  // Wake from deep sleep on motion
  GeoLinker.enableMotionWake(true);

  // --------------------------------------------------------------------------
  // 3.8 LED Configuration
  // --------------------------------------------------------------------------

  // WS2812B power control (via TPS22919 switch)
  GeoLinker.setLEDPower(true);

  // --------------------------------------------------------------------------
  // 3.9 SMS Configuration
  // --------------------------------------------------------------------------

  // Enable SMS command processing
  GeoLinker.enableSMS(true);

  // Enable SMS monitoring during sleep (keeps modem in low power)
  GeoLinker.enableSMSMonitoring(true);

  // Register custom SMS command handler
  GeoLinker.registerSMSHandler("MYCMD", onCustomSMS);

  // Built-in commands: STATUS, LOC, SEND, SLEEP, INTERVAL=xx, WL
  // ADD/DEL/LIST/CLEAR

  // --------------------------------------------------------------------------
  // 3.10 Call Configuration
  // --------------------------------------------------------------------------

  // Enable call processing
  GeoLinker.enableCalls(true);

  // Enable call monitoring during sleep
  GeoLinker.enableCallMonitoring(true);

  // Default action: CALL_HANGUP, CALL_SEND_GPS (default), CALL_CUSTOM
  GeoLinker.setCallAction(CALL_SEND_GPS);

  // Custom call handler
  GeoLinker.registerCallHandler(onCallReceived);

  // --------------------------------------------------------------------------
  // 3.11 Whitelist (via SMS or directly)
  // --------------------------------------------------------------------------

  // Programmatic access (first number added = admin)
  // GeoLinker.whitelist.add("+91XXXXXXXXXX");
  // GeoLinker.whitelist.add("+91YYYYYYYYYY");
  // Also manageable via SMS: WL ADD/DEL/LIST/CLEAR

  // --------------------------------------------------------------------------
  // 3.12 Sleep & Power Management
  // --------------------------------------------------------------------------

  // Full modem power off during sleep (default: true)
  GeoLinker.enableFullPowerOff(false); // Keep modem on for SMS/Call wake

  // SIM868 low-power mode: SIM868_CFUN0 or SIM868_CFUN4
  GeoLinker.setSIM868SleepMode(SIM868_CFUN0);

  // RI pin for SMS/Call wake detection (default: GPIO 16)
  GeoLinker.setRIPin(16);

  // --------------------------------------------------------------------------
  // 3.13 Heartbeat
  // --------------------------------------------------------------------------

  // Periodic alive signal (default: disabled)
  GeoLinker.enableHeartbeat(true);
  GeoLinker.setHeartbeatInterval(3600); // Every 1 hour

  // --------------------------------------------------------------------------
  // 3.14 Offline Queue
  // --------------------------------------------------------------------------

  // QUEUE_RAM (default, 10 entries), QUEUE_LITTLEFS (persistent), QUEUE_SPIFFS
  GeoLinker.setQueueStorage(QUEUE_LITTLEFS, 50);

  // --------------------------------------------------------------------------
  // 3.15 Custom Payload Fields
  // --------------------------------------------------------------------------

  // Multi-type support: float, int, string, bool (max 5 fields)
  GeoLinker.setPayloads({{"firmware_version", "1.0.0"},
                         {"sensor_temp", 25.5},
                         {"relay_state", true},
                         {"error_count", 0}});

  // --------------------------------------------------------------------------
  // 3.16 Event Callbacks
  // --------------------------------------------------------------------------

  GeoLinker.onStateChange([](SystemState from, SystemState to) {
    Serial.printf("State: %s -> %s\n", GL868_ESP32_StateToString(from),
                  GL868_ESP32_StateToString(to));
  });

  GeoLinker.onError([](ErrorCode code) {
    Serial.printf("Error: %s\n", GL868_ESP32_ErrorToString(code));
  });

  GeoLinker.onDataSent([](bool success, int httpCode) {
    Serial.printf("Data %s (HTTP %d)\n", success ? "OK" : "FAIL", httpCode);
  });

  Serial.println("Setup complete! Starting main loop...\n");
}

// ============================================================================
// SECTION 4: Main Loop
// ============================================================================

void loop() {
  // --------------------------------------------------------------------------
  // 4.1 Main Update - REQUIRED in every loop()
  // --------------------------------------------------------------------------

  GeoLinker.update();

  // --------------------------------------------------------------------------
  // 4.2 On-Demand GPS (available in all modes)
  // --------------------------------------------------------------------------

  // Blocking read: powers on GPS, waits for fix, powers off
  /*
  GPSData location;
  if (GeoLinker.getLocation(&location, 90000)) {  // 90 sec timeout
    Serial.printf("Lat: %.6f, Lon: %.6f\n", location.latitude,
  location.longitude); Serial.printf("Speed: %.1f km/h, Alt: %.1f m\n",
  location.speed, location.altitude); Serial.printf("Sats: %d, Time: %s\n",
  location.satellites, location.timestamp);
  }
  */

  // Non-blocking read (GPS must be powered on first with gpsOn())
  /*
  GeoLinker.gpsOn();
  GPSData gps;
  if (GeoLinker.getLocationNow(&gps) && gps.valid) {
    Serial.printf("Location: %.6f, %.6f\n", gps.latitude, gps.longitude);
  }
  GeoLinker.gpsOff();
  */

  // --------------------------------------------------------------------------
  // 4.3 HTTP POST to Custom Server
  // --------------------------------------------------------------------------

  // Simple POST
  // int status = GeoLinker.httpPost("http://example.com/api",
  //     "{\"key\":\"value\"}", "Content-Type: application/json");

  // POST with response capture
  // char response[512];
  // int status = GeoLinker.httpPost("http://example.com/api",
  //     "{\"key\":\"value\"}", "Content-Type: application/json",
  //     response, sizeof(response));

  // --------------------------------------------------------------------------
  // 4.4 Direct AT Commands (advanced)
  // --------------------------------------------------------------------------

  // GeoLinker.sendATCommand("AT+CSQ");
  // char resp[128];
  // GeoLinker.sendATCommand("AT+COPS?", resp, sizeof(resp));

  // --------------------------------------------------------------------------
  // 4.5 SMS & Call Functions
  // --------------------------------------------------------------------------

  // GeoLinker.sendSMS("+91XXXXXXXXXX", "Hello from tracker!");
  // GeoLinker.makeCall("+91XXXXXXXXXX", 30);  // 30 sec timeout
  // GeoLinker.hangupCall();
  // GeoLinker.answerCall();
  // bool active = GeoLinker.isCallActive();

  // --------------------------------------------------------------------------
  // 4.6 Manual Control
  // --------------------------------------------------------------------------

  // GeoLinker.forceSend();   // Force immediate data send
  // GeoLinker.forceSleep();  // Force immediate sleep

  // --------------------------------------------------------------------------
  // 4.7 Status Monitoring
  // --------------------------------------------------------------------------

  static uint32_t lastStatus = 0;
  if (millis() - lastStatus >= 30000) { // Every 30 seconds
    lastStatus = millis();
    printDeviceStatus();
  }
}

// ============================================================================
// SECTION 5: Helper Functions
// ============================================================================

void printDeviceStatus() {
  Serial.println("\n--- Device Status ---");

  // Battery
  Serial.printf("Battery: %d%% (%dmV)\n", GeoLinker.getBatteryPercent(),
                GeoLinker.getBatteryVoltageMV());

  // Network
  Serial.printf("Signal: %d/31\n", GeoLinker.getSignalStrength());
  Serial.printf("Operator: %s\n", GeoLinker.getOperator().c_str());

  // SIM/Modem info
  Serial.printf("IMEI: %s\n", GeoLinker.getIMEI().c_str());

  // GPS
  const GPSData &gps = GeoLinker.getLastGPS();
  if (gps.valid) {
    Serial.printf("GPS: %.6f, %.6f (Sats: %d)\n", gps.latitude, gps.longitude,
                  gps.satellites);
  } else {
    Serial.println("GPS: No fix");
  }

  // State
  Serial.printf("State: %s\n", GL868_ESP32_StateToString(GeoLinker.getState()));
  Serial.printf("Mode: 0x%02X\n", GeoLinker.getOperatingMode());

  Serial.println("---------------------\n");
}

// ============================================================================
// CONFIGURATION QUICK REFERENCE
// ============================================================================
/*

=== Operating Modes (call BEFORE begin()) ===
MODE_DATA_ONLY  (0x01) - GPS + HTTP only
MODE_SMS_ONLY   (0x02) - SMS only
MODE_CALL_ONLY  (0x04) - Call only
MODE_DATA_SMS   (0x03) - Data + SMS
MODE_DATA_CALL  (0x05) - Data + Call
MODE_SMS_CALL   (0x06) - SMS + Call
MODE_FULL       (0x07) - All features (default)

=== Log Levels ===
LOG_OFF     - No output
LOG_ERROR   - Errors only
LOG_INFO    - Default
LOG_DEBUG   - Detailed
LOG_VERBOSE - Everything

=== Call Actions ===
CALL_HANGUP   - Hang up immediately
CALL_SEND_GPS - Trigger GPS send (default)
CALL_CUSTOM   - Only custom handlers

=== Battery Sources ===
BATTERY_SOURCE_ADC   - ESP32 ADC (default)
BATTERY_SOURCE_MODEM - SIM868 AT+CBC

=== Motion Threshold (milligrams) ===
300-500mg - Asset in transit (detect handling)
200-400mg - Parked vehicle (detect towing/theft)
400-800mg - Equipment (detect usage)
Raw register: 0-127 (lower = more sensitive)

=== Offline Queue ===
QUEUE_RAM      - In memory (default, max 10)
QUEUE_LITTLEFS - Filesystem (persistent, max 100)
QUEUE_SPIFFS   - Legacy filesystem (max 100)

=== SIM868 Sleep Modes ===
SIM868_CFUN0 - Minimum functionality
SIM868_CFUN4 - RF disabled (airplane mode)

=== HTTP Status Codes ===
200     - Success
-1      - Connection error
400-499 - Client error
500-599 - Server error

*/
