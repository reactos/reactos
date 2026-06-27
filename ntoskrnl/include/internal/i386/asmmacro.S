/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/include/internal/i386/asmmacro.S
 * PURPOSE:         Assembly Macros for Spinlocks and common Trap Code
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

// Arguments for idt
#define INT_32_DPL0                 HEX(08E00)
#define INT_32_DPL3                 HEX(0EE00)

//
// These macros are inlined equivalents of KiAcquire/ReleaseSpinlock, that is,
// they will not be compiled into non-SMP builds. Usage is as follows:
//
// .BeginYourFunction
//      mov reg, lockaddr
//      ACQUIRE_SPINLOCK(reg, .spin)
//      <thread-safe code here>
//      RELEASE_SPINLOCK(reg)
//      <misc code here>
//  retn
//  #IFDEF CONFIG_SMP
//  .spin
//      <any necessary steps to be able to jump back safely>
//       SPIN_ON_LOCK(reg, .BeginYourFunction)
//  #ENDIF
//
#ifdef CONFIG_SMP
#define LOCK lock
#define ACQUIRE_SPINLOCK(x, y) \
    lock bts dword ptr [x], 0; \
    jb y
#define RELEASE_SPINLOCK(x) mov byte ptr [x], 0
#define SPIN_ON_LOCK(x, y) \
1: \
    test dword ptr [x], 1; \
    jz y; \
    pause; \
    jmp 1b
#else
#define LOCK
#define ACQUIRE_SPINLOCK(x, y)
#define RELEASE_SPINLOCK(x)
#endif

//
// @name IDT
//
// This macro creates an IDT entry for the given handler
//
// @param Handler
//        Pointer to the IDT handler
//
// @param Bits
//        Descriptor Bits to associate
//
// @remark None.
//
MACRO(idt, Handler, Bits)
    .long VAL(Handler)
    .short VAL(Bits)
    .short KGDT_R0_CODE
ENDM


#define KI_PUSH_FAKE_ERROR_CODE HEX(0001)
#define KI_UNUSED               HEX(0002)
#define KI_NONVOLATILES_ONLY    HEX(0004)
#define KI_FAST_SYSTEM_CALL     HEX(0008)
#define KI_SOFTWARE_TRAP        HEX(0010)
#define KI_HARDWARE_INT         HEX(0020)
#define KI_DONT_SAVE_SEGS       HEX(0100)

