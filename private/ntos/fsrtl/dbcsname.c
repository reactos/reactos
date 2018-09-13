/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DbcsName.c

Abstract:

    The name support package is for manipulating DBCS strings.  The routines
    allow the caller to dissect and compare strings.

    The following routines are provided by this package:

      o  FsRtlIsFatDbcsLegal - This routine takes an input dbcs
         string and determines if it describes a legal name or path.

      o  FsRtlIsHpfsDbcsLegal - This routine takes an input dbcs
         string and determines if it describes a legal name or path.

      o  FsRtlDissectDbcs - This routine takes a path name string and
         breaks into two parts.  The first name in the string and the
         remainder.

      o  FsRtlDoesDbcsContainWildCards - This routines tells the caller if
         a string contains any wildcard characters.

      o  FsRtlIsDbcsInExpression - This routine is used to compare a string
         against a template (possibly containing wildcards) to sees if the
         string is in the language denoted by the template.

Author:

    Gary Kimura     [GaryKi]    5-Feb-1990

Revision History:

--*/

#include "FsRtlP.h"

//
//  Trace level for the module
//

#define Dbg                              (0x10000000)

//
//  Some special debugging code
//

#if DBG

ULONG DaveDebug = 0;
#define DavePrint if (DaveDebug) DbgPrint

#else

#define DavePrint NOTHING

#endif

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('drSF')

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FsRtlDissectDbcs)
#pragma alloc_text(PAGE, FsRtlDoesDbcsContainWildCards)
#pragma alloc_text(PAGE, FsRtlIsDbcsInExpression)
#pragma alloc_text(PAGE, FsRtlIsFatDbcsLegal)
#pragma alloc_text(PAGE, FsRtlIsHpfsDbcsLegal)
#endif


BOOLEAN
FsRtlIsFatDbcsLegal (
    IN ANSI_STRING DbcsName,
    IN BOOLEAN WildCardsPermissible,
    IN BOOLEAN PathNamePermissible,
    IN BOOLEAN LeadingBackslashPermissible
    )

