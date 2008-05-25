/*
*******************************************************************************
*
*   Copyright (C) 1999-2005, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  gennames.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999sep30
*   created by: Markus W. Scherer
*
*   This program reads the Unicode character database text file,
*   parses it, and extracts the character code,
*   the "modern" character name, and optionally the
*   Unicode 1.0 character name, and (starting with ICU 2.2) the ISO 10646 comment.
*   It then tokenizes and compresses the names and builds
*   compact binary tables for random-access lookup
*   in a u_charName() API function.
*
* unames.icu file format (after UDataInfo header etc. - see udata.c)
* (all data is static const)
*
* UDataInfo fields:
*   dataFormat "unam"
*   formatVersion 1.0
*   dataVersion = Unicode version from -u or --unicode command line option, defaults to 3.0.0
*
* -- data-based names
* uint32_t tokenStringOffset,
*          groupsOffset,
*          groupStringOffset,
*          algNamesOffset;
*
* uint16_t tokenCount;
* uint16_t tokenTable[tokenCount];
*
* char     tokenStrings[]; -- padded to even count
*
* -- strings (groupStrings) are tokenized as follows:
*   for each character c
*       if(c>=tokenCount) write that character c directly
*   else
*       token=tokenTable[c];
*       if(token==0xfffe) -- lead byte of double-byte token
*           token=tokenTable[c<<8|next character];
*       if(token==-1)
*           write c directly
*       else
*           tokenString=tokenStrings+token; (tokenStrings=start of names data + tokenStringOffset;)
*           append zero-terminated tokenString;
*
*    Different strings for a code point - normal name, 1.0 name, and ISO comment -
*    are separated by ';'.
*
* uint16_t groupCount;
* struct {
*   uint16_t groupMSB; -- for a group of 32 character names stored, this is code point>>5
*   uint16_t offsetHigh; -- group strings are at start of names data + groupStringsOffset + this 32 bit-offset
*   uint16_t offsetLow;
* } groupTable[groupCount];
*
* char     groupStrings[]; -- padded to 4-count
*
* -- The actual, tokenized group strings are not zero-terminated because
*   that would take up too much space.
*   Instead, they are preceeded by their length, written in a variable-length sequence:
*   For each of the 32 group strings, one or two nibbles are stored for its length.
*   Nibbles (4-bit values, half-bytes) are read MSB first.
*   A nibble with a value of 0..11 directly indicates the length of the name string.
*   A nibble n with a value of 12..15 is a lead nibble and forms a value with the following nibble m
*   by (((n-12)<<4)|m)+12, reaching values of 12..75.
*   These lengths are sequentially for each tokenized string, not for the de-tokenized result.
*   For the de-tokenizing, see token description above; the strings immediately follow the
*   32 lengths.
*
* -- algorithmic names
*
* typedef struct AlgorithmicRange {
*     uint32_t rangeStart, rangeEnd;
*     uint8_t algorithmType, algorithmVariant;
*     uint16_t rangeSize;
* } AlgorithmicRange;
*
* uint32_t algRangesCount; -- number of data blocks for ranges of
*               algorithmic names (Unicode 3.0.0: 3, hardcoded in gennames)
*
* struct {
*     AlgorithmicRange algRange;
*     uint8_t algRangeData[]; -- padded to 4-count except in last range
* } algRanges[algNamesCount];
* -- not a real array because each part has a different size
*    of algRange.rangeSize (including AlgorithmicRange)
*
* -- algorithmic range types:
*
* 0 Names are formed from a string prefix that is stored in
*   the algRangeData (zero-terminated), followed by the Unicode code point
*   of the character in hexadecimal digits;
*   algRange.algorithmVariant digits are written
*
* 1 Names are formed by calculating modulo-factors of the code point value as follows:
*   algRange.algorithmVariant is the count of modulo factors
*   algRangeData contains
*       uint16_t factors[algRange.algorithmVariant];
*       char strings[];
*   the first zero-terminated string is written as the prefix; then:
*
*   The rangeStart is subtracted; with the difference, here "code":
*   for(i=algRange.algorithmVariant-1 to 0 step -1)
*       index[i]=code%factor[i];
*       code/=factor[i];
*
*   The strings after the prefix are short pieces that are then appended to the result
*   according to index[0..algRange.algorithmVariant-1].
*/

#include <stdio.h>
#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/uclean.h"
#include "unicode/udata.h"
#include "cmemory.h"
#include "cstring.h"
#include "uarrsort.h"
#include "unewdata.h"
#include "uoptions.h"
#include "uparse.h"

#define STRING_STORE_SIZE 1000000
#define GROUP_STORE_SIZE 5000

#define GROUP_SHIFT 5
#define LINES_PER_GROUP (1UL<<GROUP_SHIFT)
#define GROUP_MASK (LINES_PER_GROUP-1)

#define MAX_LINE_COUNT 50000
#define MAX_WORD_COUNT 20000
#define MAX_GROUP_COUNT 5000

#define DATA_NAME "unames"
#define DATA_TYPE "icu"
#define VERSION_STRING "unam"
#define NAME_SEPARATOR_CHAR ';'

/* Unicode versions --------------------------------------------------------- */

enum {
    UNI_1_0,
    UNI_1_1,
    UNI_2_0,
    UNI_3_0,
    UNI_3_1,
    UNI_3_2,
    UNI_4_0,
    UNI_4_0_1,
    UNI_4_1,
    UNI_VER_COUNT
};

