#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define SAMPLE_RATE 20000 //20kHz sample rate!
#define BLOCK_SIZE  2048          // ~100 ms of samples, 2048 samples

uint8_t adc_pins[] = {4}; //ADC on Pin 4
uint8_t adc_pins_count = 1; //Just 1 ADC

volatile bool adc_conversion_done = false;
adc_continuous_result_t *result = NULL;

uint32_t count = 0; //Sample count (current)
int64_t  sum = 0, sum_sq = 0; //Sample sum and sum of squared
int32_t  vmin = 4095, vmax = 0; //Max/min voltage in on ADC

#define LED_PIN    0 //Neopixel pin
#define LED_COUNT  1 //Neopixel count (1)

int ledTriggerCooldown = 10; //Seconds for red light trigger to turn off
long prevMillis = 100000; //Counter for red light trigger (Neopixel)

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void ARDUINO_ISR_ATTR adcComplete() { adc_conversion_done = true; } //Function call back when ADC done per sample

#define SR_CLK     1
#define LATCH_CLK  2
#define SR_DIN     5
#define SR_OE      3

void setup() {
  //Serial.begin(115200);
  pinMode(SR_CLK, OUTPUT);
  pinMode(LATCH_CLK, OUTPUT);
  pinMode(SR_OE, OUTPUT);
  pinMode(SR_DIN, OUTPUT);
  digitalWrite(SR_OE, LOW);
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

  if (count >= BLOCK_SIZE) { //Done grabbing samples
    double mean = (double)sum / count;
    double var  = (double)sum_sq / count - mean * mean;   //Get variance of samples, total power - dc power -> ac power (sound) (assumes 1 ohm as a reference value)
    double rms  = sqrt(var); //Root mean squared value finally, so its comparable to datasheet
    double dbfs = 20.0 * log10(rms / 2048.0); //Convert to decibles

    Serial.printf("n=%lu  dc=%.1f  min=%ld max=%ld  rms=%.2f  %.1f dBFS\n",
                  count, mean, vmin, vmax, rms, dbfs);

    if (dbfs > 0){
      //All red
      shiftOut(SR_DIN, SR_CLK, LSBFIRST, 0b11111111);
      strip.setPixelColor(0, 255, 0, 0);
      strip.show();
      prevMillis = millis();
    }
    else if (dbfs > -5){
      shiftOut(SR_DIN, SR_CLK, LSBFIRST, 0b11111110);
      strip.setPixelColor(0, 255, 0, 0);
      strip.show();
      prevMillis = millis();
    }
    else if (dbfs > -10){
      shiftOut(SR_DIN, SR_CLK, LSBFIRST, 0b11111100);
      strip.setPixelColor(0, 255, 0, 0);
      strip.show();
    }
    else if (dbfs > -20){
      shiftOut(SR_DIN, SR_CLK, LSBFIRST, 0b11111000);
      strip.setPixelColor(0, 255, 0, 0);
      strip.show();
    }
    else if (dbfs > -35){
      shiftOut(SR_DIN, SR_CLK, LSBFIRST, 0b11110000);

      if (millis() - prevMillis >= ledTriggerCooldown*1000){
        strip.setPixelColor(0, 255, 255, 0);
        strip.show();
      }
    }
    else if (dbfs > -40){
      shiftOut(SR_DIN, SR_CLK, LSBFIRST, 0b11100000);
      if (millis() - prevMillis >= ledTriggerCooldown*1000){
        strip.setPixelColor(0, 255, 255, 0);
        strip.show();
      }
    }
    else if (dbfs > -45){
      shiftOut(SR_DIN, SR_CLK, LSBFIRST, 0b11000000);
      
      if (millis() - prevMillis >= ledTriggerCooldown*1000){
        strip.setPixelColor(0, 0, 255, 0);
        strip.show();
      }
    }
    else if (dbfs > -50){
      shiftOut(SR_DIN, SR_CLK, LSBFIRST, 0b10000000);
      
      if (millis() - prevMillis >= ledTriggerCooldown*1000){
        strip.setPixelColor(0, 0, 255, 0);
        strip.show();
      }
    }
    else{
      shiftOut(SR_DIN, SR_CLK, LSBFIRST, 0b00000000);
      
      if (millis() - prevMillis >= ledTriggerCooldown*1000){
        strip.setPixelColor(0, 0, 255, 0);
        strip.show();
      }
    }

    
  digitalWrite(LATCH_CLK, HIGH);
  digitalWrite(LATCH_CLK, LOW);

    sum = 0; sum_sq = 0; count = 0; vmin = 4095; vmax = 0;
  }
}
