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
#include "HalInternal.h"

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
#if defined(N7S_TARGET_SAMV71Q21)
#include <SamV71Core.h>
#elif defined(N7S_TARGET_SAMRH71F20)
#include <SamRH71Core.h>
#else
#error "one of N7S_TARGET_SAMV71Q21, N7S_TARGET_SAMRH71F20 shall be defined"
#endif

#define NANOSECOND_IN_SECOND 1000000000ULL
#define TICKS_PER_RELOAD 65535ul
#define CLOCK_SELECTION_PRESCALLER 8U

static uint32_t created_semaphores_count = 0;
static rtems_id hal_semaphore_ids[RT_MAX_HAL_SEMAPHORES];

static ConcurrentAccessFlag reloads_modified_flag;
static volatile uint32_t reloads_counter;
static Tic tic;
static bool idleTaskIsWatchdogEnabled = false;

static uint64_t main_clock_frequency;
static uint64_t ns_per_tick_q32;
// Pre-split halves to speed-up computation
static uint64_t ns_per_tick_q32_hi;
static uint64_t ns_per_tick_q32_lo;

void Hal_SetReloadsCounter(const uint32_t value)
{
	reloads_counter = value;
}

void Hal_SetNsPerTickQ32(const uint64_t value)
{
	ns_per_tick_q32 = value;
	ns_per_tick_q32_hi = value >> 32;
	ns_per_tick_q32_lo = value & 0xFFFFFFFFU;
}

rtems_name generate_new_hal_semaphore_name()
{
	static rtems_name name = rtems_build_name('H', 0, 0, 0);
	return name++;
}

static Wdt wdt;

inline static void Init_setup_watchdog(void)
{
	Wdt_init(&wdt);
#ifdef RT_RTOS_NO_INIT

	Wdt_Config existingWdtConfig;
	Wdt_getConfig(&wdt, &existingWdtConfig);
	if (!existingWdtConfig.isDisabled) {
		idleTaskIsWatchdogEnabled = true;
	}
#ifdef RT_RTOS_USE_WATCHDOG
#error RT_RTOS_USE_WATCHDOG cannot be used when RT_RTOS_NO_INIT is defined
#endif
#else
#ifdef RT_RTOS_USE_WATCHDOG
	const Wdt_Config wdtConfig = {
		.counterValue = 0x0FFF,
		.deltaValue = 0x0FFF,
		.isResetEnabled = true,
		.isFaultInterruptEnabled = false,
		.isDisabled = false,
		.isHaltedOnIdle = false,
		.isHaltedOnDebug = true,
	};
#else
	const Wdt_Config wdtConfig = {
		.counterValue = 0x0FFF,
		.deltaValue = 0x0FFF,
		.isResetEnabled = false,
		.isFaultInterruptEnabled = false,
		.isDisabled = true,
		.isHaltedOnIdle = false,
		.isHaltedOnDebug = true,
	};
#endif
	Wdt_setConfig(&wdt, &wdtConfig);
	Wdt_Config existingWdtConfig;
	Wdt_getConfig(&wdt, &existingWdtConfig);
	assert((existingWdtConfig.isDisabled == wdtConfig.isDisabled) &&
	       "Unable to setup watchdog");
	assert((existingWdtConfig.isResetEnabled == wdtConfig.isResetEnabled) &&
	       "Unable to setup watchdog");
#endif
}

void Hal_ResetWatchdog()
{
	Wdt_reset(&wdt);
}

void timer_irq_handler()
{
	// Count elapsed timer reloads and mark the snapshot as dirty for readers.
	__atomic_fetch_add(&reloads_counter, 1u, __ATOMIC_RELAXED);
	ConcurrentAccessFlag_set(&reloads_modified_flag);

	Tic_ChannelStatus status;
	Tic_getChannelStatus(&tic, Tic_Channel_0, &status);
}

