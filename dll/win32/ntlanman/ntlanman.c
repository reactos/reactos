/*
 * PROJECT:         LAN Manager
 * FILE:            dll\win32\ntlanman\ntlanman.c
 * PURPOSE:         Main file
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include <windows.h>
#include <npapi.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntlanman);

DWORD
WINAPI
NPGetConnection(LPWSTR lpLocalName,
                LPWSTR lpRemoteName,
                LPDWORD lpBufferSize)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
WINAPI
NPGetCaps(DWORD nIndex)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
WINAPI
NPGetUser(LPWSTR lpName,
          LPWSTR lpUserName,
          LPDWORD lpBufferSize)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
WINAPI
NPAddConnection(LPNETRESOURCE lpNetResource,
                LPWSTR lpPassword,
                LPWSTR lpUserName)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
WINAPI
NPCancelConnection(LPWSTR lpName,
                   BOOL fForce)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
WINAPI
NPPropertyDialog(HWND hwndParent,
                 DWORD iButtonDlg,
                 DWORD nPropSel,
                 LPWSTR lpFileName,
                 DWORD nType)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
WINAPI
NPGetDirectoryType(LPWSTR lpName,
                   LPINT lpType,
                   BOOL bFlushCache)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
WINAPI
NPDirectoryNotify(HWND hwnd,
                  LPWSTR lpDir,
                  DWORD dwOper)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
WINAPI
NPGetPropertyText(DWORD iButton,
                  DWORD nPropSel,
                  LPWSTR lpName,
                  LPWSTR lpButtonName,
                  DWORD nButtonNameLen,
                  DWORD nType)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
WINAPI
NPOpenEnum(DWORD dwScope,
           DWORD dwType,
           DWORD dwUsage,
           LPNETRESOURCE lpNetResource,
           LPHANDLE lphEnum)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
WINAPI
NPEnumResource(HANDLE hEnum,
               LPDWORD lpcCount,
               LPVOID lpBuffer,
               LPDWORD lpBufferSize)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
WINAPI
NPCloseEnum(HANDLE hEnum)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
WINAPI
NPFormatNetworkName(LPWSTR lpRemoteName,
                    LPWSTR lpFormattedName,
                    LPDWORD lpnLength,
                    DWORD dwFlags,
                    DWORD dwAveCharPerLine)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
WINAPI
NPAddConnection3(HWND hwndOwner,
                 LPNETRESOURCE lpNetResource,
                 LPWSTR lpPassword,
                 LPWSTR lpUserName,
                 DWORD dwFlags)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
WINAPI
NPGetUniversalName(LPCWSTR lpLocalPath,
                   DWORD dwInfoLevel,
                   LPVOID lpBuffer,
                   LPDWORD lpBufferSize)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
WINAPI
NPGetResourceParent(LPNETRESOURCE lpNetResource,
                    LPVOID lpBuffer,
                    LPDWORD lpcbBuffer)
{
    UNIMPLEMENTED;
    return 0;
}


BOOL
WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}
