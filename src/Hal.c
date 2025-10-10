/**@file
 * This file is part of the TASTE SAMV71 RTEMS Runtime.
 *
 * @copyright 2025 N7 Space Sp. z o.o.
 *
 * Licensed under the ESA Public License (ESA-PL) Permissive (Type 3),
 * Version 2.4 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://essr.esa.int/license/list
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Hal.h"

#include <string.h>
#include <assert.h>

#include <Nvic/Nvic.h>
#include <Pmc/Pmc.h>
#include <Tic/Tic.h>
#include <Utils/ConcurrentAccessFlag.h>
#include <interfaces_info.h>
#include <rtems.h>

#include "Xdmac/xdmad.h"
#include <Nvic/Nvic.h>
#include <Pio/Pio.h>
#include <Scb/Scb.h>
#include <Uart/Uart.h>
#include <Utils/ErrorCode.h>
#include <Wdt/Wdt.h>

#define NANOSECOND_IN_SECOND 1000000000.0
#define MEGA_HZ 1000000u
#define TICKS_PER_RELOAD 65535ul
#define CLOCK_SELECTION_PRESCALLER 8.0

#ifndef MAIN_CRYSTAL_OSCILLATOR_FREQUNECY
#define MAIN_CRYSTAL_OSCILLATOR_FREQUNECY (12 * MEGA_HZ)
#endif

/* rtems_id xdmad_lock; */

static uint32_t created_semaphores_count = 0;
static rtems_id hal_semaphore_ids[RT_MAX_HAL_SEMAPHORES];

static ConcurrentAccessFlag reloads_modified_flag;
static uint32_t reloads_counter;
Pmc pmc;
static Tic tic = {};

static uint64_t mck_frequency;

rtems_name generate_new_hal_semaphore_name();

rtems_name generate_new_hal_semaphore_name()
{
	static rtems_name name = rtems_build_name('H', 0, 0, 0);
	return name++;
}

inline static void Init_setup_watchdog(void)
{
	const Wdt_Config wdtConfig = {
		.counterValue = 0x0FFF,
		.deltaValue = 0x0FFF,
		.isResetEnabled = false,
		.isFaultInterruptEnabled = false,
		.isDisabled = true,
		.isHaltedOnIdle = false,
		.isHaltedOnDebug = false,
	};

	Wdt wdt;
	Wdt_init(&wdt);
	Wdt_setConfig(&wdt, &wdtConfig);
}

void timer_irq_handler()
{
	__atomic_fetch_add(&reloads_counter, 1u, __ATOMIC_SEQ_CST);
	ConcurrentAccessFlag_set(&reloads_modified_flag);

	Tic_ChannelStatus status;
	Tic_getChannelStatus(&tic, Tic_Channel_0, &status);
}

void extract_main_oscilator_frequency()
{
	Pmc_MainckConfig main_clock_config;
	Pmc_getMainckConfig(&pmc, &main_clock_config);

	if (main_clock_config.src == Pmc_MainckSrc_XOsc) {
		mck_frequency = MAIN_CRYSTAL_OSCILLATOR_FREQUNECY;
		return;
	}

	switch (main_clock_config.rcOscFreq) {
	case Pmc_RcOscFreq_4M: {
		mck_frequency = 4 * MEGA_HZ;
		break;
	}
	case Pmc_RcOscFreq_8M: {
		mck_frequency = 8 * MEGA_HZ;
		break;
	}
#if defined(N7S_TARGET_SAMV71Q21)
	case Pmc_RcOscFreq_12M: {
		mck_frequency = 12 * MEGA_HZ;
		break;
	}
#elif defined(N7S_TARGET_SAMRH71F20) || defined(N7S_TARGET_SAMRH707F18)
	case Pmc_RcOscFreq_10M: {
		mck_frequency = 10 * MEGA_HZ;
		break;
	}
	case Pmc_RcOscFreq_12M: {
		mck_frequency = 12 * MEGA_HZ;
		break;
	}
#endif
	}
}

void apply_plla_config(Pmc_MasterckConfig *master_clock_config)
{
	if (master_clock_config->src == Pmc_MasterckSrc_Pllack) {
		Pmc_PllConfig pll_config;
		Pmc_getPllConfig(&pmc, &pll_config);
		if (pll_config.pllaDiv > 0 && pll_config.pllaMul > 0) {
			mck_frequency = (mck_frequency / pll_config.pllaDiv) *
					(pll_config.pllaMul + 1);
		} else if (pll_config.pllaDiv == 0 && pll_config.pllaMul > 0) {
			mck_frequency =
				mck_frequency * (pll_config.pllaMul + 1);
		} else if (pll_config.pllaDiv > 0 && pll_config.pllaMul == 0) {
			mck_frequency = mck_frequency / pll_config.pllaDiv;
		}
	}
}

void extract_mck_frequency()
{
	Pmc_MasterckConfig master_clock_config;
	Pmc_getMasterckConfig(&pmc, &master_clock_config);

	extract_main_oscilator_frequency();
	apply_plla_config(&master_clock_config);

	switch (master_clock_config.presc) {
	case Pmc_MasterckPresc_1: {
		break;
	}
	case Pmc_MasterckPresc_2: {
		mck_frequency = mck_frequency / 2;
		break;
	}
	case Pmc_MasterckPresc_4: {
		mck_frequency = mck_frequency / 4;
		break;
	}
	case Pmc_MasterckPresc_8: {
		mck_frequency = mck_frequency / 8;
		break;
	}
	case Pmc_MasterckPresc_16: {
		mck_frequency = mck_frequency / 16;
		break;
	}
	case Pmc_MasterckPresc_32: {
		mck_frequency = mck_frequency / 32;
		break;
	}
	case Pmc_MasterckPresc_64: {
		mck_frequency = mck_frequency / 64;
		break;
	}
#if defined(N7S_TARGET_SAMV71Q21)
	case Pmc_MasterckPresc_3: {
		mck_frequency = mck_frequency / 7;
		break;
	}
#endif
	}

	switch (master_clock_config.divider) {
	case Pmc_MasterckDiv_1: {
		break;
	}
	case Pmc_MasterckDiv_2: {
		mck_frequency = mck_frequency / 2;
		break;
	}
	}
}

