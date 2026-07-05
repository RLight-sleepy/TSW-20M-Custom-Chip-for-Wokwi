#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  pin_t pin_v;
  pin_t pin_g;
  pin_t pin_a;
  pin_t pin_d;
  uint32_t turbidity_attr;
  uint32_t threshold_attr;
} chip_state_t;

static void chip_timer_callback(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;

  // Deactivate outputs if V (5V) pin is unpowered
  if (pin_read(chip->pin_v) == LOW) {
    pin_dac_write(chip->pin_a, 0.0);
    pin_write(chip->pin_d, LOW);
    return;
  }

  float turbidity = attr_read_float(chip->turbidity_attr);
  float threshold = attr_read_float(chip->threshold_attr);

  // 0 NTU -> ~4.5V Output | 4550 NTU -> ~0.5V Output
  float voltage = 4.5 - (turbidity / 4550.0) * 4.0;
  
  if (voltage < 0.5) voltage = 0.5;
  if (voltage > 4.5) voltage = 4.5;

  pin_dac_write(chip->pin_a, voltage);

  if (turbidity >= threshold) {
    pin_write(chip->pin_d, HIGH);
  } else {
    pin_write(chip->pin_d, LOW);
  }
}

void chip_init() {
  chip_state_t *chip = malloc(sizeof(chip_state_t));

  chip->pin_v = pin_init("V", INPUT);
  chip->pin_g = pin_init("G", INPUT);
  chip->pin_a = pin_init("A", ANALOG);
  chip->pin_d = pin_init("D", OUTPUT);

  chip->turbidity_attr = attr_init_float("turbidity", 0.0);
  chip->threshold_attr = attr_init_float("threshold", 2000.0);

  const timer_config_t config = {
    .callback = chip_timer_callback,
    .user_data = chip,
  };
  timer_t timer = timer_init(&config);
  timer_start(timer, 20000, true); // Runs every 20ms
}
