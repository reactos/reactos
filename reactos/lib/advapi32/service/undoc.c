/* $Id: undoc.c,v 1.1 2000/03/26 22:00:07 dwelch Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/service/undoc.c
 * PURPOSE:         Undocumented service functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *	19990413 EA	created
 *	19990515 EA
 */

/* INCLUDES ******************************************************************/

#include <windows.h>

/* FUNCTIONS *****************************************************************/

/**********************************************************************
 *	I_ScSetServiceBitsA
 *
 * Undocumented
 *
 * Return value unknown.
 */
DWORD
STDCALL
I_ScSetServiceBitsA(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 1;
}


/**********************************************************************
 *	I_ScSetServiceBitsW
 *
 * Undocumented
 *
 * Return value unknown.
 */
DWORD
STDCALL
I_ScSetServiceBitsW(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 1;
}


