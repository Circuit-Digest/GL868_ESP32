/*
 * SOSPanicButton.ino
 * GL868_ESP32 Library - SOS Panic Button
 *
 * Emergency alert system with a button on GPIO48:
 * - Short press: Send SOS alert (SMS + Call) to configured numbers
 * - Long press (3 sec): Toggle arm/disarm
 *
 * CRITICAL FEATURE: Once triggered, keeps sending location periodically
 * until disarmed. Even if GPS fails, sends emergency alert without location,
 * then retries GPS and sends updates.
 */

#include <GL868_ESP32.h>

// ============================================================================
// Configuration - EDIT THESE VALUES
// ============================================================================
#define DEVICE_ID "YOUR_DEVICE_ID"
#define API_KEY "YOUR_API_KEY"

// Emergency contacts (add more as needed)
const char *alertNumbers[] = {"+91XXXXXXXXXX", "+91YYYYYYYYYY"};
const int numContacts = sizeof(alertNumbers) / sizeof(alertNumbers[0]);

// Button configuration
#define BUTTON_PIN 48
#define LONG_PRESS_MS 3000 // 3 seconds for arm/disarm
#define DEBOUNCE_MS 50

// GPS retry configuration
#define GPS_TIMEOUT 30000 // 30 seconds per attempt
#define GPS_MAX_RETRIES 3 // Number of retries

// SOS mode configuration
#define SOS_UPDATE_INTERVAL 60000 // Send location update every 60 seconds

// ============================================================================
// Global Variables
// ============================================================================
bool isArmed = true;    // Start armed
bool sosActive = false; // SOS mode active
uint32_t lastSOSUpdate = 0;
uint32_t buttonPressStart = 0;
bool buttonWasPressed = false;
bool longPressHandled = false;

void setup() {
  Serial.begin(115200);

  // Initialize button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize
  GeoLinker.begin(DEVICE_ID, API_KEY);

  // Enable GPS for location in alerts
  GeoLinker.setSendInterval(0); // No automatic sending
  GeoLinker.enableMotionTrigger(false);

  Serial.println("SOS Panic Button Started");
  Serial.println("Short press: Send SOS alert");
  Serial.println("Long press (3s): Toggle arm/disarm");
  Serial.println("Status: ARMED");
}

void loop() {
  GeoLinker.update();
  handleButton();

  // If SOS is active, keep sending location updates
  if (sosActive && isArmed) {
    handleSOSUpdates();
  }
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
  bool isPressed = !digitalRead(BUTTON_PIN); // Active LOW

  if (isPressed && !buttonWasPressed) {
    // Button just pressed
    buttonPressStart = millis();
    buttonWasPressed = true;
    longPressHandled = false;
  } else if (isPressed && buttonWasPressed) {
    // Button held
    if (!longPressHandled && (millis() - buttonPressStart >= LONG_PRESS_MS)) {
      // Long press detected
      longPressHandled = true;
      toggleArmDisarm();
    }
  } else if (!isPressed && buttonWasPressed) {
    // Button released
    if (!longPressHandled && (millis() - buttonPressStart >= DEBOUNCE_MS)) {
      // Short press - trigger SOS
      if (isArmed) {
        triggerSOS();
      } else {
        Serial.println("SOS disabled - system disarmed");
      }
    }
    buttonWasPressed = false;
  }
}

// ============================================================================
// Toggle Arm/Disarm
// ============================================================================
void toggleArmDisarm() {
  isArmed = !isArmed;

  if (isArmed) {
    Serial.println("\n=== SYSTEM ARMED ===");
    GeoLinker.sendSMS(alertNumbers[0], "SOS System ARMED");
  } else {
    Serial.println("\n=== SYSTEM DISARMED ===");
    sosActive = false; // Stop SOS updates
    GeoLinker.sendSMS(alertNumbers[0],
                      "SOS System DISARMED - Emergency mode ended");
  }
}

// ============================================================================
// Trigger SOS Alert (Initial)
// ============================================================================
void triggerSOS() {
  Serial.println("\n!!! SOS TRIGGERED !!!");

  sosActive = true;
  lastSOSUpdate = 0; // Force immediate update

  // Get location - but send alert even if GPS fails
  GPSData gps;
  bool hasLocation = getValidLocation(&gps);

  // Build and send initial SOS message
  sendSOSMessage(&gps, hasLocation, true); // true = initial alert

  // Call all contacts
  Serial.println("Calling emergency contacts...");
  for (int i = 0; i < numContacts; i++) {
    Serial.print("  Calling: ");
    Serial.println(alertNumbers[i]);
    GeoLinker.makeCall(alertNumbers[i], 30);
    delay(2000);
  }

  // Also send to cloud
  // Data will be sent via GeoLinker.update() automatically

  Serial.println("Initial SOS sent! Will keep sending updates until disarmed.");
  lastSOSUpdate = millis();
}

// ============================================================================
// Handle SOS Updates (Periodic)
// ============================================================================
void handleSOSUpdates() {
  if (millis() - lastSOSUpdate < SOS_UPDATE_INTERVAL) {
    return;
  }

  Serial.println("\n--- SOS UPDATE ---");

  GPSData gps;
  bool hasLocation = getValidLocation(&gps);

  sendSOSMessage(&gps, hasLocation, false); // false = update, not initial

  // Also send to cloud
  // Data will be sent via GeoLinker.update() automatically

  lastSOSUpdate = millis();
}

// ============================================================================
// Send SOS Message
// ============================================================================
void sendSOSMessage(GPSData *gps, bool hasLocation, bool isInitial) {
  char message[300];

  if (hasLocation) {
    int battery = GeoLinker.getBatteryPercent();

    snprintf(message, sizeof(message),
             "%s\n"
             "%s\n\n"
             "Location:\n"
             "Lat: %.6f\n"
             "Lon: %.6f\n"
             "Battery: %d%%\n\n"
             "Track: https://maps.google.com/?q=%.6f,%.6f",
             isInitial ? "*** SOS EMERGENCY ***" : "*** SOS UPDATE ***",
             isInitial ? "Panic button pressed!" : "Continuous tracking active",
             gps->latitude, gps->longitude, battery, gps->latitude,
             gps->longitude);
  } else {
    int battery = GeoLinker.getBatteryPercent();
    snprintf(message, sizeof(message),
             "%s\n"
             "%s\n\n"
             "Location: No GPS fix (retrying)\n"
             "Battery: %d%%\n"
             "Long-press button to disarm.",
             isInitial ? "*** SOS EMERGENCY ***" : "*** SOS UPDATE ***",
             isInitial ? "Panic button pressed!" : "Continuous tracking active",
             battery);
  }

  // Send SMS to all contacts
  Serial.println("Sending SOS to all contacts...");
  for (int i = 0; i < numContacts; i++) {
    Serial.print("  -> ");
    Serial.println(alertNumbers[i]);
    GeoLinker.sendSMS(alertNumbers[i], message);
    delay(1000); // Small delay between messages
  }
}