/*++

Routine Description:

    This routine simple returns whether the specified file names conforms
    to the file system specific rules for legal file names.  This routine
    will check the single name, or if PathNamePermissible is specified as
    TRUE, whether the whole path is a legal name.

    For FAT, the following rules apply:

    A. A Fat file name may not contain any of the following characters:

       0x00-0x1F " / : | + , ; = [ ]

    B. A Fat file name is either of the form N.E or just N, where N is a
       string of 1-8 bytes and E is a string of 1-3 bytes conformant to
       rule A above. In addition, neither N nor E may contain a period
       character or end with a space character.

       Incidently, N corresponds to name and E to extension.

    Case: Lower case characters are taken as valid, but are up-shifted upon
          receipt, ie. Fat only deals with upper case file names.

    For example, the files ".foo", "foo.", and "foo .b" are illegal, while
    "foo. b" and " bar" are legal.

Arguments:

    DbcsName - Supllies the name/path to check.

    WildCardsPermissible - Specifies if Nt wild card characters are to be
        considered considered legal.

    PathNamePermissible - Spcifes if Name may be a path name separated by
        backslash characters, or just a simple file name.

    LeadingBackSlashPermissible - Specifies if a single leading backslash
        is permissible in the file/path name.

Return Value:

    BOOLEAN - TRUE if the name is legal, FALSE otherwise.

--*/
{
    BOOLEAN ExtensionPresent = FALSE;

    ULONG Index;

    UCHAR Char;

    PAGED_CODE();

    //
    //  Empty names are not valid.
    //

    if ( DbcsName.Length == 0 ) { return FALSE; }

    //
    //  If Wild Cards are OK, then for directory enumeration to work
    //  correctly we have to accept . and ..
    //

    if ( WildCardsPermissible &&
         ( ( (DbcsName.Length == 1) &&
             ((DbcsName.Buffer[0] == '.') ||
              (DbcsName.Buffer[0] == ANSI_DOS_DOT)) )
           ||
           ( (DbcsName.Length == 2) &&
             ( ((DbcsName.Buffer[0] == '.') &&
                (DbcsName.Buffer[1] == '.')) ||
               ((DbcsName.Buffer[0] == ANSI_DOS_DOT) &&
                (DbcsName.Buffer[1] == ANSI_DOS_DOT)) ) ) ) ) {

        return TRUE;
    }

    //
    //  If a leading \ is OK, skip over it (if there's more)
    //

    if ( DbcsName.Buffer[0] == '\\' ) {

        if ( LeadingBackslashPermissible ) {

            if ( (DbcsName.Length > 1) ) {

                DbcsName.Buffer += 1;
                DbcsName.Length -= 1;

            } else { return TRUE; }

        } else { return FALSE; }
    }

    //
    //  If we got a path name, check each componant.
    //

    if ( PathNamePermissible ) {

        ANSI_STRING FirstName;
        ANSI_STRING RemainingName;

        RemainingName = DbcsName;

        while ( RemainingName.Length != 0 ) {

            //
            //  This will catch the case of an illegal double backslash.
            //

            if ( RemainingName.Buffer[0] == '\\' ) { return FALSE; }

            FsRtlDissectDbcs(RemainingName, &FirstName, &RemainingName);

            if ( !FsRtlIsFatDbcsLegal( FirstName,
                                       WildCardsPermissible,
                                       FALSE,
                                       FALSE) ) {

                return FALSE;
            }
        }

        //
        //  All the componants were OK, so the path is OK.
        //

        return TRUE;
    }

    //
    //  If this name contains wild cards, just check for invalid characters.
    //

    if ( WildCardsPermissible && FsRtlDoesDbcsContainWildCards(&DbcsName) ) {

        for ( Index = 0; Index < DbcsName.Length; Index += 1 ) {

            Char = DbcsName.Buffer[ Index ];

            //
            //  Skip over any Dbcs chacters
            //

            if ( FsRtlIsLeadDbcsCharacter( Char ) ) {

                ASSERT( Index != (ULONG)(DbcsName.Length - 1) );
                Index += 1;
                continue;
            }

            //
            //  Make sure this character is legal, and if a wild card, that
            //  wild cards are permissible.
            //

            if ( !FsRtlIsAnsiCharacterLegalFat(Char, WildCardsPermissible) ) {
                return FALSE;
            }
        }

        return TRUE;
    }


    //
    //  At this point we should only have a single name, which can't have
    //  more than 12 characters (including a single period)
    //

    if ( DbcsName.Length > 12 ) { return FALSE; }

    for ( Index = 0; Index < DbcsName.Length; Index += 1 ) {

        Char = DbcsName.Buffer[ Index ];

        //
        //  Skip over and Dbcs chacters
        //

        if ( FsRtlIsLeadDbcsCharacter( Char ) ) {

            //
            //  FsRtlIsFatDbcsLegal(): fat name part and extension part dbcs check
            //
            //  1) if we're looking at base part ( !ExtensionPresent ) and the 8th byte
            //     is in the dbcs leading byte range, it's error ( Index == 7 ). If the
            //     length of base part is more than 8 ( Index > 7 ), it's definitely error.
            //
            //  2) if the last byte ( Index == DbcsName.Length - 1 ) is in the dbcs leading
            //     byte range, it's error
            //

            if ( (!ExtensionPresent && (Index >= 7)) ||
                 ( Index == (ULONG)(DbcsName.Length - 1) ) ) {
                return FALSE;
            }

            Index += 1;

            continue;
        }

        //
        //  Make sure this character is legal, and if a wild card, that
        //  wild cards are permissible.
        //

        if ( !FsRtlIsAnsiCharacterLegalFat(Char, WildCardsPermissible) ) {

            return FALSE;
        }

        if ( (Char == '.') || (Char == ANSI_DOS_DOT) ) {

            //
            //  We stepped onto a period.  We require the following things:
            //
            //      - It can't be the first character
            //      - There can only be one
            //      - There can't be more than three characters following
            //      - The previous character can't be a space.
            //

            if ( (Index == 0) ||
                 ExtensionPresent ||
                 (DbcsName.Length - (Index + 1) > 3) ||
                 (DbcsName.Buffer[Index - 1] == ' ') ) {

                return FALSE;
            }

            ExtensionPresent = TRUE;
        }

        //
        //  The base part of the name can't be more than 8 characters long.
        //

        if ( (Index >= 8) && !ExtensionPresent ) { return FALSE; }
    }

    //
    //  The name cannot end in a space or a period.
    //

    if ( (Char == ' ') || (Char == '.') || (Char == ANSI_DOS_DOT)) { return FALSE; }

    return TRUE;
}

