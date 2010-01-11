/*
 * FILE:            ntoskrnl/ke/i386/trap.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Traps, Entrypoints and Exitpoints
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * NOTE:            See asmmacro.S for the shared entry/exit code.
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
#include <internal/i386/asmmacro.S>
#include <internal/i386/callconv.s>
.intel_syntax noprefix

#define Running 2
#define WrDispatchInt 0x1F

/* GLOBALS *******************************************************************/

.data
.globl _KiIdt
_KiIdt:
/* This is the Software Interrupt Table that we handle in this file:        */
idt _KiTrap0,          INT_32_DPL0  /* INT 00: Divide Error (#DE)           */
idt _KiTrap1,          INT_32_DPL0  /* INT 01: Debug Exception (#DB)        */
idt _KiTrap2,          INT_32_DPL0  /* INT 02: NMI Interrupt                */
idt _KiTrap3,          INT_32_DPL3  /* INT 03: Breakpoint Exception (#BP)   */
idt _KiTrap4,          INT_32_DPL3  /* INT 04: Overflow Exception (#OF)     */
idt _KiTrap5,          INT_32_DPL0  /* INT 05: BOUND Range Exceeded (#BR)   */
idt _KiTrap6,          INT_32_DPL0  /* INT 06: Invalid Opcode Code (#UD)    */
idt _KiTrap7,          INT_32_DPL0  /* INT 07: Device Not Available (#NM)   */
idt _KiTrap8,          INT_32_DPL0  /* INT 08: Double Fault Exception (#DF) */
idt _KiTrap9,          INT_32_DPL0  /* INT 09: RESERVED                     */
idt _KiTrap10,         INT_32_DPL0  /* INT 0A: Invalid TSS Exception (#TS)  */
idt _KiTrap11,         INT_32_DPL0  /* INT 0B: Segment Not Present (#NP)    */
idt _KiTrap12,         INT_32_DPL0  /* INT 0C: Stack Fault Exception (#SS)  */
idt _KiTrap13,         INT_32_DPL0  /* INT 0D: General Protection (#GP)     */
idt _KiTrap14,         INT_32_DPL0  /* INT 0E: Page-Fault Exception (#PF)   */
idt _KiTrap0F,         INT_32_DPL0  /* INT 0F: RESERVED                     */
idt _KiTrap16,         INT_32_DPL0  /* INT 10: x87 FPU Error (#MF)          */
idt _KiTrap17,         INT_32_DPL0  /* INT 11: Align Check Exception (#AC)  */
idt _KiTrap0F,         INT_32_DPL0  /* INT 12: Machine Check Exception (#MC)*/
idt _KiTrap0F,         INT_32_DPL0  /* INT 13: SIMD FPU Exception (#XF)     */
.rept 22
idt _KiTrap0F,         INT_32_DPL0  /* INT 14-29: UNDEFINED INTERRUPTS      */
.endr
idt _KiGetTickCount,   INT_32_DPL3  /* INT 2A: Get Tick Count Handler       */
idt _KiCallbackReturn, INT_32_DPL3  /* INT 2B: User-Mode Callback Return    */
idt _KiRaiseAssertion, INT_32_DPL3  /* INT 2C: Debug Assertion Handler      */
idt _KiDebugService,   INT_32_DPL3  /* INT 2D: Debug Service Handler        */
idt _KiSystemService,  INT_32_DPL3  /* INT 2E: System Call Service Handler  */
idt _KiTrap0F,         INT_32_DPL0  /* INT 2F: RESERVED                     */
GENERATE_IDT_STUBS                  /* INT 30-FF: UNEXPECTED INTERRUPTS     */

/* Trap handlers referenced from C code                                     */
.globl _KiTrap8
.globl _KiTrap19

/* System call code referenced from C code                                  */
.globl _CopyParams
.globl _ReadBatch

/* System call entrypoints:                                                 */
.globl _KiFastCallEntry
.globl _KiSystemService

/* And special system-defined software traps:                               */
.globl _NtRaiseException@12
.globl _NtContinue@8
.globl _KiDispatchInterrupt@0

/* Interrupt template entrypoints                                           */
.globl _KiInterruptTemplate
.globl _KiInterruptTemplateObject
.globl _KiInterruptTemplateDispatch

