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
idt _KiTrap00,         INT_32_DPL0  /* INT 00: Divide Error (#DE)           */
idt _KiTrap01,         INT_32_DPL0  /* INT 01: Debug Exception (#DB)        */
idt _KiTrap02,         INT_32_DPL0  /* INT 02: NMI Interrupt                */
idt _KiTrap03,         INT_32_DPL3  /* INT 03: Breakpoint Exception (#BP)   */
idt _KiTrap04,         INT_32_DPL3  /* INT 04: Overflow Exception (#OF)     */
idt _KiTrap05,         INT_32_DPL0  /* INT 05: BOUND Range Exceeded (#BR)   */
idt _KiTrap06,         INT_32_DPL0  /* INT 06: Invalid Opcode Code (#UD)    */
idt _KiTrap07,         INT_32_DPL0  /* INT 07: Device Not Available (#NM)   */
idt _KiTrap08,         INT_32_DPL0  /* INT 08: Double Fault Exception (#DF) */
idt _KiTrap09,         INT_32_DPL0  /* INT 09: RESERVED                     */
idt _KiTrap0A,         INT_32_DPL0  /* INT 0A: Invalid TSS Exception (#TS)  */
idt _KiTrap0B,         INT_32_DPL0  /* INT 0B: Segment Not Present (#NP)    */
idt _KiTrap0C,         INT_32_DPL0  /* INT 0C: Stack Fault Exception (#SS)  */
idt _KiTrap0D,         INT_32_DPL0  /* INT 0D: General Protection (#GP)     */
idt _KiTrap0E,         INT_32_DPL0  /* INT 0E: Page-Fault Exception (#PF)   */
idt _KiTrap0F,         INT_32_DPL0  /* INT 0F: RESERVED                     */
idt _KiTrap10,         INT_32_DPL0  /* INT 10: x87 FPU Error (#MF)          */
idt _KiTrap11,         INT_32_DPL0  /* INT 11: Align Check Exception (#AC)  */
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

/* System call entrypoints:                                                 */
.globl _KiFastCallEntry
.globl _KiSystemService

/* And special system-defined software traps:                               */
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
.globl _Kei386EoiHelper@0           /* Exit from interrupt or H/W trap      */

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

/* SOFTWARE INTERRUPT SERVICES ***********************************************/
.text

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
    UNHANDLED_PATH

/* UNEXPECTED INTERRUPT HANDLERS **********************************************/

.globl _KiStartUnexpectedRange@0
_KiStartUnexpectedRange@0:

GENERATE_INT_HANDLERS

.globl _KiEndUnexpectedRange@0
_KiEndUnexpectedRange@0:
    jmp _KiUnexpectedInterruptTail


/* DPC INTERRUPT HANDLER ******************************************************/

.func KiDispatchInterrupt@0
_KiDispatchInterrupt@0:

    /* Preserve EBX */
    push ebx

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
    pop ebx
    ret

QuantumEnd:
    /* Disable quantum end and process it */
    mov byte ptr [ebx+KPCR_PRCB_QUANTUM_END], 0
    call _KiQuantumEnd@0
    pop ebx
    ret
.endfunc

/*
 * We setup the stack for a trap frame in the KINTERRUPT DispatchCode itself and
 * then move the stack address in ECX, since the handlers are FASTCALL. We also
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
.func KiInterruptTemplate
_KiInterruptTemplate:
    TRAP_HANDLER_PROLOG 1, 0

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
