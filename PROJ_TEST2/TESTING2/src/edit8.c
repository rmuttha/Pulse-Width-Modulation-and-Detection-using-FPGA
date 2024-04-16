/*************************************************************************************************************************************************************
*###############################################################
*Project by Rutuja Muttha (rmuttha@pdx.edu)
*PSU ID: 916655417
*Course: ECE 544 Embedded System Design with FPGA
*Project 1: Pulse Width Modulation and Detection
*Date: 13th February 2024
*###############################################################
* @brief
* This program implements a test program for the Nexys4IO peripheral and driver.
*
* The program includes functionality to test the operation of LEDs, 7-segment display, switches, and pushbuttons on the Nexys4IO board.
* The FIT interrupt is enables after that and writes the switches to the LEDs and the pushbuttons to the decimal points on the
* 7-segment display.
*
* @note
*The buttons are mapped
* as follows [NexysA7(Boolean)]:
* 
*                o BTNU(BTN3)
*
* BTNL(BTN1) o   o  BTNC(4)   o BTNR(BTN0)
*
*                o BTND(BTN2)
*
* The button vector expected is {BTNR(BTN4), BTNL(BTN3), BTND(BTN2), BTNU(BTN1), BTNC(BTN0)}
****************************************************************************************************************************************************************/

/*********** Include Files **********/
#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xstatus.h"
#include "xgpio.h"
#include "microblaze_sleep.h"
#include "PWM_Analyzer.h"
#include "xtmrctr.h"
#include "xintc.h"
#include "nexys4IO.h"
#include "xgpio_l.h"


/********** DEBUG OUTPUT FLAG **********/
//#define _DEBUG  1		// uncomment to enable debug output

/*********** Peripheral-related constants **********/
// Clock frequencies
#define CPU_CLOCK_FREQ_HZ		XPAR_CPU_CORE_CLOCK_FREQ_HZ
#define AXI_CLOCK_FREQ_HZ		XPAR_CPU_M_AXI_DP_FREQ_HZ

// AXI timer parameters (used to set Nexys4IO RGB Clock period)
// Timing Interval = (1998 + 2) * (1/10MHz) -> 50 KHz/PWM Period * 512) => 98Hz
#define N4IO_PWM_CLK_DEVICE_ID	XPAR_TMRCTR_0_DEVICE_ID
#define N4IO_PWM_CLK_BASEADDR	XPAR_TMRCTR_0_BASEADDR
#define N4IO_PWM_CLK_HIGHADDR	XPAR_TMRCTR_0_HIGHADDR
#define N4IO_PWM_CLK_FREQ_HZ 	XPAR_TMRCTR_0_CLOCK_FREQ_HZ
#define N4IO_PWM_CLK_TIMER_NUM	0
#define N4IO_PWM_CLK_TIMER_VAL	1998

// Definitions for peripheral NEXYS4IO
#define N4IO_DEVICE_ID		    XPAR_NEXYS4IO_0_DEVICE_ID
#define N4IO_BASEADDR		    XPAR_NEXYS4IO_0_S00_AXI_BASEADDR
#define N4IO_HIGHADDR		    XPAR_NEXYS4IO_0_S00_AXI_HIGHADDR

// Definitions for Fixed Interval timer - 100 MHz input clock, 2Hz output clock
// Used as interval to handle the slide switches and pushbuttons
#define FIT_IN_CLOCK_FREQ_HZ	XPAR_CPU_CORE_CLOCK_FREQ_HZ
#define FIT_CLOCK_FREQ_HZ		2
#define FIT_INTR_NUM			XPAR_MICROBLAZE_0_AXI_INTC_FIT_TIMER_0_INTERRUPT_INTR

//Definitions for Interrupt Controller
#define INTC_DEVICE_ID			XPAR_INTC_0_DEVICE_ID
#define INTC_BASEADDR			XPAR_INTC_0_BASEADDR
#define INTC_HIGHADDR			XPAR_INTC_0_HIGHADDR

/*********** Application-specific constants **********/
#define NBTNS 			5		// number of buttons
#define MAXDC			1023
#define DCINCR			63


/********** AXI Peripheral Instances **********/
XTmrCtr		N4IO_TimerInst;	// Timer instance for N4IO rgb clock input
XIntc 		INTC_Inst;		// Interrupt Controller instance


