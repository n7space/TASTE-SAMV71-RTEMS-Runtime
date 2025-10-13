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

#ifndef HAL_H
#define HAL_H

/**
 * @file    Hal.h
 * @brief   Header for Hal
 */

#include <stdbool.h>
#include <stdint.h>

#include <rtems.h>

#include <Nvic/Nvic.h>
#include <Uart/Uart.h>
#include <Pmc/PmcPeripheralId.h>

#ifndef RT_MAX_HAL_SEMAPHORES
#define RT_MAX_HAL_SEMAPHORES 8
#endif

/// \brief A function serving as a callback called upon an interrupt if the
/// interrupt
///          was subscribed to.
typedef void(InterruptCallback)(void *);

/// \brief Subscribes to interrupt and sets function that is called upon the
/// interrupt
///         reception
/// \param [in] irq Numeric identifier of the interrupt to subscribe to
/// \param [in] callback Callback function pointer
void Hal_subscribe_to_interrupt(Nvic_Irq irq, InterruptCallback callback);

/**
 * @brief               Initializes the Hal module.
 *
 * @return              Bool indicating whether the initialization was
 *                      successful
 */
bool Hal_Init(void);

/**
 * @brief               Returns time elapsed from the initialization of the
 *                      runtime
 *
 * @return              Time elapsed from the initialization of the runtime
 */
uint64_t Hal_GetElapsedTimeInNs(void);

/**
 * @brief               Suspends the current thread for the given amount of time
 *
 * @param[in] time_ns   time in nanoseconds
 *
 * @return              Bool indicating whether the sleep was successful
 */
bool Hal_SleepNs(uint64_t time_ns);

/**
 * @brief               Creates an RTOS backed semaphore. This function is not
 *                      thread safe, but it is assumed to be used only during
 *                      system initialization, from a single thread/Init task.
 *
 * @return              ID of the created semaphore
 */
rtems_id Hal_SemaphoreCreate(void);

/**
 * @brief               Obtains the indicated semaphore, suspending the
 *                      execution of the current thread if necessary.
 *
 * @param[in] id        id of the semaphore
 *
 * @return              Bool indicating whether the obtain was successful
 */
bool Hal_SemaphoreObtain(int32_t id);

/**
 * @brief               Releases the indicated semaphore, potentially resuming
 *                      threads waiting on the semaphore
 *
 * @param[in] id        id of the semaphore
 *
 * @return              Bool indicating whether the release was successful
 */
bool Hal_SemaphoreRelease(int32_t id);

/**
 * @brief               Enable peripheral clock.
 *
 * @param[in] peripheralId clock identifier.
 */
void Hal_EnablePeripheralClock(const Pmc_PeripheralId peripheralId);

/**
 * @brief               Get frequency of main clock.
 *
 * @return              Main clock frequency in Hz.
 */
uint64_t Hal_GetMainClockFrequency();

/**
 * @brief               Subscribe to interrupt.
 *
 * @param[in] vector    Number of interrupt.
 * @param[in] info      Short description of interrupt handler.
 * @param[in] handler   The function to handle interrupt.
 * @param[in] handler_arg A parameter which shall be passed when calling handler.
  */
void Hal_InterruptSubscribe(const rtems_vector_number vector, const char *info,
			    rtems_interrupt_handler handler, void *handler_arg);

#endif
