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
*   created on: 2001may25
*   created by: Markus W. Scherer
*
*   Store Unicode normalization data in a memory-mappable file.
*/

#include <stdio.h>
#include <stdlib.h>
#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "cmemory.h"
#include "cstring.h"
#include "filestrm.h"
#include "unicode/udata.h"
#include "utrie.h"
#include "unicode/uset.h"
#include "toolutil.h"
#include "unewdata.h"
#include "writesrc.h"
#include "unormimp.h"
#include "gennorm.h"

#define DO_DEBUG_OUT 0

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

/*
 * The new implementation of the normalization code loads its data from
 * unorm.icu, which is generated with this gennorm tool.
 * The format of that file is described in unormimp.h .
 */

/* file data ---------------------------------------------------------------- */

#if UCONFIG_NO_NORMALIZATION

/* dummy UDataInfo cf. udata.h */
static UDataInfo dataInfo = {
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    U_SIZEOF_UCHAR,
    0,

    { 0, 0, 0, 0 },                 /* dummy dataFormat */
    { 0, 0, 0, 0 },                 /* dummy formatVersion */
    { 0, 0, 0, 0 }                  /* dummy dataVersion */
};

#else

/* UDataInfo cf. udata.h */
static UDataInfo dataInfo={
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    U_SIZEOF_UCHAR,
    0,

    { 0x4e, 0x6f, 0x72, 0x6d },   /* dataFormat="Norm" */
    { 2, 3, UTRIE_SHIFT, UTRIE_INDEX_SHIFT },   /* formatVersion */
    { 3, 2, 0, 0 }                /* dataVersion (Unicode version) */
};

extern void
setUnicodeVersion(const char *v) {
    UVersionInfo version;
    u_versionFromString(version, v);
    uprv_memcpy(dataInfo.dataVersion, version, 4);
}

static int32_t indexes[_NORM_INDEX_TOP]={ 0 };

/* builder data ------------------------------------------------------------- */

/* modularization flags, see gennorm.h (default to "store everything") */
uint32_t gStoreFlags=0xffffffff;

typedef void EnumTrieFn(void *context, uint32_t code, Norm *norm);

static UNewTrie
    *normTrie,
    *norm32Trie,
    *fcdTrie,
    *auxTrie;

static UToolMemory *normMem, *utf32Mem, *extraMem, *combiningTriplesMem;

static Norm *norms;

/*
 * set a flag for each code point that was seen in decompositions -
 * avoid to decompose ones that have not been used before
 */
static uint32_t haveSeenFlags[256];

/* set of characters with NFD_QC=No (i.e., those with canonical decompositions) */
static USet *nfdQCNoSet;

/* see addCombiningCP() for details */
static uint32_t combiningCPs[2000];

/*
 * after processCombining() this contains for each code point in combiningCPs[]
 * the runtime combining index
 */
static uint16_t combiningIndexes[2000];

/* section limits for combiningCPs[], see addCombiningCP() */
static uint16_t combineFwdTop=0, combineBothTop=0, combineBackTop=0;

/**
 * Structure for a triple of code points, stored in combiningTriplesMem.
 * The lead and trail code points combine into the the combined one,
 * i.e., there is a canonical decomposition of combined-> <lead, trail>.
 *
 * Before processCombining() is called, leadIndex and trailIndex are 0.
 * After processCombining(), they contain the indexes of the lead and trail
 * code point in the combiningCPs[] array.
 * They are then sorted by leadIndex, then trailIndex.
 * They are not sorted by code points.
 */
typedef struct CombiningTriple {
    uint16_t leadIndex, trailIndex;
    uint32_t lead, trail, combined;
} CombiningTriple;

/* 15b in the combining index -> <=0x8000 uint16_t values in the combining table */
static uint16_t combiningTable[0x8000];
static uint16_t combiningTableTop=0;

#define _NORM_MAX_SET_SEARCH_TABLE_LENGTH 0x4000
static uint16_t canonStartSets[_NORM_MAX_CANON_SETS+2*_NORM_MAX_SET_SEARCH_TABLE_LENGTH
                               +10000]; /* +10000 for exclusion sets */
static int32_t canonStartSetsTop=_NORM_SET_INDEX_TOP;
static int32_t canonSetsCount=0;

/* allocate and initialize a Norm unit */
static Norm *
allocNorm() {
    /* allocate Norm */
    Norm *p=(Norm *)utm_alloc(normMem);
    /*
     * The combiningIndex must not be initialized to 0 because 0 is the
     * combiningIndex of the first forward-combining character.
     */
    p->combiningIndex=0xffff;
    return p;
}

extern void
init() {
    uint16_t *p16;

    normTrie = (UNewTrie *)uprv_malloc(sizeof(UNewTrie));
    uprv_memset(normTrie, 0, sizeof(UNewTrie));
    norm32Trie = (UNewTrie *)uprv_malloc(sizeof(UNewTrie));
    uprv_memset(norm32Trie, 0, sizeof(UNewTrie));
    fcdTrie = (UNewTrie *)uprv_malloc(sizeof(UNewTrie));
    uprv_memset(fcdTrie, 0, sizeof(UNewTrie));
    auxTrie = (UNewTrie *)uprv_malloc(sizeof(UNewTrie));
    uprv_memset(auxTrie, 0, sizeof(UNewTrie));

    /* initialize the two tries */
    if(NULL==utrie_open(normTrie, NULL, 30000, 0, 0, FALSE)) {
        fprintf(stderr, "error: failed to initialize tries\n");
        exit(U_MEMORY_ALLOCATION_ERROR);
    }

    /* allocate Norm structures and reset the first one */
    normMem=utm_open("gennorm normalization structs", 20000, 20000, sizeof(Norm));
    norms=allocNorm();

    /* allocate UTF-32 string memory */
    utf32Mem=utm_open("gennorm UTF-32 strings", 30000, 30000, 4);

    /* reset all "have seen" flags */
    uprv_memset(haveSeenFlags, 0, sizeof(haveSeenFlags));

    /* open an empty set */
    nfdQCNoSet=uset_open(1, 0);

    /* allocate extra data memory for UTF-16 decomposition strings and other values */
    extraMem=utm_open("gennorm extra 16-bit memory", _NORM_EXTRA_INDEX_TOP, _NORM_EXTRA_INDEX_TOP, 2);
    /* initialize the extraMem counter for the top of FNC strings */
    p16=(uint16_t *)utm_alloc(extraMem);
    *p16=1;

    /* allocate temporary memory for combining triples */
    combiningTriplesMem=utm_open("gennorm combining triples", 0x4000, 0x4000, sizeof(CombiningTriple));

    /* set the minimum code points for no/maybe quick check values to the end of the BMP */
    indexes[_NORM_INDEX_MIN_NFC_NO_MAYBE]=0xffff;
    indexes[_NORM_INDEX_MIN_NFKC_NO_MAYBE]=0xffff;
    indexes[_NORM_INDEX_MIN_NFD_NO_MAYBE]=0xffff;
    indexes[_NORM_INDEX_MIN_NFKD_NO_MAYBE]=0xffff;

    /* preset the indexes portion of canonStartSets */
    uprv_memset(canonStartSets, 0, _NORM_SET_INDEX_TOP*2);
}

/*
 * get or create a Norm unit;
 * get or create the intermediate trie entries for it as well
 */
static Norm *
createNorm(uint32_t code) {
    Norm *p;
    uint32_t i;

    i=utrie_get32(normTrie, (UChar32)code, NULL);
    if(i!=0) {
        p=norms+i;
    } else {
        /* allocate Norm */
        p=allocNorm();
        if(!utrie_set32(normTrie, (UChar32)code, (uint32_t)(p-norms))) {
            fprintf(stderr, "error: too many normalization entries\n");
            exit(U_BUFFER_OVERFLOW_ERROR);
        }
    }
    return p;
}

/* get an existing Norm unit */
static Norm *
getNorm(uint32_t code) {
    uint32_t i;

    i=utrie_get32(normTrie, (UChar32)code, NULL);
    if(i==0) {
        return NULL;
    }
    return norms+i;
}

/* get the canonical combining class of a character */
static uint8_t
getCCFromCP(uint32_t code) {
    Norm *norm=getNorm(code);
    if(norm==NULL) {
        return 0;
    } else {
        return norm->udataCC;
    }
}

/*
 * enumerate all code points with their Norm structs and call a function for each
 * return the number of code points with data
 */
static uint32_t
enumTrie(EnumTrieFn *fn, void *context) {
    uint32_t count, i;
    UChar32 code;
    UBool isInBlockZero;

    count=0;
    for(code=0; code<=0x10ffff;) {
        i=utrie_get32(normTrie, code, &isInBlockZero);
        if(isInBlockZero) {
            code+=UTRIE_DATA_BLOCK_LENGTH;
        } else {
            if(i!=0) {
                fn(context, (uint32_t)code, norms+i);
                ++count;
            }
            ++code;
        }
    }
    return count;
}

static void
setHaveSeenString(const uint32_t *s, int32_t length) {
    uint32_t c;

    while(length>0) {
        c=*s++;
        haveSeenFlags[(c>>5)&0xff]|=(1<<(c&0x1f));
        --length;
    }
}

#define HAVE_SEEN(c) (haveSeenFlags[((c)>>5)&0xff]&(1<<((c)&0x1f)))

/* handle combining data ---------------------------------------------------- */

