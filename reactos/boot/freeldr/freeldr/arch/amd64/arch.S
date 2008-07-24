.intel_syntax noprefix
.text
.code16

//.org 0x8000

#define STACK16ADDR	0x7000	/* The 16-bit stack top will be at 0000:7000 */
#define STACK64ADDR	0x74000	/* The 64-bit stack top will be at 0x74000 */

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

	/* GO! */
	xor rcx, rcx
	call _BootMain

	/* Return into real mode */
	call x86_64_SwitchToReal
	.code16

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
 * We defines one 2MB page at the start of memory, so we can access the first 
 * 2MBs as if paging was disabled
 */

#define PML4_PAGENUM 60 // Put it high enough so it doesn't interfere with freeldr

#define PAGESIZE 4096
#define PDP_PAGENUM (PML4_PAGENUM + 1)
#define PD_PAGENUM (PDP_PAGENUM + 1)
#define PML4_ADDRESS (PML4_PAGENUM * PAGESIZE)
#define PDP_ADDRESS (PDP_PAGENUM * PAGESIZE)
#define PML4_SEG (PML4_ADDRESS / 16)

x86_16_BuildPageTables:
	pusha
	push es

	mov ax, PML4_SEG
	mov es, ax
	cld
	xor di, di

	/* One entry in the PML4 pointing to PDP */
	mov ax, ((PDP_PAGENUM << 12) & 0xffff) | 0x00f
	stosw
	mov ax, (PDP_PAGENUM >> 4)
	stosw
	xor ax,ax
	mov cx, 0x07fe
	rep stosw

	/* One entry in the PDP pointing to PD */
	mov ax, ((PD_PAGENUM << 12) & 0xffff) | 0x00f
	stosw
	mov ax, (PD_PAGENUM >> 4)
	stosw
	xor ax,ax
	mov cx, 0x07fe
	rep stosw

	/* One entry in the PD defining a 2MB page */
	mov ax, 0x018f
	stosw
	xor ax,ax
	mov cx,0x07ff
	rep stosw

	/* Return */
	pop es
	popa
	ret


#define LMODE_CS	0x08
#define LMODE_DS	0x10
#define RMODE_CS	0x18	/* RMode code selector, base 0 limit 64k */
#define RMODE_DS	0x20	/* RMode data selector, base 0 limit 64k */

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

	mov edx, PML4_ADDRESS	// Point cr3 at PML4
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

	/* Step 1 - deactivate long mode, by disabling paging */
	mov rax, cr0
	and rax, 0x000000007fffffff //~0x80000000
	mov cr0, rax

	/* Step 2 - disable long mode in EFER MSR */
	mov rcx, 0xC0000080		// Specify EFER MSR
	rdmsr					
	and eax, ~0x00000100	// Disable EFER.LME
	wrmsr

	.code32
	/* jmp to 16-bit segment to set the limit correctly */
	jmp RMODE_CS: offset SwitchToReal2

SwitchToReal2:
	.code16

	/* Step 3 - Disable Protected Mode */
	mov eax, cr0
	and eax, ~0x00000001
	mov cr0, eax

	/* Clear prefetch queue & correct CS */
	jmp 0:offset BeReal

BeReal:
	/* Restore segment registers */
	mov ax, 0
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	/* Rstore 16 bit stack */
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

gdt:
	.quad 0x0000000000000000
	.quad 0x0020980000000000
	.quad 0x0000900000000000
	.word	0xFFFF, 0x0000, 0x9E00, 0x0000 /* 16-bit real mode CS */
	.word	0xFFFF, 0x0000, 0x9200, 0x0000 /* 16-bit real mode DS */

/* GDT table pointer */
gdtptr:
	.word	0x27		/* Limit */
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

