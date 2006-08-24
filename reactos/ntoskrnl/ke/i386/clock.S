/*
 * FILE:            ntoskrnl/ke/i386/clock.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Clock Management
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
#include <internal/i386/asmmacro.S>
.intel_syntax noprefix

/* GLOBALS *******************************************************************/

.extern _KeTimeAdjustment
.extern _KiTickOffset
.extern _KeTickCount
.extern _KeMaximumIncrement

/* FUNCTIONS ******************************************************************/

.globl _KeUpdateSystemTime2@0
.func KeUpdateSystemTime2@0
_KeUpdateSystemTime2@0:

    /* Get shared data in ECX */
    mov ecx, USER_SHARED_DATA

    /* Get interrupt time */
    mov edi, [ecx+USER_SHARED_DATA_INTERRUPT_TIME]
    mov esi, [ecx+USER_SHARED_DATA_INTERRUPT_TIME+4]

    /* Add the increment and get the carry */
    add edi, eax
    adc esi, 0

    /* Now store the updated times */
    mov [ecx+USER_SHARED_DATA_INTERRUPT_TIME+8], esi
    mov [ecx+USER_SHARED_DATA_INTERRUPT_TIME], edi
    mov [ecx+USER_SHARED_DATA_INTERRUPT_TIME+4], esi

    /* Substract tick count and get the low count */
    LOCK sub _KiTickOffset, eax
    mov eax, _KeTickCount
    mov ebx, eax
    jg IncompleteTick

    /* Get shared data in ECX */
    mov ebx, USER_SHARED_DATA

    /* Get system time */
    mov edi, [ebx+USER_SHARED_DATA_SYSTEM_TIME]
    mov esi, [ebx+USER_SHARED_DATA_SYSTEM_TIME+4]

    /* Add the increment and get the carry */
    add ecx, _KeTimeAdjustment
    adc edx, 0

    /* Now store the updated times */
    mov [ebx+USER_SHARED_DATA_SYSTEM_TIME+8], edx
    mov [ebx+USER_SHARED_DATA_SYSTEM_TIME], ecx
    mov [ebx+USER_SHARED_DATA_SYSTEM_TIME+4], edx

    /* Put tick count back in EBX */
    mov ebx, eax

    /* Copyit in ECX and get hich count */
    mov ecx, eax
    mov edx, _KeTickCount + 4

    /* Add the increment and get the carry */
    add ecx, 1
    adc edx, 0

    /* Now store the updated tick */
    mov [_KeTickCount+8], edx
    mov [_KeTickCount], ecx
    mov [_KeTickCount+4], edx

    /* Store in in shared data too */
    mov [USER_SHARED_DATA+USER_SHARED_DATA_TICK_COUNT+8], edx
    mov [USER_SHARED_DATA+USER_SHARED_DATA_TICK_COUNT], ecx
    mov [USER_SHARED_DATA+USER_SHARED_DATA_TICK_COUNT+4], edx

IncompleteTick:

    /* Queue DPC to handle registered timers */

    /* Check if this was a full tick */
    cmp dword ptr _KiTickOffset, 0
    jg IncompleteTick2

    /* Increase tick offset */
    mov eax, _KeMaximumIncrement
    add _KiTickOffset, eax

    /* Update system run time */
    push esp
    call _KeUpdateRunTime@8
    jmp Done

IncompleteTick2:
    /* Increase interrupt count */
    inc dword ptr [fs:KPCR_PRCB_INTERRUPT_COUNT]

Done:
    /* Exit the interrupt */
    mov esi, $
    cli
    call _HalEndSystemInterrupt@8
    jmp _Kei386EoiHelper@0
.endfunc
