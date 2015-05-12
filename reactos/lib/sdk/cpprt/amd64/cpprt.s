#include <asm.inc>

.code

MACRO(DEFINE_ALIAS, alias, orig, type)
EXTERN &orig:&type
ALIAS <&alias> = <&orig>
ENDM

; void __cdecl `eh vector constructor iterator'(void * __ptr64,unsigned __int64,int,void (__cdecl*)(void * __ptr64),void (__cdecl*)(void * __ptr64))
DEFINE_ALIAS ??_L@YAXPEAX_KHP6AX0@Z2@Z, ?MSVCRTEX_eh_vector_constructor_iterator@@YAXPEAX_KHP6AX0@Z2@Z

; void __cdecl `eh vector destructor iterator'(void * __ptr64,unsigned __int64,int,void (__cdecl*)(void * __ptr64))
DEFINE_ALIAS ??_M@YAXPEAX_KHP6AX0@Z@Z, ?MSVCRTEX_eh_vector_destructor_iterator@@YAXPEAX_KHP6AX0@Z@Z

; These are the same
DEFINE_ALIAS __CxxFrameHandler3, __CxxFrameHandler

; void __cdecl operator delete(void * __ptr64,struct std::nothrow_t const & __ptr64)
DEFINE_ALIAS ??3@YAXPEAXAEBUnothrow_t@std@@@Z, ??3@YAXPEAX@Z

; void __cdecl operator delete[](void * __ptr64,struct std::nothrow_t const & __ptr64)
DEFINE_ALIAS ??_V@YAXPEAXAEBUnothrow_t@std@@@Z, ??3@YAXPEAX@Z

END
