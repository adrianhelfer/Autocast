#include "AutocastDisplay.h"

const uint16_t AutocastDisplay::CHARACTER_PATTERNS[] = {
  0b0000000000000000, /* (space) */
  0b0000000100001001, /* ! */
  0b0000000000011000, /* " */
  0b0101010010011001, /* # */
  0b0101010010110011, /* $ */
  0b0001111011010111, /* % */
  0b0111100000110100, /* & */
  0b0000000000010000, /* ' */
  0b0000100001000000, /* ( */
  0b0000001000000100, /* ) */
  0b0001111011010100, /* * */
  0b0001010010010000, /* + */
  0b0000001000000000, /* , */
  0b0001000010000000, /* - */
  0b0000000100000000, /* . */
  0b0000001001000000, /* / */
  0b0110001001101011, /* 0 */
  0b0000000001001001, /* 1 */
  0b0111000010101000, /* 2 */
  0b0100000010101001, /* 3 */
  0b0001000010001011, /* 4 */
  0b0101100000100010, /* 5 */
  0b0111000010100011, /* 6 */
  0b0000000000101001, /* 7 */
  0b0111000010101011, /* 8 */
  0b0101000010101011, /* 9 */
  0b0000010000010000, /* : */
  0b0000001000010000, /* ; */
  0b0001100001000000, /* < */
  0b0101000010000000, /* = */
  0b0000001010000100, /* > */
  0b0000010110101000, /* ? */
  0b0110000010111010, /* @ */
  0b0011000010101011, /* A */
  0b0100010010111001, /* B */
  0b0110000000100010, /* C */
  0b0100010000111001, /* D */
  0b0111000000100010, /* E */
  0b0011000000100010, /* F */
  0b0110000010100011, /* G */
  0b0011000010001011, /* H */
  0b0100010000110000, /* I */
  0b0110000000001001, /* J */
  0b0011100001000010, /* K */
  0b0110000000000010, /* L */
  0b0010000001001111, /* M */
  0b0010100000001111, /* N */
  0b0110000000101011, /* O */
  0b0011000010101010, /* P */
  0b0110100000101011, /* Q */
  0b0011100010101010, /* R */
  0b0101000010100011, /* S */
  0b0000010000110000, /* T */
  0b0110000000001011, /* U */
  0b0010001001000010, /* V */
  0b0010101000001011, /* W */
  0b0000101001000100, /* X */
  0b0101000010001011, /* Y */
  0b0100001001100000, /* Z */
  0b0110000000100010, /* [ */
  0b0000100000000100, /* \ */
  0b0100000000101001, /* ] */
  0b0000101000000000, /* ^ */
  0b0100000000000000, /* _ */
  0b0000000000000100, /* ` */
  0b0111010000000000, /* a */
  0b0111100000000010, /* b */
  0b0111000010000000, /* c */
  0b0100001010001001, /* d */
  0b0111001000000000, /* e */
  0b0001010011000000, /* f */
  0b0100000011001001, /* g */
  0b0011010000000010, /* h */
  0b0000010000000000, /* i */
  0b0010001000010000, /* j */
  0b0000110001010000, /* k */
  0b0010000000000010, /* l */
  0b0011010010000001, /* m */
  0b0011010000000000, /* n */
  0b0111000010000001, /* o */
  0b0011000000000110, /* p */
  0b0000000011001001, /* q */
  0b0011000000000000, /* r */
  0b0100100010000000, /* s */
  0b0111000000000010, /* t */
  0b0110000000000001, /* u */
  0b0010001000000000, /* v */
  0b0010101000000001, /* w */
  0b0000101001000100, /* x */
  0b0100000010011001, /* y */
  0b0101001000000000, /* z */
  0b0101001000100100, /* { */
  0b0000010000010000, /* | */
  0b0100100011100000, /* } */
  0b0001001011000000, /* ~ */
};

AutocastDisplay::AutocastDisplay() {}