#ifndef HAL_INTERRUPT_SUPPORT_IN_C
/* Chained and Normal generic interrupt handlers for 1st and 2nd level entry*/
.globl _KiChainedDispatch2ndLvl@0
.globl _KiInterruptDispatch@0
.globl _KiChainedDispatch@0
#endif

/* We implement the following trap exit points:                             */
.globl _KiServiceExit               /* Exit from syscall                    */
.globl _KiServiceExit2              /* Exit from syscall with complete frame*/
.globl _Kei386EoiHelper@0           /* Exit from interrupt or H/W trap      */
.globl _Kei386EoiHelper2ndEntry     /* Exit from unexpected interrupt       */

.globl _KiIdtDescriptor
_KiIdtDescriptor:
    .short 0
    .short 0x7FF
    .long _KiIdt

.globl _KiUnexpectedEntrySize
_KiUnexpectedEntrySize:
    .long _KiUnexpectedInterrupt1 - _KiUnexpectedInterrupt0

_UnexpectedMsg:
    .asciz "\n\x7\x7!!! Unexpected Interrupt %02lx !!!\n"

_V86UnhandledMsg:
    .asciz "\n\x7\x7!!! Unhandled V8086 (VDM) support at line: %lx!!!\n"

_UnhandledMsg:
    .asciz "\n\x7\x7!!! Unhandled or Unexpected Code at line: %lx [%s]!!!\n"

_IsrTimeoutMsg:
    .asciz "\n*** ISR at %lx took over .5 second\n"

_IsrOverflowMsg:
    .asciz "\n*** ISR at %lx appears to have an interrupt storm\n"

_KiTrapPrefixTable:
    .byte 0xF2                      /* REP                                  */
    .byte 0xF3                      /* REP INS/OUTS                         */
    .byte 0x67                      /* ADDR                                 */
    .byte 0xF0                      /* LOCK                                 */
    .byte 0x66                      /* OP                                   */
    .byte 0x2E                      /* SEG                                  */
    .byte 0x3E                      /* DS                                   */
    .byte 0x26                      /* ES                                   */
    .byte 0x64                      /* FS                                   */
    .byte 0x65                      /* GS                                   */
    .byte 0x36                      /* SS                                   */

_KiTrapIoTable:
    .byte 0xE4                      /* IN                                   */
    .byte 0xE5                      /* IN                                   */
    .byte 0xEC                      /* IN                                   */
    .byte 0xED                      /* IN                                   */
    .byte 0x6C                      /* INS                                  */
    .byte 0x6D                      /* INS                                  */
    .byte 0xE6                      /* OUT                                  */
    .byte 0xE7                      /* OUT                                  */
    .byte 0xEE                      /* OUT                                  */
    .byte 0xEF                      /* OUT                                  */
    .byte 0x6E                      /* OUTS                                 */
    .byte 0x6F                      /* OUTS                                 */

/* SOFTWARE INTERRUPT SERVICES ***********************************************/
.text


.func KiSystemService
TRAP_FIXUPS kss_a, kss_t, DoNotFixupV86, DoNotFixupAbios
_KiSystemService:

    /* Enter the shared system call prolog */
    SYSCALL_PROLOG kss_a, kss_t

    /* Jump to the actual handler */
    jmp SharedCode
.endfunc

.func KiFastCallEntry
TRAP_FIXUPS FastCallDrSave, FastCallDrReturn, DoNotFixupV86, DoNotFixupAbios
_KiFastCallEntry:

    /* Enter the fast system call prolog */
    FASTCALL_PROLOG FastCallDrSave, FastCallDrReturn

SharedCode:

    /*
     * Find out which table offset to use. Converts 0x1124 into 0x10.
     * The offset is related to the Table Index as such: Offset = TableIndex x 10
     */
    mov edi, eax
    shr edi, SERVICE_TABLE_SHIFT
    and edi, SERVICE_TABLE_MASK
    mov ecx, edi

    /* Now add the thread's base system table to the offset */
    add edi, [esi+KTHREAD_SERVICE_TABLE]

    /* Get the true syscall ID and check it */
    mov ebx, eax
    and eax, SERVICE_NUMBER_MASK
    cmp eax, [edi+SERVICE_DESCRIPTOR_LIMIT]

    /* Invalid ID, try to load Win32K Table */
    jnb KiBBTUnexpectedRange

    /* Check if this was Win32K */
    cmp ecx, SERVICE_TABLE_TEST
    jnz NotWin32K

    /* Get the TEB */
    mov ecx, PCR[KPCR_TEB]

    /* Check if we should flush the User Batch */
    xor ebx, ebx
