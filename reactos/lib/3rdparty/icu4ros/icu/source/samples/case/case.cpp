/*
*******************************************************************************
*
*   Copyright (C) 2003-2004, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*/

#include <unicode/unistr.h>
#include <unicode/ustdio.h>
#include <unicode/brkiter.h>
#include <stdlib.h>

U_CFUNC int c_main(UFILE *out);

void printUnicodeString(UFILE *out, const UnicodeString &s) {
    UnicodeString other = s;
    u_fprintf(out, "\"%S\"", other.getTerminatedBuffer());
}


int main( void )
{
    UFILE *out;
    UErrorCode status  = U_ZERO_ERROR;
    out = u_finit(stdout, NULL, NULL);
    if(!out) {
        fprintf(stderr, "Could not initialize (finit()) over stdout! \n");
        return 1;
    }
    ucnv_setFromUCallBack(u_fgetConverter(out), UCNV_FROM_U_CALLBACK_ESCAPE,
        NULL, NULL, NULL, &status);
    if(U_FAILURE(status)) {
        u_fprintf(out, "Warning- couldn't set the substitute callback - err %s\n", u_errorName(status));
    }

    /* End Demo boilerplate */

    u_fprintf(out,"ICU Case Mapping Sample Program\n\n");
    u_fprintf(out, "C++ Case Mapping\n\n");

    UnicodeString string("This is a test");
    /* lowercase = "istanbul" */ 
    UChar lowercase[] = {0x69, 0x73, 0x74, 0x61, 0x6e, 0x62, 0x75, 0x6c, 0};
    /* uppercase = "LATIN CAPITAL I WITH DOT ABOVE STANBUL" */  
    UChar uppercase[] = {0x0130, 0x53, 0x54, 0x41, 0x4e, 0x42, 0x55, 0x4C, 0};

    UnicodeString upper(uppercase);
    UnicodeString lower(lowercase);

    u_fprintf(out, "\nstring: ");
    printUnicodeString(out, string);
    string.toUpper(); /* string = "THIS IS A TEST" */
    u_fprintf(out, "\ntoUpper(): ");
    printUnicodeString(out, string);
    string.toLower(); /* string = "this is a test" */
    u_fprintf(out, "\ntoLower(): ");
    printUnicodeString(out, string);

    u_fprintf(out, "\n\nlowercase=%S, uppercase=%S\n", lowercase, uppercase);


    string = upper; 
    string.toLower(Locale("tr", "TR")); /* Turkish lower case map string =
                                        lowercase */
    u_fprintf(out, "\nupper.toLower: ");
    printUnicodeString(out, string);

    string = lower;
    string.toUpper(Locale("tr", "TR")); /* Turkish upper case map string =
                                        uppercase */
    u_fprintf(out, "\nlower.toUpper: ");
    printUnicodeString(out, string);


    u_fprintf(out, "\nEnd C++ sample\n\n");

    // Call the C version
    int rc = c_main(out);
    u_fclose(out);
    return rc;
}

