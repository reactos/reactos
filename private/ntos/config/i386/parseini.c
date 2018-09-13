/*++

Copyright (c) 1998  Microsoft Corporation


Module Name:

    parseini.c

Abstract:

    This modules contains routines to parse an inf file. This is based on
    the code from the osloader. All indices are zero based.

Author:

    Santosh Jodh (santoshj) 08-Aug-1998


Environment:

    Kernel mode.

Revision History:

--*/

#include "cmp.h"
#include "string.h"
#include "ctype.h"
#include "stdlib.h"
#include "parseini.h"

typedef struct _value   VALUE,      *PVALUE;
typedef struct _line    LINE,       *PLINE;
typedef struct _section SECTION,    *PSECTION;
typedef struct _inf     INF,        *PINF;
typedef struct _token   TOKEN,      *PTOKEN;
typedef enum _tokentype TOKENTYPE,  *PTOKENTTYPE;
typedef enum _stringsSectionType    STRINGSSECTIONTYPE;;

struct _value
{
    PVALUE  pNext;
    PCHAR   pName;
    BOOLEAN Allocated;
};

struct _line
{
    PLINE   pNext;
    PCHAR   pName;
    PVALUE  pValue;
    BOOLEAN Allocated;
};

struct _section
{
    PSECTION    pNext;
    PCHAR       pName;
    PLINE       pLine;
    BOOLEAN     Allocated;
};

struct _inf
{
    PSECTION            pSection;
    PSECTION            pSectionRecord;
    PLINE               pLineRecord;
    PVALUE              pValueRecord;
    STRINGSSECTIONTYPE  StringsSectionType;
    PSECTION            StringsSection;
};

//
// [Strings] section types.
//
enum _stringsSectionType
{
    StringsSectionNone,
    StringsSectionPlain,
    StringsSectionLoosePrimaryMatch,
    StringsSectionExactPrimaryMatch,
    StringsSectionExactMatch
};

enum _tokentype
{
    TOK_EOF,
    TOK_EOL,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_STRING,
    TOK_EQUAL,
    TOK_COMMA,
    TOK_ERRPARSE,
    TOK_ERRNOMEM
};

struct _token
{
    TOKENTYPE   Type;
    PCHAR       pValue;
    BOOLEAN     Allocated;
};

VOID
CmpFreeValueList(
    IN PVALUE pValue
    );

VOID
CmpFreeLineList(
    IN PLINE pLine
    );

VOID
CmpFreeSectionList(
    IN PSECTION pSection
    );

PCHAR
CmpProcessForSimpleStringSub(
    IN PINF pInf,
    IN PCHAR String
    );

BOOLEAN
CmpAppendSection(
    IN PINF  pInf,
    IN PCHAR pSectionName,
    IN BOOLEAN Allocated
    );

BOOLEAN
CmpAppendLine(
    IN PINF pInf,
    IN PCHAR pLineKey,
    IN BOOLEAN Allocated
    );

BOOLEAN
CmpAppendValue(
    IN PINF pInf,
    IN PCHAR pValueString,
    IN BOOLEAN Allocated
    );

VOID
CmpGetToken(
    IN OUT PCHAR *Stream,
    IN PCHAR MaxStream,
    IN OUT PULONG LineNumber,
    IN OUT PTOKEN Token
    );

PINF
CmpParseInfBuffer(
    IN PCHAR Buffer,
    IN ULONG Size,
    IN OUT PULONG ErrorLine
    );

PVALUE
CmpSearchValueInLine(
    IN PLINE pLine,
    IN ULONG ValueIndex
    );

PLINE
CmpSearchLineInSectionByIndex(
    IN PSECTION pSection,
    IN ULONG    LineIndex
    );

PSECTION
CmpSearchSectionByName(
    IN PINF  pInf,
    IN PCHAR SectionName
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,CmpFreeValueList)
#pragma alloc_text(INIT,CmpFreeLineList)
#pragma alloc_text(INIT,CmpFreeSectionList)
#pragma alloc_text(INIT,CmpProcessForSimpleStringSub)
#pragma alloc_text(INIT,CmpAppendSection)
#pragma alloc_text(INIT,CmpAppendLine)
#pragma alloc_text(INIT,CmpAppendValue)
#pragma alloc_text(INIT,CmpGetToken)
#pragma alloc_text(INIT,CmpParseInfBuffer)
#pragma alloc_text(INIT,CmpSearchValueInLine)
#pragma alloc_text(INIT,CmpSearchLineInSectionByIndex)
#pragma alloc_text(INIT,CmpSearchSectionByName)
#pragma alloc_text(INIT,CmpSearchInfLine)
#pragma alloc_text(INIT,CmpOpenInfFile)
#pragma alloc_text(INIT,CmpCloseInfFile)
#pragma alloc_text(INIT,CmpGetKeyName)
#pragma alloc_text(INIT,CmpSearchInfSection)
#pragma alloc_text(INIT,CmpGetSectionLineIndex)
#pragma alloc_text(INIT,CmpGetSectionLineIndexValueCount)
#pragma alloc_text(INIT,CmpGetIntField)
#pragma alloc_text(INIT,CmpGetBinaryField)
#endif


//
// Globals used by the token parser.
// String terminators are the whitespace characters (isspace: space, tab,
// linefeed, formfeed, vertical tab, carriage return) or the chars given below.
//

