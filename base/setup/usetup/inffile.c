/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/inffile.c
 * PURPOSE:         .inf files support functions
 * PROGRAMMERS:     Hervé Poussineau
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES ******************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* SETUP* API COMPATIBILITY FUNCTIONS ****************************************/

/* Functions from the INFLIB library */
#ifdef __REACTOS__

extern VOID InfCloseFile(HINF InfHandle);
// #define SetupCloseInfFile InfCloseFile
VOID
WINAPI
SetupCloseInfFile(HINF InfHandle)
{
    InfCloseFile(InfHandle);
}

// #define SetupFindFirstLineW InfpFindFirstLineW
BOOL
WINAPI
SetupFindFirstLineW(
    IN HINF InfHandle,
    IN PCWSTR Section,
    IN PCWSTR Key,
    IN OUT PINFCONTEXT Context)
{
    PINFCONTEXT pContext;
    BOOL ret;

    ret = InfFindFirstLine(InfHandle, Section, Key, &pContext);
    if (!ret)
        return FALSE;

    memcpy(Context, pContext, sizeof(INFCONTEXT));
    InfFreeContext(pContext);
    return TRUE;
}

extern BOOLEAN InfFindNextLine(PINFCONTEXT ContextIn,
                               PINFCONTEXT ContextOut);
// #define SetupFindNextLine InfFindNextLine
BOOL
WINAPI
SetupFindNextLine(PINFCONTEXT ContextIn,
                  PINFCONTEXT ContextOut)
{
    return !!InfFindNextLine(ContextIn, ContextOut);
}

extern LONG InfGetFieldCount(PINFCONTEXT Context);
// #define SetupGetFieldCount InfGetFieldCount
LONG
WINAPI
SetupGetFieldCount(PINFCONTEXT Context)
{
    return InfGetFieldCount(Context);
}

extern BOOLEAN InfGetIntField(PINFCONTEXT Context,
                              ULONG FieldIndex,
                              INT *IntegerValue);
// #define SetupGetIntField InfGetIntField
BOOLEAN
WINAPI
SetupGetIntField(PINFCONTEXT Context,
                 ULONG FieldIndex,
                 INT *IntegerValue)
{
    return InfGetIntField(Context, FieldIndex, IntegerValue);
}

extern BOOLEAN InfGetBinaryField(PINFCONTEXT Context,
                                 ULONG FieldIndex,
                                 PUCHAR ReturnBuffer,
                                 ULONG ReturnBufferSize,
                                 PULONG RequiredSize);
// #define SetupGetBinaryField InfGetBinaryField
BOOL
WINAPI
SetupGetBinaryField(PINFCONTEXT Context,
                    ULONG FieldIndex,
                    PUCHAR ReturnBuffer,
                    ULONG ReturnBufferSize,
                    PULONG RequiredSize)
{
    return !!InfGetBinaryField(Context,
                               FieldIndex,
                               ReturnBuffer,
                               ReturnBufferSize,
                               RequiredSize);
}

extern BOOLEAN InfGetMultiSzField(PINFCONTEXT Context,
                                  ULONG FieldIndex,
                                  PWSTR ReturnBuffer,
                                  ULONG ReturnBufferSize,
                                  PULONG RequiredSize);
// #define SetupGetMultiSzFieldW InfGetMultiSzField
BOOL
WINAPI
SetupGetMultiSzFieldW(PINFCONTEXT Context,
                      ULONG FieldIndex,
                      PWSTR ReturnBuffer,
                      ULONG ReturnBufferSize,
                      PULONG RequiredSize)
{
    return !!InfGetMultiSzField(Context,
                                FieldIndex,
                                ReturnBuffer,
                                ReturnBufferSize,
                                RequiredSize);
}

extern BOOLEAN InfGetStringField(PINFCONTEXT Context,
                                 ULONG FieldIndex,
                                 PWSTR ReturnBuffer,
                                 ULONG ReturnBufferSize,
                                 PULONG RequiredSize);
// #define SetupGetStringFieldW InfGetStringField
BOOL
WINAPI
SetupGetStringFieldW(PINFCONTEXT Context,
                     ULONG FieldIndex,
                     PWSTR ReturnBuffer,
                     ULONG ReturnBufferSize,
                     PULONG RequiredSize)
{
    return !!InfGetStringField(Context,
                               FieldIndex,
                               ReturnBuffer,
                               ReturnBufferSize,
                               RequiredSize);
}


