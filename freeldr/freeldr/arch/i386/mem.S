/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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

#define ASM
#include "arch.h"
#include "multiboot.h"



	.code32
EXTERN(_GetExtendedMemorySize)

	//
	// get extended memory size in KB
	//
	pushl	%edx
	pushl	%ecx
	pushl	%ebx

	call	switch_to_real
	.code16

	movw	$0xe801,%ax
	int		$0x15
	jc		.oldstylemem

	cmpw	$0,%ax
	je		.cmem

	movzwl	%bx,%ebx
	shll	$6,%ebx
	movzwl	%ax,%eax
	addl	%ebx,%eax
	jmp		.done_mem

.cmem:
	cmpw	$0,%cx
	je		.oldstylemem

	movzwl	%dx,%edx
	shll	$6,%edx
	movzwl	%cx,%ecx
	addl	%ecx,%edx
	movl	%edx,%eax
	jmp		.done_mem

.oldstylemem:
	// int 15h opt e801 don't work , try int 15h, option 88h
	movb	$0x88,%ah
	int		$0x15
	cmp		$0,%ax
	je		.cmosmem
	movzwl	%ax,%eax
	jmp		.done_mem

.cmosmem:
	// int 15h opt 88h don't work , try read cmos
	xorl	%eax,%eax
	movb	$0x31,%al
	outb	%al,$0x70
	inb		$0x71,%al
	andl	$0xffff,%eax	// clear carry
	shll	$8,%eax

.done_mem:


	/* Save return value */
	movl	%eax,%edx

	call	switch_to_prot

	.code32

	/* Restore return value */
	movl	%edx,%eax

	popl	%ebx
	popl	%ecx
	popl	%edx

	ret



	.code32
EXTERN(_GetConventionalMemorySize)

	//
	// get conventional memory size in KB
	//
	pushl	%edx

	call	switch_to_real
	.code16

	xorl	%eax,%eax
	int		$0x12

	/*xorl	%eax,%eax
	movb	$0x30,%al
	outb	%al,$0x70
	inb		$0x71,%al
	andl	$0xffff,%eax*/	// clear carry

	/* Save return value */
	movl	%eax,%edx

	call	switch_to_prot

	.code32

	/* Restore return value */
	movl	%edx,%eax

	popl	%edx

	ret



	.code32
_gbmm_mem_map_length:
	.long 0
_gbmm_memory_map_addr:
	.long 0
EXTERN(_GetBiosMemoryMap)
	//
	// Retrieve BIOS memory map if available
	//
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	pushl	%edi

	movl	$0,_gbmm_mem_map_length

	/* Get memory map address off stack */
	movl	0x10(%esp),%eax
	movl	%eax,_gbmm_memory_map_addr

	call	switch_to_real
	.code16

	xorl	%ebx,%ebx
	movl	_gbmm_memory_map_addr,%edi

.mmap_next:

	movl	$0x534D4150,%edx	// 'SMAP'
	movl	$/*sizeof(memory_map_t)*/24,%ecx
	movl	0xE820,%eax
	int		$0x15
	jc		.done_mmap

	cmpl	$0x534D4150,%eax	// 'SMAP'
	jne		.done_mmap

	addl	$/*sizeof(memory_map_t)*/24,%edi
	addl	$/*sizeof(memory_map_t)*/24,_gbmm_mem_map_length

	cmpl	$0,%ebx
	jne		.mmap_next

.done_mmap:

	call	switch_to_prot

	.code32

	/* Get return value */
	movl	_gbmm_mem_map_length,%eax

	popl	%edi
	popl	%ebx
	popl	%ecx
	popl	%edx

	ret
