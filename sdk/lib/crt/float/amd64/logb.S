

#include <asm.inc>

.code64

FUNC _logb
    .endprolog

    fld dword ptr [rsp + 8]
    fxtract
    fstp st
    ret
ENDFUNC

END
