/*
 * PROJECT:         ReactOS Kernel - Vista+ APIs
 * LICENSE:         GPL v2 - See COPYING in the top level directory
 * FILE:            lib/drivers/ntoskrnl_vista/rtl.c
 * PURPOSE:         Implementation of RtlIsNtDdiVersionAvailable and RtlIsServicePackVersionInstalled 
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include <ntdef.h>
#include <ntifs.h>

BOOLEAN
WdmlibRtlIsNtDdiVersionAvailable(
    _In_ ULONG Version)
{
    UNICODE_STRING ImportName;
    ULONG Major, Minor, Current;
    BOOLEAN (NTAPI *pRtlIsNtDdiVersionAvailable)(ULONG Version);

    /* Try to use ntoskrnl version if available */
    RtlInitUnicodeString(&ImportName, L"RtlIsNtDdiVersionAvailable");
    pRtlIsNtDdiVersionAvailable = MmGetSystemRoutineAddress(&ImportName);
    if (pRtlIsNtDdiVersionAvailable)
    {
        return pRtlIsNtDdiVersionAvailable(Version);
    }

    /* Only provide OS version. No SP */
    if (SPVER(Version) || SUBVER(Version))
    {
        return FALSE;
    }

    /* Compute the version and compare */
    Major = 0;
    Minor = 0;
    PsGetVersion(&Major, &Minor, NULL, NULL);
    Current = (Minor + (Major << 8)) << 16;

    return (Current >= Version);
}
