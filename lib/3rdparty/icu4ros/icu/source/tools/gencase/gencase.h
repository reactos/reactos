/*
*******************************************************************************
*
*   Copyright (C) 2004-2005, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  gencase.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2004aug28
*   created by: Markus W. Scherer
*/

#ifndef __GENCASE_H__
#define __GENCASE_H__

#include "unicode/utypes.h"
#include "utrie.h"
#include "ucase.h"

U_CDECL_BEGIN

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

extern int32_t ucdVersion;

/* gencase ------------------------------------------------------------------ */

#define UGENCASE_EXC_SHIFT     16
#define UGENCASE_EXC_MASK      0xffff0000

/*
 * Values for the ucase.icu unfold[] data array, see store.c.
 * The values are stored in ucase.icu so that the runtime code will work with
 * changing values, but they are hardcoded for gencase for simplicity.
 * They are optimized, that is, provide for minimal table column widths,
 * for the actual Unicode data, so that the table size is minimized.
 * Future versions of Unicode may require increases of some of these values.
 */
enum {
    UGENCASE_UNFOLD_STRING_WIDTH=3,
    UGENCASE_UNFOLD_CP_WIDTH=2,
    UGENCASE_UNFOLD_WIDTH=UGENCASE_UNFOLD_STRING_WIDTH+UGENCASE_UNFOLD_CP_WIDTH,
    UGENCASE_UNFOLD_MAX_ROWS=250
};

/* Values for additional data stored in pv column 1 */
enum {
    UGENCASE_IS_MID_LETTER_SHIFT        /* bit 0 */
};

/* special casing data */
typedef struct {
    UChar32 code;
    UBool isComplex;
    UChar lowerCase[32], upperCase[32], titleCase[32];
} SpecialCasing;

/* case folding data */
typedef struct {
    UChar32 code, simple;
    char status;
    UChar full[32];
} CaseFolding;

/* case mapping properties */
typedef struct {
    UChar32 code, lowerCase, upperCase, titleCase;
    UChar32 closure[8];
    SpecialCasing *specialCasing;
    CaseFolding *caseFolding;
    uint8_t gc, cc;
} Props;

/* global flags */
extern UBool beVerbose, haveCopyright;

/* properties vectors in gencase.c */
extern uint32_t *pv;

/* prototypes */
U_CFUNC void
writeUCDFilename(char *basename, const char *filename, const char *suffix);

U_CFUNC UBool
isToken(const char *token, const char *s);

extern void
setUnicodeVersion(const char *v);

extern void
setProps(Props *p);

U_CFUNC uint32_t U_EXPORT2
getFoldedPropsValue(UNewTrie *trie, UChar32 start, int32_t offset);

extern void
addCaseSensitive(UChar32 first, UChar32 last);

extern void
makeCaseClosure(void);

extern void
makeExceptions(void);

extern void
generateData(const char *dataDir, UBool csource);

U_CDECL_END

#endif