MACRO(KiEnterTrap, Flags)
    LOCAL not_v86_trap
    LOCAL set_sane_segs

    /* Check what kind of trap frame this trap requires */
    if (Flags AND KI_FAST_SYSTEM_CALL)

        /* SYSENTER requires us to build a complete ring transition trap frame */
        FrameSize = KTRAP_FRAME_EIP

        /* Fixup fs. cx is free to clobber */
        mov cx, KGDT_R0_PCR
        mov fs, cx

        /* Get pointer to the TSS */
        mov ecx, fs:[KPCR_TSS]

        /* Get a stack pointer */
        mov esp, [ecx + KTSS_ESP0]

        /* Set up a fake hardware trap frame */
        push KGDT_R3_DATA or RPL_MASK
        push edx
        pushfd
        push KGDT_R3_CODE or RPL_MASK
        push dword ptr ds:[KUSER_SHARED_SYSCALL_RET]

    elseif (Flags AND KI_SOFTWARE_TRAP)

        /* Software traps need a complete non-ring transition trap frame */
        FrameSize = KTRAP_FRAME_ESP

        /* Software traps need to get their EIP from the caller's frame */
        pop eax

    elseif (Flags AND KI_PUSH_FAKE_ERROR_CODE)

        /* If the trap doesn't have an error code, we'll make space for it */
        FrameSize = KTRAP_FRAME_EIP

    else

        /* The trap already has an error code, so just make space for the rest */
        FrameSize = KTRAP_FRAME_ERROR_CODE

    endif

    /* Make space for this frame */
    sub esp, FrameSize

    /* Save nonvolatile registers */
    mov [esp + KTRAP_FRAME_EBP], ebp
    mov [esp + KTRAP_FRAME_EBX], ebx
    mov [esp + KTRAP_FRAME_ESI], esi
    mov [esp + KTRAP_FRAME_EDI], edi

    /* Save eax for system calls, for use by the C handler */
    mov [esp + KTRAP_FRAME_EAX], eax

    /* Does the caller want nonvolatiles only? */
    if (NOT (Flags AND KI_NONVOLATILES_ONLY))
        /* Otherwise, save the volatiles as well */
        mov [esp + KTRAP_FRAME_ECX], ecx
        mov [esp + KTRAP_FRAME_EDX], edx
    endif

    /* Save segment registers? */
    if (Flags AND KI_DONT_SAVE_SEGS)

        /* Initialize TrapFrame segment registers with sane values */
        mov eax, KGDT_R3_DATA OR RPL_MASK
        mov ecx, fs
        mov [esp + KTRAP_FRAME_DS], eax
        mov [esp + KTRAP_FRAME_ES], eax
        mov [esp + KTRAP_FRAME_FS], ecx
        mov dword ptr [esp + KTRAP_FRAME_GS], 0

    else

        /* Check for V86 mode */
        test byte ptr [esp + KTRAP_FRAME_EFLAGS + 2], (EFLAGS_V86_MASK / HEX(10000))
        jz not_v86_trap

            /* Restore V8086 segments into Protected Mode segments */
            mov eax, [esp + KTRAP_FRAME_V86_DS]
            mov ecx, [esp + KTRAP_FRAME_V86_ES]
            mov [esp + KTRAP_FRAME_DS], eax
            mov [esp + KTRAP_FRAME_ES], ecx
            mov eax, [esp + KTRAP_FRAME_V86_FS]
            mov ecx, [esp + KTRAP_FRAME_V86_GS]
            mov [esp + KTRAP_FRAME_FS], eax
            mov [esp + KTRAP_FRAME_GS], ecx
            jmp set_sane_segs

        not_v86_trap:

            /* Save segment selectors */
            mov eax, ds
            mov ecx, es
            mov [esp + KTRAP_FRAME_DS], eax
            mov [esp + KTRAP_FRAME_ES], ecx
            mov eax, fs
            mov ecx, gs
            mov [esp + KTRAP_FRAME_FS], eax
            mov [esp + KTRAP_FRAME_GS], ecx

    endif

set_sane_segs:
    /* Load correct data segments */
    mov ax, KGDT_R3_DATA OR RPL_MASK
    mov ds, ax
    mov es, ax

    /* Fast system calls have fs already fixed */
    if (Flags AND KI_FAST_SYSTEM_CALL)

        /* Enable interrupts and set a sane FS value */
        or dword ptr [esp + KTRAP_FRAME_EFLAGS], EFLAGS_INTERRUPT_MASK
        mov dword ptr [esp + KTRAP_FRAME_FS], KGDT_R3_TEB or RPL_MASK

        /* Set sane active EFLAGS */
        push 2
        popfd

        /* Point edx to the usermode parameters */
        add edx, 8
    else
        /* Otherwise fix fs now */
        mov ax, KGDT_R0_PCR
        mov fs, ax
    endif

    /* Save ExceptionList, since the C handler might use SEH */
    mov eax, fs:[KPCR_EXCEPTION_LIST]
    mov [esp + KTRAP_FRAME_EXCEPTION_LIST], eax

