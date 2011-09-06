
#include <asm.inc>
#include "tsc.h"

.code

EXTERN _TscCalibrationPhase:BYTE
EXTERN _TscCalibrationArray:QWORD

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
    mov dword ptr _TscCalibrationArray[ecx * 2], eax
    mov dword ptr _TscCalibrationArray[ecx * 2 + 4], edx

    /* Advance phase */
    inc byte ptr ds:[_TscCalibrationPhase]

_CalibrationISR_Exit:
    pop edx
    pop ecx
    pop eax
    iretd

END
