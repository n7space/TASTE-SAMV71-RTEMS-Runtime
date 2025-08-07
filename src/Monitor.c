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

#include <Monitor.h>
#include <Hal.h>

#ifndef RT_EXEC_LOG_SIZE
#define RT_EXEC_LOG_SIZE 0
#endif

#if RT_EXEC_LOG_SIZE <= 0
static struct Monitor_InterfaceActivationEntry *_activation_log_buffer = NULL;
#else
static struct Monitor_InterfaceActivationEntry _activation_log_buffer[RT_EXEC_LOG_SIZE];
#endif

static bool _is_frozen = true;
static uint32_t _latest_activation_entry_index = 0;
static uint32_t _size_of_activation_log = 0;

static void UpdateActivationLog(const uint32_t index, const enum interfaces_enum interface, 
                                const enum Monitor_EntryType entry_type)
{
    uint64_t timestamp = Hal_GetElapsedTimeInNs();

    _activation_log_buffer[_latest_activation_entry_index].interface = interface;
    _activation_log_buffer[_latest_activation_entry_index].entry_type = entry_type;
    _activation_log_buffer[_latest_activation_entry_index].timestamp = timestamp;
}

static bool HandleActivationLogCyclicBuffer(const enum interfaces_enum interface, 
                                               const enum Monitor_EntryType entry_Type)
{
    if(RT_EXEC_LOG_SIZE <= 0 || _is_frozen){
        return false;
    }

    if(_latest_activation_entry_index == 0 && _size_of_activation_log == 0){
        UpdateActivationLog(_latest_activation_entry_index, interface, entry_Type);
        _size_of_activation_log++;
    }
    else if((_latest_activation_entry_index < RT_EXEC_LOG_SIZE - 1) && (_size_of_activation_log < RT_EXEC_LOG_SIZE)){
        _latest_activation_entry_index++;
        UpdateActivationLog(_latest_activation_entry_index, interface, entry_Type);
        _size_of_activation_log++;
    }
    else if((_latest_activation_entry_index == RT_EXEC_LOG_SIZE - 1) && (_size_of_activation_log == RT_EXEC_LOG_SIZE)){
        _latest_activation_entry_index = 0;
        UpdateActivationLog(_latest_activation_entry_index, interface, entry_Type);
    }
    else if((_latest_activation_entry_index < RT_EXEC_LOG_SIZE - 1) && (_size_of_activation_log == RT_EXEC_LOG_SIZE)){
        _latest_activation_entry_index++;
        UpdateActivationLog(_latest_activation_entry_index, interface, entry_Type);
    }
    else{
        return false;
    }

    return true;
}

bool Monitor_Init()
{
    return true;
}

bool Monitor_IndicateInterfaceActivated(const enum interfaces_enum interface)
{
    return HandleActivationLogCyclicBuffer(interface, Monitor_EntryType_activation);
}

bool Monitor_IndicateInterfaceDeactivated(const enum interfaces_enum interface)
{
    return HandleActivationLogCyclicBuffer(interface, Monitor_EntryType_deactivation);
}

bool Monitor_GetInterfaceActivationEntryLog(struct Monitor_InterfaceActivationEntry *activation_log, 
                                            uint32_t *latest_activation_entry_index, 
                                            uint32_t *size_of_activation_log)
{
    if(RT_EXEC_LOG_SIZE <= 0){
        return false;
    }

    activation_log = _activation_log_buffer;
    *latest_activation_entry_index = _latest_activation_entry_index;
    *size_of_activation_log = _size_of_activation_log;

    return true;
}

bool Monitor_FreezeInterfaceActivationLogging()
{
    if(RT_EXEC_LOG_SIZE <= 0){
        return false;
    }

    _is_frozen = true;

    return true;
}

bool Monitor_UnfreezeInterfaceActivationLogging()
{
    if(RT_EXEC_LOG_SIZE <= 0){
        return false;
    }

    _is_frozen = false;

    return true;
}

bool Monitor_ClearInterfaceActivationLog()
{
    if(RT_EXEC_LOG_SIZE <= 0){
        return false;
    }

    _latest_activation_entry_index = 0;
    _size_of_activation_log = 0;

    return true;
}