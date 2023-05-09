#pragma once

#include "Statistic.h"
#include "elapsedMillis.h"
#include "wled.h"

/*
 * Heavily based off code from https://github.com/Diod-dev/New_Visualizer_Skeleton
 * Ported to WLED by Netmindz
 */

const int numFFTBins = 16; // TODO: don't hard code value here

class AudioReactiveAuto : public Usermod {

  private:

    // Private class members. You can declare variables and functions only accessible to your usermod here

    // set your config variables to their boot default value (this can also be done in readFromConfig() or a constructor if you prefer)
    // bool testBool = false;
    // unsigned long testULong = 42424242;
    // float testFloat = 42.42;
    // String testString = "Forty-Two";

    // These config variables have defaults set inside readFromConfig()
    // int testInt;
    // long testLong;
    // int8_t testPins[2];


    // AUDIO DETECTION VARIABLES
    // Raw FFT data
    int spectrumValue[numFFTBins];
    int spectrumValueOld[numFFTBins];
    // Running averages in the FFT bins
    double average[numFFTBins];
    // Running standard deviations in the FFT bins
    double stDev[numFFTBins];
    // TODO: implement total volume data. 
    int volume, volumeAverage;
    // Statistic vol;
    // fadeValues are used to control how fast LEDs fade that are colored by the FFT data directly.
    // The higher the FFT data (the louder the sound is), the higher the fadeValue (the faster the light fades).
    int fadeVals[numFFTBins];
    // freq is an array of values that we can apply the 'Statistic' library to
    Statistic freq[numFFTBins];
    // beatDetected monitors whether a beat has been detected. 2 = new beat detected. 1 = old beat hasn't gone down yet. 0 = no beat detected
    uint8_t beatDetected[numFFTBins] = {};
    // BeatTimer measures the amount of time between detected beats
    elapsedMillis beatTimer[numFFTBins] = {};
    // BeatIntervals are where beatTimer data is stored
    int beatInterval[numFFTBins];
    int beatIntervalOld[numFFTBins];
    // constantBeatCounter keeps track of how many beats have occurred with a constant time between them
    int constantBeatCounter[numFFTBins];
    // TODO: utilize the peaks of the beats to have more accurate peak detection and utilize for beat-dependent patterns
    int beatPeak[numFFTBins];
    int beatPeakOld[numFFTBins];
    // The higher the activity in the FFT bin, the higher the score
    int binScore[numFFTBins];
    // Which FFT bins in the low frequencies, mid frequencies, and high frequencies have the the strongest beats within the song.
    int lowBeatBin, midBeatBin, highBeatBin;
    // Which FFT bins in the low frequencies, mid frequencies, and high frequencies have the highest averages
    // int lowAveBin, midAveBin, highAveBin;
    // How long has silence been detected
    elapsedMillis silenceDuration;
    // How long has song been going on for. Useful for DJ sets that don't have silence between songs. This keeps the song data clearing every 5 mins
    elapsedMillis songDuration;

    bool silence = true;
    // Constant beat detected variables
    bool constBeat = false;
    int maxConstBeat = 0;
    uint8_t constBeatBin;
    // mixAmount bounces between 0 and 255 depending on whether a strong beat is detected. 255 = beat detcted, 0 = no beat detected
    uint8_t mixAmount;

    // any private methods should go here (non-inline methosd should be defined out of class)
    void publishMqtt(const char* state, bool retain = false); // example for publishing MQTT message


  public:

    AudioReactiveAuto(const char *name, bool enabled):Usermod(name, enabled) {} //WLEDMM

    // non WLED related methods, may be used for data exchange between usermods (non-inline methods should be defined out of class)

    /**
     * Enable/Disable the usermod
     */
    inline void enable(bool enable) { enabled = enable; }

    /**
     * Get usermod enabled/disabled state
     */
    inline bool isEnabled() { return enabled; }

