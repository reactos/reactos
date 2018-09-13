/***
*wild.c - wildcard expander
*
*    Copyright (c) 1985-1997, Microsoft Corporation. All rights reserved.
*
*Purpose:
*     expands wildcards in argv
*
*     handles '*' (none or more of any char) and '?' (exactly one char)
*
*Revision History:
*    05-21-84  RN    initial version
*    06-07-85  TDC    since dos accepts forward slash, added
*            code to accept forward slash in a manner consistent
*            with reverse slash.
*    09-20-86  SKS    Modified for OS/2
*            All argument strings to this function have a
*            leading flag character. If the flag is a quote,
*            that argument string was quoted on the command
*            line and should have not wildcard expansion.  In all
*            cases the leading flag character is removed from
*            the string.
*    11-11-86  JMB    Added Kanji support under KANJI switch.
*    09-21-88  WAJ    initial 386 version
*    04-09-90  GJF    Added #include <cruntime.h> and removed #include
*            <register.h>. Made calling types explicit (_CALLTYPE1
*            or _CALLTYPE4). Also, fixed the copyright.
*    04-10-90  GJF    Added #include <internal.h> and fixed compiler warnings
*            (-W3).
*    07-03-90  SBM   Compiles cleanly with -W3 under KANJI, removed
*            redundant includes, removed #include <internal.h>
*            to keep wild.c free of private stuff, should we
*            decide to release it
*    09-07-90  SBM   put #include <internal.h> back in, reason for
*            removing it discovered to be horribly bogus
*    10-08-90  GJF    New-style function declarators.
*    01-18-91  GJF    ANSI naming.
*    04-06-93  SKS    Replace _CRTAPI* with __cdecl
*            Remove explicit declarations of __argc & __argv.
*            They are declared in <stdlib.h>
*    05-05-93  SKS    Filename sorting should be case-insensitive
*    06-09-93  KRS    Update _MBCS support.
*    10-20-93  GJF    Merged in NT version.
*    11-23-93  CFW    Wide char enable, grab _find from stdargv.c.
*    12-07-93  CFW    Change _TCHAR to _TSCHAR.
*    04-22-94  GJF    Made defintions of arghead, argend, _WildFindHandle
*            and findbuf conditional on DLL_FOR_WIN32S.
*    01-10-95  CFW    Debug CRT allocs.
*    01-18-95  GJF    Must replace _tcsdup with _malloc_crt/_tcscpy for
*            _DEBUG build.
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <msdos.h>
#include <internal.h>
#include <tchar.h>

#ifdef _MBCS
#include <mbdata.h>
#include <mbstring.h>
#endif
//#include <dbgint.h>

/*
** these are the data structures
**
**     __argv
**     -------       ------
**     |     |---->|    |---->"arg0"
**     -------       ------
**           |    |---->"arg1"
**           ------
**            ....
**           ------
**           |    |---->"argn"
**           ------
**           |NULL|
**           ------
**                     argend
**                     -------
**     -------                 |     |
**     |     | __argc             -------
**     -------                    |
**                        |
**  arghead                    V
**  ------     ---------        ----------
**  |     |---->|   |   |----> .... ---->|   |NULL|
**  ------     ---------        ----------
**         |              |
**         V              V
**          "narg0"               "nargn"
*/

/* local function tchars */
#ifdef WPRFLAG
#define tmatch  wmatch
#define tadd    wadd
#define tsort   wsort
#define tfind   wfind
#else
#define tmatch  match
#define tadd    add
#define tsort   sort
#define tfind   find
#endif

#define SLASHCHAR       _T('\\')
#define FWDSLASHCHAR    _T('/')
#define COLONCHAR       _T(':')
#define QUOTECHAR       _T('"')

#define SLASH           _T("\\")
#define FWDSLASH        _T("/")
#define STAR            _T("*.*")
#define DOT             _T(".")
#define DOTDOT          _T("..")

#define WILDSTRING      _T("*?")

#ifdef    DLL_FOR_WIN32S

#define arghead     (_GetPPD()->_ppd_arghead)
#define argend        (_GetPPD()->_ppd_argend)

#else    /* ndef DLL_FOR_WIN32S */

struct argnode {
    _TSCHAR *argptr;
    struct argnode *nextnode;
};

static struct argnode *arghead;
static struct argnode *argend;

#endif    /* DLL_FOR_WIN32S */

