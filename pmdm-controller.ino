/*
  Poor Mans Drum Machine controller MUX
  

  Includes code by:
    Dave Benn - Handling MUXs, a few other bits and original inspiration  https://www.notesandvolts.com/2019/01/teensy-synth-part-10-hardware.html

  Arduino IDE
  Tools Settings:
  Board: "Teensy3.5"
  USB Type: "Serial + MIDI + Audio"
  CPU Speed: "120"
  Optimize: "Fastest"

  Additional libraries:
    Agileware CircularBuffer available in Arduino libraries manager
    Replacement files are in the Modified Libraries folder and need to be placed in the teensy Audio folder.
*/

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>
#include "MidiCC.h"
#include "Constants.h"
#include "Parameters.h"
#include "Drums.h"
#include "PatchMgr.h"
#include "HWControls.h"
#include "EepromMgr.h"
#include "Settings.h"
#include <ShiftRegister74HC595.h>

#define PARAMETER 0      //The main page for displaying the current patch and control (parameter) changes
#define RECALL 1         //Patches list
#define SAVE 2           //Save patch page
#define REINITIALISE 3   // Reinitialise message
#define PATCH 4          // Show current patch bypassing PARAMETER
#define PATCHNAMING 5    // Patch naming page
#define DELETE 6         //Delete patch page
#define DELETEMSG 7      //Delete patch message page
#define SETTINGS 8       //Settings page
#define SETTINGSVALUE 9  //Settings page

unsigned int state = PARAMETER;

#include "ST7735Display.h"

boolean cardStatus = false;

//MIDI 5 Pin DIN
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);  //RX - Pin 0

int count = 0;  //For MIDI Clk Sync
int DelayForSH3 = 50;
int patchNo = 0;
unsigned long buttonDebounce = 0;

//ShiftRegister74HC595<3> sr(44, 45, 46);

void setup() {
  SPI.begin();
  setupDisplay();
  setUpSettings();
  setupHardware();

  cardStatus = SD.begin(BUILTIN_SDCARD);
  if (cardStatus) {
    Serial.println("SD card is connected");
    //Get patch numbers and names from SD card
    loadPatches();
    if (patches.size() == 0) {
      //save an initialised patch to SD card
      savePatch("1", INITPATCH);
      loadPatches();
    }
  } else {
    Serial.println("SD card is not connected or unusable");
    reinitialiseToPanel();
    showPatchPage("No SD", "conn'd / usable");
  }

  //Read MIDI Channel from EEPROM
  midiChannel = getMIDIChannel();
  Serial.println("MIDI Ch:" + String(midiChannel) + " (0 is Omni On)");

  //USB Client MIDI
  usbMIDI.setHandleControlChange(myConvertControlChange);
  usbMIDI.setHandleProgramChange(myProgramChange);
  Serial.println("USB Client MIDI Listening");

  //MIDI 5 Pin DIN
  MIDI.begin();
  MIDI.setHandleControlChange(myConvertControlChange);
  MIDI.setHandleProgramChange(myProgramChange);
  //MIDI.setHandleNoteOn(DinHandleNoteOn);
  //MIDI.setHandleNoteOff(DinHandleNoteOff);
  MIDI.turnThruOn(midi::Thru::Mode::Off);
  Serial.println("MIDI In DIN Listening");

  //Read Encoder Direction from EEPROM
  encCW = getEncoderDir();
  patchNo = getLastPatch();
  recallPatch(patchNo);  //Load first patch

  //  reinitialiseToPanel();
}

void allNotesOff() {
}

