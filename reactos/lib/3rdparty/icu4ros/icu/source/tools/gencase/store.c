/*
*******************************************************************************
*
*   Copyright (C) 2004-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  store.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2004aug28
*   created by: Markus W. Scherer
*
*   Store Unicode case mapping properties efficiently for
*   random access.
*/

#include <stdio.h>
#include <stdlib.h>
#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "cmemory.h"
#include "cstring.h"
#include "filestrm.h"
#include "utrie.h"
#include "uarrsort.h"
#include "unicode/udata.h"
#include "unewdata.h"
#include "propsvec.h"
#include "writesrc.h"
#include "gencase.h"

#define LENGTHOF(array) (sizeof(array)/sizeof((array)[0]))

/* Unicode case mapping properties file format ---------------------------------

The file format prepared and written here contains several data
structures that store indexes or data.

Before the data contents described below, there are the headers required by
the udata API for loading ICU data. Especially, a UDataInfo structure
precedes the actual data. It contains platform properties values and the
file format version.

The following is a description of format version 1.1 .

Format version 1.1 adds data for case closure.

The file contains the following structures:

    const int32_t indexes[i0] with values i0, i1, ...:
    (see UCASE_IX_... constants for names of indexes)

    i0 indexLength; -- length of indexes[] (UCASE_IX_TOP)
    i1 dataLength; -- length in bytes of the post-header data (incl. indexes[])
    i2 trieSize; -- size in bytes of the case mapping properties trie
    i3 exceptionsLength; -- length in uint16_t of the exceptions array
    i4 unfoldLength; -- length in uint16_t of the reverse-folding array (new in format version 1.1)

    i5..i14 reservedIndexes; -- reserved values; 0 for now

    i15 maxFullLength; -- maximum length of a full case mapping/folding string


    Serialized trie, see utrie.h;

    const uint16_t exceptions[exceptionsLength];

    const UChar unfold[unfoldLength];


Trie data word:
Bits
if(exception) {
    15..4   unsigned exception index
} else {
    if(not uncased) {
        15..6   signed delta to simple case mapping code point
                (add delta to input code point)
    } else {
            6   the code point is case-ignorable
                (U+0307 is also case-ignorable but has an exception)
    }
     5..4   0 normal character with cc=0
            1 soft-dotted character
            2 cc=230
            3 other cc
}
    3   exception
    2   case sensitive
 1..0   0 uncased
        1 lowercase
        2 uppercase
        3 titlecase


Exceptions:
A sub-array of the exceptions array is indexed by the exception index in a
trie word.
The sub-array consists of the following fields:
    uint16_t excWord;
    uint16_t optional values [];
    UTF-16 strings for full (string) mappings for lowercase, case folding, uppercase, titlecase

excWord: (see UCASE_EXC_...)
Bits
    15  conditional case folding
    14  conditional special casing
13..12  same as non-exception trie data bits 5..4
        moved here because the exception index needs more bits than the delta
        0 normal character with cc=0
        1 soft-dotted character
        2 cc=230
        3 other cc
11.. 9  reserved
     8  if set, then for each optional-value slot there are 2 uint16_t values
        (high and low parts of 32-bit values)
        instead of single ones
 7.. 0  bits for which optional value is present

Optional-value slots:
0   lowercase mapping (code point)
1   case folding (code point)
2   uppercase mapping (code point)
3   titlecase mapping (code point)
4   reserved
5   reserved
6   closure mappings (new in format version 1.1)
7   there is at least one full (string) case mapping
    the length of each is encoded in a nibble of this optional value,
    and the strings follow this optional value in the same order:
    lower/fold/upper/title

The optional closure mappings value is used as follows:
Bits 0..3 contain the length of a string of code points for case closure.
The string immediately follows the full case mappings, or the closure value
slot if there are no full case mappings.
Bits 4..15 are reserved and could be used in the future to indicate the
number of strings for case closure.
Complete case closure for a code point is given by the union of all simple
and full case mappings and foldings, plus the case closure code points
(and potentially, in the future, case closure strings).

For space saving, some values are not stored. Lookups are as follows:
- If special casing is conditional, then no full lower/upper/title mapping
  strings are stored.
- If case folding is conditional, then no simple or full case foldings are
  stored.
- Fall back in this order:
    full (string) mapping -- if full mappings are used
    simple (code point) mapping of the same type
    simple fold->simple lower
    simple title->simple upper
    finally, the original code point (no mapping)

This fallback order is strict:
In particular, the fallback from full case folding is to simple case folding,
not to full lowercase mapping.

Reverse case folding data ("unfold") array: (new in format version 1.1)

This array stores some miscellaneous values followed by a table. The data maps
back from multi-character strings to their original code points, for use
in case closure.

The table contains two columns of strings.
The string in the first column is the case folding of each of the code points
in the second column. The strings are terminated with NUL or by the end of the
column, whichever comes first.

The miscellaneous data takes up one pseudo-row and includes:
- number of rows
- number of UChars per row
- number of UChars in the left (folding string) column

The table is sorted by its first column. Values in the first column are unique.

----------------------------------------------------------------------------- */

