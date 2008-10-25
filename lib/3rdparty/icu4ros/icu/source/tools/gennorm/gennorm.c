/*
*******************************************************************************
*
*   Copyright (C) 2001-2005, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  gennorm.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2001may25
*   created by: Markus W. Scherer
*
*   This program reads the Unicode character database text file,
*   parses it, and extracts the data for normalization.
*   It then preprocesses it and writes a binary file for efficient use
*   in various Unicode text normalization processes.
*/

#include <stdio.h>
#include <stdlib.h>
#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/putil.h"
#include "unicode/uclean.h"
#include "unicode/udata.h"
#include "unicode/uset.h"
#include "cmemory.h"
#include "cstring.h"
#include "unewdata.h"
#include "uoptions.h"
#include "uparse.h"
#include "unormimp.h"

U_CDECL_BEGIN
#include "gennorm.h"
U_CDECL_END

UBool beVerbose=FALSE, haveCopyright=TRUE;

/* prototypes --------------------------------------------------------------- */

static void
parseDerivedNormalizationProperties(const char *filename, UErrorCode *pErrorCode, UBool reportError);

static void
parseDB(const char *filename, UErrorCode *pErrorCode);

/* -------------------------------------------------------------------------- */

enum {
    HELP_H,
    HELP_QUESTION_MARK,
    VERBOSE,
    COPYRIGHT,
    DESTDIR,
    SOURCEDIR,
    UNICODE_VERSION,
    ICUDATADIR,
    CSOURCE,
    STORE_FLAGS
};

static UOption options[]={
    UOPTION_HELP_H,
    UOPTION_HELP_QUESTION_MARK,
    UOPTION_VERBOSE,
    UOPTION_COPYRIGHT,
    UOPTION_DESTDIR,
    UOPTION_SOURCEDIR,
    UOPTION_DEF("unicode", 'u', UOPT_REQUIRES_ARG),
    UOPTION_ICUDATADIR,
    UOPTION_DEF("csource", 'C', UOPT_NO_ARG),
    UOPTION_DEF("prune", 'p', UOPT_REQUIRES_ARG)
};