void updateVariables() {
  switch (drum_number) {
    case 1:
      sample = sample1;
      volume = volume1;
      tuning = tuning1;
      filter = filter1;
      break;

    case 2:
      sample = sample2;
      volume = volume2;
      tuning = tuning2;
      filter = filter2;
      break;

    case 3:
      sample = sample3;
      volume = volume3;
      tuning = tuning3;
      filter = filter3;
      break;

    case 4:
      sample = sample4;
      volume = volume4;
      tuning = tuning4;
      filter = filter4;
      break;

    case 5:
      sample = sample5;
      volume = volume5;
      tuning = tuning5;
      filter = filter5;
      break;

    case 6:
      sample = sample6;
      volume = volume6;
      tuning = tuning6;
      filter = filter6;
      break;

    case 7:
      sample = sample7;
      volume = volume7;
      tuning = tuning7;
      filter = filter7;
      break;

    case 8:
      sample = sample8;
      volume = volume8;
      tuning = tuning8;
      filter = filter8;
      break;

    case 9:
      sample = sample9;
      volume = volume9;
      tuning = tuning9;
      filter = filter9;
      break;

    case 10:
      sample = sample10;
      volume = volume10;
      tuning = tuning10;
      filter = filter10;
      break;

    case 11:
      sample = sample11;
      volume = volume11;
      tuning = tuning11;
      filter = filter11;
      break;

    case 12:
      sample = sample12;
      volume = volume12;
      tuning = tuning12;
      filter = filter12;
      break;

    case 13:
      sample = sample13;
      volume = volume13;
      tuning = tuning13;
      filter = filter13;
      break;

    case 14:
      sample = sample14;
      volume = volume14;
      tuning = tuning14;
      filter = filter14;
      break;

    case 15:
      sample = sample15;
      volume = volume15;
      tuning = tuning15;
      filter = filter15;
      break;

    case 16:
      sample = sample16;
      volume = volume16;
      tuning = tuning16;
      filter = filter16;
      break;
  }
  if (drumsample) {
    updateDrum_Sample();
  }
  if (drumvolume) {
    updateVolume();
  }
  if (drumfilter) {
    updateFilterSW(0);
  }
  if (drumtuning) {
    updateTuning();
  }
}

void updateDrumNumber() {
  updateVariables();
  updateFilterSW(0);
}

void updateFilterSW(boolean announce) {

  if (filter == 127) {
    if ((announce) || (drumfilter && (prevfilter != filter))) {
      showCurrentParameterPage("LP Filter", String("On"));

      drumsample = 0;
      drumtuning = 0;
      drumvolume = 0;
      drumfilter = 1;
    }
    digitalWrite(FILTER_LED, HIGH);
  }
  if (filter == 0) {
    if ((announce) || (drumfilter && (prevfilter != filter))) {
      showCurrentParameterPage("LP Filter", String("Off"));

      drumsample = 0;
      drumtuning = 0;
      drumvolume = 0;
      drumfilter = 1;
    }
    digitalWrite(FILTER_LED, LOW);
  }
  prevfilter = filter;
}

void updateDrum_Sample() {
  if (sample < 13) {
  showCurrentParameterPage("Drum Sample", String(drum_names1[drum_number -1][sample -1]));
  }
  if (sample > 12 && sample < 25 ) {
  showCurrentParameterPage("Drum Sample", String(drum_names2[drum_number -1][sample -13]));
  }
    if (sample > 24 && sample < 37) {
  showCurrentParameterPage("Drum Sample", String(drum_names3[drum_number -1][sample -25]));
  }
  if (sample > 36 ) {
  showCurrentParameterPage("Drum Sample", String(drum_names4[drum_number -1][sample -37]));
  }
  drumsample = 1;
  drumtuning = 0;
  drumvolume = 0;
  drumfilter = 0;
}

void updateTuning() {
  showCurrentParameterPage("Drum Tuning", int(tuning / 8));
  drumsample = 0;
  drumtuning = 1;
  drumvolume = 0;
  drumfilter = 0;
}

void updateVolume() {
  showCurrentParameterPage("Drum Volume", int(volume / 8));
  drumsample = 0;
  drumtuning = 0;
  drumvolume = 1;
  drumfilter = 0;
}

void updatevolumeControl() {
  showCurrentParameterPage("Master Volume", int(volumeControlstr));
}

// ////////////////////////////////////////////////////////////////

void updatePatchname() {
  showPatchPage(String(patchNo), patchName);
}

void updateRecallDrum_Sample() {
  switch (drum_number) {
    case 1:
      midiPRGout(sample1);
      break;

    case 2:
      midiPRGout(sample2);
      break;

    case 3:
      midiPRGout(sample3);
      break;

    case 4:
      midiPRGout(sample4);
      break;

    case 5:
      midiPRGout(sample5);
      break;

    case 6:
      midiPRGout(sample6);
      break;

    case 7:
      midiPRGout(sample7);
      break;

    case 8:
      midiPRGout(sample8);
      break;

    case 9:
      midiPRGout(sample9);
      break;

    case 10:
      midiPRGout(sample10);
      break;

    case 11:
      midiPRGout(sample11);
      break;

    case 12:
      midiPRGout(sample12);
      break;

    case 13:
      midiPRGout(sample13);
      break;

    case 14:
      midiPRGout(sample14);
      break;

    case 15:
      midiPRGout(sample15);
      break;

    case 16:
      midiPRGout(sample16);
      break;
  }
}