/*
 * Insert an entry into combiningCPs[] for the new code point code with its flags.
 * The flags indicate if code combines forward, backward, or both.
 *
 * combiningCPs[] contains three sections:
 * 1. code points that combine forward
 * 2. code points that combine forward and backward
 * 3. code points that combine backward
 *
 * Search for code in the entire array.
 * If it is found and already is in the right section (old flags==new flags)
 * then we are done.
 * If it is found but the flags are different, then remove it,
 * union the old and new flags, and reinsert it into its correct section.
 * If it is not found, then just insert it.
 *
 * Within each section, the code points are not sorted.
 */
static void
addCombiningCP(uint32_t code, uint8_t flags) {
    uint32_t newEntry;
    uint16_t i;

    newEntry=code|((uint32_t)flags<<24);

    /* search for this code point */
    for(i=0; i<combineBackTop; ++i) {
        if(code==(combiningCPs[i]&0xffffff)) {
            /* found it */
            if(newEntry==combiningCPs[i]) {
                return; /* no change */
            }

            /* combine the flags, remove the old entry from the old place, and insert the new one */
            newEntry|=combiningCPs[i];
            if(i!=--combineBackTop) {
                uprv_memmove(combiningCPs+i, combiningCPs+i+1, (combineBackTop-i)*4);
            }
            if(i<combineBothTop) {
                --combineBothTop;
            }
            if(i<combineFwdTop) {
                --combineFwdTop;
            }
            break;
        }
    }

    /* not found or modified, insert it */
    if(combineBackTop>=sizeof(combiningCPs)/4) {
        fprintf(stderr, "error: gennorm combining code points - trying to use more than %ld units\n",
                (long)(sizeof(combiningCPs)/4));
        exit(U_MEMORY_ALLOCATION_ERROR);
    }

    /* set i to the insertion point */
    flags=(uint8_t)(newEntry>>24);
    if(flags==1) {
        i=combineFwdTop++;
        ++combineBothTop;
    } else if(flags==3) {
        i=combineBothTop++;
    } else /* flags==2 */ {
        i=combineBackTop;
    }

    /* move the following code points up one and insert newEntry at i */
    if(i<combineBackTop) {
        uprv_memmove(combiningCPs+i+1, combiningCPs+i, (combineBackTop-i)*4);
    }
    combiningCPs[i]=newEntry;

    /* finally increment the total counter */
    ++combineBackTop;
}

/**
 * Find the index in combiningCPs[] where code point code is stored.
 * @param code code point to look for
 * @param isLead is code a forward combining code point?
 * @return index in combiningCPs[] where code is stored
 */
static uint16_t
findCombiningCP(uint32_t code, UBool isLead) {
    uint16_t i, limit;

    if(isLead) {
        i=0;
        limit=combineBothTop;
    } else {
        i=combineFwdTop;
        limit=combineBackTop;
    }

    /* search for this code point */
    for(; i<limit; ++i) {
        if(code==(combiningCPs[i]&0xffffff)) {
            /* found it */
            return i;
        }
    }

    /* not found */
    return 0xffff;
}

static void
addCombiningTriple(uint32_t lead, uint32_t trail, uint32_t combined) {
    CombiningTriple *triple;

    if(DO_NOT_STORE(UGENNORM_STORE_COMPOSITION)) {
        return;
    }

    /*
     * set combiningFlags for the two code points
     * do this after decomposition so that getNorm() above returns NULL
     * if we do not have actual sub-decomposition data for the initial NFD here
     */
    createNorm(lead)->combiningFlags|=1;    /* combines forward */
    createNorm(trail)->combiningFlags|=2;    /* combines backward */

    addCombiningCP(lead, 1);
    addCombiningCP(trail, 2);

    triple=(CombiningTriple *)utm_alloc(combiningTriplesMem);
    triple->lead=lead;
    triple->trail=trail;
    triple->combined=combined;
}

static int
compareTriples(const void *l, const void *r) {
    int diff;
    diff=(int)((CombiningTriple *)l)->leadIndex-
         (int)((CombiningTriple *)r)->leadIndex;
    if(diff==0) {
        diff=(int)((CombiningTriple *)l)->trailIndex-
             (int)((CombiningTriple *)r)->trailIndex;
    }
    return diff;
}

static void
processCombining() {
    CombiningTriple *triples;
    uint16_t *p;
    uint32_t combined;
    uint16_t i, j, count, tableTop, finalIndex, combinesFwd;

    triples=utm_getStart(combiningTriplesMem);

    /* add lead and trail indexes to the triples for sorting */
    count=(uint16_t)utm_countItems(combiningTriplesMem);
    for(i=0; i<count; ++i) {
        /* findCombiningCP() must always find the code point */
        triples[i].leadIndex=findCombiningCP(triples[i].lead, TRUE);
        triples[i].trailIndex=findCombiningCP(triples[i].trail, FALSE);
    }

    /* sort them by leadIndex, trailIndex */
    qsort(triples, count, sizeof(CombiningTriple), compareTriples);

    /* calculate final combining indexes and store them in the Norm entries */
    tableTop=0;
    j=0; /* triples counter */

    /* first, combining indexes of fwd/both characters are indexes into the combiningTable */
    for(i=0; i<combineBothTop; ++i) {
        /* start a new table */

        /* assign combining index */
        createNorm(combiningCPs[i]&0xffffff)->combiningIndex=combiningIndexes[i]=tableTop;

        /* calculate the length of the combining data for this lead code point in the combiningTable */
        while(j<count && i==triples[j].leadIndex) {
            /* count 2 to 3 16-bit units per composition entry (back-index, code point) */
            combined=triples[j++].combined;
            if(combined<=0x1fff) {
                tableTop+=2;
            } else {
                tableTop+=3;
            }
        }
    }

    /* second, combining indexes of back-only characters are simply incremented from here to be unique */
    finalIndex=tableTop;
    for(; i<combineBackTop; ++i) {
        createNorm(combiningCPs[i]&0xffffff)->combiningIndex=combiningIndexes[i]=finalIndex++;
    }

    /* it must be finalIndex<=0x8000 because bit 15 is used in combiningTable as an end-for-this-lead marker */
    if(finalIndex>0x8000) {
        fprintf(stderr, "error: gennorm combining table - trying to use %u units, more than the %ld units available\n",
                tableTop, (long)(sizeof(combiningTable)/4));
        exit(U_MEMORY_ALLOCATION_ERROR);
    }

    combiningTableTop=tableTop;

    /* store the combining data in the combiningTable, with the final indexes from above */
    p=combiningTable;
    j=0; /* triples counter */

    /*
     * this is essentially the same loop as above, but
     * it writes the table data instead of calculating and setting the final indexes;
     * it is necessary to have two passes so that all the final indexes are known before
     * they are written into the table
     */
    for(i=0; i<combineBothTop; ++i) {
        /* start a new table */

        combined=0; /* avoid compiler warning */

        /* store the combining data for this lead code point in the combiningTable */
        while(j<count && i==triples[j].leadIndex) {
            finalIndex=combiningIndexes[triples[j].trailIndex];
            combined=triples[j++].combined;

            /* is combined a starter? (i.e., cc==0 && combines forward) */
            combinesFwd=(uint16_t)((getNorm(combined)->combiningFlags&1)<<13);

            *p++=finalIndex;
            if(combined<=0x1fff) {
                *p++=(uint16_t)(combinesFwd|combined);
            } else if(combined<=0xffff) {
                *p++=(uint16_t)(0x8000|combinesFwd);
                *p++=(uint16_t)combined;
            } else {
                *p++=(uint16_t)(0xc000|combinesFwd|((combined-0x10000)>>10));
                *p++=(uint16_t)(0xdc00|(combined&0x3ff));
            }
        }

        /* set a marker on the last final trail index in this lead's table */
        if(combined<=0x1fff) {
            *(p-2)|=0x8000;
        } else {
            *(p-3)|=0x8000;
        }
    }

    /* post condition: tableTop==(p-combiningTable) */
}

/* processing incoming normalization data ----------------------------------- */

/*
 * Decompose Hangul syllables algorithmically and fill a pseudo-Norm struct.
 * c must be a Hangul syllable code point.
 */
static void
getHangulDecomposition(uint32_t c, Norm *pHangulNorm, uint32_t hangulBuffer[3]) {
    /* Hangul syllable: decompose algorithmically */
    uint32_t c2;
    uint8_t length;

    uprv_memset(pHangulNorm, 0, sizeof(Norm));

    c-=HANGUL_BASE;

    c2=c%JAMO_T_COUNT;
    c/=JAMO_T_COUNT;
    if(c2>0) {
        hangulBuffer[2]=JAMO_T_BASE+c2;
        length=3;
    } else {
        hangulBuffer[2]=0;
        length=2;
    }

    hangulBuffer[1]=JAMO_V_BASE+c%JAMO_V_COUNT;
    hangulBuffer[0]=JAMO_L_BASE+c/JAMO_V_COUNT;

    pHangulNorm->nfd=hangulBuffer;
    pHangulNorm->lenNFD=length;
    if(DO_STORE(UGENNORM_STORE_COMPAT)) {
        pHangulNorm->nfkd=hangulBuffer;
        pHangulNorm->lenNFKD=length;
    }
}

/*
 * decompose the one decomposition further, may generate two decompositions
 * apply all previous characters' decompositions to this one
 */