#if DBG
    /* Keep the frame chain intact */
    mov eax, [esp + KTRAP_FRAME_EIP]
    mov [esp + KTRAP_FRAME_DEBUGEIP], eax
    mov [esp + KTRAP_FRAME_DEBUGEBP], ebp
    mov ebp, esp

    /* Tell GDB what just happened */
    CFI_DEF_CFA_REGISTER ebp
    CFI_ADJUST_CFA_OFFSET FrameSize
    CFI_REL_OFFSET ss, KTRAP_FRAME_SS
    CFI_REL_OFFSET gs, KTRAP_FRAME_GS
    CFI_REL_OFFSET fs, KTRAP_FRAME_FS
    CFI_REL_OFFSET es, KTRAP_FRAME_ES
    CFI_REL_OFFSET ds, KTRAP_FRAME_DS
    CFI_REL_OFFSET cs, KTRAP_FRAME_CS

    CFI_REL_OFFSET edi, KTRAP_FRAME_EDI
    CFI_REL_OFFSET esi, KTRAP_FRAME_ESI
    CFI_REL_OFFSET ebx, KTRAP_FRAME_EBX
    CFI_REL_OFFSET ebp, KTRAP_FRAME_EBP
    CFI_REL_OFFSET eip, KTRAP_FRAME_EIP
    CFI_REL_OFFSET esp, KTRAP_FRAME_ESP

if (NOT (Flags AND KI_NONVOLATILES_ONLY))
    CFI_REL_OFFSET eax, KTRAP_FRAME_EAX
    CFI_REL_OFFSET ecx, KTRAP_FRAME_ECX
    CFI_REL_OFFSET edx, KTRAP_FRAME_EDX
endif
#endif

    /* Set parameter 1 (ECX) to point to the frame */
    mov ecx, esp

    /* Clear direction flag */
    cld

ENDM

MACRO(KiCallHandler, Handler)
#if DBG
    /* Use a call to get the return address for back traces */
    call Handler
#else
    /* Use the faster jmp */
    jmp Handler
#endif
    nop
ENDM

MACRO(TRAP_ENTRY, Trap, Flags)
    EXTERN @&Trap&Handler@4 :PROC
    PUBLIC _&Trap
    .PROC _&Trap
        /* Generate proper debugging symbols */
        FPO 0, 0, 0, 0, 1, FRAME_TRAP

        /* Common code to create the trap frame */
        KiEnterTrap Flags

        /* Call the C handler */
        KiCallHandler @&Trap&Handler@4
    .ENDP
ENDM

#define KI_NMI  HEX(0001)

MACRO(TASK_ENTRY, Trap, Flags)
    EXTERN _&Trap&Handler :PROC
    PUBLIC _&Trap
    .PROC _&Trap
        /* Generate proper debugging symbols */
        FPO 0, 0, 0, 0, 0, FRAME_TSS

        /* Call the C handler */
        call _&Trap&Handler

        if (Flags AND KI_NMI)
            /* Return from NMI: return with iret and handle NMI recursion */
            iretd
            jmp _&Trap
        endif

    .ENDP
ENDM

#define KI_RESTORE_EAX        HEX(0001)
#define KI_RESTORE_ECX_EDX    HEX(0002)
#define KI_RESTORE_FS         HEX(0004)
#define KI_RESTORE_SEGMENTS   HEX(0008)
#define KI_RESTORE_EFLAGS     HEX(0010)
#define KI_EXIT_SYSCALL       HEX(0020)
#define KI_EXIT_JMP           HEX(0040)
#define KI_EXIT_RET           HEX(0080)
#define KI_EXIT_IRET          HEX(0100)
#define KI_EDITED_FRAME       HEX(0200)
#define KI_EXIT_RET8          HEX(0400)
#define KI_RESTORE_VOLATILES  (KI_RESTORE_EAX OR KI_RESTORE_ECX_EDX)

