cpu 486
segment .text use32

global __SEHCleanHandlerEnvironment
__SEHCleanHandlerEnvironment:
 cld
 ret

global __SEHRegisterFrame
__SEHRegisterFrame:
 mov ecx, [esp+4]
 mov eax, [fs:0]
 mov [ecx+0], eax
 mov [fs:0], ecx
 ret

global __SEHUnregisterFrame
__SEHUnregisterFrame:
 mov ecx, [esp+4]
 mov ecx, [ecx]
 mov [fs:0], ecx
 ret

global __SEHUnwind
__SEHUnwind:

 extern RtlUnwind

 mov ecx, [esp+4]

; RtlUnwind clobbers all the "don't clobber" registers, so we save them
 push esi
 push edi
 push ebx

 push eax
 push eax
 push eax
 push ecx
 call RtlUnwind

 pop ebx
 pop edi
 pop esi

 ret

; EOF
