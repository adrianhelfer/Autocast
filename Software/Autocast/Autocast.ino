#include <Arduino.h>
#include "BluetoothA2DPSink.h"
#include "AutocastDisplay.h"

// ---------------------------------------------------------------------
// Pin definitions
// ---------------------------------------------------------------------
const uint8_t SENSOR_VP_PIN = 36;   // adjust to your actual wiring
const uint8_t SENSOR_VN_PIN = 39;

const int CONFIG_PIN_SER   = 2;   // IO2  -> SER (serial data in)
const int CONFIG_PIN_SRCLK = 17;  // IO17 -> SRCLK (shift register clock)
const int CONFIG_PIN_RCLK  = 13;  // IO13 -> RCLK (storage register clock / latch)

const int ILLUMINATION_SIGNAL_PIN = 4; // HIGH when headlights or parking lights are switched on
const int DIMM_DISPLAY_PIN = 15; // Display gets darker when set to HIGH

// =======================================================================
// TYPE DEFINITIONS (must precede all forward declarations / function use)
// =======================================================================

// ---- Top-level input selection state ----
enum class InputSelection : uint8_t {
  OFF,
  BLUETOOTH,
  USB,
  ERROR_STATE
};

// ---- Bluetooth-mode-local display state ----
enum bt_display_information_t {
  BT_NOT_CONNECTED,
  BT_NOW_PLAYING,
  BT_VOLUME_OVERLAY
};

struct TrackInfo {
  String title = "";
  String artist = "";
};

// =======================================================================
// FORWARD DECLARATIONS
// =======================================================================

// ---- State machine core ----
InputSelection decodeState(int vpState, int vnState);
void enterState(InputSelection s);
void exitState(InputSelection s);
void runState(InputSelection s);

// ---- OFF ----
void enterOffMode();
void exitOffMode();
void runOffMode();

// ---- BLUETOOTH ----
void enterBluetoothMode();
void exitBluetoothMode();
void runBluetoothMode();
void avrc_metadata_callback(uint8_t id, const uint8_t* text);
void connection_state_changed(esp_a2d_connection_state_t state, void* ptr);
void audio_state_changed(esp_a2d_audio_state_t state, void* ptr);

// ---- USB ----
void enterUsbMode();
void exitUsbMode();
void runUsbMode();

// ---- ERROR ----
void enterErrorMode();
void exitErrorMode();
void runErrorMode();

void shiftOutByte(uint8_t data, bool msbFirst = true) {
  for (int i = 0; i < 8; i++) {
    // Pick the correct bit depending on shift order
    uint8_t bit = msbFirst ? ((data >> (7 - i)) & 0x01)
                           : ((data >> i) & 0x01);

    digitalWrite(CONFIG_PIN_SER, bit);

    // Pulse SRCLK to shift the bit in
    digitalWrite(CONFIG_PIN_SRCLK, HIGH);
    delayMicroseconds(1);   // tiny settle time, optional at low speed
    digitalWrite(CONFIG_PIN_SRCLK, LOW);
  }

  // Latch the shifted data to the output pins
  digitalWrite(CONFIG_PIN_RCLK, HIGH);
  delayMicroseconds(1);
  digitalWrite(CONFIG_PIN_RCLK, LOW);
}


// =======================================================================
// GLOBALS
// =======================================================================

InputSelection autocast_input_selection = InputSelection::OFF;

// ---- Debounce configuration ----
const uint8_t DEBOUNCE_THRESHOLD       = 5;   // consecutive agreeing samples required
const unsigned long SAMPLE_INTERVAL_MS = 10;  // time between samples, ms

static InputSelection candidateState  = InputSelection::OFF;
static uint8_t        debounceCounter = 0;
static unsigned long  lastSampleTime  = 0;

// ---- Bluetooth (A2DP sink) globals ----
BluetoothA2DPSink a2dp_sink;
AutocastDisplay display;   // shared display hardware, used by any mode

int bt_last_volume, bt_current_volume;
TrackInfo currentTrack;

bt_display_information_t bt_display_information = BT_NOT_CONNECTED;

String bt_top_display_content;
String bt_bottom_display_content;

const uint16_t BT_OVERLAY_DURATION_MS = 1000;
unsigned long  bt_overlay_timestamp   = 0;
bool           bt_overlay_on          = false;

// =======================================================================
// STATE MACHINE CORE
// =======================================================================

