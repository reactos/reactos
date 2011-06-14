
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
    mov sp, word ptr ds:stack16

    /* Enable A20 address line */
    call EnableA20

    /* Get address of optional header */
    mov eax, dword ptr ds:[FREELDR_PE_BASE + IMAGE_DOS_HEADER_e_lfanew]
    add eax, FREELDR_PE_BASE + 4 + IMAGE_FILE_HEADER_SIZE

    /* Get address of entry point */
    mov eax, dword ptr ds:[eax + IMAGE_OPTIONAL_HEADER_AddressOfEntryPoint]
    add eax, FREELDR_PE_BASE

    /* Safe the entry point */
    mov dword ptr [BSS_EntryPoint], eax

    /* Patch the long jump instruction */
    mov word ptr [pm_offset], ax

/*
 * Switches the processor to protected mode
 * it destroys eax
 */
switch_to_prot:

    /* Load the GDT */
    lgdt    gdtptr

    /* Enable Protected Mode */
    mov eax, cr0
    or eax, CR0_PE_SET
    mov cr0, eax

    /* Clear prefetch queue & correct CS */
    .byte HEX(0ea) // jmp far PMODE_CS:entry_point
pm_offset:
    .word 0 // receives address of PE entry point
    .word PMODE_CS
    nop



    /* 16-bit stack pointer */
stack16:
    .word    STACK16ADDR


.align 4	/* force 4-byte alignment */
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
	.word HEX(27)		/* Limit */
	.long gdt			/* Base Address */

.org 1024

#include "helpers.inc"

.org (FREELDR_PE_BASE - FREELDR_BASE)
.endcode16

END
