/*
 *
 *  BIOS ROM Startup Assembler
 */

 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "consts.h"
#include "memory_layout.h"


	.code32

.section .text, "ax"
         .org 0x00
         jmp	start_linux
         
.global	Cromwellconfig
Cromwellconfig:      	
	.org 0x0c
		// Space for the SHA1 checksum
        .org 0x20   

        // The Value positions are fixed, do never change them, used everywhere
        .long 0x0	// 0x20 if XBE, then this bit is 0, if Cromwell mode, the bit is set to 1 by the Startuploader
        .long 0x0	// 0x24 ImageRetryLoads
        .long 0x0	// 0x28 Bank, from where Loaded
        .long 0x0	// 0x2C 0 .. Bios = 256 k, 1 .. Bios = 1MB 
        .long 0x0	// 0x30 free
        .long _end_compleate_rom       // 0x34 free
        .long 0x0       // 0x38 free
        .long 0x0	// free
	
.align 16
tableGdt:
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // 0x00 dummy
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00 // 0x08 code32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00 // 0x10 code32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xcf, 0x00 // 0x18 data32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0x8f, 0x00 // 0x20 code16 (8f indicates 4K granularity, ie, huge limit)
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0x8f, 0x00 // 0x28 data16

tableGdtDescriptor:
	// This is the GDT header having 8 bytes
	.word tableGdtDescriptor-tableGdt  // 30 byte GDT
	.long GDT_LOC                      // GDT located at 0xA0000
	.word 0                            // Padding
tableGdtEnd:

.align 16
tableIdtDescriptor:

	.word 2048
	.long IDT_LOC                      // IDT located at 0xB0000
	.word 0     // fill Word, so we get alliged again
        
        // We are again Dword Alligated now

.align 16        
	.globl start_linux
start_linux:
       	
       	// i am sure, the irq's are turned off, but who knows this for sure
       	// never trust anybody
       	cli
  	
	// kill the cache  = Disable bit 30 + 29 = CD + NW 
	// CD = Cache Disable (disable = 1)
	// NW Not write through (disable = 1)
       //	mov 	%cr0, %eax
	//orl	$0x60000000, %eax
	mov 	$0x60010033, %eax
	mov	%eax, %cr0
	wbinvd

	// Flush the TLB
	xor %eax, %eax
	mov %eax, %cr3
	
	// We kill the Local Descriptor Table        
        xor	%eax, %eax
	lldt	%ax	
	
	// DR6/DR7: Clear the debug registers
	xor %eax, %eax
	mov %eax, %dr6
	mov %eax, %dr7
	mov %eax, %dr0
	mov %eax, %dr1
	mov %eax, %dr2
	mov %eax, %dr3

	
	// IMPORTANT!  Linux expects the GDT located at a specific position,
	// 0xA0000, so we have to move it there.

	// Copy the GDT to its final location
	movl $GDT_LOC, %edi
	movl $tableGdt, %esi
	movl $(tableGdtEnd-tableGdt)/4, %ecx
	rep movsl

	// Load the new GDT
	lgdt GDT_LOC+(tableGdtDescriptor-tableGdt)
	
	// Kill the LDT, if any
	xor	%eax, %eax
	lldt %ax

	// Reload CS as 0010 from the new GDT using a far jump
	.byte 0xEA          // jmp far 0010:reload_cs
	.long reload_cs
	.word 0x0010
