/* $Id: hwprofiles.c,v 1.4 2004/09/13 12:13:35 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/misc/hwprofiles.c
 * PURPOSE:         advapi32.dll Hardware Functions
 * PROGRAMMER:      Steven Edwards
 * UPDATE HISTORY:
 *	20042502
 */

#include "advapi32.h"
#include <debug.h>

/******************************************************************************
 * GetCurrentHwProfileA [ADVAPI32.@]
 *
 * Get the current hardware profile.
 *
 * PARAMS
 *  pInfo [O] Destination for hardware profile information.
 *
 * RETURNS
 *  Success: TRUE. pInfo is updated with the hardware profile details.
 *  Failure: FALSE.
 */
BOOL STDCALL
GetCurrentHwProfileA(LPHW_PROFILE_INFOA pInfo)
{
  DPRINT("GetCurrentHwProfileA stub\n");
  return TRUE;
}


BOOL STDCALL
GetCurrentHwProfileW(LPHW_PROFILE_INFOW pInfo)
{
  DPRINT("GetCurrentHwProfileW stub\n");
  return TRUE;
}
