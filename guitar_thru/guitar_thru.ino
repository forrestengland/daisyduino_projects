// guitar headphone amp / fx with knob
#include "DaisyDuino.h"

DaisyHardware hw;
daisysp::Autowah autowah;

constexpr size_t MAX_DELAY = 48000;
DelayLine<float, MAX_DELAY> delayLine;

float feedback = 0.09f;
float delayTime = 0.9;

size_t num_channels;

int sensorValue = 0;
float sensor1nv = 0.0;
float aSensor1nv = 0.0;
int sensor1dv = 0;
float aSensor1nvA = 0.0;

void MyCallback(float **in, float **out, size_t size) {

  float sig;

  aSensor1nvA = aSensor1nvA * 0.99 + aSensor1nv * 0.01;

	float delaySamples = delayTime * DAISY.AudioSampleRate() * aSensor1nvA;
	delayLine.SetDelay(delaySamples);

  for (size_t i = 0; i < size; i++) {

    sig = in[0][i] * 50.0 * aSensor1nv;

    // clipping
    //if (sig > 0.5f) sig = 0.5f;
    //else if (sig < -0.5f) sig = -0.5f;

    // autowah
    sig = autowah.Process(sig);

		// delay
		float dryInput = sig;
		float wetSignal = delayLine.Read();
		float feedbackSignal = dryInput + (wetSignal * feedback);
		delayLine.Write(feedbackSignal);
		sig = dryInput + wetSignal;

    out[0][i] = sig;
    out[1][i] = sig;    
  }
}

void setup() {

  float sample_rate;
  // Initialize for Daisy seed at 48kHz
  hw = DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  num_channels = hw.num_channels;
  sample_rate = DAISY.get_samplerate();

    // Initialize the onboard LED pin as an output
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(A1, INPUT);
    // Turn the LED on

  Serial.begin(115200);
  analogReadResolution(16);

  digitalWrite(LED_BUILTIN, HIGH);

  autowah.Init(sample_rate);
  autowah.SetWah(0.7);
  autowah.SetLevel(0.8);
  autowah.SetDryWet(100.0);

	delayLine.Init();

  DAISY.begin(MyCallback);
}

void loop() {

  aSensor1nv = aSensor1nv * 0.9 + sensor1nv * 0.1;

  sensorValue = analogRead(A1);
  float normalizedValue = sensorValue / 65535.0;
  if (sensor1nv != normalizedValue) {
    sensor1nv = normalizedValue;
    autowah.SetWah(sensor1nv);
  }
}
