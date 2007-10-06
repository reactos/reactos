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
    .short 0
    .short 0x800
    .long _KiIdt

.globl _KiUnexpectedEntrySize
_KiUnexpectedEntrySize:
    .long _KiUnexpectedInterrupt1 - _KiUnexpectedInterrupt0

_UnexpectedMsg:
    .asciz "\n\x7\x7!!! Unexpected Interrupt %02lx !!!\n"

_UnhandledMsg:
    .asciz "\n\x7\x7!!! Unhandled or Unexpected Code at line: %lx!!!\n"

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

_KiGetTickCount:
_KiCallbackReturn:
_KiRaiseAssertion:
    /* FIXME: TODO */
    UNHANDLED_PATH

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
ReadBatch:
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
    inc dword ptr PCR[KPCR_SYSTEM_CALLS]

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
    jz CopyParams

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

#ifdef DBG
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
    UNHANDLED_PATH

.func KiDebugService
TRAP_FIXUPS kids_a, kids_t, DoFixupV86, DoFixupAbios
_KiDebugService:

    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG kids_a, kids_t

    /* Increase EIP so we skip the INT3 */
    inc dword ptr [ebp+KTRAP_FRAME_EIP]

    /* Call debug service dispatcher */
    mov eax, [ebp+KTRAP_FRAME_EAX]
    mov ecx, [ebp+KTRAP_FRAME_ECX]
    mov edx, [ebp+KTRAP_FRAME_EDX]

    /* Jump to INT3 handler */
    jmp PrepareInt3
.endfunc

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
    cmp ecx, 0
    jz NoParams

    /* Get information */
    lea ebx, [esp+SIZEOF_EXCEPTION_RECORD]
    mov [ebx], edx
    mov [ebx+4], esi
    mov [ebx+8], edi

NoParams:

    /* Set the record in ECX and check if this was V86 */
    mov ecx, esp
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jz SetPreviousMode

    /* Set V86 mode */
    mov eax, 0xFFFF
    jmp MaskMode

SetPreviousMode:

    /* Get the caller's CS */
    mov eax, [ebp+KTRAP_FRAME_CS]

MaskMode:
    /* Check if it was user-mode or kernel-mode */
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

.func KiFixupFrame
_KiFixupFrame:

    /* TODO: Routine to fixup a KTRAP_FRAME when faulting from a syscall. */
    UNHANDLED_PATH
.endfunc

.func KiTrap0
TRAP_FIXUPS kit0_a, kit0_t, DoFixupV86, DoNotFixupAbios
_KiTrap0:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG kit0_a, kit0_t

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
    mov ebx, PCR[KPCR_CURRENT_THREAD]
    mov ebx, [ebx+KTHREAD_APCSTATE_PROCESS]
    cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    jz SendException

    /* We don't support this yet! */
V86Int0:
    /* FIXME: TODO */
    UNHANDLED_PATH
.endfunc

.func KiTrap1
TRAP_FIXUPS kit1_a, kit1_t, DoFixupV86, DoNotFixupAbios
_KiTrap1:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG kit1_a, kit1_t

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
    mov ebx, PCR[KPCR_CURRENT_THREAD]
    mov ebx, [ebx+KTHREAD_APCSTATE_PROCESS]
    cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    jz EnableInterrupts

    /* We don't support VDM! */
    UNHANDLED_PATH
.endfunc

.globl _KiTrap2
.func KiTrap2
_KiTrap2:

    /* FIXME: This is an NMI, nothing like a normal exception */
    mov eax, 2
    jmp _KiSystemFatalException
.endfunc

.func KiTrap3
TRAP_FIXUPS kit3_a, kit3_t, DoFixupV86, DoNotFixupAbios
_KiTrap3:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG kit3_a, kit3_t

    /* Set status code */
    mov eax, 0 //STATUS_SUCCESS

    /* Check for V86 */
PrepareInt3:
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
    mov ecx, 3
    mov eax, STATUS_BREAKPOINT
    call _CommonDispatchException

V86Int3:
    /* Check if this is a VDM process */
    mov ebx, PCR[KPCR_CURRENT_THREAD]
    mov ebx, [ebx+KTHREAD_APCSTATE_PROCESS]
    cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    jz EnableInterrupts3

    /* We don't support VDM! */
    UNHANDLED_PATH
