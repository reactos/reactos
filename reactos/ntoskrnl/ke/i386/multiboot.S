#include <internal/ntoskrnl.h>
#include <internal/i386/segment.h>

#define KERNEL_BASE (0xc0000000)

#define MULTIBOOT_HEADER_MAGIC (0x1BADB002)

#define MULTIBOOT_HEADER_FLAGS (0x00010003)

#define V2P(x) (x - 0xc0000000 + 0x200000)
	
	
.globl _NtProcessStartup
.globl _start
.globl _init_stack
.globl _init_stack_top
.globl _trap_stack
.globl _trap_stack_top	
.globl _unmap_me
.globl _unmap_me2
.globl _unmap_me3
		
	/*
	 * This is called by the realmode loader, with protected mode
	 * enabled, paging disabled and the segment registers pointing
	 * a 4Gb, 32-bit segment starting at zero.
	 *
	 *    EBX = Points to a structure in lowmem with data from the
	 *    loader
	 */

_NtProcessStartup:
_start:
	jmp	_multiboot_entry

	/* Align 32 bits boundary */
	.align 4
	
	/* Multiboot header */
multiboot_header:
	/* magic */
	.long MULTIBOOT_HEADER_MAGIC
	/* flags */
	.long MULTIBOOT_HEADER_FLAGS
	/* checksum */
	.long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
	/* header_addr */
	.long (0x200000 + multiboot_header - KERNEL_BASE)
	/* load_addr */
	.long 0x200000
	/* load_end_addr */
	.long (__bss_start__ + 0x200000 - KERNEL_BASE)
	/* bss_end_addr */
	.long (__bss_end__ + 0x200000 - KERNEL_BASE)
	/* entry_addr */
	.long (0x200000 + _start - KERNEL_BASE)

_multiboot_entry:
	/*
	 * This	must be PIC because we haven't set up paging yet
	 */
	
	/*
	 * Gcc expects this at all times
	 */
	cld

	/*
	 * Zero the BSS
	 */
	movl	%eax, %edx
	movl	$0, %eax
	movl	$__bss_end__, %ecx
	subl	$__bss_start__, %ecx
	shr	$2, %ecx
	movl	$__bss_start__, %edi
	subl	$0xc0000000, %edi
	addl	$0x200000, %edi
	rep
	stosl

	/*
	 * Initialize the page directory
	 */
	movl	$V2P(startup_pagedirectory), %esi
	movl	$(V2P(lowmem_pagetable) + 0x7), 0x0(%esi)
	movl	$(V2P(kernel_pagetable) + 0x7), 0xC00(%esi)
	movl	$(V2P(lowmem_pagetable) + 0x7), 0xD00(%esi)
	movl	$(V2P(startup_pagedirectory) + 0x7), 0xF00(%esi) 

	/*
	 * Initialize the page table that maps low memory
	 */
	movl	$V2P(lowmem_pagetable), %esi
	movl	$0x7, %eax
	movl	$0, %edi
.l3:
	movl	%eax, (%esi, %edi)
	addl	$0x1000, %eax
	addl	$4, %edi
	cmpl	$4096, %edi
	jl	.l3

	/*
	 * Initialize the page table that maps kernel memory
	 */
	movl	$V2P(kernel_pagetable), %esi
	movl	$0x200007, %eax
	movl	$0, %edi
.l4:
	movl	%eax, (%esi, %edi)
	addl	$0x1000, %eax
	addl	$4, %edi
	cmpl	$2048, %edi
	jl	.l4
	
	/*
	 * Set up the PDBR
	 */	
	movl	$(V2P(startup_pagedirectory)), %eax
	movl	%eax, %cr3

	/*
	 * Enable paging
	 */
	movl	%cr0, %eax
	orl	$0x80000000, %eax
	movl	%eax, %cr0

	/*
	 * Do an absolute jump because we now want to execute at 0xc0000000
	 */	
	movl	$.l2, %eax
	jmp	*%eax
.l2:
	
	/*
	 * Load the GDTR and IDTR with new tables located above
	 * 0xc0000000
	 */
	lgdt	_KiGdtDescriptor
	lidt	_KiIdtDescriptor

	/*
	 * Reload the data segment registers 
	 */
	movl	$KERNEL_DS, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %fs
	movl	%eax, %gs
	movl	%eax, %ss

	/*
	 * Load the initial ring0 stack
	 */
	movl	$_init_stack_top, %esp

	/*
	 * Initialize EFLAGS
	 */
	pushl	$0
	popfl

	/*
	 * Call the main ring0 initialization
	 */
	pushl	%ebx
	pushl	%edx
	pushl	$0
	pushl	$KERNEL_CS
	pushl	$__main
	lret

	/*
	 * This needs to be page aligned so put it at the beginning of the bss
	 * segment
	 */
.bss
startup_pagedirectory:
	.fill 4096, 1, 0

lowmem_pagetable:
	.fill 4096, 1, 0

kernel_pagetable:
	.fill 4096, 1, 0	
	
_unmap_me:
	.fill 4096, 1, 0
	
_init_stack:
	.fill 3*4096, 1, 0
_init_stack_top: 

_unmap_me2:	
	.fill 4096, 1, 0
	
_trap_stack:	
	.fill 3*4096, 1, 0
_trap_stack_top:	
	
_unmap_me3:	
	.fill 4096, 1, 0			


