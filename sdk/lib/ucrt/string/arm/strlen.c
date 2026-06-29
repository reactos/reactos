/***
*strlen.c - contains strlen() routine
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       strlen returns the length of a null-terminated string,
*       not including the null byte itself.
*
*******************************************************************************/

#include <string.h>

#pragma function(strlen)

/***
*strlen - return the length of a null-terminated string
*
*Purpose:
*       Finds the length in bytes of the given string, not including
*       the final null character.
*
*Entry:
*       const char * str - string whose length is to be computed
*
*Exit:
*       length of the string "str", exclusive of the final null byte
*
*Exceptions:
*
*******************************************************************************/

size_t __cdecl strlen (
        const char * str
        )
{
        const char *eos = str;

        while( *eos++ ) ;

        return( eos - str - 1 );
}
