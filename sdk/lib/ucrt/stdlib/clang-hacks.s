
#include <asm.inc>

#ifdef _M_IX86
.code
#define SYM(name) _##name
#else
.code64
#define SYM(name) name
#endif

MACRO(CREATE_ALIAS, alias, target)
    EXTERN SYM(&target):PROC
    PUBLIC SYM(&alias)
    SYM(&alias):
        jmp SYM(&target)
ENDM

    #undef _lrotl
    CREATE_ALIAS _lrotl, ___lrotl
    #undef _lrotr
    CREATE_ALIAS _lrotr, ___lrotr
    #undef _rotl
    CREATE_ALIAS _rotl, ___rotl
    #undef _rotl64
    CREATE_ALIAS _rotl64, ___rotl64
    #undef _rotr
    CREATE_ALIAS _rotr, ___rotr
    #undef _rotr64
    CREATE_ALIAS _rotr64, ___rotr64

END
