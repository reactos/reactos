/* $Id: name.c,v 1.7 2003/07/10 16:43:10 ekohl Exp $
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
 * @implemented
 */
BOOLEAN STDCALL
FsRtlDoesNameContainWildCards (IN PUNICODE_STRING Name)
{
  PWCHAR Ptr;

  if (Name->Length == 0)
    return FALSE;

  /* Set pointer to last character of the string */
  Ptr = (PWCHAR)((ULONG_PTR)Name->Buffer + Name->Length - sizeof(WCHAR));

  while (Ptr > Name->Buffer)
    {
      /* Stop at backslash */
      if (*Ptr == L'\\')
	return FALSE;

      /* Check for wildcards */
      if ((*Ptr < '@') &&
	  (*Ptr == L'\"' || *Ptr == L'*' || *Ptr == L'<' ||
	   *Ptr == L'>' || *Ptr == L'?'))
	return TRUE;

      /* Move to previous character */
      Ptr--;
    }

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
