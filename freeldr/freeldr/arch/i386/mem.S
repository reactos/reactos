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
#include <arch.h>
#include <multiboot.h>



/*
 * ULONG GetExtendedMemorySize(VOID);
 */
ExtendedMemorySize:
	.long	0
EXTERN(_GetExtendedMemorySize)
	.code32

	pushal

	movl	$0,ExtendedMemorySize

	call	switch_to_real
	.code16

	movw	$0xE801,%ax
	int		$0x15
	jc		GetExtendedMemorySizeTryInt15Func88

	cmpw	$0,%ax
	je		GetExtendedMemorySizeUseCXDX

	movzwl	%bx,%ebx
	shll	$6,%ebx
	movzwl	%ax,%eax
	addl	%ebx,%eax
	movl	%eax,ExtendedMemorySize
	jmp		GetExtendedMemorySizeDone

GetExtendedMemorySizeUseCXDX:
	cmpw	$0,%cx
	je		GetExtendedMemorySizeTryInt15Func88

	movzwl	%dx,%edx
	shll	$6,%edx
	movzwl	%cx,%ecx
	addl	%ecx,%edx
	movl	%edx,ExtendedMemorySize
	jmp		GetExtendedMemorySizeDone

GetExtendedMemorySizeTryInt15Func88:
	movb	$0x88,%ah
	int		$0x15
	jc		GetExtendedMemorySizeTryCMOS
	cmpw	$0,%ax
	je		GetExtendedMemorySizeTryCMOS
	movzwl	%ax,%eax
	movl	%eax,ExtendedMemorySize
	jmp		GetExtendedMemorySizeDone

GetExtendedMemorySizeTryCMOS:
	xorl	%eax,%eax
	movb	$0x31,%al
	outb	%al,$0x70
	inb		$0x71,%al
	andl	$0xffff,%eax
	shll	$8,%eax
	movl	%eax,ExtendedMemorySize

GetExtendedMemorySizeDone:

	call	switch_to_prot

	.code32

	popal

	/* Restore return value */
	movl	ExtendedMemorySize,%eax

	ret



/*
 * ULONG GetConventionalMemorySize(VOID);
 */
ConventionalMemorySize:
	.long	0
EXTERN(_GetConventionalMemorySize)
	.code32

	pushal

	movl	$0,ConventionalMemorySize

	call	switch_to_real
	.code16

	xorl	%eax,%eax
	int		$0x12

	/* Save return value */
	movzwl	%ax,%eax
	movl	%eax,ConventionalMemorySize

	call	switch_to_prot

	.code32

	popal

	/* Restore return value */
	movl	ConventionalMemorySize,%eax

	ret



/*
 * ULONG GetBiosMemoryMap(BIOS_MEMORY_MAP BiosMemoryMap[32]);
 */
_gbmm_mem_map_length:
	.long 0
_gbmm_memory_map_addr:
	.long 0
_gbmm_memory_map:
	.rept	32
	.quad	0
	.quad	0
	.long	0
	.long	0
	.endr
EXTERN(_GetBiosMemoryMap)
	.code32

	pushal

	movl	$0,_gbmm_mem_map_length

	/* Get memory map structure array address off stack */
	movl	0x24(%esp),%eax
	movl	%eax,_gbmm_memory_map_addr

	call	switch_to_real
	.code16

	xorl	%ebx,%ebx
	movl	$_gbmm_memory_map,%edi

GetBiosMemoryMapNext:

	movl	$0x534D4150,%edx	// 'SMAP'
	movl	$24,%ecx
	movl	$0xE820,%eax
	int		$0x15
	jc		GetBiosMemoryMapDone

	// If the BIOS didn't return 'SMAP' in EAX then
	// it doesn't support this call
	cmpl	$0x534D4150,%eax	// 'SMAP'
	jne		GetBiosMemoryMapDone

	// Increment our count of items and the offset in the array
	addl	$24,%edi
	incl	_gbmm_mem_map_length

	// If we have copied 32 items then we can't hold any
	// more in our array so we're done
	cmpl	$32,_gbmm_mem_map_length
	jae		GetBiosMemoryMapDone

	// If the continuation value is zero then this was
	// the last entry so we're done
	cmpl	$0,%ebx
	jne		GetBiosMemoryMapNext

GetBiosMemoryMapDone:

	call	switch_to_prot

	.code32

	// Copy the memory map data into the supplied buffer
	movl	$_gbmm_memory_map,%esi
	movl	_gbmm_memory_map_addr,%edi
	movl	$(24 * 32),%ecx
	rep movsb

	popal

	/* Get return value */
	movl	_gbmm_mem_map_length,%eax

	ret
