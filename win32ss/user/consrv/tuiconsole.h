/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/tuiconsole.h
 * PURPOSE:         TUI terminal emulator
 * PROGRAMMERS:
 */

#include "conio.h"

NTSTATUS FASTCALL TuiInitConsole(PCONSOLE Console,
                                 PCONSOLE_INFO ConsoleInfo);
PCONSOLE FASTCALL TuiGetFocusConsole(VOID);

/* EOF */
