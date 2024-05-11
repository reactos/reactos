/***
*loaddll.c - load or free a Dynamic Link Library
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _loaddll() and _unloaddll() - load and unload DLL
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <errno.h>
#include <stdlib.h>

#define _CRT_ENABLE_OBSOLETE_LOADLIBRARY_FUNCTIONS

#include <process.h>

/***
*int _loaddll(filename) - Load a dll
*
*Purpose:
*       Load a DLL into memory
*
*Entry:
*       char *filename - file to load
*
*Exit:
*       returns a unique DLL (module) handle if succeeds
*       returns 0 if fails
*
*Exceptions:
*
*******************************************************************************/

extern "C" intptr_t __cdecl _loaddll(char* szName)
{
    return reinterpret_cast<intptr_t>(__acrt_LoadLibraryExA(szName, nullptr, 0));
}

/***
*int _unloaddll(handle) - Unload a dll
*
*Purpose:
*       Unloads a DLL. The resources of the DLL will be freed if no other
*       processes are using it.
*
*Entry:
*       int handle - handle from _loaddll
*
*Exit:
*       returns 0 if succeeds
*       returns DOS error if fails
*
*Exceptions:
*
*******************************************************************************/

extern "C" int __cdecl _unloaddll(intptr_t hMod)
{
        if (!FreeLibrary(reinterpret_cast<HMODULE>(hMod))) {
            return ((int)GetLastError());
        }
        return (0);
}
