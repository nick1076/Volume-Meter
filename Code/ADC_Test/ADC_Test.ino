#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define SAMPLE_RATE 20000
#define BLOCK_SIZE  2048          // ~100 ms of samples

uint8_t adc_pins[] = {4};
uint8_t adc_pins_count = 1;

volatile bool adc_conversion_done = false;
adc_continuous_result_t *result = NULL;

uint32_t count = 0;
int64_t  sum = 0, sum_sq = 0;
int32_t  vmin = 4095, vmax = 0;

#define LED_PIN    0
#define LED_COUNT  1

long prevMillis = 100000;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void ARDUINO_ISR_ATTR adcComplete() { adc_conversion_done = true; }

void setup() {
  Serial.begin(115200);
  pinMode(1, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(5, OUTPUT);
  digitalWrite(3, LOW);
  strip.begin();
  strip.setPixelColor(0, 255, 0, 0);
  strip.setBrightness(5);
  strip.show();
  analogContinuousSetWidth(12);
  analogContinuousSetAtten(ADC_11db);
  analogContinuous(adc_pins, adc_pins_count, 1, SAMPLE_RATE, &adcComplete);
  analogContinuousStart();
}

void loop() {
  if (adc_conversion_done) {
    adc_conversion_done = false;
    if (analogContinuousRead(&result, 0)) {
      int32_t v = result[0].avg_read_raw;   // 1 conversion = the raw sample
      sum    += v;
      sum_sq += (int64_t)v * v;
      if (v < vmin) vmin = v;
      if (v > vmax) vmax = v;
      count++;
    }
  }

  if (count >= BLOCK_SIZE) {
    double mean = (double)sum / count;
    double var  = (double)sum_sq / count - mean * mean;   // AC power
    double rms  = sqrt(var);
    double dbfs = 20.0 * log10(rms / 2048.0);

    Serial.printf("n=%lu  dc=%.1f  min=%ld max=%ld  rms=%.2f  %.1f dBFS\n",
                  count, mean, vmin, vmax, rms, dbfs);

    if (dbfs > -10){
      //All red
      shiftOut(5, 1, LSBFIRST, 0b11111111);
      strip.setPixelColor(0, 255, 0, 0);
      strip.show();
      prevMillis = millis();
    }
    else if (dbfs > -15){
      shiftOut(5, 1, LSBFIRST, 0b11111110);
      strip.setPixelColor(0, 255, 0, 0);
      strip.show();
      prevMillis = millis();
    }
    else if (dbfs > -25){
      shiftOut(5, 1, LSBFIRST, 0b11111100);
      strip.setPixelColor(0, 255, 0, 0);
      strip.show();
    }
    else if (dbfs > -30){
      shiftOut(5, 1, LSBFIRST, 0b11111000);
      strip.setPixelColor(0, 255, 0, 0);
      strip.show();
    }
    else if (dbfs > -35){
      shiftOut(5, 1, LSBFIRST, 0b11110000);

      if (millis() - prevMillis >= 10000){
        strip.setPixelColor(0, 255, 255, 0);
        strip.show();
      }
    }
    else if (dbfs > -40){
      shiftOut(5, 1, LSBFIRST, 0b11100000);
      if (millis() - prevMillis >= 10000){
        strip.setPixelColor(0, 255, 255, 0);
        strip.show();
      }
    }
    else if (dbfs > -45){
      shiftOut(5, 1, LSBFIRST, 0b11000000);
      
      if (millis() - prevMillis >= 10000){
        strip.setPixelColor(0, 0, 255, 0);
        strip.show();
      }
    }
    else if (dbfs > -50){
      shiftOut(5, 1, LSBFIRST, 0b10000000);
      
      if (millis() - prevMillis >= 10000){
        strip.setPixelColor(0, 0, 255, 0);
        strip.show();
      }
    }
    else{
      shiftOut(5, 1, LSBFIRST, 0b00000000);
      
      if (millis() - prevMillis >= 10000){
        strip.setPixelColor(0, 0, 255, 0);
        strip.show();
      }
    }

    
  digitalWrite(2, HIGH);
  digitalWrite(2, LOW);

    sum = 0; sum_sq = 0; count = 0; vmin = 4095; vmax = 0;
  }
}
