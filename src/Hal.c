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

#include "Hal.h"

#include <string.h>

#include <Nvic/Nvic.h>
#include <Pmc/Pmc.h>
#include <Tic/Tic.h>
#include <Utils/ConcurrentAccessFlag.h>
#include <interfaces_info.h>
#include <rtems.h>

#include "UsartRegisters.h"
#include "Xdmac/xdmad.h"
#include <Nvic/Nvic.h>
#include <Pio/Pio.h>
#include <Scb/Scb.h>
#include <Uart/Uart.h>
#include <Utils/ErrorCode.h>
#include <Wdt/Wdt.h>

#define DEFAULT_PERIPH_CLOCK 75000000

#define NANOSECOND_IN_SECOND 1000000000.0
#define MEGA_HZ 1000000u
#define TICKS_PER_RELOAD 65535ul
#define CLOCK_SELECTION_PRESCALLER 8.0

#ifndef MAIN_CRYSTAL_OSCILLATOR_FREQUNECY
#define MAIN_CRYSTAL_OSCILLATOR_FREQUNECY (12 * MEGA_HZ)
#endif

rtems_id xdmad_lock;

static uint32_t created_semaphores_count = 0;
static rtems_id hal_semaphore_ids[RT_MAX_HAL_SEMAPHORES];

static ConcurrentAccessFlag reloads_modified_flag;
static uint32_t reloads_counter;
Pmc pmc;
static Tic tic = {};

static uint64_t mck_frequency;

rtems_name generate_new_hal_semaphore_name();

rtems_name generate_new_hal_semaphore_name()
{
	static rtems_name name = rtems_build_name('H', 0, 0, 0);
	return name++;
}

inline static void Init_setup_watchdog(void)
{
	const Wdt_Config wdtConfig = {
		.counterValue = 0x0FFF,
		.deltaValue = 0x0FFF,
		.isResetEnabled = false,
		.isFaultInterruptEnabled = false,
		.isDisabled = true,
		.isHaltedOnIdle = false,
		.isHaltedOnDebug = false,
	};

	Wdt wdt;
	Wdt_init(&wdt);
	Wdt_setConfig(&wdt, &wdtConfig);
}

#define USART_BAUD_RATE 115200

static volatile InterruptCallback *interruptSubscription[Nvic_InterruptCount] = {
	NULL
};

static Uart *uart0handle;
static Uart *uart1handle;
static Uart *uart2handle;
static Uart *uart3handle;
static Uart *uart4handle;

/**
 * @brief UART priotity definition
 * System interrupts priorities levels must be smaller than
 * kernel interrupts levels. The lower the priority value the
 * higher the priority is. Thus, the UART interrupt priority value
 * must be equal or greater then configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY.
 */
#define UART_INTERRUPT_PRIORITY RTEMS_MAXIMUM_PRIORITY
#define UART_XDMAC_INTERRUPT_PRIORITY UART_INTERRUPT_PRIORITY

#define XDMAD_NO_POLLING 0

#define UART_ID_UART0 "UART0: "
#define UART_ID_UART1 "UART1: "
#define UART_ID_UART2 "UART2: "
#define UART_ID_UART3 "UART3: "
#define UART_ID_UART4 "UART4: "

#define UART_XDMAD_ERROR_NO_AVALIABLE_CHANNELS \
	"Hal:Hal_uartWrite: The xdmac channels are not avaliable.\n\r"

#define UART_READ_ERROR_OVERRUN_ERROR "Hal:Hal_uartRead: Overrun error.\n\r"
#define UART_READ_ERROR_FRAME_ERROR "Hal:Hal_uartRead: Frame error.\n\r"
#define UART_READ_ERROR_PARITY_ERROR "Hal:Hal_uartRead: Parity error.\n\r"

#define UART_RX_INTERRUPT_ERROR_FIFO_FULL \
	"Hal:Hal_interruptHandler: FIFO is full.\n\r"

void UART0_Handler(void)
{
	if (interruptSubscription[Nvic_Irq_Uart0] != NULL)
		interruptSubscription[Nvic_Irq_Uart0](NULL);
	else if (uart0handle != NULL)
		Uart_handleInterrupt(uart0handle);
}

void UART1_Handler(void)
{
	if (interruptSubscription[Nvic_Irq_Uart1] != NULL)
		interruptSubscription[Nvic_Irq_Uart1](NULL);
	else if (uart1handle != NULL)
		Uart_handleInterrupt(uart1handle);
}

