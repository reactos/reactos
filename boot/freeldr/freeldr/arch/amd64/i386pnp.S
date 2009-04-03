/*
 *  FreeLoader
 *  Copyright (C) 2003  Eric Kohl
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
	.code64

	push rdi
	push rsi
	push rcx
	push rdx

	xor edi, edi

	/* init esi */
	mov esi, 0xF0000

pnp_again:
	mov eax, [%esi]
	cmp eax, 0x506E5024			/* "$PnP" */
	je pnp_found

	cmp esi, 0xFFFF0
	je pnp_not_found

pnp_add:
	add esi, 0x10
	jmp pnp_again

pnp_found:
	/* first calculate the checksum */
	push rsi

	push 0x21
	pop rcx
	xor edx, edx

pnp_loop:
	lodsb
	add dl, al
	loop pnp_loop

	test dl, dl
	pop rsi
	jnz pnp_add

	mov edi, esi

	/* Calculate the bios entry point (far pointer) */
	xor eax, eax
	mov ax, [esi + 0x0F]
	shl eax, 16
	mov ax, [esi + 0x0D]
	mov _pnp_bios_entry_point, eax

	/* Store bios data segment */
	mov ax, [esi + 0x1B]
	mov _pnp_bios_data_segment, ax

pnp_not_found:
	mov eax, edi

	pop rdx
	pop rcx
	pop rsi
	pop rdi

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
	.code64

	push rbp
	mov rbp, rsp

//	pushal FIXME
//	push es

	call x86_64_SwitchToReal
	.code16

	mov ax, _pnp_bios_data_segment
	push ax

	push cs
	mov ax, _pnp_node_size
	push ax

	push cs
	mov ax, _pnp_node_count
	push ax

	push 0

	call [_pnp_bios_entry_point]
	add sp, 12

	movzx ecx, ax
	mov _pnp_result, ecx

	call	x86_16_SwitchToLong
	.code64

	mov esi, [rbp + 0x08]
	mov ax, _pnp_node_size
	movzx ecx, ax
	mov [rsi], ecx

	mov rsi, [ebp + 0x0C]
	mov ax, _pnp_node_count
	movzx ecx, ax
	mov [rsi], eax

//	pop es
//	popal

	mov rsp, rbp
	pop rbp

	xor rax, rax
	mov eax, _pnp_result

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
	.code64

	push rbp
	mov rbp, rsp

//	pushal
//	push es

	/* get current node number */
	mov rsi, [rbp + 0x08]
	mov al, [rsi]
	mov _pnp_node_number, al

	/* convert pointer to node buffer to segment/offset */ 
	mov eax, [rbp + 0x0C]
	shr eax, 4
	and eax, 0xf000
	mov _pnp_buffer_segment	, ax
	mov eax, [rbp + 0x0C]
	and eax, 0xffff
	mov _pnp_buffer_offset, ax

	call x86_64_SwitchToReal
	.code16

	/* push bios segment */
	mov ax, _pnp_bios_data_segment
	push ax

	/* push control flag */	
	push 0x0001

	/* push pointer to node buffer (segment/offset) */
	mov ax, _pnp_buffer_segment
	push ax
	mov ax, _pnp_buffer_offset
	push ax

	/* push pointer to node number (segment/offset) */
	push cs
	mov ax, _pnp_node_number
	push ax

	/* push function number */
	push 1

	/* call entry point */
	call [_pnp_bios_entry_point]
	add sp, 14

	movzx ecx, ax
	mov _pnp_result, ecx

	call x86_16_SwitchToLong
	.code64

	/* update node number */
	mov rsi, [rbp + 0x08]
	mov al, _pnp_node_number
	mov [rsi], al

//	pop es
//	popal

	mov rsp, rbp
	pop rbp

	xor rax, rax
	mov eax, _pnp_result

	ret

/* EOF */
