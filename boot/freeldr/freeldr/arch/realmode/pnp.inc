
PnpBiosGetDeviceNodeCount:

    mov ax, word ptr ds:[BSS_PnpBiosDataSegment]
    push ax

    push cs
    mov ax, BSS_PnpNodeSize
    push ax

    push cs
    mov ax, BSS_PnpNodeCount
    push ax

    push 0

    call dword ptr ds:[BSS_PnpBiosEntryPoint]
    add sp, 12

    movzx ecx, ax
    mov dword ptr ds:[BSS_PnpResult], ecx

    ret


PnpBiosGetDeviceNode:

    /* push bios segment */
    mov ax, word ptr ds:[BSS_PnpBiosDataSegment]
    push ax

    /* push control flag */
    push 1

    /* push pointer to node buffer (segment/offset) */
    mov ax, word ptr ds:[BSS_PnpBiosBufferSegment]
    push ax
    mov ax, word ptr ds:[BSS_PnpBiosBufferOffset]
    push ax

    /* push pointer to node number (segment/offset) */
    push cs
    mov ax, BSS_PnpNodeNumber
    push ax

    /* push function number */
    push 1

    /* call entry point */
    call dword ptr ds:[BSS_PnpBiosEntryPoint]
    add sp, 14

    movzx ecx, ax
    mov dword ptr ds:[BSS_PnpResult], ecx

    ret
