/* $Id: stubs.c,v 1.1 2004/08/10 00:09:40 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Access Control List Editor
 * FILE:            lib/acledit/stubs.c
 * PURPOSE:         acledit.dll stubs
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * NOTES:           If you implement a function, remove it from this file
 *
 *                  Based on guess work and on this nice article:
 *                    http://www.sysinternals.com/ntw2k/info/acledit.shtml
 *
 * UPDATE HISTORY:
 *      07/09/2004  Created
 */
#include <windows.h>
#include <aclui.h>
#include "internal.h"

ULONG DbgPrint(PCH Format,...);

#define UNIMPLEMENTED \
  DbgPrint("ACLUI:  %s at %s:%d is UNIMPLEMENTED!\n",__FUNCTION__,__FILE__,__LINE__)


HPROPSHEETPAGE
WINAPI
CreateSecurityPage(LPSECURITYINFO psi)
{
  UNIMPLEMENTED;
  return NULL;
}

BOOL
WINAPI
EditSecurity(HWND hwndOwner, LPSECURITYINFO psi)
{
  UNIMPLEMENTED;
  return FALSE;
}

/* EOF */
