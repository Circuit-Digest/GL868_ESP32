/*
 * AntiTheftTracker.ino
 * GL868_ESP32 Library - Anti-Theft Car Tracker
 *
 * Sends SMS and makes a call when:
 * 1. Motion is detected AND
 * 2. Vehicle has moved more than 10 meters from armed position
 *
 * CRITICAL: Once theft is detected, keeps sending location updates
 * until disarmed (continuous tracking mode).
 *
 * Uses a switch on GPIO48 for arming/disarming.
 * GPS retry with timeout and configurable retries.
 */

#include <GL868_ESP32.h>

// ============================================================================
// Configuration - EDIT THESE VALUES
// ============================================================================
#define DEVICE_ID "YOUR_DEVICE_ID"
#define API_KEY "YOUR_API_KEY"
#define ALERT_NUMBER "+91XXXXXXXXXX" // Phone number for alerts

#define ARM_SWITCH_PIN 48   // GPIO for arm/disarm switch
#define MOVE_THRESHOLD 10.0 // Meters - trigger alert if moved more than this

// GPS retry configuration
#define GPS_TIMEOUT 30000 // 30 seconds per attempt
#define GPS_MAX_RETRIES 3 // Number of retries

// Theft tracking configuration
#define THEFT_UPDATE_INTERVAL 60000 // Send update every 60 seconds during theft

// ============================================================================
// Global Variables
// ============================================================================
bool isArmed = false;
double armedLat = 0.0;
double armedLon = 0.0;
bool theftDetected = false;
uint32_t lastTheftUpdate = 0;

void setup() {
  Serial.begin(115200);

  // Initialize arm/disarm switch
  pinMode(ARM_SWITCH_PIN, INPUT_PULLUP);

  // Initialize
  GeoLinker.begin(DEVICE_ID, API_KEY);
  GeoLinker.enableMotionTrigger(true);
  GeoLinker.setSendInterval(0); // Disable scheduled sending

  Serial.println("Anti-Theft Tracker Started");
  Serial.println("Use switch on GPIO48 to arm/disarm");
}

