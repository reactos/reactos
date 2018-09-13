/***
*crt0dat.c - 32-bit C run-time initialization/termination routines
*
* Copyright (c) 1986 - 1999 Microsoft Corporation. All rights reserved.*
*Purpose:
*       This module contains the routines _cinit, exit, and _exit
*       for C run-time startup and termination.  _cinit and exit
*       are called from the _astart code in crt0.asm.
*       This module also defines several data variables used by the
*       runtime.
*
*       [NOTE: Lock segment definitions are at end of module.]
*
*       *** FLOATING POINT INITIALIZATION AND TERMINATION ARE NOT ***
*       *** YET IMPLEMENTED IN THE MAC VERSION                    ***
*
*******************************************************************************/
#define _CRTBLD

int _fltused = 0x9875;
int __fastflag = 0;
int _adjust_fdiv = 0;

#ifdef _WIN32

#include <cruntime.h>
#include <msdos.h>
#include <dos.h>
#include <oscalls.h>
#include <mtdll.h>
#include <internal.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <dbgint.h>


/* define errno */
#ifndef _MT
int errno = 0;            /* libc error value */
unsigned long _doserrno = 0;  /* OS system error value */
#endif  /* _MT */



/* define umask */
int _umaskval = 0;


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


/* callable exit flag */
char _exitflag = 0;

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
 * pointers to initialization functions
 */

_PVFV _FPinit;          /* floating point init. */

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

extern __declspec(allocate(".CRT$XIA")) _PVFV __xi_a[];
extern __declspec(allocate(".CRT$XIZ")) _PVFV __xi_z[]; /* C initializers */
extern __declspec(allocate(".CRT$XCA")) _PVFV __xc_a[];
extern __declspec(allocate(".CRT$XCZ")) _PVFV __xc_z[]; /* C++ initializers */
extern __declspec(allocate(".CRT$XPA")) _PVFV __xp_a[];
extern __declspec(allocate(".CRT$XPZ")) _PVFV __xp_z[]; /* C pre-terminators */
extern __declspec(allocate(".CRT$XTA")) _PVFV __xt_a[];
extern __declspec(allocate(".CRT$XTZ")) _PVFV __xt_z[]; /* C terminators */

#else
extern _PVFV __xi_a[], __xi_z[];    /* C initializers */
extern _PVFV __xc_a[], __xc_z[];    /* C++ initializers */
extern _PVFV __xp_a[], __xp_z[];    /* C pre-terminators */
extern _PVFV __xt_a[], __xt_z[];    /* C terminators */
#endif // defined(_M_IA64)

#if defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) || defined (_M_IA64)
/*
 * For MIPS compiler, must explicitly force in and call the floating point
 * initialization.
 */
// BUGBUG: istvanc comment this out for new since we don't drop the EXE anyway..
//extern void __cdecl _fpmath(void);
#endif  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) */

/*
 * pointers to the start and finish of the _onexit/atexit table
 */
_PVFV *__onexitbegin;
_PVFV *__onexitend;


/*
 * static (internal) function that walks a table of function pointers,
 * calling each entry between the two pointers, skipping NULL entries
 *
 * Needs to be exported for CRT DLL so that C++ initializers in the
 * client EXE / DLLs can be initialized
 */
#ifdef CRTDLL
void __cdecl _initterm(_PVFV *, _PVFV *);
#else  /* CRTDLL */
static void __cdecl _initterm(_PVFV *, _PVFV *);
#endif  /* CRTDLL */


/***
*_cinit - C initialization
*
*Purpose:
*       This routine performs the shared DOS and Windows initialization.
*       The following order of initialization must be preserved -
*
*       1.  Check for devices for file handles 0 - 2
*       2.  Integer divide interrupt vector setup
*       3.  General C initializer routines
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
         * initialize floating point package, if present
         */
