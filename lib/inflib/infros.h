/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * PROGRAMMER: Royce Mitchell III
 *             Eric Kohl
 *             Ge van Geldorp <gvg@reactos.org>
 */

#ifndef INFROS_H_INCLUDED
#define INFROS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <infcommon.h>

extern VOID InfSetHeap(PVOID Heap);
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
extern VOID InfCloseFile(HINF InfHandle);
extern BOOLEAN InfFindFirstLine(HINF InfHandle,
                                PCWSTR Section,
                                PCWSTR Key,
                                PINFCONTEXT *Context);
extern BOOLEAN InfFindNextLine(PINFCONTEXT ContextIn,
                               PINFCONTEXT ContextOut);
extern BOOLEAN InfFindFirstMatchLine(PINFCONTEXT ContextIn,
                                     PCWSTR Key,
                                     PINFCONTEXT ContextOut);
extern BOOLEAN InfFindNextMatchLine(PINFCONTEXT ContextIn,
                                    PCWSTR Key,
                                    PINFCONTEXT ContextOut);
extern LONG InfGetLineCount(HINF InfHandle,
                            PCWSTR Section);
extern LONG InfGetFieldCount(PINFCONTEXT Context);
extern BOOLEAN InfGetBinaryField(PINFCONTEXT Context,
                                 ULONG FieldIndex,
                                 PUCHAR ReturnBuffer,
                                 ULONG ReturnBufferSize,
                                 PULONG RequiredSize);
extern BOOLEAN InfGetIntField(PINFCONTEXT Context,
                              ULONG FieldIndex,
                              PLONG IntegerValue);
extern BOOLEAN InfGetMultiSzField(PINFCONTEXT Context,
                                  ULONG FieldIndex,
                                  PWSTR ReturnBuffer,
                                  ULONG ReturnBufferSize,
                                  PULONG RequiredSize);
extern BOOLEAN InfGetStringField(PINFCONTEXT Context,
                                 ULONG FieldIndex,
                                 PWSTR ReturnBuffer,
                                 ULONG ReturnBufferSize,
                                 PULONG RequiredSize);
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

#endif /* INFROS_H_INCLUDED */

/* EOF */
