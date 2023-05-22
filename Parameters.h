//Values below are just for initialising and will be changed when synth is initialised to current panel controls & EEPROM settings
byte midiChannel = MIDI_CHANNEL_OMNI;//(EEPROM)
String patchName = INITPATCHNAME;
boolean encCW = true;//This is to set the encoder to increment when turned CW - Settings Option
boolean drum_encCW = true;//This is to set the encoder to increment when turned CW - Settings Option
int drum_number = 1;

boolean drumsample = false;
boolean drumtuning = false;
boolean drumvolume = false;
boolean drumfilter = false;

int volumeControl = 0;
int volumeControlstr = 0;

int tuning = 0;
int volume = 0;

int filter = 0;

int sample = 0;
int sample1 = 0;
int sample2 = 0;
int sample3 = 0;
int sample4 = 0;
int sample5 = 0;
int sample6 = 0;
int sample7 = 0;
int sample8 = 0;
int sample9 = 0;
int sample10 = 0;
int sample11 = 0;
int sample12 = 0;
int sample13 = 0;
int sample14 = 0;
int sample15 = 0;
int sample16 = 0;

int volume1 = 0;
int volume2 = 0;
int volume3 = 0;
int volume4 = 0;
int volume5 = 0;
int volume6 = 0;
int volume7 = 0;
int volume8 = 0;
int volume9 = 0;
int volume10 = 0;
int volume11 = 0;
int volume12 = 0;
int volume13 = 0;
int volume14 = 0;
int volume15 = 0;
int volume16 = 0;

int tuning1 = 0;
int tuning2 = 0;
int tuning3 = 0;
int tuning4 = 0;
int tuning5 = 0;
int tuning6 = 0;
int tuning7 = 0;
int tuning8 = 0;
int tuning9 = 0;
int tuning10 = 0;
int tuning11 = 0;
int tuning12 = 0;
int tuning13 = 0;
int tuning14 = 0;
int tuning15 = 0;
int tuning16 = 0;

int filter1 = 0;
int filter2 = 0;
int filter3 = 0;
int filter4 = 0;
int filter5 = 0;
int filter6 = 0;
int filter7 = 0;
int filter8 = 0;
int filter9 = 0;
int filter10 = 0;
int filter11 = 0;
int filter12 = 0;
int filter13 = 0;
int filter14 = 0;
int filter15 = 0;
int filter16 = 0;

int previousSW = 0;
int nextSW = 0;
int filterSW = 0;

int returnvalue = 0;
