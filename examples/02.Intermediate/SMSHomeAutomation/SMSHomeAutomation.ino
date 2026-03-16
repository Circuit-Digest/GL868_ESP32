/*
 * SMSHomeAutomation.ino
 * GL868_ESP32 Library - SMS Home Automation
 *
 * Control GPIOs via SMS commands from authorized numbers.
 * Example commands:
 *   "ON 2"   - Turn GPIO2 HIGH
 *   "OFF 2"  - Turn GPIO2 LOW
 *   "STATUS" - Get status of all configured pins
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

// Reply number for status messages
#define REPLY_NUMBER "+91XXXXXXXXXX"

// Controllable GPIO pins
const int controlPins[] = {2, 4, 12, 13};
const int numPins = sizeof(controlPins) / sizeof(controlPins[0]);

// ============================================================================
// SMS Command Handlers
// ============================================================================

// Handle "ON <pin>" command
void handleOnCommand(const char *sender, const char *command,
                     const char *args) {
  if (!args || strlen(args) == 0) {
    GeoLinker.sendSMS(sender, "Usage: ON <pin>");
    return;
  }
  int pin = atoi(args);
  controlPin(pin, HIGH, sender);
}

// Handle "OFF <pin>" command
void handleOffCommand(const char *sender, const char *command,
                      const char *args) {
  if (!args || strlen(args) == 0) {
    GeoLinker.sendSMS(sender, "Usage: OFF <pin>");
    return;
  }
  int pin = atoi(args);
  controlPin(pin, LOW, sender);
}

// Handle "STATUS" command (uses built-in STATUS, but we override)
void handleStatusCommand(const char *sender, const char *command,
                         const char *args) {
  sendStatus(sender);
}

// ============================================================================
// Control Pin
// ============================================================================
void controlPin(int pin, int state, const char *replyTo) {
  // Validate pin
  bool validPin = false;
  for (int i = 0; i < numPins; i++) {
    if (controlPins[i] == pin) {
      validPin = true;
      break;
    }
  }

  if (!validPin) {
    char msg[50];
    snprintf(msg, sizeof(msg), "Error: GPIO%d not in allowed list", pin);
    GeoLinker.sendSMS(replyTo, msg);
    return;
  }

  // Set pin state
  digitalWrite(pin, state);

  // Confirm action
  char msg[50];
  snprintf(msg, sizeof(msg), "GPIO%d set to %s", pin,
           state == HIGH ? "ON" : "OFF");
  Serial.println(msg);
  GeoLinker.sendSMS(replyTo, msg);
}

// ============================================================================
// Send Status
// ============================================================================
void sendStatus(const char *replyTo) {
  String status = "GPIO Status:\n";

  for (int i = 0; i < numPins; i++) {
    int pin = controlPins[i];
    int state = digitalRead(pin);
    status +=
        "GPIO" + String(pin) + ": " + (state == HIGH ? "ON" : "OFF") + "\n";
  }

  // Add battery info
  int battery = GeoLinker.getBatteryPercent();
  status += "Battery: " + String(battery) + "%";

  GeoLinker.sendSMS(replyTo, status.c_str());
}

// ============================================================================
// Setup
// ============================================================================
void setup() {
  Serial.begin(115200);

  // Initialize control pins as outputs
  for (int i = 0; i < numPins; i++) {
    pinMode(controlPins[i], OUTPUT);
    digitalWrite(controlPins[i], LOW); // Start LOW
  }

  // Use SMS-only mode (no GPS tracking needed)
  GeoLinker.setOperatingMode(MODE_SMS_ONLY);

  // Initialize
  GeoLinker.begin(DEVICE_ID, API_KEY);

  // Whitelist authorized number
  if (!ACCEPT_ANY_NUMBER) {
    GeoLinker.whitelist.add(AUTHORIZED_NUMBER);
  }

  // Register SMS command handlers
  GeoLinker.registerSMSHandler("ON", handleOnCommand);
  GeoLinker.registerSMSHandler("OFF", handleOffCommand);
  GeoLinker.registerSMSHandler("PINSTATUS", handleStatusCommand);

  Serial.println("SMS Home Automation Started");
  Serial.print("Controllable pins: ");
  for (int i = 0; i < numPins; i++) {
    Serial.print(controlPins[i]);
    if (i < numPins - 1)
      Serial.print(", ");
  }
  Serial.println();
}

void loop() { GeoLinker.update(); }
