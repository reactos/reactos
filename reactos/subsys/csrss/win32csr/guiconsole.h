/* $Id: guiconsole.h,v 1.2 2004/01/11 17:31:16 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/guiconsole.h
 * PURPOSE:         Interface to GUI consoles
 */

#include "api.h"

extern NTSTATUS FASTCALL GuiInitConsole(PCSRSS_CONSOLE Console);
extern VOID STDCALL GuiConsoleDrawRegion(PCSRSS_CONSOLE Console, SMALL_RECT Region);
extern VOID STDCALL GuiConsoleCopyRegion(PCSRSS_CONSOLE Console,
                                         RECT *Source,
                                         RECT *Dest);
extern VOID STDCALL GuiConsoleChangeTitle(PCSRSS_CONSOLE Console);
extern VOID STDCALL GuiConsoleDeleteConsole(PCSRSS_CONSOLE Console);

/*EOF*/
