#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

/*
    refreneces (used C SDK and hardware api for pwm functions and datasheets):
        1. Raspberry Pi Pico 2 W  Datasheet: https://datasheets.raspberrypi.com/picow/pico-2-w-datasheet.pdf
        2. RP2350 - https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf
        3. Hardware API - https://www.raspberrypi.com/documentation/pico-sdk/hardware.html
        4. MPU-6000 and MPU-6050 Product Specification - https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf
        5. MPU-6000 and MPU-6050 Register Map and Descriptions - https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
        6. https://www.youtube.com/watch?v=GK1t8YIvGM8&t=1s
*/

#define I2C i2c1 // i2c instance
#define I2C_FREQ (400 * 1000)
#define I2C_SDA 26 // i2c sda pin
#define I2C_SCL 27 // i2c scl pin

#define MPU6050_INT 22  // mpu6050 interrupt pin
#define MPU6050_I2C I2C // mpu6050 i2c instance
#define MPU6050 0x68    // mpu6050 i2c address
// mpu6050 registers, see the datasheet for a complete list
#define MPU6050_REG_CONFIG 0x1a
#define MPU6050_REG_GYRO_CONFIG 0x1b
#define MPU6050_REG_INT_PIN_CFG 0x37
#define MPU6050_REG_INT_ENABLE 0x38
#define MPU6050_REG_GYRO_XOUT_H 0x43
#define MPU6050_REG_PWR_MGMT_1 0x6b
#define MPU6050_REG_PWR_MGMT_2 0x6c

#define VOLTAGE_INPUT_PIN 28                                        // GPIO 26 for ADC0
#define RESISTOR_1 1000000                                          // 1M resistor
#define RESISTOR_2 1000000                                          // 1M resistor
#define BAT_RATIO_VOLT_DIV (RESISTOR_1 + RESISTOR_2) / (RESISTOR_2) // reading from above resistor 2

#define CALIBRATION_SAMPLES 1000 // number of samples to average for calibration

// pico has 24 pwm channels -> datasheet pg. 4
// using pins 0 - 3 to control the mosfets
#define PWM_PIN0 6      // this corresponds to motor 1 on pcb
#define PWM_PIN1 7      // this corresponds to motor 2 on pcb
#define PWM_PIN2 8      // this corresponds to motor 3 on pcb
#define PWM_PIN3 9      // this corresponds to motor 4 on pcb
#define PWM_FREQ 480000 // 48KHz for drone motors

// the orientation is based on the usb being the back of the drone
// the pwm pins are soldered in a different order due to the motor driver pcb board
#define MOTOR_FRONT_LEFT PWM_PIN2
#define MOTOR_FRONT_RIGHT PWM_PIN3
#define MOTOR_BACK_RIGHT PWM_PIN0
#define MOTOR_BACK_LEFT PWM_PIN1

// these are the desired amount of change in roll and pitch we want the drone, we want no pitch and no roll, just want it to hover
#define ROLL_SETPOINT 0.0f
#define PITCH_SETPOINT 0.0f

static int calibration_count;
static int ready = 0;
static int32_t gyro_calibration_sum[3];
static int16_t gryo_error[3];

static float t;
static float x; // roll
static float y; // pitch
static float z; // yaw

// values to adjust for PID control
volatile float Kp = 1.20f;
volatile float Ki = 0.005f;
volatile float Kd = 0.00f;

static uint32_t pwm_wrap; // wrap for counter

// variables for PID control loop
static float dt = 0.01f;
static float prev_roll_error = 0.0f;
static float prev_pitch_error = 0.0f;
static float roll_integral = 0.0f;
static float pitch_integral = 0.0f;
static float roll_rate;
static float pitch_rate;

// function to keep values within a certain range

static float clamp(float value, float min, float max)
{
    if (value < min)
    {
        return min;
    }
    if (value > max)
    {
        return max;
    }
    return value;
}

static void mpu6050_write(uint8_t reg, uint8_t data)
{
    uint8_t buffer[] = {reg, data};
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

                // reset time and orientation
                t = 0;
                x = 0;
                y = 0;
                z = 0;
            }
        }
        else if (ready)
        {
            // remove the error offset from the measurements
            gyro[0] -= gryo_error[0];
            gyro[1] -= gryo_error[1];
            gyro[2] -= gryo_error[2];

            // going to use rated control for PID, its more common
            roll_rate = (float)gyro[0] / 131.0f;
            pitch_rate = (float)gyro[1] / 131.0f;

            // convert raw measurements into deg/s and update absolute orientation
            t += 1 / 1000.0f;
            x += gyro[0] / 131000.0f;
            y += gyro[1] / 131000.0f;
            z += gyro[2] / 131000.0f;
        }
    }
}

void pwmSetUp(void)
{
    // set up for all four pwm, four pins will be used for pins 0-3
    // each pin controls a mosfet that allows the motor to spin based on our duty cycle
    // wrap is the number we want to count up to, default sys clck is 150Mhz, 150Mhz info -
    // info is from 8.1.2.1 in the Raspberry Pi Pico RP2350 Datasheet, link above
    pwm_wrap = ((150000000 / PWM_FREQ) - 1);

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
    pwm_set_wrap(slice_pin0, pwm_wrap);
    pwm_set_wrap(slice_pin1, pwm_wrap);
    pwm_set_wrap(slice_pin2, pwm_wrap);
    pwm_set_wrap(slice_pin3, pwm_wrap);

    // enabling all the pins
    pwm_set_enabled(slice_pin0, true);
    pwm_set_enabled(slice_pin1, true);
    pwm_set_enabled(slice_pin2, true);
    pwm_set_enabled(slice_pin3, true);
}

