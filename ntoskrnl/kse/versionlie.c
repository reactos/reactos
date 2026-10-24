/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     KSE 'VersionLie' shim implementation
 * COPYRIGHT:   Copyright 2026 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2026 Adam Słaboń (asaillen@protonmail.com)
 */

/*
 * Some drivers refuse to load when the kernel reports a version that they do
 * not know about. For example, DriverEntry of the XP / 2003 x64 NVIDIA driver
 * (nv4_mini.sys) fails when PsGetVersion returns a major version above 5.
 * These shims report a fixed version to the shimmed driver.
 */

#include <ntoskrnl.h>
#include <initguid.h>

#define NDEBUG
#include <debug.h>

DEFINE_GUID(KseWin2k3VersionLieGuid, 0xb120afe6, 0x7bdd, 0x4672, 0xa9, 0x90, 0x00, 0xb1, 0x3d, 0x50, 0x78, 0x05);

typedef struct _KSE_VERSION_LIE
{
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG BuildNumber;
    USHORT ServicePackMajor;
    UNICODE_STRING CSDVersion;
} KSE_VERSION_LIE, *PKSE_VERSION_LIE;

static KSE_VERSION_LIE KseWin2k3VersionLie = { 5, 2, 3790, 2, RTL_CONSTANT_STRING(L"Service Pack 2") };

static NTSTATUS
KseCommonRtlGetVersion(
    IN OUT PRTL_OSVERSIONINFOW lpVersionInformation,
    IN PKSE_VERSION_LIE VersionLie)
{
    PRTL_OSVERSIONINFOEXW InfoEx;
    NTSTATUS Status;

    /* Let the kernel fill in the fields that are not faked */
    Status = RtlGetVersion(lpVersionInformation);
    if (!NT_SUCCESS(Status))
        return Status;

    lpVersionInformation->dwMajorVersion = VersionLie->MajorVersion;
    lpVersionInformation->dwMinorVersion = VersionLie->MinorVersion;
    lpVersionInformation->dwBuildNumber = VersionLie->BuildNumber;
    lpVersionInformation->dwPlatformId = VER_PLATFORM_WIN32_NT;

    RtlZeroMemory(lpVersionInformation->szCSDVersion, sizeof(lpVersionInformation->szCSDVersion));
    RtlCopyMemory(lpVersionInformation->szCSDVersion,
                  VersionLie->CSDVersion.Buffer,
                  min(VersionLie->CSDVersion.Length, sizeof(lpVersionInformation->szCSDVersion) - sizeof(WCHAR)));

    if (lpVersionInformation->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
    {
        InfoEx = (PRTL_OSVERSIONINFOEXW)lpVersionInformation;
        InfoEx->wServicePackMajor = VersionLie->ServicePackMajor;
        InfoEx->wServicePackMinor = 0;
    }

    return STATUS_SUCCESS;
}

static BOOLEAN
KseCommonPsGetVersion(
    OUT PULONG MajorVersion OPTIONAL,
    OUT PULONG MinorVersion OPTIONAL,
    OUT PULONG BuildNumber OPTIONAL,
    OUT PUNICODE_STRING CSDVersion OPTIONAL,
    IN PKSE_VERSION_LIE VersionLie)
{
    if (MajorVersion) *MajorVersion = VersionLie->MajorVersion;
    if (MinorVersion) *MinorVersion = VersionLie->MinorVersion;
    if (BuildNumber ) *BuildNumber  = VersionLie->BuildNumber;
    if (CSDVersion  ) *CSDVersion   = VersionLie->CSDVersion;

    /* Still report whether this is a checked build */
    return PsGetVersion(NULL, NULL, NULL, NULL);
}

static NTSTATUS
NTAPI
KseWin2k3RtlGetVersion(IN OUT PRTL_OSVERSIONINFOW lpVersionInformation)
{
    return KseCommonRtlGetVersion(lpVersionInformation, &KseWin2k3VersionLie);
}

static BOOLEAN
NTAPI
KseWin2k3PsGetVersion(OUT PULONG MajorVersion OPTIONAL,
                      OUT PULONG MinorVersion OPTIONAL,
                      OUT PULONG BuildNumber  OPTIONAL,
                      OUT PUNICODE_STRING CSDVersion OPTIONAL)
{
    return KseCommonPsGetVersion(MajorVersion, MinorVersion, BuildNumber, CSDVersion, &KseWin2k3VersionLie);
}

static KSE_HOOK KseWin2k3VersionLieHooksNT[] = {
    { KseHookFunction, { "RtlGetVersion" }, (PVOID)(ULONG_PTR)KseWin2k3RtlGetVersion },
    { KseHookFunction, { "PsGetVersion" }, (PVOID)(ULONG_PTR)KseWin2k3PsGetVersion },
    { KseHookInvalid }
};

static KSE_HOOK_COLLECTION KseWin2k3VersionLieShimCollections[] = {
    { KseCollectionNtExport, NULL, KseWin2k3VersionLieHooksNT },
    { KseCollectionInvalid },
};

static KSE_SHIM KseVersionLieShims[] = {
    { sizeof(KSE_SHIM), &KseWin2k3VersionLieGuid, L"KmWin2k3VersionLie", NULL, NULL, NULL, KseWin2k3VersionLieShimCollections },
};

CODE_SEG("INIT")
NTSTATUS
NTAPI
KseVersionLieInitialize(VOID)
{
    NTSTATUS Status, Result = STATUS_SUCCESS;
    ULONG n;

    for (n = 0; n < RTL_NUMBER_OF(KseVersionLieShims); n++)
    {
        Status = KseRegisterShim(&KseVersionLieShims[n], NULL, 0);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("KSE: Unable to register %S shim (0x%lx)\n", KseVersionLieShims[n].ShimName, Status);
            Result = Status;
        }
    }

    return Result;
}