BOOLEAN
FsRtlIsHpfsDbcsLegal (
    IN ANSI_STRING DbcsName,
    IN BOOLEAN WildCardsPermissible,
    IN BOOLEAN PathNamePermissible,
    IN BOOLEAN LeadingBackslashPermissible
    )

/*++

Routine Description:

    This routine simple returns whether the specified file names conforms
    to the file system specific rules for legal file names.  This routine
    will check the single name, or if PathNamePermissible is specified as
    TRUE, whether the whole path is a legal name.

    For HPFS, the following rules apply:

    A. An HPFS file name may not contain any of the following characters:

       0x0000 - 0x001F  " / : < > ? | *

    B. An HPFS file name may not end in a period or a space.

    C. An HPFS file name must contain no more than 255 bytes.

    Case: HPFS is case preserving, but not case sensitive.  Case is
          preserved on creates, but not checked for on file name compares.

    For example, the files "foo " and "foo." are illegal, while ".foo",
    " foo" and "foo.bar.foo" are legal.

Arguments:

    DbcsName - Supllies the name/path to check.

    WildCardsPermissible - Specifies if Nt wild card characters are to be
        considered considered legal.

    PathNamePermissible - Spcifes if Name may be a path name separated by
        backslash characters, or just a simple file name.

    LeadingBackSlashPermissible - Specifies if a single leading backslash
        is permissible in the file/path name.

Return Value:

    BOOLEAN - TRUE if the name is legal, FALSE otherwise.

--*/
{
    BOOLEAN ExtensionPresent = FALSE;

    ULONG Index;

    UCHAR Char;

    PAGED_CODE();

    //
    //  Empty names are not valid.
    //

    if ( DbcsName.Length == 0 ) { return FALSE; }

    //
    //  If Wild Cards are OK, then for directory enumeration to work
    //  correctly we have to accept . and ..
    //

    if ( WildCardsPermissible &&
         ( ( (DbcsName.Length == 1) &&
             ((DbcsName.Buffer[0] == '.') ||
              (DbcsName.Buffer[0] == ANSI_DOS_DOT)) )
           ||
           ( (DbcsName.Length == 2) &&
             ( ((DbcsName.Buffer[0] == '.') &&
                (DbcsName.Buffer[1] == '.')) ||
               ((DbcsName.Buffer[0] == ANSI_DOS_DOT) &&
                (DbcsName.Buffer[1] == ANSI_DOS_DOT)) ) ) ) ) {

        return TRUE;
    }

    //
    //  If a leading \ is OK, skip over it (if there's more)
    //

    if ( DbcsName.Buffer[0] == '\\' ) {

        if ( LeadingBackslashPermissible ) {

            if ( (DbcsName.Length > 1) ) {

                DbcsName.Buffer += 1;
                DbcsName.Length -= 1;

            } else { return TRUE; }

        } else { return FALSE; }
    }

    //
    //  If we got a path name, check each componant.
    //

    if ( PathNamePermissible ) {

        ANSI_STRING FirstName;
        ANSI_STRING RemainingName;

        RemainingName = DbcsName;

        while ( RemainingName.Length != 0 ) {

            //
            //  This will catch the case of an illegal double backslash.
            //

            if ( RemainingName.Buffer[0] == '\\' ) { return FALSE; }

            FsRtlDissectDbcs(RemainingName, &FirstName, &RemainingName);

            if ( !FsRtlIsHpfsDbcsLegal( FirstName,
                                       WildCardsPermissible,
                                       FALSE,
                                       FALSE) ) {

                return FALSE;
            }
        }

        //
        //  All the componants were OK, so the path is OK.
        //

        return TRUE;
    }

    //
    //  At this point we should only have a single name, which can't have
    //  more than 255 characters
    //

    if ( DbcsName.Length > 255 ) { return FALSE; }

    for ( Index = 0; Index < DbcsName.Length; Index += 1 ) {

        Char = DbcsName.Buffer[ Index ];

        //
        //  Skip over and Dbcs chacters
        //

        if ( FsRtlIsLeadDbcsCharacter( Char ) ) {

            //
            //  FsRtlIsHpfsDbcsLegal () hpfs dbcs check
            //
            //  If the last byte ( Index == DbcsName.Length - 1 ) is in the
            //  dbcs leading byte range, it's error.
            //

            if ( Index == (ULONG)(DbcsName.Length - 1) ) {

                return FALSE;
            }

            Index += 1;
            continue;
        }

        //
        //  Make sure this character is legal, and if a wild card, that
        //  wild cards are permissible.
        //

        if ( !FsRtlIsAnsiCharacterLegalHpfs(Char, WildCardsPermissible) ) {

            return FALSE;
        }
    }

    //
    //  The name cannot end in a space or a period.
    //

    if ( (Char == ' ') || (Char == '.') || (Char == ANSI_DOS_DOT) ) {

        return FALSE;
    }

    return TRUE;
}


