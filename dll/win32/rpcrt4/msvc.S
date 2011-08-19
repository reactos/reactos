
#include <asm.inc>

.code32

EXTERN _ndr_client_call:PROC

PUBLIC _call_stubless_func
_call_stubless_func:

    mov ecx,[esp+4]        /* This Pointer */
    mov ecx,[ecx]          /* This->lpVtbl */
    mov ecx,[ecx-8]        /* MIDL_STUBLESS_PROXY_INFO */
    mov edx,[ecx+8]        /* Info->FormatStringOffset */
    mov edx,[edx+eax*2]    /* FormatStringOffset[index] */
    and edx, 0000FFFFh
    add edx,[ecx+4]        /* info->ProcFormatString + offset */
    mov eax, [edx+8]       /* arguments size */
    and eax, 0000FFFFh
    push eax
    lea eax, [esp+8]       /* &This */
    push eax
    push edx               /* format string */
    push [ecx]             /* info->pstubdesc */
    call _ndr_client_call
    lea esp, [esp+12]
    pop edx                /* arguments size */
    mov ecx, [esp]         /* return address */
    add esp, edx
    jmp ecx

END