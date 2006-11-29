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
.intel_syntax noprefix

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

/* System call entrypoints:                                                 */
.globl _KiFastCallEntry
.globl _KiSystemService

/* And special system-defined software traps:                               */
.globl _NtRaiseException@12
.globl _NtContinue@8
.globl _KiCoprocessorError@0
.globl _KiDispatchInterrupt@0

/* Interrupt template entrypoints                                           */
.globl _KiInterruptTemplate
.globl _KiInterruptTemplateObject
.globl _KiInterruptTemplateDispatch

/* Chained and Normal generic interrupt handlers for 1st and 2nd level entry*/
.globl _KiChainedDispatch2ndLvl@0
.globl _KiInterruptDispatch@0
.globl _KiChainedDispatch@0

/* We implement the following trap exit points:                             */
.globl _KiServiceExit               /* Exit from syscall                    */
.globl _KiServiceExit2              /* Exit from syscall with complete frame*/
.globl _Kei386EoiHelper@0           /* Exit from interrupt or H/W trap      */
.globl _Kei386EoiHelper2ndEntry     /* Exit from unexpected interrupt       */

.globl _KiIdtDescriptor
_KiIdtDescriptor:
    .short 0x800
    .long _KiIdt

.globl _KiUnexpectedEntrySize
_KiUnexpectedEntrySize:
    .long _KiUnexpectedInterrupt1 - _KiUnexpectedInterrupt0

_UnexpectedMsg:
    .asciz "\n\x7\x7!!! Unexpected Interrupt %02lx !!!\n"

/* SOFTWARE INTERRUPT SERVICES ***********************************************/
.text

_KiGetTickCount:
_KiCallbackReturn:
_KiRaiseAssertion:
    int 3

.func KiSystemService
_KiSystemService:

    /* Enter the shared system call prolog */
    SYSCALL_PROLOG

    /* Jump to the actual handler */
    jmp SharedCode
.endfunc

.func KiFastCallEntry
_KiFastCallEntry:

    /* Set FS to PCR */
    mov ecx, KGDT_R0_PCR
    mov fs, cx
    //push KGDT_R0_PCR
    //pop fs

    /* Set user selector */
    mov ecx, KGDT_R3_DATA | RPL_MASK

    /* Set DS/ES to User Selector */
    mov ds, cx
    mov es, cx

    /* Set the current stack to Kernel Stack */
    mov ecx, [fs:KPCR_TSS]
    mov esp, ss:[ecx+KTSS_ESP0]

    /* Set up a fake INT Stack. */
    push KGDT_R3_DATA + RPL_MASK
    push edx                            /* Ring 3 SS:ESP */
    pushf                               /* Ring 3 EFLAGS */
    push 2                              /* Ring 0 EFLAGS */
    add edx, 8                          /* Skip user parameter list */
    popf                                /* Set our EFLAGS */
    or dword ptr [esp], EFLAGS_INTERRUPT_MASK   /* Re-enable IRQs in EFLAGS, to fake INT */
    push KGDT_R3_CODE + RPL_MASK
    push dword ptr ds:KUSER_SHARED_SYSCALL_RET

    /* Setup the Trap Frame stack */
    push 0
    push ebp
    push ebx
    push esi
    push edi
    push KGDT_R3_TEB + RPL_MASK

    /* Save pointer to our PCR */
    mov ebx, [fs:KPCR_SELF]

    /* Get a pointer to the current thread */
    mov esi, [ebx+KPCR_CURRENT_THREAD]

    /* Set the exception handler chain terminator */
    push [ebx+KPCR_EXCEPTION_LIST]
    mov dword ptr [ebx+KPCR_EXCEPTION_LIST], -1

    /* Use the thread's stack */
    mov ebp, [esi+KTHREAD_INITIAL_STACK]

    /* Push previous mode */
    push UserMode

    /* Skip the other registers */
    sub esp, 0x48

    /* Make space for us on the stack */
    sub ebp, 0x29C

    /* Write the previous mode */
    mov byte ptr [esi+KTHREAD_PREVIOUS_MODE], UserMode

    /* Sanity check */
    cmp ebp, esp
    jnz BadStack

    /* Flush DR7 */
    and dword ptr [ebp+KTRAP_FRAME_DR7], 0

    /* Check if the thread was being debugged */
    test byte ptr [esi+KTHREAD_DEBUG_ACTIVE], 0xFF

    /* Set the thread's trap frame */
    mov [esi+KTHREAD_TRAP_FRAME], ebp

    /* Save DR registers if needed */
    //jnz Dr_FastCallDrSave

    /* Set the trap frame debug header */
    SET_TF_DEBUG_HEADER

    /* Enable interrupts */
    sti

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
    mov ecx, [fs:KPCR_TEB]

    /* Check if we should flush the User Batch */
    xor ebx, ebx
    or ebx, [ecx+TEB_GDI_BATCH_COUNT]
    jz NotWin32K

    /* Flush it */
    push edx
    push eax
    //call [_KeGdiFlushUserBatch]
    pop eax
    pop edx

NotWin32K:
    /* Increase total syscall count */
    inc dword ptr fs:[KPCR_SYSTEM_CALLS]

#ifdef DBG
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

CopyParams:
    /* Copy the parameters */
    rep movsd

