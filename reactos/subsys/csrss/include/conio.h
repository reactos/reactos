/* $Id: conio.h,v 1.1 2003/12/02 11:38:46 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/include/conio.h
 * PURPOSE:         CSRSS internal console I/O interface
 */

#ifndef CONIO_H_INCLUDED
#define CONIO_H_INCLUDED

#include "api.h"

/* Object type magic numbers */

#define CONIO_CONSOLE_MAGIC         0x00000001
#define CONIO_SCREEN_BUFFER_MAGIC   0x00000002

VOID STDCALL ConioDeleteConsole(Object_t *Object);
VOID STDCALL ConioDeleteScreenBuffer(Object_t *Buffer);
void STDCALL CsrProcessKey(MSG *msg, PCSRSS_CONSOLE Console);

/* api/conio.c */
CSR_API(CsrWriteConsole);
CSR_API(CsrAllocConsole);
CSR_API(CsrFreeConsole);
CSR_API(CsrReadConsole);
CSR_API(CsrConnectProcess);
CSR_API(CsrGetScreenBufferInfo);
CSR_API(CsrSetCursor);
CSR_API(CsrFillOutputChar);
CSR_API(CsrReadInputEvent);
CSR_API(CsrWriteConsoleOutputChar);
CSR_API(CsrWriteConsoleOutputAttrib);
CSR_API(CsrFillOutputAttrib);
CSR_API(CsrGetCursorInfo);
CSR_API(CsrSetCursorInfo);
CSR_API(CsrSetTextAttrib);
CSR_API(CsrSetConsoleMode);
CSR_API(CsrGetConsoleMode);
CSR_API(CsrCreateScreenBuffer);
CSR_API(CsrSetScreenBuffer);
CSR_API(CsrSetTitle);
CSR_API(CsrGetTitle);
CSR_API(CsrWriteConsoleOutput);
CSR_API(CsrFlushInputBuffer);
CSR_API(CsrScrollConsoleScreenBuffer);
CSR_API(CsrReadConsoleOutputChar);
CSR_API(CsrReadConsoleOutputAttrib);
CSR_API(CsrGetNumberOfConsoleInputEvents);
CSR_API(CsrPeekConsoleInput);
CSR_API(CsrReadConsoleOutput);
CSR_API(CsrWriteConsoleInput);
CSR_API(CsrHardwareStateProperty);
CSR_API(CsrGetConsoleWindow);

#endif /* CONIO_H_INCLUDED */

/* EOF */