CHAR  StringTerminators[] = "[]=,\t \"\n\f\v\r";
PCHAR QStringTerminators = StringTerminators + 6;
PCHAR EmptyValue;
CHAR  DblSpaceSection[] = "DBLSPACE_SECTION";

BOOLEAN
CmpAppendSection(
    IN PINF  pInf,
    IN PCHAR pSectionName,
    IN BOOLEAN Allocated
    )

/*++

    Routine Description:

        This routine creates a new section or merges with an existing section in the inf.

    Input Parameters:

        pInf - Pointer to the inf to be processed.

        pSectionName - Name of the section.

        Allocated - TRUE if memory was allocated for the section name.

    Return Value:

        TRUE iff successful.

--*/

{
    PSECTION            pNewSection;
    PLINE               pLineRecord;
    STRINGSSECTIONTYPE  type;
    USHORT              id;
    USHORT              threadLang;
    PCHAR               p;

    //
    // Check to see if INF initialised and the parameters passed in is valid
    //

    if (    pInf == (PINF)NULL ||
            pSectionName == (PCHAR)NULL)
    {
        return (FALSE);
    }

    //
    // See if we already have a section by this name. If so we want
    // to merge sections.
    //

    for(    pNewSection = pInf->pSection;
            pNewSection;
            pNewSection = pNewSection->pNext)
    {
        if(pNewSection->pName && _stricmp(pNewSection->pName,pSectionName) == 0)
        {
            break;
        }
    }

    if(pNewSection)
    {
        //
        // Set pLineRecord to point to the last line currently in the section.
        //

        for(    pLineRecord = pNewSection->pLine;
                pLineRecord && pLineRecord->pNext;
                pLineRecord = pLineRecord->pNext);

        pInf->pLineRecord = pLineRecord;
    }
    else
    {
        //
        // Allocate memory for the new section
        //

        pNewSection = (PSECTION)ExAllocatePoolWithTag(PagedPool, sizeof(SECTION), CM_PARSEINI_TAG);

        if (pNewSection == (PSECTION)NULL)
        {
            ASSERT(pNewSection);
            return (FALSE);
        }

        //
        // Initialize the new section.
        //

        pNewSection->pNext = NULL;
        pNewSection->pLine = NULL;
        pNewSection->pName = pSectionName;
        pNewSection->Allocated = Allocated;

        //
        // Link it in.
        //

        pNewSection->pNext = pInf->pSection;
        pInf->pSection = pNewSection;

        if(_strnicmp(pSectionName, "Strings", 7) == 0)
        {
            type = StringsSectionNone;

            if(pSectionName[7] == '.')
            {
                //
                // The langid part must be in the form of 4 hex digits.
                //

                id = (USHORT)strtoul(pSectionName + 8, &p, 16);
                if(p == (pSectionName + 8 + 5) && *p == '\0')
                {
                    threadLang = LANGIDFROMLCID(NtCurrentTeb()->CurrentLocale);

                    if(threadLang == id)
                    {
                        type = StringsSectionExactMatch;
                    }
                    else
                    {
                        if(id == PRIMARYLANGID(threadLang))
                        {
                            type = StringsSectionExactPrimaryMatch;
                        }
                        else
                        {
                            if(PRIMARYLANGID(id) == PRIMARYLANGID(threadLang))
                            {
                                type = StringsSectionLoosePrimaryMatch;
                            }
                        }
                    }
                }
            }
            else
            {
                if(!pSectionName[7])
                {
                    type = StringsSectionPlain;
                }
            }

            if(type > pInf->StringsSectionType)
            {
                pInf->StringsSection = pNewSection;
            }
        }

        //
        // Reset the current line record.
        //

        pInf->pLineRecord = NULL;
    }

    pInf->pSectionRecord = pNewSection;
    pInf->pValueRecord = NULL;

    return (TRUE);
}

BOOLEAN
CmpAppendLine(
    IN PINF pInf,
    IN PCHAR pLineKey,
    IN BOOLEAN Allocated
    )

/*++

    Routine Description:

        This routine creates a new line and appends it to the end of the line list.

    Input Parameters:

        pInf - Pointer to the inf to be processed.

        pLineKey - Name of the line.

        Allocated - TRUE if memory was allocated for the line name.

    Return Value:

        TRUE iff successful.

--*/

{
    PLINE pNewLine;

    //
    // Check to see if current section initialized.
    //

    if (pInf->pSectionRecord == (PSECTION)NULL)
    {
        return (FALSE);
    }

    //
    // Allocate memory for the new Line.
    //

    pNewLine = (PLINE)ExAllocatePoolWithTag(PagedPool, sizeof(LINE), CM_PARSEINI_TAG);
    if (pNewLine == (PLINE)NULL)
    {
        ASSERT(pNewLine);
        return (FALSE);
    }

    //
    // Link it in.
    //

    pNewLine->pNext  = (PLINE)NULL;
    pNewLine->pValue = (PVALUE)NULL;
    pNewLine->pName  = pLineKey;
    pNewLine->Allocated = Allocated;

    if (pInf->pLineRecord == (PLINE)NULL)
    {
        pInf->pSectionRecord->pLine = pNewLine;
    }
    else
    {
        pInf->pLineRecord->pNext = pNewLine;
    }

    pInf->pLineRecord  = pNewLine;

    //
    // Reset the current value record
    //

    pInf->pValueRecord = (PVALUE)NULL;

    return (TRUE);
}

