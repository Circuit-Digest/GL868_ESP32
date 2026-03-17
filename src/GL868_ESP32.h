/*
 * GL868_ESP32.h
 * Main library header for GL868_ESP32 GPS Tracker Library
 *
 * A comprehensive GPS tracking library for ESP32 with SIM868 modem,
 * featuring non-blocking operation, power management, SMS/Call control,
 * motion detection, and offline data queuing.
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_H
#define GL868_ESP32_H

#include <Arduino.h>
#include <initializer_list>

// Include all component headers
#include "GL868_ESP32_Battery.h"
#include "GL868_ESP32_Call.h"
#include "GL868_ESP32_Config.h"
#include "GL868_ESP32_GPS.h"
#include "GL868_ESP32_GSM.h"
#include "GL868_ESP32_JSON.h"
#include "GL868_ESP32_LED.h"
#include "GL868_ESP32_Logger.h"
#include "GL868_ESP32_Modem.h"
#include "GL868_ESP32_Motion.h"
#include "GL868_ESP32_Queue.h"
#include "GL868_ESP32_SMS.h"
#include "GL868_ESP32_Sleep.h"
#include "GL868_ESP32_StateMachine.h"
#include "GL868_ESP32_Types.h"
#include "GL868_ESP32_Whitelist.h"

/**
 * Main GL868_ESP32 Library Class
 *
 * This is the primary interface for the GPS tracker library.
 * Call begin() in setup() and update() in loop().
 */
class GL868_ESP32 {
public:
  GL868_ESP32();

  // ========================================================================
  // Initialization
  // ========================================================================

  /**
   * Initialize the library
   * @param deviceId Device identifier string
   * @param apiKey API key for server authentication
   */
  void begin(const char *deviceId, const char *apiKey);

  /**
   * Main update loop - call this in loop()
   * Handles all non-blocking state machine operations
   */
  void update();

  // ========================================================================
  // Configuration
  // ========================================================================

  /**
   * Set GPS data send interval
   * @param seconds Interval in seconds
   */
  void setSendInterval(uint32_t seconds);

  /**
   * Set GPS fix timeout
   * @param seconds Timeout in seconds
   */
  void setGPSTimeout(uint32_t seconds);

  /**
   * Set timeout in seconds before the ESP32 reboots when encountering
   * a critical network error (NO SIM, NO NETWORK).
   * Setting to 0 disables auto-reboot. Default: 300 (5 minutes).
   */
  void setErrorRebootTimeout(uint32_t seconds);

  /**
   * Set timezone offset for timestamps
   * @param hours Hour offset (-12 to +14)
   * @param minutes Minute offset (0, 15, 30, 45)
   */
  void setTimeOffset(int8_t hours, int8_t minutes);

  /**
   * Set battery voltage range for percentage calculation
   * Optional - defaults to 3200mV (0%) to 4200mV (100%)
   * @param minMV Minimum voltage (0%) in millivolts
   * @param maxMV Maximum voltage (100%) in millivolts
   */
  void setBatteryRange(uint16_t minMV, uint16_t maxMV);

  /**
   * Set battery voltage source
   * @param source BATTERY_SOURCE_ADC (default) or BATTERY_SOURCE_MODEM
   */
  void setBatterySource(BatterySource source);

  /**
   * Enable/disable WS2812B LED power (via TPS22919DCKR switch)
   * @param enabled true = power on, false = power off
   */
  void setLEDPower(bool enabled);

  /**
   * Set the maximum brightness of the WS2812B RGB LED
   * @param percentage Brightness level from 0 to 100 (default is 30)
   */
  void setLEDBrightness(uint8_t percentage);

  /**
   * Set operating mode (DATA, SMS, CALL combinations)
   * Call BEFORE begin() to configure which features are enabled
   * @param mode Operating mode (MODE_DATA_ONLY, MODE_SMS_ONLY, MODE_FULL, etc.)
   */
  void setOperatingMode(OperatingMode mode);

  /**
   * Set operating mode using custom flag combination
   * @param modeFlags Bitwise OR of MODE_DATA, MODE_SMS, MODE_CALL
   */
  void setOperatingMode(uint8_t modeFlags);

  /**
   * Get current operating mode
   */
  uint8_t getOperatingMode() const;

  /**
   * Check if a specific mode flag is enabled
   * @param flag MODE_DATA, MODE_SMS, or MODE_CALL
   */
  bool isModeEnabled(OperatingModeFlags flag) const;

