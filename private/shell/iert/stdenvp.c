/***
*stdenvp.c - standard _setenvp routine
*
*    Copyright (c) 1989-1997, Microsoft Corporation. All rights reserved.
*
*Purpose:
*    This module is called by the C start-up routine to set up "_environ".
*    Its sets up an array of pointers to strings in the environment.
*    The global symbol "_environ" is set to point to this array.
*
*
*******************************************************************************/
#ifdef _MBCS
#undef _MBCS
#endif

#include <cruntime.h>
#include <string.h>
#include <stdlib.h>
#include <internal.h>
#include <rterr.h>
#include <oscalls.h>
#include <tchar.h>

/***
*_setenvp - set up "envp" for C programs
*
*Purpose:
*    Reads the environment and build the envp array for C programs.
*
*Entry:
*    The environment strings occur at _aenvptr.
*    The list of environment strings is terminated by an extra null
*    byte.  Thus two null bytes in a row indicate the end of the
*    last environment string and the end of the environment, resp.
*
*Exit:
*       "environ" points to a null-terminated list of pointers to ASCIZ
*       strings, each of which is of the form "VAR=VALUE".  The strings
*       are copied from the environment area. This array of pointers will
*       be malloc'ed.  The block pointed to by _aenvptr is deallocated.
*
*Uses:
*    Allocates space on the heap for the environment pointers.
*
*Exceptions:
*    If space cannot be allocated, program is terminated.
*
*******************************************************************************/

#ifdef WPRFLAG
void __cdecl _wsetenvp (
#else
void __cdecl _setenvp (
#endif
    void
    )
{
    _TSCHAR *p;
    _TSCHAR **env;            /* _environ ptr traversal pointer */
    int numstrings;         /* number of environment strings */
        int cchars;

    numstrings = 0;

#ifdef WPRFLAG
        p = _wenvptr;
#else
        p = _aenvptr;
#endif

    /*
         * NOTE: starting with single null indicates no environ.
     * Count the number of strings. Skip drive letter settings
         * ("=C:=C:\foo" type) by skipping all environment variables
         * that begin with '=' character.
         */

    while (*p != _T('\0')) {
            /* don't count "=..." type */
            if (*p != _T('='))
            ++numstrings;
        p += _tcslen(p) + 1;
    }

    /* need pointer for each string, plus one null ptr at end */
        if ( (_tenviron = env = (_TSCHAR **)
            malloc((numstrings+1) * sizeof(_TSCHAR *))) == NULL )
        _amsg_exit(_RT_SPACEENV);

    /* copy strings to malloc'd memory and save pointers in _environ */
#ifdef WPRFLAG
        for ( p = _wenvptr ; *p != L'\0' ; p += cchars ) {
#else
        for ( p = _aenvptr ; *p != '\0' ; p += cchars ) {
#endif
            cchars = _tcslen(p) + 1;
            /* don't copy "=..." type */
            if (*p != _T('=')) {
            if ( (*env = (_TSCHAR *)malloc(cchars * sizeof(_TSCHAR))) == NULL )
                _amsg_exit(_RT_SPACEENV);
            _tcscpy(*env, p);
                env++;
            }
    }

#ifdef WPRFLAG
    free(_wenvptr);
    _wenvptr = NULL;
#else
    free(_aenvptr);
    _aenvptr = NULL;
#endif

    /* and a final NULL pointer */
    *env = NULL;
}
