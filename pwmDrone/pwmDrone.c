#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"

/*
    refreneces (used C SDK and hardware api for pwm functions and datasheets):
        1. Raspberry Pi Pico 2 W  Datasheet: https://datasheets.raspberrypi.com/picow/pico-2-w-datasheet.pdf
        2. RP2350 - https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf
        3. Hardware API - https://www.raspberrypi.com/documentation/pico-sdk/hardware.html
        4. MPU-6000 and MPU-6050 Product Specification - https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf
        5. MPU-6000 and MPU-6050 Register Map and Descriptions - https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
*/

#define I2C i2c0 // i2c instance
#define I2C_FREQ (400*1000)
#define I2C_SDA 4 // i2c sda pin
#define I2C_SCL 5 // i2c scl pin

#define MPU6050_INT 6 // mpu6050 interrupt pin
#define MPU6050_I2C I2C // mpu6050 i2c instance
#define MPU6050 0x68 // mpu6050 i2c address
// mpu6050 registers, see the datasheet for a complete list
#define MPU6050_REG_CONFIG 0x1a
#define MPU6050_REG_GYRO_CONFIG 0x1b
#define MPU6050_REG_INT_PIN_CFG 0x37
#define MPU6050_REG_INT_ENABLE 0x38
#define MPU6050_REG_GYRO_XOUT_H 0x43
#define MPU6050_REG_PWR_MGMT_1 0x6b
#define MPU6050_REG_PWR_MGMT_2 0x6c

static void mpu6050_write(uint8_t reg, uint8_t data)
{
    uint8_t buffer[] = { reg, data };
    i2c_write_blocking(MPU6050_I2C, MPU6050, buffer, 2, false);
}

static void mpu6050_read(uint8_t reg, uint8_t *data, size_t len)
{
    i2c_write_blocking(MPU6050_I2C, MPU6050, &reg, 1, true);
    i2c_read_blocking(MPU6050_I2C, MPU6050, data, len, false);
}

static void mpu6050_get_measurement(int16_t *gyro)
{
    uint8_t gyro_raw[6];
    mpu6050_read(MPU6050_REG_GYRO_XOUT_H, gyro_raw, 6);
    gyro[0] = (int16_t)gyro_raw[0] << 8 | gyro_raw[1];
    gyro[1] = (int16_t)gyro_raw[2] << 8 | gyro_raw[3];
    gyro[2] = (int16_t)gyro_raw[4] << 8 | gyro_raw[5];
}

static void mpu6050_startup(void)
{
    // reset procedure (see datasheet)
    mpu6050_write(MPU6050_REG_PWR_MGMT_1, 0x80); // device reset
    sleep_ms(100);
    mpu6050_write(MPU6050_REG_PWR_MGMT_1, 0x00); // disable sleep mode
    sleep_ms(100);

    // use gyroscope X axis as reference clock (recommended by datasheet)
    mpu6050_write(MPU6050_REG_PWR_MGMT_1, 0x01);
    // put accelerometer into standby mode
    mpu6050_write(MPU6050_REG_PWR_MGMT_2, 0x38);
    // set gyroscope 1kHz sample rate
    mpu6050_write(MPU6050_REG_CONFIG, 0x01);
    // set gryoscope full scale range +-250deg/s
    mpu6050_write(MPU6050_REG_GYRO_CONFIG, 0x00);
}

static void mpu6050_enable_interrupts(void)
{
    // set interrupt pin active high, push-pull
    mpu6050_write(MPU6050_REG_INT_PIN_CFG, 0x30);
    // enable data ready interrupt
    mpu6050_write(MPU6050_REG_INT_ENABLE, 0x01);
}

#define CALIBRATION_SAMPLES 1000 // number of samples to average for calibration

static int calibration_count;
static int ready;
static int32_t gyro_calibration_sum[3];
static int16_t gryo_error[3];

static void start_calibration(void)
{
    calibration_count = CALIBRATION_SAMPLES;
    memset(gyro_calibration_sum, 0, sizeof gyro_calibration_sum);
}

