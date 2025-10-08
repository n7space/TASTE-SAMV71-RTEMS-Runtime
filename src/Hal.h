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

#ifndef HAL_H
#define HAL_H

/**
 * @file    Hal.h
 * @brief   Header for Hal
 */

#include <stdbool.h>
#include <stdint.h>

#include <Nvic/Nvic.h>
#include <Uart/Uart.h>

#ifndef RT_MAX_HAL_SEMAPHORES
#define RT_MAX_HAL_SEMAPHORES 8
#endif

/// \brief Uart configuration structure
typedef struct {
	Uart_Id id; /// Uart device identifier (0-4)
	Uart_Parity parity; //< used parity bits
	uint32_t baudrate; //< chosen baud rate
} Hal_Uart_Config;

/// \brief uart structure
typedef struct {
	Uart uart;
	ByteFifo rxFifo;
	ByteFifo txFifo;
} Hal_Uart;

/// \brief A function serving as a callback called upon an interrupt if the
/// interrupt
///          was subscribed to.
typedef void(InterruptCallback)(void *);

/// \brief Subscribes to interrupt and sets function that is called upon the
/// interrupt
///         reception
/// \param [in] irq Numeric identifier of the interrupt to subscribe to
/// \param [in] callback Callback function pointer
void Hal_subscribe_to_interrupt(Nvic_Irq irq, InterruptCallback callback);

/// \brief Starts up, initializes and configures Uart and coresponding
/// periferals \param [in] halUart Hal_Uart structure contains uart device
/// descriptor and relevant fifos. \param [in] halUartConfig configuration
/// structure
void Hal_uart_init(Hal_Uart *const halUart, Hal_Uart_Config halUartConfig);

/// \brief Asynchronously sends bytes over uart.
/// \param [in] halUart Hal_Uart structure contains uart device descriptor and
/// relevant fifos. \param [in] buffer array containing bytes to send \param
/// [in] length length of array of bytes \param [in] txHandler pointer to the
/// handler called after successful array transmission
void Hal_uart_write(Hal_Uart *const halUart, uint8_t *const buffer,
		    const uint16_t length,
		    const Uart_TxHandler *const txHandler);

/// \brief Asynchronously receives bytes over uart.
/// \param [in] halUart Hal_Uart structure contains uart device descriptor and
/// relevant fifos. \param [in] buffer array where received bytes will be
/// storedx \param [in] length length of array of bytes \param [in] rxHandler
/// handler called after successful array reception or after maching character
/// was found
void Hal_uart_read(Hal_Uart *const halUart, uint8_t *const buffer,
		   const uint16_t length, const Uart_RxHandler rxHandler);

/// \brief Initializes USART 1, which is connected to virtual com port gateway
/**
 * Initializes pmc for pio port B and for usart 1
 * Initializes pio PB4 (TXD1)
 * Initializes USART1 Mode Register (MR) - no parity, 1 stop bit
 * Initializes USART1 baudrate 115200
 */
void Hal_console_usart_init(void);

/// \brief Synchroniously writes bytes over USART1
/// \param [in] buffer array containing bytes to send
/// \param [in] count length of array of bytes
void Hal_console_usart_write(const uint8_t *const buffer, const uint16_t count);

/**
 * @brief               Initializes the Hal module.
 *
 * @return              Bool indicating whether the initialization was
 *                      successful
 */
bool Hal_Init(void);

/**
 * @brief               Returns time elapsed from the initialization of the
 *                      runtime
 *
 * @return              Time elapsed from the initialization of the runtime
 */
uint64_t Hal_GetElapsedTimeInNs(void);

/**
 * @brief               Suspends the current thread for the given amount of time
 *
 * @param[in] time_ns   time in nanoseconds
 *
 * @return              Bool indicating whether the sleep was successful
 */
bool Hal_SleepNs(uint64_t time_ns);

/**
 * @brief               Creates an RTOS backed semaphore. This function is not
 *                      thread safe, but it is assumed to be used only during
 *                      system initialization, from a single thread/Init task.
 *
 * @return              ID of the created semaphore
 */
int32_t Hal_SemaphoreCreate(void);

/**
 * @brief               Obtains the indicated semaphore, suspending the
 *                      execution of the current thread if necessary.
 *
 * @param[in] id        id of the semaphore
 *
 * @return              Bool indicating whether the obtain was successful
 */
bool Hal_SemaphoreObtain(int32_t id);

/**
 * @brief               Releases the indicated semaphore, potentially resuming
 *                      threads waiting on the semaphore
 *
 * @param[in] id        id of the semaphore
 *
 * @return              Bool indicating whether the release was successful
 */
bool Hal_SemaphoreRelease(int32_t id);

#endif
