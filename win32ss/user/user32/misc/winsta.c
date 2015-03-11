/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/misc/winsta.c
 * PURPOSE:         Window stations
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      04-06-2001  CSH  Created
 */

#include <user32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(winsta);


/*
 * @implemented
 */
HWINSTA WINAPI
CreateWindowStationA(LPCSTR lpwinsta,
                     DWORD dwReserved,
                     ACCESS_MASK dwDesiredAccess,
                     LPSECURITY_ATTRIBUTES lpsa)
{
    UNICODE_STRING WindowStationNameU;
    HWINSTA hWinSta;

    if (lpwinsta)
    {
        /* After conversion, the buffer is zero-terminated */
        RtlCreateUnicodeStringFromAsciiz(&WindowStationNameU, lpwinsta);
    }
    else
    {
        RtlInitUnicodeString(&WindowStationNameU, NULL);
    }

    hWinSta = CreateWindowStationW(WindowStationNameU.Buffer,
                                   dwReserved,
                                   dwDesiredAccess,
                                   lpsa);

    /* Free the string, if it was allocated */
    if (lpwinsta) RtlFreeUnicodeString(&WindowStationNameU);

    return hWinSta;
}


/*
 * @implemented
 */
HWINSTA WINAPI
CreateWindowStationW(LPCWSTR lpwinsta,
                     DWORD dwReserved,
                     ACCESS_MASK dwDesiredAccess,
                     LPSECURITY_ATTRIBUTES lpsa)
{
    UNICODE_STRING WindowStationName;
    UNICODE_STRING WindowStationsDir = RTL_CONSTANT_STRING(L"\\Windows\\WindowStations");
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hWindowStationsDir;
    NTSTATUS Status;
    HWINSTA hwinsta;

    /* Open WindowStations directory */
    InitializeObjectAttributes(&ObjectAttributes,
                               &WindowStationsDir,
                               OBJ_CASE_INSENSITIVE,
                               0,
                               0);

    Status = NtOpenDirectoryObject(&hWindowStationsDir, 
                                   DIRECTORY_CREATE_OBJECT, 
                                   &ObjectAttributes);
    if(!NT_SUCCESS(Status))
    {
        ERR("Failed to open WindowStations directory\n");
        return NULL;
    }

    RtlInitUnicodeString(&WindowStationName, lpwinsta);

    /* Create the window station object */
    InitializeObjectAttributes(&ObjectAttributes,
                               &WindowStationName,
                               OBJ_CASE_INSENSITIVE,
                               hWindowStationsDir,
                               0);

    /* Check if the handle should be inheritable */
    if (lpsa && lpsa->bInheritHandle)
    { 
        ObjectAttributes.Attributes |= OBJ_INHERIT;
    }

    hwinsta = NtUserCreateWindowStation(&ObjectAttributes,
                                        dwDesiredAccess,
                                        0, 0, 0, 0, 0);

    NtClose(hWindowStationsDir);

    return hwinsta;
}

/*
 * Common code for EnumDesktopsA/W and EnumWindowStationsA/W
 */
BOOL FASTCALL
EnumNamesW(HWINSTA WindowStation,
           NAMEENUMPROCW EnumFunc,
           LPARAM Context,
           BOOL Desktops)
{
    CHAR Buffer[256];
    PVOID NameList;
    PWCHAR Name;
    NTSTATUS Status;
    ULONG RequiredSize;
    ULONG CurrentEntry, EntryCount;
    BOOL Ret;

    /* Check parameters */
    if (WindowStation == NULL && Desktops)
    {
        WindowStation = GetProcessWindowStation();
    }

    /* Try with fixed-size buffer */
    Status = NtUserBuildNameList(WindowStation, sizeof(Buffer), Buffer, &RequiredSize);
    if (NT_SUCCESS(Status))
    {
        /* Fixed-size buffer is large enough */
        NameList = (PWCHAR) Buffer;
    }
    else if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        /* Allocate a larger buffer */
        NameList = HeapAlloc(GetProcessHeap(), 0, RequiredSize);
        if (NameList == NULL)
            return FALSE;

        /* Try again */
        Status = NtUserBuildNameList(WindowStation, RequiredSize, NameList, NULL);
        if (!NT_SUCCESS(Status))
        {
            HeapFree(GetProcessHeap(), 0, NameList);
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }
    }
    else
    {
        /* Some unrecognized error occured */
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /* Enum the names one by one */
    EntryCount = *((DWORD *) NameList);
    Name = (PWCHAR) ((PCHAR) NameList + sizeof(DWORD));
    Ret = TRUE;
    for (CurrentEntry = 0; CurrentEntry < EntryCount && Ret; ++CurrentEntry)
    {
        Ret = (*EnumFunc)(Name, Context);
        Name += wcslen(Name) + 1;
    }

    /* Cleanup */
    if (NameList != Buffer)
    {
        HeapFree(GetProcessHeap(), 0, NameList);
    }

    return Ret;
}


/* For W->A conversion */
typedef struct tagENUMNAMESASCIICONTEXT
{
    NAMEENUMPROCA UserEnumFunc;
    LPARAM UserContext;
} ENUMNAMESASCIICONTEXT, *PENUMNAMESASCIICONTEXT;

/*
 * Callback used by Ascii versions. Converts the Unicode name to
 * Ascii and then calls the user callback
 */
