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

#ifndef MONITOR_H
#define MONITOR_H

/**
 * @file    Monitor.h
 * @brief   Header for Monitor
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "interfaces_info.h"

enum Monitor_EntryType {
    Monitor_EntryType_activation = 0,
    Monitor_EntryType_deactivation = 1
};

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
 * @brief                                           Provides access to optional interface activation log.
 *
 * @param[in] activation_log                        pointer pointing to beginning of cyclic buffer holding 
 *                                                  all activation entries
 * @param[in] out_latest_activation_entry_index     representing latest activation log index
 * @param[in] out_size_of_activation_log            representing size of activation log
 * 
 * @return                                          Bool indicating whether the query was successful
 */
bool Monitor_GetInterfaceActivationEntryLog(struct Monitor_InterfaceActivationEntry *activation_log, 
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
