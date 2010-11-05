/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/misc/version.c
 * PURPOSE:         Version functions
 * PROGRAMMER:      Ariadne (ariadne@xs4all.nl)
                    Ged Murphy (gedmurphy@reactos.org)
 */

#include <k32.h>
#include <reactos/buildno.h>
#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(kernel32ver);

#define UNICODIZE1(x) L##x
#define UNICODIZE(x) UNICODIZE1(x)

static UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\ReactOS\\Settings\\Version");
static UNICODE_STRING ValName = RTL_CONSTANT_STRING(L"ReportAsWorkstation");

/* FUNCTIONS ******************************************************************/


static VOID
SetRosSpecificInfo(LPOSVERSIONINFOW lpVersionInformation)
{
    PKEY_VALUE_PARTIAL_INFORMATION kvpInfo = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    DWORD ReportAsWorkstation = 0;
    HANDLE hKey;
    DWORD dwSize;
    INT ln, maxlen;
    NTSTATUS Status;

    TRACE("Setting Ros Specific version info\n");

    if (!lpVersionInformation)
        return;

    /* Only the EX version has a product type */
    if (lpVersionInformation->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXW))
    {
        /* Allocate memory for our reg query */
        dwSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD);
        kvpInfo = (PKEY_VALUE_PARTIAL_INFORMATION)HeapAlloc(GetProcessHeap(), 0, dwSize);
        if (!kvpInfo)
            return;

        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        /* Don't change anything if the key doesn't exist */
        Status = NtOpenKey(&hKey,
                           KEY_READ,
                           &ObjectAttributes);
        if (NT_SUCCESS(Status))
        {
            /* Get the value from the registry */
            Status = NtQueryValueKey(hKey,
                                     &ValName,
                                     KeyValuePartialInformation,
                                     kvpInfo,
                                     dwSize,
                                     &dwSize);
            if (NT_SUCCESS(Status))
            {
                /* It should be a DWORD */
                if (kvpInfo->Type == REG_DWORD)
                {
                    /* Copy the value for ease of use */
                    RtlMoveMemory(&ReportAsWorkstation,
                                  kvpInfo->Data,
                                  kvpInfo->DataLength);

                    /* Is the value set? */
                    if (((LPOSVERSIONINFOEXW)lpVersionInformation)->wProductType == VER_NT_SERVER  &&
                        ReportAsWorkstation)
                    {
                        /* It is, modify the product type to report a workstation */
                        ((LPOSVERSIONINFOEXW)lpVersionInformation)->wProductType = VER_NT_WORKSTATION;
                        TRACE("We modified the reported OS from NtProductServer to NtProductWinNt\n");
                    }
                }
            }

            NtClose(hKey);
         }

        HeapFree(GetProcessHeap(), 0, kvpInfo);
    }


    /* Append a reactos specific string to the szCSDVersion string ... very hackish ... */
    /* FIXME: Does anything even use this??? I think not.... - Ged */
    ln = wcslen(lpVersionInformation->szCSDVersion) + 1;
    maxlen = (sizeof(lpVersionInformation->szCSDVersion) / sizeof(lpVersionInformation->szCSDVersion[0]) - 1);
    if(maxlen > ln)
    {
        PWCHAR szVer = lpVersionInformation->szCSDVersion + ln;
        RtlZeroMemory(szVer, (maxlen - ln + 1) * sizeof(WCHAR));
        wcsncpy(szVer,
                L"ReactOS " UNICODIZE(KERNEL_VERSION_STR) L" (Build " UNICODIZE(KERNEL_VERSION_BUILD_STR) L")",
                maxlen - ln);
    }
}


/*
 * @implemented
 */
