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
#include <rtems/score/cpu.h>

#ifndef RT_EXEC_LOG_SIZE
#define RT_EXEC_LOG_SIZE 0
#endif

#if RT_EXEC_LOG_SIZE <= 0
static struct Monitor_InterfaceActivationEntry *activation_log_buffer = NULL;
#else
static struct Monitor_InterfaceActivationEntry activation_log_buffer[RT_EXEC_LOG_SIZE];
#endif

#define STACK_U32_PATTERN 0xA5A5A5A5

#define CPU_STACK_CHECK_PATTERN_INITIALIZER \
  { \
    0xFEEDF00D, 0x0BAD0D06, /* FEED FOOD to  BAD DOG */ \
    0xDEADF00D, 0x600D0D06  /* DEAD FOOD but GOOD DOG */ \
  }

#define CPU_STACK_CHECK_PATTERN_INITIALIZER_ARRAY_SIZE 4

// The "magic pattern" used to mark the end of the stack.
static const uint32_t Stack_check_Sanity_pattern[] =
  CPU_STACK_CHECK_PATTERN_INITIALIZER;

#define SANITY_PATTERN_SIZE_BYTES sizeof(Stack_check_Sanity_pattern)

// Where the pattern goes in the stack area is dependent upon
// whether the stack grow to the high or low area of the memory.
#if (CPU_STACK_GROWS_UP == TRUE)
  #define Stack_check_Get_pattern( _the_stack ) \
    ((char *)(_the_stack)->area + \
         (_the_stack)->size - SANITY_PATTERN_SIZE_BYTES )

  #define Stack_check_Calculate_used( _low, _size, _high_water ) \
      ((char *)(_high_water) - (char *)(_low))

  #define Stack_check_Usable_stack_start(_the_stack) \
    ((_the_stack)->area)
#else
  #define Stack_check_Get_pattern( _the_stack ) \
    ((char *)(_the_stack)->area)

  #define Stack_check_Calculate_used( _low, _size, _high_water) \
      ( ((char *)(_low) + (_size)) - (char *)(_high_water) )

  #define Stack_check_Usable_stack_start(_the_stack) \
      ((char *)(_the_stack)->area + SANITY_PATTERN_SIZE_BYTES)
#endif

#define Stack_check_Usable_stack_size(_the_stack) \
    ((_the_stack)->size - SANITY_PATTERN_SIZE_BYTES)

static volatile bool is_frozen = true;
static uint32_t activation_entry_counter = 0;

static uint32_t benchmarking_ticks = 0;
static Timestamp_Control uptime_at_last_reset = 0;
static Timestamp_Control total_usage_time = 0;
static struct Monitor_CPUUsageData idle_cpu_usage_data;

struct Monitor_MaxiumumStackUsageData {
	enum interfaces_enum interface;
	uint32_t maximum_stack_usage;
    bool is_found;
};