static const UVersionInfo
unicodeVersions[]={
    { 1, 0, 0, 0 },
    { 1, 1, 0, 0 },
    { 2, 0, 0, 0 },
    { 3, 0, 0, 0 },
    { 3, 1, 0, 0 },
    { 3, 2, 0, 0 },
    { 4, 0, 0, 0 },
    { 4, 0, 1, 0 },
    { 4, 1, 0, 0 }
};

static int32_t ucdVersion=UNI_4_1;

static int32_t
findUnicodeVersion(const UVersionInfo version) {
    int32_t i;

    for(i=0; /* while(version>unicodeVersions[i]) {} */
        i<UNI_VER_COUNT && uprv_memcmp(version, unicodeVersions[i], 4)>0;
        ++i) {}
    if(0<i && i<UNI_VER_COUNT && uprv_memcmp(version, unicodeVersions[i], 4)<0) {
        --i; /* fix 4.0.2 to land before 4.1, for valid x>=ucdVersion comparisons */
    }
    return i; /* version>=unicodeVersions[i] && version<unicodeVersions[i+1]; possible: i==UNI_VER_COUNT */
}

/* generator data ----------------------------------------------------------- */

/* UDataInfo cf. udata.h */
static UDataInfo dataInfo={
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    sizeof(UChar),
    0,

    {0x75, 0x6e, 0x61, 0x6d},     /* dataFormat="unam" */
    {1, 0, 0, 0},                 /* formatVersion */
    {3, 0, 0, 0}                  /* dataVersion */
};

static UBool beVerbose=FALSE, beQuiet=FALSE, haveCopyright=TRUE;

static uint8_t stringStore[STRING_STORE_SIZE],
               groupStore[GROUP_STORE_SIZE],
               lineLengths[LINES_PER_GROUP];

static uint32_t lineTop=0, wordBottom=STRING_STORE_SIZE, lineLengthsTop;

typedef struct {
    uint32_t code;
    int16_t length;
    uint8_t *s;
} Line;

typedef struct {
    int32_t weight; /* -(cost for token) + (number of occurences) * (length-1) */
    int16_t count;
    int16_t length;
    uint8_t *s;
} Word;

static Line lines[MAX_LINE_COUNT];
static Word words[MAX_WORD_COUNT];

static uint32_t lineCount=0, wordCount=0;

static int16_t leadByteCount;

#define LEADBYTE_LIMIT 16

static int16_t tokens[LEADBYTE_LIMIT*256];
static uint32_t tokenCount;

/* prototypes --------------------------------------------------------------- */

static void
init(void);

static void
parseDB(const char *filename, UBool store10Names);

static void
parseName(char *name, int16_t length);

static int16_t
skipNoise(char *line, int16_t start, int16_t limit);

static int16_t
getWord(char *line, int16_t start, int16_t limit);

static void
compress(void);

static void
compressLines(void);

static int16_t
compressLine(uint8_t *s, int16_t length, int16_t *pGroupTop);

static int32_t
compareWords(const void *context, const void *word1, const void *word2);

static void
generateData(const char *dataDir);

static uint32_t
generateAlgorithmicData(UNewDataMemory *pData);

static int16_t
findToken(uint8_t *s, int16_t length);

static Word *
findWord(char *s, int16_t length);

static Word *
addWord(char *s, int16_t length);

static void
countWord(Word *word);

static void
addLine(uint32_t code, char *names[], int16_t lengths[], int16_t count);

static void
addGroup(uint32_t groupMSB, uint8_t *strings, int16_t length);

static uint32_t
addToken(uint8_t *s, int16_t length);

static void
appendLineLength(int16_t length);

static void
appendLineLengthNibble(uint8_t nibble);

static uint8_t *
allocLine(int32_t length);

static uint8_t *
allocWord(uint32_t length);

/* -------------------------------------------------------------------------- */

static UOption options[]={
    UOPTION_HELP_H,
    UOPTION_HELP_QUESTION_MARK,
    UOPTION_VERBOSE,
    UOPTION_QUIET,
    UOPTION_COPYRIGHT,
    UOPTION_DESTDIR,
    { "unicode", NULL, NULL, NULL, 'u', UOPT_REQUIRES_ARG, 0 },
    { "unicode1-names", NULL, NULL, NULL, '1', UOPT_NO_ARG, 0 }
};