/* UDataInfo cf. udata.h */
static UDataInfo dataInfo={
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    U_SIZEOF_UCHAR,
    0,

    /* dataFormat="cAsE" */
    { UCASE_FMT_0, UCASE_FMT_1, UCASE_FMT_2, UCASE_FMT_3 },
    { 1, 1, UTRIE_SHIFT, UTRIE_INDEX_SHIFT },   /* formatVersion */
    { 4, 0, 1, 0 }                              /* dataVersion */
};

enum {
    /* maximum number of exceptions expected */
    MAX_EXC_COUNT=1000
};

/* exceptions values */
static uint16_t exceptions[UCASE_MAX_EXCEPTIONS+100];
static uint16_t exceptionsTop=0;
static Props excProps[MAX_EXC_COUNT];
static uint16_t exceptionsCount=0;

/* becomes indexes[UCASE_IX_MAX_FULL_LENGTH] */
static int32_t maxFullLength=U16_MAX_LENGTH;

/* reverse case folding ("unfold") data */
static UChar unfold[UGENCASE_UNFOLD_MAX_ROWS*UGENCASE_UNFOLD_WIDTH]={
    0, UGENCASE_UNFOLD_WIDTH, UGENCASE_UNFOLD_STRING_WIDTH, 0, 0
};
static uint16_t unfoldRows=0;
static uint16_t unfoldTop=UGENCASE_UNFOLD_WIDTH;

/* Unicode versions --------------------------------------------------------- */

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

int32_t ucdVersion=UNI_4_1;

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

extern void
setUnicodeVersion(const char *v) {
    UVersionInfo version;
    u_versionFromString(version, v);
    uprv_memcpy(dataInfo.dataVersion, version, 4);
    ucdVersion=findUnicodeVersion(version);
}

/* -------------------------------------------------------------------------- */

static void
addUnfolding(UChar32 c, const UChar *s, int32_t length) {
    int32_t i;

    if(length>UGENCASE_UNFOLD_STRING_WIDTH) {
        fprintf(stderr, "gencase error: case folding too long (length=%ld>%d=UGENCASE_UNFOLD_STRING_WIDTH)\n",
                (long)length, UGENCASE_UNFOLD_STRING_WIDTH);
        exit(U_INTERNAL_PROGRAM_ERROR);
    }
    if(unfoldTop >= (LENGTHOF(unfold) - UGENCASE_UNFOLD_STRING_WIDTH)) {
        fprintf(stderr, "gencase error: too many multi-character case foldings\n");
        exit(U_BUFFER_OVERFLOW_ERROR);
    }
    u_memset(unfold+unfoldTop, 0, UGENCASE_UNFOLD_WIDTH);
    u_memcpy(unfold+unfoldTop, s, length);

    i=unfoldTop+UGENCASE_UNFOLD_STRING_WIDTH;
    U16_APPEND_UNSAFE(unfold, i, c);

    ++unfoldRows;
    unfoldTop+=UGENCASE_UNFOLD_WIDTH;
}

/* store a character's properties ------------------------------------------- */