static bool handle_activation_log_cyclic_buffer(const enum interfaces_enum interface, 
                                                const enum Monitor_EntryType entry_type)
{
    if(RT_EXEC_LOG_SIZE <= 0 || is_frozen){
        return false;
    }

    activation_log_buffer[activation_entry_counter % RT_EXEC_LOG_SIZE].interface = interface;
    activation_log_buffer[activation_entry_counter % RT_EXEC_LOG_SIZE].entry_type = entry_type;
    activation_log_buffer[activation_entry_counter % RT_EXEC_LOG_SIZE].timestamp = Hal_GetElapsedTimeInNs();
    activation_entry_counter++;

    return true;
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
	_Timestamp_Divide(&used_time, &total_usage_time, &integer_val, &float_val);

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

static inline void *find_high_water_mark(const void *stack_start, const uint32_t stack_size)
{
  const uint32_t *base;
  const uint32_t *ebase;
  uint32_t length;

  base = stack_start;
  length = stack_size / 4;

  #if ( CPU_STACK_GROWS_UP == TRUE )
     // start at higher memory and find first word that does not match pattern

    base += length - 1;
    for (ebase = stack_start; base > ebase; base--){
        if (*base != STACK_U32_PATTERN){
            return (void *) base;
        }
    }
      
  #else
     // start at lower memory and find first word that does not match pattern

    base += CPU_STACK_CHECK_PATTERN_INITIALIZER_ARRAY_SIZE;
    for (ebase = base + length; base < ebase; base++)
      if (*base != STACK_U32_PATTERN)
        return (void *) base;
  #endif

  return NULL;
}

static bool thread_stack_usage_visitor(Thread_Control *the_thread, void *arg)
{
    struct Monitor_MaxiumumStackUsageData *stack_usage_data = (struct Monitor_MaxiumumStackUsageData *)arg;
    const uint32_t id = the_thread->Object.id;

    if(threads_info[stack_usage_data->interface].id != id){
        return false;
    }

    const Stack_Control *stack = &the_thread->Start.Initial_stack;

    // This is likely to occur if the stack checker is not actually enabled
    if (stack->area == NULL) {
        return true;
    }

    uint32_t stack_size = Stack_check_Usable_stack_size(stack);
    void *stack_start Stack_check_Usable_stack_start(stack);
    void *high_water_mark = find_high_water_mark(stack_start, stack_size);

    if(high_water_mark){
        stack_usage_data->maximum_stack_usage = Stack_check_Calculate_used(stack_start, stack_size, high_water_mark);
    }

    stack_usage_data->is_found = true;
    return true;
}

bool Monitor_Init()
{
    _Timestamp_Set_to_zero(&total_usage_time);
    rtems_cpu_usage_reset();
	_TOD_Get_uptime(&uptime_at_last_reset);
	
    for(int i = 0; i < RUNTIME_THREAD_COUNT; i++){
        idle_cpu_usage_data.maximum_cpu_usage = 0.0;
        idle_cpu_usage_data.minimum_cpu_usage = FLT_MAX;
        idle_cpu_usage_data.average_cpu_usage = 0.0;
    }

    return true;
}

bool Monitor_MonitoringTick(void)
{
	rtems_task_iterate(cpu_usage_visitor, NULL);
    benchmarking_ticks++;
}

bool Monitor_GetUsageData(const enum interfaces_enum interface, struct Monitor_InterfaceUsageData *const usage_data)
{
    usage_data->interface = interface;
    usage_data->maximum_execution_time = threads_info[interface].max_thread_execution_time;
    usage_data->minimum_execution_time = threads_info[interface].min_thread_execution_time;
    usage_data->average_execution_time = (uint64_t)threads_info[interface].mean_thread_execution_time;
    return true;
}

bool Monitor_GetIdleCPUUsageData(struct Monitor_CPUUsageData *const cpu_usage_data)
{
    *cpu_usage_data = idle_cpu_usage_data;
    return true;
}

int32_t Monitor_GetMaximumStackUsage(const enum interfaces_enum interface)
{
    struct Monitor_MaxiumumStackUsageData stack_usage;
    stack_usage.interface = interface;
    stack_usage.maximum_stack_usage = 0;
    stack_usage.is_found = false;

    rtems_task_iterate(thread_stack_usage_visitor, &stack_usage);

    if(stack_usage.is_found){
        return stack_usage.maximum_stack_usage;
    }

    return -1;
}

bool Monitor_IndicateInterfaceActivated(const enum interfaces_enum interface)
{
    return handle_activation_log_cyclic_buffer(interface, Monitor_EntryType_activation);
}

bool Monitor_IndicateInterfaceDeactivated(const enum interfaces_enum interface)
{
    return handle_activation_log_cyclic_buffer(interface, Monitor_EntryType_deactivation);
}

bool Monitor_GetInterfaceActivationEntryLog(struct Monitor_InterfaceActivationEntry **activation_log, 
                                            uint32_t *out_latest_activation_entry_index, 
                                            uint32_t *out_size_of_activation_log)
{
    if(RT_EXEC_LOG_SIZE <= 0){
        return false;
    }

    *activation_log = activation_log_buffer;

    if(activation_entry_counter == 0){
        *out_latest_activation_entry_index = 0;
        *out_size_of_activation_log = 0;
    }
    else{
        *out_latest_activation_entry_index = (activation_entry_counter % RT_EXEC_LOG_SIZE) - 1;

        if(activation_entry_counter > RT_EXEC_LOG_SIZE){
            *out_size_of_activation_log = RT_EXEC_LOG_SIZE;
        }
        else{
            *out_size_of_activation_log = activation_entry_counter;
        }
    }

    return true;
}

bool Monitor_FreezeInterfaceActivationLogging()
{
    if(RT_EXEC_LOG_SIZE <= 0){
        return false;
    }

    is_frozen = true;

    return true;
}

bool Monitor_UnfreezeInterfaceActivationLogging()
{
    if(RT_EXEC_LOG_SIZE <= 0){
        return false;
    }

    is_frozen = false;

    return true;
}

bool Monitor_ClearInterfaceActivationLog()
{
    if(RT_EXEC_LOG_SIZE <= 0){
        return false;
    }

    activation_entry_counter = 0;

    return true;
}