/******************************************************************************
* Copyright (c) 2014 - 2022 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
*
* @file microblaze_sleep.c
*
* Contains implementation of microblaze sleep function.
*
* <pre>
* MODIFICATION HISTORY:

*
******************************************************************************/

/***************************** Include Files *********************************/

#include "microblaze_sleep.h"
#include "bspconfig.h"

/***************** Macros (Inline Functions) Definitions *********************/

#if defined (XSLEEP_TIMER_IS_AXI_TIMER)
#define COUNTS_PER_SECOND (SLEEP_TIMER_FREQUENCY)
#define COUNTS_PER_MSECOND ((COUNTS_PER_SECOND + 500) / 1000)
#define COUNTS_PER_USECOND ((COUNTS_PER_SECOND + 500000)/ 1000000)
#warning "May wait for more than the specified delay"
#elif defined (XSLEEP_TIMER_IS_DEFAULT_TIMER) || defined (FREERTOS_BSP)
#define ITERS_PER_SEC	(Xil_GetMBFrequency() / 4U)
#define ITERS_PER_MSEC	((ITERS_PER_SEC + 500U) / 1000U)
#define ITERS_PER_USEC	((ITERS_PER_MSEC + 500U) / 1000U)
#pragma message ("For the sleep routines, assembly instructions are used")
#endif
/***************** Variable Definitions *********************/
#if defined (XSLEEP_TIMER_IS_DEFAULT_TIMER) || defined (FREERTOS_BSP)
/**
 * This variable stores current value of Microblaze frequency.
 * It would be used by sleep routines to generate delay.
 */
static u32 MBFreq = XPAR_CPU_CORE_CLOCK_FREQ_HZ;
#endif

/*****************************************************************************/

#if defined (XSLEEP_TIMER_IS_DEFAULT_TIMER) || defined (FREERTOS_BSP)
/*****************************************************************************/
/**
* @brief	Sets variable which stores Microblaze frequency value
* @param	Val - Frequency value to be set
* @return	XST_SUCCESS - If frequency updated successfully
* 			XST_INVALID_PARAM - If specified frequency value is not valid
*
* @note		It must be called after runtime change in Microblaze frequency,
* 			failing to do so would result in to incorrect behavior of sleep
* 			routines
*
******************************************************************************/
u32 Xil_SetMBFrequency(u32 Val)
{
	if ( Val != 0U) {
		MBFreq = Val;
		return XST_SUCCESS;
	}
	return XST_INVALID_PARAM;
}

/*****************************************************************************/
/**
* @brief	Returns current Microblaze frequency value
* @return	MBFreq - Current Microblaze frequency value
*
******************************************************************************/
u32 Xil_GetMBFrequency(void)
{
	return MBFreq;
}

static void sleep_common(u32 n, u32 iters)
{
	asm volatile (
			"1:               \n\t"
			"addik %1, %1, -1 \n\t"
			"add   r7, r0, %0 \n\t"
			"2:               \n\t"
			"addik r7, r7, -1 \n\t"
			"bneid  r7, 2b    \n\t"
			"or  r0, r0, r0   \n\t"
			"bneid %1, 1b     \n\t"
			"or  r0, r0, r0   \n\t"
			:
			: "r"(iters), "r"(n)
	);
}
#endif

/*****************************************************************************/
/**
* @brief    Provides delay for requested duration.
* @param	useconds- time in useconds.
* @return	0
*
* @note		Instruction cache should be enabled for this to work.
*
******************************************************************************/
void usleep_MB(ULONG useconds)
{
#if defined (XSLEEP_TIMER_IS_DEFAULT_TIMER) || defined (FREERTOS_BSP)
	sleep_common((u32)useconds, ITERS_PER_USEC);
#elif defined (XSLEEP_TIMER_IS_AXI_TIMER)
	/* Start Axi timer */
	XTime_StartAxiTimer();
	Xil_SleepAxiTimer(useconds, COUNTS_PER_USECOND);
#endif

}

