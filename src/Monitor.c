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

bool Monitor_Init()
{
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