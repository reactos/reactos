/*
*******************************************************************************
*
*   Copyright (C) 2002-2005, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  propsvec.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2002feb22
*   created by: Markus W. Scherer
*
*   Store additional Unicode character properties in bit set vectors.
*/

#include <stdlib.h>
#include "unicode/utypes.h"
#include "cmemory.h"
#include "utrie.h"
#include "uarrsort.h"
#include "propsvec.h"

static uint32_t *
_findRow(uint32_t *pv, UChar32 rangeStart) {
    uint32_t *row;
    int32_t *hdr;
    int32_t columns, i, start, limit, prevRow, rows;

    hdr=(int32_t *)pv;
    columns=hdr[UPVEC_COLUMNS];
    limit=hdr[UPVEC_ROWS];
    prevRow=hdr[UPVEC_PREV_ROW];
    rows=hdr[UPVEC_ROWS];
    pv+=UPVEC_HEADER_LENGTH;

    /* check the vicinity of the last-seen row */
    if(prevRow<rows) {
        row=pv+prevRow*columns;
        if(rangeStart>=(UChar32)row[0]) {
            if(rangeStart<(UChar32)row[1]) {
                /* same row as last seen */
                return row;
            } else if(
                ++prevRow<rows &&
                rangeStart>=(UChar32)(row+=columns)[0] && rangeStart<(UChar32)row[1]
            ) {
                /* next row after the last one */
                hdr[UPVEC_PREV_ROW]=prevRow;
                return row;
            }
        }
    }

    /* do a binary search for the start of the range */
    start=0;
    while(start<limit-1) {
        i=(start+limit)/2;
        row=pv+i*columns;
        if(rangeStart<(UChar32)row[0]) {
            limit=i;
        } else if(rangeStart<(UChar32)row[1]) {
            hdr[UPVEC_PREV_ROW]=i;
            return row;
        } else {
            start=i;
        }
    }

    /* must be found because all ranges together always cover all of Unicode */
    hdr[UPVEC_PREV_ROW]=start;
    return pv+start*columns;
}

U_CAPI uint32_t * U_EXPORT2
upvec_open(int32_t columns, int32_t maxRows) {
    uint32_t *pv, *row;
    int32_t length;

    if(columns<1 || maxRows<1) {
        return NULL;
    }

    columns+=2; /* count range start and limit columns */
    length=UPVEC_HEADER_LENGTH+maxRows*columns;
    pv=(uint32_t *)uprv_malloc(length*4);
    if(pv!=NULL) {
        /* set header */
        pv[UPVEC_COLUMNS]=(uint32_t)columns;
        pv[UPVEC_MAXROWS]=(uint32_t)maxRows;
        pv[UPVEC_ROWS]=1;
        pv[UPVEC_PREV_ROW]=0;

        /* set initial row */
        row=pv+UPVEC_HEADER_LENGTH;
        *row++=0;
        *row++=0x110000;
        columns-=2;
        do {
            *row++=0;
        } while(--columns>0);
    }
    return pv;
}

U_CAPI void U_EXPORT2
upvec_close(uint32_t *pv) {
    if(pv!=NULL) {
        uprv_free(pv);
    }
}

U_CAPI UBool U_EXPORT2
upvec_setValue(uint32_t *pv,
               UChar32 start, UChar32 limit,
               int32_t column,
               uint32_t value, uint32_t mask,
               UErrorCode *pErrorCode) {
    uint32_t *firstRow, *lastRow;
    int32_t columns;
    UBool splitFirstRow, splitLastRow;

    /* argument checking */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return FALSE;
    }

    if( pv==NULL ||
        start<0 || start>limit || limit>0x110000 ||
        column<0 || (uint32_t)(column+1)>=pv[UPVEC_COLUMNS]
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return FALSE;
    }
    if(start==limit) {
        /* empty range, nothing to do */
        return TRUE;
    }

    /* initialize */
    columns=(int32_t)pv[UPVEC_COLUMNS];
    column+=2; /* skip range start and limit columns */
    value&=mask;

    /* find the rows whose ranges overlap with the input range */

    /* find the first row, always successful */
    firstRow=_findRow(pv, start);

    /* find the last row, always successful */
    lastRow=firstRow;
    while(limit>(UChar32)lastRow[1]) {
        lastRow+=columns;
    }

    /*
     * Rows need to be split if they partially overlap with the
     * input range (only possible for the first and last rows)
     * and if their value differs from the input value.
     */
    splitFirstRow= (UBool)(start!=(UChar32)firstRow[0] && value!=(firstRow[column]&mask));
    splitLastRow= (UBool)(limit!=(UChar32)lastRow[1] && value!=(lastRow[column]&mask));

    /* split first/last rows if necessary */
    if(splitFirstRow || splitLastRow) {
        int32_t count, rows;

        rows=(int32_t)pv[UPVEC_ROWS];
        if((rows+splitFirstRow+splitLastRow)>(int32_t)pv[UPVEC_MAXROWS]) {
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return FALSE;
        }

        /* count the number of row cells to move after the last row, and move them */
        count = (int32_t)((pv+UPVEC_HEADER_LENGTH+rows*columns)-(lastRow+columns));
        if(count>0) {
            uprv_memmove(
                lastRow+(1+splitFirstRow+splitLastRow)*columns,
                lastRow+columns,
                count*4);
        }
        pv[UPVEC_ROWS]=rows+splitFirstRow+splitLastRow;

        /* split the first row, and move the firstRow pointer to the second part */
        if(splitFirstRow) {
            /* copy all affected rows up one and move the lastRow pointer */
            count = (int32_t)((lastRow-firstRow)+columns);
            uprv_memmove(firstRow+columns, firstRow, count*4);
            lastRow+=columns;

            /* split the range and move the firstRow pointer */
            firstRow[1]=firstRow[columns]=(uint32_t)start;
            firstRow+=columns;
        }

        /* split the last row */
        if(splitLastRow) {
            /* copy the last row data */
            uprv_memcpy(lastRow+columns, lastRow, columns*4);

            /* split the range and move the firstRow pointer */
            lastRow[1]=lastRow[columns]=(uint32_t)limit;
        }
    }

    /* set the "row last seen" to the last row for the range */
    pv[UPVEC_PREV_ROW]=(uint32_t)((lastRow-(pv+UPVEC_HEADER_LENGTH))/columns);

    /* set the input value in all remaining rows */
    firstRow+=column;
    lastRow+=column;
    mask=~mask;
    for(;;) {
        *firstRow=(*firstRow&mask)|value;
        if(firstRow==lastRow) {
            break;
        }
        firstRow+=columns;
    }
    return TRUE;
}

