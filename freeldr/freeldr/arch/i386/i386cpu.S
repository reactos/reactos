/*
 *  FreeLoader
 *  Copyright (C) 2003  Eric Kohl  <ekohl@rz-online.de>
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

/*
 * U32 CpuidSupported(VOID);
 *
 * RETURNS:
 *    0x00000001: CPU supports the CPUID instruction
 *    0x00000300: Found 80386 CPU
 *    0x00000400: Found 80486 CPU without CPUID support
 */

EXTERN(_CpuidSupported)
	.code32

	pushl	%ecx			/* save ECX */

	pushfl					/* push original EFLAGS */
	popl	%eax			/* get original EFLAGS */
	movl	%eax,%ecx		/* save original EFLAGS */
	xorl	$0x40000,%eax	/* flip AC bit in EFLAGS */
	pushl	%eax			/* save new EFLAGS value on stack */
	popfl					/* replace current EFLAGS value */

	pushfl					/* get new EFLAGS */
	popl	%eax			/* store new EFLAGS in EAX */
	xorl	%ecx, %eax		/* can't toggle AC bit, processor=80386 */

	movl	$0x300,%eax		/* return processor id */
    jz		NoCpuid			/* jump if 80386 processor */

	pushl	%ecx
	popfl					/* restore AC bit in EFLAGS first */

	movl	%ecx,%eax		/* get original EFLAGS */
	xorl	$0x200000,%eax	/* flip ID bit in EFLAGS */
	pushl	%eax			/* save new EFLAGS value on stack */
	popfl					/* replace current EFLAGS value */
	pushfl					/* get new EFLAGS */
	popl	%eax			/* store new EFLAGS in EAX */
	xorl	%ecx,%eax		/* can't toggle ID bit, */

	movl	$0x400,%eax		/* return processor id */
	je		NoCpuid			/* processor=80486 */

	movl	$1,%eax			/* CPUID supported */

NoCpuid:
	pushl	%ecx
	popfl					/* restore EFLAGS */
	popl	%ecx			/* retore ECX */

	ret


/*
 * VOID GetCpuid(U32 Level, U32 *eax, U32 *ebx, U32 *ecx, U32 *edx);
 */

EXTERN(_GetCpuid)
	.code32

	pushl	%ebp
	movl	%esp,%ebp

	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%esi

	movl	0x08(%ebp),%eax
	cpuid

	movl	0x0C(%ebp),%esi
	movl	%eax, (%esi)

	movl	0x10(%ebp),%esi
	movl	%ebx, (%esi)

	movl	0x14(%ebp),%esi
	movl	%ecx, (%esi)

	movl	0x18(%ebp),%esi
	movl	%edx, (%esi)

	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax

	movl	%ebp,%esp
	popl	%ebp
	ret

/*
 * U32 MpsSupported(VOID);
 *
 * RETURNS:
 */

	.code16
_mps_supported:
	.long	0
_mps_fp_table_offset:
	.word	0

EXTERN(_MpsSupported)
	.code32

	pushl	%ebp
	movl	%esp,%ebp
	pushal
	push	%es

	call	switch_to_real

	.code16

	/* init ES */
	pushw	$0xF000
	popw	%es

	/* init SI */
	movw	$0xFFF0,%si

mps_again:
	movw	%es:(%si),%ax
	cmp		$0x4D5F,%ax			/* "_M" */
	jne		mps_next
	
	push	%si
	add		$2,%si
	movw	%es:(%si),%ax
	cmp		$0x5F50,%ax			/* "P_" */
	pop		%si
	je		mps_found

mps_next:
	cmp		$0,%si
	je		mps_not_found

	sub		$0x10,%si
	jmp		mps_again

mps_found:
	movw	%si,_mps_fp_table_offset
	movzwl	%si,%eax
	movl	%eax,_mps_supported

mps_not_found:
	call	switch_to_prot

	.code32

	pop	%es
	popal
	movl	%ebp,%esp
	popl	%ebp

	movl	_mps_supported,%eax

	ret


/*
 * U32 MpsGetDefaultConfiguration(VOID);
 *
 * RETURNS:
 */
	.code16
_mps_default_configuration:
	.long	0

EXTERN(_MpsGetDefaultConfiguration)
	.code32

	pushl	%ebp
	movl	%esp,%ebp
	pushal
	push	%es

	call	switch_to_real

	.code16

	/* init ES */
	pushw	$0xF000
	popw	%es

	/* init SI */
	movw	_mps_fp_table_offset,%si

	/* read default configuration (feature byte 1) */
	addw	$0x0B,%si
	movb	%es:(%si),%al

	/* save configuration */
	movzbl	%al,%ecx
	movl	%ecx,_mps_default_configuration

	call	switch_to_prot

	.code32

	pop	%es
	popal
	movl	%ebp,%esp
	popl	%ebp

	movl	_mps_default_configuration,%eax

	ret


/*
 * U32 MpsGetConfigurationTable(PVOID ConfigTable);
 *
 * RETURNS:
 */
	.code16
_mps_buffer_segment:
	.word	0
_mps_buffer_offset:
	.word	0

_mps_config_segment:
	.word	0
_mps_config_offset:
	.word	0
_mps_config_length:
	.word	0

EXTERN(_MpsGetConfigurationTable)
	.code32

	pushl	%ebp
	movl	%esp,%ebp
	pushal
	push	%es

	/* convert pointer to node buffer to segment/offset */ 
	movl	0x08(%ebp),%eax
	shrl	$4,%eax
	andl	$0xf000,%eax
	movw	%ax,_mps_buffer_segment	
	movl	0x08(%ebp),%eax
	andl	$0xffff,%eax
	movw	%ax,_mps_buffer_offset	

	call	switch_to_real

	.code16

	/* init ES */
	pushw	$0xF000
	popw	%es

	/* init SI */
	movw	_mps_fp_table_offset,%si

	/* read pointer to configuration table */
	addw	$0x04,%si
	movl	%es:(%si),%eax

	/* save table offset */
	movw	%ax,_mps_config_offset

	/* save table segment */
	shrl	$0x4,%eax
	andw	$0xf000,%ax
	movw	%ax,_mps_config_segment

	/* init ES */
	pushw	_mps_config_segment
	popw	%es

	/* init CX (read table lenght) */
	movw	_mps_config_offset,%si
	addw	$0x04,%si
	movw	%es:(%si),%cx

	/* init ES */
	pushw	_mps_buffer_segment
	popw	%es

	/* init DI */
	movw	_mps_buffer_offset,%di

	/* init SI */
	movw	_mps_config_offset,%si

	/* init DS */
.code32
	movw	_mps_config_segment,%ax
.code16
	pushw	%ds
	pushw	%ax
	popw	%ds

	/* copy table */
//	cld
	rep movsb

	/* restore DS */
	pop	%ds

	call	switch_to_prot

	.code32

	pop	%es
	popal
	movl	%ebp,%esp
	popl	%ebp

	ret

/* EOF */
