/*
 * SendDataToServer.ino
 * GL868_ESP32 Library - Send Data to Custom Server
 *
 * Demonstrates using httpPost() to send data to any HTTP server.
 * Uses httpbin.org for testing - it echoes back what you send.
 */

#include <GL868_ESP32.h>

// ============================================================================
// Configuration - EDIT THESE VALUES
// ============================================================================
#define DEVICE_ID "YOUR_DEVICE_ID"
#define API_KEY "YOUR_API_KEY"

// Test server URL (httpbin.org echoes back your request)
#define TEST_URL "http://httpbin.org/post"

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("GL868_ESP32 - Send Data to Server Example");

  // Initialize
  GeoLinker.begin(DEVICE_ID, API_KEY);

  // Power on modem
  Serial.println("Powering on modem...");
  if (!GeoLinker.modem.powerOn()) {
    Serial.println("Failed to power on modem!");
    while (1)
      delay(1000);
  }
  Serial.println("Modem ready!");

  // Wait for network
  Serial.print("Waiting for network");
  while (!GeoLinker.gsm.waitNetworkRegistration(10000)) {
    Serial.print(".");
  }
  Serial.println("\nNetwork registered!");

  // Example 1: Simple POST with JSON body
  Serial.println("\n--- Example 1: Simple JSON POST ---");
  sendJsonData();

  // Example 2: POST with custom headers
  Serial.println("\n--- Example 2: POST with Custom Headers ---");
  sendWithHeaders();

  // Example 3: POST and capture response
  Serial.println("\n--- Example 3: POST with Response Capture ---");
  sendAndGetResponse();
}

void loop() { delay(1000); }

// ============================================================================
// Example 1: Simple JSON POST
// ============================================================================
void sendJsonData() {
  const char *jsonBody =
      "{\"device\":\"GL868_ESP32\",\"temperature\":25.5,\"humidity\":60}";

  Serial.print("Sending to: ");
  Serial.println(TEST_URL);
  Serial.print("Body: ");
  Serial.println(jsonBody);

  int statusCode =
      GeoLinker.httpPost(TEST_URL, jsonBody, "Content-Type: application/json");

  Serial.print("HTTP Status: ");
  Serial.println(statusCode);

  if (statusCode == 200) {
    Serial.println("Success!");
  } else {
    Serial.println("Failed!");
  }
}

// ============================================================================
// Example 2: POST with Custom Headers
// ============================================================================
void sendWithHeaders() {
  const char *body = "sensor=temperature&value=25.5";

  // Multiple headers separated by \r\n
  const char *headers = "Content-Type: application/x-www-form-urlencoded\r\n"
                        "Authorization: Bearer your_token_here\r\n"
                        "X-Custom-Header: MyValue";

  Serial.print("Sending to: ");
  Serial.println(TEST_URL);
  Serial.print("Body: ");
  Serial.println(body);

  int statusCode = GeoLinker.httpPost(TEST_URL, body, headers);

  Serial.print("HTTP Status: ");
  Serial.println(statusCode);
}

// ============================================================================
// Example 3: POST and Capture Response
// ============================================================================
void sendAndGetResponse() {
  const char *body = "{\"message\":\"Hello from ESP32!\"}";

  // Buffer to store server response
  char response[512];

  Serial.print("Sending to: ");
  Serial.println(TEST_URL);

  int statusCode =
      GeoLinker.httpPost(TEST_URL, body, "Content-Type: application/json",
                         response, sizeof(response));

  Serial.print("HTTP Status: ");
  Serial.println(statusCode);

  if (statusCode == 200) {
    Serial.println("Server Response:");
    Serial.println(response);
  } else {
    Serial.println("Request failed");
  }
}
