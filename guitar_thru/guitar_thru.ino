// guitar headphone amp / fx with knobs, led, switches

// cycfi q for guitar synth
#include "q/support/literals.hpp"
#include "q/pitch/pitch_detector.hpp"
#include "q/fx/signal_conditioner.hpp"

using namespace cycfi::q::literals;

// daisy seed
#include "DaisyDuino.h"
DaisyHardware hw;

// guitar synth
static Oscillator osc;
static Oscillator lfo;
static cycfi::q::signal_conditioner* preprocessor = nullptr;
static cycfi::q::pitch_detector* pd = nullptr;
static cycfi::q::peak_envelope_follower* env_follower = nullptr; // ADD THIS LINE
float target_frequency = 440.0;
float current_frequency = 440.0;
const float pitch_smoothing = 0.15;
// env follower
static float synth_envelope = 0.0f;
const float env_attack = 0.1f;   // Lower = faster attack response
const float env_release = 0.7f; // Higher = longer note decay tail
// ladder filter
daisysp::MoogLadder ladderFilter;

// autowah
daisysp::Autowah autowah;

// delay
constexpr size_t MAX_DELAY = 48000;
DelayLine<float, MAX_DELAY> delayLine;
float feedback = 0.09f;
float delayTime = 0.9;

// pitch shift
daisysp::PitchShifter ps;

// chorus
daisysp::Chorus ch;

// input channel count
size_t num_channels;

// pot 1 values
int sensorValue = 0; // raw
float sensor1nv = 0.0; // normalized
float aSensor1nv = 0.0; // smoothed

// pot 2 values
int sensor2Value = 0; // raw
float sensor2nv = 0.0; // normalized
float aSensor2nv = 0.0; // smoothed

// pot 3 values
int sensor3Value = 0; // raw
float sensor3nv = 0.0; // normalized
float aSensor3nv = 0.0; // smoothed

// pot 4 values
int sensor4Value = 0; // raw
float sensor4nv = 0.0; // normalized
float aSensor4nv = 0.0; // normalized

// wet/dry mix value
float mix = 0.5;

// middle button - bypass
Switch button;
int bypass = 0;

// current effect
int effectNum = 0;
int effectCount = 64;

// right button - next effect
Switch nextButton;

// left button - previous effect
Switch prevButton;

// led array button - switch mode
Switch ledButton;
int ledMode = 0;
int sampleValue = 0;
float smoothedSampleValue = 0.0;

// external led
const int LED_PIN = 26;

int led_arr_pins[] = {2,3,4,5,6,7};
int led_arr_pins_count = 6;

// gain to boost input
const float INPUT_GAIN = 10.0;

// smoothing for pots
const float POT_SMOOTH = 0.001;

const float PITCHSHIFT_MAX = 24.0;
const float CHORUS_LFORATEMAX = 25.0;
const float CHORUS_LFODEPTHMAX = 0.9;

