//based on lab3
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define PWM_PIN_1 6  // GPIO6 (Slice 3, Channel A)
#define PWM_PIN_2 7  // GPIO7 (Slice 3, Channel B)

void setup_pwm(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, 100); // Set period to 100 counts (like ARR = 99)
    pwm_set_chan_level(slice, pwm_gpio_to_channel(pin), 0); // Start at 0% duty
    pwm_set_enabled(slice, true);
}

void set_pwm_duty_percent(uint pin, float percent) {
    uint slice = pwm_gpio_to_slice_num(pin);
    uint level = (uint)(percent / 100.0f * 100); // Convert percent to 0–100
    pwm_set_chan_level(slice, pwm_gpio_to_channel(pin), level);
}

int main() {
    stdio_init_all();

    setup_pwm(PWM_PIN_1);
    setup_pwm(PWM_PIN_2);

    float brightness = 0;
    float step = 1.0;

    while (1) {
        set_pwm_duty_percent(PWM_PIN_1, brightness);
        set_pwm_duty_percent(PWM_PIN_2, brightness);

        brightness += step;
        if (brightness >= 100.0f || brightness <= 0.0f) {
            step = -step;
        }

        sleep_ms(10); // Faster update for smooth waveform
    }
}
