/*
*******************************************************************************
*
*   Copyright (C) 2004-2005, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  store.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2004dec30
*   created by: Markus W. Scherer
*
*   Store Unicode bidi/shaping properties efficiently for
*   random access.
*/

#include <stdio.h>
#include <stdlib.h>
#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "cmemory.h"
#include "cstring.h"
#include "utrie.h"
#include "uarrsort.h"
#include "unicode/udata.h"
#include "unewdata.h"
#include "propsvec.h"
#include "writesrc.h"
#include "ubidi_props.h"
#include "genbidi.h"

#define LENGTHOF(array) (sizeof(array)/sizeof((array)[0]))

/* Unicode bidi/shaping properties file format ---------------------------------

The file format prepared and written here contains several data
structures that store indexes or data.

Before the data contents described below, there are the headers required by
the udata API for loading ICU data. Especially, a UDataInfo structure
precedes the actual data. It contains platform properties values and the
file format version.

The following is a description of format version 1.0 .

The file contains the following structures:

    const int32_t indexes[i0] with values i0, i1, ...:
    (see UBIDI_IX_... constants for names of indexes)

    i0 indexLength; -- length of indexes[] (UBIDI_IX_TOP)
    i1 dataLength; -- length in bytes of the post-header data (incl. indexes[])
    i2 trieSize; -- size in bytes of the bidi/shaping properties trie
    i3 mirrorLength; -- length in uint32_t of the bidi mirroring array

    i4 jgStart; -- first code point with Joining_Group data
    i5 jgLimit; -- limit code point for Joining_Group data

    i6..i14 reservedIndexes; -- reserved values; 0 for now

    i15 maxValues; -- maximum code values for enumerated properties
                      bits 23..16 contain the max value for Joining_Group,
                      otherwise the bits are used like enum fields in the trie word

    Serialized trie, see utrie.h;

    const uint32_t mirrors[mirrorLength];

    const uint8_t jgArray[i5-i4]; -- (i5-i4) is always a multiple of 4

Trie data word:
Bits
15..13  signed delta to bidi mirroring code point
        (add delta to input code point)
        0 no such code point (source maps to itself)
        -3..-1, 1..3 delta
        -4 look in mirrors table
    12  is mirrored
    11  Bidi_Control
    10  Join_Control
 9.. 8  reserved (set to 0)
 7.. 5  Joining_Type
 4.. 0  BiDi category


Mirrors:
Stores some of the bidi mirroring data, where each code point maps to
at most one other.
Most code points do not have a mirroring code point; most that do have a signed
delta stored in the trie data value. Only those where the delta does not fit
into the trie data are stored in this table.

Logically, this is a two-column table with source and mirror code points.

Physically, the table is compressed by taking advantage of the fact that each
mirror code point is also a source code point
(each of them is a mirror of the other).
Therefore, both logical columns contain the same set of code points, which needs
to be stored only once.

The table stores source code points, and also for each the index of its mirror
code point in the same table, in a simple array of uint32_t.
Bits
31..21  index to mirror code point (unsigned)
20.. 0  source code point

The table is sorted by source code points.


Joining_Group array:
The Joining_Group values do not fit into the 16-bit trie, but the data is also
limited to a small range of code points (Arabic and Syriac) and not
well compressible.

The start and limit code points for the range are stored in the indexes[]
array, and the jgArray[] stores a byte for each of these code points,
containing the Joining_Group value.

All code points outside of this range have No_Joining_Group (0).

----------------------------------------------------------------------------- */

/* UDataInfo cf. udata.h */
static UDataInfo dataInfo={
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    U_SIZEOF_UCHAR,
    0,

    /* dataFormat="BiDi" */
    { UBIDI_FMT_0, UBIDI_FMT_1, UBIDI_FMT_2, UBIDI_FMT_3 },
    { 1, 0, UTRIE_SHIFT, UTRIE_INDEX_SHIFT },   /* formatVersion */
    { 4, 0, 1, 0 }                              /* dataVersion */
};