#if DBG
#define KI_TRAP_EXIT_FINAL_SNAPSHOT_MAGIC              HEX(4658544B)
#define KI_TRAP_EXIT_FINAL_SNAPSHOT_MAXIMUM_PROCESSORS HEX(20)
#define KI_TRAP_EXIT_FINAL_SNAPSHOT_SIZE               HEX(9C)
#define KI_TRAP_EXIT_FINAL_MAGIC                       HEX(00)
#define KI_TRAP_EXIT_FINAL_VERSION                     HEX(04)
#define KI_TRAP_EXIT_FINAL_SEQUENCE                    HEX(08)
#define KI_TRAP_EXIT_FINAL_STAGE                       HEX(0C)
#define KI_TRAP_EXIT_FINAL_CPU                         HEX(10)
#define KI_TRAP_EXIT_FINAL_FLAGS                       HEX(14)
#define KI_TRAP_EXIT_FINAL_OFFSET_ESP                  HEX(18)
#define KI_TRAP_EXIT_FINAL_TRAP_FRAME                  HEX(1C)
#define KI_TRAP_EXIT_FINAL_EIP                         HEX(20)
#define KI_TRAP_EXIT_FINAL_CS                          HEX(24)
#define KI_TRAP_EXIT_FINAL_EFLAGS                      HEX(28)
#define KI_TRAP_EXIT_FINAL_ESP                         HEX(2C)
#define KI_TRAP_EXIT_FINAL_SS                          HEX(30)
#define KI_TRAP_EXIT_FINAL_DS                          HEX(34)
#define KI_TRAP_EXIT_FINAL_ES                          HEX(38)
#define KI_TRAP_EXIT_FINAL_FS                          HEX(3C)
#define KI_TRAP_EXIT_FINAL_GS                          HEX(40)
#define KI_TRAP_EXIT_FINAL_EAX                         HEX(44)
#define KI_TRAP_EXIT_FINAL_ECX                         HEX(48)
#define KI_TRAP_EXIT_FINAL_EDX                         HEX(4C)
#define KI_TRAP_EXIT_FINAL_LIVE_ESP                    HEX(50)
#define KI_TRAP_EXIT_FINAL_GDTR                        HEX(54)
#define KI_TRAP_EXIT_FINAL_IDTR                        HEX(5A)
#define KI_TRAP_EXIT_FINAL_CR0                         HEX(60)
#define KI_TRAP_EXIT_FINAL_CR2                         HEX(64)
#define KI_TRAP_EXIT_FINAL_CR3                         HEX(68)
#define KI_TRAP_EXIT_FINAL_CR4                         HEX(6C)
#define KI_TRAP_EXIT_FINAL_TR                          HEX(70)
#define KI_TRAP_EXIT_FINAL_GDT_CS_LOW                  HEX(74)
#define KI_TRAP_EXIT_FINAL_GDT_CS_HIGH                 HEX(78)
#define KI_TRAP_EXIT_FINAL_GDT_SS_LOW                  HEX(7C)
#define KI_TRAP_EXIT_FINAL_GDT_SS_HIGH                 HEX(80)
#define KI_TRAP_EXIT_FINAL_GDT_FS_LOW                  HEX(84)
#define KI_TRAP_EXIT_FINAL_GDT_FS_HIGH                 HEX(88)
#define KI_TRAP_EXIT_FINAL_GDT_TR_LOW                  HEX(8C)
#define KI_TRAP_EXIT_FINAL_GDT_TR_HIGH                 HEX(90)
#define KI_TRAP_EXIT_FINAL_TSS_BASE                    HEX(94)
#define KI_TRAP_EXIT_FINAL_TSS_ESP0                    HEX(98)

MACRO(KiTracePageFaultAsmEntry, EventCode)
    pushfd
    pushad

    xor eax, eax
    mov ax, fs
    push eax

    mov ax, KGDT_R0_PCR
    mov fs, ax

    mov esi, [esp + HEX(10)]
    add esi, HEX(4)

    push dword ptr [esi + HEX(10)]
    push dword ptr [esi + HEX(0C)]
    push dword ptr [esi + HEX(08)]
    push dword ptr [esi + HEX(04)]
    push dword ptr [esi]
    mov eax, cr2
    push eax
    push EventCode
    call _KiI386BootTraceRecord@28

    pop eax
    mov fs, ax

    popad
    popfd
ENDM