void UART2_Handler(void)
{
	if (interruptSubscription[Nvic_Irq_Uart2] != NULL)
		interruptSubscription[Nvic_Irq_Uart2](NULL);
	else if (uart2handle != NULL)
		Uart_handleInterrupt(uart2handle);
}

void UART3_Handler(void)
{
	if (interruptSubscription[Nvic_Irq_Uart3] != NULL)
		interruptSubscription[Nvic_Irq_Uart3](NULL);
	else if (uart3handle != NULL)
		Uart_handleInterrupt(uart3handle);
}

void UART4_Handler(void)
{
	if (interruptSubscription[Nvic_Irq_Uart4] != NULL)
		interruptSubscription[Nvic_Irq_Uart4](NULL);
	else if (uart4handle != NULL)
		Uart_handleInterrupt(uart4handle);
}

inline static void Init_setup_xdmad_lock()
{
	const rtems_status_code status_code =
		rtems_semaphore_create(generate_new_hal_semaphore_name(),
				       1, // Initial value, unlocked
				       RTEMS_BINARY_SEMAPHORE,
				       0, // Priority ceiling
				       &xdmad_lock);
	assert(status_code == RTEMS_SUCCESSFUL);
}

void timer_irq_handler()
{
	__atomic_fetch_add(&reloads_counter, 1u, __ATOMIC_SEQ_CST);
	ConcurrentAccessFlag_set(&reloads_modified_flag);

	Tic_ChannelStatus status;
	Tic_getChannelStatus(&tic, Tic_Channel_0, &status);
}

void extract_main_oscilator_frequency()
{
	Pmc_MainckConfig main_clock_config;
	Pmc_getMainckConfig(&pmc, &main_clock_config);

	if (main_clock_config.src == Pmc_MainckSrc_XOsc) {
		mck_frequency = MAIN_CRYSTAL_OSCILLATOR_FREQUNECY;
		return;
	}

	switch (main_clock_config.rcOscFreq) {
	case Pmc_RcOscFreq_4M: {
		mck_frequency = 4 * MEGA_HZ;
		break;
	}
	case Pmc_RcOscFreq_8M: {
		mck_frequency = 8 * MEGA_HZ;
		break;
	}
#if defined(N7S_TARGET_SAMV71Q21)
	case Pmc_RcOscFreq_12M: {
		mck_frequency = 12 * MEGA_HZ;
		break;
	}
#elif defined(N7S_TARGET_SAMRH71F20) || defined(N7S_TARGET_SAMRH707F18)
	case Pmc_RcOscFreq_10M: {
		mck_frequency = 10 * MEGA_HZ;
		break;
	}
	case Pmc_RcOscFreq_12M: {
		mck_frequency = 12 * MEGA_HZ;
		break;
	}
#endif
	}
}

void apply_plla_config(Pmc_MasterckConfig *master_clock_config)
{
	if (master_clock_config->src == Pmc_MasterckSrc_Pllack) {
		Pmc_PllConfig pll_config;
		Pmc_getPllConfig(&pmc, &pll_config);
		if (pll_config.pllaDiv > 0 && pll_config.pllaMul > 0) {
			mck_frequency = (mck_frequency / pll_config.pllaDiv) *
					(pll_config.pllaMul + 1);
		} else if (pll_config.pllaDiv == 0 && pll_config.pllaMul > 0) {
			mck_frequency =
				mck_frequency * (pll_config.pllaMul + 1);
		} else if (pll_config.pllaDiv > 0 && pll_config.pllaMul == 0) {
			mck_frequency = mck_frequency / pll_config.pllaDiv;
		}
	}
}

void extract_mck_frequency()
{
	Pmc_MasterckConfig master_clock_config;
	Pmc_getMasterckConfig(&pmc, &master_clock_config);

	extract_main_oscilator_frequency();
	apply_plla_config(&master_clock_config);

	switch (master_clock_config.presc) {
	case Pmc_MasterckPresc_1: {
		break;
	}
	case Pmc_MasterckPresc_2: {
		mck_frequency = mck_frequency / 2;
		break;
	}
	case Pmc_MasterckPresc_4: {
		mck_frequency = mck_frequency / 4;
		break;
	}
	case Pmc_MasterckPresc_8: {
		mck_frequency = mck_frequency / 8;
		break;
	}
	case Pmc_MasterckPresc_16: {
		mck_frequency = mck_frequency / 16;
		break;
	}
	case Pmc_MasterckPresc_32: {
		mck_frequency = mck_frequency / 32;
		break;
	}
	case Pmc_MasterckPresc_64: {
		mck_frequency = mck_frequency / 64;
		break;
	}
#if defined(N7S_TARGET_SAMV71Q21)
	case Pmc_MasterckPresc_3: {
		mck_frequency = mck_frequency / 7;
		break;
	}
#endif
	}

	switch (master_clock_config.divider) {
	case Pmc_MasterckDiv_1: {
		break;
	}
	case Pmc_MasterckDiv_2: {
		mck_frequency = mck_frequency / 2;
		break;
	}
	}
}