void AutocastDisplay::begin(uint8_t csPin, int8_t sckPin, int8_t misoPin, int8_t mosiPin) {
  _csPin = csPin;
  pinMode(_csPin, OUTPUT);
  digitalWrite(_csPin, HIGH);

  if (sckPin >= 0 && misoPin >= 0 && mosiPin >= 0) {
    SPI.begin(sckPin, misoPin, mosiPin, _csPin);
  } else {
    SPI.begin();
  }

  _prevTop = "";
  _prevBottom = "";
  _lastScrollTime = 0;
  _topOffset = 0;
  _bottomOffset = 0;
}

String AutocastDisplay::asciiIfy(const String& unicodeStr) {
  String ret = unicodeStr;

  // German
  ret.replace("ä", "ae");
  ret.replace("ö", "oe");
  ret.replace("ü", "ue");
  ret.replace("Ä", "AE");
  ret.replace("Ö", "OE");
  ret.replace("Ü", "UE");
  ret.replace("ß", "sz");
  ret.replace("ẞ", "SZ");

  // Accented vowels (Western / Central Europe)
  ret.replace("à", "a");
  ret.replace("á", "a");
  ret.replace("â", "a");
  ret.replace("ã", "a");
  ret.replace("å", "a");
  ret.replace("À", "A");
  ret.replace("Á", "A");
  ret.replace("Â", "A");
  ret.replace("Ã", "A");
  ret.replace("Å", "A");

  ret.replace("è", "e");
  ret.replace("é", "e");
  ret.replace("ê", "e");
  ret.replace("ë", "e");
  ret.replace("È", "E");
  ret.replace("É", "E");
  ret.replace("Ê", "E");
  ret.replace("Ë", "E");

  ret.replace("ì", "i");
  ret.replace("í", "i");
  ret.replace("î", "i");
  ret.replace("ï", "i");
  ret.replace("Ì", "I");
  ret.replace("Í", "I");
  ret.replace("Î", "I");
  ret.replace("Ï", "I");

  ret.replace("ò", "o");
  ret.replace("ó", "o");
  ret.replace("ô", "o");
  ret.replace("õ", "o");
  ret.replace("Ò", "O");
  ret.replace("Ó", "O");
  ret.replace("Ô", "O");
  ret.replace("Õ", "O");

  ret.replace("ù", "u");
  ret.replace("ú", "u");
  ret.replace("û", "u");
  ret.replace("Ù", "U");
  ret.replace("Ú", "U");
  ret.replace("Û", "U");

  // Nordic / French
  ret.replace("æ", "ae");
  ret.replace("Æ", "AE");
  ret.replace("œ", "oe");
  ret.replace("Œ", "OE");
  ret.replace("ø", "o");
  ret.replace("Ø", "O");

  // Slavic / Eastern Europe
  ret.replace("č", "c");
  ret.replace("ć", "c");
  ret.replace("ç", "c");
  ret.replace("Č", "C");
  ret.replace("Ć", "C");
  ret.replace("Ç", "C");

  ret.replace("š", "s");
  ret.replace("ś", "s");
  ret.replace("Š", "S");
  ret.replace("Ś", "S");

  ret.replace("ž", "z");
  ret.replace("ź", "z");
  ret.replace("ż", "z");
  ret.replace("Ž", "Z");
  ret.replace("Ź", "Z");
  ret.replace("Ż", "Z");

  ret.replace("ñ", "n");
  ret.replace("Ñ", "N");

  ret.replace("ý", "y");
  ret.replace("ÿ", "y");
  ret.replace("Ý", "Y");

  // Cyrillic
  ret.replace("А", "A");
  ret.replace("Б", "B");
  ret.replace("В", "V");
  ret.replace("Г", "G");
  ret.replace("Д", "D");
  ret.replace("Е", "E");
  ret.replace("Ё", "Yo");
  ret.replace("Ж", "Zh");
  ret.replace("З", "Z");
  ret.replace("И", "I");
  ret.replace("Й", "Y");
  ret.replace("К", "K");
  ret.replace("Л", "L");
  ret.replace("М", "M");
  ret.replace("Н", "N");
  ret.replace("О", "O");
  ret.replace("П", "P");
  ret.replace("Р", "R");
  ret.replace("С", "S");
  ret.replace("Т", "T");
  ret.replace("У", "U");
  ret.replace("Ф", "F");
  ret.replace("Х", "Kh");
  ret.replace("Ц", "Ts");
  ret.replace("Ч", "Ch");
  ret.replace("Ш", "Sh");
  ret.replace("Щ", "Shch");
  ret.replace("Ы", "Y");
  ret.replace("Э", "E");
  ret.replace("Ю", "Yu");
  ret.replace("Я", "Ya");

  ret.replace("а", "a");
  ret.replace("б", "b");
  ret.replace("в", "v");
  ret.replace("г", "g");
  ret.replace("д", "d");
  ret.replace("е", "e");
  ret.replace("ё", "yo");
  ret.replace("ж", "zh");
  ret.replace("з", "z");
  ret.replace("и", "i");
  ret.replace("й", "y");
  ret.replace("к", "k");
  ret.replace("л", "l");
  ret.replace("м", "m");
  ret.replace("н", "n");
  ret.replace("о", "o");
  ret.replace("п", "p");
  ret.replace("р", "r");
  ret.replace("с", "s");
  ret.replace("т", "t");
  ret.replace("у", "u");
  ret.replace("ф", "f");
  ret.replace("х", "kh");
  ret.replace("ц", "ts");
  ret.replace("ч", "ch");
  ret.replace("ш", "sh");
  ret.replace("щ", "shch");
  ret.replace("ы", "y");
  ret.replace("э", "e");
  ret.replace("ю", "yu");
  ret.replace("я", "ya");

  // Ligatures / oddballs
  ret.replace("ð", "d");
  ret.replace("Ð", "D");
  ret.replace("þ", "th");
  ret.replace("Þ", "TH");
  ret.replace("ł", "l");
  ret.replace("Ł", "L");

  return ret;
}

