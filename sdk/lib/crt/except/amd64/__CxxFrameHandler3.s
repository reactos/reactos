
#include <asm.inc>

.code64
.align 4

MACRO(DEFINE_ALIAS, alias, orig)
EXTERN &orig : PROC
ALIAS <&alias> = <&orig>
ENDM

; These are the same
DEFINE_ALIAS __CxxFrameHandler3, __CxxFrameHandler

END
