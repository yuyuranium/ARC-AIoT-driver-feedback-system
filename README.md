# ARC-AIoT-driver-feedback-system

## Introduction

The project is aiming for constructing a driver feedback system that can detect the behavior of the vehicle and analyze the steering style of the driver.

## Develop Board/Module (till May 30th)

![](./img/IMG_2381.JPG)

* Himax WE-1 Plus EVB
  * Edge AI development board, for real time inference
  * Labeling
    * Master device on I2C for accelerometer data transmission when labeling
  * Inference time
    * Master device on I2C for detection result transmission during inference time
  * [Source Code](./himax_we1_evb)
* Arduino Nano
  * Simple development board 
  * Labeling
    * Slave device on I2C to receive data from Himax WE-1 Plus EVB and write it to SD card
  * Inference time
    * Slave device on I2C to receive detection result and display it via LEDs
  * [Source Code](./arduino)
* Micro SD Card Module
  * Record data when labeling

## Hardware Architecture

Upcoming design

![](./img/SafeDriverArchitecture.png)



## Schematic Diagram

TBD.

## Deploy Steps



## Model

### Training

* Dataset
* Classification Target
  * **0_idle**: The vehicle is started but not moving
  * **1_cruise**: The vehicle is running with a stable speed
  * **2_start**: The vehicle is getting started from still
  * **3_stop (*TODO: rename to 3_brake*)**: The vehicle is braking and comes to a halt

## Demo

|   ![](./img/cruise.gif)    |   ![](./img/start.gif)    |   ![](./img/stop.gif)   |
| :------------------------: | :-----------------------: | :---------------------: |
| 1_cruise (LED displays 01) | 2_start (LED displays 10) | 3_stop (LED display 11) |







