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

#include "SamV71Core.h"

#include <stdint.h>
#include <Pmc/Pmc.h>

#define MEGA_HZ 1000000u
#ifndef MAIN_CRYSTAL_OSCILLATOR_FREQUNECY
#define MAIN_CRYSTAL_OSCILLATOR_FREQUNECY (12 * MEGA_HZ)
#endif

// xdmad.c requires global pmc
Pmc pmc;
static uint64_t mck_frequency = 0;

static void extract_main_oscilator_frequency(void)
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

static void apply_plla_config(Pmc_MasterckConfig *master_clock_config)
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

static void extract_mck_frequency(void)
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

void SamV71Core_Init(void)
{
	Pmc_init(&pmc, Pmc_getDeviceRegisterStartAddress());

	extract_mck_frequency();
}

void SamV71Core_EnablePeripheralClock(const Pmc_PeripheralId peripheralId)
{
	Pmc_enablePeripheralClk(&pmc, peripheralId);
}

uint64_t SamV71Core_GetMainClockFrequency(void)
{
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