/* SetupOpenInfFileW with support for a user-provided LCID */
// #define SetupOpenInfFileExW InfpOpenInfFileW
HINF
WINAPI
SetupOpenInfFileExW(
    IN PCWSTR FileName,
    IN PCWSTR InfClass,
    IN DWORD InfStyle,
    IN LCID LocaleId,
    OUT PUINT ErrorLine)
{
    HINF hInf = NULL;
    UNICODE_STRING FileNameU;
    ULONG ErrorLineUL;
    NTSTATUS Status;

    RtlInitUnicodeString(&FileNameU, FileName);
    Status = InfOpenFile(&hInf,
                         &FileNameU,
                         LANGIDFROMLCID(LocaleId),
                         &ErrorLineUL);
    *ErrorLine = (UINT)ErrorLineUL;
    if (!NT_SUCCESS(Status))
        return INVALID_HANDLE_VALUE;

    return hInf;
}

#endif /* __REACTOS__ */


/* HELPER FUNCTIONS **********************************************************/

BOOLEAN
INF_GetData(
    IN PINFCONTEXT Context,
    OUT PWCHAR *Key,
    OUT PWCHAR *Data)
{
#ifdef __REACTOS__
    return InfGetData(Context, Key, Data);
#else
    static PWCHAR pLastCallData[4] = { NULL, NULL, NULL, NULL };
    static DWORD currentIndex = 0;
    DWORD dwSize, i;
    BOOL ret;

    currentIndex ^= 2;

    if (Key)
        *Key = NULL;

    if (Data)
        *Data = NULL;

    if (SetupGetFieldCount(Context) != 1)
        return FALSE;

    for (i = 0; i <= 1; i++)
    {
        ret = SetupGetStringFieldW(Context,
                                   i,
                                   NULL,
                                   0,
                                   &dwSize);
        if (!ret)
            return FALSE;

        HeapFree(GetProcessHeap(), 0, pLastCallData[i + currentIndex]);
        pLastCallData[i + currentIndex] = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(WCHAR));
        ret = SetupGetStringFieldW(Context,
                                   i,
                                   pLastCallData[i + currentIndex],
                                   dwSize,
                                   NULL);
        if (!ret)
            return FALSE;
    }

    if (Key)
        *Key = pLastCallData[0 + currentIndex];

    if (Data)
        *Data = pLastCallData[1 + currentIndex];

    return TRUE;
#endif /* !__REACTOS__ */
}

BOOLEAN
INF_GetDataField(
    IN PINFCONTEXT Context,
    IN ULONG FieldIndex,
    OUT PWCHAR *Data)
{
#ifdef __REACTOS__
    return InfGetDataField(Context, FieldIndex, Data);
#else
    static PWCHAR pLastCallsData[] = { NULL, NULL, NULL };
    static DWORD NextIndex = 0;
    DWORD dwSize;
    BOOL ret;

    *Data = NULL;

    ret = SetupGetStringFieldW(Context,
                               FieldIndex,
                               NULL,
                               0,
                               &dwSize);
    if (!ret)
        return FALSE;

    HeapFree(GetProcessHeap(), 0, pLastCallsData[NextIndex]);
    pLastCallsData[NextIndex] = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(WCHAR));
    ret = SetupGetStringFieldW(Context,
                               FieldIndex,
                               pLastCallsData[NextIndex],
                               dwSize,
                               NULL);
    if (!ret)
        return FALSE;

    *Data = pLastCallsData[NextIndex];
    NextIndex = (NextIndex + 1) % (sizeof(pLastCallsData) / sizeof(pLastCallsData[0]));
    return TRUE;
#endif /* !__REACTOS__ */
}

HINF WINAPI
INF_OpenBufferedFileA(
    IN PSTR FileBuffer,
    IN ULONG FileSize,
    IN PCSTR InfClass,
    IN DWORD InfStyle,
    IN LCID LocaleId,
    OUT PUINT ErrorLine)
{
#ifdef __REACTOS__
    HINF hInf = NULL;
    ULONG ErrorLineUL;
    NTSTATUS Status;

    Status = InfOpenBufferedFile(&hInf,
                                 FileBuffer,
                                 FileSize,
                                 LANGIDFROMLCID(LocaleId),
                                 &ErrorLineUL);
    *ErrorLine = (UINT)ErrorLineUL;
    if (!NT_SUCCESS(Status))
        return INVALID_HANDLE_VALUE;

    return hInf;
#else
    return INVALID_HANDLE_VALUE;
#endif /* !__REACTOS__ */
}

/* EOF */