BOOLEAN
CmpAppendValue(
    IN PINF pInf,
    IN PCHAR pValueString,
    IN BOOLEAN Allocated
    )

/*++

    Routine Description:

        This routine creates a new value and appends it to the end of the value list.

    Input Parameters:

        pInf - Pointer to the inf to be processed.

        pValueString - Name of the value.

        Allocated - TRUE if memory was allocated for the value name.

    Return Value:

        TRUE iff successful.

--*/

{
    PVALUE pNewValue;

    //
    // Check to see if current line record has been initialised and
    // the parameter passed in is valid.
    //

    if (    pInf->pLineRecord == (PLINE)NULL ||
            pValueString == (PCHAR)NULL)
    {
        return (FALSE);
    }

    //
    // Allocate memory for the new value record.
    //

    pNewValue = (PVALUE)ExAllocatePoolWithTag(PagedPool, sizeof(VALUE), CM_PARSEINI_TAG);

    if (pNewValue == (PVALUE)NULL)
    {
        ASSERT(pNewValue);
        return (FALSE);
    }

    //
    // Link it in.
    //

    pNewValue->pNext  = (PVALUE)NULL;
    pNewValue->pName  = pValueString;
    pNewValue->Allocated = Allocated;

    if (pInf->pValueRecord == (PVALUE)NULL)
    {
        pInf->pLineRecord->pValue = pNewValue;
    }
    else
    {
        pInf->pValueRecord->pNext = pNewValue;
    }

    pInf->pValueRecord = pNewValue;

    return (TRUE);
}

VOID
CmpGetToken(
    IN OUT PCHAR *Stream,
    IN PCHAR MaxStream,
    IN OUT PULONG LineNumber,
    IN OUT PTOKEN Token
    )

/*++

Routine Description:

    This function returns the Next token from the configuration stream.

Arguments:

    Stream - Supplies the address of the configuration stream.  Returns
        the address of where to start looking for tokens within the
        stream.

    MaxStream - Supplies the address of the last character in the stream.


Return Value:

    None.

--*/

{

    PCHAR   pch;
    PCHAR   pchStart;
    PCHAR   pchNew;
    ULONG   length;
    BOOLEAN done;

    Token->Allocated = FALSE;
    Token->pValue = NULL;

    do
    {
        done = TRUE;

        //
        //  Skip whitespace (except for EOL).
        //

        for (   pch = *Stream;
                pch < MaxStream && *pch != '\n' && isspace(*pch);
                pch++);

        //
        // Check for comments and remove them.
        //

        if (    pch < MaxStream &&
                (*pch == '#' || *pch == ';'))
        {
            while (pch < MaxStream && *pch != '\n')
            {
                pch++;
            }
        }

        //
        // Check to see if EOF has been reached, set the token to the right
        // value.
        //

        if (pch >= MaxStream || *pch == 26)
        {
            *Stream = pch;
            Token->Type  = TOK_EOF;
            Token->pValue = NULL;

            return;
        }

        switch (*pch)
        {
            case '[':

                pch++;
                Token->Type  = TOK_LBRACE;
                break;

            case ']':

                pch++;
                Token->Type  = TOK_RBRACE;
                break;

            case '=':

                pch++;
                Token->Type  = TOK_EQUAL;
                break;

            case ',':

                pch++;
                Token->Type  = TOK_COMMA;
                break;

            case '\n':

                pch++;
                Token->Type  = TOK_EOL;
                break;

            case '\"':

                pch++;

                //
                // Determine quoted string.
                //

                for (   pchStart = pch;
                        pch < MaxStream && (strchr(QStringTerminators, *pch) == NULL);
                        pch++);

                if (pch >= MaxStream || *pch != '\"')
                {
                    Token->Type   = TOK_ERRPARSE;
                }
                else
                {

                    //
                    // We require a quoted string to end with a double-quote.
                    // (If the string ended with anything else, the if() above
                    // would not have let us into the else clause.) The quote
                    // character is irrelevent, however, and can be overwritten.
                    // So we'll save some heap and use the string in-place.
                    // No need to make a copy.
                    //
                    // Note that this alters the image of txtsetup.sif we pass
                    // to setupdd.sys. Thus the inf parser in setupdd.sys must
                    // be able to treat a nul character as if it were a terminating
                    // double quote.
                    //

                    *pch++ = '\0';
                    Token->Type = TOK_STRING;
                    Token->pValue = pchStart;
                }
                break;

            case '\\':

                for (   pchNew = ++pch;
                        pchNew < MaxStream &&
                            *pchNew != '\n' && isspace(*pchNew);
                        pchNew++);

                if (*pchNew == '\n')
                {
                    pch = pchNew + 1;
                    done = FALSE;
                    break;
                }

            default:

                //
                // Determine regular string.
                //

                for (   pchStart = pch;
                        pch < MaxStream && (strchr(StringTerminators, *pch) == NULL);
                        pch++);

                if (pch == pchStart)
                {
                    pch++;
                    Token->Type  = TOK_ERRPARSE;
                }
                else
                {
                    length = (ULONG)(pch - pchStart);
                    pchNew = ExAllocatePoolWithTag(PagedPool, length + 1, CM_PARSEINI_TAG);
                    if (pchNew == NULL)
                    {
                        ASSERT(pchNew);
                        Token->Type = TOK_ERRNOMEM;
                    }
                    else
                    {
                        strncpy(pchNew, pchStart, length);
                        pchNew[length] = 0;
                        Token->Type = TOK_STRING;
                        Token->pValue = pchNew;
                        Token->Allocated = TRUE;
                    }
                }
                break;
        }

        *Stream = pch;
    }
    while (!done);

    return;
}

