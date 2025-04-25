# ECE 5780 – Quadcopter

**Team:** Rudis Argueta, Donggeon Kim, Taehoon Kim, Luke Stillings  
**Date:** April 25 2025

---

## Project Summary
A fully realized quadcopter platform built around a Raspberry Pi Pico 2W, powered by a LiPo battery with integrated solar panels.  
- **Stabilization:** MPU6050 IMU via I²C at 1 kHz (interrupt-driven, offset-calibrated)  
- **Control:** Closed-loop motor control (PID-style) driving MOSFET-based ESCs with PWM  
- **Monitoring:** On-board ADC for battery-voltage readings  

---

## System Architecture

1. **Power:** LiPo → voltage divider → Pico & ESC  
2. **Sensing:** MPU6050 → I²C → Pico (1 kHz interrupts)  
3. **Control:** Pico computes motor commands → PWM → MOSFETs → motors  

---

## Development Milestones

### Milestone 1  
- Designed and fabricated proto-PCB for motor-driver validation  
- Finalized PCB layout (Pico, MOSFETs, IMU)  
- Documented component selection and circuit design  

### Milestone 2  
- Assembled and bench-tested custom PCB & frame  
- Verified PWM control (0 %→75 %) and MOSFET switching behavior  
- Resolved a mounting-induced short by redesigning board support  

### Milestone 3  
- Integrated MPU6050 over I²C with 1 kHz sampling and drift calibration  
- Implemented ADC-based battery monitoring (accounted for resolution/noise)  
- Conducted full system flight test → **Stable, controlled hover and maneuvers achieved**

