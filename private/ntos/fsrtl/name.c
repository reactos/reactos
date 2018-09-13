/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Name.c

Abstract:

    The unicode name support package is for manipulating unicode strings
    The routines allow the caller to dissect and compare strings.

    This package uses the same FSRTL_COMPARISON_RESULT typedef used by name.c

    The following routines are provided by this package:

      o  FsRtlDissectName - This routine takes a path name string and breaks
         into two parts.  The first name in the string and the remainder.
         It also checks that the first name is valid for an NT file.

      o  FsRtlColateNames - This routine is used to colate directories
         according to lexical ordering.  Lexical ordering is strict unicode
         numerical oerdering.

      o  FsRtlDoesNameContainsWildCards - This routine tells the caller if
         a string contains any wildcard characters.

      o  FsRtlIsNameInExpression - This routine is used to compare a string
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
//  Some special debugging stuff
//

#if DBG

extern ULONG DaveDebug;
#define DavePrint if (DaveDebug) DbgPrint

#else

#define DavePrint NOTHING

#endif

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('nrSF')

//
//  Local support routine prototypes
//

BOOLEAN
FsRtlIsNameInExpressionPrivate (
    IN PUNICODE_STRING Expression,
    IN PUNICODE_STRING Name,
    IN BOOLEAN IgnoreCase,
    IN PWCH UpcaseTable
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FsRtlAreNamesEqual)
#pragma alloc_text(PAGE, FsRtlDissectName)
#pragma alloc_text(PAGE, FsRtlDoesNameContainWildCards)
#pragma alloc_text(PAGE, FsRtlIsNameInExpression)
#pragma alloc_text(PAGE, FsRtlIsNameInExpressionPrivate)
#endif


VOID
FsRtlDissectName (
    IN UNICODE_STRING Path,
    OUT PUNICODE_STRING FirstName,
    OUT PUNICODE_STRING RemainingName
    )

/*++

Routine Description:

    This routine cracks a path.  It picks off the first element in the
    given path name and provides both it and the remaining part.  A path
    is a set of file names separated by backslashes.  If a name begins
    with a backslash, the FirstName is the string immediately following
    the backslash.  Here are some examples:

        Path           FirstName    RemainingName
        ----           ---------    -------------
        empty          empty        empty

        \              empty        empty

        A              A            empty

        \A             A            empty

        A\B\C\D\E      A            B\C\D\E

        *A?            *A?          empty


    Note that both output strings use the same string buffer memory of the
    input string, and are not necessarily null terminated.

    Also, this routine makes no judgement as to the legality of each
    file name componant.  This must be done separatly when each file name
    is extracted.

Arguments:

    Path - The full path name to crack.

    FirstName - The first name in the path.  Don't allocate a buffer for
        this string.

    RemainingName - The rest of the path.  Don't allocate a buffer for this
        string.

Return Value:

    None.

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

    PathLength = Path.Length / sizeof(WCHAR);

    //
    //  Check for an empty input string
    //

    if (PathLength == 0) {

        return;
    }

    //
    //  Skip over a starting backslash, and make sure there is more.
    //

    if ( Path.Buffer[0] == L'\\' ) {

        i = 1;
    }

    //
    //  Now run down the input string until we hit a backslash or the end
    //  of the string, remembering where we started;
    //

    for ( FirstNameStart = i;
          (i < PathLength) && (Path.Buffer[i] != L'\\');
          i += 1 ) {

        NOTHING;
    }

    //
    //  At this point all characters up to (but not including) i are
    //  in the first part.   So setup the first name
    //

    FirstName->Length = (USHORT)((i - FirstNameStart) * sizeof(WCHAR));
    FirstName->MaximumLength = FirstName->Length;
    FirstName->Buffer = &Path.Buffer[FirstNameStart];

    //
    //  Now the remaining part needs a string only if the first part didn't
    //  exhaust the entire input string.  We know that if anything is left
    //  that is must start with a backslash.  Note that if there is only
    //  a trailing backslash, the length will get correctly set to zero.
    //

    if (i < PathLength) {

        RemainingName->Length = (USHORT)((PathLength - (i + 1)) * sizeof(WCHAR));
        RemainingName->MaximumLength = RemainingName->Length;
        RemainingName->Buffer = &Path.Buffer[i + 1];
    }

    //
    //  And return to our caller
    //

    return;
}

BOOLEAN
FsRtlDoesNameContainWildCards (
    IN PUNICODE_STRING Name
    )

/*++

Routine Description:

    This routine simply scans the input Name string looking for any Nt
    wild card characters.

Arguments:

    Name - The string to check.

Return Value:

    BOOLEAN - TRUE if one or more wild card characters was found.

--*/
{
    PUSHORT p;

    PAGED_CODE();

    //
    //  Check each character in the name to see if it's a wildcard
    //  character.
    //

    if( Name->Length ) {
        for( p = Name->Buffer + (Name->Length / sizeof(WCHAR)) - 1;
             p >= Name->Buffer && *p != L'\\' ;
             p-- ) {

            //
            //  check for a wild card character
            //

            if (FsRtlIsUnicodeCharacterWild( *p )) {

                //
                //  Tell caller that this name contains wild cards
                //

                return TRUE;
            }
        }
    }

    //
    //  No wildcard characters were found, so return to our caller
    //

    return FALSE;
}


