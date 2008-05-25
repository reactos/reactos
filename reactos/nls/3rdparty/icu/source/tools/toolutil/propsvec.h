/*
*******************************************************************************
*
*   Copyright (C) 2002-2005, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  propsvec.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2002feb22
*   created by: Markus W. Scherer
*
*   Store additional Unicode character properties in bit set vectors.
*/

#ifndef __UPROPSVEC_H__
#define __UPROPSVEC_H__

#include "unicode/utypes.h"
#include "utrie.h"

/*
 * Unicode Properties Vectors associated with code point ranges.
 * Stored in an array of uint32_t.
 *
 * The array starts with a header, then rows of integers store
 * the range limits and the properties vectors.
 *
 * In each row, row[0] contains the start code point and
 * row[1] contains the limit code point,
 * which is the start of the next range.
 *
 * Initially, there is only one range [0..0x110000[ with values 0.
 *
 * It would be possible to store only one range boundary per row,
 * but self-contained rows allow to later sort them by contents.
 */
enum {
    /* stores number of columns, plus two for start & limit values */
    UPVEC_COLUMNS,
    UPVEC_MAXROWS,
    UPVEC_ROWS,
    /* search optimization: remember last row seen */
    UPVEC_PREV_ROW,
    UPVEC_HEADER_LENGTH
};

U_CAPI uint32_t * U_EXPORT2
upvec_open(int32_t columns, int32_t maxRows);

U_CAPI void U_EXPORT2
upvec_close(uint32_t *pv);

U_CAPI UBool U_EXPORT2
upvec_setValue(uint32_t *pv,
               UChar32 start, UChar32 limit,
               int32_t column,
               uint32_t value, uint32_t mask,
               UErrorCode *pErrorCode);

U_CAPI uint32_t U_EXPORT2
upvec_getValue(uint32_t *pv, UChar32 c, int32_t column);

/*
 * pRangeStart and pRangeLimit can be NULL.
 * @return NULL if rowIndex out of range and for illegal arguments
 */
U_CAPI uint32_t * U_EXPORT2
upvec_getRow(uint32_t *pv, int32_t rowIndex,
             UChar32 *pRangeStart, UChar32 *pRangeLimit);

/*
 * Compact the vectors:
 * - modify the memory
 * - keep only unique vectors
 * - store them contiguously from the beginning of the memory
 * - for each (non-unique) row, call the handler function
 *
 * The handler's rowIndex is the uint32_t index of the row in the compacted
 * memory block.
 * (Therefore, it starts at 0 increases in increments of the columns value.)
 */

typedef void U_CALLCONV
UPVecCompactHandler(void *context,
                    UChar32 start, UChar32 limit,
                    int32_t rowIndex, uint32_t *row, int32_t columns,
                    UErrorCode *pErrorCode);

U_CAPI int32_t U_EXPORT2
upvec_compact(uint32_t *pv, UPVecCompactHandler *handler, void *context, UErrorCode *pErrorCode);

/* context=UNewTrie, stores the rowIndex values into the trie */
U_CAPI void U_CALLCONV
upvec_compactToTrieHandler(void *context,
                           UChar32 start, UChar32 limit,
                           int32_t rowIndex, uint32_t *row, int32_t columns,
                           UErrorCode *pErrorCode);

#endif
