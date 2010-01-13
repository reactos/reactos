/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * PROGRAMMER: Royce Mitchell III
 *             Eric Kohl
 *             Ge van Geldorp <gvg@reactos.org>
 */

#ifndef INFHOST_H_INCLUDED
#define INFHOST_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "infcommon.h"

extern int InfHostOpenBufferedFile(PHINF InfHandle,
                                   void *Buffer,
                                   ULONG BufferSize,
                                   ULONG *ErrorLine);
extern int InfHostOpenFile(PHINF InfHandle,
                           const CHAR *FileName,
                           ULONG *ErrorLine);
extern int InfHostWriteFile(HINF InfHandle,
                            const CHAR *FileName,
                            const CHAR *HeaderComment);
extern void InfHostCloseFile(HINF InfHandle);
extern int InfHostFindFirstLine(HINF InfHandle,
                                const CHAR *Section,
                                const CHAR *Key,
                                PINFCONTEXT *Context);
extern int InfHostFindNextLine(PINFCONTEXT ContextIn,
                               PINFCONTEXT ContextOut);
extern int InfHostFindFirstMatchLine(PINFCONTEXT ContextIn,
                                     const CHAR *Key,
                                     PINFCONTEXT ContextOut);
extern int InfHostFindNextMatchLine(PINFCONTEXT ContextIn,
                                    const CHAR *Key,
                                    PINFCONTEXT ContextOut);
extern LONG InfHostGetLineCount(HINF InfHandle,
                                const CHAR *Section);
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
                                  CHAR *ReturnBuffer,
                                  ULONG ReturnBufferSize,
                                  ULONG *RequiredSize);
extern int InfHostGetStringField(PINFCONTEXT Context,
                                 ULONG FieldIndex,
                                 CHAR *ReturnBuffer,
                                 ULONG ReturnBufferSize,
                                 ULONG *RequiredSize);
extern int InfHostGetData(PINFCONTEXT Context,
                          CHAR **Key,
                          CHAR **Data);
extern int InfHostGetDataField(PINFCONTEXT Context,
                               ULONG FieldIndex,
                               CHAR **Data);
extern int InfHostFindOrAddSection(HINF InfHandle,
                                   const CHAR *Section,
                                   PINFCONTEXT *Context);
extern int InfHostAddLine(PINFCONTEXT Context, const CHAR *Key);
extern int InfHostAddField(PINFCONTEXT Context, const CHAR *Data);
extern void InfHostFreeContext(PINFCONTEXT Context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INFROS_H_INCLUDED */

/* EOF */
