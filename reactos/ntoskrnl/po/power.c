/* $Id: power.c,v 1.1 1999/08/20 16:31:17 ea Exp $
 *
 * reactos/ntoskrnl/po/power.c
 *
 */
#include <ntos.h>


/**********************************************************************
 * NAME							EXPORTED
 *	PoQueryPowerSequence@0
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *	WNT400: always 1
 *	WNT500: ?
 *
 * NOTES
 *	Not available in versions before 400 and actually 
 *	implemented only in versions >= 500.
 */
DWORD
STDCALL
PoQueryPowerSequence (
	VOID
	)
{
	return 1;
}


/**********************************************************************
 * NAME							EXPORTED
 *	PoRequestPowerChange@12
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *	Unknown0
 *		?
 *	Unknown1
 *		?
 *	Unknown2
 *		?
 *
 * RETURN VALUE
 *	WNT400: STATUS_NOT_IMPLEMENTED
 *	WNT500: ?
 *
 * NOTES
 *	Not available in versions before 400 and actually 
 *	implemented only in versions >= 500.
 */
DWORD
STDCALL
PoRequestPowerChange (
	DWORD	Unknown0,
	DWORD	Unknonw1,
	DWORD	Unknonw2
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/**********************************************************************
 * NAME							EXPORTED
 *	PoSetDeviceIdleDetection@8
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *	Unknown0
 *		?
 *	Unknown1
 *		?
 *
 * RETURN VALUE
 *	NONE
 *
 * NOTES
 *	Not available in versions before 400 and actually 
 *	implemented only in versions >= 500.
 */
VOID
STDCALL
PoSetDeviceIdleDetection (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
}


/* EOF */
