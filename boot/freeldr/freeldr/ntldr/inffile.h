/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     INF file parser that caches contents of INF file in memory.
 * COPYRIGHT:   Copyright 2002-2006 Royce Mitchell III
 *              Copyright 2003-2019 Eric Kohl
 */

#pragma once

#define STATUS_BAD_SECTION_NAME_LINE   (0xC0700001)
#define STATUS_SECTION_NAME_TOO_LONG   (0xC0700002)
#define STATUS_WRONG_INF_STYLE         (0xC0700003)
#define STATUS_NOT_ENOUGH_MEMORY       (0xC0700004)

#define MAX_INF_STRING_LENGTH  512

typedef PULONG HINF, *PHINF;

typedef struct _INFCONTEXT
{
  PVOID Inf;
  PVOID CurrentInf;
  PVOID Section;
  PVOID Line;
} INFCONTEXT, *PINFCONTEXT;


/* FUNCTIONS ****************************************************************/

BOOLEAN
InfOpenFile (PHINF InfHandle,
         PCSTR FileName,
         PULONG ErrorLine);

VOID
InfCloseFile (HINF InfHandle);


BOOLEAN
InfFindFirstLine (HINF InfHandle,
          PCSTR Section,
          PCSTR Key,
          PINFCONTEXT Context);

BOOLEAN
InfFindNextLine (PINFCONTEXT ContextIn,
         PINFCONTEXT ContextOut);

BOOLEAN
InfFindFirstMatchLine (PINFCONTEXT ContextIn,
               PCSTR Key,
               PINFCONTEXT ContextOut);

BOOLEAN
InfFindNextMatchLine (PINFCONTEXT ContextIn,
              PCSTR Key,
              PINFCONTEXT ContextOut);


LONG
InfGetLineCount (HINF InfHandle,
         PCSTR Section);

LONG
InfGetFieldCount (PINFCONTEXT Context);


BOOLEAN
InfGetBinaryField (PINFCONTEXT Context,
           ULONG FieldIndex,
           PUCHAR ReturnBuffer,
           ULONG ReturnBufferSize,
           PULONG RequiredSize);

BOOLEAN
InfGetIntField (PINFCONTEXT Context,
        ULONG FieldIndex,
        LONG *IntegerValue);

BOOLEAN
InfGetMultiSzField (PINFCONTEXT Context,
            ULONG FieldIndex,
            PCHAR ReturnBuffer,
            ULONG ReturnBufferSize,
            PULONG RequiredSize);

BOOLEAN
InfGetStringField (PINFCONTEXT Context,
           ULONG FieldIndex,
           PCHAR ReturnBuffer,
           ULONG ReturnBufferSize,
           PULONG RequiredSize);



BOOLEAN
InfGetData (PINFCONTEXT Context,
        PCSTR *Key,
        PCSTR *Data);

BOOLEAN
InfGetDataField (PINFCONTEXT Context,
         ULONG FieldIndex,
         PCSTR *Data);

/* EOF */
