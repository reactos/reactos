/*
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/console/utils.c
 * PURPOSE:         Utility functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "console.h"

#define NDEBUG
#include <debug.h>


/*
 * A function that locates the insertion point (index) for a given value 'Value'
 * in a list 'List' to maintain its sorted order by increasing values.
 *
 * - When 'BisectRightOrLeft' == TRUE, the bisection is performed to the right,
 *   i.e. the returned insertion point comes after (to the right of) any existing
 *   entries of 'Value' in 'List'.
 *   The returned insertion point 'i' partitions the list 'List' into two halves
 *   such that:
 *       all(val <= Value for val in List[start:i[) for the left side, and
 *       all(val >  Value for val in List[i:end+1[) for the right side.
 *
 * - When 'BisectRightOrLeft' == FALSE, the bisection is performed to the left,
 *   i.e. the returned insertion point comes before (to the left of) any existing
 *   entries of 'Value' in 'List'.
 *   The returned insertion point 'i' partitions the list 'List' into two halves
 *   such that:
 *       all(val <  Value for val in List[start:i[) for the left side, and
 *       all(val >= Value for val in List[i:end+1[) for the right side.
 *
 * The exact value of List[i] may, or may not, be equal to Value, depending on
 * whether or not 'Value' is actually present on the list.
 */
UINT
BisectListSortedByValueEx(
    IN PLIST_CTL ListCtl,
    IN ULONG_PTR Value,
    IN UINT itemStart,
    IN UINT itemEnd,
    OUT PUINT pValueItem OPTIONAL,
    IN BOOL BisectRightOrLeft)
{
    UINT iItemStart, iItemEnd, iItem;
    ULONG_PTR itemData;

    /* Sanity checks */
    if (itemStart > itemEnd)
        return CB_ERR; // Fail

    /* Initialize */
    iItemStart = itemStart;
    iItemEnd = itemEnd;
    iItem = iItemStart;

    if (pValueItem)
        *pValueItem = CB_ERR;

    while (iItemStart <= iItemEnd)
    {
        /*
         * Bisect. Note the following:
         * - if iItemEnd == iItemStart + 1, then iItem == iItemStart;
         * - if iItemStart == iItemEnd, then iItemStart == iItem == iItemEnd.
         * In all but the last case, iItemStart <= iItem < iItemEnd.
         */
        iItem = (iItemStart + iItemEnd) / 2;

        itemData = ListCtl->GetData(ListCtl, iItem);
        if (itemData == CB_ERR)
            return CB_ERR; // Fail

        if (Value == itemData)
        {
            /* Found a candidate */
            if (pValueItem)
                *pValueItem = iItem;

            /*
             * Try to find the last element (if BisectRightOrLeft == TRUE)
             * or the first element (if BisectRightOrLeft == FALSE).
             */
            if (BisectRightOrLeft)
            {
                iItemStart = iItem + 1; // iItemStart may be > iItemEnd
            }
            else
            {
                if (iItem <= itemStart) break;
                iItemEnd = iItem - 1;   // iItemEnd may be < iItemStart, i.e. iItemStart may be > iItemEnd
            }
        }
        else if (Value < itemData)
        {
            if (iItem <= itemStart) break;
            /* The value should be before iItem */
            iItemEnd = iItem - 1;   // iItemEnd may be < iItemStart, i.e. iItemStart may be > iItemEnd, if iItem == iItemStart.
        }
        else // if (itemData < Value)
        {
            /* The value should be after iItem */
            iItemStart = iItem + 1; // iItemStart may be > iItemEnd, if iItem == iItemEnd.
        }

        /* Here, iItemStart may be == iItemEnd */
    }

    return iItemStart;
}

UINT
BisectListSortedByValue(
    IN PLIST_CTL ListCtl,
    IN ULONG_PTR Value,
    OUT PUINT pValueItem OPTIONAL,
    IN BOOL BisectRightOrLeft)
{
    INT iItemEnd = ListCtl->GetCount(ListCtl);
    if (iItemEnd == CB_ERR || iItemEnd <= 0)
        return CB_ERR; // Fail

    return BisectListSortedByValueEx(ListCtl, Value,
                                     0, (UINT)(iItemEnd - 1),
                                     pValueItem,
                                     BisectRightOrLeft);
}