#if defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) || defined (_M_IA64)
        /*
         * MIPS compiler doesn't emit external reference to _fltused. Therefore,
         * must always force in the floating point initialization.
         */
// BUGBUG: istvanc comment this out for new since we don't drop the EXE anyway..
//        _fpmath();
#else  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) */
        if ( _FPinit != NULL )
            (*_FPinit)();
#endif  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) */

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



static void __cdecl doexit (
        int code,
        int quick,
        int retcaller
        )
{
#ifdef _DEBUG
        static int fExit = 0;
#endif  /* _DEBUG */

#ifdef _MT
        _lockexit();        /* assure only 1 thread in exit path */
#endif  /* _MT */

        if (_C_Exit_Done == TRUE)                               /* if doexit() is being called recursively */
                TerminateProcess(GetCurrentProcess(),code);     /* terminate with extreme prejudice */

        _C_Termination_Done = TRUE;

        /* save callable exit flag (for use by terminators) */
        _exitflag = (char) retcaller;  /* 0 = term, !0 = callable exit */

        if (!quick) {

            /*
             * do _onexit/atexit() terminators
             * (if there are any)
             *
             * These terminators MUST be executed in reverse order (LIFO)!
             *
             * NOTE:
             *  This code assumes that __onexitbegin points
             *  to the first valid onexit() entry and that
             *  __onexitend points past the last valid entry.
             *  If __onexitbegin == __onexitend, the table
             *  is empty and there are no routines to call.
             */

            if (__onexitbegin) {
                _PVFV * pfend = __onexitend;

                while ( --pfend >= __onexitbegin )
                /*
                 * if current table entry is non-NULL,
                 * call thru it.
                 */
                if ( *pfend != NULL )
                    (**pfend)();
            }

            /*
             * do pre-terminators
             */
            _initterm(__xp_a, __xp_z);
        }

        /*
         * do terminators
         */
        _initterm(__xt_a, __xt_z);

        /* return to OS or to caller */

        if (retcaller) {
#ifdef _MT
            _unlockexit();      /* unlock the exit code path */
#endif  /* _MT */
            return;
        }


        _C_Exit_Done = TRUE;

        ExitProcess(code);


}

#ifdef _MT
/***
* _lockexit - Aquire the exit code lock
*
*Purpose:
*       Makes sure only one thread is in the exit code at a time.
*       If a thread is already in the exit code, it must be allowed
*       to continue.  All other threads must pend.
*
*       Notes:
*
*       (1) It is legal for a thread that already has the lock to
*       try and get it again(!).  That is, consider the following
*       sequence:
*
*           (a) program calls exit()
*           (b) thread locks exit code
*           (c) user onexit() routine calls _exit()
*           (d) same thread tries to lock exit code
*
*       Since _exit() must ALWAYS be able to work (i.e., can be called
*       from anywhere with no regard for locking), we must make sure the
*       program does not deadlock at step (d) above.
*
*       (2) If a thread executing exit() or _exit() aquires the exit lock,
*       other threads trying to get the lock will pend forever.  That is,
*       since exit() and _exit() terminate the process, there is not need
*       for them to unlock the exit code path.
*
*       (3) Note that onexit()/atexit() routines call _lockexit/_unlockexit
*       to protect mthread access to the onexit table.
*
*       (4) The 32-bit OS semaphore calls DO allow a single thread to acquire
*       the same lock multiple times* thus, this version is straight forward.
*
*Entry: <none>
*
*Exit:
*       Calling thread has exit code path locked on return.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _lockexit (
        void
        )
{
        _mlock(_EXIT_LOCK1);
}

/***
* _unlockexit - Release exit code lock
*
*Purpose:
*       [See _lockexit() description above.]
*
*       This routine is called by _cexit(), _c_exit(), and onexit()/atexit().
*       The exit() and _exit() routines never unlock the exit code path since
*       they are terminating the process.
*
*Entry:
*       Exit code path is unlocked.
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _unlockexit (
        void
        )
{
        _munlock(_EXIT_LOCK1);
}

#endif  /* _MT */


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

