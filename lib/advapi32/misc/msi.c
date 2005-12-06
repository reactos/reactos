/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/misc/msi.c
 * PURPOSE:         advapi32.dll MSI interface funcs
 * NOTES:           Copied from Wine
 *                  Copyright 1995 Sven Verdoolaege
 */

#include <advapi32.h>

#define NDEBUG
#include <debug.h>

typedef UINT (WINAPI *fnMsiProvideComponentFromDescriptor)(LPCWSTR,LPWSTR,DWORD*,DWORD*);

DWORD WINAPI CommandLineFromMsiDescriptor( WCHAR *szDescriptor,
                    WCHAR *szCommandLine, DWORD *pcchCommandLine )
{
    static const WCHAR szMsi[] = { 'm','s','i',0 };
    fnMsiProvideComponentFromDescriptor mpcfd;
    HMODULE hmsi;
    UINT r = ERROR_CALL_NOT_IMPLEMENTED;

    DPRINT("%S %p %p\n", szDescriptor, szCommandLine, pcchCommandLine);

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
