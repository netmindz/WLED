#include "wled.h"
#include "fcn_declare.h"
#include "const.h"

//threading/network callback details: https://github.com/Aircoookie/WLED/pull/2336#discussion_r762276994
bool requestJSONBufferLock(uint8_t module)
{
  unsigned long now = millis();

  while (jsonBufferLock && millis()-now < 1000) delay(1); // wait for a second for buffer lock

  if (millis()-now >= 1000) {
    DEBUG_PRINT(F("ERROR: Locking JSON buffer failed! ("));
    DEBUG_PRINT(jsonBufferLock);
    DEBUG_PRINTLN(")");
    return false; // waiting time-outed
  }

  jsonBufferLock = module ? module : 255;
  fileDoc = &doc;  // used for applying presets (presets.cpp)
  doc.clear();
  return true;
}


void releaseJSONBufferLock()
{
  DEBUG_PRINT(F("JSON buffer released. ("));
  DEBUG_PRINT(jsonBufferLock);
  DEBUG_PRINTLN(")");
  fileDoc = nullptr;
  jsonBufferLock = 0;
}


// extracts effect mode (or palette) name from names serialized string
// caller must provide large enough buffer for name (incluing SR extensions)!
uint8_t extractModeName(uint8_t mode, const char *src, char *dest, uint8_t maxLen)
{
  uint8_t qComma = 0;
  bool insideQuotes = false;
  uint8_t printedChars = 0;
  char singleJsonSymbol;
  size_t len = strlen_P(src);

  // Find the mode name in JSON
  for (size_t i = 0; i < len; i++) {
    singleJsonSymbol = pgm_read_byte_near(src + i);
    if (singleJsonSymbol == '\0') break;
    if (singleJsonSymbol == '@' && insideQuotes && qComma == mode) break; //stop when SR extension encountered
    switch (singleJsonSymbol) {
      case '"':
        insideQuotes = !insideQuotes;
        break;
      case '[':
      case ']':
        break;
      case ',':
        if (!insideQuotes) qComma++;
      default:
        if (!insideQuotes || (qComma != mode)) break;
        dest[printedChars++] = singleJsonSymbol;
    }
    if ((qComma > mode) || (printedChars >= maxLen)) break;
  }
  dest[printedChars] = '\0';
  return strlen(dest);
}


CRGB getCRGBForBand(int x, int pal) { 
  extern int fftResult[];                         // summary of bins array. 16 summary bins.
  CRGB value;
  CHSV hsv;
  if(pal == 71) { // bit hacky to use palette id here, but don't want to litter the code with lots of different methods. TODO: add enum for palette creation type
    if(x == 0) {
      value = CRGB(uint8_t(fftResult[10]/2), uint8_t(fftResult[4]/2), uint8_t(fftResult[0]/2));
    }
    else if(x == 255) {
      value = CRGB(uint8_t(fftResult[10]/2), uint8_t(fftResult[0]/2), uint8_t(fftResult[4]/2));
    } 
    else {
      value = CRGB(uint8_t(fftResult[0]/2), uint8_t(fftResult[4]/2), uint8_t(fftResult[10]/2));
    } 
  }
  else if(pal == 72) {
    int b = map(x, 0, 255, 0, 8); // convert palette position to lower half of freq band
    hsv = CHSV(uint8_t(fftResult[b]), 255, uint8_t(map(fftResult[b], 0, 255, 30, 255)));  // pick hue
    hsv2rgb_rainbow(hsv, value);  // convert to R,G,B
  }
  return value;
} 