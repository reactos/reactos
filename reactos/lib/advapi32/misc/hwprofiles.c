/* $Id: hwprofiles.c,v 1.1 2004/02/25 23:12:39 sedwards Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/misc/sysfun.c
 * PURPOSE:         advapi32.dll Hardware Functions
 * PROGRAMMER:      Steven Edwards
 * UPDATE HISTORY:
 *	20042502
 */
#include <debug.h>
#include <windows.h>

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
BOOL STDCALL GetCurrentHwProfileA(LPHW_PROFILE_INFOA pInfo)
{
	DPRINT("GetCurrentHwProfileA stub\n");
	return 1;
}
