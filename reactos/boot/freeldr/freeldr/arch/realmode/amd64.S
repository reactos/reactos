
#include <asm.inc>
#include <arch/pc/x86common.h>

#define IMAGE_DOS_HEADER_e_lfanew 36
#define IMAGE_FILE_HEADER_SIZE 20
#define IMAGE_OPTIONAL_HEADER_AddressOfEntryPoint 16


.code16

/* fat helper code */
#include "fathelp.inc"

.org 512
RealEntryPoint:

	cli

	/* Setup real mode segment registers */
	xor ax, ax
	mov	ds, ax
	mov es, ax
	mov fs, ax
	mov	gs, ax
	mov ss, ax

	/* checkPoint Charlie - where it all began... */
	mov si, offset CheckPoint0
	call writestr

	/* Setup a real mode stack */
	mov	sp, stack16

	/* Zero BootDrive and BootPartition */
	xor eax, eax
	mov BootDrive, eax
	mov BootPartition, eax

	/* Store the boot drive */
	mov BootDrive, dl

	/* Store the boot partition */
	mov BootPartition, dh

	/* Load the GDT */
	lgdt gdtptr
	/* Load the IDT */
//	lidt idtptr

	call x86_16_EnableA20

	/* checkPoint Charlie - where it all began... */
	mov si, offset CheckPoint1
	call writestr

	call x86_16_BuildPageTables

	/* checkPoint Charlie - where it all began... */
	mov si, offset CheckPoint2
	call writestr

	/* Check if CPU supports CPUID */
	pushfd
	pop eax
	mov ebx, eax
	xor eax, HEX(00200000)
	push eax
	popfd
	pushfd
	pop eax
	cmp eax,ebx
	jz no_cpuid_support_detected

	/* CPUID support detected - getting the PAE/PGE */

	mov eax,1 // Fn0000_0001 - PAE in EDX[6]
	cpuid
	xor eax,eax
	and edx, HEX(00a0)
	test edx,edx // are PAE and PGE bits set?
	jz no_x64_support_detected

	/* PAE and PGE are here */

	xor edx, edx
	mov eax, HEX(80000001)
	cpuid
	and edx, HEX(20000000)
	test edx,edx
	jz no_x64_support_detected

	/* X64 Processor */

	/* checkPoint Charlie - where it all began... */
	mov si, offset CheckPoint3
	call writestr

    /* Get address of optional header */
    mov eax, dword ptr ds:[FREELDR_PE_BASE + IMAGE_DOS_HEADER_e_lfanew]
    add eax, FREELDR_PE_BASE + 4 + IMAGE_FILE_HEADER_SIZE

    /* Get address of entry point */
    mov eax, dword ptr ds:[eax + IMAGE_OPTIONAL_HEADER_AddressOfEntryPoint]

    /* Store the address in the callback return variable */
    mov dword ptr ds:[CallbackReturnAddress], eax

switch64:
    mov
	jmp x86_16_ReturnToLong


no_x64_support_detected:
	mov  si, offset NotAnX64Processor	// Loading message
	call writestr
	jmp fail

no_cpuid_support_detected:
	mov  si, offset NoCPUIDSupport	// Loading message
   	call writestr

fail:
	jmp fail
	nop
	nop

/*
 * We define 512 2MB pages at the start of memory, so we can access the first
 * 1 GB as if paging was disabled
 */
x86_16_BuildPageTables:
	pusha
	push es

	/* Get segment of pml4 */
	mov eax, offset pml4_startup
	shr eax, 4
	mov es, ax
	cld
	xor di, di

	/* One entry in the PML4 pointing to PDP */
	mov eax, offset pdp_startup
	or eax, HEX(00f)
	stosd
	/* clear rest */
	xor eax, eax
	mov cx, HEX(03ff)
	rep stosd

	/* One entry in the PDP pointing to PD */
	mov eax, offset pd_startup
	or eax, HEX(00f)
	stosd
	/* clear rest */
	xor eax, eax
	mov ecx, HEX(03ff)
	rep stosd

	/* 512 entries in the PD defining a 2MB page each */
	mov ecx, 512
	mov eax, HEX(008f)

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





x86_16_ReturnToLong:

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

	mov edx, offset pml4_startup // Point cr3 at PML4
	mov cr3, edx

	mov ecx, HEX(0C0000080)		// Specify EFER MSR

	rdmsr					// Enable long mode
	or eax, HEX(00000100)
	wrmsr

	mov ebx, cr0			// Activate long mode
	or ebx, HEX(080000001)		// by enabling paging and protection simultaneously
	mov cr0, ebx			// skipping protected mode entirely

	//jmp LMODE_CS:offset LongCat	//Load CS with 64 bit segment and flush the instruction cache
    // Do a long jmp to the CallbackReturn address


#include "helpers.inc"

.endcode16

END
