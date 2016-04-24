/*
 * msvcrt20 implementation
 *
 * Copyright 2002 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_NO_STATUS

#include <stdio.h>
#define _CRT_PRECOMP_H
#include <internal/tls.h>
//#include <stdlib.h>
//#include <windows.h>
#include <internal/wine/msvcrt.h>
#include <internal/locale.h>
//#include <locale.h>
//#include <mbctype.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* EXTERNAL PROTOTYPES ********************************************************/

extern int BlockEnvToEnvironA(void);
extern int BlockEnvToEnvironW(void);
extern void FreeEnvironment(char **environment);

extern unsigned int _osplatform;
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

extern void CDECL __getmainargs(int *argc, char** *argv, char** *envp,
                                int expand_wildcards, int *new_mode);
extern void CDECL __wgetmainargs(int *argc, WCHAR** *wargv, WCHAR** *wenvp,
                                 int expand_wildcards, int *new_mode);

/* LIBRARY ENTRY POINT ********************************************************/

BOOL
WINAPI
DllMain(PVOID hinstDll, ULONG dwReason, PVOID reserved)
{
    OSVERSIONINFOW osvi;
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        /* initialize version info */
        TRACE("Process Attach\n");
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
        GetVersionExW( &osvi );
        _winver     = (osvi.dwMajorVersion << 8) | osvi.dwMinorVersion;
        _winmajor   = osvi.dwMajorVersion;
        _winminor   = osvi.dwMinorVersion;
        _osplatform = osvi.dwPlatformId;
        _osver      = osvi.dwBuildNumber;

        /* create tls stuff */
        if (!msvcrt_init_tls())
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

        /* Initialization of the WINE code */
        msvcrt_init_mt_locks();
        //msvcrt_init_math();
        msvcrt_init_io();
        //msvcrt_init_console();
        //msvcrt_init_args();
        //msvcrt_init_signals();
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

/* LIBRARY EXPORTS ************************************************************/

/*********************************************************************
 *		__getmainargs (MSVCRT20.@)
 *
 * new_mode is not a pointer in msvcrt20.
 */
void CDECL MSVCRT20__getmainargs( int *argc, char** *argv, char** *envp,
                                  int expand_wildcards, int new_mode )
{
    __getmainargs( argc, argv, envp, expand_wildcards, &new_mode );
}

/*********************************************************************
 *		__wgetmainargs (MSVCRT20.@)
 *
 * new_mode is not a pointer in msvcrt20.
 */
void CDECL MSVCRT20__wgetmainargs( int *argc, WCHAR** *wargv, WCHAR** *wenvp,
                                   int expand_wildcards, int new_mode )
{
    __wgetmainargs( argc, wargv, wenvp, expand_wildcards, &new_mode );
}

/* EOF */