/* exceptions values */
static uint32_t mirrors[UBIDI_MAX_MIRROR_INDEX+1][2];
static uint16_t mirrorTop=0;

/* -------------------------------------------------------------------------- */

extern void
setUnicodeVersion(const char *v) {
    UVersionInfo version;
    u_versionFromString(version, v);
    uprv_memcpy(dataInfo.dataVersion, version, 4);
}

/* bidi mirroring table ----------------------------------------------------- */

extern void
addMirror(UChar32 src, UChar32 mirror) {
    UErrorCode errorCode;
    int32_t delta;

    delta=mirror-src;
    if(delta==0) {
        return; /* mapping to self=no mapping */
    }

    if(delta<UBIDI_MIN_MIRROR_DELTA || UBIDI_MAX_MIRROR_DELTA<delta) {
        /* delta does not fit into the trie properties value, store in the mirrors[] table */
        if(mirrorTop==LENGTHOF(mirrors)) {
            fprintf(stderr, "genbidi error: too many long-distance mirroring mappings\n");
            exit(U_BUFFER_OVERFLOW_ERROR);
        }

        /* possible: search the table so far and see if src is already listed */

        mirrors[mirrorTop][0]=(uint32_t)src;
        mirrors[mirrorTop][1]=(uint32_t)mirror;
        ++mirrorTop;

        /* set an escape marker in src's properties */
        delta=UBIDI_ESC_MIRROR_DELTA;
    }

    errorCode=U_ZERO_ERROR;
    if(
        !upvec_setValue(
            pv, src, src+1, 0,
            (uint32_t)delta<<UBIDI_MIRROR_DELTA_SHIFT, (uint32_t)(-1)<<UBIDI_MIRROR_DELTA_SHIFT,
            &errorCode)
    ) {
        fprintf(stderr, "genbidi error: unable to set mirroring delta, code: %s\n",
                        u_errorName(errorCode));
        exit(errorCode);
    }
}

static int32_t U_CALLCONV
compareMirror(const void *context, const void *left, const void *right) {
    UChar32 l, r;

    l=UBIDI_GET_MIRROR_CODE_POINT(((const uint32_t *)left)[0]);
    r=UBIDI_GET_MIRROR_CODE_POINT(((const uint32_t *)right)[0]);
    return l-r;
}

static void
makeMirror() {
    uint32_t *reducedMirror;
    UErrorCode errorCode;
    int32_t i, j, start, limit, step;
    uint32_t c;

    /* sort the mirroring table by source code points */
    errorCode=U_ZERO_ERROR;
    uprv_sortArray(mirrors, mirrorTop, 8,
                   compareMirror, NULL, FALSE, &errorCode);

    /*
     * reduce the 2-column table to a single column
     * by putting the index to the mirror entry into the source entry
     *
     * first:
     * find each mirror code point in the source column and set each other's indexes
     *
     * second:
     * reduce the table, combine the source code points with their indexes
     * and store as a simple array of uint32_t
     */
    for(i=0; i<mirrorTop; ++i) {
        c=mirrors[i][1]; /* mirror code point */
        if(c>0x1fffff) {
            continue; /* this entry already has an index */
        }

        /* search for the mirror code point in the source column */
        if(c<mirrors[i][0]) {
            /* search before i */
            start=i-1;
            limit=-1;
            step=-1;
        } else {
            start=i+1;
            limit=mirrorTop;
            step=1;
        }

        for(j=start;; j+=step) {
            if(j==limit) {
                fprintf(stderr,
                        "genbidi error: bidi mirror does not roundtrip - %04lx->%04lx->?\n",
                        (long)mirrors[i][0], (long)mirrors[i][1]);
                errorCode=U_ILLEGAL_ARGUMENT_ERROR;
            }
            if(c==mirrors[j][0]) {
                /*
                 * found the mirror code point c in the source column,
                 * set both entries' indexes to each other
                 */
                if(UBIDI_GET_MIRROR_CODE_POINT(mirrors[i][0])!=UBIDI_GET_MIRROR_CODE_POINT(mirrors[j][1])) {
                    /* roundtrip check fails */
                    fprintf(stderr,
                            "genbidi error: bidi mirrors do not roundtrip - %04lx->%04lx->%04lx\n",
                            (long)mirrors[i][0], (long)mirrors[i][1], (long)mirrors[j][1]);
                    errorCode=U_ILLEGAL_ARGUMENT_ERROR;
                } else {
                    mirrors[i][1]|=(uint32_t)j<<UBIDI_MIRROR_INDEX_SHIFT;
                    mirrors[j][1]|=(uint32_t)i<<UBIDI_MIRROR_INDEX_SHIFT;
                }
                break;
            }
        }
    }

    /* now the second step, the actual reduction of the table */
    reducedMirror=mirrors[0];
    for(i=0; i<mirrorTop; ++i) {
        reducedMirror[i]=mirrors[i][0]|(mirrors[i][1]&~0x1fffff);
    }

    if(U_FAILURE(errorCode)) {
        exit(errorCode);
    }
}

