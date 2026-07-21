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

#define CRC_INITIAL_VALUE 0xFFFF
#define CRC_POLYNOMIAL 0x1021
#define CRC_MOST_SYGNIFICANT_BIT 0x8000

extern const uint32_t bsp_section_rtemsstack_end;
extern DeathReportWriter_DeathReport DEATH_REPORT_BEGIN;

static void save_stack(DeathReportWriter_DeathReport *const death_report)
{
	/* Copy the active stack region into the report, capped to the report buffer.
	 *
	 * Calculate available stack space in bytes. DEATH_REPORT_STACK_TRACE_SIZE is
	 * in words, so max capacity is DEATH_REPORT_STACK_TRACE_SIZE * sizeof(uint32_t)
	 * bytes. */
	const uint32_t available_bytes =
		bsp_section_rtemsstack_end - death_report->stack_trace_pointer;
	const uint32_t max_bytes =
		DEATH_REPORT_STACK_TRACE_SIZE * sizeof(uint32_t);

	if (available_bytes < max_bytes) {
		death_report->stack_trace_length = available_bytes;
	} else {
		death_report->stack_trace_length = max_bytes;
	}

	uint8_t *const stack_trace_bytes = (uint8_t *)death_report->stack_trace;
	const uint8_t *const stack_bytes =
		(const uint8_t *)(uintptr_t)death_report->stack_trace_pointer;
	for (uint32_t i = 0; i < death_report->stack_trace_length; i++) {
		stack_trace_bytes[i] = stack_bytes[i];
	}
}

static uint16_t calculate_crc(const uint8_t *const data, const size_t length)
{
	uint16_t crc = CRC_INITIAL_VALUE;

	for (unsigned int i = 0; i < length; i++) {
		crc ^= (uint16_t)data[i] << 8;
		for (int j = 0; j < 8; j++) {
			if (crc & CRC_MOST_SYGNIFICANT_BIT)
				crc = (crc << 1) ^ CRC_POLYNOMIAL;
			else
				crc <<= 1;
		}
	}

	return crc;
}

static uint16_t calculate_report_crc(const void *const report,
				     const size_t total_size)
{
	// Compute the checksum over the report payload, excluding the CRC field.
	const uint8_t *const address_without_crc =
		(const uint8_t *)report + sizeof(uint16_t);
	const size_t size_without_crc = total_size - sizeof(uint16_t);

	return calculate_crc(address_without_crc, size_without_crc);
}

bool DeathReportWriter_Init()
{
	return true;
}

bool DeathReportWriter_GenerateDeathReport()
{
	DeathReportWriter_DeathReport *const death_report =
		(DeathReportWriter_DeathReport *)&DEATH_REPORT_BEGIN;

	// Populate the shared death-report area before the fatal reset path runs.
	save_stack(death_report);

	death_report->padding = 0u;
	death_report->was_seen = false;
	death_report->checksum =
		calculate_report_crc((const void *)death_report,
				     sizeof(DeathReportWriter_DeathReport));

	return true;
}
