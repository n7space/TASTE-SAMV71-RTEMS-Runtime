/**@file
 * This file is part of the TASTE Leon3 Runtime.
 *
 * @copyright 2025 N7 Space Sp. z o.o.
 *
 * Leon3 Runtime is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with Leon3 Runtime. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HAL_H
#define HAL_H

/**
 * @file    Hal.h
 * @brief   Header for Hal
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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
int32_t Hal_SemaphoreCreate(void);

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

#endif
