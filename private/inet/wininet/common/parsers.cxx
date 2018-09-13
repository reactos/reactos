/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    parsers.cxx

Abstract:

    Common text parsing functions (generally moved here from other protocols)

    Contents:
        ExtractWord
        ExtractDword
        ExtractInt
        SkipWhitespace
        SkipSpaces
        SkipLine
        FindToken
        NiceNum

Author:

    Richard L Firth (rfirth) 03-Jul-1996

Revision History:

    03-Jul-1996 rfirth
        Created

--*/

#include <wininetp.h>

//
// functions
//


BOOL
ExtractWord(
    IN OUT LPSTR* pString,
    IN DWORD NumberLength,
    OUT LPWORD pConvertedNumber
    )

/*++

Routine Description:

    Sucks a <NumberLength> character number out of a string.

    Assumes:    1. The number to be converted is an unsigned short
                2. A whole number is contained within *pString

Arguments:

    pString             - pointer to pointer to string from which to get number

    NumberLength        - number of characters that comprise number string, if
                          not equal to 0, else if 0, we don't know the length
                          of the number string a priori

    pConvertedNumber    - pointer to variable where converted number written

Return Value:

    BOOL
        TRUE    - number converted OK

        FALSE   - one of the characters in the number is not a digit

--*/

{
    WORD number;
    BOOL exact;
    LPSTR string;

    //
    // if the caller doesn't know how many characters comprise the number, then
    // we will convert until the next non-digit character, or until we have
    // converted the maximum number of digits that can comprise an unsigned
    // short value
    //

    if (NumberLength == 0) {
        NumberLength = sizeof("65535") - 1;
        exact = FALSE;
    } else {
        exact = TRUE;
    }
    number = 0;
    string = *pString;
    while (NumberLength && isdigit(*string)) {
        number = number * 10 + (WORD)((BYTE)(*string++) - (BYTE)'0');
        --NumberLength;
    }
    *pConvertedNumber = number;
    *pString = string;

    //
    // if we were asked to convert a certain number of characters but failed
    // because we hit a non-digit character, then return FALSE. Anything else
    // (we converted required number of characters, or the caller didn't know
    // how many characters comprised the number) is TRUE
    //

    return (exact && (NumberLength != 0)) ? FALSE : TRUE;
}


BOOL
ExtractDword(
    IN OUT LPSTR* pString,
    IN DWORD NumberLength,
    OUT LPDWORD pConvertedNumber
    )

/*++

Routine Description:

    Sucks a <NumberLength> character number out of a string.

    Assumes:    1. The number to be converted is an unsigned long
                2. A whole number is contained within *pString

Arguments:

    pString             - pointer to pointer to string from which to get number

    NumberLength        - number of characters that comprise number string, if
                          not equal to 0, else if 0, we don't know the length
                          of the number string a priori

    pConvertedNumber    - pointer to variable where converted number written

Return Value:

    BOOL
        TRUE    - number converted OK

        FALSE   - one of the characters in the number is not a digit

--*/

{
    DWORD number;
    BOOL exact;

    //
    // if the caller doesn't know how many characters comprise the number, then
    // we will convert until the next non-digit character, or until we have
    // converted the maximum number of digits that can comprise an unsigned
    // short value
    //

    if (NumberLength == 0) {
        NumberLength = sizeof("4294967295") - 1;
        exact = FALSE;
    } else {
        exact = TRUE;
    }
    for (number = 0; isdigit(**pString) && NumberLength--; ) {
        number = number * 10 + (DWORD)((BYTE)*((*pString)++) - (BYTE)'0');
    }
    *pConvertedNumber = number;

    //
    // if we were asked to convert a certain number of characters but failed
    // because we hit a non-digit character, then return FALSE. Anything else
    // (we converted required number of characters, or the caller didn't know
    // how many characters comprised the number) is TRUE
    //

    return (exact && (NumberLength != 0)) ? FALSE : TRUE;
}


BOOL
ExtractInt(
    IN OUT LPSTR* pString,
    IN DWORD NumberLength,
    OUT LPINT pConvertedNumber
    )

/*++

Routine Description:

    Sucks a <NumberLength> character number out of a string.

    Assumes:    1. The number to be converted is an signed integer (32-bits)

Arguments:

    pString             - pointer to pointer to string from which to get number

    NumberLength        - number of characters that comprise number string, if
                          not equal to 0, else if 0, we don't know the length
                          of the number string a priori

    pConvertedNumber    - pointer to variable where converted number written

Return Value:

    BOOL
        TRUE    - number converted OK

        FALSE   - one of the characters in the number is not a digit

--*/

{
    int number;
    int sign;
    BOOL exact;

    if ((**pString == '-') || (**pString == '+')) {
        sign = (**pString == '-') ? -1 : +1;
        if (NumberLength) {
            --NumberLength;
        }
        ++*pString;
    } else {
        sign = 1;
    }

    //
    // if the caller doesn't know how many characters comprise the number, then
    // we will convert until the next non-digit character, or until we have
    // converted the maximum number of digits that can comprise an unsigned
    // short value
    //

    if (NumberLength == 0) {
        NumberLength = sizeof("2147483647") - 1;
        exact = FALSE;
    } else {
        exact = TRUE;
    }
    for (number = 0; isdigit(**pString) && NumberLength; ) {
        number = number * 10 + (INT)(((BYTE)**pString) - (BYTE)'0');
        ++*pString;
        --NumberLength;
    }
    *pConvertedNumber = number * sign;

    //
    // if we were asked to convert a certain number of characters but failed
    // because we hit a non-digit character, then return FALSE. Anything else
    // (we converted required number of characters, or the caller didn't know
    // how many characters comprised the number) is TRUE
    //

    return (exact && (NumberLength != 0)) ? FALSE : TRUE;
}


