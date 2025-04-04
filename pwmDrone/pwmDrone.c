#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

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
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); // setting the output of the LED to 1 which turns ON the LED
                                                   // CYW43_WL_GPIO_LED_PIN from pic2_w.h file

    while (true)
    {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
