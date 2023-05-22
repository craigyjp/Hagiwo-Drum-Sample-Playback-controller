#include "SettingsService.h"

void settingsMIDICh(int index, const char *value);
void settingsEncoderDir(char *value);

int currentIndexMIDICh();
int currentIndexEncoderDir();

void settingsMIDICh(int index, const char *value) {
  if (strcmp(value, "ALL") == 0) {
    midiChannel = MIDI_CHANNEL_OMNI;
  } else {
    midiChannel = atoi(value);
  }
  storeMidiChannel(midiChannel);
}

void settingsEncoderDir(int index, const char *value) {
  if (strcmp(value, "Type 1") == 0) {
    encCW = true;
  } else {
    encCW = false;
  }
  storeEncoderDir(encCW ? 1 : 0);
}

int currentIndexMIDICh() {
  return getMIDIChannel();
}

int currentIndexEncoderDir() {
  return getEncoderDir() ? 0 : 1;
}

// add settings to the circular buffer
void setUpSettings() {
  settings::append(settings::SettingsOption{ "MIDI In Ch.", { "All", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "\0" }, settingsMIDICh, currentIndexMIDICh });
  settings::append(settings::SettingsOption{ "Encoder", { "Type 1", "Type 2", "\0" }, settingsEncoderDir, currentIndexEncoderDir });
}