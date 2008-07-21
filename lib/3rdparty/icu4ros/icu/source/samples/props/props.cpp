/*
*******************************************************************************
*
*   Copyright (C) 2000, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  props.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2000sep22
*   created by: Markus W. Scherer
*
*   This file contains sample code that illustrates the use of the ICU APIs
*   for Unicode character properties.
*/

#include <stdio.h>
#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/uclean.h"

static void
printProps(UChar32 codePoint) {
    char buffer[100];
    UErrorCode errorCode;

    /* get the character name */
    errorCode=U_ZERO_ERROR;
    u_charName(codePoint, U_UNICODE_CHAR_NAME, buffer, sizeof(buffer), &errorCode);

    /* print the code point and the character name */
    printf("U+%04lx\t%s\n", codePoint, buffer);

    /* print some properties */
    printf("  general category (numeric enum value): %u\n", u_charType(codePoint));

    /* note: these APIs do not provide the data from SpecialCasing.txt */
    printf("  is lowercase: %d  uppercase: U+%04lx\n", u_islower(codePoint), u_toupper(codePoint));

    printf("  is digit: %d  decimal digit value: %d\n", u_isdigit(codePoint), u_charDigitValue(codePoint));

    printf("  BiDi directional category (numeric enum value): %u\n", u_charDirection(codePoint));
}

/* Note: In ICU 2.0, the Unicode class is deprecated - it is a pure wrapper around the C APIs above. */

extern int
main(int argc, const char *argv[]) {
    static const UChar32
    codePoints[]={
        0xd, 0x20, 0x2d, 0x35, 0x65, 0x284, 0x665, 0x5678, 0x23456, 0x10317, 0x1D01F, 0x10fffd
    };
    int i;

    for(i=0; i<sizeof(codePoints)/sizeof(codePoints[0]); ++i) {
        printProps(codePoints[i]);
        puts("");
    }

    u_cleanup();
    return 0;
}
