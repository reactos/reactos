/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1999-2002, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/unum.h"
#include "unicode/ustring.h"
#include <stdio.h>
#include <stdlib.h>

static void uprintf(const UChar* str) {
    char buf[256];
    u_austrcpy(buf, str);
    printf("%s", buf);
}

void capi() {
    UNumberFormat *fmt;
    UErrorCode status = U_ZERO_ERROR;
    /* The string "987654321.123" as UChars */
    UChar str[] = { 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33,
                    0x32, 0x31, 0x30, 0x2E, 0x31, 0x32, 0x33, 0 };
    UChar buf[256];
    int32_t needed;
    double a;
    
    /* Create a formatter for the US locale */
    fmt = unum_open(
          UNUM_DECIMAL,      /* style         */
          0,                 /* pattern       */
          0,                 /* patternLength */
          "en_US",           /* locale        */
          0,                 /* parseErr      */
          &status);
    if (U_FAILURE(status)) {
        printf("FAIL: unum_open\n");
        exit(1);
    }

    /* Use the formatter to parse a number.  When using the C API,
       we have to specify whether we want a double or a long in advance.

       We pass in NULL for the position pointer in order to get the
       default behavior which is to parse from the start. */
    a = unum_parseDouble(fmt, str, u_strlen(str), NULL, &status);
    if (U_FAILURE(status)) {
        printf("FAIL: unum_parseDouble\n");
        exit(1);
    }

    /* Show the result */
    printf("unum_parseDouble(\"");
    uprintf(str);
    printf("\") => %g\n", a);

    /* Use the formatter to format the same number back into a string
       in the US locale.  The return value is the buffer size needed.
       We're pretty sure we have enough space, but in a production
       application one would check this value.

       We pass in NULL for the UFieldPosition pointer because we don't
       care to receive that data. */
    needed = unum_formatDouble(fmt, a, buf, 256, NULL, &status);
    if (U_FAILURE(status)) {
        printf("FAIL: format_parseDouble\n");
        exit(1);
    }

    /* Show the result */
    printf("unum_formatDouble(%g) => \"", a);
    uprintf(buf);
    printf("\"\n");

    /* Release the storage used by the formatter */
    unum_close(fmt);
}
