/*
 * GL868_ESP32_GSM.h
 * GSM layer for network registration, GPRS, and HTTP
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_GSM_H
#define GL868_ESP32_GSM_H

#include "GL868_ESP32_Config.h"
#include "GL868_ESP32_Modem.h"
#include "GL868_ESP32_Types.h"
#include <Arduino.h>

// Forward declaration
class GL868_ESP32_Modem;

class GL868_ESP32_GSM {
public:
  GL868_ESP32_GSM();

  /**
   * Set modem reference
   */
  void setModem(GL868_ESP32_Modem *modem) { _modem = modem; }

  /**
   * Check if SIM card is ready
   * @return true if SIM is ready
   */
  bool checkSIMReady();

  /**
   * Wait for network registration
   * @param timeout Maximum wait time in ms
   * @return true if registered
   */
  bool
  waitNetworkRegistration(uint32_t timeout = GL868_ESP32_NETWORK_REG_TIMEOUT);

  /**
   * Attach to GPRS
   * @return true if attached
   */
  bool attachGPRS();

  /**
   * Detach from GPRS
   * @return true if detached
   */
  bool detachGPRS();

  /**
   * Start TCP/IP stack for HTTP
   * @return true if successful
   */
  bool startTCPIP();

  /**
   * Stop TCP/IP stack
   */
  void stopTCPIP();

  /**
   * HTTP POST using CIPSEND
   * @param url URL to post to
   * @param apiKey API key for authorization
   * @param contentType Content-Type header
   * @param body Request body
   * @return HTTP status code, or -1 on error
   */
  int httpPOST(const char *url, const char *apiKey, const char *contentType,
               const char *body);

  /**
   * Raw HTTP POST with custom headers
   * Allows sending any data to any server with custom headers
   * @param url Full URL (e.g., "http://httpbin.org/post")
   * @param body Request body (any format: JSON, form-data, raw text)
   * @param headers Custom headers as key:value pairs separated by \r\n
   *                Example: "Content-Type: application/json\r\nAuthorization:
   * Bearer token" If nullptr, only Host and Content-Length are sent
   * @return HTTP status code (200, 404, etc.) or -1 on connection error
   */
  int httpPOSTRaw(const char *url, const char *body,
                  const char *headers = nullptr);

  /**
   * Raw HTTP POST with custom headers and response capture
   * @param url Full URL
   * @param body Request body
   * @param headers Custom headers (nullptr for defaults)
   * @param response Buffer to store server response body
   * @param responseMaxLen Maximum length of response buffer
   * @return HTTP status code or -1 on error
   */
  int httpPOSTRaw(const char *url, const char *body, const char *headers,
                  char *response, size_t responseMaxLen);

  /**
   * Get signal strength (CSQ value)
   * @return Signal strength 0-31, or 99 if unknown
   */
  int getSignalStrength();

  /**
   * Check if registered to network
   */
  bool isRegistered() const { return _registered; }

  /**
   * Get the numerical registration status from AT+CREG?
   * 0: Not searching, 1: Registered home, 2: Searching, 3: Denied, 4: Unknown,
   * 5: Registered roaming
   */
  int getRegistrationStatus() const { return _lastRegStatus; }

  /**
   * Check if GPRS is attached
   */
  bool isGPRSAttached() const { return _gprsAttached; }

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
   * Set APN for GPRS connection (call before attachGPRS)
   * @param apn APN name (e.g., "internet", "iot.com")
   */
  void setAPN(const char *apn);

  /**
   * Set APN with authentication (for carriers that require username/password)
   * @param apn APN name
   * @param user Username
   * @param pass Password
   */
  void setAPN(const char *apn, const char *user, const char *pass);

  /**
   * Get current APN
   */
  const char *getAPN() const { return _apn; }

private:
  bool parseURL(const char *url, String &host, uint16_t &port, String &path,
                bool &useSSL);
  bool connectTCP(const char *host, uint16_t port, bool useSSL);
  void disconnectTCP();
  int readHTTPResponse();
  int readHTTPResponse(char *responseBuffer, size_t bufferLen);

  GL868_ESP32_Modem *_modem;
  bool _registered;
  int _lastRegStatus; // Numeric status from +CREG
  bool _gprsAttached;
  bool _tcpConnected;

  // Runtime APN configuration
  char _apn[64];
  char _apnUser[32];
  char _apnPass[32];
};

#endif // GL868_ESP32_GSM_H