    // in such case add the following to another usermod:
    //  in private vars:
    //   #ifdef USERMOD_EXAMPLE
    //   MyExampleUsermod* UM;
    //   #endif
    //  in setup()
    //   #ifdef USERMOD_EXAMPLE
    //   UM = (MyExampleUsermod*) usermods.lookup(USERMOD_ID_EXAMPLE);
    //   #endif
    //  somewhere in loop() or other member method
    //   #ifdef USERMOD_EXAMPLE
    //   if (UM != nullptr) isExampleEnabled = UM->isEnabled();
    //   if (!isExampleEnabled) UM->enable(true);
    //   #endif


    // methods called by WLED (can be inlined as they are called only once but if you call them explicitly define them out of class)

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * readFromConfig() is called prior to setup()
     * You can use it to initialize variables, sensors or similar.
     */
    void setup() {
      // do your set-up here
      //Serial.println("Hello from my usermod!");
      initDone = true;
      clearStats();
    }


    /*
     * loop() is called continuously. Here you can check for events, read sensors, etc.
     * 
     * Tips:
     * 1. You can use "if (WLED_CONNECTED)" to check for a successful network connection.
     *    Additionally, "if (WLED_MQTT_CONNECTED)" is available to check for a connection to an MQTT broker.
     * 
     * 2. Try to avoid using the delay() function. NEVER use delays longer than 10 milliseconds.
     *    Instead, use a timer check as shown here.
     */
    void loop() {
      // if usermod is disabled or called during strip updating just exit
      // NOTE: on very long strips strip.isUpdating() may always return true so update accordingly
      if (!enabled || strip.isUpdating()) return;

      // do your magic here
      if (millis() - lastTime > 1000) {
        //Serial.println("I'm alive!");
        lastTime = millis();
      }

      musicAnalytics();
      autoMusicVisualizing();
    }


    /*
     * addToJsonInfo() can be used to add custom entries to the /json/info part of the JSON API.
     * Creating an "u" object allows you to add custom key/value pairs to the Info section of the WLED web UI.
     * Below it is shown how this could be used for e.g. a light sensor
     */
    void addToJsonInfo(JsonObject& root)
    {
      // if "u" object does not exist yet wee need to create it
      JsonObject user = root["u"];
      if (user.isNull()) user = root.createNestedObject("u");

      JsonArray infoArr = user.createNestedArray(FPSTR("AudioReactiveAuto"));

      // String uiDomString = F("<button class=\"btn btn-xs\" onclick=\"requestJson({");
      // uiDomString += FPSTR(_name);
      // uiDomString += F(":{");
      // uiDomString += FPSTR(enabled);
      // uiDomString += enabled ? F(":false}});\">") : F(":true}});\">");
      // uiDomString += F("<i class=\"icons");
      // uiDomString += enabled ? F(" on") : F(" off");
      // uiDomString += F("\">&#xe08f;</i>");
      // uiDomString += F("</button>");
      // infoArr.add(uiDomString);

      if (enabled) {
        if(silence) {
          infoArr.add("Silence");
        }
        else {
          if(constBeat) {
            infoArr.add("Constant Beat");
          }
          else {
            infoArr.add("Non-Constant Beat");
          }
        }
      }
      else {
        infoArr.add("ARA Disabled");
      }
      //this code adds "u":{"ExampleUsermod":[20," lux"]} to the info object
      //int reading = 20;
      //JsonArray lightArr = user.createNestedArray(FPSTR(_name))); //name
      //lightArr.add(reading); //value
      //lightArr.add(F(" lux")); //unit

      // if you are implementing a sensor usermod, you may publish sensor data
      //JsonObject sensor = root[F("sensor")];
      //if (sensor.isNull()) sensor = root.createNestedObject(F("sensor"));
      //temp = sensor.createNestedArray(F("light"));
      //temp.add(reading);
      //temp.add(F("lux"));
    }


