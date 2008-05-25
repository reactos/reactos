/*
*******************************************************************************
*
*   Copyright (C) 2004-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  genbidi.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2004dec30
*   created by: Markus W. Scherer
*
*   This program reads several of the Unicode character database text files,
*   parses them, and extracts the bidi/shaping properties for each character.
*   It then writes a binary file containing the properties
*   that is designed to be used directly for random-access to
*   the properties of each Unicode character.
*/

#include <stdio.h>
#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/putil.h"
#include "unicode/uclean.h"
#include "cmemory.h"
#include "cstring.h"
#include "uarrsort.h"
#include "unewdata.h"
#include "uoptions.h"
#include "uparse.h"
#include "propsvec.h"
#include "ubidi_props.h"
#include "genbidi.h"

#define LENGTHOF(array) (sizeof(array)/sizeof((array)[0]))

/* data --------------------------------------------------------------------- */

uint32_t *pv;

UBool beVerbose=FALSE, haveCopyright=TRUE;

/* prototypes --------------------------------------------------------------- */

static UBool
isToken(const char *token, const char *s);

static void
parseBidiMirroring(const char *filename, UErrorCode *pErrorCode);

static void
parseDB(const char *filename, UErrorCode *pErrorCode);

/* miscellaneous ------------------------------------------------------------ */

/* TODO: more common code, move functions to uparse.h|c */

static char *
trimTerminateField(char *s, char *limit) {
    /* trim leading whitespace */
    s=(char *)u_skipWhitespace(s);

    /* trim trailing whitespace */
    while(s<limit && (*(limit-1)==' ' || *(limit-1)=='\t')) {
        --limit;
    }
    *limit=0;

    return s;
}

static void
parseTwoFieldFile(char *filename, char *basename,
                  const char *ucdFile, const char *suffix,
                  UParseLineFn *lineFn,
                  UErrorCode *pErrorCode) {
    char *fields[2][2];

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    writeUCDFilename(basename, ucdFile, suffix);

    u_parseDelimitedFile(filename, ';', fields, 2, lineFn, NULL, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "error parsing %s.txt: %s\n", ucdFile, u_errorName(*pErrorCode));
    }
}

static void U_CALLCONV
bidiClassLineFn(void *context,
                char *fields[][2], int32_t fieldCount,
                UErrorCode *pErrorCode);

/* parse files with single enumerated properties ---------------------------- */

/* TODO: more common code, move functions to uparse.h|c */

struct SingleEnum {
    const char *ucdFile, *propName;
    UProperty prop;
    int32_t vecWord, vecShift;
    uint32_t vecMask;
};
typedef struct SingleEnum SingleEnum;

static void
parseSingleEnumFile(char *filename, char *basename, const char *suffix,
                    const SingleEnum *sen,
                    UErrorCode *pErrorCode);

static const SingleEnum jtSingleEnum={
    "DerivedJoiningType", "joining type",
    UCHAR_JOINING_TYPE,
    0, UBIDI_JT_SHIFT, UBIDI_JT_MASK
};

static const SingleEnum jgSingleEnum={
    "DerivedJoiningGroup", "joining group",
    UCHAR_JOINING_GROUP,
    1, 0, 0xff                  /* column 1 bits 7..0 */
};