extern int
main(int argc, char* argv[]) {
#if !UCONFIG_NO_NORMALIZATION
    char filename[300];
#endif
    const char *srcDir=NULL, *destDir=NULL, *suffix=NULL;
    char *basename=NULL;
    UErrorCode errorCode=U_ZERO_ERROR;

    U_MAIN_INIT_ARGS(argc, argv);

    /* preset then read command line options */
    options[4].value=u_getDataDirectory();
    options[5].value="";
    options[6].value="3.0.0";
    options[ICUDATADIR].value=u_getDataDirectory();
    argc=u_parseArgs(argc, argv, sizeof(options)/sizeof(options[0]), options);

    /* error handling, printing usage message */
    if(argc<0) {
        fprintf(stderr,
            "error in command line argument \"%s\"\n",
            argv[-argc]);
    }
    if(argc<0 || options[0].doesOccur || options[1].doesOccur) {
        /*
         * Broken into chucks because the C89 standard says the minimum
         * required supported string length is 509 bytes.
         */
        fprintf(stderr,
            "Usage: %s [-options] [suffix]\n"
            "\n"
            "Read the UnicodeData.txt file and other Unicode properties files and\n"
            "create a binary file " U_ICUDATA_NAME "_" DATA_NAME "." DATA_TYPE " with the normalization data\n"
            "\n",
            argv[0]);
        fprintf(stderr,
            "Options:\n"
            "\t-h or -? or --help  this usage text\n"
            "\t-v or --verbose     verbose output\n"
            "\t-c or --copyright   include a copyright notice\n"
            "\t-u or --unicode     Unicode version, followed by the version like 3.0.0\n"
            "\t-C or --csource     generate a .c source file rather than the .icu binary\n");
        fprintf(stderr,
            "\t-p or --prune flags Prune for data modularization:\n"
            "\t                    Determine what data is to be stored.\n"
            "\t        0 (zero) stores minimal data (only for NFD)\n"
            "\t        lowercase letters turn off data, uppercase turn on (use with 0)\n");
        fprintf(stderr,
            "\t        k: compatibility decompositions (NFKC, NFKD)\n"
            "\t        c: composition data (NFC, NFKC)\n"
            "\t        f: FCD data (will be generated at load time)\n"
            "\t        a: auxiliary data (canonical closure etc.)\n"
            "\t        x: exclusion sets (Unicode 3.2-level normalization)\n");
        fprintf(stderr,
            "\t-d or --destdir     destination directory, followed by the path\n"
            "\t-s or --sourcedir   source directory, followed by the path\n"
            "\t-i or --icudatadir  directory for locating any needed intermediate data files,\n"
            "\t                    followed by path, defaults to <%s>\n"
            "\tsuffix              suffix that is to be appended with a '-'\n"
            "\t                    to the source file basenames before opening;\n"
            "\t                    'gennorm new' will read UnicodeData-new.txt etc.\n",
            u_getDataDirectory());
        return argc<0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
    }

    /* get the options values */
    beVerbose=options[2].doesOccur;
    haveCopyright=options[3].doesOccur;
    srcDir=options[5].value;
    destDir=options[4].value;

    if(argc>=2) {
        suffix=argv[1];
    } else {
        suffix=NULL;
    }

#if UCONFIG_NO_NORMALIZATION

    fprintf(stderr,
        "gennorm writes a dummy " U_ICUDATA_NAME "_" DATA_NAME "." DATA_TYPE
        " because UCONFIG_NO_NORMALIZATION is set, \n"
        "see icu/source/common/unicode/uconfig.h\n");
    generateData(destDir, options[CSOURCE].doesOccur);

#else

    setUnicodeVersion(options[6].value);

    if (options[ICUDATADIR].doesOccur) {
        u_setDataDirectory(options[ICUDATADIR].value);
    }

    if(options[STORE_FLAGS].doesOccur) {
        const char *s=options[STORE_FLAGS].value;
        char c;

        while((c=*s++)!=0) {
            switch(c) {
            case '0':
                gStoreFlags=0;  /* store minimal data (only for NFD) */
                break;

            /* lowercase letters: omit data */
            case 'k':
                gStoreFlags&=~U_MASK(UGENNORM_STORE_COMPAT);
                break;
            case 'c':
                gStoreFlags&=~U_MASK(UGENNORM_STORE_COMPOSITION);
                break;
            case 'f':
                gStoreFlags&=~U_MASK(UGENNORM_STORE_FCD);
                break;
            case 'a':
                gStoreFlags&=~U_MASK(UGENNORM_STORE_AUX);
                break;
            case 'x':
                gStoreFlags&=~U_MASK(UGENNORM_STORE_EXCLUSIONS);
                break;

            /* uppercase letters: include data (use with 0) */
            case 'K':
                gStoreFlags|=U_MASK(UGENNORM_STORE_COMPAT);
                break;
            case 'C':
                gStoreFlags|=U_MASK(UGENNORM_STORE_COMPOSITION);
                break;
            case 'F':
                gStoreFlags|=U_MASK(UGENNORM_STORE_FCD);
                break;
            case 'A':
                gStoreFlags|=U_MASK(UGENNORM_STORE_AUX);
                break;
            case 'X':
                gStoreFlags|=U_MASK(UGENNORM_STORE_EXCLUSIONS);
                break;

            default:
                fprintf(stderr, "ignoring undefined prune flag '%c'\n", c);
                break;
            }
        }
    }

    /*
     * Verify that we can work with properties
     * but don't call u_init() because that needs unorm.icu which we are just
     * going to build here.
     */
    {
        U_STRING_DECL(ideo, "[:Ideographic:]", 15);
        USet *set;

        U_STRING_INIT(ideo, "[:Ideographic:]", 15);
        set=uset_openPattern(ideo, -1, &errorCode);
        if(U_FAILURE(errorCode) || !uset_contains(set, 0xf900)) {
            fprintf(stderr, "gennorm is unable to work with properties (uprops.icu): %s\n", u_errorName(errorCode));
            exit(errorCode);
        }
        uset_close(set);
    }

    /* prepare the filename beginning with the source dir */
    uprv_strcpy(filename, srcDir);
    basename=filename+uprv_strlen(filename);
    if(basename>filename && *(basename-1)!=U_FILE_SEP_CHAR) {
        *basename++=U_FILE_SEP_CHAR;
    }

    /* initialize */
    init();

    /* process DerivedNormalizationProps.txt (name changed for Unicode 3.2, to <=31 characters) */
    if(suffix==NULL) {
        uprv_strcpy(basename, "DerivedNormalizationProps.txt");
    } else {
        uprv_strcpy(basename, "DerivedNormalizationProps");
        basename[30]='-';
        uprv_strcpy(basename+31, suffix);
        uprv_strcat(basename+31, ".txt");
    }
    parseDerivedNormalizationProperties(filename, &errorCode, FALSE);
    if(U_FAILURE(errorCode)) {
        /* can be only U_FILE_ACCESS_ERROR - try filename from before Unicode 3.2 */
        if(suffix==NULL) {
            uprv_strcpy(basename, "DerivedNormalizationProperties.txt");
        } else {
            uprv_strcpy(basename, "DerivedNormalizationProperties");
            basename[30]='-';
            uprv_strcpy(basename+31, suffix);
            uprv_strcat(basename+31, ".txt");
        }
        parseDerivedNormalizationProperties(filename, &errorCode, TRUE);
    }

    /* process UnicodeData.txt */
    if(suffix==NULL) {
        uprv_strcpy(basename, "UnicodeData.txt");
    } else {
        uprv_strcpy(basename, "UnicodeData");
        basename[11]='-';
        uprv_strcpy(basename+12, suffix);
        uprv_strcat(basename+12, ".txt");
    }
    parseDB(filename, &errorCode);

    /* process parsed data */
    if(U_SUCCESS(errorCode)) {
        processData();

        /* write the properties data file */
        generateData(destDir, options[CSOURCE].doesOccur);

        cleanUpData();
    }

#endif

    return errorCode;
}