_ReadBatch:
    or ebx, [ecx+TEB_GDI_BATCH_COUNT]
    jz NotWin32K

    /* Flush it */
    push edx
    push eax
    call [_KeGdiFlushUserBatch]
    pop eax
    pop edx

NotWin32K:
    /* Increase total syscall count */
    inc dword ptr PCR[KPCR_SYSTEM_CALLS]

#if DBG
    /* Increase per-syscall count */
    mov ecx, [edi+SERVICE_DESCRIPTOR_COUNT]
    jecxz NoCountTable
    inc dword ptr [ecx+eax*4]
#endif

    /* Users's current stack frame pointer is source */
NoCountTable:
    mov esi, edx

    /* Allocate room for argument list from kernel stack */
    mov ebx, [edi+SERVICE_DESCRIPTOR_NUMBER]
    xor ecx, ecx
    mov cl, [eax+ebx]

    /* Get pointer to function */
    mov edi, [edi+SERVICE_DESCRIPTOR_BASE]
    mov ebx, [edi+eax*4]

    /* Allocate space on our stack */
    sub esp, ecx

    /* Set the size of the arguments and the destination */
    shr ecx, 2
    mov edi, esp

    /* Make sure we're within the User Probe Address */
    cmp esi, _MmUserProbeAddress
    jnb AccessViolation

_CopyParams:
    /* Copy the parameters */
    rep movsd

    /* Do the System Call */
    call ebx

AfterSysCall:
#if DBG
    /* Make sure the user-mode call didn't return at elevated IRQL */
    test byte ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jz SkipCheck
    mov esi, eax                /* We need to save the syscall's return val */
    call _KeGetCurrentIrql@0
    or al, al
    jnz InvalidIrql
    mov eax, esi                /* Restore it */

    /* Get our temporary current thread pointer for sanity check */
    mov ecx, PCR[KPCR_CURRENT_THREAD]

    /* Make sure that we are not attached and that APCs are not disabled */
    mov dl, [ecx+KTHREAD_APC_STATE_INDEX]
    or dl, dl
    jnz InvalidIndex
    mov edx, [ecx+KTHREAD_COMBINED_APC_DISABLE]
    or edx, edx
    jnz InvalidIndex
#endif

SkipCheck:

    /* Deallocate the kernel stack frame  */
    mov esp, ebp

KeReturnFromSystemCall:

    /* Get the Current Thread */
    mov ecx, PCR[KPCR_CURRENT_THREAD]

    /* Restore the old trap frame pointer */
    mov edx, [ebp+KTRAP_FRAME_EDX]
    mov [ecx+KTHREAD_TRAP_FRAME], edx
.endfunc

.func KiServiceExit
_KiServiceExit:
    /* Disable interrupts */
    cli

    /* Check for, and deliver, User-Mode APCs if needed */
    CHECK_FOR_APC_DELIVER 1

    /* Exit and cleanup */
    TRAP_EPILOG FromSystemCall, DoRestorePreviousMode, DoNotRestoreSegments, DoNotRestoreVolatiles, DoRestoreEverything
.endfunc

KiBBTUnexpectedRange:

    /* If this isn't a Win32K call, fail */
    cmp ecx, SERVICE_TABLE_TEST
    jne InvalidCall

    /* Set up Win32K Table */
    push edx
    push ebx
    call _PsConvertToGuiThread@0

    /* Check return code */
    or eax, eax

    /* Restore registers */
    pop eax
    pop edx

    /* Reset trap frame address */
    mov ebp, esp
    mov [esi+KTHREAD_TRAP_FRAME], ebp

    /* Try the Call again, if we suceeded */
    jz SharedCode

    /*
     * The Shadow Table should have a special byte table which tells us
     * whether we should return FALSE, -1 or STATUS_INVALID_SYSTEM_SERVICE.
     */

    /* Get the table limit and base */
    lea edx, _KeServiceDescriptorTableShadow + SERVICE_TABLE_TEST
    mov ecx, [edx+SERVICE_DESCRIPTOR_LIMIT]
    mov edx, [edx+SERVICE_DESCRIPTOR_BASE]

    /* Get the table address and add our index into the array */
    lea edx, [edx+ecx*4]
    and eax, SERVICE_NUMBER_MASK
    add edx, eax

    /* Find out what we should return */
    movsx eax, byte ptr [edx]
    or eax, eax

    /* Return either 0 or -1, we've set it in EAX */
    jle KeReturnFromSystemCall

    /* Set STATUS_INVALID_SYSTEM_SERVICE */
    mov eax, STATUS_INVALID_SYSTEM_SERVICE
    jmp KeReturnFromSystemCall