VOID
FsRtlDissectDbcs (
    IN ANSI_STRING Path,
    OUT PANSI_STRING FirstName,
    OUT PANSI_STRING RemainingName
    )

/*++

Routine Description:

    This routine takes an input Dbcs string and dissects it into two
    substrings.  The first output string contains the name that appears at
    the beginning of the input string, the second output string contains the
    remainder of the input string.

    In the input string backslashes are used to separate names.  The input
    string must not start with a backslash.  Both output strings will not
    begin with a backslash.

    If the input string does not contain any names then both output strings
    are empty.  If the input string contains only one name then the first
    output string contains the name and the second string is empty.

    Note that both output strings use the same string buffer memory of the
    input string.

    Example of its results are:

//. .     InputString    FirstPart    RemainingPart
//
//. .     empty          empty        empty
//
//. .     A              A            empty
//
//. .     A\B\C\D\E      A            B\C\D\E
//
//. .     *A?            *A?          empty
//
//. .     \A             A            empty
//
//. .     A[,]           A[,]         empty
//
//. .     A\\B+;\C       A            \B+;\C

Arguments:

    InputName - Supplies the input string being dissected

    Is8dot3 - Indicates if the first part of the input name must be 8.3
        or can be long file name.

    FirstPart - Receives the first name in the input string

    RemainingPart - Receives the remaining part of the input string

Return Value:

    NONE

--*/

{
    ULONG i = 0;
    ULONG PathLength;
    ULONG FirstNameStart;

    PAGED_CODE();

    //
    //  Make both output strings empty for now
    //

    FirstName->Length = 0;
    FirstName->MaximumLength = 0;
    FirstName->Buffer = NULL;

    RemainingName->Length = 0;
    RemainingName->MaximumLength = 0;
    RemainingName->Buffer = NULL;

    PathLength = Path.Length;

    //
    //  Check for an empty input string
    //

    if (PathLength == 0) {

        return;
    }

    //
    //  Skip over a starting backslash, and make sure there is more.
    //

    if ( Path.Buffer[0] == '\\' ) {

        i = 1;
    }

    //
    //  Now run down the input string until we hit a backslash or the end
    //  of the string, remembering where we started;
    //

    for ( FirstNameStart = i;
          (i < PathLength) && (Path.Buffer[i] != '\\');
          i += 1 ) {

        //
        //  If this is the first byte of a Dbcs character, skip over the
        //  next byte as well.
        //

        if ( FsRtlIsLeadDbcsCharacter( Path.Buffer[i] ) ) {

            i += 1;
        }
    }

    //
    //  At this point all characters up to (but not including) i are
    //  in the first part.   So setup the first name
    //

    FirstName->Length = (USHORT)(i - FirstNameStart);
    FirstName->MaximumLength = FirstName->Length;
    FirstName->Buffer = &Path.Buffer[FirstNameStart];

    //
    //  Now the remaining part needs a string only if the first part didn't
    //  exhaust the entire input string.  We know that if anything is left
    //  that is must start with a backslash.  Note that if there is only
    //  a trailing backslash, the length will get correctly set to zero.
    //

    if (i < PathLength) {

        RemainingName->Length = (USHORT)(PathLength - (i + 1));
        RemainingName->MaximumLength = RemainingName->Length;
        RemainingName->Buffer = &Path.Buffer[i + 1];
    }

    //
    //  And return to our caller
    //

    return;
}


