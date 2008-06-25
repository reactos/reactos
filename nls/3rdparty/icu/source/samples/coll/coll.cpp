/********************************************************************
 * COPYRIGHT:
 * Copyright (C) 2002-2006 IBM, Inc.   All Rights Reserved.
 *
 ********************************************************************/

/** 
 * This program demos string collation
 */

const char gHelpString[] =
    "usage: coll [options*] -source source_string -target target_string\n"
    "-help            Display this message.\n"
    "-locale name     ICU locale to use.  Default is en_US\n"
    "-rules rule      Collation rules file (overrides locale)\n"
    "-french          French accent ordering\n"
    "-norm            Normalizing mode on\n"
    "-shifted         Shifted mode\n"
    "-lower           Lower case first\n"
    "-upper           Upper case first\n"
    "-case            Enable separate case level\n"
    "-level n         Sort level, 1 to 5, for Primary, Secndary, Tertiary, Quaternary, Identical\n"
	"-source string   Source string for comparison\n"
	"-target string   Target string for comparison\n"
    "Example coll -rules \\u0026b\\u003ca -source a -target b\n"
	"The format \\uXXXX is supported for the rules and comparison strings\n"
	;

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/ustring.h>

/** 
 * Command line option variables
 *    These global variables are set according to the options specified
 *    on the command line by the user.
 */
char * opt_locale     = "en_US";
char * opt_rules      = 0;
UBool  opt_help       = FALSE;
UBool  opt_norm       = FALSE;
UBool  opt_french     = FALSE;
UBool  opt_shifted    = FALSE;
UBool  opt_lower      = FALSE;
UBool  opt_upper      = FALSE;
UBool  opt_case       = FALSE;
int    opt_level      = 0;
char * opt_source     = "abc";
char * opt_target     = "abd";
UCollator * collator  = 0;

/** 
 * Definitions for the command line options
 */
struct OptSpec {
    const char *name;
    enum {FLAG, NUM, STRING} type;
    void *pVar;
};

OptSpec opts[] = {
    {"-locale",      OptSpec::STRING, &opt_locale},
    {"-rules",       OptSpec::STRING, &opt_rules},
	{"-source",      OptSpec::STRING, &opt_source},
    {"-target",      OptSpec::STRING, &opt_target},
    {"-norm",        OptSpec::FLAG,   &opt_norm},
    {"-french",      OptSpec::FLAG,   &opt_french},
    {"-shifted",     OptSpec::FLAG,   &opt_shifted},
    {"-lower",       OptSpec::FLAG,   &opt_lower},
    {"-upper",       OptSpec::FLAG,   &opt_upper},
    {"-case",        OptSpec::FLAG,   &opt_case},
    {"-level",       OptSpec::NUM,    &opt_level},
    {"-help",        OptSpec::FLAG,   &opt_help},
    {"-?",           OptSpec::FLAG,   &opt_help},
    {0, OptSpec::FLAG, 0}
};

/**  
 * processOptions()  Function to read the command line options.
 */
UBool processOptions(int argc, const char **argv, OptSpec opts[])
{
    for (int argNum = 1; argNum < argc; argNum ++) {
        const char *pArgName = argv[argNum];
        OptSpec *pOpt;
        for (pOpt = opts;  pOpt->name != 0; pOpt ++) {
            if (strcmp(pOpt->name, pArgName) == 0) {
                switch (pOpt->type) {
                case OptSpec::FLAG:
                    *(UBool *)(pOpt->pVar) = TRUE;
                    break;
                case OptSpec::STRING:
                    argNum ++;
                    if (argNum >= argc) {
                        fprintf(stderr, "value expected for \"%s\" option.\n", 
							    pOpt->name);
                        return FALSE;
                    }
                    *(const char **)(pOpt->pVar) = argv[argNum];
                    break;
                case OptSpec::NUM:
                    argNum ++;
                    if (argNum >= argc) {
                        fprintf(stderr, "value expected for \"%s\" option.\n", 
							    pOpt->name);
                        return FALSE;
                    }
                    char *endp;
                    int i = strtol(argv[argNum], &endp, 0);
                    if (endp == argv[argNum]) {
                        fprintf(stderr, 
							    "integer value expected for \"%s\" option.\n", 
								pOpt->name);
                        return FALSE;
                    }
                    *(int *)(pOpt->pVar) = i;
                }
                break;
            }
        }
        if (pOpt->name == 0)
        {
            fprintf(stderr, "Unrecognized option \"%s\"\n", pArgName);
            return FALSE;
        }
    }
	return TRUE;
}