static sXdmad xdmad;

void XDMAC_Handler(void)
{
	XDMAD_Handler(&xdmad);
}

bool Hal_Init(void)
{
	reloads_counter = 0u;

	rtems_interrupt_handler_install(58, "xdmac", RTEMS_INTERRUPT_UNIQUE,
					XDMAC_Handler, 0);
	rtems_interrupt_handler_install(7, "uart0", RTEMS_INTERRUPT_UNIQUE,
					UART0_Handler, 0);
	rtems_interrupt_vector_enable(7);
	rtems_interrupt_handler_install(8, "uart1", RTEMS_INTERRUPT_UNIQUE,
					UART1_Handler, 0);
	rtems_interrupt_vector_enable(8);
	rtems_interrupt_handler_install(44, "uart2", RTEMS_INTERRUPT_UNIQUE,
					UART2_Handler, 0);
	rtems_interrupt_vector_enable(44);
	rtems_interrupt_handler_install(45, "uart3", RTEMS_INTERRUPT_UNIQUE,
					UART3_Handler, 0);
	rtems_interrupt_vector_enable(45);
	rtems_interrupt_handler_install(46, "uart4", RTEMS_INTERRUPT_UNIQUE,
					UART4_Handler, 0);
	rtems_interrupt_vector_enable(46);
	rtems_interrupt_handler_install(23, "timer0", RTEMS_INTERRUPT_UNIQUE,
					timer_irq_handler, 0);

	Init_setup_watchdog();
	Pmc_init(&pmc, Pmc_getDeviceRegisterStartAddress());
	Pmc_enablePeripheralClk(&pmc, Pmc_PeripheralId_Tc0Ch0);

	extract_mck_frequency();

	/* Nvic_setInterruptHandlerAddress(Nvic_Irq_Timer0_Channel0,
   * timer_irq_handler); */
	/* Nvic_enableInterrupt(Nvic_Irq_Timer0_Channel0); */

	Tic_init(&tic, Tic_Id_0);
	Tic_writeProtect(&tic, false);

	Tic_ChannelConfig config = {};
	config.isEnabled = true;
	config.clockSource = Tic_ClockSelection_MckBy8;
	config.irqConfig.isCounterOverflowIrqEnabled = true;
	Tic_setChannelConfig(&tic, Tic_Channel_0, &config);

	Tic_enableChannel(&tic, Tic_Channel_0);
	Tic_triggerChannel(&tic, Tic_Channel_0);

	Init_setup_xdmad_lock();

	return true;
}

uint64_t Hal_GetElapsedTimeInNs(void)
{
	uint32_t reloads;
	uint32_t ticks;

	do {
		ConcurrentAccessFlag_reset(&reloads_modified_flag);
		reloads = __atomic_load_n(&reloads_counter, __ATOMIC_SEQ_CST);
		ticks = Tic_getCounterValue(&tic, Tic_Channel_0);
	} while (ConcurrentAccessFlag_check(&reloads_modified_flag));

	const uint64_t total_ticks =
		(uint64_t)(reloads * TICKS_PER_RELOAD) + (uint64_t)ticks;
	const double clock_frequency =
		(double)mck_frequency / CLOCK_SELECTION_PRESCALLER;

	return (uint64_t)((double)total_ticks /
			  (clock_frequency / NANOSECOND_IN_SECOND));
}

bool Hal_SleepNs(uint64_t time_ns)
{
	const double sleep_tick_count =
		time_ns * ((double)mck_frequency / NANOSECOND_IN_SECOND);

	return rtems_task_wake_after((rtems_interval)sleep_tick_count) ==
	       RTEMS_SUCCESSFUL;
}

int32_t Hal_SemaphoreCreate(void)
{
	if (created_semaphores_count >= RT_MAX_HAL_SEMAPHORES) {
		return 0;
	}

	const rtems_status_code status_code = rtems_semaphore_create(
		generate_new_hal_semaphore_name(),
		1, // Initial value, unlocked
		RTEMS_BINARY_SEMAPHORE,
		0, // Priority ceiling
		&hal_semaphore_ids[created_semaphores_count]);

	if (status_code == RTEMS_SUCCESSFUL) {
		return hal_semaphore_ids[created_semaphores_count++];
	}

	return 0;
}