static void gpio_interrupt_handler(uint gpio, uint32_t event_mask)
{
    if (gpio == MPU6050_INT)
    {
        // mpu6050 interrupt is cleared by read
        int16_t gyro[3];
        mpu6050_get_measurement(gyro);

        if (calibration_count > 0)
        {
            // system is calibrating
            // during calibration, the system is assumed to be holding still
            // average error offset is calculated
            gyro_calibration_sum[0] += gyro[0];
            gyro_calibration_sum[1] += gyro[1];
            gyro_calibration_sum[2] += gyro[2];

            if (--calibration_count <= 0)
            {
                // calibration is complete, take the average
                gryo_error[0] = gyro_calibration_sum[0] / CALIBRATION_SAMPLES;
                gryo_error[1] = gyro_calibration_sum[1] / CALIBRATION_SAMPLES;
                gryo_error[2] = gyro_calibration_sum[2] / CALIBRATION_SAMPLES;

                // the system is ready after at least one calibration
                ready = 1;
            }
        }
        else if (ready)
        {
            // remove the error offset from the measurements
            gyro[0] -= gryo_error[0];
            gyro[1] -= gryo_error[1];
            gyro[2] -= gryo_error[2];

            // convert raw measurements into deg/s and estimate absolute orientation
            static float t, x, y, z;
            t += 1/1000.0f;
            x += gyro[0]/131000.0f;
            y += gyro[1]/131000.0f;
            z += gyro[2]/131000.0f;

            // print current orientation every 100ms
            static int count;
            if (++count > 100)
            {
                printf("%7.3fs %8.3fdeg %8.3fdeg %8.3fdeg\n", t, x, y, z);
                count = 0;
            }
        }
    }
}

// pico has 24 pwm channels -> datasheet pg. 4
// using pins 0 - 3 to control the mosfets
#define PWM_PIN0 0     // this corresponds to motor 1
#define PWM_PIN1 1     // this corresponds to motor 2
#define PWM_PIN2 2     // this corresponds to motor 3
#define PWM_PIN3 3     // this corresponds to motor 4
#define PWM_FREQ 20000 // 20kHz freq for motor control, I believe prof said this is a good one to use anything beyond this is not necessary

int motor_num = 0; // counter for for loop, number of motors is 4

int main()
{
    // initing all standard i/o
    stdio_init_all();

    // initialize the i2c peripheral and use gpio pull-up
    i2c_init(I2C, I2C_FREQ);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL);

    // setup mpu6050 interrupt pin and enable gpio interrupts
    gpio_init(MPU6050_INT);
    gpio_set_irq_enabled_with_callback(MPU6050_INT, GPIO_IRQ_EDGE_RISE, true, gpio_interrupt_handler);

    // wait for start while testing
    while (getchar() != 's')
    {
        sleep_ms(100);
    }

    // startup the mpu6050 and start calibration
    mpu6050_startup();
    mpu6050_enable_interrupts();
    start_calibration();

    // initializing the Wi-Fi chip
    // onboard LED is connected to the Wi-Fi/Bluetooth chip so we must access this chip to enable the onboard LED

    if (cyw43_arch_init())
    {
        // default code from PICO
        printf("Wi-Fi init failed\n"); // print statement if initializing the Wi-Fi chip fails
        return -1;
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    while (true)
    {
        // sensor data is read in interrupt now
        sleep_ms(100);
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); // setting the output of the LED to off
                                                   // CYW43_WL_GPIO_LED_PIN from pic2_w.h file

    // set up for all four pwm, four pins will be used for pins 0-3
    // each pin controls a mosfet that allows the motor to spin based on our duty cycle
    // wrap is the number we want to count up to, default sys clck is 150Mhz, 150Mhz info -
    // info is from 8.1.2.1 in the Raspberry Pi Pico RP2350 Datasheet, link above
    uint32_t wrap = ((150000000 / PWM_FREQ) - 1);

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
    // info from hardware api - link above
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
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); // turning LED on, start of going through motor cycle
        for (motor_num = 0; motor_num < 4; motor_num++)
        {
            // using float because I am using % for duty cycle
            // incrementally increasing speed by 0.01
            for (float duty_cycle = 0.0; duty_cycle <= 0.75f; duty_cycle += 0.01)
            {
                uint16_t pwm_level = wrap * duty_cycle; // multiplying because we want to go from 0 to 75% duty cycle not full power
                pwm_set_gpio_level(motor_num, pwm_level);
                // wait for 100ms
                sleep_ms(100);
            }

            // resetting back down to 0% duty cycle
            pwm_set_gpio_level(motor_num, 0);

            // wait a bit for next motor
            sleep_ms(1000);
        }
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); // turning LED off, meaning all motors have been ran
        sleep_ms(1000);
    }
}
