/*
 * PROJECT:   ReactOS RTL
 * LICENSE:   LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:   RtlGetExpWinVer function
 * COPYRIGHT: Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#ifdef _WIN32K_
    #include <win32k.h>

    #define NDEBUG
    #include <debug.h>
#else
    #include <user32.h>
    #include <wine/debug.h>

    WINE_DEFAULT_DEBUG_CHANNEL(user32);
#endif

/* Get the expected OS version from the application module */
ULONG
RtlGetExpWinVer(_In_ PVOID BaseAddress)
{
    ULONG dwMajorVersion = 3, dwMinorVersion = 10; /* Set default to Windows 3.10 */
    PIMAGE_NT_HEADERS pNT;
    ULONG_PTR AlignedAddress = (ULONG_PTR)BaseAddress;

    TRACE("(%p)\n", BaseAddress);

    if (AlignedAddress & 1)
        AlignedAddress = (AlignedAddress & ~1);

    if (AlignedAddress && !LOWORD(AlignedAddress))
    {
        pNT = RtlImageNtHeader((PVOID)AlignedAddress);
        if (pNT)
        {
            dwMajorVersion = pNT->OptionalHeader.MajorSubsystemVersion;
            if (dwMajorVersion == 1)
                dwMajorVersion = 3;
            else
                dwMinorVersion = pNT->OptionalHeader.MinorSubsystemVersion;
        }
    }

    return MAKEWORD(dwMinorVersion, dwMajorVersion);
}