.endfunc

.func KiTrap4
TRAP_FIXUPS kit4_a, kit4_t, DoFixupV86, DoNotFixupAbios
_KiTrap4:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG kit4_a, kit4_t

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
    mov ebx, PCR[KPCR_CURRENT_THREAD]
    mov ebx, [ebx+KTHREAD_APCSTATE_PROCESS]
    cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    jz SendException4

    /* We don't support this yet! */
V86Int4:
    UNHANDLED_PATH
.endfunc

.func KiTrap5
TRAP_FIXUPS kit5_a, kit5_t, DoFixupV86, DoNotFixupAbios
_KiTrap5:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG kit5_a, kit5_t

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
    mov ebx, PCR[KPCR_CURRENT_THREAD]
    mov ebx, [ebx+KTHREAD_APCSTATE_PROCESS]
    cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    jz SendException5

    /* We don't support this yet! */
V86Int5:
    UNHANDLED_PATH
.endfunc

.func KiTrap6
TRAP_FIXUPS kit6_a, kit6_t, DoFixupV86, DoNotFixupAbios
_KiTrap6:

    /* It this a V86 GPF? */
    test dword ptr [esp+8], EFLAGS_V86_MASK
    jz NotV86UD

    /* Enter V86 Trap */
    V86_TRAP_PROLOG kit6_a, kit6_v

    /* Not yet supported (Invalid OPCODE from V86) */
    UNHANDLED_PATH

NotV86UD:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG kit6_a, kit6_t

    /* Check if this happened in kernel mode */
    test byte ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jz KmodeOpcode

    /* Check for VDM */
    cmp word ptr [ebp+KTRAP_FRAME_CS], KGDT_R3_CODE + RPL_MASK
    jz UmodeOpcode

    /* Check if the process is vDM */
    mov ebx, PCR[KPCR_CURRENT_THREAD]
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
    push PCR[KPCR_EXCEPTION_LIST]
    mov PCR[KPCR_EXCEPTION_LIST], esp

OpcodeLoop:
    /* Get the instruction and check if it's LOCK */
    mov al, [esi]
    cmp al, 0xF0
    jz LockCrash

    /* Keep moving */
    add esi, 1
    loop OpcodeLoop

    /* Undo SEH frame */
    pop PCR[KPCR_EXCEPTION_LIST]
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
    pop PCR[KPCR_EXCEPTION_LIST]
    add esp, 8

    /* Setup invalid lock exception and dispatch it */
    mov ebx, [ebp+KTRAP_FRAME_EIP]
    mov eax, STATUS_INVALID_LOCK_SEQUENCE
    jmp _DispatchNoParam

IsVdmOpcode:

    /* Unhandled yet */
    UNHANDLED_PATH

    /* Return to caller */
    jmp _Kei386EoiHelper@0

OpcodeSEH:

    /* Get SEH frame */
    mov esp, [esp+8]
    pop PCR[KPCR_EXCEPTION_LIST]
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
TRAP_FIXUPS kit7_a, kit7_t, DoFixupV86, DoNotFixupAbios
_KiTrap7:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG kit7_a, kit7_t

    /* Get the current thread and stack */
StartTrapHandle:
    mov eax, PCR[KPCR_CURRENT_THREAD]
    mov ecx, [eax+KTHREAD_INITIAL_STACK]
    sub ecx, NPX_FRAME_LENGTH

    /* Check if emulation is enabled */
    test byte ptr [ecx+FN_CR0_NPX_STATE], CR0_EM
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
    mov edx, PCR[KPCR_NPX_THREAD]
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
    frstor [ecx]

AfterRestore:
    /* Set state loaded */
    mov byte ptr [eax+KTHREAD_NPX_STATE], NPX_STATE_LOADED
    mov PCR[KPCR_NPX_THREAD], eax

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
    mov eax, PCR[KPCR_CURRENT_THREAD]

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
    mov dword ptr PCR[KPCR_NPX_THREAD], 0

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
    mov eax, PCR[KPCR_CURRENT_THREAD]
    mov ebx, [eax+KTHREAD_APCSTATE_PROCESS]
    cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    jz HandleUserNpx

    /* V86 NPX not handled */
    UNHANDLED_PATH

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
TRAP_FIXUPS kit9_a, kit9_t, DoFixupV86, DoNotFixupAbios
_KiTrap9:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG kit9_a, kit9_t

    /* Enable interrupts and bugcheck */
    sti
    mov eax, 9
    jmp _KiSystemFatalException
