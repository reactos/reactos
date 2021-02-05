

#include <asm.inc>

.code64

PUBLIC _logb
FUNC _logb
    .endprolog

    fld dword ptr [rsp + 8]
    fxtract
    fstp st
    ret
ENDFUNC

END