PINF
CmpParseInfBuffer(
    IN PCHAR Buffer,
    IN ULONG Size,
    IN OUT PULONG ErrorLine
    )

/*++

Routine Description:

   Given a character buffer containing the INF file, this routine parses
   the INF into an internal form with Section records, Line records and
   Value records.

Arguments:

   Buffer - contains to ptr to a buffer containing the INF file

   Size - contains the size of the buffer.

   ErrorLine - if a parse error occurs, this variable receives the line
        number of the line containing the error.


Return Value:

   PVOID - INF handle ptr to be used in subsequent INF calls.

--*/

{
    PINF        pInf;
    ULONG       state;
    PCHAR       stream;
    PCHAR       maxStream;
    PCHAR       pchSectionName;
    PCHAR       pchValue;
    TOKEN       token;
    BOOLEAN     done;
    BOOLEAN     error;
    ULONG       infLine;
    BOOLEAN     allocated;

    //
    // Need EmptyValue to point at a NULL character.
    //

    EmptyValue = StringTerminators + strlen(StringTerminators);

    //
    // Allocate memory for the INF record.
    //

    pInf = (PINF)ExAllocatePoolWithTag(PagedPool, sizeof(INF), CM_PARSEINI_TAG);

    if (pInf == NULL)
    {
        ASSERT(pInf);
        return (pInf);
    }

    pInf->pSection = NULL;
    pInf->pSectionRecord = NULL;
    pInf->pLineRecord = NULL;
    pInf->pValueRecord = NULL;
    pInf->StringsSectionType = StringsSectionNone;
    pInf->StringsSection = NULL;

    //
    // Set initial state.
    //

    state     = 1;
    stream    = Buffer;
    maxStream = Buffer + Size;
    pchSectionName = NULL;
    pchValue = NULL;
    done      = FALSE;
    error     = FALSE;
    infLine = 1;

    //
    // Enter token processing loop.
    //

    while (!done)
    {

       CmpGetToken(&stream, maxStream, &infLine, &token);

        switch (state)
        {
            //
            // STATE1: Start of file, this state remains till first
            //         section is found
            // Valid Tokens: TOK_EOL, TOK_EOF, TOK_LBRACE
            //               TOK_STRING when reading Dblspace.inf
            //

            case 1:

                switch (token.Type)
                {
                    case TOK_EOL:

                        break;

                    case TOK_EOF:

                        done = TRUE;

                        break;

                    case TOK_LBRACE:

                        state = 2;

                        break;

                    case TOK_STRING:

                        pchSectionName = ExAllocatePoolWithTag(PagedPool, sizeof(DblSpaceSection), CM_PARSEINI_TAG);
                        if (pchSectionName)
                        {
                            strcpy(pchSectionName, DblSpaceSection);
                            pchValue = token.pValue;
                            allocated = TRUE;
                            token.Allocated = TRUE;
                            if (CmpAppendSection(pInf, pchSectionName, TRUE))
                            {
                                pchSectionName = NULL;
                                state = 6;
                            }
                            else
                            {
                                error = done = TRUE;
                            }
                        }
                        else
                        {
                            ASSERT(pchSectionName);
                            error = done = TRUE;
                        }

                        break;

                    default:

                        error = done = TRUE;

                        break;
                }

                break;

            //
            // STATE 2: Section LBRACE has been received, expecting STRING
            //
            // Valid Tokens: TOK_STRING, TOK_RBRACE
            //

            case 2:

                switch (token.Type)
                {
                    case TOK_STRING:

                        state = 3;
                        pchSectionName = token.pValue;
                        allocated = token.Allocated;

                        break;

                    case TOK_RBRACE:

                        token.pValue = EmptyValue;
                        token.Allocated = FALSE;
                        allocated = FALSE;
                        state = 4;

                        break;

                    default:

                        error = done = TRUE;

                        break;

                }

                break;

            //
            // STATE 3: Section Name received, expecting RBRACE
            //
            // Valid Tokens: TOK_RBRACE
            //

            case 3:

                switch (token.Type)
                {
                    case TOK_RBRACE:

                        state = 4;

                        break;

                    default:

                        error = done = TRUE;

                        break;
                }

                break;

            //
            // STATE 4: Section Definition Complete, expecting EOL
            //
            // Valid Tokens: TOK_EOL, TOK_EOF
            //

            case 4:

                switch (token.Type)
                {

                    case TOK_EOL:

                        if (!CmpAppendSection(pInf, pchSectionName, allocated))
                        {

                            error = done = TRUE;
                        }
                        else
                        {
                            pchSectionName = NULL;
                            state = 5;
                        }

                        break;

                    case TOK_EOF:

                        if (!CmpAppendSection(pInf, pchSectionName, allocated))
                        {
                            error = done = TRUE;
                        }
                        else
                        {
                            pchSectionName = NULL;
                            done = TRUE;
                        }

                        break;

                    default:

                        error = done = TRUE;

                        break;
                }

                break;

            //
            // STATE 5: Expecting Section Lines
            //
            // Valid Tokens: TOK_EOL, TOK_EOF, TOK_STRING, TOK_LBRACE
            //

            case 5:

                switch (token.Type)
                {
                    case TOK_EOL:

                        break;

                    case TOK_EOF:

                        done = TRUE;

                        break;

                    case TOK_STRING:

                        pchValue = token.pValue;
                        allocated = token.Allocated;
                        state = 6;

                        break;

                    case TOK_LBRACE:

                        state = 2;

                        break;

                    default:

                        error = done = TRUE;

                        break;
                }

                break;

            //
            // STATE 6: String returned, not sure whether it is key or value
            //
            // Valid Tokens: TOK_EOL, TOK_EOF, TOK_COMMA, TOK_EQUAL
            //

            case 6:

                switch (token.Type)
                {

                    case TOK_EOL:

                        if (    !CmpAppendLine(pInf, NULL, FALSE) ||
                                !CmpAppendValue(pInf, pchValue, allocated))
                        {
                            error = done = TRUE;
                        }
                        else
                        {
                            pchValue = NULL;
                            state = 5;
                        }

                        break;

                    case TOK_EOF:

                        if (    !CmpAppendLine(pInf, NULL, FALSE) ||
                                !CmpAppendValue(pInf, pchValue, allocated))
                        {
                            error = done = TRUE;
                        }
                        else
                        {
                            pchValue = NULL;
                            done = TRUE;
                        }

                        break;

                    case TOK_COMMA:

                        if (    !CmpAppendLine(pInf, NULL, FALSE) ||
                                !CmpAppendValue(pInf, pchValue, allocated))
                        {
                            error = done = TRUE;
                        }
                        else
                        {
                            pchValue = NULL;
                            state = 7;
                        }

                        break;

                    case TOK_EQUAL:

                        if (!CmpAppendLine(pInf, pchValue, allocated))
                        {
                            error = done = TRUE;
                        }
                        else
                        {
                            pchValue = NULL;
                            state = 8;
                        }

                        break;

                    default:

                        error = done = TRUE;

                        break;
                }

                break;

            //
            // STATE 7: Comma received, Expecting another string
            //
            // Valid Tokens: TOK_STRING TOK_COMMA
            //   A comma means we have an empty value.
            //

            case 7:

                switch (token.Type)
                {

                    case TOK_COMMA:

                        token.pValue = EmptyValue;
                        token.Allocated = FALSE;
                        allocated = FALSE;
                        if (!CmpAppendValue(pInf, token.pValue, FALSE))
                        {
                            error = done = TRUE;
                        }

                        //
                        // State stays at 7 because we are expecting a string
                        //

                        break;

                    case TOK_STRING:

                        if (!CmpAppendValue(pInf, token.pValue, token.Allocated))
                        {
                            error = done = TRUE;
                        }
                        else
                        {
                            state = 9;
                        }

                        break;

                    default:

                        error = done = TRUE;

                        break;
                }

                break;

            //
            // STATE 8: Equal received, Expecting another string
            //          If none, assume there is a single empty string on the RHS
            //
            // Valid Tokens: TOK_STRING, TOK_EOL, TOK_EOF
            //

            case 8:

                switch (token.Type)
                {
                    case TOK_EOF:

                        token.pValue = EmptyValue;
                        token.Allocated = FALSE;
                        allocated = FALSE;
                        if(!CmpAppendValue(pInf, token.pValue, FALSE))
                        {
                            error = TRUE;
                        }

                        done = TRUE;

                        break;

                    case TOK_EOL:

                        token.pValue = EmptyValue;
                        token.Allocated = FALSE;
                        allocated = FALSE;
                        if(!CmpAppendValue(pInf, token.pValue, FALSE))
                        {
                            error = TRUE;
                            done = TRUE;
                        }
                        else
                        {
                            state = 5;
                        }

                        break;

                    case TOK_STRING:

                        if (!CmpAppendValue(pInf, token.pValue, FALSE))
                        {
                            error = done = TRUE;
                        }
                        else
                        {
                            state = 9;
                        }

                        break;

                    default:

                        error = done = TRUE;

                        break;
                }

                break;

            //
            // STATE 9: String received after equal, value string
            //
            // Valid Tokens: TOK_EOL, TOK_EOF, TOK_COMMA
            //

            case 9:

                switch (token.Type)
                {
                    case TOK_EOL:

                        state = 5;

                        break;

                    case TOK_EOF:

                        done = TRUE;

                        break;

                    case TOK_COMMA:

                        state = 7;

                        break;

                    default:

                        error = done = TRUE;

                        break;
                }

                break;

            //
            // STATE 10: Value string definitely received
            //
            // Valid Tokens: TOK_EOL, TOK_EOF, TOK_COMMA
            //

            case 10:

                switch (token.Type)
                {
                    case TOK_EOL:

                        state =5;

                        break;

                    case TOK_EOF:

                        done = TRUE;

                        break;

                    case TOK_COMMA:

                        state = 7;

                        break;

                    default:

                        error = done = TRUE;

                        break;
                }

                break;

            default:

                error = done = TRUE;

                break;

        } // END switch(state)


        if (error)
        {
            *ErrorLine = infLine;
            if (pchSectionName != (PCHAR)NULL && allocated)
            {
                ExFreePool(pchSectionName);
            }

            if (pchValue != (PCHAR)NULL && allocated)
            {
                ExFreePool(pchValue);
            }

            ExFreePool(pInf);

            pInf = (PINF)NULL;
        }
        else
        {
            //
            // Keep track of line numbers for error reporting.
            //

            if (token.Type == TOK_EOL)
            {
                infLine++;
            }
        }

    } // END while

    if (pInf)
    {
        pInf->pSectionRecord = NULL;
    }

    return(pInf);
}

