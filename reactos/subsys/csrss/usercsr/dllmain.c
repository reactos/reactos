/* $Id: dllmain.c,v 1.1 2003/10/20 18:02:04 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/usercsr/dllmain.c
 * PURPOSE:         Initialization 
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include "guiconsole.h"
#include "usercsr.h"

/* GLOBALS *******************************************************************/

HANDLE UserCsrApiHeap;
USERCSRPROCESSKEYCALLBACK UserCsrProcessKey;

/* FUNCTIONS *****************************************************************/

BOOL STDCALL
DllMain(HANDLE hDll,
	DWORD dwReason,
	LPVOID lpReserved)
{
  return TRUE;
}

BOOL STDCALL
UserCsrInitialization(PUSERCSRFUNCS Funcs,
                      HANDLE CsrssApiHeap,
                      USERCSRPROCESSKEYCALLBACK ProcessKey)
{
  UserCsrApiHeap = CsrssApiHeap;
  UserCsrProcessKey = ProcessKey;

  GuiConsoleInitConsoleSupport();

  Funcs->InitConsole = GuiConsoleInitConsole;
  Funcs->DrawRegion = GuiConsoleDrawRegion;
  Funcs->CopyRegion = GuiConsoleCopyRegion;
  Funcs->ChangeTitle = GuiConsoleChangeTitle;
  Funcs->DeleteConsole = GuiConsoleDeleteConsole;

  return TRUE;
}

/* EOF */
