/* $Id: hwprofiles.c,v 1.3 2004/08/15 17:03:14 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/misc/sysfun.c
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
	return 1;
}
