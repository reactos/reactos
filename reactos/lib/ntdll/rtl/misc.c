/* $Id: misc.c,v 1.2 2000/08/15 12:41:13 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Various functions
 * FILE:              lib/ntdll/rtl/misc.c
 * PROGRAMER:         Eric Kohl <ekohl@zr-online.de>
 * REVISION HISTORY:
 *                    10/08/2000: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <napi/shared_data.h>

/* GLOBALS ******************************************************************/

extern ULONG NtGlobalFlag;

/* FUNCTIONS ****************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 * 	RtlGetNtGlobalFlags
 *
 * DESCRIPTION
 *	Retrieves the global os flags.
 *
 * ARGUMENTS
 *	None
 *
 * RETURN VALUE
 *	global flags
 *
 * REVISIONS
 * 	2000-08-10 ekohl
 */

ULONG STDCALL
RtlGetNtGlobalFlags(VOID)
{
   return (NtGlobalFlag);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	RtlGetNtProductType
 *
 * DESCRIPTION
 *	Retrieves the OS product type.
 *
 * ARGUMENTS
 *	ProductType	Pointer to the product type variable.
 *
 * RETURN VALUE
 *	TRUE if successful, otherwise FALSE
 *
 * NOTE
 *	ProductType can be one of the following values:
 *	  1	Workstation (Winnt)
 *	  2	Server (Lanmannt)
 *	  3	Advanced Server (Servernt)
 *
 * REVISIONS
 * 	2000-08-10 ekohl
 */

BOOLEAN STDCALL
RtlGetNtProductType(PNT_PRODUCT_TYPE ProductType)
{
   *ProductType = ((PKUSER_SHARED_DATA)USER_SHARED_DATA_BASE)->NtProductType;
   return TRUE;
}

/* EOF */