void updateRecallTuning() {
  switch (drum_number) {
    case 1:
      midiCCOut(9, tuning1 / 8);
      break;

    case 2:
      midiCCOut(9, tuning2 / 8);
      break;

    case 3:
      midiCCOut(9, tuning3 / 8);
      break;

    case 4:
      midiCCOut(9, tuning4 / 8);
      break;

    case 5:
      midiCCOut(9, tuning5 / 8);
      break;

    case 6:
      midiCCOut(9, tuning6 / 8);
      break;

    case 7:
      midiCCOut(9, tuning7 / 8);
      break;

    case 8:
      midiCCOut(9, tuning8 / 8);
      break;

    case 9:
      midiCCOut(9, tuning9 / 8);
      break;

    case 10:
      midiCCOut(9, tuning10 / 8);
      break;

    case 11:
      midiCCOut(9, tuning11 / 8);
      break;

    case 12:
      midiCCOut(9, tuning12 / 8);
      break;

    case 13:
      midiCCOut(9, tuning13 / 8);
      break;

    case 14:
      midiCCOut(9, tuning14 / 8);
      break;

    case 15:
      midiCCOut(9, tuning15 / 8);
      break;

    case 16:
      midiCCOut(9, tuning16 / 8);
      break;
  }
}

void updateRecallFilterSW() {
  switch (drum_number) {
    case 1:
      midiCCOut(10, filter1);
      break;

    case 2:
      midiCCOut(10, filter2);
      break;

    case 3:
      midiCCOut(10, filter3);
      break;

    case 4:
      midiCCOut(10, filter4);
      break;

    case 5:
      midiCCOut(10, filter5);
      break;

    case 6:
      midiCCOut(10, filter6);
      break;

    case 7:
      midiCCOut(10, filter7);
      break;

    case 8:
      midiCCOut(10, filter8);
      break;

    case 9:
      midiCCOut(10, filter9);
      break;

    case 10:
      midiCCOut(10, filter10);
      break;

    case 11:
      midiCCOut(10, filter11);
      break;

    case 12:
      midiCCOut(10, filter12);
      break;

    case 13:
      midiCCOut(10, filter13);
      break;

    case 14:
      midiCCOut(10, filter14);
      break;

    case 15:
      midiCCOut(10, filter15);
      break;

    case 16:
      midiCCOut(10, filter16);
      break;
  }
}

void myConvertControlChange(byte channel, byte control, byte value) {
  myControlChange(channel, control, value);
}

