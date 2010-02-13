/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/include/i386/asmmacro.S
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
    .long \Handler
    .short \Bits
    .short KGDT_R0_CODE
ENDM


KI_PUSH_FAKE_ERROR_CODE = HEX(0001)
KI_UNUSED               = HEX(0002)
KI_NONVOLATILES_ONLY    = HEX(0004)
KI_FAST_SYSTEM_CALL     = HEX(0008)
KI_SOFTWARE_TRAP        = HEX(0010)
KI_HARDWARE_INT         = HEX(0020)
KI_DONT_SAVE_SEGS       = HEX(0100)

MACRO(KiEnterTrap, Flags)
    LOCAL kernel_trap
    LOCAL not_v86_trap
    LOCAL set_sane_segs

    /* Check what kind of trap frame this trap requires */
    if (Flags AND KI_FAST_SYSTEM_CALL)

        /* SYSENTER requires us to build a complete ring transition trap frame */
        FrameSize = KTRAP_FRAME_V86_ES

        /* Fixup fs. cx is free to clobber */
        mov cx, KGDT_R0_PCR
        mov fs, cx

        /* Get pointer to the TSS */
        mov ecx, fs:[KPCR_TSS]

        /* Get a stack pointer */
        mov esp, [ecx + KTSS_ESP0]

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

    /* Save nonvolatile registers */
    mov [esp - FrameSize + KTRAP_FRAME_EBP], ebp
    mov [esp - FrameSize + KTRAP_FRAME_EBX], ebx
    mov [esp - FrameSize + KTRAP_FRAME_ESI], esi
    mov [esp - FrameSize + KTRAP_FRAME_EDI], edi

    /* Save eax for system calls, for use by the C handler */
    mov [esp - FrameSize + KTRAP_FRAME_EAX], eax

    /* Does the caller want nonvolatiles only? */
    if ((Flags AND KI_NONVOLATILES_ONLY) == 0)
        /* Otherwise, save the volatiles as well */
        mov [esp - FrameSize + KTRAP_FRAME_ECX], ecx
        mov [esp - FrameSize + KTRAP_FRAME_EDX], edx
    endif

    /* Save segment registers? */
    if ((Flags AND KI_DONT_SAVE_SEGS) == 0)

        /* Check for V86 mode */
        test byte ptr [esp - FrameSize + KTRAP_FRAME_EFLAGS + 2], (EFLAGS_V86_MASK >> 16)
        jz not_v86_trap

            /* Restore V8086 segments into Protected Mode segments */
            mov eax, [esp - FrameSize + KTRAP_FRAME_V86_DS]
            mov ecx, [esp - FrameSize + KTRAP_FRAME_V86_ES]
            mov [esp - FrameSize + KTRAP_FRAME_DS], eax
            mov [esp - FrameSize + KTRAP_FRAME_ES], ecx
            mov eax, [esp - FrameSize + KTRAP_FRAME_V86_FS]
            mov ecx, [esp - FrameSize + KTRAP_FRAME_V86_GS]
            mov [esp - FrameSize + KTRAP_FRAME_FS], eax
            mov [esp - FrameSize + KTRAP_FRAME_GS], ecx
            jmp set_sane_segs

        not_v86_trap:

            /* Save segment selectors */
            mov eax, ds
            mov ecx, es
            mov [esp - FrameSize + KTRAP_FRAME_DS], eax
            mov [esp - FrameSize + KTRAP_FRAME_ES], ecx
            mov eax, fs
            mov ecx, gs
            mov [esp - FrameSize + KTRAP_FRAME_FS], eax
            mov [esp - FrameSize + KTRAP_FRAME_GS], ecx

    endif

set_sane_segs:
    /* Load correct data segments */
    mov ax, KGDT_R3_DATA OR RPL_MASK
    mov ds, ax
    mov es, ax

    /* Fast system calls have fs already fixed */
    if ((Flags AND KI_FAST_SYSTEM_CALL) == 0)
        /* Otherwise fix fs now */
        mov ax, KGDT_R0_PCR
        mov fs, ax
    endif

    /* Make space for this frame */
    sub esp, FrameSize

    /* Clear direction flag */
    cld

    /* Set parameter 1 (ECX) to point to the frame */
    mov ecx, esp

ENDM

MACRO(TRAP_ENTRY, Trap, Flags)
EXTERN @&Trap&Handler@4 :PROC
    PUBLIC _&Trap
    _&Trap:
    KiEnterTrap Flags
    jmp @&Trap&Handler@4
ENDM