PCHAR
CmpProcessForSimpleStringSub(
    IN PINF pInf,
    IN PCHAR String
    )

/*++

    Routine Description:

        This routine substitutes reference to string in the STRINGS section of the inf.

    Input Parameters:

        pInf - Pointer to the inf to be processed.

        String - String to be substituted.

    Return Value:

        None.

--*/

{
    ULONG       len;
    PCHAR       returnString;
    PSECTION    pSection;
    PLINE       pLine;

    //
    // Assume no substitution necessary.
    //

    returnString = String;
    len = strlen(String);
    pSection = pInf->StringsSection;

    //
    // If it starts and end with % then look it up in the
    // strings section. Note the initial check before doing a
    // wcslen, to preserve performance in the 99% case where
    // there is no substitution.
    //

    if( String[0] == '%' &&
        len > 2 &&
        String[len - 1] == '%' &&
        pSection)
    {

        for(pLine = pSection->pLine; pLine; pLine = pLine->pNext)
        {
            if( pLine->pName &&
                _strnicmp(pLine->pName, String + 1, len - 2) == 0 &&
                pLine->pName[len - 2] == '\0')
            {
                break;
            }
        }

        if(pLine && pLine->pValue && pLine->pValue->pName)
        {
            returnString = pLine->pValue->pName;
        }
    }

    return(returnString);
}

