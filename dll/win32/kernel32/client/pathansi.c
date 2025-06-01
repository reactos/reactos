
/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/client/path.c
 * PURPOSE:         Handles path APIs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PRTL_CONVERT_STRING Basep8BitStringToUnicodeString = RtlAnsiStringToUnicodeString;
PRTL_CONVERT_STRINGA BasepUnicodeStringTo8BitString = RtlUnicodeStringToAnsiString;
PRTL_COUNT_STRING BasepUnicodeStringTo8BitSize = BasepUnicodeStringToAnsiSize;
PRTL_COUNT_STRINGA Basep8BitStringToUnicodeSize = BasepAnsiStringToUnicodeSize;

ULONG
NTAPI
BasepUnicodeStringToOemSize(IN PUNICODE_STRING String)
{
    return RtlUnicodeStringToOemSize(String);
}

ULONG
NTAPI
BasepOemStringToUnicodeSize(IN PANSI_STRING String)
{
    return RtlOemStringToUnicodeSize(String);
}

ULONG
NTAPI
BasepUnicodeStringToAnsiSize(IN PUNICODE_STRING String)
{
    return RtlUnicodeStringToAnsiSize(String);
}

ULONG
NTAPI
BasepAnsiStringToUnicodeSize(IN PANSI_STRING String)
{
    return RtlAnsiStringToUnicodeSize(String);
}

/* PRIVATE FUNCTIONS **********************************************************/

/*
 * @implemented
 */
DWORD
WINAPI
GetShortPathNameA(IN LPCSTR lpszLongPath,
                  OUT LPSTR lpszShortPath,
                  IN DWORD cchBuffer)
{
    ULONG Result, PathLength;
    PWCHAR ShortPath;
    NTSTATUS Status;
    UNICODE_STRING LongPathUni, ShortPathUni;
    ANSI_STRING ShortPathAnsi;
    WCHAR ShortPathBuffer[MAX_PATH];

    ShortPath = NULL;
    ShortPathAnsi.Buffer = NULL;
    LongPathUni.Buffer = NULL;
    Result = 0;

    if (!lpszLongPath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    Status = Basep8BitStringToDynamicUnicodeString(&LongPathUni, lpszLongPath);
    if (!NT_SUCCESS(Status)) goto Quickie;

    ShortPath = ShortPathBuffer;

    PathLength = GetShortPathNameW(LongPathUni.Buffer, ShortPathBuffer, MAX_PATH);
    if (PathLength >= MAX_PATH)
    {
        ShortPath = RtlAllocateHeap(RtlGetProcessHeap(), 0, PathLength * sizeof(WCHAR));
        if (!ShortPath)
        {
            PathLength = 0;
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
        else
        {
            PathLength = GetShortPathNameW(LongPathUni.Buffer, ShortPath, PathLength);
        }
    }

    if (!PathLength) goto Quickie;

    LongPathUni.MaximumLength = (USHORT)PathLength * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    ShortPathUni.Buffer = ShortPath;
    ShortPathUni.Length = (USHORT)PathLength * sizeof(WCHAR);

    Status = BasepUnicodeStringTo8BitString(&ShortPathAnsi, &ShortPathUni, TRUE);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        Result = 0;
    }

    Result = ShortPathAnsi.Length;
    if ((lpszShortPath) && (cchBuffer > ShortPathAnsi.Length))
    {
        RtlMoveMemory(lpszShortPath, ShortPathAnsi.Buffer, ShortPathAnsi.Length);
        lpszShortPath[Result] = ANSI_NULL;
    }
    else
    {
        Result = ShortPathAnsi.Length + sizeof(ANSI_NULL);
    }

Quickie:
    if (LongPathUni.Buffer) RtlFreeUnicodeString(&LongPathUni);
    if (ShortPathAnsi.Buffer) RtlFreeAnsiString(&ShortPathAnsi);
    if ((ShortPath) && (ShortPath != ShortPathBuffer))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, ShortPath);
    }
    return Result;
}