static void
decompStoreNewNF(uint32_t code, Norm *norm) {
    uint32_t nfd[40], nfkd[40], hangulBuffer[3];
    Norm hangulNorm;

    uint32_t *s32;
    Norm *p;
    uint32_t c;
    int32_t i, length;
    uint8_t lenNFD=0, lenNFKD=0;
    UBool changedNFD=FALSE, changedNFKD=FALSE;

    if((length=norm->lenNFD)!=0) {
        /* always allocate the original string */
        changedNFD=TRUE;
        s32=norm->nfd;
    } else if((length=norm->lenNFKD)!=0) {
        /* always allocate the original string */
        changedNFKD=TRUE;
        s32=norm->nfkd;
    } else {
        /* no decomposition here, nothing to do */
        return;
    }

    /* decompose each code point */
    for(i=0; i<length; ++i) {
        c=s32[i];
        p=getNorm(c);
        if(p==NULL) {
            if(HANGUL_BASE<=c && c<(HANGUL_BASE+HANGUL_COUNT)) {
                getHangulDecomposition(c, &hangulNorm, hangulBuffer);
                p=&hangulNorm;
            } else {
                /* no data, no decomposition */
                nfd[lenNFD++]=c;
                nfkd[lenNFKD++]=c;
                continue;
            }
        }

        /* canonically decompose c */
        if(changedNFD) {
            if(p->lenNFD!=0) {
                uprv_memcpy(nfd+lenNFD, p->nfd, p->lenNFD*4);
                lenNFD+=p->lenNFD;
            } else {
                nfd[lenNFD++]=c;
            }
        }

        /* compatibility-decompose c */
        if(p->lenNFKD!=0) {
            uprv_memcpy(nfkd+lenNFKD, p->nfkd, p->lenNFKD*4);
            lenNFKD+=p->lenNFKD;
            changedNFKD=TRUE;
        } else if(p->lenNFD!=0) {
            uprv_memcpy(nfkd+lenNFKD, p->nfd, p->lenNFD*4);
            lenNFKD+=p->lenNFD;
            /*
             * not  changedNFKD=TRUE;
             * so that we do not store a new nfkd if there was no nfkd string before
             * and we only see canonical decompositions
             */
        } else {
            nfkd[lenNFKD++]=c;
        }
    }

    /* assume that norm->lenNFD==1 or ==2 */
    if(norm->lenNFD==2 && !(norm->combiningFlags&0x80)) {
        addCombiningTriple(s32[0], s32[1], code);
    }

    if(changedNFD) {
        if(lenNFD!=0) {
            s32=utm_allocN(utf32Mem, lenNFD);
            uprv_memcpy(s32, nfd, lenNFD*4);
        } else {
            s32=NULL;
        }
        norm->lenNFD=lenNFD;
        norm->nfd=s32;
        setHaveSeenString(nfd, lenNFD);
    }
    if(changedNFKD) {
        if(lenNFKD!=0) {
            s32=utm_allocN(utf32Mem, lenNFKD);
            uprv_memcpy(s32, nfkd, lenNFKD*4);
        } else {
            s32=NULL;
        }
        norm->lenNFKD=lenNFKD;
        norm->nfkd=s32;
        setHaveSeenString(nfkd, lenNFKD);
    }
}

typedef struct DecompSingle {
    uint32_t c;
    Norm *norm;
} DecompSingle;

/*
 * apply this one character's decompositions (there is at least one!) to
 * all previous characters' decompositions to decompose them further
 */
static void
decompWithSingleFn(void *context, uint32_t code, Norm *norm) {
    uint32_t nfd[40], nfkd[40];
    uint32_t *s32;
    DecompSingle *me=(DecompSingle *)context;
    uint32_t c, myC;
    int32_t i, length;
    uint8_t lenNFD=0, lenNFKD=0, myLenNFD, myLenNFKD;
    UBool changedNFD=FALSE, changedNFKD=FALSE;

    /* get the new character's data */
    myC=me->c;
    myLenNFD=me->norm->lenNFD;
    myLenNFKD=me->norm->lenNFKD;
    /* assume that myC has at least one decomposition */

    if((length=norm->lenNFD)!=0 && myLenNFD!=0) {
        /* apply NFD(myC) to norm->nfd */
        s32=norm->nfd;
        for(i=0; i<length; ++i) {
            c=s32[i];
            if(c==myC) {
                uprv_memcpy(nfd+lenNFD, me->norm->nfd, myLenNFD*4);
                lenNFD+=myLenNFD;
                changedNFD=TRUE;
            } else {
                nfd[lenNFD++]=c;
            }
        }
    }

    if((length=norm->lenNFKD)!=0) {
        /* apply NFD(myC) and NFKD(myC) to norm->nfkd */
        s32=norm->nfkd;
        for(i=0; i<length; ++i) {
            c=s32[i];
            if(c==myC) {
                if(myLenNFKD!=0) {
                    uprv_memcpy(nfkd+lenNFKD, me->norm->nfkd, myLenNFKD*4);
                    lenNFKD+=myLenNFKD;
                } else /* assume myLenNFD!=0 */ {
                    uprv_memcpy(nfkd+lenNFKD, me->norm->nfd, myLenNFD*4);
                    lenNFKD+=myLenNFD;
                }
                changedNFKD=TRUE;
            } else {
                nfkd[lenNFKD++]=c;
            }
        }
    } else if((length=norm->lenNFD)!=0 && myLenNFKD!=0) {
        /* apply NFKD(myC) to norm->nfd, forming a new norm->nfkd */
        s32=norm->nfd;
        for(i=0; i<length; ++i) {
            c=s32[i];
            if(c==myC) {
                uprv_memcpy(nfkd+lenNFKD, me->norm->nfkd, myLenNFKD*4);
                lenNFKD+=myLenNFKD;
                changedNFKD=TRUE;
            } else {
                nfkd[lenNFKD++]=c;
            }
        }
    }

    /* set the new decompositions, forget the old ones */
    if(changedNFD) {
        if(lenNFD!=0) {
            if(lenNFD>norm->lenNFD) {
                s32=utm_allocN(utf32Mem, lenNFD);
            } else {
                s32=norm->nfd;
            }
            uprv_memcpy(s32, nfd, lenNFD*4);
        } else {
            s32=NULL;
        }
        norm->lenNFD=lenNFD;
        norm->nfd=s32;
    }
    if(changedNFKD) {
        if(lenNFKD!=0) {
            if(lenNFKD>norm->lenNFKD) {
                s32=utm_allocN(utf32Mem, lenNFKD);
            } else {
                s32=norm->nfkd;
            }
            uprv_memcpy(s32, nfkd, lenNFKD*4);
        } else {
            s32=NULL;
        }
        norm->lenNFKD=lenNFKD;
        norm->nfkd=s32;
    }
}

/*
 * process the data for one code point listed in UnicodeData;
 * UnicodeData itself never maps a code point to both NFD and NFKD
 */
extern void
storeNorm(uint32_t code, Norm *norm) {
    DecompSingle decompSingle;
    Norm *p;

    if(DO_NOT_STORE(UGENNORM_STORE_COMPAT)) {
        /* ignore compatibility decomposition */
        norm->lenNFKD=0;
    }

    /* copy existing derived normalization properties */
    p=createNorm(code);
    norm->qcFlags=p->qcFlags;
    norm->combiningFlags=p->combiningFlags;
    norm->fncIndex=p->fncIndex;

    /* process the decomposition if there is one here */
    if((norm->lenNFD|norm->lenNFKD)!=0) {
        /* decompose this one decomposition further, may generate two decompositions */
        decompStoreNewNF(code, norm);

        /* has this code point been used in previous decompositions? */
        if(HAVE_SEEN(code)) {
            /* use this decomposition to decompose other decompositions further */
            decompSingle.c=code;
            decompSingle.norm=norm;
            enumTrie(decompWithSingleFn, &decompSingle);
        }
    }

    /* store the data */
    uprv_memcpy(p, norm, sizeof(Norm));
}

extern void
setQCFlags(uint32_t code, uint8_t qcFlags) {
    if(DO_NOT_STORE(UGENNORM_STORE_COMPAT)) {
        /* ignore compatibility decomposition: unset the KC/KD flags */
        qcFlags&=~(_NORM_QC_NFKC|_NORM_QC_NFKD);

        /* set the KC/KD flags to the same values as the C/D flags */
        qcFlags|=qcFlags<<1;
    }
    if(DO_NOT_STORE(UGENNORM_STORE_COMPOSITION)) {
        /* ignore composition data: unset the C/KC flags */
        qcFlags&=~(_NORM_QC_NFC|_NORM_QC_NFKC);

        /* set the C/KC flags to the same values as the D/KD flags */
        qcFlags|=qcFlags>>2;
    }

    createNorm(code)->qcFlags|=qcFlags;

    /* adjust the minimum code point for quick check no/maybe */
    if(code<0xffff) {
        if((qcFlags&_NORM_QC_NFC) && (uint16_t)code<indexes[_NORM_INDEX_MIN_NFC_NO_MAYBE]) {
            indexes[_NORM_INDEX_MIN_NFC_NO_MAYBE]=(uint16_t)code;
        }
        if((qcFlags&_NORM_QC_NFKC) && (uint16_t)code<indexes[_NORM_INDEX_MIN_NFKC_NO_MAYBE]) {
            indexes[_NORM_INDEX_MIN_NFKC_NO_MAYBE]=(uint16_t)code;
        }
        if((qcFlags&_NORM_QC_NFD) && (uint16_t)code<indexes[_NORM_INDEX_MIN_NFD_NO_MAYBE]) {
            indexes[_NORM_INDEX_MIN_NFD_NO_MAYBE]=(uint16_t)code;
        }
        if((qcFlags&_NORM_QC_NFKD) && (uint16_t)code<indexes[_NORM_INDEX_MIN_NFKD_NO_MAYBE]) {
            indexes[_NORM_INDEX_MIN_NFKD_NO_MAYBE]=(uint16_t)code;
        }
    }

    if(qcFlags&_NORM_QC_NFD) {
        uset_add(nfdQCNoSet, (UChar32)code);
    }
}