U_CAPI uint32_t U_EXPORT2
upvec_getValue(uint32_t *pv, UChar32 c, int32_t column) {
    uint32_t *row;

    if(pv==NULL || c<0 || c>=0x110000) {
        return 0;
    }
    row=_findRow(pv, c);
    return row[2+column];
}

U_CAPI uint32_t * U_EXPORT2
upvec_getRow(uint32_t *pv, int32_t rowIndex,
             UChar32 *pRangeStart, UChar32 *pRangeLimit) {
    uint32_t *row;
    int32_t columns;

    if(pv==NULL || rowIndex<0 || rowIndex>=(int32_t)pv[UPVEC_ROWS]) {
        return NULL;
    }

    columns=(int32_t)pv[UPVEC_COLUMNS];
    row=pv+UPVEC_HEADER_LENGTH+rowIndex*columns;
    if(pRangeStart!=NULL) {
        *pRangeStart=row[0];
    }
    if(pRangeLimit!=NULL) {
        *pRangeLimit=row[1];
    }
    return row+2;
}

static int32_t U_CALLCONV
upvec_compareRows(const void *context, const void *l, const void *r) {
    const uint32_t *left=(const uint32_t *)l, *right=(const uint32_t *)r;
    const uint32_t *pv=(const uint32_t *)context;
    int32_t i, count, columns;

    count=columns=(int32_t)pv[UPVEC_COLUMNS]; /* includes start/limit columns */

    /* start comparing after start/limit but wrap around to them */
    i=2;
    do {
        if(left[i]!=right[i]) {
            return left[i]<right[i] ? -1 : 1;
        }
        if(++i==columns) {
            i=0;
        }
    } while(--count>0);

    return 0;
}

U_CAPI int32_t U_EXPORT2
upvec_compact(uint32_t *pv, UPVecCompactHandler *handler, void *context, UErrorCode *pErrorCode) {
    uint32_t *row;
    int32_t columns, valueColumns, rows, count;
    UChar32 start, limit;

    /* argument checking */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(pv==NULL || handler==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    row=pv+UPVEC_HEADER_LENGTH;
    columns=(int32_t)pv[UPVEC_COLUMNS];
    rows=(int32_t)pv[UPVEC_ROWS];

    if(rows==0) {
        return 0;
    }

    /* sort the properties vectors to find unique vector values */
    if(rows>1) {
        uprv_sortArray(pv+UPVEC_HEADER_LENGTH, rows, columns*4,
                       upvec_compareRows, pv, FALSE, pErrorCode);
    }
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /*
     * Move vector contents up to a contiguous array with only unique
     * vector values, and call the handler function for each vector.
     *
     * This destroys the Properties Vector structure and replaces it
     * with an array of just vector values.
     */
    valueColumns=columns-2; /* not counting start & limit */
    count=-valueColumns;

    do {
        /* fetch these first before memmove() may overwrite them */
        start=(UChar32)row[0];
        limit=(UChar32)row[1];

        /* add a new values vector if it is different from the current one */
        if(count<0 || 0!=uprv_memcmp(row+2, pv+count, valueColumns*4)) {
            count+=valueColumns;
            uprv_memmove(pv+count, row+2, valueColumns*4);
        }

        handler(context, start, limit, count, pv+count, valueColumns, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            return 0;
        }

        row+=columns;
    } while(--rows>0);

    /* count is at the beginning of the last vector, add valueColumns to include that last vector */
    return count+valueColumns;
}

U_CAPI void U_CALLCONV
upvec_compactToTrieHandler(void *context,
                           UChar32 start, UChar32 limit,
                           int32_t rowIndex, uint32_t *row, int32_t columns,
                           UErrorCode *pErrorCode) {
    if(!utrie_setRange32((UNewTrie *)context, start, limit, (uint32_t)rowIndex, FALSE)) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }
}
