


#include <asm.inc>

.code64

PUBLIC _clearfp
FUNC _clearfp
    .ENDPROLOG
    fnclex

ENDFUNC

END
