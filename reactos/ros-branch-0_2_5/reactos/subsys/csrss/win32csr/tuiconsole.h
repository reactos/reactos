/* $Id: tuiconsole.h,v 1.1 2004/01/11 17:31:16 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/tuiconsole.h
 * PURPOSE:         Interface to text-mode consoles
 */

#include "api.h"

extern NTSTATUS FASTCALL TuiInitConsole(PCSRSS_CONSOLE Console);
extern PCSRSS_CONSOLE FASTCALL TuiGetFocusConsole(VOID);
extern BOOL FASTCALL TuiSwapConsole(int Next);

/* EOF */