bool Hal_SemaphoreObtain(int32_t id)
{
	return rtems_semaphore_obtain(id, RTEMS_WAIT, RTEMS_NO_TIMEOUT) ==
	       RTEMS_SUCCESSFUL;
}

bool Hal_SemaphoreRelease(int32_t id)
{
	return rtems_semaphore_release(id) == RTEMS_SUCCESSFUL;
}

void Hal_uart_xdmad_handler(uint32_t xdmacChannel, void *args)
{
	XDMAD_FreeChannel(&xdmad, xdmacChannel);
	Uart_TxHandler *uartTxHandler = (Uart_TxHandler *)args;
	uartTxHandler->callback(uartTxHandler->arg);
}

static inline void Hal_uart_print_uart_id(Uart_Id id)
{
	/* switch(id) { */
	/*     case Uart_Id_0: */
	/*         Hal_console_usart_write(UART_ID_UART0, strlen(UART_ID_UART0)); */
	/*         break; */
	/*     case Uart_Id_1: */
	/*         Hal_console_usart_write(UART_ID_UART1, strlen(UART_ID_UART1)); */
	/*         break; */
	/*     case Uart_Id_2: */
	/*         Hal_console_usart_write(UART_ID_UART2, strlen(UART_ID_UART2)); */
	/*         break; */
	/*     case Uart_Id_3: */
	/*         Hal_console_usart_write(UART_ID_UART3, strlen(UART_ID_UART3)); */
	/*         break; */
	/*     case Uart_Id_4: */
	/*         Hal_console_usart_write(UART_ID_UART4, strlen(UART_ID_UART4)); */
	/*         break; */
	/* } */
}

static inline void Hal_uart_error_handler(Uart_ErrorFlags errorFlags, void *arg)
{
	Hal_Uart *halUart = (Hal_Uart *)arg;

	Hal_uart_print_uart_id(halUart->uart.id);
	if (errorFlags.hasOverrunOccurred == true) {
		/* Hal_console_usart_write(UART_READ_ERROR_OVERRUN_ERROR,
     * strlen(UART_READ_ERROR_OVERRUN_ERROR)); */
	}
	if (errorFlags.hasFramingErrorOccurred == true) {
		/* Hal_console_usart_write(UART_READ_ERROR_FRAME_ERROR,
     * strlen(UART_READ_ERROR_FRAME_ERROR)); */
	}
	if (errorFlags.hasParityErrorOccurred == true) {
		/* Hal_console_usart_write(UART_READ_ERROR_PARITY_ERROR,
     * strlen(UART_READ_ERROR_PARITY_ERROR)); */
	}
	if (errorFlags.hasRxFifoFullErrorOccurred == true) {
		/* Hal_console_usart_write(UART_RX_INTERRUPT_ERROR_FIFO_FULL,
     * strlen(UART_RX_INTERRUPT_ERROR_FIFO_FULL)); */
		assert(false && "Rx FIFO is full.");
	}
}

inline static void Hal_uart_init_uart0_pio(Pio_Port *const port,
					   Pio_Port_Config *const pioConfigTx,
					   Pio_Port_Config *const pioConfigRx)
{
	*port = Pio_Port_A;

	pioConfigRx->pins = PIO_PIN_9;
	pioConfigRx->pinsConfig.control = Pio_Control_PeripheralA;

	pioConfigTx->pins = PIO_PIN_10;
	pioConfigTx->pinsConfig.control = Pio_Control_PeripheralA;
}

inline static void Hal_uart_init_uart1_pio(Pio_Port *const port,
					   Pio_Port_Config *const pioConfigTx,
					   Pio_Port_Config *const pioConfigRx)
{
	*port = Pio_Port_A;

	pioConfigRx->pins = PIO_PIN_5;
	pioConfigRx->pinsConfig.control = Pio_Control_PeripheralC;

	pioConfigTx->pins = PIO_PIN_6;
	pioConfigTx->pinsConfig.control = Pio_Control_PeripheralC;
}

inline static void Hal_uart_init_uart2_pio(Pio_Port *const port,
					   Pio_Port_Config *const pioConfigTx,
					   Pio_Port_Config *const pioConfigRx)
{
	*port = Pio_Port_D;

	pioConfigRx->pins = PIO_PIN_25;
	pioConfigRx->pinsConfig.control = Pio_Control_PeripheralC;

	pioConfigTx->pins = PIO_PIN_26;
	pioConfigTx->pinsConfig.control = Pio_Control_PeripheralC;
}

