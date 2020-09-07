/* ESP8266/32 Audio Spectrum Analyser on an SSD1306/SH1106 Display
 * The MIT License (MIT) Copyright (c) 2017 by David Bird. 
 * The formulation and display of an AUdio Spectrum using an ESp8266 or ESP32 and SSD1306 or SH1106 OLED Display using a Fast Fourier Transform
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files 
 * (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, 
 * publish, distribute, but not to use it commercially for profit making or to sub-license and/or to sell copies of the Software or to 
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:  
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE 
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 * See more at http://dsbird.org.uk 
*/

#include <Wire.h>
#include "arduinoFFT.h" // Standard Arduino FFT library 
// https://github.com/kosme/arduinoFFT, in IDE, Sketch, Include Library, Manage Library, then search for FFT
arduinoFFT FFT = arduinoFFT();
/////////////////////////////////////////////////////////////////////////
// Comment out the display your nNOT using e.g. if you have a 1.3" display comment out the SSD1306 library and object
//#include "SH1106.h"     // https://github.com/squix78/esp8266-oled-ssd1306
//SH1106 display(0x3c, 21,22); // 1.3" OLED display object definition (address, SDA, SCL) Connect OLED SDA , SCL pins to ESP SDA, SCL pins

#include "SSD1306.h"  // https://github.com/squix78/esp8266-oled-ssd1306
SSD1306 display(0x3c, 5,4);  // 0.96" OLED display object definition (address, SDA, SCL) Connect OLED SDA , SCL pins to ESP SDA, SCL pins
/////////////////////////////////////////////////////////////////////////
#define SAMPLES 512              // Must be a power of 2
#define SAMPLING_FREQUENCY 40000 // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define amplitude 100            // Depending on your audio source level, you may need to increase this value
unsigned int sampling_period_us;
unsigned long microseconds;
byte peak[] = {0,0,0,0,0,0,0};
double vReal[SAMPLES];
double vImag[SAMPLES];
unsigned long newTime, oldTime;
/////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  Wire.begin(5,4); // SDA, SCL
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.flipScreenVertically(); // Adjust to suit or remove
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
}

void loop() {
  display.clear();
  display.drawString(0,53,"0.2 0.4 1K 2K  4K  8K  12K");
  for (int i = 0; i < SAMPLES; i++) {
    newTime = micros()-oldTime;
    oldTime = newTime;
    vReal[i] = analogRead(A0); // A conversion takes about 1uS on an ESP32
    vImag[i] = 0;
    while (micros() < (newTime + sampling_period_us)) { /* do nothing to wait */ }
  }
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
  for (int i = 2; i < (SAMPLES/2); i++){ // Don't use sample 0 and only first SAMPLES/2 are usable. Each array eleement represents a frequency and its value the amplitude.
    if (vReal[i] > 2000) { // Add a crude noise filter, 10 x amplitude or more
      if (i<=2 )             displayBand(0,(int)vReal[i]/amplitude); // 125Hz
      if (i >3   && i<=5 )   displayBand(1,(int)vReal[i]/amplitude); // 250Hz
      if (i >5   && i<=7 )   displayBand(2,(int)vReal[i]/amplitude); // 500Hz
      if (i >7   && i<=15 )  displayBand(3,(int)vReal[i]/amplitude); // 1000Hz
      if (i >15  && i<=30 )  displayBand(4,(int)vReal[i]/amplitude); // 2000Hz
      if (i >30  && i<=53 )  displayBand(5,(int)vReal[i]/amplitude); // 4000Hz
      if (i >53  && i<=200 ) displayBand(6,(int)vReal[i]/amplitude); // 8000Hz
      if (i >200           ) displayBand(7,(int)vReal[i]/amplitude); // 16000Hz
      //Serial.println(i);
    }
    for (byte band = 0; band <= 6; band++) display.drawHorizontalLine(18*band,50-peak[band],14);
  }
  if (millis()%4 == 0) {for (byte band = 0; band <= 6; band++) {if (peak[band] > 0) peak[band] -= 1;}} // Decay the peak
  display.display();
}

void displayBand(int band, int dsize){
  int dmax = 50;
  if (dsize > dmax) dsize = dmax;
  if (band == 7) display.drawHorizontalLine(18*6,0, 14);
  for (int s = 0; s <= dsize; s=s+2){display.drawHorizontalLine(18*band,50-s, 14);}
  if (dsize > peak[band]) {peak[band] = dsize;}
}
