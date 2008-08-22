/*
 * FILE:            ntoskrnl/ke/i386/v86m_sup.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Virtual 8086 (V86) Mode Support
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * NOTE:            See asmmacro.S for the V86 trap code.
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
#include <internal/i386/asmmacro.S>
.intel_syntax noprefix

/* FIXME: Can we make a nice macro to generate V86 Opcode handlers? */

/* GLOBALS *******************************************************************/

//
// This table contains indexes into the OpcodeDispatchV86 Table for opcodes in
// Virtual-8086 Mode.
// There are 256 entries.
//
OpcodeIndex:
    INVALID_V86_OPCODE 15                       /* OP 00-14: UNHANDLED       */
    .byte 1                                     /* OP    0F: 0F              */
    INVALID_V86_OPCODE 22                       /* OP 10-25: UNHANDLED       */
    .byte 2                                     /* OP    26: ES Prefix       */
    INVALID_V86_OPCODE 7                        /* OP 27-2D: UNHANDLED       */
    .byte 3                                     /* OP    2E: CS Prefix       */
    INVALID_V86_OPCODE 7                        /* OP 2F-35: UNHANDLED       */
    .byte 4                                     /* OP    36: SS Prefix       */
    INVALID_V86_OPCODE 7                        /* OP 37-3D: UNHANDLED       */
    .byte 5                                     /* OP    3E: DS Prefix       */
    INVALID_V86_OPCODE 37                       /* OP 3F-63: UNHANDLED       */
    .byte 6                                     /* OP    64: FS Prefix       */
    .byte 7                                     /* OP    65: GS Prefix       */
    .byte 8                                     /* OP    66: OPER32 Prefix   */
    .byte 9                                     /* OP    67: ADDR32 Prefix   */
    INVALID_V86_OPCODE 4                        /* OP 68-6B: UNHANDLED       */
    .byte 10                                    /* OP    6C: INSB            */
    .byte 11                                    /* OP    6D: INSW            */
    .byte 12                                    /* OP    6E: OUTSB           */
    .byte 13                                    /* OP    6F: OUTSW           */
    INVALID_V86_OPCODE 43                       /* OP 70-9A: UNHANDLED       */
    .byte 19                                    /* OP    9B: NPX             */
    .byte 14                                    /* OP    9C: PUSHF           */
    .byte 15                                    /* OP    9D: POPF            */
    INVALID_V86_OPCODE 47                       /* OP 9E-CC: UNHANDLED       */
    .byte 16                                    /* OP    CD: INTnn           */
    .byte 17                                    /* OP    CE: INTO            */
    .byte 18                                    /* OP    CF: IRETD           */
    INVALID_V86_OPCODE 8                        /* OP D0-D7: UNHANDLED       */
    .byte 19                                    /* OP    D8: NPX             */
    .byte 19                                    /* OP    D9: NPX             */
    .byte 19                                    /* OP    DA: NPX             */
    .byte 19                                    /* OP    DB: NPX             */
    .byte 19                                    /* OP    DC: NPX             */
    .byte 19                                    /* OP    DD: NPX             */
    .byte 19                                    /* OP    DE: NPX             */
    .byte 19                                    /* OP    DF: NPX             */
    INVALID_V86_OPCODE 4                        /* OP DE-E3: UNHANDLED       */
    .byte 20                                    /* OP    E4: INBimm          */
    .byte 21                                    /* OP    E5: INWimm          */
    .byte 22                                    /* OP    E6: OUTBimm         */
    .byte 23                                    /* OP    E7: OUTWimm         */
    INVALID_V86_OPCODE 4                        /* OP E8-EB: UNHANDLED       */
    .byte 24                                    /* OP    EC: INB             */
    .byte 25                                    /* OP    EF: INW             */
    .byte 26                                    /* OP    EE: OUTB            */
    .byte 27                                    /* OP    EF: OUTW            */
    .byte 28                                    /* OP    F0: LOCK Prefix     */
    .byte 0                                     /* OP    F1: UNHANDLED       */
    .byte 29                                    /* OP    F2: REPNE Prefix    */
    .byte 30                                    /* OP    F3: REP Prefix      */
    .byte 33                                    /* OP    F4: HLT             */
    INVALID_V86_OPCODE 5                        /* OP F5-F9: UNHANDLED       */
    .byte 31                                    /* OP    FA: CLI             */
    .byte 32                                    /* OP    FB: STI             */
    INVALID_V86_OPCODE 4                        /* OP FC-FF: UNHANDLED       */

