/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

#define PIN_POT 28
#define ADC_INPUT_POT 2
#define ADC_SAMPLE_MS 50
#define PERIOD_ZONE1_MS 300
#define PERIOD_ZONE2_MS 500

const int PIN_LED_B = 4;

const float conversion_factor = 3.3f / (1 << 12);

volatile int g_adc_flag = 0;
volatile int g_led_flag = 0;

bool adc_timer_callback(repeating_timer_t *rt) {
    g_adc_flag = 1;
    return true;
}

bool led_timer_callback(repeating_timer_t *rt) {
    g_led_flag = 1;
    return true;
}

/**
 * 0..1.0V: Desligado
 * 1..2.0V: 300 ms
 * 2..3.3V: 500 ms
 */
int main() {
    stdio_init_all();

    adc_init();
    adc_gpio_init(PIN_POT);
    adc_select_input(ADC_INPUT_POT);

    gpio_init(PIN_LED_B);
    gpio_set_dir(PIN_LED_B, GPIO_OUT);
    gpio_put(PIN_LED_B, 0);

    repeating_timer_t adc_timer;
    repeating_timer_t led_timer;
    add_repeating_timer_ms(ADC_SAMPLE_MS, adc_timer_callback, NULL, &adc_timer);

    int zone = 0;
    int led_timer_on = 0;
    int led_state = 0;

    while (1) {
        if (g_adc_flag) {
            g_adc_flag = 0;

            uint16_t raw = adc_read();
            float voltage = raw * conversion_factor;

            int new_zone;
            if (voltage < 1.0f) {
                new_zone = 0;
            } else if (voltage < 2.0f) {
                new_zone = 1;
            } else {
                new_zone = 2;
            }

            if (new_zone != zone) {
                zone = new_zone;

                if (led_timer_on) {
                    cancel_repeating_timer(&led_timer);
                    led_timer_on = 0;
                }
                g_led_flag = 0;
                led_state = 0;
                gpio_put(PIN_LED_B, 0);

                if (zone == 1) {
                    add_repeating_timer_ms(PERIOD_ZONE1_MS, led_timer_callback, NULL, &led_timer);
                    led_timer_on = 1;
                } else if (zone == 2) {
                    add_repeating_timer_ms(PERIOD_ZONE2_MS, led_timer_callback, NULL, &led_timer);
                    led_timer_on = 1;
                }
            }
        }

        if (g_led_flag) {
            g_led_flag = 0;
            if (led_timer_on) {
                led_state = !led_state;
                gpio_put(PIN_LED_B, led_state);
            }
        }
    }
}