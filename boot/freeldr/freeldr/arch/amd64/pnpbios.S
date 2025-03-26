/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PnP BIOS helper functions
 * COPYRIGHT:   Copyright 2003 Eric Kohl <eric.kohl@reactos.org>
 *              Copyright 2011 Timo Kreuzer <timo.kreuzer@reactos.org>
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

    /* Save registers */
    push rbx
    push rcx
    push rdx

    /* Call the real mode function */
    mov bx, FNID_PnpBiosGetDeviceNodeCount
    call CallRealMode

    /* Restore param-regs */
    pop rdx
    pop rcx
    pop rbx

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

    /* Save registers */
    push rcx
    push rbx

    /* Call the real mode function */
    mov bx, FNID_PnpBiosGetDeviceNode
    call CallRealMode

    /* Restore registers */
    pop rbx
    pop rcx

    /* update node number */
    mov al, byte ptr [BSS_PnpNodeNumber]
    mov [rcx], al

    mov eax, [BSS_PnpResult]

    ret


/*
 * U32 PnpBiosGetDockStationInformation(U8 *DockingStationInfo<rcx>);
 *
 * RETURNS:
 */
PUBLIC PnpBiosGetDockStationInformation
PnpBiosGetDockStationInformation:

    /* Convert pointer to info structure to segment/offset */
    mov rax, rcx
    shr eax, 4
    mov word ptr [BSS_PnpBiosBufferSegment], ax
    mov rax, rcx
    and eax, HEX(0f)
    mov word ptr [BSS_PnpBiosBufferOffset], ax

    /* Save registers */
    push rcx
    push rbx

    /* Call the real mode function */
    mov bx, FNID_PnpBiosGetDockStationInformation
    call CallRealMode

    /* Restore registers */
    pop rbx
    pop rcx

    mov eax, dword ptr [BSS_PnpResult]

    ret

END
/* EOF */