.align 16
reload_cs:

	// CS is now a valid entry in the GDT.  Set SS, DS, and ES to valid
	// descriptors, but clear FS and GS as they are not necessary.

	// Set SS, DS, and ES to a data32 segment with maximum limit.
	movw $0x0018, %ax
	mov %eax, %ss
	mov %eax, %ds
	mov %eax, %es

	// Clear FS and GS
	xor %eax, %eax
	mov %eax, %fs
	mov %eax, %gs

	// Set the stack pointer to give us a valid stack
	movl $STACK_TOP, %esp
        
    // We clear the IDT in RAM   (IDT located @  0xb0000 )
	xor %eax, %eax
	movl $0x5000/4, %ecx
	movl $IDT_LOC, %edi
	rep stosl
	wbinvd
	
	// We load the Interrupt Descriptor Table
	lidt tableIdtDescriptor

       // FPU SETUP
	xor	%eax, %eax
	xor	%edx, %edx
	xor	%ecx, %ecx
	
	clts
	fninit
	
	xor	%eax, %eax
	xor	%edx, %edx
	xor	%ecx, %ecx
	
  	// We flush the TLB
	mov	%cr3, %eax
	mov	%eax, %cr3

	// Clear Mmeory Type register
	movl	$0x2ff, %ecx
	xor	%eax, %eax
	xor	%edx, %edx
	wrmsr


	// MTRR for RAM
	// from address 0, Writeback Caching, 128MB range
        movl	$0x200, %ecx
	movl	$0x00000000, %edx
	movl	$0x00000006, %eax 	// == WB_CACHE == 6
	//movl	$0x00000004, %eax 	// Temporary, as USB development
	wrmsr

	// MASK0 set to 0xf8000[000] == 128M
        movl	$0x201, %ecx
	movl	$0x0000000f, %edx
	movl	$0xf8000800, %eax
	wrmsr

	// MTRR for shadow RAM
	// from address 0xf0000000, Write-combining Caching, 128MB range
        movl	$0x202, %ecx
	movl	$0x00000000, %edx
	movl	$0xf0000001, %eax 	// Write-Combining == 1
	wrmsr

	// MASK0 set to 0xf8000[000] == 128M
        movl	$0x203, %ecx
	movl	$0x0000000f, %edx
	movl	$0xf8000800, %eax
	wrmsr

     // MTRR for FLASH
	movl	$0x204, %ecx
	movl	$0x00000000, %edx
	movl	$0xFff00000, %eax 	    // We set to Uncacheable
	wrmsr

        movl	$0x205, %ecx
	movl	$0x0000000f, %edx
	movl	$0xfff00800, %eax
	wrmsr

        xor	%eax, %eax
	xor	%edx, %edx

	movl	$0x206, %ecx		// IA32_MTRR_PHYS Base 3
        wrmsr
	movl	$0x207, %ecx		// IA32_MTRR_PHYS_MASK 3
        wrmsr

	movl	$0x208, %ecx		// IA32_MTRR_PHYS Base 4
        wrmsr                                                   
	movl	$0x209, %ecx		// IA32_MTRR_PHYS_MASK 4
        wrmsr                                                   

	movl	$0x20a, %ecx		// IA32_MTRR_PHYS Base 5
        wrmsr                                                   
	movl	$0x20b, %ecx		// IA32_MTRR_PHYS_MASK 5
        wrmsr                                                   

	movl	$0x20c, %ecx		// IA32_MTRR_PHYS Base 6
        wrmsr                                                   
	movl	$0x20d, %ecx		// IA32_MTRR_PHYS_MASK 6
        wrmsr

	movl	$0x20e, %ecx		// IA32_MTRR_PHYS Base 7
        wrmsr
	movl	$0x20f, %ecx		// IA32_MTRR_PHYS_MASK 7
        wrmsr                                                   


	// Ok, we tell now the processor, we finished Memory Type definitions
	// We set the Default Memory Type in the Box now to Type == 0 .. Means UC .. Uncacheable
	
	movl	$0x2ff, %ecx
	xor	%edx, %edx
	movl	$0x800, %eax 		//Enable MTRRs
	wrmsr
      
        
   
//	movl	$0x600, %eax
//	mov	%eax, %cr4

	mov	%eax, %eax       // Flush the TLB    and resets it
	mov	%eax, %cr3
 	wbinvd 
 
   	/* turn on normal cache */
	//  bit 30 + 29 = CD + NW 
	// CD = Cache Disable (disable = 1)
	// NW Not write through (disable = 1)
	
//	movl	%cr0, %eax
//	mov 	%eax, %ebx
//	andl	$0x9fFFffFF,%eax
//	movl	%eax, %cr0
//	wbinvd

	jmp BootResetAction