static void pid_control(void)
{
    // all PID code is similar from video from ECE 3610: MATLAB Drone Simulation and Control, Part 2: How Do You Get a Drone to Hover?
    // both classes had sections on PID
    // error portion of PID
    float roll_error = ROLL_SETPOINT - roll_rate;
    float pitch_error = PITCH_SETPOINT - pitch_rate;

    // intergral portion of PID
    roll_integral += roll_error * dt;
    pitch_integral += pitch_error * dt;

    // derivative portion of PID
    float roll_deriv = (roll_error - prev_roll_error) / dt;
    float pitch_deriv = (pitch_error - prev_pitch_error) / dt;

    // PID formula
    float roll_control = (Kp * roll_error) + (Ki * roll_integral) + (Kd * roll_deriv);
    float pitch_control = (Kp * pitch_error) + (Ki * pitch_integral) + (Kd * pitch_deriv);

    // saving previous errors for both pitch and roll
    prev_roll_error = roll_error;
    prev_pitch_error = pitch_error;

    // base speed for motors, and minimum speed for motors
    float base_duty_cycle = pwm_wrap;        // base speed for the motors
    float min_duty_cycle = 0.90f * pwm_wrap; // min speed for the motors

    // math from quadcopter PID video
    float motor_1 = base_duty_cycle + roll_control + pitch_control;
    float motor_2 = base_duty_cycle - roll_control + pitch_control;
    float motor_3 = base_duty_cycle - roll_control - pitch_control;
    float motor_4 = base_duty_cycle + roll_control - pitch_control;

    // clamping values
    // motors arent very strong, so we need a higher duty cycle to get them spinning fast enough to make the quadcopter float
    motor_1 = clamp(motor_1, min_duty_cycle, pwm_wrap);
    motor_2 = clamp(motor_2, min_duty_cycle, pwm_wrap);
    motor_3 = clamp(motor_3, min_duty_cycle, pwm_wrap);
    motor_4 = clamp(motor_4, min_duty_cycle, pwm_wrap);

    // setting pwm levels
    pwm_set_chan_level(pwm_gpio_to_slice_num(MOTOR_FRONT_LEFT), pwm_gpio_to_channel(MOTOR_FRONT_LEFT), motor_1);
    pwm_set_chan_level(pwm_gpio_to_slice_num(MOTOR_FRONT_RIGHT), pwm_gpio_to_channel(MOTOR_FRONT_RIGHT), motor_2);
    pwm_set_chan_level(pwm_gpio_to_slice_num(MOTOR_BACK_RIGHT), pwm_gpio_to_channel(MOTOR_BACK_RIGHT), motor_3);
    pwm_set_chan_level(pwm_gpio_to_slice_num(MOTOR_BACK_LEFT), pwm_gpio_to_channel(MOTOR_BACK_LEFT), motor_4);
}

int main()
{
    // initing all standard i/o
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

    // initialize the i2c peripheral and use gpio pull-up
    i2c_init(I2C, I2C_FREQ);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL);

    // setup mpu6050 interrupt pin and enable gpio interrupts
    gpio_init(MPU6050_INT);
    gpio_set_irq_enabled_with_callback(MPU6050_INT, GPIO_IRQ_EDGE_RISE, true, gpio_interrupt_handler);

    // startup the mpu6050 and start calibration
    mpu6050_startup();
    mpu6050_enable_interrupts();
    start_calibration();

    pwmSetUp(); // setting up all pwm signals

    // starting with 0 speed on all motors
    pwm_set_chan_level(pwm_gpio_to_slice_num(PWM_PIN0), pwm_gpio_to_channel(PWM_PIN0), 0);
    pwm_set_chan_level(pwm_gpio_to_slice_num(PWM_PIN1), pwm_gpio_to_channel(PWM_PIN1), 0);
    pwm_set_chan_level(pwm_gpio_to_slice_num(PWM_PIN2), pwm_gpio_to_channel(PWM_PIN2), 0);
    pwm_set_chan_level(pwm_gpio_to_slice_num(PWM_PIN3), pwm_gpio_to_channel(PWM_PIN3), 0);

    // initialize adc hardware
    adc_init();

    // set gpio 26 as adc input (adc0)
    adc_gpio_init(VOLTAGE_INPUT_PIN);

    int counter = 0; // counter for displaying values in serial monitor

    while (true)
    {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); // turning LED on, start of going through motor cycle
        pid_control();
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); // turning LED off, meaning all motors have been ran
        sleep_ms(1);

        if (counter >= 1000)
        {
            adc_select_input(2); // adc channel 2 = gpio 28

            // read raw 12-bit adc value (0 to 4095)
            uint16_t raw = adc_read();

            // convert to actual voltage (0.0 to 3.3v)
            float voltage = (raw * 3.3f) / 4095.0f;
            float actual_bat_voltage = voltage * BAT_RATIO_VOLT_DIV;
            // printing roll,pitch rates and current battery voltage
            printf("Roll Rate:  %.2f °/s, Pitch Rate: %.2f °/s, Battery Voltage: %.2f V\n",
                   roll_rate, pitch_rate, actual_bat_voltage);

            counter = 0;
        }
        counter++;
    }
}
