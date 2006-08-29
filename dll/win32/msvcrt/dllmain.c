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

#define NDEBUG
#include <internal/debug.h>


/* EXTERNAL PROTOTYPES ********************************************************/

//void __fileno_init(void);
extern BOOL __fileno_init(void);
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
STDCALL
DllMain(PVOID hinstDll, ULONG dwReason, PVOID reserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH://1
        /* initialize version info */
        //DPRINT1("Process Attach %d\n", nAttachCount);
        //DPRINT1("Process Attach\n");
        _osver = GetVersion();
        _winmajor = (_osver >> 8) & 0xFF;
        _winminor = _osver & 0xFF;
        _winver = (_winmajor << 8) + _winminor;
        _osver = (_osver >> 16) & 0xFFFF;
        hHeap = HeapCreate(0, 100000, 0);
        if (hHeap == NULL)
            return FALSE;
        if (!__fileno_init()) 
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

        /* FIXME: Initialization of the WINE code */
        msvcrt_init_mt_locks();

        DPRINT("Attach done\n");
        break;

    case DLL_THREAD_ATTACH://2
        break;

    case DLL_THREAD_DETACH://4
        FreeThreadData(NULL);
        break;

    case DLL_PROCESS_DETACH://0
        //DPRINT1("Detach %d\n", nAttachCount);
        //DPRINT("Detach\n");
        /* FIXME: more cleanup... */
        _fcloseall();
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

        DPRINT("Detach done\n");
        break;
    }

    return TRUE;
}

/* EOF */
