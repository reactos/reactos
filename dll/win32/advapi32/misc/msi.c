/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/misc/msi.c
 * PURPOSE:         advapi32.dll MSI interface funcs
 * NOTES:           Copied from Wine
 *                  Copyright 1995 Sven Verdoolaege
 */

#include <advapi32.h>

WINE_DEFAULT_DEBUG_CHANNEL(advapi);
#ifndef _UNICODE
#define debugstr_aw debugstr_a
#else
#define debugstr_aw debugstr_w
#endif


typedef UINT (WINAPI *fnMsiProvideComponentFromDescriptor)(LPCWSTR,LPWSTR,DWORD*,DWORD*);

DWORD WINAPI CommandLineFromMsiDescriptor( WCHAR *szDescriptor,
                    WCHAR *szCommandLine, DWORD *pcchCommandLine )
{
    static const WCHAR szMsi[] = { 'm','s','i',0 };
    fnMsiProvideComponentFromDescriptor mpcfd;
    HMODULE hmsi;
    UINT r = ERROR_CALL_NOT_IMPLEMENTED;

    TRACE("%S %p %p\n", szDescriptor, szCommandLine, pcchCommandLine);

    hmsi = LoadLibraryW( szMsi );
    if (!hmsi)
        return r;
    mpcfd = (void*) GetProcAddress( hmsi, "MsiProvideComponentFromDescriptorW" );
    if (mpcfd)
        r = mpcfd( szDescriptor, szCommandLine, pcchCommandLine, NULL );
    FreeLibrary( hmsi );
    return r;
}

/* EOF */
