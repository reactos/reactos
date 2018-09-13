/***
*crt0.c - C runtime initialization routine
*
*       Copyright (c) 1989-1997, Microsoft Corporation. All rights reserved.
*
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
*       #define _WINMAIN_ for Windows apps
*       #define WPRFLAG for wide versions
*
*******************************************************************************/

#ifdef _MBCS
#undef _MBCS
#endif

#include <cruntime.h>
#include <dos.h>
#include <internal.h>
#include <stdlib.h>
#include <string.h>
#include <rterr.h>
#include <oscalls.h>
#include <awint.h>
#include <tchar.h>
#include <excpt.h>

extern CRITICAL_SECTION s_cs;

// wWinMain is not yet defined in winbase.h. When it is, this should be
// removed.
int
WINAPI
wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine,
    int nShowCmd
    );

int __cdecl _XcptFilter (
        unsigned long xcptnum,
        PEXCEPTION_POINTERS pxcptinfoptrs
        );

#define SPACECHAR   _T(' ')
#define DQUOTECHAR  _T('\"')


// command line, environment, and a few other globals
#ifdef WPRFLAG
wchar_t *_wcmdln;           /* points to wide command line */
#else
char *_acmdln;              /* points to command line */
#endif

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
#else
void WinMainCRTStartup(
#endif

#else // _WINMAIN_

#ifdef WPRFLAG
void wmainCRTStartup(
#else
void mainCRTStartup(
#endif

#endif // _WINMAIN_ 
        void
        )

{
        int mainret;

#ifdef _WINMAIN_
        _TUCHAR *lpszCommandLine;
        STARTUPINFO StartupInfo;
#endif

        __error_mode = _OUT_TO_DEFAULT;
#ifdef  _WINMAIN_
        __app_type = _GUI_APP;
#else
        __app_type = _CONSOLE_APP;
#endif
        /*
         * Get the full Win32 version
         */
        _osver = GetVersion();

        _winminor = (_osver >> 8) & 0x00FF ;
        _winmajor = _osver & 0x00FF ;
        _winver = (_winmajor << 8) + _winminor;
        _osver = (_osver >> 16) & 0x00FFFF ;

        InitializeCriticalSection(&s_cs);

#ifdef  _MT
        if( !_mtinit() )                    /* initialize multi-thread */
            _exit(_RT_THREAD);               /* write message and die */
#endif

        /*
         * Guard the remainder of the initialization code and the call
         * to user's main, or WinMain, function in a __try/__except
         * statement.
         */

        __try {

#if defined(_MBCS)
            /*
             * Initialize multibyte ctype table. Always done since it is
             * needed for startup wildcard and argv processing.
             */
            __initmbctable();
#endif

#ifdef WPRFLAG
            _wenvptr = NULL;   // points to wide environment block
            
            /* get wide cmd line info */
            _wcmdln = (wchar_t *)GetCommandLineW();

            /* get wide environ info */
            _wenvptr = (wchar_t *)GetEnvironmentStringsW();

            if ((_wcmdln == NULL) || (_wenvptr == NULL)) {
                exit(-1);
            }

            _wsetargv();
            _wsetenvp();
#else
            _aenvptr = NULL;      // points to environment block
            
            /* get cmd line info */
            _acmdln = (char *)GetCommandLineA();

            /* get environ info */
            _aenvptr = (char *)GetEnvironmentStringsA();

            if ((_aenvptr == NULL) || (_acmdln == NULL)) {
                exit(-1);
            }

            _setargv();
            _setenvp();
#endif  // WPRFLAG

            _cinit();                       /* do C data initialize */


#ifdef _WINMAIN_
            /*
             * Skip past program name (first token in command line).
             * Check for and handle quoted program name.
             */
#ifdef WPRFLAG
            lpszCommandLine = (wchar_t *)_wcmdln;
#else
            lpszCommandLine = (unsigned char *)_acmdln;
#endif

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
#endif
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
#else
            mainret = WinMain( GetModuleHandle(NULL),
#endif
                               NULL,
                               lpszCommandLine,
                               StartupInfo.dwFlags & STARTF_USESHOWWINDOW
                                    ? StartupInfo.wShowWindow
                                    : SW_SHOWDEFAULT
                             );
#else /* WIN_MAIN */

#ifdef WPRFLAG
            __winitenv = _wenviron;
            mainret = wmain(__argc, __wargv, _wenviron);
#else
            __initenv = _environ;
            mainret = main(__argc, __argv, _environ);
#endif

#endif /* WIN_MAIN */
            exit(mainret);
        }
        __except ( _XcptFilter(GetExceptionCode(), GetExceptionInformation()) )
        {
            /*
             * Should never reach here
             */
            _exit( GetExceptionCode() );

        } /* end of try - except */

}