#ifdef WPRFLAG
static int __cdecl wmatch(wchar_t *, wchar_t *);
static int __cdecl wadd(wchar_t *);
static void __cdecl wsort(struct argnode *);
static wchar_t * __cdecl wfind (wchar_t *pattern);
#else
static int __cdecl match(char *, char *);
static int __cdecl add(char *);
static void __cdecl sort(struct argnode *);
static char * __cdecl find (char *pattern);
#endif

/***
*int _cwild() - wildcard expander
*
*Purpose:
*    expands wildcard in file specs in argv
*
*    handles '*' (none or more of any char), '?' (exactly one char), and
*    '[string]' (chars which match string chars or between n1 and n2
*    if 'n1-n2' in string inclusive)
*
*Entry:
*
*Exit:
*    returns 0 if successful, -1 if any malloc() calls fail
*    if problems with malloc, the old argc and argv are not touched
*
*Exceptions:
*
*******************************************************************************/

#ifdef WPRFLAG
int __cdecl _wcwild (
#else
int __cdecl _cwild (
#endif
    void
    )
{
#ifdef WPRFLAG
    REG1 wchar_t **argv = __wargv;
#else
    REG1 char **argv = __argv;
#endif
    REG2 struct argnode *nodeptr;
    REG3 int argc;
    REG4 _TSCHAR **tmp;
    _TSCHAR *wchar;

    arghead = argend = NULL;

#ifdef WPRFLAG
    for (argv = __wargv; *argv; argv++)    /* for each arg... */
#else
    for (argv = __argv; *argv; argv++)    /* for each arg... */
#endif
    if ( *(*argv)++ == QUOTECHAR )
        /* strip leading quote from quoted arg */
    {
        if (tadd(*argv))
        return(-1);
    }
    else if (wchar = _tcspbrk( *argv, WILDSTRING )) {
        /* attempt to expand arg with wildcard */
        if (tmatch( *argv, wchar ))
        return(-1);
    }
    else if (tadd( *argv ))    /* normal arg, just add */
        return(-1);

    /* count the args */
    for (argc = 0, nodeptr = arghead; nodeptr;
        nodeptr = nodeptr->nextnode, argc++)
        ;

    /* try to get new arg vector */
    if (!(tmp = (_TSCHAR **)    (sizeof(_TSCHAR *)*(argc+1))))
    return(-1);

    /* the new arg vector... */
#ifdef WPRFLAG
    __wargv = tmp;
#else
    __argv = tmp;
#endif

    /* the new arg count... */
    __argc = argc;

    /* install the new args */
    for (nodeptr = arghead; nodeptr; nodeptr = nodeptr->nextnode)
    *tmp++ = nodeptr->argptr;

    /* the terminal NULL */
    *tmp = NULL;

    /* free up local data */
    for (nodeptr = arghead; nodeptr; nodeptr = arghead) {
    arghead = arghead->nextnode;
    free(nodeptr);
    }

    /* return success */
    return(0);
}


/***
*match(arg, ptr) - [STATIC]
*
*Purpose:
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

#ifdef WPRFLAG
static int __cdecl wmatch (
#else
static int __cdecl match (
#endif
    REG4 _TSCHAR *arg,
    REG1 _TSCHAR *ptr
    )
{
    REG2 _TSCHAR *new;
    REG3 int length = 0;
    _TSCHAR *all;
    REG5 struct argnode *first;
    REG6 int gotone = 0;

    while (ptr != arg && *ptr != SLASHCHAR && *ptr != FWDSLASHCHAR
    && *ptr != COLONCHAR) {
    /* find first slash or ':' before wildcard */
#ifdef _MBCS
    if (--ptr > arg)
        ptr = _mbsdec(arg,ptr+1);
#else
    ptr--;
#endif
    }

    if (*ptr == COLONCHAR && ptr != arg+1) /* weird name, just add it as is */
    return(tadd(arg));

    if (*ptr == SLASHCHAR || *ptr == FWDSLASHCHAR
    || *ptr == COLONCHAR) /* pathname */
    length = (int)(ptr - arg + 1); /* length of dir prefix */

    if (new = tfind(arg)) { /* get the first file name */
    first = argend;

    do  { /* got a file name */
        if (_tcscmp(new, DOT) && _tcscmp(new, DOTDOT)) {
        if (*ptr != SLASHCHAR && *ptr != COLONCHAR
            && *ptr != FWDSLASHCHAR ) {
            /* current directory; don't need path */
#ifdef    _DEBUG
            if (!(arg=malloc((_tcslen(new)+1)*sizeof(_TSCHAR)))
            || tadd(_tcscpy(arg,new)))
#else    /* ndef _DEBUG */
            if (!(arg = _tcsdup(new)) || tadd(arg))
#endif    /* _DEBUG */
            return(-1);
        }
        else    /* add full pathname */
            if (!(all=malloc((length+_tcslen(new)+1)*sizeof(_TSCHAR)))
            || tadd(_tcscpy(_tcsncpy(all,arg,length)+length,new)
            - length))
            return(-1);

        gotone++;
        }

    }
    while (new = tfind(NULL));  /* get following files */

    if (gotone) {
        tsort(first ? first->nextnode : arghead);
        return(0);
    }
    }

    return(tadd(arg)); /* no match */
}