inline static void Hal_uart_init_uart3_pio(Pio_Port *const port,
					   Pio_Port_Config *const pioConfigTx,
					   Pio_Port_Config *const pioConfigRx)
{
	*port = Pio_Port_D;

	pioConfigRx->pins = PIO_PIN_28;
	pioConfigRx->pinsConfig.control = Pio_Control_PeripheralA;

	pioConfigTx->pins = PIO_PIN_30;
	pioConfigTx->pinsConfig.control = Pio_Control_PeripheralA;
}

inline static void Hal_uart_init_uart4_pio(Pio_Port *const port,
					   Pio_Port_Config *const pioConfigTx,
					   Pio_Port_Config *const pioConfigRx)
{
	*port = Pio_Port_D;

	pioConfigRx->pins = PIO_PIN_18;
	pioConfigRx->pinsConfig.control = Pio_Control_PeripheralC;

	pioConfigTx->pins = PIO_PIN_19;
	pioConfigTx->pinsConfig.control = Pio_Control_PeripheralC;
}

static inline Pmc_PeripheralId Hal_get_periph_uart_id(Uart_Id id)
{
	switch (id) {
	case Uart_Id_0:
		return Pmc_PeripheralId_Uart0;
	case Uart_Id_1:
		return Pmc_PeripheralId_Uart1;
	case Uart_Id_2:
		return Pmc_PeripheralId_Uart2;
	case Uart_Id_3:
		return Pmc_PeripheralId_Uart3;
	case Uart_Id_4:
		return Pmc_PeripheralId_Uart4;
	}
}

static inline Pmc_PeripheralId Hal_get_periph_uart_pio_id(Uart_Id id)
{
	switch (id) {
	case Uart_Id_0:
	case Uart_Id_1:
		return Pmc_PeripheralId_PioA;
	case Uart_Id_2:
	case Uart_Id_3:
	case Uart_Id_4:
		return Pmc_PeripheralId_PioD;
	}
}

static inline Uart_Id Hal_get_nvic_uart_id(Uart_Id id)
{
	switch (id) {
	case Uart_Id_0:
		return Nvic_Irq_Uart0;
	case Uart_Id_1:
		return Nvic_Irq_Uart1;
	case Uart_Id_2:
		return Nvic_Irq_Uart2;
	case Uart_Id_3:
		return Nvic_Irq_Uart3;
	case Uart_Id_4:
		return Nvic_Irq_Uart4;
	}
}

static inline void Hal_uart_init_pio(Uart_Id id)
{
	Pio_Port port;
	Pio_Port_Config pioConfigTx = {.pinsConfig =
                                     {
                                         .pull = Pio_Pull_Up,
                                         .filter = Pio_Filter_None,
                                         .isMultiDriveEnabled = false,
                                         .isSchmittTriggerDisabled = false,
                                         .irq = Pio_Irq_None,
                                         .direction = Pio_Direction_Output,
                                     },
                                 .debounceFilterDiv = 0};
	pioConfigTx.pinsConfig.direction = Pio_Direction_Output;

	Pio_Port_Config pioConfigRx = pioConfigRx;
	pioConfigRx.pinsConfig.direction = Pio_Direction_Input;

	switch (id) {
	case Uart_Id_0:
		Hal_uart_init_uart0_pio(&port, &pioConfigTx, &pioConfigRx);
		break;
	case Uart_Id_1:
		Hal_uart_init_uart1_pio(&port, &pioConfigTx, &pioConfigRx);
		break;
	case Uart_Id_2:
		Hal_uart_init_uart2_pio(&port, &pioConfigTx, &pioConfigRx);
		break;
	case Uart_Id_3:
		Hal_uart_init_uart3_pio(&port, &pioConfigTx, &pioConfigRx);
		break;
	case Uart_Id_4:
		Hal_uart_init_uart4_pio(&port, &pioConfigTx, &pioConfigRx);
		break;
	}
	Pio pio;
	ErrorCode errorCode = 0;
	Pio_init(port, &pio, &errorCode);
	Pio_setPortConfig(&pio, &pioConfigTx, &errorCode);
	Pio_setPortConfig(&pio, &pioConfigRx, &errorCode);
}

inline static void Hal_uart_init_pmc(Uart_Id id)
{
	Pmc_enablePeripheralClk(&pmc, Hal_get_periph_uart_pio_id(id));
	Pmc_enablePeripheralClk(&pmc, Hal_get_periph_uart_id(id));
}