extern void
setCompositionExclusion(uint32_t code) {
    if(DO_STORE(UGENNORM_STORE_COMPOSITION)) {
        createNorm(code)->combiningFlags|=0x80;
    }
}

static void
setHangulJamoSpecials() {
    Norm *norm;
    uint32_t c, hangul;

    /*
     * Hangul syllables are algorithmically decomposed into Jamos,
     * and Jamos are algorithmically composed into Hangul syllables.
     * The quick check flags are parsed, except for Hangul.
     */

    /* set Jamo L specials */
    hangul=0xac00;
    for(c=0x1100; c<=0x1112; ++c) {
        norm=createNorm(c);
        norm->specialTag=_NORM_EXTRA_INDEX_TOP+_NORM_EXTRA_JAMO_L;
        if(DO_STORE(UGENNORM_STORE_COMPOSITION)) {
            norm->combiningFlags=1;
        }

        /* for each Jamo L create a set with its associated Hangul block */
        norm->canonStart=uset_open(hangul, hangul+21*28-1);
        hangul+=21*28;
    }

    /* set Jamo V specials */
    for(c=0x1161; c<=0x1175; ++c) {
        norm=createNorm(c);
        norm->specialTag=_NORM_EXTRA_INDEX_TOP+_NORM_EXTRA_JAMO_V;
        if(DO_STORE(UGENNORM_STORE_COMPOSITION)) {
            norm->combiningFlags=2;
        }
        norm->unsafeStart=TRUE;
    }

    /* set Jamo T specials */
    for(c=0x11a8; c<=0x11c2; ++c) {
        norm=createNorm(c);
        norm->specialTag=_NORM_EXTRA_INDEX_TOP+_NORM_EXTRA_JAMO_T;
        if(DO_STORE(UGENNORM_STORE_COMPOSITION)) {
            norm->combiningFlags=2;
        }
        norm->unsafeStart=TRUE;
    }

    /* set Hangul specials, precompacted */
    norm=allocNorm();
    norm->specialTag=_NORM_EXTRA_INDEX_TOP+_NORM_EXTRA_HANGUL;
    if(DO_STORE(UGENNORM_STORE_COMPAT)) {
        norm->qcFlags=_NORM_QC_NFD|_NORM_QC_NFKD;
    } else {
        norm->qcFlags=_NORM_QC_NFD;
    }

    if(!utrie_setRange32(normTrie, 0xac00, 0xd7a4, (uint32_t)(norm-norms), TRUE)) {
        fprintf(stderr, "error: too many normalization entries (setting Hangul)\n");
        exit(U_BUFFER_OVERFLOW_ERROR);
    }
}

/*
 * set FC-NFKC-Closure string
 * s contains the closure string; s[0]==length, s[1..length] is the actual string
 * may modify s[0]
 */
U_CFUNC void
setFNC(uint32_t c, UChar *s) {
    uint16_t *p;
    int32_t length, i, count;
    UChar first;

    if( DO_NOT_STORE(UGENNORM_STORE_COMPAT) ||
        DO_NOT_STORE(UGENNORM_STORE_COMPOSITION) ||
        DO_NOT_STORE(UGENNORM_STORE_AUX)
    ) {
        return;
    }

    count=utm_countItems(extraMem);
    length=s[0];
    first=s[1];

    /* try to overlay single-unit strings with existing ones */
    if(length==1 && first<0xff00) {
        p=utm_getStart(extraMem);
        for(i=1; i<count; ++i) {
            if(first==p[i]) {
                break;
            }
        }
    } else {
        i=count;
    }

    /* append the new string if it cannot be overlayed with an old one */
    if(i==count) {
        if(count>_NORM_AUX_MAX_FNC) {
            fprintf(stderr, "gennorm error: too many FNC strings\n");
            exit(U_INDEX_OUTOFBOUNDS_ERROR);
        }

        /* prepend 0xffxx with xx==length */
        s[0]=(uint16_t)(0xff00+length);
        ++length;
        p=(uint16_t *)utm_allocN(extraMem, length);
        uprv_memcpy(p, s, length*2);

        /* update the top index in extraMem[0] */
        count+=length;
        ((uint16_t *)utm_getStart(extraMem))[0]=(uint16_t)count;
    }

    /* store the index to the string */
    createNorm(c)->fncIndex=i;
}

/* build runtime structures ------------------------------------------------- */

/* canonically reorder a UTF-32 string; return { leadCC, trailCC } */
static uint16_t
reorderString(uint32_t *s, int32_t length) {
    uint8_t ccs[40];
    uint32_t c;
    int32_t i, j;
    uint8_t cc, prevCC;

    if(length<=0) {
        return 0;
    }

    for(i=0; i<length; ++i) {
        /* get the i-th code point and its combining class */
        c=s[i];
        cc=getCCFromCP(c);
        if(cc!=0 && i!=0) {
            /* it is a combining mark, see if it needs to be moved back */
            j=i;
            do {
                prevCC=ccs[j-1];
                if(prevCC<=cc) {
                    break;  /* found the right place */
                }
                /* move the previous code point here and go back */
                s[j]=s[j-1];
                ccs[j]=prevCC;
            } while(--j!=0);
            s[j]=c;
            ccs[j]=cc;
        } else {
            /* just store the combining class */
            ccs[i]=cc;
        }
    }

    return (uint16_t)(((uint16_t)ccs[0]<<8)|ccs[length-1]);
}

#if 0
static UBool combineAndQC[64]={ 0 };
#endif

/*
 * canonically reorder the up to two decompositions
 * and store the leading and trailing combining classes accordingly
 *
 * also process canonical decompositions for canonical closure
 */
static void
postParseFn(void *context, uint32_t code, Norm *norm) {
    int32_t length;

    /* canonically order the NFD */
    length=norm->lenNFD;
    if(length>0) {
        norm->canonBothCCs=reorderString(norm->nfd, length);
    }

    /* canonically reorder the NFKD */
    length=norm->lenNFKD;
    if(length>0) {
        norm->compatBothCCs=reorderString(norm->nfkd, length);
    }

    /* verify that code has a decomposition if and only if the quick check flags say "no" on NF(K)D */
    if((norm->lenNFD!=0) != ((norm->qcFlags&_NORM_QC_NFD)!=0)) {
        fprintf(stderr, "gennorm warning: U+%04lx has NFD[%d] but quick check 0x%02x\n", (long)code, norm->lenNFD, norm->qcFlags);
    }
    if(((norm->lenNFD|norm->lenNFKD)!=0) != ((norm->qcFlags&(_NORM_QC_NFD|_NORM_QC_NFKD))!=0)) {
        fprintf(stderr, "gennorm warning: U+%04lx has NFD[%d] NFKD[%d] but quick check 0x%02x\n", (long)code, norm->lenNFD, norm->lenNFKD, norm->qcFlags);
    }

    /* see which combinations of combiningFlags and qcFlags are used for NFC/NFKC */
#if 0
    combineAndQC[(norm->qcFlags&0x33)|((norm->combiningFlags&3)<<2)]=1;
#endif

    if(norm->combiningFlags&1) {
        if(norm->udataCC!=0) {
            /* illegal - data-derivable composition exclusion */
            fprintf(stderr, "gennorm warning: U+%04lx combines forward but udataCC==%u\n", (long)code, norm->udataCC);
        }
    }
    if(norm->combiningFlags&2) {
        if((norm->qcFlags&0x11)==0) {
            fprintf(stderr, "gennorm warning: U+%04lx combines backward but qcNF?C==0\n", (long)code);
        }
#if 0
        /* occurs sometimes, this one is ok (therefore #if 0) - still here for documentation */
        if(norm->udataCC==0) {
            printf("U+%04lx combines backward but udataCC==0\n", (long)code);
        }
#endif
    }
    if((norm->combiningFlags&3)==3 && beVerbose) {
        printf("U+%04lx combines both ways\n", (long)code);
    }

    /*
     * process canonical decompositions for canonical closure
     *
     * in each canonical decomposition:
     *   add the current character (code) to the set of canonical starters of its norm->nfd[0]
     *   set the "unsafe starter" flag for each norm->nfd[1..]
     */
    length=norm->lenNFD;
    if(length>0) {
        Norm *otherNorm;
        UChar32 c;
        int32_t i;

        /* nfd[0].canonStart.add(code) */
        c=norm->nfd[0];
        otherNorm=createNorm(c);
        if(otherNorm->canonStart==NULL) {
            otherNorm->canonStart=uset_open(code, code);
            if(otherNorm->canonStart==NULL) {
                fprintf(stderr, "gennorm error: out of memory in uset_open()\n");
                exit(U_MEMORY_ALLOCATION_ERROR);
            }
        } else {
            uset_add(otherNorm->canonStart, code);
            if(!uset_contains(otherNorm->canonStart, code)) {
                fprintf(stderr, "gennorm error: uset_add(setOf(U+%4x), U+%4x)\n", (int)c, (int)code);
                exit(U_INTERNAL_PROGRAM_ERROR);
            }
        }

        /* for(i=1..length-1) nfd[i].unsafeStart=TRUE */
        for(i=1; i<length; ++i) {
            createNorm(norm->nfd[i])->unsafeStart=TRUE;
        }
    }
}

