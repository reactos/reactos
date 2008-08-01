/*
*******************************************************************************
*
*   Copyright (C) 2004-2005, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  gencase.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2004aug28
*   created by: Markus W. Scherer
*
*   This program reads several of the Unicode character database text files,
*   parses them, and the case mapping properties for each character.
*   It then writes a binary file containing the properties
*   that is designed to be used directly for random-access to
*   the properties of each Unicode character.
*/

#include <stdio.h>
#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/uset.h"
#include "unicode/putil.h"
#include "unicode/uclean.h"
#include "cmemory.h"
#include "cstring.h"
#include "uarrsort.h"
#include "unewdata.h"
#include "uoptions.h"
#include "uparse.h"
#include "uprops.h"
#include "propsvec.h"
#include "gencase.h"

#define LENGTHOF(array) (sizeof(array)/sizeof((array)[0]))

/* data --------------------------------------------------------------------- */

uint32_t *pv;

UBool beVerbose=FALSE, haveCopyright=TRUE;

/*
 * Unicode set collecting the case-sensitive characters;
 * see uchar.h UCHAR_CASE_SENSITIVE.
 * Add code points from case mappings/foldings in
 * the root locale and with default options.
 */
static USet *caseSensitive;

/* prototypes --------------------------------------------------------------- */

static void
parseSpecialCasing(const char *filename, UErrorCode *pErrorCode);

static void
parseCaseFolding(const char *filename, UErrorCode *pErrorCode);

static void
parseDB(const char *filename, UErrorCode *pErrorCode);

/* parse files with multiple binary properties ------------------------------ */

/* TODO: more common code, move functions to uparse.h|c */

/* TODO: similar to genprops/props2.c but not the same */

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
    { "Soft_Dotted",                        0, UCASE_SOFT_DOTTED,   UCASE_DOT_MASK }
};

static const Binaries
propListBinaries={
    "PropList", propListNames, LENGTHOF(propListNames)
};

static const Binary
derCorePropsNames[]={
    { "Lowercase",                          0, UCASE_LOWER,         UCASE_TYPE_MASK },
    { "Uppercase",                          0, UCASE_UPPER,         UCASE_TYPE_MASK }
};

static const Binaries
derCorePropsBinaries={
    "DerivedCoreProperties", derCorePropsNames, LENGTHOF(derCorePropsNames)
};

/* treat Word_Break=MidLetter as a binary property (we ignore all other Word_Break values) */
static const Binary
wordBreakNames[]={
    { "MidLetter",                          1, U_MASK(UGENCASE_IS_MID_LETTER_SHIFT), U_MASK(UGENCASE_IS_MID_LETTER_SHIFT) }
};

