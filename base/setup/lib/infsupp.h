/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/infsupp.h
 * PURPOSE:         Interfacing with Setup* API .inf files support functions
 * PROGRAMMER:      Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

#ifdef __REACTOS__
#define _SETUPAPI_
#endif

// FIXME: Temporary measure until all the users of this header
// (usetup...) use or define SetupAPI-conforming APIs.
#if 0

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

// #define SetupCloseInfFile InfCloseFile
VOID
WINAPI
SetupCloseInfFile(HINF InfHandle);

// #define SetupFindFirstLineW InfpFindFirstLineW
BOOL
WINAPI
SetupFindFirstLineW(
    IN HINF InfHandle,
    IN PCWSTR Section,
    IN PCWSTR Key,
    IN OUT PINFCONTEXT Context);

// #define SetupFindNextLine InfFindNextLine
BOOL
WINAPI
SetupFindNextLine(PINFCONTEXT ContextIn,
                  PINFCONTEXT ContextOut);

// #define SetupGetFieldCount InfGetFieldCount
LONG
WINAPI
SetupGetFieldCount(PINFCONTEXT Context);

// #define SetupGetBinaryField InfGetBinaryField
BOOL
WINAPI
SetupGetBinaryField(PINFCONTEXT Context,
                    ULONG FieldIndex,
                    PUCHAR ReturnBuffer,
                    ULONG ReturnBufferSize,
                    PULONG RequiredSize);

// #define SetupGetIntField InfGetIntField
BOOLEAN
WINAPI
SetupGetIntField(PINFCONTEXT Context,
                 ULONG FieldIndex,
                 INT *IntegerValue);

// #define SetupGetMultiSzFieldW InfGetMultiSzField
BOOL
WINAPI
SetupGetMultiSzFieldW(PINFCONTEXT Context,
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
                     PULONG RequiredSize);

#endif

/* Lower the MAX_INF_STRING_LENGTH value in order to avoid too much stack usage */
#undef MAX_INF_STRING_LENGTH
#define MAX_INF_STRING_LENGTH   1024 // Still larger than in infcommon.h

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


/*
 * This function corresponds to an undocumented but exported SetupAPI function
 * that exists since WinNT4 and is still present in Win10.
 * The returned string pointer is a read-only pointer to a string in the
 * maintained INF cache, and is always in UNICODE (on NT systems).
 */
PCWSTR
WINAPI
pSetupGetField(PINFCONTEXT Context,
               ULONG FieldIndex);

/* A version of SetupOpenInfFileW with support for a user-provided LCID */
// #define SetupOpenInfFileExW InfpOpenInfFileW
HINF
WINAPI
SetupOpenInfFileExW(
    IN PCWSTR FileName,
    IN PCWSTR InfClass,
    IN DWORD InfStyle,
    IN LCID LocaleId,
    OUT PUINT ErrorLine);


/* HELPER FUNCTIONS **********************************************************/

FORCEINLINE VOID
INF_FreeData(IN PWCHAR InfData)
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
    OUT PWCHAR *Data);

BOOLEAN
INF_GetData(
    IN PINFCONTEXT Context,
    OUT PWCHAR *Key,
    OUT PWCHAR *Data);

/* EOF */