#ifdef CRTDLL
void __cdecl _initterm (
#else  /* CRTDLL */
static void __cdecl _initterm (
#endif  /* CRTDLL */
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

#else  /* _WIN32 */

#include <cruntime.h>
#include <msdos.h>
#include <stdio.h>
#include <stdlib.h>
#include <internal.h>
#include <fltintrn.h>
#include <mpw.h>
#include <mtdll.h>
#include <macos\types.h>
#include <macos\segload.h>
#include <macos\gestalte.h>
#include <macos\osutils.h>
#include <macos\traps.h>
#include <dbgint.h>

/* define errno */

int _VARTYPE1 errno = 0;                /* libc error value */
int _VARTYPE1 _macerrno = 0;    /* OS system error value */

/* define umask */
int _umaskval = 0;

/* define version info variables */
unsigned int _VARTYPE1 _osver = 0;
//unsigned int _VARTYPE1 _osversion = 0;
//unsigned int _VARTYPE1 _osmajor = 0;
//unsigned int _VARTYPE1 _osminor = 0;

/* exprot them as in C7*/
int __argc = 0;
char **__argv = NULL;

/* number of allowable file handles */
int _nfile = _NHANDLE_;

/* file handle database -- stdout, stdin, stderr are NOT open */
char _osfile[_NHANDLE_] = {(unsigned char)(FOPEN+FTEXT), (unsigned char)(FOPEN+FTEXT), (unsigned char)(FOPEN+FTEXT)};
int _osfhnd [_NHANDLE_] = {-1, -1, -1};
unsigned char _osperm [_NHANDLE_];
short _osVRefNum [_NHANDLE_];
unsigned char _osfileflags[_NHANDLE_];

/* environment pointer */
char **_environ = NULL;

char _exitflag = 0;

/* MPW block pointer */

MPWBLOCK * _pMPWBlock = NULL;


/*
 * pointers to initialization functions
 */

extern PFV __xi_a ;

extern PFV __xi_z ;

extern PFV __xc_a ;  /* C++ initializers */

extern PFV __xc_z ;

extern PFV __xp_a ;  /* C pre-terminators */

extern PFV __xp_z ;

extern PFV __xt_a ;   /* C terminators */

extern PFV __xt_z ;




/*
 * pointers to the start and finish of the _onexit/atexit table
 */
extern PFV *__onexitbegin;
extern PFV *__onexitend;

#ifndef _M_MPPC
static void _CALLTYPE4 _initterm(PFV *, PFV *);
#else  /* _M_MPPC */
void _CALLTYPE4 _initterm(PFV *, PFV *);
#endif  /* _M_MPPC */

/***
*_cinit - C initialization
*
*Purpose:
*       This routine performs the shared DOS and Windows initialization.
*       The following order of initialization must be preserved -
*
*    ?  1.      Check for devices for file handles 0 - 2
*    ?  2.      Integer divide interrupt vector setup
*       3.      General C initializer routines
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
int _CALLTYPE1 __cinit (
        void
        )
{
        long lRespond;
        OSErr osErr;

        /*
         * do initializations
         */
        _initterm( &__xi_a, &__xi_z );

        /*
         * do C++ initializations
         */
        _initterm( &__xc_a, &__xc_z );

        osErr = Gestalt(gestaltSystemVersion, &lRespond);
        if (!osErr)
            _osver = lRespond;

        return 0;
}


/***
*exit(status), _exit(status), _cexit(void), _c_exit(void) - C termination
*
*Purpose:
*
*       Entry points:
*
*               exit(code):  Performs all the C termination functions
*                       and terminates the process with the return code
*                       supplied by the user.
*
*               _exit(code):  Performs a quick exit routine that does not
*                       do certain 'high-level' exit processing.  The _exit
*                       routine terminates the process with the return code
*                       supplied by the user.
*
*               _cexit():  Performs the same C lib termination processing
*                       as exit(code) but returns control to the caller
*                       when done (i.e., does NOT terminate the process).
*
*               _c_exit():  Performs the same C lib termination processing
*                       as _exit(code) but returns control to the caller
*                       when done (i.e., does NOT terminate the process).
*
*       Termination actions:
*
*               exit(), _cexit():
*
*               1.      call user's terminator routines
*               2.      call C runtime preterminators
*
*               _exit(), _c_exit():
*
*               3.      call C runtime terminators
*               4.      return to DOS or caller
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
*               int status - exit status (0-255)
*
*       _cexit(), _c_exit()
*               <no input>
*
*Exit:
*       exit(), _exit()
*               <EXIT to DOS>
*
*       _cexit(), _c_exit()
*               Return to caller
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

/* worker routine prototype */
/* public doexit and make exit and _exit function substitutable */

void _CALLTYPE4 doexit (int code, int quick, int retcaller);

void _CALLTYPE1 _cexit (
        void
        )
{
        doexit(0, 0, 1);        /* full term, return to caller */
}

void _CALLTYPE1 _c_exit (
        void
        )
{
        doexit(0, 1, 1);        /* quick term, return to caller */
}


void _CALLTYPE4 doexit (
        int code,
        int quick,
        int retcaller
        )
{
#ifdef _MT
        _lockexit();            /* assure only 1 thread in exit path */
#endif  /* _MT */

        /* save callable exit flag (for use by terminators) */
        _exitflag = (char) retcaller;  /* 0 = term, !0 = callable exit */

        if (!quick) {

            /*
             * do _onexit/atexit() terminators
             * (if there are any)
             */

            if (__onexitbegin)
                _initterm(__onexitbegin, __onexitend);

            /*
             * do pre-terminators
             */

            _initterm(&__xp_a, &__xp_z);
        }

        /*
         * do terminators
         */
        _initterm(&__xt_a, &__xt_z);


        /*
         * Floating point termination should go here...
         */

        /* return to caller */
        if (retcaller) {
#ifdef _MT
            _unlockexit();      /* unlock the exit code path */
#endif  /* _MT */
            return;
        }
        if (_pMPWBlock != NULL)
            {
            _pMPWBlock->retCode = code;
            }
        _ShellReturn();

}


#ifdef _M_MPPC
void __cdecl _DoExitSpecial(int code, int retcaller,
                            PFV *pAppPreTermBegin, PFV *pAppPreTermEnd,
                            PFV *pAppTermBegin, PFV *pAppTermEnd,
                            PFV *pAppOnexitBeg, PFV *pAppOnexitEnd)
        {
#ifdef _DEBUG
        static int fExit = 0;
#endif  /* _DEBUG */

        /*
         * do _onexit/atexit() terminators
         * (if there are any) including C++ destructors
         */

        if (pAppOnexitBeg && pAppOnexitBeg != (PFV *)(-1))
            _initterm(pAppOnexitBeg, pAppOnexitEnd);

        if (__onexitbegin)
            _initterm(__onexitbegin, __onexitend);

        /*
         * do pre-terminators, do app's one first
         */
        _initterm(pAppPreTermBegin, pAppPreTermEnd);

        _initterm(&__xp_a, &__xp_z);

        /*
         * do terminators, do app's first
         */
        _initterm(pAppTermBegin, pAppTermEnd);

        _initterm(&__xt_a, &__xt_z);

#ifdef _DEBUG
        /* Dump all memory leaks */
        if (!fExit && _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) & _CRTDBG_LEAK_CHECK_DF)
        {
            fExit = 1;
            _CrtDumpMemoryLeaks();
        }
#endif  /* _DEBUG */

        if (_pMPWBlock != NULL)
            {
            _pMPWBlock->retCode = code;
            }
        if (!retcaller)
            {
            _ShellReturn();
            }
        else
            {
            return;
            }

}
#endif  /* _M_MPPC */