static uint32_t
make32BitNorm(Norm *norm) {
    UChar extra[100];
    const Norm *other;
    uint32_t word;
    int32_t i, length, beforeZero=0, count, start;

    /*
     * Check for assumptions:
     *
     * Test that if a "true starter" (cc==0 && NF*C_YES) decomposes,
     * then the decomposition also begins with a true starter.
     */
    if(norm->udataCC==0) {
        /* this is a starter */
        if((norm->qcFlags&_NORM_QC_NFC)==0 && norm->lenNFD>0) {
            /* a "true" NFC starter with a canonical decomposition */
            if( norm->canonBothCCs>=0x100 || /* lead cc!=0 or */
                ((other=getNorm(norm->nfd[0]))!=NULL && (other->qcFlags&_NORM_QC_NFC)!=0) /* nfd[0] not NFC_YES */
            ) {
                fprintf(stderr,
                    "error: true NFC starter canonical decomposition[%u] does not begin\n"
                    "    with a true NFC starter: U+%04lx U+%04lx%s\n",
                    norm->lenNFD, (long)norm->nfd[0], (long)norm->nfd[1],
                    norm->lenNFD<=2 ? "" : " ...");
                exit(U_INVALID_TABLE_FILE);
            }
        }

        if((norm->qcFlags&_NORM_QC_NFKC)==0) {
            if(norm->lenNFKD>0) {
                /* a "true" NFKC starter with a compatibility decomposition */
                if( norm->compatBothCCs>=0x100 || /* lead cc!=0 or */
                    ((other=getNorm(norm->nfkd[0]))!=NULL && (other->qcFlags&_NORM_QC_NFKC)!=0) /* nfkd[0] not NFKC_YES */
                ) {
                    fprintf(stderr,
                        "error: true NFKC starter compatibility decomposition[%u] does not begin\n"
                        "    with a true NFKC starter: U+%04lx U+%04lx%s\n",
                        norm->lenNFKD, (long)norm->nfkd[0], (long)norm->nfkd[1],
                        norm->lenNFKD<=2 ? "" : " ...");
                    exit(U_INVALID_TABLE_FILE);
                }
            } else if(norm->lenNFD>0) {
                /* a "true" NFKC starter with only a canonical decomposition */
                if( norm->canonBothCCs>=0x100 || /* lead cc!=0 or */
                    ((other=getNorm(norm->nfd[0]))!=NULL && (other->qcFlags&_NORM_QC_NFKC)!=0) /* nfd[0] not NFKC_YES */
                ) {
                    fprintf(stderr,
                        "error: true NFKC starter canonical decomposition[%u] does not begin\n"
                        "    with a true NFKC starter: U+%04lx U+%04lx%s\n",
                        norm->lenNFD, (long)norm->nfd[0], (long)norm->nfd[1],
                        norm->lenNFD<=2 ? "" : " ...");
                    exit(U_INVALID_TABLE_FILE);
                }
            }
        }
    }

    /* reset the 32-bit word and set the quick check flags */
    word=norm->qcFlags;

    /* set the UnicodeData combining class */
    word|=(uint32_t)norm->udataCC<<_NORM_CC_SHIFT;

    /* set the combining flag and index */
    if(norm->combiningFlags&3) {
        word|=(uint32_t)(norm->combiningFlags&3)<<6;
    }

    /* set the combining index value into the extra data */
    /* 0xffff: no combining index; 0..0x7fff: combining index */
    if(norm->combiningIndex!=0xffff) {
        extra[0]=norm->combiningIndex;
        beforeZero=1;
    }

    count=beforeZero;

    /* write the decompositions */
    if((norm->lenNFD|norm->lenNFKD)!=0) {
        extra[count++]=0; /* set the pieces when available, into extra[beforeZero] */

        length=norm->lenNFD;
        if(length>0) {
            if(norm->canonBothCCs!=0) {
                extra[beforeZero]|=0x80;
                extra[count++]=norm->canonBothCCs;
            }
            start=count;
            for(i=0; i<length; ++i) {
                UTF_APPEND_CHAR_UNSAFE(extra, count, norm->nfd[i]);
            }
            extra[beforeZero]|=(UChar)(count-start); /* set the decomp length as the number of UTF-16 code units */
        }

        length=norm->lenNFKD;
        if(length>0) {
            if(norm->compatBothCCs!=0) {
                extra[beforeZero]|=0x8000;
                extra[count++]=norm->compatBothCCs;
            }
            start=count;
            for(i=0; i<length; ++i) {
                UTF_APPEND_CHAR_UNSAFE(extra, count, norm->nfkd[i]);
            }
            extra[beforeZero]|=(UChar)((count-start)<<8); /* set the decomp length as the number of UTF-16 code units */
        }
    }

    /* allocate and copy the extra data */
    if(count!=0) {
        UChar *p;

        if(norm->specialTag!=0) {
            fprintf(stderr, "error: gennorm - illegal to have both extra data and a special tag (0x%x)\n", norm->specialTag);
            exit(U_ILLEGAL_ARGUMENT_ERROR);
        }

        p=(UChar *)utm_allocN(extraMem, count);
        uprv_memcpy(p, extra, count*2);

        /* set the extra index, offset by beforeZero */
        word|=(uint32_t)(beforeZero+(p-(UChar *)utm_getStart(extraMem)))<<_NORM_EXTRA_SHIFT;
    } else if(norm->specialTag!=0) {
        /* set a special tag instead of an extra index */
        word|=(uint32_t)norm->specialTag<<_NORM_EXTRA_SHIFT;
    }

    return word;
}

/* turn all Norm structs into corresponding 32-bit norm values */
static void
makeAll32() {
    uint32_t *pNormData;
    uint32_t n;
    int32_t i, normLength, count;

    count=(int32_t)utm_countItems(normMem);
    for(i=0; i<count; ++i) {
        norms[i].value32=make32BitNorm(norms+i);
    }

    pNormData=utrie_getData(norm32Trie, &normLength);

    count=0; /* count is now just used for debugging */
    for(i=0; i<normLength; ++i) {
        n=pNormData[i];
        if(0!=(pNormData[i]=norms[n].value32)) {
            ++count;
        }
    }
}

/*
 * extract all Norm.canonBothCCs into the FCD table
 * set 32-bit values to use the common fold and compact functions
 */
static void
makeFCD() {
    uint32_t *pFCDData;
    uint32_t n;
    int32_t i, count, fcdLength;
    uint16_t bothCCs;

    count=utm_countItems(normMem);
    for(i=0; i<count; ++i) {
        bothCCs=norms[i].canonBothCCs;
        if(bothCCs==0) {
            /* if there are no decomposition cc's then use the udataCC twice */
            bothCCs=norms[i].udataCC;
            bothCCs|=bothCCs<<8;
        }
        norms[i].value32=bothCCs;
    }

    pFCDData=utrie_getData(fcdTrie, &fcdLength);

    for(i=0; i<fcdLength; ++i) {
        n=pFCDData[i];
        pFCDData[i]=norms[n].value32;
    }
}

/**
 * If the given set contains exactly one character, then return it.
 * Otherwise return -1.
 */
static int32_t
usetContainsOne(const USet* set) {
    if(uset_getItemCount(set)==1) {
        /* there is a single item (a single range) */
        UChar32 start, end;
        UErrorCode ec=U_ZERO_ERROR;
        int32_t len=uset_getItem(set, 0, &start, &end, NULL, 0, &ec);
        if (len==0 && start==end) { /* a range (len==0) with a single code point */
            return start;
        }
    }
    return -1;
}

static void
makeCanonSetFn(void *context, uint32_t code, Norm *norm) {
    if(norm->canonStart!=NULL && !uset_isEmpty(norm->canonStart)) {
        uint16_t *table;
        int32_t c, tableLength;
        UErrorCode errorCode=U_ZERO_ERROR;

        /* does the set contain exactly one code point? */
        c=usetContainsOne(norm->canonStart);

        /* add an entry to the BMP or supplementary search table */
        if(code<=0xffff) {
            table=canonStartSets+_NORM_MAX_CANON_SETS;
            tableLength=canonStartSets[_NORM_SET_INDEX_CANON_BMP_TABLE_LENGTH];

            table[tableLength++]=(uint16_t)code;

            if(c>=0 && c<=0xffff && (c&_NORM_CANON_SET_BMP_MASK)!=_NORM_CANON_SET_BMP_IS_INDEX) {
                /* single-code point BMP result for BMP code point */
                table[tableLength++]=(uint16_t)c;
            } else {
                table[tableLength++]=(uint16_t)(_NORM_CANON_SET_BMP_IS_INDEX|canonStartSetsTop);
                c=-1;
            }
            canonStartSets[_NORM_SET_INDEX_CANON_BMP_TABLE_LENGTH]=(uint16_t)tableLength;
        } else {
            table=canonStartSets+_NORM_MAX_CANON_SETS+_NORM_MAX_SET_SEARCH_TABLE_LENGTH;
            tableLength=canonStartSets[_NORM_SET_INDEX_CANON_SUPP_TABLE_LENGTH];

            table[tableLength++]=(uint16_t)(code>>16);
            table[tableLength++]=(uint16_t)code;

            if(c>=0) {
                /* single-code point result for supplementary code point */
                table[tableLength-2]|=(uint16_t)(0x8000|((c>>8)&0x1f00));
                table[tableLength++]=(uint16_t)c;
            } else {
                table[tableLength++]=(uint16_t)canonStartSetsTop;
            }
            canonStartSets[_NORM_SET_INDEX_CANON_SUPP_TABLE_LENGTH]=(uint16_t)tableLength;
        }

        if(c<0) {
            /* write a USerializedSet */
            ++canonSetsCount;
            canonStartSetsTop+=
                    uset_serialize(norm->canonStart,
                            canonStartSets+canonStartSetsTop,
                            _NORM_MAX_CANON_SETS-canonStartSetsTop,
                            &errorCode);
        }
        canonStartSets[_NORM_SET_INDEX_CANON_SETS_LENGTH]=(uint16_t)canonStartSetsTop;

        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "gennorm error: uset_serialize()->%s (canonStartSetsTop=%d)\n", u_errorName(errorCode), (int)canonStartSetsTop);
            exit(errorCode);
        }
        if(tableLength>_NORM_MAX_SET_SEARCH_TABLE_LENGTH) {
            fprintf(stderr, "gennorm error: search table for canonical starter sets too long\n");
            exit(U_INDEX_OUTOFBOUNDS_ERROR);
        }
    }
}