extern int
main(int argc, char* argv[]) {
    UVersionInfo version;
    UBool store10Names=FALSE;
    UErrorCode errorCode = U_ZERO_ERROR;

    U_MAIN_INIT_ARGS(argc, argv);

    /* Initialize ICU */
    u_init(&errorCode);
    if (U_FAILURE(errorCode) && errorCode != U_FILE_ACCESS_ERROR) {
        /* Note: u_init() will try to open ICU property data.
         *       failures here are expected when building ICU from scratch.
         *       ignore them.
         */
        fprintf(stderr, "%s: can not initialize ICU.  errorCode = %s\n",
            argv[0], u_errorName(errorCode));
        exit(1);
    }

    /* preset then read command line options */
    options[5].value=u_getDataDirectory();
    options[6].value="4.1";
    argc=u_parseArgs(argc, argv, sizeof(options)/sizeof(options[0]), options);

    /* error handling, printing usage message */
    if(argc<0) {
        fprintf(stderr,
            "error in command line argument \"%s\"\n",
            argv[-argc]);
    } else if(argc<2) {
        argc=-1;
    }
    if(argc<0 || options[0].doesOccur || options[1].doesOccur) {
        /*
         * Broken into chucks because the C89 standard says the minimum
         * required supported string length is 509 bytes.
         */
        fprintf(stderr,
            "Usage: %s [-1[+|-]] [-v[+|-]] [-c[+|-]] filename\n"
            "\n"
            "Read the UnicodeData.txt file and \n"
            "create a binary file " DATA_NAME "." DATA_TYPE " with the character names\n"
            "\n"
            "\tfilename  absolute path/filename for the Unicode database text file\n"
            "\t\t(default: standard input)\n"
            "\n",
            argv[0]);
        fprintf(stderr,
            "Options:\n"
            "\t-h or -? or --help  this usage text\n"
            "\t-v or --verbose     verbose output\n"
            "\t-q or --quiet       no output\n"
            "\t-c or --copyright   include a copyright notice\n"
            "\t-d or --destdir     destination directory, followed by the path\n"
            "\t-u or --unicode     Unicode version, followed by the version like 3.0.0\n"
            "\t-1 or --unicode1-names  store Unicode 1.0 character names\n");
        return argc<0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
    }

    /* get the options values */
    beVerbose=options[2].doesOccur;
    beQuiet=options[3].doesOccur;
    haveCopyright=options[4].doesOccur;
    store10Names=options[7].doesOccur;

    /* set the Unicode version */
    u_versionFromString(version, options[6].value);
    uprv_memcpy(dataInfo.dataVersion, version, 4);
    ucdVersion=findUnicodeVersion(version);

    init();
    parseDB(argc>=2 ? argv[1] : "-", store10Names);
    compress();
    generateData(options[5].value);

    u_cleanup();
    return 0;
}

static void
init() {
    int i;

    for(i=0; i<256; ++i) {
        tokens[i]=0;
    }
}

/* parsing ------------------------------------------------------------------ */

/* get a name, strip leading and trailing whitespace */
static int16_t
getName(char **pStart, char *limit) {
    /* strip leading whitespace */
    char *start=(char *)u_skipWhitespace(*pStart);

    /* strip trailing whitespace */
    while(start<limit && (*(limit-1)==' ' || *(limit-1)=='\t')) {
        --limit;
    }

    /* return results */
    *pStart=start;
    return (int16_t)(limit-start);
}

static void U_CALLCONV
lineFn(void *context,
       char *fields[][2], int32_t fieldCount,
       UErrorCode *pErrorCode) {
    char *names[3];
    int16_t lengths[3];
    static uint32_t prevCode=0;
    uint32_t code=0;

    if(U_FAILURE(*pErrorCode)) {
        return;
    }
    /* get the character code */
    code=uprv_strtoul(fields[0][0], NULL, 16);

    /* get the character name */
    names[0]=fields[1][0];
    lengths[0]=getName(names+0, fields[1][1]);
    if(names[0][0]=='<') {
        /* do not store pseudo-names in <> brackets */
        lengths[0]=0;
    }

    /* store 1.0 names */
    /* get the second character name, the one from Unicode 1.0 */
    /* do not store pseudo-names in <> brackets */
    names[1]=fields[10][0];
    lengths[1]=getName(names+1, fields[10][1]);
    if(*(UBool *)context && names[1][0]!='<') {
        /* keep the name */
    } else {
        lengths[1]=0;
    }

    /* get the ISO 10646 comment */
    names[2]=fields[11][0];
    lengths[2]=getName(names+2, fields[11][1]);

    if(lengths[0]+lengths[1]+lengths[2]==0) {
        return;
    }

    /* check for non-character code points */
    if(!UTF_IS_UNICODE_CHAR(code)) {
        fprintf(stderr, "gennames: error - properties for non-character code point U+%04lx\n",
                (unsigned long)code);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }

    /* check that the code points (code) are in ascending order */
    if(code<=prevCode && code>0) {
        fprintf(stderr, "gennames: error - UnicodeData entries out of order, U+%04lx after U+%04lx\n",
                (unsigned long)code, (unsigned long)prevCode);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }
    prevCode=code;

    parseName(names[0], lengths[0]);
    parseName(names[1], lengths[1]);
    parseName(names[2], lengths[2]);

    /*
     * set the count argument to
     * 1: only store regular names
     * 2: store regular and 1.0 names
     * 3: store names and ISO 10646 comment
     */
    addLine(code, names, lengths, 3);
}

static void
parseDB(const char *filename, UBool store10Names) {
    char *fields[15][2];
    UErrorCode errorCode=U_ZERO_ERROR;

    u_parseDelimitedFile(filename, ';', fields, 15, lineFn, &store10Names, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "gennames parse error: %s\n", u_errorName(errorCode));
        exit(errorCode);
    }

    if(!beQuiet) {
        printf("size of all names in the database: %lu\n",
            (unsigned long)lineTop);
        printf("number of named Unicode characters: %lu\n",
            (unsigned long)lineCount);
        printf("number of words in the dictionary from these names: %lu\n",
            (unsigned long)wordCount);
    }
}

