# Calorie Burner Cycle (IoT-Based Smart Fitness System)

## Project Overview

The **Calorie Burner Cycle** is an IoT-based smart system that integrates fitness tracking with practical energy utilization. This project uses an ESP8266 (NodeMCU) microcontroller along with sensors to monitor real-time cycling parameters such as speed and calories burned.

In addition to tracking fitness data, the system uniquely converts pedaling energy into mechanical power to operate a mixer, demonstrating sustainable energy usage and functional fitness.

---

## Features

* Real-time speed monitoring
* Calorie burn calculation
* TFT display for live data visualization
* Pedal-powered mixer mechanism
* Low-cost and energy-efficient design

---

## Technologies Used

* ESP8266 (NodeMCU)
* Embedded C (Arduino IDE)
* Hall Effect Sensor
* TFT Display (SPI Interface)
* IoT Concepts
* Real-time Data Processing

---

## Working Principle

* A Hall Effect sensor detects wheel rotation.
* The ESP8266 processes the sensor data to calculate speed.
* Calories burned are estimated using dynamic MET values.
* The calculated data is displayed on a TFT screen in real time.
* Mechanical energy generated from pedaling is transferred to a mixer using a belt mechanism.

---

## Running Process

### Step 1: Setup Environment

* Install Arduino IDE
* Install ESP8266 Board Package
* Install required libraries (TFT_eSPI)

### Step 2: Hardware Connections

* Connect Hall sensor to digital pin
* Connect TFT display via SPI
* Connect push button for control
* Set up mechanical linkage for mixer

### Step 3: Upload Code

* Open `.ino` file in Arduino IDE
* Select NodeMCU board and COM port
* Upload the code to ESP8266

### Step 4: Execution

* Power ON the system
* Press button to start cycling session
* View speed and calorie data on TFT display
* Pedal to operate mixer simultaneously

---

## Applications

* Fitness monitoring systems
* Sustainable energy projects
* Smart gym equipment
* Educational IoT demonstrations

---

## Conclusion

The Calorie Burner Cycle successfully combines **fitness tracking and energy utilization** into a single system. It encourages physical activity while demonstrating how human-generated energy can be effectively used for practical purposes. This project highlights the potential of IoT and embedded systems in creating innovative and sustainable solutions.

---

## Future Scope

* Mobile app integration using Wi-Fi
* GPS tracking for distance measurement
* Solar-powered charging system
* Advanced calorie estimation using AI
* Power generation for charging small devices

---