static void U_CALLCONV
singleEnumLineFn(void *context,
                 char *fields[][2], int32_t fieldCount,
                 UErrorCode *pErrorCode) {
    const SingleEnum *sen;
    char *s;
    uint32_t start, limit, uv;
    int32_t value;

    sen=(const SingleEnum *)context;

    u_parseCodePointRange(fields[0][0], &start, &limit, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "genbidi: syntax error in %s.txt field 0 at %s\n", sen->ucdFile, fields[0][0]);
        exit(*pErrorCode);
    }
    ++limit;

    /* parse property alias */
    s=trimTerminateField(fields[1][0], fields[1][1]);
    value=u_getPropertyValueEnum(sen->prop, s);
    if(value<0) {
        if(sen->prop==UCHAR_BLOCK) {
            if(isToken("Greek", s)) {
                value=UBLOCK_GREEK; /* Unicode 3.2 renames this to "Greek and Coptic" */
            } else if(isToken("Combining Marks for Symbols", s)) {
                value=UBLOCK_COMBINING_MARKS_FOR_SYMBOLS; /* Unicode 3.2 renames this to "Combining Diacritical Marks for Symbols" */
            } else if(isToken("Private Use", s)) {
                value=UBLOCK_PRIVATE_USE; /* Unicode 3.2 renames this to "Private Use Area" */
            }
        }
    }
    if(value<0) {
        fprintf(stderr, "genbidi error: unknown %s name in %s.txt field 1 at %s\n",
                        sen->propName, sen->ucdFile, s);
        exit(U_PARSE_ERROR);
    }

    uv=(uint32_t)(value<<sen->vecShift);
    if((uv&sen->vecMask)!=uv) {
        fprintf(stderr, "genbidi error: %s value overflow (0x%x) at %s\n",
                        sen->propName, (int)uv, s);
        exit(U_INTERNAL_PROGRAM_ERROR);
    }

    if(!upvec_setValue(pv, start, limit, sen->vecWord, uv, sen->vecMask, pErrorCode)) {
        fprintf(stderr, "genbidi error: unable to set %s code: %s\n",
                        sen->propName, u_errorName(*pErrorCode));
        exit(*pErrorCode);
    }
}

static void
parseSingleEnumFile(char *filename, char *basename, const char *suffix,
                    const SingleEnum *sen,
                    UErrorCode *pErrorCode) {
    char *fields[2][2];

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    writeUCDFilename(basename, sen->ucdFile, suffix);

    u_parseDelimitedFile(filename, ';', fields, 2, singleEnumLineFn, (void *)sen, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "error parsing %s.txt: %s\n", sen->ucdFile, u_errorName(*pErrorCode));
    }
}

/* parse files with multiple binary properties ------------------------------ */

/* TODO: more common code, move functions to uparse.h|c */

/* TODO: similar to genbidi/props2.c but not the same; same as in gencase/gencase.c */

struct Binary {
    const char *propName;
    int32_t vecWord;
    uint32_t vecValue, vecMask;
};
typedef struct Binary Binary;

struct Binaries {
    const char *ucdFile;
    const Binary *binaries;
    int32_t binariesCount;
};
typedef struct Binaries Binaries;

static const Binary
propListNames[]={
    { "Bidi_Control",                       0, U_MASK(UBIDI_BIDI_CONTROL_SHIFT), U_MASK(UBIDI_BIDI_CONTROL_SHIFT) },
    { "Join_Control",                       0, U_MASK(UBIDI_JOIN_CONTROL_SHIFT), U_MASK(UBIDI_JOIN_CONTROL_SHIFT) }
};

static const Binaries
propListBinaries={
    "PropList", propListNames, LENGTHOF(propListNames)
};

static void U_CALLCONV
binariesLineFn(void *context,
               char *fields[][2], int32_t fieldCount,
               UErrorCode *pErrorCode) {
    const Binaries *bin;
    char *s;
    uint32_t start, limit;
    int32_t i;

    bin=(const Binaries *)context;

    u_parseCodePointRange(fields[0][0], &start, &limit, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "genbidi: syntax error in %s.txt field 0 at %s\n", bin->ucdFile, fields[0][0]);
        exit(*pErrorCode);
    }
    ++limit;

    /* parse binary property name */
    s=(char *)u_skipWhitespace(fields[1][0]);
    for(i=0;; ++i) {
        if(i==bin->binariesCount) {
            /* ignore unrecognized properties */
            return;
        }
        if(isToken(bin->binaries[i].propName, s)) {
            break;
        }
    }

    if(bin->binaries[i].vecMask==0) {
        fprintf(stderr, "genbidi error: mask value %d==0 for %s %s\n",
                        (int)bin->binaries[i].vecMask, bin->ucdFile, bin->binaries[i].propName);
        exit(U_INTERNAL_PROGRAM_ERROR);
    }

    if(!upvec_setValue(pv, start, limit, bin->binaries[i].vecWord, bin->binaries[i].vecValue, bin->binaries[i].vecMask, pErrorCode)) {
        fprintf(stderr, "genbidi error: unable to set %s, code: %s\n",
                        bin->binaries[i].propName, u_errorName(*pErrorCode));
        exit(*pErrorCode);
    }
}