.endfunc

.func KiTrap10
TRAP_FIXUPS kita_a, kita_t, DoFixupV86, DoNotFixupAbios
_KiTrap10:
    /* Enter trap */
    TRAP_PROLOG kita_a, kita_t

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
TRAP_FIXUPS kitb_a, kitb_t, DoFixupV86, DoNotFixupAbios
_KiTrap11:
    /* Enter trap */
    TRAP_PROLOG kitb_a, kitb_t

    /* FIXME: ROS Doesn't handle segment faults yet */
    mov eax, 11
    jmp _KiSystemFatalException
.endfunc

.func KiTrap12
TRAP_FIXUPS kitc_a, kitc_t, DoFixupV86, DoNotFixupAbios
_KiTrap12:
    /* Enter trap */
    TRAP_PROLOG kitc_a, kitc_t

    /* FIXME: ROS Doesn't handle stack faults yet */
    mov eax, 12
    jmp _KiSystemFatalException
.endfunc

.func KiTrapExceptHandler
_KiTrapExceptHandler:

    /* Setup SEH handler frame */
    mov esp, [esp+8]
    pop PCR[KPCR_EXCEPTION_LIST]
    add esp, 4
    pop ebp

    /* Check if the fault came from user mode */
    test dword ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jnz SetException

    /* Kernel fault, bugcheck */
    push ebp
    push 0
    push 0
    push 0
    push 0
    push KMODE_EXCEPTION_NOT_HANDLED
    call _KeBugCheckWithTf@24
.endfunc

.func KiTrap13
TRAP_FIXUPS kitd_a, kitd_t, DoFixupV86, DoNotFixupAbios
_KiTrap13:

    /* It this a V86 GPF? */
    test dword ptr [esp+12], EFLAGS_V86_MASK
    jz NotV86

    /* Enter V86 Trap */
    V86_TRAP_PROLOG kitd_a, kitd_v

    /* Make sure that this is a V86 process */
    mov ecx, PCR[KPCR_CURRENT_THREAD]
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
    UNHANDLED_PATH

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
    TRAP_PROLOG kitd_a, kitd_t

    /* Check if this was from kernel-mode */
    test dword ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jnz UserModeGpf

    /* Check if we have a VDM alert */
    cmp dword ptr PCR[KPCR_VDM_ALERT], 0
    jnz VdmAlertGpf

    /* Check for GPF during GPF */
    mov eax, [ebp+KTRAP_FRAME_EIP]
    cmp eax, offset CheckPrivilegedInstruction
    jbe KmodeGpf
    cmp eax, offset CheckPrivilegedInstruction2

    /* FIXME: TODO */
    UNHANDLED_PATH

    /* Get the opcode and trap frame */
KmodeGpf:
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
    mov ebx, PCR[KPCR_CURRENT_THREAD]
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
    UNHANDLED_PATH

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

    /* If the previous mode was kernel, raise a fatal exception */
    mov eax, 13
    test byte ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jz _KiSystemFatalException

    /* Get the process and check which CS this came from */
    mov ebx, PCR[KPCR_CURRENT_THREAD]
    mov ebx, [ebx+KTHREAD_APCSTATE_PROCESS]
    cmp word ptr [ebp+KTRAP_FRAME_CS], KGDT_R3_CODE + RPL_MASK
    jz CheckVdmGpf

    /* Check if this is a VDM */
    cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    jnz DispatchV86Gpf

    /* Enable interrupts and check if we have an error code */
    sti
    cmp word ptr [ebp+KTRAP_FRAME_ERROR_CODE], 0
    jnz SetException
    jmp CheckPrivilegedInstruction

HandleSegPop2:
    /* Update EIP (will be updated below again) */
    add dword ptr [ebp+KTRAP_FRAME_EIP], 1

HandleEsPop:
    /* Clear the segment, update EIP and ESP */
    mov dword ptr [edx], 0
    add dword ptr [ebp+KTRAP_FRAME_EIP], 1
    add dword ptr [ebp+KTRAP_FRAME_ESP], 4
    jmp _Kei386EoiHelper@0