static const Binaries
wordBreakBinaries={
    "WordBreakProperty", wordBreakNames, LENGTHOF(wordBreakNames)
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
        fprintf(stderr, "gencase: syntax error in %s.txt field 0 at %s\n", bin->ucdFile, fields[0][0]);
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
        fprintf(stderr, "gencase error: mask value %d==0 for %s %s\n",
                        (int)bin->binaries[i].vecMask, bin->ucdFile, bin->binaries[i].propName);
        exit(U_INTERNAL_PROGRAM_ERROR);
    }

    if(!upvec_setValue(pv, start, limit, bin->binaries[i].vecWord, bin->binaries[i].vecValue, bin->binaries[i].vecMask, pErrorCode)) {
        fprintf(stderr, "gencase error: unable to set %s, code: %s\n",
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

enum
{
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
            "create a binary file " UCASE_DATA_NAME "." UCASE_DATA_TYPE " with the case mapping properties\n"
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
            "\t                    'gencase new' will read UnicodeData-new.txt etc.\n",
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
    caseSensitive=uset_open(1, 0); /* empty set (start>end) */

    /* process SpecialCasing.txt */
    writeUCDFilename(basename, "SpecialCasing", suffix);
    parseSpecialCasing(filename, &errorCode);

    /* process CaseFolding.txt */
    writeUCDFilename(basename, "CaseFolding", suffix);
    parseCaseFolding(filename, &errorCode);

    /* process additional properties files */
    *basename=0;

    parseBinariesFile(filename, basename, suffix, &propListBinaries, &errorCode);

    parseBinariesFile(filename, basename, suffix, &derCorePropsBinaries, &errorCode);

    if(ucdVersion>=UNI_4_1) {
        parseBinariesFile(filename, basename, suffix, &wordBreakBinaries, &errorCode);
    }

    /* process UnicodeData.txt */
    writeUCDFilename(basename, "UnicodeData", suffix);
    parseDB(filename, &errorCode);

    /* process parsed data */
    makeCaseClosure();

    makeExceptions();

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
U_CFUNC UBool
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

static int32_t
getTokenIndex(const char *const tokens[], int32_t countTokens, const char *s) {
    const char *t, *z;
    int32_t i, j;

    s=u_skipWhitespace(s);
    for(i=0; i<countTokens; ++i) {
        t=tokens[i];
        if(t!=NULL) {
            for(j=0;; ++j) {
                if(t[j]!=0) {
                    if(s[j]!=t[j]) {
                        break;
                    }
                } else {
                    z=u_skipWhitespace(s+j);
                    if(*z==';' || *z==0 || *z=='#' || *z=='\r' || *z=='\n') {
                        return i;
                    } else {
                        break;
                    }
                }
            }
        }
    }
    return -1;
}

static void
_set_addAll(USet *set, const UChar *s, int32_t length) {
    UChar32 c;
    int32_t i;

    /* needs length>=0 */
    for(i=0; i<length; /* U16_NEXT advances i */) {
        U16_NEXT(s, i, length, c);
        uset_add(set, c);
    }
}

/* parser for SpecialCasing.txt --------------------------------------------- */

#define MAX_SPECIAL_CASING_COUNT 500

static SpecialCasing specialCasings[MAX_SPECIAL_CASING_COUNT];
static int32_t specialCasingCount=0;

static void U_CALLCONV
specialCasingLineFn(void *context,
                    char *fields[][2], int32_t fieldCount,
                    UErrorCode *pErrorCode) {
    char *end;

    /* get code point */
    specialCasings[specialCasingCount].code=(UChar32)uprv_strtoul(u_skipWhitespace(fields[0][0]), &end, 16);
    end=(char *)u_skipWhitespace(end);
    if(end<=fields[0][0] || end!=fields[0][1]) {
        fprintf(stderr, "gencase: syntax error in SpecialCasing.txt field 0 at %s\n", fields[0][0]);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }

    /* is this a complex mapping? */
    if(*(end=(char *)u_skipWhitespace(fields[4][0]))!=0 && *end!=';' && *end!='#') {
        /* there is some condition text in the fifth field */
        specialCasings[specialCasingCount].isComplex=TRUE;

        /* do not store any actual mappings for this */
        specialCasings[specialCasingCount].lowerCase[0]=0;
        specialCasings[specialCasingCount].upperCase[0]=0;
        specialCasings[specialCasingCount].titleCase[0]=0;
    } else {
        /* just set the "complex" flag and get the case mappings */
        specialCasings[specialCasingCount].isComplex=FALSE;
        specialCasings[specialCasingCount].lowerCase[0]=
            (UChar)u_parseString(fields[1][0], specialCasings[specialCasingCount].lowerCase+1, 31, NULL, pErrorCode);
        specialCasings[specialCasingCount].upperCase[0]=
            (UChar)u_parseString(fields[3][0], specialCasings[specialCasingCount].upperCase+1, 31, NULL, pErrorCode);
        specialCasings[specialCasingCount].titleCase[0]=
            (UChar)u_parseString(fields[2][0], specialCasings[specialCasingCount].titleCase+1, 31, NULL, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            fprintf(stderr, "gencase: error parsing special casing at %s\n", fields[0][0]);
            exit(*pErrorCode);
        }

        uset_add(caseSensitive, (UChar32)specialCasings[specialCasingCount].code);
        _set_addAll(caseSensitive, specialCasings[specialCasingCount].lowerCase+1, specialCasings[specialCasingCount].lowerCase[0]);
        _set_addAll(caseSensitive, specialCasings[specialCasingCount].upperCase+1, specialCasings[specialCasingCount].upperCase[0]);
        _set_addAll(caseSensitive, specialCasings[specialCasingCount].titleCase+1, specialCasings[specialCasingCount].titleCase[0]);
    }

    if(++specialCasingCount==MAX_SPECIAL_CASING_COUNT) {
        fprintf(stderr, "gencase: too many special casing mappings\n");
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        exit(U_INDEX_OUTOFBOUNDS_ERROR);
    }
}

static int32_t U_CALLCONV
compareSpecialCasings(const void *context, const void *left, const void *right) {
    return ((const SpecialCasing *)left)->code-((const SpecialCasing *)right)->code;
}

static void
parseSpecialCasing(const char *filename, UErrorCode *pErrorCode) {
    char *fields[5][2];
    int32_t i, j;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    u_parseDelimitedFile(filename, ';', fields, 5, specialCasingLineFn, NULL, pErrorCode);

    /* sort the special casing entries by code point */
    if(specialCasingCount>0) {
        uprv_sortArray(specialCasings, specialCasingCount, sizeof(SpecialCasing),
                       compareSpecialCasings, NULL, FALSE, pErrorCode);
    }
    if(U_FAILURE(*pErrorCode)) {
        return;
    }

    /* replace multiple entries for any code point by one "complex" one */
    j=0;
    for(i=1; i<specialCasingCount; ++i) {
        if(specialCasings[i-1].code==specialCasings[i].code) {
            /* there is a duplicate code point */
            specialCasings[i-1].code=0x7fffffff;    /* remove this entry in the following sorting */
            specialCasings[i].isComplex=TRUE;       /* make the following one complex */
            specialCasings[i].lowerCase[0]=0;
            specialCasings[i].upperCase[0]=0;
            specialCasings[i].titleCase[0]=0;
            ++j;
        }
    }

    /* if some entries just were removed, then re-sort */
    if(j>0) {
        uprv_sortArray(specialCasings, specialCasingCount, sizeof(SpecialCasing),
                       compareSpecialCasings, NULL, FALSE, pErrorCode);
        specialCasingCount-=j;
    }
    if(U_FAILURE(*pErrorCode)) {
        return;
    }

    /*
     * Add one complex mapping to caseSensitive that was filtered out above:
     * Greek final Sigma has a conditional mapping but not locale-sensitive,
     * and it is taken when lowercasing just U+03A3 alone.
     * 03A3; 03C2; 03A3; 03A3; Final_Sigma; # GREEK CAPITAL LETTER SIGMA
     */
    uset_add(caseSensitive, 0x3c2);
}

/* parser for CaseFolding.txt ----------------------------------------------- */

#define MAX_CASE_FOLDING_COUNT 2000

static CaseFolding caseFoldings[MAX_CASE_FOLDING_COUNT];
static int32_t caseFoldingCount=0;

static void U_CALLCONV
caseFoldingLineFn(void *context,
                  char *fields[][2], int32_t fieldCount,
                  UErrorCode *pErrorCode) {
    char *end;
    static UChar32 prevCode=0;
    int32_t count;
    char status;

    /* get code point */
    caseFoldings[caseFoldingCount].code=(UChar32)uprv_strtoul(u_skipWhitespace(fields[0][0]), &end, 16);
    end=(char *)u_skipWhitespace(end);
    if(end<=fields[0][0] || end!=fields[0][1]) {
        fprintf(stderr, "gencase: syntax error in CaseFolding.txt field 0 at %s\n", fields[0][0]);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }

    /* get the status of this mapping */
    caseFoldings[caseFoldingCount].status=status=*u_skipWhitespace(fields[1][0]);
    if(status!='L' && status!='E' && status!='C' && status!='S' && status!='F' && status!='I' && status!='T') {
        fprintf(stderr, "gencase: unrecognized status field in CaseFolding.txt at %s\n", fields[0][0]);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }

    /* ignore all case folding mappings that are the same as the UnicodeData.txt lowercase mappings */
    if(status=='L') {
        return;
    }

    /* get the mapping */
    count=caseFoldings[caseFoldingCount].full[0]=
        (UChar)u_parseString(fields[2][0], caseFoldings[caseFoldingCount].full+1, 31, (uint32_t *)&caseFoldings[caseFoldingCount].simple, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "gencase: error parsing CaseFolding.txt mapping at %s\n", fields[0][0]);
        exit(*pErrorCode);
    }

    /* there is a simple mapping only if there is exactly one code point (count is in UChars) */
    if(count==0 || count>2 || (count==2 && UTF_IS_SINGLE(caseFoldings[caseFoldingCount].full[1]))) {
        caseFoldings[caseFoldingCount].simple=0;
    }

    /* update the case-sensitive set */
    if(status!='T') {
        uset_add(caseSensitive, (UChar32)caseFoldings[caseFoldingCount].code);
        _set_addAll(caseSensitive, caseFoldings[caseFoldingCount].full+1, caseFoldings[caseFoldingCount].full[0]);
    }

    /* check the status */
    if(status=='S') {
        /* check if there was a full mapping for this code point before */
        if( caseFoldingCount>0 &&
            caseFoldings[caseFoldingCount-1].code==caseFoldings[caseFoldingCount].code &&
            caseFoldings[caseFoldingCount-1].status=='F'
        ) {
            /* merge the two entries */
            caseFoldings[caseFoldingCount-1].simple=caseFoldings[caseFoldingCount].simple;
            return;
        }
    } else if(status=='F') {
        /* check if there was a simple mapping for this code point before */
        if( caseFoldingCount>0 &&
            caseFoldings[caseFoldingCount-1].code==caseFoldings[caseFoldingCount].code &&
            caseFoldings[caseFoldingCount-1].status=='S'
        ) {
            /* merge the two entries */
            uprv_memcpy(caseFoldings[caseFoldingCount-1].full, caseFoldings[caseFoldingCount].full, 32*U_SIZEOF_UCHAR);
            return;
        }
    } else if(status=='I' || status=='T') {
        /* check if there was a default mapping for this code point before (remove it) */
        while(caseFoldingCount>0 &&
              caseFoldings[caseFoldingCount-1].code==caseFoldings[caseFoldingCount].code
        ) {
            prevCode=0;
            --caseFoldingCount;
        }
        /* store only a marker for special handling for cases like dotless i */
        caseFoldings[caseFoldingCount].simple=0;
        caseFoldings[caseFoldingCount].full[0]=0;
    }

    /* check that the code points (caseFoldings[caseFoldingCount].code) are in ascending order */
    if(caseFoldings[caseFoldingCount].code<=prevCode && caseFoldings[caseFoldingCount].code>0) {
        fprintf(stderr, "gencase: error - CaseFolding entries out of order, U+%04lx after U+%04lx\n",
                (unsigned long)caseFoldings[caseFoldingCount].code,
                (unsigned long)prevCode);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }
    prevCode=caseFoldings[caseFoldingCount].code;

    if(++caseFoldingCount==MAX_CASE_FOLDING_COUNT) {
        fprintf(stderr, "gencase: too many case folding mappings\n");
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        exit(U_INDEX_OUTOFBOUNDS_ERROR);
    }
}

static void
parseCaseFolding(const char *filename, UErrorCode *pErrorCode) {
    char *fields[3][2];

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    u_parseDelimitedFile(filename, ';', fields, 3, caseFoldingLineFn, NULL, pErrorCode);
}

/* parser for UnicodeData.txt ----------------------------------------------- */

/* general categories */
const char *const
genCategoryNames[U_CHAR_CATEGORY_COUNT]={
    "Cn",
    "Lu", "Ll", "Lt", "Lm", "Lo", "Mn", "Me",
    "Mc", "Nd", "Nl", "No",
    "Zs", "Zl", "Zp",
    "Cc", "Cf", "Co", "Cs",
    "Pd", "Ps", "Pe", "Pc", "Po",
    "Sm", "Sc", "Sk", "So",
    "Pi", "Pf"
};

static int32_t specialCasingIndex=0, caseFoldingIndex=0;

static void U_CALLCONV
unicodeDataLineFn(void *context,
                  char *fields[][2], int32_t fieldCount,
                  UErrorCode *pErrorCode) {
    Props p;
    char *end;
    static UChar32 prevCode=0;
    UChar32 value;
    int32_t i;

    /* reset the properties */
    uprv_memset(&p, 0, sizeof(Props));

    /* get the character code, field 0 */
    p.code=(UChar32)uprv_strtoul(fields[0][0], &end, 16);
    if(end<=fields[0][0] || end!=fields[0][1]) {
        fprintf(stderr, "gencase: syntax error in field 0 at %s\n", fields[0][0]);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }

    /* get general category, field 2 */
    i=getTokenIndex(genCategoryNames, U_CHAR_CATEGORY_COUNT, fields[2][0]);
    if(i>=0) {
        p.gc=(uint8_t)i;
    } else {
        fprintf(stderr, "gencase: unknown general category \"%s\" at code 0x%lx\n",
            fields[2][0], (unsigned long)p.code);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }

    /* get canonical combining class, field 3 */
    value=(UChar32)uprv_strtoul(fields[3][0], &end, 10);
    if(end<=fields[3][0] || end!=fields[3][1] || value>0xff) {
        fprintf(stderr, "gencase: syntax error in field 3 at %s\n", fields[0][0]);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }
    p.cc=(uint8_t)value;

    /* get uppercase mapping, field 12 */
    value=(UChar32)uprv_strtoul(fields[12][0], &end, 16);
    if(end!=fields[12][1]) {
        fprintf(stderr, "gencase: syntax error in field 12 at code 0x%lx\n",
            (unsigned long)p.code);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }
    if(value!=0 && value!=p.code) {
        p.upperCase=value;
        uset_add(caseSensitive, p.code);
        uset_add(caseSensitive, value);
    }

    /* get lowercase value, field 13 */
    value=(UChar32)uprv_strtoul(fields[13][0], &end, 16);
    if(end!=fields[13][1]) {
        fprintf(stderr, "gencase: syntax error in field 13 at code 0x%lx\n",
            (unsigned long)p.code);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }
    if(value!=0 && value!=p.code) {
        p.lowerCase=value;
        uset_add(caseSensitive, p.code);
        uset_add(caseSensitive, value);
    }

    /* get titlecase value, field 14 */
    value=(UChar32)uprv_strtoul(fields[14][0], &end, 16);
    if(end!=fields[14][1]) {
        fprintf(stderr, "gencase: syntax error in field 14 at code 0x%lx\n",
            (unsigned long)p.code);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }
    if(value!=0 && value!=p.code) {
        p.titleCase=value;
        uset_add(caseSensitive, p.code);
        uset_add(caseSensitive, value);
    }

    /* set additional properties from previously parsed files */
    if(specialCasingIndex<specialCasingCount && p.code==specialCasings[specialCasingIndex].code) {
        p.specialCasing=specialCasings+specialCasingIndex++;
    } else {
        p.specialCasing=NULL;
    }
    if(caseFoldingIndex<caseFoldingCount && p.code==caseFoldings[caseFoldingIndex].code) {
        p.caseFolding=caseFoldings+caseFoldingIndex++;

        /* ignore "Common" mappings (simple==full) that map to the same code point as the regular lowercase mapping */
        if( p.caseFolding->status=='C' &&
            p.caseFolding->simple==p.lowerCase
        ) {
            p.caseFolding=NULL;
        }
    } else {
        p.caseFolding=NULL;
    }

    /* check for non-character code points */
    if((p.code&0xfffe)==0xfffe || (uint32_t)(p.code-0xfdd0)<0x20) {
        fprintf(stderr, "gencase: error - properties for non-character code point U+%04lx\n",
                (unsigned long)p.code);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }

    /* check that the code points (p.code) are in ascending order */
    if(p.code<=prevCode && p.code>0) {
        fprintf(stderr, "gencase: error - UnicodeData entries out of order, U+%04lx after U+%04lx\n",
                (unsigned long)p.code, (unsigned long)prevCode);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }

    /* properties for a single code point */
    setProps(&p);

    prevCode=p.code;
}

static void
parseDB(const char *filename, UErrorCode *pErrorCode) {
    char *fields[15][2];
    UChar32 start, end;
    int32_t i;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    u_parseDelimitedFile(filename, ';', fields, 15, unicodeDataLineFn, NULL, pErrorCode);

    /* are all sub-properties consumed? */
    if(specialCasingIndex<specialCasingCount) {
        fprintf(stderr, "gencase: error - some code points in SpecialCasing.txt are missing from UnicodeData.txt\n");
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }
    if(caseFoldingIndex<caseFoldingCount) {
        fprintf(stderr, "gencase: error - some code points in CaseFolding.txt are missing from UnicodeData.txt\n");
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }

    if(U_FAILURE(*pErrorCode)) {
        return;
    }

    for(i=0;
        0==uset_getItem(caseSensitive, i, &start, &end, NULL, 0, pErrorCode) && U_SUCCESS(*pErrorCode);
        ++i
    ) {
        addCaseSensitive(start, end);
    }
    if(*pErrorCode==U_INDEX_OUTOFBOUNDS_ERROR) {
        *pErrorCode=U_ZERO_ERROR;
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