static void
parseBinariesFile(char *filename, char *basename, const char *suffix,
                  const Binaries *bin,
                  UErrorCode *pErrorCode) {
    char *fields[2][2];

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    writeUCDFilename(basename, bin->ucdFile, suffix);

    u_parseDelimitedFile(filename, ';', fields, 2, binariesLineFn, (void *)bin, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "error parsing %s.txt: %s\n", bin->ucdFile, u_errorName(*pErrorCode));
    }
}

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
    CSOURCE
};

/* Keep these values in sync with the above enums */
static UOption options[]={
    UOPTION_HELP_H,
    UOPTION_HELP_QUESTION_MARK,
    UOPTION_VERBOSE,
    UOPTION_COPYRIGHT,
    UOPTION_DESTDIR,
    UOPTION_SOURCEDIR,
    UOPTION_DEF("unicode", 'u', UOPT_REQUIRES_ARG),
    UOPTION_ICUDATADIR,
    UOPTION_DEF("csource", 'C', UOPT_NO_ARG)
};

extern int
main(int argc, char* argv[]) {
    char filename[300];
    const char *srcDir=NULL, *destDir=NULL, *suffix=NULL;
    char *basename=NULL;
    UErrorCode errorCode=U_ZERO_ERROR;

    U_MAIN_INIT_ARGS(argc, argv);

    /* preset then read command line options */
    options[DESTDIR].value=u_getDataDirectory();
    options[SOURCEDIR].value="";
    options[UNICODE_VERSION].value="";
    options[ICUDATADIR].value=u_getDataDirectory();
    argc=u_parseArgs(argc, argv, sizeof(options)/sizeof(options[0]), options);

    /* error handling, printing usage message */
    if(argc<0) {
        fprintf(stderr,
            "error in command line argument \"%s\"\n",
            argv[-argc]);
    }
    if(argc<0 || options[HELP_H].doesOccur || options[HELP_QUESTION_MARK].doesOccur) {
        /*
         * Broken into chucks because the C89 standard says the minimum
         * required supported string length is 509 bytes.
         */
        fprintf(stderr,
            "Usage: %s [-options] [suffix]\n"
            "\n"
            "read the UnicodeData.txt file and other Unicode properties files and\n"
            "create a binary file " UBIDI_DATA_NAME "." UBIDI_DATA_TYPE " with the bidi/shaping properties\n"
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
            "\t-d or --destdir     destination directory, followed by the path\n"
            "\t-s or --sourcedir   source directory, followed by the path\n"
            "\t-i or --icudatadir  directory for locating any needed intermediate data files,\n"
            "\t                    followed by path, defaults to %s\n"
            "\tsuffix              suffix that is to be appended with a '-'\n"
            "\t                    to the source file basenames before opening;\n"
            "\t                    'genbidi new' will read UnicodeData-new.txt etc.\n",
            u_getDataDirectory());
        return argc<0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
    }

    /* get the options values */
    beVerbose=options[VERBOSE].doesOccur;
    haveCopyright=options[COPYRIGHT].doesOccur;
    srcDir=options[SOURCEDIR].value;
    destDir=options[DESTDIR].value;

    if(argc>=2) {
        suffix=argv[1];
    } else {
        suffix=NULL;
    }

    if(options[UNICODE_VERSION].doesOccur) {
        setUnicodeVersion(options[UNICODE_VERSION].value);
    }
    /* else use the default dataVersion in store.c */

    if (options[ICUDATADIR].doesOccur) {
        u_setDataDirectory(options[ICUDATADIR].value);
    }

    /* prepare the filename beginning with the source dir */
    uprv_strcpy(filename, srcDir);
    basename=filename+uprv_strlen(filename);
    if(basename>filename && *(basename-1)!=U_FILE_SEP_CHAR) {
        *basename++=U_FILE_SEP_CHAR;
    }

    /* initialize */
    pv=upvec_open(2, 10000);

    /* process BidiMirroring.txt */
    writeUCDFilename(basename, "BidiMirroring", suffix);
    parseBidiMirroring(filename, &errorCode);

    /* process additional properties files */
    *basename=0;

    parseBinariesFile(filename, basename, suffix, &propListBinaries, &errorCode);

    parseSingleEnumFile(filename, basename, suffix, &jtSingleEnum, &errorCode);

    parseSingleEnumFile(filename, basename, suffix, &jgSingleEnum, &errorCode);

    /* process UnicodeData.txt */
    writeUCDFilename(basename, "UnicodeData", suffix);
    parseDB(filename, &errorCode);

    /* set proper bidi class for unassigned code points (Cn) */
    parseTwoFieldFile(filename, basename, "DerivedBidiClass", suffix, bidiClassLineFn, &errorCode);

    /* process parsed data */
    if(U_SUCCESS(errorCode)) {
        /* write the properties data file */
        generateData(destDir, options[CSOURCE].doesOccur);
    }

    u_cleanup();
    return errorCode;
}

