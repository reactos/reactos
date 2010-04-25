/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * PROGRAMMER: Royce Mitchell III
 *             Eric Kohl
 *             Ge van Geldorp <gvg@reactos.org>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "infcommon.h"

extern NTSTATUS NTAPI RtlMultiByteToUnicodeN(IN PWCHAR UnicodeString,
    IN ULONG UnicodeSize, IN PULONG ResultSize, IN PCSTR MbString, IN ULONG MbSize);

extern BOOLEAN NTAPI RtlIsTextUnicode( PVOID buf, INT len, INT *pf );


extern int InfHostOpenBufferedFile(PHINF InfHandle,
                                   void *Buffer,
                                   ULONG BufferSize,
                                   LCID LocaleId,
                                   ULONG *ErrorLine);
extern int InfHostOpenFile(PHINF InfHandle,
                           const CHAR *FileName,
                           LCID LocaleId,
                           ULONG *ErrorLine);
extern int InfHostWriteFile(HINF InfHandle,
                            const CHAR *FileName,
                            const CHAR *HeaderComment);
extern void InfHostCloseFile(HINF InfHandle);
extern int InfHostFindFirstLine(HINF InfHandle,
                                const WCHAR *Section,
                                const WCHAR *Key,
                                PINFCONTEXT *Context);
extern int InfHostFindNextLine(PINFCONTEXT ContextIn,
                               PINFCONTEXT ContextOut);
extern int InfHostFindFirstMatchLine(PINFCONTEXT ContextIn,
                                     const WCHAR *Key,
                                     PINFCONTEXT ContextOut);
extern int InfHostFindNextMatchLine(PINFCONTEXT ContextIn,
                                    const WCHAR *Key,
                                    PINFCONTEXT ContextOut);
extern LONG InfHostGetLineCount(HINF InfHandle,
                                const WCHAR *Section);
extern LONG InfHostGetFieldCount(PINFCONTEXT Context);
extern int InfHostGetBinaryField(PINFCONTEXT Context,
                                 ULONG FieldIndex,
                                 UCHAR *ReturnBuffer,
                                 ULONG ReturnBufferSize,
                                 ULONG *RequiredSize);
extern int InfHostGetIntField(PINFCONTEXT Context,
                              ULONG FieldIndex,
                              ULONG *IntegerValue);
extern int InfHostGetMultiSzField(PINFCONTEXT Context,
                                  ULONG FieldIndex,
                                  WCHAR *ReturnBuffer,
                                  ULONG ReturnBufferSize,
                                  ULONG *RequiredSize);
extern int InfHostGetStringField(PINFCONTEXT Context,
                                 ULONG FieldIndex,
                                 WCHAR *ReturnBuffer,
                                 ULONG ReturnBufferSize,
                                 ULONG *RequiredSize);
extern int InfHostGetData(PINFCONTEXT Context,
                          WCHAR **Key,
                          WCHAR **Data);
extern int InfHostGetDataField(PINFCONTEXT Context,
                               ULONG FieldIndex,
                               WCHAR **Data);
extern int InfHostFindOrAddSection(HINF InfHandle,
                                   const WCHAR *Section,
                                   PINFCONTEXT *Context);
extern int InfHostAddLine(PINFCONTEXT Context, const WCHAR *Key);
extern int InfHostAddField(PINFCONTEXT Context, const WCHAR *Data);
extern void InfHostFreeContext(PINFCONTEXT Context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* EOF */