BOOLEAN
FsRtlAreNamesEqual (
    PCUNICODE_STRING ConstantNameA,
    PCUNICODE_STRING ConstantNameB,
    IN BOOLEAN IgnoreCase,
    IN PCWCH UpcaseTable OPTIONAL
    )

/*++

Routine Description:

    This routine simple returns whether the two names are exactly equal.
    If the two names are known to be constant, this routine is much
    faster than FsRtlIsNameInExpression.

Arguments:

    ConstantNameA - Constant name.

    ConstantNameB - Constant name.

    IgnoreCase - TRUE if the Names should be Upcased before comparing.

    UpcaseTable - If supplied, use this table for case insensitive compares,
        otherwise, use the default system upcase table.

Return Value:

    BOOLEAN - TRUE if the two names are lexically equal.

--*/

{
    ULONG Index;
    ULONG NameLength;
    BOOLEAN FreeStrings = FALSE;

    UNICODE_STRING LocalNameA;
    UNICODE_STRING LocalNameB;

    PAGED_CODE();

    //
    // If the names aren't even the same size, then return FALSE right away.
    //

    if ( ConstantNameA->Length != ConstantNameB->Length ) {

        return FALSE;
    }

    NameLength = ConstantNameA->Length / sizeof(WCHAR);

    //
    //  If we weren't given an upcase table, we have to upcase the names
    //  ourselves.
    //

    if ( IgnoreCase && !ARGUMENT_PRESENT(UpcaseTable) ) {

        NTSTATUS Status;

        Status = RtlUpcaseUnicodeString( &LocalNameA, ConstantNameA, TRUE );

        if ( !NT_SUCCESS(Status) ) {

            ExRaiseStatus( Status );
        }

        Status = RtlUpcaseUnicodeString( &LocalNameB, ConstantNameB, TRUE );

        if ( !NT_SUCCESS(Status) ) {

            RtlFreeUnicodeString( &LocalNameA );

            ExRaiseStatus( Status );
        }

        ConstantNameA = &LocalNameA;
        ConstantNameB = &LocalNameB;

        IgnoreCase = FALSE;
        FreeStrings = TRUE;
    }

    //
    //  Do either case sensitive or insensitive compare.
    //

    if ( !IgnoreCase ) {

        BOOLEAN BytesEqual;

        BytesEqual = (BOOLEAN) RtlEqualMemory( ConstantNameA->Buffer,
                                               ConstantNameB->Buffer,
                                               ConstantNameA->Length );

        if ( FreeStrings ) {

            RtlFreeUnicodeString( &LocalNameA );
            RtlFreeUnicodeString( &LocalNameB );
        }

        return BytesEqual;

    } else {

        for (Index = 0; Index < NameLength; Index += 1) {

            if ( UpcaseTable[ConstantNameA->Buffer[Index]] !=
                 UpcaseTable[ConstantNameB->Buffer[Index]] ) {

                return FALSE;
            }
        }

        return TRUE;
    }
}


//
//  The following routine is just a wrapper around
//  FsRtlIsNameInExpressionPrivate to make a last minute fix a bit safer.
//