VOID
CmpFreeValueList(
    IN PVALUE pValue
    )

/*++

    Routine Description:

        This routine releases memory for the list of values.

    Input Parameters:

        pValue - Pointer to the value list to be freed.

    Return Value:

        None.

--*/

{
    PVALUE pNext;

    while (pValue)
    {
        //
        // Save the next pointer so we dont access memory after it has
        // been freed.
        //

        pNext = pValue->pNext;

        //
        // Free any data inside this value.
        //

        if (pValue->Allocated && pValue->pName)
        {
            ExFreePool((PVOID)pValue->pName);
        }

        //
        // Free memory for this value.
        //

        ExFreePool(pValue);

        //
        // Go to the next value.
        //

        pValue = pNext;
    }
}

VOID
CmpFreeLineList(
    IN PLINE pLine
    )

/*++

    Routine Description:

        This routine releases memory for the list of lines and
        values under it.

    Input Parameters:

        pLine - Pointer to the line list to be freed.

    Return Value:

        None.

--*/

{
    PLINE pNext;

    while (pLine)
    {
        //
        // Save the next pointer so we dont access memory after it has
        // been freed.
        //

        pNext = pLine->pNext;

        //
        // Free any data inside this Line.
        //

        if (pLine->Allocated && pLine->pName)
        {
            ExFreePool((PVOID)pLine->pName);
        }

        //
        // Free the list of values inside this Line.
        //

        CmpFreeValueList(pLine->pValue);

        //
        // Free memory for this line itself.
        //

        ExFreePool((PVOID)pLine);

        //
        // Go to the next line.
        //

        pLine = pNext;
    }
}

VOID
CmpFreeSectionList(
    IN PSECTION pSection
    )

/*++

    Routine Description:

        This routine releases memory for the list of sections and
        lines under it.

    Input Parameters:

        pSection - Pointer to the section list to be freed.

    Return Value:

        None.

--*/

{
    PSECTION pNext;

    while (pSection)
    {
        //
        // Save the next pointer so we dont access memory after it has
        // been freed.
        //

        pNext = pSection->pNext;

        //
        // Free any data inside this Line.
        //

        if (pSection->Allocated && pSection->pName)
        {
            ExFreePool((PVOID)pSection->pName);
        }

        //
        // Free the list of values inside this Line.
        //

        CmpFreeLineList(pSection->pLine);

        //
        // Free memory for this line itself.
        //

        ExFreePool((PVOID)pSection);

        //
        // Go to the next line.
        //

        pSection = pNext;
    }

}

PVALUE
CmpSearchValueInLine(
    IN PLINE pLine,
    IN ULONG ValueIndex
    )

/*++

    Routine Description:

        This routine searches for the specified value in the inf.

    Input Parameters:

        pLine - Pointer to the line to be searched.

        ValueIndex - Index of the value to be searched.

    Return Value:

        Pointer to the value iff found. Else NULL.

--*/

{
    ULONG   i;
    PVALUE  pValue = NULL;

    if (pLine)
    {
        for (   i = 0, pValue = pLine->pValue;
                i < ValueIndex && pValue;
                i++, pValue = pValue->pNext);
    }

    return (pValue);
}


