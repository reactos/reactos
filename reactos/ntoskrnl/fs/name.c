/* $Id: name.c,v 1.10 2004/08/18 02:32:00 navaraf Exp $
 *
 * reactos/ntoskrnl/fs/name.c
 *
 */

#include <ntoskrnl.h>

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
  while ((NameOffset + NameLength < Length) &&
         (Name.Buffer[NameOffset + NameLength] != L'\\'))
  {
    NameLength++;
  }

  FirstPart->Length = 
  FirstPart->MaximumLength = NameLength * sizeof(WCHAR);
  FirstPart->Buffer = &Name.Buffer[NameOffset];

  NameOffset += NameLength + 1;
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
  Ptr = Name->Buffer + (Name->Length / sizeof(WCHAR));

  while (Ptr >= Name->Buffer)
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
 * @implemented
 */
BOOLEAN STDCALL
FsRtlIsNameInExpression (IN PUNICODE_STRING Expression,
			 IN PUNICODE_STRING Name,
			 IN BOOLEAN IgnoreCase,
			 IN PWCHAR UpcaseTable OPTIONAL)
{
  USHORT ExpressionPosition, NamePosition;
  UNICODE_STRING TempExpression, TempName;

  ExpressionPosition = 0;
  NamePosition = 0;
  while (ExpressionPosition < (Expression->Length / sizeof(WCHAR)) &&
         NamePosition < (Name->Length / sizeof(WCHAR)))
    {
      if (Expression->Buffer[ExpressionPosition] == L'*')
        {
          ExpressionPosition++;
          if (ExpressionPosition == (Expression->Length / sizeof(WCHAR)))
            {
              return TRUE;
            }
          while (NamePosition < (Name->Length / sizeof(WCHAR)))
            {
              TempExpression.Length =
              TempExpression.MaximumLength =
                Expression->Length - (ExpressionPosition * sizeof(WCHAR));
              TempExpression.Buffer = Expression->Buffer + ExpressionPosition;
              TempName.Length =
              TempName.MaximumLength =
                Name->Length - (NamePosition * sizeof(WCHAR));
              TempName.Buffer = Name->Buffer + NamePosition;
              /* FIXME: Rewrite to get rid of recursion */
              if (FsRtlIsNameInExpression(&TempExpression, &TempName,
                                          IgnoreCase, UpcaseTable))
                {
                  return TRUE;
                }
              NamePosition++;
            }
        }

      /* FIXME: Take UpcaseTable into account! */
      if (Expression->Buffer[ExpressionPosition] == L'?' ||
          (IgnoreCase &&
           RtlUpcaseUnicodeChar(Expression->Buffer[ExpressionPosition]) ==
           RtlUpcaseUnicodeChar(Name->Buffer[NamePosition])) ||
          (!IgnoreCase &&
           Expression->Buffer[ExpressionPosition] ==
           Name->Buffer[NamePosition]))
        {
          NamePosition++;
          ExpressionPosition++;
        }
      else
        {
          return FALSE;
        }
    }

  if (ExpressionPosition == (Expression->Length / sizeof(WCHAR)) &&
      NamePosition == (Name->Length / sizeof(WCHAR)))
    {
      return TRUE;
    }

  return FALSE;
}

/* EOF */
