/***
*_ctype.c - function versions of ctype macros
*
*       Copyright (c) 1989-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This files provides function versions of the character
*       classification and conversion macros in ctype.h.
*
*******************************************************************************/

/***
*ctype - Function versions of ctype macros
*
*Purpose:
*       Function versions of the macros in ctype.h.  In order to define
*       these, we use a trick -- we undefine the macro so we can use the
*       name in the function declaration, then re-include the file so
*       we can use the macro in the definition part.
*
*       Functions defined:
*           isalpha     isupper     islower
*           isdigit     isxdigit    isspace
*           ispunct     isalnum     isprint
*           isgraph     isctrl      __isascii
*           __toascii   __iscsym    __iscsymf
*
*Entry:
*       int c = character to be tested
*Exit:
*       returns non-zero = character is of the requested type
*                  0 = character is NOT of the requested type
*
*Exceptions:
*       None.
*
*******************************************************************************/

#include <cruntime.h>
#include <ctype.h>

int (__cdecl isalpha) (
        int c
        )
{
        return isalpha(c);
}

int (__cdecl isupper) (
        int c
        )
{
        return isupper(c);
}

int (__cdecl islower) (
        int c
        )
{
        return islower(c);
}

int (__cdecl isdigit) (
        int c
        )
{
        return isdigit(c);
}

int (__cdecl isxdigit) (
        int c
        )
{
        return isxdigit(c);
}

int (__cdecl isspace) (
        int c
        )
{
        return isspace(c);
}

int (__cdecl ispunct) (
        int c
        )
{
        return ispunct(c);
}

int (__cdecl isalnum) (
        int c
        )
{
        return isalnum(c);
}

int (__cdecl isprint) (
        int c
        )
{
        return isprint(c);
}

int (__cdecl isgraph) (
        int c
        )
{
        return isgraph(c);
}

int (__cdecl iscntrl) (
        int c
        )
{
        return iscntrl(c);
}

int (__cdecl __isascii) (
        int c
        )
{
        return __isascii(c);
}

int (__cdecl __toascii) (
        int c
        )
{
        return __toascii(c);
}

#ifndef __iscsymf
#define __iscsymf(_c)	(isalpha(_c) || ((_c) == '_'))
#endif
#ifndef __iscsym
#define __iscsym(_c)	(isalnum(_c) || ((_c) == '_'))
#endif

int (__cdecl __iscsymf) (
        int c
        )
{
        return __iscsymf(c);
}

int (__cdecl __iscsym) (
        int c
        )
{
        return __iscsym(c);
}
