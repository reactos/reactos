/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    name.cxx

Abstract:

    Support for pattern-matching file names versus file specs, used by
    FtpFindFirstFile() to form the list of found files.  Lifted from
    ntos\fsrtl\name.c, and trimmed to fit.

    This module was included in the ftphelp project because of the need
    for binary portability to Chicago.

    ---

    The unicode name support package is for manipulating unicode strings
    The routines allow the caller to dissect and compare strings.

    This package uses the same FSRTL_COMPARISON_RESULT typedef used by name.c

    The following routines are provided by this package:

      o  FsRtlDissectName - removed

      o  FsRtlColateNames - removed

      o  FsRtlDoesNameContainsWildCards - This routine tells the caller if
         a string contains any wildcard characters.

      o  FsRtlIsNameInExpression - This routine is used to compare a string
         against a template (possibly containing wildcards) to sees if the
         string is in the language denoted by the template.

Author:

    Gary Kimura     [GaryKi]    5-Feb-1990

Revision History:

        Heath Hunnicutt [t-heathh] 13-Jul-1994

--*/

#include <wininetp.h>
#include "ftpapih.h"
#include "namep.h"

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
//  Local support routine prototypes
//

BOOLEAN
FsRtlIsNameInExpressionPrivate
(
    IN ANSI_STRING *Expression,
    IN ANSI_STRING *Name
);