#ifdef DBG
    /*
     * The following lines are for the benefit of GDB. It will see the return
     * address of the "call ebx" below, find the last label before it and
     * thinks that that's the start of the function. It will then check to see
     * if it starts with a standard function prolog (push ebp, mov ebp,esp1).
     * When that standard function prolog is not found, it will stop the
     * stack backtrace. Since we do want to backtrace into usermode, let's
     * make GDB happy and create a standard prolog.
     */
KiSystemService:
    push ebp
    mov ebp,esp
    pop ebp
#endif

    /* Do the System Call */
    call ebx

AfterSysCall:
#ifdef DBG
    /* Make sure the user-mode call didn't return at elevated IRQL */
    test byte ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jz SkipCheck
    mov esi, eax                /* We need to save the syscall's return val */
    call _KeGetCurrentIrql@0
    or al, al
    jnz InvalidIrql
    mov eax, esi                /* Restore it */

    /* Get our temporary current thread pointer for sanity check */
    mov ecx, fs:[KPCR_CURRENT_THREAD]

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
    mov ecx, [fs:KPCR_CURRENT_THREAD]

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
    jz CopyParams

    /* Caller sent invalid parameters, fail here */
    mov eax, STATUS_ACCESS_VIOLATION
    jmp AfterSysCall

BadStack:

    /* Restore ESP0 stack */
    mov ecx, [fs:KPCR_TSS]
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

#ifdef DBG
InvalidIrql:
    /* Save current IRQL */
    push fs:[KPCR_IRQL]

    /* Set us at passive */
    mov dword ptr fs:[KPCR_IRQL], 0
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
    /* Not yet supported */
    int 3

.func KiDebugService
_KiDebugService:

    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(kids)

    /* Increase EIP so we skip the INT3 */
    //inc dword ptr [ebp+KTRAP_FRAME_EIP]

    /* Call debug service dispatcher */
    mov eax, [ebp+KTRAP_FRAME_EAX]
    mov ecx, [ebp+KTRAP_FRAME_ECX]
    mov edx, [ebp+KTRAP_FRAME_EAX]

    /* Check for V86 mode */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz NotUserMode

    /* Check if this is kernel or user-mode */
    test byte ptr [ebp+KTRAP_FRAME_CS], 1
    jz CallDispatch
    cmp word ptr [ebp+KTRAP_FRAME_CS], KGDT_R3_CODE + RPL_MASK
    jnz NotUserMode

    /* Re-enable interrupts */
VdmProc:
    sti

    /* Call the debug routine */
CallDispatch:
    mov esi, ecx
    mov edi, edx
    mov edx, eax
    mov ecx, 3
    push edi
    push esi
    push edx
    call _KdpServiceDispatcher@12

NotUserMode:

    /* Get the current process */
    mov ebx, [fs:KPCR_CURRENT_THREAD]
    mov ebx, [ebx+KTHREAD_APCSTATE_PROCESS]

    /* Check if this is a VDM Process */
    //cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    //jz VdmProc

    /* Exit through common routine */
    jmp _Kei386EoiHelper@0
.endfunc

.func NtRaiseException@12
_NtRaiseException@12:

    /* NOTE: We -must- be called by Zw* to have the right frame! */
    /* Push the stack frame */
    push ebp

    /* Get the current thread and restore its trap frame */
    mov ebx, [fs:KPCR_CURRENT_THREAD]
    mov edx, [ebp+KTRAP_FRAME_EDX]
    mov [ebx+KTHREAD_TRAP_FRAME], edx

    /* Set up stack frame */
    mov ebp, esp

    /* Get the Trap Frame in EBX */
    mov ebx, [ebp+0]

    /* Get the exception list and restore */
    mov eax, [ebx+KTRAP_FRAME_EXCEPTION_LIST]
    mov [fs:KPCR_EXCEPTION_LIST], eax

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
    mov ebx, [fs:KPCR_CURRENT_THREAD]
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

/* EXCEPTION DISPATCHERS *****************************************************/

.func CommonDispatchException
_CommonDispatchException:

    /* Make space for an exception record */
    sub esp, EXCEPTION_RECORD_LENGTH

    /* Set it up */
    mov [esp+EXCEPTION_RECORD_EXCEPTION_CODE], eax
    xor eax, eax
    mov [esp+EXCEPTION_RECORD_EXCEPTION_FLAGS], eax
    mov [esp+EXCEPTION_RECORD_EXCEPTION_RECORD], eax
    mov [esp+EXCEPTION_RECORD_EXCEPTION_ADDRESS], ebx
    mov [esp+EXCEPTION_RECORD_NUMBER_PARAMETERS], ecx

    /* Check parameter count */
    cmp eax, 0
    jz NoParams

    /* Get information */
    lea ebx, [esp+SIZEOF_EXCEPTION_RECORD]
    mov [ebx], edx
    mov [ebx+4], esi
    mov [ebx+8], edi

