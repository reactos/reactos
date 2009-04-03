/*
*******************************************************************************
*
*   Copyright (C) 1999-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  store.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999dec11
*   created by: Markus W. Scherer
*
*   Store Unicode character properties efficiently for
*   random access.
*/

#include <stdio.h>
#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "cmemory.h"
#include "cstring.h"
#include "utrie.h"
#include "unicode/udata.h"
#include "unewdata.h"
#include "writesrc.h"
#include "uprops.h"
#include "genprops.h"

#define DO_DEBUG_OUT 0

/* Unicode character properties file format ------------------------------------

The file format prepared and written here contains several data
structures that store indexes or data.

Before the data contents described below, there are the headers required by
the udata API for loading ICU data. Especially, a UDataInfo structure
precedes the actual data. It contains platform properties values and the
file format version.

The following is a description of format version 4 .

The format changes between version 3 and 4 because the properties related to
case mappings and bidi/shaping are pulled out into separate files
for modularization.
In order to reduce the need for code changes, some of the previous data
structures are omitted, rather than rearranging everything.

For details see "Changes in format version 4" below.

Data contents:

The contents is a parsed, binary form of several Unicode character
database files, most prominently UnicodeData.txt.

Any Unicode code point from 0 to 0x10ffff can be looked up to get
the properties, if any, for that code point. This means that the input
to the lookup are 21-bit unsigned integers, with not all of the
21-bit range used.

It is assumed that client code keeps a uint32_t pointer
to the beginning of the data:

    const uint32_t *p32;

Formally, the file contains the following structures:

    const int32_t indexes[16] with values i0..i15:

  i0 indicates the length of the main trie.
  i0..i3 all have the same value in format version 4.0;
         the related props32[] and exceptions[] and uchars[] were used in format version 3

    i0 propsIndex; -- 32-bit unit index to the table of 32-bit properties words
    i1 exceptionsIndex;  -- 32-bit unit index to the table of 32-bit exception words
    i2 exceptionsTopIndex; -- 32-bit unit index to the array of UChars for special mappings

    i3 additionalTrieIndex; -- 32-bit unit index to the additional trie for more properties
    i4 additionalVectorsIndex; -- 32-bit unit index to the table of properties vectors
    i5 additionalVectorsColumns; -- number of 32-bit words per properties vector

    i6 reservedItemIndex; -- 32-bit unit index to the top of the properties vectors table
    i7..i9 reservedIndexes; -- reserved values; 0 for now

    i10 maxValues; -- maximum code values for vector word 0, see uprops.h (new in format version 3.1+)
    i11 maxValues2; -- maximum code values for vector word 2, see uprops.h (new in format version 3.2)
    i12..i15 reservedIndexes; -- reserved values; 0 for now

    PT serialized properties trie, see utrie.h (byte size: 4*(i0-16))

  P, E, and U are not used (empty) in format version 4

    P  const uint32_t props32[i1-i0];
    E  const uint32_t exceptions[i2-i1];
    U  const UChar uchars[2*(i3-i2)];

    AT serialized trie for additional properties (byte size: 4*(i4-i3))
    PV const uint32_t propsVectors[(i6-i4)/i5][i5]==uint32_t propsVectors[i6-i4];

Trie lookup and properties:

In order to condense the data for the 21-bit code space, several properties of
the Unicode code assignment are exploited:
- The code space is sparse.
- There are several 10k of consecutive codes with the same properties.
- Characters and scripts are allocated in groups of 16 code points.
- Inside blocks for scripts the properties are often repetitive.
- The 21-bit space is not fully used for Unicode.

The lookup of properties for a given code point is done with a trie lookup,
using the UTrie implementation.
The trie lookup result is a 16-bit properties word.

With a given Unicode code point

    UChar32 c;

and 0<=c<0x110000, the lookup is done like this:

    uint16_t props;
    UTRIE_GET16(trie, c, props);

Each 16-bit properties word contains:

 0.. 4  general category
 5.. 7  numeric type
        non-digit numbers are stored with multiple types and pseudo-types
        in order to facilitate compact encoding:
        0 no numeric value (0)
        1 decimal digit value (0..9)
        2 digit value (0..9)
        3 (U_NT_NUMERIC) normal non-digit numeric value 0..0xff
        4 (internal type UPROPS_NT_FRACTION) fraction
        5 (internal type UPROPS_NT_LARGE) large number >0xff
        6..7 reserved

        when returning the numeric type from a public API,
        internal types must be turned into U_NT_NUMERIC

 8..15  numeric value
        encoding of fractions and large numbers see below

Fractions:
    // n is the 8-bit numeric value from bits 8..15 of the trie word (shifted down)
    int32_t num, den;
    num=n>>3;       // num=0..31
    den=(n&7)+2;    // den=2..9
    if(num==0) {
        num=-1;     // num=-1 or 1..31
    }
    double result=(double)num/(double)den;

Large numbers:
    // n is the 8-bit numeric value from bits 8..15 of the trie word (shifted down)
    int32_t m, e;
    m=n>>4;         // m=0..15
    e=(n&0xf);
    if(m==0) {
        m=1;        // for large powers of 10
        e+=18;      // e=18..33
    } else {
        e+=2;       // e=2..17
    } // m==10..15 are reserved
    double result=(double)m*10^e;

--- Additional properties (new in format version 2.1) ---

The second trie for additional properties (AT) is also a UTrie with 16-bit data.
The data words consist of 32-bit unit indexes (not row indexes!) into the
table of unique properties vectors (PV).
Each vector contains a set of properties.
The width of a vector (number of uint32_t per row) may change
with the formatVersion, it is stored in i5.

Current properties: see icu/source/common/uprops.h

--- Changes in format version 3.1 ---

See i10 maxValues above, contains only UBLOCK_COUNT and USCRIPT_CODE_LIMIT.

--- Changes in format version 3.2 ---

- The tries use linear Latin-1 ranges.
- The additional properties bits store full properties XYZ instead
  of partial Other_XYZ, so that changes in the derivation formulas
  need not be tracked in runtime library code.
- Joining Type and Line Break are also stored completely, so that uprops.c
  needs no runtime formulas for enumerated properties either.
- Store the case-sensitive flag in the main properties word.
- i10 also contains U_LB_COUNT and U_EA_COUNT.
- i11 contains maxValues2 for vector word 2.

--- Changes in format version 4 ---

The format changes between version 3 and 4 because the properties related to
case mappings and bidi/shaping are pulled out into separate files
for modularization.
In order to reduce the need for code changes, some of the previous data
structures are omitted, rather than rearranging everything.

(The change to format version 4 is for ICU 3.4. The last CVS revision of
genprops/store.c for format version 3.2 is 1.48.)

The main trie's data is significantly simplified:
- The trie's 16-bit data word is used directly instead of as an index
  into props32[].
- The trie uses the default trie folding functions instead of custom ones.
- Numeric values are stored directly in the trie data word, with special
  encodings.
- No more exception data (the data that needed it was pulled out, or, in the
  case of numeric values, encoded differently).
- No more string data (pulled out - was for case mappings).

Also, some of the previously used properties vector bits are reserved again.

The indexes[] values for the omitted structures are still filled in
(indicating zero-length arrays) so that the swapper code remains unchanged.

----------------------------------------------------------------------------- */

