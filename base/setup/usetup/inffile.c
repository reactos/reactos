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
 * PROGRAMMER:      Hervé Poussineau
 */

/* INCLUDES ******************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

BOOL
WINAPI
InfpFindFirstLineW(
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


HINF
WINAPI
InfpOpenInfFileW(
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


BOOLEAN
INF_GetData(
    IN PINFCONTEXT Context,
    OUT PWCHAR *Key,
    OUT PWCHAR *Data)
{
    return InfGetData(Context, Key, Data);
}


BOOLEAN
INF_GetDataField(
    IN PINFCONTEXT Context,
    IN ULONG FieldIndex,
    OUT PWCHAR *Data)
{
    return InfGetDataField(Context, FieldIndex, Data);
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
}

/* EOF */
