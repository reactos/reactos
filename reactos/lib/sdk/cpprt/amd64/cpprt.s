#include <asm.inc>

.code

MACRO(DEFINE_ALIAS, alias, orig, type)
EXTERN &orig:&type
ALIAS <&alias> = <&orig>
ENDM

; void __cdecl `eh vector constructor iterator'(void *,unsigned __int64,int,void (__cdecl*)(void *),void (__cdecl*)(void *))
DEFINE_ALIAS ??_L@YAXPEAX_KHP6AX0@Z2@Z, ?MSVCRTEX_eh_vector_constructor_iterator@@YAXPEAX_KHP6AX0@Z2@Z

; void __cdecl `eh vector destructor iterator'(void *,unsigned __int64,int,void (__cdecl*)(void *))
DEFINE_ALIAS ??_M@YAXPEAX_KHP6AX0@Z@Z, ?MSVCRTEX_eh_vector_destructor_iterator@@YAXPEAX_KHP6AX0@Z@Z

; These are the same
DEFINE_ALIAS __CxxFrameHandler3, __CxxFrameHandler

END
