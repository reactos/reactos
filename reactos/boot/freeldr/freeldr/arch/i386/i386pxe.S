/*
 *  FreeLoader
 *  Copyright (C) 2011  Hervé Poussineau
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

	.text
	.code16

#define ASM

#include <arch.h>

/*
 * U16 PxeCallApi(U16 Segment, U16 Offset, U16 Service, VOID *Parameter);
 *
 * RETURNS:
 */
_pxe_function:
	.word	0
_pxe_entry_point:
	.long	0
_pxe_buffer_segment:
	.word0
_pxe_buffer_offset:
	.word0
_pxe_result:
	.word	0

EXTERN(_PxeCallApi)
	.code32
	pushl	%ebp
	movl	%esp,%ebp

	pushal
	push	%es

	/* copy entry point */
	movl	0x08(%ebp),%eax
	shll	$16,%eax
	movw	0x0C(%ebp),%ax
	movl	%eax,_pxe_entry_point

	/* copy function */
	movw	0x10(%ebp),%ax
	movw	%ax,_pxe_function

	/* convert pointer to data buffer to segment/offset */
	movl	0x14(%ebp),%eax
	shrl	$4,%eax
	andl	$0xf000,%eax
	movw	%ax,_pxe_buffer_segment
	movl	0x14(%ebp),%eax
	andl	$0xffff,%eax
	movw	%ax,_pxe_buffer_offset

	call	switch_to_real
	.code16

	movw	_pxe_buffer_segment,%ax
	push	%ax
	movw	_pxe_buffer_offset,%ax
	push	%ax
	movw	_pxe_function,%ax
	push	%ax
	lcall	*_pxe_entry_point
	addw	$6,%sp
	movw	%ax,_pxe_result

	call	switch_to_prot
	.code32

	pop	%es
	popal

	movl	%ebp,%esp
	popl	%ebp

	movw	_pxe_result,%ax

	ret

/* EOF */

