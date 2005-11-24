/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         .inf file parser
 * FILE:            lib/inflib/infros.h
 * PURPOSE:         Public .inf routines for use on the host build system
 * PROGRAMMER:      Royce Mitchell III
 *                  Eric Kohl
 *                  Ge van Geldorp
 */

#ifndef INFHOST_H_INCLUDED
#define INFHOST_H_INCLUDED

#include <infcommon.h>

extern int InfHostOpenBufferedFile(PHINF InfHandle,
                                   void *Buffer,
                                   unsigned long BufferSize,
                                   unsigned long *ErrorLine);
extern int InfHostOpenFile(PHINF InfHandle,
                           char *FileName,
                           unsigned long *ErrorLine);
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
extern VOID InfHostFreeContext(PINFCONTEXT Context);

#endif /* INFROS_H_INCLUDED */

/* EOF */
