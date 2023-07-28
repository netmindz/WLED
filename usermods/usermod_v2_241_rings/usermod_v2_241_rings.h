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
  um_data_t *um_data;
  if (!usermods.getUMData(&um_data, USERMOD_ID_AUDIOREACTIVE)) {
    // add support for no audio
    um_data = simulateSound(SEGMENT.soundSim);
  }
  uint8_t *fftResult = (uint8_t*)um_data->u_data[2];

  uint8_t val = fftResult[index];
  // Visualize leds to the beat
  CRGB color =  SEGMENT.color_from_palette(val, false, false, 0);
  color.nscale8_video(val);
  setRing(ring, color);
}


uint16_t mode_Rings() {
  
  int JUMP = map(SEGMENT.custom1, 0, 255, 1, 16); // TODO: set range
  bool INWARD = (SEGMENT.custom2 > 125) ? true  : false;

  for (int r = 0; r < RINGS; r++) {
    setRing(r, SEGMENT.color_from_palette(hue[r], false, false, 0));
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
  int JUMP = map(SEGMENT.custom1, 0, 255, 1, 16); // TODO: set range
  static int j;
  for (int r = 0; r < RINGS; r++) {
    setRing(r, SEGMENT.color_from_palette(j + (r * JUMP), false, false, 0));
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

  bool newReading = true; // TODO
  bool INWARD = (SEGMENT.custom1 > 125) ? true  : false;

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
      CRGB color = SEGMENT.color_from_palette(val, false, false, 0, val);
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



