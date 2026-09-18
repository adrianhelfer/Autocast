#pragma once

#include <Arduino.h>
#include <SPI.h>

class AutocastDisplay {
public:
  AutocastDisplay();

  // Initializes the CS pin and SPI bus.
  // If sckPin/misoPin/mosiPin are all >= 0, SPI.begin() is called with
  // those explicit pins (as on ESP32); otherwise the default SPI.begin()
  // is used.
  void begin(uint8_t csPin, int8_t sckPin = -1, int8_t misoPin = -1, int8_t mosiPin = -1);

  // Renders topLine/bottomLine onto the display. Lines longer than the
  // display width automatically scroll; intervalMs controls how often
  // the scroll position advances. Call this every loop() iteration.
  void update(const String& topLine, const String& bottomLine, uint16_t intervalMs = 300);

  // Replaces common German umlauts/sharp-s with their ASCII digraphs
  // (ä -> ae, ß -> sz, etc.) since the display can only render ASCII.
  static String asciiIfy(const String& unicodeStr);

  static const uint8_t DISPLAY_WIDTH = 14;

private:
  uint8_t _csPin = 0;

  // Scroll state, kept per-instance (was function-local `static` in the
  // original code, which only worked because there was a single display).
  String _prevTop;
  String _prevBottom;
  unsigned long _lastScrollTime = 0;
  uint16_t _topOffset = 0;
  uint16_t _bottomOffset = 0;

  static const uint8_t SEPARATOR_SPACES = 2;
  static const uint16_t CHARACTER_PATTERNS[];

  static uint16_t pattern(char c);
  void displayText(const char* str);
  String prepareLine(const String& line, uint16_t& offset, uint8_t width);
};
