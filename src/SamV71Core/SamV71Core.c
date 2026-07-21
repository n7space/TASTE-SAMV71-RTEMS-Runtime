/**@file
 * This file is part of the TASTE SAMV71 RTEMS Runtime.
 *
 * @copyright 2025-2026 N7 Space Sp. z o.o.
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

#include "SamV71Core.h"

#include <stdint.h>
#include <assert.h>

#include <rtems/config.h>

#include <Utils/ErrorCode.h>
#include <Systick/Systick.h>
#include <Pmc/Pmc.h>
#include <Mpu/Mpu.h>

#define MEGA_HZ 1000000u
#ifndef MAIN_CRYSTAL_OSCILLATOR_FREQUENCY
#define MAIN_CRYSTAL_OSCILLATOR_FREQUENCY (12 * MEGA_HZ)
#endif

extern void setCoreClockFrequency(uint64_t frequency);

// xdmad.c requires global pmc
Pmc pmc;
static Mpu mpu;

static uint64_t extract_main_oscillator_frequency(void)
{
	// Derive the base clock from the PMC main-clock source configuration.
	Pmc_MainckConfig main_clock_config;
	Pmc_getMainckConfig(&pmc, &main_clock_config);

	if (main_clock_config.src == Pmc_MainckSrc_XOsc) {
		return MAIN_CRYSTAL_OSCILLATOR_FREQUENCY;
	}

	switch (main_clock_config.rcOscFreq) {
	case Pmc_RcOscFreq_4M: {
		return 4 * MEGA_HZ;
	}
	case Pmc_RcOscFreq_8M: {
		return 8 * MEGA_HZ;
	}
#if defined(N7S_TARGET_SAMV71Q21)
	case Pmc_RcOscFreq_12M: {
		return 12 * MEGA_HZ;
	}
#elif defined(N7S_TARGET_SAMRH71F20) || defined(N7S_TARGET_SAMRH707F18)
	case Pmc_RcOscFreq_10M: {
		return 10 * MEGA_HZ;
	}
	case Pmc_RcOscFreq_12M: {
		return 12 * MEGA_HZ;
	}
#endif
	}

	return 0;
}

static void apply_plla_config(Pmc_MasterckConfig *master_clock_config,
			      uint64_t *mck_frequency)
{
	// Fold the PLLA settings into the clock frequency when PLLACK is selected.
	if (master_clock_config->src == Pmc_MasterckSrc_Pllack) {
		Pmc_PllConfig pll_config;
		Pmc_getPllConfig(&pmc, &pll_config);

		if (pll_config.pllaDiv > 0 && pll_config.pllaMul > 0) {
			*mck_frequency = (*mck_frequency / pll_config.pllaDiv) *
					 (pll_config.pllaMul + 1);
		} else if (pll_config.pllaDiv == 0 && pll_config.pllaMul > 0) {
			*mck_frequency =
				*mck_frequency * (pll_config.pllaMul + 1);
		} else if (pll_config.pllaDiv > 0 && pll_config.pllaMul == 0) {
			*mck_frequency = *mck_frequency / pll_config.pllaDiv;
		}
	}
}

uint64_t SamV71Core_GetProcessorClockFrequency(void)
{
	Pmc_MasterckConfig master_clock_config;
	Pmc_getMasterckConfig(&pmc, &master_clock_config);

	uint64_t mck_frequency = extract_main_oscillator_frequency();
	apply_plla_config(&master_clock_config, &mck_frequency);

	switch (master_clock_config.presc) {
	case Pmc_MasterckPresc_1: {
		break;
	}
	case Pmc_MasterckPresc_2: {
		mck_frequency = mck_frequency / 2;
		break;
	}
	case Pmc_MasterckPresc_3: {
		mck_frequency = mck_frequency / 3;
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
	}
	return mck_frequency;
}

#if defined(SAMV71_CPU_FREQUENCY_150MHZ) ||     \
	defined(SAMV71_CPU_FREQUENCY_120MHZ) || \
	defined(SAMV71_CPU_FREQUENCY_100MHZ) || \
	defined(SAMV71_CPU_FREQUENCY_75MHZ) ||  \
	defined(SAMV71_CPU_FREQUENCY_50MHZ) ||  \
	defined(SAMV71_CPU_FREQUENCY_25MHZ)
#if defined(RT_RTOS_NO_INIT)
#error "Processor frequency cannot be defined together with RT_RTOS_NO_INIT"
#endif
#if defined(SAMV71_PLLA_MULTIPLIER) || defined(SAMV71_PLLA_DIVIDER) || \
	defined(SAMV71_RCOSC_FREQUENCY_4) ||                           \
	defined(SAMV71_RCOSC_FREQUENCY_8) ||                           \
	defined(SAMV71_RCOSC_FREQUENCY_12) ||                          \
	defined(SAMV71_MAIN_CLOCK_SOURCE_RCOSC) ||                     \
	defined(SAMV71_MAIN_CLOCK_SOURCE_XOSC) ||                      \
	defined(SAMV71_MAIN_CLOCK_SOURCE_XOSC_BYPASSED) ||             \
	defined(SAMV71_MASTER_CLOCK_SOURCE_SLCK) ||                    \
	defined(SAMV71_MASTER_CLOCK_SOURCE_MAINCK) ||                  \
	defined(SAMV71_MASTER_CLOCK_SOURCE_PLLACK) ||                  \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_1) ||                    \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_2) ||                    \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_3) ||                    \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_4) ||                    \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_8) ||                    \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_16) ||                   \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_32) ||                   \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_64) ||                   \
	defined(SAMV71_MASTER_CLOCK_DIVIDER_1) ||                      \
	defined(SAMV71_MASTER_CLOCK_DIVIDER_2)
#error "Low level configuration cannot be used when setting processor frequency directly"
#endif
#endif

#if defined(SAMV71_CPU_FREQUENCY_150MHZ)
#define SAMV71_RCOSC_FREQUENCY_12
#define SAMV71_MAIN_CLOCK_SOURCE_RCOSC
#define SAMV71_PLLA_MULTIPLIER 24
#define SAMV71_PLLA_DIVIDER 1 // PLLA 300 MHz
#define SAMV71_MASTER_CLOCK_SOURCE_PLLACK
#define SAMV71_MASTER_CLOCK_PRESCALER_2
#define SAMV71_MASTER_CLOCK_DIVIDER_2
#elif defined(SAMV71_CPU_FREQUENCY_120MHZ)
#define SAMV71_RCOSC_FREQUENCY_12
#define SAMV71_MAIN_CLOCK_SOURCE_RCOSC
#define SAMV71_PLLA_MULTIPLIER 29
#define SAMV71_PLLA_DIVIDER 1 // PLLA 360 MHz
#define SAMV71_MASTER_CLOCK_SOURCE_PLLACK
#define SAMV71_MASTER_CLOCK_PRESCALER_3
#define SAMV71_MASTER_CLOCK_DIVIDER_2
#elif defined(SAMV71_CPU_FREQUENCY_100MHZ)
#define SAMV71_RCOSC_FREQUENCY_12
#define SAMV71_MAIN_CLOCK_SOURCE_RCOSC
#define SAMV71_PLLA_MULTIPLIER 24
#define SAMV71_PLLA_DIVIDER 1 // PLLA 300 MHz
#define SAMV71_MASTER_CLOCK_SOURCE_PLLACK
#define SAMV71_MASTER_CLOCK_PRESCALER_3
#define SAMV71_MASTER_CLOCK_DIVIDER_2
#elif defined(SAMV71_CPU_FREQUENCY_75MHZ)
#define SAMV71_RCOSC_FREQUENCY_12
#define SAMV71_MAIN_CLOCK_SOURCE_RCOSC
#define SAMV71_PLLA_MULTIPLIER 24
#define SAMV71_PLLA_DIVIDER 1 // PLLA 300 MHz
#define SAMV71_MASTER_CLOCK_SOURCE_PLLACK
#define SAMV71_MASTER_CLOCK_PRESCALER_4
#define SAMV71_MASTER_CLOCK_DIVIDER_2
#elif defined(SAMV71_CPU_FREQUENCY_50MHZ)
#define SAMV71_RCOSC_FREQUENCY_12
#define SAMV71_MAIN_CLOCK_SOURCE_RCOSC
#define SAMV71_PLLA_MULTIPLIER 24
#define SAMV71_PLLA_DIVIDER 2 // PLLA 150 MHz
#define SAMV71_MASTER_CLOCK_SOURCE_PLLACK
#define SAMV71_MASTER_CLOCK_PRESCALER_3
#define SAMV71_MASTER_CLOCK_DIVIDER_2
#elif defined(SAMV71_CPU_FREQUENCY_25MHZ)
#define SAMV71_RCOSC_FREQUENCY_12
#define SAMV71_MAIN_CLOCK_SOURCE_RCOSC
#define SAMV71_PLLA_MULTIPLIER 24
#define SAMV71_PLLA_DIVIDER 4 // PLLA 75 MHz
#define SAMV71_MASTER_CLOCK_SOURCE_PLLACK
#define SAMV71_MASTER_CLOCK_PRESCALER_3
#define SAMV71_MASTER_CLOCK_DIVIDER_1
#endif

#if defined(SAMV71_PLLA_MULTIPLIER)
#if defined(RT_RTOS_NO_INIT)
#error "SAMV71_PLLA_MULTIPLIER cannot be defined together with RT_RTOS_NO_INIT"
#endif
#if SAMV71_PLLA_MULTIPLIER < 0 || SAMV71_PLLA_MULTIPLIER > 62
#error "SAMV71_PLLA_MULTIPLIER shall be between 0 and 62"
#endif
#else
#define SAMV71_PLLA_MULTIPLIER 24
#endif

#if defined(SAMV71_PLLA_DIVIDER)
#if defined(RT_RTOS_NO_INIT)
#error "SAMV71_PLLA_DIVIDER cannot be defined together with RT_RTOS_NO_INIT"
#endif
#if SAMV71_PLLA_DIVIDER < 0 || SAMV71_PLLA_DIVIDER > 255
#error "SAMV71_PLLA_DIVIDER shall be between 0 and 255"
#endif
#else
#define SAMV71_PLLA_DIVIDER 1
#endif

#if defined(SAMV71_PLLA_STARTUP_TIME)
#if defined(RT_RTOS_NO_INIT)
#error "SAMV71_PLLA_STARTUP_TIME cannot be defined together with RT_RTOS_NO_INIT"
#endif
#if SAMV71_PLLA_STARTUP_TIME < 0 || SAMV71_PLLA_STARTUP_TIME > 63
#error "SAMV71_PLLA_STARTUP_TIME shall be between 0 and 63"
#endif
#else
#define SAMV71_PLLA_STARTUP_TIME 60
#endif

#if defined(SAMV71_RCOSC_XOSC_STARTUP_TIME)
#if defined(RT_RTOS_NO_INIT)
#error "SAMV71_RCOSC_XOSC_STARTUP_TIME cannot be defined together with RT_RTOS_NO_INIT"
#endif
#if SAMV71_RCOSC_XOSC_STARTUP_TIME < 0 || SAMV71_RCOSC_XOSC_STARTUP_TIME > 2040
#error "SAMV71_RCOSC_XOSC_STARTUP_TIME shall be between 0 and 2040"
#endif
#if SAMV71_RCOSC_XOSC_STARTUP_TIME % 8 != 0
#error "SAMV71_RCOSC_XOSC_STARTUP_TIME shall be multiple of 8"
#endif
#else
#define SAMV71_RCOSC_XOSC_STARTUP_TIME 0
#endif

#if defined(SAMV71_RCOSC_FREQUENCY)
#undef SAMV71_RCOSC_FREQUENCY
#endif

#if defined(SAMV71_RCOSC_FREQUENCY_4) || defined(SAMV71_RCOSC_FREQUENCY_8) || \
	defined(SAMV71_RCOSC_FREQUENCY_12)
#if defined(RT_RTOS_NO_INIT)
#error "SAMV71_RCOSC_FREQUENCY_* cannot be defined together with RT_RTOS_NO_INIT"
#endif
#endif

#if defined(SAMV71_RCOSC_FREQUENCY_4)
#if defined(SAMV71_RCOSC_FREQUENCY_8) || defined(SAMV71_RCOSC_FREQUENCY_12)
#error "Only one of the macros SAMV71_RCOSC_FREQUENCY_* shall be defined at once."
#endif
#define SAMV71_RCOSC_FREQUENCY Pmc_RcOscFreq_4M
#elif defined(SAMV71_RCOSC_FREQUENCY_8)
#if defined(SAMV71_RCOSC_FREQUENCY_12)
#error "Only one of the macros SAMV71_RCOSC_FREQUENCY_* shall be defined at once."
#endif
#define SAMV71_RCOSC_FREQUENCY Pmc_RcOscFreq_8M
#elif defined(SAMV71_RCOSC_FREQUENCY_12)
#define SAMV71_RCOSC_FREQUENCY Pmc_RcOscFreq_12M
#else
#define SAMV71_RCOSC_FREQUENCY Pmc_RcOscFreq_12M
#endif

#if defined(SAMV71_MAIN_CLOCK_SOURCE)
#undef SAMV71_MAIN_CLOCK_SOURCE
#endif

#if defined(SAMV71_MAIN_CLOCK_SOURCE_RCOSC) ||    \
	defined(SAMV71_MAIN_CLOCK_SOURCE_XOSC) || \
	defined(SAMV71_MAIN_CLOCK_SOURCE_XOSC_BYPASSED)
#if defined(RT_RTOS_NO_INIT)
#error "SAMV71_MAIN_CLOCK_SOURCE_* cannot be defined together with RT_RTOS_NO_INIT"
#endif
#endif

#if defined(SAMV71_MAIN_CLOCK_SOURCE_RCOSC)
#if defined(SAMV71_MAIN_CLOCK_SOURCE_XOSC) || \
	defined(SAMV71_MAIN_CLOCK_SOURCE_XOSC_BYPASSED)
#error "Only one of the macros SAMV71_MAIN_CLOCK_SOURCE_* shall be defined at once."
#endif
#define SAMV71_MAIN_CLOCK_SOURCE Pmc_MainckSrc_RcOsc
#elif defined(SAMV71_MAIN_CLOCK_SOURCE_XOSC)
#if defined(SAMV71_MAIN_CLOCK_SOURCE_XOSC_BYPASSED)
#error "Only one of the macros SAMV71_MAIN_CLOCK_SOURCE_* shall be defined at once."
#endif
#define SAMV71_MAIN_CLOCK_SOURCE Pmc_MainckSrc_XOsc
#elif defined(SAMV71_MAIN_CLOCK_SOURCE_XOSC_BYPASSED)
#define SAMV71_MAIN_CLOCK_SOURCE Pmc_MainckSrc_XOscBypassed
#else
#define SAMV71_MAIN_CLOCK_SOURCE Pmc_MainckSrc_RcOsc
#endif

#if defined(SAMV71_MASTER_CLOCK_SOURCE)
#undef SAMV71_MASTER_CLOCK_SOURCE
#endif

#if defined(SAMV71_MASTER_CLOCK_SOURCE_SLCK) ||       \
	defined(SAMV71_MASTER_CLOCK_SOURCE_MAINCK) || \
	defined(SAMV71_MASTER_CLOCK_SOURCE_PLLACK)
#if defined(RT_RTOS_NO_INIT)
#error "SAMV71_MASTER_CLOCK_SOURCE_* cannot be defined together with RT_RTOS_NO_INIT"
#endif
#endif

#if defined(SAMV71_MASTER_CLOCK_SOURCE_SLCK)
#define SAMV71_MASTER_CLOCK_SOURCE Pmc_MasterckSrc_Slck
#if defined(SAMV71_MASTER_CLOCK_SOURCE_MAINCK) || \
	defined(SAMV71_MASTER_CLOCK_SOURCE_PLLACK)
#error "Only one of the macros SAMV71_MASTER_CLOCK_SOURCE_* shall be defined at once."
#endif
#elif defined(SAMV71_MASTER_CLOCK_SOURCE_MAINCK)
#if defined(SAMV71_MASTER_CLOCK_SOURCE_PLLACK)
#error "Only one of the macros SAMV71_MASTER_CLOCK_SOURCE_* shall be defined at once."
#endif
#define SAMV71_MASTER_CLOCK_SOURCE Pmc_MasterckSrc_Mainck
#elif defined(SAMV71_MASTER_CLOCK_SOURCE_PLLACK)
#define SAMV71_MASTER_CLOCK_SOURCE Pmc_MasterckSrc_Pllack
#else
#define SAMV71_MASTER_CLOCK_SOURCE Pmc_MasterckSrc_Pllack
#endif

#if defined(SAMV71_MASTER_CLOCK_PRESCALER)
#undef SAMV71_MASTER_CLOCK_PRESCALER
#endif

#if defined(SAMV71_MASTER_CLOCK_PRESCALER_1) ||      \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_2) ||  \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_3) ||  \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_4) ||  \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_8) ||  \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_16) || \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_32) || \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_64)
#if defined(RT_RTOS_NO_INIT)
#error "SAMV71_MASTER_CLOCK_PRESCALER_* cannot be defined together with RT_RTOS_NO_INIT"
#endif
#endif

#if defined(SAMV71_MASTER_CLOCK_PRESCALER_1)
#if defined(SAMV71_MASTER_CLOCK_PRESCALER_2) ||      \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_3) ||  \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_4) ||  \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_8) ||  \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_16) || \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_32) || \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_64)
#error "Only one of the macros SAMV71_MASTER_CLOCK_PRESCALER_* shall be defined at once."
#endif
#define SAMV71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_1
#elif defined(SAMV71_MASTER_CLOCK_PRESCALER_2)
#if defined(SAMV71_MASTER_CLOCK_PRESCALER_3) ||      \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_4) ||  \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_8) ||  \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_16) || \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_32) || \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_64)
#error "Only one of the macros SAMV71_MASTER_CLOCK_PRESCALER_* shall be defined at once."
#endif
#define SAMV71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_2
#elif defined(SAMV71_MASTER_CLOCK_PRESCALER_3)
#if defined(SAMV71_MASTER_CLOCK_PRESCALER_4) ||      \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_8) ||  \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_16) || \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_32) || \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_64)
#error "Only one of the macros SAMV71_MASTER_CLOCK_PRESCALER_* shall be defined at once."
#endif
#define SAMV71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_3
#elif defined(SAMV71_MASTER_CLOCK_PRESCALER_4)
#if defined(SAMV71_MASTER_CLOCK_PRESCALER_8) ||      \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_16) || \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_32) || \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_64)
#error "Only one of the macros SAMV71_MASTER_CLOCK_PRESCALER_* shall be defined at once."
#endif
#define SAMV71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_4
#elif defined(SAMV71_MASTER_CLOCK_PRESCALER_8)
#if defined(SAMV71_MASTER_CLOCK_PRESCALER_16) ||     \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_32) || \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_64)
#error "Only one of the macros SAMV71_MASTER_CLOCK_PRESCALER_* shall be defined at once."
#endif
#define SAMV71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_8
#elif defined(SAMV71_MASTER_CLOCK_PRESCALER_16)
#if defined(SAMV71_MASTER_CLOCK_PRESCALER_32) || \
	defined(SAMV71_MASTER_CLOCK_PRESCALER_64)
#error "Only one of the macros SAMV71_MASTER_CLOCK_PRESCALER_* shall be defined at once."
#endif
#define SAMV71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_16
#elif defined(SAMV71_MASTER_CLOCK_PRESCALER_32)
#if defined(SAMV71_MASTER_CLOCK_PRESCALER_64)
#error "Only one of the macros SAMV71_MASTER_CLOCK_PRESCALER_* shall be defined at once."
#endif
#define SAMV71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_32
#elif defined(SAMV71_MASTER_CLOCK_PRESCALER_64)
#define SAMV71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_64
#else
#define SAMV71_MASTER_CLOCK_PRESCALER Pmc_MasterckPresc_2
#endif

#if defined(SAMV71_MASTER_CLOCK_DIVIDER)
#undef SAMV71_MASTER_CLOCK_DIVIDER
#endif

#if defined(SAMV71_MASTER_CLOCK_DIVIDER_1) || \
	defined(SAMV71_MASTER_CLOCK_DIVIDER_2)
#if defined(RT_RTOS_NO_INIT)
#error "SAMV71_MASTER_CLOCK_DIVIDER_* cannot be defined together with RT_RTOS_NO_INIT"
#endif
#endif

#if defined(SAMV71_MASTER_CLOCK_DIVIDER_1)
#if defined(SAMV71_MASTER_CLOCK_DIVIDER_2)
#error "Only one of SAMV71_MASTER_CLOCK_DIVIDER_1, SAMV71_MASTER_CLOCK_DIVIDER_2 shall be defined at once."
#endif
#define SAMV71_MASTER_CLOCK_DIVIDER Pmc_MasterckDiv_1
#elif defined(SAMV71_MASTER_CLOCK_DIVIDER_2)
#define SAMV71_MASTER_CLOCK_DIVIDER Pmc_MasterckDiv_2
#else
#define SAMV71_MASTER_CLOCK_DIVIDER Pmc_MasterckDiv_2
#endif

void SamV71Core_Init(void)
{
	Pmc_init(&pmc, Pmc_getDeviceRegisterStartAddress());
	Mpu_init(&mpu);
	Mpu_Config mpuConf = { .isEnabled = true,
			       .isDefaultMemoryMapEnabled = true,
			       .isMpuEnabledInHandlers = true };
	Mpu_setConfig(&mpu, &mpuConf);

#ifndef RT_RTOS_NO_INIT
	// Configure RC Oscillator as source for main clock.
	// Configure PLLA and master clock.
	const Pmc_Config pmcConfig = {
		.mainck = { .src = SAMV71_MAIN_CLOCK_SOURCE,
			    .rcOscFreq = SAMV71_RCOSC_FREQUENCY,
			    .xoscStartupTime = SAMV71_RCOSC_XOSC_STARTUP_TIME },
		.pll = { .pllaMul = SAMV71_PLLA_MULTIPLIER,
			 .pllaDiv = SAMV71_PLLA_DIVIDER,
			 .pllaStartupTime = SAMV71_PLLA_STARTUP_TIME },
		.masterck = { .src = SAMV71_MASTER_CLOCK_SOURCE,
			      .presc = SAMV71_MASTER_CLOCK_PRESCALER,
			      .divider = SAMV71_MASTER_CLOCK_DIVIDER },
		.pck = { { .isEnabled = false,
			   .src = Pmc_PckSrc_Slck,
			   .presc = 0 } },
	};

	ErrorCode errCode = ErrorCode_NoError;
	const bool isSettingConfigSuccessful =
		Pmc_setConfig(&pmc, &pmcConfig, 1000000u, &errCode);
	assert(isSettingConfigSuccessful && "Cannot configure PMC");
#endif

	uint64_t coreFrequency = SamV71Core_GetProcessorClockFrequency();
	setCoreClockFrequency(coreFrequency);

	uint32_t systickReloadValue =
		(uint32_t)((coreFrequency *
			    rtems_configuration_get_microseconds_per_tick()) /
			   1000000u);

	Systick systick;
	Systick_init(&systick, Systick_getDeviceRegisterStartAddress());
	Systick_Config systickConfig;
	Systick_getConfig(&systick, &systickConfig);
	systickConfig.reloadValue = systickReloadValue;

	Systick_setConfig(&systick, &systickConfig);
}

void SamV71Core_EnablePeripheralClock(const Pmc_PeripheralId peripheralId)
{
	Pmc_enablePeripheralClk(&pmc, peripheralId);
	const bool is_enabled = Pmc_isPeripheralClkEnabled(&pmc, peripheralId);
	assert(is_enabled && "Peripheral clock not enabled");
}

uint64_t SamV71Core_GetMainClockFrequency(void)
{
	Pmc_MasterckConfig master_clock_config;
	Pmc_getMasterckConfig(&pmc, &master_clock_config);

	uint64_t mck_frequency = SamV71Core_GetProcessorClockFrequency();

	switch (master_clock_config.divider) {
	case Pmc_MasterckDiv_1: {
		break;
	}
	case Pmc_MasterckDiv_2: {
		mck_frequency = mck_frequency / 2;
		break;
	}
	}

	return mck_frequency;
}

void SamV71Core_InterruptSubscribe(const rtems_vector_number vector,
				   const char *info,
				   rtems_interrupt_handler handler,
				   void *handler_arg)
{
	rtems_interrupt_handler_install(vector, info, RTEMS_INTERRUPT_UNIQUE,
					handler, handler_arg);
	rtems_interrupt_vector_enable(vector);
}

rtems_name SamV71Core_GenerateNewSemaphoreName(void)
{
	static rtems_name name = rtems_build_name('C', 0, 0, 0);
	return name++;
}

rtems_name SamV71Core_GenerateNewTaskName(void)
{
	static rtems_name name = rtems_build_name('D', 0, 0, 0);
	return name++;
}

bool SamV71Core_SetPckConfig(const Pmc_PckId id,
			     const Pmc_PckConfig *const config,
			     const uint32_t timeout, ErrorCode *const errCode)
{
	return Pmc_setPckConfig(&pmc, id, config, timeout, errCode);
}

void SamV71Core_DisableDataCacheInRegion(void *address, size_t sizeExponent)
{
	// At this moment it is used by can driver,
	// where the Mpu_RegionMemoryType_StronglyOrdered breaks the driver:
	// - the BSP uses memcpy from newlib to copy data from/to message ram
	// - if the memcpy has to copy 3 bytes, it copies one byte and then two next bytes
	//    using only one arm instruction `ldrh`, but the address of two next bytes is odd,
	// - the processor raises exception due to unaligned access.
	assert(((uint32_t)address & (~MPU_RBAR_ADDR_MASK)) ==
	       0); // verify proper alignment of address
	assert(sizeExponent >= 4); // exponents less than 4 are reserved
	assert(sizeExponent <=
	       31); // maximum exponent is 31 which defines 4GB region size

	// The Mpu allows to define 16 regions, where the higher region number has
	// higher priority. Regions can overlap, therefore the small regions with
	// disabled cache shall have higher priority. The default memory map defines
	// 11 regions, from 0 to 11 so these region shall be left intact.
	static uint8_t region = 15; // start from with highest priority
	assert(region >
	       11); // verify if region does not overwrite default memory map
	Mpu_RegionConfig mpuRegionConf = {
		.address = (uint32_t)address,
		.isEnabled = true,
		.size = sizeExponent,
		.subregionDisableMask = 0x00,
		.isShareable = false,
		.isExecutable = false,
		.memoryType = Mpu_RegionMemoryType_Normal,
		.innerCachePolicy = Mpu_RegionCachePolicy_NonCacheable,
		.outerCachePolicy = Mpu_RegionCachePolicy_NonCacheable,
		.privilegedAccess = Mpu_RegionAccess_ReadWrite,
		.unprivilegedAccess = Mpu_RegionAccess_ReadWrite,
	};
	Mpu_setRegionConfig(&mpu, region, &mpuRegionConf);
	--region; // decrease region number, so next call will define a new one
}
