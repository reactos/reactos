cpu 486
segment .text use32

extern _KeRosContinue@8
extern KeSystemCallReturn
extern KeSystemCallReturn2

global _NtContinue@8
_NtContinue@8:
 ; Make KeRosContinue "return forwards" to our .ret label. This will take care
 ; of resetting the stack to the address KeSystemCallReturn[2] expects
 mov [esp], dword .ret

 ; Call the real function (see ps\thread.c)
 jmp _KeRosContinue@8

.ret
 ; Test the return value
 cmp eax, 0

 ; Success: return without overwriting EAX with the return value
 jge KeSystemCallReturn2

 ; Failure: normal return
 jmp KeSystemCallReturn

; EOF
