/*
 * PROJECT:     NEC PC-98 series HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Busy-wait loop implementation
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ks386.inc>

#include "delay.h"

/* GLOBALS *******************************************************************/

#define PIC1_CONTROL_PORT      HEX(00)
#define PIC1_DATA_PORT         HEX(02)
#define PIC2_CONTROL_PORT      HEX(08)
#define PIC2_DATA_PORT         HEX(0A)

#define PIC_EOI      HEX(20)

#define MSR_RDTSC    HEX(10)

/* FUNCTIONS *****************************************************************/

.code

#ifdef _USE_ML
INIT_ASM SEGMENT PARA PUBLIC USE32 READ WRITE EXECUTE DISCARD
#endif

PUBLIC _HalpTscCalibrationISR
_HalpTscCalibrationISR:
    push edx
    push ecx
    push eax

    /* The first thing we do is read the current TSC value */
    rdtsc

    mov ecx, dword ptr ds:[_TscCalibrationPhase]

    /* Ignore the first interrupt since it fires randomly */
    test ecx, ecx
    je .FirstInterrupt

    /* Check if we're already done */
    cmp ecx, NUM_SAMPLES
    ja .Done

    /* Store the current TSC value (the phase number is 1-based) */
    mov dword ptr ds:[ecx*8 + _TscCalibrationArray + 0 - 8], eax
    mov dword ptr ds:[ecx*8 + _TscCalibrationArray + 4 - 8], edx
    jmp .AdvancePhase

.FirstInterrupt:
    /* Reset TSC value to 0 */
    mov ecx, MSR_RDTSC
    xor eax, eax
    xor edx, edx
    wrmsr

.AdvancePhase:
    inc dword ptr ds:[_TscCalibrationPhase]

.Done:
    /* Send the EOI for the IRQ */
    mov al, PIC_EOI
    out PIC1_CONTROL_PORT, al

    pop eax
    pop ecx
    pop edx
    iretd

.align 8

PUBLIC _TscCalibrationArray
_TscCalibrationArray:
    .space NUM_SAMPLES * 8

PUBLIC _TscCalibrationPhase
_TscCalibrationPhase:
    .long 0

#ifdef _USE_ML
INIT_ASM ENDS
#endif

PUBLIC _KeStallExecutionProcessor@4
_KeStallExecutionProcessor@4:
    push ebx

    /*
     * Force the in-order execution of the RDTSC instruction.
     * HAL will overwrite this with a no-op instruction on older processors.
     */
PUBLIC _HalpStallExecutionSerialize
_HalpStallExecutionSerialize:
    xor eax, eax
    cpuid

    /* Get the initial time */
    rdtsc

    /* Calculate the ending time */
    mov ecx, eax
    mov eax, fs:[KPCR_STALL_SCALE_FACTOR]
    mov ebx, edx
    mul dword ptr [esp + 8]
    add ecx, eax
    adc ebx, edx

    /* Loop until time is elapsed */
.Loop:
    rdtsc
    cmp eax, ecx
    mov eax, edx
    sbb eax, ebx
    jc .Loop

    pop ebx
    ret 4

END