//
// This table contains the emulation routines for
// Virtual-8086 Mode. There are 34 entries.
//
OpcodeDispatchV86:
    .long _OpcodeInvalidV86
    .long _Opcode0FV86
    .long _OpcodeESPrefixV86
    .long _OpcodeCSPrefixV86
    .long _OpcodeSSPrefixV86
    .long _OpcodeDSPrefixV86
    .long _OpcodeFSPrefixV86
    .long _OpcodeGSPrefixV86
    .long _OpcodeOPER32PrefixV86
    .long _OpcodeADDR32PrefixV86
    .long _OpcodeINSBV86
    .long _OpcodeINSWV86
    .long _OpcodeOUTSBV86
    .long _OpcodeOUTSWV86
    .long _OpcodePUSHFV86
    .long _OpcodePOPFV86
    .long _OpcodeINTnnV86
    .long _OpcodeINTOV86
    .long _OpcodeIRETV86
    .long _OpcodeNPXV86
    .long _OpcodeINBimmV86
    .long _OpcodeINWimmV86
    .long _OpcodeOUTBimmV86
    .long _OpcodeOUTWimmV86
    .long _OpcodeINBV86
    .long _OpcodeINWV86
    .long _OpcodeOUTBV86
    .long _OpcodeOUTWV86
    .long _OpcodeLOCKPrefixV86
    .long _OpcodeREPNEPrefixV86
    .long _OpcodeREPPrefixV86
    .long _OpcodeCLIV86
    .long _OpcodeSTIV86
    .long _OpcodeHLTV86

_ExVdmOpcodeDispatchCounts:
    .rept 34
        .long 0
    .endr

V86DebugMsg:
    .asciz "Received V86 Emulation Opcode: %lx\n"

/* VIRTUAL-8086 MODE OPCODER HANDLERS ****************************************/

.func OpcodeInvalidV86
_OpcodeInvalidV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func Opcode0FV86
_Opcode0FV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeESPrefixV86
_OpcodeESPrefixV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeCSPrefixV86
_OpcodeCSPrefixV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeSSPrefixV86
_OpcodeSSPrefixV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeDSPrefixV86
_OpcodeDSPrefixV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeFSPrefixV86
_OpcodeFSPrefixV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeGSPrefixV86
_OpcodeGSPrefixV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeOPER32PrefixV86
_OpcodeOPER32PrefixV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeADDR32PrefixV86
_OpcodeADDR32PrefixV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeINSBV86
_OpcodeINSBV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeINSWV86
_OpcodeINSWV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeOUTSBV86
_OpcodeOUTSBV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeOUTSWV86
_OpcodeOUTSWV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodePUSHFV86
_OpcodePUSHFV86:

    /* Get VDM state */
    mov eax, FIXED_NTVDMSTATE_LINEAR_PC_AT
    mov eax, [eax]

    /* Get EFLAGS and mask out IF */
    mov edx, [ebp+KTRAP_FRAME_EFLAGS]
    and eax, ~EFLAGS_INTERRUPT_MASK

    /* Mask align check and interrupt mask */
    and eax, EFLAGS_ALIGN_CHECK + EFLAGS_NESTED_TASK + EFLAGS_INTERRUPT_MASK
    or eax, edx

    /* Add IOPL Mask */
    or eax, EFLAGS_IOPL

    /* Get flat ESP */
    movzx ecx, word ptr [ebp+KTRAP_FRAME_SS]
    shl ecx, 4
    movzx edx, word ptr [ebp+KTRAP_FRAME_ESP]
    sub dx, 2

    /* Check if there is an OPER32 prefix */
    test ebx, 0x4000
    jnz SkipPrefix

    /* Push EFLAGS */
    mov [ecx+edx], ax

UpdateFrame:

    /* Update ESP and EIP */
    mov [ebp+KTRAP_FRAME_ESP], dx
    add [ebp+KTRAP_FRAME_EIP], edi

    /* Return success */
    mov eax, 1
    ret

SkipPrefix:

    /* Skip the prefix, push EFLAGS and jump back */
    sub dx, 2
    mov [edx+ecx], eax
    jmp UpdateFrame
.endfunc

