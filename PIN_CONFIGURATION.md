# 🔌 Robotic Arm Pinout & Hardware Connection Guide

Detailed pinout and power isolation schematic for the **Wi-Fi Controlled Robotic Arm using ESP32**.

---

## 📌 ESP32 to Servo Motor Pinout Table

| Servo Joint | ESP32 GPIO | Channel Type | Signal Frequency | Initial Angle | Motion Range |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Base (Rotation)** | `GPIO 27` | PWM | 50 Hz | 90° | 0° – 180° |
| **Shoulder (Elevation)** | `GPIO 26` | PWM | 50 Hz | 90° | 0° – 180° |
| **Elbow (Reach)** | `GPIO 25` | PWM | 50 Hz | 90° | 0° – 180° |
| **Gripper (End-Effector)** | `GPIO 33` | PWM | 50 Hz | 90° | 0° – 180° |

---

## ⚡ Power Supply & Common Ground Architecture

> ⚠️ **CRITICAL ELECTRICAL WARNING:**  
> Never power multiple servo motors directly from the ESP32's `3.3V` or `VIN` pins! Simultaneous movement of 4 servos draws 1.5A to 2.5A peak current during movement/stall, which causes brownout resets or damages the ESP32 voltage regulator.

### Wiring Rule:
1. **Dedicated 5V Supply:** Connect the positive (`+`, Red wire) of all servo motors to an **external regulated 5V (2A – 3A) DC power supply** or 5V 2.4A USB Power Bank.
2. **Common Ground:** Connect the negative (`-`, Black/Brown wire) of all servos to **both the external power supply GND AND the ESP32 GND**. This shared reference ground is required for stable PWM signal timing.
3. **Signal Lines:** Connect the signal (Orange/Yellow wire) of each servo directly to its designated ESP32 GPIO pin.

---

## 🗺️ Circuit Schematic Diagram

```text
  +-------------------------------------------------------+
  |              EXTERNAL 5V POWER SUPPLY (2A+)           |
  |                                                       |
  |   [+5V Output] -----------------+---> Base Servo VCC  |
  |                                 +---> Shoulder VCC    |
  |                                 +---> Elbow Servo VCC |
  |                                 +---> Gripper VCC     |
  |                                                       |
  |   [GND Output] -------+                               |
  +-----------------------|-------------------------------+
                          | (Shared Common Ground)
                          v
  +-------------------------------------------------------+
  |                       ESP32                           |
  |                                                       |
  |   [GND Pin] <---------+                               |
  |                                                       |
  |   [GPIO 27] (PWM) -----------------> Base Servo SIG   |
  |   [GPIO 26] (PWM) -----------------> Shoulder SIG     |
  |   [GPIO 25] (PWM) -----------------> Elbow Servo SIG  |
  |   [GPIO 33] (PWM) -----------------> Gripper SIG      |
  +-------------------------------------------------------+
