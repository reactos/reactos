/*
 * PROJECT:     ReactOS 'General' Shim library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test shim used to verify the inner working of the shim engine
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <shimlib.h>
#include <strsafe.h>


#define SHIM_NS         ShimTest
#include <setup_shim.inl>


typedef BOOL (WINAPI *tGetComputerNameA)(LPSTR lpBuffer, LPDWORD lpnSize);

BOOL WINAPI SHIM_OBJ_NAME(GetComputerNameA)(LPSTR lpBuffer, LPDWORD lpnSize)
{
    CHAR Buffer[100] = {0};
    DWORD dwSize = sizeof(Buffer) - 1;
    size_t cchLen;

    if (CALL_SHIM(0, tGetComputerNameA)(Buffer, &dwSize))
    {
        SHIM_INFO("Original function returned: '%s'\n", Buffer);
    }
    else
    {
        SHIM_INFO("Original function failed\n");
        Buffer[0] = '\0';
    }

    if (!lpnSize)
    {
        SHIM_WARN("No lpnSize\n");
        return FALSE;
    }

    StringCchCopyA(lpBuffer, *lpnSize, "ShimTest:");
    StringCchCatA(lpBuffer, *lpnSize, Buffer);
    StringCchLengthA(lpBuffer, *lpnSize, &cchLen);
    *lpnSize = (DWORD)cchLen;
    return TRUE;
}

typedef INT (WINAPI *tSHStringFromGUIDA)(REFGUID guid, LPSTR lpszDest, INT cchMax);
int WINAPI SHIM_OBJ_NAME(SHStringFromGUIDA)(REFGUID guid, LPSTR lpszDest, INT cchMax)
{
    CHAR Buffer[100] = {0};

    if (CALL_SHIM(1, tSHStringFromGUIDA)(guid, Buffer, sizeof(Buffer)-1))
    {
        SHIM_INFO("Original function returned: '%s'\n", Buffer);
    }
    else
    {
        SHIM_INFO("Original function failed\n");
        Buffer[0] = '\0';
    }

    StringCchCopyA(lpszDest, cchMax, "ShimTest:");
    StringCchCatA(lpszDest, cchMax, Buffer);
    return 0;
}


BOOL WINAPI SHIM_OBJ_NAME(Notify)(DWORD fdwReason, PVOID ptr)
{
    if (fdwReason == SHIM_REASON_INIT)
    {
        SHIM_MSG("SHIM_REASON_INIT\n");
    }
    return TRUE;
}

#define SHIM_NOTIFY_FN SHIM_OBJ_NAME(Notify)
#define SHIM_NUM_HOOKS  2
#define SHIM_SETUP_HOOKS \
    SHIM_HOOK(0, "KERNEL32.DLL", "GetComputerNameA", SHIM_OBJ_NAME(GetComputerNameA)) \
    SHIM_HOOK(1, "SHLWAPI.DLL", (PCSTR)23, SHIM_OBJ_NAME(SHStringFromGUIDA))


#include <implement_shim.inl>

