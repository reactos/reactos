/* $Id: stubs.c,v 1.3 2004/08/10 15:47:54 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Access Control List Editor
 * FILE:            lib/aclui/stubs.c
 * PURPOSE:         aclui.dll stubs
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * NOTES:           If you implement a function, remove it from this file
 *
 * UPDATE HISTORY:
 *      08/10/2004  Created
 */
#include <windows.h>
#include <aclui.h>
#include "internal.h"

#define UNIMPLEMENTED \
  DbgPrint("ACLUI:  %s at %s:%d is UNIMPLEMENTED!\n",__FUNCTION__,__FILE__,__LINE__)


HPROPSHEETPAGE
WINAPI
CreateSecurityPage(LPSECURITYINFO psi)
{
  UNIMPLEMENTED;
  return NULL;
}

/* EOF */