static void
parseName(char *name, int16_t length) {
    int16_t start=0, limit, wordLength/*, prevStart=-1*/;
    Word *word;

    while(start<length) {
        /* skip any "noise" characters */
        limit=skipNoise(name, start, length);
        if(start<limit) {
            /*prevStart=-1;*/
            start=limit;
        }
        if(start==length) {
            break;
        }

        /* get a word and add it if it is longer than 1 */
        limit=getWord(name, start, length);
        wordLength=(int16_t)(limit-start);
        if(wordLength>1) {
            word=findWord(name+start, wordLength);
            if(word==NULL) {
                word=addWord(name+start, wordLength);
            }
            countWord(word);
        }

#if 0
        /*
         * if there was a word before this
         * (with no noise in between), then add the pair of words, too
         */
        if(prevStart!=-1) {
            wordLength=limit-prevStart;
            word=findWord(name+prevStart, wordLength);
            if(word==NULL) {
                word=addWord(name+prevStart, wordLength);
            }
            countWord(word);
        }
#endif

        /*prevStart=start;*/
        start=limit;
    }
}

static UBool U_INLINE
isWordChar(char c) {
    return ('A'<=c && c<='I') || /* EBCDIC-safe check for letters */
           ('J'<=c && c<='R') ||
           ('S'<=c && c<='Z') ||

           ('a'<=c && c<='i') || /* lowercase letters for ISO comments */
           ('j'<=c && c<='r') ||
           ('s'<=c && c<='z') ||

           ('0'<=c && c<='9');
}

static int16_t
skipNoise(char *line, int16_t start, int16_t limit) {
    /* skip anything that is not part of a word in this sense */
    while(start<limit && !isWordChar(line[start])) {
        ++start;
    }

    return start;
}

static int16_t
getWord(char *line, int16_t start, int16_t limit) {
    char c=0; /* initialize to avoid a compiler warning although the code was safe */

    /* a unicode character name word consists of A-Z0-9 */
    while(start<limit && isWordChar(line[start])) {
        ++start;
    }

    /* include a following space or dash */
    if(start<limit && ((c=line[start])==' ' || c=='-')) {
        ++start;
    }

    return start;
}

/* compressing -------------------------------------------------------------- */

static void
compress() {
    uint32_t i, letterCount;
    int16_t wordNumber;
    UErrorCode errorCode;

    /* sort the words in reverse order by weight */
    errorCode=U_ZERO_ERROR;
    uprv_sortArray(words, wordCount, sizeof(Word),
                    compareWords, NULL, FALSE, &errorCode);

    /* remove the words that do not save anything */
    while(wordCount>0 && words[wordCount-1].weight<1) {
        --wordCount;
    }

    /* count the letters in the token range */
    letterCount=0;
    for(i=LEADBYTE_LIMIT; i<256; ++i) {
        if(tokens[i]==-1) {
            ++letterCount;
        }
    }
    if(!beQuiet) {
        printf("number of letters used in the names: %d\n", (int)letterCount);
    }

    /* do we need double-byte tokens? */
    if(wordCount+letterCount<=256) {
        /* no, single-byte tokens are enough */
        leadByteCount=0;
        for(i=0, wordNumber=0; wordNumber<(int16_t)wordCount; ++i) {
            if(tokens[i]!=-1) {
                tokens[i]=wordNumber;
                if(beVerbose) {
                    printf("tokens[0x%03x]: word%8ld \"%.*s\"\n",
                            (int)i, (long)words[wordNumber].weight,
                            words[wordNumber].length, words[wordNumber].s);
                }
                ++wordNumber;
            }
        }
        tokenCount=i;
    } else {
        /*
         * The tokens that need two token bytes
         * get their weight reduced by their count
         * because they save less.
         */
        tokenCount=256-letterCount;
        for(i=tokenCount; i<wordCount; ++i) {
            words[i].weight-=words[i].count;
        }

        /* sort these words in reverse order by weight */
        errorCode=U_ZERO_ERROR;
        uprv_sortArray(words+tokenCount, wordCount-tokenCount, sizeof(Word),
                        compareWords, NULL, FALSE, &errorCode);

        /* remove the words that do not save anything */
        while(wordCount>0 && words[wordCount-1].weight<1) {
            --wordCount;
        }

        /* how many tokens and lead bytes do we have now? */
        tokenCount=wordCount+letterCount+(LEADBYTE_LIMIT-1);
        /*
         * adjust upwards to take into account that
         * double-byte tokens must not
         * use NAME_SEPARATOR_CHAR as a second byte
         */
        tokenCount+=(tokenCount-256+254)/255;

        leadByteCount=(int16_t)(tokenCount>>8);
        if(leadByteCount<LEADBYTE_LIMIT) {
            /* adjust for the real number of lead bytes */
            tokenCount-=(LEADBYTE_LIMIT-1)-leadByteCount;
        } else {
            /* limit the number of lead bytes */
            leadByteCount=LEADBYTE_LIMIT-1;
            tokenCount=LEADBYTE_LIMIT*256;
            wordCount=tokenCount-letterCount-(LEADBYTE_LIMIT-1);
            /* adjust again to skip double-byte tokens with ';' */
            wordCount-=(tokenCount-256+254)/255;
        }

        /* set token 0 to word 0 */
        tokens[0]=0;
        if(beVerbose) {
            printf("tokens[0x000]: word%8ld \"%.*s\"\n",
                    (long)words[0].weight,
                    words[0].length, words[0].s);
        }
        wordNumber=1;

        /* set the lead byte tokens */
        for(i=1; (int16_t)i<=leadByteCount; ++i) {
            tokens[i]=-2;
        }

        /* set the tokens */
        for(; i<256; ++i) {
            /* if store10Names then the parser set tokens[NAME_SEPARATOR_CHAR]=-1 */
            if(tokens[i]!=-1) {
                tokens[i]=wordNumber;
                if(beVerbose) {
                    printf("tokens[0x%03x]: word%8ld \"%.*s\"\n",
                            (int)i, (long)words[wordNumber].weight,
                            words[wordNumber].length, words[wordNumber].s);
                }
                ++wordNumber;
            }
        }

        /* continue above 255 where there are no letters */
        for(; (uint32_t)wordNumber<wordCount; ++i) {
            if((i&0xff)==NAME_SEPARATOR_CHAR) {
                tokens[i]=-1; /* do not use NAME_SEPARATOR_CHAR as a second token byte */
            } else {
                tokens[i]=wordNumber;
                if(beVerbose) {
                    printf("tokens[0x%03x]: word%8ld \"%.*s\"\n",
                            (int)i, (long)words[wordNumber].weight,
                            words[wordNumber].length, words[wordNumber].s);
                }
                ++wordNumber;
            }
        }
        tokenCount=i; /* should be already tokenCount={i or i+1} */
    }

    if(!beQuiet) {
        printf("number of lead bytes: %d\n", leadByteCount);
        printf("number of single-byte tokens: %lu\n",
            (unsigned long)256-letterCount-leadByteCount);
        printf("number of tokens: %lu\n", (unsigned long)tokenCount);
    }

    compressLines();
}