#if 0	
        // something for Video ?
	mov 	$0x80000854, %eax
	movw 	$0xcf8, %dx
	outl 	%eax, %dx
	movw 	$0xcfc, %dx
	inl 	%dx, %eax 
	orl 	$0x88000000, %eax
	outl 	%eax, %dx 
        wbinvd
	
	// Important
	mov 	$0x80000064, %eax 
	movw 	$0xcf8, %dx 
	outl	%eax, %dx 
	movw 	$0xcfc, %dx
	inl 	%dx, %eax 
	orl 	$0x88000000, %eax 
	outl	%eax, %dx 
	wbinvd
	
	// Important
	mov 	$0x8000006c, %eax 
	movw 	$0xcf8, %dx
	outl	%eax, %dx 
	movw 	$0xcfc, %dx 
	inl 	%dx, %eax
	push 	%eax 
	andl 	$0xfffffffe, %eax 
	outl	%eax, %dx 
	pop 	%eax
	outl	%eax, %dx 
      	wbinvd
      	
/*      	
	// CPU Whoami   ? sesless ?
	mov 	$0x80000080, %eax 
	movw 	$0xcf8, %dx 
	outl	%eax, %dx 
	movw 	$0xcfc, %dx 
	movl 	$0x100, %eax 
	outl	%eax, %dx 
        wbinvd
*/	
	mov 	$0x8000088C, %eax 
	movw 	$0xcf8, %dx 
	outl	%eax, %dx 
	movw 	$0xcfc, %dx 
	movl 	$0x40000000, %eax 
	outl	%eax, %dx
	wbinvd
	
	jmp BootResetAction
#endif
       


///////////////////////////////////////////
//
//  Interrupt Service Routines
//


		.global	IntHandlerTimer0
IntHandlerTimer0:
	cli
	pusha
	pushf
	call	IntHandlerCTimer0
	mov	$0x20, %al
	outb	%al, $0x20
	popf
//	sti
	popa
	iret


		.global	IntHandler1
IntHandler1:
	pusha
	pushf
	cli
	call	IntHandler1C
	mov	$0x21, %al
	outb	%al, $0x20
	popf
//	sti
	popa
	iret


		.global	IntHandler2
IntHandler2:
	cli
	pusha
	pushf
	call	IntHandler2C
	mov	$0x22, %al
	outb	%al, $0x20
	popf
	popa
//	sti
	iret



		.global	IntHandler3
IntHandler3:
	cli
	pusha
	pushf
	call	IntHandler3VsyncC
	mov	$0x23, %al
	outb	%al, $0x20
	popf
	popa
//	sti
	iret


		.global	IntHandler4
IntHandler4:
	cli
	pusha
	pushf
	call	IntHandler4C
	mov	$0x24, %al
	outb	%al, $0x20
	popf
	popa
//	sti
	iret


		.global	IntHandler5
IntHandler5:
	cli
	pusha
	pushf
	call	IntHandler5C
	mov	$0x25, %al
	outb	%al, $0x20
	popf
	popa
//	sti
	iret


		.global	IntHandler6
IntHandler6:
	cli
	pusha
	pushf
	call	IntHandler6C
	mov	$0x26, %al
	outb	%al, $0x20
	popf
	popa
	sti
	iret



		.global	IntHandler7
IntHandler7:
	cli
	pusha
	pushf
	call	IntHandler7C
	mov	$0x27, %al
	outb	%al, $0x20
	popf
	popa
	sti	
	iret



	.global IntHandler8
IntHandler8:
	cli
	pusha
	pushf
	call	IntHandler8C
		// EOI on master and slave needed
	mov	$0x60, %al
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret




	.global IntHandler9
IntHandler9:
	pusha
	pushf
	cli
	call	IntHandler9C
		// EOI on master and slave needed
	mov	$0x61, %al
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret

		// Int 10 interrupts

	.global IntHandler10
IntHandler10:
	pusha
	pushf
	cli
	call	IntHandler10C
		// EOI on master and slave needed
	mov	$0x62, %al
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret

		// Int 11 interrupts

	.global IntHandlerI2C
IntHandlerI2C:
	pusha
	pushf
	cli
	call	IntHandlerCI2C

		// EOI on master and slave needed
	mov	$0x63, %al
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret

		// Int 12 interrupts

	.global IntHandlerSmc
IntHandlerSmc:
	cli
	pusha
	pushf

	call	IntHandlerCSmc

		// acknowledge EXTSMI# action (from PIC p6)

	mov	$0x8020, %dx
	inw %dx, %ax
	or $0x0200, %ax
	outw	%ax, %dx

		// EOI on master and slave needed
	mov	$0x64, %al // is int12
	out	%al, $0xa0
	mov	$0x62, %al // do cascaded master
	out	%al, $0x20
	popf
	popa
	iret

		// Int 13 interrupts

	.global IntHandler13