U_CFUNC void
writeUCDFilename(char *basename, const char *filename, const char *suffix) {
    int32_t length=(int32_t)uprv_strlen(filename);
    uprv_strcpy(basename, filename);
    if(suffix!=NULL) {
        basename[length++]='-';
        uprv_strcpy(basename+length, suffix);
        length+=(int32_t)uprv_strlen(suffix);
    }
    uprv_strcpy(basename+length, ".txt");
}

/* TODO: move to toolutil */
static UBool
isToken(const char *token, const char *s) {
    const char *z;
    int32_t j;

    s=u_skipWhitespace(s);
    for(j=0;; ++j) {
        if(token[j]!=0) {
            if(s[j]!=token[j]) {
                break;
            }
        } else {
            z=u_skipWhitespace(s+j);
            if(*z==';' || *z==0) {
                return TRUE;
            } else {
                break;
            }
        }
    }

    return FALSE;
}

/* parser for BidiMirroring.txt --------------------------------------------- */

static void U_CALLCONV
mirrorLineFn(void *context,
             char *fields[][2], int32_t fieldCount,
             UErrorCode *pErrorCode) {
    char *end;
    UChar32 src, mirror;

    src=(UChar32)uprv_strtoul(fields[0][0], &end, 16);
    if(end<=fields[0][0] || end!=fields[0][1]) {
        fprintf(stderr, "genbidi: syntax error in BidiMirroring.txt field 0 at %s\n", fields[0][0]);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }

    mirror=(UChar32)uprv_strtoul(fields[1][0], &end, 16);
    if(end<=fields[1][0] || end!=fields[1][1]) {
        fprintf(stderr, "genbidi: syntax error in BidiMirroring.txt field 1 at %s\n", fields[1][0]);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }

    addMirror(src, mirror);
}

static void
parseBidiMirroring(const char *filename, UErrorCode *pErrorCode) {
    char *fields[2][2];

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    u_parseDelimitedFile(filename, ';', fields, 2, mirrorLineFn, NULL, pErrorCode);
}

/* parser for UnicodeData.txt ----------------------------------------------- */

static void U_CALLCONV
unicodeDataLineFn(void *context,
                  char *fields[][2], int32_t fieldCount,
                  UErrorCode *pErrorCode) {
    char *end;
    UErrorCode errorCode;
    UChar32 c;

    errorCode=U_ZERO_ERROR;

    /* get the character code, field 0 */
    c=(UChar32)uprv_strtoul(fields[0][0], &end, 16);
    if(end<=fields[0][0] || end!=fields[0][1]) {
        fprintf(stderr, "genbidi: syntax error in field 0 at %s\n", fields[0][0]);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }

    /* get Mirrored flag, field 9 */
    if(*fields[9][0]=='Y') {
        if(!upvec_setValue(pv, c, c+1, 0, U_MASK(UBIDI_IS_MIRRORED_SHIFT), U_MASK(UBIDI_IS_MIRRORED_SHIFT), &errorCode)) {
            fprintf(stderr, "genbidi error: unable to set 'is mirrored' for U+%04lx, code: %s\n",
                            (long)c, u_errorName(errorCode));
            exit(errorCode);
        }
    } else if(fields[9][1]-fields[9][0]!=1 || *fields[9][0]!='N') {
        fprintf(stderr, "genbidi: syntax error in field 9 at U+%04lx\n",
            (long)c);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }
}

