/***
*crt0dat.c - 32-bit C run-time initialization/termination routines
*
*       Copyright (c) 1986-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This module contains the routines _cinit, exit, and _exit
*       for C run-time startup and termination.
*
*       This file is a modified version for use with Forms3 that removes most
*       of the C run-time startup code for improved performance.
*
*******************************************************************************/

typedef void (__cdecl *_PVFV)(void);
#include "windows.h"
#include "excpt.h"

// The compiler generates references to this variable when you use floating
// point, so we need it here.
int _fltused = 0x9875;
int __fastflag = 0;
int _adjust_fdiv = 0;
int _ldused = 0x9873;
int _iob;
char _osfile[20];
int errno;

CRITICAL_SECTION s_cs;
int errno1 = 0;
int __error_mode;   // error_mode and _apptype, together, determine how error messages are written out.
int __app_type;


#if DBG == 1
void lock_cleanup(void);
#endif

void BreakHere();

int * _cdecl _errno(void)
{
    return &errno1;
}

#define errno (*_errno())

extern int      _cpvfv;
extern _PVFV *  _apvfv;

void __cdecl _cinit (void);
void __cdecl _cexit (void);

//
// common globals used by console and gui exe support.
//

//command line, environment, and a few other globals
char *_acmdln;              /* points to command line */
char *_aenvptr;             /* points to environment block */
wchar_t *_wenvptr;          /* points to wide environment block */

int _umaskval = 0;  // define umask 

/* define version info variables */
_CRTIMP unsigned int _osver = 0;
_CRTIMP unsigned int _winver = 0;
_CRTIMP unsigned int _winmajor = 0;
_CRTIMP unsigned int _winminor = 0;

/* argument vector and environment */
_CRTIMP int __argc = 0;
_CRTIMP char **__argv = NULL;
_CRTIMP wchar_t **__wargv = NULL;
_CRTIMP char **_environ = NULL;
_CRTIMP char **__initenv = NULL;
_CRTIMP wchar_t **_wenviron = NULL;
_CRTIMP wchar_t **__winitenv = NULL;
_CRTIMP char *_pgmptr = NULL;           /* ptr to program name */
_CRTIMP wchar_t *_wpgmptr = NULL;       /* ptr to wide program name */

char _exitflag = 0; // callable exit flag 
//
// end common globals used by console and gui exe support.
//

/*
 * flag indicating if C runtime termination has been done. set if exit,
 * _exit, _cexit or _c_exit has been called. checked when _CRTDLL_INIT
 * is called with DLL_PROCESS_DETACH.
 */
int _C_Termination_Done = FALSE;
int _C_Exit_Done = FALSE;

/*
 * NOTE: THE USE OF THE POINTERS DECLARED BELOW DEPENDS ON THE PROPERTIES
 * OF C COMMUNAL VARIABLES. SPECIFICALLY, THEY ARE NON-NULL IFF THERE EXISTS
 * A DEFINITION ELSEWHERE INITIALIZING THEM TO NON-NULL VALUES.
 */


/*
 * pointers to initialization sections
 */

#if defined(_M_IA64)

#pragma section(".CRT$XIA",long,read)
#pragma section(".CRT$XIZ",long,read)
#pragma section(".CRT$XCA",long,read)
#pragma section(".CRT$XCZ",long,read)
#pragma section(".CRT$XPA",long,read)
#pragma section(".CRT$XPZ",long,read)
#pragma section(".CRT$XTA",long,read)
#pragma section(".CRT$XTZ",long,read)