static void
compressLines() {
    Line *line=NULL;
    uint32_t i=0, inLine, outLine=0xffffffff /* (uint32_t)(-1) */,
             groupMSB=0xffff, lineCount2;
    int16_t groupTop=0;

    /* store the groups like lines, reusing the lines' memory */
    lineTop=0;
    lineCount2=lineCount;
    lineCount=0;

    /* loop over all lines */
    while(i<lineCount2) {
        line=lines+i++;
        inLine=line->code;

        /* segment the lines to groups of 32 */
        if(inLine>>GROUP_SHIFT!=groupMSB) {
            /* finish the current group with empty lines */
            while((++outLine&GROUP_MASK)!=0) {
                appendLineLength(0);
            }

            /* store the group like a line */
            if(groupTop>0) {
                if(groupTop>GROUP_STORE_SIZE) {
                    fprintf(stderr, "gennames: group store overflow\n");
                    exit(U_BUFFER_OVERFLOW_ERROR);
                }
                addGroup(groupMSB, groupStore, groupTop);
                if(lineTop>(uint32_t)(line->s-stringStore)) {
                    fprintf(stderr, "gennames: group store runs into string store\n");
                    exit(U_INTERNAL_PROGRAM_ERROR);
                }
            }

            /* start the new group */
            lineLengthsTop=0;
            groupTop=0;
            groupMSB=inLine>>GROUP_SHIFT;
            outLine=(inLine&~GROUP_MASK)-1;
        }

        /* write empty lines between the previous line in the group and this one */
        while(++outLine<inLine) {
            appendLineLength(0);
        }

        /* write characters and tokens for this line */
        appendLineLength(compressLine(line->s, line->length, &groupTop));
    }

    /* finish and store the last group */
    if(line && groupMSB!=0xffff) {
        /* finish the current group with empty lines */
        while((++outLine&GROUP_MASK)!=0) {
            appendLineLength(0);
        }

        /* store the group like a line */
        if(groupTop>0) {
            if(groupTop>GROUP_STORE_SIZE) {
                fprintf(stderr, "gennames: group store overflow\n");
                exit(U_BUFFER_OVERFLOW_ERROR);
            }
            addGroup(groupMSB, groupStore, groupTop);
            if(lineTop>(uint32_t)(line->s-stringStore)) {
                fprintf(stderr, "gennames: group store runs into string store\n");
                exit(U_INTERNAL_PROGRAM_ERROR);
            }
        }
    }

    if(!beQuiet) {
        printf("number of groups: %lu\n", (unsigned long)lineCount);
    }
}

static int16_t
compressLine(uint8_t *s, int16_t length, int16_t *pGroupTop) {
    int16_t start, limit, token, groupTop=*pGroupTop;

    start=0;
    do {
        /* write any "noise" characters */
        limit=skipNoise((char *)s, start, length);
        while(start<limit) {
            groupStore[groupTop++]=s[start++];
        }

        if(start==length) {
            break;
        }

        /* write a word, as token or directly */
        limit=getWord((char *)s, start, length);
        if(limit-start==1) {
            groupStore[groupTop++]=s[start++];
        } else {
            token=findToken(s+start, (int16_t)(limit-start));
            if(token!=-1) {
                if(token>0xff) {
                    groupStore[groupTop++]=(uint8_t)(token>>8);
                }
                groupStore[groupTop++]=(uint8_t)token;
                start=limit;
            } else {
                while(start<limit) {
                    groupStore[groupTop++]=s[start++];
                }
            }
        }
    } while(start<length);

    length=(int16_t)(groupTop-*pGroupTop);
    *pGroupTop=groupTop;
    return length;
}

static int32_t
compareWords(const void *context, const void *word1, const void *word2) {
    /* reverse sort by word weight */
    return ((Word *)word2)->weight-((Word *)word1)->weight;
}

/* generate output data ----------------------------------------------------- */

