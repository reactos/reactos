/*
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
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* EXTERNAL PROTOTYPES ********************************************************/

BOOL crt_process_init(void);

extern void FreeEnvironment(char **environment);

#undef _environ
extern char** _environ;      /* pointer to environment block */
extern char** __initenv;     /* pointer to initial environment block */
extern wchar_t** _wenviron;  /* pointer to environment block */
extern wchar_t** __winitenv; /* pointer to initial environment block */

/* LIBRARY ENTRY POINT ********************************************************/

BOOL
WINAPI
DllMain(PVOID hinstDll, ULONG dwReason, PVOID reserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:

        TRACE("Process Attach\n");

        if (!crt_process_init())
        {
            ERR("crt_init() failed!\n");
            return FALSE;
        }

        TRACE("Attach done\n");
        break;

    case DLL_THREAD_ATTACH:
        //msvcrt_get_thread_data creates data when first called
        break;

    case DLL_THREAD_DETACH:
        msvcrt_free_tls_mem();
        break;

    case DLL_PROCESS_DETACH:
        TRACE("Detach\n");
        /* Deinit of the WINE code */
        msvcrt_free_io();
        if (reserved) break;
        msvcrt_free_popen_data();
        msvcrt_free_mt_locks();
        //msvcrt_free_console();
        //msvcrt_free_args();
        //msvcrt_free_signals();
        msvcrt_free_tls_mem();
        if (!msvcrt_free_tls())
          return FALSE;
        if(global_locale)
          MSVCRT__free_locale(global_locale);

        if (__winitenv && __winitenv != _wenviron)
            FreeEnvironment((char**)__winitenv);
        if (_wenviron)
            FreeEnvironment((char**)_wenviron);

        if (__initenv && __initenv != _environ)
            FreeEnvironment(__initenv);
        if (_environ)
            FreeEnvironment(_environ);

        TRACE("Detach done\n");
        break;
    }

    return TRUE;
}

/* FIXME: This hack is required to prevent the VC linker from linking these
   exports to the functions from libntdll. See CORE-10753 */
#ifdef _MSC_VER
#ifdef _M_IX86
#pragma comment(linker, "/include:__vsnprintf")
#pragma comment(linker, "/include:_bsearch")
#pragma comment(linker, "/include:_strcspn")
#else
#pragma comment(linker, "/include:_vsnprintf")
#pragma comment(linker, "/include:bsearch")
#pragma comment(linker, "/include:strcspn")
#endif // _M_IX86
#endif // _MSC_VER

/* EOF */