/********** Global Variables **********/
// These are volatile because they are generated in the FIT handler which is asynchronous
// to the program. We want to make sure the current values of the variables are returned
volatile bool newbtnsSw = false; // true if the FIT handler updated global buttons and switch values
volatile uint16_t sw = 0;	// switches - set in the FIT handler
volatile uint8_t btns = 0;	// buttons - set in the FIT handler
u16 pwmRedDC = 0, pwmGreenDC = 0, pwmBlueDC = 0;
XGpio_Config GPIO_Cfg;
XGpio GPIO_Inst;
bool pwmEnable = true;
u32 RED_DIGIT7 = 0, RED_DIGIT6 = 0, GREEN_DIGIT4 = 0, GREEN_DIGIT3 = 0, BLUE_DIGIT1 = 0, BLUE_DIGIT0 = 0;
u32 Red1=0, Green1=0, Blue1=0;
u32 Seven_Seg_Green, Seven_Seg_Red, Seven_Seg_Blue;

/********** Function Prototypes **********/
// interrupt handlers
void FIT_Handler(void);

// initialization functions
int  N4IO_RGB_timer_initialize(void);
int	 do_init(void);

// other functions
uint8_t swizzleBtns(uint8_t btns);
void nexys4io_selfTest(void);

u32 handleBtns(u32 btns);
u32 buildPWMCtrlReg(bool enable, u16 RedDC, u16 GreenDC, u16 BlueDC);

/********** Main Program **********/
int main()
{
	// u32 switches, prev_switches = 0xFFFF;
	u32 btns;
	u32 PWMCntrlReg;
	// Announce that the application has started
	xil_printf("ECE 544 Nexys4IO Test Application\r\n");
	xil_printf("By Rutuja Muttha (rmuttha@pdx.edu)\r\n\n");
	xil_printf("ECE 544 Project #1  -  Pulse Width Modulation and Detection");
	init_platform();
	uint32_t sts = do_init();
	if (XST_SUCCESS != sts){
		xil_printf("FATAL(main): System initialization failed\r\n");
		return 1;
	}

	// perform the self test
	microblaze_disable_interrupts();
   // Run a diagnostic test on the Nexys4IO board component
	nexys4io_selfTest();

	// main loop
	microblaze_enable_interrupts();
	while (1) {

		// update/analyze the rgb PWM duty cycle(s) if they were changed by the FIT handler
		if (newbtnsSw) {
			btns = NX4IO_getBtns();
			PWMCntrlReg = handleBtns(btns);
			XGpio_DiscreteWrite(&GPIO_Inst,1 , PWMCntrlReg);
			// write the switches to the LEDs
			NX4IO_setLEDs(sw);
			xil_printf("New switches: 0x%04X\tNew buttons: 0x%02X\n\r", sw, btns);
			// Write the pushbutton values to the seven-segment display
			newbtnsSw = false;
			usleep(1000);
		}


	}
    // say goodbye and exit - should never reach here
	microblaze_disable_interrupts();
	NX410_SSEG_setAllDigits(SSEGHI, CC_BLANK, CC_B, CC_LCY, CC_E, DP_NONE);
	NX410_SSEG_setAllDigits(SSEGLO, CC_B, CC_LCY, CC_E, CC_BLANK, DP_NONE);
    xil_printf("ECE 544 Nexys4IO Test Program...ending\r\n");
    cleanup_platform();
    return 0;
}

/*********** HELPER FUNCTIONS *************/

/**
 * handleBtns() - processes button presses
 *
 * @brief looks for rising and falling edges on the pushbuttons and updates
 * the PWM duty cycle and PWM enable variables.  Returns the updated value
 * of the PWM control register which can be sent to GPO4 in the I/O module.
 * The previous button values are used to detect edges and are static in this
 * function.  Reads/modifies the global PWM duty cycle registers
 *
 * @param btns	button register from NX4IO.
 *
 * @return		the PWM Control register value and digit display on seven segment display
 *
 * @note:  pushbutton mapping: {btnR, btnL, btnD, btnU, btnC}
 * 			btnC = disable PWM, btnR = Blue, btnU = Green, btnL = Set PWMs to 0,  btnD = Red
 */