static void Hal_InitTimer(void)
{
	// Configure the periodic timer that backs elapsed-time and sleep services.
	reloads_counter = 0u;
#if defined(N7S_TARGET_SAMV71Q21)
	SamV71Core_EnablePeripheralClock(Pmc_PeripheralId_Tc0Ch0);
#elif defined(N7S_TARGET_SAMRH71F20)
	SamRH71Core_EnablePeripheralClock(Pmc_PeripheralId_Tc0Ch0);
#endif

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

	Tic_ChannelConfig config = {
		.isEnabled = true,
		.clockSource = Tic_ClockSelection_MckBy8,
		.irqConfig.isCounterOverflowIrqEnabled = true,
		.rc = 65535u,
	};
	Tic_setChannelConfig(&tic, Tic_Channel_0, &config);

	Tic_triggerChannel(&tic, Tic_Channel_0);
}

bool Hal_Init(void)
{
	Init_setup_watchdog();
#if defined(N7S_TARGET_SAMV71Q21)
	SamV71Core_Init();
#elif defined(N7S_TARGET_SAMRH71F20)
	SamRH71Core_Init();
#endif

	Hal_InitTimer();

#if defined(N7S_TARGET_SAMV71Q21)
	main_clock_frequency = SamV71Core_GetMainClockFrequency();
#elif defined(N7S_TARGET_SAMRH71F20)
	main_clock_frequency = SamRH71Core_GetMainClockFrequency();
#endif
	const uint64_t prescaled_clock_frequency =
		main_clock_frequency / CLOCK_SELECTION_PRESCALLER;
	// ns_per_tick = 1e9 / prescaled_clock_frequency in Q32.32 fixed-point
	ns_per_tick_q32 =
		(NANOSECOND_IN_SECOND << 32) / prescaled_clock_frequency;
	ns_per_tick_q32_hi = ns_per_tick_q32 >> 32;
	ns_per_tick_q32_lo = ns_per_tick_q32 & 0xFFFFFFFFU;

	return true;
}

uint64_t Hal_GetElapsedTimeInNs(void)
{
	// Read reload and timer state until a consistent snapshot is observed.
	uint32_t reloads;
	uint32_t ticks;

	do {
		ConcurrentAccessFlag_reset(&reloads_modified_flag);
		reloads = __atomic_load_n(&reloads_counter, __ATOMIC_RELAXED);
		ticks = Tic_getCounterValue(&tic, Tic_Channel_0);
	} while (ConcurrentAccessFlag_check(&reloads_modified_flag));

	const uint64_t total_ticks =
		(uint64_t)reloads * TICKS_PER_RELOAD + ticks;

	const uint64_t ticks_high = total_ticks >> 32;
	const uint64_t ticks_low = total_ticks & 0xFFFFFFFFU;

	const uint64_t ns_high = ticks_high * ns_per_tick_q32;
	const uint64_t ns_low = (ticks_low * ns_per_tick_q32_hi) +
				((ticks_low * ns_per_tick_q32_lo) >> 32);

	return ns_high + ns_low;
}

bool Hal_SleepNs(uint64_t time_ns)
{
	// Convert the requested duration to RTEMS ticks and sleep for that span.
	const double sleep_tick_count =
		time_ns * ((double)main_clock_frequency / NANOSECOND_IN_SECOND);

	return rtems_task_wake_after((rtems_interval)sleep_tick_count) ==
	       RTEMS_SUCCESSFUL;
}

uint32_t Hal_SemaphoreCreate(void)
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

bool Hal_SemaphoreObtain(uint32_t id)
{
	if (created_semaphores_count >= RT_MAX_HAL_SEMAPHORES) {
		return false;
	}

	const rtems_status_code result =
		rtems_semaphore_obtain(id, RTEMS_WAIT, RTEMS_NO_TIMEOUT);
	return result == RTEMS_SUCCESSFUL;
}

bool Hal_SemaphoreRelease(uint32_t id)
{
	if (created_semaphores_count >= RT_MAX_HAL_SEMAPHORES) {
		return false;
	}

	const rtems_status_code result = rtems_semaphore_release(id);
	return result == RTEMS_SUCCESSFUL;
}

void *Hal_IdleTask(uintptr_t ignored)
{
	(void)ignored;
	while (1) {
		if (idleTaskIsWatchdogEnabled) {
			Hal_ResetWatchdog();
		}
	}

	return NULL;
}

enum Reset_Reason Hal_GetResetReason()
{
	return BootHelper_GetResetReason();
}