void myControlChange(byte channel, byte control, int value) {

  switch (control) {

    case CCtuning:
      tuning = value;

      switch (drum_number) {
        case 1:
          tuning1 = tuning;
          midiCCOut(9, tuning / 8);
          break;

        case 2:
          tuning2 = tuning;
          midiCCOut(9, tuning / 8);
          break;

        case 3:
          tuning3 = tuning;
          midiCCOut(9, tuning / 8);
          break;

        case 4:
          tuning4 = tuning;
          midiCCOut(9, tuning / 8);
          break;

        case 5:
          tuning5 = tuning;
          midiCCOut(9, tuning / 8);
          break;

        case 6:
          tuning6 = tuning;
          midiCCOut(9, tuning / 8);
          break;

        case 7:
          tuning7 = tuning;
          midiCCOut(9, tuning / 8);
          break;

        case 8:
          tuning8 = tuning;
          midiCCOut(9, tuning / 8);
          break;

        case 9:
          tuning9 = tuning;
          midiCCOut(9, tuning / 8);
          break;

        case 10:
          tuning10 = tuning;
          midiCCOut(9, tuning / 8);
          break;

        case 11:
          tuning11 = tuning;
          midiCCOut(9, tuning / 8);
          break;

        case 12:
          tuning12 = tuning;
          midiCCOut(9, tuning / 8);
          break;

        case 13:
          tuning13 = tuning;
          midiCCOut(9, tuning / 8);
          break;

        case 14:
          tuning14 = tuning;
          midiCCOut(9, tuning / 8);
          break;

        case 15:
          tuning15 = tuning;
          midiCCOut(9, tuning / 8);
          break;

        case 16:
          tuning16 = tuning;
          midiCCOut(9, tuning / 8);
          break;
      }
      updateTuning();
      break;

    case CCvolume:
      volume = value;
      switch (drum_number) {
        case 1:
          volume1 = volume;
          break;

        case 2:
          volume2 = volume;
          break;

        case 3:
          volume3 = volume;
          break;

        case 4:
          volume4 = volume;
          break;

        case 5:
          volume5 = volume;
          break;

        case 6:
          volume6 = volume;
          break;

        case 7:
          volume7 = volume;
          break;

        case 8:
          volume8 = volume;
          break;

        case 9:
          volume9 = volume;
          break;

        case 10:
          volume10 = volume;
          break;

        case 11:
          volume11 = volume;
          break;

        case 12:
          volume12 = volume;
          break;

        case 13:
          volume13 = volume;
          break;

        case 14:
          volume14 = volume;
          break;

        case 15:
          volume15 = volume;
          break;

        case 16:
          volume16 = volume;
          break;
      }
      updateVolume();
      break;

    case CCvolumeControl:
      volumeControl = value;
      volumeControlstr = value / 8;
      updatevolumeControl();
      break;

    case CCdrum:
      drum_number = value;
      updateDrumNumber();
      break;

    case CCsample:
      sample = value;
      switch (drum_number) {
        case 1:
          sample1 = sample;
          midiPRGout(sample);
          break;

        case 2:
          sample2 = sample;
          midiPRGout(sample);
          break;

        case 3:
          sample3 = sample;
          midiPRGout(sample);
          break;

        case 4:
          sample4 = sample;
          midiPRGout(sample);
          break;

        case 5:
          sample5 = sample;
          midiPRGout(sample);
          break;

        case 6:
          sample6 = sample;
          midiPRGout(sample);
          break;

        case 7:
          sample7 = sample;
          midiPRGout(sample);
          break;

        case 8:
          sample8 = sample;
          midiPRGout(sample);
          break;

        case 9:
          sample9 = sample;
          midiPRGout(sample);
          break;

        case 10:
          sample10 = sample;
          midiPRGout(sample);
          break;

        case 11:
          sample11 = sample;
          midiPRGout(sample);
          break;

        case 12:
          sample12 = sample;
          midiPRGout(sample);
          break;

        case 13:
          sample13 = sample;
          midiPRGout(sample);
          ;
          break;

        case 14:
          sample14 = sample;
          midiPRGout(sample);
          break;

        case 15:
          sample15 = sample;
          midiPRGout(sample);
          break;

        case 16:
          sample16 = sample;
          midiPRGout(sample);
          break;
      }
      updateDrum_Sample();
      break;

    case CCfilterSW:
      value > 0 ? filterSW = 1 : filterSW = 0;
      if (filterSW) {
        filter = 127;
      } else {
        filter = 0;
      }
      switch (drum_number) {
        case 1:
          filter1 = filter;
          midiCCOut(10, filter);
          break;

        case 2:
          filter2 = filter;
          midiCCOut(10, filter);
          break;

        case 3:
          filter3 = filter;
          midiCCOut(10, filter);
          break;

        case 4:
          filter4 = filter;
          midiCCOut(10, filter);
          break;

        case 5:
          filter5 = filter;
          midiCCOut(10, filter);
          break;

        case 6:
          filter6 = filter;
          midiCCOut(10, filter);
          break;

        case 7:
          filter7 = filter;
          midiCCOut(10, filter);
          break;

        case 8:
          filter8 = filter;
          midiCCOut(10, filter);
          break;

        case 9:
          filter9 = filter;
          midiCCOut(10, filter);
          break;

        case 10:
          filter10 = filter;
          midiCCOut(10, filter);
          break;

        case 11:
          filter11 = filter;
          midiCCOut(10, filter);
          break;

        case 12:
          filter12 = filter;
          midiCCOut(10, filter);
          break;

        case 13:
          filter13 = filter;
          midiCCOut(10, filter);
          break;

        case 14:
          filter14 = filter;
          midiCCOut(10, filter);
          break;

        case 15:
          filter15 = filter;
          midiCCOut(10, filter);
          break;

        case 16:
          filter16 = filter;
          midiCCOut(10, filter);
          break;
      }
      updateFilterSW(1);
      break;

    case CCallnotesoff:
      allNotesOff();
      break;
  }
}

void myProgramChange(byte channel, byte program) {
  state = PATCH;
  patchNo = program + 1;
  recallPatch(patchNo);
  Serial.print("MIDI Pgm Change:");
  Serial.println(patchNo);
  state = PARAMETER;
}

void recallPatch(int patchNo) {
  old_drum_number = drum_number;
  allNotesOff();
  File patchFile = SD.open(String(patchNo).c_str());
  if (!patchFile) {
    Serial.println("File not found");
  } else {
    String data[NO_OF_PARAMS];  //Array of data read in
    recallPatchData(patchFile, data);
    setCurrentPatchData(data);
    patchFile.close();
    storeLastPatch(patchNo);
  }
}

