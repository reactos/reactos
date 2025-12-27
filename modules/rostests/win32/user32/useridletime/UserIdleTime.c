/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Calculate and print user idle time once per second by using
 *              user32!GetLastInputInfo() API and SharedUserData->LastSystemRITEventTickCount
 *              field separately. The former updates in real-time, while the latter updates
 *              periodically (60s in NT5, 1s since NT6.0). Press Ctrl+C to exit this program.
 * COPYRIGHT:   Copyright 2026 Ratin Gao <ratin@knsoft.org>
 */

#include <stdio.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winuser.h>

#define NTOS_MODE_USER
#include <ndk/kefuncs.h>
#include <ndk/psfuncs.h>

int
_cdecl
wmain(
    _In_ int argc,
    _In_reads_(argc) _Pre_z_ wchar_t** argv)
{
    DWORD dwTickCount, dwError;
    LASTINPUTINFO lii = { sizeof(lii) };

    puts("User idle time in second (LastSystemRITEventTickCount, GetLastInputInfo):");
    while (TRUE)
    {
        dwTickCount = GetTickCount();
        if (!GetLastInputInfo(&lii))
        {
            dwError = GetLastError();
            printf("GetLastInputInfo failed with: 0x%08lX\n", dwError);
            return dwError;
        }
        printf("\t%lu, %lu\n",
               (dwTickCount - SharedUserData->LastSystemRITEventTickCount) / 1000UL,
               (dwTickCount - lii.dwTime) / 1000UL);
        Sleep(1000);
    }

    return 0;
}
