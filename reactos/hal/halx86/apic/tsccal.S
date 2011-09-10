
#include <asm.inc>
#include "tsc.h"

.code

#ifdef _M_IX86

EXTERN _TscCalibrationPhase:BYTE
EXTERN _TscCalibrationArray:QWORD
EXTERN _HalpSendEOI@0:PROC

PUBLIC _TscCalibrationISR
_TscCalibrationISR:
    push eax
    push ecx
    push edx

    /* The first thing we do is read the current TSC value */
    rdtsc

    /* Read the current phase */
    movzx ecx, byte ptr ds:[_TscCalibrationPhase]

    /* Check if we're already done */
    cmp cl, NUM_SAMPLES
    jnb _CalibrationISR_Exit

    /* Store the current value */
    shl ecx, 3
    mov dword ptr _TscCalibrationArray[ecx], eax
    mov dword ptr _TscCalibrationArray[ecx + 4], edx

    /* Advance phase */
    inc byte ptr ds:[_TscCalibrationPhase]

_CalibrationISR_Exit:

    /* Read CMOS register C */
    mov al, HEX(0C)
    out HEX(70), al
    jmp $+2
    in al, HEX(71)
    jmp $+2

    /* Send EOI */
    call _HalpSendEOI@0

    pop edx
    pop ecx
    pop eax
    iretd

#else

EXTERN TscCalibrationPhase:BYTE
EXTERN TscCalibrationArray:DWORD

PUBLIC TscCalibrationISR
FUNC TscCalibrationISR
    push rax
    push rcx
    push rdx
    .ENDPROLOG

    /* The first thing we do is read the current TSC value */
    rdtsc

    /* Read the current phase */
    movzx rcx, byte ptr [TscCalibrationPhase]

    /* Check if we're already done */
    cmp cl, NUM_SAMPLES
    jnb CalibrationISR_Exit

    /* Store the current value */
    shl rcx, 1
    lea rax, [TscCalibrationArray]
    mov dword ptr [rax + rcx], eax
    mov dword ptr [rax + rcx + 4], edx

    /* Advance phase */
    inc byte ptr [TscCalibrationPhase]

CalibrationISR_Exit:
    pop rdx
    pop rcx
    pop rax
    iretq
ENDFUNC TscCalibrationISR
#endif

END