void setCurrentPatchData(String data[]) {
  patchName = data[0];
  sample1 = data[1].toInt();
  sample2 = data[2].toInt();
  sample3 = data[3].toInt();
  sample4 = data[4].toInt();
  sample5 = data[5].toInt();
  sample6 = data[6].toInt();
  sample7 = data[7].toInt();
  sample8 = data[8].toInt();
  sample9 = data[9].toInt();
  sample10 = data[10].toInt();
  sample11 = data[11].toInt();
  sample12 = data[12].toInt();
  sample13 = data[13].toInt();
  sample14 = data[14].toInt();
  sample15 = data[15].toInt();
  sample16 = data[16].toInt();
  tuning1 = data[17].toInt();
  tuning2 = data[18].toInt();
  tuning3 = data[19].toInt();
  tuning4 = data[20].toInt();
  tuning5 = data[21].toInt();
  tuning6 = data[22].toInt();
  tuning7 = data[23].toInt();
  tuning8 = data[24].toInt();
  tuning9 = data[25].toInt();
  tuning10 = data[26].toInt();
  tuning11 = data[27].toInt();
  tuning12 = data[28].toInt();
  tuning13 = data[29].toInt();
  tuning14 = data[30].toInt();
  tuning15 = data[31].toInt();
  tuning16 = data[32].toInt();
  volume1 = data[33].toInt();
  volume2 = data[34].toInt();
  volume3 = data[35].toInt();
  volume4 = data[36].toInt();
  volume5 = data[37].toInt();
  volume6 = data[38].toInt();
  volume7 = data[39].toInt();
  volume8 = data[40].toInt();
  volume9 = data[41].toInt();
  volume10 = data[42].toInt();
  volume11 = data[43].toInt();
  volume12 = data[44].toInt();
  volume13 = data[45].toInt();
  volume14 = data[46].toInt();
  volume15 = data[47].toInt();
  volume16 = data[48].toInt();
  filter1 = data[49].toInt();
  filter2 = data[50].toInt();
  filter3 = data[51].toInt();
  filter4 = data[52].toInt();
  filter5 = data[53].toInt();
  filter6 = data[54].toInt();
  filter7 = data[55].toInt();
  filter8 = data[56].toInt();
  filter9 = data[57].toInt();
  filter10 = data[58].toInt();
  filter11 = data[59].toInt();
  filter12 = data[60].toInt();
  filter13 = data[61].toInt();
  filter14 = data[62].toInt();
  filter15 = data[63].toInt();
  filter16 = data[64].toInt();
  volumeControl = data[65].toInt();
  //Switches

  for (drum_number = 1; drum_number < 17; drum_number++) {
    updateRecallDrum_Sample();
    updateRecallTuning();
    updateRecallFilterSW();
  }
  drum_number = old_drum_number;
  updateVariables();
  updateFilterSW(0);



  //Patchname
  updatePatchname();

  Serial.print("Set Patch: ");
  Serial.println(patchName);
}

String getCurrentPatchData() {
  return patchName + "," + String(sample1) + "," + String(sample2) + "," + String(sample3) + "," + String(sample4) + "," + String(sample5) + "," + String(sample6) + "," + String(sample7) + "," + String(sample8) + "," + String(sample9) + "," + String(sample10) + "," + String(sample11) + "," + String(sample12) + "," + String(sample13) + "," + String(sample14) + "," + String(sample15) + "," + String(sample16)
         + "," + String(tuning1) + "," + String(tuning2) + "," + String(tuning3) + "," + String(tuning4) + "," + String(tuning5) + "," + String(tuning6) + "," + String(tuning7) + "," + String(tuning8) + "," + String(tuning9) + "," + String(tuning10) + "," + String(tuning11) + "," + String(tuning12) + "," + String(tuning13) + "," + String(tuning14) + "," + String(tuning15) + "," + String(tuning16)
         + "," + String(volume1) + "," + String(volume2) + "," + String(volume3) + "," + String(volume4) + "," + String(volume5) + "," + String(volume6) + "," + String(volume7) + "," + String(volume8) + "," + String(volume9) + "," + String(volume10) + "," + String(volume11) + "," + String(volume12) + "," + String(volume13) + "," + String(volume14) + "," + String(volume15) + "," + String(volume16)
         + "," + String(filter1) + "," + String(filter2) + "," + String(filter3) + "," + String(filter4) + "," + String(filter5) + "," + String(filter6) + "," + String(filter7) + "," + String(filter8) + "," + String(filter9) + "," + String(filter10) + "," + String(filter11) + "," + String(filter12) + "," + String(filter13) + "," + String(filter14) + "," + String(filter15) + "," + String(filter16)
         + "," + String(volumeControl);
}

