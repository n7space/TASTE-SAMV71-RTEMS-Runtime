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

#include <BrokerLock.h>

#include <rtems.h>

extern rtems_id broker_Semaphore;

void
Broker_acquire_lock()
{
    while (rtems_semaphore_obtain(broker_Semaphore, RTEMS_WAIT, RTEMS_NO_WAIT) != RTEMS_SUCCESSFUL);
}

void
Broker_release_lock()
{
    rtems_semaphore_release(broker_Semaphore);
}
