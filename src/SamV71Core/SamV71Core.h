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

#ifndef SAMV71_CORE_H
#define SAMV71_CORE_H

/**
 * @file    SamV71Core.h
 * @brief   Core functions for Samv71 MCU.
 */

#include <stdint.h>
#include <stdbool.h>
#include <rtems.h>

#include <Pmc/PmcPeripheralId.h>
#include <Pmc/Pmc.h>
#include <Utils/ErrorCode.h>

/**
 * @brief               Initialize SAMV71 Core module.
 */
void SamV71Core_Init(void);

/**
 * @brief               Subscribe to interrupt.
 *
 * @param[in] vector    Number of interrupt.
 * @param[in] info      Short description of interrupt handler.
 * @param[in] handler   The function to handle interrupt.
 * @param[in] handler_arg A parameter which shall be passed when calling handler.
  */
void SamV71Core_InterruptSubscribe(const rtems_vector_number vector,
				   const char *info,
				   rtems_interrupt_handler handler,
				   void *handler_arg);

/**
 * @brief               Enable peripheral clock.
 *
 * @param[in] peripheralId clock identifier.
 */
void SamV71Core_EnablePeripheralClock(const Pmc_PeripheralId peripheralId);

/**
 * @brief               Get frequency of main clock.
 *
 * @return              Main clock frequency in Hz.
 */
uint64_t SamV71Core_GetMainClockFrequency(void);

/**
 * @brief               Generate new unique name for semaphore.
 *
 * @return              Unique name for semaphore.
 */
rtems_name SamV71Core_GenerateNewSemaphoreName(void);

/**
 * @brief               Generate new unique name for task
 *
 * @return              Unique name for task.
 */
rtems_name SamV71Core_GenerateNewTaskName(void);

/**
 * @brief               Set configuration of PCKx.
 *
 * @param[in] id        PCK identifier.
 * @param[in] config    PCK configuration to set.
 * @param[in] timeout   Timeout of operation.
 * @param[out] errCode  ErrorCode pointer to write optional error code.

 * @return              Boolean value indicating operation success.
 */
bool SamV71Core_SetPckConfig(const Pmc_PckId id,
			     const Pmc_PckConfig *const config,
			     const uint32_t timeout, ErrorCode *const errCode);

/**
 * @brief               Disable data cache for given memory region.
 *
 *                      This function is not thread-safe.
 *
 * @param[in] address   Region address.
 * @param[in] sizeExponent    Establishes size of the region as 2**(sizeExponent + 1)
 *                      Only values from 4 upto 31 are valid, where 4 means 32 bytes and 31 means 4GB.
 */
void SamV71Core_DisableDataCacheInRegion(void *address, size_t sizeExponent);

#endif