static void
generateData(const char *dataDir) {
    UNewDataMemory *pData;
    UErrorCode errorCode=U_ZERO_ERROR;
    uint16_t groupWords[3];
    uint32_t i, groupTop=lineTop, offset, size,
             tokenStringOffset, groupsOffset, groupStringOffset, algNamesOffset;
    long dataLength;
    int16_t token;

    pData=udata_create(dataDir, DATA_TYPE,DATA_NAME, &dataInfo,
                       haveCopyright ? U_COPYRIGHT_STRING : NULL, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "gennames: unable to create data memory, error %d\n", errorCode);
        exit(errorCode);
    }

    /* first, see how much space we need, and prepare the token strings */
    for(i=0; i<tokenCount; ++i) {
        token=tokens[i];
        if(token!=-1 && token!=-2) {
            tokens[i]=(int16_t)(addToken(words[token].s, words[token].length)-groupTop);
        }
    }

    /*
     * Required padding for data swapping:
     * The token table undergoes a permutation during data swapping when the
     * input and output charsets are different.
     * The token table cannot grow during swapping, so we need to make sure that
     * the table is long enough for successful in-place permutation.
     *
     * We simply round up tokenCount to the next multiple of 256 to account for
     * all possible permutations.
     *
     * An optimization is possible if we only ever swap between ASCII and EBCDIC:
     *
     * If tokenCount>256, then a semicolon (NAME_SEPARATOR_CHAR) is used
     * and will be swapped between ASCII and EBCDIC between
     * positions 0x3b (ASCII semicolon) and 0x5e (EBCDIC semicolon).
     * This should be the only -1 entry in tokens[256..511] on which the data
     * swapper bases its trail byte permutation map (trailMap[]).
     *
     * It would be sufficient to increase tokenCount so that its lower 8 bits
     * are at least 0x5e+1 to make room for swapping between the two semicolons.
     * For values higher than 0x5e, the trail byte permutation map (trailMap[])
     * should always be an identity map, where we do not need additional room.
     */
    i=tokenCount;
    tokenCount=(tokenCount+0xff)&~0xff;
    if(!beQuiet && i<tokenCount) {
        printf("number of tokens[] padding entries for data swapping: %lu\n", (unsigned long)(tokenCount-i));
    }
    for(; i<tokenCount; ++i) {
        if((i&0xff)==NAME_SEPARATOR_CHAR) {
            tokens[i]=-1; /* do not use NAME_SEPARATOR_CHAR as a second token byte */
        } else {
            tokens[i]=0; /* unused token for padding */
        }
    }

    /*
     * Calculate the total size in bytes of the data including:
     * - the offset to the token strings, uint32_t (4)
     * - the offset to the group table, uint32_t (4)
     * - the offset to the group strings, uint32_t (4)
     * - the offset to the algorithmic names, uint32_t (4)
     *
     * - the number of tokens, uint16_t (2)
     * - the token table, uint16_t[tokenCount] (2*tokenCount)
     *
     * - the token strings, each zero-terminated (tokenSize=(lineTop-groupTop)), 2-padded
     *
     * - the number of groups, uint16_t (2)
     * - the group table, { uint16_t groupMSB, uint16_t offsetHigh, uint16_t offsetLow }[6*groupCount]
     *
     * - the group strings (groupTop), 2-padded
     *
     * - the size of the data for the algorithmic names
     */
    tokenStringOffset=4+4+4+4+2+2*tokenCount;
    groupsOffset=(tokenStringOffset+(lineTop-groupTop+1))&~1;
    groupStringOffset=groupsOffset+2+6*lineCount;
    algNamesOffset=(groupStringOffset+groupTop+3)&~3;

    offset=generateAlgorithmicData(NULL);
    size=algNamesOffset+offset;

    if(!beQuiet) {
        printf("size of the Unicode Names data:\n"
               "total data length %lu, token strings %lu, compressed strings %lu, algorithmic names %lu\n",
                (unsigned long)size, (unsigned long)(lineTop-groupTop),
                (unsigned long)groupTop, (unsigned long)offset);
    }

    /* write the data to the file */
    /* offsets */
    udata_write32(pData, tokenStringOffset);
    udata_write32(pData, groupsOffset);
    udata_write32(pData, groupStringOffset);
    udata_write32(pData, algNamesOffset);

    /* token table */
    udata_write16(pData, (uint16_t)tokenCount);
    udata_writeBlock(pData, tokens, 2*tokenCount);

    /* token strings */
    udata_writeBlock(pData, stringStore+groupTop, lineTop-groupTop);
    if((lineTop-groupTop)&1) {
        /* 2-padding */
        udata_writePadding(pData, 1);
    }

    /* group table */
    udata_write16(pData, (uint16_t)lineCount);
    for(i=0; i<lineCount; ++i) {
        /* groupMSB */
        groupWords[0]=(uint16_t)lines[i].code;

        /* offset */
        offset = (uint32_t)(lines[i].s - stringStore);
        groupWords[1]=(uint16_t)(offset>>16);
        groupWords[2]=(uint16_t)(offset);
        udata_writeBlock(pData, groupWords, 6);
    }

    /* group strings */
    udata_writeBlock(pData, stringStore, groupTop);

    /* 4-align the algorithmic names data */
    udata_writePadding(pData, algNamesOffset-(groupStringOffset+groupTop));

    generateAlgorithmicData(pData);

    /* finish up */
    dataLength=udata_finish(pData, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "gennames: error %d writing the output file\n", errorCode);
        exit(errorCode);
    }

    if(dataLength!=(long)size) {
        fprintf(stderr, "gennames: data length %ld != calculated size %lu\n",
dataLength, (unsigned long)size);
        exit(U_INTERNAL_PROGRAM_ERROR);
    }
}

