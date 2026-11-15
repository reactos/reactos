/*
 * PROJECT:     VerOS NT System Libraries
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        dll/ntdll/rtl/version.c
 * PURPOSE:     Process functions
 * COPYRIGHT:   See COPYING in the top level directory
 *              Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ariadne (ariadne@xs4all.nl)
 *              Ryan Wilson <ryan@verosnt.com>
 */

/*
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *******************************************************************/

#include <ntdll.h>
#include <reactos/runtimever.h>

#define NDEBUG
#include <debug.h>

C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved2) + (6 * sizeof(ULONG)) ==
         RUNTIME_NT_SHARED_BUILD_OFFSET);

/* FUNCTIONS ******************************************************************/

static signed char g_ReportProductType = 0;

static ULONG
RtlpSharedBuildNumber(VOID)
{
    return *(PULONG)((PUCHAR)SharedUserData + RUNTIME_NT_SHARED_BUILD_OFFSET);
}

/* HACK: ReactOS specific changes, see bug-reports CORE-6611 and CORE-4620 (aka. #5003) */
static VOID NTAPI
SetRosSpecificInfo(IN OUT PRTL_OSVERSIONINFOEXW VersionInformation)
{
    CHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION kvpInfo = (PVOID)Buffer;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hKey;
    ULONG Length;
    NTSTATUS Status;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\ReactOS\\Settings\\Version");
    UNICODE_STRING ValName = RTL_CONSTANT_STRING(L"ReportAsWorkstation");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Don't change anything if the key doesn't exist */
    Status = (g_ReportProductType == 0) ? NtOpenKey(&hKey, KEY_READ, &ObjectAttributes) : STATUS_CANCELLED;
    if (NT_SUCCESS(Status))
    {
        /* Get the value from the registry and make sure it's a 32-bit value */
        Status = NtQueryValueKey(hKey,
                                 &ValName,
                                 KeyValuePartialInformation,
                                 kvpInfo,
                                 sizeof(Buffer),
                                 &Length);
        if (NT_SUCCESS(Status) &&
            (kvpInfo->Type == REG_DWORD) &&
            (kvpInfo->DataLength == sizeof(ULONG)))
        {
            BOOLEAN IsWorkstation = SharedUserData->NtProductType == NtProductWinNt;
            BOOLEAN ReportAsWorkstation = (*(PULONG)kvpInfo->Data) != 0;
            if (IsWorkstation != ReportAsWorkstation)
            {
                g_ReportProductType = ReportAsWorkstation ? VER_NT_WORKSTATION : VER_NT_SERVER;
            }
        }

        /* Close the handle */
        NtClose(hKey);
    }

    if (g_ReportProductType > 0)
    {
        VersionInformation->wProductType = g_ReportProductType;
        DPRINT("We modified the reported OS product type from %d to %d\n",
                SharedUserData->NtProductType, VersionInformation->wProductType);
    }
    else
    {
        g_ReportProductType = -1; /* No override, caller gets the real value */
    }
}

/**********************************************************************
 * NAME                         EXPORTED
 *  RtlGetNtProductType
 *
 * DESCRIPTION
 *  Retrieves the OS product type.
 *
 * ARGUMENTS
 *  ProductType Pointer to the product type variable.
 *
 * RETURN VALUE
 *  TRUE if successful, otherwise FALSE
 *
 * NOTE
 *  ProductType can be one of the following values:
 *    1 Workstation (WinNT)
 *    2 Server (LanmanNT)
 *    3 Advanced Server (ServerNT)
 *
 * REVISIONS
 *  2000-08-10 ekohl
 *
 * @implemented
 */
BOOLEAN NTAPI
RtlGetNtProductType(_Out_ PNT_PRODUCT_TYPE ProductType)
{
    *ProductType = SharedUserData->NtProductType;

    if (g_ReportProductType == 0)
    {
        /* Initialize cached value */
        RTL_OSVERSIONINFOEXW ovi;
        ovi.dwOSVersionInfoSize = sizeof(ovi);
        ovi.wProductType = *ProductType;
        SetRosSpecificInfo(&ovi);
    }

    if (g_ReportProductType > 0)
    {
        *ProductType = g_ReportProductType;
        DPRINT("Overriding RtlGetNtProductType to return %d\n", *ProductType);
    }
    return TRUE;
}

