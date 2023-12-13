/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Getting OS information
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

/* The flags of GetOSInfo() */
#define OSINFO_NT    0x01
#define OSINFO_CJK   0x10
#define OSINFO_IMM   0x20
#define OSINFO_DBCS  0x40

static inline DWORD
GetOSInfo(VOID)
{
    DWORD dwOsInfo = 0;

    /* Check OS version info */
    OSVERSIONINFOW VerInfo = { sizeof(VerInfo) };
    GetVersionExW(&VerInfo);
    if (VerInfo.dwPlatformId == DLLVER_PLATFORM_NT)
        dwOsInfo |= OSINFO_NT;

    /* Check codepage */
    switch (GetACP())
    {
        case 932: /* Japanese (Japan) */
        case 936: /* Chinese (PRC, Singapore) */
        case 949: /* Korean (Korea) */
        case 950: /* Chinese (Taiwan, Hong Kong) */
            dwOsInfo |= OSINFO_CJK;
            break;
    }

    if (GetSystemMetrics(SM_IMMENABLED))
        dwOsInfo |= OSINFO_IMM;

    if (GetSystemMetrics(SM_DBCSENABLED))
        dwOsInfo |= OSINFO_DBCS;

    /* I'm not interested in other flags */

    return dwOsInfo;
}