.func OpcodePOPFV86
_OpcodePOPFV86:

    /* Get VDM state */
    mov eax, FIXED_NTVDMSTATE_LINEAR_PC_AT

    /* Get flat ESP */
    mov ecx, [ebp+KTRAP_FRAME_SS]
    shl ecx, 4
    movzx edx, word ptr [ebp+KTRAP_FRAME_ESP]

    /* Pop EFLAGS */
    mov ecx, [ecx+edx]
    add edx, 4

    /* Check for OPER32 prefix */
    test ebx, 0x4000
    jnz NoPrefix

    /* Skip 2 bytes */
    and ecx, 0xFFFF
    sub edx, 2

NoPrefix:

    /* Set new ESP */
    mov [ebp+KTRAP_FRAME_ESP], edx

    /* Mask out EFLAGS */
    and eax, ~EFLAGS_IOPL
    mov ebx, ebx
    and ebx, ~EFLAGS_NESTED_TASK
    and ecx, EFLAGS_ALIGN_CHECK + EFLAGS_NESTED_TASK + EFLAGS_INTERRUPT_MASK

    /* FIXME: Support VME */

    /* Save VDM State pointer */
    push eax

    /* Set new EFLAGS, make sure to add IF and V86 */
    or ebx, EFLAGS_INTERRUPT_MASK + EFLAGS_V86_MASK
    push [ebp+KTRAP_FRAME_EFLAGS]
    mov [ebp+KTRAP_FRAME_EFLAGS], ebx

    /* Make sure we were in V86 mode */
    test ebx, EFLAGS_V86_MASK
    jnz CheckEspAdjust
    int 3

CheckEspAdjust:

    /* Check if we have to update ESP0 and fixup the stack from our push */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    lea esp, [esp+4]
    jnz NoAdjustEsp0

    /* Adjust it */
    push ebp
    call _Ki386AdjustEsp0@4

NoAdjustEsp0:

    /* Restore VDM state */
    pop eax

    /* Update the flags in the VDM State */
    LOCK and dword ptr [eax], ~(EFLAGS_ALIGN_CHECK + EFLAGS_NESTED_TASK + EFLAGS_INTERRUPT_MASK)
    LOCK or [eax], ecx

    /* Update EIP */
    add [ebp+KTRAP_FRAME_EIP], edi

    /* FIXME: Check for VDM Pending interrupts */

    /* Return success */
    mov eax, 1
    ret
.endfunc

.func OpcodeINTnnV86
_OpcodeINTnnV86:

    /* Get EFlags */
    mov edx, [ebp+KTRAP_FRAME_EFLAGS]

    /* Remove the flag in the VDM State */
    mov eax, FIXED_NTVDMSTATE_LINEAR_PC_AT
    mov ecx, [eax]
    LOCK and dword ptr [eax], ~EFLAGS_INTERRUPT_MASK

    /* Mask it out from EFLAGS too */
    mov eax, edx
    and eax, ~EFLAGS_INTERRUPT_MASK

    /* Mask out the alignment check and IF flag from the VDM state */
    and ecx, EFLAGS_ALIGN_CHECK + EFLAGS_INTERRUPT_MASK

    /* FIXME: Support VME */

    /* Now mask out VIF and TF */
    or eax, ecx
    and edx, ~(EFLAGS_VIF + EFLAGS_NESTED_TASK + EFLAGS_TF)
    mov [ebp+KTRAP_FRAME_EFLAGS], edx

    /* Set the IOPL Mask */
    or eax, EFLAGS_IOPL

    /* Get stack flat address */
    movzx ecx, word ptr [ebp+KTRAP_FRAME_SS]
    shl ecx, 4
    movzx edx, word ptr [ebp+KTRAP_FRAME_ESP]

    /* Push EFLAGS */
    sub dx, 2
    mov word ptr [ecx+edx], ax

    /* Push CS */
    mov ax, word ptr [ebp+KTRAP_FRAME_CS]
    sub dx, 2
    mov word ptr [ecx+edx], ax

    /* Push IP */
    movzx eax, word ptr [ebp+KTRAP_FRAME_EIP]
    add eax, edi
    inc eax
    sub dx, 2
    mov word ptr [ecx+edx], ax

    /* Update ESP */
    mov [ebp+KTRAP_FRAME_ESP], dx

    /* Get the interrupt */
    inc esi
    movzx ecx, byte ptr [esi]
    /* FIXME: Analyze and see if this is a hooked VDM (PM) Interrupt */

    /* Get the entry in the IVT */
    mov ebx, [ecx*4]
    mov eax, ebx
    shr eax, 16

    /* Update EIP */
    mov word ptr [ebp+KTRAP_FRAME_EIP], bx

    /* Check if this was V86 mode */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz SetCs

    /* Check if it was a kernel CS */
    or ax, RPL_MASK
    cmp ax, KGDT_R0_CODE
    jnb SetCs

    /* Set user-mode CS */
    mov ax, KGDT_R3_CODE + RPL_MASK