bool Hal_Init(void)
{
	reloads_counter = 0u;

	// nvic cannot be used for registration of interrupt handlers:
	// instead, the rtems api shall be used
	/* rtems_interrupt_handler_install(58, "xdmac", RTEMS_INTERRUPT_UNIQUE, */
	/* 				(rtems_interrupt_handler)&XDMAC_Handler, */
	/* 				0); */
	/* rtems_interrupt_handler_install(7, "uart0", RTEMS_INTERRUPT_UNIQUE, */
	/* 				(rtems_interrupt_handler)&UART0_Handler, */
	/* 				0); */
	/* rtems_interrupt_vector_enable(7); */
	/* rtems_interrupt_handler_install(8, "uart1", RTEMS_INTERRUPT_UNIQUE, */
	/* 				(rtems_interrupt_handler)&UART1_Handler, */
	/* 				0); */
	/* rtems_interrupt_vector_enable(8); */
	/* rtems_interrupt_handler_install(44, "uart2", RTEMS_INTERRUPT_UNIQUE, */
	/* 				(rtems_interrupt_handler)&UART2_Handler, */
	/* 				0); */
	/* rtems_interrupt_vector_enable(44); */
	/* rtems_interrupt_handler_install(45, "uart3", RTEMS_INTERRUPT_UNIQUE, */
	/* 				(rtems_interrupt_handler)&UART3_Handler, */
	/* 				0); */
	/* rtems_interrupt_vector_enable(45); */
	/* rtems_interrupt_handler_install(46, "uart4", RTEMS_INTERRUPT_UNIQUE, */
	/* 				(rtems_interrupt_handler)&UART4_Handler, */
	/* 				0); */
	/* rtems_interrupt_vector_enable(46); */
	rtems_interrupt_handler_install(23, "timer0", RTEMS_INTERRUPT_UNIQUE,
					timer_irq_handler, 0);

	Init_setup_watchdog();
	Pmc_init(&pmc, Pmc_getDeviceRegisterStartAddress());
	Pmc_enablePeripheralClk(&pmc, Pmc_PeripheralId_Tc0Ch0);

	extract_mck_frequency();

	Tic_init(&tic, Tic_Id_0);
	Tic_writeProtect(&tic, false);

	Tic_ChannelConfig config = {};
	config.isEnabled = true;
	config.clockSource = Tic_ClockSelection_MckBy8;
	config.irqConfig.isCounterOverflowIrqEnabled = true;
	Tic_setChannelConfig(&tic, Tic_Channel_0, &config);

	Tic_enableChannel(&tic, Tic_Channel_0);
	Tic_triggerChannel(&tic, Tic_Channel_0);

	Init_setup_xdmad_lock();

	return true;
}

uint64_t Hal_GetElapsedTimeInNs(void)
{
	uint32_t reloads;
	uint32_t ticks;

	do {
		ConcurrentAccessFlag_reset(&reloads_modified_flag);
		reloads = __atomic_load_n(&reloads_counter, __ATOMIC_SEQ_CST);
		ticks = Tic_getCounterValue(&tic, Tic_Channel_0);
	} while (ConcurrentAccessFlag_check(&reloads_modified_flag));

	const uint64_t total_ticks =
		(uint64_t)(reloads * TICKS_PER_RELOAD) + (uint64_t)ticks;
	const double clock_frequency =
		(double)mck_frequency / CLOCK_SELECTION_PRESCALLER;

	return (uint64_t)((double)total_ticks /
			  (clock_frequency / NANOSECOND_IN_SECOND));
}

bool Hal_SleepNs(uint64_t time_ns)
{
	const double sleep_tick_count =
		time_ns * ((double)mck_frequency / NANOSECOND_IN_SECOND);

	return rtems_task_wake_after((rtems_interval)sleep_tick_count) ==
	       RTEMS_SUCCESSFUL;
}

int32_t Hal_SemaphoreCreate(void)
{
	if (created_semaphores_count >= RT_MAX_HAL_SEMAPHORES) {
		return 0;
	}

	const rtems_status_code status_code = rtems_semaphore_create(
		generate_new_hal_semaphore_name(),
		1, // Initial value, unlocked
		RTEMS_BINARY_SEMAPHORE,
		0, // Priority ceiling
		&hal_semaphore_ids[created_semaphores_count]);

	if (status_code == RTEMS_SUCCESSFUL) {
		return hal_semaphore_ids[created_semaphores_count++];
	}

	return 0;
}

bool Hal_SemaphoreObtain(int32_t id)
{
	return rtems_semaphore_obtain(id, RTEMS_WAIT, RTEMS_NO_TIMEOUT) ==
	       RTEMS_SUCCESSFUL;
}

bool Hal_SemaphoreRelease(int32_t id)
{
	return rtems_semaphore_release(id) == RTEMS_SUCCESSFUL;
}
