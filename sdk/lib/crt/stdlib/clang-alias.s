#include <asm.inc>

.code

MACRO(DEFINE_ALIAS, alias, orig)
EXTERN &orig : PROC
ALIAS <&alias> = <&orig>
ENDM

#ifdef _M_X64
DEFINE_ALIAS _rotl, __function_rotl
DEFINE_ALIAS _rotr, __function_rotr
DEFINE_ALIAS _lrotl, __function_lrotl
DEFINE_ALIAS _lrotr, __function_lrotr
#else
DEFINE_ALIAS __rotl, ___function_rotl
DEFINE_ALIAS __rotr, ___function_rotr
DEFINE_ALIAS __lrotl, ___function_lrotl
DEFINE_ALIAS __lrotr, ___function_lrotr
#endif

END
