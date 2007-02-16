/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * PROGRAMMER: Royce Mitchell III
 *             Eric Kohl
 *             Ge van Geldorp <gvg@reactos.org>
 */

#ifndef INFPRIV_H_INCLUDED
#define INFPRIV_H_INCLUDED


#define INF_STATUS_INSUFFICIENT_RESOURCES  (0xC000009A)
#define INF_STATUS_BAD_SECTION_NAME_LINE   (0xC0700001)
#define INF_STATUS_SECTION_NAME_TOO_LONG   (0xC0700002)
#define INF_STATUS_WRONG_INF_STYLE         (0xC0700003)
#define INF_STATUS_NOT_ENOUGH_MEMORY       (0xC0700004)

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

  PTSTR Key;

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

typedef struct _INFCONTEXT
{
  PINFCACHE Inf;
  PINFCACHESECTION Section;
  PINFCACHELINE Line;
} INFCONTEXT;

typedef long INFSTATUS;

/* FUNCTIONS ****************************************************************/

extern INFSTATUS InfpParseBuffer(PINFCACHE file,
                                 const CHAR *buffer,
                                 const CHAR *end,
                                 PULONG error_line);
extern PINFCACHESECTION InfpFreeSection(PINFCACHESECTION Section);
extern PINFCACHESECTION InfpAddSection(PINFCACHE Cache,
                                       PCTSTR Name);
extern PINFCACHELINE InfpAddLine(PINFCACHESECTION Section);
extern PVOID InfpAddKeyToLine(PINFCACHELINE Line,
                              PCTSTR Key);
extern PVOID InfpAddFieldToLine(PINFCACHELINE Line,
                                PCTSTR Data);
extern PINFCACHELINE InfpFindKeyLine(PINFCACHESECTION Section,
                                     PCTSTR Key);
extern PINFCACHESECTION InfpFindSection(PINFCACHE Cache,
                                        PCTSTR Section);

extern INFSTATUS InfpBuildFileBuffer(PINFCACHE InfHandle,
                                     char **Buffer,
                                     unsigned long *BufferSize);

extern INFSTATUS InfpFindFirstLine(PINFCACHE InfHandle,
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

extern INFSTATUS InfpFindOrAddSection(PINFCACHE Cache,
                                      PCTSTR Section,
                                      PINFCONTEXT *Context);
extern INFSTATUS InfpAddLineWithKey(PINFCONTEXT Context, PCTSTR Key);
extern INFSTATUS InfpAddField(PINFCONTEXT Context, PCTSTR Data);

extern VOID InfpFreeContext(PINFCONTEXT Context);

#endif /* INFPRIV_H_INCLUDED */

/* EOF */