/* the structure for algorithmic names needs to be 4-aligned */
typedef struct AlgorithmicRange {
    uint32_t rangeStart, rangeEnd;
    uint8_t algorithmType, algorithmVariant;
    uint16_t rangeSize;
} AlgorithmicRange;

static uint32_t
generateAlgorithmicData(UNewDataMemory *pData) {
    static char prefix[] = "CJK UNIFIED IDEOGRAPH-";
#   define PREFIX_LENGTH 23
#   define PREFIX_LENGTH_4 24
    uint32_t countAlgRanges;

    static AlgorithmicRange cjkExtA={
        0x3400, 0x4db5,
        0, 4,
        sizeof(AlgorithmicRange)+PREFIX_LENGTH_4
    };
    static AlgorithmicRange cjk={
        0x4e00, 0x9fa5,
        0, 4,
        sizeof(AlgorithmicRange)+PREFIX_LENGTH_4
    };
    static AlgorithmicRange cjkExtB={
        0x20000, 0x2a6d6,
        0, 5,
        sizeof(AlgorithmicRange)+PREFIX_LENGTH_4
    };

    static char jamo[]=
        "HANGUL SYLLABLE \0"

        "G\0GG\0N\0D\0DD\0R\0M\0B\0BB\0"
        "S\0SS\0\0J\0JJ\0C\0K\0T\0P\0H\0"

        "A\0AE\0YA\0YAE\0EO\0E\0YEO\0YE\0O\0"
        "WA\0WAE\0OE\0YO\0U\0WEO\0WE\0WI\0"
        "YU\0EU\0YI\0I\0"

        "\0G\0GG\0GS\0N\0NJ\0NH\0D\0L\0LG\0LM\0"
        "LB\0LS\0LT\0LP\0LH\0M\0B\0BS\0"
        "S\0SS\0NG\0J\0C\0K\0T\0P\0H"
    ;

    static AlgorithmicRange hangul={
        0xac00, 0xd7a3,
        1, 3,
        sizeof(AlgorithmicRange)+6+sizeof(jamo)
    };

    /* modulo factors, maximum 8 */
    /* 3 factors: 19, 21, 28, most-to-least-significant */
    static uint16_t hangulFactors[3]={
        19, 21, 28
    };

    uint32_t size;

    size=0;

    if(ucdVersion>=UNI_4_1) {
        /* Unicode 4.1 and up has a longer CJK Unihan range than before */
        cjk.rangeEnd=0x9FBB;
    }

    /* number of ranges of algorithmic names */
    if(ucdVersion>=UNI_3_1) {
        /* Unicode 3.1 and up has 4 ranges including CJK Extension B */
        countAlgRanges=4;
    } else if(ucdVersion>=UNI_3_0) {
        /* Unicode 3.0 has 3 ranges including CJK Extension A */
        countAlgRanges=3;
    } else {
        /* Unicode 2.0 has 2 ranges including Hangul and CJK Unihan */
        countAlgRanges=2;
    }

    if(pData!=NULL) {
        udata_write32(pData, countAlgRanges);
    } else {
        size+=4;
    }

    /*
     * each range:
     * uint32_t rangeStart
     * uint32_t rangeEnd
     * uint8_t algorithmType
     * uint8_t algorithmVariant
     * uint16_t size of range data
     * uint8_t[size] data
     */

    /* range 0: cjk extension a */
    if(countAlgRanges>=3) {
        if(pData!=NULL) {
            udata_writeBlock(pData, &cjkExtA, sizeof(AlgorithmicRange));
            udata_writeString(pData, prefix, PREFIX_LENGTH);
            if(PREFIX_LENGTH<PREFIX_LENGTH_4) {
                udata_writePadding(pData, PREFIX_LENGTH_4-PREFIX_LENGTH);
            }
        } else {
            size+=sizeof(AlgorithmicRange)+PREFIX_LENGTH_4;
        }
    }

    /* range 1: cjk */
    if(pData!=NULL) {
        udata_writeBlock(pData, &cjk, sizeof(AlgorithmicRange));
        udata_writeString(pData, prefix, PREFIX_LENGTH);
        if(PREFIX_LENGTH<PREFIX_LENGTH_4) {
            udata_writePadding(pData, PREFIX_LENGTH_4-PREFIX_LENGTH);
        }
    } else {
        size+=sizeof(AlgorithmicRange)+PREFIX_LENGTH_4;
    }

    /* range 2: hangul syllables */
    if(pData!=NULL) {
        udata_writeBlock(pData, &hangul, sizeof(AlgorithmicRange));
        udata_writeBlock(pData, hangulFactors, 6);
        udata_writeString(pData, jamo, sizeof(jamo));
    } else {
        size+=sizeof(AlgorithmicRange)+6+sizeof(jamo);
    }

    /* range 3: cjk extension b */
    if(countAlgRanges>=4) {
        if(pData!=NULL) {
            udata_writeBlock(pData, &cjkExtB, sizeof(AlgorithmicRange));
            udata_writeString(pData, prefix, PREFIX_LENGTH);
            if(PREFIX_LENGTH<PREFIX_LENGTH_4) {
                udata_writePadding(pData, PREFIX_LENGTH_4-PREFIX_LENGTH);
            }
        } else {
            size+=sizeof(AlgorithmicRange)+PREFIX_LENGTH_4;
        }
    }

    return size;
}