#if !UCONFIG_NO_NORMALIZATION

/* parser for DerivedNormalizationProperties.txt ---------------------------- */

static void U_CALLCONV
derivedNormalizationPropertiesLineFn(void *context,
                                     char *fields[][2], int32_t fieldCount,
                                     UErrorCode *pErrorCode) {
    UChar string[32];
    char *s;
    uint32_t start, end;
    int32_t count;
    uint8_t qcFlags;

    /* get code point range */
    count=u_parseCodePointRange(fields[0][0], &start, &end, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "gennorm: error parsing DerivedNormalizationProperties.txt mapping at %s\n", fields[0][0]);
        exit(*pErrorCode);
    }

    /* ignore hangul - handle explicitly */
    if(start==0xac00) {
        return;
    }

    /* get property - ignore unrecognized ones */
    s=(char *)u_skipWhitespace(fields[1][0]);
    if(*s=='N' && s[1]=='F') {
        /* quick check flag */
        qcFlags=0x11;
        s+=2;
        if(*s=='K') {
            qcFlags<<=1;
            ++s;
        }

        if(*s=='C' && s[1]=='_') {
            s+=2;
        } else if(*s=='D' && s[1]=='_') {
            qcFlags<<=2;
            s+=2;
        } else {
            return;
        }

        if(0==uprv_strncmp(s, "NO", 2)) {
            qcFlags&=0xf;
        } else if(0==uprv_strncmp(s, "MAYBE", 5)) {
            qcFlags&=0x30;
        } else if(0==uprv_strncmp(s, "QC", 2) && *(s=(char *)u_skipWhitespace(s+2))==';') {
            /*
             * Unicode 4.0.1:
             * changes single field "NFD_NO" -> two fields "NFD_QC; N" etc.
             */
            /* start of the field */
            s=(char *)u_skipWhitespace(s+1);
            if(*s=='N') {
                qcFlags&=0xf;
            } else if(*s=='M') {
                qcFlags&=0x30;
            } else {
                return; /* do nothing for "Yes" because it's the default value */
            }
        } else {
            return; /* do nothing for "Yes" because it's the default value */
        }

        /* set this flag for all code points in this range */
        while(start<=end) {
            setQCFlags(start++, qcFlags);
        }
    } else if(0==uprv_memcmp(s, "Comp_Ex", 7) || 0==uprv_memcmp(s, "Full_Composition_Exclusion", 26)) {
        /* full composition exclusion */
        while(start<=end) {
            setCompositionExclusion(start++);
        }
    } else if(
        ((0==uprv_memcmp(s, "FNC", 3) && *(s=(char *)u_skipWhitespace(s+3))==';') || 
        (0==uprv_memcmp(s, "FC_NFKC", 7) && *(s=(char *)u_skipWhitespace(s+7))==';'))
        
    ) {
        /* FC_NFKC_Closure, parse field 2 to get the string */
        char *t;

        /* start of the field */
        s=(char *)u_skipWhitespace(s+1);

        /* find the end of the field */
        for(t=s; *t!=';' && *t!='#' && *t!=0 && *t!='\n' && *t!='\r'; ++t) {}
        *t=0;

        string[0]=(UChar)u_parseString(s, string+1, 31, NULL, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            fprintf(stderr, "gennorm error: illegal FNC string at %s\n", fields[0][0]);
            exit(*pErrorCode);
        }
        while(start<=end) {
            setFNC(start++, string);
        }
    }
}

static void
parseDerivedNormalizationProperties(const char *filename, UErrorCode *pErrorCode, UBool reportError) {
    char *fields[2][2];

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    u_parseDelimitedFile(filename, ';', fields, 2, derivedNormalizationPropertiesLineFn, NULL, pErrorCode);
    if(U_FAILURE(*pErrorCode) && (reportError || *pErrorCode!=U_FILE_ACCESS_ERROR)) {
        fprintf(stderr, "gennorm error: u_parseDelimitedFile(\"%s\") failed - %s\n", filename, u_errorName(*pErrorCode));
        exit(*pErrorCode);
    }
}