void checkMux() {

  mux1Read = analogRead(MUX1_S);
  if (mux1Read > (mux1ValuesPrev + QUANTISE_FACTOR) || mux1Read < (mux1ValuesPrev - QUANTISE_FACTOR)) {
    mux1ValuesPrev = mux1Read;
    myControlChange(midiChannel, CCtuning, mux1Read);
  }

  mux2Read = analogRead(MUX2_S);
  if (mux2Read > (mux2ValuesPrev + QUANTISE_FACTOR) || mux2Read < (mux2ValuesPrev - QUANTISE_FACTOR)) {
    mux2ValuesPrev = mux2Read;
    myControlChange(midiChannel, CCvolume, mux2Read);
  }

  mux3Read = analogRead(MUX3_S);
  if (mux3Read > (mux3ValuesPrev + QUANTISE_FACTOR) || mux3Read < (mux3ValuesPrev - QUANTISE_FACTOR)) {
    mux3ValuesPrev = mux3Read;
    myControlChange(midiChannel, CCvolumeControl, mux3Read);
  }
}

void midiCCOut(byte cc, byte value) {
  MIDI.sendControlChange(cc, value, drum_number);  //MIDI DIN is set to Out
}

void midiPRGout(byte value) {
  MIDI.sendProgramChange(value, drum_number);
}

void writeDemux() {

  //DEMUX 1
  switch (muxOutput) {
    case 0:
      analogWrite(A21, int(volume1 / 1.57));
      break;
    case 1:
      analogWrite(A21, int(volume2 / 1.57));
      break;
    case 2:
      analogWrite(A21, int(volume3 / 1.57));
      break;
    case 3:
      analogWrite(A21, int(volume4 / 1.57));
      break;
    case 4:
      analogWrite(A21, int(volume5 / 1.57));
      break;
    case 5:
      analogWrite(A21, int(volume6 / 1.57));
      break;
    case 6:
      analogWrite(A21, int(volume7 / 1.57));
      break;
    case 7:
      analogWrite(A21, int(volume8 / 1.57));
      break;
    case 8:
      analogWrite(A21, int(volume9 / 1.57));
      break;
    case 9:
      analogWrite(A21, int(volume10 / 1.57));
      break;
    case 10:
      analogWrite(A21, int(volume11 / 1.57));
      break;
    case 11:
      analogWrite(A21, int(volume12 / 1.57));
      break;
    case 12:
      analogWrite(A21, int(volume13 / 1.57));
      break;
    case 13:
      analogWrite(A21, int(volume14 / 1.57));
      break;
    case 14:
      analogWrite(A21, int(volume15 / 1.57));
      break;
    case 15:
      analogWrite(A21, int(volume16 / 1.57));
      break;
  }
  digitalWriteFast(DEMUX_EN_1, LOW);
  delayMicroseconds(DelayForSH3);
  digitalWriteFast(DEMUX_EN_1, HIGH);

  muxOutput++;
  if (muxOutput >= DEMUXCHANNELS)
    muxOutput = 0;

  digitalWriteFast(DEMUX_0, muxOutput & B0001);
  digitalWriteFast(DEMUX_1, muxOutput & B0010);
  digitalWriteFast(DEMUX_2, muxOutput & B0100);
  digitalWriteFast(DEMUX_3, muxOutput & B1000);
}


void showSettingsPage() {
  showSettingsPage(settings::current_setting(), settings::current_setting_value(), state);
}