MACRO(KiTraceTrapExitFrame, EventCode)
    push ecx
    push dword ptr [ecx + KTRAP_FRAME_GS]
    push dword ptr [ecx + KTRAP_FRAME_FS]
    push dword ptr [ecx + KTRAP_FRAME_ES]
    push dword ptr [ecx + KTRAP_FRAME_DS]
    push dword ptr [ecx + KTRAP_FRAME_EIP]
    push ecx
    push EventCode
    call _KiI386BootTraceRecord@28
    pop ecx
ENDM

MACRO(KiTraceTrapExitFinalBegin)
    LOCAL cpu_ready

    movzx edx, byte ptr fs:[PcNumber]
    cmp edx, KI_TRAP_EXIT_FINAL_SNAPSHOT_MAXIMUM_PROCESSORS
    jb cpu_ready
    xor edx, edx
cpu_ready:
    mov eax, edx
    imul edx, edx, KI_TRAP_EXIT_FINAL_SNAPSHOT_SIZE
    add edx, offset _KiTrapExitFinalSnapshotByCpu
    mov [edx + KI_TRAP_EXIT_FINAL_CPU], eax
ENDM

MACRO(KiTraceTrapExitFinalStage, StageValue, FlagsValue, OffsetValue)
    mov dword ptr [edx + KI_TRAP_EXIT_FINAL_MAGIC], KI_TRAP_EXIT_FINAL_SNAPSHOT_MAGIC
    mov dword ptr [edx + KI_TRAP_EXIT_FINAL_VERSION], HEX(1)
    mov dword ptr [edx + KI_TRAP_EXIT_FINAL_STAGE], StageValue
    mov dword ptr [edx + KI_TRAP_EXIT_FINAL_FLAGS], FlagsValue
    mov dword ptr [edx + KI_TRAP_EXIT_FINAL_OFFSET_ESP], OffsetValue

    mov eax, [_KiTrapExitFinalSnapshotCount]
    inc eax
    mov [_KiTrapExitFinalSnapshotCount], eax
    mov [edx + KI_TRAP_EXIT_FINAL_SEQUENCE], eax

    lea eax, [esp - OffsetValue]
    mov [edx + KI_TRAP_EXIT_FINAL_TRAP_FRAME], eax
    mov eax, [esp - OffsetValue + KTRAP_FRAME_EIP]
    mov [edx + KI_TRAP_EXIT_FINAL_EIP], eax
    mov eax, [esp - OffsetValue + KTRAP_FRAME_CS]
    mov [edx + KI_TRAP_EXIT_FINAL_CS], eax
    mov eax, [esp - OffsetValue + KTRAP_FRAME_EFLAGS]
    mov [edx + KI_TRAP_EXIT_FINAL_EFLAGS], eax
    mov eax, [esp - OffsetValue + KTRAP_FRAME_ESP]
    mov [edx + KI_TRAP_EXIT_FINAL_ESP], eax
    mov eax, [esp - OffsetValue + KTRAP_FRAME_SS]
    mov [edx + KI_TRAP_EXIT_FINAL_SS], eax
    mov eax, [esp - OffsetValue + KTRAP_FRAME_DS]
    mov [edx + KI_TRAP_EXIT_FINAL_DS], eax
    mov eax, [esp - OffsetValue + KTRAP_FRAME_ES]
    mov [edx + KI_TRAP_EXIT_FINAL_ES], eax
    mov eax, [esp - OffsetValue + KTRAP_FRAME_FS]
    mov [edx + KI_TRAP_EXIT_FINAL_FS], eax
    mov eax, [esp - OffsetValue + KTRAP_FRAME_GS]
    mov [edx + KI_TRAP_EXIT_FINAL_GS], eax
    mov eax, [esp - OffsetValue + KTRAP_FRAME_EAX]
    mov [edx + KI_TRAP_EXIT_FINAL_EAX], eax
    mov eax, [esp - OffsetValue + KTRAP_FRAME_ECX]
    mov [edx + KI_TRAP_EXIT_FINAL_ECX], eax
    mov eax, [esp - OffsetValue + KTRAP_FRAME_EDX]
    mov [edx + KI_TRAP_EXIT_FINAL_EDX], eax
    mov dword ptr [edx + KI_TRAP_EXIT_FINAL_LIVE_ESP], esp