void loop() {
  GeoLinker.update();

  // Check arm/disarm switch
  bool switchState = !digitalRead(ARM_SWITCH_PIN); // LOW = armed (switch ON)

  if (switchState && !isArmed) {
    // Arm the system
    armSystem();
  } else if (!switchState && isArmed) {
    // Disarm the system
    disarmSystem();
  }

  // Check for unauthorized movement when armed (before theft detected)
  if (isArmed && !theftDetected) {
    checkMovement();
  }

  // If theft detected, keep sending updates until disarmed
  if (isArmed && theftDetected) {
    handleTheftUpdates();
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
// Arm System
// ============================================================================
void armSystem() {
  Serial.println("\n=== ARMING SYSTEM ===");

  // Get current GPS position with retry
  GPSData gps;
  if (getValidLocation(&gps)) {
    armedLat = gps.latitude;
    armedLon = gps.longitude;
    isArmed = true;
    theftDetected = false;

    Serial.println("System ARMED");
    Serial.print("Armed position: ");
    Serial.print(armedLat, 6);
    Serial.print(", ");
    Serial.println(armedLon, 6);

    // Confirmation SMS with location
    char msg[150];
    snprintf(msg, sizeof(msg),
             "Anti-Theft ARMED\n"
             "Position: https://maps.google.com/?q=%.6f,%.6f",
             armedLat, armedLon);
    GeoLinker.sendSMS(ALERT_NUMBER, msg);
  } else {
    Serial.println("Cannot arm - no GPS fix after retries");
    GeoLinker.sendSMS(ALERT_NUMBER, "Anti-Theft ARM FAILED - No GPS fix");
  }
}

// ============================================================================
// Disarm System
// ============================================================================
void disarmSystem() {
  bool wasTheftMode = theftDetected;
  isArmed = false;
  theftDetected = false;

  Serial.println("\n=== SYSTEM DISARMED ===");

  if (wasTheftMode) {
    GeoLinker.sendSMS(ALERT_NUMBER,
                      "Anti-Theft DISARMED - Theft tracking stopped");
  } else {
    GeoLinker.sendSMS(ALERT_NUMBER, "Anti-Theft DISARMED");
  }
}

// ============================================================================
// Check Movement (before theft detected)
// ============================================================================
void checkMovement() {
  // Only check after motion is detected
  if (!GeoLinker.motion.isMoving()) {
    return;
  }

  GPSData gps;
  if (!getValidLocation(&gps)) {
    return; // Can't check without GPS
  }

  // Calculate distance from armed position
  double distance =
      calculateDistance(armedLat, armedLon, gps.latitude, gps.longitude);

  Serial.print("Distance moved: ");
  Serial.print(distance, 1);
  Serial.println(" meters");

  if (distance > MOVE_THRESHOLD) {
    triggerTheftAlert(&gps, distance);
  }
}

// ============================================================================
// Trigger Theft Alert (Initial)
// ============================================================================
void triggerTheftAlert(GPSData *gps, double distance) {
  Serial.println("\n!!! THEFT ALERT !!!");

  theftDetected = true;
  lastTheftUpdate = 0; // Force immediate update

  // Build initial alert message
  char message[250];
  snprintf(message, sizeof(message),
           "*** THEFT ALERT ***\n"
           "Vehicle moved %.0fm!\n\n"
           "Current location:\n"
           "Lat: %.6f\n"
           "Lon: %.6f\n\n"
           "Track: https://maps.google.com/?q=%.6f,%.6f",
           distance, gps->latitude, gps->longitude, gps->latitude,
           gps->longitude);

  // Send SMS alert
  Serial.println("Sending THEFT SMS...");
  GeoLinker.sendSMS(ALERT_NUMBER, message);

  // Make voice call
  Serial.println("Making alert call...");
  GeoLinker.makeCall(ALERT_NUMBER, 30);

  // Send to cloud
  // Data will be sent via GeoLinker.update() automatically

  Serial.println("Theft mode active - sending updates until disarmed!");
  lastTheftUpdate = millis();
}

// ============================================================================
// Handle Theft Updates (Periodic - until disarmed)
// ============================================================================
void handleTheftUpdates() {
  if (millis() - lastTheftUpdate < THEFT_UPDATE_INTERVAL) {
    return;
  }

  Serial.println("\n--- THEFT TRACKING UPDATE ---");

  GPSData gps;
  bool hasLocation = getValidLocation(&gps);

  char message[250];

  if (hasLocation) {
    double distance =
        calculateDistance(armedLat, armedLon, gps.latitude, gps.longitude);
    int battery = GeoLinker.getBatteryPercent();

    snprintf(message, sizeof(message),
             "*** THEFT UPDATE ***\n"
             "Distance: %.0fm from start\n"
             "Speed: %.1f km/h\n"
             "Battery: %d%%\n\n"
             "Track: https://maps.google.com/?q=%.6f,%.6f",
             distance, gps.speed, battery, gps.latitude, gps.longitude);
  } else {
    int battery = GeoLinker.getBatteryPercent();
    snprintf(message, sizeof(message),
             "*** THEFT UPDATE ***\n"
             "Location: No GPS (retrying)\n"
             "Battery: %d%%\n"
             "Toggle switch to disarm.",
             battery);
  }

  Serial.println("Sending theft update SMS...");
  GeoLinker.sendSMS(ALERT_NUMBER, message);

  // Send to cloud
  // Data will be sent via GeoLinker.update() automatically

  lastTheftUpdate = millis();
}

// ============================================================================
// Calculate Distance (Haversine formula)
// ============================================================================
double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0; // Earth radius in meters
  double dLat = radians(lat2 - lat1);
  double dLon = radians(lon2 - lon1);

  double a = sin(dLat / 2) * sin(dLat / 2) + cos(radians(lat1)) *
                                                 cos(radians(lat2)) *
                                                 sin(dLon / 2) * sin(dLon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));

  return R * c;
}
