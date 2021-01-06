
#include <asm.inc>

.code64

FUNC _fpreset
    .endprolog
    fninit
    ret
ENDFUNC

END
