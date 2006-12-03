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

/* FUNCTIONS ******************************************************************/

.globl _KeUpdateRunTime@4
.func KeUpdateRunTime@4
_KeUpdateRunTime@4:

    /* Get KPCR */
    mov eax, [fs:KPCR_SELF]

    /* Save EBX */
    push ebx

    /* Increase interrupt count */
    inc dword ptr [eax+KPCR_PRCB_INTERRUPT_COUNT]

    /* Get the current thread and process */
    mov ebx, [eax+KPCR_CURRENT_THREAD]
    mov ecx, [ebx+KTHREAD_APCSTATE_PROCESS]

    /* Check if this was V86 or user mode */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz NotKern
    test byte ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jnz NotKern

    /* Increase kernel time */
    inc dword ptr [eax+KPCR_PRCB_KERNEL_TIME]

    /* Check if IRQL was DISPATCH_LEVEL */
    cmp byte ptr [esp+8], DISPATCH_LEVEL
    jb BelowDispatch
    ja AboveDispatch

    /* Check if the DPC routine is active */
    cmp dword ptr[eax+KPCR_PRCB_DPC_ROUTINE_ACTIVE], 0
    jz BelowDispatch

    /* At dispatch, increase DPC time */
    inc dword ptr [eax+KPCR_PRCB_DPC_TIME]
    jmp AfterSet

AboveDispatch:
    /* Update interrupt time */
    inc dword ptr [eax+KPCR_PRCB_INTERRUPT_TIME]
    jmp AfterSet

BelowDispatch:
    /* Update kernel time */
    inc dword ptr [ebx+KTHREAD_KERNEL_TIME]
    jmp AfterSet

NotKern:
    /* Update user time */
    inc dword ptr [eax+KPCR_PRCB_USER_TIME]
    inc dword ptr [ebx+KTHREAD_USER_TIME]

AfterSet:
    /* Get the DPC Count and last count, and set the ne wone */
    mov ecx, [eax+KPCR_PRCB_DPC_COUNT]
    mov edx, [eax+KPCR_PRCB_DPC_LAST_COUNT]
    mov [eax+KPCR_PRCB_DPC_LAST_COUNT], ecx

    /* Substract counts and add request rate, divide by two to get average */
    sub ecx, edx
    add ecx, [eax+KPCR_PRCB_DPC_REQUEST_RATE]
    shr ecx, 1

    /* Set this as the new request rate */
    mov [eax+KPCR_PRCB_DPC_REQUEST_RATE], ecx

    /* Check for depth > 0, DPCs to be inactive, and no other pending request */
    cmp dword ptr [eax+KPCR_PRCB_DPC_QUEUE_DEPTH], 0
    je DontRequest
    cmp byte ptr [eax+KPCR_PRCB_DPC_ROUTINE_ACTIVE], 0
    jnz DontRequest
    cmp byte ptr [eax+KPCR_PRCB_DPC_INTERRUPT_REQUESTED], 0
    jnz DontRequest

    /* Request a DPC */
    mov ecx, DISPATCH_LEVEL
    call @HalRequestSoftwareInterrupt@4

    /* Restore PCR address */
    mov eax, [fs:KPCR_SELF]

    /* Get the DPC request rate and threshold adjust, and set it */
    mov ecx, [eax+KPCR_PRCB_DPC_REQUEST_RATE]
    mov edx, _KiAdjustDpcThreshold
    mov [eax+KPCR_PRCB_ADJUST_DPC_THRESHOLD], edx

    /* Check if the rate now is not ideal */
    cmp ecx, _KiIdealDpcRate
    jge RateOk
    cmp dword ptr [eax+KPCR_PRCB_MAXIMUM_DPC_QUEUE_DEPTH], 1
    je RateOk

    /* Fix the depth */
    dec dword ptr [eax+KPCR_PRCB_MAXIMUM_DPC_QUEUE_DEPTH]
    jmp RateOk

DontRequest:
    /* We didn't request a DPC, decrease the threshold */
    dec dword ptr [eax+KPCR_PRCB_ADJUST_DPC_THRESHOLD]
    jnz RateOk

    /* We're at 0 now, reset it */
    mov ecx, _KiAdjustDpcThreshold
    mov [eax+KPCR_PRCB_ADJUST_DPC_THRESHOLD], ecx

    /* Get maximum depth and check it */
    mov ecx, _KiMaximumDpcQueueDepth
    cmp ecx, [eax+KPCR_PRCB_MAXIMUM_DPC_QUEUE_DEPTH]
    je RateOk

    /* Increase it, it's below maximum */
    inc dword ptr [eax+KPCR_PRCB_MAXIMUM_DPC_QUEUE_DEPTH]

RateOk:
    /* Decrement quantum and verify it */
    sub byte ptr [ebx+KTHREAD_QUANTUM], CLOCK_QUANTUM_DECREMENT
    jg QuantumNotEmpty

    /* Make sure this isn't the idle thread */
    cmp ebx, [eax+KPCR_PRCB_IDLE_THREAD]
    jz QuantumNotEmpty

    /* Set quantum end */
    mov byte ptr [eax+KPCR_PRCB_QUANTUM_END], 1
    mov ecx, DISPATCH_LEVEL
    call @HalRequestSoftwareInterrupt@4

QuantumNotEmpty:
    /* Restore ebx and return */
    pop ebx
    ret 4
.endfunc

.globl _KeUpdateSystemTime@0
.func KeUpdateSystemTime@0
_KeUpdateSystemTime@0:

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
    mov ecx, [ebx+USER_SHARED_DATA_SYSTEM_TIME]
    mov edx, [ebx+USER_SHARED_DATA_SYSTEM_TIME+4]

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
    mov ds:[USER_SHARED_DATA+USER_SHARED_DATA_TICK_COUNT+8], edx
    mov ds:[USER_SHARED_DATA+USER_SHARED_DATA_TICK_COUNT], ecx
    mov ds:[USER_SHARED_DATA+USER_SHARED_DATA_TICK_COUNT+4], edx

    /* FIXME: HACK */
    mov ds:[USER_SHARED_DATA], ecx

IncompleteTick:

    /* FIXME: NASTY Queue DPC to handle registered timers */
    push 0
    push [esp+KTRAP_FRAME_EIP]
    push offset _KiExpireTimerDpc
    call _KeInsertQueueDpc@12

    /* Check if this was a full tick */
    cmp dword ptr _KiTickOffset, 0
    jg IncompleteTick2

    /* Increase tick offset */
    mov eax, _KeMaximumIncrement
    add _KiTickOffset, eax

    /* Update system run time */
    push [esp]
    call _KeUpdateRunTime@4
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
