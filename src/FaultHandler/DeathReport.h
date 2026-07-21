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

#ifndef DEATHREPORT_H
#define DEATHREPORT_H

/**
 * @file    DeathReport.h
 * @brief   Header containing DeathReport structure
 */

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief   Death report stack trace size given in 4 bytes words
 */
#define DEATH_REPORT_STACK_TRACE_SIZE 128

/**
 * @brief   BSW DeathReport may contain additional data after the stack trace,
 *          e.g. self-tests status, which is not used by the TASTE applications.
 *          However, it's necessary to calculate the checksum over this data
 *          even if it's unused, to match the BSW behavior - otherwise the death
 *          report will always fail checksum check on BSW side and will never
 *          be recognized as valid, making it impossible for BSW to detect ASW
 *          crash. The amount of bytes is not constant, as it depends on the BSW
 *          tailoring, so it's left for the user to define that in Makefile.
*/
#ifndef DEATH_REPORT_RESERVED_BYTES
// Default value matches the default self-test layout for the SAMV71 build.
#define DEATH_REPORT_RESERVED_BYTES 35
#endif

/**
 * @brief Structure representing DeathReport.
 *
 * The report layout is packed to match the boot software checksum format.
 */
typedef struct __attribute__((packed, aligned(sizeof(uint32_t)))) {
	uint16_t checksum; // Report checksum.
	bool was_seen; // Set by the boot software after the report is consumed.
	uint8_t padding; // Alignment padding.
	uint32_t exception_id; // Faulting exception ID.

	/**
	 * @brief Structure holding register values captured at the fault point.
	 */
	struct __attribute__((packed)) {
		uint32_t r0; // R0 register copy.
		uint32_t r1; // R1 register copy.
		uint32_t r2; // R2 register copy.
		uint32_t r3; // R3 register copy.
		uint32_t r4; // R4 register copy.
		uint32_t r5; // R5 register copy.
		uint32_t r6; // R6 register copy.
		uint32_t r7; // R7 register copy.
		uint32_t r8; // R8 register copy.
		uint32_t r9; // R9 register copy.
		uint32_t r10; // R10 register copy.
		uint32_t r11; // R11 register copy.
		uint32_t r12; // R12 register copy.
		uint32_t msp; // MSP register copy.
		uint32_t psp; // PSP register copy.
		uint32_t lr; // LR register copy.
		uint32_t pc; // PC register copy.
		uint32_t psr; // PSR register copy.
		uint32_t pri_mask; // PRIMASK register copy.
		uint32_t fault_mask; // FAULTMASK register copy.
		uint32_t base_pri; // BASEPRI register copy.
		uint32_t control; // CONTROL register copy.
	} registers; // Registers copy.

	/**
	 * @brief System Control Block status copied into the report.
	 */
	struct __attribute__((packed)) {
		uint32_t cfsr; // Configurable Fault Status Register copy.
		uint32_t hfsr; // HardFault Status Register copy.
		uint32_t mmar; // MemManage Fault Address Register copy.
		uint32_t bfar; // BusFault Address Register copy.
	} system_control_block;

	uint32_t stack_trace_pointer; // Saved stack trace pointer.
	uint32_t stack_trace_length; // Saved stack trace length.
	// Stack words are copied verbatim so post-mortem tooling can reconstruct the fault context.
	uint32_t stack_trace[DEATH_REPORT_STACK_TRACE_SIZE];
#if DEATH_REPORT_RESERVED_BYTES > 0
	uint8_t _reserved[DEATH_REPORT_RESERVED_BYTES];
#endif
} DeathReportWriter_DeathReport;

#endif