BOOLEAN
FsRtlDoesDbcsContainWildCards (
    IN PANSI_STRING Name
    )

/*++

Routine Description:

    This routine checks if the input Dbcs name contains any wild card
    characters (i.e., *, ?, ANSI_DOS_STAR, or ANSI_DOS_QM).

Arguments:

    Name - Supplies the name to examine

Return Value:

    BOOLEAN - TRUE if the input name contains any wildcard characters and
        FALSE otherwise.

--*/

{
    CLONG i;

    PAGED_CODE();

    //
    //  Check each character in the name to see if it's a wildcard
    //  character
    //

    for (i = 0; i < Name->Length; i += 1) {

        //
        //  check for dbcs character because we'll just skip over those
        //

        if (FsRtlIsLeadDbcsCharacter( Name->Buffer[i] )) {

            i += 1;

        //
        //  else check for a wild card character
        //

        } else if (FsRtlIsAnsiCharacterWild( Name->Buffer[i] )) {

            //
            //  Tell caller that this name contains wild cards
            //

            return TRUE;
        }
    }

    //
    //  No wildcard characters were found, so return to our caller
    //

    return FALSE;
}

#define GetDbcs(BUF,OFFSET,DBCS_CHAR,LENGTH) {               \
    if (FsRtlIsLeadDbcsCharacter( (BUF)[(OFFSET)] )) {       \
        *(DBCS_CHAR) = (WCHAR)((BUF)[(OFFSET)] +             \
                               0x100 * (BUF)[(OFFSET) + 1]); \
        *(LENGTH) = 2;                                       \
    } else {                                                 \
        *(DBCS_CHAR) = (WCHAR)(BUF)[(OFFSET)];               \
        *(LENGTH) = 1;                                       \
    }                                                        \
}

#define MATCHES_ARRAY_SIZE 16

BOOLEAN
FsRtlIsDbcsInExpression (
    IN PANSI_STRING Expression,
    IN PANSI_STRING Name
    )

/*++

Routine Description:

    This routine compares a Dbcs name and an expression and tells the caller
    if the name is in the language defined by the expression.  The input name
    cannot contain wildcards, while the expression may contain wildcards.

    Expression wild cards are evaluated as shown in the nondeterministic
    finite automatons below.  Note that ~* and ~? are DOS_STAR and DOS_QM.


             ~* is DOS_STAR, ~? is DOS_QM, and ~. is DOS_DOT


                                       S
                                    <-----<
                                 X  |     |  e       Y
             X * Y ==       (0)----->-(1)->-----(2)-----(3)


                                      S-.
                                    <-----<
                                 X  |     |  e       Y
             X ~* Y ==      (0)----->-(1)->-----(2)-----(3)



                                X     S     S     Y
             X ?? Y ==      (0)---(1)---(2)---(3)---(4)



                                X     .        .      Y
             X ~.~. Y ==    (0)---(1)----(2)------(3)---(4)
                                   |      |________|
                                   |           ^   |
                                   |_______________|
                                      ^EOF or .^


                                X     S-.     S-.     Y
             X ~?~? Y ==    (0)---(1)-----(2)-----(3)---(4)
                                   |      |________|
                                   |           ^   |
                                   |_______________|
                                      ^EOF or .^



         where S is any single character

               S-. is any single character except the final .

               e is a null character transition

               EOF is the end of the name string

    In words:

        * matches 0 or more characters.

        ? matches exactly 1 character.

        DOS_STAR matches 0 or more characters until encountering and matching
            the final . in the name.

        DOS_QM matches any single character, or upon encountering a period or
            end of name string, advances the expression to the end of the
            set of contiguous DOS_QMs.

        DOS_DOT matches either a . or zero characters beyond name string.

Arguments:

    Expression - Supplies the input expression to check against

    Name - Supplies the input name to check for.

Return Value:

    BOOLEAN - TRUE if Name is an element in the set of strings denoted
        by the input Expression and FALSE otherwise.

--*/