/* UDataInfo cf. udata.h */
static UDataInfo dataInfo={
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    U_SIZEOF_UCHAR,
    0,

    { 0x55, 0x50, 0x72, 0x6f },                 /* dataFormat="UPro" */
    { 4, 0, UTRIE_SHIFT, UTRIE_INDEX_SHIFT },   /* formatVersion */
    { 4, 0, 1, 0 }                              /* dataVersion */
};

static UNewTrie *pTrie=NULL;

/* -------------------------------------------------------------------------- */

extern void
setUnicodeVersion(const char *v) {
    UVersionInfo version;
    u_versionFromString(version, v);
    uprv_memcpy(dataInfo.dataVersion, version, 4);
}

extern void
initStore() {
    pTrie=utrie_open(NULL, NULL, 40000, 0, 0, TRUE);
    if(pTrie==NULL) {
        fprintf(stderr, "error: unable to create a UNewTrie\n");
        exit(U_MEMORY_ALLOCATION_ERROR);
    }

    initAdditionalProperties();
}

extern void
exitStore() {
    utrie_close(pTrie);
    exitAdditionalProperties();
}

static uint32_t printNumericTypeValueError(Props *p) {
    fprintf(stderr, "genprops error: unable to encode numeric type & value %d  %ld/%lu E%d\n",
            (int)p->numericType, (long)p->numericValue, (unsigned long)p->denominator, p->exponent);
    exit(U_ILLEGAL_ARGUMENT_ERROR);
    return 0;
}

