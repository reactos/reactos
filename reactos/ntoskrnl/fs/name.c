/* $Id: name.c,v 1.1 2000/02/24 23:25:16 ea Exp $
 *
 * reactos/ntoskrnl/fs/name.c
 *
 */
#include <ntos.h>

/* DATA */

PUCHAR	* FsRtlLegalAnsiCharacterArray;


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlAreNamesEqual@16
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 */
BOOLEAN
STDCALL
FsRtlAreNamesEqual (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	FsRtlDissectName@16
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
FsRtlDissectName (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
}


/**********************************************************************
 * NAME							EXPORTED
 * 	FsRtlDoesNameContainWildCards@4
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * NOTE
 * 	From Bo Branten's ntifs.h v12.
 */
BOOLEAN
STDCALL
FsRtlDoesNameContainWildCards (
	IN	PUNICODE_STRING	Name
	)
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	FsRtlIsNameInExpression@16
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * NOTE
 * 	From Bo Branten's ntifs.h v12.
 */
BOOLEAN
STDCALL
FsRtlIsNameInExpression (
	IN	PUNICODE_STRING	Expression,
	IN	PUNICODE_STRING	Name,
	IN	BOOLEAN		IgnoreCase,
	IN	PWCHAR		UpcaseTable	OPTIONAL
	)
{
	return FALSE;
}


/* EOF */
