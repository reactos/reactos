
#include <asm.inc>
#include "../../include/arch/pc/x86common.h"

#define IMAGE_DOS_HEADER_e_lfanew 60
#define IMAGE_FILE_HEADER_SIZE 20
#define IMAGE_OPTIONAL_HEADER_AddressOfEntryPoint 16

.code16

/* FAT helper code */
#include "fathelp.inc"

.org 512
RealModeEntryPoint:

    cli

    /* Setup segment registers */
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    /* Setup the stack */
    mov sp, STACK16ADDR

    /* Enable A20 address line */
    call EnableA20

    /* Update BSS_CurrentBaseAddress, instruction pointer and code segment */
    call UpdateCurrentCodeSegment

    /* Output first status */
    mov si, offset Msg_Starting
    call cs_writestr

    /* Save real mode entry point in shared memory */
    mov dword ptr ds:[BSS_RealModeEntry], offset switch_to_real16 + FREELDR_BASE

    /* Address the image with es segment */
    mov ax, cs
    add ax, (FREELDR_PE_BASE - FREELDR_BASE) / 16
    mov es, ax

    /* Get address of optional header */
    mov eax, dword ptr es:[IMAGE_DOS_HEADER_e_lfanew]
    add eax, 4 + IMAGE_FILE_HEADER_SIZE

    /* Get offset of entry point */
    mov eax, dword ptr es:[eax + IMAGE_OPTIONAL_HEADER_AddressOfEntryPoint]

    /* Save entry point */
    mov dword ptr cs:[pm_entrypoint], eax

    /* Restore es */
    xor ax, ax
    mov es, ax

    jmp exit_to_protected

Msg_Starting:
    .ascii "Starting FreeLoader...", CR, LF, NUL

/* This is the entry point from protected mode */
switch_to_real16:

    /* Disable Protected Mode */
    mov eax, cr0
    and eax, CR0_PE_CLR
    mov cr0, eax

    /* Clear prefetch queue & correct CS */
    ljmp16 FREELDR_BASE / 16, inrmode

inrmode:
    /* Set real mode segments */
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    /* Clear out the high 16-bits of ESP */
    /* This is needed because I have one */
    /* machine that hangs when booted to dos if */
    /* anything other than 0x0000 is in the high */
    /* 16-bits of ESP. Even though real-mode */
    /* code should only use SP and not ESP. */
    xor esp, esp

    /* Restore real mode stack */
    mov sp, word ptr cs:[stack16]

    /* Load IDTR with real mode value */
    lidt lXdtPrefix cs:[rmode_idtptr]

    sti /* These are ok now */

    /* Do the callback, specified by bx */
    shl bx, 1
    call word ptr cs:CallbackTable[bx]


/*
 * Switches the processor to protected mode
 * it destroys eax
 */
exit_to_protected:

    cli

    /* Save current stack pointer */
    mov word ptr cs:[stack16], sp

    /* Set ds to cs segment register */
    mov ax, cs
    mov ds, ax

    /* Copy the gdt entries to GDTDATA_BASE */
    mov cx, offset gdt_end - gdt
    mov si, offset gdt
    mov di, GDTDATA_BASE
    rep movsb

    /* Copy the protected mode code to TEMPCODE16_BASE */
    mov cx, offset pm_entrypoint_end - inpmode
    mov si, offset inpmode
    mov di, TEMPCODE16_BASE
    rep movsb

    /* Convert the entry point offset to absolute address */
    mov eax, dword ptr ss:[BSS_CurrentBaseAddress]
    add eax, FREELDR_PE_BASE - FREELDR_BASE
    add dword ptr ss:[(pm_entrypoint - inpmode) + TEMPCODE16_BASE], eax

    /* Reset ds */
    xor ax, ax
    mov ds, ax

    /* Load the GDT */
    lgdt lXdtPrefix cs:[gdtptr]

    /* Enable Protected Mode */
    mov eax, cr0
    or eax, CR0_PE_SET
    mov cr0, eax

    /* Clear prefetch queue & correct CS */
    ljmp16 PMODE_CS, TEMPCODE16_BASE
inpmode:
    .byte HEX(0ff), HEX(25) // opcode of indirect jump
#ifdef _USE_ML
    .long (offset pm_entrypoint - inpmode) + TEMPCODE16_BASE
#else
    .long (pm_entrypoint - inpmode) + TEMPCODE16_BASE
#endif
pm_entrypoint:
    .long 0 // receives address of PE entry point
    nop
pm_entrypoint_end:

/* FNID_* functions */
CallbackTable:
    .word Int386
    .word Reboot
    .word Relocator16Boot
    .word PxeCallApi
    .word PnpBiosGetDeviceNodeCount
    .word PnpBiosGetDeviceNode
    .word PnpBiosGetDockStationInformation


    /* 16-bit stack pointer */
stack16:
    .word    STACK16ADDR


.align 4    /* force 4-byte alignment */
gdt:
    /* NULL Descriptor */
    .word HEX(0000)
    .word HEX(0000)
    .word HEX(0000)
    .word HEX(0000)

    /* 32-bit flat CS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9A00)
    .word HEX(00CF)

    /* 32-bit flat DS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9200)
    .word HEX(00CF)

    /* 16-bit flat CS (!) */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9B00)
    .word HEX(008F)

    /* 16-bit real mode DS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9200)
    .word HEX(0000)
gdt_end:

/* GDT table pointer */
gdtptr:
    .word HEX(27)       /* Limit */
    .long GDTDATA_BASE  /* Base Address */

/* Real-mode IDT pointer */
rmode_idtptr:
    .word HEX(3FF)      /* Limit */
    .long 0             /* Base Address */

#include "int386.inc"
#include "helpers_all.inc"
#if defined(SARCH_PC98)
#include "helpers_pc98.inc"
#else
#include "helpers.inc"
#endif
#include "pxe.inc"
#include "pnp.inc"

.org (FREELDR_PE_BASE - FREELDR_BASE - 1)
.byte 0
.endcode16

END