BOOL CALLBACK
EnumNamesCallback(LPWSTR Name, LPARAM Param)
{
    PENUMNAMESASCIICONTEXT Context = (PENUMNAMESASCIICONTEXT) Param;
    CHAR FixedNameA[32];
    LPSTR NameA;
    INT Len;
    BOOL Ret;

    /*
     * Determine required size of Ascii string and see
     * if we can use fixed buffer.
     */
    Len = WideCharToMultiByte(CP_ACP, 0, Name, -1, NULL, 0, NULL, NULL);
    if (Len <= 0)
    {
        /* Some strange error occured */
        return FALSE;
    }
    else if (Len <= sizeof(FixedNameA))
    {
        /* Fixed-size buffer is large enough */
        NameA = FixedNameA;
    }
    else
    {
        /* Allocate a larger buffer */
        NameA = HeapAlloc(GetProcessHeap(), 0, Len);
        if (NULL == NameA)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    /* Do the Unicode ->Ascii conversion */
    if (0 == WideCharToMultiByte(CP_ACP, 0, Name, -1, NameA, Len, NULL, NULL))
    {
        /* Something went wrong, clean up */
        if (NameA != FixedNameA)
        {
            HeapFree(GetProcessHeap(), 0, NameA);
        }
        return FALSE;
    }

    /* Call user callback */
    Ret = Context->UserEnumFunc(NameA, Context->UserContext);

    /* Cleanup */
    if (NameA != FixedNameA)
    {
        HeapFree(GetProcessHeap(), 0, NameA);
    }

    return Ret;
}

/*
 * Common code for EnumDesktopsA and EnumWindowStationsA
 */
BOOL FASTCALL
EnumNamesA(HWINSTA WindowStation,
           NAMEENUMPROCA EnumFunc,
           LPARAM Context,
           BOOL Desktops)
{
    ENUMNAMESASCIICONTEXT PrivateContext;

    PrivateContext.UserEnumFunc = EnumFunc;
    PrivateContext.UserContext = Context;

    return EnumNamesW(WindowStation, EnumNamesCallback, (LPARAM) &PrivateContext, Desktops);
}

/*
 * @implemented
 */
BOOL WINAPI
EnumWindowStationsA(WINSTAENUMPROCA EnumFunc,
                    LPARAM Context)
{
    return EnumNamesA(NULL, EnumFunc, Context, FALSE);
}


/*
 * @implemented
 */
BOOL WINAPI
EnumWindowStationsW(WINSTAENUMPROCW EnumFunc,
                    LPARAM Context)
{
    return EnumNamesW(NULL, EnumFunc, Context, FALSE);
}


/*
 * @implemented
 */
HWINSTA WINAPI
OpenWindowStationA(LPCSTR lpszWinSta,
                   BOOL fInherit,
                   ACCESS_MASK dwDesiredAccess)
{
    UNICODE_STRING WindowStationNameU;
    HWINSTA hWinSta;

    if (lpszWinSta)
    {
        /* After conversion, the buffer is zero-terminated */
        RtlCreateUnicodeStringFromAsciiz(&WindowStationNameU, lpszWinSta);
    }
    else
    {
        RtlInitUnicodeString(&WindowStationNameU, NULL);
    }

    hWinSta = OpenWindowStationW(WindowStationNameU.Buffer,
                                 fInherit,
                                 dwDesiredAccess);

    /* Free the string, if it was allocated */
    if (lpszWinSta) RtlFreeUnicodeString(&WindowStationNameU);

    return hWinSta;
}


/*
 * @implemented
 */
HWINSTA WINAPI
OpenWindowStationW(LPCWSTR lpszWinSta,
                   BOOL fInherit,
                   ACCESS_MASK dwDesiredAccess)
{
    UNICODE_STRING WindowStationName;
    UNICODE_STRING WindowStationsDir = RTL_CONSTANT_STRING(L"\\Windows\\WindowStations");
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hWindowStationsDir;
    NTSTATUS Status;
    HWINSTA hwinsta;

    /* Open WindowStations directory */
    InitializeObjectAttributes(&ObjectAttributes,
                               &WindowStationsDir,
                               OBJ_CASE_INSENSITIVE,
                               0,
                               0);

    Status = NtOpenDirectoryObject(&hWindowStationsDir, 
                                   DIRECTORY_TRAVERSE, 
                                   &ObjectAttributes);
    if(!NT_SUCCESS(Status))
    {
        ERR("Failed to open WindowStations directory\n");
        return NULL;
    }

    /* Open the window station object */
    RtlInitUnicodeString(&WindowStationName, lpszWinSta);

    InitializeObjectAttributes(&ObjectAttributes,
                               &WindowStationName,
                               OBJ_CASE_INSENSITIVE,
                               hWindowStationsDir,
                               0);

    if(fInherit)
    {
        ObjectAttributes.Attributes |= OBJ_INHERIT;
    }

    hwinsta = NtUserOpenWindowStation(&ObjectAttributes, dwDesiredAccess);

    NtClose(hWindowStationsDir);

    return hwinsta;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetWindowStationUser(HWINSTA hWindowStation,
                     PLUID pluid,
                     PSID psid,
                     DWORD size)
{
    BOOL Success;

    Success = NtUserSetWindowStationUser(hWindowStation, pluid, psid, size);
    if (Success)
    {
        /* Signal log-on/off to WINSRV */

        /* User is logging on if pluid != LuidNone, otherwise it is a log-off */
        LUID LuidNone = {0, 0};
        BOOL IsLogon = (pluid && !RtlEqualLuid(pluid, &LuidNone));

        Logon(IsLogon);
    }

    return Success;
}

/* EOF */
