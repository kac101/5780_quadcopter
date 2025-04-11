#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/pwm.h"

// pico has 24 pwm channels -> datasheet pg. 4
// using pins 0 - 3 to control the mosfets
#define PWM_PIN0 0     // this corresponds to motor 1
#define PWM_PIN1 1     // this corresponds to motor 2
#define PWM_PIN2 2     // this corresponds to motor 3
#define PWM_PIN3 3     // this corresponds to motor 4
#define PWM_FREQ 20000 // 20kHz freq for motor control, I believe prof said this is a good one to use anything beyond this is not nesccary

int main()
{
    // initing all standar i/o
    stdio_init_all();

    // initializing the Wi-Fi chip
    // onboard LED is connected to the Wi-Fi/Bluetooth chip so we must access this chip to enable the onboard LED

    if (cyw43_arch_init())
    {
        // default code from PICO
        printf("Wi-Fi init failed\n"); // print statement if initializing the Wi-Fi chip fails
        return -1;
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); // setting the output of the LED to off
                                                   // CYW43_WL_GPIO_LED_PIN from pic2_w.h file

    // set up for all four pwm, four pins will be used for pins 0-3
    // each pin controls a mosfet that allows the motor to spin based on our duty cycle
    // wrap is the number we want to count up to, default sys clck is 150Mhz, 150Mhz info -
    // info is from 8.1.2.1 in the Raspberry Pi Pico RP2350 Datasheet, link above
    uint32_t wrap = ((150000000 / PWM_FREQ) - 1); // clock divider to pwm_freq

    // each slice has a pair of pins that will have the same freq
    // grabbing slice numbers for each pin
    int slice_pin0 = pwm_gpio_to_slice_num(PWM_PIN0);
    int slice_pin1 = pwm_gpio_to_slice_num(PWM_PIN1);
    int slice_pin2 = pwm_gpio_to_slice_num(PWM_PIN2);
    int slice_pin3 = pwm_gpio_to_slice_num(PWM_PIN3);

    // setting pins to pwm function
    gpio_set_function(PWM_PIN0, GPIO_FUNC_PWM); // setting function of gpio 0 to pwm
    gpio_set_function(PWM_PIN1, GPIO_FUNC_PWM); // setting function of gpio 1 to pwm
    gpio_set_function(PWM_PIN2, GPIO_FUNC_PWM); // setting function of gpio 2 to pwm
    gpio_set_function(PWM_PIN3, GPIO_FUNC_PWM); // setting function of gpio 3 to pwm

    // setting up clk dividers for pin, dont need to divide the clock up really
    pwm_set_clkdiv(slice_pin0, 1.0f);
    pwm_set_clkdiv(slice_pin1, 1.0f);
    pwm_set_clkdiv(slice_pin2, 1.0f);
    pwm_set_clkdiv(slice_pin3, 1.0f);

    // wrap -> highest number the counter will go up to before returning back down to 0
    // info from hardware api - link abov
    pwm_set_wrap(slice_pin0, wrap);
    pwm_set_wrap(slice_pin1, wrap);
    pwm_set_wrap(slice_pin2, wrap);
    pwm_set_wrap(slice_pin2, wrap);

    // enabling all the pins
    pwm_set_enabled(slice_pin0, true);
    pwm_set_enabled(slice_pin1, true);
    pwm_set_enabled(slice_pin2, true);
    pwm_set_enabled(slice_pin3, true);

    while (true)
    {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); // turning LED on, start of going through drone cycle

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); // turning LED off
        sleep_ms(1000);
    }
}
