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

#include <BootHelper.h>
#include <Rstc/Rstc.h>

enum BootHelper_ResetReason BootHelper_GetResetReason()
{
	const Rstc_ResetType reset_type = Rstc_getLastResetType();

	switch (reset_type) {
	case Rstc_ResetType_Powerup: {
		return BootHelper_ResetReason_Powerup;
	}
	case Rstc_ResetType_Backup: {
		return BootHelper_ResetReason_Backup;
	}
	case Rstc_ResetType_Watchdog: {
		return BootHelper_ResetReason_Watchdog;
	}
	case Rstc_ResetType_Software: {
		return BootHelper_ResetReason_Software;
	}
	case Rstc_ResetType_User: {
		return BootHelper_ResetReason_User;
	}
	default: {
		return BootHelper_ResetReason_Unknown;
	}
	}
}