/* for getSkippableFlags ---------------------------------------------------- */

/* combine the lead and trail code points; return <0 if they do not combine */
static int32_t
combine(uint32_t lead, uint32_t trail) {
    CombiningTriple *triples;
    uint32_t i, count;

    /* search for all triples with c as lead code point */
    triples=utm_getStart(combiningTriplesMem);
    count=utm_countItems(combiningTriplesMem);

    /* triples are not sorted by code point but for each lead CP there is one contiguous block */
    for(i=0; i<count && lead!=triples[i].lead; ++i) {}

    /* check each triple for this code point */
    for(; i<count && lead==triples[i].lead; ++i) {
        if(trail==triples[i].trail) {
            return (int32_t)triples[i].combined;
        }
    }

    return -1;
}

/*
 * Starting from the canonical decomposition s[0..length[ of a single code point,
 * is the code point c consumed in an NFC/FCC recomposition?
 *
 * No need to handle discontiguous composition because that would not consume some
 * intermediate character, so would not compose back to the original character.
 * See comments in canChangeWithFollowing().
 *
 * No need to compose beyond where c canonically orders because if it is consumed
 * then the result differs from the original anyway.
 *
 * Possible optimization:
 * - Verify that there are no cases of the same combining mark stacking twice.
 * - return FALSE right away if c inserts after a copy of itself
 *   without attempting to recompose; will happen because each mark in
 *   the decomposition will be enumerated and passed in as c.
 *   More complicated and fragile though than it is already.
 *
 * markus 2002nov04
 */
static UBool
doesComposeConsume(const uint32_t *s, int32_t length, uint32_t c, uint8_t cc) {
    int32_t starter, i;

    /* ignore trailing characters where cc<prevCC */
    while(length>1 && cc<getCCFromCP(s[length-1])) {
        --length;
    }

    /* start consuming/combining from the beginning */
    starter=(int32_t)s[0];
    for(i=1; i<length; ++i) {
        starter=combine((uint32_t)starter, s[i]);
        if(starter<0) {
            fprintf(stderr, "error: unable to consume normal decomposition in doesComposeConsume(<%04x, %04x, ...>[%d], U+%04x, %u)\n",
                (int)s[0], (int)s[1], (int)length, (int)c, cc);
            exit(U_INTERNAL_PROGRAM_ERROR);
        }
    }

    /* try to combine/consume c, return TRUE if it is consumed */
    return combine((uint32_t)starter, c)>=0;
}

/* does the starter s[0] combine forward with another char that is below trailCC? */
static UBool
canChangeWithFollowing(const uint32_t *s, int32_t length, uint8_t trailCC) {
    if(trailCC<=1) {
        /* no character will combine ahead of the trailing char of the decomposition */
        return FALSE;
    }

    /*
     * We are only checking skippable condition (f).
     * Therefore, the original character does not have quick check flag NFC_NO (c),
     * i.e., the decomposition recomposes completely back into the original code point.
     * So s[0] must be a true starter with cc==0 and
     * combining with following code points.
     *
     * Similarly, length==1 is not possible because that would be a singleton
     * decomposition which is marked with NFC_NO and does not pass (c).
     *
     * Only a character with cc<trailCC can change the composition.
     * Reason: A char with cc>=trailCC would order after decomposition s[],
     * composition would consume all of the decomposition, and here we know that
     * the original char passed check d), i.e., it does not combine forward,
     * therefore does not combine with anything after the decomposition is consumed.
     *
     * Now see if there is a character that
     * 1. combines backward
     * 2. has cc<trailCC
     * 3. is consumed in recomposition
     *
     * length==2 is simple:
     *
     * Characters that fulfill these conditions are exactly the ones that combine directly
     * with the starter c==s[0] because there is no intervening character after
     * reordering.
     * We can just enumerate all chars with which c combines (they all pass 1. and 3.)
     * and see if one has cc<trailCC (passes 2.).
     *
     * length>2 is a little harder:
     *
     * Since we will get different starters during recomposition, we need to
     * enumerate each backward-combining character (1.)
     * with cc<trailCC (2.) and
     * see if it gets consumed in recomposition. (3.)
     * No need to enumerate both-ways combining characters because they must have cc==0.
     */
    if(length==2) {
        /* enumerate all chars that combine with this one and check their cc */
        CombiningTriple *triples;
        uint32_t c, i, count;
        uint8_t cc;

        /* search for all triples with c as lead code point */
        triples=utm_getStart(combiningTriplesMem);
        count=utm_countItems(combiningTriplesMem);
        c=s[0];

        /* triples are not sorted by code point but for each lead CP there is one contiguous block */
        for(i=0; i<count && c!=triples[i].lead; ++i) {}

        /* check each triple for this code point */
        for(; i<count && c==triples[i].lead; ++i) {
            cc=getCCFromCP(triples[i].trail);
            if(cc>0 && cc<trailCC) {
                /* this trail code point combines with c and has cc<trailCC */
                return TRUE;
            }
        }
    } else {
        /* enumerate all chars that combine backward */
        uint32_t c2;
        uint16_t i;
        uint8_t cc;

        for(i=combineBothTop; i<combineBackTop; ++i) {
            c2=combiningCPs[i]&0xffffff;
            cc=getCCFromCP(c2);
            /* pass in length-1 because we already know that c2 will insert before the last character with trailCC */
            if(cc>0 && cc<trailCC && doesComposeConsume(s, length-1, c2, cc)) {
                return TRUE;
            }
        }
    }

    /* this decomposition is not modified by any appended character */
    return FALSE;
}

/* see unormimp.h for details on NF*C Skippable flags */
static uint32_t
getSkippableFlags(const Norm *norm) {
    /* ignore NF*D skippable properties because they are covered by norm32, test at runtime */

    /* ignore Hangul, test those at runtime (LV Hangul are not skippable) */
    if(norm->specialTag==_NORM_EXTRA_INDEX_TOP+_NORM_EXTRA_HANGUL) {
        return 0;
    }

    /* ### TODO check other data generation functions whether they should & do ignore Hangul/Jamo specials */

    /*
     * Note:
     * This function returns a non-zero flag only if (a)..(e) indicate skippable but (f) does not.
     *
     * This means that (a)..(e) must always be derived from the runtime norm32 value,
     * and (f) be checked from the auxTrie if the character is skippable per (a)..(e),
     * the form is NF*C and there is a canonical decomposition (NFD_NO).
     *
     * (a) unassigned code points get "not skippable"==false because they
     * don't have a Norm struct so they won't get here
     */

    /* (b) not skippable if cc!=0 */
    if(norm->udataCC!=0) {
        return 0; /* non-zero flag for (f) only */
    }

    /*
     * not NFC_Skippable if
     * (c) quick check flag == NO  or
     * (d) combines forward  or
     * (e) combines back or
     * (f) can change if another character is added
     *
     * for (f):
     * For NF*C: Get corresponding decomposition, get its last starter (cc==0),
     *           check its composition list,
     *           see if any of the second code points in the list
     *           has cc less than the trailCC of the decomposition.
     *
     * For FCC: Test at runtime if the decomposition has a trailCC>1
     *          -> there are characters with cc==1, they would order before the trail char
     *          and prevent contiguous combination with the trail char.
     */
    if( (norm->qcFlags&(_NORM_QC_NFC&_NORM_QC_ANY_NO))!=0 ||
        (norm->combiningFlags&3)!=0) {
        return 0; /* non-zero flag for (f) only */
    }
    if(norm->lenNFD!=0 && canChangeWithFollowing(norm->nfd, norm->lenNFD, (uint8_t)norm->canonBothCCs)) {
        return _NORM_AUX_NFC_SKIP_F_MASK;
    }

    return 0; /* skippable */
}

static void
makeAux() {
    Norm *norm;
    uint32_t *pData;
    int32_t i, length;

    pData=utrie_getData(auxTrie, &length);

    for(i=0; i<length; ++i) {
        norm=norms+pData[i];
        /*
         * 16-bit auxiliary normalization properties
         * see unormimp.h
         */
        pData[i]=
            ((uint32_t)(norm->combiningFlags&0x80)<<(_NORM_AUX_COMP_EX_SHIFT-7))|
            (uint32_t)norm->fncIndex;

        if(norm->unsafeStart || norm->udataCC!=0) {
            pData[i]|=_NORM_AUX_UNSAFE_MASK;
        }

        pData[i]|=getSkippableFlags(norm);
    }
}

