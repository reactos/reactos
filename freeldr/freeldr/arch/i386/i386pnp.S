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
 * U32 PnpBiosSupported(VOID);
 *
 * RETURNS:
 */
_pnp_bios_entry_point:
	.long	0
_pnp_bios_data_segment:
	.word	0

EXTERN(_PnpBiosSupported)
	.code32

	pushl	%edi
	pushl	%esi

	xorl	%edi,%edi

	/* init esi */
	movl	$0xFFFF0,%esi

pnp_again:
	movl	(%esi),%eax
	cmp		$0x506E5024,%eax			/* "$PnP" */
	je		pnp_found

	cmp		$0xF0000,%esi
	je		pnp_not_found

	subl	$0x10,%esi
	jmp		pnp_again

pnp_found:
	movl	%esi,%edi

	/* Calculate the bios entry point (far pointer) */
	xorl	%eax,%eax
	movw	0x0F(%esi),%ax
	shll	$16,%eax
	movw	0x0D(%esi),%ax
	movl	%eax,_pnp_bios_entry_point

	/* Store bios data segment */
	movw	0x1B(%esi),%ax
	movw	%ax,_pnp_bios_data_segment

pnp_not_found:
	movl	%edi,%eax

	popl	%esi
	popl	%edi

	ret


/*
 * U32 PnpBiosGetDeviceNodeCount(U32 *NodeSize, U32 *NodeCount);
 *
 * RETURNS:
 */
_pnp_result:
	.long	0
_pnp_node_size:
	.word	0
_pnp_node_count:
	.word	0

EXTERN(_PnpBiosGetDeviceNodeCount)
	.code32

	pushl	%ebp
	movl	%esp,%ebp

	pushal
	push	%es

	call	switch_to_real
	.code16

	movw	_pnp_bios_data_segment,%ax
	pushw	%ax

	pushw	%cs
	movw	$(_pnp_node_size),%ax
	pushw	%ax

	pushw	%cs
	movw	$(_pnp_node_count),%ax
	pushw	%ax

	pushw	$0

	lcall	*_pnp_bios_entry_point
	addw	$12,%sp

	movzwl	%ax,%ecx
	movl	%ecx,_pnp_result

	
	call	switch_to_prot
	.code32

	movl	0x08(%ebp),%esi
	movw	_pnp_node_size,%ax
	movzwl	%ax,%ecx
	movl	%ecx, (%esi)

	movl	0x0C(%ebp),%esi
	movw	_pnp_node_count,%ax
	movzwl	%ax,%ecx
	movl	%eax, (%esi)

	pop		%es
	popal

	movl	%ebp,%esp
	popl	%ebp

	movl	_pnp_result,%eax

	ret


/*
 * U32 PnpBiosGetDeviceNode(U8 *NodeId, U8 *NodeBuffer);
 *
 * RETURNS:
 */
_pnp_buffer_segment:
	.word	0
_pnp_buffer_offset:
	.word	0

_pnp_node_number:
	.byte	0

EXTERN(_PnpBiosGetDeviceNode)
	.code32

	pushl	%ebp
	movl	%esp,%ebp

	pushal
	push	%es

	/* get current node number */
	movl	0x08(%ebp),%esi
	movb	(%esi),%al
	movb	%al,_pnp_node_number

	/* convert pointer to node buffer to segment/offset */ 
	movl	0x0C(%ebp),%eax
	shrl	$4,%eax
	andl	$0xf000,%eax
	movw	%ax,_pnp_buffer_segment	
	movl	0x0C(%ebp),%eax
	andl	$0xffff,%eax
	movw	%ax,_pnp_buffer_offset	

	call	switch_to_real
	.code16

	/* push bios segment */
	movw	_pnp_bios_data_segment,%ax
	pushw	%ax

	/* push control flag */	
	pushw	$0x0001

	/* push pointer to node buffer (segment/offset) */
	movw	_pnp_buffer_segment,%ax
	pushw	%ax
	movw	_pnp_buffer_offset,%ax
	pushw	%ax

	/* push pointer to node number (segment/offset) */
	pushw	%cs
	movw	$(_pnp_node_number),%ax
	pushw	%ax

	/* push function number */
	pushw	$1

	/* call entry point */
	lcall	*_pnp_bios_entry_point
	addw	$14,%sp

	movzwl	%ax,%ecx
	movl	%ecx,_pnp_result

	call	switch_to_prot
	.code32

	/* update node number */
	movl	0x08(%ebp),%esi
	movb	_pnp_node_number,%al
	movb	%al,(%esi)

	pop		%es
	popal

	movl	%ebp,%esp
	popl	%ebp

	movl	_pnp_result,%eax

	ret

/* EOF */
