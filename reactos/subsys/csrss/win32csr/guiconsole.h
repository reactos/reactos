/* $Id: guiconsole.h,v 1.3 2004/03/07 21:00:11 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/guiconsole.h
 * PURPOSE:         Interface to GUI consoles
 */

#include "api.h"

extern NTSTATUS FASTCALL GuiInitConsole(PCSRSS_CONSOLE Console);
extern VOID STDCALL GuiConsoleDrawRegion(PCSRSS_CONSOLE Console, SMALL_RECT Region);
extern VOID STDCALL GuiConsoleCopyRegion(HWND hWnd,
                                         RECT *Source,
                                         RECT *Dest);
extern VOID STDCALL GuiConsoleChangeTitle(PCSRSS_CONSOLE Console);
extern VOID STDCALL GuiConsoleDeleteConsole(PCSRSS_CONSOLE Console);

/*EOF*/
