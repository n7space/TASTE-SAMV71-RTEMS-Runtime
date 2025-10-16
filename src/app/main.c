/**@file
 * This file is part of the TASTE SAMV71 RTEMS Drivers.
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

/**
 * @file     main.c
 * @brief    File to check if drivers code can be properly compiled and linked.
 */

#include <rtems.h>

#include <Hal.h>

#define RUNTIME_TASK_COUNT (1 + 3 + 0)
#define RUNTIME_FUNCTION_COUNT (1 + 2 + (0 * 2))

#define PROVIDED_INTERFACE_COUNT (0 + 1 + 1 + 1)

#define MAX_TLS_SIZE                                             \
	RTEMS_ALIGN_UP(((64 > (8 * PROVIDED_INTERFACE_COUNT)) ?  \
				64 :                             \
				(8 * PROVIDED_INTERFACE_COUNT)), \
		       RTEMS_TASK_STORAGE_ALIGNMENT)

#define TASK_ATTRIBUTES RTEMS_DEFAULT_ATTRIBUTES

#define TASK_STORAGE_SIZE                                                \
	RTEMS_TASK_STORAGE_SIZE(MAX_TLS_SIZE + RTEMS_MINIMUM_STACK_SIZE, \
				TASK_ATTRIBUTES)

rtems_task Init(rtems_task_argument argument)
{
	Hal_Init();
}

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER

#define CONFIGURE_MAXIMUM_PROCESSORS 1

#define CONFIGURE_MAXIMUM_BARRIERS 0

#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES RUNTIME_TASK_COUNT

#define CONFIGURE_MAXIMUM_PARTITIONS 0

#define CONFIGURE_MAXIMUM_PERIODS 0

#define CONFIGURE_MAXIMUM_SEMAPHORES RUNTIME_FUNCTION_COUNT

#define CONFIGURE_MAXIMUM_TASKS RUNTIME_TASK_COUNT

#define CONFIGURE_MINIMUM_TASKS_WITH_USER_PROVIDED_STORAGE \
	CONFIGURE_MAXIMUM_TASKS

#define CONFIGURE_MAXIMUM_TIMERS RUNTIME_TASK_COUNT

#define CONFIGURE_MAXIMUM_USER_EXTENSIONS 0

#define CONFIGURE_MICROSECONDS_PER_TICK 1000

#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 0

#define CONFIGURE_DISABLE_NEWLIB_REENTRANCY

#define CONFIGURE_APPLICATION_DISABLE_FILESYSTEM

#define CONFIGURE_MAXIMUM_THREAD_LOCAL_STORAGE_SIZE MAX_TLS_SIZE

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT_TASK_ATTRIBUTES TASK_ATTRIBUTES | RTEMS_FLOATING_POINT

#define CONFIGURE_INIT_TASK_INITIAL_MODES RTEMS_DEFAULT_MODES

#define CONFIGURE_INIT_TASK_CONSTRUCT_STORAGE_SIZE TASK_STORAGE_SIZE

#define CONFIGURE_INIT

#include <rtems/confdefs.h>
