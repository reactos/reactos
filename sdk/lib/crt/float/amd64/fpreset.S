
#include <asm.inc>

.code64

PUBLIC _fpreset
FUNC _fpreset
    .endprolog
    fninit
    ret
ENDFUNC

END
