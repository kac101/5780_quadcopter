#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#include <stdint.h>  // required for uint32_t

typedef uint32_t uint;
#define PWM_PIN 6  // GPIO 6

void setup_pwm(uint32_t pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, 100);  // period
    pwm_set_chan_level(slice, pwm_gpio_to_channel(pin), 0);
    pwm_set_enabled(slice, true);
}

void set_pwm_duty(uint pin, float duty_percent) {
    uint slice = pwm_gpio_to_slice_num(pin);
    uint level = duty_percent / 100.0f * 100;
    pwm_set_chan_level(slice, pwm_gpio_to_channel(pin), level);
}

int main() {
    stdio_init_all();
    setup_pwm(PWM_PIN);

    float brightness = 0;
    float step = 1.0;

    while (true) {
        set_pwm_duty(PWM_PIN, brightness);
        brightness += step;
        if (brightness >= 100 || brightness <= 0) step = -step;
        sleep_ms(10);
    }
}
