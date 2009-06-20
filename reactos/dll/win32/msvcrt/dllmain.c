/* $Id$
 *
 * dllmain.c
 *
 * ReactOS MSVCRT.DLL Compatibility Library
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRENTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warrenties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.24 $
 * $Author$
 * $Date$
 *
 */

#include <precomp.h>
#include <internal/wine/msvcrt.h>
#include <locale.h>
#include <mbctype.h>

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* EXTERNAL PROTOTYPES ********************************************************/

extern int BlockEnvToEnvironA(void);
extern int BlockEnvToEnvironW(void);
extern void FreeEnvironment(char **environment);
extern void _atexit_cleanup(void);

extern unsigned int _osver;
extern unsigned int _winminor;
extern unsigned int _winmajor;
extern unsigned int _winver;

extern char* _acmdln;        /* pointer to ascii command line */
extern wchar_t* _wcmdln;     /* pointer to wide character command line */
#undef _environ
extern char** _environ;      /* pointer to environment block */
extern char** __initenv;     /* pointer to initial environment block */
extern wchar_t** _wenviron;  /* pointer to environment block */
extern wchar_t** __winitenv; /* pointer to initial environment block */


/* LIBRARY GLOBAL VARIABLES ***************************************************/

HANDLE hHeap = NULL;        /* handle for heap */


/* LIBRARY ENTRY POINT ********************************************************/

BOOL
WINAPI
DllMain(PVOID hinstDll, ULONG dwReason, PVOID reserved)
{
    OSVERSIONINFOW osvi;
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH://1
        /* initialize version info */
        //DPRINT1("Process Attach %d\n", nAttachCount);
        //DPRINT1("Process Attach\n");
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
        GetVersionExW( &osvi );
        _winver     = (osvi.dwMajorVersion << 8) | osvi.dwMinorVersion;
        _winmajor   = osvi.dwMajorVersion;
        _winminor   = osvi.dwMinorVersion;
        _osver      = osvi.dwBuildNumber;
        hHeap = HeapCreate(0, 100000, 0);
        if (hHeap == NULL)
            return FALSE;

        /* create tls stuff */
        if (!CreateThreadData())
            return FALSE;

        if (BlockEnvToEnvironA() < 0)
            return FALSE;

        if (BlockEnvToEnvironW() < 0)
        {
            FreeEnvironment(_environ);
            return FALSE;
        }

        _acmdln = _strdup(GetCommandLineA());
        _wcmdln = _wcsdup(GetCommandLineW());

        /* FIXME: more initializations... */

        /* Initialization of the WINE code */
        msvcrt_init_mt_locks();
        msvcrt_init_io();
        setlocale(0, "C");
        //_setmbcp(_MB_CP_LOCALE);

        TRACE("Attach done\n");
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        FreeThreadData(NULL);
        break;

    case DLL_PROCESS_DETACH:
        //DPRINT1("Detach %d\n", nAttachCount);
        //DPRINT("Detach\n");
        /* FIXME: more cleanup... */
        /* Deinit of the WINE code */
        msvcrt_free_io();
        msvcrt_free_mt_locks();

        _atexit_cleanup();


        /* destroy tls stuff */
        DestroyThreadData();

	if (__winitenv && __winitenv != _wenviron)
            FreeEnvironment((char**)__winitenv);
        if (_wenviron)
            FreeEnvironment((char**)_wenviron);

	if (__initenv && __initenv != _environ)
            FreeEnvironment(__initenv);
        if (_environ)
            FreeEnvironment(_environ);

        /* destroy heap */
        HeapDestroy(hHeap);

        TRACE("Detach done\n");
        break;
    }

    return TRUE;
}

/* EOF */
