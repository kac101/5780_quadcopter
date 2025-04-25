# ECE 5780 – Quadcopter

**Team:** Rudis Argueta, Donggeon Kim, Taehoon Kim, Luke Stillings  
**Date:** April 25, 2025

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

### Milestone 2  
- Assembled & bench-tested PCB + 3D-printed frame  
- Verified PWM control (0 %→75 % duty) and MOSFET switching  
- Eliminated a board-short by redesigning mount  

### Milestone 3  
- I²C MPU6050 integration with drift calibration  
- ADC-based battery monitoring (noise & resolution characterized)  
- Full system flight test

---

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