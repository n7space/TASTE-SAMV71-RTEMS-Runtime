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

#ifndef THREADSCOMMON_H
#define THREADSCOMMON_H

/**
 * @file    ThreadsCommon.h
 * @brief   Header for ThreadsCommon
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define EMPTY_REQUEST_DATA_BUFFER_SIZE 8

/**
 * @brief   Struct representing empty request sent periodically to cyclic
 * interface
 */
struct CyclicInterfaceEmptyRequestData {
	uint32_t m_sender_pid;
	uint32_t m_length;
	uint8_t m_data[EMPTY_REQUEST_DATA_BUFFER_SIZE]
	    __attribute__((aligned(16)));
};

/**
 * @brief               Creates a timer that periodically sends a request to
 *                      the indicated queue. The request is empty.
 *
 * @param[in] interval_ns         cyclic interval period, expressed in
 * nanoseconds
 * @param[in] dispatch_offset_ns  cyclic interval dispatch offset, expressed in
 * nanoseconds
 * @param[in] queue_id            ID of the queue to send the request to
 * @param[in] request_size        size of the request data
 *
 * @return              Bool indicating whether the creation was
 *                      successful
 */
bool ThreadsCommon_CreateCyclicRequest(uint64_t interval_ns,
				       uint64_t dispatch_offset_ns,
				       uint32_t queue_id,
				       uint32_t request_size);

/**
 * @brief               Function is responsible for invoking the provided user
 *                      function with provided request data, and performing all
 *                      required logging and monitoring
 *
 * @param[in] request_data   pointer to request data
 * @param[in] request_size   size of the request data
 * @param[in] user_function  pointer to user function to execute
 * @param[in] thread_id      used for performance logging
 *
 * @return              Bool indicating whether the request processing was
 *                      successful
 */
bool ThreadsCommon_ProcessRequest(void *request_data, uint32_t request_size,
				  void *user_function, uint32_t thread_id);

#endif