ENDM

MACRO(KiTraceTrapExitFinalDescriptor, SelectorOffset, LowOffset, HighOffset, OffsetValue)
    movzx ecx, word ptr [esp - OffsetValue + SelectorOffset]
    and ecx, NOT HEX(7)
    mov eax, [edx + KI_TRAP_EXIT_FINAL_GDTR + HEX(2)]
    mov eax, [eax + ecx]
    mov [edx + LowOffset], eax
    movzx ecx, word ptr [esp - OffsetValue + SelectorOffset]
    and ecx, NOT HEX(7)
    mov eax, [edx + KI_TRAP_EXIT_FINAL_GDTR + HEX(2)]
    mov eax, [eax + ecx + HEX(4)]
    mov [edx + HighOffset], eax
ENDM

MACRO(KiTraceTrapExitFinalContract, OffsetValue)
    sgdt [edx + KI_TRAP_EXIT_FINAL_GDTR]
    sidt [edx + KI_TRAP_EXIT_FINAL_IDTR]

    mov eax, cr0
    mov [edx + KI_TRAP_EXIT_FINAL_CR0], eax
    mov eax, cr2
    mov [edx + KI_TRAP_EXIT_FINAL_CR2], eax
    mov eax, cr3
    mov [edx + KI_TRAP_EXIT_FINAL_CR3], eax
    mov eax, cr4
    mov [edx + KI_TRAP_EXIT_FINAL_CR4], eax

    mov dword ptr [edx + KI_TRAP_EXIT_FINAL_TR], HEX(0)
    str word ptr [edx + KI_TRAP_EXIT_FINAL_TR]

    KiTraceTrapExitFinalDescriptor KTRAP_FRAME_CS, KI_TRAP_EXIT_FINAL_GDT_CS_LOW, KI_TRAP_EXIT_FINAL_GDT_CS_HIGH, OffsetValue
    KiTraceTrapExitFinalDescriptor KTRAP_FRAME_SS, KI_TRAP_EXIT_FINAL_GDT_SS_LOW, KI_TRAP_EXIT_FINAL_GDT_SS_HIGH, OffsetValue
    KiTraceTrapExitFinalDescriptor KTRAP_FRAME_FS, KI_TRAP_EXIT_FINAL_GDT_FS_LOW, KI_TRAP_EXIT_FINAL_GDT_FS_HIGH, OffsetValue

    movzx ecx, word ptr [edx + KI_TRAP_EXIT_FINAL_TR]
    and ecx, NOT HEX(7)
    mov eax, [edx + KI_TRAP_EXIT_FINAL_GDTR + HEX(2)]
    mov eax, [eax + ecx]
    mov [edx + KI_TRAP_EXIT_FINAL_GDT_TR_LOW], eax
    movzx ecx, word ptr [edx + KI_TRAP_EXIT_FINAL_TR]
    and ecx, NOT HEX(7)
    mov eax, [edx + KI_TRAP_EXIT_FINAL_GDTR + HEX(2)]
    mov eax, [eax + ecx + HEX(4)]
    mov [edx + KI_TRAP_EXIT_FINAL_GDT_TR_HIGH], eax

    mov eax, [edx + KI_TRAP_EXIT_FINAL_GDT_TR_LOW]
    shr eax, HEX(10)
    and eax, HEX(0000FFFF)
    mov ecx, [edx + KI_TRAP_EXIT_FINAL_GDT_TR_HIGH]
    shl ecx, HEX(10)
    and ecx, HEX(00FF0000)
    or eax, ecx
    mov ecx, [edx + KI_TRAP_EXIT_FINAL_GDT_TR_HIGH]
    and ecx, HEX(FF000000)
    or eax, ecx
    mov [edx + KI_TRAP_EXIT_FINAL_TSS_BASE], eax
    mov eax, [eax + HEX(4)]
    mov [edx + KI_TRAP_EXIT_FINAL_TSS_ESP0], eax
    mov dword ptr [edx + KI_TRAP_EXIT_FINAL_VERSION], HEX(2)