  // ========================================================================
  // APN Configuration
  // ========================================================================

  /**
   * Set APN for GPRS connection (call BEFORE begin())
   * If not called, uses compile-time default from GL868_ESP32_Config.h
   * @param apn APN name (e.g., "internet", "iot.com")
   */
  void setAPN(const char *apn);

  /**
   * Set APN with authentication (for carriers requiring username/password)
   * @param apn APN name
   * @param user Username
   * @param pass Password
   */
  void setAPN(const char *apn, const char *user, const char *pass);

  /**
   * Get current APN
   */
  const char *getAPN() const;

  // ========================================================================
  // Direct AT Command Access
  // ========================================================================

  /**
   * Send AT command and wait for OK
   * @param cmd Command string (with or without "AT" prefix)
   * @param timeout Response timeout in ms (default 3000)
   * @return true if OK received
   */
  bool sendATCommand(const char *cmd, uint32_t timeout = 3000);

  /**
   * Send AT command and get response into buffer
   * @param cmd Command string
   * @param response Buffer for response
   * @param maxLen Buffer size
   * @param timeout Response timeout in ms
   * @return true if successful
   */
  bool sendATCommand(const char *cmd, char *response, size_t maxLen,
                     uint32_t timeout = 3000);

  /**
   * Get last AT command response
   */
  String getATResponse();

  /**
   * Get direct access to modem serial for raw AT communication
   * Use with caution - bypasses library state management
   */
  HardwareSerial &getModemSerial();

  // ========================================================================
  // On-Demand GPS Access (available in all operating modes)
  // ========================================================================

  /**
   * Power on GPS module manually
   * @return true if successful
   */
  bool gpsOn();

  /**
   * Power off GPS module manually
   */
  bool gpsOff();

  /**
   * Check if GPS is currently powered on
   */
  bool isGpsPowered();

  /**
   * Get current GPS location (blocking, waits for fix)
   * @param data Pointer to GPSData structure to fill
   * @param timeout Maximum wait time in ms (default 90 seconds)
   * @return true if valid fix obtained
   */
  bool getLocation(GPSData *data, uint32_t timeout = 90000);

  /**
   * Get GPS reading (non-blocking, returns immediately)
   * GPS must be powered on first with gpsOn()
   * @param data Pointer to GPSData structure to fill
   * @return true if data is valid
   */
  bool getLocationNow(GPSData *data);

  // ========================================================================
  // User-Callable HTTP Functions
  // ========================================================================

  /**
   * Send HTTP POST request to any URL with custom headers
   * @param url Full URL (e.g., "http://httpbin.org/post")
   * @param body Request body (any format: JSON, form-data, raw text)
   * @param headers Custom headers as key:value pairs separated by \r\n
   *                Example: "Content-Type: application/json\r\nAuthorization:
   * Bearer token" If nullptr, only Host and Content-Length are sent
   * @return HTTP status code (200, 404, etc.) or -1 on connection error
   */
  int httpPost(const char *url, const char *body,
               const char *headers = nullptr);

  /**
   * Send HTTP POST and capture server response body
   * @param url Full URL
   * @param body Request body
   * @param headers Custom headers (nullptr for defaults)
   * @param response Buffer to store server response body
   * @param responseMaxLen Maximum length of response buffer
   * @return HTTP status code or -1 on error
   */
  int httpPost(const char *url, const char *body, const char *headers,
               char *response, size_t responseMaxLen);

  // ========================================================================
  // User-Callable SMS Functions
  // ========================================================================

  /**
   * Send an SMS message
   * @param number Recipient phone number
   * @param message Message text (max 160 chars for single SMS)
   * @return true if sent successfully
   */
  bool sendSMS(const char *number, const char *message);

  // ========================================================================
  // User-Callable Call Functions
  // ========================================================================

  /**
   * Make an outgoing voice call
   * @param number Phone number to call
   * @param timeout Seconds to wait for connection (0 = don't wait)
   * @return true if call connected/initiated
   */
  bool makeCall(const char *number, uint8_t timeout = 30);

  /**
   * Answer an incoming call
   * @return true if answered
   */
  bool answerCall();

  /**
   * Hang up current call
   */
  void hangupCall();

  /**
   * Check if a call is in progress
   */
  bool isCallActive();

  // ========================================================================
  // Payload
  // ========================================================================

