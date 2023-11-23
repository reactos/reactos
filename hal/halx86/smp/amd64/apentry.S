/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Assembly file for real mode AP code
 * COPYRIGHT:   Copyright 2023 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>
#include <ksamd64.inc>

#define ZERO_OFFSET(f) (offset f - offset HalpAPEntry16)

#ifndef _USE_ML
.macro data32, opcode:vararg
    .byte 0x66
    \opcode
.endm
#endif

PUBLIC HalpAPEntry16
PUBLIC HalpAPEntryData
PUBLIC HalpAPEntry16End

.code64 // Workaround to be able to link this code

///////////////////////////////////////////////////////////////////////////////
// 16-bit code
///////////////////////////////////////////////////////////////////////////////

// The following code is executed in real mode.
// We compile it as 64-bit code, because otherwise we cannot link it.
// To fix differences between 64-bit and 16-bt code, we need to manually
// add the data32 (0x66) prefix to some instructions.
// We also need to use indirect addressing, because the 64-bit assembler
// does not support the 16-bit addressing mode.

HalpAPEntry16:

    /* Disable interrupts */
    cli

    /* Use esi = 0 for register relative addressing */
    data32 xor esi, esi

    /* Set up ds segment */
    data32 mov edi, cs
    data32 mov ds, edi

    /* Calculate the flat base address */
    data32 shl edi, 4

    /* Enable A20 address line */
    data32 call EnableA20

    /* Set up far pointer offset for switch to 32 bit */
    data32 lea eax, [edi + ZERO_OFFSET(APEntry32)]
    data32 mov [esi + ZERO_OFFSET(Jump32Offset)], eax

    /* Set up far pointer offset for switch to 64 bit */
    data32 lea eax, [edi + ZERO_OFFSET(APEntry64)]
    data32 mov [esi + ZERO_OFFSET(Jump64Offset)], eax

    /* Set up temporary GDT pointer */
    data32 lea eax, [edi + ZERO_OFFSET(TempGdt)]
    data32 mov [esi + ZERO_OFFSET(TempGdtr_Base)], eax
 
    /* Load the initial GDT */
#ifdef _USE_ML
    lgdt fword ptr [esi + ZERO_OFFSET(TempGdtr)]
#else
    lgdt ds:[esi + ZERO_OFFSET(TempGdtr)]
#endif

    /* Set PAE and PGE: 10100000b */
    mov rax, cr4
    data32 or eax, HEX(00A0)
    mov cr4, rax

    /* Enable protected mode */
    mov rax, cr0
    data32 or eax, HEX(00000001)
    mov cr0, rax

    /* Long jump, 32bit address */
    .byte HEX(66)
    .byte HEX(EA)
Jump32Offset:
    .long 0
    .short HEX(20) // Protected mode CS

Empty8042:
    .word HEX(00eb), HEX(00eb)      // jmp $+2, jmp $+2
    in al, HEX(64)
    cmp al, HEX(0ff)                // legacy-free machine without keyboard
    jz Empty8042_ret                // controllers on Intel Macs read back 0xFF
    test al, 2
    jnz Empty8042
Empty8042_ret:
    data32 ret

EnableA20:
    data32 call Empty8042
    mov al, HEX(0D1)                // command write
    out HEX(064), al
    data32 call Empty8042
    mov al, HEX(0DF)                // A20 on
    out HEX(060), al
    data32 call Empty8042
    mov al, HEX(0FF)                // pulse output port
    out HEX(064), al
    data32 call Empty8042
    data32 ret

.align 8
TempGdt:
    .word HEX(0000), HEX(0000), HEX(0000), HEX(0000) /* 00: NULL descriptor */
    .word HEX(0000), HEX(0000), HEX(0000), HEX(0000) /* 08:  */
    .word HEX(0000), HEX(0000), HEX(9B00), HEX(0020) /* 10: long mode CS */
    .word HEX(FFFF), HEX(0000), HEX(9300), HEX(00CF) /* 18: long mode DS */
    .word HEX(FFFF), HEX(0000), HEX(9B00), HEX(00CF) /* 20: protected mode CS */
    .word HEX(FFFF), HEX(0000), HEX(9300), HEX(00CF) /* 28: protected mode DS */

/* GDT table pointer */
TempGdtr:
    .word HEX(2F)       /* Limit */
TempGdtr_Base:
    .long 0 /* Base */

///////////////////////////////////////////////////////////////////////////////
// 32-bit code
///////////////////////////////////////////////////////////////////////////////

APEntry32:

    /* Load protected mode DS */
    mov eax, HEX(28)
    mov ds, eax

    /* Set up the long mode page table in cr3 */
    lea eax, [rdi + PAGE_SIZE]
    mov cr3, rax

    /* Enable EFER.LME (long mode enable) and NXE (no-execute enable) */
    mov ecx, MSR_EFER
    rdmsr
    or eax, HEX(00000900)
    wrmsr

    /* Enable paging for long mode */
    mov rax, cr0
    or eax, HEX(80000000)
    mov cr0, rax

    /* Long jump to 64 bit segment to activate long mode */
    .byte HEX(EA)
Jump64Offset:
    .long 0
    .short HEX(10) // Long mode CS


///////////////////////////////////////////////////////////////////////////////
// 64-bit code
///////////////////////////////////////////////////////////////////////////////

APEntry64:

    /* Load long mode segments */
    mov ax, KGDT64_R0_DATA
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    /* Load ProcessorState pointer */
    mov rsi, [rdi + ZERO_OFFSET(ProcessorState)]

    /* Load the final GDT and IDT from the ProcessorState */
    lgdt fword ptr [rsi + PsGdtr]
    lidt fword ptr [rsi + PsIdtr]

    /* Set LTR */
    mov ax, KGDT64_SYS_TSS
    ltr ax

    /* Set up the stack */
    mov rsp, [rsi + PsContextFrame + CxRsp]

    /* Set parameters and jump to entry point */
    mov rcx, [rsi + PsContextFrame + CxRcx]
    mov rdx, [rsi + PsContextFrame + CxRdx]
    mov r8, [rsi + PsContextFrame + CxR8]
    mov r9, [rsi + PsContextFrame + CxR9]
    jmp qword ptr [rsi + PsContextFrame + CxRip]

// Layout in sync with AP_ENTRY_DATA in spinup.c
.align 8
HalpAPEntryData:
ProcessorState:
    .quad 0

HalpAPEntry16End:
    nop

END