ENDM

MACRO(KiTraceTrapExitSelectorPreflight)
    pushfd
    pushad

    xor eax, eax
    mov ax, ds
    push eax
    xor eax, eax
    mov ax, es
    push eax
    xor eax, eax
    mov ax, fs
    push eax
    xor eax, eax
    mov ax, gs
    push eax

    mov ds, [ecx + KTRAP_FRAME_DS]
    KiTraceTrapExitFrame HEX(E15C)
    mov es, [ecx + KTRAP_FRAME_ES]
    KiTraceTrapExitFrame HEX(E15D)
    mov gs, [ecx + KTRAP_FRAME_GS]
    KiTraceTrapExitFrame HEX(E15E)
    mov fs, [ecx + KTRAP_FRAME_FS]
    mov ax, KGDT_R0_PCR
    mov fs, ax
    KiTraceTrapExitFrame HEX(E15F)

    pop eax
    mov gs, ax
    pop eax
    mov fs, ax
    pop eax
    mov es, ax
    pop eax
    mov ds, ax

    popad
    popfd
ENDM
#endif

MACRO(KiTrapExitStub, Name, Flags)
    LOCAL ret8_instruction
    LOCAL not_nested_int

PUBLIC @&Name&@4
@&Name&@4:

    if (Flags AND KI_EXIT_RET8) OR (Flags AND KI_EXIT_IRET)

        /* This is the IRET frame */
        OffsetEsp = KTRAP_FRAME_EIP

    elseif (Flags AND KI_RESTORE_EFLAGS)

        /* We will pop EFlags off the stack */
        OffsetEsp = KTRAP_FRAME_EFLAGS

    else

        OffsetEsp = 0

    endif

#if DBG
    if (Flags AND KI_EXIT_IRET)

        /* Record the IRET source frame before the stub repoints ESP. */
        push ecx
        push dword ptr [ecx + KTRAP_FRAME_SS]
        push dword ptr [ecx + KTRAP_FRAME_ESP]
        push dword ptr [ecx + KTRAP_FRAME_EFLAGS]
        push dword ptr [ecx + KTRAP_FRAME_CS]
        push dword ptr [ecx + KTRAP_FRAME_EIP]
        push ecx
        push HEX(E15A)
        call _KiI386BootTraceRecord@28
        pop ecx

        push ecx
        push Flags
        push dword ptr [ecx + KTRAP_FRAME_GS]
        push dword ptr [ecx + KTRAP_FRAME_FS]
        push dword ptr [ecx + KTRAP_FRAME_ES]
        push dword ptr [ecx + KTRAP_FRAME_DS]
        push ecx
        push HEX(E15B)
        call _KiI386BootTraceRecord@28
        pop ecx

        if (Flags AND KI_RESTORE_SEGMENTS)
            KiTraceTrapExitSelectorPreflight
        endif

    endif
