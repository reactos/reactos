/* $Id: guiconsole.h,v 1.1 2003/12/02 11:38:46 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/guiconsole.h
 * PURPOSE:         Interface to GUI consoles
 */

#include "api.h"

extern BOOL STDCALL GuiConsoleInitConsole(PCSRSS_CONSOLE Console);
extern VOID STDCALL GuiConsoleDrawRegion(PCSRSS_CONSOLE Console, SMALL_RECT Region);
extern VOID STDCALL GuiConsoleCopyRegion(PCSRSS_CONSOLE Console,
                                         RECT *Source,
                                         RECT *Dest);
extern VOID STDCALL GuiConsoleChangeTitle(PCSRSS_CONSOLE Console);
extern VOID STDCALL GuiConsoleDeleteConsole(PCSRSS_CONSOLE Console);

extern VOID FASTCALL GuiConsoleInitConsoleSupport(VOID);

/*EOF*/
