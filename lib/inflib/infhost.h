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

#include <infcommon.h>

extern int InfHostOpenBufferedFile(PHINF InfHandle,
                                   void *Buffer,
                                   unsigned long BufferSize,
                                   unsigned long *ErrorLine);
extern int InfHostOpenFile(PHINF InfHandle,
                           const char *FileName,
                           unsigned long *ErrorLine);
extern int InfHostWriteFile(HINF InfHandle,
                            const char *FileName,
                            const char *HeaderComment);
extern void InfHostCloseFile(HINF InfHandle);
extern int InfHostFindFirstLine(HINF InfHandle,
                                const char *Section,
                                const char *Key,
                                PINFCONTEXT *Context);
extern int InfHostFindNextLine(PINFCONTEXT ContextIn,
                               PINFCONTEXT ContextOut);
extern int InfHostFindFirstMatchLine(PINFCONTEXT ContextIn,
                                     const char *Key,
                                     PINFCONTEXT ContextOut);
extern int InfHostFindNextMatchLine(PINFCONTEXT ContextIn,
                                    const char *Key,
                                    PINFCONTEXT ContextOut);
extern long InfHostGetLineCount(HINF InfHandle,
                                const char *Section);
extern long InfHostGetFieldCount(PINFCONTEXT Context);
extern int InfHostGetBinaryField(PINFCONTEXT Context,
                                 unsigned long FieldIndex,
                                 unsigned char *ReturnBuffer,
                                 unsigned long ReturnBufferSize,
                                 unsigned long *RequiredSize);
extern int InfHostGetIntField(PINFCONTEXT Context,
                              unsigned long FieldIndex,
                              unsigned long *IntegerValue);
extern int InfHostGetMultiSzField(PINFCONTEXT Context,
                                  unsigned long FieldIndex,
                                  char *ReturnBuffer,
                                  unsigned long ReturnBufferSize,
                                  unsigned long *RequiredSize);
extern int InfHostGetStringField(PINFCONTEXT Context,
                                 unsigned long FieldIndex,
                                 char *ReturnBuffer,
                                 unsigned long ReturnBufferSize,
                                 unsigned long *RequiredSize);
extern int InfHostGetData(PINFCONTEXT Context,
                          char **Key,
                          char **Data);
extern int InfHostGetDataField(PINFCONTEXT Context,
                               unsigned long FieldIndex,
                               char **Data);
extern int InfHostFindOrAddSection(HINF InfHandle,
                                   const char *Section,
                                   PINFCONTEXT *Context);
extern int InfHostAddLine(PINFCONTEXT Context, const char *Key);
extern int InfHostAddField(PINFCONTEXT Context, const char *Data);
extern void InfHostFreeContext(PINFCONTEXT Context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INFROS_H_INCLUDED */

/* EOF */
