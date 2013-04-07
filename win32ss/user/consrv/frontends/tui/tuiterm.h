/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/frontends/tui/tuiterm.h
 * PURPOSE:         TUI Terminal Front-End
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "conio.h"

NTSTATUS FASTCALL TuiInitConsole(PCONSOLE Console,
                                 PCONSOLE_INFO ConsoleInfo);
PCONSOLE FASTCALL TuiGetFocusConsole(VOID);

/* EOF */
