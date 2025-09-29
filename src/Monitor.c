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

#ifndef RT_EXEC_LOG_SIZE
#define RT_EXEC_LOG_SIZE 0
#endif

#if RT_EXEC_LOG_SIZE <= 0
static struct Monitor_InterfaceActivationEntry *activation_log_buffer = NULL;
#else
static struct Monitor_InterfaceActivationEntry activation_log_buffer[RT_EXEC_LOG_SIZE];
#endif

static volatile bool is_frozen = true;
static uint32_t activation_entry_counter = 0;

static uint32_t benchmarking_ticks = 0;
static Timestamp_Control uptime_at_last_reset = 0;
static Timestamp_Control total_usage_time = 0;
static struct Monitor_CPUUsageData idle_cpu_usage_data;

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