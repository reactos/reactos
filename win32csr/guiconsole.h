/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/guiconsole.h
 * PURPOSE:         Interface to GUI consoles
 */

#include "api.h"

#define CONGUI_MIN_WIDTH      10
#define CONGUI_MIN_HEIGHT     10
#define CONGUI_UPDATE_TIME    0
#define CONGUI_UPDATE_TIMER   1

NTSTATUS FASTCALL GuiInitConsole(PCSRSS_CONSOLE Console, BOOL Visible);

/*EOF*/
