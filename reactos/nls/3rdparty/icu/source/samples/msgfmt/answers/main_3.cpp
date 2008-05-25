/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1999-2002, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/unistr.h"
#include "unicode/msgfmt.h"
#include "unicode/calendar.h"
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

// The message format pattern.  It takes a single argument, an integer,
// and formats it as "no", "one", or a number, using a NumberFormat.
static UnicodeString PATTERN(
    "Received {0,choice,0#no arguments|1#one argument|2#{0,number,integer} arguments}"
    " on {1,date,long}."
);

int main(int argc, char **argv) {

    UErrorCode status = U_ZERO_ERROR;
    UnicodeString str;
    FieldPosition pos;

    // Create a message format
    MessageFormat msg(PATTERN, status);
    check(status, "MessageFormat::ct");

    // Create the argument list
    Formattable msgArgs[2];
    msgArgs[0].setLong(argc-1);
    msgArgs[1].setDate(Calendar::getNow());

    // Format the arguments
    msg.format(msgArgs, 2, str, pos, status);
    check(status, "MessageFormat::format");

    printf("Message: ");
    uprintf(str);
    printf("\n");

    printf("Exiting successfully\n");
    return 0;
}
