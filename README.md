# ECE 544 Embedded System Design with FPGA - Pulse Width Modulation and Detection

## Project Overview

This projects centers on replacing the MicroBlaze MCS and IOModule with a more advanced MicroBlaze/AXI-based system. The project involves hands-on experience with Pulse Width Modulation (PWM) detection and interrupt handling on FPGA platforms. By utilizing IPs from Xilinx, Digilent, and ProfRoyK, the project specifically focuses on manipulating and monitoring the RGB LEDs on a Nexys4IO board, using PWM signals to drive and analyze LED behavior.

## Key Features

- **PWM Signal Detection**: Utilizing the Digilent PWMAnalyzer IP to monitor and display the duty cycles of PWM signals on RGB1 and RGB2 LEDs.
- **Interactive LED Control**: Using pushbuttons to control the duty cycles of RGB1 LEDs and displaying results on a 7-segment display.
- **Interrupt Handling**: Employing the AXI FIT timer to manage interrupts, ensuring timely responses to user inputs like button presses and switch toggles.

## Hardware Requirements

- Nexys4IO FPGA board or similar
- Necessary IPs including MicroBlaze/AXI, Digilent PWMAnalyzer, and rgbPWM from ProfRoyK

## Software and Tools

- Xilinx SDK and Vivado for development
- Vitis for application execution

## Setup and Installation

1. **Initialize System**: Setup the project in Vivado by integrating the required IPs into your block design.
2. **Build Project**: Synthesize, implement, and generate the bitstream in Vivado.
3. **Launch Application**: Export hardware to Vitis, set up your application project, and launch it.

## Usage

After setting up the hardware and software:
- **Interact**: Use the switches to control the green LEDs.
- **Modify PWM**: Utilize the pushbuttons to adjust the duty cycles of the RGB LEDs.
- **Observe**: Watch the duty cycle changes reflected on the 7-segment display and the RGB2 LED.

## Configuration Details

- **RGB LEDs**: RGB1 connected to pwm outputs, and RGB2 shows detected duty cycles.
- **Switches and Pushbuttons**: Control LED displays and reset the system.
- **7-Segment Display**: Shows duty cycles for RGB LEDs.

## Common Issues and Troubleshooting

- Ensure proper initialization of all peripherals to prevent unresponsive hardware.
- Check the correct assignment of button functions, as misconfigurations can lead to incorrect duty cycle adjustments.
- Review PWMAnalyzer settings if duty cycles do not appear as expected.

## Contributing

For contributions, please contact the project leader Rutuja Muttha at rmuttha@pdx.edu. Ensure you understand the project's architecture and requirements before making changes.