IntHandler13:
	cli
	pusha
	pushf

	call	IntHandler13C
		// EOI on master and slave needed
	mov	$0x65, %al
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret

			// Int 14 interrupts

	.global IntHandlerIde
IntHandlerIde:
	cli
	pusha
	pushf
	
	call	IntHandlerCIde

		// EOI on master and slave needed
	mov	$0x66, %al // was $0x20
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret

		// Int 15 interrupts

	.global IntHandler15
IntHandler15:
	cli
	pusha
	pushf
	
	call	IntHandler15C
		// EOI on master and slave needed
	mov	$0x67, %al
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret


		// unused interrupts on master PIC

	.global IntHandlerUnused
IntHandlerUnused:
	cli
	pusha
	pushf
	
	call	IntHandlerUnusedC
	mov	$0x20, %al
	out	%al, $0x20
	popf
	popa
	iret

		// unused interrupts on slave PIC

	.global IntHandlerUnusedPic2
IntHandlerUnusedPic2:
	cli
	pusha
	pushf
	
	call	IntHandlerUnusedC2
	mov $0xffff, %ax
	mov	$0x8028, %dx
	outw	%ax, %dx
	mov	$0x80cc, %dx
	mov	$0x40, %al
	outb	%al, %dx

	mov	$0x20, %al
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret

	.global SpareIntNop
SpareIntNop:
	iret


		// CPU Exception Interrupts

	.global IntHandlerException0
IntHandlerException0:
	pusha
	pushf
	cli
	call	IntHandlerException0C
	popf
	popa
	iret

	.global IntHandlerException1
IntHandlerException1:
	pusha
	pushf
	cli
	call	IntHandlerException1C
	popf
	popa
	iret

	.global IntHandlerException2
IntHandlerException2:
	pusha
	pushf
	cli
	call	IntHandlerException2C
	popf
	popa
	iret

	.global IntHandlerException3
IntHandlerException3:
	pusha
	pushf
	cli
	call	IntHandlerException3C
	popf
	popa
	iret

	.global IntHandlerException4
IntHandlerException4:
	pusha
	pushf
	cli
	call	IntHandlerException4C
	popf
	popa
	iret

	.global IntHandlerException5
IntHandlerException5:
	pusha
	pushf
	cli
	call	IntHandlerException5C
	popf
	popa
	iret

	.global IntHandlerException6
IntHandlerException6:
	pusha
	pushf
	cli
	call	IntHandlerException6C
	popf
	popa
	iret

	.global IntHandlerException7
IntHandlerException7:
	pusha
	pushf
	cli
	call	IntHandlerException7C
	popf
	popa
	iret

	.global IntHandlerException8
IntHandlerException8:
	pusha
	pushf
	cli
	call	IntHandlerException8C
	popf
	popa
	iret

	.global IntHandlerException9
IntHandlerException9:
	pusha
	pushf
	cli
	call	IntHandlerException9C
	popf
	popa
	iret

	.global IntHandlerExceptionA
IntHandlerExceptionA:
	pusha
	pushf
	cli
	call	IntHandlerExceptionAC
	popf
	popa
	iret

	.global IntHandlerExceptionB
IntHandlerExceptionB:
	pusha
	pushf
	cli
	call	IntHandlerExceptionBC
	popf
	popa
	iret

	.global IntHandlerExceptionC
IntHandlerExceptionC:
	pusha
	pushf
	cli
	call	IntHandlerExceptionCC
	popf
	popa
	iret

	.global IntHandlerExceptionD
IntHandlerExceptionD:
	pusha
	pushf
	cli
	call	IntHandlerExceptionDC
	popf
	popa
	iret

	.global IntHandlerExceptionE
IntHandlerExceptionE:
	pusha
	pushf
	cli
	call	IntHandlerExceptionEC
	popf
	popa
	iret

	.global IntHandlerExceptionF
IntHandlerExceptionF:
	pusha
	pushf
	cli
	call	IntHandlerExceptionFC
	popf
	popa
	iret

	.global IntHandlerException10
IntHandlerException10:
	pusha
	pushf
	cli
	call	IntHandlerException10C
	popf
	popa
	iret





