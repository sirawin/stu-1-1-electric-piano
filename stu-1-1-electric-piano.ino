#include <Arduino.h>

const int SENSOR_PINS[3] = {13,12,14};
const int DAC_PIN = 25;

const float NOTE_FREQS[3] = {261.63, 329.63, 392.00};
const int HOVER_THRESHOLD = 2000; 

const float SAMPLE_RATE = 20000.0;
const float TWO_PI_F = 6.28318530718;

float phase[3] = {0.0, 0.0, 0.0};
float amplitude[3] = {0.0, 0.0, 0.0};
bool prev_state[3] = {false, false, false};

const float MAX_VOLUME = 0.15; 

void setup() {
  Serial.begin(115200);
  pinMode(DAC_PIN, OUTPUT);
  for (int i = 0; i < 3; i++) {
    pinMode(SENSOR_PINS[i], INPUT);
  }
}

void loop() {
  // 1. Read optical sensors periodically (every 30 ms)
  static unsigned long last_sensor_time = 0;
  if (millis() - last_sensor_time >= 30) {
    last_sensor_time = millis();
    for (int i = 0; i < 3; i++) {
      int val = analogRead(SENSOR_PINS[i]);
      bool hover_detected = (val < HOVER_THRESHOLD);

      if (hover_detected && !prev_state[i]) {
        amplitude[i] = 1.0; 
        phase[i] = 0.0;
      }
      prev_state[i] = hover_detected;
    }
  }

  // 2. High-speed Audio Synthesis (50 microseconds = 20 kHz)
  static unsigned long last_micros = 0;
  unsigned long now = micros();
  
  if (now - last_micros >= 50) {
    last_micros = now;

    float mixed_sample = 0.0;
    int active_notes = 0;

    for (int i = 0; i < 3; i++) {
      if (amplitude[i] > 0.001) {
        float sample = sin(phase[i]) 
                     + 0.50 * sin(2.0 * phase[i]) 
                     + 0.25 * sin(3.0 * phase[i]);
        
        mixed_sample += sample * amplitude[i];
        active_notes++;

        phase[i] += (TWO_PI_F * NOTE_FREQS[i]) / SAMPLE_RATE;
        if (phase[i] >= TWO_PI_F) phase[i] -= TWO_PI_F;

        amplitude[i] *= 0.999885; 
      } else {
        amplitude[i] = 0.0;
      }
    }

    if (active_notes > 0) {
      mixed_sample = mixed_sample / active_notes;
    }

    uint8_t dac_val = (uint8_t)(128 + (mixed_sample * 127.0 * MAX_VOLUME));
    dacWrite(DAC_PIN, dac_val);
  }
}