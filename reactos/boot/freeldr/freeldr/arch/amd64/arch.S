.intel_syntax noprefix
.text
.code16

#define ASM
#include <arch.h>

//.org 0x8000

.global RealEntryPoint
RealEntryPoint:

	cli

	/* Setup real mode segment registers */
	xor ax, ax
	mov	ds, ax
	mov es, ax
	mov fs, ax
	mov	gs, ax
	mov ss, ax

	/* Setup a real mode stack */
	mov	sp, stack16

	/* Zero BootDrive and BootPartition */
	xor eax, eax
	mov _BootDrive, eax
	mov _BootPartition, eax

	/* Store the boot drive */
	mov _BootDrive, dl

	/* Store the boot partition */
	mov _BootPartition, dh

	/* Load the GDT */
	lgdt gdtptr
	/* Load the IDT */
//	lidt idtptr

	call x86_16_EnableA20

	call x86_16_BuildPageTables

	/* Switch to long mode */
	call x86_16_SwitchToLong

	.code64

	/* Checkpoint */
//	mov ax, LMODE_DS
//	mov ds, ax
//	mov word ptr ds:[0xb8000], 0x0e00 + '1'

	/* GO! */
	xor rcx, rcx
	call _BootMain

	/* Checkpoint */
//	mov ax, LMODE_DS
//	mov ds, ax
//	mov word ptr ds:[0xb8002], 0x0e00 + '2'

	/* Return into real mode */
	call x86_64_SwitchToReal
	.code16

	/* Checkpoint */
//	mov ax, 0xb800
//	mov fs, ax
//	mov word ptr fs:[0xA0], 0x0e00 + '0'

//	int	0x19

	/* We should never get here */
stop:
	jmp	stop
	nop
	nop


/** 16 Bit helper functions ***************************************************/
.code16

x86_16_Empty8042:
	.word	0x00eb,0x00eb		// jmp $+2, jmp $+2
	in	al, 0x64
	cmp	al, 0xff				// legacy-free machine without keyboard
	jz	empty_8042_ret			// controllers on Intel Macs read back 0xFF
	test al, 0x02
	jnz	x86_16_Empty8042
empty_8042_ret:
	ret

x86_16_EnableA20:
	pusha
	call x86_16_Empty8042
	mov al, 0xD1				// command write
	out 0x64, al
	call x86_16_Empty8042
	mov al, 0xDF				// A20 on
	out 0x60, al
	call x86_16_Empty8042
	popa
	ret


/*
 * We define 512 2MB pages at the start of memory, so we can access the first 
 * 1 GB as if paging was disabled
 */
x86_16_BuildPageTables:
	pusha
	push es

	/* Get segment of pml4 */
	mov eax, offset _pml4_startup
	shr eax, 4
	mov es, ax
	cld
	xor di, di

	/* One entry in the PML4 pointing to PDP */
	mov eax, offset _pdp_startup
	or eax, 0x00f
	stosd
	/* clear rest */
	xor eax, eax
	mov cx, 0x03ff
	rep stosd

	/* One entry in the PDP pointing to PD */
	mov eax, offset _pd_startup
	or eax, 0x00f
	stosd
	/* clear rest */
	xor eax, eax
	mov ecx, 0x03ff
	rep stosd

	/* 512 entries in the PD defining a 2MB page each */
	mov ecx, 512
	mov eax, 0x008f

Bpt2:
	mov es: [di], eax
	mov dword ptr es: [di + 4], 0
	add eax, 512 << 12 // add 512 4k pages
	add di, 8

	/* Loop it */
	dec cx
	jnz Bpt2

	/* Return */
	pop es
	popa
	ret