SetCs:
    /* Set new CS */
    mov [ebp+KTRAP_FRAME_CS], ax

    /* Return success */
    mov eax, 1
    ret
.endfunc

.func OpcodeINTOV86
_OpcodeINTOV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeIRETV86
_OpcodeIRETV86:

    /* Get the VDM State */
    mov eax, FIXED_NTVDMSTATE_LINEAR_PC_AT

    /* Get flat ESP */
    movzx ecx, word ptr [ebp+KTRAP_FRAME_SS]
    shl ecx, 4
    movzx edx, word ptr [ebp+KTRAP_FRAME_ESP]
    add ecx, edx

    /* Check for OPER32 prefix */
    test ebx, 0x4000
    jnz Iret32

    /* Get flat IP */
    movzx edi, word ptr [ecx]
    mov [ebp+KTRAP_FRAME_EIP], edi
    movzx esi, word ptr [ecx+2]
    mov [ebp+KTRAP_FRAME_CS], esi

    /* Update ESP */
    add edx, 6
    mov [ebp+KTRAP_FRAME_ESP], edx

    /* Get EFLAGS */
    movzx ebx, word ptr [ecx+4]

MaskEFlags:

    /* Mask out EFLAGS */
    and ebx, ~(EFLAGS_IOPL + EFLAGS_VIF + EFLAGS_NESTED_TASK + EFLAGS_VIP)
    mov ecx, ebx

    /* FIXME: Check for VME support */

    /* Save VDM state */
    push eax

    /* Enable V86 and Interrupts */
    or ebx, EFLAGS_V86_MASK + EFLAGS_INTERRUPT_MASK

    /* Save old flags */
    mov eax, [ebp+KTRAP_FRAME_EFLAGS]
    push eax

    /* Mask out VIP and set new eflags */
    and eax, EFLAGS_VIP
    or eax, ebx
    mov [ebp+KTRAP_FRAME_EFLAGS], eax

    /* Check if we have to update ESP0 */
    pop ebx
    test ebx, EFLAGS_V86_MASK
    jnz NoEsp0Update

    /* Save ECX and ECX */
    push ecx
    push edx

    /* Update esp0 and restore registers */
    push ebp
    call _Ki386AdjustEsp0@4
    pop edx
    pop ecx

NoEsp0Update:

    /* Put VDM state in EAX and update VDM EFlags */
    pop eax
    and ecx, EFLAGS_INTERRUPT_MASK
    LOCK and dword ptr [eax], ~EFLAGS_INTERRUPT_MASK
    LOCK or [eax], ecx

    /* Get flat EIP and check if this is the BOP */
    shl esi, 4
    add esi, edi
    mov ax, [esi]
    cmp ax, 0xC4C4
    jz IsBop

    /* FIXME: Check for VDM interrupts */

    /* Return success */
RetIret:
    mov eax, 1
    ret

IsBop:

    /* Call the BOP handler */
    push ebp
    call _VdmDispatchBop@4
    jmp RetIret

Iret32:

    /* Get 32-bit flat EIP */
    mov edi, [ecx]
    mov [ebp+KTRAP_FRAME_EIP], edi
    movzx esi, word ptr [ecx+4]
    mov [ebp+KTRAP_FRAME_CS], esi

    /* Set new ESP */
    add edx, 12
    mov [ebp+KTRAP_FRAME_ESP], edx

    /* Get EFLAGS and continue */
    mov ebx, [ecx+8]
    jmp MaskEFlags
.endfunc

.func OpcodeNPXV86
_OpcodeNPXV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeINBimmV86
_OpcodeINBimmV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeINWimmV86
_OpcodeINWimmV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeOUTBimmV86
_OpcodeOUTBimmV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeOUTWimmV86
_OpcodeOUTWimmV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeINBV86
_OpcodeINBV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeINWV86
_OpcodeINWV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeOUTBV86
_OpcodeOUTBV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeOUTWV86
_OpcodeOUTWV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeLOCKPrefixV86
_OpcodeLOCKPrefixV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeREPNEPrefixV86
_OpcodeREPNEPrefixV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeREPPrefixV86
_OpcodeREPPrefixV86:
    UNHANDLED_V86_OPCODE
