/***
*crt0.c - C runtime initialization routine
*
* Copyright (c) 1989 - 1999 Microsoft Corporation. All rights reserved.*
*Purpose:
*       This the actual startup routine for console apps.  It calls the
*       user's main routine main() after performing C Run-Time Library
*       initialization.
*
*       (With ifdef's, this source file also provides the source code for
*       wcrt0.c, the startup routine for console apps with wide characters,
*       wincrt0.c, the startup routine for Windows apps, and wwincrt0.c,
*       the startup routine for Windows apps with wide characters.)
*
*******************************************************************************/
#define _CRTBLD

#ifdef _WIN32

#ifndef CRTDLL

#include <cruntime.h>
#include <dos.h>
#include <internal.h>
#include <stdlib.h>
#include <string.h>
#include <rterr.h>
#include <oscalls.h>
#include <awint.h>
#include <tchar.h>
#include <dbgint.h>

EXTERN_C BOOL Runtime_init();
EXTERN_C void Runtime_exit();
EXTERN_C BOOL Memory_init();
EXTERN_C void Memory_exit();

/*
 * wWinMain is not yet defined in winbase.h. When it is, this should be
 * removed.
 */

int
WINAPI
wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine,
    int nShowCmd
    );

#define SPACECHAR   _T(' ')
#define DQUOTECHAR  _T('\"')

// comes from stdargs.c
extern char * g_pargs;

/*
 * command line, environment, and a few other globals
 */

#ifdef WPRFLAG
wchar_t *_wcmdln;           /* points to wide command line */
#else  /* WPRFLAG */
char *_acmdln;              /* points to command line */
#endif  /* WPRFLAG */

char *_aenvptr = NULL;      /* points to environment block */
wchar_t *_wenvptr = NULL;   /* points to wide environment block */


void (__cdecl * _aexit_rtn)(int) = _exit;   /* RT message return procedure */

/*
 * _error_mode and _apptype, together, determine how error messages are
 * written out.
 */
int __error_mode = _OUT_TO_DEFAULT;
#ifdef _WINMAIN_
int __app_type = _GUI_APP;
#else  /* _WINMAIN_ */
int __app_type = _CONSOLE_APP;
#endif  /* _WINMAIN_ */


/***
*BaseProcessStartup(PVOID Peb)
*
*Purpose:
*       This routine does the C runtime initialization, calls main(), and
*       then exits.  It never returns.
*
*Entry:
*       PVOID Peb - pointer to Win32 Process Environment Block (not used)
*
*Exit:
*       This function never returns.
*
*******************************************************************************/

#ifdef _WINMAIN_

