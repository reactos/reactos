

#include <asm.inc>

.code
.align 4

EXTERN _CxxHandleV8Frame@20 : PROC
PUBLIC ___CxxFrameHandler3
___CxxFrameHandler3:
    push eax
    push dword ptr [esp + 20]
    push dword ptr [esp + 20]
    push dword ptr [esp + 20]
    push dword ptr [esp + 20]
    call _CxxHandleV8Frame@20
    ret

EXTERN ___CxxFrameHandler : PROC
PUBLIC _CallCxxFrameHandler
_CallCxxFrameHandler:
    mov eax, dword ptr [esp + 20]
    jmp ___CxxFrameHandler

END