void checkSwitches() {

  filterSwitch.update();
  if (filterSwitch.fallingEdge()) {
    filterSW = !filterSW;
    myControlChange(midiChannel, CCfilterSW, filterSW);
  }


  saveButton.update();
  if (saveButton.held()) {
    switch (state) {
      case PARAMETER:
      case PATCH:
        state = DELETE;
        break;
    }
  } else if (saveButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        if (patches.size() < PATCHES_LIMIT) {
          resetPatchesOrdering();  //Reset order of patches from first patch
          patches.push({ patches.size() + 1, INITPATCHNAME });
          state = SAVE;
        }
        break;
      case SAVE:
        //Save as new patch with INITIALPATCH name or overwrite existing keeping name - bypassing patch renaming
        patchName = patches.last().patchName;
        state = PATCH;
        savePatch(String(patches.last().patchNo).c_str(), getCurrentPatchData());
        showPatchPage(patches.last().patchNo, patches.last().patchName);
        patchNo = patches.last().patchNo;
        loadPatches();  //Get rid of pushed patch if it wasn't saved
        setPatchesOrdering(patchNo);
        renamedPatch = "";
        state = PARAMETER;
        break;
      case PATCHNAMING:
        if (renamedPatch.length() > 0) patchName = renamedPatch;  //Prevent empty strings
        state = PATCH;
        savePatch(String(patches.last().patchNo).c_str(), getCurrentPatchData());
        showPatchPage(patches.last().patchNo, patchName);
        patchNo = patches.last().patchNo;
        loadPatches();  //Get rid of pushed patch if it wasn't saved
        setPatchesOrdering(patchNo);
        renamedPatch = "";
        state = PARAMETER;
        break;
    }
  }

  settingsButton.update();
  if (settingsButton.held()) {
    //If recall held, set current patch to match current hardware state
    //Reinitialise all hardware values to force them to be re-read if different
    state = REINITIALISE;
    reinitialiseToPanel();
  } else if (settingsButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        state = SETTINGS;
        showSettingsPage();
        break;
      case SETTINGS:
        showSettingsPage();
      case SETTINGSVALUE:
        settings::save_current_value();
        state = SETTINGS;
        showSettingsPage();
        break;
    }
  }

  backButton.update();
  if (backButton.held()) {
    //If Back button held, Panic - all notes off
  } else if (backButton.numClicks() == 1) {
    switch (state) {
      case RECALL:
        setPatchesOrdering(patchNo);
        state = PARAMETER;
        break;
      case SAVE:
        renamedPatch = "";
        state = PARAMETER;
        loadPatches();  //Remove patch that was to be saved
        setPatchesOrdering(patchNo);
        break;
      case PATCHNAMING:
        charIndex = 0;
        renamedPatch = "";
        state = SAVE;
        break;
      case DELETE:
        setPatchesOrdering(patchNo);
        state = PARAMETER;
        break;
      case SETTINGS:
        state = PARAMETER;
        break;
      case SETTINGSVALUE:
        state = SETTINGS;
        showSettingsPage();
        break;
    }
  }

  //Encoder switch
  recallButton.update();
  if (recallButton.held()) {
    //If Recall button held, return to current patch setting
    //which clears any changes made
    state = PATCH;
    //Recall the current patch
    patchNo = patches.first().patchNo;
    recallPatch(patchNo);
    state = PARAMETER;
  } else if (recallButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        state = RECALL;  //show patch list
        break;
      case RECALL:
        state = PATCH;
        //Recall the current patch
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        break;
      case SAVE:
        showRenamingPage(patches.last().patchName);
        patchName = patches.last().patchName;
        state = PATCHNAMING;
        break;
      case PATCHNAMING:
        if (renamedPatch.length() < 12)  //actually 12 chars
        {
          renamedPatch.concat(String(currentCharacter));
          charIndex = 0;
          currentCharacter = CHARACTERS[charIndex];
          showRenamingPage(renamedPatch);
        }
        break;
      case DELETE:
        //Don't delete final patch
        if (patches.size() > 1) {
          state = DELETEMSG;
          patchNo = patches.first().patchNo;     //PatchNo to delete from SD card
          patches.shift();                       //Remove patch from circular buffer
          deletePatch(String(patchNo).c_str());  //Delete from SD card
          loadPatches();                         //Repopulate circular buffer to start from lowest Patch No
          renumberPatchesOnSD();
          loadPatches();                      //Repopulate circular buffer again after delete
          patchNo = patches.first().patchNo;  //Go back to 1
          recallPatch(patchNo);               //Load first patch
        }
        state = PARAMETER;
        break;
      case SETTINGS:
        state = SETTINGSVALUE;
        showSettingsPage();
        break;
      case SETTINGSVALUE:
        settings::save_current_value();
        state = SETTINGS;
        showSettingsPage();
        break;
    }
  }
}

void reinitialiseToPanel() {
  //This sets the current patch to be the same as the current hardware panel state - all the pots
  //The four button controls stay the same state
  //This reinialises the previous hardware values to force a re-read
  mux1ValuesPrev = RE_READ;
  mux2ValuesPrev = RE_READ;
  mux3ValuesPrev = RE_READ;

  patchName = INITPATCHNAME;
  showPatchPage("Initial", "Panel Settings");
}