CheckVdmGpf:
    /* Check if this is a VDM */
    cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    jz CheckPrivilegedInstruction

    /* Bring interrupts back */
    sti

    /* Check what kind of instruction this is */
    mov eax, [ebp+KTRAP_FRAME_EIP]
    mov eax, [eax]

    /* FIXME: Check for BOP4 */

    /* Check if this is POP ES */
    mov edx, ebp
    add edx, KTRAP_FRAME_ES
    cmp al, 0x07
    jz HandleEsPop

    /* Check if this is POP FS */
    add edx, KTRAP_FRAME_FS - KTRAP_FRAME_ES
    cmp ax, 0xA10F
    jz HandleSegPop2

    /* Check if this is POP GS */
    add edx, KTRAP_FRAME_GS - KTRAP_FRAME_FS
    cmp ax, 0xA90F
    jz HandleSegPop2

CheckPrivilegedInstruction:
    /* Bring interrupts back */
    sti

    /* Setup a SEH handler */
    push ebp
    push offset _KiTrapExceptHandler
    push PCR[KPCR_EXCEPTION_LIST]
    mov PCR[KPCR_EXCEPTION_LIST], esp

    /* Get EIP */
    mov esi, [ebp+KTRAP_FRAME_EIP]

    /* Setup loop count */
    mov ecx, 15

InstLoop:
    /* Save loop count */
    push ecx

    /* Get the instruction */
    lods byte ptr [esi]

    /* Now lookup in the prefix table */
    mov ecx, 11
    mov edi, offset _KiTrapPrefixTable
    repnz scasb

    /* Restore loop count */
    pop ecx

    /* If it's not a prefix byte, check other instructions */
    jnz NotPrefixByte

    /* FIXME */
    UNHANDLED_PATH

NotPrefixByte:
    /* FIXME: Check if it's a HLT */

    /* Check if the instruction has two bytes */
    cmp al, 0xF
    jne CheckRing3Io

    /* FIXME */
    UNHANDLED_PATH

CheckRing3Io:
    /* Get EFLAGS and IOPL */
    mov ebx, [ebp+KTRAP_FRAME_EFLAGS]
    and ebx, 0x3000
    shr ebx, 12

    /* Check the CS's RPL mask */
    mov ecx, [ebp+KTRAP_FRAME_CS]
    and ecx, RPL_MASK
    cmp ebx, ecx
    jge NotIoViolation

CheckPrivilegedInstruction2:
    /* Check if this is a CLI or STI */
    cmp al, 0xFA
    je IsPrivInstruction
    cmp al, 0xFB
    je IsPrivInstruction

    /* Setup I/O table lookup */
    mov ecx, 13
    mov edi, offset _KiTrapIoTable

    /* Loopup in the table */
    repnz scasb
    jnz NotIoViolation

    /* FIXME: Check IOPM!!! */

IsPrivInstruction:
    /* Cleanup the SEH frame */
    pop PCR[KPCR_EXCEPTION_LIST]
    add esp, 8

    /* Setup the exception */
    mov ebx, [ebp+KTRAP_FRAME_EIP]
    mov eax, STATUS_PRIVILEGED_INSTRUCTION
    jmp _DispatchNoParam

NotIoViolation:
    /* Cleanup the SEH frame */
    pop PCR[KPCR_EXCEPTION_LIST]
    add esp, 8

SetException:
    /* Setup the exception */
    mov ebx, [ebp+KTRAP_FRAME_EIP]
    mov esi, -1
    mov eax, STATUS_ACCESS_VIOLATION
    jmp _DispatchTwoParam

DispatchV86Gpf:
    /* FIXME */
    UNHANDLED_PATH
.endfunc

