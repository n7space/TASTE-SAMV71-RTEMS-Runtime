/**@file
 * This file is part of the TASTE SAMRH71 RTEMS Runtime.
 *
 * @copyright 2026 N7 Space Sp. z o.o.
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

#ifndef SAMRH71_CORE_H
#define SAMRH71_CORE_H

/**
 * @file    SamRH71Core.h
 * @brief   Core functions for the SAMRH71 MCU.
 */

#include <stdint.h>
#include <stdbool.h>
#include <rtems.h>

#include <Pmc/PmcPeripheralId.h>
#include <Pmc/Pmc.h>
#include <Utils/ErrorCode.h>

/**
 * @brief Initialize the SAMRH71 core helper module.
 */
void SamRH71Core_Init(void);

/**
 * @brief               Subscribe to interrupt.
 *
 * @param[in] vector    Number of interrupt.
 * @param[in] info      Short description of interrupt handler.
 * @param[in] handler   The function to handle interrupt.
 * @param[in] handler_arg A parameter which shall be passed when calling handler.
  */
void SamRH71Core_InterruptSubscribe(const rtems_vector_number vector,
				    const char *info,
				    rtems_interrupt_handler handler,
				    void *handler_arg);

/**
 * @brief Enable the clock for a peripheral.
 *
 * @param[in] peripheralId Peripheral identifier.
 */
void SamRH71Core_EnablePeripheralClock(const Pmc_PeripheralId peripheralId);

/**
 * @brief Get the main clock frequency in hertz.
 *
 * @return Main clock frequency in Hz.
 */
uint64_t SamRH71Core_GetMainClockFrequency(void);

/**
 * @brief               Generate a new unique name for semaphore.
 *
 * @return Unique RTEMS semaphore name.
 */
rtems_name SamRH71Core_GenerateNewSemaphoreName(void);

/**
 * @brief               Generate a new unique name for task
 *
 * @return Unique RTEMS task name.
 */
rtems_name SamRH71Core_GenerateNewTaskName(void);

/**
 * @brief Configure a programmable clock (PCKx) output.
 *
 * @param[in] id        PCK identifier.
 * @param[in] config    Configuration to apply.
 * @param[in] timeout   Timeout for the operation.
 * @param[out] errCode  Optional error code output.
 *
 * @return True on success.
 */
bool SamRH71Core_SetPckConfig(const Pmc_PckId id,
			      const Pmc_PckConfig *const config,
			      const uint32_t timeout, ErrorCode *const errCode);

/**
 * @brief Disable the data cache for a memory region.
 *
 * This function is not thread-safe.
 *
 * @param[in] address   Region address.
 * @param[in] sizeExponent    Establishes size of the region as 2**(sizeExponent + 1)
 *                      Only values from 4 upto 31 are valid, where 4 means 32 bytes and 31 means 4GB.
 */
void SamRH71Core_DisableDataCacheInRegion(void *address, size_t sizeExponent);

#endif