BOOLEAN
FsRtlIsNameInExpression (
    IN PUNICODE_STRING Expression,
    IN PUNICODE_STRING Name,
    IN BOOLEAN IgnoreCase,
    IN PWCH UpcaseTable OPTIONAL
    )

{
    BOOLEAN Result;
    UNICODE_STRING LocalName;

    //
    //  If we weren't given an upcase table, we have to upcase the names
    //  ourselves.
    //

    if ( IgnoreCase && !ARGUMENT_PRESENT(UpcaseTable) ) {

        NTSTATUS Status;

        Status = RtlUpcaseUnicodeString( &LocalName, Name, TRUE );

        if ( !NT_SUCCESS(Status) ) {

            ExRaiseStatus( Status );
        }

        Name = &LocalName;

        IgnoreCase = FALSE;

    } else {

        LocalName.Buffer = NULL;
    }

    //
    //  Now call the main routine, remembering to free the upcased string
    //  if we allocated one.
    //

    try {

        Result = FsRtlIsNameInExpressionPrivate( Expression,
                                                 Name,
                                                 IgnoreCase,
                                                 UpcaseTable );

    } finally {

        if (LocalName.Buffer != NULL) {

            RtlFreeUnicodeString( &LocalName );
        }
    }

    return Result;
}


#define MATCHES_ARRAY_SIZE 16

//
//  Local support routine prototypes
//

