/* $Id: dbcsname.c,v 1.5 2004/08/15 16:39:01 chorns Exp $
 *
 * reactos/ntoskrnl/fs/dbcsname.c
 *
 */

#include <ntoskrnl.h>


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlDissectDbcs@16
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
VOID
STDCALL
FsRtlDissectDbcs (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlDoesDbcsContainWildCards@4
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
BOOLEAN
STDCALL
FsRtlDoesDbcsContainWildCards (
	DWORD	Unknown0
	)
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlIsDbcsInExpression@8
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
BOOLEAN
STDCALL
FsRtlIsDbcsInExpression (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlIsFatDbcsLegal@20
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
BOOLEAN
STDCALL
FsRtlIsFatDbcsLegal (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlIsHpfsDbcsLegal@20
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
BOOLEAN
STDCALL
FsRtlIsHpfsDbcsLegal (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	return FALSE;
}


/* EOF */
