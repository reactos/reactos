/* $Id: dllmain.c,v 1.22 2004/05/27 11:49:48 hbirr Exp $
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
 * $Revision: 1.22 $
 * $Author: hbirr $
 * $Date: 2004/05/27 11:49:48 $
 *
 */

#include <windows.h>
#include <msvcrt/internal/tls.h>
#include <msvcrt/stdlib.h>
#include "../wine/msvcrt.h"

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>


/* EXTERNAL PROTOTYPES ********************************************************/

//void __fileno_init(void);
extern BOOL __fileno_init(void);
extern int BlockEnvToEnviron(void);

extern unsigned int _osver;
extern unsigned int _winminor;
extern unsigned int _winmajor;
extern unsigned int _winver;

extern char* _acmdln;       /* pointer to ascii command line */
#undef _environ
extern char** _environ;     /* pointer to environment block */
extern char** __initenv;    /* pointer to initial environment block */


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
        DPRINT("Attach %d\n", nAttachCount);
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

        _acmdln = strdup(GetCommandLineA());

        /* FIXME: This crashes all applications */
        if (BlockEnvToEnviron() < 0)
          return FALSE;

        /* FIXME: more initializations... */

        /* FIXME: Initialization of the WINE code */
        msvcrt_init_mt_locks();
        msvcrt_init_args();

        DPRINT("Attach done\n");
        break;

    case DLL_THREAD_ATTACH://2
        break;

    case DLL_THREAD_DETACH://4
        FreeThreadData(NULL);
        break;

    case DLL_PROCESS_DETACH://0
        DPRINT("Detach %d\n", nAttachCount);
        /* FIXME: more cleanup... */
        _fcloseall();

        /* destroy tls stuff */
        DestroyThreadData();

	if (__initenv && __initenv != _environ)
	  {
            free(__initenv[0]);
	    free(__initenv);
	  }
        if (_environ)
          {
            free(_environ[0]);
            free(_environ);
          }
        /* destroy heap */
        HeapDestroy(hHeap);

        DPRINT("Detach done\n");
        break;
    }

    return TRUE;
}

/* EOF */
