#include <kxarm64.h>

    TEXTAREA

    MACRO
    DEFINE_ALIAS $FuncName, $Target
        LCLS _FuncName
        LCLS _Target
_FuncName SETS "|$FuncName|"
_Target SETS "|$Target|"
        IMPORT $_FuncName, WEAK $_Target
    MEND

    DEFINE_ALIAS ??3@YAXPEAXAEBUnothrow_t@std@@@Z, ??3@YAXPEAX@Z
    DEFINE_ALIAS ??_V@YAXPEAXAEBUnothrow_t@std@@@Z, ??3@YAXPEAX@Z
    DEFINE_ALIAS ??3@YAXPEAX_K@Z, ??3@YAXPEAX@Z

    END
