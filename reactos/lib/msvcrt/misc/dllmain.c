/* $Id: dllmain.c,v 1.19 2003/06/01 17:08:03 hbirr Exp $
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
 * $Revision: 1.19 $
 * $Author: hbirr $
 * $Date: 2003/06/01 17:08:03 $
 *
 */

#include <windows.h>
#include <msvcrt/internal/tls.h>
#include <msvcrt/stdlib.h>

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

static int nAttachCount = 0;

HANDLE hHeap = NULL;        /* handle for heap */


/* LIBRARY ENTRY POINT ********************************************************/

BOOL
STDCALL
DllMain(PVOID hinstDll, ULONG dwReason, PVOID reserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH://1
#if 0
#else
        /* initialize version info */
        DPRINT("Attach %d\n", nAttachCount);
        _osver = GetVersion();
        _winmajor = (_osver >> 8) & 0xFF;
        _winminor = _osver & 0xFF;
        _winver = (_winmajor << 8) + _winminor;
        _osver = (_osver >> 16) & 0xFFFF;
        if (hHeap == NULL || hHeap == INVALID_HANDLE_VALUE)
        {
            hHeap = HeapCreate(0, 100000, 0);
            if (hHeap == NULL || hHeap == INVALID_HANDLE_VALUE)
            {
                return FALSE;
            }
        }
        if (nAttachCount==0)
        {
            if (!__fileno_init()) {
                HeapDestroy(hHeap);
                hHeap = NULL;
                return FALSE;
            }
        }

        /* create tls stuff */
        if (!CreateThreadData())
            return FALSE;

        _acmdln = (char *)GetCommandLineA();

        /* FIXME: This crashes all applications */
        if (BlockEnvToEnviron() < 0)
          return FALSE;

        /* FIXME: more initializations... */

        nAttachCount++;
        DPRINT("Attach done\n");
#endif
        break;

    case DLL_THREAD_ATTACH://2
        break;

    case DLL_THREAD_DETACH://4
        FreeThreadData(NULL);
        break;

    case DLL_PROCESS_DETACH://0
        DPRINT("Detach %d\n", nAttachCount);
        if (nAttachCount > 0)
        {
            nAttachCount--;

            /* FIXME: more cleanup... */
            _fcloseall();

            /* destroy tls stuff */
            DestroyThreadData();

            /* destroy heap */
            if (nAttachCount == 0)
            {
		if (__initenv && __initenv != _environ)
		{
                    FreeEnvironmentStringsA(__initenv[0]);
		    free(__initenv);
		    __initenv = NULL;
		}
                if (_environ)
                {
                    FreeEnvironmentStringsA(_environ[0]);
                    free(_environ);
                    _environ = NULL;
                }
#if 1
                HeapDestroy(hHeap);
                hHeap = NULL;
#endif
            }
        DPRINT("Detach done\n");
        }
        break;
    }

    return TRUE;
}

/* EOF */
