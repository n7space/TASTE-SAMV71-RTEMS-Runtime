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

#include <FaultHandler.h>
#include <DeathReport.h>
#include <DeathReportWriter.h>

#include <Scb/Scb.h>
#include <Rstc/Rstc.h>
#include <Nvic/Nvic.h>
#include <Nvic/NvicVectorTable.h>

extern const Nvic_VectorTable exception_table;

void __attribute__((noreturn)) Fault_HandlerTail(void)
{
  DeathReportWriter_GenerateDeathReport();

  (void)Scb_cleanDCache();
	(void)Scb_invalidateDCache();
	Rstc_resetSystem();
  while(1);
}

void __attribute__((naked, aligned(8))) Fault_Handler()
{
  // In order to avoid a preamble which
  // interferes with the SP and R7 contents, the function is declared as naked. In order
  // not to break anything due to the lack of preamble, no local variables can be used.

  // emit Thumb instruction and check which stack is active
  // save stack pointer to R3
  asm volatile(".syntax unified \n"
               ".thumb  \n"
               ".align 2 \n"
               "cpsid i\n"
               "tst lr, #4 \n"
               "ite eq \n"
               "mrseq r3, msp \n"
               "mrsne r3, psp \n");

  // Watchdog reset before fault handling.
  asm volatile("ldr r0, WdtRstKey \n"
               "ldr r1, WdtCrAddr \n"
               "str r0, [r1] \n");

  // Move general purpose registers to DeathReport before they are overwritten.
  asm volatile("ldr r0, BootReportSectionBegin\n"
               "add r0, r0, %[DeathReportOffset]\n"
               "mrs r1, ipsr\n"                       // Read IPSR.
               "and r1, r1, 0x3F\n"                   // Extract Exception number.
               "str r1, [r0, %[ExceptionIdOffset]]\n" // Save Exception number.
               "ldr r1, [r3, #0]\n"                   // Old R0 is at [R3 + 0].
               "str r1, [r0, %[R0Offset]]\n"          // Save old R0.
               "ldr r1, [r3, #4]\n"                   // Old R1 is at [R3 + 4].
               "str r1, [r0, %[R1Offset]]\n"          // Save old R1.
               "ldr r1, [r3, #8]\n"                   // Old R2 is at [R3 + 8].
               "str r1, [r0, %[R2Offset]]\n"          // Save old R2.
               "ldr r1, [r3, #12]\n"                  // Old R3 is at [R3 + 12].
               "str r1, [r0, %[R3Offset]]\n"          // Save old R3.
               "str r4, [r0, %[R4Offset]]\n"          // Save R4.
               "str r5, [r0, %[R5Offset]]\n"          // Save R5.
               "str r6, [r0, %[R6Offset]]\n"          // Save R6.
               "str r7, [r0, %[R7Offset]]\n"          // Save R7.
               "str r8, [r0, %[R8Offset]]\n"          // Save R8.
               "str r9, [r0, %[R9Offset]]\n"          // Save R9.
               "str r10, [r0, %[R10Offset]]\n"        // Save R10.
               "str r11, [r0, %[R11Offset]]\n"        // Save R11.
               "ldr r1, [r3, #16]\n"                  // Old R12 is at [R3 + 16].
               "str r1, [r0, %[R12Offset]]\n"         // Save old R12.
               "mrs r1, msp\n"                        // Transfer special register to r1.
               "str r1, [r0, %[MspOffset]]\n"         // Save MSP.
               "mrs r1, psp\n"                        // Transfer special register to r1.
               "str r1, [r0, %[PspOffset]]\n"         // Save PSP.
               "ldr r1, [r3, #20]\n"                  // Old LR is at [R3 + 20].
               "str r1, [r0, %[LrOffset]]\n"          // Save old LR.
               "ldr r1, [r3, #24]\n"                  // Old PC is at [R3 + 24].
               "str r1, [r0, %[PcOffset]]\n"          // Save old PC.
               "ldr r1, [r3, #28]\n"                  // Old PSR is at [R3 + 28].
               "str r1, [r0, %[PsrOffset]]\n"         // Save old PSR.
               "mrs r1, primask\n"                    // Transfer special register to r1.
               "str r1, [r0, %[PrimaskOffset]]\n"     // Save PRIMASK.
               "mrs r1, faultmask\n"                  // Transfer special register to r1.
               "str r1, [r0, %[FaultmaskOffset]]\n"   // Save FAULTMASK
               "mrs r1, basepri\n"                    // Transfer special register to r1.
               "str r1, [r0, %[BasepriOffset]]\n"     // Save BASEPRI.
               "mrs r1, control\n"                    // Transfer special register to r1.
               "str r1, [r0, %[ControlOffset]]\n"     // Save CONTROL.
               "ldr r1, CfsrAddress\n"                // SCB CFSR address loaded.
               "ldr r2, [r1], #4\n"                   // Load CFSR and move address to HFSR.
               "str r2, [r0, %[CfsrOffset]]\n"        // Save CFSR.
               "ldr r2, [r1], #8\n"                   // Load HFSR and move address to MMAR.
               "str r2, [r0, %[HfsrOffset]]\n"        // Save HFSR.
               "ldr r2, [r1], #4\n"                   // Load MMAR and move address to BFAR.
               "str r2, [r0, %[MmarOffset]]\n"        // Save MMAR.
               "ldr r2, [r1]\n"                       // Load BFAR.
               "str r2, [r0, %[BfarOffset]]\n"        // Save BFAR.
               "str r3, [r0, %[SpOffset]]\n"          // Save SP.
               // Jump to the C code part of the handler. We are not coming back.
               "b %[Tail]\n"
               "BootReportSectionBegin: .word 0x2045F968\n"
               "CfsrAddress:            .word 0xE000ED28\n"
               "WdtCrAddr:              .word 0x40100250\n"
               "WdtRstKey:              .word 0xA5000001\n"
               :
               : [DeathReportOffset] "i"(DEATH_REPORT_OFFSET),
               [ExceptionIdOffset] "i"(offsetof(DeathReportWriter_DeathReport, exception_id)),
               [R0Offset] "i"(offsetof(DeathReportWriter_DeathReport, registers.r0)),
               [R1Offset] "i"(offsetof(DeathReportWriter_DeathReport, registers.r1)),
               [R2Offset] "i"(offsetof(DeathReportWriter_DeathReport, registers.r2)),
               [R3Offset] "i"(offsetof(DeathReportWriter_DeathReport, registers.r3)),
               [R4Offset] "i"(offsetof(DeathReportWriter_DeathReport, registers.r4)),
               [R5Offset] "i"(offsetof(DeathReportWriter_DeathReport, registers.r5)),
               [R6Offset] "i"(offsetof(DeathReportWriter_DeathReport, registers.r6)),
               [R7Offset] "i"(offsetof(DeathReportWriter_DeathReport, registers.r7)),
               [R8Offset] "i"(offsetof(DeathReportWriter_DeathReport, registers.r8)),
               [R9Offset] "i"(offsetof(DeathReportWriter_DeathReport, registers.r9)),
               [R10Offset] "i"(offsetof(DeathReportWriter_DeathReport, registers.r10)),
               [R11Offset] "i"(offsetof(DeathReportWriter_DeathReport, registers.r11)),
               [R12Offset] "i"(offsetof(DeathReportWriter_DeathReport, registers.r12)),
               [MspOffset] "i"(offsetof(DeathReportWriter_DeathReport, registers.msp)),
               [PspOffset] "i"(offsetof(DeathReportWriter_DeathReport, registers.psp)),
               [LrOffset] "i"(offsetof(DeathReportWriter_DeathReport, registers.lr)),
               [PcOffset] "i"(offsetof(DeathReportWriter_DeathReport, registers.pc)),
               [PsrOffset] "i"(offsetof(DeathReportWriter_DeathReport, registers.psr)),
               [PrimaskOffset] "i"(offsetof(DeathReportWriter_DeathReport, registers.pri_mask)),
               [FaultmaskOffset] "i"(offsetof(DeathReportWriter_DeathReport, registers.fault_mask)),
               [BasepriOffset] "i"(offsetof(DeathReportWriter_DeathReport, registers.base_pri)),
               [ControlOffset] "i"(offsetof(DeathReportWriter_DeathReport, registers.control)),
               [CfsrOffset] "i"(offsetof(DeathReportWriter_DeathReport, system_control_block.cfsr)),
               [HfsrOffset] "i"(offsetof(DeathReportWriter_DeathReport, system_control_block.hfsr)),
               [MmarOffset] "i"(offsetof(DeathReportWriter_DeathReport, system_control_block.mmar)),
               [BfarOffset] "i"(offsetof(DeathReportWriter_DeathReport, system_control_block.bfar)),
               [SpOffset] "i"(offsetof(DeathReportWriter_DeathReport, stack_trace_pointer)),
               [Tail] "i"(&Fault_HandlerTail));
}

bool FaultHandler_Init()
{
  static volatile Scb_Registers *const scb =
			(volatile Scb_Registers *)SCB_BASE_ADDRESS;

	scb->shcsr = scb->shcsr | SCB_SHCSR_USGFAULTENA_MASK;
	scb->ccr |= SCB_CCR_DIV_0_TRP_MASK;

  Nvic_relocateVectorTable(&exception_table);

	return true;
}