/* store a character's properties ------------------------------------------- */

extern uint32_t
makeProps(Props *p) {
    uint32_t den;
    int32_t type, value, exp;

    /* encode numeric type & value */
    type=p->numericType;
    value=p->numericValue;
    den=p->denominator;
    exp=p->exponent;

    if(den!=0) {
        /* fraction */
        if( type!=U_NT_NUMERIC ||
            value<-1 || value==0 || value>UPROPS_FRACTION_MAX_NUM ||
            den<UPROPS_FRACTION_MIN_DEN || UPROPS_FRACTION_MAX_DEN<den ||
            exp!=0
        ) {
            return printNumericTypeValueError(p);
        }
        type=UPROPS_NT_FRACTION;

        if(value==-1) {
            value=0;
        }
        den-=UPROPS_FRACTION_DEN_OFFSET;
        value=(value<<UPROPS_FRACTION_NUM_SHIFT)|den;
    } else if(exp!=0) {
        /* very large value */
        if( type!=U_NT_NUMERIC ||
            value<1 || 9<value ||
            exp<UPROPS_LARGE_MIN_EXP || UPROPS_LARGE_MAX_EXP_EXTRA<exp
        ) {
            return printNumericTypeValueError(p);
        }
        type=UPROPS_NT_LARGE;

        if(exp<=UPROPS_LARGE_MAX_EXP) {
            /* 1..9 * 10^(2..17) */
            exp-=UPROPS_LARGE_EXP_OFFSET;
        } else {
            /* 1 * 10^(18..33) */
            if(value!=1) {
                return printNumericTypeValueError(p);
            }
            value=0;
            exp-=UPROPS_LARGE_EXP_OFFSET_EXTRA;
        }
        value=(value<<UPROPS_LARGE_MANT_SHIFT)|exp;
    } else if(value>UPROPS_MAX_SMALL_NUMBER) {
        /* large value */
        if(type!=U_NT_NUMERIC) {
            return printNumericTypeValueError(p);
        }
        type=UPROPS_NT_LARGE;

        /* split the value into mantissa and exponent, base 10 */
        while((value%10)==0) {
            value/=10;
            ++exp;
        }
        if(value>9) {
            return printNumericTypeValueError(p);
        }

        exp-=UPROPS_LARGE_EXP_OFFSET;
        value=(value<<UPROPS_LARGE_MANT_SHIFT)|exp;
    } else if(value<0) {
        /* unable to encode negative values, other than fractions -1/x */
        return printNumericTypeValueError(p);

    /* } else normal value=0..0xff { */
    }

    /* encode the properties */
    return
        (uint32_t)p->generalCategory |
        ((uint32_t)type<<UPROPS_NUMERIC_TYPE_SHIFT) |
        ((uint32_t)value<<UPROPS_NUMERIC_VALUE_SHIFT);
}

extern void
addProps(uint32_t c, uint32_t x) {
    if(!utrie_set32(pTrie, (UChar32)c, x)) {
        fprintf(stderr, "error: too many entries for the properties trie\n");
        exit(U_BUFFER_OVERFLOW_ERROR);
    }
}

extern uint32_t
getProps(uint32_t c) {
    return utrie_get32(pTrie, (UChar32)c, NULL);
}

/* areas of same properties ------------------------------------------------- */

extern void
repeatProps(uint32_t first, uint32_t last, uint32_t x) {
    if(!utrie_setRange32(pTrie, (UChar32)first, (UChar32)(last+1), x, FALSE)) {
        fprintf(stderr, "error: too many entries for the properties trie\n");
        exit(U_BUFFER_OVERFLOW_ERROR);
    }
}

