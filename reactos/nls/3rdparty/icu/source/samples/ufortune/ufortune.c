/**************************************************************************
*
*   Copyright (C) 2001-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
***************************************************************************
*
*   ufortune - An ICU resources sample program
*
*      Demonstrates
*         Defining resources for use by an application
*         Compiling and packaging them into a dll
*         Referencing the resource-containing dll from application code
*         Loading resource data using ICU's API
*
*      Created Nov. 7, 2001  by Andy Heninger
*
*      ufortune is a variant of the Unix "fortune" command, with
*               ICU resources that contain the fortune-cookie sayings.
*               Using resources allows  fortunes in different languages to
*               be selected based on locale.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "unicode/udata.h"     /* ICU API for data handling.                 */
#include "unicode/ures.h"      /* ICU API for resource loading               */
#include "unicode/ustdio.h"    /* ICU API for reading & writing Unicode data */
                               /*   to files, possibly including character   */
                               /*   set conversions.                         */
#include "unicode/ustring.h"

#ifndef UFORTUNE_NOSETAPPDATA
/*
 *  Resource Data Reference.  The data is packaged as a dll (or .so or
 *           whatever, depending on the platform) that exports a data
 *           symbol.  The application (that's us) references that symbol,
 *           here, and will pass the data address to ICU, which will then
 *           be able to fetch resources from the data.
 */
extern  const void U_IMPORT *fortune_resources_dat;
#endif

void u_write(const UChar *what, int len);


/*
 *  main()   This one function is all of the application code.
 */
int main(int argc, char **argv)
{
    UBool              displayUsage  = FALSE;    /* Set true if command line err or help      */
                                                 /*   option was requested.                   */
    UBool              verbose       = FALSE;    /* Set true if -v command line option.       */
    char              *optionError   = NULL;     /* If command line contains an unrecognized  */
                                                 /*   option, this will point to it.          */
    char              *locale=NULL;              /* Locale name.  Null for system default,    */
                                                 /*   otherwise set from command line.        */
    const char *       programName   = argv[0];  /* Program invocation name.                  */


    UFILE             *u_stdout;                 /* Unicode stdout file.                      */
    UErrorCode         err           = U_ZERO_ERROR;   /* Error return, used for most ICU     */
                                                       /*   functions.                        */

    UResourceBundle   *myResources;              /* ICU Resource "handles"                    */
    UResourceBundle   *fortunes_r;

    int32_t            numFortunes;              /* Number of fortune strings available.      */
    int                i;

    const UChar       *resString;                /* Points to strings fetched from Resources. */
    int32_t            len;


    /*  Process command line options.
     *     -l  locale          specify a locale
     *     -v                  verbose mode.  Display extra messages.
     *     -? or --help        display a usage line
     */
    for (i=1; i<argc; i++) {
        if (strcmp(argv[i], "-l") ==0) {
            if (++i < argc) {
                locale = argv[i];
            }
            continue;
        }
        if (strcmp(argv[i], "-v") == 0) {
            verbose = TRUE;
            continue;}
        if (strcmp(argv[i], "-?") == 0 ||
            strcmp(argv[i], "--help") == 0) {
            displayUsage = TRUE;
            continue;}
        optionError = argv[i];
        displayUsage = TRUE;
        break;
    }

    /* ICU's icuio package provides a convenient way to write Unicode
     *    data to stdout.  The string data that we get from resources
     *    will be UChar * strings, which icuio can handle nicely.
     */
    u_stdout = u_finit(stdout, NULL /*locale*/,  NULL /*codepage */);
    if (verbose) {
        u_fprintf(u_stdout, "%s:  checking output via icuio.\n", programName);
    }

#ifndef UFORTUNE_NOSETAPPDATA
    /* Tell ICU where our resource data is located in memory.
     *   The data lives in the Fortune_Resources dll, and we just
     *   pass the address of an exported symbol from that library
     *   to ICU.
     */
    udata_setAppData("fortune_resources", &fortune_resources_dat, &err);
    if (U_FAILURE(err)) {
        fprintf(stderr, "%s: udata_setAppData failed with error \"%s\"\n", programName, u_errorName(err));
        exit(-1);
    }
#endif

    /* Open our resources.
    */
    myResources = ures_open("fortune_resources", locale, &err);
    if (U_FAILURE(err)) {
        fprintf(stderr, "%s: ures_open failed with error \"%s\"\n", programName, u_errorName(err));
        exit(-1);
    }
    if (verbose) {
        u_fprintf(u_stdout, "status from ures_open(\"fortune_resources\", %s) is %s\n",
            locale? locale: " ", u_errorName(err));
    }

    /*
     * Display any command line option usage errors and/or the
     *     usage help message.  These messages come from our resource bundle.
     */
    if (optionError != NULL) {
        const UChar *msg = ures_getStringByKey(myResources, "optionMessage", &len, &err);
        if (U_FAILURE(err)) {
            fprintf(stderr, "%s: ures_getStringByKey(\"optionMessage\") failed, %s\n",
                programName, u_errorName(err));
            exit(-1);
        }
        u_file_write(msg,  len, u_stdout);              /* msg is UChar *, from resource    */
        u_fprintf(u_stdout, " %s\n", optionError);      /* optionError is char *, from argv */
    }

    if (displayUsage) {
        const UChar *usage;
        int          returnValue=0;

        usage = ures_getStringByKey(myResources, "usage", &len, &err);
        if (U_FAILURE(err)) {
            fprintf(stderr, "%s: ures_getStringByKey(\"usage\") failed, %s\n", programName, u_errorName(err));
            exit(-1);
        }
        u_file_write(usage,  len, u_stdout);
        if (optionError != NULL) {returnValue = -1;}
        return returnValue;
    }

    /*
     * Open the "fortunes" resources from within the already open resources
     */
    fortunes_r = ures_getByKey(myResources, "fortunes", NULL, &err);
    if (U_FAILURE(err)) {
        fprintf(stderr, "%s: ures_getByKey(\"fortunes\") failed, %s\n", programName, u_errorName(err));
        exit(-1);
    }


    /*
     * Pick up and display a random fortune
     *
     */
    numFortunes = ures_countArrayItems(myResources, "fortunes", &err);
    if (U_FAILURE(err)) {
        fprintf(stderr, "%s: ures_countArrayItems(\"fortunes\") failed, %s\n", programName, u_errorName(err));
        exit(-1);
    }
    if (numFortunes <= 0) {
        fprintf(stderr, "%s: no fortunes found.\n", programName);
        exit(-1);
    }

    i = (int)time(NULL) % numFortunes;    /*  Use time to pick a somewhat-random fortune.  */
    resString = ures_getStringByIndex(fortunes_r, i, &len, &err);
    if (U_FAILURE(err)) {
        fprintf(stderr, "%s: ures_getStringByIndex(%d) failed, %s\n", programName, i, u_errorName(err));
        exit(-1);
    }

    u_file_write(resString, len, u_stdout);      /* Write out the message           */
	u_fputc(0x0a, u_stdout);                     /*   and a trailing newline	    */

    return 0;
}