InvalidCall:

    /* Invalid System Call */
    mov eax, STATUS_INVALID_SYSTEM_SERVICE
    jmp KeReturnFromSystemCall

AccessViolation:

    /* Check if this came from kernel-mode */
    test byte ptr [ebp+KTRAP_FRAME_CS], MODE_MASK

    /* It's fine, go ahead with it */
    jz _CopyParams

    /* Caller sent invalid parameters, fail here */
    mov eax, STATUS_ACCESS_VIOLATION
    jmp AfterSysCall

BadStack:

    /* Restore ESP0 stack */
    mov ecx, PCR[KPCR_TSS]
    mov esp, ss:[ecx+KTSS_ESP0]

    /* Generate V86M Stack for Trap 6 */
    push 0
    push 0
    push 0
    push 0

    /* Generate interrupt stack for Trap 6 */
    push KGDT_R3_DATA + RPL_MASK
    push 0
    push 0x20202
    push KGDT_R3_CODE + RPL_MASK
    push 0
    jmp _KiTrap6

#if DBG
InvalidIrql:
    /* Save current IRQL */
    push PCR[KPCR_IRQL]

    /* Set us at passive */
    mov dword ptr PCR[KPCR_IRQL], 0
    cli

    /* Bugcheck */
    push 0
    push 0
    push eax
    push ebx
    push IRQL_GT_ZERO_AT_SYSTEM_SERVICE
    call _KeBugCheckEx@20

InvalidIndex:

    /* Get the index and APC state */
    movzx eax, byte ptr [ecx+KTHREAD_APC_STATE_INDEX]
    mov edx, [ecx+KTHREAD_COMBINED_APC_DISABLE]

    /* Bugcheck */
    push 0
    push edx
    push eax
    push ebx
    push APC_INDEX_MISMATCH
    call _KeBugCheckEx@20
    ret
#endif

.func KiServiceExit2
_KiServiceExit2:

    /* Disable interrupts */
    cli

    /* Check for, and deliver, User-Mode APCs if needed */
    CHECK_FOR_APC_DELIVER 0

    /* Exit and cleanup */
    TRAP_EPILOG NotFromSystemCall, DoRestorePreviousMode, DoRestoreSegments, DoRestoreVolatiles, DoNotRestoreEverything
.endfunc

.func Kei386EoiHelper@0
_Kei386EoiHelper@0:

    /* Disable interrupts */
    cli

    /* Check for, and deliver, User-Mode APCs if needed */
    CHECK_FOR_APC_DELIVER 0

    /* Exit and cleanup */
_Kei386EoiHelper2ndEntry:
    TRAP_EPILOG NotFromSystemCall, DoNotRestorePreviousMode, DoRestoreSegments, DoRestoreVolatiles, DoNotRestoreEverything
.endfunc

V86_Exit:
    /* Move to EDX position */
    add esp, KTRAP_FRAME_EDX

    /* Restore volatiles */
    pop edx
    pop ecx
    pop eax

    /* Move to non-volatiles */
    lea esp, [ebp+KTRAP_FRAME_EDI]
    pop edi
    pop esi
    pop ebx
    pop ebp

    /* Skip error code and return */
    add esp, 4
    iret

AbiosExit:
    /* FIXME: TODO */
    UNHANDLED_PATH "ABIOS Exit"

GENERATE_TRAP_HANDLER KiGetTickCount, 1
GENERATE_TRAP_HANDLER KiCallbackReturn, 1        
GENERATE_TRAP_HANDLER KiRaiseAssertion, 1
GENERATE_TRAP_HANDLER KiDebugService, 1