/* folding value for normalization: just store the offset (16 bits) if there is any non-0 entry */
static uint32_t U_CALLCONV
getFoldedNormValue(UNewTrie *trie, UChar32 start, int32_t offset) {
    uint32_t value, leadNorm32=0;
    UChar32 limit;
    UBool inBlockZero;

    limit=start+0x400;
    while(start<limit) {
        value=utrie_get32(trie, start, &inBlockZero);
        if(inBlockZero) {
            start+=UTRIE_DATA_BLOCK_LENGTH;
        } else {
            if(value!=0) {
                leadNorm32|=value;
            }
            ++start;
        }
    }

    /* turn multi-bit fields into the worst-case value */
    if(leadNorm32&_NORM_CC_MASK) {
        leadNorm32|=_NORM_CC_MASK;
    }

    /* clean up unnecessarily ored bit fields */
    leadNorm32&=~((uint32_t)0xffffffff<<_NORM_EXTRA_SHIFT);

    if(leadNorm32==0) {
        /* nothing to do (only composition exclusions?) */
        return 0;
    }

    /* add the extra surrogate index, offset by the BMP top, for the new stage 1 location */
    leadNorm32|=(
        (uint32_t)_NORM_EXTRA_INDEX_TOP+
        (uint32_t)((offset-UTRIE_BMP_INDEX_LENGTH)>>UTRIE_SURROGATE_BLOCK_BITS)
    )<<_NORM_EXTRA_SHIFT;

    return leadNorm32;
}

/* folding value for FCD: use default function (just store the offset (16 bits) if there is any non-0 entry) */

/*
 * folding value for auxiliary data:
 * store the non-zero offset in bits 9..0 (FNC bits)
 * if there is any non-0 entry;
 * "or" [verb!] together data bits 15..10 of all of the 1024 supplementary code points
 */
static uint32_t U_CALLCONV
getFoldedAuxValue(UNewTrie *trie, UChar32 start, int32_t offset) {
    uint32_t value, oredValues;
    UChar32 limit;
    UBool inBlockZero;

    oredValues=0;
    limit=start+0x400;
    while(start<limit) {
        value=utrie_get32(trie, start, &inBlockZero);
        if(inBlockZero) {
            start+=UTRIE_DATA_BLOCK_LENGTH;
        } else {
            oredValues|=value;
            ++start;
        }
    }

    if(oredValues!=0) {
        /* move the 10 significant offset bits into bits 9..0 */
        offset>>=UTRIE_SURROGATE_BLOCK_BITS;
        if(offset>_NORM_AUX_FNC_MASK) {
            fprintf(stderr, "gennorm error: folding offset too large (auxTrie)\n");
            exit(U_INDEX_OUTOFBOUNDS_ERROR);
        }
        return (uint32_t)offset|(oredValues&~_NORM_AUX_FNC_MASK);
    } else {
        return 0;
    }
}

extern void
processData() {
#if 0
    uint16_t i;
#endif

    processCombining();

    /* canonically reorder decompositions and assign combining classes for decompositions */
    enumTrie(postParseFn, NULL);

#if 0
    for(i=1; i<64; ++i) {
        if(combineAndQC[i]) {
            printf("combiningFlags==0x%02x  qcFlags(NF?C)==0x%02x\n", (i&0xc)>>2, i&0x33);
        }
    }
#endif

    /* add hangul/jamo specials */
    setHangulJamoSpecials();

    /* set this value; will be updated as makeCanonSetFn() adds sets (if there are any, see gStoreFlags) */
    canonStartSets[_NORM_SET_INDEX_CANON_SETS_LENGTH]=(uint16_t)canonStartSetsTop;

    /* store search tables and USerializedSets for canonical starters (after Hangul/Jamo specials!) */
    if(DO_STORE(UGENNORM_STORE_AUX) && DO_STORE(UGENNORM_STORE_COMPOSITION)) {
        enumTrie(makeCanonSetFn, NULL);
    }

    /* clone the normalization builder trie to make the final data tries */
    if( NULL==utrie_clone(norm32Trie, normTrie, NULL, 0) ||
        NULL==utrie_clone(fcdTrie, normTrie, NULL, 0) ||
        NULL==utrie_clone(auxTrie, normTrie, NULL, 0)
    ) {
        fprintf(stderr, "error: unable to clone the normalization trie\n");
        exit(U_MEMORY_ALLOCATION_ERROR);
    }

    /* --- finalize data for quick checks & normalization --- */

    /* turn the Norm structs (stage2, norms) into 32-bit data words */
    makeAll32();

    /* --- finalize data for FCD checks --- */

    /* FCD data: take Norm.canonBothCCs and store them in the FCD table */
    makeFCD();

    /* --- finalize auxiliary normalization data --- */
    makeAux();

    if(beVerbose) {
#if 0
        printf("number of stage 2 entries: %ld\n", stage2Mem->index);
        printf("size of stage 1 (BMP) & 2 (uncompacted) + extra data: %ld bytes\n", _NORM_STAGE_1_BMP_COUNT*2+stage2Mem->index*4+extraMem->index*2);
#endif
        printf("combining CPs tops: fwd %u  both %u  back %u\n", combineFwdTop, combineBothTop, combineBackTop);
        printf("combining table count: %u\n", combiningTableTop);
    }
}

#endif /* #if !UCONFIG_NO_NORMALIZATION */

extern void
generateData(const char *dataDir, UBool csource) {
    static uint8_t normTrieBlock[100000], fcdTrieBlock[100000], auxTrieBlock[100000];

    UNewDataMemory *pData;
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t size, dataLength;

#if UCONFIG_NO_NORMALIZATION

    size=0;

#else

    U_STRING_DECL(nxCJKCompatPattern, "[:Ideographic:]", 15);
    U_STRING_DECL(nxUnicode32Pattern, "[:^Age=3.2:]", 12);
    USet *set;
    int32_t normTrieSize, fcdTrieSize, auxTrieSize;

    normTrieSize=utrie_serialize(norm32Trie, normTrieBlock, sizeof(normTrieBlock), getFoldedNormValue, FALSE, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "error: utrie_serialize(normalization properties) failed, %s\n", u_errorName(errorCode));
        exit(errorCode);
    }

    if(DO_STORE(UGENNORM_STORE_FCD)) {
        fcdTrieSize=utrie_serialize(fcdTrie, fcdTrieBlock, sizeof(fcdTrieBlock), NULL, TRUE, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "error: utrie_serialize(FCD data) failed, %s\n", u_errorName(errorCode));
            exit(errorCode);
        }
    } else {
        fcdTrieSize=0;
    }

    if(DO_STORE(UGENNORM_STORE_AUX)) {
        auxTrieSize=utrie_serialize(auxTrie, auxTrieBlock, sizeof(auxTrieBlock), getFoldedAuxValue, TRUE, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "error: utrie_serialize(auxiliary data) failed, %s\n", u_errorName(errorCode));
            exit(errorCode);
        }
    } else {
        auxTrieSize=0;
    }

    /* move the parts of canonStartSets[] together into a contiguous block */
    if( canonStartSetsTop<_NORM_MAX_CANON_SETS &&
        canonStartSets[_NORM_SET_INDEX_CANON_BMP_TABLE_LENGTH]!=0
    ) {
        uprv_memmove(canonStartSets+canonStartSetsTop,
                     canonStartSets+_NORM_MAX_CANON_SETS,
                     canonStartSets[_NORM_SET_INDEX_CANON_BMP_TABLE_LENGTH]*2);
    }
    canonStartSetsTop+=canonStartSets[_NORM_SET_INDEX_CANON_BMP_TABLE_LENGTH];

    if( canonStartSetsTop<(_NORM_MAX_CANON_SETS+_NORM_MAX_SET_SEARCH_TABLE_LENGTH) &&
        canonStartSets[_NORM_SET_INDEX_CANON_SUPP_TABLE_LENGTH]!=0
    ) {
        uprv_memmove(canonStartSets+canonStartSetsTop,
                     canonStartSets+_NORM_MAX_CANON_SETS+_NORM_MAX_SET_SEARCH_TABLE_LENGTH,
                     canonStartSets[_NORM_SET_INDEX_CANON_SUPP_TABLE_LENGTH]*2);
    }
    canonStartSetsTop+=canonStartSets[_NORM_SET_INDEX_CANON_SUPP_TABLE_LENGTH];

    /* create the normalization exclusion sets */
    /*
     * nxCJKCompatPattern should be [[:Ideographic:]&[:NFD_QC=No:]]
     * but we cannot use NFD_QC from the pattern because that would require
     * unorm.icu which we are just going to generate.
     * Therefore we have manually collected nfdQCNoSet and intersect Ideographic
     * with that.
     */
    U_STRING_INIT(nxCJKCompatPattern, "[:Ideographic:]", 15);
    U_STRING_INIT(nxUnicode32Pattern, "[:^Age=3.2:]", 12);

    canonStartSets[_NORM_SET_INDEX_NX_CJK_COMPAT_OFFSET]=canonStartSetsTop;
    set=uset_openPattern(nxCJKCompatPattern, -1, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "error: uset_openPattern([:Ideographic:]&[:NFD_QC=No:]) failed, %s\n", u_errorName(errorCode));
        exit(errorCode);
    }
    uset_retainAll(set, nfdQCNoSet);
    if(DO_NOT_STORE(UGENNORM_STORE_EXCLUSIONS)) {
        uset_clear(set);
    }
    canonStartSetsTop+=uset_serialize(set, canonStartSets+canonStartSetsTop, LENGTHOF(canonStartSets)-canonStartSetsTop, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "error: uset_serialize([:Ideographic:]&[:NFD_QC=No:]) failed, %s\n", u_errorName(errorCode));
        exit(errorCode);
    }
    uset_close(set);

    canonStartSets[_NORM_SET_INDEX_NX_UNICODE32_OFFSET]=canonStartSetsTop;
    set=uset_openPattern(nxUnicode32Pattern, -1, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "error: uset_openPattern([:^Age=3.2:]) failed, %s\n", u_errorName(errorCode));
        exit(errorCode);
    }
    if(DO_NOT_STORE(UGENNORM_STORE_EXCLUSIONS)) {
        uset_clear(set);
    }
    canonStartSetsTop+=uset_serialize(set, canonStartSets+canonStartSetsTop, LENGTHOF(canonStartSets)-canonStartSetsTop, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "error: uset_serialize([:^Age=3.2:]) failed, %s\n", u_errorName(errorCode));
        exit(errorCode);
    }
    uset_close(set);

    canonStartSets[_NORM_SET_INDEX_NX_RESERVED_OFFSET]=canonStartSetsTop;

    /* make sure that the FCD trie is 4-aligned */
    if((utm_countItems(extraMem)+combiningTableTop)&1) {
        combiningTable[combiningTableTop++]=0x1234; /* add one 16-bit word for an even number */
    }

    /* pad canonStartSets to 4-alignment, too */
    if(canonStartSetsTop&1) {
        canonStartSets[canonStartSetsTop++]=0x1235;
    }

    size=
        _NORM_INDEX_TOP*4+
        normTrieSize+
        utm_countItems(extraMem)*2+
        combiningTableTop*2+
        fcdTrieSize+
        auxTrieSize+
        canonStartSetsTop*2;

    if(beVerbose) {
        printf("size of normalization trie              %5u bytes\n", (int)normTrieSize);
        printf("size of 16-bit extra memory             %5u UChars/uint16_t\n", (int)utm_countItems(extraMem));
        printf("  of that: FC_NFKC_Closure size         %5u UChars/uint16_t\n", ((uint16_t *)utm_getStart(extraMem))[0]);
        printf("size of combining table                 %5u uint16_t\n", combiningTableTop);
        printf("size of FCD trie                        %5u bytes\n", (int)fcdTrieSize);
        printf("size of auxiliary trie                  %5u bytes\n", (int)auxTrieSize);
        printf("size of canonStartSets[]                %5u uint16_t\n", (int)canonStartSetsTop);
        printf("  number of indexes                     %5u uint16_t\n", _NORM_SET_INDEX_TOP);
        printf("  size of sets                          %5u uint16_t\n", canonStartSets[_NORM_SET_INDEX_CANON_SETS_LENGTH]-_NORM_SET_INDEX_TOP);
        printf("  number of sets                        %5d\n", (int)canonSetsCount);
        printf("  size of BMP search table              %5u uint16_t\n", canonStartSets[_NORM_SET_INDEX_CANON_BMP_TABLE_LENGTH]);
        printf("  size of supplementary search table    %5u uint16_t\n", canonStartSets[_NORM_SET_INDEX_CANON_SUPP_TABLE_LENGTH]);
        printf("  length of exclusion sets              %5u uint16_t\n", canonStartSets[_NORM_SET_INDEX_NX_RESERVED_OFFSET]-canonStartSets[_NORM_SET_INDEX_NX_CJK_COMPAT_OFFSET]);
        printf("size of " U_ICUDATA_NAME "_" DATA_NAME "." DATA_TYPE " contents: %ld bytes\n", (long)size);
    }

    indexes[_NORM_INDEX_TRIE_SIZE]=normTrieSize;
    indexes[_NORM_INDEX_UCHAR_COUNT]=(uint16_t)utm_countItems(extraMem);

    indexes[_NORM_INDEX_COMBINE_DATA_COUNT]=combiningTableTop;
    indexes[_NORM_INDEX_COMBINE_FWD_COUNT]=combineFwdTop;
    indexes[_NORM_INDEX_COMBINE_BOTH_COUNT]=(uint16_t)(combineBothTop-combineFwdTop);
    indexes[_NORM_INDEX_COMBINE_BACK_COUNT]=(uint16_t)(combineBackTop-combineBothTop);

    /* the quick check minimum code points are already set */

    indexes[_NORM_INDEX_FCD_TRIE_SIZE]=fcdTrieSize;
    indexes[_NORM_INDEX_AUX_TRIE_SIZE]=auxTrieSize;
    indexes[_NORM_INDEX_CANON_SET_COUNT]=canonStartSetsTop;

