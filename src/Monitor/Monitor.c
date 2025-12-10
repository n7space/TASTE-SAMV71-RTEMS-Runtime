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

#include <Monitor.h>
#include <Hal.h>
#include <string.h>
#include <rtems/score/cpu.h>

#ifdef RT_EXEC_LOG_ACTIVE
#define RT_EXEC_LOG_BUFFER_ALIGNMENT 4

extern const uint32_t log_buffer_start;
extern const uint32_t log_buffer_end;

#define RT_EXEC_LOG_BUFFER_SIZE \
    ((uint32_t)&log_buffer_end - (uint32_t)&log_buffer_start) \
    / sizeof(struct Monitor_InterfaceActivationEntry)

static volatile bool is_frozen = true;
static uint32_t activation_entry_counter = 0;

__attribute__((section(".logsection"), aligned(RT_EXEC_LOG_BUFFER_ALIGNMENT)))
static struct Monitor_InterfaceActivationEntry *const activation_log_buffer = 
	(struct Monitor_InterfaceActivationEntry *const)&log_buffer_start;
#endif

#define STACK_BYTE_PATTERN (uint32_t)0xA5A5A5A5

static uint32_t benchmarking_ticks = 0;
static Timestamp_Control uptime_at_last_reset = 0;
static Timestamp_Control total_usage_time = 0;
static struct Monitor_CPUUsageData idle_cpu_usage_data;

struct Monitor_MaximumStackUsageData {
	enum interfaces_enum interface;
	uint32_t maximum_stack_usage;
	bool is_found;
};

Monitor_MessageQueueOverflow Monitor_MessageQueueOverflowCallback;

static bool
handle_activation_log_cyclic_buffer(const enum interfaces_enum interface,
				    const enum Monitor_EntryType entry_type)
{
#ifndef RT_EXEC_LOG_ACTIVE
	return false;
#else
	if (is_frozen) {
		return false;
	}

	activation_log_buffer[activation_entry_counter % RT_EXEC_LOG_BUFFER_SIZE]
		.interface = interface;
	activation_log_buffer[activation_entry_counter % RT_EXEC_LOG_BUFFER_SIZE]
		.entry_type = entry_type;
	activation_log_buffer[activation_entry_counter % RT_EXEC_LOG_BUFFER_SIZE]
		.timestamp = Hal_GetElapsedTimeInNs();
	activation_entry_counter++;

	return true;
#endif
}

static bool cpu_usage_visitor(Thread_Control *the_thread, void *arg)
{
	float usage_percent;
	uint32_t integer_val;
	uint32_t float_val;
	Timestamp_Control uptime;
	const Timestamp_Control used_time =
		_Thread_Get_CPU_time_used_after_last_reset(the_thread);

	_TOD_Get_uptime(&uptime);
	_Timestamp_Subtract(&uptime_at_last_reset, &uptime, &total_usage_time);
	_Timestamp_Divide(&used_time, &total_usage_time, &integer_val,
			  &float_val);

	usage_percent = (float)float_val / TOD_NANOSECONDS_PER_MICROSECOND;
	usage_percent += (float)integer_val;

	if (usage_percent < idle_cpu_usage_data.minimum_cpu_usage) {
		idle_cpu_usage_data.minimum_cpu_usage = usage_percent;
	}

	if (usage_percent > idle_cpu_usage_data.maximum_cpu_usage) {
		idle_cpu_usage_data.maximum_cpu_usage = usage_percent;
	}

	idle_cpu_usage_data.average_cpu_usage =
		idle_cpu_usage_data.average_cpu_usage +
		(usage_percent - idle_cpu_usage_data.average_cpu_usage) /
			(benchmarking_ticks + 1);

	// only first idle thread is needed, stop iteration after first step
	return true;
}

static inline void *find_high_water_mark(const void *stack_start,
					 const uint32_t stack_size)
{
#if (CPU_STACK_GROWS_UP == TRUE)

	for (uintptr_t pointer = (uintptr_t)stack_start + stack_size;
	     pointer > (uintptr_t)stack_start; pointer -= sizeof(uint32_t)) {
		if (*(uint32_t *)pointer != STACK_BYTE_PATTERN) {
			return (void *)pointer;
		}
	}

#else

	for (uintptr_t pointer = (uintptr_t)stack_start;
	     pointer < (uintptr_t)stack_start + stack_size;
	     pointer += sizeof(uint32_t)) {
		if (*(uint32_t *)pointer != STACK_BYTE_PATTERN) {
			return (void *)pointer;
		}
	}

#endif

	return NULL;
}

// Where the pattern goes in the stack area is dependent upon
// whether the stack grow to the high or low area of the memory.
static inline uint32_t calculate_used_stack(void *stack_start,
					    uint32_t stack_size,
					    void *high_water_mark)
{
#if (CPU_STACK_GROWS_UP == TRUE)
	return (uint8_t *)(high_water_mark) - (uint8_t *)(stack_start);
#else
	return ((uint8_t *)(stack_start) + (stack_size)) -
	       (uint8_t *)(high_water_mark);
#endif
}

