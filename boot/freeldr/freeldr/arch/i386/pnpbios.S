/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PnP BIOS helper functions
 * COPYRIGHT:   Copyright 2003 Eric Kohl <eric.kohl@reactos.org>
 */

#include <asm.inc>
#include <arch/pc/x86common.h>

EXTERN i386CallRealMode:PROC

.code32

/*
 * U32 PnpBiosSupported(VOID);
 *
 * RETURNS:
 */
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
    cmp eax, HEX(506E5024)  /* "$PnP" */
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
    mov ax, [esi + HEX(0F)]
    shl eax, 16
    mov ax, [esi + HEX(0D)]
    mov dword ptr ds:[BSS_PnpBiosEntryPoint], eax

    /* Store bios data segment */
    mov ax, [esi + HEX(1B)]
    mov word ptr ds:[BSS_PnpBiosDataSegment], ax

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
PUBLIC _PnpBiosGetDeviceNodeCount
_PnpBiosGetDeviceNodeCount:

    push ebp
    mov ebp, esp

    pusha
    push es

    mov bx, FNID_PnpBiosGetDeviceNodeCount
    call i386CallRealMode

    mov esi, [ebp + 8]
    mov ax, [BSS_PnpNodeSize]
    movzx ecx, ax
    mov [esi], ecx

    mov esi, [ebp + 12]
    mov ax, [BSS_PnpNodeCount]
    movzx ecx, ax
    mov [esi], ecx

    pop es
    popa

    mov esp, ebp
    pop ebp

    mov eax, dword ptr [BSS_PnpResult]

    ret


/*
 * U32 PnpBiosGetDeviceNode(U8 *NodeId, U8 *NodeBuffer);
 *
 * RETURNS:
 */
PUBLIC _PnpBiosGetDeviceNode
_PnpBiosGetDeviceNode:

    push ebp
    mov ebp, esp

    pusha
    push es

    /* get current node number */
    mov esi, [ebp + 8]
    mov al, [esi]
    mov byte ptr ds:[BSS_PnpNodeNumber], al

    /* convert pointer to node buffer to segment/offset */
    mov eax, [ebp + 12]
    shr eax, 4
    and eax, HEX(0f000)
    mov word ptr ds:[BSS_PnpBiosBufferSegment], ax
    mov eax, [ebp + 12]
    and eax, HEX(0ffff)
    mov word ptr ds:[BSS_PnpBiosBufferOffset], ax

    mov bx, FNID_PnpBiosGetDeviceNode
    call i386CallRealMode

    /* update node number */
    mov esi, [ebp + 8]
    mov al, byte ptr ds:[BSS_PnpNodeNumber]
    mov [esi], al

    pop es
    popa

    mov esp, ebp
    pop ebp

    mov eax, [BSS_PnpResult]

    ret


/*
 * U32 PnpBiosGetDockStationInformation(U8 *DockingStationInfo);
 *
 * RETURNS:
 */
PUBLIC _PnpBiosGetDockStationInformation
_PnpBiosGetDockStationInformation:

    push ebp
    mov ebp, esp

    pusha
    push es

    /* Convert pointer to info structure to segment/offset */
    mov eax, [ebp + 8]
    shr eax, 4
    and eax, HEX(0f000)
    mov word ptr ds:[BSS_PnpBiosBufferSegment], ax
    mov eax, [ebp + 8]
    and eax, HEX(0ffff)
    mov word ptr ds:[BSS_PnpBiosBufferOffset], ax

    mov bx, FNID_PnpBiosGetDockStationInformation
    call i386CallRealMode

    pop es
    popa

    mov esp, ebp
    pop ebp

    mov eax, [BSS_PnpResult]

    ret

END

/* EOF */
