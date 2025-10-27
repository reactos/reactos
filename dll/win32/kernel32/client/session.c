/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/client/session.c
 * PURPOSE:         Session Support APIs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Jiayu Ge (yepian@stu2025.jnu.edu.cn)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
DWORD
WINAPI
DosPathToSessionPathW(IN DWORD SessionID,
                      IN LPWSTR InPath,
                      OUT LPWSTR *OutPath)
{
    DWORD LastError = ERROR_SUCCESS;
    if (InPath == NULL)
    {
        LastError = ERROR_INVALID_PARAMETER;
    }
    else
    {
        size_t InPathLength = wcslen(InPath);
        size_t OutPathLength = 0;
        LPWSTR SessionPath = NULL;
        if (!BaseStaticServerData->LUIDDeviceMapsEnabled)
        {
            if (SessionID == 0)
            {
                OutPathLength += wcslen(L"GLOBALROOT\\DosDevices\\");
                OutPathLength += InPathLength;
            }
            else
            {
                OutPathLength += wcslen(L"GLOBALROOT\\Sessions\\");
                OutPathLength += 10; // Length of SessionID(char)
                OutPathLength += wcslen(L"\\DosDevices\\");
                OutPathLength += InPathLength;
            }
        }
        else
        {
            OutPathLength += InPathLength;
        }
        OutPathLength += 1; // \0 termination
        SessionPath = (LPWSTR)LocalAlloc(LMEM_FIXED, OutPathLength * sizeof(WCHAR));
        if (!SessionPath)
        {
            if (BaseStaticServerData->LUIDDeviceMapsEnabled)
            {
                RtlStringCchPrintfW(SessionPath, OutPathLength, L"%ws", InPath);
            }
            else if (SessionID == 0)
            { 
                // GLOBALROOT\DosDevices\$InPath
                RtlStringCchPrintfW(SessionPath, OutPathLength, L"GLOBALROOT\\DosDevices\\%ws", InPath);
            }
            else
            {
                // GLOBALROOT\Sessions\$SessionID\DosDevices\$InPath
                RtlStringCchPrintfW(SessionPath, OutPathLength, L"GLOBALROOT\\Sessions\\%u\\DosDevices\\%ws", SessionID, InPath);
            }
            *OutPath = SessionPath;
            return 1;
        }
        else
        {
            LastError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    RtlSetLastWin32Error(LastError);
    return 0;
}

/*
 * @implemented
 */
DWORD
WINAPI
DosPathToSessionPathA(IN DWORD SessionID,
                      IN LPSTR InPath,
                      OUT LPSTR *OutPath)
{
    DWORD LastError = ERROR_SUCCESS;
    if (InPath == NULL)
    {
        LastError = ERROR_INVALID_PARAMETER;
    }
    else
    { // AnsiPath: InPath(ANSI) -> UnicodePath: InPath(Unicode) -> wcOutPath: OutPath(wide-char) -> UnicodePath: OutPath(Unicode) -> AnsiPath: OutPath(ANSI)
        STRING AnsiPath = { 0 };
        RtlInitAnsiString(&AnsiPath, InPath);
        UNICODE_STRING UnicodePath;
        NTSTATUS status = RtlAnsiStringToUnicodeString(&UnicodePath, &AnsiPath, TRUE); // UnicodePath = InPath(Unicode)
        if (!NT_SUCCESS(status))
        {
            BaseSetLastNTError(status);
            return 0;
        }
        LPWSTR wcOutPath = NULL; // OutPath(wide-char)
        if (!DosPathToSessionPathW(SessionID, UnicodePath.Buffer, &wcOutPath))
        {
            RtlFreeUnicodeString(&UnicodePath);
            return 0;
        }
        RtlFreeUnicodeString(&UnicodePath);
        RtlInitUnicodeString(&UnicodePath, wcOutPath); // UnicodePath = OutPath(Unicode)
        status = RtlUnicodeStringToAnsiString(&AnsiPath, &UnicodePath, TRUE);
        if (!NT_SUCCESS(status))
        {
            BaseSetLastNTError(status);
            LocalFree(wcOutPath);
            return 0;
        }
        size_t AnsiOutPathLen = strlen(AnsiPath.Buffer);
        AnsiOutPathLen += 1;
        char *SessionPath = LocalAlloc(LMEM_FIXED, AnsiOutPathLen * sizeof(CHAR));
        if (SessionPath != NULL)
        {
            RtlStringCchCopyA(SessionPath, AnsiOutPathLen, AnsiPath.Buffer);
            *OutPath = SessionPath;
            LocalFree(wcOutPath);
            RtlFreeAnsiString(&AnsiPath);
            return 1;
        }
        else
        {
            LocalFree(wcOutPath);
            RtlFreeAnsiString(&AnsiPath);
            LastError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    RtlSetLastWin32Error(LastError);
    return 0;
}

/*
 * @implemented
 */
DWORD
WINAPI
WTSGetActiveConsoleSessionId(VOID)
{
    return SharedUserData->ActiveConsoleId;
}

/* EOF */