    /*
     * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void addToJsonState(JsonObject& root)
    {
      if (!initDone || !enabled) return;  // prevent crash on boot applyPreset()

      JsonObject usermod = root[FPSTR(_name)];
      if (usermod.isNull()) usermod = root.createNestedObject(FPSTR(_name));

      //usermod["user0"] = userVar0;
    }


    /*
     * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void readFromJsonState(JsonObject& root)
    {
      if (!initDone) return;  // prevent crash on boot applyPreset()

      JsonObject usermod = root[FPSTR(_name)];
      if (!usermod.isNull()) {
        // expect JSON usermod data in usermod name object: {"ExampleUsermod:{"user0":10}"}
        userVar0 = usermod["user0"] | userVar0; //if "user0" key exists in JSON, update, else keep old value
      }
      // you can as well check WLED state JSON keys
      //if (root["bri"] == 255) Serial.println(F("Don't burn down your garage!"));
    }


    /*
     * addToConfig() can be used to add custom persistent settings to the cfg.json file in the "um" (usermod) object.
     * It will be called by WLED when settings are actually saved (for example, LED settings are saved)
     * If you want to force saving the current state, use serializeConfig() in your loop().
     * 
     * CAUTION: serializeConfig() will initiate a filesystem write operation.
     * It might cause the LEDs to stutter and will cause flash wear if called too often.
     * Use it sparingly and always in the loop, never in network callbacks!
     * 
     * addToConfig() will make your settings editable through the Usermod Settings page automatically.
     *
     * Usermod Settings Overview:
     * - Numeric values are treated as floats in the browser.
     *   - If the numeric value entered into the browser contains a decimal point, it will be parsed as a C float
     *     before being returned to the Usermod.  The float data type has only 6-7 decimal digits of precision, and
     *     doubles are not supported, numbers will be rounded to the nearest float value when being parsed.
     *     The range accepted by the input field is +/- 1.175494351e-38 to +/- 3.402823466e+38.
     *   - If the numeric value entered into the browser doesn't contain a decimal point, it will be parsed as a
     *     C int32_t (range: -2147483648 to 2147483647) before being returned to the usermod.
     *     Overflows or underflows are truncated to the max/min value for an int32_t, and again truncated to the type
     *     used in the Usermod when reading the value from ArduinoJson.
     * - Pin values can be treated differently from an integer value by using the key name "pin"
     *   - "pin" can contain a single or array of integer values
     *   - On the Usermod Settings page there is simple checking for pin conflicts and warnings for special pins
     *     - Red color indicates a conflict.  Yellow color indicates a pin with a warning (e.g. an input-only pin)
     *   - Tip: use int8_t to store the pin value in the Usermod, so a -1 value (pin not set) can be used
     *
     * See usermod_v2_auto_save.h for an example that saves Flash space by reusing ArduinoJson key name strings
     * 
     * If you need a dedicated settings page with custom layout for your Usermod, that takes a lot more work.  
     * You will have to add the setting to the HTML, xml.cpp and set.cpp manually.
     * See the WLED Soundreactive fork (code and wiki) for reference.  https://github.com/atuline/WLED
     * 
     * I highly recommend checking out the basics of ArduinoJson serialization and deserialization in order to use custom settings!
     */
    void addToConfig(JsonObject& root)
    {
      Usermod::addToConfig(root); JsonObject top = root[FPSTR(_name)]; //WLEDMM

      //save these vars persistently whenever settings are saved
      // top["great"] = userVar0;
      // top["testBool"] = testBool;
      // top["testInt"] = testInt;
      // top["testLong"] = testLong;
      // top["testULong"] = testULong;
      // top["testFloat"] = testFloat;
      // top["testString"] = testString;
    }