  /**
   * Set custom payload fields using initializer list
   * Example: setPayloads({{"temperature", 27.2}, {"humidity", 53.1}})
   * Clears existing payloads before adding new ones
   * Call with empty {} to clear all payloads
   * @param payloads Initializer list of PayloadField
   */
  void setPayloads(std::initializer_list<PayloadField> payloads);

  /**
   * Set custom payload fields (legacy method)
   * @param fields Array of PayloadField structures
   * @param count Number of fields (max 5)
   */
  void setPayloadFields(PayloadField *fields, uint8_t count);

  /**
   * Clear all custom payload fields
   */
  void clearPayloads();

  // ========================================================================
  // SMS/Call Control
  // ========================================================================

  /**
   * Enable/disable SMS command processing
   * @param enabled Enable SMS
   */
  void enableSMS(bool enabled);

  /**
   * Enable/disable incoming call processing
   * @param enabled Enable calls
   */
  void enableCalls(bool enabled);

  /**
   * Enable/disable SMS monitoring during sleep (keeps modem in low power)
   * @param enabled Enable SMS monitoring
   */
  void enableSMSMonitoring(bool enabled);

  /**
   * Enable/disable call monitoring during sleep (keeps modem in low power)
   * @param enabled Enable call monitoring
   */
  void enableCallMonitoring(bool enabled);

  /**
   * Register custom SMS command handler
   * @param command Command string (e.g., "ALERT")
   * @param handler Callback function
   */
  void registerSMSHandler(const char *command, SMSHandler handler);

  /**
   * Register custom call handler
   * @param handler Callback function
   */
  void registerCallHandler(CallHandler handler);

  /**
   * Set default action on incoming call
   * @param action CALL_HANGUP, CALL_SEND_GPS, or CALL_CUSTOM
   */
  void setCallAction(CallAction action);

  // ========================================================================
  // Motion Sensor
  // ========================================================================

  /**
   * Enable/disable motion-triggered sending
   * @param enabled Enable motion trigger
   */
  void enableMotionTrigger(bool enabled);

  /**
   * Set motion detection sensitivity (raw register value)
   * @param threshold Sensitivity (0-127, higher = less sensitive, default 40)
   */
  void setMotionSensitivity(uint8_t threshold);

  /**
   * Set motion detection threshold in milligrams
   * Recommended values:
   *   300-500mg: Asset in transit (detect handling)
   *   200-400mg: Parked vehicle (detect towing/theft)
   *   400-800mg: Equipment (detect usage)
   * @param mg Threshold in milligrams (16-2032)
   */
  void setMotionThreshold(float mg);

  // ========================================================================
  // Sleep & Power
  // ========================================================================

  /**
   * Enable/disable full modem power off during sleep
   * When enabled and sleep interval > 5min, modem fully powers off
   * @param enabled Enable full power off (default: true)
   */
  void enableFullPowerOff(bool enabled);

  /**
   * Set SIM868 sleep mode when not fully powered off
   * @param mode SIM868_CFUN0 or SIM868_CFUN4
   */
  void setSIM868SleepMode(SIM868SleepMode mode);

  /**
   * Enable/disable motion wake from deep sleep
   * @param enabled Enable motion wake (default: false)
   */
  void enableMotionWake(bool enabled);

  /**
   * Set RI pin for call/SMS wake detection
   * @param pin GPIO pin number (default: 15)
   */
  void setRIPin(uint8_t pin);

  // ========================================================================
  // Heartbeat
  // ========================================================================

  /**
   * Enable/disable heartbeat (periodic alive signal)
   * @param enabled Enable heartbeat (default: false)
   */
  void enableHeartbeat(bool enabled);

  /**
   * Set heartbeat interval
   * @param seconds Interval in seconds (default: 3600 = 1 hour)
   */
  void setHeartbeatInterval(uint32_t seconds);

  // ========================================================================
  // Offline Queue
  // ========================================================================

  /**
   * Configure offline data queue
   * @param type QUEUE_RAM (default), QUEUE_LITTLEFS, or QUEUE_SPIFFS
   * @param maxEntries Maximum entries to store (default: 10 for RAM)
   */
  void setQueueStorage(QueueStorage type, uint8_t maxEntries);

  // ========================================================================
  // Logging
  // ========================================================================

  /**
   * Set runtime log level
   * @param level LOG_OFF, LOG_ERROR, LOG_INFO, LOG_DEBUG, or LOG_VERBOSE
   */
  void setLogLevel(LogLevel level);

  // ========================================================================
  // Status
  // ========================================================================

  /**
   * Get current system state
   */
  SystemState getState() const;

