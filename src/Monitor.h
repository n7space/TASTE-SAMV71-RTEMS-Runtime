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

#ifndef MONITOR_H
#define MONITOR_H

/**
 * @file    Monitor.h
 * @brief   Header for Monitor
 */

#include "interfaces_info.h"
#include <float.h>
#include <rtems.h>
#include <rtems/cpuuse.h>
#include <rtems/score/threadimpl.h>
#include <rtems/score/todimpl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief   Struct representing usage and benchmarking data for the given
 * interface
 */
struct Monitor_InterfaceUsageData {
	enum interfaces_enum interface;
	uint64_t maximum_execution_time;
	uint64_t minimum_execution_time;
	uint64_t average_execution_time;
};

/**
 * @brief   Struct representing cpu usage data
 */
struct Monitor_CPUUsageData {
	float maximum_cpu_usage;
	float minimum_cpu_usage;
	float average_cpu_usage;
};

/**
 * @brief   Struct representing two possible types of entry value
 */
enum Monitor_EntryType {
	Monitor_EntryType_activation = 0,
	Monitor_EntryType_deactivation = 1
};

/**
 * @brief   Struct representing the interface activation entry
 */
struct Monitor_InterfaceActivationEntry {
	enum interfaces_enum interface;
	enum Monitor_EntryType entry_type;
	uint64_t timestamp;
};

/**
 * @brief                       Initializes the Monitor module.
 *
 * @return                      Bool indicating whether the initialization was
 *                              successful
 */
bool Monitor_Init(void);

/**
 * @brief                       Gathers monitoring information about every sporadic/cyclic interface and update 
 *                              internal data structure that hold these information.
 * 
 * @return                      Bool indicating whether the tick was successful
 */
bool Monitor_MonitoringTick(void);

/**
 * @brief                       Returns structure containing information about maximum execution time, minimum execution time,
 *                              average execution time of a given sporadic/cyclic interface
 *
 * @param[in] interface         Represents interface to obtain usage data
 * @param[out] usage_data       pointer to struct representing usage data of given sporadic/cyclic
 *                              interface, it is considered an output parameter
 * 
 * @return                      Bool indicating whether the query about usage data was successful
 */
bool Monitor_GetUsageData(const enum interfaces_enum interface,
			  struct Monitor_InterfaceUsageData *const usage_data);

/**
 * @brief                       Returns structure containing information about current CPU usage in idle state (time when CPU was not
 *                              used by any sporadic/cyclic interface)
 *
 * @param[out] cpu_usage_data   pointer to struct representing idle usage data regarding time when CPU was
 *                              not used by any sporadic/cyclic interface
 * 
 * @return                      Bool indicating whether the query about CPU usage data was successful
 */
bool Monitor_GetIdleCPUUsageData(
	struct Monitor_CPUUsageData *const cpu_usage_data);

/**
 * @brief                       Informs the monitor about given interface activation, 
 *                              monitor stores timestamp of activation in specific 
 *                              configurable memory location.
 *
 * @param[in] interface         enum representing activated interface
 * 
 * @return                      Bool indicating whether the indication was
 *                              successful
 */
bool Monitor_IndicateInterfaceActivated(const enum interfaces_enum interface);

/**
 * @brief                       Informs the monitor about given interface deactivation, 
 *                              monitor stores timestamp of deactivation in specific 
 *                              configurable memory location.
 *
 * @param[in] interface         enum representing deactivated interface
 * 
 * @return                      Bool indicating whether the indication was
 *                              successful
 */
bool Monitor_IndicateInterfaceDeactivated(const enum interfaces_enum interface);

/**
 * @brief                                            Provides access to optional interface activation log.
 *
 * @param[out] activation_log                        pointer pointing to beginning of cyclic buffer holding 
 *                                                   all activation entries
 * @param[out] out_latest_activation_entry_index     representing latest activation log index
 * @param[out] out_size_of_activation_log            representing size of activation log
 * 
 * @return                                           Bool indicating whether the query was successful
 */
bool Monitor_GetInterfaceActivationEntryLog(
	struct Monitor_InterfaceActivationEntry **activation_log,
	uint32_t *out_latest_activation_entry_index,
	uint32_t *out_size_of_activation_log);

/**
 * @brief                       Stops storing of interface activation logs, all activation 
 *                              logs are lost after this operation
 * 
 * @return                      Bool indicating whether the freeze was successful
 */
bool Monitor_FreezeInterfaceActivationLogging();

/**
 * @brief                       Starts storing of interface activation logs
 * 
 * @return                      Bool indicating whether the unfreeze was successful
 */
bool Monitor_UnfreezeInterfaceActivationLogging();

/**
 * @brief                       Clears all interface activation logs, resets latest_activation_entry_index 
 *                              and size_of_activation_log
 * 
 * @return                      Bool indicating whether the clear was successful
 */
bool Monitor_ClearInterfaceActivationLog();

#endif
