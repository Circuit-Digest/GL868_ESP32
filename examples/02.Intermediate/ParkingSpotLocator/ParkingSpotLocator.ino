/*
 * ParkingSpotLocator.ino
 * GL868_ESP32 Library - Parking Spot Locator
 *
 * When button on GPIO48 is pressed, sends current location via SMS
 * with a Google Maps URL to help you find your parked vehicle.
 * GPS retry with timeout and configurable retries.
 */

#include <GL868_ESP32.h>

// ============================================================================
// Configuration - EDIT THESE VALUES
// ============================================================================
#define DEVICE_ID "YOUR_DEVICE_ID"
#define API_KEY "YOUR_API_KEY"
#define SMS_NUMBER "+91XXXXXXXXXX" // Number to send location to

#define BUTTON_PIN 48
#define DEBOUNCE_MS 50

// GPS retry configuration
#define GPS_TIMEOUT 30000 // 30 seconds per attempt
#define GPS_MAX_RETRIES 3 // Number of retries

// ============================================================================
// Global Variables
// ============================================================================
uint32_t lastButtonPress = 0;
bool buttonState = false;
bool lastButtonState = false;

void setup() {
  Serial.begin(115200);

  // Initialize button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize
  GeoLinker.begin(DEVICE_ID, API_KEY);

  // Disable automatic tracking
  GeoLinker.setSendInterval(0);
  GeoLinker.enableMotionTrigger(false);

  Serial.println("Parking Spot Locator Started");
  Serial.println("Press button on GPIO48 to send current location");
}

void loop() {
  GeoLinker.update();
  handleButton();
}

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
// Handle Button
// ============================================================================
void handleButton() {
  bool currentState = !digitalRead(BUTTON_PIN); // Active LOW

  // Debounce
  if (currentState != lastButtonState) {
    lastButtonPress = millis();
  }

  if ((millis() - lastButtonPress) > DEBOUNCE_MS) {
    if (currentState != buttonState) {
      buttonState = currentState;

      if (buttonState) {
        // Button pressed
        sendParkingLocation();
      }
    }
  }

  lastButtonState = currentState;
}

// ============================================================================
// Send Parking Location
// ============================================================================
void sendParkingLocation() {
  Serial.println("\nButton pressed - getting location...");

  // Get GPS location with retry
  GPSData gps;
  bool hasLocation = getValidLocation(&gps);

  char message[300];

  if (hasLocation) {
    int battery = GeoLinker.getBatteryPercent();

    // Format the message with location
    snprintf(message, sizeof(message),
             "Parking Location Saved!\n\n"
             "Coordinates:\n"
             "Lat: %.6f\n"
             "Lon: %.6f\n\n"
             "Accuracy: %.1fm\n"
             "Battery: %d%%\n\n"
             "Find your car:\n"
             "https://maps.google.com/?q=%.6f,%.6f",
             gps.latitude, gps.longitude,
             gps.hdop * 5.0, // Approximate accuracy in meters
             battery, gps.latitude, gps.longitude);

    Serial.println("Location found:");
    Serial.printf("  Lat: %.6f\n", gps.latitude);
    Serial.printf("  Lon: %.6f\n", gps.longitude);
  } else {
    // No GPS fix after retries
    int battery = GeoLinker.getBatteryPercent();
    int sats = gps.satellites;

    snprintf(message, sizeof(message),
             "Parking Location Request\n\n"
             "Status: No GPS fix (after %d retries)\n"
             "Satellites: %d\n"
             "Battery: %d%%\n\n"
             "Please try again in a moment.",
             GPS_MAX_RETRIES, sats, battery);

    Serial.println("No GPS fix after retries");
    Serial.printf("  Satellites: %d\n", sats);
  }

  // Send SMS
  Serial.print("Sending SMS to: ");
  Serial.println(SMS_NUMBER);

  if (GeoLinker.sendSMS(SMS_NUMBER, message)) {
    Serial.println("SMS sent successfully!");
  } else {
    Serial.println("Failed to send SMS");
  }
}
