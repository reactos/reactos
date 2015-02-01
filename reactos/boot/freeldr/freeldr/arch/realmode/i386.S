
#include <asm.inc>
#include "../../include/arch/pc/x86common.h"

#define IMAGE_DOS_HEADER_e_lfanew 60
#define IMAGE_FILE_HEADER_SIZE 20
#define IMAGE_OPTIONAL_HEADER_AddressOfEntryPoint 16

.code16

/* fat helper code */
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
    mov sp, word ptr ds:[stack16]

    /* Enable A20 address line */
    call EnableA20

    /* Safe real mode entry point in shared memory */
    mov dword ptr ds:[BSS_RealModeEntry], offset switch_to_real16

    /* Address the image with es segment */
    mov ax, FREELDR_PE_BASE / 16
    mov es, ax

    /* Get address of optional header */
    mov eax, dword ptr es:[IMAGE_DOS_HEADER_e_lfanew]
    add eax, 4 + IMAGE_FILE_HEADER_SIZE

    /* Get address of entry point */
    mov eax, dword ptr es:[eax + IMAGE_OPTIONAL_HEADER_AddressOfEntryPoint]
    add eax, FREELDR_PE_BASE

    /* Save entry point */
    mov dword ptr ds:[pm_entrypoint], eax

    /* Restore es */
    xor ax, ax
    mov es, ax

    jmp exit_to_protected


/* This is the entry point from protected mode */
switch_to_real16:

    /* Restore segment registers to correct limit */
    mov ax, RMODE_DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    /* Disable Protected Mode */
    mov eax, cr0
    and eax, CR0_PE_CLR
    mov cr0, eax

    /* Clear prefetch queue & correct CS */
    ljmp16 0, inrmode

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
    mov sp, word ptr ds:[stack16]

    /* Load IDTR with real mode value */
#ifdef _USE_ML
    lidt fword ptr rmode_idtptr
#else
    lidt rmode_idtptr
#endif

    sti /* These are ok now */

    /* Do the callback, specified by bx */
    shl bx, 1
    call word ptr ds:callback_table[bx]


/*
 * Switches the processor to protected mode
 * it destroys eax
 */
exit_to_protected:

    cli

    /* Safe current stack pointer */
    mov word ptr ds:[stack16], sp

    /* Load the GDT */
#ifdef _USE_ML
    lgdt fword ptr gdtptr
#else
    lgdt gdtptr
#endif

    /* Enable Protected Mode */
    mov eax, cr0
    or eax, CR0_PE_SET
    mov cr0, eax

    /* Clear prefetch queue & correct CS */
    ljmp16 PMODE_CS, inpmode
inpmode:
    .byte HEX(0ff), HEX(25) // opcode of indirect jump
    .word pm_entrypoint, 0
pm_entrypoint:
    .long 0 // receives address of PE entry point
    nop

callback_table:
    .word Int386
    .word Reboot
    .word ChainLoadBiosBootSectorCode
    .word PxeCallApi
    .word PnpBiosGetDeviceNodeCount
    .word PnpBiosGetDeviceNode
    .word BootLinuxKernel


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

    /* 16-bit real mode CS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9E00)
    .word HEX(0000)

    /* 16-bit real mode DS */
    .word HEX(FFFF)
    .word HEX(0000)
    .word HEX(9200)
    .word HEX(0000)

/* GDT table pointer */
gdtptr:
    .word HEX(27)       /* Limit */
    .word gdt, 0        /* Base Address */

/* Real-mode IDT pointer */
rmode_idtptr:
    .word HEX(3ff)      /* Limit */
    .long 0             /* Base Address */

#include "int386.inc"
#include "pxe.inc"
#include "pnp.inc"
#include "linux.inc"
#include "helpers.inc"

.org (FREELDR_PE_BASE - FREELDR_BASE - 1)
.byte 0
.endcode16
END