.endfunc

.func OpcodeCLIV86
_OpcodeCLIV86:

    /* Get VDM State */
    mov eax, FIXED_NTVDMSTATE_LINEAR_PC_AT

    /* FIXME: Support VME */

    /* FIXME: Support VDM Interrupts */

    /* Disable interrupts */
    LOCK and dword ptr [eax], ~EFLAGS_INTERRUPT_MASK

    /* Update EIP (remember EDI == instruction size) */
    add [ebp+KTRAP_FRAME_EIP], edi

    /* Return success */
    mov eax, 1
    ret
.endfunc

.func OpcodeSTIV86
_OpcodeSTIV86:

    /* Get VDM State */
    mov eax, FIXED_NTVDMSTATE_LINEAR_PC_AT

    /* FIXME: Support VME */

    /* Enable interrupts */
    LOCK or dword ptr [eax], EFLAGS_INTERRUPT_MASK

    /* Update EIP (remember EDI == instruction size) */
    add [ebp+KTRAP_FRAME_EIP], edi

    /* FIXME: Support VDM Interrupts */

    /* Return success */
    mov eax, 1
    ret
.endfunc

.func OpcodeHLTV86
_OpcodeHLTV86:
    UNHANDLED_V86_OPCODE
.endfunc

/* FUNCTIONS *****************************************************************/

.globl _Ki386SetupAndExitToV86Mode@4
.func Ki386SetupAndExitToV86Mode@4
_Ki386SetupAndExitToV86Mode@4:

    /* Save nonvolatiles */
    push ebp
    push ebx
    push esi
    push edi

    /* Give us a little stack */
    sub esp, 12
    mov ecx, esp

    /* Go past the KTRAP_FRAME and NPX Frame and set a new frame in EAX */
    sub esp, NPX_FRAME_LENGTH
    and esp, ~15
    sub esp, KTRAP_FRAME_LENGTH
    mov eax, esp

    /* Create a fake user-mode frame */
    mov dword ptr [eax+KTRAP_FRAME_CS], KGDT_R0_CODE + RPL_MASK
    mov dword ptr [eax+KTRAP_FRAME_ES], 0
    mov dword ptr [eax+KTRAP_FRAME_DS], 0
    mov dword ptr [eax+KTRAP_FRAME_FS], 0
    mov dword ptr [eax+KTRAP_FRAME_GS], 0
    mov dword ptr [eax+KTRAP_FRAME_ERROR_CODE], 0

    /* Get the current thread's initial stack */
    mov ebx, [fs:KPCR_SELF]
    mov edi, [ebx+KPCR_CURRENT_THREAD]
    mov edx, [edi+KTHREAD_INITIAL_STACK]
    sub edx, NPX_FRAME_LENGTH

    /* Save it on our stack, as well as the real TEB addresses */
    mov [ecx], edx
    mov edx, [edi+KTHREAD_TEB]
    mov [ecx+4], edx
    mov edx, [fs:KPCR_TEB]
    mov [ecx+8] , edx

    /* Set our ESP in ESI, and the return function in EIP */
    mov edi, offset _Ki386BiosCallReturnAddress
    mov [eax+KTRAP_FRAME_ESI], ecx
    mov [eax+KTRAP_FRAME_EIP], edi

    /* Push the flags and sanitize them */
    pushfd
    pop edi
    and edi, 0x60DD7
    or edi, EFLAGS_INTERRUPT_MASK

    /* Set SS and ESP, and fill out the rest of the frame */
    mov dword ptr [eax+KTRAP_FRAME_SS], KGDT_R3_DATA + RPL_MASK;
    mov dword ptr [eax+KTRAP_FRAME_ESP], 0x11FFE;
    mov dword ptr [eax+KTRAP_FRAME_EFLAGS], edi
    mov dword ptr [eax+KTRAP_FRAME_EXCEPTION_LIST], -1
    mov dword ptr [eax+KTRAP_FRAME_PREVIOUS_MODE], -1
    mov dword ptr [eax+KTRAP_FRAME_DR7], 0
    mov dword ptr [eax+KTRAP_FRAME_DEBUGARGMARK], 0xBADB0D00

    /* Jump past the frame now */
    add eax, KTRAP_FRAME_LENGTH
    cli

    /* Save the current stack */
    push ecx

    /* Get the current thread's intial stack again */
    mov edi, [ebx+KPCR_CURRENT_THREAD]
    mov esi, [edi+KTHREAD_INITIAL_STACK]
    sub esi, NPX_FRAME_LENGTH

    /* Set the size of the copy, and the destination, and copy the NPX frame */
    mov ecx, NPX_FRAME_LENGTH / 4
    mov edi, eax
    rep movsd

    /* Restore stack */
    pop ecx

    /* Get the current thread and TSS */
    mov edi, [ebx+KPCR_CURRENT_THREAD]
    mov esi, [ebx+KPCR_TSS]

    /* Bias the V86 vrame */
    sub eax, KTRAP_FRAME_V86_GS - KTRAP_FRAME_SS

    /* Set exception list and new ESP */
    mov dword ptr [ebx+KPCR_EXCEPTION_LIST], -1
    mov [esi+KTSS_ESP0], eax

    /* Now skip past the NPX frame and V86 fields and set this as the intial stack */
    add eax, NPX_FRAME_LENGTH + (KTRAP_FRAME_V86_GS - KTRAP_FRAME_SS)
    mov [edi+KTHREAD_INITIAL_STACK], eax

    /* Setup our fake TEB pointer */
    mov eax, [ecx+0x20]
    mov [fs:KPCR_TEB], eax
    mov [edi+KTHREAD_TEB], eax

    /* Setup the descriptors for the fake TEB */
    mov ebx, [fs:KPCR_GDT]
    mov [ebx+0x3A], ax
    shr eax, 16
    mov [ebx+0x3C], al
    mov [ebx+0x3F], ah
    sti

    /*
     * Start VDM execution. This will save this fake 32-bit KTRAP_FRAME and
     * initialize a real 16-bit VDM context frame
     */
    push 0
    push 0 // VdmStartExecution
    call _NtVdmControl@8

    /* Exit to V86 mode */
    mov ebp, esp
    jmp _Kei386EoiHelper@0