.func NtRaiseException@12
_NtRaiseException@12:

    /* NOTE: We -must- be called by Zw* to have the right frame! */
    /* Push the stack frame */
    push ebp

    /* Get the current thread and restore its trap frame */
    mov ebx, PCR[KPCR_CURRENT_THREAD]
    mov edx, [ebp+KTRAP_FRAME_EDX]
    mov [ebx+KTHREAD_TRAP_FRAME], edx

    /* Set up stack frame */
    mov ebp, esp

    /* Get the Trap Frame in EBX */
    mov ebx, [ebp+0]

    /* Get the exception list and restore */
    mov eax, [ebx+KTRAP_FRAME_EXCEPTION_LIST]
    mov PCR[KPCR_EXCEPTION_LIST], eax

    /* Get the parameters */
    mov edx, [ebp+16] /* Search frames */
    mov ecx, [ebp+12] /* Context */
    mov eax, [ebp+8]  /* Exception Record */

    /* Raise the exception */
    push edx
    push ebx
    push 0
    push ecx
    push eax
    call _KiRaiseException@20

    /* Restore trap frame in EBP */
    pop ebp
    mov esp, ebp

    /* Check the result */
    or eax, eax
    jz _KiServiceExit2

    /* Restore debug registers too */
    jmp _KiServiceExit
.endfunc

.func NtContinue@8
_NtContinue@8:

    /* NOTE: We -must- be called by Zw* to have the right frame! */
    /* Push the stack frame */
    push ebp

    /* Get the current thread and restore its trap frame */
    mov ebx, PCR[KPCR_CURRENT_THREAD]
    mov edx, [ebp+KTRAP_FRAME_EDX]
    mov [ebx+KTHREAD_TRAP_FRAME], edx

    /* Set up stack frame */
    mov ebp, esp

    /* Save the parameters */
    mov eax, [ebp+0]
    mov ecx, [ebp+8]

    /* Call KiContinue */
    push eax
    push 0
    push ecx
    call _KiContinue@12

    /* Check if we failed (bad context record) */
    or eax, eax
    jnz Error

    /* Check if test alert was requested */
    cmp dword ptr [ebp+12], 0
    je DontTest

    /* Test alert for the thread */
    mov al, [ebx+KTHREAD_PREVIOUS_MODE]
    push eax
    call _KeTestAlertThread@4

DontTest:
    /* Return to previous context */
    pop ebp
    mov esp, ebp
    jmp _KiServiceExit2

Error:
    pop ebp
    mov esp, ebp
    jmp _KiServiceExit
.endfunc

/* HARDWARE TRAP HANDLERS ****************************************************/

GENERATE_TRAP_HANDLER KiTrap0, 1
GENERATE_TRAP_HANDLER KiTrap1, 1
GENERATE_TRAP_HANDLER KiTrap3, 1
GENERATE_TRAP_HANDLER KiTrap4, 1
GENERATE_TRAP_HANDLER KiTrap5, 1
GENERATE_TRAP_HANDLER KiTrap6, 1
GENERATE_TRAP_HANDLER KiTrap7, 1
GENERATE_TRAP_HANDLER KiTrap8, 0
GENERATE_TRAP_HANDLER KiTrap9, 1
GENERATE_TRAP_HANDLER KiTrap10, 0
GENERATE_TRAP_HANDLER KiTrap11, 0
GENERATE_TRAP_HANDLER KiTrap12, 0
GENERATE_TRAP_HANDLER KiTrap13, 0
GENERATE_TRAP_HANDLER KiTrap14, 0
GENERATE_TRAP_HANDLER KiTrap0F, 1
GENERATE_TRAP_HANDLER KiTrap16, 1
GENERATE_TRAP_HANDLER KiTrap17, 1
GENERATE_TRAP_HANDLER KiTrap19, 1

/* UNEXPECTED INTERRUPT HANDLERS **********************************************/

.globl _KiStartUnexpectedRange@0
_KiStartUnexpectedRange@0:

GENERATE_INT_HANDLERS

.globl _KiEndUnexpectedRange@0
_KiEndUnexpectedRange@0:
    jmp _KiUnexpectedInterruptTail

