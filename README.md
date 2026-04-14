# Capstone_Project: Simulation and Real-Time Control of Automotive Adaptive Cruise Control System Using CAN-Based STM32 Architecture   

## Overview:
This project presents the development of an Electronic Throttle Control (ETC) system integrated with a UART–CAN communication framework. The system connects a MATLAB/Simulink environment with embedded hardware based on STM32, enabling real-time control, monitoring, and data exchange.  

A custom communication protocol is implemented to ensure efficient and reliable transmission between simulation and hardware.  

## Objective:
* Develop a real-time electronic throttle control system
* Integrate MATLAB/Simulink with embedded hardware
* Enable bidirectional communication via UART and CAN
* Evaluate system performance through monitoring and visualization

## System Architecture

The system is composed of the following components:

* MATLAB/Simulink  
    - Generates control signals   
    - Communicates with STM32 via UART
    - Display system outputs through numerical data and graphical plots  
* STM32-based CAN Gateway  
    - Converts UART data into CAN messages and vice versa  
    - Handles communication between simulation and embedded nodes  
* Embedded Control System  
    - Executes throttle control algorithms  
    - Reads sensor data and sends feedback  

## Technologies Used
* MATLAB & Simulink – modeling, simulation, and visualization
* STM32 Microcontroller – embedded system development
* Arduino IDE / Embedded C – firmware implementation
* UART Communication – interface between simulation and hardware
* CAN Bus – communication between system components

## Results
* Achieved stable real-time communication between MATLAB and hardware
* Reliable data exchange via UART and CAN
* Good tracking performance between desired and actual throttle position
* Effective visualization for system analysis

## Project Demonstration:
https://drive.google.com/file/d/1B_2K0y2pJsv6b1HldQCrPIpG6AewvB1J/view?usp=sharing

## Author:  
* Nguyen Tran Nguyen Tuong (me)  
* Le Chi Trung  
* Pham Tan Duy  

## Responsibilities:
* Developed a MATLAB Simulink Serial block for bidirectional UART communication with an STM32-based CAN gateway, enabling CAN data exchange and processing.  
* Implemented UART interrupt handling on STM32 to parse incoming data and update control parameters via a custom protocol.  
* Developed a control and monitoring application to adjust input parameters and visualize system outputs for performance analysis.  