extern void
setProps(Props *p) {
    UErrorCode errorCode;
    uint32_t value, oldValue;
    int32_t delta;
    UBool isCaseIgnorable;

    /* get the non-UnicodeData.txt properties */
    value=oldValue=upvec_getValue(pv, p->code, 0);

    /* default: map to self */
    delta=0;

    if(p->gc==U_TITLECASE_LETTER) {
        /* the Titlecase property is read late, from UnicodeData.txt */
        value|=UCASE_TITLE;
    }

    if(p->upperCase!=0) {
        /* uppercase mapping as delta if the character is lowercase */
        if((value&UCASE_TYPE_MASK)==UCASE_LOWER) {
            delta=p->upperCase-p->code;
        } else {
            value|=UCASE_EXCEPTION;
        }
    }
    if(p->lowerCase!=0) {
        /* lowercase mapping as delta if the character is uppercase or titlecase */
        if((value&UCASE_TYPE_MASK)>=UCASE_UPPER) {
            delta=p->lowerCase-p->code;
        } else {
            value|=UCASE_EXCEPTION;
        }
    }
    if(p->upperCase!=p->titleCase) {
        value|=UCASE_EXCEPTION;
    }
    if(p->closure[0]!=0) {
        value|=UCASE_EXCEPTION;
    }
    if(p->specialCasing!=NULL) {
        value|=UCASE_EXCEPTION;
    }
    if(p->caseFolding!=NULL) {
        value|=UCASE_EXCEPTION;
    }

    if(delta<UCASE_MIN_DELTA || UCASE_MAX_DELTA<delta) {
        value|=UCASE_EXCEPTION;
    }

    if(p->cc!=0) {
        if(value&UCASE_DOT_MASK) {
            fprintf(stderr, "gencase: a soft-dotted character has cc!=0\n");
            exit(U_INTERNAL_PROGRAM_ERROR);
        }
        if(p->cc==230) {
            value|=UCASE_ABOVE;
        } else {
            value|=UCASE_OTHER_ACCENT;
        }
    }

    /* encode case-ignorable as delta==1 on uncased characters */
    isCaseIgnorable=FALSE;
    if((value&UCASE_TYPE_MASK)==UCASE_NONE) {
        if(ucdVersion>=UNI_4_1) {
            /* Unicode 4.1 and up: (D47a) Word_Break=MidLetter or Mn, Me, Cf, Lm, Sk */
            if(
                (U_MASK(p->gc)&(U_GC_MN_MASK|U_GC_ME_MASK|U_GC_CF_MASK|U_GC_LM_MASK|U_GC_SK_MASK))!=0 ||
                ((upvec_getValue(pv, p->code, 1)>>UGENCASE_IS_MID_LETTER_SHIFT)&1)!=0
            ) {
                isCaseIgnorable=TRUE;
            }
        } else {
            /* before Unicode 4.1: Mn, Me, Cf, Lm, Sk or 0027 or 00AD or 2019 */
            if(
                (U_MASK(p->gc)&(U_GC_MN_MASK|U_GC_ME_MASK|U_GC_CF_MASK|U_GC_LM_MASK|U_GC_SK_MASK))!=0 ||
                p->code==0x27 || p->code==0xad || p->code==0x2019
            ) {
                isCaseIgnorable=TRUE;
            }
        }
    }

    if(isCaseIgnorable && p->code!=0x307) {
        /*
         * We use one of the delta/exception bits, which works because we only
         * store the case-ignorable flag for uncased characters.
         * There is no delta for uncased characters (see checks above).
         * If there is an exception for an uncased, case-ignorable character
         * (although there should not be any case mappings if it's uncased)
         * then we have a problem.
         * There is one character which is case-ignorable but has an exception:
         * U+0307 is uncased, Mn, has conditional special casing and
         * is therefore handled in code instead.
         */
        if(value&UCASE_EXCEPTION) {
            fprintf(stderr, "gencase error: unable to encode case-ignorable for U+%04lx with exceptions\n",
                            (unsigned long)p->code);
            exit(U_INTERNAL_PROGRAM_ERROR);
        }

        delta=1;
    }

    /* handle exceptions */
    if(value&UCASE_EXCEPTION) {
        /* simply store exceptions for later processing and encoding */
        value|=(uint32_t)exceptionsCount<<UGENCASE_EXC_SHIFT;
        uprv_memcpy(excProps+exceptionsCount, p, sizeof(*p));
        if(++exceptionsCount==MAX_EXC_COUNT) {
            fprintf(stderr, "gencase: too many exceptions\n");
            exit(U_INDEX_OUTOFBOUNDS_ERROR);
        }
    } else {
        /* store the simple case mapping delta */
        value|=((uint32_t)delta<<UCASE_DELTA_SHIFT)&UCASE_DELTA_MASK;
    }

    errorCode=U_ZERO_ERROR;
    if( value!=oldValue &&
        !upvec_setValue(pv, p->code, p->code+1, 0, value, 0xffffffff, &errorCode)
    ) {
        fprintf(stderr, "gencase error: unable to set case mapping values, code: %s\n",
                        u_errorName(errorCode));
        exit(errorCode);
    }

    /* add the multi-character case folding to the "unfold" data */
    if(p->caseFolding!=NULL) {
        int32_t length=p->caseFolding->full[0];
        if(length>1 && u_strHasMoreChar32Than(p->caseFolding->full+1, length, 1)) {
            addUnfolding(p->code, p->caseFolding->full+1, length);
        }
    }
}

extern void
addCaseSensitive(UChar32 first, UChar32 last) {
    UErrorCode errorCode=U_ZERO_ERROR;
    if(!upvec_setValue(pv, first, last+1, 0, UCASE_SENSITIVE, UCASE_SENSITIVE, &errorCode)) {
        fprintf(stderr, "gencase error: unable to set UCASE_SENSITIVE, code: %s\n",
                        u_errorName(errorCode));
        exit(errorCode);
    }
}

/* finalize reverse case folding ("unfold") data ---------------------------- */

static int32_t U_CALLCONV
compareUnfold(const void *context, const void *left, const void *right) {
    return u_memcmp((const UChar *)left, (const UChar *)right, UGENCASE_UNFOLD_WIDTH);
}