static void
parseDB(const char *filename, UErrorCode *pErrorCode) {
    /* default Bidi classes for unassigned code points */
    static const UChar32 defaultBidi[][3]={ /* { start, end, class } */
        /* R: U+0590..U+05FF, U+07C0..U+08FF, U+FB1D..U+FB4F, U+10800..U+10FFF */
        { 0x0590, 0x05FF, U_RIGHT_TO_LEFT },
        { 0x07C0, 0x08FF, U_RIGHT_TO_LEFT },
        { 0xFB1D, 0xFB4F, U_RIGHT_TO_LEFT },
        { 0x10800, 0x10FFF, U_RIGHT_TO_LEFT },

        /* AL: U+0600..U+07BF, U+FB50..U+FDCF, U+FDF0..U+FDFF, U+FE70..U+FEFE */
        { 0x0600, 0x07BF, U_RIGHT_TO_LEFT_ARABIC },
        { 0xFB50, 0xFDCF, U_RIGHT_TO_LEFT_ARABIC },
        { 0xFDF0, 0xFDFF, U_RIGHT_TO_LEFT_ARABIC },
        { 0xFE70, 0xFEFE, U_RIGHT_TO_LEFT_ARABIC }

        /* L otherwise */
    };

    char *fields[15][2];
    UChar32 start, end;
    int32_t i;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    /*
     * Set default Bidi classes for unassigned code points.
     * See the documentation for Bidi_Class in UCD.html in the Unicode data.
     * http://www.unicode.org/Public/
     *
     * Starting with Unicode 5.0, DerivedBidiClass.txt should (re)set
     * the Bidi_Class values for all code points including unassigned ones
     * and including L values for these.
     * This code becomes unnecesary but harmless. Leave it for now in case
     * someone uses genbidi on pre-Unicode 5.0 data.
     */
    for(i=0; i<LENGTHOF(defaultBidi); ++i) {
        start=defaultBidi[i][0];
        end=defaultBidi[i][1];
        if(!upvec_setValue(pv, start, end+1, 0, (uint32_t)defaultBidi[i][2], UBIDI_CLASS_MASK, pErrorCode)) {
            fprintf(stderr, "genbidi error: unable to set default bidi class for U+%04lx..U+%04lx, code: %s\n",
                            (long)start, (long)end, u_errorName(*pErrorCode));
            exit(*pErrorCode);
        }
    }

    u_parseDelimitedFile(filename, ';', fields, 15, unicodeDataLineFn, NULL, pErrorCode);

    if(U_FAILURE(*pErrorCode)) {
        return;
    }
}

/* DerivedBidiClass.txt ----------------------------------------------------- */

static void U_CALLCONV
bidiClassLineFn(void *context,
                char *fields[][2], int32_t fieldCount,
                UErrorCode *pErrorCode) {
    char *s;
    uint32_t start, limit, value;

    /* get the code point range */
    u_parseCodePointRange(fields[0][0], &start, &limit, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "genbidi: syntax error in DerivedBidiClass.txt field 0 at %s\n", fields[0][0]);
        exit(*pErrorCode);
    }
    ++limit;

    /* parse bidi class */
    s=trimTerminateField(fields[1][0], fields[1][1]);
    value=u_getPropertyValueEnum(UCHAR_BIDI_CLASS, s);
    if((int32_t)value<0) {
        fprintf(stderr, "genbidi error: unknown bidi class in DerivedBidiClass.txt field 1 at %s\n", s);
        exit(U_PARSE_ERROR);
    }

    if(!upvec_setValue(pv, start, limit, 0, value, UBIDI_CLASS_MASK, pErrorCode)) {
        fprintf(stderr, "genbidi error: unable to set derived bidi class for U+%04x..U+%04x - %s\n",
                (int)start, (int)limit-1, u_errorName(*pErrorCode));
        exit(*pErrorCode);
    }
}

/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 *
 */