.func KiUnexpectedInterruptTail
TRAP_FIXUPS kui_a, kui_t, DoFixupV86, DoFixupAbios
_KiUnexpectedInterruptTail:

    /* Enter interrupt trap */
    INT_PROLOG kui_a, kui_t, DoNotPushFakeErrorCode

    /* Increase interrupt count */
    inc dword ptr PCR[KPCR_PRCB_INTERRUPT_COUNT]

    /* Put vector in EBX and make space for KIRQL */
    mov ebx, [esp]
    sub esp, 4

    /* Begin interrupt */
    push esp
    push ebx
    push HIGH_LEVEL
    call _HalBeginSystemInterrupt@12

    /* Check if it was spurious or not */
    or al, al
    jnz Handled

    /* Spurious, ignore it */
    add esp, 8
    jmp _Kei386EoiHelper2ndEntry

Handled:
    /* Unexpected interrupt, print a message on debug builds */
#if DBG
    push [esp+4]
    push offset _UnexpectedMsg
    call _DbgPrint
    add esp, 8
#endif

    /* Exit the interrupt */
    mov esi, $
    cli
    call _HalEndSystemInterrupt@8
    jmp _Kei386EoiHelper@0
.endfunc

.globl _KiUnexpectedInterrupt
_KiUnexpectedInterrupt:

    /* Bugcheck with invalid interrupt code */
    push TRAP_CAUSE_UNKNOWN
    call _KeBugCheck@4

/* INTERRUPT HANDLERS ********************************************************/

.func KiDispatchInterrupt@0
_KiDispatchInterrupt@0:

    /* Get the PCR  and disable interrupts */
    mov ebx, PCR[KPCR_SELF]
    cli

    /* Check if we have to deliver DPCs, timers, or deferred threads */
    mov eax, [ebx+KPCR_PRCB_DPC_QUEUE_DEPTH]
    or eax, [ebx+KPCR_PRCB_TIMER_REQUEST]
    or eax, [ebx+KPCR_PRCB_DEFERRED_READY_LIST_HEAD]
    jz CheckQuantum

    /* Save stack pointer and exception list, then clear it */
    push ebp
    push dword ptr [ebx+KPCR_EXCEPTION_LIST]
    mov dword ptr [ebx+KPCR_EXCEPTION_LIST], -1

    /* Save the stack and switch to the DPC Stack */
    mov edx, esp
    mov esp, [ebx+KPCR_PRCB_DPC_STACK]
    push edx

    /* Deliver DPCs */
    mov ecx, [ebx+KPCR_PRCB]
    call @KiRetireDpcList@4

    /* Restore stack and exception list */
    pop esp
    pop dword ptr [ebx+KPCR_EXCEPTION_LIST]
    pop ebp

CheckQuantum:

    /* Re-enable interrupts */
    sti

    /* Check if we have quantum end */
    cmp byte ptr [ebx+KPCR_PRCB_QUANTUM_END], 0
    jnz QuantumEnd

    /* Check if we have a thread to swap to */
    cmp byte ptr [ebx+KPCR_PRCB_NEXT_THREAD], 0
    je Return

    /* Make space on the stack to save registers */
    sub esp, 3 * 4
    mov [esp+8], esi
    mov [esp+4], edi
    mov [esp+0], ebp

    /* Get the current thread */
    mov edi, [ebx+KPCR_CURRENT_THREAD]

#ifdef CONFIG_SMP
    /* Raise to synch level */
    call _KeRaiseIrqlToSynchLevel@0

    /* Set context swap busy */
    mov byte ptr [edi+KTHREAD_SWAP_BUSY], 1

    /* Acquire the PRCB Lock */
    lock bts dword ptr [ebx+KPCR_PRCB_PRCB_LOCK], 0
    jnb GetNext
    lea ecx, [ebx+KPCR_PRCB_PRCB_LOCK]
    call @KefAcquireSpinLockAtDpcLevel@4
#endif

GetNext:
    /* Get the next thread and clear it */
    mov esi, [ebx+KPCR_PRCB_NEXT_THREAD]
    and dword ptr [ebx+KPCR_PRCB_NEXT_THREAD], 0

    /* Set us as the current running thread */
    mov [ebx+KPCR_CURRENT_THREAD], esi
    mov byte ptr [esi+KTHREAD_STATE_], Running
    mov byte ptr [edi+KTHREAD_WAIT_REASON], WrDispatchInt

    /* Put thread in ECX and get the PRCB in EDX */
    mov ecx, edi
    lea edx, [ebx+KPCR_PRCB_DATA]
    call @KiQueueReadyThread@8

    /* Set APC_LEVEL and do the swap */
    mov cl, APC_LEVEL
    call @KiSwapContextInternal@0