__declspec(allocate(".CRT$XIA")) _PVFV __xi_a[] = { NULL };
__declspec(allocate(".CRT$XIZ")) _PVFV __xi_z[] = { NULL };
__declspec(allocate(".CRT$XCA")) _PVFV __xc_a[] = { NULL };
__declspec(allocate(".CRT$XCZ")) _PVFV __xc_z[] = { NULL };
__declspec(allocate(".CRT$XPA")) _PVFV __xp_a[] = { NULL };
__declspec(allocate(".CRT$XPZ")) _PVFV __xp_z[] = { NULL };
__declspec(allocate(".CRT$XTA")) _PVFV __xt_a[] = { NULL };
__declspec(allocate(".CRT$XTZ")) _PVFV __xt_z[] = { NULL };

#pragma comment(linker, "/merge:.CRT=.rdata")

#else

#pragma data_seg(".CRT$XIA")
_PVFV __xi_a[] = { NULL };

#pragma data_seg(".CRT$XIZ")
_PVFV __xi_z[] = { NULL };

#pragma data_seg(".CRT$XCA")
_PVFV __xc_a[] = { NULL };

#pragma data_seg(".CRT$XCZ")
_PVFV __xc_z[] = { NULL };

#pragma data_seg(".CRT$XPA")
_PVFV __xp_a[] = { NULL };

#pragma data_seg(".CRT$XPZ")
_PVFV __xp_z[] = { NULL };

#pragma data_seg(".CRT$XTA")
_PVFV __xt_a[] = { NULL };

#pragma data_seg(".CRT$XTZ")
_PVFV __xt_z[] = { NULL };

#pragma data_seg()  /* reset */

#pragma comment(linker, "-merge:.CRT=.data")
#endif

/*
 * static (internal) function that walks a table of function pointers,
 * calling each entry between the two pointers, skipping NULL entries
 *
 * Needs to be exported for CRT DLL so that C++ initializers in the
 * client EXE / DLLs can be initialized
 */
static void __cdecl _initterm(_PVFV *, _PVFV *);


/***
*_cinit - C initialization -- modified Forms3 version
*
*Purpose:
*       Calls C and C++ initializer routines
*
*Entry:
*       No parameters: Called from __crtstart and assumes data
*       set up correctly there.
*
*Exit:
*       Initializes C runtime data.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _cinit (
        void
        )
{
        /*
         * do initializations
         */
        _initterm( __xi_a, __xi_z );

        /*
         * do C++ initializations
         */
        _initterm( __xc_a, __xc_z );
}


/***
*exit(status), _exit(status), _cexit(void), _c_exit(void) - C termination
*
*Purpose:
*
*       Entry points:
*
*           exit(code):  Performs all the C termination functions
*               and terminates the process with the return code
*               supplied by the user.
*
*           _exit(code):  Performs a quick exit routine that does not
*               do certain 'high-level' exit processing.  The _exit
*               routine terminates the process with the return code
*               supplied by the user.
*
*           _cexit():  Performs the same C lib termination processing
*               as exit(code) but returns control to the caller
*               when done (i.e., does NOT terminate the process).
*
*           _c_exit():  Performs the same C lib termination processing
*               as _exit(code) but returns control to the caller
*               when done (i.e., does NOT terminate the process).
*
*       Termination actions:
*
*           exit(), _cexit():
*
*           1.  call user's terminator routines
*           2.  call C runtime preterminators
*
*           _exit(), _c_exit():
*
*           3.  call C runtime terminators
*           4.  return to DOS or caller
*
*       Notes:
*
*       The termination sequence is complicated due to the multiple entry
*       points sharing the common code body while having different entry/exit
*       sequences.
*
*       Multi-thread notes:
*
*       1. exit() should NEVER be called when mthread locks are held.
*          The exit() routine can make calls that try to get mthread locks.
*
*       2. _exit()/_c_exit() can be called from anywhere, with or without locks held.
*          Thus, _exit() can NEVER try to get locks (otherwise, deadlock
*          may occur).  _exit() should always 'work' (i.e., the process
*          should always terminate successfully).
*
*       3. Only one thread is allowed into the exit code (see _lockexit()
*          and _unlockexit() routines).
*
*Entry:
*       exit(), _exit()
*           int status - exit status (0-255)
*
*       _cexit(), _c_exit()
*           <no input>
*
*Exit:
*       exit(), _exit()
*           <EXIT to DOS>
*
*       _cexit(), _c_exit()
*           Return to caller
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

/* worker routine prototype */
static void __cdecl doexit (int code, int quick, int retcaller);

