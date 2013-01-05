/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/tuiconsole.h
 * PURPOSE:         Interface to text-mode consoles
 * PROGRAMMERS:
 */

#include "conio.h"

extern NTSTATUS FASTCALL TuiInitConsole(PCONSOLE Console);
extern PCONSOLE FASTCALL TuiGetFocusConsole(VOID);
extern BOOL FASTCALL TuiSwapConsole(int Next);

/* EOF */