BOOLEAN
FsRtlDoesNameContainWildCards
(
    IN LPCSTR pszName
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
    ULONG i;
    USHORT Length;

//    PAGED_CODE();

    //
    //  Check each character in the name to see if it's a wildcard
    //  character.
    //

    Length =(unsigned short) lstrlenA( pszName );
    for (i = 0; i < Length; i += 1) {

        //
        //  check for a wild card character
        //

        if (FsRtlIsAnsiCharacterWild( pszName[i] )) {

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


//
//  The following routine is just a wrapper around
//  FsRtlIsNameInExpressionPrivate to make a last minute fix a bit safer.
//

BOOLEAN
FsRtlIsNameInExpression
(
    IN LPCSTR pszExpression,
    IN LPCSTR pszName,
    IN BOOLEAN IgnoreCase
)
{
    BOOLEAN Result=FALSE;

        ANSI_STRING Expression;
        ANSI_STRING Name;

        Name.Buffer = NewString( pszName );

        if ( Name.Buffer==NULL )
            {
                return( FALSE );
            }

        Expression.Buffer = NewString( pszExpression );

        if ( Expression.Buffer==NULL )
            {
                FREE_MEMORY( Name.Buffer );
                return( FALSE );
            }

        if ( IgnoreCase )
            {
                strupr( Name.Buffer );
                strupr( Expression.Buffer );
            }

        Name.Length = (unsigned short) lstrlenA( Name.Buffer );
        Name.MaximumLength = Name.Length;

        Expression.Length = (unsigned short) lstrlenA( Expression.Buffer );
        Expression.MaximumLength = Expression.Length;

    //
    //  Now call the main routine, remembering to free the upcased string
    //  if we allocated one.
    //

    __try {

        Result = FsRtlIsNameInExpressionPrivate( &Expression,
                                                 &Name );

    } __finally {
        FREE_MEMORY(Name.Buffer);
        FREE_MEMORY(Expression.Buffer);
    }
    ENDFINALLY
    return Result;
}


#define MATCHES_ARRAY_SIZE 16

//
//  Local support routine prototypes
//

BOOLEAN
FsRtlIsNameInExpressionPrivate
(
    IN ANSI_STRING *Expression,
    IN ANSI_STRING *Name
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

               S-. is any single character except .

               e is a null character transition

               EOF is the end of the name string


    The last construction, ~? (the DOS question mark), can either match any
    single character, or upon encountering a period or end of input string,
    advances the expression to the end of the set of contiguous ~?s.  This may
    seem somewhat convoluted, but is what DOS needs.

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

    ULONG StartingNameOffset;

    CHAR NameChar, ExprChar;

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

//    PAGED_CODE();

    INET_ASSERT(Name->Length != 0);
    INET_ASSERT(Expression->Length != 0);

    //
    //  If one string is empty return FALSE.  If both are empty return TRUE.
    //

    if ( (Name->Length == 0) || (Expression->Length == 0) ) {

        return (BOOLEAN)(!(Name->Length + Expression->Length));
    }

    //
    //  Special case by far the most common wild card search of *, or *.*
    //

    if ((Expression->Length == 1 && Expression->Buffer[0] == '*')
    || (Expression->Length == 3 && memcmp(Expression->Buffer,"*.*",3)==0)) {

        return TRUE;
    }

    INET_ASSERT(FsRtlDoesNameContainWildCards(Expression->Buffer));

    //
    // Before special casing *X, we must special case *., as people tend
    // to use that expression incorrectly to mean "all files without
    // extensions."  However, if this case, fails, it falls through to the
    // next special case, wherein files ending in dots match *.
    //

    if ( Expression->Length == 2 && memcmp(Expression->Buffer,"*.",2)==0 ) {

        PVOID pvDot;

        //
        // Attempt to find a dot in the name buffer.  A dot would indicate
        // the presence of an extension.
        //

        pvDot = memchr( Name->Buffer, '.', Name->Length );

        //
        // If there is no dot, return that this name matches the expression "*."
        //

        if ( pvDot==NULL ) {
            return( TRUE );
        }
    }

    //
    //  Also special case expressions of the form *X.  With this and the prior
    //  case we have covered virtually all normal queries.
    //

    if (Expression->Buffer[0] == '*') {

        //
        //  Only special case an expression with a single *, recognized
        //  by the fact that the tail contains no wildcards.
        //

        if ( !FsRtlDoesNameContainWildCards( Expression->Buffer + 1 ) ) {
            if (Name->Length < (USHORT)(Expression->Length - sizeof(CHAR))) {
                return FALSE;
            }

                        //
                        // Calculate the offset to the Name's tail.
                        //

            StartingNameOffset = ( Name->Length - ( Expression->Length - 1 ) );

            //
            //  Compare the tail of the expression with the name.
            //

            return( (BOOLEAN)
                            memcmp( Expression->Buffer + 1,
                                            Name->Buffer + StartingNameOffset,
                                            Name->Length - StartingNameOffset ) == 0 );
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

            NameChar = Name->Buffer[ NameOffset ];

            NameOffset ++;

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

        while ( SrcCount < MatchesCount )
            {

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

                INET_ASSERT(ExprOffset >= 0);
                INET_ASSERT(ExprOffset <= Expression->Length);

                if ( ExprOffset == Expression->Length ) {
                    break;
                }

                //
                //  The first time through the loop we don't want
                //  to increment ExprOffset.
                //

                ExprOffset += Length;
                Length = sizeof(CHAR);

                CurrentState = (USHORT)(ExprOffset*2);

                if ( ExprOffset == Expression->Length ) {
                    CurrentMatches[DestCount++] = MaxState;
                    break;
                }

                ExprChar = Expression->Buffer[ExprOffset];

                //
                //  Before we get started, we have to check for something
                //  really gross.  We may be about to exhaust the local
                //  space for ExpressionMatches[][], so we have to allocate
                //  some pool if this is the case.  Yuk!
                //

                if ( (DestCount >= MATCHES_ARRAY_SIZE - 2) && (AuxBuffer == NULL) ) {

                    ULONG ExpressionChars;

                    ExpressionChars = Expression->Length / sizeof(CHAR);

                    AuxBuffer = (USHORT *)
                                ALLOCATE_MEMORY(LMEM_FIXED,
                                                (ExpressionChars + 1)
                                                    * sizeof(USHORT) * 2 * 2
                                                );

                    CopyMemory( AuxBuffer,
                                CurrentMatches,
                                MATCHES_ARRAY_SIZE * sizeof(USHORT) );

                    CurrentMatches = AuxBuffer;

                    CopyMemory( AuxBuffer + (ExpressionChars+1)*2,
                                PreviousMatches,
                                MATCHES_ARRAY_SIZE * sizeof(USHORT) );

                    PreviousMatches = AuxBuffer + (ExpressionChars+1)*2;
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
                //  The following expreesion characters all match by consuming
                //  a character, thus force the expression, and thus state
                //  forward.
                //

                CurrentState += (USHORT)(sizeof(CHAR) * 2);

                //
                //  A DOS_DOT can match either a period, or zero characters
                //  beyond the end of name.
                //

                if (ExprChar == DOS_DOT) {

                    INET_ASSERT(FALSE);

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
                //  Check if the expression char matches the name char
                //

                if ( !NameFinished && ExprChar == NameChar ) {
                    CurrentMatches[DestCount++] = CurrentState;
                    break;
                }

                //
                //  The expression didn't match so go look at the next
                //  previous match.
                //

                break;
            } // while() that tries to use up expression chars


            //
            //  Prevent duplication in the destination array.
            //
            //  Each of the arrays is montonically increasing and non-
            //  duplicating, thus we skip over any source element in the src
            //  array if we just added the same element to the destination
            //  array.  This guarentees non-duplication in the dest. array.
            //

            if ((SrcCount < MatchesCount) && (PreviousDestCount < DestCount) ) {
                while (PreviousDestCount < DestCount) {
                    if ( PreviousMatches[SrcCount] < CurrentMatches[PreviousDestCount] ) {
                        SrcCount += 1;
                    }

                    PreviousDestCount += 1;
                }
            }
        } // while() that uses up name chars

        //
        //  If we found no matches in the just finished itteration, it's time
        //  to bail.
        //

        if ( DestCount == 0 ) {
            if (AuxBuffer != NULL) {
                FREE_MEMORY(AuxBuffer);
            }

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

    if (AuxBuffer != NULL) {
        FREE_MEMORY(AuxBuffer);
    }


    return (BOOLEAN)(CurrentState == MaxState);
}