// audio process callback
void MyCallback(float **in, float **out, size_t size) {

  float sig;

  for (size_t i = 0; i < size; i++) {

			// smooth pot values
		aSensor1nv = aSensor1nv * (1.0 - POT_SMOOTH) + sensor1nv * POT_SMOOTH;
		aSensor2nv = aSensor2nv * (1.0 - POT_SMOOTH) + sensor2nv * POT_SMOOTH;
		aSensor3nv = aSensor3nv * (1.0 - POT_SMOOTH) + sensor3nv * POT_SMOOTH;
		aSensor4nv = aSensor4nv * (1.0 - POT_SMOOTH) + sensor4nv * POT_SMOOTH;

		// apply wet/dry mix based on smoothed pot 1 value
		mix = aSensor1nv;

		// amplify input 0
		sig = in[0][i] * INPUT_GAIN;

		// apply effect if not bypassed
    if (!bypass) {

			float drysig = sig;
			float wetsig = sig;

			if (effectNum == 0) { // delay

				// change delay time based on smoothed pot 2 value
				float delaySamples = delayTime * DAISY.AudioSampleRate() * aSensor2nv;
				delayLine.SetDelay(delaySamples);

				// change feedback based on smoothed pot 3 value
				feedback = aSensor3nv;

				// apply delay
				float dryInput = wetsig;
				float wetSignal = delayLine.Read();
				float feedbackSignal = dryInput + (wetSignal * feedback);
				delayLine.Write(feedbackSignal);
				wetsig = dryInput + wetSignal;

			} else if (effectNum == 1) { // autowah

				// change wah amount based on smoothed pot 2 value
				autowah.SetWah(sensor2nv);

				wetsig = autowah.Process(wetsig);

			} else if (effectNum == 2) { // pitch shift

				ps.SetTransposition(PITCHSHIFT_MAX * aSensor2nv);
				wetsig = ps.Process(wetsig);
				
			} else if (effectNum == 3) { // chorus

				ch.SetLfoFreq(aSensor2nv * CHORUS_LFORATEMAX);
				ch.SetLfoDepth(aSensor3nv * CHORUS_LFODEPTHMAX);				
				wetsig = ch.Process(wetsig) * aSensor4nv * 4.0;
				
			} else if (effectNum == 4) { // guitar synth

				if (preprocessor != nullptr && pd != nullptr) {
					// 1. Clean the signal using the Q conditioner
					float clean_signal = (*preprocessor)(wetsig);
					
					// 2. Custom Envelope Follower Calculation
					float raw_mag = fabsf(clean_signal);
					if (raw_mag > synth_envelope) {
						// Follow the rising edge quickly (Attack)
						synth_envelope = synth_envelope * (1.0f - env_attack) + raw_mag * env_attack;
					} else {
						// Smoothly decay down over time (Release)
						synth_envelope = synth_envelope * env_release;
					}

					// 3. Track and update the pitch via Q
					if ((*pd)(clean_signal)) {
						target_frequency = cycfi::q::as_float(cycfi::q::frequency(pd->get_frequency()));
					}

					// Prevent pitch jitter
					current_frequency += (target_frequency - current_frequency) * pitch_smoothing;
					osc.SetFreq(current_frequency);
					lfo.SetFreq(100 * aSensor4nv);

					// 4. Apply the custom envelope to the oscillator output
					// Multiply by a gain modifier (e.g. 2.0f) if your synth needs a volume boost
					wetsig = osc.Process() * synth_envelope * 2.0f;

					// apply ladder filter
					ladderFilter.SetFreq(5000 * aSensor2nv * synth_envelope * (lfo.Process() / 2.0 + 0.5));
					ladderFilter.SetRes(0.5 * aSensor3nv);
					wetsig = ladderFilter.Process(wetsig);
					
				} else {
					wetsig = 0.0f;
				}
			}
				
			sig = (wetsig * mix) + (drysig * (1.0 - mix));
    }

		for (int c=0; c<num_channels; c++) {
			out[c][i] = sig;
			out[c][i] = sig;
		}

		float sample = sig;
		float mag = fmaxf(0.0f, fabsf(sample));
		smoothedSampleValue = (smoothedSampleValue * 0.99 + mag * 0.01);
		sampleValue = (int)(smoothedSampleValue * 6.0 * 1.5);
		if (sampleValue > 6) sampleValue = 6;
  }
}

void update_led_array() {

	if (ledMode == 0) {
		// show effectNum in binary on the led array
		for (int i=0; i<led_arr_pins_count; i++) {
			digitalWrite(led_arr_pins[led_arr_pins_count - i - 1], (effectNum & (0x01 << i)) >> i);
		}
	} else {
		// show vu meter
		int value = sampleValue;
		for (int i=0; i<led_arr_pins_count; i++) {
			digitalWrite(led_arr_pins[led_arr_pins_count - i - 1], value >= i);
		}
	}
}

