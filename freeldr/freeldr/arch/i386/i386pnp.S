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
 * U32 PnpBiosSupported(PVOID InstallationCheck);
 *
 * RETURNS:
 */
_pnp_support_result:
	.long	0
_pnp_entry_point:
	.word	0
_pnp_code_segment:
	.word	0
_pnp_data_segment:
	.word	0

EXTERN(_PnpBiosSupported)
	.code32

	pushal
	push	%es

	/* convert pointer to node buffer to segment/offset */ 
	movl	0x08(%ebp),%eax
	shrl	$4,%eax
	andl	$0xf000,%eax
	movw	%ax,_pnp_buffer_segment	
	movl	0x08(%ebp),%eax
	andl	$0xffff,%eax
	movw	%ax,_pnp_buffer_offset	

	call	switch_to_real

	.code16

	/* init ES */
	pushw	$0xF000
	popw	%es

	/* init cx */
	movw	$0xFFF0,%si

pnp_again:
	movw	%es:(%si),%ax
	cmp		$0x5024,%ax			/* "$P" */
	jne		pnp_next
	
	push	%si
	add		$2,%si
	movw	%es:(%si),%ax
	cmp		$0x506E,%ax			/* "nP" */
	pop		%si
	je		pnp_found

pnp_next:
	cmp		$0,%si
	je		pnp_not_found

	sub		$0x10,%si
	jmp		pnp_again

pnp_found:
	movzwl	%si,%eax
	movl	%eax,_pnp_support_result

	add		$0x0D,%si
	movw	%es:(%si),%ax
	movw	%ax,_pnp_entry_point

	add		$0x02,%si
	movw	%es:(%si),%ax
	movw	%ax,_pnp_code_segment

	add		$0x0C,%si
	movw	%es:(%si),%ax
	movw	%ax,_pnp_data_segment

    /* copy data to buffer */
    push	%ds
    movw	%es, %ax
    movw	%ax, %ds
    movl	_pnp_support_result, %esi
    movw	_pnp_buffer_segment, %es
    movw	_pnp_buffer_offset, %di
	movw	$0x21, %cx
	rep		movsb
	
	pop		%ds
pnp_not_found:

	call	switch_to_prot

	.code32

	pop		%es
	popal

	movl	_pnp_support_result,%eax

	ret


/*
 * U32 PnpBiosGetDeviceNodeCount(U32 *NodeSize, U32 *NodeCount);
 *
 * RETURNS:
 */
_entry_point:
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

	movw	_pnp_data_segment,%ax
	pushw	%ax

	pushw	%cs
	movw	$(_pnp_node_size),%ax
	pushw	%ax

	pushw	%cs
	movw	$(_pnp_node_count),%ax
	pushw	%ax

	pushw	$0

	movw	_pnp_code_segment,%ax
	movzwl	%ax,%ecx
	shll	$16,%ecx
	movw	_pnp_entry_point,%ax
	movzwl	%ax,%ebx
	orl		%ebx,%ecx
	movl	%ecx,_entry_point

	lcall	*_entry_point
	addw	$12,%sp

	movzwl	%ax,%ecx
	movl	%ecx,_pnp_support_result

	
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

	movl	_pnp_support_result,%eax

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
	movw	_pnp_data_segment,%ax
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
	lcall	*_entry_point
	addw	$14,%sp

	movzwl	%ax,%ecx
	movl	%ecx,_pnp_support_result


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

	movl	_pnp_support_result,%eax

	ret

/* EOF */
