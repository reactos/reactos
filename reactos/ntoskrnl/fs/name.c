/* $Id: name.c,v 1.13 2004/12/30 18:30:05 ion Exp $
 *
 * reactos/ntoskrnl/fs/name.c
 *
 */

#include <ntoskrnl.h>

/* FUNCTIONS ***************************************************************/

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
 * @implemented
 */
BOOLEAN STDCALL
FsRtlAreNamesEqual (IN PUNICODE_STRING Name1,
		    IN PUNICODE_STRING Name2,
		    IN BOOLEAN IgnoreCase,
		    IN PWCHAR UpcaseTable OPTIONAL)
{

    UNICODE_STRING UpcaseName1;
    UNICODE_STRING UpcaseName2;
    BOOLEAN StringsAreEqual;
    
    /* Well, first check their size */
    if (Name1->Length != Name2->Length) {
        /* Not equal! */
        return FALSE;
    }
    
    /* Turn them into Upcase if we don't have a table */
    if (IgnoreCase && !UpcaseTable) {
        RtlUpcaseUnicodeString(&UpcaseName1, Name1, TRUE);
        RtlUpcaseUnicodeString(&UpcaseName2, Name2, TRUE);
        Name1 = &UpcaseName1;
        Name2 = &UpcaseName2;

        goto ManualCase;       
    }
    
    /* Do a case-sensitive search */
    if (!IgnoreCase) {
        
ManualCase:
        /* Use a raw memory compare */
        StringsAreEqual = RtlEqualMemory(Name1->Buffer,
                                         Name2->Buffer,
                                         Name1->Length);
        
        /* Clear the strings if we need to */
        if (IgnoreCase) {
            RtlFreeUnicodeString(&UpcaseName1);
            RtlFreeUnicodeString(&UpcaseName2);
        }
        
        /* Return the equality */
        return StringsAreEqual;
    
    } else {
        
        /* Case in-sensitive search */
        
        LONG i;
        
        for (i = Name1->Length / sizeof(WCHAR) - 1; i >= 0; i--) {
            
            if (UpcaseTable[Name1->Buffer[i]] != UpcaseTable[Name2->Buffer[i]]) {
                
                /* Non-match found! */
                return FALSE;
            }
        }   
        
        /* We finished the loop so we are equal */
        return TRUE;
    }
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

    /* Loop through every character */
    if (Name->Length) {
        for (Ptr = Name->Buffer + (Name->Length / sizeof(WCHAR))-1;
             Ptr >= Name->Buffer && *Ptr != L'\\';Ptr--) {

            /* Check for Wildcard */
            if (FsRtlIsUnicodeCharacterWild(*Ptr)) {
                return TRUE;
            }
        }
    }

    /* Nothing Found */
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
 * 	From Bo Branten's ntifs.h v12. This function should be rewritten
 *      to avoid recursion and better wildcard handling should be
 *      implemented (see FsRtlDoesNameContainWildCards).
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
      else
        {
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
    }

  /* Handle matching of "f0_*.*" expression to "f0_000" file name. */
  if (ExpressionPosition < (Expression->Length / sizeof(WCHAR)) &&
      Expression->Buffer[ExpressionPosition] == L'.')
    {
      while (ExpressionPosition < (Expression->Length / sizeof(WCHAR)) &&
             (Expression->Buffer[ExpressionPosition] == L'.' ||
              Expression->Buffer[ExpressionPosition] == L'*' ||
              Expression->Buffer[ExpressionPosition] == L'?'))
        {
          ExpressionPosition++;
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