.endfunc

.globl _Ki386BiosCallReturnAddress
.func Ki386BiosCallReturnAddress
_Ki386BiosCallReturnAddress:

    /* Get the PCR */
    mov eax, [fs:KPCR_SELF]

    /* Get NPX destination */
    mov edi, [ebp+KTRAP_FRAME_ESI]
    mov edi, [edi]

    /* Get initial stack */
    mov ecx, [eax+KPCR_CURRENT_THREAD]
    mov esi, [ecx+KTHREAD_INITIAL_STACK]
    sub esi, NPX_FRAME_LENGTH

    /* Set length and copy the NPX frame */
    mov ecx, NPX_FRAME_LENGTH / 4
    rep movsd

    /* Restore stack */
    mov esp, [ebp+KTRAP_FRAME_ESI]
    add esp, 4

    /* Set initial stack */
    mov ecx, [eax+KPCR_CURRENT_THREAD]
    mov [ecx+KTHREAD_INITIAL_STACK], edi

    /* Get TSS and set the ESP 0 */
    mov eax, [eax+KPCR_TSS]
    sub edi, NPX_FRAME_LENGTH + (KTRAP_FRAME_V86_GS - KTRAP_FRAME_SS)
    mov [eax+KTSS_ESP0], edi

    /* Restore KTHREAD TEB in EDX */
    pop edx
    mov [ecx+KTHREAD_TEB], edx

    /* Restore PCR TEB in EDX */
    pop edx
    mov [fs:KPCR_TEB], edx

    /* Setup the descriptors for the real TEB */
    mov ebx, [fs:KPCR_GDT]
    mov [ebx+0x3A], dx
    shr edx, 16
    mov [ebx+0x3C], dl
    mov [ebx+0x3F], dh

    /* Enable interrupts and pop back non-volatiles */
    sti
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret 4
.endfunc

.globl _Ki386HandleOpcodeV86@0
.func Ki386HandleOpcodeV86@0
_Ki386HandleOpcodeV86@0:

    /* Get flat EIP */
    mov esi, [ebp+KTRAP_FRAME_CS]
    shl esi, 4
    add esi, [ebp+KTRAP_FRAME_EIP]

    /* Get the opcode entry in the table */
    movzx ecx, byte ptr [esi]
    movzx edx, byte ptr OpcodeIndex[ecx]

    /* Set instruction length and prefix flags */
    mov edi, 1
    xor ebx, ebx

    /* Accounting statistics */
//    inc dword ptr _ExVdmOpcodeDispatchCounts[edx*4] // FIXME: Generates protection fault

    /* Handle the opcode */
    jmp OpcodeDispatchV86[edx*4]
.endfunc
