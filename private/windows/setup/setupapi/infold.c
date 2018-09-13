/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    infold.c

Abstract:

    Routines to load an old-style inf file.
    Based on prsinf\spinf.c

Author:

    Ted Miller (tedm) 19-Jan-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Internal temporary representation of the inf file.
// The win95 representation is built from these structures
// which are then throw away.
//
typedef struct _X_VALUE {
    struct _X_VALUE *Next;
    PTCHAR Name;
} X_VALUE, *PX_VALUE;

typedef struct _X_LINE {
    struct _X_LINE *Next;
    PTCHAR Name;
    PX_VALUE Value;
    UINT ValueCount;
} X_LINE, *PX_LINE;

typedef struct _X_SECTION {
    struct _X_SECTION *Next;
    PTCHAR Name;
    PX_LINE Line;
    UINT LineCount;
} X_SECTION, *PX_SECTION;

typedef struct _X_INF {
    PX_SECTION Section;
    UINT SectionCount;
    UINT TotalLineCount;
    UINT TotalValueCount;
} X_INF, *PX_INF;


//
// Global parse context.
//
typedef struct _PARSE_CONTEXT {
    PX_INF Inf;
    PX_SECTION Section;
    PX_LINE Line;
    PX_VALUE Value;
} PARSE_CONTEXT, *PPARSE_CONTEXT;

//
// Token parser values.
//
typedef enum _X_TOKENTYPE {
    TOK_EOF,
    TOK_EOL,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_STRING,
    TOK_EQUAL,
    TOK_COMMA,
    TOK_ERRPARSE,
    TOK_ERRNOMEM
} X_TOKENTYPE, *PX_TOKENTTYPE;

//
// Token parser data type
//
typedef struct _X_TOKEN {
    X_TOKENTYPE Type;
    PTCHAR pValue;
} X_TOKEN, *PX_TOKEN;


//
// string terminators are the whitespace characters (isspace: space, tab,
// linefeed, formfeed, vertical tab, carriage return) or the chars given below
//
// quoted string terminators allow some of the regular terminators to
// appear as characters
//
PTCHAR szStrTerms    = TEXT("[]=,\" \t\n\f\v\r");
PTCHAR szBrcStrTerms = TEXT("[]=,\"\t\n\f\v\r");
PTCHAR szQStrTerms   = TEXT("\"\n\f\v\r");
PTCHAR szCBrStrTerms = TEXT("}\n\f\v\r");

#define IsStringTerminator(terminators,ch)   (_tcschr((terminators),(ch)) != NULL)


VOID
SpFreeTemporaryParseStructures(
   IN PX_INF Inf
   )

/*++

Routine Description:

    Free the structures built by the old-style-inf parser.

Arguments:

    Inf - supplies pointer to inf descriptor structure.

Return Value:

    None.

--*/

{
    PX_SECTION Section,NextSection;
    PX_LINE Line,NextLine;
    PX_VALUE Value,NextValue;

    for(Section=Inf->Section; Section; Section=NextSection) {

        for(Line=Section->Line; Line; Line=NextLine) {

            for(Value=Line->Value; Value; Value=NextValue) {

                NextValue = Value->Next;
                if(Value->Name) {
                    MyFree(Value->Name);
                }
                MyFree(Value);
            }

            NextLine = Line->Next;
            if(Line->Name) {
                MyFree(Line->Name);
            }
            MyFree(Line);
        }

        NextSection = Section->Next;
        MyFree(Section->Name);
        MyFree(Section);
    }

    MyFree(Inf);
}


BOOL
SpAppendSection(
    IN PPARSE_CONTEXT Context,
    IN PTCHAR         SectionName
    )

/*++

Routine Description:

    This appends a new section to the section list in the current INF.
    All further lines and values pertain to this new section, so it resets
    the line list and value lists too.

Arguments:

    Context - supplies the parse context

    SectionName - Name of the new section. ( [SectionName] )

Return Value:

    BOOL - FALSE if failure (out of memory)

--*/

{
    PX_SECTION NewSection;

    MYASSERT(Context->Inf);

    //
    // Allocate memory for the new section
    //
    if((NewSection = MyMalloc(sizeof(X_SECTION))) == NULL) {
        return(FALSE);
    }

    //
    // initialize the new section
    //
    ZeroMemory(NewSection,sizeof(X_SECTION));
    NewSection->Name = SectionName;

    //
    // Link it in
    //
    if(Context->Section) {
        Context->Section->Next = NewSection;
    } else {
        Context->Inf->Section = NewSection;
    }

    Context->Section = NewSection;

    //
    // reset the current line record and current value record field
    //
    Context->Line = NULL;
    Context->Value = NULL;

    Context->Inf->SectionCount++;

    return(TRUE);
}


