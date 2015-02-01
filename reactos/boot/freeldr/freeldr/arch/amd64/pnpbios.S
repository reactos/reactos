/*
 *  FreeLoader
 *  Copyright (C) 2003  Eric Kohl
 *  Copyright (C) 2011 Timo Kreuzer
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

EXTERN CallRealMode:PROC

.code64

/*
 * U32 PnpBiosSupported(VOID);
 *
 * RETURNS:
 */
PUBLIC PnpBiosSupported
PnpBiosSupported:

    push rdi
    push rsi
    push rcx
    push rdx

    xor rdi, rdi

    /* init rsi */
    mov rsi, HEX(0F0000)

pnp_again:
    mov eax, [rsi]
    cmp eax, HEX(506E5024)  /* "$PnP" */
    je pnp_found

    cmp rsi, HEX(0FFFF0)
    je pnp_not_found

pnp_add:
    add rsi, 16
    jmp pnp_again

pnp_found:
    /* first calculate the checksum */
    push rsi

    push HEX(21)
    pop rcx
    xor edx, edx

pnp_loop:
    lodsb
    add dl, al
    loop pnp_loop

    test dl, dl
    pop rsi
    jnz pnp_add

    mov rdi, rsi

    /* Calculate the bios entry point (far pointer) */
    xor eax, eax
    mov ax, [rsi + HEX(0F)]
    shl eax, 16
    mov ax, [rsi + HEX(0D)]
    mov dword ptr [BSS_PnpBiosEntryPoint], eax

    /* Store bios data segment */
    mov ax, [rsi + HEX(1B)]
    mov word ptr [BSS_PnpBiosDataSegment], ax

pnp_not_found:
    mov rax, rdi

    pop rdx
    pop rcx
    pop rsi
    pop rdi

    ret


/*
 * U32 PnpBiosGetDeviceNodeCount(U32 *NodeSize<rcx>, U32 *NodeCount<rdx>);
 *
 * RETURNS:
 */
PUBLIC PnpBiosGetDeviceNodeCount
PnpBiosGetDeviceNodeCount:

    /* Call the real mode function */
    mov bx, FNID_PnpBiosGetDeviceNodeCount
    call CallRealMode

    xor eax, eax
    mov ax, [BSS_PnpNodeSize]
    mov [rcx], eax
    mov ax, [BSS_PnpNodeCount]
    mov [rdx], eax

    mov eax, dword ptr [BSS_PnpResult]

    ret



/*
 * U32 PnpBiosGetDeviceNode(U8 *NodeId<rcx>, U8 *NodeBuffer<rdx>);
 *
 * RETURNS:
 */
PUBLIC PnpBiosGetDeviceNode
PnpBiosGetDeviceNode:

    /* get current node number */
    mov al, [rcx]
    mov byte ptr [BSS_PnpNodeNumber], al

    /* convert pointer to node buffer to segment/offset */
    mov rax, rdx
    shr eax, 4
    mov word ptr [BSS_PnpBiosBufferSegment], ax
    mov rax, rdx
    and eax, HEX(0f)
    mov word ptr [BSS_PnpBiosBufferOffset], ax

    /* Call the real mode function */
    mov bx, FNID_PnpBiosGetDeviceNode
    call CallRealMode

    /* update node number */
    mov al, byte ptr [BSS_PnpNodeNumber]
    mov [rcx], al

    mov eax, [BSS_PnpResult]

    ret

END
/* EOF */
