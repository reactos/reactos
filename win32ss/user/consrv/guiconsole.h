/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/guiconsole.h
 * PURPOSE:         Interface to gui-mode consoles
 * PROGRAMMERS:
 */

#include "conio.h"

#define CONGUI_MIN_WIDTH      10
#define CONGUI_MIN_HEIGHT     10
#define CONGUI_UPDATE_TIME    0
#define CONGUI_UPDATE_TIMER   1

NTSTATUS FASTCALL GuiInitConsole(PCSRSS_CONSOLE Console, BOOL Visible);
VOID FASTCALL GuiConsoleHandleScrollbarMenu(VOID);

/*EOF*/