#endif

    if (Flags AND KI_EDITED_FRAME)

        /* Load the requested ESP */
        mov esp, [ecx + KTRAP_FRAME_TEMPESP]

        /* Put return address on the new stack */
        push [ecx + KTRAP_FRAME_EIP]

        /* Put EFLAGS on the new stack */
        push [ecx + KTRAP_FRAME_EFLAGS]

    else

        /* Point esp to an appropriate member of the frame */
        lea esp, [ecx + OffsetEsp]

    endif

    /* Restore non volatiles */
    mov ebx, [ecx + KTRAP_FRAME_EBX]
    mov esi, [ecx + KTRAP_FRAME_ESI]
    mov edi, [ecx + KTRAP_FRAME_EDI]
    mov ebp, [ecx + KTRAP_FRAME_EBP]

    if (Flags AND KI_RESTORE_EAX)

        /* Restore eax */
        mov eax, [ecx + KTRAP_FRAME_EAX]

    endif

    if (Flags AND KI_RESTORE_ECX_EDX)

        /* Restore volatiles */
        mov edx, [ecx + KTRAP_FRAME_EDX]
        mov ecx, [ecx + KTRAP_FRAME_ECX]

    elseif (Flags AND KI_EXIT_JMP)

        /* Load return address into edx */
        mov edx, [esp - OffsetEsp + KTRAP_FRAME_EIP]

    elseif (Flags AND KI_EXIT_SYSCALL)

        /* Set sysexit parameters */
        mov edx, [esp - OffsetEsp + KTRAP_FRAME_EIP]
        mov ecx, [esp - OffsetEsp + KTRAP_FRAME_ESP]

        /* Keep interrupts disabled until the sti / sysexit */
        and byte ptr [esp - OffsetEsp + KTRAP_FRAME_EFLAGS + 1], NOT (EFLAGS_INTERRUPT_MASK / HEX(100))

    endif

    if (Flags AND KI_RESTORE_SEGMENTS)

        /* Restore segments for user mode */
        KiTraceTrapExitFinalBegin
        KiTraceTrapExitFinalStage HEX(E160), Flags, OffsetEsp
        mov ds, [esp - OffsetEsp + KTRAP_FRAME_DS]
        KiTraceTrapExitFinalStage HEX(E161), Flags, OffsetEsp
        mov es, [esp - OffsetEsp + KTRAP_FRAME_ES]
        KiTraceTrapExitFinalStage HEX(E162), Flags, OffsetEsp
        mov gs, [esp - OffsetEsp + KTRAP_FRAME_GS]
        KiTraceTrapExitFinalStage HEX(E163), Flags, OffsetEsp

    endif

    if ((Flags AND KI_RESTORE_FS) OR (Flags AND KI_RESTORE_SEGMENTS))

        /* Restore user mode FS */
        mov fs, [esp - OffsetEsp + KTRAP_FRAME_FS]

        if (Flags AND KI_RESTORE_SEGMENTS)
            KiTraceTrapExitFinalStage HEX(E164), Flags, OffsetEsp
            KiTraceTrapExitFinalContract OffsetEsp

            if (Flags AND KI_RESTORE_EAX)
                mov eax, [esp - OffsetEsp + KTRAP_FRAME_EAX]
            endif

            if (Flags AND KI_RESTORE_ECX_EDX)
                mov edx, [esp - OffsetEsp + KTRAP_FRAME_EDX]
                mov ecx, [esp - OffsetEsp + KTRAP_FRAME_ECX]
            endif
        endif

    endif

    if (Flags AND KI_RESTORE_EFLAGS)

        if (Flags AND KI_EXIT_RET8)

            /* Check if we return from a nested interrupt, i.e. an interrupt
               that occurred in the ret8 return path between restoring
               EFLAGS and returning with the ret instruction. */
            cmp dword ptr [esp], offset ret8_instruction
            jne not_nested_int

            /* This is a nested interrupt, so we have 2 IRET frames.
               Skip the first, and go directly to the previous return address.
               Do not pass Go. Do not collect $200 */
            add esp, 12

not_nested_int:
            /* We are at the IRET frame, so push EFLAGS first */
            push dword ptr [esp + 8]

        endif

        /* Restore EFLAGS */
        popfd

    endif

    if (Flags AND KI_EXIT_SYSCALL)

        /* Enable interrupts and return to user mode.
           Both must follow directly after another to be "atomic". */
        sti
        sysexit

    elseif (Flags AND KI_EXIT_IRET)

        /* Return with iret */
        iretd

    elseif (Flags AND KI_EXIT_JMP)

        /* Return to kernel mode with a jmp */
        jmp edx

    elseif (Flags AND KI_EXIT_RET8)

        /* Return to kernel mode with a ret 8 */
ret8_instruction:
        ret 8

    elseif (Flags AND KI_EXIT_RET)

        /* Return to kernel mode with a ret */
        ret

    endif

ENDM
