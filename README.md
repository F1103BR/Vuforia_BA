| Supported Targets | ESP32 |
| ----------------- | ----- | 

# DIY Power PCB TestSoftware
This software is designed to test the DIY Power PCB by Fabian Zaske. It provides all the necessary functionalities for controlling the bridge, measuring currents, reading external sensors, and adjusting parameters through a menu.


## How to Use
After the welcome screen, you will be taken to the main menu. In the first position, you can change the mode between MCPWM, BLDC, and brushedDC by pressing the encoder button. At the bottom of all modes, the status of the bridge is displayed as one of the following: Deactive, OC (OverCurrent), UV (UnderVoltage), or Active. By selecting configure ->, you can access another menu where you can modify most PWM parameters (currently, the Dead-Time parameter cannot be changed). By selecting sensor info ->, you can view detailed information about the current and external sensors.

The Modes:
MCPWM Mode: 
In this mode, you can start a PWM on each output. When the status is set to "Stopped," press the button to enable the PWM so that it changes to "Started." Verify that the status changes to "Active" to ensure the PWM is functioning correctly. Use the "OUT:" option to cycle through the three phases. The first phase displayed is configured as High-side, and the second as Low-side.

BLDC Mode: 
In BLDC mode, the selected phase is determined by the connected Hall sensor values. If no Hall sensor is connected, "ERROR" will be displayed. The operation is similar to MCPWM mode; however, currently, only duty cycles below 5% work reliably, likely due to coding limitations.

brushedDC mode:
In brushed DC mode, you have more options for enabling the High-side. You can choose to activate only one High-side or all at once. The rest of the operation is similar to the other modes.

### Hardware Required
DIY-Power-PCB v2 with ESP32


### Configure the Project
You can configure the pin assignments and some PWM settings in the ESP-IDF kconfig menu. Most configurations can also be adjusted at runtime.

### Build and Flash
Build the project using CMake and flash it via USB-C. If flashing fails, try turning the board's encoder one step. Due to the dual use of pins, the encoder may block the flashing process every other step.

## Troubleshooting
The encoder functions correctly only when powered by the 5V USB-C port.