BOOL
SpAppendLine(
    IN PPARSE_CONTEXT Context,
    IN PTCHAR         LineKey
    )

/*++

Routine Description:

    This appends a new line to the line list in the current section.
    All further values pertain to this new line, so it resets
    the value list too.

Arguments:

    Context - supplies the parse context.

    LineKey - Key to be used for the current line, this could be NULL.

Return Value:

    BOOL - FALSE if failure (out of memory)

--*/


{
    PX_LINE NewLine;

    MYASSERT(Context->Section);

    //
    // Allocate memory for the new Line
    //
    if((NewLine = MyMalloc(sizeof(X_LINE))) == NULL) {
        return(FALSE);
    }

    ZeroMemory(NewLine,sizeof(X_LINE));

    NewLine->Name = LineKey;

    //
    // Link it in
    //
    if(Context->Line) {
        Context->Line->Next = NewLine;
    } else {
        Context->Section->Line = NewLine;
    }

    Context->Line = NewLine;

    //
    // Reset the current value record
    //
    Context->Value = NULL;

    //
    // Adjust counts.
    //
    Context->Inf->TotalLineCount++;
    Context->Section->LineCount++;
    if(LineKey) {
        Context->Inf->TotalValueCount++;
        NewLine->ValueCount = 1;
    }

    return(TRUE);
}


BOOL
SpAppendValue(
    IN PPARSE_CONTEXT Context,
    IN PTCHAR         ValueString
    )

/*++

Routine Description:

    This appends a new value to the value list in the current line.

Arguments:

    Context - supplies the parse context.

    ValueString - The value string to be added.

Return Value:

    BOOL - FALSE if failure (out of memory)

--*/

{
    PX_VALUE NewValue;

    MYASSERT(Context->Line);

    //
    // Allocate memory for the new value record
    //
    if((NewValue = MyMalloc(sizeof(X_VALUE))) == NULL) {
        return(FALSE);
    }

    ZeroMemory(NewValue,sizeof(X_VALUE));

    NewValue->Name = ValueString;

    //
    // Link it in.
    //
    if(Context->Value) {
        Context->Value->Next = NewValue;
    } else {
        Context->Line->Value = NewValue;
    }

    //
    // Adjust counts
    //
    Context->Value = NewValue;
    Context->Inf->TotalValueCount++;
    Context->Line->ValueCount++;

    return(TRUE);
}



X_TOKEN
SpGetToken(
    IN OUT PCTSTR *Stream,
    IN     PCTSTR  StreamEnd,
    IN     PTCHAR  pszStrTerms,
    IN     PTCHAR  pszQStrTerms,
    IN     PTCHAR  pszCBrStrTerms
    )

/*++

Routine Description:

    This function returns the Next token from the configuration stream.

Arguments:

    Stream - Supplies the address of the configuration stream.  Returns
        the address of where to start looking for tokens within the
        stream.

    StreamEnd - Supplies the memory address immediately following the
        character stream.

Return Value:

    The next token

--*/