/*****************************************************************************/
/**
* @brief    Provides delay for requested duration.
* @param	seconds- time in useconds.
* @return	0
*
* @note		Instruction cache should be enabled for this to work.
*
******************************************************************************/
void sleep_MB(u32 seconds)
{
#if defined (XSLEEP_TIMER_IS_DEFAULT_TIMER) || defined (FREERTOS_BSP)
	 sleep_common(seconds, ITERS_PER_SEC);
#elif defined (XSLEEP_TIMER_IS_AXI_TIMER)
	/* Start Axi timer */
	XTime_StartAxiTimer();
	Xil_SleepAxiTimer(seconds, COUNTS_PER_SECOND);
#endif

}

/*****************************************************************************/
/**
*
* @brief    Provides delay for requested duration..
*
* @param	MilliSeconds- Delay time in milliseconds.
*
* @return	None.
*
* @note		Instruction cache should be enabled for this to work.
*
******************************************************************************/
void MB_Sleep(u32 MilliSeconds)
{
#if defined (XSLEEP_TIMER_IS_DEFAULT_TIMER) || defined (FREERTOS_BSP)
	sleep_common(MilliSeconds, ITERS_PER_MSEC);

#elif defined (XSLEEP_TIMER_IS_AXI_TIMER)
	/* Start Axi timer */
	XTime_StartAxiTimer();
	Xil_SleepAxiTimer(MilliSeconds, COUNTS_PER_MSECOND);
#endif
}

/*****************************************************************************/
/**
 *
 * @brief    Provides delay for requested duration by using Axi Timer.
 *
 * @param	 delay - delay time in seconds/milli seconds/micro seconds.
 *           frequency - Number of counts per
 *                       second/milli second/micro second.
 *
 * @return	 None.
 *
 *
  ******************************************************************************/
#if defined (XSLEEP_TIMER_IS_AXI_TIMER)
static void Xil_SleepAxiTimer(u32 delay, u64 frequency)
{
	u64 tEnd = 0U;
	u64 tCur = 0U;
	u32 TimeHighVal = 0U;
	u32 TimeLowVal1 = 0U;
	u32 TimeLowVal2 = 0U;

	TimeLowVal1 = Xil_In32((SLEEP_TIMER_BASEADDR) +
			(XSLEEP_TIMER_AXITIMER_TCR_OFFSET));
	tEnd = (u64)TimeLowVal1 + ((u64)(delay) * frequency);
	do
	{
		TimeLowVal2 = Xil_In32((SLEEP_TIMER_BASEADDR) +
				(XSLEEP_TIMER_AXITIMER_TCR_OFFSET));
		if (TimeLowVal2 < TimeLowVal1) {
			TimeHighVal++;
		}
		TimeLowVal1 = TimeLowVal2;
		tCur = (((u64) TimeHighVal) << 32U) | (u64)TimeLowVal2;
	}while (tCur < tEnd);
}

/*****************************************************************************/
/**
 *
 * @brief   This API starts the Axi Timer only when the timer is not enabled.
 *
 * @param	None.
 *
 * @return	None.
 *
 * @note    Instruction cache should be enabled for this to work.
 *
  ******************************************************************************/
  static void XTime_StartAxiTimer(void)
  {
	u32 ControlStatusReg;

	/*  Checking if the timer is enabled  */
	if(Xil_In32(SLEEP_TIMER_BASEADDR + XSLEEP_TIMER_AXITIMER_TCSR0_OFFSET) &&
	                                   XSLEEP_TIMER_AXITIMER_CSR_ENABLE_TMR_MASK)
	{
		return;
	}
	/*
	 * Read the current register contents such that only the necessary bits
	 * of the register are modified in the following operations
	 */
	ControlStatusReg = Xil_In32(SLEEP_TIMER_BASEADDR +
			                          XSLEEP_TIMER_AXITIMER_TCSR0_OFFSET);
	/*
	 * Remove the reset condition such that the timer counter starts running
	 * with the value loaded from the compare register
	 */
	Xil_Out32((SLEEP_TIMER_BASEADDR + XSLEEP_TIMER_AXITIMER_TCSR0_OFFSET),
			(ControlStatusReg | XSLEEP_TIMER_AXITIMER_CSR_ENABLE_TMR_MASK |
				       XSLEEP_TIMER_AXITIMER_CSR_AUTO_RELOAD_MASK));
  }
#endif
