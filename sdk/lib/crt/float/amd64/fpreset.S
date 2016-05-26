
#include <asm.inc>

.code64

FUNC _fpreset
    .endprolog
    fninit
ENDFUNC

END