InputSelection decodeState(int vpState, int vnState) {
  bool vp = (vpState != LOW);
  bool vn = (vnState != LOW);

  if (vp && vn)        return InputSelection::OFF;
  else if (!vp && vn)  return InputSelection::BLUETOOTH;
  else if (vp && !vn)  return InputSelection::USB;
  else                 return InputSelection::ERROR_STATE;
}

void enterState(InputSelection s) {
  switch (s) {
    case InputSelection::OFF:         enterOffMode();       break;
    case InputSelection::BLUETOOTH:   enterBluetoothMode(); break;
    case InputSelection::USB:         enterUsbMode();       break;
    case InputSelection::ERROR_STATE: enterErrorMode();     break;
  }
}

void exitState(InputSelection s) {
  switch (s) {
    case InputSelection::OFF:         exitOffMode();       break;
    case InputSelection::BLUETOOTH:   exitBluetoothMode(); break;
    case InputSelection::USB:         exitUsbMode();       break;
    case InputSelection::ERROR_STATE: exitErrorMode();     break;
  }
}

void runState(InputSelection s) {
  switch (s) {
    case InputSelection::OFF:         runOffMode();       break;
    case InputSelection::BLUETOOTH:   runBluetoothMode(); break;
    case InputSelection::USB:         runUsbMode();       break;
    case InputSelection::ERROR_STATE: runErrorMode();     break;
  }
}

// =======================================================================
// OFF MODE
// =======================================================================

void enterOffMode() {
  shiftOutByte(0b00000000);
  display.update("", "", 300);
}

void exitOffMode() { }

void runOffMode() { }

// =======================================================================
// BLUETOOTH MODE
// =======================================================================

void avrc_metadata_callback(uint8_t id, const uint8_t* text) {
  String metadata = String((char*)text);

  switch (id) {
    case ESP_AVRC_MD_ATTR_TITLE:
      metadata.replace(" • Lossless", "");
      currentTrack.title = AutocastDisplay::asciiIfy(metadata);
      break;
    case ESP_AVRC_MD_ATTR_ARTIST:
      metadata.replace(" • Lossless", "");
      currentTrack.artist = AutocastDisplay::asciiIfy(metadata);
      break;
  }
}

void connection_state_changed(esp_a2d_connection_state_t state, void* ptr) {
  if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
    bt_display_information = BT_NOW_PLAYING;
  } else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
    bt_display_information = BT_NOT_CONNECTED;
  }
}

void audio_state_changed(esp_a2d_audio_state_t state, void* ptr) {
  switch (state) {
    case ESP_A2D_AUDIO_STATE_STARTED:
      bt_display_information = BT_NOW_PLAYING;
      break;
    default:
      break;
  }
}

