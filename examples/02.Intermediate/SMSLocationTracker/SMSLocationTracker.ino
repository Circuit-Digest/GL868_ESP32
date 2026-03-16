/*
 * SMSLocationTracker.ino
 * GL868_ESP32 Library - SMS Location Tracker
 *
 * When an SMS command is received, replies with current location
 * and device status including a Google Maps URL.
 *
 * GPS retry with timeout and configurable retries.
 * Commands: "LOCATE" or "WHERE"
 */

#include <GL868_ESP32.h>

// ============================================================================
// Configuration - EDIT THESE VALUES
// ============================================================================
#define DEVICE_ID "YOUR_DEVICE_ID"
#define API_KEY "YOUR_API_KEY"

// Set to true to accept commands from any number
// Set to false to accept only from AUTHORIZED_NUMBER
#define ACCEPT_ANY_NUMBER false
#define AUTHORIZED_NUMBER "+91XXXXXXXXXX"

// Number to send location replies to (can be same as authorized)
#define REPLY_NUMBER "+91XXXXXXXXXX"

// GPS retry configuration
#define GPS_TIMEOUT 30000 // 30 seconds per attempt
#define GPS_MAX_RETRIES 3 // Number of retries

// ============================================================================
// Get Valid GPS Location with Retry
// ============================================================================
bool getValidLocation(GPSData *gps) {
  Serial.println("Getting GPS location...");

  for (int retry = 0; retry < GPS_MAX_RETRIES; retry++) {
    Serial.printf("  Attempt %d/%d\n", retry + 1, GPS_MAX_RETRIES);

    uint32_t start = millis();
    while (millis() - start < GPS_TIMEOUT) {
      if (GeoLinker.getLocationNow(gps) && gps->valid) {
        Serial.printf("  GPS fix: %.6f, %.6f\n", gps->latitude, gps->longitude);
        return true;
      }
      delay(1000);
    }
    Serial.println("  Timeout, retrying...");
  }

  Serial.println("  GPS failed after all retries");
  return false;
}

// ============================================================================
// SMS Command Handlers
// ============================================================================
void handleLocateCommand(const char *sender, const char *command,
                         const char *args) {
  Serial.printf("Location request from %s (cmd: %s)\n", sender, command);
  sendLocationReply(sender);
}

// ============================================================================
// Send Location Reply
// ============================================================================
void sendLocationReply(const char *replyTo) {
  Serial.println("Processing location request...");

  GPSData gps;
  bool hasLocation = getValidLocation(&gps);

  // Build response message
  char message[300];

  if (hasLocation) {
    // Get battery and signal info
    int battery = GeoLinker.getBatteryPercent();
    int signal = GeoLinker.getSignalStrength();

    snprintf(message, sizeof(message),
             "Device Location:\n"
             "Lat: %.6f\n"
             "Lon: %.6f\n"
             "Speed: %.1f km/h\n"
             "Battery: %d%%\n"
             "Signal: %d\n"
             "Map: https://maps.google.com/?q=%.6f,%.6f",
             gps.latitude, gps.longitude, gps.speed, battery, signal,
             gps.latitude, gps.longitude);
  } else {
    // No GPS fix after retries
    int battery = GeoLinker.getBatteryPercent();
    int signal = GeoLinker.getSignalStrength();

    snprintf(message, sizeof(message),
             "Device Status:\n"
             "Location: No GPS fix (after %d retries)\n"
             "Battery: %d%%\n"
             "Signal: %d\n"
             "Satellites: %d",
             GPS_MAX_RETRIES, battery, signal, gps.satellites);
  }

  // Send reply
  Serial.println("Sending location reply...");
  GeoLinker.sendSMS(replyTo, message);
}

// ============================================================================
// Setup
// ============================================================================
void setup() {
  Serial.begin(115200);

  // Use SMS-only mode (respond only to SMS commands)
  GeoLinker.setOperatingMode(MODE_SMS_ONLY);

  // Initialize
  GeoLinker.begin(DEVICE_ID, API_KEY);

  // Whitelist authorized number
  if (!ACCEPT_ANY_NUMBER) {
    GeoLinker.whitelist.add(AUTHORIZED_NUMBER);
  }

  // Register SMS command handlers
  GeoLinker.registerSMSHandler("LOCATE", handleLocateCommand);
  GeoLinker.registerSMSHandler("WHERE", handleLocateCommand);
  GeoLinker.registerSMSHandler("LOCATION", handleLocateCommand);

  Serial.println("SMS Location Tracker Started");
  Serial.println("Send 'LOCATE' or 'WHERE' via SMS to get location");
  if (ACCEPT_ANY_NUMBER) {
    Serial.println("Accepting commands from any number");
  } else {
    Serial.print("Accepting commands only from: ");
    Serial.println(AUTHORIZED_NUMBER);
  }
}

void loop() { GeoLinker.update(); }