/* helpers ------------------------------------------------------------------ */

static int16_t
findToken(uint8_t *s, int16_t length) {
    int16_t i, token;

    for(i=0; i<(int16_t)tokenCount; ++i) {
        token=tokens[i];
        if(token>=0 && length==words[token].length && 0==uprv_memcmp(s, words[token].s, length)) {
            return i;
        }
    }

    return -1;
}

static Word *
findWord(char *s, int16_t length) {
    uint32_t i;

    for(i=0; i<wordCount; ++i) {
        if(length==words[i].length && 0==uprv_memcmp(s, words[i].s, length)) {
            return words+i;
        }
    }

    return NULL;
}

static Word *
addWord(char *s, int16_t length) {
    uint8_t *stringStart;
    Word *word;

    if(wordCount==MAX_WORD_COUNT) {
        fprintf(stderr, "gennames: too many words\n");
        exit(U_BUFFER_OVERFLOW_ERROR);
    }

    stringStart=allocWord(length);
    uprv_memcpy(stringStart, s, length);

    word=words+wordCount;

    /*
     * Initialize the weight with the costs for this token:
     * a zero-terminated string and a 16-bit offset.
     */
    word->weight=-(length+1+2);
    word->count=0;
    word->length=length;
    word->s=stringStart;

    ++wordCount;

    return word;
}

static void
countWord(Word *word) {
    /* add to the weight the savings: the length of the word minus 1 byte for the token */
    word->weight+=word->length-1;
    ++word->count;
}

static void
addLine(uint32_t code, char *names[], int16_t lengths[], int16_t count) {
    uint8_t *stringStart;
    Line *line;
    int16_t i, length;

    if(lineCount==MAX_LINE_COUNT) {
        fprintf(stderr, "gennames: too many lines\n");
        exit(U_BUFFER_OVERFLOW_ERROR);
    }

    /* find the last non-empty name */
    while(count>0 && lengths[count-1]==0) {
        --count;
    }
    if(count==0) {
        return; /* should not occur: caller should not have called */
    }

    /* there will be (count-1) separator characters */
    i=count;
    length=count-1;

    /* add lengths of strings */
    while(i>0) {
        length+=lengths[--i];
    }

    /* allocate line memory */
    stringStart=allocLine(length);

    /* copy all strings into the line memory */
    length=0; /* number of chars copied so far */
    for(i=0; i<count; ++i) {
        if(i>0) {
            stringStart[length++]=NAME_SEPARATOR_CHAR;
        }
        if(lengths[i]>0) {
            uprv_memcpy(stringStart+length, names[i], lengths[i]);
            length+=lengths[i];
        }
    }

    line=lines+lineCount;

    line->code=code;
    line->length=length;
    line->s=stringStart;

    ++lineCount;

    /* prevent a character value that is actually in a name from becoming a token */
    while(length>0) {
        tokens[stringStart[--length]]=-1;
    }
}

static void
addGroup(uint32_t groupMSB, uint8_t *strings, int16_t length) {
    uint8_t *stringStart;
    Line *line;

    if(lineCount==MAX_LINE_COUNT) {
        fprintf(stderr, "gennames: too many groups\n");
        exit(U_BUFFER_OVERFLOW_ERROR);
    }

    /* store the line lengths first, then the strings */
    lineLengthsTop=(lineLengthsTop+1)/2;
    stringStart=allocLine(lineLengthsTop+length);
    uprv_memcpy(stringStart, lineLengths, lineLengthsTop);
    uprv_memcpy(stringStart+lineLengthsTop, strings, length);

    line=lines+lineCount;

    line->code=groupMSB;
    line->length=length;
    line->s=stringStart;

    ++lineCount;
}

static uint32_t
addToken(uint8_t *s, int16_t length) {
    uint8_t *stringStart;

    stringStart=allocLine(length+1);
    uprv_memcpy(stringStart, s, length);
    stringStart[length]=0;

    return (uint32_t)(stringStart - stringStore);
}

static void
appendLineLength(int16_t length) {
    if(length>=76) {
        fprintf(stderr, "gennames: compressed line too long\n");
        exit(U_BUFFER_OVERFLOW_ERROR);
    }
    if(length>=12) {
        length-=12;
        appendLineLengthNibble((uint8_t)((length>>4)|12));
    }
    appendLineLengthNibble((uint8_t)length);
}

static void
appendLineLengthNibble(uint8_t nibble) {
    if((lineLengthsTop&1)==0) {
        lineLengths[lineLengthsTop/2]=(uint8_t)(nibble<<4);
    } else {
        lineLengths[lineLengthsTop/2]|=nibble&0xf;
    }
    ++lineLengthsTop;
}

static uint8_t *
allocLine(int32_t length) {
    uint32_t top=lineTop+length;
    uint8_t *p;

    if(top>wordBottom) {
        fprintf(stderr, "gennames: out of memory\n");
        exit(U_MEMORY_ALLOCATION_ERROR);
    }
    p=stringStore+lineTop;
    lineTop=top;
    return p;
}

static uint8_t *
allocWord(uint32_t length) {
    uint32_t bottom=wordBottom-length;

    if(lineTop>bottom) {
        fprintf(stderr, "gennames: out of memory\n");
        exit(U_MEMORY_ALLOCATION_ERROR);
    }
    wordBottom=bottom;
    return stringStore+bottom;
}

/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 *
 */
