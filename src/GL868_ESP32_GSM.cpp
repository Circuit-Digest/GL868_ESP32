/*
 * GL868_ESP32_GSM.cpp
 * GSM layer implementation
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#include "GL868_ESP32_GSM.h"
#include "GL868_ESP32_Logger.h"

GL868_ESP32_GSM::GL868_ESP32_GSM()
    : _modem(nullptr), _registered(false), _lastRegStatus(-1),
      _gprsAttached(false), _tcpConnected(false) {
  // Initialize with compile-time default APN
  strncpy(_apn, GL868_ESP32_APN, sizeof(_apn) - 1);
  _apn[sizeof(_apn) - 1] = '\0';
  _apnUser[0] = '\0';
  _apnPass[0] = '\0';

#if defined(GL868_ESP32_APN_USER) && defined(GL868_ESP32_APN_PASS)
  strncpy(_apnUser, GL868_ESP32_APN_USER, sizeof(_apnUser) - 1);
  _apnUser[sizeof(_apnUser) - 1] = '\0';
  strncpy(_apnPass, GL868_ESP32_APN_PASS, sizeof(_apnPass) - 1);
  _apnPass[sizeof(_apnPass) - 1] = '\0';
#endif
}

bool GL868_ESP32_GSM::checkSIMReady() {
  if (!_modem)
    return false;

  GL868_ESP32_LOG_D("Checking SIM status...");

  // Check SIM status with retries
  for (int i = 0; i < 5; i++) {
    _modem->flushSerial();
    _modem->getSerial().println("AT+CPIN?");

    uint32_t start = millis();
    while (millis() - start < 2000) {
      if (_modem->getSerial().available()) {
        String line = _modem->getSerial().readStringUntil('\n');
        line.trim();

        if (line.indexOf("+CPIN: READY") >= 0) {
          GL868_ESP32_LOG_I("SIM card ready");
          return true;
        }
        if (line.indexOf("SIM PIN") >= 0) {
          GL868_ESP32_LOG_E("SIM requires PIN");
          return false;
        }
        if (line.indexOf("SIM PUK") >= 0) {
          GL868_ESP32_LOG_E("SIM requires PUK");
          return false;
        }
        if (line.indexOf("NOT INSERTED") >= 0) {
          GL868_ESP32_LOG_E("SIM not inserted");
          return false;
        }
      }
      yield();
    }

    // Wait before retry
    uint32_t delayStart = millis();
    while (millis() - delayStart < 1000)
      yield();
  }

  GL868_ESP32_LOG_E("SIM check timeout");
  return false;
}

bool GL868_ESP32_GSM::waitNetworkRegistration(uint32_t timeout) {
  if (!_modem)
    return false;

  GL868_ESP32_LOG_I("Waiting for network registration...");

  uint32_t start = millis();
  while (millis() - start < timeout) {
    _modem->flushSerial();
    _modem->getSerial().println("AT+CREG?");

    uint32_t cmdStart = millis();
    while (millis() - cmdStart < 2000) {
      if (_modem->getSerial().available()) {
        String line = _modem->getSerial().readStringUntil('\n');
        line.trim();

        // Parse +CREG: n,stat
        if (line.startsWith("+CREG:")) {
          int commaPos = line.indexOf(',');
          if (commaPos > 0) {
            int stat = line.substring(commaPos + 1).toInt();
            _lastRegStatus = stat;

            // 1 = registered home, 5 = registered roaming
            if (stat == 1 || stat == 5) {
              _registered = true;
              GL868_ESP32_LOG_I("Network registered (status: %d)", stat);
              return true;
            }

            // Status 0: Not searching, 3: Registration denied, 4: Unknown
            // If we get these, it's a fatal registration error - no point
            // waiting
            if (stat == 0 || stat == 3 || stat == 4) {
              GL868_ESP32_LOG_E("Network registration fatal error (status: %d)",
                                stat);
              _registered = false;
              return false;
            }

            GL868_ESP32_LOG_D("Registration status: %d", stat);
          }
        }
      }
      yield();
    }

    // Wait before next check
    uint32_t delayStart = millis();
    while (millis() - delayStart < 2000)
      yield();
  }

  GL868_ESP32_LOG_E("Network registration timeout");
  _registered = false;
  return false;
}

bool GL868_ESP32_GSM::attachGPRS() {
  if (!_modem || !_registered)
    return false;

  GL868_ESP32_LOG_I("Attaching GPRS...");

  // Reset GPRS state flag to force re-check
  _gprsAttached = false;

  // First, shut down any stale IP connections (important after CFUN wake)
  _modem->sendAT("+CIPSHUT", 5000);
  _modem->flushSerial();

  // Small delay after CIPSHUT
  delay(500);

  // Check current GPRS attach status
  bool alreadyAttached = false;
  _modem->flushSerial();
  _modem->getSerial().println("AT+CGATT?");

  uint32_t start = millis();
  while (millis() - start < 2000) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');
      if (line.indexOf("+CGATT: 1") >= 0) {
        alreadyAttached = true;
        GL868_ESP32_LOG_D("GPRS attach status: attached");
        break;
      } else if (line.indexOf("+CGATT: 0") >= 0) {
        GL868_ESP32_LOG_D("GPRS attach status: not attached");
        break;
      }
    }
    yield();
  }

  // Try to attach GPRS with retry
  for (int attempt = 0; attempt < 3; attempt++) {
    if (attempt > 0) {
      GL868_ESP32_LOG_W("GPRS attach retry %d", attempt + 1);
      // Detach first before retry
      _modem->sendAT("+CGATT=0", 5000);
      delay(1000);
    }

    // Attempt explicit GPRS attach (non-fatal if it fails)
    // On roaming networks, CGATT=1 often returns ERROR but the modem
    // auto-attaches when APN is set via CSTT/CIICR
    if (!alreadyAttached || attempt > 0) {
      if (!_modem->sendAT("+CGATT=1", 30000)) {
        GL868_ESP32_LOG_W("CGATT=1 failed (may auto-attach with APN)");
        // Don't retry CGATT - proceed with APN setup anyway
      }
    }

    // Flush any URCs (like +CREG:) that arrived during CGATT
    _modem->flushSerial();

    // Set APN using runtime configuration
    char apnCmd[128];
    if (_apnUser[0] != '\0' && _apnPass[0] != '\0') {
      snprintf(apnCmd, sizeof(apnCmd), "+CSTT=\"%s\",\"%s\",\"%s\"", _apn,
               _apnUser, _apnPass);
    } else {
      snprintf(apnCmd, sizeof(apnCmd), "+CSTT=\"%s\"", _apn);
    }
    if (!_modem->sendAT(apnCmd, 5000)) {
      GL868_ESP32_LOG_W("Failed to set APN");
      continue;
    }

    // Set single connection mode
    _modem->sendAT("+CIPMUX=0", 1000);

    _gprsAttached = true;
    GL868_ESP32_LOG_I("GPRS attached (APN: %s)", _apn);
    return true;
  }

  GL868_ESP32_LOG_E("GPRS attach failed after retries");
  return false;
}

bool GL868_ESP32_GSM::detachGPRS() {
  if (!_modem)
    return false;

  if (_modem->sendAT("+CGATT=0", 5000)) {
    _gprsAttached = false;
    GL868_ESP32_LOG_I("GPRS detached");
    return true;
  }

  return false;
}

bool GL868_ESP32_GSM::startTCPIP() {
  if (!_modem || !_gprsAttached)
    return false;

  GL868_ESP32_LOG_D("Starting TCP/IP stack...");

  // Flush any leftover data from serial buffer first
  _modem->flushSerial();

  // Verify modem is still responsive
  if (!_modem->isResponsive()) {
    GL868_ESP32_LOG_W("Modem not responsive before TCP/IP start");
    return false;
  }

  // First, shut down any existing IP connection to reset state
  // This fixes "ERROR" response when PDP context is stale
  _modem->sendAT("+CIPSHUT", 5000);

  // Flush again and delay after shutdown
  _modem->flushSerial();
  uint32_t delayStart = millis();
  while (millis() - delayStart < 500)
    yield();

  // Re-set APN using runtime configuration
  char apnCmd[128];
  if (_apnUser[0] != '\0' && _apnPass[0] != '\0') {
    snprintf(apnCmd, sizeof(apnCmd), "+CSTT=\"%s\",\"%s\",\"%s\"", _apn,
             _apnUser, _apnPass);
  } else {
    snprintf(apnCmd, sizeof(apnCmd), "+CSTT=\"%s\"", _apn);
  }
  _modem->sendAT(apnCmd, 5000);

  // Bring up wireless connection
  if (!_modem->sendAT("+CIICR", 30000)) {
    GL868_ESP32_LOG_E("Failed to bring up wireless");
    return false;
  }

  // Get local IP
  _modem->flushSerial();
  _modem->getSerial().println("AT+CIFSR");

  uint32_t start = millis();
  while (millis() - start < 5000) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');
      line.trim();

      // IP address should be a dotted quad
      if (line.length() > 0 && line[0] >= '0' && line[0] <= '9') {
        GL868_ESP32_LOG_I("Got IP: %s", line.c_str());
        return true;
      }
    }
    yield();
  }

  GL868_ESP32_LOG_E("Failed to get IP address");
  return false;
}

void GL868_ESP32_GSM::stopTCPIP() {
  if (!_modem)
    return;

  disconnectTCP();
  _modem->sendAT("+CIPSHUT", 5000);
  GL868_ESP32_LOG_D("TCP/IP stack stopped");
}

bool GL868_ESP32_GSM::parseURL(const char *url, String &host, uint16_t &port,
                               String &path, bool &useSSL) {
  String urlStr = url;

  // Determine protocol
  if (urlStr.startsWith("https://")) {
    useSSL = true;
    urlStr = urlStr.substring(8);
    port = 443;
  } else if (urlStr.startsWith("http://")) {
    useSSL = false;
    urlStr = urlStr.substring(7);
    port = 80;
  } else {
    return false;
  }

  // Find path
  int pathStart = urlStr.indexOf('/');
  if (pathStart >= 0) {
    path = urlStr.substring(pathStart);
    urlStr = urlStr.substring(0, pathStart);
  } else {
    path = "/";
  }

  // Check for port in host
  int portStart = urlStr.indexOf(':');
  if (portStart >= 0) {
    port = urlStr.substring(portStart + 1).toInt();
    urlStr = urlStr.substring(0, portStart);
  }

  host = urlStr;
  return true;
}

bool GL868_ESP32_GSM::connectTCP(const char *host, uint16_t port, bool useSSL) {
  if (!_modem)
    return false;

  GL868_ESP32_LOG_D("Connecting to %s:%u (SSL: %d)", host, port, useSSL);

  // Set SSL mode if needed
  if (useSSL) {
    _modem->sendAT("+CIPSSL=1", 1000);
  } else {
    _modem->sendAT("+CIPSSL=0", 1000);
  }

  // Start TCP connection
  char cmd[80];
  snprintf(cmd, sizeof(cmd), "+CIPSTART=\"TCP\",\"%s\",\"%u\"", host, port);

  _modem->flushSerial();
  _modem->getSerial().print("AT");
  _modem->getSerial().println(cmd);

  // Wait for CONNECT OK
  uint32_t start = millis();
  while (millis() - start < 30000) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');
      line.trim();

      GL868_ESP32_LOG_V("<< %s", line.c_str());

      if (line == "CONNECT OK" || line.indexOf("CONNECT OK") >= 0) {
        _tcpConnected = true;
        GL868_ESP32_LOG_I("TCP connected");
        return true;
      }
      if (line.indexOf("CONNECT FAIL") >= 0 || line.indexOf("CLOSED") >= 0 ||
          line.indexOf("ERROR") >= 0 || line.indexOf("FAIL") >= 0) {
        GL868_ESP32_LOG_W("TCP connection failed: %s", line.c_str());
        return false;
      }
    }
    yield();
  }

  GL868_ESP32_LOG_W("TCP connection timeout");
  return false;
}

void GL868_ESP32_GSM::disconnectTCP() {
  if (!_modem)
    return;

  if (_tcpConnected) {
    _modem->sendAT("+CIPCLOSE", 3000);
    _tcpConnected = false;
    GL868_ESP32_LOG_D("TCP disconnected");
  }
}

int GL868_ESP32_GSM::httpPOST(const char *url, const char *apiKey,
                              const char *contentType, const char *body) {
  if (!_modem)
    return -1;

  String host, path;
  uint16_t port;
  bool useSSL;

  // Parse URL
  if (!parseURL(url, host, port, path, useSSL)) {
    GL868_ESP32_LOG_E("Invalid URL: %s", url);
    return -1;
  }

  // Make sure TCP/IP is started
  if (!startTCPIP()) {
    return -1;
  }

  // Try SSL connection first if URL is HTTPS
  bool connected = false;
  if (useSSL) {
    connected = connectTCP(host.c_str(), port, true);
    if (!connected) {
      GL868_ESP32_LOG_W("SSL connection failed, trying HTTP fallback...");
      // Fallback to HTTP endpoint
      stopTCPIP();

      // Use HTTP endpoint instead
      String httpUrl = GL868_ESP32_API_ENDPOINT_HTTP;
      String httpHost, httpPath;
      uint16_t httpPort;
      bool httpSSL;

      if (parseURL(httpUrl.c_str(), httpHost, httpPort, httpPath, httpSSL)) {
        host = httpHost;
        port = httpPort;
        path = httpPath;
        useSSL = false;

        // Restart TCP/IP for HTTP
        if (!startTCPIP()) {
          return -1;
        }
        connected = connectTCP(host.c_str(), port, false);
      }
    }
  } else {
    connected = connectTCP(host.c_str(), port, false);
  }

  if (!connected) {
    GL868_ESP32_LOG_E("Failed to connect to server");
    stopTCPIP();
    return -1;
  }

  // Build HTTP request
  size_t bodyLen = strlen(body);
  String request;
  request.reserve(256 + bodyLen);

  request += "POST ";
  request += path;
  request += " HTTP/1.1\r\n";
  request += "Host: ";
  request += host;
  request += "\r\n";
  request += "Authorization: ";
  request += apiKey;
  request += "\r\n";
  request += "Content-Type: ";
  request += contentType;
  request += "\r\n";
  request += "Content-Length: ";
  request += String(bodyLen);
  request += "\r\n";
  request += "Connection: close\r\n";
  request += "\r\n";
  request += body;

  // Send with CIPSEND
  char lenCmd[32];
  snprintf(lenCmd, sizeof(lenCmd), "+CIPSEND=%u", request.length());

  _modem->flushSerial();
  _modem->getSerial().print("AT");
  _modem->getSerial().println(lenCmd);

  // Wait for prompt
  uint32_t start = millis();
  bool gotPrompt = false;
  while (millis() - start < 5000) {
    if (_modem->getSerial().available()) {
      char c = _modem->getSerial().read();
      if (c == '>') {
        gotPrompt = true;
        break;
      }
    }
    yield();
  }

  if (!gotPrompt) {
    GL868_ESP32_LOG_E("No CIPSEND prompt");
    disconnectTCP();
    stopTCPIP();
    return -1;
  }

  // Send request data
  GL868_ESP32_LOG_V(">> [HTTP Request %u bytes]", request.length());
  _modem->getSerial().print(request);

  // Wait for SEND OK
  start = millis();
  while (millis() - start < 10000) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');
      line.trim();

      if (line == "SEND OK") {
        GL868_ESP32_LOG_D("HTTP request sent");
        break;
      }
      if (line.indexOf("ERROR") >= 0) {
        GL868_ESP32_LOG_E("Send error");
        disconnectTCP();
        stopTCPIP();
        return -1;
      }
    }
    yield();
  }

  // Read HTTP response
  int statusCode = readHTTPResponse();

  // Cleanup
  disconnectTCP();
  stopTCPIP();

  return statusCode;
}

int GL868_ESP32_GSM::httpPOSTRaw(const char *url, const char *body,
                                 const char *headers) {
  if (!_modem)
    return -1;

  String host, path;
  uint16_t port;
  bool useSSL;

  // Parse URL
  if (!parseURL(url, host, port, path, useSSL)) {
    GL868_ESP32_LOG_E("Invalid URL: %s", url);
    return -1;
  }

  // Make sure TCP/IP is started
  if (!startTCPIP()) {
    return -1;
  }

  // Connect to server
  if (!connectTCP(host.c_str(), port, useSSL)) {
    GL868_ESP32_LOG_E("Failed to connect to server");
    stopTCPIP();
    return -1;
  }

  // Build HTTP request
  size_t bodyLen = body ? strlen(body) : 0;
  String request;
  request.reserve(256 + bodyLen);

  request += "POST ";
  request += path;
  request += " HTTP/1.1\r\n";
  request += "Host: ";
  request += host;
  request += "\r\n";

  // Add custom headers if provided
  if (headers && strlen(headers) > 0) {
    request += headers;
    // Ensure headers end with \r\n
    if (!request.endsWith("\r\n")) {
      request += "\r\n";
    }
  }

  request += "Content-Length: ";
  request += String(bodyLen);
  request += "\r\n";
  request += "Connection: close\r\n";
  request += "\r\n";

  if (body) {
    request += body;
  }

  // Send with CIPSEND
  char lenCmd[32];
  snprintf(lenCmd, sizeof(lenCmd), "+CIPSEND=%u", request.length());

  _modem->flushSerial();
  _modem->getSerial().print("AT");
  _modem->getSerial().println(lenCmd);

  // Wait for prompt
  uint32_t start = millis();
  bool gotPrompt = false;
  while (millis() - start < 5000) {
    if (_modem->getSerial().available()) {
      char c = _modem->getSerial().read();
      if (c == '>') {
        gotPrompt = true;
        break;
      }
    }
    yield();
  }

  if (!gotPrompt) {
    GL868_ESP32_LOG_E("No CIPSEND prompt");
    disconnectTCP();
    stopTCPIP();
    return -1;
  }

  // Send request data
  GL868_ESP32_LOG_V(">> [HTTP Request %u bytes]", request.length());
  _modem->getSerial().print(request);

  // Wait for SEND OK
  start = millis();
  while (millis() - start < 10000) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');
      line.trim();

      if (line == "SEND OK") {
        GL868_ESP32_LOG_D("HTTP request sent");
        break;
      }
      if (line.indexOf("ERROR") >= 0) {
        GL868_ESP32_LOG_E("Send error");
        disconnectTCP();
        stopTCPIP();
        return -1;
      }
    }
    yield();
  }

  // Read HTTP response
  int statusCode = readHTTPResponse();

  // Cleanup
  disconnectTCP();
  stopTCPIP();

  return statusCode;
}

int GL868_ESP32_GSM::httpPOSTRaw(const char *url, const char *body,
                                 const char *headers, char *response,
                                 size_t responseMaxLen) {
  if (!_modem)
    return -1;

  String host, path;
  uint16_t port;
  bool useSSL;

  // Parse URL
  if (!parseURL(url, host, port, path, useSSL)) {
    GL868_ESP32_LOG_E("Invalid URL: %s", url);
    return -1;
  }

  // Make sure TCP/IP is started
  if (!startTCPIP()) {
    return -1;
  }

  // Connect to server
  if (!connectTCP(host.c_str(), port, useSSL)) {
    GL868_ESP32_LOG_E("Failed to connect to server");
    stopTCPIP();
    return -1;
  }

  // Build HTTP request
  size_t bodyLen = body ? strlen(body) : 0;
  String request;
  request.reserve(256 + bodyLen);

  request += "POST ";
  request += path;
  request += " HTTP/1.1\r\n";
  request += "Host: ";
  request += host;
  request += "\r\n";

  // Add custom headers if provided
  if (headers && strlen(headers) > 0) {
    request += headers;
    if (!request.endsWith("\r\n")) {
      request += "\r\n";
    }
  }

  request += "Content-Length: ";
  request += String(bodyLen);
  request += "\r\n";
  request += "Connection: close\r\n";
  request += "\r\n";

  if (body) {
    request += body;
  }

  // Send with CIPSEND
  char lenCmd[32];
  snprintf(lenCmd, sizeof(lenCmd), "+CIPSEND=%u", request.length());

  _modem->flushSerial();
  _modem->getSerial().print("AT");
  _modem->getSerial().println(lenCmd);

  // Wait for prompt
  uint32_t start = millis();
  bool gotPrompt = false;
  while (millis() - start < 5000) {
    if (_modem->getSerial().available()) {
      char c = _modem->getSerial().read();
      if (c == '>') {
        gotPrompt = true;
        break;
      }
    }
    yield();
  }

  if (!gotPrompt) {
    GL868_ESP32_LOG_E("No CIPSEND prompt");
    disconnectTCP();
    stopTCPIP();
    return -1;
  }

  // Send request data
  GL868_ESP32_LOG_V(">> [HTTP Request %u bytes]", request.length());
  _modem->getSerial().print(request);

  // Wait for SEND OK
  start = millis();
  while (millis() - start < 10000) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');
      line.trim();

      if (line == "SEND OK") {
        GL868_ESP32_LOG_D("HTTP request sent");
        break;
      }
      if (line.indexOf("ERROR") >= 0) {
        GL868_ESP32_LOG_E("Send error");
        disconnectTCP();
        stopTCPIP();
        return -1;
      }
    }
    yield();
  }

  // Read HTTP response with body capture
  int statusCode = readHTTPResponse(response, responseMaxLen);

  // Cleanup
  disconnectTCP();
  stopTCPIP();

  return statusCode;
}

int GL868_ESP32_GSM::readHTTPResponse() {
  if (!_modem)
    return -1;

  GL868_ESP32_LOG_D("Reading HTTP response...");

  String responseBody;
  int statusCode = -1;
  bool inBody = false;
  int contentLength = -1;
  int bodyBytesRead = 0;

  uint32_t start = millis();
  while (millis() - start < GL868_ESP32_HTTP_RESPONSE_TIMEOUT) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');
      line.trim();

      // Log all response lines in verbose mode
      GL868_ESP32_LOG_V("<< %s", line.c_str());

      // Parse HTTP status line
      if (line.startsWith("HTTP/1.") && statusCode == -1) {
        int spacePos = line.indexOf(' ');
        if (spacePos > 0) {
          statusCode = line.substring(spacePos + 1, spacePos + 4).toInt();
          GL868_ESP32_LOG_I("HTTP Status: %d", statusCode);
        }
      }

      // Parse Content-Length header
      if (line.startsWith("Content-Length:") ||
          line.startsWith("content-length:")) {
        contentLength = line.substring(15).toInt();
        GL868_ESP32_LOG_V("Content-Length: %d", contentLength);
      }

      // Detect body start (empty line after headers)
      if (line.length() == 0 && !inBody) {
        inBody = true;
        GL868_ESP32_LOG_V("--- Response Body ---");
        continue;
      }

      // Capture response body
      if (inBody && line.length() > 0 && line != "CLOSED") {
        responseBody += line;
        bodyBytesRead += line.length();
      }

      // Check for CLOSED
      if (line == "CLOSED") {
        GL868_ESP32_LOG_V("--- End Response ---");
        break;
      }
    }
    yield();
  }

  // Log the complete response body in verbose mode
  if (responseBody.length() > 0) {
    GL868_ESP32_LOG_V("Server Response: %s", responseBody.c_str());
  }

  // Determine success/failure based on HTTP code
  if (statusCode >= 200 && statusCode < 300) {
    GL868_ESP32_LOG_I("HTTP Success: %d - Data sent successfully", statusCode);
  } else if (statusCode >= 400 && statusCode < 500) {
    GL868_ESP32_LOG_W("HTTP Client Error: %d - %s", statusCode,
                      responseBody.c_str());
  } else if (statusCode >= 500) {
    GL868_ESP32_LOG_W("HTTP Server Error: %d - %s", statusCode,
                      responseBody.c_str());
  } else if (statusCode == -1) {
    GL868_ESP32_LOG_W("No HTTP response received");
  }

  return statusCode;
}

int GL868_ESP32_GSM::readHTTPResponse(char *responseBuffer, size_t bufferLen) {
  if (!_modem)
    return -1;

  GL868_ESP32_LOG_D("Reading HTTP response (with capture)...");

  size_t responsePos = 0;
  if (responseBuffer && bufferLen > 0) {
    responseBuffer[0] = '\0';
  }

  int statusCode = -1;
  bool inBody = false;

  uint32_t start = millis();
  while (millis() - start < GL868_ESP32_HTTP_RESPONSE_TIMEOUT) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');
      line.trim();

      // Log all response lines in verbose mode
      GL868_ESP32_LOG_V("<< %s", line.c_str());

      // Parse HTTP status line
      if (line.startsWith("HTTP/1.") && statusCode == -1) {
        int spacePos = line.indexOf(' ');
        if (spacePos > 0) {
          statusCode = line.substring(spacePos + 1, spacePos + 4).toInt();
          GL868_ESP32_LOG_I("HTTP Status: %d", statusCode);
        }
      }

      // Detect body start (empty line after headers)
      if (line.length() == 0 && !inBody) {
        inBody = true;
        GL868_ESP32_LOG_V("--- Response Body ---");
        continue;
      }

      // Capture response body to buffer
      if (inBody && line.length() > 0 && line != "CLOSED") {
        if (responseBuffer && responsePos < bufferLen - 1) {
          size_t copyLen = min(line.length(), bufferLen - 1 - responsePos);
          strncpy(responseBuffer + responsePos, line.c_str(), copyLen);
          responsePos += copyLen;
          responseBuffer[responsePos] = '\0';
        }
      }

      // Check for CLOSED
      if (line == "CLOSED") {
        GL868_ESP32_LOG_V("--- End Response ---");
        break;
      }
    }
    yield();
  }

  // Log the captured response
  if (responseBuffer && responsePos > 0) {
    GL868_ESP32_LOG_I("Server Response: %s", responseBuffer);
  }

  // Determine success/failure based on HTTP code
  if (statusCode >= 200 && statusCode < 300) {
    GL868_ESP32_LOG_I("HTTP Success: %d", statusCode);
  } else if (statusCode >= 400 && statusCode < 500) {
    GL868_ESP32_LOG_W("HTTP Client Error: %d", statusCode);
  } else if (statusCode >= 500) {
    GL868_ESP32_LOG_W("HTTP Server Error: %d", statusCode);
  } else if (statusCode == -1) {
    GL868_ESP32_LOG_W("No HTTP response received");
  }

  return statusCode;
}

int GL868_ESP32_GSM::getSignalStrength() {
  if (!_modem || !_modem->isResponsive())
    return 99;

  _modem->flushSerial();
  _modem->getSerial().println("AT+CSQ");

  uint32_t start = millis();
  while (millis() - start < 2000) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');

      // Parse +CSQ: rssi,ber
      if (line.startsWith("+CSQ:")) {
        int commaPos = line.indexOf(',');
        if (commaPos > 0) {
          int rssi = line.substring(6, commaPos).toInt();
          GL868_ESP32_LOG_V("Signal strength: %d", rssi);
          return rssi;
        }
      }
    }
    yield();
  }

  return 99;
}

String GL868_ESP32_GSM::getOperator() {
  if (!_modem || !_modem->isResponsive())
    return "";

  _modem->flushSerial();
  _modem->getSerial().println("AT+COPS?");

  uint32_t start = millis();
  while (millis() - start < 2000) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');

      // Parse +COPS: mode,format,"operator"
      if (line.startsWith("+COPS:")) {
        int quoteStart = line.indexOf('"');
        int quoteEnd = line.lastIndexOf('"');
        if (quoteStart > 0 && quoteEnd > quoteStart) {
          return line.substring(quoteStart + 1, quoteEnd);
        }
      }
    }
    yield();
  }

  return "";
}

String GL868_ESP32_GSM::getIMEI() {
  if (!_modem || !_modem->isResponsive())
    return "";

  _modem->flushSerial();
  _modem->getSerial().println("AT+GSN");

  uint32_t start = millis();
  while (millis() - start < 2000) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');
      line.trim();

      // IMEI is a 15-digit number
      if (line.length() == 15 && isDigit(line[0])) {
        GL868_ESP32_LOG_D("IMEI: %s", line.c_str());
        return line;
      }
    }
    yield();
  }

  return "";
}

String GL868_ESP32_GSM::getIMSI() {
  if (!_modem || !_modem->isResponsive())
    return "";

  _modem->flushSerial();
  _modem->getSerial().println("AT+CIMI");

  uint32_t start = millis();
  while (millis() - start < 2000) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');
      line.trim();

      // IMSI is a 15-digit number
      if (line.length() >= 14 && line.length() <= 15 && isDigit(line[0])) {
        GL868_ESP32_LOG_D("IMSI: %s", line.c_str());
        return line;
      }
    }
    yield();
  }

  return "";
}

String GL868_ESP32_GSM::getICCID() {
  if (!_modem || !_modem->isResponsive())
    return "";

  // Try AT+CCID first (SIM868 command)
  _modem->flushSerial();
  _modem->getSerial().println("AT+CCID");

  uint32_t start = millis();
  while (millis() - start < 2000) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');
      line.trim();

      // Check for numeric string of appropriate length (ICCID is typically
      // 19-20 digits)
      if (line.length() >= 19 && isDigit(line[0])) {
        // Return full string up to 20 chars
        String iccid = line.substring(0, min((unsigned int)line.length(), 20u));
        GL868_ESP32_LOG_D("ICCID: %s", iccid.c_str());
        return iccid;
      }

      // Some modems return +CCID: <iccid>
      if (line.startsWith("+CCID:")) {
        String iccid = line.substring(7);
        iccid.trim();
        if (iccid.length() >= 19) {
          iccid = iccid.substring(0, min((unsigned int)iccid.length(), 20u));
        }
        GL868_ESP32_LOG_D("ICCID: %s", iccid.c_str());
        return iccid;
      }
    }
    yield();
  }

  return "";
}

String GL868_ESP32_GSM::getPhoneNumber() {
  if (!_modem || !_modem->isResponsive())
    return "";

  _modem->flushSerial();
  _modem->getSerial().println("AT+CNUM");

  uint32_t start = millis();
  while (millis() - start < 2000) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');
      line.trim();

      // Parse +CNUM: "name","number",type
      if (line.startsWith("+CNUM:")) {
        int firstQuote = line.indexOf('"');
        int secondQuote = line.indexOf('"', firstQuote + 1);
        int thirdQuote = line.indexOf('"', secondQuote + 1);
        int fourthQuote = line.indexOf('"', thirdQuote + 1);

        if (thirdQuote >= 0 && fourthQuote > thirdQuote) {
          String number = line.substring(thirdQuote + 1, fourthQuote);
          GL868_ESP32_LOG_D("Phone: %s", number.c_str());
          return number;
        }
      }
    }
    yield();
  }

  // Phone number not stored on SIM
  return "";
}

void GL868_ESP32_GSM::setAPN(const char *apn) {
  if (apn && strlen(apn) < sizeof(_apn)) {
    strncpy(_apn, apn, sizeof(_apn) - 1);
    _apn[sizeof(_apn) - 1] = '\0';
    _apnUser[0] = '\0';
    _apnPass[0] = '\0';
    GL868_ESP32_LOG_I("APN set to: %s", _apn);
  }
}

void GL868_ESP32_GSM::setAPN(const char *apn, const char *user,
                             const char *pass) {
  if (apn && strlen(apn) < sizeof(_apn)) {
    strncpy(_apn, apn, sizeof(_apn) - 1);
    _apn[sizeof(_apn) - 1] = '\0';
  }
  if (user && strlen(user) < sizeof(_apnUser)) {
    strncpy(_apnUser, user, sizeof(_apnUser) - 1);
    _apnUser[sizeof(_apnUser) - 1] = '\0';
  }
  if (pass && strlen(pass) < sizeof(_apnPass)) {
    strncpy(_apnPass, pass, sizeof(_apnPass) - 1);
    _apnPass[sizeof(_apnPass) - 1] = '\0';
  }
  GL868_ESP32_LOG_I("APN set to: %s (with auth)", _apn);
}
