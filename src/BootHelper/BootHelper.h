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

#ifndef BOOTHELPER_H
#define BOOTHELPER_H

/**
 * @file    BootHelper.h
 * @brief   Header for BootHelper
 */

/**
 * @brief   Enum representing possible reasons of hardware reset
 */
enum Reset_Reason {
	Reset_Reason_Unknown = 0,
	Reset_Reason_Powerup = 1,
	Reset_Reason_Backup = 2,
	Reset_Reason_Watchdog = 3,
	Reset_Reason_Software = 4,
	Reset_Reason_User = 5
};

/**
 * @brief               Returns information about reason of hardware reset
 *
 * @return              Enum representing possible reasons of hardware reset
 */
enum Reset_Reason BootHelper_GetResetReason();

#endif
