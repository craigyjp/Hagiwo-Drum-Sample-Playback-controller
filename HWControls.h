// This optional setting causes Encoder to use more optimized code,
// It must be defined before Encoder.h is included.
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <Bounce.h>
#include "TButton.h"
#include <ADC.h>
#include <ADC_util.h>

ADC *adc = new ADC();

#define MUX1_S A18
#define MUX2_S A19
#define MUX3_S A20

#define DEMUX_0 40
#define DEMUX_1 41
#define DEMUX_2 43
#define DEMUX_3 42

#define DEMUX_EN_1 6

//DeMux 1 Connections
#define DEMUX1_noiseLevel 0                 // 0-2v
#define DEMUX1_osc2PulseLevel 1             // 0-2v
#define DEMUX1_osc2Level 2                  // 0-2v
#define DEMUX1_LfoDepth 3                   // 0-2v
#define DEMUX1_osc1PWM 4                    // 0-2v
#define DEMUX1_PitchBend 5                  // 0-2v
#define DEMUX1_osc1PulseLevel 6             // 0-2v
#define DEMUX1_osc1Level 7                  // 0-2v
#define DEMUX1_modwheel 8                   // 0-2v
#define DEMUX1_volumeControl 9              // 0-2v
#define DEMUX1_osc2PWM 10                   // 0-2v
#define DEMUX1_spare 11                     // spare VCA 0-2v
#define DEMUX1_AmpEGLevel 12                // Amplevel 0-5v
#define DEMUX1_MasterTune 13                // -15v to +15v  control 12   +/-13v
#define DEMUX1_osc2Detune 14                // -15v to +15v control 17 +/-13v
#define DEMUX1_pitchEGLevel 15              // PitchEGlevel 0-5v

#define RECALL_SW 17
#define SAVE_SW 24
#define SETTINGS_SW 12
#define BACK_SW 10

#define PREVIOUS_SW 30
#define NEXT_SW 31
#define FILTER_SW 32
#define FILTER_LED 9

#define ENCODER_PINA 4
#define ENCODER_PINB 5
#define ENCODER_DRUMA 6
#define ENCODER_DRUMB 7

#define DEMUXCHANNELS 16
#define QUANTISE_FACTOR 15

#define DEBOUNCE 30

static byte muxOutput = 0;
static int mux1ValuesPrev = 0;
static int mux2ValuesPrev = 0;
static int mux3ValuesPrev = 0;
static int mux1Read = 0;
static int mux2Read = 0;
static int mux3Read = 0;

static long encPrevious = 0;
static long drum_encPrevious = 0;

//These are pushbuttons and require debouncing
Bounce previousSwitch = Bounce(PREVIOUS_SW, DEBOUNCE);
Bounce nextSwitch = Bounce(NEXT_SW, DEBOUNCE);
Bounce filterSwitch = Bounce(FILTER_SW, DEBOUNCE);

TButton saveButton{SAVE_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION};
TButton settingsButton{SETTINGS_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION};
TButton backButton{BACK_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION};
TButton recallButton{RECALL_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION}; //On encoder
Encoder encoder(ENCODER_PINB, ENCODER_PINA);//This often needs the pins swapping depending on the encoder
Encoder drum_encoder(ENCODER_DRUMB, ENCODER_DRUMA);//This often needs the pins swapping depending on the encoder

void setupHardware()
{
  //Volume Pot is on ADC0
  adc->adc0->setAveraging(16); // set number of averages 0, 4, 8, 16 or 32.
  adc->adc0->setResolution(10); // set bits of resolution  8, 10, 12 or 16 bits.
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED); // change the conversion speed
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED); // change the sampling speed

  //MUXs on ADC1
  adc->adc1->setAveraging(16); // set number of averages 0, 4, 8, 16 or 32.
  adc->adc1->setResolution(10); // set bits of resolution  8, 10, 12 or 16 bits.
  adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED); // change the conversion speed
  adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED); // change the sampling speed


  //Mux address pins

  pinMode(DEMUX_0, OUTPUT);
  pinMode(DEMUX_1, OUTPUT);
  pinMode(DEMUX_2, OUTPUT);
  pinMode(DEMUX_3, OUTPUT);
  pinMode(FILTER_LED, OUTPUT); 

  digitalWrite(DEMUX_0, LOW);
  digitalWrite(DEMUX_1, LOW);
  digitalWrite(DEMUX_2, LOW);
  digitalWrite(DEMUX_3, LOW);
  digitalWrite(FILTER_LED, LOW);

  pinMode(DEMUX_EN_1, OUTPUT);

  digitalWrite(DEMUX_EN_1, HIGH);

  analogWriteResolution(10);
  analogReadResolution(10);


  //Switches

  pinMode(RECALL_SW, INPUT_PULLUP); //On encoder
  pinMode(SAVE_SW, INPUT_PULLUP);
  pinMode(SETTINGS_SW, INPUT_PULLUP);
  pinMode(BACK_SW, INPUT_PULLUP);

  pinMode(PREVIOUS_SW, INPUT_PULLUP);
  pinMode(NEXT_SW, INPUT_PULLUP);
  pinMode(FILTER_SW, INPUT_PULLUP);
  
}