static void
makeUnfoldData() {
    static const UChar
        iDot[2]=        { 0x69, 0x307 };

    UChar *p, *q;
    int32_t i, j, k;
    UErrorCode errorCode;

    /*
     * add a case folding that we missed because it's conditional:
     * 0130; F; 0069 0307; # LATIN CAPITAL LETTER I WITH DOT ABOVE
     */
    addUnfolding(0x130, iDot, 2);

    /* sort the data */
    errorCode=U_ZERO_ERROR;
    uprv_sortArray(unfold+UGENCASE_UNFOLD_WIDTH, unfoldRows, UGENCASE_UNFOLD_WIDTH*2,
                   compareUnfold, NULL, FALSE, &errorCode);

    /* make unique-string rows by merging adjacent ones' code point columns */

    /* make p point to row i-1 */
    p=(UChar *)unfold+UGENCASE_UNFOLD_WIDTH;

    for(i=1; i<unfoldRows;) {
        if(0==u_memcmp(p, p+UGENCASE_UNFOLD_WIDTH, UGENCASE_UNFOLD_STRING_WIDTH)) {
            /* concatenate code point columns */
            q=p+UGENCASE_UNFOLD_STRING_WIDTH;
            for(j=1; j<UGENCASE_UNFOLD_CP_WIDTH && q[j]!=0; ++j) {}
            for(k=0; k<UGENCASE_UNFOLD_CP_WIDTH && q[UGENCASE_UNFOLD_WIDTH+k]!=0; ++j, ++k) {
                q[j]=q[UGENCASE_UNFOLD_WIDTH+k];
            }
            if(j>UGENCASE_UNFOLD_CP_WIDTH) {
                fprintf(stderr, "gencase error: too many code points in unfold[]: %ld>%d=UGENCASE_UNFOLD_CP_WIDTH\n",
                        (long)j, UGENCASE_UNFOLD_CP_WIDTH);
                exit(U_BUFFER_OVERFLOW_ERROR);
            }

            /* move following rows up one */
            --unfoldRows;
            unfoldTop-=UGENCASE_UNFOLD_WIDTH;
            u_memmove(p+UGENCASE_UNFOLD_WIDTH, p+UGENCASE_UNFOLD_WIDTH*2, (unfoldRows-i)*UGENCASE_UNFOLD_WIDTH);
        } else {
            p+=UGENCASE_UNFOLD_WIDTH;
            ++i;
        }
    }

    unfold[UCASE_UNFOLD_ROWS]=(UChar)unfoldRows;

    if(beVerbose) {
        puts("unfold data:");

        p=(UChar *)unfold;
        for(i=0; i<unfoldRows; ++i) {
            p+=UGENCASE_UNFOLD_WIDTH;
            printf("[%2d] %04x %04x %04x <- %04x %04x\n",
                   (int)i, p[0], p[1], p[2], p[3], p[4]);
        }
    }
}

/* case closure ------------------------------------------------------------- */

static void
addClosureMapping(UChar32 src, UChar32 dest) {
    uint32_t value;

    if(beVerbose) {
        printf("add closure mapping U+%04lx->U+%04lx\n",
                (unsigned long)src, (unsigned long)dest);
    }

    value=upvec_getValue(pv, src, 0);
    if(value&UCASE_EXCEPTION) {
        Props *p=excProps+(value>>UGENCASE_EXC_SHIFT);
        int32_t i;

        /* append dest to src's closure array */
        for(i=0;; ++i) {
            if(i==LENGTHOF(p->closure)) {
                fprintf(stderr, "closure[] overflow for U+%04lx->U+%04lx\n",
                                (unsigned long)src, (unsigned long)dest);
                exit(U_BUFFER_OVERFLOW_ERROR);
            } else if(p->closure[i]==dest) {
                break; /* do not store duplicates */
            } else if(p->closure[i]==0) {
                p->closure[i]=dest;
                break;
            }
        }
    } else {
        Props p2={ 0 };
        UChar32 next;
        UErrorCode errorCode;

        /*
         * decode value into p2 (enough for makeException() to work properly),
         * add the closure mapping,
         * and set the new exception for src
         */
        p2.code=src;
        p2.closure[0]=dest;

        if((value&UCASE_TYPE_MASK)>UCASE_NONE) {
            /* one simple case mapping, don't care which one */
            next=src+((int16_t)value>>UCASE_DELTA_SHIFT);
            if(next!=src) {
                if((value&UCASE_TYPE_MASK)==UCASE_LOWER) {
                    p2.upperCase=p2.titleCase=next;
                } else {
                    p2.lowerCase=next;
                }
            }
        } else if(value&UCASE_DELTA_MASK) {
            fprintf(stderr, "gencase error: unable to add case closure exception to case-ignorable U+%04lx\n",
                            (unsigned long)src);
            exit(U_INTERNAL_PROGRAM_ERROR);
        }

        value&=~(UGENCASE_EXC_MASK|UCASE_DELTA_MASK); /* remove previous simple mapping */
        value|=(uint32_t)exceptionsCount<<UGENCASE_EXC_SHIFT;
        value|=UCASE_EXCEPTION;
        uprv_memcpy(excProps+exceptionsCount, &p2, sizeof(p2));
        if(++exceptionsCount==MAX_EXC_COUNT) {
            fprintf(stderr, "gencase: too many exceptions\n");
            exit(U_INDEX_OUTOFBOUNDS_ERROR);
        }

        errorCode=U_ZERO_ERROR;
        if(!upvec_setValue(pv, src, src+1, 0, value, 0xffffffff, &errorCode)) {
            fprintf(stderr, "gencase error: unable to set case mapping values, code: %s\n",
                            u_errorName(errorCode));
            exit(errorCode);
        }
    }
}

