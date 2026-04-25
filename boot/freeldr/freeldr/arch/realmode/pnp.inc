
PnpBiosGetDeviceNodeCount:

    mov ax, word ptr [BSS_PnpBiosDataSegment]
    push ax

    push cs
    mov ax, BSS_PnpNodeSize
    push ax

    push cs
    mov ax, BSS_PnpNodeCount
    push ax

    push 0

#ifdef __clang__
    call_far32 BSS_PnpBiosEntryPoint
#else
    call dword ptr ds:[BSS_PnpBiosEntryPoint]
#endif
    add sp, 12

    movzx ecx, ax
    mov dword ptr [BSS_PnpResult], ecx

    ret


PnpBiosGetDeviceNode:

    /* push bios segment */
    mov ax, word ptr [BSS_PnpBiosDataSegment]
    push ax

    /* push control flag */
    push 1

    /* push pointer to node buffer (segment/offset) */
    mov ax, word ptr [BSS_PnpBiosBufferSegment]
    push ax
    mov ax, word ptr [BSS_PnpBiosBufferOffset]
    push ax

    /* push pointer to node number (segment/offset) */
    push cs
    mov ax, BSS_PnpNodeNumber
    push ax

    /* push function number */
    push 1

    /* call entry point */
#ifdef __clang__
    call_far32 BSS_PnpBiosEntryPoint
#else
    call dword ptr ds:[BSS_PnpBiosEntryPoint]
#endif
    add sp, 14

    movzx ecx, ax
    mov dword ptr [BSS_PnpResult], ecx

    ret


PnpBiosGetDockStationInformation:

    /* push bios segment */
    mov ax, word ptr [BSS_PnpBiosDataSegment]
    push ax

    /* push pointer to info structure (segment/offset) */
    mov ax, word ptr [BSS_PnpBiosBufferSegment]
    push ax
    mov ax, word ptr [BSS_PnpBiosBufferOffset]
    push ax

    /* push function number */
    push 5

    /* call entry point */
#ifdef __clang__
    call_far32 BSS_PnpBiosEntryPoint
#else
    call dword ptr ds:[BSS_PnpBiosEntryPoint]
#endif
    add sp, 8

    movzx ecx, ax
    mov dword ptr [BSS_PnpResult], ecx

    ret
