/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * PROGRAMMER: Royce Mitchell III
 *             Eric Kohl
 *             Ge van Geldorp <gvg@reactos.org>
 */

#pragma once

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(t,f) ((ptrdiff_t)&(((t*)0)->f))
#endif

#define INF_STATUS_INSUFFICIENT_RESOURCES  ((INFSTATUS)0xC000009A)
#define INF_STATUS_BAD_SECTION_NAME_LINE   ((INFSTATUS)0xC0700001)
#define INF_STATUS_SECTION_NAME_TOO_LONG   ((INFSTATUS)0xC0700002)
#define INF_STATUS_WRONG_INF_STYLE         ((INFSTATUS)0xC0700003)
#define INF_STATUS_NOT_ENOUGH_MEMORY       ((INFSTATUS)0xC0700004)

typedef struct _INFCACHEFIELD
{
  struct _INFCACHEFIELD *Next;
  struct _INFCACHEFIELD *Prev;

  WCHAR Data[1];
} INFCACHEFIELD, *PINFCACHEFIELD;

typedef struct _INFCACHELINE
{
  struct _INFCACHELINE *Next;
  struct _INFCACHELINE *Prev;
  UINT Id;

  LONG FieldCount;

  PWCHAR Key;

  PINFCACHEFIELD FirstField;
  PINFCACHEFIELD LastField;

} INFCACHELINE, *PINFCACHELINE;

typedef struct _INFCACHESECTION
{
  struct _INFCACHESECTION *Next;
  struct _INFCACHESECTION *Prev;

  PINFCACHELINE FirstLine;
  PINFCACHELINE LastLine;
  UINT Id;

  LONG LineCount;
  UINT NextLineId;

  WCHAR Name[1];
} INFCACHESECTION, *PINFCACHESECTION;

typedef struct _INFCACHE
{
  LANGID LanguageId;
  PINFCACHESECTION FirstSection;
  PINFCACHESECTION LastSection;
  UINT NextSectionId;

  PINFCACHESECTION StringsSection;
} INFCACHE, *PINFCACHE;

typedef struct _INFCONTEXT
{
  PINFCACHE Inf;
  PINFCACHE CurrentInf;
  UINT Section;
  UINT Line;
} INFCONTEXT;

typedef int INFSTATUS;

/* FUNCTIONS ****************************************************************/

extern INFSTATUS InfpParseBuffer(PINFCACHE file,
                                 const WCHAR *buffer,
                                 const WCHAR *end,
                                 PULONG error_line);
extern PINFCACHESECTION InfpFreeSection(PINFCACHESECTION Section);
extern PINFCACHESECTION InfpAddSection(PINFCACHE Cache,
                                       PCWSTR Name);
extern PINFCACHELINE InfpAddLine(PINFCACHESECTION Section);
extern PVOID InfpAddKeyToLine(PINFCACHELINE Line,
                              PCWSTR Key);
extern PVOID InfpAddFieldToLine(PINFCACHELINE Line,
                                PCWSTR Data);
extern PINFCACHELINE InfpFindKeyLine(PINFCACHESECTION Section,
                                     PCWSTR Key);
extern PINFCACHESECTION InfpFindSection(PINFCACHE Cache,
                                        PCWSTR Section);

extern INFSTATUS InfpBuildFileBuffer(PINFCACHE InfHandle,
                                     PWCHAR *Buffer,
                                     PULONG BufferSize);

extern INFSTATUS InfpFindFirstLine(PINFCACHE InfHandle,
                                   PCWSTR Section,
                                   PCWSTR Key,
                                   PINFCONTEXT *Context);
extern INFSTATUS InfpFindNextLine(PINFCONTEXT ContextIn,
                                  PINFCONTEXT ContextOut);
extern INFSTATUS InfpFindFirstMatchLine(PINFCONTEXT ContextIn,
                                        PCWSTR Key,
                                        PINFCONTEXT ContextOut);
extern INFSTATUS InfpFindNextMatchLine(PINFCONTEXT ContextIn,
                                       PCWSTR Key,
                                       PINFCONTEXT ContextOut);
extern LONG InfpGetLineCount(HINF InfHandle,
                             PCWSTR Section);
extern LONG InfpGetFieldCount(PINFCONTEXT Context);
extern INFSTATUS InfpGetBinaryField(PINFCONTEXT Context,
                                    ULONG FieldIndex,
                                    PUCHAR ReturnBuffer,
                                    ULONG ReturnBufferSize,
                                    PULONG RequiredSize);
extern INFSTATUS InfpGetIntField(PINFCONTEXT Context,
                                 ULONG FieldIndex,
                                 INT *IntegerValue);
extern INFSTATUS InfpGetMultiSzField(PINFCONTEXT Context,
                                     ULONG FieldIndex,
                                     PWSTR ReturnBuffer,
                                     ULONG ReturnBufferSize,
                                     PULONG RequiredSize);
extern INFSTATUS InfpGetStringField(PINFCONTEXT Context,
                                    ULONG FieldIndex,
                                    PWSTR ReturnBuffer,
                                    ULONG ReturnBufferSize,
                                    PULONG RequiredSize);
extern INFSTATUS InfpGetData(PINFCONTEXT Context,
                             PWCHAR *Key,
                             PWCHAR *Data);
extern INFSTATUS InfpGetDataField(PINFCONTEXT Context,
                                  ULONG FieldIndex,
                                  PWCHAR *Data);

extern INFSTATUS InfpFindOrAddSection(PINFCACHE Cache,
                                      PCWSTR Section,
                                      PINFCONTEXT *Context);
extern INFSTATUS InfpAddLineWithKey(PINFCONTEXT Context, PCWSTR Key);
extern INFSTATUS InfpAddField(PINFCONTEXT Context, PCWSTR Data);

extern VOID InfpFreeContext(PINFCONTEXT Context);
PINFCACHELINE
InfpFindLineById(PINFCACHESECTION Section, UINT Id);
PINFCACHESECTION
InfpGetSectionForContext(PINFCONTEXT Context);
PINFCACHELINE
InfpGetLineForContext(PINFCONTEXT Context);

/* EOF */
