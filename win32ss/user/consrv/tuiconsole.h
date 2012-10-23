/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            win32ss/user/consrv/tuiconsole.h
 * PURPOSE:         Interface to text-mode consoles
 */

// #include "api.h"
#include "conio.h"

extern NTSTATUS FASTCALL TuiInitConsole(PCSRSS_CONSOLE Console);
extern PCSRSS_CONSOLE FASTCALL TuiGetFocusConsole(VOID);
extern BOOL FASTCALL TuiSwapConsole(int Next);

/* EOF */
