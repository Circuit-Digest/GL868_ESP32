/*
 * GL868_ESP32_Modem.h
 * SIM868 modem driver for power control and AT commands
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#ifndef GL868_ESP32_MODEM_H
#define GL868_ESP32_MODEM_H

#include "GL868_ESP32_Config.h"
#include "GL868_ESP32_Types.h"
#include <Arduino.h>

class GL868_ESP32_Modem {
public:
  GL868_ESP32_Modem();

  /**
   * Initialize modem UART and control pins
   */
  void begin();

  /**
   * Power on modem using PWRKEY
   * @return true if modem responds to AT
   */
  bool powerOn();

  /**
   * Power off modem using PWRKEY
   * @return true if power off successful
   */
  bool powerOff();

  /**
   * Set modem functionality mode using AT+CFUN
   * @param mode Power mode to set
   * @return true if successful
   */
  bool setFunctionality(ModemPowerMode mode);

  /**
   * Check if modem is responsive
   * @return true if modem responds to AT
   */
  bool isResponsive();

  /**
   * Check if modem is powered on
   */
  bool isPowered() const { return _powered; }

  /**
   * Get current power mode
   */
  ModemPowerMode getPowerMode() const { return _powerMode; }

  /**
   * Send AT command and wait for OK
   * @param cmd AT command (without AT prefix)
   * @param timeout Response timeout in ms
   * @return true if command succeeded
   */
  bool sendAT(const char *cmd,
              uint32_t timeout = GL868_ESP32_AT_RESPONSE_TIMEOUT);

  /**
   * Send raw command string
   * @param cmd Full command string
   * @param timeout Response timeout in ms
   * @return true if command succeeded
   */
  bool sendCommand(const char *cmd,
                   uint32_t timeout = GL868_ESP32_AT_RESPONSE_TIMEOUT);

  /**
   * Wait for specific response
   * @param expected Expected response string
   * @param timeout Timeout in ms
   * @return true if response received
   */
  bool waitResponse(const char *expected,
                    uint32_t timeout = GL868_ESP32_AT_RESPONSE_TIMEOUT);

  /**
   * Wait for OK or ERROR response
   * @param timeout Timeout in ms
   * @return true if OK received, false if ERROR or timeout
   */
  bool waitOK(uint32_t timeout = GL868_ESP32_AT_RESPONSE_TIMEOUT);

  /**
   * Get last response
   */
  String getResponse() const { return _response; }

  /**
   * Send AT command and get response into buffer
   * @param cmd AT command
   * @param buffer Buffer for response
   * @param len Buffer length
   * @param timeout Response timeout
   * @return true if successful
   */
  bool sendATGetResponse(const char *cmd, char *buffer, size_t len,
                         uint32_t timeout = GL868_ESP32_AT_RESPONSE_TIMEOUT);

  /**
   * Flush serial buffer
   */
  void flushSerial();

  /**
   * Get reference to serial for direct access
   */
  HardwareSerial &getSerial() { return Serial2; }

private:
  void pulseKey(uint32_t duration);
  bool waitForBoot();

  bool _powered;
  ModemPowerMode _powerMode;
  String _response;
  uint32_t _lastCommandTime;
};

#endif // GL868_ESP32_MODEM_H