#ifdef _MT
/***
* _lockexit - Aquire the exit code lock
*
*Purpose:
*       Makes sure only one thread is in the exit code at a time.
*       If a thread is already in the exit code, it must be allowed
*       to continue.  All other threads must pend.
*
*       Notes:
*
*       (1) It is legal for a thread that already has the lock to
*       try and get it again(!).  That is, consider the following
*       sequence:
*
*               (a) program calls exit()
*               (b) thread locks exit code
*               (c) user onexit() routine calls _exit()
*               (d) same thread tries to lock exit code
*
*       Since _exit() must ALWAYS be able to work (i.e., can be called
*       from anywhere with no regard for locking), we must make sure the
*       program does not deadlock at step (d) above.
*
*       (2) If a thread executing exit() or _exit() aquires the exit lock,
*       other threads trying to get the lock will pend forever.  That is,
*       since exit() and _exit() terminate the process, there is not need
*       for them to unlock the exit code path.
*
*       (3) Note that onexit()/atexit() routines call _lockexit/_unlockexit
*       to protect mthread access to the onexit table.
*
*       (4) The 32-bit semaphore calls DO allow a single thread to aquire the
*       same lock multiple times thus, this version is straight forward.
*
*Entry: <none>
*
*Exit:
*       Calling thread has exit code path locked on return.
*
*Exceptions:
*
*******************************************************************************/