DWORD
WINAPI
GetVersion(VOID)
{
    PPEB pPeb = NtCurrentPeb();
    DWORD nVersion;

    nVersion = MAKEWORD(pPeb->OSMajorVersion, pPeb->OSMinorVersion);

     /* behave consistently when posing as another operating system build number */
    if(pPeb->OSPlatformId != VER_PLATFORM_WIN32_WINDOWS)
        nVersion |= ((DWORD)(pPeb->OSBuildNumber)) << 16;

    /* non-NT platform flag */
    if(pPeb->OSPlatformId != VER_PLATFORM_WIN32_NT)
        nVersion |= 0x80000000;

    return nVersion;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetVersionExW(LPOSVERSIONINFOW lpVersionInformation)
{
    NTSTATUS Status;

    if(lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOW) &&
       lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXW))
    {
        /* for some reason win sets ERROR_INSUFFICIENT_BUFFER even if it is large
           enough but doesn't match the exact sizes supported, ERROR_INVALID_PARAMETER
           would've been much more appropriate... */
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    Status = RtlGetVersion((PRTL_OSVERSIONINFOW)lpVersionInformation);
    if(NT_SUCCESS(Status))
    {
        /* ReactOS specific changes */
        SetRosSpecificInfo(lpVersionInformation);

        return TRUE;
    }

    return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetVersionExA(LPOSVERSIONINFOA lpVersionInformation)
{
    OSVERSIONINFOEXW viw;

    RtlZeroMemory(&viw, sizeof(viw));

    switch(lpVersionInformation->dwOSVersionInfoSize)
    {
        case sizeof(OSVERSIONINFOA):
            viw.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
            break;

        case sizeof(OSVERSIONINFOEXA):
            viw.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
            break;

     default:
        /* for some reason win sets ERROR_INSUFFICIENT_BUFFER even if it is large
           enough but doesn't match the exact sizes supported, ERROR_INVALID_PARAMETER
           would've been much more appropriate... */
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    if(GetVersionExW((LPOSVERSIONINFOW)&viw))
    {
        ANSI_STRING CSDVersionA;
        UNICODE_STRING CSDVersionW;

        /* copy back fields that match both supported structures */
        lpVersionInformation->dwMajorVersion = viw.dwMajorVersion;
        lpVersionInformation->dwMinorVersion = viw.dwMinorVersion;
        lpVersionInformation->dwBuildNumber = viw.dwBuildNumber;
        lpVersionInformation->dwPlatformId = viw.dwPlatformId;

        /* convert the win version string */
        RtlInitUnicodeString(&CSDVersionW, viw.szCSDVersion);

        CSDVersionA.Length = 0;
        CSDVersionA.MaximumLength = sizeof(lpVersionInformation->szCSDVersion);
        CSDVersionA.Buffer = lpVersionInformation->szCSDVersion;

        RtlUnicodeStringToAnsiString(&CSDVersionA, &CSDVersionW, FALSE);

        /* convert the ReactOS version string */
        CSDVersionW.Buffer = viw.szCSDVersion + CSDVersionW.Length / sizeof(WCHAR) + 1;
        CSDVersionW.MaximumLength = sizeof(viw.szCSDVersion) - (CSDVersionW.Length + sizeof(WCHAR));
        CSDVersionW.Length = wcslen(CSDVersionW.Buffer) * sizeof(WCHAR);
        CSDVersionA.Buffer = lpVersionInformation->szCSDVersion + CSDVersionA.Length + 1;
        CSDVersionA.MaximumLength = sizeof(lpVersionInformation->szCSDVersion) - (CSDVersionA.Length + 1);
        CSDVersionA.Length = 0;

        RtlUnicodeStringToAnsiString(&CSDVersionA, &CSDVersionW, FALSE);

        /* copy back the extended fields */
        if(viw.dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXW))
        {
            ((LPOSVERSIONINFOEXA)lpVersionInformation)->wServicePackMajor = viw.wServicePackMajor;
            ((LPOSVERSIONINFOEXA)lpVersionInformation)->wServicePackMinor = viw.wServicePackMinor;
            ((LPOSVERSIONINFOEXA)lpVersionInformation)->wSuiteMask = viw.wSuiteMask;
            ((LPOSVERSIONINFOEXA)lpVersionInformation)->wProductType = viw.wProductType;
            ((LPOSVERSIONINFOEXA)lpVersionInformation)->wReserved = viw.wReserved;
        }

        return TRUE;
    }

    return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
VerifyVersionInfoW(LPOSVERSIONINFOEXW lpVersionInformation,
                   DWORD dwTypeMask,
                   DWORDLONG dwlConditionMask)
{
    NTSTATUS Status;

    Status = RtlVerifyVersionInfo((PRTL_OSVERSIONINFOEXW)lpVersionInformation,
                                  dwTypeMask,
                                  dwlConditionMask);
    switch(Status)
    {
        case STATUS_INVALID_PARAMETER:
            SetLastError(ERROR_BAD_ARGUMENTS);
            return FALSE;

        case STATUS_REVISION_MISMATCH:
        SetLastError(ERROR_OLD_WIN_VERSION);
        return FALSE;

        default:
            /* RtlVerifyVersionInfo shouldn't report any other failure code! */
            ASSERT(NT_SUCCESS(Status));

            /* ReactOS specific changes */
            SetRosSpecificInfo((LPOSVERSIONINFOW)lpVersionInformation);

            return TRUE;
    }
}


/*
 * @implemented
 */
BOOL
WINAPI
VerifyVersionInfoA(LPOSVERSIONINFOEXA lpVersionInformation,
                   DWORD dwTypeMask,
                   DWORDLONG dwlConditionMask)
{
    OSVERSIONINFOEXW viex;

    viex.dwOSVersionInfoSize = sizeof(viex);
    viex.dwMajorVersion = lpVersionInformation->dwMajorVersion;
    viex.dwMinorVersion = lpVersionInformation->dwMinorVersion;
    viex.dwBuildNumber = lpVersionInformation->dwBuildNumber;
    viex.dwPlatformId = lpVersionInformation->dwPlatformId;
    /* NOTE: szCSDVersion is ignored, we don't need to convert it to unicode */
    viex.wServicePackMajor = lpVersionInformation->wServicePackMajor;
    viex.wServicePackMinor = lpVersionInformation->wServicePackMinor;
    viex.wSuiteMask = lpVersionInformation->wSuiteMask;
    viex.wProductType = lpVersionInformation->wProductType;
    viex.wReserved = lpVersionInformation->wReserved;

    return VerifyVersionInfoW(&viex, dwTypeMask, dwlConditionMask);
}
