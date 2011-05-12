/*
 * Copyright 2011 Samuel Serapion
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
 *
 */

#include "ntlm.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntlm);


PVOID
NtlmAllocate(IN ULONG Size)
{
    PVOID buffer = NULL;

    if(Size == 0)
    {
        ERR("Allocating 0 bytes!\n");
        return NULL;
    }

    switch(NtlmMode)
    {
        case NtlmLsaMode:
            buffer = NtlmLsaFuncTable->AllocateLsaHeap(Size);
            if (buffer != NULL)
                RtlZeroMemory(buffer, Size);
            break;
        case NtlmUserMode:
            buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
            break;
        default:
            ERR("NtlmState unknown!\n");
            break;
    }
    return buffer;
}

VOID
NtlmFree(IN PVOID Buffer)
{
    if (Buffer)
    {
        switch (NtlmMode)
        {
            case NtlmLsaMode:
                NtlmLsaFuncTable->FreeLsaHeap(Buffer);
                break;
            case NtlmUserMode:
                HeapFree(GetProcessHeap(),0,Buffer);
                break;
            default:
                ERR("NtlmState unknown!\n");
                break;
        }
    }
    else
    {
        ERR("Trying to free NULL!\n");
    }
}

BOOLEAN
NtlmIntervalElapsed(IN LARGE_INTEGER Start,IN LONG Timeout)
{
    LARGE_INTEGER now;
    LARGE_INTEGER elapsed;
    LARGE_INTEGER interval;

    /* timeout is never */
    if (Timeout > 0xffffffff)
        return FALSE;

    /* get current time */
    NtQuerySystemTime(&now);
    elapsed.QuadPart = now.QuadPart - Start.QuadPart;

    /* convert from milliseconds into 100ns */
    interval.QuadPart = Int32x32To64(Timeout, 10000);

    /* time overflowed or elapsed is greater than interval */
    if (elapsed.QuadPart < 0 || elapsed.QuadPart > interval.QuadPart )
        return TRUE;

    return FALSE;
}

/* hack: see dllmain.c */
/* from base/services/umpnpmgr/umpnpmgr.c */
BOOL
SetupIsActive(VOID)
{
    HKEY hKey = NULL;
    DWORD regType, active, size;
    LONG rc;
    BOOL ret = FALSE;

    rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\Setup", 0, KEY_QUERY_VALUE, &hKey);
    if (rc != ERROR_SUCCESS)
        goto cleanup;

    size = sizeof(DWORD);
    rc = RegQueryValueExW(hKey, L"SystemSetupInProgress", NULL, &regType, (LPBYTE)&active, &size);
    if (rc != ERROR_SUCCESS)
        goto cleanup;
    if (regType != REG_DWORD || size != sizeof(DWORD))
        goto cleanup;

    ret = (active != 0);

cleanup:
    if (hKey != NULL)
        RegCloseKey(hKey);

    TRACE("System setup in progress? %S\n", ret ? L"YES" : L"NO");

   return ret;
}