inline static void Hal_uart_init_nvic(Uart_Id id)
{
	Nvic_enableInterrupt(Hal_get_nvic_uart_id(id));
	// TODO change to rtems API
	Nvic_setInterruptPriority(Hal_get_nvic_uart_id(id),
				  UART_INTERRUPT_PRIORITY);
}

inline static void Hal_uart_init_handle(Uart *uart, Uart_Id id)
{
	switch (id) {
	case Uart_Id_0:
		uart0handle = uart;
		break;
	case Uart_Id_1:
		uart1handle = uart;
		break;
	case Uart_Id_2:
		uart2handle = uart;
		break;
	case Uart_Id_3:
		uart3handle = uart;
		break;
	case Uart_Id_4:
		uart4handle = uart;
		break;
	}
}

static inline void Hal_uart_init_dma(void)
{
	Pmc_enablePeripheralClk(&pmc, Pmc_PeripheralId_Xdmac);

	Nvic_clearInterruptPending(Nvic_Irq_Xdmac);
	// TODO change to rtems API
	Nvic_setInterruptPriority(Nvic_Irq_Xdmac,
				  UART_XDMAC_INTERRUPT_PRIORITY);
	Nvic_enableInterrupt(Nvic_Irq_Xdmac);

	XDMAD_Initialize(&xdmad, XDMAD_NO_POLLING);
}

void Hal_subscribe_to_interrupt(Nvic_Irq irq, InterruptCallback callback)
{
	interruptSubscription[irq] = callback;
}

void Hal_uart_init(Hal_Uart *const halUart, Hal_Uart_Config halUartConfig)
{
	assert(halUartConfig.id <= Uart_Id_4);
	assert((halUartConfig.parity <= Uart_Parity_Odd) ||
	       (halUartConfig.parity == Uart_Parity_None));

	// init uart
	Hal_uart_init_pmc(halUartConfig.id);
	Hal_uart_init_pio(halUartConfig.id);
	Hal_uart_init_nvic(halUartConfig.id);
	Hal_uart_init_handle(&halUart->uart, halUartConfig.id);

	Uart_init(halUartConfig.id, &halUart->uart);
	Uart_startup(&halUart->uart);

	Uart_Config config = { .isTxEnabled = true,
			       .isRxEnabled = true,
			       .isTestModeEnabled = false,
			       .parity = halUartConfig.parity,
			       .baudRate = halUartConfig.baudrate,
			       .baudRateClkSrc = Uart_BaudRateClk_PeripheralCk,
			       .baudRateClkFreq = DEFAULT_PERIPH_CLOCK };
	Uart_setConfig(&halUart->uart, &config);

	Hal_uart_init_dma();
}

static inline void Hal_uart_write_init_xdmac_channel(
	Hal_Uart *const halUart, uint8_t *const buffer, const uint16_t length,
	const Uart_TxHandler *const txHandler, uint32_t channelNumber)
{
	eXdmadRC prepareResult = XDMAD_PrepareChannel(&xdmad, channelNumber);
	assert(prepareResult == XDMAD_OK);

	//< Get Uart Tx peripheral xdmac id
	uint32_t periphID = xdmad.XdmaChannels[channelNumber].bDstTxIfID
			    << XDMAC_CC_PERID_Pos;
	sXdmadCfg config = {
		.mbr_ubc =
			length, //< uBlock max length is equal to uart write max data
		// length. Thus one uBlock can be used.
		.mbr_sa = (uint32_t)buffer, //< Data buffer as source addres
		.mbr_da =
			(uint32_t)&halUart->uart.reg
				->thr, //< Uart tx holding register as a destination address
		.mbr_cfg =
			XDMAC_CC_TYPE_PER_TRAN | XDMAC_CC_MBSIZE_SINGLE |
			XDMAC_CC_DSYNC_MEM2PER | XDMAC_CC_SWREQ_HWR_CONNECTED |
			XDMAC_CC_MEMSET_NORMAL_MODE | XDMAC_CC_DWIDTH_BYTE |
			XDMAC_CC_SIF_AHB_IF1 | XDMAC_CC_DIF_AHB_IF1 |
			XDMAC_CC_SAM_INCREMENTED_AM | XDMAC_CC_DAM_FIXED_AM |
			periphID, //< Config memory to peripheral transfer. Increment
		// source buffer address. Keep
		// desitnation address buffer fixed
		.mbr_bc = 0, //< do not add any data stride
		.mbr_ds = 0,
		.mbr_sus = 0,
		.mbr_dus = 0,
	};

	eXdmadRC configureResult = XDMAD_ConfigureTransfer(
		&xdmad, channelNumber, &config, 0, 0,
		XDMAC_CIE_BIE | XDMAC_CIE_RBIE | XDMAC_CIE_WBIE |
			XDMAC_CIE_ROIE);
	assert(configureResult == XDMAD_OK);
	eXdmadRC callbackResult = XDMAD_SetCallback(&xdmad, channelNumber,
						    Hal_uart_xdmad_handler,
						    (void *)txHandler);
	assert(callbackResult == XDMAD_OK);
}