/**********************************************************************
 * NAME                         EXPORTED
 *  RtlGetNtVersionNumbers
 *
 * DESCRIPTION
 *  Get the version numbers of the run time library.
 *
 * ARGUMENTS
 *  pMajorVersion [OUT] Destination for the Major version
 *  pMinorVersion [OUT] Destination for the Minor version
 *  pBuildNumber  [OUT] Destination for the Build version
 *
 * RETURN VALUE
 *  Nothing.
 *
 * NOTES
 *  - Introduced in Windows XP (NT 5.1)
 *  - Since this call didn't exist before XP, we report at least the version
 *    5.1. This fixes the loading of msvcrt.dll as released with XP Home,
 *    which fails in DLLMain() if the major version isn't 5.
 *
 * @implemented
 */
VOID NTAPI
RtlGetNtVersionNumbers(OUT PULONG pMajorVersion,
                       OUT PULONG pMinorVersion,
                       OUT PULONG pBuildNumber)
{
    ULONG MajorVersion = SharedUserData->NtMajorVersion;
    ULONG MinorVersion = SharedUserData->NtMinorVersion;

    if (pMajorVersion)
    {
        *pMajorVersion = MajorVersion < 5 ? 5 : MajorVersion;
    }

    if (pMinorVersion)
    {
        if ((MajorVersion < 5) ||
            ((MajorVersion == 5) && (MinorVersion < 1)))
            *pMinorVersion = 1;
        else
            *pMinorVersion = MinorVersion;
    }

    if (pBuildNumber)
    {
        /* Windows really does this! */
        *pBuildNumber = (0xF0000000 | RtlpSharedBuildNumber());
    }
}

/*
 * @implemented
 * @note User-mode version of RtlGetVersion in ntoskrnl/rtl/misc.c
 */
NTSTATUS NTAPI
RtlGetVersion(IN OUT PRTL_OSVERSIONINFOW lpVersionInformation)
{
    SIZE_T Length;
    PPEB Peb = NtCurrentPeb();

    if (lpVersionInformation->dwOSVersionInfoSize != sizeof(RTL_OSVERSIONINFOW) &&
        lpVersionInformation->dwOSVersionInfoSize != sizeof(RTL_OSVERSIONINFOEXW))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* The real version. GetVersion reads the PEB, which may be the 6.2 lie. */
    lpVersionInformation->dwMajorVersion = SharedUserData->NtMajorVersion;
    lpVersionInformation->dwMinorVersion = SharedUserData->NtMinorVersion;
    lpVersionInformation->dwBuildNumber = RtlpSharedBuildNumber();
    lpVersionInformation->dwPlatformId = VER_PLATFORM_WIN32_NT;
    RtlZeroMemory(lpVersionInformation->szCSDVersion, sizeof(lpVersionInformation->szCSDVersion));

    /* If we have a CSD version string, initialized by Application Compatibility... */
    if (Peb->CSDVersion.Length && Peb->CSDVersion.Buffer && Peb->CSDVersion.Buffer[0] != UNICODE_NULL)
    {
        /* ... copy it... */
        Length = min(wcslen(Peb->CSDVersion.Buffer), ARRAYSIZE(lpVersionInformation->szCSDVersion) - 1);
        wcsncpy(lpVersionInformation->szCSDVersion, Peb->CSDVersion.Buffer, Length);
    }
    else
    {
        /* ... otherwise we just null-terminate it */
        Length = 0;
    }

    /* Always null-terminate the user CSD version string */
    lpVersionInformation->szCSDVersion[Length] = UNICODE_NULL;

    if (lpVersionInformation->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
    {
        PRTL_OSVERSIONINFOEXW InfoEx = (PRTL_OSVERSIONINFOEXW)lpVersionInformation;
        InfoEx->wServicePackMajor = (Peb->OSCSDVersion >> 8) & 0xFF;
        InfoEx->wServicePackMinor = Peb->OSCSDVersion & 0xFF;
        InfoEx->wSuiteMask = SharedUserData->SuiteMask & 0xFFFF;
        InfoEx->wProductType = SharedUserData->NtProductType;
        InfoEx->wReserved = 0;

        /* HACK: ReactOS specific changes, see bug-reports CORE-6611 and CORE-4620 (aka. #5003) */
        SetRosSpecificInfo(InfoEx);
    }

    return STATUS_SUCCESS;
}

/* EOF */