#ifdef CONFIG_SMP
    /* Lower IRQL back to dispatch */
    mov cl, DISPATCH_LEVEL
    call @KfLowerIrql@4
#endif

    /* Restore registers */
    mov ebp, [esp+0]
    mov edi, [esp+4]
    mov esi, [esp+8]
    add esp, 3*4

Return:
    /* All done */
    ret

QuantumEnd:
    /* Disable quantum end and process it */
    mov byte ptr [ebx+KPCR_PRCB_QUANTUM_END], 0
    call _KiQuantumEnd@0
    ret
.endfunc

/*
 * This is how the new-style interrupt template will look like.
 *
 * We setup the stack for a trap frame in the KINTERRUPT DispatchCode itself and
 * then mov the stack address in ECX, since the handlers are FASTCALL. We also
 * need to know the address of the KINTERRUPT. To do this, we maintain the old
 * dynamic patching technique (EDX instead of EDI, however) and let the C API
 * up in KeInitializeInterrupt replace the 0 with the address. Since this is in
 * EDX, it becomes the second parameter for our FASTCALL function.
 *
 * Finally, we jump directly to the C interrupt handler, which will choose the
 * appropriate dispatcher (chained, single, flat, floating) that was setup. The
 * dispatchers themselves are also C FASTCALL functions. This double-indirection
 * maintains the NT model should anything depend on it.
 *
 * Note that since we always jump to the C handler which then jumps to the C
 * dispatcher, the first JMP in the template object is NOT patched anymore since
 * it's static. Also, keep in mind this code is dynamically copied into nonpaged
 * pool! It runs off the KINTERRUPT directly, so you can't just JMP to the code
 * since JMPs are relative, and the location of the JMP below is dynamic. So we
 * use EDI to store the absolute offset, and jump to that instead.
 *
 */
#ifdef HAL_INTERRUPT_SUPPORT_IN_C
.func KiInterruptTemplate
_KiInterruptTemplate:
    push 0
    pushad
    sub esp, KTRAP_FRAME_LENGTH - KTRAP_FRAME_PREVIOUS_MODE
    mov ecx, esp

_KiInterruptTemplate2ndDispatch:
    /* Dummy code, will be replaced by the address of the KINTERRUPT */
    mov edx, 0

_KiInterruptTemplateObject:
    /* Jump to C code */
    mov edi, offset @KiInterruptHandler@8
    jmp edi

_KiInterruptTemplateDispatch:
    /* Marks the end of the template so that the jump above can be edited */
.endfunc

#else

.func KiInterruptTemplate
_KiInterruptTemplate:

    /* Enter interrupt trap */
    INT_PROLOG kit_a, kit_t, DoPushFakeErrorCode

_KiInterruptTemplate2ndDispatch:
    /* Dummy code, will be replaced by the address of the KINTERRUPT */
    mov edi, 0

_KiInterruptTemplateObject:
    /* Dummy jump, will be replaced by the actual jump */
    jmp _KeSynchronizeExecution@12

_KiInterruptTemplateDispatch:
    /* Marks the end of the template so that the jump above can be edited */

TRAP_FIXUPS kit_a, kit_t, DoFixupV86, DoFixupAbios
.endfunc

.func KiChainedDispatch2ndLvl@0
_KiChainedDispatch2ndLvl@0:

NextSharedInt:
    /* Raise IRQL if necessary */
    mov cl, [edi+KINTERRUPT_SYNCHRONIZE_IRQL]
    cmp cl, [edi+KINTERRUPT_IRQL]
    je 1f
    call @KfRaiseIrql@4

1:
    /* Acquire the lock */
    mov esi, [edi+KINTERRUPT_ACTUAL_LOCK]
GetIntLock2:
    ACQUIRE_SPINLOCK(esi, IntSpin2)

    /* Make sure that this interrupt isn't storming */
    VERIFY_INT kid2

    /* Save the tick count */
    mov esi, _KeTickCount

    /* Call the ISR */
    mov eax, [edi+KINTERRUPT_SERVICE_CONTEXT]
    push eax
    push edi
    call [edi+KINTERRUPT_SERVICE_ROUTINE]

    /* Save the ISR result */
    mov bl, al

    /* Check if the ISR timed out */
    add esi, _KiISRTimeout
    cmp _KeTickCount, esi
    jnc ChainedIsrTimeout