{

    PCTSTR pch, pchStart;
    PTCHAR pchNew;
    DWORD Length, i;
    X_TOKEN Token;

    //
    // Skip whitespace (except for eol)
    //
    pch = *Stream;

    while(pch < StreamEnd) {

        SkipWhitespace(&pch, StreamEnd);

        if((pch < StreamEnd) && !(*pch)) {
            //
            // We hit a NULL char--skip it
            // and keep looking for a token.
            //
            pch++;

        } else {
            break;
        }
    }

    //
    // Check for comments and remove them
    //
    if((pch < StreamEnd) &&
       ((*pch == TEXT(';')) || (*pch == TEXT('#')) ||
        ((*pch == TEXT('/')) && (*(pch+1) == TEXT('/')))))
    {
        do {
            pch++;
        } while((pch < StreamEnd) && (*pch != TEXT('\n')));
    }

    if(pch == StreamEnd) {
        *Stream = pch;
        Token.Type = TOK_EOF;
        Token.pValue = NULL;
        return(Token);
    }

    switch (*pch) {

    case TEXT('['):
        pch++;
        Token.Type = TOK_LBRACE;
        Token.pValue = NULL;
        break;

    case TEXT(']'):
        pch++;
        Token.Type = TOK_RBRACE;
        Token.pValue = NULL;
        break;

    case TEXT('='):
        pch++;
        Token.Type = TOK_EQUAL;
        Token.pValue = NULL;
        break;

    case TEXT(','):
        pch++;
        Token.Type = TOK_COMMA;
        Token.pValue = NULL;
        break;

    case TEXT('\n'):
        pch++;
        Token.Type = TOK_EOL;
        Token.pValue = NULL;
        break;

    case TEXT('\"'):
        pch++;
        //
        // determine quoted string
        //
        pchStart = pch;
        while((pch < StreamEnd) && !IsStringTerminator(pszQStrTerms,*pch)) {
            pch++;
        }

        //
        //
        // Only valid terminator is double quote
        //
        if((pch == StreamEnd) || (*pch != TEXT('\"'))) {
            Token.Type = TOK_ERRPARSE;
            Token.pValue = NULL;
        } else {

            //
            // Got a valid string. Allocate space for it and save.
            //
            Length = (DWORD)(pch - pchStart);
            if((pchNew = MyMalloc((Length+1)*sizeof(TCHAR))) == NULL) {
                Token.Type = TOK_ERRNOMEM;
                Token.pValue = NULL;
            } else {
                //
                // We can't use string copy here, since there may be
                // NULL chars in the string (which we convert to
                // spaces during the copy).
                //
                // lstrcpyn(pchNew,pchStart,Length+1);
                //
                for(i = 0; i < Length; i++) {
                    if(!(pchNew[i] = pchStart[i])) {
                        pchNew[i] = TEXT(' ');
                    }
                }
                pchNew[Length] = 0;
                Token.Type = TOK_STRING;
                Token.pValue = pchNew;
            }
            pch++;   // advance past the quote
        }
        break;

    case TEXT('{'):
        //
        // determine quoted string
        //
        pchStart = pch;
        while((pch < StreamEnd) && !IsStringTerminator(pszCBrStrTerms,*pch)) {
            pch++;
        }

        //
        // Only valid terminator is curly brace
        if((pch == StreamEnd) || (*pch != TEXT('}'))) {
            Token.Type = TOK_ERRPARSE;
            Token.pValue = NULL;
        } else {

            //
            // Got a valid string. Allocate space for it and save.
            //
            Length = (DWORD)(pch - pchStart) + 1;
            if((pchNew = MyMalloc((Length+1)*sizeof(TCHAR))) == NULL) {
                Token.Type = TOK_ERRNOMEM;
                Token.pValue = NULL;
            } else {
                //
                // We can't use string copy here, since there may be
                // NULL chars in the string (which we convert to
                // spaces during the copy).
                //
                // lstrcpyn(pchNew,pchStart,Length+1);
                //
                for(i = 0; i < Length; i++) {
                    if(!(pchNew[i] = pchStart[i])) {
                        pchNew[i] = TEXT(' ');
                    }
                }
                pchNew[Length] = TEXT('\0');
                Token.Type = TOK_STRING;
                Token.pValue = pchNew;
            }
            pch++;   // advance past the brace
        }
        break;

    default:
        //
        // determine regular string
        //
        pchStart = pch;
        while((pch < StreamEnd) && !IsStringTerminator(pszStrTerms,*pch)) {
            pch++;
        }

        //
        // Disallow empty strings here
        //
        if(pch == pchStart) {
            pch++;
            Token.Type = TOK_ERRPARSE;
            Token.pValue = NULL;
        } else {

            Length = (DWORD)(pch - pchStart);
            if((pchNew = MyMalloc((Length+1)*sizeof(TCHAR))) == NULL) {
                Token.Type = TOK_ERRNOMEM;
                Token.pValue = NULL;
            } else {
                //
                // We can't use string copy here, since there may be
                // NULL chars in the string (which we convert to
                // spaces during the copy).
                //
                // lstrcpyn(pchNew,pchStart,Length+1);
                //
                for(i = 0; i < Length; i++) {
                    if(!(pchNew[i] = pchStart[i])) {
                        pchNew[i] = TEXT(' ');
                    }
                }
                pchNew[Length] = 0;
                Token.Type = TOK_STRING;
                Token.pValue = pchNew;
            }
        }
        break;
    }

    *Stream = pch;
    return(Token);
}


DWORD
ParseInfBuffer(
    IN  PCTSTR  Buffer,
    IN  DWORD   BufferSize,
    OUT PX_INF *Inf,
    OUT UINT   *ErrorLineNumber
    )

