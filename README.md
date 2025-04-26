# ECE 5780 – Quadcopter

**Team:** Rudis Argueta, Donggeon Kim, Taehoon Kim, Luke Stillings  
**Date:** April 25, 2025


<img src="videos_images/IMG_0630.gif" alt="Drone Demo" width="400"/>

**Quadcopter**

---

## Project Summary
A fully realized quadcopter platform built around a Raspberry Pi Pico 2W, powered by a LiPo battery with integrated solar panels.  
- **Stabilization:** MPU6050 IMU via I²C at 1 kHz (interrupt-driven, offset-calibrated)  
- **Control:** Closed-loop PID motor control driving MOSFET-based ESCs with 48 kHz PWM  
- **Monitoring:** On-board ADC for battery-voltage readings (via 1 MΩ / 1 MΩ divider)  
- **Telemetry:** USB serial prints roll & pitch rates plus battery voltage every second  
- **Status LED:** CYW43/Wi-Fi chip toggles onboard LED to mark each control cycle  

---

## System Architecture

1. **Power**  
   - LiPo → voltage divider → ADC (GPIO 28) → Pico  
   - 5 V regulator → Pico & ESC driver MOSFET gates  

2. **Sensing**  
   - MPU6050 → I²C (GPIO 26/27 @ 400 kHz)  
   - Data-ready interrupt (GPIO 22) at 1 kHz  

3. **Control & Telemetry**  
   - PID loop (Kp=1.20, Ki=0.005, Kd=0.00) runs every 1 ms  
   - PWM on four channels (GPIO 6–9) at 48 kHz  
   - Serial telemetry over USB (stdio)  
   - Onboard LED blink marks start/end of each loop  

---

## Development Milestones

### Milestone 1  
- Proto-PCB built to verify MOSFET motor-driver circuit  
- Final PCB layout completed (Pico, MOSFETs, IMU)  
- Component selection and documentation finalized  
<img src="videos_images/QuadCopter_Motor_Layout_IMG.png" alt="Motor Driver Layout" width="400"/>
Motor Driver Layout

### Milestone 2  
- Assembled & bench-tested PCB + 3D-printed frame  
- Verified PWM control (0 %→75 % duty) and MOSFET switching  
- Eliminated a board-short by redesigning mount  


### Milestone 3  
- I²C MPU6050 integration with drift calibration  
- ADC-based battery monitoring (noise & resolution characterized)  
- Full system flight test
<img src="videos_images/Serial_OUTPUT.png" alt="Serial Output Example" width="400"/>
Serial OUTPUT


### Milestone 4
- Demonstation
[Click here to see full demo video](videos_images/IMG_0630.MOV)

---


## Hardware Details

### Wiring

Connect the wire based on following information

MPU 6050 has a 3.3V regulator, so we connected it to VBUS

#### MPU 6050   -> Pico 
         VCC ->         VBUS
         SDA ->         I2C1 SCL
         SCL ->         I2C1 SDA
         GND ->         GND
         INT ->         GP22

#### PWM- Pico  -> Motor Drive
            GP6 -> Motor Drive Motor 1 
            GP7 -> Motor Drive Motor 2
            GP8 -> Motor Drive Motor 3   
            GP9 -> Motor Drive Motor 4
            GND -> GND

#### Voltage Divider -> Motor Driver
                 VCC ->              PWR
                 GND ->              GND
      Divider output ->             GP28

<img src="videos_images/Pico2W_pin_layout.png" alt="Pi Pico2W Pin" width="400"/>



### Frame & how to make

The motor holder parts were removed to reduce the weight, and motors were super glued to the frame. 
Also, gyro MPU 6050 was glued to the frame. Between Pi Pico and motor driver, a wood plate was added to prevent short.
Battery was placed at the bottom bed of this drone and cable tied. All components were wrapped around by tape to be secured.

<img src="videos_images/drone_frame_jpg.png" alt="Drone Frame" width="400"/>

**Drone Frame from Fusion 360**

<br>

<img src="videos_images/Drone_image.jpg" alt="Drone Assembled" width="400"/>

**Drone Image**

### Component

1x Pi Pico 2W (commerical)

1x MPU6050 (commerical)

1x Voltage Divider (we made it)

1x Motor Driver (we made it)

4x Brush Motor (commerical)

1x Drone Frame (we made it)

## Software Details

- **Language & SDK:** C using Pico SDK & hardware API  
- **Key modules:**  
  - `mpu6050_*` – sensor init, interrupt handler, drift calibration  
  - `pwmSetUp()` – configures 4 PWM slices, sets wrap based on 150 MHz/48 kHz  
  - `pid_control()` – computes motor duty cycles from roll/pitch errors  
  - Main loop – toggles CYW43 LED, calls PID, prints telemetry, samples ADC  
- **References:**  
  - Raspberry Pi Pico 2 W Datasheet & RP2350 Datasheet  
  - Pico SDK hardware API  
  - MPU-6000/6050 datasheets & register map
