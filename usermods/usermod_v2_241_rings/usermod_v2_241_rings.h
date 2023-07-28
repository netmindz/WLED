#pragma once

#include "wled.h"
//========================================================================================================================


static const char _data_FX_mode_Rings[] PROGMEM = "Rings- Rings@Speed;;1;2";
static const char _data_FX_mode_SimpleRings[] PROGMEM = "Rings - Simple@Speed;;1;2";
static const char _data_FX_mode_RandomFlow[] PROGMEM = "Rings - Random Flow@Speed;;1;2";
static const char _data_FX_mode_AudioRings[] PROGMEM = "Rings - AudioRings@Speed;;1;2";

#define ringCount 9 // total Number of Rings
#define RINGS 9

int hue[RINGS];

//Map rings on disk to indicies.
//This is where all the magic happens.
//Each represents one of the concentric rings.
uint8_t ringMap[ringCount][2] = {
  {0, 0},     //0 Center Point
  {1, 8},     //1
  {9, 20},   //2
  {21, 36},   //3
  {37, 60},   //4
  {61, 92},   //5
  {93, 132},  //6
  {133, 180}, //7
  {181, 240}, //8 Outer Ring
};

//For convenience, last ring index
uint8_t lastRing = ringCount - 1;

void setRing(int ring, CRGB colour) {
  for (int i = ringMap[ring][0]; i <= ringMap[ring][1]; i++) {
    SEGMENT.setPixelColor(i, colour);
  }
}

void setRingFromFtt(int index, int ring) {
  uint8_t val = fftResult[index];
  // Visualize leds to the beat
  CRGB color = ColorFromPalette(currentPalette, val, 255, currentBlending);
  color.nscale8_video(val);
  setRing(ring, color);
}


uint16_t mode_Rings() {
  for (int r = 0; r < RINGS; r++) {
    setRing(r, ColorFromPalette(currentPalette, hue[r], 255, currentBlending));
  }
  FastLED.delay(SPEED);
  if (INWARD) {
    for (int r = 0; r < RINGS; r++) {
      hue[(r - 1)] = hue[r]; // set ring one less to that of the outer
    }
    hue[(RINGS - 1)] += JUMP;
  }
  else {
    for (int r = (RINGS - 1); r >= 1; r--) {
      hue[r] = hue[(r - 1)]; // set this ruing based on the inner
    }
    hue[0] += JUMP;
  }
}

uint16_t mode_SimpleRings() {
  static int j;
  for (int r = 0; r < RINGS; r++) {
    setRing(r, ColorFromPalette(currentPalette, j + (r * JUMP), 255, currentBlending));
  }
  j += JUMP;
  FastLED.delay(SPEED);
}


uint16_t mode_RandomFlow() {
  hue[0] = random(0, 255);
  for (int r = 0; r < RINGS; r++) {
    setRing(r, CHSV(hue[r], 255, 255));
  }
  for (int r = (RINGS - 1); r >= 1; r--) {
    hue[r] = hue[(r - 1)]; // set this ruing based on the inner
  }
  FastLED.delay(SPEED);
}


uint16_t mode_AudioRings() {
//   if(!audioRec && pgm == 0 && millis() > 5000) {
//     Serial.println("Skip audioRings as no data");
// //    autopgm++;
//     autopgm = random(1, (gPatternCount - 1));
//   }
  if(newReading) {
    newReading = false;
    for (int i = 0; i < 7; i++) {

      uint8_t val;
      if(INWARD) {
        val = fftResult[(i*2)];
      }
      else {
        int b = 14 -(i*2);
        val = fftResult[b];
      }
  
      // Visualize leds to the beat
      CRGB color = ColorFromPalette(currentPalette, val, val, currentBlending);
//      CRGB color = ColorFromPalette(currentPalette, val, 255, currentBlending);
//      color.nscale8_video(val);
      setRing(i, color);
//        setRingFromFtt((i * 2), i); 
    }

    setRingFromFtt(2, 7); // set outer ring to base
    setRingFromFtt(0, 8); // set outer ring to base

  }
}


class Rings241Usermod : public Usermod {

  public:

    Rings241Usermod(const char *name, bool enabled):Usermod(name, enabled) {} //WLEDMM
	

    void setup() {
		
		if(!enabled) return;

		strip.addEffect(250, &mode_Rings, _data_FX_mode_Rings);
		strip.addEffect(251, &mode_SimpleRings, _data_FX_mode_SimpleRings);
		strip.addEffect(252, &mode_RandomFlow, _data_FX_mode_RandomFlow);
		strip.addEffect(253, &mode_AudioRings, _data_FX_mode_AudioRings);

		initDone = true;
    }

	void loop() {}

    uint16_t getId()
    {
      return USERMOD_ID_241RINGS;
    }

};



