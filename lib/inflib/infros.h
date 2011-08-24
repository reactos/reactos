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

#include <infcommon.h>

extern NTSTATUS InfOpenBufferedFile(PHINF InfHandle,
                                    PVOID Buffer,
                                    ULONG BufferSize,
                                    PULONG ErrorLine);
extern NTSTATUS InfOpenFile(PHINF InfHandle,
                            PUNICODE_STRING FileName,
                            PULONG ErrorLine);
extern NTSTATUS InfWriteFile(HINF InfHandle,
                             PUNICODE_STRING FileName,
                             PUNICODE_STRING HeaderComment);
extern BOOLEAN InfFindFirstLine(HINF InfHandle,
                                PCWSTR Section,
                                PCWSTR Key,
                                PINFCONTEXT *Context);
extern BOOLEAN InfFindFirstMatchLine(PINFCONTEXT ContextIn,
                                     PCWSTR Key,
                                     PINFCONTEXT ContextOut);
extern BOOLEAN InfFindNextMatchLine(PINFCONTEXT ContextIn,
                                    PCWSTR Key,
                                    PINFCONTEXT ContextOut);
extern LONG InfGetLineCount(HINF InfHandle,
                            PCWSTR Section);
extern LONG InfGetFieldCount(PINFCONTEXT Context);
extern BOOLEAN InfGetIntField(PINFCONTEXT Context,
                              ULONG FieldIndex,
                              PINT IntegerValue);
extern BOOLEAN InfGetData(PINFCONTEXT Context,
                          PWCHAR *Key,
                          PWCHAR *Data);
extern BOOLEAN InfGetDataField(PINFCONTEXT Context,
                               ULONG FieldIndex,
                               PWCHAR *Data);
extern BOOLEAN InfFindOrAddSection(HINF InfHandle,
                                   PCWSTR Section,
                                   PINFCONTEXT *Context);
extern BOOLEAN InfAddLine(PINFCONTEXT Context, PCWSTR Key);
extern BOOLEAN InfAddField(PINFCONTEXT Context, PCWSTR Data);
extern VOID InfFreeContext(PINFCONTEXT Context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* EOF */
