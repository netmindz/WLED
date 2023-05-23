#include "wled.h"

/*
 * Support for DMX Output via MAX485.
 * Change the output pin in src/dependencies/ESPDMX.cpp, if needed (ESP8266)
 * Change the output pin in src/dependencies/SparkFunDMX.cpp, if needed (ESP32)
 * ESP8266 Library from:
 * https://github.com/Rickgg/ESP-Dmx
 * ESP32 Library from:
 * https://github.com/sparkfun/SparkFunDMX
 */

#ifdef WLED_ENABLE_DMX

// WLEDMM: seems that DMX output triggers watchdog resets when compiling for IDF 4.4.x
#ifdef ARDUINO_ARCH_ESP32
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 2, 0)
#warning DMX output support might cause watchdog reset when compiling with ESP-IDF V4.4.x
// E (24101) task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time:
// E (24101) task_wdt:  - IDLE (CPU 0)
// E (24101) task_wdt: Tasks currently running:
// E (24101) task_wdt: CPU 0: FFT
// E (24101) task_wdt: CPU 1: IDLE
// E (24101) task_wdt: Aborting.
//abort() was called at PC 0x40143b6c on core 0
#endif
#endif

void handleDMX()
{
  // don't act, when in DMX Proxy mode
  if (e131ProxyUniverse != 0) return;

  uint8_t brightness = strip.getBrightness();

  bool calc_brightness = true;

   // check if no shutter channel is set
   for (byte i = 0; i < DMXChannels; i++)
   {
     if (DMXFixtureMap[i] == 5) calc_brightness = false;
   }

  uint16_t len = strip.getLengthTotal();
  for (int i = DMXStartLED; i < len; i++) {        // uses the amount of LEDs as fixture count

    uint32_t in = strip.getPixelColor(i);     // get the colors for the individual fixtures as suggested by Aircoookie in issue #462
    byte w = W(in);
    byte r = R(in);
    byte g = G(in);
    byte b = B(in);

    int DMXFixtureStart = DMXStart + (DMXGap * (i - DMXStartLED));
    for (int j = 0; j < DMXChannels; j++) {
      int DMXAddr = DMXFixtureStart + j;
      switch (DMXFixtureMap[j]) {
        case 0:        // Set this channel to 0. Good way to tell strobe- and fade-functions to fuck right off.
          dmx.write(DMXAddr, 0);
          break;
        case 1:        // Red
          dmx.write(DMXAddr, calc_brightness ? (r * brightness) / 255 : r);
          break;
        case 2:        // Green
          dmx.write(DMXAddr, calc_brightness ? (g * brightness) / 255 : g);
          break;
        case 3:        // Blue
          dmx.write(DMXAddr, calc_brightness ? (b * brightness) / 255 : b);
          break;
        case 4:        // White
          dmx.write(DMXAddr, calc_brightness ? (w * brightness) / 255 : w);
          break;
        case 5:        // Shutter channel. Controls the brightness.
          dmx.write(DMXAddr, brightness);
          break;
        case 6:        // Sets this channel to 255. Like 0, but more wholesome.
          dmx.write(DMXAddr, 255);
          break;
      }
    }
  }

  dmx.update();        // update the DMX bus
}

void initDMX() {
 #ifdef ESP8266
  dmx.init(512);        // initialize with bus length
 #else
  dmx.initWrite(512);  // initialize with bus length
 #endif
}
#else
void handleDMX() {}
#endif

#define WLED_ENABLE_DMX_INPUT
#ifdef WLED_ENABLE_DMX_INPUT

#include <esp_dmx.h>
#include "rdm/mdb.h"

