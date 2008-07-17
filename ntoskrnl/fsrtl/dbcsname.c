/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/name.c
 * PURPOSE:         Provides DBCS parsing and other support routines for FSDs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlDissectDbcs
 * @implemented
 *
 * Dissects a given path name into first and remaining part.
 *
 * @param Name
 *        ANSI string to dissect.
 *
 * @param FirstPart
 *        Pointer to user supplied ANSI_STRING, that will later point
 *        to the first part of the original name.
 *
 * @param RemainingPart
 *        Pointer to user supplied ANSI_STRING, that will later point
 *        to the remaining part of the original name.
 *
 * @return None
 *
 * @remarks Example:
 *          Name:           \test1\test2\test3
 *          FirstPart:      test1
 *          RemainingPart:  test2\test3
 *
 *--*/
VOID
NTAPI
FsRtlDissectDbcs(IN ANSI_STRING Name,
                 OUT PANSI_STRING FirstPart,
                 OUT PANSI_STRING RemainingPart)
{
    ULONG FirstPosition, i;
    ULONG SkipFirstSlash = 0;

    /* Just quit if the string is empty */
    if (!Name.Length) return;

    /* Find first backslash */
    FirstPosition = Name.Length / sizeof(CHAR) ;
    for (i = 0; i < Name.Length / sizeof(CHAR); i++)
    {
        /* If we found one... */
        if (Name.Buffer[i] == '\\')
        {
            /* If it begins string, just notice it and continue */
            if (i == 0)
            {
                SkipFirstSlash = 1;
            }
            else
            {
                /* Else, save its position and break out of the loop */
                FirstPosition = i;
                break;
            }
        }
    }

    /* Set up the first result string */
    FirstPart->Buffer = Name.Buffer + SkipFirstSlash;
    FirstPart->Length = (FirstPosition - SkipFirstSlash) * sizeof(CHAR);
    FirstPart->MaximumLength = Name.MaximumLength - FirstPart->Length;

    /* And second one, if necessary */
    if (FirstPosition < (Name.Length / sizeof(CHAR)))
    {
        RemainingPart->Buffer = Name.Buffer + FirstPosition + 1;
        RemainingPart->Length = (Name.Length - FirstPosition) * sizeof(CHAR);
        RemainingPart->MaximumLength = Name.MaximumLength - RemainingPart->Length;
    }
}

/*++
 * @name FsRtlDoesDbcsContainWildCards
 * @implemented
 *
 * Returns TRUE if the given DbcsName contains wildcards such as *, ?, 
 * ANSI_DOS_STAR, ANSI_DOS_DOT, and ANSI_DOS_QM
 *
 * @param Name
 *        The Name to check
 *
 * @return TRUE if there are wildcards, FALSE otherwise
 *
 * @remarks None
 *
 *--*/
BOOLEAN
NTAPI
FsRtlDoesDbcsContainWildCards(IN PANSI_STRING Name)
{
    ULONG i;

    /* Check every character */
    for (i = 0; i < Name->Length; i++)
    {
        /* First make sure it's not the Lead DBCS */
        if (FsRtlIsLeadDbcsCharacter(Name->Buffer[i]))
        {
            i++;
        }
        else if (FsRtlIsAnsiCharacterWild(Name->Buffer[i]))
        {
            /* Now return if it has a wildcard */
            return TRUE;
        }
    }

    /* We didn't return above...so none found */
    return FALSE;
}

/*++
 * @name FsRtlIsDbcsInExpression
 * @unimplemented
 *
 * FILLME
 *
 * @param Expression
 *        FILLME
 *
 * @param Name
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
BOOLEAN
NTAPI
FsRtlIsDbcsInExpression(IN PANSI_STRING Expression,
                        IN PANSI_STRING Name)
{
    KEBUGCHECK(0);
    return FALSE;
}

/*++
 * @name FsRtlIsFatDbcsLegal
 * @implemented
 *
 * Returns TRUE if the given DbcsName is a valid FAT filename (in 8.3)
 *
 * @param DbcsName
 *        The filename to check. It can also contains pathname.
 *
 * @param WildCardsPermissible
 *        If this is set to FALSE and if filename contains wildcard, the function 
 *        will fail
 *
 * @param PathNamePermissible
 *        If this is set to FALSE and if the filename comes with a pathname, the
 *        function will fail
 *
 * @param LeadingBackslashPermissible
 *        If this is set to FALSE and if the filename starts with a backslash, the
 *        function will fail
 *
 * @return TRUE if the DbcsName is legal, FALSE otherwise
 *
 * @remarks None
 *
 *--*/