#endif

    if(csource) {
#if UCONFIG_NO_NORMALIZATION
    /* no csource for dummy mode..? */
    fprintf(stderr, "gennorm error: UCONFIG_NO_NORMALIZATION is on in csource mode.\n");
    exit(1);
#else
        /* write .c file for hardcoded data */
        UTrie normTrie2={ NULL }, fcdTrie2={ NULL }, auxTrie2={ NULL };
        FILE *f;

        utrie_unserialize(&normTrie2, normTrieBlock, normTrieSize, &errorCode);
        if(fcdTrieSize>0) {
            utrie_unserialize(&fcdTrie2, fcdTrieBlock, fcdTrieSize, &errorCode);
        }
        if(auxTrieSize>0) {
            utrie_unserialize(&auxTrie2, auxTrieBlock, auxTrieSize, &errorCode);
        }
        if(U_FAILURE(errorCode)) {
            fprintf(
                stderr,
                "gennorm error: failed to utrie_unserialize() one of the tries - %s\n",
                u_errorName(errorCode));
            exit(errorCode);
        }

        f=usrc_create(dataDir, "unorm_props_data.c");
        if(f!=NULL) {
            usrc_writeArray(f,
                "static const UVersionInfo formatVersion={ ",
                dataInfo.formatVersion, 8, 4,
                " };\n\n");
            usrc_writeArray(f,
                "static const UVersionInfo dataVersion={ ",
                dataInfo.dataVersion, 8, 4,
                " };\n\n");
            usrc_writeArray(f,
                "static const int32_t indexes[_NORM_INDEX_TOP]={\n",
                indexes, 32, _NORM_INDEX_TOP,
                "\n};\n\n");
            usrc_writeUTrieArrays(f,
                "static const uint16_t normTrie_index[%ld]={\n",
                "static const uint32_t normTrie_data32[%ld]={\n",
                &normTrie2,
                "\n};\n\n");
            usrc_writeUTrieStruct(f,
                "static const UTrie normTrie={\n",
                &normTrie2, "normTrie_index", "normTrie_data32", "getFoldingNormOffset",
                "};\n\n");
            usrc_writeArray(f,
                "static const uint16_t extraData[%ld]={\n",
                utm_getStart(extraMem), 16, utm_countItems(extraMem),
                "\n};\n\n");
            usrc_writeArray(f,
                "static const uint16_t combiningTable[%ld]={\n",
                combiningTable, 16, combiningTableTop,
                "\n};\n\n");
            if(fcdTrieSize>0) {
                usrc_writeUTrieArrays(f,
                    "static const uint16_t fcdTrie_index[%ld]={\n", NULL,
                    &fcdTrie2,
                    "\n};\n\n");
                usrc_writeUTrieStruct(f,
                    "static const UTrie fcdTrie={\n",
                    &fcdTrie2, "fcdTrie_index", NULL, NULL,
                    "};\n\n");
            } else {
                fputs( "static const UTrie fcdTrie={ NULL };\n\n", f);
            }
            if(auxTrieSize>0) {
                usrc_writeUTrieArrays(f,
                    "static const uint16_t auxTrie_index[%ld]={\n", NULL,
                    &auxTrie2,
                    "\n};\n\n");
                usrc_writeUTrieStruct(f,
                    "static const UTrie auxTrie={\n",
                    &auxTrie2, "auxTrie_index", NULL, "getFoldingAuxOffset",
                    "};\n\n");
            } else {
                fputs( "static const UTrie auxTrie={ NULL };\n\n", f);
            }
            usrc_writeArray(f,
                "static const uint16_t canonStartSets[%ld]={\n",
                canonStartSets, 16, canonStartSetsTop,
                "\n};\n\n");
            fclose(f);
        }
#endif
    } else {
        /* write the data */
        pData=udata_create(dataDir, DATA_TYPE, DATA_NAME, &dataInfo,
                        haveCopyright ? U_COPYRIGHT_STRING : NULL, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "gennorm: unable to create the output file, error %d\n", errorCode);
            exit(errorCode);
        }

#if !UCONFIG_NO_NORMALIZATION

        udata_writeBlock(pData, indexes, sizeof(indexes));
        udata_writeBlock(pData, normTrieBlock, normTrieSize);
        udata_writeBlock(pData, utm_getStart(extraMem), utm_countItems(extraMem)*2);
        udata_writeBlock(pData, combiningTable, combiningTableTop*2);
        udata_writeBlock(pData, fcdTrieBlock, fcdTrieSize);
        udata_writeBlock(pData, auxTrieBlock, auxTrieSize);
        udata_writeBlock(pData, canonStartSets, canonStartSetsTop*2);

#endif

        /* finish up */
        dataLength=udata_finish(pData, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "gennorm: error %d writing the output file\n", errorCode);
            exit(errorCode);
        }

        if(dataLength!=size) {
            fprintf(stderr, "gennorm error: data length %ld != calculated size %ld\n",
                (long)dataLength, (long)size);
            exit(U_INTERNAL_PROGRAM_ERROR);
        }
    }
}

#if !UCONFIG_NO_NORMALIZATION

extern void
cleanUpData(void) {
    int32_t i, count;

    count=utm_countItems(normMem);
    for(i=0; i<count; ++i) {
        uset_close(norms[i].canonStart);
    }

    utm_close(normMem);
    utm_close(utf32Mem);
    utm_close(extraMem);
    utm_close(combiningTriplesMem);
    utrie_close(normTrie);
    utrie_close(norm32Trie);
    utrie_close(fcdTrie);
    utrie_close(auxTrie);

    uset_close(nfdQCNoSet);

    uprv_free(normTrie);
    uprv_free(norm32Trie);
    uprv_free(fcdTrie);
    uprv_free(auxTrie);
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
