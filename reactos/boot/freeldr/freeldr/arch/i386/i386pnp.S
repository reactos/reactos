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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <asm.inc>
#include <arch/pc/x86common.h>

.code32


/*
 * U32 PnpBiosSupported(VOID);
 *
 * RETURNS:
 */
_pnp_bios_entry_point:
	.long	0
_pnp_bios_data_segment:
	.word	0

PUBLIC _PnpBiosSupported
_PnpBiosSupported:

	push edi
	push esi
	push ecx
	push edx

	xor edi, edi

	/* init esi */
	mov esi, HEX(0F0000)

pnp_again:
	mov eax, [esi]
	cmp eax, HEX(506E5024)			/* "$PnP" */
	je pnp_found

	cmp esi, HEX(0FFFF0)
	je pnp_not_found

pnp_add:
	add esi, 16
	jmp pnp_again

pnp_found:
	/* first calculate the checksum */
	push esi

	push HEX(21)
	pop ecx
	xor edx, edx

pnp_loop:
	lodsb
	add dl, al
	loop pnp_loop

	test dl, dl
	pop esi
	jnz pnp_add

	mov edi, esi

	/* Calculate the bios entry point (far pointer) */
	xor eax, eax
	mov ax, [esi + 15]
	shl eax, 16
	mov ax, [esi + 0x0D]
	mov _pnp_bios_entry_point, eax

	/* Store bios data segment */
	mov ax, [esi + 0x1B]
	mov _pnp_bios_data_segment, ax

pnp_not_found:
	mov eax, edi

	pop edx
	pop ecx
	pop esi
	pop edi

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

PUBLIC _PnpBiosGetDeviceNodeCount
_PnpBiosGetDeviceNodeCount:

	push ebp
	mov ebp, esp

	pusha
	push es

	call switch_to_real
	.code16

	mov ax, word ptr [_pnp_bios_data_segment]
	push ax

	push cs
	mov ax, offset _pnp_node_size
	push ax

	push cs
	mov ax, offset _pnp_node_count
	push ax

	push 0

	call dword ptr [_pnp_bios_entry_point]
	add sp, 12

	movzx ecx, ax
	mov _pnp_result, ecx


	call switch_to_prot
	.code32

	mov esi, [ebp + 8]
	mov ax, _pnp_node_size
	movzx ecx, ax
	mov [esi], ecx

	mov esi, [ebp + 12]
	mov ax, _pnp_node_count
	movzx ecx, ax
	mov [esi], eax

	pop es
	popa

	mov esp, ebp
	pop ebp

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
	.code32

	push ebp
	mov ebp, esp

	pusha
	push es

	/* get current node number */
	mov esi, [ebp + 8]
	mov al, [esi]
	mov _pnp_node_number, al

	/* convert pointer to node buffer to segment/offset */
	mov eax, [ebp + 12]
	shr eax, 4
	and eax, 0xf000
	mov _pnp_buffer_segment, ax
	mov eax, [ebp + 12]
	and eax, 0xffff
	mov _pnp_buffer_offset, ax

	call switch_to_real
	.code16

	/* push bios segment */
	mov ax, word ptr [_pnp_bios_data_segment]
	push ax

	/* push control flag */
	push 1

	/* push pointer to node buffer (segment/offset) */
	mov ax, word ptr [_pnp_buffer_segment]
	push ax
	mov ax, word ptr [_pnp_buffer_offset]
	push ax

	/* push pointer to node number (segment/offset) */
	push cs
	mov ax, offset _pnp_node_number
	push ax

	/* push function number */
	push 1

	/* call entry point */
	call dword ptr [_pnp_bios_entry_point]
	add sp, 14

	movzx ecx, ax
	mov _pnp_result, ecx

	call switch_to_prot
	.code32

	/* update node number */
	mov esi, [ebp + 8]
	mov al, _pnp_node_number
	mov [esi], al

	pop es
	popa

	mov esp, ebp
	pop ebp

	mov eax, _pnp_result

	ret

/* EOF */