void enterBluetoothMode() {
  shiftOutByte(0b00001011);

  i2s_pin_config_t pin_config = {
    .bck_io_num = 26,
    .ws_io_num = 25,
    .data_out_num = 22,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  a2dp_sink.set_pin_config(pin_config);

  a2dp_sink.set_on_connection_state_changed(connection_state_changed);
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.set_on_audio_state_changed(audio_state_changed);
  a2dp_sink.start("Mazda 323");

  bt_last_volume = bt_current_volume = a2dp_sink.get_volume();
  bt_display_information = BT_NOT_CONNECTED;
  bt_overlay_on = false;
  currentTrack.title = "";
  currentTrack.artist = "";
}

void exitBluetoothMode() {
  // Tear down the A2DP sink so re-entering BLUETOOTH mode later starts clean.
  a2dp_sink.end(true);
}

void runBluetoothMode() {

  // dimm display at headlight signal
  digitalWrite(DIMM_DISPLAY_PIN, digitalRead(ILLUMINATION_SIGNAL_PIN));

  bt_current_volume = a2dp_sink.get_volume();

  if (bt_current_volume != bt_last_volume) {
    bt_display_information = BT_VOLUME_OVERLAY;
    bt_last_volume = bt_current_volume;
    bt_overlay_timestamp = millis();
  }

  switch (bt_display_information) {
    case BT_NOT_CONNECTED:
      bt_top_display_content = "   Ready to";
      bt_bottom_display_content = "   connect";
      break;

    case BT_NOW_PLAYING:
      bt_top_display_content = currentTrack.title;
      bt_bottom_display_content = currentTrack.artist;

      if (bt_top_display_content.length() < 1 && bt_bottom_display_content.length() < 1) {
        bt_top_display_content = "    Device";
        bt_bottom_display_content = "   connected";
      }
      break;

    case BT_VOLUME_OVERLAY:
      if (bt_overlay_on) {
        if (millis() - bt_overlay_timestamp > BT_OVERLAY_DURATION_MS) {
          bt_overlay_on = false;
          bt_display_information = BT_NOW_PLAYING;
        }
      } else {
        bt_overlay_on = true;
        bt_overlay_timestamp = millis();
      }

      bt_top_display_content = "Volume";
      bt_bottom_display_content = "[";

      for (int i = 0; i < bt_current_volume; i += 11) {
        bt_bottom_display_content += "-";
      }
      for (int i = bt_bottom_display_content.length(); i < 13; i++) {
        bt_bottom_display_content += " ";
      }
      bt_bottom_display_content += "]";
      break;
  }

  display.update(bt_top_display_content, bt_bottom_display_content, 300);

  delay(10); // paces display refresh / BT servicing, mirrors original loop cadence
}

// =======================================================================
// USB MODE
// =======================================================================

void enterUsbMode() { 
  shiftOutByte(0b00011111);
}

void exitUsbMode() { }

void runUsbMode() {
  digitalWrite(DIMM_DISPLAY_PIN, digitalRead(ILLUMINATION_SIGNAL_PIN));
  display.update("Charging with", "1.22 A 15.01 W", 300);
}

// =======================================================================
// ERROR MODE
// =======================================================================

void enterErrorMode() {
  display.update("Failed to read", "selection", 300);
}

void exitErrorMode() { /* clear fault indication */ }

void runErrorMode() { /* e.g. blink an error LED */ }

// =======================================================================
// SETUP / LOOP
// =======================================================================

void setup() {
  Serial.begin(115200);

  // Configuration register
  pinMode(CONFIG_PIN_SER, OUTPUT);
  pinMode(CONFIG_PIN_SRCLK, OUTPUT);
  pinMode(CONFIG_PIN_RCLK, OUTPUT);

  digitalWrite(CONFIG_PIN_SER, LOW);
  digitalWrite(CONFIG_PIN_SRCLK, LOW);
  digitalWrite(CONFIG_PIN_RCLK, LOW);

  // Input selector
  pinMode(SENSOR_VP_PIN, INPUT);
  pinMode(SENSOR_VN_PIN, INPUT);

  // Display dimming
  pinMode(ILLUMINATION_SIGNAL_PIN, INPUT);
  pinMode(DIMM_DISPLAY_PIN, OUTPUT);

  // Display hardware is shared across modes, so it's brought up once here
  // rather than inside any single mode's entry point.
  display.begin(5, 18, 19, 23); // no miso. pin 19 used for something else. fix later

  // Prime the state machine with an immediate (un-debounced) read so we
  // start in a sensible state rather than always booting into OFF.
  int vp = digitalRead(SENSOR_VP_PIN);
  int vn = digitalRead(SENSOR_VN_PIN);
  autocast_input_selection = decodeState(vp, vn);
  candidateState = autocast_input_selection;
  debounceCounter = DEBOUNCE_THRESHOLD;

  enterState(autocast_input_selection);
}

void loop() {
  unsigned long now = millis();

  // Only sample the pins at a fixed cadence; do real work every loop.
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;

    int vpState = digitalRead(SENSOR_VP_PIN);
    int vnState = digitalRead(SENSOR_VN_PIN);
    InputSelection sampled = decodeState(vpState, vnState);

    if (sampled == candidateState) {
      // Reading agrees with our running candidate: count it, capped at
      // the threshold so the counter can't overflow over a long stable run.
      if (debounceCounter < DEBOUNCE_THRESHOLD) {
        debounceCounter++;
      }
    } else {
      // Reading disagrees: start a fresh count for the new candidate.
      candidateState  = sampled;
      debounceCounter = 1;
    }

    // Commit the transition only once the candidate has been stable for
    // DEBOUNCE_THRESHOLD consecutive samples.
    if (debounceCounter >= DEBOUNCE_THRESHOLD &&
        candidateState != autocast_input_selection) {
      exitState(autocast_input_selection);
      autocast_input_selection = candidateState;
      enterState(autocast_input_selection);
    }
  }

  runState(autocast_input_selection);
}