static bool thread_stack_usage_visitor(Thread_Control *the_thread, void *arg)
{
	struct Monitor_MaximumStackUsageData *stack_usage_data =
		(struct Monitor_MaximumStackUsageData *)arg;
	const uint32_t id = the_thread->Object.id;

	if (threads_info[stack_usage_data->interface].id != id) {
		return false;
	}

	const Stack_Control *stack = &the_thread->Start.Initial_stack;

	// This is likely to occur if the stack checker is not actually enabled
	if (stack->area == NULL) {
		return true;
	}

	uint32_t stack_size = stack->size;
	void *stack_start = stack->area;
	void *high_water_mark = find_high_water_mark(stack_start, stack_size);

	if (high_water_mark) {
		stack_usage_data->maximum_stack_usage = calculate_used_stack(
			stack_start, stack_size, high_water_mark);
	}

	stack_usage_data->is_found = true;
	return true;
}

bool Monitor_Init()
{
	_Timestamp_Set_to_zero(&total_usage_time);
	rtems_cpu_usage_reset();
	_TOD_Get_uptime(&uptime_at_last_reset);

	for (int i = 0; i < RUNTIME_THREAD_COUNT; i++) {
		maximum_queued_items[i] = 0;
	}

	return true;
}

bool Monitor_MonitoringTick(void)
{
	// update information about cpu usage
	rtems_task_iterate(cpu_usage_visitor, NULL);
	benchmarking_ticks++;
}

bool Monitor_GetUsageData(const enum interfaces_enum interface,
			  struct Monitor_InterfaceUsageData *const usage_data)
{
	usage_data->interface = interface;
	usage_data->maximum_execution_time =
		threads_info[interface].max_thread_execution_time;
	usage_data->minimum_execution_time =
		threads_info[interface].min_thread_execution_time;
	usage_data->average_execution_time =
		(uint64_t)threads_info[interface].mean_thread_execution_time;
	return true;
}

bool Monitor_GetIdleCPUUsageData(
	struct Monitor_CPUUsageData *const cpu_usage_data)
{
	*cpu_usage_data = idle_cpu_usage_data;
	return true;
}

int32_t Monitor_GetMaximumStackUsage(const enum interfaces_enum interface)
{
#ifndef RT_MEASURE_STACK
	return -1;
#endif

	struct Monitor_MaximumStackUsageData stack_usage;
	stack_usage.interface = interface;
	stack_usage.maximum_stack_usage = 0;
	stack_usage.is_found = false;

	rtems_task_iterate(thread_stack_usage_visitor, &stack_usage);

	if (stack_usage.is_found) {
		return stack_usage.maximum_stack_usage;
	}

	return -1;
}

bool Monitor_SetMessageQueueOverflowCallback(
	Monitor_MessageQueueOverflow overflow_callback)
{
	Monitor_MessageQueueOverflowCallback = overflow_callback;
	return true;
}

int32_t Monitor_GetQueuedItemsCount(const enum interfaces_enum interface)
{
	if (interface_to_queue_map[interface] == RTEMS_ID_NONE) {
		return -1;
	}

	uint32_t count;
	if (rtems_message_queue_get_number_pending(
		    interface_to_queue_map[interface], &count) !=
	    RTEMS_SUCCESSFUL) {
		return -1;
	}

	return (int32_t)count;
}

int32_t Monitor_GetMaximumQueuedItemsCount(const enum interfaces_enum interface)
{
	return maximum_queued_items[interface];
}

bool Monitor_IndicateInterfaceActivated(const enum interfaces_enum interface)
{
	return handle_activation_log_cyclic_buffer(
		interface, Monitor_EntryType_activation);
}

bool Monitor_IndicateInterfaceDeactivated(const enum interfaces_enum interface)
{
	return handle_activation_log_cyclic_buffer(
		interface, Monitor_EntryType_deactivation);
}

bool Monitor_GetInterfaceActivationEntryLog(
	struct Monitor_InterfaceActivationEntry **activation_log,
	uint32_t *out_latest_activation_entry_index,
	uint32_t *out_size_of_activation_log)
{
#ifndef RT_EXEC_LOG_ACTIVE
	return false;
#else
	*activation_log = activation_log_buffer;

	if (activation_entry_counter == 0) {
		*out_latest_activation_entry_index = 0;
		*out_size_of_activation_log = 0;
	} else {
		*out_latest_activation_entry_index =
			(activation_entry_counter % RT_EXEC_LOG_BUFFER_SIZE) - 1;

		if (activation_entry_counter > RT_EXEC_LOG_BUFFER_SIZE) {
			*out_size_of_activation_log = RT_EXEC_LOG_BUFFER_SIZE;
		} else {
			*out_size_of_activation_log = activation_entry_counter;
		}
	}

	return true;
#endif
}

bool Monitor_FreezeInterfaceActivationLogging()
{
#ifndef RT_EXEC_LOG_ACTIVE
	return false;
#else
	if (RT_EXEC_LOG_BUFFER_SIZE <= 0) {
		return false;
	}

	is_frozen = true;

	return true;
#endif
}

bool Monitor_UnfreezeInterfaceActivationLogging()
{
#ifndef RT_EXEC_LOG_ACTIVE
	return false;
#else
	if (RT_EXEC_LOG_BUFFER_SIZE <= 0) {
		return false;
	}

	is_frozen = false;

	return true;
#endif
}

bool Monitor_ClearInterfaceActivationLog()
{
#ifndef RT_EXEC_LOG_ACTIVE
	return false;
#else
	if (RT_EXEC_LOG_BUFFER_SIZE <= 0) {
		return false;
	}

	activation_entry_counter = 0;

	return true;
#endif
}