/***
*add(arg) - [STATIC]
*
*Purpose:
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

#ifdef WPRFLAG
static int __cdecl wadd (
#else
static int __cdecl add (
#endif
    _TSCHAR *arg
    )
{
    REG1 struct argnode *nodeptr;

    if (!(nodeptr = (struct argnode *)malloc(sizeof(struct argnode))))
    return(-1);

    nodeptr->argptr = arg;
    nodeptr->nextnode = NULL;

    if (arghead)
    argend->nextnode = nodeptr;
    else
    arghead = nodeptr;

    argend = nodeptr;
    return(0);
}


/***
*sort(first) - [STATIC]
*
*Purpose:
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

#ifdef WPRFLAG
static void __cdecl wsort (
#else
static void __cdecl sort (
#endif
    REG2 struct argnode *first
    )
{
    REG1 struct argnode *nodeptr;
    REG3 _TSCHAR *temp;

    if (first) /* something to sort */
    while (nodeptr = first->nextnode) {
        do    {
#ifdef _POSIX_
                if (_tcscmp(nodeptr->argptr, first->argptr) < 0) {
#else
                if (_tcsicmp(nodeptr->argptr, first->argptr) < 0) {
#endif /* _POSIX_ */
                temp = first->argptr;
                first->argptr = nodeptr->argptr;
            nodeptr->argptr = temp;
        }
        }
        while (nodeptr = nodeptr->nextnode);

        first = first->nextnode;
    }
}


/***
*find(pattern) - find matching filename
*
*Purpose:
*    if argument is non-null, do a DOSFINDFIRST on that pattern
*    otherwise do a DOSFINDNEXT call.  Return matching filename
*    or NULL if no more matches.
*
*Entry:
*    pattern = pointer to pattern or NULL
*        (NULL means find next matching filename)
*
*Exit:
*    returns pointer to matching file name
*        or NULL if no more matches.
*
*Exceptions:
*
*******************************************************************************/

#ifdef WPRFLAG
static wchar_t * __cdecl wfind (
#else
static char * __cdecl find (
#endif /* WPRFLAG */
    _TSCHAR *pattern
    )
{
    _TSCHAR *retval;

#ifdef    DLL_FOR_WIN32S
    #define _WildFindHandle (_GetPPD()->_ppd__WildFindHandle)
    #define findbuf     ((WIN32_FIND_DATA *)(_GetPPD()->_ppd_findbuf))
#else    /* ndef DLL_FOR_WIN32S */
        static HANDLE _WildFindHandle;
    static LPWIN32_FIND_DATA findbuf;
#endif    /* DLL_FOR_WIN32S */

        if (pattern) {
            if (findbuf == NULL)
                findbuf = (LPWIN32_FIND_DATA)malloc(MAX_PATH + sizeof(*findbuf));

            if (_WildFindHandle != NULL) {
                (void)FindClose( _WildFindHandle );
                _WildFindHandle = NULL;
            }

            _WildFindHandle = FindFirstFile( (LPTSTR)pattern, findbuf );
            if (_WildFindHandle == INVALID_HANDLE_VALUE)
                return NULL;
        }
        else if (!FindNextFile( _WildFindHandle, findbuf )) {
            (void)FindClose( _WildFindHandle );
            _WildFindHandle = NULL;
            return NULL;
        }

        retval = findbuf->cFileName;

        return retval;
}
