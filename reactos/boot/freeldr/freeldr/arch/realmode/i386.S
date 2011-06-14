
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

    /* Safe real mode entry point in shared memory */
    mov dword ptr [BSS_RealModeEntry], offset switch_to_real16

    /* Get address of optional header */
    mov eax, dword ptr ds:[FREELDR_PE_BASE + IMAGE_DOS_HEADER_e_lfanew]
    add eax, FREELDR_PE_BASE + 4 + IMAGE_FILE_HEADER_SIZE

    /* Get address of entry point */
    mov eax, dword ptr ds:[eax + IMAGE_OPTIONAL_HEADER_AddressOfEntryPoint]
    add eax, FREELDR_PE_BASE

    /* Patch the long jump instruction */
    mov word ptr [pm_offset], ax

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
	mov	eax, cr0
	and eax, CR0_PE_CLR
	mov	cr0, eax

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
	lidt rmode_idtptr

	sti		/* These are ok now */

    /* Do the callback, specified by cx */
    // call word ptr CallbackTable[cx * 4]
    mov ax, cx
    call writehex4

/*
 * Switches the processor to protected mode
 * it destroys eax
 */
exit_to_protected:

    cli

    /* Safe current stack pointer */
    mov word ptr ds:[stack16], sp

    /* Load the GDT */
    lgdt gdtptr

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

/* Real-mode IDT pointer */
rmode_idtptr:
	.word HEX(3ff)		/* Limit */
	.long 0			/* Base Address */

//.org 1024

#include "int386.inc"
#include "helpers.inc"

.org (FREELDR_PE_BASE - FREELDR_BASE)
.endcode16

END