void Hal_uart_write(Hal_Uart *const halUart, uint8_t *const buffer,
		    const uint16_t length,
		    const Uart_TxHandler *const txHandler)
{
	uint32_t channelNumber =
		XDMAD_AllocateChannel(&xdmad, XDMAD_TRANSFER_MEMORY,
				      Hal_get_periph_uart_id(halUart->uart.id));
	if (channelNumber <
	    (xdmad.pXdmacs->XDMAC_GTYPE & XDMAC_GTYPE_NB_CH_Msk)) {
		Hal_uart_write_init_xdmac_channel(halUart, buffer, length,
						  txHandler, channelNumber);
		eXdmadRC startResult =
			XDMAD_StartTransfer(&xdmad, channelNumber);
		assert(startResult == XDMAD_OK);
	} else {
		/* Hal_console_usart_write((uint8_t*)UART_XDMAD_ERROR_NO_AVALIABLE_CHANNELS,
     */
		/*                         strlen(UART_XDMAD_ERROR_NO_AVALIABLE_CHANNELS));
     */
	}
}

void Hal_uart_read(Hal_Uart *const halUart, uint8_t *const buffer,
		   const uint16_t length, const Uart_RxHandler rxHandler)
{
	Uart_ErrorHandler errorHandler = { .callback = Hal_uart_error_handler,
					   .arg = halUart };
	ByteFifo_init(&halUart->rxFifo, buffer, length);
	Uart_registerErrorHandler(&halUart->uart, errorHandler);
	Uart_readAsync(&halUart->uart, &halUart->rxFifo, rxHandler);
}

static inline void Hal_console_usart_init_pio(void)
{
	Pio pioB;
	ErrorCode errorCode = 0;
	Pio_init(Pio_Port_B, &pioB, &errorCode);

	Pio_Pin_Config pinConf;
	pinConf.control = Pio_Control_PeripheralD;
	pinConf.direction = Pio_Direction_Input;
	pinConf.pull = Pio_Pull_None;
	pinConf.filter = Pio_Filter_None;
	pinConf.isMultiDriveEnabled = false;
	pinConf.isSchmittTriggerDisabled = false;
	pinConf.irq = Pio_Irq_None;

	Pio_setPinsConfig(&pioB, PIO_PIN_4, &pinConf, &errorCode);
}

static inline void Hal_console_usart_init_pmc(void)
{
	Pmc_enablePeripheralClk(&pmc, Pmc_PeripheralId_PioB);
	Pmc_enablePeripheralClk(&pmc, Pmc_PeripheralId_Usart1);
}

