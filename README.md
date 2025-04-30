# CS454-Amazing-Ball-System
:floppy_disk: This repo is an archive of our work for CS 454 Embededd Systems (Sping 2025) at Boston University, the course is originally by Dr.Denato Manuso and was taught by Dr.Shahin Roozkhosh.
The code was developed on the MPLab IDE and VS Code (for the Linux and Python portions), and ran on a dsPIC33FJ256MC710 microcontroller. 

**By Team 3a:** Rigpea Wangchuk, Brian Wong, and Joey Zhu

## Video Demo
Here is a video demo of our final assignment:
// TODO

## Summary of each lab/folder
1. **Hello World:** Starter project and basics -- playing with the LCD module and LEDs
2. **Interrupt & Timers:** Learned about using system timers and timer interrupts
3. **UART:** Learned about Serial Communication and implemented UART communication between our microcontroller and a Linux computer
4. **POSIX & FFT:**
    - Learned about Fast Fourier Transform (FFT) on Python
    - Used a LabJack U3-LV data acquisition system (DAQ) and Linux's CLOCK_REALTIME POSIX signal to emulate digital signals, then read them using an oscilloscope
5. **ADC & PWM:** Used ADC to read a joystick input, which is used to control a servos through PWM
6. **Touchscreen ADC:** Implemented 12-bit touchscreen sampling using the ADC (took 5 samples at different locations and took their medians)
7. **Balancing Ball Sim Python:** Implemented PD control for a 2D ball balancing system in Python simulation
8. **Balancing Ball:** Implemented the same PD control on the microcontroller at 20Hz (sampling every 50ms)

:tada:*Special thanks to our TAs Koneshka and Yann for all the help and support!*
