/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/process.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Ariadne (ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *******************************************************************/

#include <ntdll.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/* HACK: ReactOS specific changes, see bug-reports CORE-6611 and CORE-4620 (aka. #5003) */
static VOID NTAPI
SetRosSpecificInfo(IN OUT PRTL_OSVERSIONINFOEXW VersionInformation)
{
    CHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION kvpInfo = (PVOID)Buffer;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG ReportAsWorkstation = 0;
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
    Status = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);
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
            /* Is the value set? */
            ReportAsWorkstation = *(PULONG)kvpInfo->Data;
            if ((VersionInformation->wProductType == VER_NT_SERVER) &&
                (ReportAsWorkstation != 0))
            {
                /* It is, modify the product type to report a workstation */
                VersionInformation->wProductType = VER_NT_WORKSTATION;
                DPRINT1("We modified the reported OS from NtProductServer to NtProductWinNt\n");
            }
        }

        /* Close the handle */
        NtClose(hKey);
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
RtlGetNtProductType(PNT_PRODUCT_TYPE ProductType)
{
    *ProductType = SharedUserData->NtProductType;
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
    PPEB pPeb = NtCurrentPeb();

    if (pMajorVersion)
    {
        *pMajorVersion = pPeb->OSMajorVersion < 5 ? 5 : pPeb->OSMajorVersion;
    }

    if (pMinorVersion)
    {
        if ( (pPeb->OSMajorVersion  < 5) ||
            ((pPeb->OSMajorVersion == 5) && (pPeb->OSMinorVersion < 1)) )
            *pMinorVersion = 1;
        else
            *pMinorVersion = pPeb->OSMinorVersion;
    }

    if (pBuildNumber)
    {
        /* Windows really does this! */
        *pBuildNumber = (0xF0000000 | pPeb->OSBuildNumber);
    }
}

/*
 * @implemented
 * @note User-mode version of RtlGetVersion in ntoskrnl/rtl/misc.c
 */
NTSTATUS NTAPI
RtlGetVersion(IN OUT PRTL_OSVERSIONINFOW lpVersionInformation)
{
    ULONG Length;
    PPEB Peb = NtCurrentPeb();

    if (lpVersionInformation->dwOSVersionInfoSize != sizeof(RTL_OSVERSIONINFOW) &&
        lpVersionInformation->dwOSVersionInfoSize != sizeof(RTL_OSVERSIONINFOEXW))
    {
        return STATUS_INVALID_PARAMETER;
    }

    lpVersionInformation->dwMajorVersion = Peb->OSMajorVersion;
    lpVersionInformation->dwMinorVersion = Peb->OSMinorVersion;
    lpVersionInformation->dwBuildNumber = Peb->OSBuildNumber;
    lpVersionInformation->dwPlatformId = Peb->OSPlatformId;
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