static inline void Hal_console_usart_init_mode(void)
{
	// cppcheck-suppress misra2012_11_4
	uint32_t *US_MR = (uint32_t *)USART1_MR_ADDRESS;

	*US_MR = 0; // Clear
	*US_MR |= USART_MR_MODE_NORMAL
		  << USART_MR_MODE_OFFSET; // UART_MODE NORMAL
	*US_MR |= USART_MR_USCLKS_MCK << USART_MR_USCLKS_OFFSET; // USCLKS PCK
	*US_MR |= USART_MR_CHRL_8BIT << USART_MR_CHRL_OFFSET; // CHRL 8BIT
	*US_MR |= USART_MR_SYNC_ASYNCHRONOUS
		  << USART_MR_SYNC_OFFSET; // SYNC Asynchronous
	*US_MR |= USART_MR_PAR_NO << UART_MR_PAR_OFFSET; // PAR No parity
	*US_MR |= USART_MR_NBSTOP_1_BIT
		  << USART_MR_NBSTOP_OFFSET; // NBSTOP 1 stop bit
	*US_MR |= USART_MR_CHMODE_NORMAL
		  << UART_MR_CHMODE_OFFSET; // CHMODE Normal mode
	*US_MR |= USART_MR_MSBF_LSB << USART_MR_MSBF_OFFSET; // MSBF MSB
	*US_MR |= USART_MR_MODE9_CHRL
		  << USART_MR_MODE9_OFFSET; // MODE9 CHRL defines length
	*US_MR |= USART_MR_CLKO_NO_SCK
		  << USART_MR_CLKO_OFFSET; // CLKO does not drive SCK pin
	*US_MR |= USART_MR_OVER_16X
		  << USART_MR_OVER_OFFSET; // OVER 16x oversampling
	*US_MR |= USART_MR_INACK_NOT_GEN
		  << USART_MR_INACK_OFFSET; // INACK NSCK is not generated
	*US_MR |= USART_MR_DSNACK_NO_NACK
		  << USART_MR_DSNACK_OFFSET; // DSNACK don't care - no NACK
	*US_MR |= USART_MR_VAR_SYNC_DISABLE
		  << USART_MR_VAR_SYNC_OFFSET; // VAR_SYNC MODSYNC defines sync
	*US_MR |= USART_MR_INVDATA_DISABLED
		  << USART_MR_INVDATA_OFFSET; // INVDATA do not invert data
	*US_MR |=
		USART_MR_MAX_ITERATION_DISABLE
		<< USART_MR_MAX_ITERATION_OFFSET; // MAX_ITERATION don't care -
	// valid in ISO7816 protocol
	*US_MR |=
		USART_MR_FILTER_DISABLE
		<< USART_MR_FILTER_OFFSET; // FILTER do not filter incomming data
	*US_MR |= USART_MR_MAN_DISABLE
		  << USART_MR_MAN_OFFSET; // MAN manchester coding disabled
	*US_MR |=
		USART_MR_MODSYNC_DISABLE
		<< USART_MR_MODSYNC_OFFSET; // MODSYNC don't care - manchester disabled
	*US_MR |=
		USART_MR_ONEBIT_1_BIT
		<< USART_MR_ONEBIT_OFFSET; // ONEBIT 1 bit start frame delimiterx
}

static inline void Hal_console_usart_init_baudrate()
{
	// cppcheck-suppress misra2012_11_4
	uint32_t *US_BRGR = (uint32_t *)USART1_BRGR_ADDRESS;
	// BaudRate = CLK / ((coarseDiv + fineDiv / 8) * 16)
	uint32_t coarseDiv = DEFAULT_PERIPH_CLOCK / (16u * USART_BAUD_RATE);
	uint64_t fineDiv = 8uLL * (((uint64_t)DEFAULT_PERIPH_CLOCK * 1000uLL /
				    (16uLL * USART_BAUD_RATE)) -
				   (uint64_t)coarseDiv * 1000uLL);
	fineDiv /= 1000uLL;

	*US_BRGR = coarseDiv |
		   ((uint32_t)fineDiv << USART_BRGR_FINE_DIV_OFFSET);
}

static inline void Hal_console_usart_init_bus_matrix()
{
	// cppcheck-suppress misra2012_11_4
	uint32_t *CCFG_SYSIO = (uint32_t *)MATRIX_CCFG_SYSIO_ADDR;
	// Assign the PB4 pin to the PIO controller (TDI is the default function)
	*CCFG_SYSIO |= MATRIX_CCFG_SYSIO_PB4_SELECTED
		       << MATRIX_CCFG_SYSIO_SYSIO4_OFFSET;
}

static inline void Hal_console_usart_init_tx_enable()
{
	// cppcheck-suppress misra2012_11_4
	uint32_t *US_CR = (uint32_t *)USART1_CR_ADDRESS;
	*US_CR = USART_CR_TXEN_ENABLE
		 << UART_CR_TXEN_OFFSET; // Enable the transmitter
}

void Hal_console_usart_init(void)
{
	Hal_console_usart_init_bus_matrix();
	Hal_console_usart_init_pmc();
	Hal_console_usart_init_pio();

	Hal_console_usart_init_mode();
	Hal_console_usart_init_baudrate();
	Hal_console_usart_init_tx_enable();
}

static inline void waitForTransmitterReady(void)
{
	volatile uint32_t *const US_CSR = (uint32_t *)USART1_CSR_ADDRESS;
	while (((*US_CSR) & USART_CSR_TXRDY_MASK) == 0)
		asm volatile("nop");
}

static inline void writeByte(const uint8_t data)
{
	waitForTransmitterReady();

	volatile uint32_t *const US_THR = (uint32_t *)USART1_THR_ADDRESS;
	*US_THR = data;
}

/* void */
/* Hal_console_usart_write(const uint8_t* const buffer, const uint16_t count) */
/* { */
/*     for(uint32_t i = 0; i < count; i++) { */
/*         writeByte(buffer[i]); */
/*     } */

/*     waitForTransmitterReady(); */
/* } */
