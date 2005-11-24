/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         .inf file parser
 * FILE:            lib/inflib/infcache.h
 * PURPOSE:         INF file parser that caches contents of INF file in memory
 * PROGRAMMER:      Royce Mitchell III
 *                  Eric Kohl
 *                  Ge van Geldorp <gvg@reactos.org>
 */

#ifndef INFPRIV_H_INCLUDED
#define INFPRIV_H_INCLUDED


#define INF_STATUS_INSUFFICIENT_RESOURCES  (0xC000009A)
#define INF_STATUS_BAD_SECTION_NAME_LINE   (0xC0700001)
#define INF_STATUS_SECTION_NAME_TOO_LONG   (0xC0700002)
#define INF_STATUS_WRONG_INF_STYLE         (0xC0700003)
#define INF_STATUS_NOT_ENOUGH_MEMORY       (0xC0700004)

typedef struct _INFCONTEXT
{
  PVOID Inf;
  PVOID Section;
  PVOID Line;
} INFCONTEXT;

typedef struct _INFCACHEFIELD
{
  struct _INFCACHEFIELD *Next;
  struct _INFCACHEFIELD *Prev;

  TCHAR Data[1];
} INFCACHEFIELD, *PINFCACHEFIELD;

typedef struct _INFCACHELINE
{
  struct _INFCACHELINE *Next;
  struct _INFCACHELINE *Prev;

  LONG FieldCount;

  PTCHAR Key;

  PINFCACHEFIELD FirstField;
  PINFCACHEFIELD LastField;

} INFCACHELINE, *PINFCACHELINE;

typedef struct _INFCACHESECTION
{
  struct _INFCACHESECTION *Next;
  struct _INFCACHESECTION *Prev;

  PINFCACHELINE FirstLine;
  PINFCACHELINE LastLine;

  LONG LineCount;

  TCHAR Name[1];
} INFCACHESECTION, *PINFCACHESECTION;

typedef struct _INFCACHE
{
  PINFCACHESECTION FirstSection;
  PINFCACHESECTION LastSection;

  PINFCACHESECTION StringsSection;
} INFCACHE, *PINFCACHE;

typedef long INFSTATUS;

/* FUNCTIONS ****************************************************************/

extern INFSTATUS InfpParseBuffer(PINFCACHE file,
                                 const CHAR *buffer,
                                 const CHAR *end,
                                 PULONG error_line);
extern PINFCACHESECTION InfpCacheFreeSection(PINFCACHESECTION Section);
extern PINFCACHELINE InfpCacheFindKeyLine(PINFCACHESECTION Section,
                                          PTCHAR Key);

extern INFSTATUS InfpFindFirstLine(HINF InfHandle,
                                   PCTSTR Section,
                                   PCTSTR Key,
                                   PINFCONTEXT *Context);
extern INFSTATUS InfpFindNextLine(PINFCONTEXT ContextIn,
                                  PINFCONTEXT ContextOut);
extern INFSTATUS InfpFindFirstMatchLine(PINFCONTEXT ContextIn,
                                        PCTSTR Key,
                                        PINFCONTEXT ContextOut);
extern INFSTATUS InfpFindNextMatchLine(PINFCONTEXT ContextIn,
                                       PCTSTR Key,
                                       PINFCONTEXT ContextOut);
extern LONG InfpGetLineCount(HINF InfHandle,
                             PCTSTR Section);
extern LONG InfpGetFieldCount(PINFCONTEXT Context);
extern INFSTATUS InfpGetBinaryField(PINFCONTEXT Context,
                                    ULONG FieldIndex,
                                    PUCHAR ReturnBuffer,
                                    ULONG ReturnBufferSize,
                                    PULONG RequiredSize);
extern INFSTATUS InfpGetIntField(PINFCONTEXT Context,
                                 ULONG FieldIndex,
                                 PLONG IntegerValue);
extern INFSTATUS InfpGetMultiSzField(PINFCONTEXT Context,
                                     ULONG FieldIndex,
                                     PTSTR ReturnBuffer,
                                     ULONG ReturnBufferSize,
                                     PULONG RequiredSize);
extern INFSTATUS InfpGetStringField(PINFCONTEXT Context,
                                    ULONG FieldIndex,
                                    PTSTR ReturnBuffer,
                                    ULONG ReturnBufferSize,
                                    PULONG RequiredSize);
extern INFSTATUS InfpGetData(PINFCONTEXT Context,
                             PTCHAR *Key,
                             PTCHAR *Data);
extern INFSTATUS InfpGetDataField(PINFCONTEXT Context,
                                  ULONG FieldIndex,
                                  PTCHAR *Data);
extern VOID InfpFreeContext(PINFCONTEXT Context);

#endif /* INFPRIV_H_INCLUDED */

/* EOF */