/* parser for UnicodeData.txt ----------------------------------------------- */

static void U_CALLCONV
unicodeDataLineFn(void *context,
                  char *fields[][2], int32_t fieldCount,
                  UErrorCode *pErrorCode) {
    uint32_t decomp[40];
    Norm norm;
    const char *s;
    char *end;
    uint32_t code, value;
    int32_t length;
    UBool isCompat, something=FALSE;

    /* ignore First and Last entries for ranges */
    if( *fields[1][0]=='<' &&
        (length=(int32_t)(fields[1][1]-fields[1][0]))>=9 &&
        (0==uprv_memcmp(", First>", fields[1][1]-8, 8) || 0==uprv_memcmp(", Last>", fields[1][1]-7, 7))
    ) {
        return;
    }

    /* reset the properties */
    uprv_memset(&norm, 0, sizeof(Norm));

    /*
     * The combiningIndex must not be initialized to 0 because 0 is the
     * combiningIndex of the first forward-combining character.
     */
    norm.combiningIndex=0xffff;

    /* get the character code, field 0 */
    code=(uint32_t)uprv_strtoul(fields[0][0], &end, 16);
    if(end<=fields[0][0] || end!=fields[0][1]) {
        fprintf(stderr, "gennorm: syntax error in field 0 at %s\n", fields[0][0]);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }

    /* get canonical combining class, field 3 */
    value=(uint32_t)uprv_strtoul(fields[3][0], &end, 10);
    if(end<=fields[3][0] || end!=fields[3][1] || value>0xff) {
        fprintf(stderr, "gennorm: syntax error in field 3 at %s\n", fields[0][0]);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }
    if(value>0) {
        norm.udataCC=(uint8_t)value;
        something=TRUE;
    }

    /* get the decomposition, field 5 */
    if(fields[5][0]<fields[5][1]) {
        if(*(s=fields[5][0])=='<') {
            ++s;
            isCompat=TRUE;

            /* skip and ignore the compatibility type name */
            do {
                if(s==fields[5][1]) {
                    /* missing '>' */
                    fprintf(stderr, "gennorm: syntax error in field 5 at %s\n", fields[0][0]);
                    *pErrorCode=U_PARSE_ERROR;
                    exit(U_PARSE_ERROR);
                }
            } while(*s++!='>');
        } else {
            isCompat=FALSE;
        }

        /* parse the decomposition string */
        length=u_parseCodePoints(s, decomp, sizeof(decomp)/4, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            fprintf(stderr, "gennorm error parsing UnicodeData.txt decomposition of U+%04lx - %s\n",
                    (long)code, u_errorName(*pErrorCode));
            exit(*pErrorCode);
        }

        /* store the string */
        if(length>0) {
            something=TRUE;
            if(isCompat) {
                norm.lenNFKD=(uint8_t)length;
                norm.nfkd=decomp;
            } else {
                if(length>2) {
                    fprintf(stderr, "gennorm: error - length of NFD(U+%04lx) = %ld >2 in UnicodeData - illegal\n",
                            (long)code, (long)length);
                    *pErrorCode=U_PARSE_ERROR;
                    exit(U_PARSE_ERROR);
                }
                norm.lenNFD=(uint8_t)length;
                norm.nfd=decomp;
            }
        }
    }

    /* check for non-character code points */
    if((code&0xfffe)==0xfffe || (uint32_t)(code-0xfdd0)<0x20 || code>0x10ffff) {
        fprintf(stderr, "gennorm: error - properties for non-character code point U+%04lx\n",
                (long)code);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }

    if(something) {
        /* there are normalization values, so store them */
#if 0
        if(beVerbose) {
            printf("store values for U+%04lx: cc=%d, lenNFD=%ld, lenNFKD=%ld\n",
                   (long)code, norm.udataCC, (long)norm.lenNFD, (long)norm.lenNFKD);
        }
#endif
        storeNorm(code, &norm);
    }
}

static void
parseDB(const char *filename, UErrorCode *pErrorCode) {
    char *fields[15][2];

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    u_parseDelimitedFile(filename, ';', fields, 15, unicodeDataLineFn, NULL, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "gennorm error: u_parseDelimitedFile(\"%s\") failed - %s\n", filename, u_errorName(*pErrorCode));
        exit(*pErrorCode);
    }
}

#endif /* #if !UCONFIG_NO_NORMALIZATION */

/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 *
 */