    /*
     * readFromConfig() can be used to read back the custom settings you added with addToConfig().
     * This is called by WLED when settings are loaded (currently this only happens immediately after boot, or after saving on the Usermod Settings page)
     * 
     * readFromConfig() is called BEFORE setup(). This means you can use your persistent values in setup() (e.g. pin assignments, buffer sizes),
     * but also that if you want to write persistent values to a dynamic buffer, you'd need to allocate it here instead of in setup.
     * If you don't know what that is, don't fret. It most likely doesn't affect your use case :)
     * 
     * Return true in case the config values returned from Usermod Settings were complete, or false if you'd like WLED to save your defaults to disk (so any missing values are editable in Usermod Settings)
     * 
     * getJsonValue() returns false if the value is missing, or copies the value into the variable provided and returns true if the value is present
     * The configComplete variable is true only if the "exampleUsermod" object and all values are present.  If any values are missing, WLED will know to call addToConfig() to save them
     * 
     * This function is guaranteed to be called on boot, but could also be called every time settings are updated
     */
    bool readFromConfig(JsonObject& root)
    {
      // default settings values could be set here (or below using the 3-argument getJsonValue()) instead of in the class definition or constructor
      // setting them inside readFromConfig() is slightly more robust, handling the rare but plausible use case of single value being missing after boot (e.g. if the cfg.json was manually edited and a value was removed)

      bool configComplete = Usermod::readFromConfig(root);JsonObject top = root[FPSTR(_name)]; //WLEDMM

      configComplete &= getJsonValue(top["great"], userVar0);
      // configComplete &= getJsonValue(top["testBool"], testBool);
      // configComplete &= getJsonValue(top["testULong"], testULong);
      // configComplete &= getJsonValue(top["testFloat"], testFloat);
      // configComplete &= getJsonValue(top["testString"], testString);

      // // A 3-argument getJsonValue() assigns the 3rd argument as a default value if the Json value is missing
      // configComplete &= getJsonValue(top["testInt"], testInt, 42);  
      // configComplete &= getJsonValue(top["testLong"], testLong, -42424242);

      return configComplete;
    }


    /*
     * appendConfigData() is called when user enters usermod settings page
     * it may add additional metadata for certain entry fields (adding drop down is possible)
     * be careful not to add too much as oappend() buffer is limited to 3k
     */
    void appendConfigData()
    {
      oappend(SET_F("addInfo('")); oappend(String(FPSTR(_name)).c_str()); oappend(SET_F(":great")); oappend(SET_F("',1,'<i>(this is a great config value)</i>');"));
      oappend(SET_F("addInfo('")); oappend(String(FPSTR(_name)).c_str()); oappend(SET_F(":testString")); oappend(SET_F("',1,'enter any string you want');"));
      oappend(SET_F("dd=addDropdown('")); oappend(String(FPSTR(_name)).c_str()); oappend(SET_F("','testInt');"));
      oappend(SET_F("addOption(dd,'Nothing',0);"));
      oappend(SET_F("addOption(dd,'Everything',42);"));
    }
  

#ifndef WLED_DISABLE_MQTT
    /**
     * handling of MQTT message
     * topic only contains stripped topic (part after /wled/MAC)
     */
    bool onMqttMessage(char* topic, char* payload) {
      // check if we received a command
      //if (strlen(topic) == 8 && strncmp_P(topic, PSTR("/command"), 8) == 0) {
      //  String action = payload;
      //  if (action == "on") {
      //    enabled = true;
      //    return true;
      //  } else if (action == "off") {
      //    enabled = false;
      //    return true;
      //  } else if (action == "toggle") {
      //    enabled = !enabled;
      //    return true;
      //  }
      //}
      return false;
    }

    /**
     * onMqttConnect() is called when MQTT connection is established
     */
    void onMqttConnect(bool sessionPresent) {
      // do any MQTT related initialisation here
      //publishMqtt("I am alive!");
    }
#endif


    /**
     * onStateChanged() is used to detect WLED state change
     * @mode parameter is CALL_MODE_... parameter used for notifications
     */
    void onStateChange(uint8_t mode) {
      // do something if WLED state changed (color, brightness, effect, preset, etc)
    }


    /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
    uint16_t getId()
    {
      return USERMOD_ID_AUDIOREACTIVE_AUTO;
    }

    // Clears the running mean and st dev. Used mostly between songs when silence is detected.
    void clearStats() {
      for (int i = 0; i < numFFTBins; i++) {
        freq[i].clear();
        beatPeak[i] = 0;
        binScore[i] = 0;
        lowBeatBin = 2;
        midBeatBin = 8;
        highBeatBin = 12;
      }
    }
    