/*++

Routine Description:

    Given a character buffer containing the INF file, this routine parses
    the INF into an internal form with Section records, Line records and
    Value records.

Arguments:

    Buffer - contains to ptr to a buffer containing the INF file

    BufferSize - contains the size of Buffer, in characters.

    Inf - if the return value is NO_ERROR, receives a pointer to the
        inf descriptor for the parsed inf.

    ErrorLineNumber - receives the line number where a syntax/oom error
        was encountered, if the return value is not NO_ERROR.

Return Value:

    Win32 error code (with inf extensions) indicating outcome.

    If NO_ERROR, Inf is filled in.
    If not NO_ERROR, ErrorLineNumber is filled in.

--*/

{
    PCTSTR Stream, StreamEnd;
    PTCHAR pchSectionName, pchValue, pchEmptyString;
    DWORD State, InfLine;
    DWORD LastState;
    X_TOKEN Token;
    BOOL Done;
    PTCHAR pszStrTermsCur    = szStrTerms;
    PTCHAR pszQStrTermsCur   = szQStrTerms;
    PTCHAR pszCBrStrTermsCur = szCBrStrTerms;
    DWORD ErrorCode;
    PARSE_CONTEXT Context;

    //
    // Initialize the globals and create an inf record structure.
    //
    ZeroMemory(&Context,sizeof(PARSE_CONTEXT));
    if((Context.Inf = MyMalloc(sizeof(X_INF))) == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    ZeroMemory(Context.Inf,sizeof(X_INF));

    //
    // Set initial state
    //
    State     = 1;
    LastState = State;
    InfLine   = 1;
    StreamEnd = (Stream = Buffer) + BufferSize;
    Done      = FALSE;
    ErrorCode = NO_ERROR;

    //
    // Initialize the token type, so we'll know not to free any
    // memory for it if we hit an exception right off the bat.
    //
    Token.Type = TOK_ERRPARSE;

    pchSectionName = NULL;
    pchValue       = NULL;
    pchEmptyString = NULL;

    //
    // Guard token processing loop with try/except in case we
    // get an inpage error.
    //
    try {

        while(!Done) {

            Token = SpGetToken(&Stream,
                               StreamEnd,
                               pszStrTermsCur,
                               pszQStrTermsCur,
                               pszCBrStrTermsCur
                              );

            //
            // If you need to debug the parser, uncomment the following:
#if 0
             DebugPrint(TEXT("STATE: %u TOKEN: %u (%s) LAST: %u\r\n"),
                        State, Token.Type,
                        Token.pValue ? Token.pValue : TEXT("NULL"),
                        LastState);
#endif            

            if(Token.Type == TOK_ERRNOMEM) {
                 Done = TRUE;
                 ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            } else {

                switch (State) {
                //
                // STATE1: Start of file, this state remains till first
                //         section is found
                // Valid Tokens: TOK_EOL, TOK_EOF, TOK_LBRACE
                //
                case 1:
                    switch (Token.Type) {

                    case TOK_EOL:
                        break;

                    case TOK_EOF:
                        Done = TRUE;
                        break;

                    case TOK_LBRACE:
                        pszStrTermsCur = szBrcStrTerms;
                        State = 2;
                        break;

                    default:
                        Done = TRUE;
                        ErrorCode = ERROR_EXPECTED_SECTION_NAME;
                        break;
                    }
                    break;

                //
                // STATE 2: Section LBRACE has been received, expecting STRING
                //
                // Valid Tokens: TOK_STRING
                //
                case 2:
                    //
                    // allow spaces in section names
                    //
                    switch (Token.Type) {

                    case TOK_STRING:
                        State = 3;
                        //
                        // restore term. string with space
                        //
                        pszStrTermsCur = szStrTerms;
                        pchSectionName = Token.pValue;
                        break;

                    default:
                        Done = TRUE;
                        ErrorCode = ERROR_BAD_SECTION_NAME_LINE;
                        break;
                    }
                    break;

                //
                // STATE 3: Section Name received, expecting RBRACE
                //
                // Valid Tokens: TOK_RBRACE
                //
                case 3:
                    switch (Token.Type) {

                    case TOK_RBRACE:
                        State = 4;
                        break;

                    default:
                        Done = TRUE;
                        ErrorCode = ERROR_BAD_SECTION_NAME_LINE;
                        break;
                    }
                    break;

                //
                // STATE 4: Section Definition Complete, expecting EOL
                //
                // Valid Tokens: TOK_EOL, TOK_EOF
                //
                case 4:
                    switch (Token.Type) {

                    case TOK_EOL:
                        if(SpAppendSection(&Context,pchSectionName)) {
                            pchSectionName = NULL;
                            State = 5;
                        } else {
                            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                            Done = TRUE;
                        }
                        break;

                    case TOK_EOF:
                        if(SpAppendSection(&Context,pchSectionName)) {
                            pchSectionName = NULL;
                        } else {
                            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                        }
                        Done = TRUE;
                        break;

                    default:
                        ErrorCode = ERROR_BAD_SECTION_NAME_LINE;
                        Done = TRUE;
                        break;
                    }
                    break;

                //
                // STATE 5: Expecting Section Lines
                //
                // Valid Tokens: TOK_EOL, TOK_EOF, TOK_STRING, TOK_LBRACE
                //
                case 5:
                    switch (Token.Type) {

                    case TOK_EOL:
                        break;

                    case TOK_EOF:
                        Done = TRUE;
                        break;

                    case TOK_STRING:
                        pchValue = Token.pValue;
                        //
                        // Set token's pValue pointer to NULL, so we won't
                        // try to free the same memory twice if we hit an
                        // exception
                        //
                        Token.pValue = NULL;
                        State = 6;
                        break;

                    case TOK_LBRACE:
                        pszStrTermsCur = szBrcStrTerms;
                        State = 2;
                        break;

                    default:
                        // Done = TRUE;
                        // ErrorCode = ERROR_GENERAL_SYNTAX;
                        State = 20;
                        LastState = 5;
                        break;
                    }
                    break;

                //
                // STATE 6: String returned, not sure whether it is key or value
                //
                // Valid Tokens: TOK_EOL, TOK_EOF, TOK_COMMA, TOK_EQUAL
                //
                case 6:
                    switch (Token.Type) {

                    case TOK_EOL:
                        if(SpAppendLine(&Context,NULL) && SpAppendValue(&Context,pchValue)) {
                            pchValue = NULL;
                            State = 5;
                        } else {
                            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                            Done = TRUE;
                        }
                        break;

                    case TOK_EOF:
                        if(SpAppendLine(&Context,NULL) && SpAppendValue(&Context,pchValue)) {
                            pchValue = NULL;
                        } else {
                            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                        }
                        Done = TRUE;
                        break;

                    case TOK_COMMA:
                        if(SpAppendLine(&Context,NULL) && SpAppendValue(&Context,pchValue)) {
                            pchValue = NULL;
                            State = 7;
                        } else {
                            Done = TRUE;
                            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                        }
                        break;

                    case TOK_EQUAL:
                        if(SpAppendLine(&Context,pchValue)) {
                            pchValue = NULL;
                            State = 8;
                        } else {
                            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                            Done = TRUE;
                        }
                        break;

                    case TOK_STRING:
                        MyFree(Token.pValue);
                        Token.pValue = NULL;
                        // fall through

                    default:
                        // Done = TRUE;
                        // ErrorCode = ERROR_GENERAL_SYNTAX;
                        //
                        if(pchValue) {
                            MyFree(pchValue);
                            pchValue = NULL;
                        }
                        State = 20;
                        LastState = 5;
                        break;
                    }
                    break;

                //
                // STATE 7: Comma received, Expecting another string
                //
                // Valid Tokens: TOK_STRING, TOK_EOL, TOK_EOF, TOK_COMMA
                //
                case 7:
                    switch (Token.Type) {

                    case TOK_STRING:
                        if(SpAppendValue(&Context,Token.pValue)) {
                            State = 9;
                        } else {
                            Done = TRUE;
                            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                        }
                        break;

                    case TOK_COMMA:
                    case TOK_EOL:
                    case TOK_EOF:
                        //
                        // If we hit end-of-line or end-of-file, then add an
                        // empty-string value.
                        //
                        if(pchEmptyString = MyMalloc(sizeof(TCHAR))) {
                            *pchEmptyString = TEXT('\0');
                            if(SpAppendValue(&Context, pchEmptyString)) {
                                if(Token.Type == TOK_EOL) {
                                    State = 5;
                                } else if (Token.Type == TOK_COMMA) {
                                    State = 7;
                                } else {
                                    Done = TRUE;
                                }
                            } else {
                                MyFree(pchEmptyString);
                                pchEmptyString = NULL;
                                Done = TRUE;
                                ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        } else {
                            Done = TRUE;
                            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                        }
                        break;

                    default:
                        // Done = TRUE;
                        // ErrorCode = ERROR_GENERAL_SYNTAX;
                        State = 20;
                        LastState = 7;
                        break;
                    }
                    break;

                //
                // STATE 8: Equal received, Expecting another string
                //
                // Valid Tokens: TOK_STRING, TOK_EOL, TOK_EOF, TOK_COMMA
                //
                case 8:
                    switch (Token.Type) {

                    case TOK_STRING:
                        if(SpAppendValue(&Context,Token.pValue)) {
                            State = 9;
                        } else {
                            Done = TRUE;
                            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                        }
                        break;

                    case TOK_COMMA:
                    case TOK_EOL:
                    case TOK_EOF:
                        //
                        // If we hit end-of-line or end-of-file, then add an
                        // empty-string value.
                        //
                        if(pchEmptyString = MyMalloc(sizeof(TCHAR))) {
                            *pchEmptyString = TEXT('\0');
                            if(SpAppendValue(&Context, pchEmptyString)) {
                                if(Token.Type == TOK_EOL) {
                                    State = 5;
                                } else if (Token.Type == TOK_COMMA) {
                                    State = 7;
                                } else {
                                    Done = TRUE;
                                }
                            } else {
                                MyFree(pchEmptyString);
                                pchEmptyString = NULL;
                                Done = TRUE;
                                ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        } else {
                            Done = TRUE;
                            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                        }
                        break;

                    default:
                        // Done = TRUE;
                        // ErrorCode = ERROR_GENERAL_SYNTAX;
                        State = 20;
                        LastState = 8;
                        break;
                    }
                    break;

                //
                // STATE 9: String received after equal, value string
                //
                // Valid Tokens: TOK_EOL, TOK_EOF, TOK_COMMA
                //
                case 9:
                    switch (Token.Type) {

                    case TOK_EOL:
                        State = 5;
                        break;

                    case TOK_EOF:
                        Done = TRUE;
                        break;

                    case TOK_COMMA:
                        State = 7;
                        break;

                    case TOK_STRING:
                        MyFree(Token.pValue);
                        Token.pValue = NULL;
                        // fall through

                    default:
                        // Done = TRUE;
                        // ErrorCode = ERROR_GENERAL_SYNTAX;
                        State = 20;
                        LastState = 5;
                        break;
                    }
                    break;

                //
                // STATE 10: Value string definitely received
                //
                // Valid Tokens: TOK_EOL, TOK_EOF, TOK_COMMA
                //
                case 10:
                    switch (Token.Type) {

                    case TOK_EOL:
                      State =5;
                      break;

                    case TOK_EOF:
                        Done = TRUE;
                        break;

                    case TOK_COMMA:
                        State = 7;
                        break;

                    case TOK_STRING:
                        MyFree(Token.pValue);
                        Token.pValue = NULL;
                        // fall through

                    default:
                        // Done = TRUE;
                        // ErrorCode = ERROR_GENERAL_SYNTAX;
                        State = 20;
                        LastState = 10;
                        break;
                    }
                    break;

                //
                // STATE 20: Eat a line of INF
                //
                // Valid Tokens: TOK_EOL, TOK_EOF
                //
                case 20:
                    switch (Token.Type) {

                    case TOK_EOL:
                        State = LastState;
                        break;

                    case TOK_EOF:
                        Done = TRUE;
                        break;

                    case TOK_STRING:
                        MyFree(Token.pValue);
                        Token.pValue = NULL;
                        // fall through

                    default:
                        break;
                    }
                    break;

                default:

                    Done = TRUE;
                    ErrorCode = ERROR_GENERAL_SYNTAX;
                    break;

                } // end switch(State)

            } // end else

            if(ErrorCode == NO_ERROR) {

                //
                // Keep track of line numbers
                //
                if(Token.Type == TOK_EOL) {
                    InfLine++;
                }

            }

        } // End while

    } except(EXCEPTION_EXECUTE_HANDLER) {

        ErrorCode = ERROR_READ_FAULT;

        //
        // Reference the following string pointers here in the except clause so that
        // the compiler won't re-order the code in such a way that we don't know whether
        // or not to free the corresponding buffers.
        //
        Token.pValue   = Token.pValue;
        pchEmptyString = pchEmptyString;
        pchSectionName = pchSectionName;
        pchValue       = pchValue;
    }

    if(ErrorCode != NO_ERROR) {

        if((Token.Type == TOK_STRING) && Token.pValue) {
             MyFree(Token.pValue);
        }

        if(pchEmptyString) {
            MyFree(pchEmptyString);
        }

        if(pchSectionName) {
            MyFree(pchSectionName);
        }

        if(pchValue) {
            MyFree(pchValue);
        }

        SpFreeTemporaryParseStructures(Context.Inf);
        Context.Inf = NULL;

        *ErrorLineNumber = InfLine;
    }

    *Inf = Context.Inf;
    return(ErrorCode);
}


DWORD
ParseOldInf(
    IN  PCTSTR       FileImage,
    IN  DWORD        FileImageSize,
    IN  PSETUP_LOG_CONTEXT LogContext, OPTIONAL
    OUT PLOADED_INF *Inf,
    OUT UINT        *ErrorLineNumber
    )

/*++

Routine Description:

    Top-level routine to parse an old-style inf file.

    The file is first parsed using the old parser, into data structures
    understood by that parser. Following that those structures are converted
    into the universal internal inf format.

Arguments:

    FileImage - supplies a pointer to the in-memory image of the file.
        The image is assumed to be terminated by a nul character.

    FileImageSize - supplies the number of wide chars in the FileImage.
    
    LogContext - supplies optional logging context

    Inf - receives a pointer to the inf descriptor for the file.

    ErrorLineNumber - receives the line number of a syntx error if one is
        detected in the inf file.

Return Value:

    Win32 error code (with inf extensions) indicating outcome.

    If NO_ERROR, Inf is filled in.
    If not NO_ERROR, ErrorLineNumber is filled in.

--*/

{
    PLOADED_INF inf;
    PX_INF X_Inf;
    DWORD rc;
    PX_SECTION X_Section;
    PX_LINE X_Line;
    PX_VALUE X_Value;
    LONG StringId, StringId2;
    BOOL b;
    UINT LineNumber;
    UINT ValueNumber;
    PLONG TempValueBlock;
    PTSTR SearchString;

    //
    // First go off and parse the file into the temporary (old-style)
    // inf structures.
    //
    rc = ParseInfBuffer(FileImage,FileImageSize,&X_Inf,ErrorLineNumber);
    if(rc != NO_ERROR) {
        return(rc);
    }

    //
    // Allocate a new-style inf descriptor.  (Note that we allocate an additional
    // <TotalLineCount> number of values, since each line may have a key, which
    // requires two values each.  We'll trim this down later on.)
    //
    inf = AllocateLoadedInfDescriptor(X_Inf->SectionCount,
                                      X_Inf->TotalLineCount,
                                      X_Inf->TotalValueCount + X_Inf->TotalLineCount,
                                      LogContext
                                     );

    if(!inf) {
        SpFreeTemporaryParseStructures(X_Inf);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    inf->Style = INF_STYLE_OLDNT;

    //
    // Now, parse the old-style inf structures into new-style inf structures.
    //
    b = TRUE;
    LineNumber = 0;
    ValueNumber = 0;
    for(X_Section=X_Inf->Section; b && X_Section; X_Section=X_Section->Next) {

        //
        // Add the section to the section block.
        //
        StringId = pStringTableAddString(inf->StringTable,
                                         X_Section->Name,
                                         STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                         NULL,0
                                        );
        if(StringId == -1) {
            b = FALSE;
        } else {
            inf->SectionBlock[inf->SectionCount].SectionName = StringId;
            inf->SectionBlock[inf->SectionCount].LineCount = X_Section->LineCount;
            inf->SectionBlock[inf->SectionCount].Lines = LineNumber;

            inf->SectionCount++;
        }

        for(X_Line=X_Section->Line; b && X_Line; X_Line=X_Line->Next) {

            //
            // Add the line to the line block.
            //
            inf->LineBlock[LineNumber].ValueCount = (WORD)X_Line->ValueCount;

            if(X_Line->Name) {
                inf->LineBlock[LineNumber].Flags = INF_LINE_HASKEY | INF_LINE_SEARCHABLE;
                inf->LineBlock[LineNumber].ValueCount++;
            } else if(X_Line->ValueCount == 1) {
                //
                // If the line only has a single value, then it's searchable, even if it
                // doesn't have a key.
                //
                inf->LineBlock[LineNumber].Flags = INF_LINE_SEARCHABLE;
                inf->LineBlock[LineNumber].ValueCount++;
            } else {
                inf->LineBlock[LineNumber].Flags = 0;
            }

            if(b) {

                inf->LineBlock[LineNumber].Values = ValueNumber;
                X_Value = X_Line->Value;

                //
                // If the line is searchable (i.e., has a key xor a single value), then add the
                // search value twice--once case insensitively and once case-sensitively.
                //
                if(ISSEARCHABLE(&(inf->LineBlock[LineNumber]))) {

                    if(X_Line->Name) {
                        SearchString = X_Line->Name;
                    } else {
                        SearchString = X_Value->Name;
                        X_Value = X_Value->Next;
                    }

                    //
                    // First get the case-sensitive string id...
                    //
                    StringId = pStringTableAddString(
                                    inf->StringTable,
                                    SearchString,
                                    STRTAB_CASE_SENSITIVE,
                                    NULL,0
                                    );
                    //
                    // And now get the case-insensitive string id...
                    //
                    StringId2 = pStringTableAddString(inf->StringTable,
                                                      SearchString,
                                                      STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                                      NULL,0
                                                     );

                    if((StringId == -1) || (StringId2 == -1)) {
                        b = FALSE;
                    } else {
                        inf->ValueBlock[ValueNumber++] = StringId2;  // Add the searchable string...
                        inf->ValueBlock[ValueNumber++] = StringId;   // and then the displayable one.
                    }
                }

                for( ; b && X_Value; X_Value=X_Value->Next) {

                    //
                    // Add the value to the value block.
                    //
                    StringId = pStringTableAddString(inf->StringTable,
                                                     X_Value->Name,
                                                     STRTAB_CASE_SENSITIVE,
                                                     NULL,0
                                                    );
                    if(StringId == -1) {
                        b = FALSE;
                    } else {
                        inf->ValueBlock[ValueNumber++] = StringId;
                    }
                }

                LineNumber++;
            }
        }
    }

    //
    // Record the sizes of the INF data blocks.
    //
    inf->SectionBlockSizeBytes = X_Inf->SectionCount * sizeof(INF_SECTION);
    inf->LineBlockSizeBytes    = X_Inf->TotalLineCount * sizeof(INF_LINE);

    //
    // We don't need the temporary inf descriptors any more.
    //
    SpFreeTemporaryParseStructures(X_Inf);

    //
    // Attempt to trim the value block down to exact size necessary.  Since this buffer is
    // either shrinking or staying the same, the realloc shouldn't fail, but if it does, we'll
    // just continue to use the original block.
    //
    inf->ValueBlockSizeBytes = ValueNumber * sizeof(LONG);
    if(TempValueBlock = MyRealloc(inf->ValueBlock, ValueNumber * sizeof(LONG))) {
        inf->ValueBlock = TempValueBlock;
    }

    //
    // If an error has occured, free the inf descriptor we've
    // been building. Otherwise we want to pass that descriptor
    // back to the caller.
    //
    if(b) {
        *Inf = inf;
    } else {
        *ErrorLineNumber = 0;
        FreeLoadedInfDescriptor(inf);
    }

    return(b ? NO_ERROR : ERROR_NOT_ENOUGH_MEMORY);
}


DWORD
ProcessOldInfVersionBlock(
    IN PLOADED_INF Inf
    )

/*++

Routine Description:

    Set up a version node for an old-style inf file. The version node is
    simulated in that there is no [Version] section; we look for other stuff
    in the file to simulate version information.

    Class is determined from [Identification].OptionType.
    Signature is determined from [Signature].FileType.
    If the signature is MICROSOFT_FILE then we set the Provider to the localized
    version of "Microsoft."

Arguments:

    Inf - supplies a pointer to the inf descriptor for the file.

Return Value:

    Win32 error code (with inf extensions) indicating outcome.

--*/

{
    TCHAR StrBuf[128];
    PTSTR String;

    //
    // Class
    //
    if(String = InfGetKeyOrValue(Inf, TEXT("Identification"), TEXT("OptionType"), 0, 1, NULL)) {
        if(!AddDatumToVersionBlock(&(Inf->VersionBlock), pszClass, String)) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    // Signature
    //
    if(String = InfGetKeyOrValue(Inf, pszSignature, TEXT("FileType"), 0, 1, NULL)) {
        if(!AddDatumToVersionBlock(&(Inf->VersionBlock), pszSignature, String)) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    // Provider
    //
    if(String && !lstrcmpi(String, TEXT("MICROSOFT_FILE"))) {

        LoadString(MyDllModuleHandle, IDS_MICROSOFT, StrBuf, sizeof(StrBuf)/sizeof(TCHAR));

        if(!AddDatumToVersionBlock(&(Inf->VersionBlock), pszProvider, StrBuf)) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return NO_ERROR;
}