PSECTION
CmpSearchSectionByName(
    IN PINF  pInf,
    IN PCHAR SectionName
    )

/*++

    Routine Description:

        This routine searches for the specified section in the inf.

    Input Parameters:

        pInf - Pointer to the inf to be searched.

        SectionName - Name of the section to be searched.

    Return Value:

        Pointer to the section iff found. Else NULL.

--*/

{
    PSECTION    pSection = NULL;
    PSECTION    pFirstSearchedSection;

    //
    // Validate the parameters passed in.
    //

    if (pInf && SectionName)
    {
        //
        // Traverse down the section list searching each section for the
        // section name mentioned.
        //

        for (   pSection = pFirstSearchedSection = pInf->pSectionRecord;
                pSection && _stricmp(pSection->pName, SectionName);
                pSection = pSection->pNext);

        //
        // If we did not find the section, search from the beginning.
        //

        if (pSection == NULL)
        {
            for (   pSection = pInf->pSection;
                    pSection && pSection != pFirstSearchedSection;
                    pSection = pSection->pNext)
            {
                if (pSection->pName && _stricmp(pSection->pName, SectionName) == 0)
                {
                    break;
                }
            }

            if (pSection == pFirstSearchedSection)
            {
                pSection = NULL;
            }
        }

        if (pSection)
        {
            pInf->pSectionRecord = pSection;
        }
    }

    //
    // Return the section at which we stopped.
    //

    return (pSection);
}

PLINE
CmpSearchLineInSectionByIndex(
    IN PSECTION pSection,
    IN ULONG    LineIndex
    )

/*++

    Routine Description:

        This routine searches for the specified line in the inf.

    Input Parameters:

        pSection - Pointer to the section to be searched.

        LineIndex - Index of the line to be searched.

    Return Value:

        Pointer to the line iff found. Else NULL.

--*/

{
    PLINE   pLine = NULL;
    ULONG   i;

    //
    // Validate the parameters passed in.
    //

    if (pSection)
    {

        //
        // Traverse down the current line list to the LineIndex line.
        //

        for(    i = 0, pLine = pSection->pLine;
                i < LineIndex && pLine;
                i++, pLine = pLine->pNext);
    }

    //
    // Return the Line found
    //

    return (pLine);
}

PVOID
CmpOpenInfFile(
    IN  PVOID   InfImage,
    IN  ULONG   ImageSize
   )

/*++

    Routine Description:

        This routine opens an handle to the inf.

    Input Parameters:

        InfImage - Pointer to the inf image read into memory.

        ImageSize - Image size.

    Return Value:

        Returns handle to the inf iff successful. Else NULL.

--*/

{
    PINF    infHandle;
    ULONG   errorLine = 0;

    //
    // Parse the inf buffer.
    //

    infHandle = CmpParseInfBuffer(InfImage, ImageSize, &errorLine);

    if (infHandle == NULL)
    {
        DbgPrint("Error on line %d in CmpOpenInfFile!\n", errorLine);
    }

    return (infHandle);
}

VOID
CmpCloseInfFile(
    PVOID   InfHandle
    )

/*++

    Routine Description:

        This routine closes the inf handle by releasing any
        memory allocated for it during parsing.

    Input Parameters:

        InfHandle - Handle to the inf to be closed.

    Return Value:

        None.

--*/

{
    if (InfHandle)
    {
        CmpFreeSectionList(((PINF)InfHandle)->pSection);
        ExFreePool(InfHandle);
    }
}

BOOLEAN
CmpSearchInfSection(
    IN PINF  pInf,
    IN PCHAR Section
    )

/*++

    Routine Description:

        This routine searches for the specified section in the inf.

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Section - Name of the section to be read.

    Return Value:

        TRUE iff section is found in the inf.

--*/

{
    return (CmpSearchSectionByName(pInf, Section) != NULL);
}

PCHAR
CmpGetKeyName(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG LineIndex
    )

/*++

    Routine Description:

        This routine returns the name of the specified line in the inf.

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Section - Name of the section to be read.

        LineIndex - Index of the line to be read.

    Return Value:

        Pointer to the name of line in the inf iff successful. Else NULL.

--*/

{
    PSECTION    pSection;
    PLINE       pLine;

    //
    // First search the section.
    //

    pSection = CmpSearchSectionByName((PINF)InfHandle, Section);
    if(pSection)
    {
        //
        // Get the line in the section.
        //

        pLine = CmpSearchLineInSectionByIndex(pSection, LineIndex);
        if(pLine)
        {
            return(pLine->pName);
        }
    }

    return (NULL);
}

BOOLEAN
CmpSearchInfLine(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG LineIndex
    )

/*++

    Routine Description:

        This routine searches for the specified line in the inf.

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Section - Name of the section to be read.

        LineIndex - Index of the line to be read.

    Return Value:

        TRUE iff line is found in the section in the inf.

--*/

{
    PSECTION    pSection;
    PLINE       pLine = NULL;

    //
    // First search the section.
    //

    pSection = CmpSearchSectionByName((PINF)InfHandle, Section);
    if(pSection)
    {
        //
        // Search the line in the section.
        //

        pLine = CmpSearchLineInSectionByIndex(pSection, LineIndex);
    }

    return (pLine != NULL);
}