    void detectSilence() {
      uint8_t silenceCounter = 0;
      // If FFT bin data is very low, consider it as silence in the bin
      // reset the statistics because it's likely the song ended
      for (int i = 0; i < numFFTBins; i++) {
        if (spectrumValue[i] < 10) { // 10 is an arbitrary number but it means it is basically silent. Some audio sources have noise in higher FFT bins.
          silenceCounter++;
        }
        else {
          break; // stop counting silent FFT bins
        }
      }
      // If the song is 5 minutes long, reset stats.
      if (songDuration > 300000) {
        songDuration = 0;
        clearStats();
      }

      // If we've counted over 11 bins that were silent, then clear stats, set silence = true, and set songDuration = 0.
      if (silenceCounter > 11) {
        clearStats();
        silence = true;
        songDuration = 0;
      }
      else { // else (if silence isn't detected), set silence = false, silenceDuration = 0
        silence = false;
        silenceDuration = 0;
      }
    }

void fillStats() {
	// Save old FFT data in spectrumValueOld
	for (int i = 0; i < numFFTBins; i++) {
		spectrumValueOld[i] = spectrumValue[i];
  }
  um_data_t *um_data;
  if (!usermods.getUMData(&um_data, USERMOD_ID_AUDIOREACTIVE)) {
    // add support for no audio
    um_data = simulateSound(SEGMENT.soundSim);
  }
  uint8_t *fftResult = (uint8_t*)um_data->u_data[2];

	for (int i = 0; i < numFFTBins; i++) {
		spectrumValue[i] = fftResult[i]; // TODO: might need to scale values to match existing code expectations
  }

	// reset volume variable
	volume = 0;
	// Update the average and standard deviation of each FFT bin value
	for (int i = 0; i < numFFTBins; i++) {
		freq[i].add(spectrumValue[i]);
		average[i] = freq[i].average();
		stDev[i] = freq[i].pop_stdev();
		volume = volume + spectrumValue[i]; // add all the values to find the volume
	}

	// See if there is silence
	detectSilence();
}

void beatTiming(int i) {
	if (beatDetected[i] == 2) { // if a new beat has been detected
		if (beatTimer[i] < 200) // but it is less than 200ms from the last one
			beatDetected[i] = 1; // cancel the beat
		else { // if it's more than 200ms from the last one
			beatInterval[i] = beatTimer[i];  // A real beat has been detected. Save the time since last beat.
			// TODO: could use absolute value below (Wasn't working for unknown reason)
			if ((beatIntervalOld[i] - beatInterval[i]) < 30 && (beatIntervalOld[i] - beatInterval[i]) > -30) // If the time between the new interval and old interval is less than 30 ms 
				constantBeatCounter[i]++;

			beatTimer[i] = 0; // reset beat timer
			beatIntervalOld[i] = beatInterval[i]; // save new beat interval as old beat interval
		}
	}
	if (constantBeatCounter[i] > 0 && beatTimer[i] > beatInterval[i] + 50) constantBeatCounter[i] = 0; // clears beat counter when more time than the beat has passed

	// These print statements will print the constant beat counter of each bin
	//if (i == numFFTBins - 1) Serial.println(constantBeatCounter[i]);
	//else {
	//	Serial.print(constantBeatCounter[i]);
	//	Serial.print("\t");
	//}

	// NOTE: remember this beatTiming function is run within a for loop in musicAnalytics()
	if ( i < 4) { // for the 4 lowest frequency FFT bins
		if (constantBeatCounter[i] > maxConstBeat) { // if this FFT bin's constantBeatCounter is greater than the current highest constant beat detected (within 4 lowest bins)...
			maxConstBeat = constantBeatCounter[i]; // the new maxConstBeat is this bin's constantBeatCounter
			constBeatBin = i; // This bin currently has the maxConstBeat
		}
	}
	if (maxConstBeat > 10) // If we've reached over 10 beats with a constant time interval...
		constBeat = true; // Go in to a constBeat pattern ( used in normalOperations() )
	else 
		constBeat = false;
}

void beatDetection(int i) {
	// 0 = no beat detected
	// 1 = old beat hasn't dropped / reset yet
	// 2 = new beat detected
	// Peak detection is performed here. FFT value must be greater than the average+(2.3*st.dev) and greater than a threshold of 100, which is pretty quiet
  // EVERY_N_MILLIS(100) {
    DEBUG_PRINTF("band=%u\tspectrumValue=%u\taverage=%u\tstDev=%u\n", i, spectrumValue[i],  average[i] , stDev[i] );
  // }
	if (spectrumValue[i] > average[i] + 2.3 * stDev[i] && spectrumValue[i] > 100) {
		if (beatDetected[i] == 2) {// If it's already 2, then the beat has already been detected...
			beatDetected[i] = 1; // so drop the value to 1.
      DEBUG_PRINTLN("StillBEAT" + i);
    }
		if (beatDetected[i] == 0) { // If 0, then this is a new beat...
			beatDetected[i] = 2; // so make the value 2.
      DEBUG_PRINTLN("BEAT " + i);
    }
		// if it's == 1, it stays 1. no code is needed.
	}
	// This is where 1's get reset to 0. This prevents multiple beats being triggered from 1 beat, or when they aren't well defined
	else { // if beat is not detected...
		if (beatDetected[i] == 1) { // and it's value is 1...
			if (spectrumValue[i] <= average[i]) {// and it has dropped below the running average of that FFT bin...
				beatDetected[i] = 0; // reset to 0
      }
		}
	}
}
 
// scoreBins figures out which FFT bins have the best beats to visualize.
void scoreBins(int i) {
	// If there's a constant beat above 5 counts, increase score by 2
	if (constantBeatCounter[i] > 5)
		binScore[i] += 2;

	// If there's a beat detected, increase score by 1
	if (beatDetected[i] == 2)
		binScore[i] += 1;

	// If an FFT bin's score is high and it doesn't have a constant beat, lower that score
	if (binScore[i] > 300 && constantBeatCounter[i] < 2)
		binScore[i] -= 2;

	// These print statements will print the score of each bin
	//if (i == numFFTBins - 1)
	//	Serial.println(binScores[i]);
	//else {
	//	Serial.print(binScores[i]);
	//	Serial.print("\t");
	//}
}

void musicAnalytics() {
	// Write FFT data to spectrumValue[ ] array
	fillStats();

	maxConstBeat = 0; // reset this before it is set within beatTiming().
	// All the deep analytics happen from the functions in this for loop
	for (int i = 0; i < numFFTBins; i++) {
		beatDetection(i);
		beatTiming(i);
		//printDetectedBeats(i); // diagnostic function, uncomment to see if beats are being detected.
		scoreBins(i);

		// Fill Fade Values
		if (spectrumValue[i] > average[i])
			fadeVals[i] += (spectrumValue[i] - average[i]) / 2;
		else
			//fadeVals[i] -= (average[i] - spectrumValue[i]) / 2;
			fadeVals[i] -= 5;

		fadeVals[i] = constrain(fadeVals[i], 0, 255);
	}

	// This for loop checks the lowest 4 bins, mid 4 bins, high 4 bins. (Some bins are not counted b/c they're in between. Might need fixing)
	for (int i = 0; i < 4; i++) {
		// Find the beats in the low, mid, and high ranges
		if (binScore[i] > binScore[lowBeatBin]) {
      lowBeatBin = i;
    }
    // binScore[4] is not measured
		if (binScore[i + 5] > binScore[midBeatBin]) {
      midBeatBin = i + 5;
    }
		if (binScore[i + 9] > binScore[highBeatBin]) {
      highBeatBin = i + 9;
    }

		// Find highest averages in the low, mid, and high ranges
    // if (average[i] > average[lowAveBin]) {
    //   lowAveBin = i;
    // }
		// if (average[i + 5] > average[midAveBin]) {
    //   midAveBin = i + 5;
    // }
		// if (average[i + 9] > average[highAveBin]) {
    //   highAveBin = i + 9;
    // }
	}

	// This local variable is what moves the mixAmount variable
	static int mixAmountInfluencer;

	// If a beat is detected or there's currently a constant beat over 8 counts, increase mixAmount
	if (beatDetected[lowBeatBin] == 2 || constantBeatCounter[lowBeatBin] > 8)
		mixAmountInfluencer += 5;

	// Otherwise, it is constantly decreasing
	EVERY_N_MILLIS(150)
		mixAmountInfluencer -= 1;

	mixAmountInfluencer = constrain(mixAmountInfluencer, -10, 10);
	mixAmount = constrain(mixAmount + mixAmountInfluencer, 0, 255);
	//Serial.print(mixAmountInfluencer);
	//Serial.print("\t");
	//Serial.println(mixAmount);
}

void printBeatBins() {
	// Prints all the beat bins
	//Serial.print(lowBeatBin);
	//Serial.print("\t");
	//Serial.print(binScore[lowBeatBin]);
	//Serial.print("\t");
	//Serial.print(midBeatBin);
	//Serial.print("\t");
	//Serial.print(binScore[midBeatBin]);
	//Serial.print("\t");
	//Serial.print(highBeatBin);
	//Serial.print("\t");
	//Serial.println(binScore[highBeatBin]);
}

void printNumber(float n) {
  if (n >= 0.004) {
    Serial.print(n, 3);
    Serial.print(" ");
  }
  else {
    //  Serial.print("   -  "); // don't print "0.00"
    Serial.print("   0  "); // print 0 for excel purposes
  }
}


// Used for diagnostics / watching whether a song is having beats detected
void printDetectedBeats(int i) {
	// These print statements will print the beat status of each bin
	if (i == numFFTBins - 1) Serial.println(beatDetected[i]);
	else {
		Serial.print(beatDetected[i]);
		Serial.print("\t");
	}
}

