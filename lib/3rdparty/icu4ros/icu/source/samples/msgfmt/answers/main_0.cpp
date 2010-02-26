/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1999-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/unistr.h"
#include "unicode/msgfmt.h"
#include "unicode/uclean.h"
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

int main(int argc, char **argv) {

    UErrorCode status = U_ZERO_ERROR;
    UnicodeString str;

    printf("Message: ");
    uprintf(str);
    printf("\n");

    u_cleanup();
    printf("Exiting successfully\n");
    return 0;
}