/*
 * Find missing case mapping relationships and add mappings for case closure.
 * This function starts from an "original" code point and recursively
 * finds its case mappings and the case mappings of where it maps to.
 *
 * The recursion depth is capped at 3 nested calls of this function.
 * In each call, the current code point is c, and the function enumerates
 * all of c's simple (single-code point) case mappings.
 * prev is the code point that case-mapped to c.
 * prev2 is the code point that case-mapped to prev.
 *
 * The initial function call has prev2<0, prev<0, and c==orig
 * (marking no code points).
 * It enumerates c's case mappings and recurses without further action.
 *
 * The second-level function call has prev2<0, prev==orig, and c is
 * the destination code point of one of prev's case mappings.
 * The function checks if any of c's case mappings go back to orig
 * and adds a closure mapping if not.
 * In other words, it turns a case mapping relationship of
 *   orig->c
 * into
 *   orig<->c
 *
 * The third-level function call has prev2==orig, prev>=0, and c is
 * the destination code point of one of prev's case mappings.
 * (And prev is the destination of one of prev2's case mappings.)
 * The function checks if any of c's case mappings go back to orig
 * and adds a closure mapping if not.
 * In other words, it turns case mapping relationships of
 *   orig->prev->c or orig->prev<->c
 * into
 *   orig->prev->c->orig or orig->prev<->c->orig
 * etc.
 * (Graphically, this closes a triangle.)
 *
 * With repeated application on all code points until no more closure mappings
 * are added, all case equivalence groups get complete mappings.
 * That is, in each group of code points with case relationships
 * each code point will in the end have some mapping to each other
 * code point in the group.
 *
 * @return TRUE if a closure mapping was added
 */
static UBool
addClosure(UChar32 orig, UChar32 prev2, UChar32 prev, UChar32 c, uint32_t value) {
    UChar32 next;
    UBool someMappingsAdded=FALSE;

    if(c!=orig) {
        /* get the properties for c */
        value=upvec_getValue(pv, c, 0);
    }
    /* else if c==orig then c's value was passed in */

    if(value&UCASE_EXCEPTION) {
        UChar32 set[32];
        int32_t i, count=0;

        Props *p=excProps+(value>>UGENCASE_EXC_SHIFT);

        /*
         * marker for whether any of c's mappings goes to orig
         * c==orig: prevent adding a closure mapping when getting orig's own, direct mappings
         */
        UBool mapsToOrig=(UBool)(c==orig);

        /* collect c's case mapping destinations in set[] */
        if((next=p->upperCase)!=0 && next!=c) {
            set[count++]=next;
        }
        if((next=p->lowerCase)!=0 && next!=c) {
            set[count++]=next;
        }
        if(p->upperCase!=(next=p->titleCase) && next!=c) {
            set[count++]=next;
        }
        if(p->caseFolding!=NULL && (next=p->caseFolding->simple)!=0 && next!=c) {
            set[count++]=next;
        }

        /* append c's current closure mappings to set[] */
        for(i=0; i<LENGTHOF(p->closure) && (next=p->closure[i])!=0; ++i) {
            set[count++]=next;
        }

        /* process all code points to which c case-maps */
        for(i=0; i<count; ++i) {
            next=set[i]; /* next!=c */

            if(next==orig) {
                mapsToOrig=TRUE; /* remember that we map to orig */
            } else if(prev2<0 && next!=prev) {
                /*
                 * recurse unless
                 * we have reached maximum depth (prev2>=0) or
                 * this is a mapping to one of the previous code points (orig, prev, c)
                 */
                someMappingsAdded|=addClosure(orig, prev, c, next, 0);
            }
        }

        if(!mapsToOrig) {
            addClosureMapping(c, orig);
            return TRUE;
        }
    } else {
        if((value&UCASE_TYPE_MASK)>UCASE_NONE) {
            /* one simple case mapping, don't care which one */
            next=c+((int16_t)value>>UCASE_DELTA_SHIFT);
            if(next!=c) {
                /*
                 * recurse unless
                 * we have reached maximum depth (prev2>=0) or
                 * this is a mapping to one of the previous code points (orig, prev, c)
                 */
                if(prev2<0 && next!=orig && next!=prev) {
                    someMappingsAdded|=addClosure(orig, prev, c, next, 0);
                }

                if(c!=orig && next!=orig) {
                    /* c does not map to orig, add a closure mapping c->orig */
                    addClosureMapping(c, orig);
                    return TRUE;
                }
            }
        }
    }

    return someMappingsAdded;
}