  /**
   * Get wake source (after deep sleep)
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
   * Get current battery percentage
   */
  uint8_t getBatteryPercent();

  /**
   * Get current battery voltage in mV
   */
  uint16_t getBatteryVoltageMV();

  /**
   * Get GSM signal strength (0-31)
   */
  int getSignalStrength();

  /**
   * Get GPS satellite count from last fix
   */
  uint8_t getSatelliteCount() const;

  /**
   * Get operator name
   */
  String getOperator();

  /**
   * Get modem IMEI (15 digits)
   * @return IMEI string or empty on error
   */
  String getIMEI();

  /**
   * Get SIM IMSI (International Mobile Subscriber Identity)
   * @return IMSI string or empty on error
   */
  String getIMSI();

  /**
   * Get SIM ICCID (19-20 digits, may include Luhn checksum digit)
   * @return ICCID string or empty on error
   */
  String getICCID();

  /**
   * Get phone number from SIM (if available)
   * Note: Not all SIMs store phone number
   * @return Phone number or empty if not available
   */
  String getPhoneNumber();

  /**
   * Get last GPS data
   */
  const GPSData &getLastGPS() const { return _currentGPS; }

  // ========================================================================
  // Manual Control
  // ========================================================================

  /**
   * Force immediate data send
   */
  void forceSend();

  /**
   * Force immediate sleep
   */
  void forceSleep();

  // ========================================================================
  // User Status LED Control
  // ========================================================================

  /**
   * Turn the user status LED on (GPIO GL868_ESP32_LED_STATUS).
   * This LED is fully user-controlled; the library never drives it after
   * begin().
   */
  void turnOnLED() { led.turnOnLED(); }

  /**
   * Turn the user status LED off (GPIO GL868_ESP32_LED_STATUS).
   */
  void turnOffLED() { led.turnOffLED(); }

  // ========================================================================
  // Callbacks
  // ========================================================================

  /**
   * Set state change callback
   */
  void onStateChange(StateChangeCallback callback);

  /**
   * Set error callback
   */
  void onError(ErrorCallback callback);

  /**
   * Set data sent callback
   */
  void onDataSent(DataSentCallback callback);

  // ========================================================================
  // Component Access (for advanced use)
  // ========================================================================

  GL868_ESP32_LED led;
  GL868_ESP32_Battery battery;
  GL868_ESP32_Motion motion;
  GL868_ESP32_Modem modem;
  GL868_ESP32_GSM gsm;
  GL868_ESP32_GPS gps;
  GL868_ESP32_JSON json;
  GL868_ESP32_Queue queue;
  GL868_ESP32_SMS sms;
  GL868_ESP32_Call call;
  GL868_ESP32_Whitelist whitelist;
  GL868_ESP32_StateMachine stateMachine;
  GL868_ESP32_Sleep sleep;

private:
  // State handlers
  void handleBoot();
  void handleModemPowerOn();
  void handleGSMInit();
  void handleGSMRegister();
  void handleGPSPowerOn();
  void handleGPSWaitFix();
  void handleBuildJSON();
  void handleGPRSAttach();
  void handleSendHTTP();
  void handleSendOfflineQueue();
  void handleSleepPrepare();
  void handleSleep();
  void handleIdle();

  // Helpers
  void processActionRequest(const char *action, const char *args);
  bool sendDataToServer();
  void queueCurrentData();
  void updateLEDForState();

  char _deviceId[GL868_ESP32_MAX_DEVICE_ID_LEN];
  char _apiKey[GL868_ESP32_MAX_API_KEY_LEN];
  bool _initialized;

  // Current data
  GPSData _currentGPS;
  uint8_t _currentBattery;

  // Flags
  bool _forceSendRequested;
  bool _motionTriggerEnabled;
  bool _fullPowerOffEnabled;
  bool
      _offlineSentFirst; // True if offline queue processed before GPS this wake

  // Timeouts
  uint32_t _gpsTimeout;
  uint32_t _stateTimeout;
  uint32_t _errorRebootTimeout;

  // GPS freshness tracking
  char _firstGPSTimestamp[20]; // First timestamp seen in GPS_WAIT_FIX state

  // Payload fields
  PayloadField _payloadFields[MAX_PAYLOAD_KEYS];
  uint8_t _payloadCount;

  // Operating mode
  uint8_t _operatingMode;
};

// Global instance for convenient access
extern GL868_ESP32 GeoLinker;

#endif // GL868_ESP32_H