// Define a custom response to RDM_PID_SOFTWARE_VERSION_LABEL
rdm_response_type_t rdm_software_version_label(dmx_port_t dmx_num,
                                               const rdm_header_t *header,
                                               rdm_mdb_t *mdb, void *context) {
  // Log the request for testing purposes only
  ESP_LOGI(TAG, "Received RDM_PID_SOFTWARE_VERSION_LABEL request");

  // Ensure that the parameter data is the expected length
  if (mdb->pdl != 0) {
    rdm_encode_nack_reason(mdb, RDM_NR_FORMAT_ERROR);
    return RDM_RESPONSE_TYPE_NACK_REASON;
  }

  // Ensure that the CC is correct
  if (header->cc != RDM_CC_GET_COMMAND) {
    // RDM_PID_SOFTWARE_VERSION_LABEL only supports GET requests
    rdm_encode_nack_reason(mdb, RDM_NR_UNSUPPORTED_COMMAND_CLASS);
    return RDM_RESPONSE_TYPE_NACK_REASON;
  }

  // Encode the response
  const char *sw_version_label = (const char *)context;
  rdm_encode_string(mdb, sw_version_label, strlen(sw_version_label));
  return RDM_RESPONSE_TYPE_ACK;
}

dmx_port_t dmxPort = 2;
void initDMX() {
/* Set the DMX hardware pins to the pins that we want to use. */
  if(dmxReceivePin > 0) {
    USER_PRINTF("Listening for DMX on pin %u\n", dmxReceivePin);
    dmx_set_pin(dmxPort, dmxTransmitPin, dmxReceivePin, dmxEnablePin);
  }
  else {
    USER_PRINTLN("DMX input disabled due to dmxReceivePin not being set");
    return;
  }

  /* Now we can install the DMX driver! We'll tell it which DMX port to use and
    which interrupt priority it should have. If you aren't sure which interrupt
    priority to use, you can use the macro `DMX_DEFAULT_INTR_FLAG` to set the
    interrupt to its default settings.*/
  dmx_driver_install(dmxPort, ESP_INTR_FLAG_LEVEL3);

  // Set the device info for this device driver
  const rdm_device_info_t device_info = {
      .model_id = 1,  // An arbitrary value defined by the user
      .product_category = RDM_PRODUCT_CATEGORY_FIXTURE,
      .software_version_id = VERSION,  // An arbitrary value defined by the user
      .footprint = 1,
      .current_personality = 1,  // Begins at 1, not 0
      .personality_count = 1,
      .start_address = DMXAddress,
      .sub_device_count = 0,
      .sensor_count = 0};
  rdm_driver_set_device_info(dmxPort, &device_info);

  /* Register the custom callback. This overwrites the default
    RDM_PID_SOFTWARE_VERSION_LABEL response. */
  const char *sw_version_label = "WLED";
  rdm_register_callback(dmxPort, RDM_PID_SOFTWARE_VERSION_LABEL,
                        rdm_software_version_label, (void *)sw_version_label);

}
  
bool dmxIsConnected = false;
unsigned long dmxLastUpdate = 0;

void handleDMXInput() {
  if(dmxReceivePin < 1) {
    return;
  }
  byte dmxdata[DMX_PACKET_SIZE];
  dmx_packet_t packet;
  unsigned long now = millis();
  if (dmx_receive(dmxPort, &packet, 0)) {

    /* We should check to make sure that there weren't any DMX errors. */
    if (!packet.err) {
      /* If this is the first DMX data we've received, lets log it! */
      if (!dmxIsConnected) {
        USER_PRINTLN("DMX is connected!");
        dmxIsConnected = true;
      }

      dmx_read(dmxPort, dmxdata, packet.size);
      handleDMXData(1, 512, dmxdata, REALTIME_MODE_DMX, 0);
      dmxLastUpdate = now;

    } else {
      /* Oops! A DMX error occurred! Don't worry, this can happen when you first
        connect or disconnect your DMX devices. If you are consistently getting
        DMX errors, then something may have gone wrong with your code or
        something is seriously wrong with your DMX transmitter. */
     DEBUG_PRINT("A DMX error occurred - ");
     DEBUG_PRINTLN(packet.err);
    }
  }
  else if (dmxIsConnected && (now - dmxLastUpdate > 5000)) {
    dmxIsConnected = false;
    USER_PRINTLN("DMX was disconnected.");
  }
}
#else
void initDMX();
#endif