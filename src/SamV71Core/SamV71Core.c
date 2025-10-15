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

#include "SamV71Core.h"

#include <stdint.h>
#include <Pmc/Pmc.h>

extern Pmc pmc;
extern uint64_t mck_frequency;

void Hal_EnablePeripheralClock(const Pmc_PeripheralId peripheralId)
{
	Pmc_enablePeripheralClk(&pmc, peripheralId);
}

uint64_t Hal_GetMainClockFrequency()
{
	return mck_frequency;
}

void Hal_InterruptSubscribe(const rtems_vector_number vector, const char *info,
			    rtems_interrupt_handler handler, void *handler_arg)
{
	rtems_interrupt_handler_install(vector, info, RTEMS_INTERRUPT_UNIQUE,
					handler, handler_arg);
	rtems_interrupt_vector_enable(vector);
}