.func KiTrap14
TRAP_FIXUPS kite_a, kite_t, DoFixupV86, DoNotFixupAbios
_KiTrap14:

    /* Enter trap */
    TRAP_PROLOG kite_a, kite_t

    /* Check if we have a VDM alert */
    cmp dword ptr PCR[KPCR_VDM_ALERT], 0
    jnz VdmAlertGpf

    /* Get the current thread */
    mov edi, PCR[KPCR_CURRENT_THREAD]

    /* Get the stack address of the frame */
    lea eax, [esp+KTRAP_FRAME_LENGTH+NPX_FRAME_LENGTH]
    sub eax, [edi+KTHREAD_INITIAL_STACK]
    jz NoFixUp

    /* This isn't the base frame, check if it's the second */
    cmp eax, -KTRAP_FRAME_EFLAGS
    jb NoFixUp

    /* Check if we have a TEB */
    mov eax, PCR[KPCR_TEB]
    or eax, eax
    jle NoFixUp

    /* Fixup the frame */
    call _KiFixupFrame

    /* Save CR2 */
NoFixUp:
    mov edi, cr2

    /* ROS HACK: Sometimes we get called with INTS DISABLED! WTF? */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_INTERRUPT_MASK
    je HandlePf

    /* Enable interrupts and check if we got here with interrupts disabled */
    sti
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_INTERRUPT_MASK
    jz IllegalState

HandlePf:
    /* Send trap frame and check if this is kernel-mode or usermode */
    push ebp
    mov eax, [ebp+KTRAP_FRAME_CS]
    and eax, MODE_MASK
    push eax

    /* Send faulting address and check if this is read or write */
    push edi
    mov eax, [ebp+KTRAP_FRAME_ERROR_CODE]
    and eax, 1
    push eax

    /* Call the access fault handler */
    call _MmAccessFault@16
    test eax, eax
    jl AccessFail

    /* Access fault handled, return to caller */
    jmp _Kei386EoiHelper@0

AccessFail:
    /* First check if this is a fault in the S-LIST functions */
    mov ecx, offset _ExpInterlockedPopEntrySListFault@0
    cmp [ebp+KTRAP_FRAME_EIP], ecx
    jz SlistFault

    /* Check if this is a fault in the syscall handler */
    mov ecx, offset CopyParams
    cmp [ebp+KTRAP_FRAME_EIP], ecx
    jz SysCallCopyFault
    mov ecx, offset ReadBatch
    cmp [ebp+KTRAP_FRAME_EIP], ecx
    jnz CheckVdmPf

    /* FIXME: TODO */
    UNHANDLED_PATH
    jmp _Kei386EoiHelper@0

SysCallCopyFault:
    /* FIXME: TODO */
    UNHANDLED_PATH
    jmp _Kei386EoiHelper@0

    /* Check if the fault occured in a V86 mode */
CheckVdmPf:
    mov ecx, [ebp+KTRAP_FRAME_ERROR_CODE]
    and ecx, 1
    shr ecx, 1
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz VdmPF

    /* Check if the fault occured in a VDM */
    mov esi, PCR[KPCR_CURRENT_THREAD]
    mov esi, [esi+KTHREAD_APCSTATE_PROCESS]
    cmp dword ptr [esi+EPROCESS_VDM_OBJECTS], 0
    jz CheckStatus

    /* Check if we this was in kernel-mode */
    test byte ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jz CheckStatus
    cmp word ptr [ebp+KTRAP_FRAME_CS], KGDT_R3_CODE + RPL_MASK
    jz CheckStatus

VdmPF:
    /* FIXME: TODO */
    UNHANDLED_PATH

    /* Save EIP and check what kind of status failure we got */
CheckStatus:
    mov esi, [ebp+KTRAP_FRAME_EIP]
    cmp eax, STATUS_ACCESS_VIOLATION
    je AccessViol
    cmp eax, STATUS_GUARD_PAGE_VIOLATION
    je SpecialCode
    cmp eax, STATUS_STACK_OVERFLOW
    je SpecialCode

    /* Setup an in-page exception to dispatch */
    mov edx, ecx
    mov ebx, esi
    mov esi, edi
    mov ecx, 3
    mov edi, eax
    mov eax, STATUS_IN_PAGE_ERROR
    call _CommonDispatchException

AccessViol:
    /* Use more proper status code */
    mov eax, KI_EXCEPTION_ACCESS_VIOLATION

SpecialCode:
    /* Setup a normal page fault exception */
    mov ebx, esi
    mov edx, ecx
    mov esi, edi
    jmp _DispatchTwoParam

SlistFault:
    /* FIXME: TODO */
    UNHANDLED_PATH