u32 handleBtns(u32 btns) {
	static u32 prevBtns;		// previous value of button register
	static bool isInitialized;// true if the function has run at least once

	// initialize the static variables the first time the function is called
	if (!isInitialized) {
		prevBtns = btns ^ 0x0000001F;	// invert btns to get everything started
		isInitialized = true;
	}

	// XOR prev and current buttons.  If any of the bits are 1
	// at least one of the buttons has changed state since the last
	// time the function was called
	if ((prevBtns ^ btns) != 0) {

		// one or more buttons have changed state
		u32 btnMsk = 0x00000001;
		for (int i = 0; i < NBTNS; i++) {
			// look at the buttons one at a time to see if  there was a rising
			// edge on the button.  If so, increment the affected duty cycles or invert the pwmEnable
			if (btns & (btnMsk << i)) {
				// rising edge on btn[i]
				NX4IO_RGBLED_setChnlEn(RGB2, 1, 1, 1);
				switch (i) {
					// iterate through the buttons and modify global PWM duty cycle variables
					case 0:	// btnR = Blue duty cycle
						pwmBlueDC = ((pwmBlueDC + DCINCR) <= MAXDC) ? pwmBlueDC + DCINCR : 0;
						Seven_Seg_Blue = (pwmBlueDC * 100 / 1023);
						// separating the msb value
						BLUE_DIGIT1= (Seven_Seg_Blue/10) % 10;
						// separating the lsb value
						BLUE_DIGIT0= Seven_Seg_Blue % 10;
						Blue1=(PWM_Analyzer_GetDutyCycle_percent(XPAR_PWM_ANALYZER_2_S00_AXI_BASEADDR)<< 2);
						break;

					case 1:	 // btnL - Set all duty cycles to 0
						// initializing all the registers to zero

						// For RGB1_leds
						pwmGreenDC = 0;
						pwmBlueDC = 0;
						pwmRedDC = 0;

						// For RGB2_leds
						Red1=0;
						Blue1=0;
						Green1=0;

						// For 7-Segment Display
						RED_DIGIT7 = 0 ;
						RED_DIGIT6 = 0;
						GREEN_DIGIT4 = 0;
						GREEN_DIGIT3 = 0;
						BLUE_DIGIT1 = 0;
						BLUE_DIGIT0 = 0;
						NX4IO_RGBLED_setDutyCycle(RGB2,Red1,Green1,Blue1);
						break;

					case 2: // btnB - Set all duty cycles to 0
						pwmRedDC = ((pwmRedDC + DCINCR) <= MAXDC) ? pwmRedDC + DCINCR : 0;
						// Calculating seven_segment display value
						Seven_Seg_Red = (pwmRedDC * 100 / 1023);
						// separating the msb value
						RED_DIGIT7= (Seven_Seg_Red/ 10) % 10;
						// separating the lsb value
						RED_DIGIT6 = Seven_Seg_Red % 10;
						// Driving the PWM_Analyzer value to the respective RGB2_led
						Red1=( PWM_Analyzer_GetDutyCycle_percent(XPAR_PWM_ANALYZER_0_S00_AXI_BASEADDR)<< 2);
						break;

					case 3:	// btnU - Green duty cycle
						pwmGreenDC = ((pwmGreenDC + DCINCR) <= MAXDC) ? pwmGreenDC + DCINCR : 0;
						// Calculating seven_segment display value
						Seven_Seg_Green = (pwmGreenDC * 100 / 1023);
						// separating the msb value
						GREEN_DIGIT4= (Seven_Seg_Green/10) % 10;
						// separating the lsb value
						GREEN_DIGIT3=Seven_Seg_Green % 10;
						// Driving the PWM_Analyzer value to the respective RGB2_led
						Green1=(PWM_Analyzer_GetDutyCycle_percent(XPAR_PWM_ANALYZER_1_S00_AXI_BASEADDR)<< 2);
						break;

					case 4: // btnC - disable/enable PWM   //
						pwmEnable = !pwmEnable;
						NX4IO_RGBLED_setChnlEn(RGB2, 0, 0, 0);
						break;

					default:
							break;
				}
			}
		}
		// Displaying values on console
		xil_printf("RedDC= %d%% (%3d), GreenDC= %d%% (%3d), BlueDC= %d%% (%3d) pwmEnable=%s\r\n",
			(pwmRedDC * 100 / MAXDC), pwmRedDC,
			(pwmGreenDC * 100 / MAXDC), pwmGreenDC,
			(pwmBlueDC*100 / MAXDC), pwmBlueDC,
			(pwmEnable ? "ON" : "OFF")
		);
		 NX4IO_RGBLED_setDutyCycle(RGB2,Red1,Green1,Blue1);
		 // Displaying value on the 7 segment display
		 NX410_SSEG_setAllDigits(SSEGHI, RED_DIGIT7, RED_DIGIT6 , CC_BLANK, GREEN_DIGIT4, DP_NONE);
		 NX410_SSEG_setAllDigits(SSEGLO, GREEN_DIGIT3, CC_BLANK, BLUE_DIGIT1, BLUE_DIGIT0, DP_NONE);

		prevBtns = btns;
	}
	// build the control register for PWM
	u32 ctlreg = buildPWMCtrlReg(pwmEnable, pwmRedDC, pwmGreenDC, pwmBlueDC);
	return ctlreg;
}


