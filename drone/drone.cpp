#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define PWM_PIN 10      // PWM output pin (GPIO 10)
#define LED_PIN 25      // Onboard LED

void setup_pwm(uint32_t pin, uint32_t duty) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint32_t slice = pwm_gpio_to_slice_num(pin);
    uint32_t channel = pwm_gpio_to_channel(pin);
    pwm_set_wrap(slice, 100);
    pwm_set_chan_level(slice, channel, duty); // 100% = 100
    pwm_set_enabled(slice, true);
}

int main() {
    stdio_init_all();

    // Setup LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Setup PWM output
    setup_pwm(PWM_PIN, 100); // 100% duty cycle

    while (true) {
        gpio_put(LED_PIN, 1); // Turn LED ON
        sleep_ms(1000);       // Wait 1 second
        gpio_put(LED_PIN, 0); // Turn LED OFF
        sleep_ms(1000);       // Wait 1 second
    }

    return 0;
}