IllegalState:

    /* This is completely illegal, bugcheck the system */
    push ebp
    push esi
    push ecx
    push eax
    push edi
    push IRQL_NOT_LESS_OR_EQUAL
    call _KeBugCheckWithTf@24

VdmAlertGpf:

    /* FIXME: NOT SUPPORTED */
    UNHANDLED_PATH
.endfunc

.func KiTrap0F
TRAP_FIXUPS kitf_a, kitf_t, DoFixupV86, DoNotFixupAbios
_KiTrap0F:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG kitf_a, kitf_t
    sti

    /* Raise a fatal exception */
    mov eax, 15
    jmp _KiSystemFatalException
.endfunc

.func KiTrap16
TRAP_FIXUPS kit10_a, kit10_t, DoFixupV86, DoNotFixupAbios
_KiTrap16:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG kit10_a, kit10_t

    /* Check if this is the NPX Thread */
    mov eax, PCR[KPCR_CURRENT_THREAD]
    cmp eax, PCR[KPCR_NPX_THREAD]

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
TRAP_FIXUPS kit11_a, kit11_t, DoFixupV86, DoNotFixupAbios
_KiTrap17:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG kit11_a, kit11_t

    /* FIXME: ROS Doesn't handle alignment faults yet */
    mov eax, 17
    jmp _KiSystemFatalException
.endfunc

.globl _KiTrap19
.func KiTrap19
TRAP_FIXUPS kit19_a, kit19_t, DoFixupV86, DoNotFixupAbios
_KiTrap19:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG kit19_a, kit19_t

    /* Check if this is the NPX Thread */
    mov eax, PCR[KPCR_CURRENT_THREAD]
    cmp eax, PCR[KPCR_NPX_THREAD]

    /* If this is a valid fault, handle it */
    jz HandleXmmiFault

    /* Otherwise, bugcheck */
    mov eax, 19
    jmp _KiSystemFatalException

HandleXmmiFault:
    /* Get the initial stack and NPX frame */
    mov ecx, [eax+KTHREAD_INITIAL_STACK]
    lea ecx, [ecx-NPX_FRAME_LENGTH]

    /* Check if the trap came from V86 mode */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz V86Xmmi

    /* Check if it came from kernel mode */
    test byte ptr [ebp+KTRAP_FRAME_CS], MODE_MASK
    jz KernelXmmi

    /* Check if it came from a VDM */
    cmp word ptr [ebp+KTRAP_FRAME_CS], KGDT_R3_CODE + RPL_MASK
    jne VdmXmmi

HandleUserXmmi:
    /* Set new CR0 */
    mov ebx, cr0
    and ebx, ~(CR0_MP + CR0_EM + CR0_TS)
    mov cr0, ebx

    /* Check if we have FX support */
    test byte ptr _KeI386FxsrPresent, 1
    jz XmmiFnSave2

    /* Save the state */
    fxsave [ecx]
    jmp XmmiMakeCr0Dirty
XmmiFnSave2:
    fnsave [ecx]
    wait

XmmiMakeCr0Dirty:
    /* Make CR0 state not loaded */
    or ebx, NPX_STATE_NOT_LOADED
    or ebx, [ecx+FN_CR0_NPX_STATE]
    mov cr0, ebx

    /* Update NPX state */
    mov byte ptr [eax+KTHREAD_NPX_STATE], NPX_STATE_NOT_LOADED
    mov dword ptr PCR[KPCR_NPX_THREAD], 0

    /* Clear the TS bit and re-enable interrupts */
    and dword ptr [ecx+FN_CR0_NPX_STATE], ~CR0_TS

    /* Re-enable interrupts for user-mode and send the exception */
    sti
    mov ebx, [ebp+KTRAP_FRAME_EIP]

    /* Get MxCSR and get current mask (bits 7-12) */
    movzx eax, word ptr [ecx+FX_MXCSR]
    mov edx, eax
    shr edx, 7
    not edx

    /* Set faulting opcode address to 0 */
    mov esi, 0

    /* Apply legal exceptions mask */
    and eax, 0x3f

    /* Apply the mask we got in MXCSR itself */
    and eax, edx

    /* Check for invalid operation */
    test al, 1
    jz 1f

    /* Raise exception */
    mov eax, STATUS_FLOAT_MULTIPLE_TRAPS
    jmp _DispatchOneParam