/**
 * buildPWMCtrlReg() - returns a PWM ControlReg value
 *
 * @brief combines the enable and PWM duty cycle bits to create
 * a value that can be loaded into the PWM Control register
 *
 * @param enable	PWM enable.  True to enable the PWM outputs
 * @param RedDc		Duty cycle for RED pwm.  Range is 0..1023
 * @param GreenDc	Duty cycle for GREEN pwm.  Range is 0..1023
 * @param Blue		Duty cycle for BLUE pwm.  Range is 0..1023
 *
 * @return			A PWM Control register value
 */
u32 buildPWMCtrlReg(bool enable, u16 RedDC, u16 GreenDC, u16 BlueDC) {
	u32 cntlreg;

	// initialize the value depending on whether PWM is enabled
	// enable is Control register[31]
	cntlreg = (enable) ? 0x80000000 : 0x0000000;

	// add the duty cycles
	cntlreg |=	((BlueDC & 0x03FF)  << 0)  |
				((GreenDC & 0x03FF) << 10) |
				((RedDC & 0x03FF)   << 20);
	return cntlreg;
}


/********** Interrupt Handlers **********/

/****************************************************************************/
/**
* Fixed interval timer interrupt handler
*
* Reads the switches and sets the handshaking signal if any changes.
* Reads the button and sets the handshaking signal if any changes.
* Checks/sets the global newbtnsSw which is the handshake between the interrupt handler and main
*
* @note:  pushbutton mapping: {0 0 0 btnC btnU btnD btnL btnR}
*
* @note  This handler should be called about twice per second
*****************************************************************************/
void FIT_Handler(void)
{
	static bool isInitialized = false;	// true if the function has run at least once
	static uint8_t prevBtns;			// previous value of button register
	static uint16_t prevSw;				// previous value of the switch register
	static bool dpOn;					// true if decimal point 0 is on
	uint8_t btns_int;					// temp btns. Needs to be swizzled to get bits in right place for main()

	// initialize the static variables the first time the function is called
	if (!isInitialized) {
		prevBtns = 0x0F;	// invert btns to get everything started
		prevSw = 0xFFFF;	// invert switches to get everything started
		dpOn = true;
		isInitialized = true;
	}

	// toggle DP0 to indicate that FIT handler is being called
	// this happens every time the FIT handler is called
	dpOn = (dpOn) ? false: true;
	NX4IO_SSEG_setDecPt (SSEGLO, DIGIT0, dpOn);

	// return if main() hasn't handled the last button and switch changes
	if (newbtnsSw) {
		return;
	}

	// get the value of the switches.  Will be written to the LEDs in main()
	sw =  NX4IO_getSwitches();
	if (prevSw != sw) {
		newbtnsSw = true;
		prevSw = sw;
	}

	// get the value of the buttons.  Will be written to the 7-segment decimal points in main()
	btns_int = NX4IO_getBtns ();
	btns = swizzleBtns(btns_int);

	if (prevBtns != btns) {
		newbtnsSw = true;
		prevBtns = btns;
	}
}

/********** Initialization functions **********/

/****************************************************************************/
/**
 * N4IO_RGB_timer_initialize() - initializes the Nexys4IO rgbPWM clock timer
 *
 * @brief The Nexys4 IO IP block has an independant clock input for the 3 channel
 * PWM generator for the two RGB LEDs on the Nexys A7.  In the MicroBlaze-based
 * system used for this project that clock is generated by an AXI programmable
 * interval timer.  This function initializes the timer, runs the self test and
 * configures and starts the timer.
 *
 * @return			XST_Success if the timer is initialized.  XST_FAILURE if it is not
 */
