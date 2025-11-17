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

#include <Nvic/Nvic.h>
#include <Pio/Pio.h>
#include <Scb/Scb.h>
#include <Uart/Uart.h>
#include <Utils/ErrorCode.h>
#include <Wdt/Wdt.h>
#include <SamV71Core.h>

#define NANOSECOND_IN_SECOND 1000000000.0
#define TICKS_PER_RELOAD 65535ul
#define CLOCK_SELECTION_PRESCALLER 8.0

static uint32_t created_semaphores_count = 0;
static rtems_id hal_semaphore_ids[RT_MAX_HAL_SEMAPHORES];

static ConcurrentAccessFlag reloads_modified_flag;
static uint32_t reloads_counter;
static Tic tic = {};

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

static void Hal_InitTimer(void)
{
	reloads_counter = 0u;
	SamV71Core_EnablePeripheralClock(Pmc_PeripheralId_Tc0Ch0);

	// NVIC cannot be used for registration of interrupt handlers
	// instead, the RTEMS API shall be used: the interrupt vector table is managed by RTEMS,
	// calling NVIC here will overwrite RTEMS interrupt dispatch function by custom function
	// what may cause unforeseen consequences.
	// However, using NVIC interrupt names is ok.
	rtems_interrupt_handler_install(Nvic_Irq_Timer0_Channel0, "timer0",
					RTEMS_INTERRUPT_UNIQUE,
					timer_irq_handler, 0);
	rtems_interrupt_vector_enable(Nvic_Irq_Timer0_Channel0);
	Tic_init(&tic, Tic_Id_0);
	Tic_writeProtect(&tic, false);

	Tic_ChannelConfig config = {};
	config.isEnabled = true;
	config.clockSource = Tic_ClockSelection_MckBy8;
	config.irqConfig.isCounterOverflowIrqEnabled = true;
	Tic_setChannelConfig(&tic, Tic_Channel_0, &config);

	Tic_enableChannel(&tic, Tic_Channel_0);
	Tic_triggerChannel(&tic, Tic_Channel_0);
}

bool Hal_Init(void)
{
	Init_setup_watchdog();
	SamV71Core_Init();
	Hal_InitTimer();

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
		(double)SamV71Core_GetMainClockFrequency() /
		CLOCK_SELECTION_PRESCALLER;

	return (uint64_t)((double)total_ticks /
			  (clock_frequency / NANOSECOND_IN_SECOND));
}

bool Hal_SleepNs(uint64_t time_ns)
{
	const double sleep_tick_count =
		time_ns * ((double)SamV71Core_GetMainClockFrequency() /
			   NANOSECOND_IN_SECOND);

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
	if (created_semaphores_count >= RT_MAX_HAL_SEMAPHORES) {
		return false;
	}

	const rtems_status_code result =
		rtems_semaphore_obtain(id, RTEMS_WAIT, RTEMS_NO_TIMEOUT);
	return result == RTEMS_SUCCESSFUL;
}

bool Hal_SemaphoreRelease(int32_t id)
{
	if (created_semaphores_count >= RT_MAX_HAL_SEMAPHORES) {
		return false;
	}

	const rtems_status_code result = rtems_semaphore_release(id);
	return result == RTEMS_SUCCESSFUL;
}

enum BootHelper_ResetReason Hal_GetResetReason()
{
	return BootHelper_GetResetReason();
}
