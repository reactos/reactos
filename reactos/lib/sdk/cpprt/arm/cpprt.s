
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

    ; void __cdecl `eh vector destructor iterator'(void *,unsigned int,int,void (__cdecl*)(void *))
    DEFINE_ALIAS ??_M@YAXPAXIHP6AX0@Z@Z, ?MSVCRTEX_eh_vector_destructor_iterator@@YAXPAXIHP6AX0@Z@Z

    ; These are the same
    //DEFINE_ALIAS __CxxFrameHandler3, __CxxFrameHandler

    END