  void autoMusicVisualizing() {
    // A local variable to remember the last mix amount between leds2 and leds3
    static int mixAmountOld;

    // While a beat is present (mixAmount = 255), change the color palette used in non-beat pattern and change pattern in leds2
    // "mixAmountOld != 255" is used so it only changes the color palette once
    if (mixAmount == 255 && mixAmountOld != 255) {
      // TODO: nextMusicWithNoBeatPattern(); // chooses new non-beat pattern
    }

    // While no beat is present (mixAmount = 0), change the beat-dependent pattern
    // "mixAmountOld != 0" is used so it only chooses a new pattern once
    if (mixAmount == 0 && mixAmountOld != 0) {
      // TODO: nextLowBeatPattern(); // chooses new beat-dependent pattern
    }
    
    int effectNew = 0;
    if (silence) {
      // TODO: change effect to one for silence
      effectNew = FX_MODE_STATIC;
    }
    else {
      if (constBeat) {
        // TOOO: select beat effect
        effectNew = FX_MODE_2DFUNKYPLANK;
      }
      else {
        // TOOO: select non-beat effect
        effectNew = FX_MODE_2DGEQ;
      }
    }
    if(effectCurrent != effectNew) {
      DEBUG_PRINTF("ARA: Changed effect to %u\n", effectNew);
      effectCurrent = effectNew;
      colorUpdated(CALL_MODE_NO_NOTIFY);
//      stateUpdated(CALL_MODE_DIRECT_CHANGE);
    }
    else {
      // DEBUG_PRINTF("ARA: effect current %u\n", effectCurrent);
    }
    // applyPreset(effectNew);

   mixAmountOld = mixAmount;

  } 

   //Your usermod will remain compatible as it does not need to implement all methods from the Usermod base class!
};


// add more strings here to reduce flash memory usage


// implementation of non-inline member methods

void AudioReactiveAuto::publishMqtt(const char* state, bool retain)
{
#ifndef WLED_DISABLE_MQTT
  //Check if MQTT Connected, otherwise it will crash the 8266
  if (WLED_MQTT_CONNECTED) {
    char subuf[64];
    strcpy(subuf, mqttDeviceTopic);
    strcat_P(subuf, PSTR("/example"));
    mqtt->publish(subuf, 0, retain, state);
  }
#endif
}
