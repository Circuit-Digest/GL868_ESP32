/*
 * GL868_ESP32_LED.cpp
 * Non-blocking LED manager implementation using WS2812B with ESP32 RMT
 *
 * Copyright (c) 2026 Jobit Joseph and Semicon Media
 * https://github.com/jobitjoseph/GL868_ESP32
 * Licensed under MIT License
 */

#include "GL868_ESP32_LED.h"
#include "GL868_ESP32_Logger.h"

// ============================================================================
// Colour palette (R, G, B)
// ============================================================================
#define COLOR_OFF 0, 0, 0
#define COLOR_YELLOW 255, 255, 0
#define COLOR_MAGENTA 255, 0, 255
#define COLOR_BLUE 0, 0, 255
#define COLOR_WHITE 255, 255, 255
#define COLOR_PINK 255, 105, 180
#define COLOR_RED 255, 0, 0

// ============================================================================
// Constructor
// ============================================================================
GL868_ESP32_LED::GL868_ESP32_LED()
    : _state(LED_OFF), _errorCode(ERROR_NONE), _lastUpdate(0),
      _stateBlinkOn(false), _errBlinksDone(0), _errBlinkOn(false),
      _errorLoopCount(0), _errorBlinksComplete(false), _rgbOn(false),
      _powerEnabled(false), _taskHandle(NULL), _currentR(0), _currentG(0),
      _currentB(0), _brightness(25), _targetBlinks(0) {}

// ============================================================================
// Lifecycle
// ============================================================================
void GL868_ESP32_LED::begin() {
  // Status LED — initialise LOW, then hand over to the user
  pinMode(GL868_ESP32_LED_STATUS, OUTPUT);
  digitalWrite(GL868_ESP32_LED_STATUS, LOW);

  // WS2812B power control
  pinMode(GL868_ESP32_WS2812B_POWER_PIN, OUTPUT);
  digitalWrite(GL868_ESP32_WS2812B_POWER_PIN, LOW);
  setPowerEnabled(true);

  off();

  if (_taskHandle == NULL) {
    xTaskCreate(ledTask, "led_task", 2048, this, 1, &_taskHandle);
  }

  GL868_ESP32_LOG_D("LED manager initialized");
}

void GL868_ESP32_LED::end() {
  if (_taskHandle != NULL) {
    vTaskDelete(_taskHandle);
    _taskHandle = NULL;
  }
  off();
  setPowerEnabled(false);
}