void setup() {

  float sample_rate;
	
  // initialize daisy seed at 48kHz sample rate
  hw = DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  num_channels = hw.num_channels;
  sample_rate = DAISY.get_samplerate();

	// initialize the onboard LED pin as an output
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);	

	// initialize external bypass led
  digitalWrite(LED_PIN, HIGH);
  pinMode(LED_PIN, OUTPUT);

	// initialize led array
	for (int i=0; i<led_arr_pins_count; i++) {
		pinMode(led_arr_pins[i], OUTPUT);
	}

	// initialize pot analog inputs
  pinMode(A1, INPUT);
	pinMode(A2, INPUT);
	pinMode(A3, INPUT);
	pinMode(A4, INPUT);	
  analogReadResolution(16);
	
	// initialize bypass button on d27 in input_pullup mode
  button.Init(1000.0, true, 27, 2);

	// initialize next effect button on d28
	nextButton.Init(1000.0, true, 28, 2);

	// initialize prev effect button on d1
	prevButton.Init(1000.0, true, 1, 2);

	// initialize led button on d25
	ledButton.Init(1000.0, true, 25, 2);

	// initialize daisysp autowah
  autowah.Init(sample_rate);
  autowah.SetWah(0.7);
  autowah.SetLevel(0.8);
  autowah.SetDryWet(100.0);

	// initialize daisysp delay line
	delayLine.Init();

	// init pitch shift
	ps.Init(sample_rate);
	ps.SetDelSize(16384);

	// init chorus
	ch.Init(sample_rate);
	//	ch.SetLfoFreq(1.0f);
	//	ch.SetLfoDepth(0.5f);
	ch.SetDelayMs(15.0f);
	ch.SetFeedback(0.0f);

		// Initialize Cycfi Q objects
	auto lowest_freq = 50_Hz;
	auto highest_freq = 1200_Hz;

	cycfi::q::signal_conditioner::config preprocessor_config;
	preprocessor = new cycfi::q::signal_conditioner{ preprocessor_config, lowest_freq, highest_freq, sample_rate }; 
	pd = new cycfi::q::pitch_detector{ lowest_freq, highest_freq, sample_rate, -40_dB }; 

	// Initialize the daisy oscillator for the synth effect
	osc.Init(sample_rate);
	osc.SetWaveform(Oscillator::WAVE_SAW);

	lfo.Init(sample_rate);
	lfo.SetWaveform(Oscillator::WAVE_SIN);

	// init ladder filter
	ladderFilter.Init(sample_rate);

	// start audio with callback function
  DAISY.begin(MyCallback);
}

void loop() {

	// check bypass button
  button.Debounce();

	// toggle bypass when first pressed
  if (button.RisingEdge()) {
    bypass = !bypass;
    digitalWrite(LED_PIN, !bypass);
  }

	// check next effect button
  nextButton.Debounce();
	// increment effectNum when first pressed
  if (nextButton.RisingEdge()) {
		effectNum = effectNum + 1;		
		if (effectNum > effectCount - 1) {
			effectNum = effectCount - 1;
		}
		update_led_array();
  }

	// check prev effect button
	prevButton.Debounce();
	// decrement effectNum when first pressed
	if (prevButton.RisingEdge()) {
		effectNum = effectNum - 1;
		if (effectNum < 0) effectNum = 0;
		update_led_array();		
	}

	// check led button
	ledButton.Debounce();
	if (ledButton.RisingEdge()) {
		ledMode = !ledMode;
		update_led_array();		
	}

	// read pot 1
  sensorValue = analogRead(A1);
  float normalizedValue = sensorValue / 65535.0;
  if (sensor1nv != normalizedValue) {
    sensor1nv = normalizedValue;
  }

	// read pot 2
	sensor2Value = analogRead(A2);
  normalizedValue = sensor2Value / 65535.0;
  if (sensor2nv != normalizedValue) {
    sensor2nv = normalizedValue;
	}

	// read pot 3
	sensor3Value = analogRead(A3);
  normalizedValue = sensor3Value / 65535.0;
  if (sensor3nv != normalizedValue) {
    sensor3nv = normalizedValue;
	}

	// read pot 4
	sensor4Value = analogRead(A4);
  normalizedValue = sensor4Value / 65535.0;
  if (sensor4nv != normalizedValue) {
    sensor4nv = normalizedValue;
	}

	if (ledMode) update_led_array();
}
