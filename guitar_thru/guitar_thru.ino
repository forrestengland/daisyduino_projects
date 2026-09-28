// guitar headphone amp with volume knob

#include "DaisyDuino.h"
//#include <U8g2lib.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

DaisyHardware hw;

size_t num_channels;

//static WhiteNoise nse;
int sensorValue = 0;
float sensor1nv = 0.0;
float aSensor1nv = 0.0;
int sensor1dv = 0;

void MyCallback(float **in, float **out, size_t size) {
  float sig;
  for (size_t i = 0; i < size; i++) {
    //sig = nse.Process();

    for (size_t chn = 0; chn < num_channels; chn++) {
      //out[chn][i] = sig * 0.05;
      sig = in[0][i] * 50.0 * aSensor1nv;

      // clipping
      if (sig > 0.5f) sig = 0.5f;
      else if (sig < -0.5f) sig = -0.5f;

      out[chn][i] = sig;
    }
  }
}

void updateDisplay() {

   //display.setCursor(0, 30);
   //display.printf("gain: %f", sensor1nv);
   char s[255];
   int v = sensor1nv * 100.0;
   if (v != sensor1dv) {
    sensor1dv = v;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Amplifier");
    sprintf(s, "gain: %d", v);
    display.println(s);
    //display.display();
   }
   
  
}

void setup() {
  float sample_rate;
  // Initialize for Daisy pod at 48kHz
  hw = DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  num_channels = hw.num_channels;
  sample_rate = DAISY.get_samplerate();

  // nse.Init();

  DAISY.begin(MyCallback);

    // Initialize the onboard LED pin as an output
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(A1, INPUT);
    // Turn the LED on

  Serial.begin(115200);
  analogReadResolution(16);

   digitalWrite(LED_BUILTIN, HIGH);

   Wire.setSDA(14);
   Wire.setSCL(13);
   Wire.setClock(100000);
   Wire.begin();

   //u8g2.begin();
   display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
   display.setTextSize(2);
   display.setTextColor(WHITE);
   updateDisplay();
}

int counter = 0;
unsigned long lastDisplayUpdate = 0;

void loop() {

  aSensor1nv = aSensor1nv * 0.9 + sensor1nv * 0.1;

  //if (counter >= 10000) {

    //int sensorValue = 0;
    sensorValue = analogRead(A1);
    float normalizedValue = sensorValue / 65535.0;
    if (sensor1nv != normalizedValue) {
      sensor1nv = normalizedValue;
      //Serial.println(normalizedValue);
    }
   // counter = 0;
 // } else {
 //   counter++;
  //}
  //delay(100);

  //u8g2.clearBuffer();
  //u8g2.setFont(u8g2_font_ncenB08_tr);
  //u8g2.drawStr(0,0, "Amp");
  //u8g2.sendBuffer();
  if (millis() - lastDisplayUpdate >= 200) {
    lastDisplayUpdate = millis();
    updateDisplay();
  }
}
