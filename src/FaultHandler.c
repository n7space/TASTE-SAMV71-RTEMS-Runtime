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

#include <FaultHandler.h>
#include <SamV71Core/SamV71Core.h>

static void hard_fault_exception_handler(void *arg)
{
	// Generate death report
	// Exit?
}

static void memory_management_exception_handler(void *arg)
{
	// Generate death report
	// Exit?
}

static void bus_fault_exception_handler(void *arg)
{
	// Generate death report
	// Exit?
}

static void usage_fault_exception_handler(void *arg)
{
	// Generate death report
	// Exit?
}

static void ecc_fault_exception_handler(void *arg)
{
	// Generate death report
	// Exit?
}

bool FaultHandler_Init()
{
	SamV71Core_InterruptSubscribe(Nvic_Irq_HardFault, "Hard fault exception handler", hard_fault_exception_handler, NULL);
	SamV71Core_InterruptSubscribe(Nvic_Irq_MemoryManagement, "Memory management exception handler", memory_management_exception_handler, NULL);
	SamV71Core_InterruptSubscribe(Nvic_Irq_BusFault, "Bus fault exception handler", hard_fault_exception_handler, NULL);
	SamV71Core_InterruptSubscribe(Nvic_Irq_UsageFault, "Usage fault exception handler", usage_fault_exception_handler, NULL);
#if defined(N7S_TARGET_SAMV71Q21)
	SamV71Core_InterruptSubscribe(Nvic_Irq_CacheFault, "ECC fault exception handler", ecc_fault_exception_handler, NULL);
#elif defined(N7S_TARGET_SAMRH71F20)
	SamV71Core_InterruptSubscribe(Nvic_Irq_EccFault, "ECC fault exception handler", ecc_fault_exception_handler, NULL);
#else
#error Bad platform target, supported targets are N7S_TARGET_SAMV71Q21 and N7S_TARGET_SAMRH71F20
#endif

	return true;
}