extern void
makeCaseClosure() {
    UChar *p;
    uint32_t *row;
    uint32_t value;
    UChar32 start, limit, c, c2;
    int32_t i, j;
    UBool someMappingsAdded;

    /*
     * finalize the "unfold" data because we need to use it to add closure mappings
     * for situations like FB05->"st"<-FB06
     * where we would otherwise miss the FB05<->FB06 relationship
     */
    makeUnfoldData();

    /* use the "unfold" data to add mappings */

    /* p always points to the code points; this loop ignores the strings completely */
    p=unfold+UGENCASE_UNFOLD_WIDTH+UGENCASE_UNFOLD_STRING_WIDTH;

    for(i=0; i<unfoldRows; p+=UGENCASE_UNFOLD_WIDTH, ++i) {
        j=0;
        U16_NEXT_UNSAFE(p, j, c);
        while(j<UGENCASE_UNFOLD_CP_WIDTH && p[j]!=0) {
            U16_NEXT_UNSAFE(p, j, c2);
            addClosure(c, U_SENTINEL, c, c2, 0);
        }
    }

    if(beVerbose) {
        puts("---- ---- ---- ---- (done with closures from unfolding)");
    }

    /* add further closure mappings from analyzing simple mappings */
    do {
        someMappingsAdded=FALSE;

        i=0;
        while((row=upvec_getRow(pv, i, &start, &limit))!=NULL) {
            value=*row;
            if(value!=0) {
                while(start<limit) {
                    if(addClosure(start, U_SENTINEL, U_SENTINEL, start, value)) {
                        someMappingsAdded=TRUE;

                        /*
                         * stop this loop because pv was changed and row is not valid any more
                         * skip all rows below the current start
                         */
                        while((row=upvec_getRow(pv, i, NULL, &limit))!=NULL && start>=limit) {
                            ++i;
                        }
                        row=NULL; /* signal to continue with outer loop, without further ++i */
                        break;
                    }
                    ++start;
                }
                if(row==NULL) {
                    continue; /* see row=NULL above */
                }
            }
            ++i;
        }

        if(beVerbose && someMappingsAdded) {
            puts("---- ---- ---- ----");
        }
    } while(someMappingsAdded);
}

/* exceptions --------------------------------------------------------------- */

/* get the string length from zero-terminated code points in a limited-length array */
static int32_t
getLengthOfCodePoints(const UChar32 *s, int32_t maxLength) {
    int32_t i, length;

    for(i=length=0; i<maxLength && s[i]!=0; ++i) {
        length+=U16_LENGTH(s[i]);
    }
    return length;
}

static UBool
fullMappingEqualsSimple(const UChar *s, UChar32 simple, UChar32 c) {
    int32_t i, length;
    UChar32 full;

    length=*s++;
    if(length==0 || length>U16_MAX_LENGTH) {
        return FALSE;
    }
    i=0;
    U16_NEXT(s, i, length, full);

    if(simple==0) {
        simple=c; /* UCD has no simple mapping if it's the same as the code point itself */
    }
    return (UBool)(i==length && full==simple);
}

