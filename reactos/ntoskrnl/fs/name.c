/* $Id: name.c,v 1.8 2003/12/17 20:26:28 ekohl Exp $
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
BOOLEAN STDCALL
FsRtlAreNamesEqual (IN PUNICODE_STRING Name1,
		    IN PUNICODE_STRING Name2,
		    IN BOOLEAN IgnoreCase,
		    IN PWCHAR UpcaseTable OPTIONAL)
{
  return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	FsRtlDissectName@16
 *
 * DESCRIPTION
 *	Dissects a given path name into first and remaining part.
 *
 * ARGUMENTS
 *	Name
 *		Unicode string to dissect.
 *
 *	FirstPart
 *		Pointer to user supplied UNICODE_STRING, that will
 *		later point to the first part of the original name.
 *
 *	RemainingPart
 *		Pointer to user supplied UNICODE_STRING, that will
 *		later point to the remaining part of the original name.
 *
 * RETURN VALUE
 *	None
 *
 * EXAMPLE
 *	Name:		\test1\test2\test3
 *	FirstPart:	test1
 *	RemainingPart:	test2\test3
 *
 * @implemented
 */
VOID STDCALL
FsRtlDissectName (IN UNICODE_STRING Name,
		  OUT PUNICODE_STRING FirstPart,
		  OUT PUNICODE_STRING RemainingPart)
{
  USHORT NameOffset = 0;
  USHORT NameLength = 0;
  USHORT Length;

  FirstPart->Length = 0;
  FirstPart->MaximumLength = 0;
  FirstPart->Buffer = NULL;

  RemainingPart->Length = 0;
  RemainingPart->MaximumLength = 0;
  RemainingPart->Buffer = NULL;

  if (Name.Length == 0)
    return;

  /* Skip leading backslash */
  if (Name.Buffer[0] == L'\\')
    NameOffset++;

  Length = Name.Length / sizeof(WCHAR);

  /* Search for next backslash or end-of-string */
  while ((NameOffset + NameLength <= Length) &&
         (Name.Buffer[NameOffset + NameLength] != L'\\'))
  {
    NameLength++;
  }

  FirstPart->Length = NameLength * sizeof(WCHAR);
  FirstPart->MaximumLength = NameLength * sizeof(WCHAR);
  FirstPart->Buffer = &Name.Buffer[NameOffset];

  NameOffset += (NameLength + 1);
  if (NameOffset < Length)
  {
    RemainingPart->Length = (Length - NameOffset) * sizeof(WCHAR);
    RemainingPart->MaximumLength = (Length - NameOffset) * sizeof(WCHAR);
    RemainingPart->Buffer = &Name.Buffer[NameOffset];
  }
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
      if ((*Ptr < L'@') &&
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
BOOLEAN STDCALL
FsRtlIsNameInExpression (IN PUNICODE_STRING Expression,
			 IN PUNICODE_STRING Name,
			 IN BOOLEAN IgnoreCase,
			 IN PWCHAR UpcaseTable OPTIONAL)
{
  return FALSE;
}

/* EOF */