BOOLEAN
NTAPI
FsRtlIsFatDbcsLegal(IN ANSI_STRING DbcsName,
                    IN BOOLEAN WildCardsPermissible,
                    IN BOOLEAN PathNamePermissible,
                    IN BOOLEAN LeadingBackslashPermissible)
{
    BOOLEAN FirstSlash = FALSE, IsIllegal = FALSE;
    ULONG i, LastSlash = 0, LastDot = 0;
    ANSI_STRING FileName;
    
    /* Just quit if the string is empty */
    if (!DbcsName.Length)
        return FALSE;

    /* Check if we have a filename and a pathname
     * Continue until we get the last one to extract the filename
     */     
    for (i = 0; i < DbcsName.Length / sizeof(CHAR); i++)
    {
        if (DbcsName.Buffer[i] == '\\')
        {
            LastSlash = i;
            if (!i)
            {
                FirstSlash = TRUE;
            }
        }
    }

    /* DbcsName was to be a filename */
    if (!PathNamePermissible && LastSlash)
        return FALSE;

    /* DbcsName wasn't supposed to be started with \ if it's a filename */
    if (!LeadingBackslashPermissible && !LastSlash && FirstSlash)
        return FALSE;

    /* Now, only work on filename */
    if (LastSlash)
    {
        FileName.Buffer = DbcsName.Buffer + LastSlash + 1;
        FileName.Length = (DbcsName.Length - LastSlash) * sizeof(CHAR);
        FileName.MaximumLength = DbcsName.MaximumLength - FileName.Length;
        
    }
    else
    {
        FileName.Buffer = DbcsName.Buffer;
        FileName.Length = DbcsName.Length;
        FileName.MaximumLength = DbcsName.MaximumLength;
    }

    /* Filename must be 8.3 filename */
    if (FileName.Length < 3 || FileName.Length > 12)
        return FALSE;

    if (!WildCardsPermissible && FsRtlDoesDbcsContainWildCards(&FileName))
        return FALSE;

    /* Now, we will parse the filename to find everything bad in
     * It mustn't contain:
     *   0x00-0x1F, 0x22, 0x2B, 0x2C, 0x2F, 0x3A, 0x3B, 0x3D, 0x5B, 0x5D, 0x7C */
    for (i = 0; i < FileName.Length / sizeof(CHAR); i++)
    {
        if ((FileName.Buffer[i] < 0x1F) || (FileName.Buffer[i] == 0x22) ||
            (FileName.Buffer[i] == 0x2B) || (FileName.Buffer[i] == 0x2C) ||
            (FileName.Buffer[i] == 0x2F) || (FileName.Buffer[i] == 0x3A) ||
            (FileName.Buffer[i] == 0x3B) || (FileName.Buffer[i] == 0x3D) ||
            (FileName.Buffer[i] == 0x5B) || (FileName.Buffer[i] == 0x5D) ||
            (FileName.Buffer[i] == 0x7C))
        {
            IsIllegal = TRUE;
            break;
        }
        if (FileName.Buffer[i] == '.')
        {
            LastDot = i;
            if (!i)
            {
                IsIllegal = TRUE;
                break;
            }
        }
    }
    if (IsIllegal || LastDot == (FileName.Length / sizeof(CHAR)) - 1)
        return FALSE;

    /* We mustn't have spaces before dot or at the end of the filename */
    if ((LastDot && FileName.Buffer[LastDot - 1] == ' ') ||
        (FileName.Buffer[FileName.Length / sizeof(CHAR) - 1] == ' '))
        return FALSE;

    /* Filename must be 8.3 filename and not 3.8 filename */
    if (LastDot && ((FileName.Length / sizeof(CHAR) - 1) - LastDot > 3))
        return FALSE;

    /* We have a valid filename */
    return TRUE;
}

/*++
 * @name FsRtlIsHpfsDbcsLegal
 * @unimplemented
 *
 * FILLME
 *
 * @param DbcsName
 *        FILLME
 *
 * @param WildCardsPermissible
 *        FILLME
 *
 * @param PathNamePermissible
 *        FILLME
 *
 * @param LeadingBackslashPermissible
 *        FILLME
 *
 * @return TRUE if the DbcsName is legal, FALSE otherwise
 *
 * @remarks None
 *
 *--*/
BOOLEAN
STDCALL
FsRtlIsHpfsDbcsLegal(IN ANSI_STRING DbcsName,
                     IN BOOLEAN WildCardsPermissible,
                     IN BOOLEAN PathNamePermissible,
                     IN BOOLEAN LeadingBackslashPermissible)
{
    KEBUGCHECK(0);
    return FALSE;
}
