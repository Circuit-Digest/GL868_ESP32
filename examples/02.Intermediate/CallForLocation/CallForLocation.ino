/*
 * CallForLocation.ino
 * GL868_ESP32 Library - Call-for-Location
 *
 * When a call is received from an authorized number:
 * 1. Disconnect the call immediately
 * 2. Send current location via SMS with Google Maps URL
 *
 * GPS retry with timeout and configurable retries.
 * This provides a free way to request location using missed call.
 */

#include <GL868_ESP32.h>

// ============================================================================
// Configuration - EDIT THESE VALUES
// ============================================================================
#define DEVICE_ID "YOUR_DEVICE_ID"
#define API_KEY "YOUR_API_KEY"

// Set to true to accept calls from any number
// Set to false to accept only from AUTHORIZED_NUMBER
#define ACCEPT_ANY_NUMBER false
#define AUTHORIZED_NUMBER "+91XXXXXXXXXX"

// Number to send location SMS to
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
// Call Callback
// ============================================================================
void onCallReceived(const char *callerNumber) {
  Serial.printf("Incoming call from: %s\n", callerNumber);

  // Check if caller is authorized
  if (!ACCEPT_ANY_NUMBER) {
    if (strcmp(callerNumber, AUTHORIZED_NUMBER) != 0) {
      Serial.println("Ignoring - unauthorized caller");
      GeoLinker.hangupCall(); // Still hang up
      return;
    }
  }

  // Hang up the call immediately
  Serial.println("Hanging up and sending location...");
  GeoLinker.hangupCall();

  // Small delay to ensure call is disconnected
  delay(2000);

  // Send location
  sendLocationSMS(callerNumber);
}

// ============================================================================
// Send Location SMS
// ============================================================================
void sendLocationSMS(const char *callerNumber) {
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
             "Location Request (Missed Call)\n"
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
             "Location Request (Missed Call)\n"
             "Status: No GPS fix (after %d retries)\n"
             "Battery: %d%%\n"
             "Signal: %d\n"
             "Satellites: %d",
             GPS_MAX_RETRIES, battery, signal, gps.satellites);
  }

  // Send to caller (or reply number)
  const char *replyTo = ACCEPT_ANY_NUMBER ? callerNumber : REPLY_NUMBER;
  Serial.print("Sending location to: ");
  Serial.println(replyTo);
  GeoLinker.sendSMS(replyTo, message);
}

// ============================================================================
// Setup
// ============================================================================
void setup() {
  Serial.begin(115200);

  // Use call-only mode
  GeoLinker.setOperatingMode(MODE_CALL_ONLY);

  // Initialize
  GeoLinker.begin(DEVICE_ID, API_KEY);

  // Whitelist authorized number
  if (!ACCEPT_ANY_NUMBER) {
    GeoLinker.whitelist.add(AUTHORIZED_NUMBER);
  }

  // Register call handler
  GeoLinker.registerCallHandler(onCallReceived);

  // Set action to custom (we handle everything in our callback)
  GeoLinker.setCallAction(CALL_CUSTOM);

  Serial.println("Call-for-Location Started");
  Serial.println("Call this device to receive location via SMS");
  if (ACCEPT_ANY_NUMBER) {
    Serial.println("Accepting calls from any number");
  } else {
    Serial.print("Accepting calls only from: ");
    Serial.println(AUTHORIZED_NUMBER);
  }
}

void loop() { GeoLinker.update(); }