int  N4IO_RGB_timer_initialize(void) {
	uint32_t status;	// status from Xilinx Lib calls

	status = XTmrCtr_Initialize(&N4IO_TimerInst,N4IO_PWM_CLK_DEVICE_ID);
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }
	status = XTmrCtr_SelfTest(&N4IO_TimerInst, N4IO_PWM_CLK_TIMER_NUM);
    if (status != XST_SUCCESS) {
			return XST_FAILURE;
    }

	// Configure the timer
	// We are going to use the low level functions because there doesn't
	// seem to be a configuration option that enables the Generate Out options
	// and load an initial timer value.  This code comes from xtrctr_low_level_example
	uint32_t ctl = XTC_CSR_AUTO_RELOAD_MASK |  XTC_CSR_DOWN_COUNT_MASK |  XTC_CSR_EXT_GENERATE_MASK;

	XTmrCtr_SetControlStatusReg(N4IO_PWM_CLK_BASEADDR, N4IO_PWM_CLK_TIMER_NUM, (ctl | XTC_CSR_LOAD_MASK));
	XTmrCtr_SetLoadReg(N4IO_PWM_CLK_BASEADDR, N4IO_PWM_CLK_TIMER_NUM, N4IO_PWM_CLK_TIMER_VAL);
	XTmrCtr_LoadTimerCounterReg(N4IO_PWM_CLK_BASEADDR, N4IO_PWM_CLK_TIMER_NUM);
	XTmrCtr_SetControlStatusReg(N4IO_PWM_CLK_BASEADDR, N4IO_PWM_CLK_TIMER_NUM, (ctl & ~XTC_CSR_LOAD_MASK));

	// and start it
	XTmrCtr_Start(&N4IO_TimerInst, N4IO_PWM_CLK_TIMER_NUM);
	return XST_SUCCESS;
}

/**
 * do_init() - initializes the embedded system peripherals
 *
 * @brief This function initializes the AXI peripherals and sets up for interrupt handling.  Interrupts
 * are enabled/disabled in main() or other functions.  When this function exits the peripherals have
 * been initialized but interrupts are left disabled
 *
 * @return			XST_Success if the timer is initialized.  XST_FAILURE if it is not
 *
 * @note:  Digilent PWM Analyzer is a low level driver and does not need to be initialized
 */
int	 do_init(void) {
	uint32_t status;				// status from Xilinx Lib calls

	GPIO_Cfg.DeviceId = XPAR_GPIO_0_DEVICE_ID;
	GPIO_Cfg.BaseAddress = XPAR_GPIO_0_BASEADDR;
	GPIO_Cfg.InterruptPresent = FALSE;
	GPIO_Cfg.IsDual = FALSE;
	GPIO_Inst.IsReady = XIL_COMPONENT_IS_READY;

	//XGpio_CfgInitialize(&GPIO_Inst, &GPIO_Cfg, XPAR_GPIO_0_BASEADDR);
	XGpio_Initialize(&GPIO_Inst, XPAR_GPIO_0_DEVICE_ID );
	XGpio_SetDataDirection(&GPIO_Inst, 1,0);
	XGpio_GetDataDirection(&GPIO_Inst,1);

	// initial the RGB PWM clock to Nexys4IO
	status = N4IO_RGB_timer_initialize();
	if (status != XST_SUCCESS)
		return XST_FAILURE;

	// initialize the Nexys4 driver
	status = NX4IO_initialize(N4IO_BASEADDR);
	if (status != XST_SUCCESS){
		return XST_FAILURE;
	}

	 // initialize the interrupt controller
	status = XIntc_Initialize(&INTC_Inst, INTC_DEVICE_ID);
	if (status != XST_SUCCESS)
	{
	   return XST_FAILURE;
	}

	// connect the interrupt handlers to the interrupts
	status = XIntc_Connect(&INTC_Inst, FIT_INTR_NUM,
						   (XInterruptHandler)FIT_Handler,
						   (void *)0);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	// start the interrupt controller such that interrupts are enabled for
	// all devices that cause interrupts.
	status = XIntc_Start(&INTC_Inst, XIN_REAL_MODE);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	// enable/disable the interrupts
	XIntc_Enable(&INTC_Inst, FIT_INTR_NUM);
	return XST_SUCCESS;
}

