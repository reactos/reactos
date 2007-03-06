; cpu 486
segment .text use32

extern _check

global _continuePoint
_continuePoint:
 push ss
 push dword 0
 pushfd
 push cs
 push dword _continuePoint
 push ebp

 push eax
 push ecx
 push edx
 push ebx
 push esi
 push edi

 push ds
 push es
 push fs
 push gs

 ; TODO: floating point state
 sub esp, 70h

 ; Debug registers
 sub esp, 18h

 push dword 00010007h

 ; Fill the Esp field
 lea eax, [esp+0CCh]
 lea ecx, [esp+0C4h]
 mov [ecx], eax

 ; Call the function that will compare the current context with the expected one
 cld
 push esp
 call _check

 ; check() must not return
 int 3

; EOF
