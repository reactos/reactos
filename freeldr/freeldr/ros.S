/*
 *  FreeLoader
 *  Copyright (C) 1999, 2000  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

	.text
	.code16

#include "asmcode.h"

	/*
	 * Needed for enabling the a20 address line
	 */
empty_8042:
	.word	0x00eb,0x00eb            // jmp $+2, jmp $+2
	inb     $0x64,%al
    testb   $0x02,%al
    jnz     empty_8042
	ret

	/*
	 * Enable the A20 address line (to allow access to over 1mb)
	 */
	.code32
EXTERN(_enable_a20)
	call	switch_to_real
	.code16

	call	empty_8042
    movb	$0xD1,%al                // command write
    outb	%al,$0x64
	call	empty_8042
    mov		$0xDF,%al                // A20 on
    out		%al,$0x60
	call	empty_8042
	call	switch_to_prot
	.code32

	ret

	.code16

    /*
     * Reprogram the PIC because they overlap the Intel defined
     * exceptions 
	 */
reprogram_pic:
	movb	$0x11,%al                // initialization sequence
	outb    %al,$0x20                // send it to 8259A-1
	.word	0x00eb,0x00eb            // jmp $+2, jmp $+2
	outb    %al,$0xA0                // and to 8259A-2
	.word	0x00eb,0x00eb
	movb    $0x40,%al                // start of hardware int's (0x20)
	outb    %al,$0x21
	.word	0x00eb,0x00eb
	movb    $0x48,%al                // start of hardware int's 2 (0x28)
	outb    %al,$0xA1
	.word	0x00eb,0x00eb
	movb    $0x04,%al                // 8259-1 is master
	outb    %al,$0x21
	.word	0x00eb,0x00eb
	movb	$0x02,%al                // 8259-2 is slave
	outb	%al,$0xA1
	.word	0x00eb,0x00eb
	movb	$0x01,%al                // 8086 mode for both
	outb	%al,$0x21
	.word	0x00eb,0x00eb
	outb	%al,$0xA1
	.word	0x00eb,0x00eb
	movb	$0xFF,%al                // mask off all interrupts for now
	outb	%al,$0x21
	.word	0x00eb,0x00eb
	outb	%al,$0xA1
	ret

	/*
	 * In: EDI = address
	 * Out: FS = segment
	 *      DI = base
	 */
convert_to_seg:
	pushl	%eax

	movl	%edi,%eax
	//shrl	$16,%eax
	//shll	$12,%eax
	//movw	%ax,%fs

	shrl	$4,%eax
	movw	%ax,%fs
	andl	$0xf,%edi

	//andl	$0xFFFF,%edi

	popl	%eax
	ret

	/*
     * Here we assume the kernel is loaded at 1mb
     * This boots the kernel
	 */
	.code32
EXTERN(_boot_ros)
	call	switch_to_real
	.code16

    /* Save cursor position */
	movw    $3,%ax          //! Reset video mode
	int     $0x10


	movb    $10,%bl
	movb    $12,%ah
	int     $0x10

	movw    $0x1112,%ax      //! Use 8x8 font
	xorb	%bl,%bl
	int     $0x10
	movw    $0x1200,%ax      //! Use alternate print screen
	movb    $0x20,%bl
	int     $0x10
	movb    $1,%ah         //! Define cursor (scan lines 6 to 7)
	movw    $0x0607,%cx
	int     $0x10

	movb    $1,%ah
	movw    $0x600,%cx
	int     $0x10

	MOVb      $6,%AH        //SCROLL ACTIVE PAGE UP
	MOVb      $0x32,%AL      //CLEAR 25 LINES
	MOVw      $0,%CX       //UPPER LEFT OF SCROLL
	MOVw      $0x314F,%dx    //LOWER RIGHT OF SCROLL
	MOVb      $(1*0x10+1),%bh  //USE NORMAL ATTRIBUTE ON BLANKED LINE
	INT       $0x10         //VIDEO-IO


	movw     $0,%dx
	movb     $0,%dh

	movb     $2,%ah
	movb     $0,%bh
	int     $0x10

	movw     $0,%dx
	movb     $0,%dh

	movb     $2,%ah
	movb     $0,%bh
	int      $0x10

	cli

	// The page tables are setup elsewhere
	/*
	// Map in the lowmem page table (and reuse it for the identity map)
	movl	_kernel_page_directory_base,%edi
	call    convert_to_seg

	movl	_lowmem_page_table_base,%eax
	addl	$0x07,%eax
	movl	%eax,%fs:(%edi)
	movl	%eax,%fs:0xd00(%edi)//(0xd0000000/(1024*1024))(%edi)
	
	// Map the page tables from the page directory
	movl	_kernel_page_directory_base,%eax
	addl	$0x07,%eax
	movl	%eax,%fs:0xf00(%edi)//(0xf0000000/(1024*1024))(%edi)

	// Map in the kernel page table
	movl	_system_page_table_base,%eax
	addl	$0x07,%eax
	movl	%eax,%fs:0xc00(%edi)//(0xc0000000/(1024*1024))(%edi)

	// Setup the lowmem page table
	movl	_lowmem_page_table_base,%edi
	call	convert_to_seg

	movl	$0,%ebx
l9:
	movl	%ebx,%eax
	shll	$12,%eax        // ebx = ebx * 4096
	addl	$07,%eax        // user, rw, present
	movl	%eax,%fs:(%edi,%ebx,4)
	incl	%ebx
	cmpl	$1024,%ebx
	jl		l9

	// Setup the system page table
	movl	_system_page_table_base,%edi
	call	convert_to_seg

	movl	$07,%eax
l8:
	movl	%eax,%edx
	addl	_start_kernel,%edx
	movl	%edx,%fs:(%edi)
	addl	$4,%edi
	addl	$0x1000,%eax
	cmpl	$0x100007,%eax
	jl      l8
	*/

	/*
	 * Load the page directory into cr3
	 */
	movl	_kernel_page_directory_base,%eax
	movl	%eax,%cr3

	/*
	 * Setup various variables
	 */
	movw	%ds,%bx
	movzwl	%bx,%eax
	shll	$4,%eax
	addl	%eax,kernel_gdtbase

	//call	enable_a20 // enabled elsewhere
	call	reprogram_pic

	/*
	 * Load stack
	 */
	movw	%ds,%bx
	movzwl	%bx,%eax
	shll	$4,%eax
	addl	$real_stack_end,%eax
	movl	%eax,real_stack_base
	movl	real_stack_base,%esp
	movl	_boot_param_struct_base,%edx

	/*
	 * load gdt
	 */
	lgdt	kernel_gdtptr

	/*       
	 * Enter pmode and clear prefetch queue
	 */
	movl	%cr0,%eax
	orl		$0x80010001,%eax
	/*orl		$0x80000001,%eax*/
	movl	%eax,%cr0
	jmp		next