BOOL
SkipWhitespace(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength
    )

/*++

Routine Description:

    Skips any whitespace characters

Arguments:

    lpBuffer        - pointer to pointer to buffer

    lpBufferLength  - pointer to remaining buffer length

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. No more data left in buffer

--*/

{
    while ((*lpBufferLength != 0) && isspace(**lpBuffer)) {
        ++*lpBuffer;
        --*lpBufferLength;
    }
    return *lpBufferLength != 0;
}


BOOL
SkipSpaces(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength
    )

/*++

Routine Description:

    Skips any space characters. We only look for the actual space character

Arguments:

    lpBuffer        - pointer to pointer to buffer

    lpBufferLength  - pointer to remaining buffer length

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. No more data left in buffer

--*/

{
    while ((*lpBufferLength != 0) && (**lpBuffer == ' ')) {
        ++*lpBuffer;
        --*lpBufferLength;
    }
    return *lpBufferLength != 0;
}


BOOL
SkipLine(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength
    )

/*++

Routine Description:

    Positions text pointer at start of next non-empty line

Arguments:

    lpBuffer        - pointer to string. Updated on output

    lpBufferLength  - pointer to remaining length of string. Updated on output

Return Value:

    BOOL
        TRUE    - found start of next non-empty line

        FALSE   - ran out of buffer

--*/

{
    while ((*lpBufferLength != 0) && (**lpBuffer != '\r') && (**lpBuffer != '\n')) {
        ++*lpBuffer;
        --*lpBufferLength;
    }
    while ((*lpBufferLength != 0) && ((**lpBuffer == '\r') || (**lpBuffer == '\n'))) {
        ++*lpBuffer;
        --*lpBufferLength;
    }
    return *lpBufferLength != 0;
}


BOOL
FindToken(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength
    )

/*++

Routine Description:

    Moves over the current token, past any spaces, and to the start of the next
    token

Arguments:

    lpBuffer        - pointer to pointer to buffer

    lpBufferLength  - pointer to remaining buffer length

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. No more data left in buffer

--*/

{
    while ((*lpBufferLength != 0) && !isspace(**lpBuffer)) {
        ++*lpBuffer;
        --*lpBufferLength;
    }
    while ((*lpBufferLength != 0) && isspace(**lpBuffer)) {
        ++*lpBuffer;
        --*lpBufferLength;
    }
    return *lpBufferLength != 0;
}


BOOL
FindTokenSpecial(
    IN OUT LPSTR* lpBuffer,
    IN OUT LPDWORD lpBufferLength
    )

/*++

Routine Description:

    Moves over the current token, past any spaces, and to the start of the next
    token

Arguments:

    lpBuffer        - pointer to pointer to buffer

    lpBufferLength  - pointer to remaining buffer length

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. No more data left in buffer

--*/

{
    while ((*lpBufferLength != 0) && !isspace(**lpBuffer)) {
        ++*lpBuffer;
        --*lpBufferLength;
    }
    while (*lpBufferLength != 0) {
        CHAR ch =  **lpBuffer;
        if (((ch < 0x09) || (ch > 0x0d)) && (ch!=0x20))
            break;
        ++*lpBuffer;
        --*lpBufferLength;
    }
    return *lpBufferLength != 0;
}

LPSTR
NiceNum(
    OUT LPSTR Buffer,
    IN SIZE_T Number,
    IN int FieldWidth
    )

/*++

Routine Description:

    Converts a number to a string. The string is very human-sensible (i.e.
    1,234,567 instead of 1234567. Sometimes its hard to make out these numbers
    when your salary is so large)

Arguments:

    Buffer      - place to put resultant string

    Number      - to convert

    FieldWidth  - maximum width of the field, or 0 for "don't care"

Return Value:

    LPSTR
        pointer to Buffer

--*/

{
    int i;

    if (Number == 0) {
        if (FieldWidth == 0) {
            Buffer[0] = '0';
            Buffer[1] = '\0';
        } else {
            memset(Buffer, ' ', FieldWidth);
            Buffer[FieldWidth - 1] = '0';
            Buffer[FieldWidth] = '\0';
        }
    } else {

        //
        // if the caller specified zero for the field width then work out how
        // many characters the string will occupy
        //

        if (FieldWidth == 0) {

            SIZE_T n;

            n = Number;
            ++FieldWidth;
            while (n >= 10) {
                n /= 10;
                ++FieldWidth;
            }

            FieldWidth += (FieldWidth / 3) - (((FieldWidth % 3) == 0) ? 1 : 0);
        }

        //
        // now create the representation
        //

        Buffer[FieldWidth] = '\0';
        Buffer += FieldWidth;
        i = 0;
        while (Number && FieldWidth) {
            *--Buffer = (char)((Number % 10) + '0');
            --FieldWidth;
            Number /= 10;
            if ((++i == 3) && FieldWidth) {
                if (Number) {
                    *--Buffer = ',';
                    --FieldWidth;
                    i = 0;
                }
            }
        }
        while (FieldWidth--) {
            *--Buffer = ' ';
        }
    }
    return Buffer;
}
