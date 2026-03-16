/*
 * GL868_ESP32_SMS.cpp
 * SMS control plane implementation
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#include "GL868_ESP32_SMS.h"
#include "GL868_ESP32_Logger.h"

GL868_ESP32_SMS::GL868_ESP32_SMS()
    : _modem(nullptr), _whitelist(nullptr), _enabled(false),
      _builtinEnabled(true), _pendingSMS(false), _handlerCount(0),
      _actionCallback(nullptr), _ledCallback(nullptr), _lastCheck(0) {}

void GL868_ESP32_SMS::setLEDCallback(LEDCallback callback) {
  _ledCallback = callback;
}

void GL868_ESP32_SMS::begin() {
  if (!_modem)
    return;

  // Set SMS text mode
  _modem->sendAT("+CMGF=1", 2000);

  // Set character set
  _modem->sendAT("+CSCS=\"GSM\"", 2000);

  // Enable new SMS indication
  _modem->sendAT("+CNMI=2,1,0,0,0", 2000);

  // NOTE: Do NOT delete SMS here. On a Call/SMS wake the incoming message
  // (e.g. "LOC") triggered the wake — deleting it before IDLE is reached
  // would lose the command. Messages are deleted individually after
  // processing in parseIncomingSMS(), which is sufficient cleanup.

  GL868_ESP32_LOG_I("SMS system initialized");
}

void GL868_ESP32_SMS::end() {
  _enabled = false;
  GL868_ESP32_LOG_D("SMS system disabled");
}

void GL868_ESP32_SMS::enable(bool enabled) {
  _enabled = enabled;
  if (enabled) {
    begin();
  }
  GL868_ESP32_LOG_I("SMS %s", enabled ? "enabled" : "disabled");
}

bool GL868_ESP32_SMS::registerHandler(const char *command, SMSHandler handler) {
  if (_handlerCount >= GL868_ESP32_MAX_SMS_HANDLERS) {
    GL868_ESP32_LOG_E("SMS handler limit reached");
    return false;
  }

  strncpy(_customCommands[_handlerCount], command,
          GL868_ESP32_MAX_SMS_CMD_LEN - 1);
  _customCommands[_handlerCount][GL868_ESP32_MAX_SMS_CMD_LEN - 1] = '\0';

  // Convert to uppercase
  for (char *p = _customCommands[_handlerCount]; *p; p++) {
    *p = toupper(*p);
  }

  _customHandlers[_handlerCount] = handler;
  _handlerCount++;

  GL868_ESP32_LOG_D("Registered SMS handler: %s", command);
  return true;
}

void GL868_ESP32_SMS::update() {
  if (!_enabled || !_modem)
    return;

  // Rate limit checks
  if (millis() - _lastCheck < 2000)
    return;
  _lastCheck = millis();

  parseIncomingSMS();
}

void GL868_ESP32_SMS::parseIncomingSMS() {
  // List unread messages
  _modem->flushSerial();
  _modem->getSerial().println("AT+CMGL=\"REC UNREAD\"");

  String response;
  bool inMessage = false;
  String sender;
  String messageBody;
  int msgIndex = -1;

  uint32_t start = millis();
  while (millis() - start < 5000) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');
      line.trim();

      if (line.startsWith("+CMGL:")) {
        // Parse: +CMGL: index,"status","sender",,"timestamp"
        int firstComma = line.indexOf(',');
        if (firstComma > 0) {
          msgIndex = line.substring(7, firstComma).toInt();

          // Find sender in quotes
          int firstQuote = line.indexOf('"', firstComma);
          int secondQuote = line.indexOf('"', firstQuote + 1);
          int thirdQuote = line.indexOf('"', secondQuote + 1);
          int fourthQuote = line.indexOf('"', thirdQuote + 1);

          if (thirdQuote > 0 && fourthQuote > thirdQuote) {
            sender = line.substring(thirdQuote + 1, fourthQuote);
          }
        }
        inMessage = true;
        messageBody = "";
      } else if (inMessage && line.length() > 0 && line != "OK") {
        messageBody = line;
        inMessage = false;

        GL868_ESP32_LOG_I("SMS from %s: %s", sender.c_str(),
                          messageBody.c_str());

        // Signal LED: SMS received
        if (_ledCallback)
          _ledCallback(LED_SMS_RECEIVED);

        // Process this message
        processIncoming(sender.c_str(), messageBody.c_str());

        // Delete processed message
        if (msgIndex >= 0) {
          deleteSMS(msgIndex);
        }

        _pendingSMS = true;
      } else if (line == "OK") {
        break;
      }
    }
    yield();
  }
}

void GL868_ESP32_SMS::processIncoming(const char *sender, const char *message) {
  // Check whitelist
  if (_whitelist && !_whitelist->isAllowed(sender)) {
    GL868_ESP32_LOG_I("SMS from non-whitelisted number: %s", sender);
    return;
  }

  // Parse command and arguments
  String msg = message;
  msg.trim();
  msg.toUpperCase();

  String cmd = msg;
  String args = "";

  int spacePos = msg.indexOf(' ');
  if (spacePos > 0) {
    cmd = msg.substring(0, spacePos);
    args = msg.substring(spacePos + 1);
  }

  int eqPos = msg.indexOf('=');
  if (eqPos > 0) {
    cmd = msg.substring(0, eqPos);
    args = msg.substring(eqPos + 1);
  }

  GL868_ESP32_LOG_D("SMS command: %s, args: %s", cmd.c_str(), args.c_str());

  // Try built-in commands first
  if (_builtinEnabled &&
      handleBuiltinCommand(sender, cmd.c_str(), args.c_str())) {
    return;
  }

  // Try custom handlers
  for (uint8_t i = 0; i < _handlerCount; i++) {
    if (strcmp(_customCommands[i], cmd.c_str()) == 0) {
      _customHandlers[i](sender, cmd.c_str(), args.c_str());
      return;
    }
  }

  GL868_ESP32_LOG_D("Unknown SMS command: %s", cmd.c_str());
}

bool GL868_ESP32_SMS::handleBuiltinCommand(const char *sender, const char *cmd,
                                           const char *args) {
  if (strcmp(cmd, "STATUS") == 0) {
    // Request status via callback
    if (_actionCallback) {
      _actionCallback("STATUS", sender);
    }
    return true;
  }

  if (strcmp(cmd, "LOC") == 0) {
    // Request location via callback
    if (_actionCallback) {
      _actionCallback("LOC", sender);
    }
    return true;
  }

  if (strcmp(cmd, "SEND") == 0) {
    // Force data send
    if (_actionCallback) {
      _actionCallback("SEND", "");
    }
    send(sender, "Data send requested");
    return true;
  }

  if (strcmp(cmd, "SLEEP") == 0) {
    // Request sleep
    if (_actionCallback) {
      _actionCallback("SLEEP", "");
    }
    send(sender, "Entering sleep mode");
    return true;
  }

  if (strcmp(cmd, "INTERVAL") == 0 && strlen(args) > 0) {
    // Set send interval
    if (_actionCallback) {
      _actionCallback("INTERVAL", args);
    }
    char reply[64];
    snprintf(reply, sizeof(reply), "Interval set to %s min", args);
    send(sender, reply);
    return true;
  }

  // Whitelist commands (admin only)
  if (_whitelist && strncmp(cmd, "WL", 2) == 0) {
    // Check if admin
    if (!_whitelist->isAdmin(sender)) {
      send(sender, "Not authorized");
      return true;
    }

    if (strcmp(cmd, "WL") == 0 && strlen(args) > 0) {
      String argStr = args;

      if (argStr.startsWith("ADD ")) {
        String num = argStr.substring(4);
        if (_whitelist->add(num.c_str())) {
          send(sender, "Number added");
        } else {
          send(sender, "Failed to add");
        }
        return true;
      }

      if (argStr.startsWith("DEL ")) {
        String num = argStr.substring(4);
        if (_whitelist->remove(num.c_str())) {
          send(sender, "Number removed");
        } else {
          send(sender, "Failed to remove");
        }
        return true;
      }

      if (argStr == "LIST") {
        String list = "Whitelist:\n";
        for (uint8_t i = 0; i < _whitelist->count(); i++) {
          list += _whitelist->get(i);
          if (_whitelist->isAdmin(_whitelist->get(i))) {
            list += " (admin)";
          }
          list += "\n";
        }
        send(sender, list.c_str());
        return true;
      }

      if (argStr == "CLEAR") {
        _whitelist->clear();
        send(sender, "Whitelist cleared");
        return true;
      }
    }
  }

  return false;
}

bool GL868_ESP32_SMS::send(const char *number, const char *message) {
  if (!_modem)
    return false;

  GL868_ESP32_LOG_I("Sending SMS to %s: %s", number, message);

  // Signal LED: SMS sending
  if (_ledCallback)
    _ledCallback(LED_SMS_SENDING);

  // Set message destination
  char cmd[64];
  snprintf(cmd, sizeof(cmd), "+CMGS=\"%s\"", number);

  _modem->flushSerial();
  _modem->getSerial().print("AT");
  _modem->getSerial().println(cmd);

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
    GL868_ESP32_LOG_E("No SMS prompt");
    return false;
  }

  // Send message
  _modem->getSerial().print(message);
  _modem->getSerial().write(0x1A); // Ctrl+Z to send

  // Wait for +CMGS response
  start = millis();
  while (millis() - start < 30000) {
    if (_modem->getSerial().available()) {
      String line = _modem->getSerial().readStringUntil('\n');
      line.trim();

      if (line.startsWith("+CMGS:")) {
        // Restore LED to idle
        if (_ledCallback)
          _ledCallback(LED_IDLE);

        GL868_ESP32_LOG_I("SMS sent successfully");

        // Delete all SMS (inbox + sent + unsent) to free SIM868 memory
        delay(500); // Small delay to let modem finish processing
        deleteAllSMS();
        GL868_ESP32_LOG_D("SIM868 SMS memory cleaned after send");

        return true;
      }
      if (line.indexOf("ERROR") >= 0) {
        GL868_ESP32_LOG_E("SMS send error");
        return false;
      }
    }
    yield();
  }

  GL868_ESP32_LOG_E("SMS send timeout");
  return false;
}

bool GL868_ESP32_SMS::deleteSMS(int index) {
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "+CMGD=%d", index);
  return _modem->sendAT(cmd, 2000);
}

void GL868_ESP32_SMS::deleteAllSMS() {
  if (!_modem)
    return;

  // Delete all messages (index 1-4 = different types)
  _modem->sendAT("+CMGD=1,4", 5000);
}