next:
	/*
	 * NOTE: This must be position independant (no references to
	 * non absolute variables)
	 */

	/*
	 * Initalize segment registers
	 */
	movw	$KERNEL_DS,%ax
	movw	%ax,%ds
	movw	%ax,%ss
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs

	/*
	 * Initalize eflags
	 */
	pushl	$0
	popfl	

	/*
	 * Jump to start of 32 bit code at 0xc0000000 + 0x1000
	 */
	pushl	%edx
	pushl	$0
	ljmpl	$KERNEL_CS,$(KERNEL_BASE+0x1000)


	.p2align	2	/* force 4-byte alignment */
kernel_gdt:
	.word 0               // Zero descriptor
	.word 0
	.word 0
	.word 0
                        
	//.word 0x0000          // User code descriptor
	//.word 0x0000          // base: 0h limit: 3gb
	//.word 0xfa00
	//.word 0x00cc
                       
	//.word 0x0000          // User data descriptor
	//.word 0x0000          // base: 0h limit: 3gb
	//.word 0xf200
	//.word 0x00cc
                    
	//.word 0x0000         
	//.word 0x0000         
	//.word 0x0000
	//.word 0x0000

	.word 0xffff          // Kernel code descriptor 
	.word 0x0000          // 
	.word 0x9a00          // base 0h limit 4gb
	.word 0x00cf
                       
	.word 0xffff          // Kernel data descriptor
	.word 0x0000          // 
	.word 0x9200          // base 0h limit 4gb
	.word 0x00cf

    /* TSS space */    
	//.rept NR_TASKS
	//.word 0
	//.word 0
	//.word 0
	//.word 0
	//.endr

kernel_gdtptr:
	.word	(((6+NR_TASKS)*8)-1)	/* Limit */
kernel_gdtbase:
	.long	kernel_gdt				/* Base Address */

EXTERN(_boot_param_struct_base)
	.long	0
EXTERN(_start_mem)
	.long	0
EXTERN(_kernel_page_directory_base)
	.long	0
EXTERN(_system_page_table_base)
	.long	0
EXTERN(_lowmem_page_table_base)
	.long	0
EXTERN(_start_kernel)
	.long	0
EXTERN(_load_base)
	.long	0

/* boot_param structure */
EXTERN(_boot_parameters)
	.long	0		// Magic
	.long	0		// Cursor X
	.long	0		// Cursor Y
	.long	0		// nr_files
	.long	0		// start_mem
	.long	0		// end_mem

	.rept	64
	.long	0		// List of module lengths (terminated by a 0)
	.endr

	.rept	256
	.byte	0		// Kernel parameter string
	.endr

/* Our initial stack */
real_stack:
		.rept 1024
		.byte 0
		.endr
real_stack_end:

real_stack_base:
		.long 0