//.global x86_16_SwitchToLong
x86_16_SwitchToLong:
	cli

	xor	ax,ax
	mov	ds,ax
	mov	es,ax
	mov	fs,ax
	mov	gs,ax
	mov	ss,ax

	/* Get the return address off the stack */
	pop	word ptr code64ret

	/* Save 16-bit stack pointer */
	mov	stack16, sp

	mov eax, 0x00a0			// Set PAE and PGE: 10100000b
	mov cr4, eax

	mov edx, offset _pml4_startup // Point cr3 at PML4
	mov cr3, edx

	mov ecx, 0xC0000080		// Specify EFER MSR

	rdmsr					// Enable long mode
	or eax, 0x00000100
	wrmsr

	mov ebx, cr0			// Activate long mode
	or ebx, 0x80000001		// by enabling paging and protection simultaneously
	mov cr0, ebx			// skipping protected mode entirely

	jmp LMODE_CS:offset LongCat	//Load CS with 64 bit segment and flush the instruction cache

.code64
LongCat:
	/* Set up 64 bit stack */
	mov rsp, stack64

	/* Put the return address back onto the stack */
	push qword ptr code64ret

	/* Now return in long mode! */
	ret


/** 64 But functions **********************************************************/
.code64

.global x86_64_SwitchToReal
x86_64_SwitchToReal:

	/* Get the return address off the stack */
	pop	qword ptr code64ret

	/* Save 64-bit stack pointer */
	mov	stack64, rsp

//	mov ax, LMODE_DS
//	mov ds, ax
//	mov word ptr ds:[0xb8004], 0x0e00 + '3'

	/* Step 1 - jump to compatibility segment */
    ljmp jumpvector

jumpvector:
	.long	SwitchToReal1
	.word	CMODE_CS

SwitchToReal1:
.code32

//	mov word ptr ds:[0xb8006], 0x0e00 + '4'

	/* Step 2 - deactivate long mode, by disabling paging */
	mov eax, cr0
	and eax, 0x000000007fffffff //~0x80000000
	mov cr0, eax

	/* Step 2 - disable long mode in EFER MSR */
//	mov ecx, 0xC0000080		// Specify EFER MSR
//	rdmsr					
//	and eax, ~0x00000100	// Disable EFER.LME
//	wrmsr

	/* Step 3 - jump to 16-bit segment to set the limit correctly */
	jmp RMODE_CS: offset SwitchToReal2

SwitchToReal2:
	.code16

	/* Step 4 - Disable Protected Mode */
	mov eax, cr0
	and eax, ~0x00000001
	mov cr0, eax

	/* Clear prefetch queue & correct CS */
	jmp 0:offset BeReal

BeReal:
	/* Checkpoint */
//	mov ax, 0xb800
//	mov fs, ax
//	mov word ptr fs:[0x0C], 0x0e00 + '7'

	/* Restore segment registers */
	mov ax, 0
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	/* Restore 16 bit stack */
	mov sp, stack16

//	lidt	rmode_idtptr	/* Load IDTR with real mode value */

//	sti						/* These are ok now */

	/* Put the return address back onto the stack */
	push word ptr code64ret

	/* Now return in real mode! */
	ret



/** Some data *****************************************************************/

.code64

stack16:
	.quad STACK16ADDR

stack64:
	.quad STACK64ADDR

code64ret:
	.quad 0

.p2align	2
gdt:
	.quad 0x0000000000000000 /* 00: NULL descriptor */
	.quad 0x0000000000000000 /* 08:  */
	.quad 0x0020980000000000 /* 10: long mode cs */
	.quad 0x00cff3000000ffff /* 18: long mode ds */
	.word	0xFFFF, 0x0000, 0x9E00, 0x0000 /* 16-bit real mode CS */
	.word	0xFFFF, 0x0000, 0x9200, 0x0000 /* 16-bit real mode DS */
	.quad 0x00CF9B000000FFFF /* 30: compat mode cs */

/* GDT table pointer */
gdtptr:
	.word	0x37		/* Limit */
	.long	gdt			/* Base Address */


.global _BootDrive
_BootDrive:
    .long 0
    
.global _BootPartition
_BootPartition:
    .long 0


///////////////////////////////////////////////////////////////////////////////

/* Need to include them here, because of linking issues between 64 / 16 bit */
//#include "debug16.S"
#include "int386.S"
#include "boot.S"
#include "i386pnp.S"

