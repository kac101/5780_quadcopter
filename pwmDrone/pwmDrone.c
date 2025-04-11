#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// pico has 24 pwm channels -> datasheet pg. 4
// using pins 0 - 3 to control the mosfets
#define PWM_PIN0 0     // this corresponds to motor 1
#define PWM_PIN2 1     // this corresponds to motor 2
#define PWM_PIN3 2     // this corresponds to motor 3
#define PWM_PIN4 3     // this corresponds to motor 4
#define PWM_FREQ 20000 // 20kHz freq for motor control, I believe prof said this is a good one to use anything beyond this is not nesccary

int main()
{
    stdio_init_all();

    // initializing the Wi-Fi chip
    if (cyw43_arch_init())
    {
        printf("Wi-Fi init failed\n"); // print statement if initializing the Wi-Fi chip fails
        return -1;
    }

    // onboard LED is connected to the Wi-Fi/Bluetooth chip so we must access this chip to enable the onboard LED
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); // setting the output of the LED to off
                                                   // CYW43_WL_GPIO_LED_PIN from pic2_w.h file

    // set up for all four pwm, four pins will be used for pins 0-3
    // each pin controls a mosfet that and that allows the motor to spin based on our duty cycle

    while (true)
    {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); // turning LED on, start of going through drone cycle

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); // turning LED off
        sleep_ms(1000);
    }
}