ReleaseLock2:
    /* Release the lock */
    mov esi, [edi+KINTERRUPT_ACTUAL_LOCK]
    RELEASE_SPINLOCK(esi)

    /* Lower IRQL if necessary */
    mov cl, [edi+KINTERRUPT_IRQL]
    cmp cl, [edi+KINTERRUPT_SYNCHRONIZE_IRQL]
    je 1f
    call @KfLowerIrql@4

1:
    /* Check if the interrupt is handled */
    or bl, bl
    jnz 1f

    /* Try the next shared interrupt handler */
    mov eax, [edi+KINTERRUPT_INTERRUPT_LIST_HEAD]
    lea edi, [eax-KINTERRUPT_INTERRUPT_LIST_HEAD]
    jmp NextSharedInt

1:
    ret

#ifdef CONFIG_SMP
IntSpin2:
    SPIN_ON_LOCK(esi, GetIntLock2)
#endif

ChainedIsrTimeout:
    /* Print warning message */
    push [edi+KINTERRUPT_SERVICE_ROUTINE]
    push offset _IsrTimeoutMsg
    call _DbgPrint
    add esp,8

    /* Break into debugger, then continue */
    int 3
    jmp ReleaseLock2

    /* Cleanup verification */
    VERIFY_INT_END kid2, 0
.endfunc

.func KiChainedDispatch@0
_KiChainedDispatch@0:

    /* Increase interrupt count */
    inc dword ptr PCR[KPCR_PRCB_INTERRUPT_COUNT]

    /* Save trap frame */
    mov ebp, esp

    /* Save vector and IRQL */
    mov eax, [edi+KINTERRUPT_VECTOR]
    mov ecx, [edi+KINTERRUPT_IRQL]

    /* Save old irql */
    push eax
    sub esp, 4

    /* Begin interrupt */
    push esp
    push eax
    push ecx
    call _HalBeginSystemInterrupt@12

    /* Check if it was handled */
    or al, al
    jz SpuriousInt

    /* Call the 2nd-level handler */
    call _KiChainedDispatch2ndLvl@0

    /* Exit the interrupt */
    INT_EPILOG 0
.endfunc

.func KiInterruptDispatch@0
_KiInterruptDispatch@0:

    /* Increase interrupt count */
    inc dword ptr PCR[KPCR_PRCB_INTERRUPT_COUNT]

    /* Save trap frame */
    mov ebp, esp

    /* Save vector and IRQL */
    mov eax, [edi+KINTERRUPT_VECTOR]
    mov ecx, [edi+KINTERRUPT_SYNCHRONIZE_IRQL]

    /* Save old irql */
    push eax
    sub esp, 4

    /* Begin interrupt */
    push esp
    push eax
    push ecx
    call _HalBeginSystemInterrupt@12

    /* Check if it was handled */
    or al, al
    jz SpuriousInt

    /* Acquire the lock */
    mov esi, [edi+KINTERRUPT_ACTUAL_LOCK]
GetIntLock:
    ACQUIRE_SPINLOCK(esi, IntSpin)

    /* Make sure that this interrupt isn't storming */
    VERIFY_INT kid

    /* Save the tick count */
    mov ebx, _KeTickCount

    /* Call the ISR */
    mov eax, [edi+KINTERRUPT_SERVICE_CONTEXT]
    push eax
    push edi
    call [edi+KINTERRUPT_SERVICE_ROUTINE]

    /* Check if the ISR timed out */
    add ebx, _KiISRTimeout
    cmp _KeTickCount, ebx
    jnc IsrTimeout

ReleaseLock:
    /* Release the lock */
    RELEASE_SPINLOCK(esi)

    /* Exit the interrupt */
    INT_EPILOG 0

SpuriousInt:
    /* Exit the interrupt */
    add esp, 8
    INT_EPILOG 1

#ifdef CONFIG_SMP
IntSpin:
    SPIN_ON_LOCK(esi, GetIntLock)
#endif

IsrTimeout:
    /* Print warning message */
    push [edi+KINTERRUPT_SERVICE_ROUTINE]
    push offset _IsrTimeoutMsg
    call _DbgPrint
    add esp,8

    /* Break into debugger, then continue */
    int 3
    jmp ReleaseLock

    /* Cleanup verification */
    VERIFY_INT_END kid, 0
.endfunc
#endif