BOOLEAN
FsRtlIsNameInExpressionPrivate (
    IN PUNICODE_STRING Expression,
    IN PUNICODE_STRING Name,
    IN BOOLEAN IgnoreCase,
    IN PWCH UpcaseTable
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
        (Caller must already upcase if passing CaseInsensitive TRUE.)

    Name - Supplies the input name to check for.

    CaseInsensitive - TRUE if Name should be Upcased before comparing.

Return Value:

    BOOLEAN - TRUE if Name is an element in the set of strings denoted
        by the input Expression and FALSE otherwise.

--*/

{
    USHORT NameOffset;
    USHORT ExprOffset;

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

    DebugTrace(+1, Dbg, "FsRtlIsNameInExpression\n", 0);
    DebugTrace( 0, Dbg, " Expression      = %Z\n", Expression );
    DebugTrace( 0, Dbg, " Name            = %Z\n", Name );
    DebugTrace( 0, Dbg, " CaseInsensitive = %08lx\n", CaseInsensitive );

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

    if ((Expression->Length == 2) && (Expression->Buffer[0] == L'*')) {

        return TRUE;
    }

    ASSERT( FsRtlDoesNameContainWildCards( Expression ) );

    ASSERT( !IgnoreCase || ARGUMENT_PRESENT(UpcaseTable) );

    //
    //  Also special case expressions of the form *X.  With this and the prior
    //  case we have covered virtually all normal queries.
    //

    if (Expression->Buffer[0] == L'*') {

        UNICODE_STRING LocalExpression;

        LocalExpression = *Expression;

        LocalExpression.Buffer += 1;
        LocalExpression.Length -= 2;

        //
        //  Only special case an expression with a single *
        //

        if ( !FsRtlDoesNameContainWildCards( &LocalExpression ) ) {

            ULONG StartingNameOffset;

            if (Name->Length < (USHORT)(Expression->Length - sizeof(WCHAR))) {

                return FALSE;
            }

            StartingNameOffset = ( Name->Length -
                                   LocalExpression.Length ) / sizeof(WCHAR);

            //
            //  Do a simple memory compare if case sensitive, otherwise
            //  we have got to check this one character at a time.
            //

            if ( !IgnoreCase ) {

                return (BOOLEAN) RtlEqualMemory( LocalExpression.Buffer,
                                                 Name->Buffer + StartingNameOffset,
                                                 LocalExpression.Length );

            } else {

                for ( ExprOffset = 0;
                      ExprOffset < (USHORT)(LocalExpression.Length / sizeof(WCHAR));
                      ExprOffset += 1 ) {

                    NameChar = Name->Buffer[StartingNameOffset + ExprOffset];
                    NameChar = UpcaseTable[NameChar];

                    ExprChar = LocalExpression.Buffer[ExprOffset];

                    ASSERT( ExprChar == UpcaseTable[ExprChar] );

                    if ( NameChar != ExprChar ) {

                        return FALSE;
                    }
                }

                return TRUE;
            }
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

            NameChar = Name->Buffer[NameOffset / sizeof(WCHAR)];

            NameOffset += sizeof(WCHAR);;

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

            USHORT Length;

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
                Length = sizeof(WCHAR);

                CurrentState = (USHORT)(ExprOffset * 2);

                if ( ExprOffset == Expression->Length ) {

                    CurrentMatches[DestCount++] = MaxState;
                    break;
                }

                ExprChar = Expression->Buffer[ExprOffset / sizeof(WCHAR)];

                ASSERT( !IgnoreCase || !((ExprChar >= L'a') && (ExprChar <= L'z')) );

                //
                //  Before we get started, we have to check for something
                //  really gross.  We may be about to exhaust the local
                //  space for ExpressionMatches[][], so we have to allocate
                //  some pool if this is the case.  Yuk!
                //

                if ( (DestCount >= MATCHES_ARRAY_SIZE - 2) &&
                     (AuxBuffer == NULL) ) {

                    ULONG ExpressionChars;

                    ExpressionChars = Expression->Length / sizeof(WCHAR);

                    AuxBuffer = FsRtlpAllocatePool( PagedPool,
                                                    (ExpressionChars+1) *
                                                    sizeof(USHORT)*2*2 );

                    RtlCopyMemory( AuxBuffer,
                                   CurrentMatches,
                                   MATCHES_ARRAY_SIZE * sizeof(USHORT) );

                    CurrentMatches = AuxBuffer;

                    RtlCopyMemory( AuxBuffer + (ExpressionChars+1)*2,
                                   PreviousMatches,
                                   MATCHES_ARRAY_SIZE * sizeof(USHORT) );

                    PreviousMatches = AuxBuffer + (ExpressionChars+1)*2;
                }

                //
                //  * matches any character zero or more times.
                //

                if (ExprChar == L'*') {

                    CurrentMatches[DestCount++] = CurrentState;
                    CurrentMatches[DestCount++] = CurrentState + 3;
                    continue;
                }

                //
                //  DOS_STAR matches any character except . zero or more times.
                //

                if (ExprChar == DOS_STAR) {

                    BOOLEAN ICanEatADot = FALSE;

                    //
                    //  If we are at a period, determine if we are allowed to
                    //  consume it, ie. make sure it is not the last one.
                    //

                    if ( !NameFinished && (NameChar == '.') ) {

                        USHORT Offset;

                        for ( Offset = NameOffset;
                              Offset < Name->Length;
                              Offset += Length ) {

                            if (Name->Buffer[Offset / sizeof(WCHAR)] == L'.') {

                                ICanEatADot = TRUE;
                                break;
                            }
                        }
                    }

                    if (NameFinished || (NameChar != L'.') || ICanEatADot) {

                        CurrentMatches[DestCount++] = CurrentState;
                        CurrentMatches[DestCount++] = CurrentState + 3;
                        continue;

                    } else {

                        //
                        //  We are at a period.  We can only match zero
                        //  characters (ie. the epsilon transition).
                        //

                        CurrentMatches[DestCount++] = CurrentState + 3;
                        continue;
                    }
                }

                //
                //  The following expreesion characters all match by consuming
                //  a character, thus force the expression, and thus state
                //  forward.
                //

                CurrentState += (USHORT)(sizeof(WCHAR) * 2);

                //
                //  DOS_QM is the most complicated.  If the name is finished,
                //  we can match zero characters.  If this name is a '.', we
                //  don't match, but look at the next expression.  Otherwise
                //  we match a single character.
                //

                if ( ExprChar == DOS_QM ) {

                    if ( NameFinished || (NameChar == L'.') ) {

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

                    if (NameChar == L'.') {

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

                if (ExprChar == L'?') {

                    CurrentMatches[DestCount++] = CurrentState;
                    break;
                }

                //
                //  Finally, check if the expression char matches the name char
                //

                if (ExprChar == (WCHAR)(IgnoreCase ?
                                        UpcaseTable[NameChar] : NameChar)) {

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
