/* $Id: unc.c,v 1.1 2000/01/20 22:14:07 ea Exp $
 *
 * reactos/ntoskrnl/fs/unc.c
 *
 */
#include <ntos.h>


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlDeregisterUncProvider@4
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
VOID
STDCALL
FsRtlDeregisterUncProvider (
	DWORD	Unknown0
	)
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlRegisterUncProvider@12
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
DWORD
STDCALL
FsRtlRegisterUncProvider (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	return 0;
}


/* EOF */