/**
 * ICU string comparison
 */
int strcmp() 
{
	UChar source[100];
	UChar target[100];
	u_unescape(opt_source, source, 100);
	u_unescape(opt_target, target, 100);
    UCollationResult result = ucol_strcoll(collator, source, -1, target, -1);
    if (result == UCOL_LESS) {
		return -1;
	}
    else if (result == UCOL_GREATER) {
		return 1;
	}
	return 0;
}

/**
 * Creates a collator
 */
UBool processCollator()
{
	// Set up an ICU collator
    UErrorCode status = U_ZERO_ERROR;
	UChar rules[100];

    if (opt_rules != 0) {
		u_unescape(opt_rules, rules, 100);
        collator = ucol_openRules(rules, -1, UCOL_OFF, UCOL_TERTIARY, 
			                  NULL, &status);
    }
    else {
        collator = ucol_open(opt_locale, &status);
    }
	if (U_FAILURE(status)) {
        fprintf(stderr, "Collator creation failed.: %d\n", status);
        return FALSE;
    }
    if (status == U_USING_DEFAULT_WARNING) {
        fprintf(stderr, "Warning, U_USING_DEFAULT_WARNING for %s\n", 
			    opt_locale);
    }
    if (status == U_USING_FALLBACK_WARNING) {
        fprintf(stderr, "Warning, U_USING_FALLBACK_ERROR for %s\n", 
			    opt_locale);
    }
    if (opt_norm) {
        ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
    }
    if (opt_french) {
        ucol_setAttribute(collator, UCOL_FRENCH_COLLATION, UCOL_ON, &status);
    }
    if (opt_lower) {
        ucol_setAttribute(collator, UCOL_CASE_FIRST, UCOL_LOWER_FIRST, 
			              &status);
    }
    if (opt_upper) {
        ucol_setAttribute(collator, UCOL_CASE_FIRST, UCOL_UPPER_FIRST, 
			              &status);
    }
    if (opt_case) {
        ucol_setAttribute(collator, UCOL_CASE_LEVEL, UCOL_ON, &status);
    }
    if (opt_shifted) {
        ucol_setAttribute(collator, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, 
			              &status);
    }
    if (opt_level != 0) {
        switch (opt_level) {
        case 1:
            ucol_setAttribute(collator, UCOL_STRENGTH, UCOL_PRIMARY, &status);
            break;
        case 2:
            ucol_setAttribute(collator, UCOL_STRENGTH, UCOL_SECONDARY, 
				              &status);
            break;
        case 3:
            ucol_setAttribute(collator, UCOL_STRENGTH, UCOL_TERTIARY, &status);
            break;
        case 4:
            ucol_setAttribute(collator, UCOL_STRENGTH, UCOL_QUATERNARY, 
				              &status);
            break;
        case 5:
            ucol_setAttribute(collator, UCOL_STRENGTH, UCOL_IDENTICAL, 
				              &status);
            break;
        default:
            fprintf(stderr, "-level param must be between 1 and 5\n");
            return FALSE;
        }
    }
    if (U_FAILURE(status)) {
        fprintf(stderr, "Collator attribute setting failed.: %d\n", status);
        return FALSE;
    }
	return TRUE;
}

/** 
 * Main   --  process command line, read in and pre-process the test file,
 *            call other functions to do the actual tests.
 */
int main(int argc, const char** argv) 
{
    if (processOptions(argc, argv, opts) != TRUE || opt_help) {
        printf(gHelpString);
        return -1;
    }

    if (processCollator() != TRUE) {
		fprintf(stderr, "Error creating collator for comparison\n");
		return -1;
	}

	fprintf(stdout, "Comparing source=%s and target=%s\n", opt_source,
			opt_target);
	int result = strcmp();
	if (result == 0) {
		fprintf(stdout, "source is equals to target\n"); 
	}
	else if (result < 0) {
		fprintf(stdout, "source is less than target\n"); 
	}
	else {
		fprintf(stdout, "source is greater than target\n"); 
	}
	
	ucol_close(collator);
	return 0;
}
