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
#include <ThreadsCommon.h>
#include <assert.h>
#include <interfaces_info.h>
#include <rtems.h>
#include <string.h>

#ifdef RUNTIME_CYCLIC_INTERFACE_COUNT
#define RT_MAX_CYCLIC_INTERFACES RUNTIME_CYCLIC_INTERFACE_COUNT
#else
#define RT_MAX_CYCLIC_INTERFACES RUNTIME_THREAD_COUNT
#endif

#define NANOSECOND_IN_MILISECOND 1000000ULL

extern rtems_name generate_new_partition_timer_name();
static void schedule_next_tick(const uint32_t cyclic_request_data_index);
static void timer_callback(rtems_id timer_id, void *cyclic_request_data_index);
static void update_execution_time_data(const uint32_t thread_id,
				       const uint64_t thread_execution_time);

typedef void (*call_function)(const char *buf, size_t len);

struct CyclicRequestData {
	rtems_id timer_id;
	rtems_interval next_wakeup_ticks;
	rtems_interval interval_ticks;
	uint32_t queue_id;
	uint32_t request_size;
};

static uint32_t cyclic_requests_count = 0;
static struct CyclicRequestData cyclic_request_data[RT_MAX_CYCLIC_INTERFACES];
static struct CyclicInterfaceEmptyRequestData empty_request;

static void schedule_next_tick(const uint32_t cyclic_request_data_index)
{
	const rtems_id timer_id =
		cyclic_request_data[cyclic_request_data_index].timer_id;

	cyclic_request_data[cyclic_request_data_index].next_wakeup_ticks +=
		cyclic_request_data[cyclic_request_data_index].interval_ticks;
	rtems_interval delta = cyclic_request_data[cyclic_request_data_index]
				       .next_wakeup_ticks -
			       rtems_clock_get_ticks_since_boot();
	rtems_timer_fire_after(timer_id, delta, timer_callback,
			       (void *)cyclic_request_data_index);
}

static void timer_callback(rtems_id timer_id, void *cyclic_request_data_index)
{
	uintptr_t index = (uintptr_t)cyclic_request_data_index;

	rtems_message_queue_send((rtems_id)cyclic_request_data[index].queue_id,
				 &empty_request,
				 cyclic_request_data[index].request_size);

	schedule_next_tick(index);
}

static void update_execution_time_data(const uint32_t thread_id,
				       const uint64_t thread_execution_time)
{
	if (thread_execution_time <
	    threads_info[thread_id].min_thread_execution_time) {
		threads_info[thread_id].min_thread_execution_time =
			thread_execution_time;
	}

	if (thread_execution_time >
	    threads_info[thread_id].max_thread_execution_time) {
		threads_info[thread_id].max_thread_execution_time =
			thread_execution_time;
	}

	threads_info[thread_id].mean_thread_execution_time =
		threads_info[thread_id].mean_thread_execution_time +
		((double)thread_execution_time -
		 threads_info[thread_id].mean_thread_execution_time) /
			((double)threads_info[thread_id].execution_time_counter +
			 1);

	threads_info[thread_id].execution_time_counter++;
}

bool ThreadsCommon_CreateCyclicRequest(uint64_t interval_ns,
				       uint64_t dispatch_offset_ns,
				       uint32_t queue_id, uint32_t request_size)
{
	assert(request_size <= sizeof(struct CyclicInterfaceEmptyRequestData));
	memset(empty_request.m_data, 0, EMPTY_REQUEST_DATA_BUFFER_SIZE);

	if (cyclic_requests_count >= RT_MAX_CYCLIC_INTERFACES) {
		return false;
	}

	rtems_name name = generate_new_partition_timer_name();
	const rtems_status_code timer_creation_status = rtems_timer_create(
		name, &cyclic_request_data[cyclic_requests_count].timer_id);
	if (timer_creation_status != RTEMS_SUCCESSFUL) {
		return false;
	}

	cyclic_request_data[cyclic_requests_count].next_wakeup_ticks =
		RTEMS_MILLISECONDS_TO_TICKS(dispatch_offset_ns /
					    NANOSECOND_IN_MILISECOND);
	cyclic_request_data[cyclic_requests_count].interval_ticks =
		RTEMS_MILLISECONDS_TO_TICKS(interval_ns /
					    NANOSECOND_IN_MILISECOND);
	cyclic_request_data[cyclic_requests_count].queue_id = queue_id;
	cyclic_request_data[cyclic_requests_count].request_size = request_size;

	schedule_next_tick(cyclic_requests_count);

	cyclic_requests_count++;
	return true;
}

bool ThreadsCommon_ProcessRequest(void *request_data, uint32_t request_size,
				  void *user_function, uint32_t thread_id)
{
	call_function cast_user_function = (call_function)user_function;

	const uint64_t time_before_execution = Hal_GetElapsedTimeInNs();
	cast_user_function((const char *)request_data, request_size);
	const uint64_t time_after_execution = Hal_GetElapsedTimeInNs();

	threads_info[thread_id].thread_execution_time =
		time_before_execution - time_after_execution;
	update_execution_time_data(
		thread_id, threads_info[thread_id].thread_execution_time);

	return true;
}
