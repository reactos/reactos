/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/infsupp.h
 * PURPOSE:         Interfacing with Setup* API .INF Files support functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

#include "spapisup.h"

// FIXME: Temporary measure until all the users of this header
// (usetup...) use or define SetupAPI-conforming APIs.
#if defined(_SETUPAPI_H_) || defined(_INC_SETUPAPI)

#include <setupapi.h>

#else

typedef PVOID HINF;
typedef struct _INFCONTEXT
{
    HINF Inf;
    HINF CurrentInf;
    UINT Section;
    UINT Line;
} INFCONTEXT, *PINFCONTEXT;

#endif

/* Lower the MAX_INF_STRING_LENGTH value in order to avoid too much stack usage */
#undef MAX_INF_STRING_LENGTH
#define MAX_INF_STRING_LENGTH   1024 // Still larger than in infcommon.h

#ifndef INF_STYLE_OLDNT
#define INF_STYLE_OLDNT 0x00000001
#endif

#ifndef INF_STYLE_WIN4
#define INF_STYLE_WIN4  0x00000002
#endif

#if 0
typedef PVOID HINF;
typedef struct _INFCONTEXT
{
    HINF Inf;
    HINF CurrentInf;
    UINT Section;
    UINT Line;
} INFCONTEXT, *PINFCONTEXT;
#endif

C_ASSERT(sizeof(INFCONTEXT) == 2 * sizeof(HINF) + 2 * sizeof(UINT));


// #define SetupCloseInfFile InfCloseFile
typedef VOID
(WINAPI* pSpInfCloseInfFile)(
    IN HINF InfHandle);

extern pSpInfCloseInfFile SpInfCloseInfFile;

// #define SetupFindFirstLineW InfpFindFirstLineW
typedef BOOL
(WINAPI* pSpInfFindFirstLine)(
    IN HINF InfHandle,
    IN PCWSTR Section,
    IN PCWSTR Key,
    IN OUT PINFCONTEXT Context);

extern pSpInfFindFirstLine SpInfFindFirstLine;

// #define SetupFindNextLine InfFindNextLine
typedef BOOL
(WINAPI* pSpInfFindNextLine)(
    IN  PINFCONTEXT ContextIn,
    OUT PINFCONTEXT ContextOut);

extern pSpInfFindNextLine SpInfFindNextLine;

// #define SetupGetFieldCount InfGetFieldCount
typedef ULONG
(WINAPI* pSpInfGetFieldCount)(
    IN PINFCONTEXT Context);

extern pSpInfGetFieldCount SpInfGetFieldCount;

// #define SetupGetBinaryField InfGetBinaryField
typedef BOOL
(WINAPI* pSpInfGetBinaryField)(
    IN  PINFCONTEXT Context,
    IN  ULONG FieldIndex,
    OUT PUCHAR ReturnBuffer,
    IN  ULONG ReturnBufferSize,
    OUT PULONG RequiredSize);

extern pSpInfGetBinaryField SpInfGetBinaryField;

// #define SetupGetIntField InfGetIntField
typedef BOOL
(WINAPI* pSpInfGetIntField)(
    IN PINFCONTEXT Context,
    IN ULONG FieldIndex,
    OUT INT *IntegerValue); // PINT

extern pSpInfGetIntField SpInfGetIntField;

// #define SetupGetMultiSzFieldW InfGetMultiSzField
typedef BOOL
(WINAPI* pSpInfGetMultiSzField)(
    IN  PINFCONTEXT Context,
    IN  ULONG FieldIndex,
    OUT PWSTR ReturnBuffer,
    IN  ULONG ReturnBufferSize,
    OUT PULONG RequiredSize);

extern pSpInfGetMultiSzField SpInfGetMultiSzField;

// #define SetupGetStringFieldW InfGetStringField
typedef BOOL
(WINAPI* pSpInfGetStringField)(
    IN  PINFCONTEXT Context,
    IN  ULONG FieldIndex,
    OUT PWSTR ReturnBuffer,
    IN  ULONG ReturnBufferSize,
    OUT PULONG RequiredSize);

extern pSpInfGetStringField SpInfGetStringField;

// #define pSetupGetField
typedef PCWSTR
(WINAPI* pSpInfGetField)(
    IN PINFCONTEXT Context,
    IN ULONG FieldIndex);

extern pSpInfGetField SpInfGetField;

/* A version of SetupOpenInfFileW with support for a user-provided LCID */
// #define SetupOpenInfFileExW InfpOpenInfFileW
typedef HINF
(WINAPI* pSpInfOpenInfFile)(
    IN PCWSTR FileName,
    IN PCWSTR InfClass,
    IN DWORD InfStyle,
    IN LCID LocaleId,
    OUT PUINT ErrorLine);

extern pSpInfOpenInfFile SpInfOpenInfFile;


/* HELPER FUNCTIONS **********************************************************/

FORCEINLINE
VOID
INF_FreeData(
    IN PCWSTR InfData)
{
#if 0
    if (InfData)
        RtlFreeHeap(ProcessHeap, 0, InfData);
#else
    UNREFERENCED_PARAMETER(InfData);
#endif
}

BOOLEAN
INF_GetDataField(
    IN PINFCONTEXT Context,
    IN ULONG FieldIndex,
    OUT PCWSTR* Data);

BOOLEAN
INF_GetData(
    IN PINFCONTEXT Context,
    OUT PCWSTR* Key,
    OUT PCWSTR* Data);

/* EOF */
