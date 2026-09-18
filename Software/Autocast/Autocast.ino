#include "BluetoothA2DPSink.h"
#include "AutocastDisplay.h"

BluetoothA2DPSink a2dp_sink;
AutocastDisplay display;

int last_volume, current_volume;

struct TrackInfo {
  String title = "";
  String artist = "";
} currentTrack;

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

enum display_information_t { NOT_CONNECTED,
                              NOW_PLAYING,
                              VOLUME_OVERLAY } display_information = NOT_CONNECTED;

void connection_state_changed(esp_a2d_connection_state_t state, void* ptr) {
  if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
    display_information = NOW_PLAYING;
  } else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
    display_information = NOT_CONNECTED;
  }
}

void audio_state_changed(esp_a2d_audio_state_t state, void* ptr) {
  switch (state) {
    case ESP_A2D_AUDIO_STATE_STARTED:
      display_information = NOW_PLAYING;
      break;
    default:
      break;
  }
}

void setup() {
  i2s_pin_config_t pin_config = {
    .bck_io_num = 26,
    .ws_io_num = 25,
    .data_out_num = 22,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  a2dp_sink.set_pin_config(pin_config);

  display.begin(5, 18, 19, 23); // no miso

  Serial.begin(115200);
  a2dp_sink.set_on_connection_state_changed(connection_state_changed);
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.set_on_audio_state_changed(audio_state_changed);
  a2dp_sink.start("Mazda 323");
  last_volume = current_volume = a2dp_sink.get_volume();
}

String top_display_content;
String bottom_display_content;

uint16_t overlay_duration_ms = 1000;
unsigned long timestamp = 0;
bool overlay_on = false;

void loop() {
  current_volume = a2dp_sink.get_volume();

  if (current_volume != last_volume) {
    display_information = VOLUME_OVERLAY;
    last_volume = current_volume;
    timestamp = millis();
  }

  switch (display_information) {
    case NOT_CONNECTED:
      top_display_content = "   Ready to";
      bottom_display_content = "   connect";
      break;

    case NOW_PLAYING:
      top_display_content = currentTrack.title;
      bottom_display_content = currentTrack.artist;

      if (top_display_content.length() < 1 && bottom_display_content.length() < 1) {
        top_display_content = "    Device";
        bottom_display_content = "   connected";
      }
      break;

    case VOLUME_OVERLAY:
      if (overlay_on) {
        if (millis() - timestamp > overlay_duration_ms) {
          overlay_on = false;
          display_information = NOW_PLAYING;
        }
      } else {
        overlay_on = true;
        timestamp = millis();
      }

      top_display_content = "Volume";
      bottom_display_content = "[";

      for (int i = 0; i < current_volume; i += 11) {
        bottom_display_content += "-";
      }
      for (int i = bottom_display_content.length(); i < 13; i++) {
        bottom_display_content += " ";
      }
      bottom_display_content += "]";
      break;
  }

  display.update(top_display_content, bottom_display_content, 300);

  delay(10);
}