/* generate output data ----------------------------------------------------- */

extern void
generateData(const char *dataDir, UBool csource) {
    static int32_t indexes[UPROPS_INDEX_COUNT]={
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    };
    static uint8_t trieBlock[40000];
    static uint8_t additionalProps[120000];

    UNewDataMemory *pData;
    UErrorCode errorCode=U_ZERO_ERROR;
    uint32_t size = 0;
    int32_t trieSize, additionalPropsSize, offset;
    long dataLength;

    trieSize=utrie_serialize(pTrie, trieBlock, sizeof(trieBlock), NULL, TRUE, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "error: utrie_serialize failed: %s (length %ld)\n", u_errorName(errorCode), (long)trieSize);
        exit(errorCode);
    }

    offset=sizeof(indexes)/4;               /* uint32_t offset to the properties trie */

    /* round up trie size to 4-alignment */
    trieSize=(trieSize+3)&~3;
    offset+=trieSize>>2;
    indexes[UPROPS_PROPS32_INDEX]=          /* set indexes to the same offsets for empty */
    indexes[UPROPS_EXCEPTIONS_INDEX]=       /* structures from the old format version 3 */
    indexes[UPROPS_EXCEPTIONS_TOP_INDEX]=   /* so that less runtime code has to be changed */
    indexes[UPROPS_ADDITIONAL_TRIE_INDEX]=offset;

    if(beVerbose) {
        printf("trie size in bytes:                    %5u\n", (int)trieSize);
    }

    if(csource) {
        /* write .c file for hardcoded data */
        UTrie trie={ NULL };
        FILE *f;

        utrie_unserialize(&trie, trieBlock, trieSize, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(
                stderr,
                "genprops error: failed to utrie_unserialize(uprops.icu main trie) - %s\n",
                u_errorName(errorCode));
            return;
        }

        f=usrc_create(dataDir, "uchar_props_data.c");
        if(f!=NULL) {
            usrc_writeArray(f,
                "static const UVersionInfo formatVersion={",
                dataInfo.formatVersion, 8, 4,
                "};\n\n");
            usrc_writeArray(f,
                "static const UVersionInfo dataVersion={",
                dataInfo.dataVersion, 8, 4,
                "};\n\n");
            usrc_writeUTrieArrays(f,
                "static const uint16_t propsTrie_index[%ld]={\n", NULL,
                &trie,
                "\n};\n\n");
            usrc_writeUTrieStruct(f,
                "static const UTrie propsTrie={\n",
                &trie, "propsTrie_index", NULL, NULL,
                "};\n\n");

            additionalPropsSize=writeAdditionalData(f, additionalProps, sizeof(additionalProps), indexes);
            size=4*offset+additionalPropsSize;      /* total size of data */

            usrc_writeArray(f,
                "static const int32_t indexes[UPROPS_INDEX_COUNT]={",
                indexes, 32, UPROPS_INDEX_COUNT,
                "};\n\n");
            fclose(f);
        }
    } else {
        /* write the data */
        pData=udata_create(dataDir, DATA_TYPE, DATA_NAME, &dataInfo,
                        haveCopyright ? U_COPYRIGHT_STRING : NULL, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "genprops: unable to create data memory, %s\n", u_errorName(errorCode));
            exit(errorCode);
        }

        additionalPropsSize=writeAdditionalData(NULL, additionalProps, sizeof(additionalProps), indexes);
        size=4*offset+additionalPropsSize;      /* total size of data */

        udata_writeBlock(pData, indexes, sizeof(indexes));
        udata_writeBlock(pData, trieBlock, trieSize);
        udata_writeBlock(pData, additionalProps, additionalPropsSize);

        /* finish up */
        dataLength=udata_finish(pData, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "genprops: error %d writing the output file\n", errorCode);
            exit(errorCode);
        }

        if(dataLength!=(long)size) {
            fprintf(stderr, "genprops: data length %ld != calculated size %lu\n",
                dataLength, (unsigned long)size);
            exit(U_INTERNAL_PROGRAM_ERROR);
        }
    }

    if(beVerbose) {
        printf("data size:                            %6lu\n", (unsigned long)size);
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