uint16_t AutocastDisplay::pattern(char c) {
  if ((unsigned char)c < 32 || (unsigned char)c > 127)
    return 0b0001000010000000; /* - */

  return CHARACTER_PATTERNS[(unsigned char)c - 32];
}

void AutocastDisplay::displayText(const char* str) {
  size_t len = strlen(str);
  digitalWrite(_csPin, LOW);
  for (int i = len - 1; i >= 0; i--) {
    SPI.transfer16(pattern(str[i]));
  }
  digitalWrite(_csPin, HIGH);
}

String AutocastDisplay::prepareLine(const String& line, uint16_t& offset, uint8_t width) {
  String result;

  if (line.length() <= width) {
    result = line;
    while (result.length() < width) {
      result += ' ';
    }
  } else {
    String ringBuffer = line;
    for (uint8_t i = 0; i < SEPARATOR_SPACES; i++) {
      ringBuffer += ' ';
    }
    ringBuffer += line;

    for (uint8_t i = 0; i < width; i++) {
      result += ringBuffer[(offset + i) % ringBuffer.length()];
    }
  }

  return result;
}

void AutocastDisplay::update(const String& topLine, const String& bottomLine, uint16_t intervalMs) {
  // Content changed -> restart scrolling from the beginning.
  if (topLine != _prevTop || bottomLine != _prevBottom) {
    _prevTop = topLine;
    _prevBottom = bottomLine;
    _topOffset = 0;
    _bottomOffset = 0;
    _lastScrollTime = millis();
  }

  unsigned long currentTime = millis();
  if (currentTime - _lastScrollTime >= intervalMs) {
    _lastScrollTime = currentTime;

    if (topLine.length() > DISPLAY_WIDTH) {
      _topOffset = (_topOffset + 1) % (topLine.length() + SEPARATOR_SPACES);
    }
    if (bottomLine.length() > DISPLAY_WIDTH) {
      _bottomOffset = (_bottomOffset + 1) % (bottomLine.length() + SEPARATOR_SPACES);
    }
  }

  String topDisplay = prepareLine(topLine, _topOffset, DISPLAY_WIDTH);
  String bottomDisplay = prepareLine(bottomLine, _bottomOffset, DISPLAY_WIDTH);

  displayText((topDisplay + bottomDisplay).c_str());
}
