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

#include <DeathReportWriter.h>
#include <DeathReport.h>

static void save_stack(DeathReportWriter_DeathReport *const death_report)
{
	const uint32_t stack_size = DEATH_REPORT_STACK_TRACE_SIZE;
	death_report->stack_trace_length =
		stack_size <= DEATH_REPORT_STACK_TRACE_SIZE ? stack_size :
								    stack_size;
	const uint32_t *const stack =
		(const uint32_t *)death_report->stack_trace_pointer;
	for (uint32_t i = 0; i < death_report->stack_trace_length; i++) {
		death_report->stack_trace[i] = stack[i];
	}
}

static uint16_t calculate_crc(const uint8_t *const data, const size_t length)
{
	uint16_t crc = 0xFFFF;  // initial value

    for (int i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }

    return crc;
}

static uint16_t calculate_report_crc(const void *const report,
				     const size_t total_size)
{
	const uint8_t *const address_without_crc =
		(const uint8_t *)report + sizeof(uint16_t);
	const size_t size_without_crc = total_size - sizeof(uint16_t);

	return calculate_crc(address_without_crc, size_without_crc);
}

bool DeathReportWriter_Init()
{
	return true;
}

__attribute__((noinline, aligned(8)))
bool DeathReportWriter_GenerateDeathReport()
{
	// SAMRH71 BSW boot report address
	void *const boot_report = (void*)0x2045F968; 

	DeathReportWriter_DeathReport *const death_report =
		boot_report + DEATH_REPORT_OFFSET;
	save_stack(death_report);

	const uint16_t crc = calculate_report_crc(death_report, sizeof(DeathReportWriter_DeathReport));

	death_report->padding = 0u;
	death_report->was_seen = false;
	death_report->checksum = crc;

	return true;
}
