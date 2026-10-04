// guitar headphone amp / fx with knobs, led, switches

// daisy seed
#include "DaisyDuino.h"
DaisyHardware hw;

// autowah
daisysp::Autowah autowah;

// delay
constexpr size_t MAX_DELAY = 48000;
DelayLine<float, MAX_DELAY> delayLine;
float feedback = 0.09f;
float delayTime = 0.9;

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
int effectCount = 2;

// right button - next effect
Switch nextButton;

// left button - previous effect
Switch prevButton;

// external led
const int LED_PIN = 26;

// gain to boost input
const float INPUT_GAIN = 10.0;

// smoothing for pots
const float POT_SMOOTH = 0.001;

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

			}

			sig = (wetsig * mix) + (drysig * (1.0 - mix));

    }

		for (int c=0; c<num_channels; c++) {
			out[c][i] = sig;
			out[c][i] = sig;
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

	// initialize daisysp autowah
  autowah.Init(sample_rate);
  autowah.SetWah(0.7);
  autowah.SetLevel(0.8);
  autowah.SetDryWet(100.0);

	// initialize daisysp delay line
	delayLine.Init();

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
  }

	// check prev effect button
	prevButton.Debounce();
	// decrement effectNum when first pressed
	if (prevButton.RisingEdge()) {
		effectNum = effectNum - 1;
		if (effectNum < 0) effectNum = 0;
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
}