{
    USHORT NameOffset;
    USHORT ExprOffset;
    USHORT Length;

    ULONG SrcCount;
    ULONG DestCount;
    ULONG PreviousDestCount;
    ULONG MatchesCount;

    WCHAR NameChar, ExprChar;

    USHORT LocalBuffer[MATCHES_ARRAY_SIZE * 2];

    USHORT *AuxBuffer = NULL;
    USHORT *PreviousMatches;
    USHORT *CurrentMatches;

    USHORT MaxState;
    USHORT CurrentState;

    BOOLEAN NameFinished = FALSE;

    //
    //  The idea behind the algorithm is pretty simple.  We keep track of
    //  all possible locations in the regular expression that are matching
    //  the name.  If when the name has been exhausted one of the locations
    //  in the expression is also just exhausted, the name is in the language
    //  defined by the regular expression.
    //

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FsRtlIsDbcsInExpression\n", 0);
    DebugTrace( 0, Dbg, " Expression      = %Z\n", Expression );
    DebugTrace( 0, Dbg, " Name            = %Z\n", Name );

    ASSERT( Name->Length != 0 );
    ASSERT( Expression->Length != 0 );

    //
    //  If one string is empty return FALSE.  If both are empty return TRUE.
    //

    if ( (Name->Length == 0) || (Expression->Length == 0) ) {

        return (BOOLEAN)(!(Name->Length + Expression->Length));
    }

    //
    //  Special case by far the most common wild card search of *
    //

    if ((Expression->Length == 1) && (Expression->Buffer[0] == '*')) {

        return TRUE;
    }

    ASSERT( FsRtlDoesDbcsContainWildCards( Expression ) );

    //
    //  Also special case expressions of the form *X.  With this and the prior
    //  case we have covered virtually all normal queries.
    //

    if (Expression->Buffer[0] == '*') {

        ANSI_STRING LocalExpression;

        LocalExpression = *Expression;

        LocalExpression.Buffer += 1;
        LocalExpression.Length -= 1;

        //
        //  Only special case an expression with a single *
        //

        if ( !FsRtlDoesDbcsContainWildCards( &LocalExpression ) ) {

            ULONG StartingNameOffset;

            if (Name->Length < (USHORT)(Expression->Length - 1)) {

                return FALSE;
            }

            StartingNameOffset = Name->Length - LocalExpression.Length;

            //
            //  FsRtlIsDbcsInExpression(): bug fix "expression[0] == *" case
            //
            //  StatingNameOffset must not bisect DBCS characters.
            //

            if (NlsMbOemCodePageTag) {

                ULONG i = 0;

                while ( i < StartingNameOffset ) {

                    i += FsRtlIsLeadDbcsCharacter( Name->Buffer[i] ) ? 2 : 1;
                }

                if ( i > StartingNameOffset ) {

                    return FALSE;
                }
            }

            //
            //  Do a simple memory compare if case sensitive, otherwise
            //  we have got to check this one character at a time.
            //

            return (BOOLEAN) RtlEqualMemory( LocalExpression.Buffer,
                                             Name->Buffer + StartingNameOffset,
                                             LocalExpression.Length );
        }
    }

    //
    //  Walk through the name string, picking off characters.  We go one
    //  character beyond the end because some wild cards are able to match
    //  zero characters beyond the end of the string.
    //
    //  With each new name character we determine a new set of states that
    //  match the name so far.  We use two arrays that we swap back and forth
    //  for this purpose.  One array lists the possible expression states for
    //  all name characters up to but not including the current one, and other
    //  array is used to build up the list of states considering the current
    //  name character as well.  The arrays are then switched and the process
    //  repeated.
    //
    //  There is not a one-to-one correspondence between state number and
    //  offset into the expression.  This is evident from the NFAs in the
    //  initial comment to this function.  State numbering is not continuous.
    //  This allows a simple conversion between state number and expression
    //  offset.  Each character in the expression can represent one or two
    //  states.  * and DOS_STAR generate two states: ExprOffset*2 and
    //  ExprOffset*2 + 1.  All other expreesion characters can produce only
    //  a single state.  Thus ExprOffset = State/2.
    //
    //
    //  Here is a short description of the variables involved:
    //
    //  NameOffset  - The offset of the current name char being processed.
    //
    //  ExprOffset  - The offset of the current expression char being processed.
    //
    //  SrcCount    - Prior match being investigated with current name char
    //
    //  DestCount   - Next location to put a matching assuming current name char
    //
    //  NameFinished - Allows one more itteration through the Matches array
    //                 after the name is exhusted (to come *s for example)
    //
    //  PreviousDestCount - This is used to prevent entry duplication, see coment
    //
    //  PreviousMatches   - Holds the previous set of matches (the Src array)
    //
    //  CurrentMatches    - Holds the current set of matches (the Dest array)
    //
    //  AuxBuffer, LocalBuffer - the storage for the Matches arrays
    //

    //
    //  Set up the initial variables
    //

    PreviousMatches = &LocalBuffer[0];
    CurrentMatches = &LocalBuffer[MATCHES_ARRAY_SIZE];

    PreviousMatches[0] = 0;
    MatchesCount = 1;

    NameOffset = 0;

    MaxState = (USHORT)(Expression->Length * 2);

    while ( !NameFinished ) {

        if ( NameOffset < Name->Length ) {

            GetDbcs( Name->Buffer, NameOffset, &NameChar, &Length );
            NameOffset += Length;

        } else {

            NameFinished = TRUE;

            //
            //  if we have already exhasted the expression, cool.  Don't
            //  continue.
            //

            if ( PreviousMatches[MatchesCount-1] == MaxState ) {

                break;
            }
        }


        //
        //  Now, for each of the previous stored expression matches, see what
        //  we can do with this name character.
        //

        SrcCount = 0;
        DestCount = 0;
        PreviousDestCount = 0;

        while ( SrcCount < MatchesCount ) {

            //
            //  We have to carry on our expression analysis as far as possible
            //  for each character of name, so we loop here until the
            //  expression stops matching.  A clue here is that expression
            //  cases that can match zero or more characters end with a
            //  continue, while those that can accept only a single character
            //  end with a break.
            //

            ExprOffset = (USHORT)((PreviousMatches[SrcCount++] + 1) / 2);

            Length = 0;


            while ( TRUE ) {

                if ( ExprOffset == Expression->Length ) {

                    break;
                }

                //
                //  The first time through the loop we don't want
                //  to increment ExprOffset.
                //

                ExprOffset += Length;

                CurrentState = (USHORT)(ExprOffset * 2);

                if ( ExprOffset == Expression->Length ) {

                    CurrentMatches[DestCount++] = MaxState;
                    break;
                }

                GetDbcs(Expression->Buffer, ExprOffset, &ExprChar, &Length);

                ASSERT( !((ExprChar >= 'a') && (ExprChar <= 'z')) );

                //
                //  Before we get started, we have to check for something
                //  really gross.  We may be about to exhaust the local
                //  space for ExpressionMatches[][], so we have to allocate
                //  some pool if this is the case.  Yuk!
                //

                if ( (DestCount >= MATCHES_ARRAY_SIZE - 2) &&
                     (AuxBuffer == NULL) ) {

                    AuxBuffer = FsRtlpAllocatePool( PagedPool,
                                                    (Expression->Length+1) *
                                                    sizeof(USHORT)*2*2 );

                    RtlCopyMemory( AuxBuffer,
                                   CurrentMatches,
                                   MATCHES_ARRAY_SIZE * sizeof(USHORT) );

                    CurrentMatches = AuxBuffer;

                    RtlCopyMemory( AuxBuffer + (Expression->Length+1)*2,
                                   PreviousMatches,
                                   MATCHES_ARRAY_SIZE * sizeof(USHORT) );

                    PreviousMatches = AuxBuffer + (Expression->Length+1)*2;

                }

                //
                //  * matches any character zero or more times.
                //

                if (ExprChar == '*') {

                    CurrentMatches[DestCount++] = CurrentState;
                    CurrentMatches[DestCount++] = CurrentState + 1;
                    continue;
                }

                //
                //  DOS_STAR matches any character except . zero or more times.
                //

                if (ExprChar == ANSI_DOS_STAR) {

                    BOOLEAN ICanEatADot = FALSE;

                    //
                    //  If we are at a period, determine if we are allowed to
                    //  consume it, ie. make sure it is not the last one.
                    //

                    if ( !NameFinished && (NameChar == '.') ) {

                        WCHAR NameChar;
                        USHORT Offset;
                        USHORT Length;

                        for ( Offset = NameOffset;
                              Offset < Name->Length;
                              Offset += Length ) {

                            GetDbcs( Name->Buffer, Offset, &NameChar, &Length );

                            if (NameChar == '.') {

                                ICanEatADot = TRUE;
                                break;
                            }
                        }
                    }

                    if (NameFinished || (NameChar != '.') || ICanEatADot) {

                        CurrentMatches[DestCount++] = CurrentState;
                        CurrentMatches[DestCount++] = CurrentState + 1;
                        continue;

                    } else {

                        //
                        //  We are at a period.  We can only match zero
                        //  characters (ie. the epsilon transition).
                        //

                        CurrentMatches[DestCount++] = CurrentState + 1;
                        continue;
                    }
                }

                //
                //  The following expreesion characters all match by consuming
                //  a character, thus force the expression, and thus state
                //  forward.
                //

                CurrentState += (USHORT)(Length * 2);

                //
                //  DOS_QM is the most complicated.  If the name is finished,
                //  we can match zero characters.  If this name is a '.', we
                //  don't match, but look at the next expression.  Otherwise
                //  we match a single character.
                //

                if ( ExprChar == ANSI_DOS_QM ) {

                    if ( NameFinished || (NameChar == '.') ) {

                        continue;
                    }

                    CurrentMatches[DestCount++] = CurrentState;
                    break;
                }

                //
                //  A DOS_DOT can match either a period, or zero characters
                //  beyond the end of name.
                //

                if (ExprChar == DOS_DOT) {

                    if ( NameFinished ) {

                        continue;
                    }

                    if (NameChar == '.') {

                        CurrentMatches[DestCount++] = CurrentState;
                        break;
                    }
                }

                //
                //  From this point on a name character is required to even
                //  continue, let alone make a match.
                //

                if ( NameFinished ) {

                    break;
                }

                //
                //  If this expression was a '?' we can match it once.
                //

                if (ExprChar == '?') {

                    CurrentMatches[DestCount++] = CurrentState;
                    break;
                }

                //
                //  Finally, check if the expression char matches the name char
                //

                if (ExprChar == NameChar) {

                    CurrentMatches[DestCount++] = CurrentState;
                    break;
                }

                //
                //  The expression didn't match so go look at the next
                //  previous match.
                //

                break;
            }


            //
            //  Prevent duplication in the destination array.
            //
            //  Each of the arrays is montonically increasing and non-
            //  duplicating, thus we skip over any source element in the src
            //  array if we just added the same element to the destination
            //  array.  This guarentees non-duplication in the dest. array.
            //

            if ((SrcCount < MatchesCount) &&
                (PreviousDestCount < DestCount) ) {

                while (PreviousDestCount < DestCount) {

                    while ( PreviousMatches[SrcCount] <
                         CurrentMatches[PreviousDestCount] ) {


                        SrcCount += 1;
                    }

                    PreviousDestCount += 1;
                }
            }
        }

        //
        //  If we found no matches in the just finished itteration, it's time
        //  to bail.
        //

        if ( DestCount == 0 ) {


            if (AuxBuffer != NULL) { ExFreePool( AuxBuffer ); }

            return FALSE;
        }

        //
        //  Swap the meaning the two arrays
        //

        {
            USHORT *Tmp;

            Tmp = PreviousMatches;

            PreviousMatches = CurrentMatches;

            CurrentMatches = Tmp;
        }

        MatchesCount = DestCount;
    }


    CurrentState = PreviousMatches[MatchesCount-1];

    if (AuxBuffer != NULL) { ExFreePool( AuxBuffer ); }


    return (BOOLEAN)(CurrentState == MaxState);
}