1:
    /* Check for zero divide */
    test al, 2
    jz 1f

    /* Raise exception */
    mov eax, STATUS_FLOAT_MULTIPLE_TRAPS
    jmp _DispatchOneParam

1:
    /* Check for denormal */
    test al, 4
    jz 1f

    /* Raise exception */
    mov eax, STATUS_FLOAT_MULTIPLE_TRAPS
    jmp _DispatchOneParam

1:
    /* Check for overflow*/
    test al, 8
    jz 1f

    /* Raise exception */
    mov eax, STATUS_FLOAT_MULTIPLE_FAULTS
    jmp _DispatchOneParam

1:
    /* Check for denormal */
    test al, 16
    jz 1f

    /* Raise exception */
    mov eax, STATUS_FLOAT_MULTIPLE_FAULTS
    jmp _DispatchOneParam

1:
    /* Check for Precision */
    test al, 32
    jz UnexpectedXmmi

    /* Raise exception */
    mov eax, STATUS_FLOAT_MULTIPLE_FAULTS
    jmp _DispatchOneParam

UnexpectedXmmi:
    /* Strange result, bugcheck the OS */
    sti
    push ebp
    push 1
    push 0
    push eax
    push 13
    push TRAP_CAUSE_UNKNOWN
    call _KeBugCheckWithTf@24

VdmXmmi:
    /* Check if this is a VDM */
    mov eax, PCR[KPCR_CURRENT_THREAD]
    mov ebx, [eax+KTHREAD_APCSTATE_PROCESS]
    cmp dword ptr [ebx+EPROCESS_VDM_OBJECTS], 0
    jz HandleUserXmmi

V86Xmmi:
    /* V86 XMMI not handled */
    UNHANDLED_PATH

KernelXmmi:
    /* Another weird situation */
    push ebp
    push 2
    push 0
    push eax
    push 13
    push TRAP_CAUSE_UNKNOWN
    call _KeBugCheckWithTf@24
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
    mov eax, PCR[KPCR_NPX_THREAD]
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

.func Ki16BitStackException
_Ki16BitStackException:

    /* Save stack */
    push ss
    push esp

    /* Go to kernel mode thread stack */
    mov eax, PCR[KPCR_CURRENT_THREAD]
    add esp, [eax+KTHREAD_INITIAL_STACK]

    /* Switch to good stack segment */
    UNHANDLED_PATH
.endfunc

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
    #error SMP Interrupt not handled!
#endif

    /* Get the next thread and clear it */
    mov esi, [ebx+KPCR_PRCB_NEXT_THREAD]
    and dword ptr [ebx+KPCR_PRCB_NEXT_THREAD], 0

    /* Set us as the current running thread */
    mov [ebx+KPCR_CURRENT_THREAD], esi
    mov byte ptr [esi+KTHREAD_STATE], Running
    mov byte ptr [edi+KTHREAD_WAIT_REASON], WrDispatchInt

    /* Put thread in ECX and get the PRCB in EDX */
    mov ecx, edi
    lea edx, [ebx+KPCR_PRCB_DATA]
    call @KiQueueReadyThread@8

    /* Set APC_LEVEL and do the swap */
    mov cl, APC_LEVEL
    call @KiSwapContextInternal@0

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

    /* Not yet supported */
    UNHANDLED_PATH
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
GetIntLock:
    mov esi, [edi+KINTERRUPT_ACTUAL_LOCK]
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
    SPIN_ON_LOCK esi, GetIntLock
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

.globl _KeSynchronizeExecution@12
.func KeSynchronizeExecution@12
_KeSynchronizeExecution@12:

    /* Save EBX and put the interrupt object in it */
    push ebx
    mov ebx, [esp+8]

    /* Go to DIRQL */
    mov cl, [ebx+KINTERRUPT_SYNCHRONIZE_IRQL]
    call @KfRaiseIrql@4

    /* Call the routine */
    push eax
    push [esp+20]
    call [esp+20]

    /* Lower IRQL */
    mov ebx, eax
    pop ecx
    call @KfLowerIrql@4

    /* Return status */
    mov eax, ebx
    pop ebx
    ret 12
.endfunc