/* generate output data ----------------------------------------------------- */

extern void
generateData(const char *dataDir, UBool csource) {
    static int32_t indexes[UBIDI_IX_TOP]={
        UBIDI_IX_TOP
    };
    static uint8_t trieBlock[40000];
    static uint8_t jgArray[0x300]; /* at most for U+0600..U+08FF */

    const uint32_t *row;
    UChar32 start, limit, prev, jgStart;
    int32_t i;

    UNewDataMemory *pData;
    UNewTrie *pTrie;
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t trieSize;
    long dataLength;

    makeMirror();

    pTrie=utrie_open(NULL, NULL, 20000, 0, 0, TRUE);
    if(pTrie==NULL) {
        fprintf(stderr, "genbidi error: unable to create a UNewTrie\n");
        exit(U_MEMORY_ALLOCATION_ERROR);
    }

    prev=jgStart=0;
    for(i=0; (row=upvec_getRow(pv, i, &start, &limit))!=NULL; ++i) {
        /* store most values from vector column 0 in the trie */
        if(!utrie_setRange32(pTrie, start, limit, *row, TRUE)) {
            fprintf(stderr, "genbidi error: unable to set trie value (overflow)\n");
            exit(U_BUFFER_OVERFLOW_ERROR);
        }

        /* store Joining_Group values from vector column 1 in a simple byte array */
        if(row[1]!=0) {
            if(start<0x600 || 0x900<=limit) {
                fprintf(stderr, "genbidi error: Joining_Group for out-of-range code points U+%04lx..U+%04lx\n",
                        (long)start, (long)limit);
                exit(U_ILLEGAL_ARGUMENT_ERROR);
            }

            if(prev==0) {
                /* first code point with any value */
                prev=jgStart=start;
            } else {
                /* add No_Joining_Group for code points between prev and start */
                while(prev<start) {
                    jgArray[prev++ -jgStart]=0;
                }
            }

            /* set Joining_Group value for start..limit */
            while(prev<limit) {
                jgArray[prev++ -jgStart]=(uint8_t)row[1];
            }
        }
    }

    /* finish jgArray, pad to multiple of 4 */
    while((prev-jgStart)&3) {
        jgArray[prev++ -jgStart]=0;
    }
    indexes[UBIDI_IX_JG_START]=jgStart;
    indexes[UBIDI_IX_JG_LIMIT]=prev;

    trieSize=utrie_serialize(pTrie, trieBlock, sizeof(trieBlock), NULL, TRUE, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "genbidi error: utrie_serialize failed: %s (length %ld)\n", u_errorName(errorCode), (long)trieSize);
        exit(errorCode);
    }

    indexes[UBIDI_IX_TRIE_SIZE]=trieSize;
    indexes[UBIDI_IX_MIRROR_LENGTH]=mirrorTop;
    indexes[UBIDI_IX_LENGTH]=
        (int32_t)sizeof(indexes)+
        trieSize+
        4*mirrorTop+
        (prev-jgStart);

    if(beVerbose) {
        printf("trie size in bytes:                    %5d\n", (int)trieSize);
        printf("size in bytes of mirroring table:      %5d\n", (int)(4*mirrorTop));
        printf("length of Joining_Group array:         %5d (U+%04x..U+%04x)\n", (int)(prev-jgStart), (int)jgStart, (int)(prev-1));
        printf("data size:                             %5d\n", (int)indexes[UBIDI_IX_LENGTH]);
    }

    indexes[UBIDI_MAX_VALUES_INDEX]=
        ((int32_t)U_CHAR_DIRECTION_COUNT-1)|
        (((int32_t)U_JT_COUNT-1)<<UBIDI_JT_SHIFT)|
        (((int32_t)U_JG_COUNT-1)<<UBIDI_MAX_JG_SHIFT);

    if(csource) {
        /* write .c file for hardcoded data */
        UTrie trie={ NULL };
        FILE *f;

        utrie_unserialize(&trie, trieBlock, trieSize, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(
                stderr,
                "genbidi error: failed to utrie_unserialize(ubidi.icu trie) - %s\n",
                u_errorName(errorCode));
            return;
        }

        f=usrc_create(dataDir, "ubidi_props_data.c");
        if(f!=NULL) {
            usrc_writeArray(f,
                "static const UVersionInfo ubidi_props_dataVersion={",
                dataInfo.dataVersion, 8, 4,
                "};\n\n");
            usrc_writeArray(f,
                "static const int32_t ubidi_props_indexes[UBIDI_IX_TOP]={",
                indexes, 32, UBIDI_IX_TOP,
                "};\n\n");
            usrc_writeUTrieArrays(f,
                "static const uint16_t ubidi_props_trieIndex[%ld]={\n", NULL,
                &trie,
                "\n};\n\n");
            usrc_writeArray(f,
                "static const uint32_t ubidi_props_mirrors[%ld]={\n",
                mirrors, 32, mirrorTop,
                "\n};\n\n");
            usrc_writeArray(f,
                "static const uint8_t ubidi_props_jgArray[%ld]={\n",
                jgArray, 8, prev-jgStart,
                "\n};\n\n");
            fputs(
                "static const UBiDiProps ubidi_props_singleton={\n"
                "  NULL,\n"
                "  ubidi_props_indexes,\n"
                "  ubidi_props_mirrors,\n"
                "  ubidi_props_jgArray,\n",
                f);
            usrc_writeUTrieStruct(f,
                "  {\n",
                &trie, "ubidi_props_trieIndex", NULL, NULL,
                "  },\n");
            usrc_writeArray(f, "  { ", dataInfo.formatVersion, 8, 4, " }\n");
            fputs("};\n", f);
            fclose(f);
        }
    } else {
        /* write the data */
        pData=udata_create(dataDir, UBIDI_DATA_TYPE, UBIDI_DATA_NAME, &dataInfo,
                        haveCopyright ? U_COPYRIGHT_STRING : NULL, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "genbidi: unable to create data memory, %s\n", u_errorName(errorCode));
            exit(errorCode);
        }

        udata_writeBlock(pData, indexes, sizeof(indexes));
        udata_writeBlock(pData, trieBlock, trieSize);
        udata_writeBlock(pData, mirrors, 4*mirrorTop);
        udata_writeBlock(pData, jgArray, prev-jgStart);

        /* finish up */
        dataLength=udata_finish(pData, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "genbidi: error %d writing the output file\n", errorCode);
            exit(errorCode);
        }

        if(dataLength!=indexes[UBIDI_IX_LENGTH]) {
            fprintf(stderr, "genbidi: data length %ld != calculated size %d\n",
                dataLength, (int)indexes[UBIDI_IX_LENGTH]);
            exit(U_INTERNAL_PROGRAM_ERROR);
        }
    }

    utrie_close(pTrie);
    upvec_close(pv);
}

/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 *
 */