static uint16_t
makeException(uint32_t value, Props *p) {
    uint32_t slots[8];
    uint32_t slotBits;
    uint16_t excWord, excIndex, excTop, i, count, length, fullLengths;
    UBool doubleSlots;

    /* excIndex will be returned for storing in the trie word */
    excIndex=exceptionsTop;
    if(excIndex>=UCASE_MAX_EXCEPTIONS) {
        fprintf(stderr, "gencase error: too many exceptions words\n");
        exit(U_BUFFER_OVERFLOW_ERROR);
    }

    excTop=excIndex+1; /* +1 for excWord which will be stored at excIndex */

    /* copy and shift the soft-dotted bits */
    excWord=((uint16_t)value&UCASE_DOT_MASK)<<UCASE_EXC_DOT_SHIFT;

    /* update maxFullLength */
    if(p->specialCasing!=NULL) {
        length=p->specialCasing->lowerCase[0];
        if(length>maxFullLength) {
            maxFullLength=length;
        }
        length=p->specialCasing->upperCase[0];
        if(length>maxFullLength) {
            maxFullLength=length;
        }
        length=p->specialCasing->titleCase[0];
        if(length>maxFullLength) {
            maxFullLength=length;
        }
    }
    if(p->caseFolding!=NULL) {
        length=p->caseFolding->full[0];
        if(length>maxFullLength) {
            maxFullLength=length;
        }
    }

    /* set the bits for conditional mappings */
    if(p->specialCasing!=NULL && p->specialCasing->isComplex) {
        excWord|=UCASE_EXC_CONDITIONAL_SPECIAL;
        p->specialCasing=NULL;
    }
    if(p->caseFolding!=NULL && p->caseFolding->simple==0 && p->caseFolding->full[0]==0) {
        excWord|=UCASE_EXC_CONDITIONAL_FOLD;
        p->caseFolding=NULL;
    }

    /*
     * Note:
     * UCD stores no simple mappings when they are the same as the code point itself.
     * SpecialCasing and CaseFolding do store simple mappings even if they are
     * the same as the code point itself.
     * Comparisons between simple regular mappings and simple special/folding
     * mappings need to compensate for the difference by comparing with the
     * original code point if a simple UCD mapping is missing (0).
     */

    /* remove redundant data */
    if(p->specialCasing!=NULL) {
        /* do not store full mappings if they are the same as the simple ones */
        if(fullMappingEqualsSimple(p->specialCasing->lowerCase, p->lowerCase, p->code)) {
            p->specialCasing->lowerCase[0]=0;
        }
        if(fullMappingEqualsSimple(p->specialCasing->upperCase, p->upperCase, p->code)) {
            p->specialCasing->upperCase[0]=0;
        }
        if(fullMappingEqualsSimple(p->specialCasing->titleCase, p->titleCase, p->code)) {
            p->specialCasing->titleCase[0]=0;
        }
    }
    if( p->caseFolding!=NULL &&
        fullMappingEqualsSimple(p->caseFolding->full, p->caseFolding->simple, p->code)
    ) {
        p->caseFolding->full[0]=0;
    }

    /* write the optional slots */
    slotBits=0;
    count=0;

    if(p->lowerCase!=0) {
        slots[count]=(uint32_t)p->lowerCase;
        slotBits|=slots[count];
        ++count;
        excWord|=U_MASK(UCASE_EXC_LOWER);
    }
    if( p->caseFolding!=NULL &&
        p->caseFolding->simple!=0 &&
        (p->lowerCase!=0 ?
            p->caseFolding->simple!=p->lowerCase :
            p->caseFolding->simple!=p->code)
    ) {
        slots[count]=(uint32_t)p->caseFolding->simple;
        slotBits|=slots[count];
        ++count;
        excWord|=U_MASK(UCASE_EXC_FOLD);
    }
    if(p->upperCase!=0) {
        slots[count]=(uint32_t)p->upperCase;
        slotBits|=slots[count];
        ++count;
        excWord|=U_MASK(UCASE_EXC_UPPER);
    }
    if(p->upperCase!=p->titleCase) {
        if(p->titleCase!=0) {
            slots[count]=(uint32_t)p->titleCase;
        } else {
            slots[count]=(uint32_t)p->code;
        }
        slotBits|=slots[count];
        ++count;
        excWord|=U_MASK(UCASE_EXC_TITLE);
    }

    /* length of case closure */
    if(p->closure[0]!=0) {
        length=getLengthOfCodePoints(p->closure, LENGTHOF(p->closure));
        slots[count]=(uint32_t)length; /* must be 1..UCASE_CLOSURE_MAX_LENGTH */
        slotBits|=slots[count];
        ++count;
        excWord|=U_MASK(UCASE_EXC_CLOSURE);
    }

    /* lengths of full case mapping strings, stored in the last slot */
    fullLengths=0;
    if(p->specialCasing!=NULL) {
        fullLengths=p->specialCasing->lowerCase[0];
        fullLengths|=p->specialCasing->upperCase[0]<<8;
        fullLengths|=p->specialCasing->titleCase[0]<<12;
    }
    if(p->caseFolding!=NULL) {
        fullLengths|=p->caseFolding->full[0]<<4;
    }
    if(fullLengths!=0) {
        slots[count]=fullLengths;
        slotBits|=slots[count];
        ++count;
        excWord|=U_MASK(UCASE_EXC_FULL_MAPPINGS);
    }

    /* write slots */
    doubleSlots=(UBool)(slotBits>0xffff);
    if(!doubleSlots) {
        for(i=0; i<count; ++i) {
            exceptions[excTop++]=(uint16_t)slots[i];
        }
    } else {
        excWord|=UCASE_EXC_DOUBLE_SLOTS;
        for(i=0; i<count; ++i) {
            exceptions[excTop++]=(uint16_t)(slots[i]>>16);
            exceptions[excTop++]=(uint16_t)slots[i];
        }
    }

    /* write the full case mapping strings */
    if(p->specialCasing!=NULL) {
        length=(uint16_t)p->specialCasing->lowerCase[0];
        u_memcpy((UChar *)exceptions+excTop, p->specialCasing->lowerCase+1, length);
        excTop+=length;
    }
    if(p->caseFolding!=NULL) {
        length=(uint16_t)p->caseFolding->full[0];
        u_memcpy((UChar *)exceptions+excTop, p->caseFolding->full+1, length);
        excTop+=length;
    }
    if(p->specialCasing!=NULL) {
        length=(uint16_t)p->specialCasing->upperCase[0];
        u_memcpy((UChar *)exceptions+excTop, p->specialCasing->upperCase+1, length);
        excTop+=length;

        length=(uint16_t)p->specialCasing->titleCase[0];
        u_memcpy((UChar *)exceptions+excTop, p->specialCasing->titleCase+1, length);
        excTop+=length;
    }

    /* write the closure data */
    if(p->closure[0]!=0) {
        UChar32 c;

        for(i=0; i<LENGTHOF(p->closure) && (c=p->closure[i])!=0; ++i) {
            U16_APPEND_UNSAFE((UChar *)exceptions, excTop, c);
        }
    }

    exceptionsTop=excTop;

    /* write the main exceptions word */
    exceptions[excIndex]=excWord;

    return excIndex;
}

extern void
makeExceptions() {
    uint32_t *row;
    uint32_t value;
    int32_t i;
    uint16_t excIndex;

    i=0;
    while((row=upvec_getRow(pv, i, NULL, NULL))!=NULL) {
        value=*row;
        if(value&UCASE_EXCEPTION) {
            excIndex=makeException(value, excProps+(value>>UGENCASE_EXC_SHIFT));
            *row=(value&~(UGENCASE_EXC_MASK|UCASE_EXC_MASK))|(excIndex<<UCASE_EXC_SHIFT);
        }
        ++i;
    }
}

/* generate output data ----------------------------------------------------- */

