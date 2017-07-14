/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/infsupp.h
 * PURPOSE:         Interfacing with Setup* API .inf files support functions
 * PROGRAMMER:      Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

// #ifndef __REACTOS__

#define _SETUPAPI_
#include <setupapi.h>

/* Lower the MAX_INF_STRING_LENGTH value in order to avoid too much stack usage */
#undef MAX_INF_STRING_LENGTH
#define MAX_INF_STRING_LENGTH   1024 // Still larger than in infcommon.h

// #else /* __REACTOS__ */

// /* Functions from the INFLIB library */
// #include <infcommon.h>

#define INF_STYLE_WIN4 0x00000002

#if 0
/* FIXME: this structure is the one used in inflib, not in setupapi
 * Delete it once we don't use inflib anymore */
typedef struct _INFCONTEXT
{
  PVOID Inf;
  PVOID CurrentInf;
  PVOID Section;
  PVOID Line;
} INFCONTEXT;
#else
C_ASSERT(sizeof(INFCONTEXT) == 2 * sizeof(PVOID) + 2 * sizeof(UINT));
#endif

// #endif

/* SetupOpenInfFileW with support for a user-provided LCID */
HINF
WINAPI
SetupOpenInfFileExW(
    IN PCWSTR FileName,
    IN PCWSTR InfClass,
    IN DWORD InfStyle,
    IN LCID LocaleId,
    OUT PUINT ErrorLine);

/* EOF */
