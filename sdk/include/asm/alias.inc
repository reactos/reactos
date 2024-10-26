
#ifdef _M_IX86
#define SYM(name) _##name
#define IMPSYM(name) __imp__##name
#else
#define SYM(name) name
#define IMPSYM(name) __imp_##name
#endif

#if (defined(_M_IX86) || defined(_M_AMD64))

#include <asm.inc>

MACRO(CREATE_ALIAS1, alias, target)
#ifdef _USE_ML
    EXTERN SYM(&target):PROC
    ALIAS <SYM(&alias)> = <SYM(&target)>
#else
    .weakref SYM(&alias), SYM(&target)
#endif
ENDM

MACRO(CREATE_ALIAS2, alias, target)
#ifdef _USE_ML
    EXTERN IMPSYM(&target):PROC
    ALIAS <IMPSYM(&alias)> = <IMPSYM(&target)>
#else
    .weakref IMPSYM(&alias), IMPSYM(&target)
#endif
ENDM

MACRO(CREATE_ALIAS, alias, target)
    CREATE_ALIAS1 &alias, &target
    CREATE_ALIAS2 &alias, &target
ENDM

#elif defined(_M_ARM)

#include <kxarm.h>

    MACRO
    CREATE_ALIAS1   $alias, $target
    IMPORT  SYM($alias), WEAK SYM($target)
    MEND

    MACRO
    CREATE_ALIAS2   $alias, $target
    IMPORT  IMPSYM($alias), WEAK IMPSYM($target)
    MEND

    MACRO
    CREATE_ALIAS   $alias, $target
    CREATE_ALIAS1 $alias, $target
    CREATE_ALIAS2 $alias, $target
    MEND
#else
#error "Unsupported platform."
#endif