// ============================================================================
// Background task
// ============================================================================
void GL868_ESP32_LED::ledTask(void *pvParameters) {
  GL868_ESP32_LED *led = (GL868_ESP32_LED *)pvParameters;
  while (1) {
    led->update();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ============================================================================
// Hardware helpers
// ============================================================================
void GL868_ESP32_LED::setPowerEnabled(bool enabled) {
  _powerEnabled = enabled;
  digitalWrite(GL868_ESP32_WS2812B_POWER_PIN, enabled ? HIGH : LOW);
  if (enabled) {
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void GL868_ESP32_LED::sendWS2812B(uint8_t r, uint8_t g, uint8_t b) {
  if (!_powerEnabled)
    return;
  uint8_t sr = (uint8_t)((r * _brightness) / 100);
  uint8_t sg = (uint8_t)((g * _brightness) / 100);
  uint8_t sb = (uint8_t)((b * _brightness) / 100);
  neopixelWrite(GL868_ESP32_WS2812B_PIN, sr, sg, sb);
}

void GL868_ESP32_LED::setRGB(uint8_t r, uint8_t g, uint8_t b) {
  if (_currentR != r || _currentG != g || _currentB != b) {
    _currentR = r;
    _currentG = g;
    _currentB = b;
    sendWS2812B(r, g, b);
  }
  _rgbOn = (r > 0 || g > 0 || b > 0);
}

void GL868_ESP32_LED::setBrightness(uint8_t percentage) {
  _brightness = (percentage > 100) ? 100 : percentage;
  if (_rgbOn)
    sendWS2812B(_currentR, _currentG, _currentB);
}

// ============================================================================
// User status LED (fully user-controlled, library never drives it after begin)
// ============================================================================
void GL868_ESP32_LED::turnOnLED() {
  digitalWrite(GL868_ESP32_LED_STATUS, HIGH);
}

void GL868_ESP32_LED::turnOffLED() {
  digitalWrite(GL868_ESP32_LED_STATUS, LOW);
}

// ============================================================================
// State control
// ============================================================================
void GL868_ESP32_LED::off() {
  setRGB(COLOR_OFF);
  // Status LED intentionally NOT touched — user-owned
  _state = LED_OFF;
  _stateBlinkOn = false;
  _errBlinksDone = 0;
  _errBlinkOn = false;
}

void GL868_ESP32_LED::setState(LEDState state) {
  if (_state != state) {
    GL868_ESP32_LOG_D("LED state: %d -> %d", _state, state);
    _state = state;
    _stateBlinkOn = false;
    _errBlinksDone = 0;
    _errBlinkOn = false;
    _lastUpdate = millis();
  }
}

void GL868_ESP32_LED::setError(ErrorCode code) {
  _errorCode = code;
  _state = LED_ERROR;
  _errBlinksDone = 0;
  _errBlinkOn = true; // Start first blink ON immediately
  _errorLoopCount = 0;
  _errorBlinksComplete = false;
  _lastUpdate = millis();

  switch (code) {
  case ERROR_NO_SIM:
    _targetBlinks = 1;
    break;
  case ERROR_NO_NETWORK:
    _targetBlinks = 2;
    break;
  case ERROR_NO_GPRS:
    _targetBlinks = 3;
    break;
  case ERROR_NO_GPS:
    _targetBlinks = 4;
    break;
  case ERROR_HTTP_FAIL:
    _targetBlinks = 5;
    break;
  case ERROR_MODEM:
    _targetBlinks = 0;
    break; // 0 = solid
  default:
    _targetBlinks = 6;
    break;
  }

  GL868_ESP32_LOG_E("LED error: %s (%d blinks)",
                    GL868_ESP32_ErrorToString(code), _targetBlinks);
}

// ============================================================================
// Colour lookup for system states
// ============================================================================
void GL868_ESP32_LED::getStateColour(uint8_t &r, uint8_t &g, uint8_t &b) const {
  switch (_state) {
  case LED_BOOT:
  case LED_MODEM_POWER_ON:
    r = 255;
    g = 255;
    b = 0; // Yellow
    break;
  case LED_GSM_INIT:
  case LED_GSM_REGISTER:
    r = 255;
    g = 0;
    b = 255; // Magenta
    break;
  case LED_GPS_POWER_ON:
  case LED_GPS_WAIT_FIX:
    r = 0;
    g = 0;
    b = 255; // Blue
    break;
  case LED_BUILD_JSON:
  case LED_GPRS_ATTACH:
  case LED_SEND_HTTP:
    r = 255;
    g = 255;
    b = 255; // White
    break;
  case LED_SEND_OFFLINE:
  case LED_IDLE:
  case LED_SMS_RECEIVED:
  case LED_SMS_SENDING:
  case LED_CALL_INCOMING:
  case LED_CALL_ACTIVE:
    r = 255;
    g = 105;
    b = 180; // Pink
    break;
  default:
    r = 0;
    g = 0;
    b = 0;
    break;
  }
}

// ============================================================================
// Main update — called from background task every 20 ms
// ============================================================================
void GL868_ESP32_LED::update() {
  uint32_t now = millis();
  uint32_t elapsed = now - _lastUpdate;

  // ---- SLEEP / OFF ---------------------------------------------------
  if (_state == LED_OFF || _state == LED_SLEEP || _state == LED_SLEEP_PREPARE) {
    if (_rgbOn)
      setRGB(COLOR_OFF);
    return;
  }

  // ---- ERROR ---------------------------------------------------------
  if (_state == LED_ERROR) {
    if (_errorBlinksComplete) {
      // All loops done — keep RGB off until state changes
      if (_rgbOn)
        setRGB(COLOR_OFF);
      return;
    }

    // Modem error: solid 2s ON / 1s OFF pattern
    if (_targetBlinks == 0) {
      if (_errBlinkOn) {
        setRGB(COLOR_RED);
        if (elapsed >= GL868_ESP32_ERROR_SOLID_ON_MS) {
          _errBlinkOn = false;
          _lastUpdate = now;
          setRGB(COLOR_OFF);
        }
      } else {
        setRGB(COLOR_OFF);
        if (elapsed >= GL868_ESP32_ERROR_SOLID_OFF_MS) {
          _errBlinkOn = true;
          _lastUpdate = now;
          _errorLoopCount++;
          if (_errorLoopCount >= GL868_ESP32_ERROR_BLINK_LOOPS) {
            _errorBlinksComplete = true;
          }
        }
      }
      return;
    }

    // N-blink error pattern:
    // ON 200ms, OFF 200ms (×N-1 inter-blinks), then ON 200ms, OFF 1000ms
    // (end-of-loop pause), repeat GL868_ESP32_ERROR_BLINK_LOOPS times.
    //
    // _errBlinksDone = index of current blink (0 .. _targetBlinks-1)
    // _errBlinkOn    = true while showing ON phase of current blink

    if (_errBlinkOn) {
      // Currently in the ON phase of blink _errBlinksDone
      setRGB(COLOR_RED);
      if (elapsed >= GL868_ESP32_ERROR_BLINK_ON_MS) {
        _errBlinkOn = false;
        _lastUpdate = now;
        setRGB(COLOR_OFF);
      }
    } else {
      // OFF phase — decide which duration applies
      setRGB(COLOR_OFF);
      bool isLastBlink = (_errBlinksDone >= (uint8_t)(_targetBlinks - 1));

      uint32_t offDur = isLastBlink ? GL868_ESP32_ERROR_BLINK_PAUSE_MS
                                    : GL868_ESP32_ERROR_BLINK_OFF_MS;

      if (elapsed >= offDur) {
        if (isLastBlink) {
          // End of this loop — count and restart
          _errorLoopCount++;
          if (_errorLoopCount >= GL868_ESP32_ERROR_BLINK_LOOPS) {
            _errorBlinksComplete = true;
            setRGB(COLOR_OFF);
            return;
          }
          // Start next loop
          _errBlinksDone = 0;
        } else {
          // Inter-blink gap done — advance to next blink
          _errBlinksDone++;
        }
        _errBlinkOn = true;
        _lastUpdate = now;
        setRGB(COLOR_RED);
      }
    }
    return;
  }

  // ---- NORMAL STATE --------------------------------------------------
  // 250ms ON, 1750ms OFF blink using state colour
  uint8_t r, g, b;
  getStateColour(r, g, b);

  if (_stateBlinkOn) {
    setRGB(r, g, b);
    if (elapsed >= GL868_ESP32_STATE_BLINK_ON_MS) {
      _stateBlinkOn = false;
      _lastUpdate = now;
      setRGB(COLOR_OFF);
    }
  } else {
    setRGB(COLOR_OFF);
    if (elapsed >= GL868_ESP32_STATE_BLINK_OFF_MS) {
      _stateBlinkOn = true;
      _lastUpdate = now;
      setRGB(r, g, b);
    }
  }
}