PCHAR
CmpGetSectionLineIndex (
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG LineIndex,
    IN ULONG ValueIndex
    )

/*++

    Routine Description:

        This routine returns the value at the specified location in the inf.

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Section - Name of the section to be read.

        LineIndex - Index of the line to be read.

        ValueIndex - Index of the value to be read.

    Return Value:

        Pointer to the value iff successful. Else NULL.

--*/

{
    PSECTION pSection;
    PLINE    pLine;
    PVALUE   pValue;

    //
    // Search the section in the inf.
    //

    pSection = CmpSearchSectionByName((PINF)InfHandle, Section);
    if(pSection)
    {
        //
        // Search the line in the section.
        //

        pLine = CmpSearchLineInSectionByIndex(pSection, LineIndex);
        if(pLine)
        {
            //
            // Search the value in the line.
            //

            pValue = CmpSearchValueInLine(pLine, ValueIndex);
            if(pValue)
            {
                //
                // The value may need to be replaced by one of the strings
                // from the string section.
                //

                return(CmpProcessForSimpleStringSub(InfHandle, pValue->pName));
            }
        }
    }

    return(NULL);
}

ULONG
CmpGetSectionLineIndexValueCount(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG LineIndex
    )

/*++

    Routine Description:

        This routine returns the number of values in the inf line.

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Section - Name of the section to be read.

        LineIndex - Index of the line to be read.

    Return Value:

        Number of values in the inf line.

--*/

{
    PSECTION    pSection;
    PLINE       pLine;
    PVALUE      pValue;
    ULONG       count = 0;

    //
    // Search the section in the inf.
    //

    pSection = CmpSearchSectionByName((PINF)InfHandle, Section);
    if(pSection)
    {
        //
        // Search the line in the section.
        //

        pLine = CmpSearchLineInSectionByIndex(pSection, LineIndex);
        if (pLine)
        {
            //
            // Count the number of values in this line.
            //

            for(    pValue = pLine->pValue;
                    pValue;
                    pValue = pValue->pNext, count++);
        }
    }

    return (count);
}

BOOLEAN
CmpGetIntField(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG LineIndex,
    IN ULONG ValueIndex,
    IN OUT PULONG Data
    )

/*++

    Routine Description:

        This routine reads integer data from the inf.

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Section - Name of the section to be read.

        LineIndex - Index of the line to be read.

        ValueIndex - Index of the value to be read.

        Data - Receives the integer data.

    Return Value:

        TRUE iff successful.

--*/

{
    PCHAR   valueStr;

    //
    // Get the specified value.
    //

    valueStr = CmpGetSectionLineIndex(  InfHandle,
                                        Section,
                                        LineIndex,
                                        ValueIndex);
    //
    // If valid value is found, convert it to an integer.
    //

    if (valueStr && *valueStr)
    {
        *Data = strtoul(valueStr, NULL, 16);
        return (TRUE);
    }

    return (FALSE);
}

BOOLEAN
CmpGetBinaryField(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG LineIndex,
    IN ULONG ValueIndex,
    IN OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN OUT PULONG ActualSize
    )

/*++

    Routine Description:

        This routine reads binary data from the inf.

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Section - Name of the section to be read.

        LineIndex - Index of the line to be read.

        ValueIndex - Index of the value to be read.

        Buffer - Receives the binary data read.

        BufferSize - Size of the buffer.

        ActualSize - Receives the size of the data buffer required.

    Return Value:

        TRUE iff successful.

--*/

{
    BOOLEAN     result = FALSE;
    ULONG       requiredSize;
    PSECTION    pSection;
    PLINE       pLine;
    PVALUE      pValue;
    ULONG       count;
    PCHAR       valueStr;

    //
    // Compute the size of buffer required to read in the binary data.
    //

    requiredSize = (CmpGetSectionLineIndexValueCount(   InfHandle,
                                                        Section,
                                                        LineIndex) - ValueIndex) * sizeof(UCHAR);
    //
    // Validate input parameters.
    //

    if (Buffer && BufferSize >= requiredSize)
    {
        //
        // Search the section in the inf.
        //

        pSection = CmpSearchSectionByName((PINF)InfHandle, Section);
        if(pSection)
        {
            //
            // Search the line in this section.
            //

            pLine = CmpSearchLineInSectionByIndex(pSection, LineIndex);
            if (pLine)
            {
                //
                // Go to the specified value.
                //

                for(    pValue = pLine->pValue, count = 0;
                        pValue && count < ValueIndex;
                        pValue = pValue->pNext, count++);

                //
                // Read in and convert the binary data.
                //

                for (   ;
                        pValue;
                        pValue = pValue->pNext)
                {
                    valueStr = CmpGetSectionLineIndex(  InfHandle,
                                                        Section,
                                                        LineIndex,
                                                        ValueIndex++);
                    if (valueStr == NULL)
                    {
                        break;
                    }
                    *((PUCHAR)Buffer)++ = (UCHAR)strtoul(valueStr, NULL, 16);
                }
                if (valueStr)
                {
                    result = TRUE;
                }
            }
        }
    }

    //
    // The caller wants to know the buffer size required.
    //

    if (ActualSize)
    {
        *ActualSize = requiredSize;
        result = TRUE;
    }

    return (result);
}