NoParams:

    /* Set the record in ECX and check if this was V86 */
    mov ecx, esp
    test dword ptr [esp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jz SetPreviousMode

    /* Set V86 mode */
    mov eax, 0xFFFF
    jmp MaskMode

SetPreviousMode:

    /* Calculate the previous mode */
    mov eax, [ebp+KTRAP_FRAME_CS]
MaskMode:
    and eax, MODE_MASK

    /* Dispatch the exception */
    push 1
    push eax
    push ebp
    push 0
    push ecx
    call _KiDispatchException@20

    /* End the trap */
    mov esp, ebp
    jmp _Kei386EoiHelper@0
.endfunc

.func DispatchNoParam
_DispatchNoParam:
    /* Call the common dispatcher */
    xor ecx, ecx
    call _CommonDispatchException
.endfunc

.func DispatchOneParam
_DispatchOneParam:
    /* Call the common dispatcher */
    xor edx, edx
    mov ecx, 1
    call _CommonDispatchException
.endfunc

.func DispatchTwoParam
_DispatchTwoParam:
    /* Call the common dispatcher */
    xor edx, edx
    mov ecx, 2
    call _CommonDispatchException
.endfunc

/* HARDWARE TRAP HANDLERS ****************************************************/

.func KiTrap0
_KiTrap0:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(0)

    /* Check for V86 */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz V86Int0

    /* Check if the frame was from kernelmode */
    test word ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jz SendException

    /* Check the old mode */
    cmp word ptr [ebp+KTRAP_FRAME_CS], KGDT_R3_CODE + RPL_MASK
    jne VdmCheck

SendException:
    /* Re-enable interrupts for user-mode and send the exception */
    sti
    mov eax, STATUS_INTEGER_DIVIDE_BY_ZERO
    mov ebx, [ebp+KTRAP_FRAME_EIP]
    jmp _DispatchNoParam

VdmCheck:
    /* Check if this is a VDM process */
    mov ebx, [fs:KPCR_CURRENT_THREAD]
    mov ebx, [ebx+KTHREAD_APCSTATE_PROCESS]
    cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    jz SendException

    /* We don't support this yet! */
V86Int0:
    int 3
.endfunc

.func KiTrap1
_KiTrap1:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(1)

    /* Check for V86 */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz V86Int1

    /* Check if the frame was from kernelmode */
    test word ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jz PrepInt1

    /* Check the old mode */
    cmp word ptr [ebp+KTRAP_FRAME_CS], KGDT_R3_CODE + RPL_MASK
    jne V86Int1

EnableInterrupts:
    /* Enable interrupts for user-mode */
    sti

PrepInt1:
    /* Prepare the exception */
    and dword ptr [ebp+KTRAP_FRAME_EFLAGS], ~EFLAGS_TF
    mov ebx, [ebp+KTRAP_FRAME_EIP]
    mov eax, STATUS_SINGLE_STEP
    jmp _DispatchNoParam

V86Int1:
    /* Check if this is a VDM process */
    mov ebx, [fs:KPCR_CURRENT_THREAD]
    mov ebx, [ebx+KTHREAD_APCSTATE_PROCESS]
    cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    jz EnableInterrupts

    /* We don't support VDM! */
    int 3
.endfunc

.globl _KiTrap2
.func KiTrap2
_KiTrap2:

    /* FIXME: This is an NMI, nothing like a normal exception */
    mov eax, 2
    jmp _KiSystemFatalException
.endfunc

.func KiTrap3
_KiTrap3:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(3)

    /* Check for V86 */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz V86Int3

    /* Check if the frame was from kernelmode */
    test word ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jz PrepInt3

    /* Check the old mode */
    cmp word ptr [ebp+KTRAP_FRAME_CS], KGDT_R3_CODE + RPL_MASK
    jne V86Int3

EnableInterrupts3:
    /* Enable interrupts for user-mode */
    sti

PrepInt3:
    /* Prepare the exception */
    mov esi, ecx
    mov edi, edx
    mov edx, eax

    /* Setup EIP, NTSTATUS and parameter count, then dispatch */
    mov ebx, [ebp+KTRAP_FRAME_EIP]
    dec ebx
    mov eax, STATUS_BREAKPOINT
    mov ecx, 3
    call _CommonDispatchException

V86Int3:
    /* Check if this is a VDM process */
    mov ebx, [fs:KPCR_CURRENT_THREAD]
    mov ebx, [ebx+KTHREAD_APCSTATE_PROCESS]
    cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    jz EnableInterrupts3

    /* We don't support VDM! */
    int 3
.endfunc

.func KiTrap4
_KiTrap4:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(4)

    /* Check for V86 */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz V86Int4

    /* Check if the frame was from kernelmode */
    test word ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jz SendException4

    /* Check the old mode */
    cmp word ptr [ebp+KTRAP_FRAME_CS], KGDT_R3_CODE + RPL_MASK
    jne VdmCheck4

SendException4:
    /* Re-enable interrupts for user-mode and send the exception */
    sti
    mov eax, STATUS_INTEGER_OVERFLOW
    mov ebx, [ebp+KTRAP_FRAME_EIP]
    dec ebx
    jmp _DispatchNoParam

VdmCheck4:
    /* Check if this is a VDM process */
    mov ebx, [fs:KPCR_CURRENT_THREAD]
    mov ebx, [ebx+KTHREAD_APCSTATE_PROCESS]
    cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    jz SendException4

    /* We don't support this yet! */
V86Int4:
    int 3
.endfunc

.func KiTrap5
_KiTrap5:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(5)

    /* Check for V86 */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz V86Int5

    /* Check if the frame was from kernelmode */
    test word ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jnz CheckMode

    /* It did, and this should never happen */
    mov eax, 5
    jmp _KiSystemFatalException

    /* Check the old mode */
CheckMode:
    cmp word ptr [ebp+KTRAP_FRAME_CS], KGDT_R3_CODE + RPL_MASK
    jne VdmCheck5

    /* Re-enable interrupts for user-mode and send the exception */
SendException5:
    sti
    mov eax, STATUS_ARRAY_BOUNDS_EXCEEDED
    mov ebx, [ebp+KTRAP_FRAME_EIP]
    jmp _DispatchNoParam

VdmCheck5:
    /* Check if this is a VDM process */
    mov ebx, [fs:KPCR_CURRENT_THREAD]
    mov ebx, [ebx+KTHREAD_APCSTATE_PROCESS]
    cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    jz SendException5

    /* We don't support this yet! */
V86Int5:
    int 3
.endfunc

.func KiTrap6
_KiTrap6:

    /* It this a V86 GPF? */
    test dword ptr [esp+8], EFLAGS_V86_MASK
    jz NotV86UD

    /* Enter V86 Trap */
    V86_TRAP_PROLOG kit6

    /* Not yet supported (Invalid OPCODE from V86) */
    int 3
    jmp $

NotV86UD:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(6)

    /* Check if this happened in kernel mode */
    test byte ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jz KmodeOpcode

    /* Check for VDM */
    cmp word ptr [ebp+KTRAP_FRAME_CS], KGDT_R3_CODE + RPL_MASK
    jz UmodeOpcode

    /* Check if the process is vDM */
    mov ebx, fs:[KPCR_CURRENT_THREAD]
    mov ebx, [ebx+KTHREAD_APCSTATE_PROCESS]
    cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    jnz IsVdmOpcode

UmodeOpcode:
    /* Get EIP and enable interrupts at this point */
    mov esi, [ebp+KTRAP_FRAME_EIP]
    sti

    /* Set intruction prefix length */
    mov ecx, 4

    /* Setup a SEH frame */
    push ebp
    push OpcodeSEH
    push fs:[KPCR_EXCEPTION_LIST]
    mov fs:[KPCR_EXCEPTION_LIST], esp

OpcodeLoop:
    /* Get the instruction and check if it's LOCK */
    mov al, [esi]
    cmp al, 0xF0
    jz LockCrash

    /* Keep moving */
    add esi, 1
    loop OpcodeLoop

    /* Undo SEH frame */
    pop fs:[KPCR_EXCEPTION_LIST]
    add esp, 8

KmodeOpcode:

    /* Re-enable interrupts */
    sti

    /* Setup illegal instruction exception and dispatch it */
    mov ebx, [ebp+KTRAP_FRAME_EIP]
    mov eax, STATUS_ILLEGAL_INSTRUCTION
    jmp _DispatchNoParam

LockCrash:

    /* Undo SEH Frame */
    pop fs:[KPCR_EXCEPTION_LIST]
    add esp, 8

    /* Setup invalid lock exception and dispatch it */
    mov ebx, [ebp+KTRAP_FRAME_EIP]
    mov eax, STATUS_INVALID_LOCK_SEQUENCE
    jmp _DispatchNoParam

IsVdmOpcode:

    /* Unhandled yet */
    int 3
    jmp $

    /* Return to caller */
    jmp _Kei386EoiHelper@0

OpcodeSEH:

    /* Get SEH frame */
    mov esp, [esp+8]
    pop fs:[KPCR_EXCEPTION_LIST]
    add esp, 4
    pop ebp

    /* Check if this was user mode */
    test dword ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jnz KmodeOpcode

    /* Do a bugcheck */
    push ebp
    push 0
    push 0
    push 0
    push 0
    push KMODE_EXCEPTION_NOT_HANDLED
    call _KeBugCheckWithTf@24
.endfunc

.func KiTrap7
_KiTrap7:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(7)

    /* Get the current thread and stack */
StartTrapHandle:
    mov eax, [fs:KPCR_CURRENT_THREAD]
    mov ecx, [eax+KTHREAD_INITIAL_STACK]
    sub ecx, NPX_FRAME_LENGTH

    /* Check if emulation is enabled */
    test dword ptr [ecx+FN_CR0_NPX_STATE], CR0_EM
    jnz EmulationEnabled

CheckState:
    /* Check if the NPX state is loaded */
    cmp byte ptr [eax+KTHREAD_NPX_STATE], NPX_STATE_LOADED
    mov ebx, cr0
    jz IsLoaded

    /* Remove flags */
    and ebx, ~(CR0_MP + CR0_TS + CR0_EM)
    mov cr0, ebx

    /* Check the NPX thread */
    mov edx, [fs:KPCR_NPX_THREAD]
    or edx, edx
    jz NoNpxThread

    /* Get the NPX Stack */
    mov esi, [edx+KTHREAD_INITIAL_STACK]
    sub esi, NPX_FRAME_LENGTH

    /* Check if we have FXSR and check which operand to use */
    test byte ptr _KeI386FxsrPresent, 1
    jz FnSave
    fxsave [esi]
    jmp AfterSave

FnSave:
    fnsave [esi]

AfterSave:
    /* Set the thread's state to dirty */
    mov byte ptr [edx+KTHREAD_NPX_STATE], NPX_STATE_NOT_LOADED

NoNpxThread:
    /* Check if we have FXSR and choose which operand to use */
    test byte ptr _KeI386FxsrPresent, 1
    jz FrRestore
    fxrstor [ecx]
    jmp AfterRestore

FrRestore:
    frstor [esi]

AfterRestore:
    /* Set state loaded */
    mov byte ptr [eax+KTHREAD_NPX_STATE], NPX_STATE_LOADED
    mov [fs:KPCR_NPX_THREAD], eax

    /* Enable interrupts to happen now */
    sti
    nop

    /* Check if CR0 needs to be reloaded due to a context switch */
    cmp dword ptr [ecx+FN_CR0_NPX_STATE], 0
    jz _Kei386EoiHelper@0

    /* We have to reload CR0... disable interrupts */
    cli

    /* Get CR0 and update it */
    mov ebx, cr0
    or ebx, [ecx+FN_CR0_NPX_STATE]
    mov cr0, ebx

    /* Restore interrupts and check if TS is back on */
    sti
    test bl, CR0_TS
    jz _Kei386EoiHelper@0

    /* Clear TS, and loop handling again */
    clts
    cli
    jmp StartTrapHandle

KernelNpx:

    /* Set delayed error */
    or dword ptr [ecx+FN_CR0_NPX_STATE], CR0_TS

    /* Check if this happened during restore */
    cmp dword ptr [ebp+KTRAP_FRAME_EIP], offset FrRestore
    jnz UserNpx

    /* Skip instruction and dispatch the exception */
    add dword ptr [ebp+KTRAP_FRAME_EIP], 3
    jmp _Kei386EoiHelper@0

IsLoaded:
    /* Check if TS is set */
    test bl, CR0_TS
    jnz TsSetOnLoadedState

HandleNpxFault:
    /* Check if the trap came from V86 mode */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz V86Npx

    /* Check if it came from kernel mode */
    test byte ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jz KernelNpx

    /* Check if it came from a VDM */
    cmp word ptr [ebp+KTRAP_FRAME_CS], KGDT_R3_CODE + RPL_MASK
    jne V86Npx

UserNpx:
    /* Get the current thread */
    mov eax, fs:[KPCR_CURRENT_THREAD]

    /* Check NPX state */
    cmp byte ptr [eax+KTHREAD_NPX_STATE], NPX_STATE_NOT_LOADED

    /* Get the NPX save area */
    mov ecx, [eax+KTHREAD_INITIAL_STACK]
    lea ecx, [ecx-NPX_FRAME_LENGTH]
    jz NoSaveRestore

HandleUserNpx:

    /* Set new CR0 */
    mov ebx, cr0
    and ebx, ~(CR0_MP + CR0_EM + CR0_TS)
    mov cr0, ebx

    /* Check if we have FX support */
    test byte ptr _KeI386FxsrPresent, 1
    jz FnSave2

    /* Save the state */
    fxsave [ecx]
    jmp MakeCr0Dirty
FnSave2:
    fnsave [ecx]
    wait

MakeCr0Dirty:
    /* Make CR0 state not loaded */
    or ebx, NPX_STATE_NOT_LOADED
    or ebx, [ecx+FN_CR0_NPX_STATE]
    mov cr0, ebx

    /* Update NPX state */
    mov byte ptr [eax+KTHREAD_NPX_STATE], NPX_STATE_NOT_LOADED
    mov dword ptr fs:[KPCR_NPX_THREAD], 0

NoSaveRestore:
    /* Clear the TS bit and re-enable interrupts */
    and dword ptr [ecx+FN_CR0_NPX_STATE], ~CR0_TS
    sti

    /* Check if we have FX support */
    test byte ptr _KeI386FxsrPresent, 1
    jz FnError

    /* Get error offset, control and status words */
    mov ebx, [ecx+FX_ERROR_OFFSET]
    movzx eax, word ptr [ecx+FX_CONTROL_WORD]
    movzx edx, word ptr [ecx+FX_STATUS_WORD]

    /* Get the faulting opcode */
    mov esi, [ecx+FX_DATA_OFFSET]
    jmp CheckError

FnError:
    /* Get error offset, control and status words */
    mov ebx, [ecx+FP_ERROR_OFFSET]
    movzx eax, word ptr [ecx+FP_CONTROL_WORD]
    movzx edx, word ptr [ecx+FP_STATUS_WORD]

    /* Get the faulting opcode */
    mov esi, [ecx+FP_DATA_OFFSET]

CheckError:
    /* Mask exceptions */
    and eax, 0x3F
    not eax
    and eax, edx

    /* Check if what's left is invalid */
    test al, 1
    jz ValidNpxOpcode

    /* Check if it was a stack fault */
    test al, 64
    jnz InvalidStack

    /* Raise exception */
    mov eax, STATUS_FLOAT_INVALID_OPERATION
    jmp _DispatchOneParam

InvalidStack:

    /* Raise exception */
    mov eax, STATUS_FLOAT_STACK_CHECK
    jmp _DispatchTwoParam

ValidNpxOpcode:

    /* Check for divide by 0 */
    test al, 4
    jz 1f

    /* Raise exception */
    mov eax, STATUS_FLOAT_DIVIDE_BY_ZERO
    jmp _DispatchOneParam

1:
    /* Check for denormal */
    test al, 2
    jz 1f

    /* Raise exception */
    mov eax, STATUS_FLOAT_INVALID_OPERATION
    jmp _DispatchOneParam

1:
    /* Check for overflow */
    test al, 8
    jz 1f

    /* Raise exception */
    mov eax, STATUS_FLOAT_OVERFLOW
    jmp _DispatchOneParam

1:
    /* Check for underflow */
    test al, 16
    jz 1f

    /* Raise exception */
    mov eax, STATUS_FLOAT_UNDERFLOW
    jmp _DispatchOneParam

1:
    /* Check for precision fault */
    test al, 32
    jz UnexpectedNpx

    /* Raise exception */
    mov eax, STATUS_FLOAT_INEXACT_RESULT
    jmp _DispatchOneParam

UnexpectedNpx:

    /* Strange result, bugcheck the OS */
    sti
    push ebp
    push 0
    push 0
    push eax
    push 1
    push TRAP_CAUSE_UNKNOWN
    call _KeBugCheckWithTf@24

V86Npx:
    /* Check if this is a VDM */
    mov eax, fs:[KPCR_CURRENT_THREAD]
    mov ebx, [eax+KTHREAD_APCSTATE_PROCESS]
    cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    jz HandleUserNpx

    /* V86 NPX not handled */
    int 3
    jmp $

EmulationEnabled:
    /* Did this come from kernel-mode? */
    cmp word ptr [ebp+KTRAP_FRAME_CS], KGDT_R0_CODE
    jz CheckState

    /* It came from user-mode, so this would only be valid inside a VDM */
    /* Since we don't actually have VDMs in ROS, bugcheck. */
    jmp UnexpectedNpx

TsSetOnLoadedState:
    /* TS shouldn't be set, unless this we don't have a Math Processor */
    test bl, CR0_MP
    jnz BogusTrap

    /* Strange that we got a trap at all, but ignore and continue */
    clts
    jmp _Kei386EoiHelper@0

BogusTrap:
    /* Cause a bugcheck */
    push 0
    push 0
    push ebx
    push 2
    push TRAP_CAUSE_UNKNOWN
    call _KeBugCheckEx@20
.endfunc

.globl _KiTrap8
.func KiTrap8
_KiTrap8:

    /* Can't really do too much */
    mov eax, 8
    jmp _KiSystemFatalException
.endfunc

.func KiTrap9
_KiTrap9:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(9)

    /* Enable interrupts and bugcheck */
    sti
    mov eax, 9
    jmp _KiSystemFatalException
.endfunc

.func KiTrap10
_KiTrap10:
    /* Enter trap */
    TRAP_PROLOG(10)

    /* Check for V86 */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz V86IntA

    /* Check if the frame was from kernelmode */
    test word ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jz Fatal

V86IntA:
    /* Check if OF was set during iretd */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAG_ZERO
    sti
    jz Fatal

    /* It was, just mask it out */
    and dword ptr [ebp+KTRAP_FRAME_EFLAGS], ~EFLAG_ZERO
    jmp _Kei386EoiHelper@0

Fatal:
    /* TSS failure for some other reason: crash */
    mov eax, 10
    jmp _KiSystemFatalException
.endfunc

.func KiTrap11
_KiTrap11:
    /* Enter trap */
    TRAP_PROLOG(11)

    /* FIXME: ROS Doesn't handle segment faults yet */
    mov eax, 11
    jmp _KiSystemFatalException
.endfunc

.func KiTrap12
_KiTrap12:
    /* Enter trap */
    TRAP_PROLOG(12)

    /* FIXME: ROS Doesn't handle stack faults yet */
    mov eax, 12
    jmp _KiSystemFatalException
.endfunc

.func KiTrap13
_KiTrap13:

    /* It this a V86 GPF? */
    test dword ptr [esp+12], EFLAGS_V86_MASK
    jz NotV86

    /* Enter V86 Trap */
    V86_TRAP_PROLOG kitd

    /* Make sure that this is a V86 process */
    mov ecx, [fs:KPCR_CURRENT_THREAD]
    mov ecx, [ecx+KTHREAD_APCSTATE_PROCESS]
    cmp dword ptr [ecx+EPROCESS_VDM_OBJECTS], 0
    jnz RaiseIrql

    /* Otherwise, something is very wrong, raise an exception */
    sti
    mov ebx, [ebp+KTRAP_FRAME_EIP]
    mov esi, -1
    mov eax, STATUS_ACCESS_VIOLATION
    jmp _DispatchTwoParam

RaiseIrql:

    /* Go to APC level */
    mov ecx, APC_LEVEL
    call @KfRaiseIrql@4

    /* Save old IRQL and enable interrupts */
    push eax
    sti

    /* Handle the opcode */
    call _Ki386HandleOpcodeV86@0

    /* Check if this was VDM */
    test al, 0xFF
    jnz NoReflect

    /* FIXME: TODO */
    int 3

NoReflect:

    /* Lower IRQL and disable interrupts */
    pop ecx
    call @KfLowerIrql@4
    cli

    /* Check if this was a V86 trap */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jz NotV86Trap

    /* Exit the V86 Trap */
    V86_TRAP_EPILOG

NotV86Trap:

    /* Either this wasn't V86, or it was, but an APC interrupted us */
    jmp _Kei386EoiHelper@0

NotV86:
    /* Enter trap */
    TRAP_PROLOG(13)

    /* Check if this was from kernel-mode */
    test dword ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jnz UserModeGpf

    /* FIXME: Check for GPF during GPF */

    /* Get the opcode and trap frame */
    mov eax, [ebp+KTRAP_FRAME_EIP]
    mov eax, [eax]
    mov edx, [ebp+KTRAP_FRAME_EBP]

    /* We want to check if this was POP [DS/ES/FS/GS] */
    add edx, KTRAP_FRAME_DS
    cmp al, 0x1F
    jz SegPopGpf
    add edx, KTRAP_FRAME_ES - KTRAP_FRAME_DS
    cmp al, 7
    jz SegPopGpf
    add edx, KTRAP_FRAME_FS - KTRAP_FRAME_ES
    cmp ax, 0xA10F
    jz SegPopGpf
    add edx, KTRAP_FRAME_GS - KTRAP_FRAME_FS
    cmp ax, 0xA90F
    jz SegPopGpf

    /* It isn't, was it IRETD? */
    cmp al, 0xCF
    jne NotIretGpf

    /* Get error code */
    lea edx, [ebp+KTRAP_FRAME_ESP]
    mov ax, [ebp+KTRAP_FRAME_ERROR_CODE]
    and ax, ~RPL_MASK

    /* Get CS */
    mov cx, word ptr [edx+4]
    and cx, ~RPL_MASK
    cmp cx, ax
    jnz NotCsGpf

    /* This should be a Ki386CallBios return */
    mov eax, offset _Ki386BiosCallReturnAddress
    cmp eax, [edx]
    jne NotBiosGpf
    mov eax, [edx+4]
    cmp ax, KGDT_R0_CODE + RPL_MASK
    jne NotBiosGpf

    /* Jump to return address */
    jmp _Ki386BiosCallReturnAddress

NotBiosGpf:
    /* Check if the thread was in kernel mode */
    mov ebx, [fs:KPCR_CURRENT_THREAD]
    test byte ptr [ebx+KTHREAD_PREVIOUS_MODE], 0xFF
    jz UserModeGpf

    /* Set RPL_MASK for check below */
    or word ptr [edx+4], RPL_MASK

NotCsGpf:
    /* Check if the IRET goes to user-mode */
    test dword ptr [edx+4], RPL_MASK
    jz UserModeGpf

    /* Setup trap frame to copy */
    mov ecx, (KTRAP_FRAME_LENGTH - 12) / 4
    lea edx, [ebp+KTRAP_FRAME_ERROR_CODE]

TrapCopy:

    /* Copy each field */
    mov eax, [edx]
    mov [edx+12], eax
    sub edx, 4
    loop TrapCopy

    /* Enable interrupts and adjust stack */
    sti
    add esp, 12
    add ebp, 12

    /* Setup exception record */
    mov ebx, [ebp+KTRAP_FRAME_EIP]
    mov esi, [ebp+KTRAP_FRAME_ERROR_CODE]
    and esi, 0xFFFF
    mov eax, STATUS_ACCESS_VIOLATION
    jmp _DispatchTwoParam

MsrCheck:

    /* FIXME: Handle RDMSR/WRMSR */
    int 3
    jmp $

NotIretGpf:

    /* Check if this was an MSR opcode */
    cmp al, 0xF
    jz MsrCheck

    /* Check if DS is Ring 3 */
    cmp word ptr [ebp+KTRAP_FRAME_DS], KGDT_R3_DATA + RPL_MASK
    jz CheckEs

    /* Otherwise, fix it up */
    mov dword ptr [ebp+KTRAP_FRAME_DS], KGDT_R3_DATA + RPL_MASK
    jmp ExitGpfTrap

CheckEs:

    /* Check if ES is Ring 3 */
    cmp word ptr [ebp+KTRAP_FRAME_ES], KGDT_R3_DATA + RPL_MASK
    jz UserModeGpf

    /* Otherwise, fix it up */
    mov dword ptr [ebp+KTRAP_FRAME_ES], KGDT_R3_DATA + RPL_MASK
    jmp ExitGpfTrap

SegPopGpf:

    /* Sanity check */
    lea eax, [ebp+KTRAP_FRAME_ESP]
    cmp edx, eax
    jz HandleSegPop
    int 3

    /* Handle segment POP fault by setting it to 0 */
HandleSegPop:
    xor eax, eax
    mov dword ptr [edx], eax

ExitGpfTrap:

    /* Do a trap exit */
    TRAP_EPILOG NotFromSystemCall, DoNotRestorePreviousMode, DoNotRestoreSegments, DoRestoreVolatiles, DoRestoreEverything

UserModeGpf:

    /* FIXME: Unhandled */
    int 3
    jmp $
.endfunc

.func KiTrap14
_KiTrap14:
    /* Enter trap */
    TRAP_PROLOG(14)

    /* Call the C exception handler */
    push 14
    push ebp
    call _KiPageFaultHandler
    add esp, 8

    /* Return to caller */
    jmp _Kei386EoiHelper@0
.endfunc

.func KiTrap0F
_KiTrap0F:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(15)
    sti

    /* Raise a fatal exception */
    mov eax, 15
    jmp _KiSystemFatalException
.endfunc

.func KiTrap16
_KiTrap16:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(16)

    /* Check if this is the NPX Thread */
    mov eax, fs:[KPCR_CURRENT_THREAD]
    cmp eax, fs:[KPCR_NPX_THREAD]

    /* Get the initial stack and NPX frame */
    mov ecx, [eax+KTHREAD_INITIAL_STACK]
    lea ecx, [ecx-NPX_FRAME_LENGTH]

    /* If this is a valid fault, handle it */
    jz HandleNpxFault

    /* Otherwise, re-enable interrupts and set delayed error */
    sti
    or dword ptr [ecx+FN_CR0_NPX_STATE], CR0_TS
    jmp _Kei386EoiHelper@0
.endfunc

.func KiTrap17
_KiTrap17:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(17)

    /* FIXME: ROS Doesn't handle alignment faults yet */
    mov eax, 17
    jmp _KiSystemFatalException
.endfunc

.func KiSystemFatalException
_KiSystemFatalException:

    /* Push the trap frame */
    push ebp

    /* Push empty parameters */
    push 0
    push 0
    push 0

    /* Push trap number and bugcheck code */
    push eax
    push UNEXPECTED_KERNEL_MODE_TRAP
    call _KeBugCheckWithTf@24
    ret
.endfunc

.func KiCoprocessorError@0
_KiCoprocessorError@0:

    /* Get the NPX Thread's Initial stack */
    mov eax, [fs:KPCR_NPX_THREAD]
    mov eax, [eax+KTHREAD_INITIAL_STACK]

    /* Make space for the FPU Save area */
    sub eax, SIZEOF_FX_SAVE_AREA

    /* Set the CR0 State */
    mov dword ptr [eax+FN_CR0_NPX_STATE], 8

    /* Update it */
    mov eax, cr0
    or eax, 8
    mov cr0, eax

    /* Return to caller */
    ret
.endfunc

/* UNEXPECTED INTERRUPT HANDLERS **********************************************/

.globl _KiStartUnexpectedRange@0
_KiStartUnexpectedRange@0:

GENERATE_INT_HANDLERS

.globl _KiEndUnexpectedRange@0
_KiEndUnexpectedRange@0:
    jmp _KiUnexpectedInterruptTail

.func KiUnexpectedInterruptTail
_KiUnexpectedInterruptTail:

    /* Enter interrupt trap */
    INT_PROLOG kui, DoNotPushFakeErrorCode

    /* Increase interrupt count */
    inc dword ptr [fs:KPCR_PRCB_INTERRUPT_COUNT]

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
    mov ebx, [fs:KPCR_SELF]
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
    //mov esp, [ebx+KPCR_PRCB_DPC_STACK]
    push edx

    /* Deliver DPCs */
    mov ecx, [ebx+KPCR_PRCB]
    call @KiRetireDpcList@4

    /* Restore stack and exception list */
    pop esp
    pop dword ptr [ebx]
    pop ebp

CheckQuantum:

    /* Re-enable interrupts */
    sti

    /* Check if we have quantum end */
    cmp byte ptr [ebx+KPCR_PRCB_QUANTUM_END], 0
    jnz QuantumEnd

    /* Check if we have a thread to swap to */
    cmp byte ptr [ebx+KPCR_PRCB_NEXT_THREAD], 0
    jz Return

    /* FIXME: Schedule new thread */
    int 3

Return:
    /* All done */
    ret

QuantumEnd:
    /* Disable quantum end and process it */
    mov byte ptr [ebx+KPCR_PRCB_QUANTUM_END], 0
    call _KiQuantumEnd@0
    ret
.endfunc

.func KiInterruptTemplate
_KiInterruptTemplate:

    /* Enter interrupt trap */
    INT_PROLOG kit, DoPushFakeErrorCode
.endfunc

_KiInterruptTemplate2ndDispatch:
    /* Dummy code, will be replaced by the address of the KINTERRUPT */
    mov edi, 0

_KiInterruptTemplateObject:
    /* Dummy jump, will be replaced by the actual jump */
    jmp _KeSynchronizeExecution@12

_KiInterruptTemplateDispatch:
    /* Marks the end of the template so that the jump above can be edited */

.func KiChainedDispatch2ndLvl@0
_KiChainedDispatch2ndLvl@0:

    /* Not yet supported */
    int 3
.endfunc

.func KiChainedDispatch@0
_KiChainedDispatch@0:

    /* Increase interrupt count */
    inc dword ptr [fs:KPCR_PRCB_INTERRUPT_COUNT]

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
    mov esi, $
    cli
    call _HalEndSystemInterrupt@8
    jmp _Kei386EoiHelper@0
.endfunc

.func KiInterruptDispatch@0
_KiInterruptDispatch@0:

    /* Increase interrupt count */
    inc dword ptr [fs:KPCR_PRCB_INTERRUPT_COUNT]

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
GetIntLock:
    mov esi, [edi+KINTERRUPT_ACTUAL_LOCK]
    ACQUIRE_SPINLOCK(esi, IntSpin)

    /* Call the ISR */
    mov eax, [edi+KINTERRUPT_SERVICE_CONTEXT]
    push eax
    push edi
    call [edi+KINTERRUPT_SERVICE_ROUTINE]

    /* Release the lock */
    RELEASE_SPINLOCK(esi)

    /* Exit the interrupt */
    cli
    call _HalEndSystemInterrupt@8
    jmp _Kei386EoiHelper@0

SpuriousInt:
    /* Exit the interrupt */
    jmp $
    add esp, 8
    jmp _Kei386EoiHelper@0

#ifdef CONFIG_SMP
IntSpin:
    SPIN_ON_LOCK esi, GetIntLock
#endif
.endfunc