/********** Helper functions **********/

/**
 * swizzleBtns() - formats the btns register from Nexys4IO for easy processing
 *
 * @brief 
 * rearranges the bits from the Nexys4IO register {3'b0, 1'b0, BTNU(BTN0), BTND(BTN3), BTNL(BTN2), BTNR(BTN1)}
 * to {BTND(BTN3), BTNL(BTN2), BTNR(BTN1), BTNU(BTN0)}
 *
 * @param btns	button vector returned by Nexys4IO driver
 * @return 		swizzled buttons
 */
uint8_t swizzleBtns(uint8_t btns) {
	uint8_t btnd, btnl, btnr, btnu,btnc;		// individual buttons
	uint8_t b;							// swizzled buttons vector
    btnc = (btns & 0x10)>> 4;
    btnu = (btns & 0x08) >> 3;
    btnd = (btns & 0x04) >> 2;
    btnl = (btns & 0x02) >> 1;
	btnr = (btns & 0x01) >> 0;
	b = (btnc<<4)|(btnd << 3) | (btnl << 2) | (btnr << 1) | (btnu << 0);
	return b;
}

/****************************************************************************/
/**
 * nexys4io_selfTest() - performs a self test on the NexysA7 peripheral
 *
 * @brief This is mostly a visual test to confirm that the 7-segment display and
 * RGB LEDs hardware and drivers are operating correctly.  The test does the following:
 *	o sends pattern(s) to the LEDs on the board
 *	o Writes a message on the 7-segment display
 *	o individually lights the RGB LEDs
 *	o sets the RGB2 LED to several values that can be observed
 *	o Turns off the LEDs and blanks the 7-segment digits and decimal points
 */
 void nexys4io_selfTest(void) {
	xil_printf("Starting Nexys4IO self test...\r\n");

	xil_printf("\tcheck functionality of LEDs\r\n");
	uint16_t ledvalue = 0x0001;
	do {
		NX4IO_setLEDs(ledvalue);
		usleep(250 * 1000);
		ledvalue = ledvalue << 1;
	} while (ledvalue != 0);

	xil_printf("\tcheck functionality of 7-segment display\r\n");
	// set the display digits to -ECE544- and turn off
	// the decimal points using the "raw" set functions.
	NX4IO_SSEG_setSSEG_DATA(SSEGHI, 0x0058E30E);
	NX4IO_SSEG_setSSEG_DATA(SSEGLO, 0x00144116);
	usleep(2000 * 1000);

	xil_printf("\tcheck functionality of the RGB2 LED\n\r");
	// RGB2 is controlled by Nexys4IO
	NX4IO_RGBLED_setChnlEn(RGB2, true, true, true);
	xil_printf("\t\tRGB2 segments set to {{99%%, 00%%, 00%%} duty cycle - display RED\n\r");
	NX4IO_RGBLED_setDutyCycle(RGB2, 255, 0, 0);
	usleep(2000 * 1000);

	xil_printf("\t\tRGB2 segments set to {{00%%, 50%%, 00%%} duty cycle - display Green\n\r");
	NX4IO_RGBLED_setDutyCycle(RGB2, 0, 255, 0);
	usleep(2000 * 1000);

	xil_printf("\t\tRGB2 segments set to {{00%%, 00%%, 25%%} duty cycle - display Blue\n\r");
	NX4IO_RGBLED_setDutyCycle(RGB2, 0, 0, 64);
	usleep(2000 * 1000);

	xil_printf("\t\tRGB2 segments set to {{50%%, 12%%, 25%%} duty cycle - display Purplish-ish\n\r");
    NX4IO_RGBLED_setDutyCycle(RGB2, 255, 32, 64);
	usleep(2000 * 1000);


	// turn off LEDs, display and RGB LEDs
	NX410_SSEG_setAllDigits(SSEGLO, CC_BLANK, CC_BLANK, CC_BLANK, CC_BLANK, DP_NONE);
	NX410_SSEG_setAllDigits(SSEGHI, CC_BLANK, CC_BLANK, CC_BLANK, CC_BLANK, DP_NONE);
    NX4IO_setLEDs(0x0000);

	// Turn off RGB2
	NX4IO_RGBLED_setDutyCycle(RGB2, 1, 1, 1);
	xil_printf("...Nexys4IO self test complete\r\n");
	return;
 }

