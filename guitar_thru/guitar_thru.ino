// guitar headphone amp / fx with knobs, led, switches

// daisy
#include "DaisyDuino.h"
DaisyHardware hw;

// daisy effects - autowah
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

// pot 3 values
int sensor3Value = 0; // raw
float sensor3nv = 0.0; // normalized

// pot 4 values
int sensor4Value = 0; // raw
float sensor4nv = 0.0; // normalized

// wet/dry mix value
float mix = 0.5;

// middle button - bypass
Switch button;
int bypass = 0;

// external led
const int LED_PIN = 26;

void MyCallback(float **in, float **out, size_t size) {

  float sig;

	// smooth pot 1 value
  aSensor1nv = aSensor1nv * 0.99 + sensor1nv * 0.01;

	// change delay time based on smoothed pot 1 value
	float delaySamples = delayTime * DAISY.AudioSampleRate() * aSensor1nv;
	delayLine.SetDelay(delaySamples);

  for (size_t i = 0; i < size; i++) {

		// amplify input 0
		sig = in[0][i] * 25.0;

		// apply effect if not bypassed
    if (!bypass) {

			float drysig = sig;
			float wetsig = sig;

			// autowah
      wetsig = autowah.Process(wetsig);

		  // delay
		  float dryInput = wetsig;
		  float wetSignal = delayLine.Read();
		  float feedbackSignal = dryInput + (wetSignal * feedback);
		  delayLine.Write(feedbackSignal);
		  wetsig = dryInput + wetSignal;

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
    Serial.printf("bypass changed: %d\n", bypass);
    digitalWrite(LED_PIN, !bypass);
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
    autowah.SetWah(sensor2nv);
	}

	// read pot 3
	sensor3Value = analogRead(A3);
  normalizedValue = sensor3Value / 65535.0;
  if (sensor3nv != normalizedValue) {
    sensor3nv = normalizedValue;
		feedback = sensor3nv;
	}

	// read pot 4
	sensor4Value = analogRead(A4);
  normalizedValue = sensor4Value / 65535.0;
  if (sensor4nv != normalizedValue) {
    sensor4nv = normalizedValue;
		mix = sensor4nv;
	}
}
