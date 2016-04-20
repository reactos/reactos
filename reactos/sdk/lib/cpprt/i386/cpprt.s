#include <asm.inc>

.code

MACRO(DEFINE_ALIAS, alias, orig, type)
EXTERN &orig:&type
ALIAS <&alias> = <&orig>
ENDM

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

; void __stdcall `eh vector constructor iterator'(void *,unsigned int,int,void (__thiscall*)(void *),void (__thiscall*)(void *))
DEFINE_ALIAS ??_L@YGXPAXIHP6EX0@Z1@Z, ?MSVCRTEX_eh_vector_constructor_iterator@@YGXPAXIHP6EX0@Z1@Z

; void __stdcall `eh vector constructor iterator'(void *,unsigned int,unsigned int,void (__thiscall*)(void *),void (__thiscall*)(void *))
DEFINE_ALIAS ??_L@YGXPAXIIP6EX0@Z1@Z, ?MSVCRTEX_eh_vector_constructor_iterator@@YGXPAXIHP6EX0@Z1@Z

; void __stdcall `eh vector destructor iterator'(void *,unsigned int,int,void (__thiscall*)(void *))
DEFINE_ALIAS ??_M@YGXPAXIHP6EX0@Z@Z, ?MSVCRTEX_eh_vector_destructor_iterator@@YGXPAXIHP6EX0@Z@Z

; void __stdcall `eh vector destructor iterator'(void *,unsigned int,unsigned int,void (__thiscall*)(void *))
DEFINE_ALIAS ??_M@YGXPAXIIP6EX0@Z@Z, ?MSVCRTEX_eh_vector_destructor_iterator@@YGXPAXIHP6EX0@Z@Z

; void __cdecl operator delete(void *,unsigned int)
DEFINE_ALIAS ??3@YAXPAXI@Z, ??3@YAXPAX@Z

; void __cdecl operator delete(void *,struct std::nothrow_t const &)
DEFINE_ALIAS ??3@YAXPAXABUnothrow_t@std@@@Z, ??3@YAXPAX@Z

; void __cdecl operator delete[](void *,struct std::nothrow_t const &)
DEFINE_ALIAS ??_V@YAXPAXABUnothrow_t@std@@@Z, ??3@YAXPAX@Z

END
