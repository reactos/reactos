/* $Id: name.c,v 1.6 2003/07/10 06:27:13 royce Exp $
 *
 * reactos/ntoskrnl/fs/name.c
 *
 */
#include <ntos.h>

/* DATA */

PUCHAR	* FsRtlLegalAnsiCharacterArray = NULL;


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
 * NOTE
 * 	From Bo Branten's ntifs.h v25.
 *
 * @unimplemented
 */
BOOLEAN
STDCALL
FsRtlAreNamesEqual (
	IN	PUNICODE_STRING	Name1,
	IN	PUNICODE_STRING	Name2,
	IN	BOOLEAN		IgnoreCase,
	IN	PWCHAR		UpcaseTable	OPTIONAL
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
 * @unimplemented
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
 *
 * @unimplemented
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
 *
 * @unimplemented
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
