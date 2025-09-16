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

#include <Hal.h>

#include <interfaces_info.h>
#include <rtems.h>
#include "arm-bsp/src/Systick/Systick.h"
#include "arm-bsp/src/Utils/ConcurrentAccessFlag.h"

#ifndef RT_MAX_HAL_SEMAPHORES
#define RT_MAX_HAL_SEMAPHORES 8
#endif

#define NANOSECOND_IN_SECOND 1000000000u

#define TICKS_PER_RELOAD ((uint64_t)(HAL_CLOCK_SYSTICK_RELOAD + 1u))
#define TICKS_PER_SECOND (N7S_BSP_CORE_CLOCK)
#define TICKS_PER_NANOSECOND (TICKS_PER_SECOND / NANOSECOND_IN_SECOND)

static uint32_t created_semaphores_count = 0;
static rtems_id hal_semaphore_ids[RT_MAX_HAL_SEMAPHORES];

static ConcurrentAccessFlag reloadsModifiedFlag;
static uint32_t reloadsCounter;
static Systick systick;

rtems_name generate_new_hal_semaphore_name();

rtems_name generate_new_hal_semaphore_name()
{
	static rtems_name name = rtems_build_name('H', 0, 0, 0);
	return name++;
}

void SysTick_Handler(void)
{
    __atomic_fetch_add(&reloadsCounter, 1u, __ATOMIC_SEQ_CST);
  	ConcurrentAccessFlag_set(&reloadsModifiedFlag);
}

bool Hal_Init(void)
{
	reloadsCounter = 0u;

    Systick_init(&systick, Systick_getDeviceRegisterStartAddress());

    const Systick_Config config = {
    	.clockSource = Systick_ClockSource_ProcessorClock,
    	.isInterruptEnabled = true,
    	.isEnabled = true,
    	.reloadValue = HAL_CLOCK_SYSTICK_RELOAD,
    };

  	Systick_setConfig(&systick, &config);

	return true;
}

uint64_t Hal_GetElapsedTimeInNs(void)
{
	uint32_t reloads;
	uint32_t ticks;

	do
  	{
    	ConcurrentAccessFlag_reset(&reloadsModifiedFlag);
    	reloads = __atomic_load_n(&reloadsCounter, __ATOMIC_SEQ_CST);
    	ticks = HAL_CLOCK_SYSTICK_RELOAD - Systick_getCurrentValue(&systick);
  	} while (ConcurrentAccessFlag_check(&reloadsModifiedFlag));

	const uint64_t total_ticks = (uint64_t)(reloads * TICKS_PER_RELOAD) + (uint64_t)ticks;

  	return total_ticks / TICKS_PER_NANOSECOND;
}

bool Hal_SleepNs(uint64_t time_ns)
{
	const rtems_interval ticks_per_second =
	    rtems_clock_get_ticks_per_second();
	const double ticks_per_ns =
	    (double)ticks_per_second / (double)NANOSECOND_IN_SECOND;
	const double sleep_tick_count = time_ns * ticks_per_ns;

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