void checkEncoder() {
  //Encoder works with relative inc and dec values
  //Detent encoder goes up in 4 steps, hence +/-3

  long encRead = encoder.read();
  if ((encCW && encRead > encPrevious + 3) || (!encCW && encRead < encPrevious - 3)) {
    switch (state) {
      case PARAMETER:
        state = PATCH;
        patches.push(patches.shift());
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        break;
      case RECALL:
        patches.push(patches.shift());
        break;
      case SAVE:
        patches.push(patches.shift());
        break;
      case PATCHNAMING:
        if (charIndex == TOTALCHARS) charIndex = 0;  //Wrap around
        currentCharacter = CHARACTERS[charIndex++];
        showRenamingPage(renamedPatch + currentCharacter);
        break;
      case DELETE:
        patches.push(patches.shift());
        break;
      case SETTINGS:
        settings::increment_setting();
        showSettingsPage();
        break;
      case SETTINGSVALUE:
        settings::increment_setting_value();
        showSettingsPage();
        break;
    }
    encPrevious = encRead;
  } else if ((encCW && encRead < encPrevious - 3) || (!encCW && encRead > encPrevious + 3)) {
    switch (state) {
      case PARAMETER:
        state = PATCH;
        patches.unshift(patches.pop());
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        break;
      case RECALL:
        patches.unshift(patches.pop());
        break;
      case SAVE:
        patches.unshift(patches.pop());
        break;
      case PATCHNAMING:
        if (charIndex == -1)
          charIndex = TOTALCHARS - 1;
        currentCharacter = CHARACTERS[charIndex--];
        showRenamingPage(renamedPatch + currentCharacter);
        break;
      case DELETE:
        patches.unshift(patches.pop());
        break;
      case SETTINGS:
        settings::decrement_setting();
        showSettingsPage();
        break;
      case SETTINGSVALUE:
        settings::decrement_setting_value();
        showSettingsPage();
        break;
    }
    encPrevious = encRead;
  }
}

void checkDrumEncoder() {

  long sample_encRead = sample_encoder.read();
  if ((sample_encCW && sample_encRead > sample_encPrevious + 3) || (!sample_encCW && sample_encRead < sample_encPrevious - 3)) {
    sample = sample + 1;
    if (sample > 48) {
      sample = 1;
    }
    sample_encPrevious = sample_encRead;
    myControlChange(midiChannel, CCsample, sample);

  } else if ((sample_encCW && sample_encRead < sample_encPrevious - 3) || (!sample_encCW && sample_encRead > sample_encPrevious + 3)) {
    sample = sample - 1;
    if (sample < 1) {
      sample = 48;
    }
    sample_encPrevious = sample_encRead;
    myControlChange(midiChannel, CCsample, sample);
  }

  long param_encRead = param_encoder.read();
  if ((param_encCW && param_encRead > param_encPrevious + 3) || (!param_encCW && param_encRead < param_encPrevious - 3)) {
    param_number = param_number + 1;
    if (param_number > 4) {
      param_number = 1;
    }
    switch (param_number) {
      case 1:
        updateDrum_Sample();
        break;

      case 2:
        updateTuning();
        break;

      case 3:
        updateVariables();
        updateFilterSW(1);
        break;

      case 4:
        updateVolume();
        break;
    }

    param_encPrevious = param_encRead;

  } else if ((param_encCW && param_encRead < param_encPrevious - 3) || (!param_encCW && param_encRead > param_encPrevious + 3)) {
    param_number = param_number - 1;
    if (param_number < 1) {
      param_number = 4;
    }
    switch (param_number) {
      case 1:
        updateDrum_Sample();
        break;

      case 2:
        updateTuning();
        break;

      case 3:
        updateVariables();
        updateFilterSW(1);
        break;

      case 4:
        updateVolume();
        break;
    }
    param_encPrevious = param_encRead;
  }

  long drum_encRead = drum_encoder.read();
  if ((drum_encCW && drum_encRead > drum_encPrevious + 3) || (!drum_encCW && drum_encRead < drum_encPrevious - 3)) {
    drum_number = drum_number + 1;
    if (drum_number > 16) {
      drum_number = 1;
    }
    drum_encPrevious = drum_encRead;
    myControlChange(midiChannel, CCdrum, drum_number);

  } else if ((drum_encCW && drum_encRead < drum_encPrevious - 3) || (!drum_encCW && drum_encRead > drum_encPrevious + 3)) {
    drum_number = drum_number - 1;
    if (drum_number < 1) {
      drum_number = 16;
    }
    drum_encPrevious = drum_encRead;
    myControlChange(midiChannel, CCdrum, drum_number);
  }
}

void loop() {
  checkSwitches();
  writeDemux();
  checkMux();
  checkEncoder();
  checkDrumEncoder();
  MIDI.read(midiChannel);
  usbMIDI.read(midiChannel);
}