#ifdef WPRFLAG
void wWinMainCRTStartup(
#else  /* WPRFLAG */
void WinMainCRTStartup(
#endif  /* WPRFLAG */

#else  /* _WINMAIN_ */

#ifdef WPRFLAG
void wmainCRTStartup(
#else  /* WPRFLAG */
void mainCRTStartup(
#endif  /* WPRFLAG */

#endif  /* _WINMAIN_ */
        void
        )

{
        int mainret;

#ifdef _WINMAIN_
        _TUCHAR *lpszCommandLine;
        STARTUPINFO StartupInfo;
#endif  /* _WINMAIN_ */

        /*
         * Get the full Win32 version
         */
        _osver = GetVersion();

        _winminor = (_osver >> 8) & 0x00FF ;
        _winmajor = _osver & 0x00FF ;
        _winver = (_winmajor << 8) + _winminor;
        _osver = (_osver >> 16) & 0x00FFFF ;

        if ( !Memory_init() )            /* initialize heap */
            _amsg_exit(_RT_HEAPINIT);   /* write message and die */

#ifdef _MT
        if( !_mtinit() )                /* initialize multi-thread */
            _amsg_exit(_RT_THREAD);     /* write message and die */
#endif  /* _MT */

        /*
         * Guard the remainder of the initialization code and the call
         * to user's main, or WinMain, function in a __try/__except
         * statement.
         */

//        __try {

#if defined (_MBCS)
            /*
             * Initialize multibyte ctype table. Always done since it is
             * needed for startup wildcard and argv processing.
             */
            __initmbctable();
#endif  /* defined (_MBCS) */

#ifdef WPRFLAG
            /* get wide cmd line info */
            _wcmdln = (wchar_t *)__crtGetCommandLineW();

            /* get wide environ info */
            _wenvptr = (wchar_t *)__crtGetEnvironmentStringsW();

            if ((_wcmdln == NULL) || (_wenvptr == NULL)) {
                exit(-1);
            }

            _wsetargv();
            _wsetenvp();
#else  /* WPRFLAG */
            /* get cmd line info */
            _acmdln = (char *)GetCommandLineA();

            if ((_acmdln == NULL)) {
                exit(-1);
            }

            _setargv();
#endif  /* WPRFLAG */

            _cinit();           /* do C data initialize */

            if (!Runtime_init())
                exit(-1);

#ifdef _WINMAIN_
            /*
             * Skip past program name (first token in command line).
             * Check for and handle quoted program name.
             */
#ifdef WPRFLAG
            lpszCommandLine = (wchar_t *)_wcmdln;
#else  /* WPRFLAG */
            lpszCommandLine = (unsigned char *)_acmdln;
#endif  /* WPRFLAG */

            if ( *lpszCommandLine == DQUOTECHAR ) {
                /*
                 * Scan, and skip over, subsequent characters until
                 * another double-quote or a null is encountered.
                 */

                while ( (*(++lpszCommandLine) != DQUOTECHAR)
                    && (*lpszCommandLine != _T('\0')) ) {

#ifdef _MBCS
                        if (_ismbblead(*lpszCommandLine))
                            lpszCommandLine++;
#endif  /* _MBCS */
                }

                /*
                 * If we stopped on a double-quote (usual case), skip
                 * over it.
                 */
                if ( *lpszCommandLine == DQUOTECHAR )
                    lpszCommandLine++;
            }
            else {
                while (*lpszCommandLine > SPACECHAR)
                    lpszCommandLine++;
            }

            /*
             * Skip past any white space preceeding the second token.
             */
            while (*lpszCommandLine && (*lpszCommandLine <= SPACECHAR)) {
                lpszCommandLine++;
            }

            StartupInfo.dwFlags = 0;
            GetStartupInfo( &StartupInfo );

#ifdef WPRFLAG
            mainret = wWinMain( GetModuleHandle(NULL),
#else  /* WPRFLAG */
            mainret = WinMain( GetModuleHandle(NULL),
#endif  /* WPRFLAG */
                               NULL,
                               lpszCommandLine,
                               StartupInfo.dwFlags & STARTF_USESHOWWINDOW
                                    ? StartupInfo.wShowWindow
                                    : SW_SHOWDEFAULT
                             );
#else  /* _WINMAIN_ */

#ifdef WPRFLAG
            __winitenv = _wenviron;
            mainret = wmain(__argc, __wargv, _wenviron);
#else  /* WPRFLAG */
            __initenv = _environ;
            mainret = main(__argc, __argv, _environ);
#endif  /* WPRFLAG */

            // delete args...
            free(g_pargs);

            Runtime_exit();

            Memory_exit();

#if DBG == 1
            {

#define LENGTH(a) (sizeof(a)/sizeof(a[0]))

                char achMem[32];

                if (GetEnvironmentVariableA("MSXML_MEMORYLEAK", achMem, LENGTH(achMem)))
                {
                    if (achMem[0] == '1')
                        mainret = 1;
                }
            }
#endif

#endif  /* _WINMAIN_ */
            exit(mainret);
//        }
//        __except ( _XcptFilter(GetExceptionCode(), GetExceptionInformation()) )
//        {
//            /*
//             * Should never reach here
//             */
//            _exit( GetExceptionCode() );
//
//        } /* end of try - except */

}



/***
*_amsg_exit(rterrnum) - Fast exit fatal errors
*
*Purpose:
*       Exit the program with error code of 255 and appropriate error
*       message.
*
*Entry:
*       int rterrnum - error message number (amsg_exit only).
*
*Exit:
*       Calls exit() (for integer divide-by-0) or _exit() indirectly
*       through _aexit_rtn [amsg_exit].
*       For multi-thread: calls _exit() function
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _amsg_exit (
        int rterrnum
        )
{
        _aexit_rtn(255);        /* normally _exit(255) */
}

#ifndef WPRFLAG


#endif  /* WPRFLAG */

#endif  /* CRTDLL */

#else  /* _WIN32 */

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <msdos.h>
#include <string.h>
#include <setjmp.h>
#include <dbgint.h>
#include <macos\types.h>
#include <macos\segload.h>
#include <macos\gestalte.h>
#include <macos\osutils.h>
#include <macos\traps.h>
#include <mpw.h>

static void _CALLTYPE4 Inherit(void);  /* local function */

int _CALLTYPE1 main(int, char **, char **);             /*generated by compiler*/

unsigned long _GetShellStack(void);

static char * _CALLTYPE1 _p2cstr_internal ( unsigned char * str );

extern MPWBLOCK * _pMPWBlock;
extern int __argc;
extern char **__argv;

/***
*__crt0()
*
*Purpose:
*       This routine does the C runtime initialization, calls main(), and
*       then exits.  It never returns.
*
*Entry:
*
*Exit:
*       This function never returns.
*
*******************************************************************************/

void _CALLTYPE1 __crt0 (
        )
{
        int mainret;
        char szPgmName[32];
        char *pArg;
        char *argv[2];

#ifndef _M_MPPC
        void *pv;

        /* This is the magic stuff that MPW tools do to get info from MPW*/

        pv = (void *)*(int *)0x316;
        if (pv != NULL && !((int)pv & 1) && *(int *)pv == 'MPGM') {
            pv = (void *)*++(int *)pv;
            if (pv != NULL && *(short *)pv == 'SH') {
                _pMPWBlock = (MPWBLOCK *)pv;
            }
        }

#endif  /* _M_MPPC */

        _environ = NULL;
        if (_pMPWBlock == NULL) {
            __argc = 1;
            memcpy(szPgmName, (char *)0x910, sizeof(szPgmName));
            pArg = _p2cstr_internal(szPgmName);
            argv[0] = pArg;
            argv[1] = NULL;
            __argv = argv;

#ifndef _M_MPPC
            _shellStack = 0;                        /* force ExitToShell */
#endif  /* _M_MPPC */
        }
#ifndef _M_MPPC
        else {
            _shellStack = _GetShellStack();        //return current a6, or first a6
            _shellStack += 4;                      //a6 + 4 is the stack pointer we want
            __argc = _pMPWBlock->argc;
            __argv = _pMPWBlock->argv;

            Inherit();       /* Inherit file handles - env is set up by _envinit if needed */
        }
#endif  /* _M_MPPC */

        /*
         * call run time initializer
         */
        __cinit();

        mainret = main(__argc, __argv, _environ);
        exit(mainret);
}


#ifndef _M_MPPC
/***
*Inherit() - obtain and process info on inherited file handles.
*
*Purpose:
*
*       Locates and interprets MPW std files.  For files we just save the
*       file handles.   For the console we save the device table address so
*       we can do console I/O.  In the latter case, FDEV is set in the _osfile
*       array.
*
*Entry:
*       Address of MPW param table
*
*Exit:
*       No return value.
*
*Exceptions:
*
*******************************************************************************/

static void _CALLTYPE4 Inherit (
        void
        )
{
        MPWFILE *pFile;
        int i;
        pFile = _pMPWBlock->pFile;
        if (pFile == NULL) {
            return;
        }
        for (i = 0; i < 3; i++) {
            switch ((pFile->pDevice)->name) {
                case 'ECON':
                    _osfile[i] |= FDEV | FOPEN;
                    _osfhnd[i] = (int)pFile;
                    break;

                case 'FSYS':
                    _osfile[i] |= FOPEN;
                    _osfhnd[i] = (*(pFile->ppFInfo))->ioRefNum;
                    break;
            }
            pFile++;
        }
}

#endif  /* _M_MPPC */



static char * _CALLTYPE1 _p2cstr_internal (
        unsigned char * str
        )
{
        unsigned char *pchSrc;
        unsigned char *pchDst;
        int  cch;

        if ( str && *str ) {
            pchDst = str;
            pchSrc = str + 1;

            for ( cch=*pchDst; cch; --cch ) {
                *pchDst++ = *pchSrc++;
            }

            *pchDst = '\0';
        }

        return( str );
}

#endif  /* _WIN32 */
