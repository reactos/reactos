
#include <kxarm.h>

    TEXTAREA

    MACRO
    DEFINE_ALIAS $FuncName, $Target
        LCLS _FuncName
        LCLS _Target
_FuncName SETS "|$FuncName|"
_Target SETS "|$Target|"
        IMPORT $_FuncName, WEAK $_Target
    MEND

    ; void __cdecl `eh vector constructor iterator'(void *,unsigned int,int,void (__cdecl*)(void *),void (__cdecl*)(void *))
    DEFINE_ALIAS ??_L@YAXPAXIHP6AX0@Z1@Z, ?MSVCRTEX_eh_vector_constructor_iterator@@YAXPAXIHP6AX0@Z1@Z

    ; void __cdecl `eh vector constructor iterator'(void *,unsigned int,unsigned int,void (__cdecl*)(void *),void (__cdecl*)(void *))
    DEFINE_ALIAS ??_L@YAXPAXIIP6AX0@Z1@Z, ?MSVCRTEX_eh_vector_constructor_iterator@@YAXPAXIHP6AX0@Z1@Z

    ; void __cdecl `eh vector destructor iterator'(void *,unsigned int,int,void (__cdecl*)(void *))
    DEFINE_ALIAS ??_M@YAXPAXIHP6AX0@Z@Z, ?MSVCRTEX_eh_vector_destructor_iterator@@YAXPAXIHP6AX0@Z@Z

    ; void __cdecl `eh vector destructor iterator'(void *,unsigned int,unsigned int,void (__cdecl*)(void *))
    DEFINE_ALIAS ??_M@YAXPAXIIP6AX0@Z@Z, ?MSVCRTEX_eh_vector_destructor_iterator@@YAXPAXIHP6AX0@Z@Z

    ; These are the same
    //DEFINE_ALIAS __CxxFrameHandler3, __CxxFrameHandler

    ; void __cdecl operator delete(void *,struct std::nothrow_t const &)
    DEFINE_ALIAS ??3@YAXPAXABUnothrow_t@std@@@Z, ??3@YAXPAX@Z

    ; void __cdecl operator delete[](void *,struct std::nothrow_t const &)
    DEFINE_ALIAS ??_V@YAXPAXABUnothrow_t@std@@@Z, ??3@YAXPAX@Z

    ; void __cdecl operator delete(void *,unsigned int)
    DEFINE_ALIAS ??3@YAXPAXI@Z, ??3@YAXPAX@Z

    END