extern void
generateData(const char *dataDir, UBool csource) {
    static int32_t indexes[UCASE_IX_TOP]={
        UCASE_IX_TOP
    };
    static uint8_t trieBlock[40000];

    const uint32_t *row;
    UChar32 start, limit;
    int32_t i;

    UNewDataMemory *pData;
    UNewTrie *pTrie;
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t trieSize;
    long dataLength;

    pTrie=utrie_open(NULL, NULL, 20000, 0, 0, TRUE);
    if(pTrie==NULL) {
        fprintf(stderr, "gencase error: unable to create a UNewTrie\n");
        exit(U_MEMORY_ALLOCATION_ERROR);
    }

    for(i=0; (row=upvec_getRow(pv, i, &start, &limit))!=NULL; ++i) {
        if(!utrie_setRange32(pTrie, start, limit, *row, TRUE)) {
            fprintf(stderr, "gencase error: unable to set trie value (overflow)\n");
            exit(U_BUFFER_OVERFLOW_ERROR);
        }
    }

    trieSize=utrie_serialize(pTrie, trieBlock, sizeof(trieBlock), NULL, TRUE, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "error: utrie_serialize failed: %s (length %ld)\n", u_errorName(errorCode), (long)trieSize);
        exit(errorCode);
    }

    indexes[UCASE_IX_EXC_LENGTH]=exceptionsTop;
    indexes[UCASE_IX_TRIE_SIZE]=trieSize;
    indexes[UCASE_IX_UNFOLD_LENGTH]=unfoldTop;
    indexes[UCASE_IX_LENGTH]=(int32_t)sizeof(indexes)+trieSize+2*exceptionsTop+2*unfoldTop;

    indexes[UCASE_IX_MAX_FULL_LENGTH]=maxFullLength;

    if(beVerbose) {
        printf("trie size in bytes:                    %5d\n", (int)trieSize);
        printf("number of code points with exceptions: %5d\n", exceptionsCount);
        printf("size in bytes of exceptions:           %5d\n", 2*exceptionsTop);
        printf("size in bytes of reverse foldings:     %5d\n", 2*unfoldTop);
        printf("data size:                             %5d\n", (int)indexes[UCASE_IX_LENGTH]);
    }

    if(csource) {
        /* write .c file for hardcoded data */
        UTrie trie={ NULL };
        FILE *f;

        utrie_unserialize(&trie, trieBlock, trieSize, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(
                stderr,
                "gencase error: failed to utrie_unserialize(ucase.icu trie) - %s\n",
                u_errorName(errorCode));
            return;
        }

        f=usrc_create(dataDir, "ucase_props_data.c");
        if(f!=NULL) {
            usrc_writeArray(f,
                "static const UVersionInfo ucase_props_dataVersion={",
                dataInfo.dataVersion, 8, 4,
                "};\n\n");
            usrc_writeArray(f,
                "static const int32_t ucase_props_indexes[UCASE_IX_TOP]={",
                indexes, 32, UCASE_IX_TOP,
                "};\n\n");
            usrc_writeUTrieArrays(f,
                "static const uint16_t ucase_props_trieIndex[%ld]={\n", NULL,
                &trie,
                "\n};\n\n");
            usrc_writeArray(f,
                "static const uint16_t ucase_props_exceptions[%ld]={\n",
                exceptions, 16, exceptionsTop,
                "\n};\n\n");
            usrc_writeArray(f,
                "static const uint16_t ucase_props_unfold[%ld]={\n",
                unfold, 16, unfoldTop,
                "\n};\n\n");
            fputs(
                "static const UCaseProps ucase_props_singleton={\n"
                "  NULL,\n"
                "  ucase_props_indexes,\n"
                "  ucase_props_exceptions,\n"
                "  ucase_props_unfold,\n",
                f);
            usrc_writeUTrieStruct(f,
                "  {\n",
                &trie, "ucase_props_trieIndex", NULL, NULL,
                "  },\n");
            usrc_writeArray(f, "  { ", dataInfo.formatVersion, 8, 4, " }\n");
            fputs("};\n", f);
            fclose(f);
        }
    } else {
        /* write the data */
        pData=udata_create(dataDir, UCASE_DATA_TYPE, UCASE_DATA_NAME, &dataInfo,
                        haveCopyright ? U_COPYRIGHT_STRING : NULL, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "gencase: unable to create data memory, %s\n", u_errorName(errorCode));
            exit(errorCode);
        }

        udata_writeBlock(pData, indexes, sizeof(indexes));
        udata_writeBlock(pData, trieBlock, trieSize);
        udata_writeBlock(pData, exceptions, 2*exceptionsTop);
        udata_writeBlock(pData, unfold, 2*unfoldTop);

        /* finish up */
        dataLength=udata_finish(pData, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "gencase: error %d writing the output file\n", errorCode);
            exit(errorCode);
        }

        if(dataLength!=indexes[UCASE_IX_LENGTH]) {
            fprintf(stderr, "gencase: data length %ld != calculated size %d\n",
                dataLength, (int)indexes[UCASE_IX_LENGTH]);
            exit(U_INTERNAL_PROGRAM_ERROR);
        }
    }

    utrie_close(pTrie);
}

/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 *
 */