void _CALLTYPE1 _lockexit (
        void
        )
{
        _mlock(_EXIT_LOCK1);
}

/***
* _unlockexit - Release exit code lock
*
*Purpose:
*       [See _lockexit() description above.]
*
*       This routine is called by _cexit(), _c_exit(), and onexit()/atexit().
*       The exit() and _exit() routines never unlock the exit code path since
*       they are terminating the process.
*
*Entry:
*       Exit code path is unlocked.
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void _CALLTYPE1 _unlockexit (
        void
        )
{
        _munlock(_EXIT_LOCK1);
}

#endif  /* _MT */

/***
* static void _initterm(PFV * pfbegin, PFV * pfend) - call entries in
*       function pointer table
*
*Purpose:
*       Walk a table of function pointers, calling each entry, as follows:
*
*               1. walk from beginning to end, pfunctbl is assumed to point
*                  to the beginning of the table, which is currently a null entry,
*                  as is the end entry.
*               2. skip NULL entries
*               3. stop walking when the end of the table is encountered
*
*Entry:
*       PFV *pfbegin    - pointer to the beginning of the table (first valid entry).
*       PFV *pfend      - pointer to the end of the table (after last valid entry).
*
*Exit:
*       No return value
*
*Notes:
*       For onexit() to work, this routine must start with the entry pointed
*       to by pfbegin and must stop before trying to call what pfend points
*       to.  __onexitbegin points to a valid entry but __onexitend does not!
*
*Exceptions:
*       If either pfbegin or pfend is NULL, or invalid, all bets are off!
*
*******************************************************************************/
#ifndef _M_MPPC
static void _CALLTYPE4 _initterm (
        PFV * pfbegin,
        PFV * pfend
        )
#else  /* _M_MPPC */
void _CALLTYPE4 _initterm (
        PFV * pfbegin,
        PFV * pfend
        )
#endif  /* _M_MPPC */
{
        /*
         * walk the table of function pointers from the top down, until
         * bottom is encountered.  Do not skip the first entry.
         */
        for ( ;pfbegin < pfend ; pfbegin++)
        {
            /*
             * if current table entry is non-NULL (and not -1), call
             * thru it from end of table to begin.
             */
            if ( *pfbegin != NULL && *pfbegin != (PFV) -1 )
                (**pfbegin)();
        }
}

#endif  /* _WIN32 */