void __cdecl exit (
        int code
        )
{
        doexit(code, 0, 0); /* full term, kill process */
}


void __cdecl _exit (
        int code
        )
{
        doexit(code, 1, 0); /* quick term, kill process */
}

void __cdecl _cexit (
        void
        )
{
        doexit(0, 0, 1);    /* full term, return to caller */
}

void __cdecl _c_exit (
        void
        )
{
        doexit(0, 1, 1);    /* quick term, return to caller */
}

void (__cdecl * _aexit_rtn)(int) = _exit;   /* RT message return procedure */

static void __cdecl doexit (
        int code,
        int quick,
        int retcaller
        )
{
    EnterCriticalSection(&s_cs);

    if (_C_Exit_Done == TRUE)                               /* if doexit() is being called recursively */
            TerminateProcess(GetCurrentProcess(),code);     /* terminate with extreme prejudice */

    _C_Termination_Done = TRUE;

    /* save callable exit flag (for use by terminators) */

    if (!quick) 
    {
        /*
         * do atexit() terminators
         * (if there are any)
         *
         * These terminators MUST be executed in reverse order (LIFO)!
         *
         */
        if (_cpvfv > 0)
        {
            _PVFV *ppfunc;
            int    i;

            for (i=_cpvfv, ppfunc=_apvfv + i - 1; i > 0; i--, ppfunc--)
            {
                if (*ppfunc)
                   (*ppfunc)();
            }

            LocalFree(_apvfv);
            _cpvfv = 0;
        }
        /* do pre-terminators */
        _initterm(__xp_a, __xp_z);
    }

    /* do terminators */
    _initterm(__xt_a, __xt_z);

    /* return to OS or to caller */
    if (retcaller) 
    {
        LeaveCriticalSection(&s_cs);
        return;
    }

    _C_Exit_Done = TRUE;
    ExitProcess(code);
}


/***
* static void _initterm(_PVFV * pfbegin, _PVFV * pfend) - call entries in
*       function pointer table
*
*Purpose:
*       Walk a table of function pointers, calling each entry, as follows:
*
*           1. walk from beginning to end, pfunctbl is assumed to point
*              to the beginning of the table, which is currently a null entry,
*              as is the end entry.
*           2. skip NULL entries
*           3. stop walking when the end of the table is encountered
*
*Entry:
*       _PVFV *pfbegin  - pointer to the beginning of the table (first valid entry).
*       _PVFV *pfend    - pointer to the end of the table (after last valid entry).
*
*Exit:
*       No return value
*
*Notes:
*       This routine must be exported in the CRT DLL model so that the client
*       EXE and client DLL(s) can call it to initialize their C++ constructors.
*
*Exceptions:
*       If either pfbegin or pfend is NULL, or invalid, all bets are off!
*
*******************************************************************************/

static void __cdecl _initterm (
        _PVFV * pfbegin,
        _PVFV * pfend
        )
{
        /*
         * walk the table of function pointers from the bottom up, until
         * the end is encountered.  Do not skip the first entry.  The initial
         * value of pfbegin points to the first valid entry.  Do not try to
         * execute what pfend points to.  Only entries before pfend are valid.
         */
        while ( pfbegin < pfend )
        {
            /*
             * if current table entry is non-NULL, call thru it.
             */
            if ( *pfbegin != NULL )
                (**pfbegin)();
            ++pfbegin;
        }
}

void __cdecl terminate()
{
    BreakHere();
    ExitProcess(1);
}

void __cdecl _amsg_exit( int ret ) 
{
    BreakHere();
    exit(ret); 
}
