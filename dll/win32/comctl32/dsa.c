/*
 * Dynamic structure array (DSA) implementation
 *
 * Copyright 1998 Eric Kohl
 *           1998 Juergen Schmied <j.schmied@metronet.de>
 *           2000 Eric Kohl for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES
 *     These functions were involuntarily documented by Microsoft in 2002 as
 *     the outcome of an anti-trust suit brought by various U.S. governments.
 *     As a result the specifications on MSDN are inaccurate, incomplete 
 *     and misleading. A much more complete (unofficial) documentation is
 *     available at:
 *
 *     http://members.ozemail.com.au/~geoffch/samples/win32/shell/comctl32  
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "commctrl.h"

#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsa);

struct _DSA
{
    INT  nItemCount;
    LPVOID pData;
    INT  nMaxCount;
    INT  nItemSize;
    INT  nGrow;
};

/**************************************************************************
 * DSA_Create [COMCTL32.320]
 *
 * Creates a dynamic storage array
 *
 * PARAMS
 *     nSize [I] size of the array elements
 *     nGrow [I] number of elements by which the array grows when it is filled
 *
 * RETURNS
 *     Success: pointer to an array control structure. Use this like a handle.
 *     Failure: NULL
 *
 * NOTES
 *     The DSA_ functions can be used to create and manipulate arrays of
 *     fixed-size memory blocks. These arrays can store any kind of data
 *     (e.g. strings and icons).
 */
HDSA WINAPI DSA_Create (INT nSize, INT nGrow)
{
    HDSA hdsa;

    TRACE("(size=%d grow=%d)\n", nSize, nGrow);

    hdsa = Alloc (sizeof(*hdsa));
    if (hdsa)
    {
        hdsa->nItemCount = 0;
        hdsa->pData = NULL;
        hdsa->nMaxCount = 0;
        hdsa->nItemSize = nSize;
        hdsa->nGrow = max(1, nGrow);
    }

    return hdsa;
}


/**************************************************************************
 * DSA_Destroy [COMCTL32.321]
 * 
 * Destroys a dynamic storage array
 *
 * PARAMS
 *     hdsa [I] pointer to the array control structure
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI DSA_Destroy (HDSA hdsa)
{
    TRACE("(%p)\n", hdsa);

    if (!hdsa)
        return FALSE;

    if (hdsa->pData && (!Free (hdsa->pData)))
        return FALSE;

    return Free (hdsa);
}


/**************************************************************************
 * DSA_GetItem [COMCTL32.322]
 *
 * Copies the specified item into a caller-supplied buffer.
 *
 * PARAMS
 *     hdsa   [I] pointer to the array control structure
 *     nIndex [I] number of the Item to get
 *     pDest  [O] destination buffer. Has to be >= dwElementSize.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI DSA_GetItem (HDSA hdsa, INT nIndex, LPVOID pDest)
{
    LPVOID pSrc;

    TRACE("(%p %d %p)\n", hdsa, nIndex, pDest);

    if (!hdsa)
        return FALSE;
    if ((nIndex < 0) || (nIndex >= hdsa->nItemCount))
        return FALSE;

    pSrc = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
    memmove (pDest, pSrc, hdsa->nItemSize);

    return TRUE;
}


/**************************************************************************
 * DSA_GetItemPtr [COMCTL32.323]
 *
 * Retrieves a pointer to the specified item.
 *
 * PARAMS
 *     hdsa   [I] pointer to the array control structure
 *     nIndex [I] index of the desired item
 *
 * RETURNS
 *     Success: pointer to an item
 *     Failure: NULL
 */
LPVOID WINAPI DSA_GetItemPtr (HDSA hdsa, INT nIndex)
{
    LPVOID pSrc;

    TRACE("(%p %d)\n", hdsa, nIndex);

    if (!hdsa)
        return NULL;
    if ((nIndex < 0) || (nIndex >= hdsa->nItemCount))
        return NULL;

    pSrc = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);

    TRACE("-- ret=%p\n", pSrc);

    return pSrc;
}


/**************************************************************************
 * DSA_SetItem [COMCTL32.325]
 *
 * Sets the contents of an item in the array.
 *
 * PARAMS
 *     hdsa   [I] pointer to the array control structure
 *     nIndex [I] index for the item
 *     pSrc   [I] pointer to the new item data
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI DSA_SetItem (HDSA hdsa, INT nIndex, LPVOID pSrc)
{
    INT  nSize, nNewItems;
    LPVOID pDest, lpTemp;

    TRACE("(%p %d %p)\n", hdsa, nIndex, pSrc);

    if ((!hdsa) || nIndex < 0)
        return FALSE;

    if (hdsa->nItemCount <= nIndex) {
        /* within the old array */
        if (hdsa->nMaxCount > nIndex) {
            /* within the allocated space, set a new boundary */
            hdsa->nItemCount = nIndex + 1;
        }
        else {
            /* resize the block of memory */
            nNewItems =
                hdsa->nGrow * ((((nIndex + 1) - 1) / hdsa->nGrow) + 1);
            nSize = hdsa->nItemSize * nNewItems;

            lpTemp = ReAlloc (hdsa->pData, nSize);
            if (!lpTemp)
                return FALSE;

            hdsa->nMaxCount = nNewItems;
            hdsa->nItemCount = nIndex + 1;
            hdsa->pData = lpTemp;
        }
    }

    /* put the new entry in */
    pDest = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
    TRACE("-- move dest=%p src=%p size=%d\n",
           pDest, pSrc, hdsa->nItemSize);
    memmove (pDest, pSrc, hdsa->nItemSize);

    return TRUE;
}


/**************************************************************************
 * DSA_InsertItem [COMCTL32.324]
 *
 * Inserts an item into the array at the specified index.
 *
 * PARAMS
 *     hdsa   [I] pointer to the array control structure
 *     nIndex [I] index for the new item
 *     pSrc   [I] pointer to the element
 *
 * RETURNS
 *     Success: position of the new item
 *     Failure: -1
 */
INT WINAPI DSA_InsertItem (HDSA hdsa, INT nIndex, LPVOID pSrc)
{
    INT   nNewItems, nSize;
    LPVOID  lpTemp, lpDest;

    TRACE("(%p %d %p)\n", hdsa, nIndex, pSrc);

    if ((!hdsa) || nIndex < 0)
        return -1;

    /* when nIndex >= nItemCount then append */
    if (nIndex >= hdsa->nItemCount)
         nIndex = hdsa->nItemCount;

    /* do we need to resize ? */
    if (hdsa->nItemCount >= hdsa->nMaxCount) {
        nNewItems = hdsa->nMaxCount + hdsa->nGrow;
        nSize = hdsa->nItemSize * nNewItems;

        if (nSize / hdsa->nItemSize != nNewItems)
            return -1;

        lpTemp = ReAlloc (hdsa->pData, nSize);
        if (!lpTemp)
            return -1;

        hdsa->nMaxCount = nNewItems;
        hdsa->pData = lpTemp;
    }

    /* do we need to move elements ? */
    if (nIndex < hdsa->nItemCount) {
        lpTemp = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
        lpDest = (char *) lpTemp + hdsa->nItemSize;
        nSize = (hdsa->nItemCount - nIndex) * hdsa->nItemSize;
        TRACE("-- move dest=%p src=%p size=%d\n",
               lpDest, lpTemp, nSize);
        memmove (lpDest, lpTemp, nSize);
    }

    /* ok, we can put the new Item in */
    hdsa->nItemCount++;
    lpDest = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
    TRACE("-- move dest=%p src=%p size=%d\n",
           lpDest, pSrc, hdsa->nItemSize);
    memmove (lpDest, pSrc, hdsa->nItemSize);

    return nIndex;
}


/**************************************************************************
 * DSA_DeleteItem [COMCTL32.326]
 *
 * Deletes the specified item from the array.
 *
 * PARAMS
 *     hdsa   [I] pointer to the array control structure
 *     nIndex [I] index for the element to delete
 *
 * RETURNS
 *     Success: number of the deleted element
 *     Failure: -1
 */
INT WINAPI DSA_DeleteItem (HDSA hdsa, INT nIndex)
{
    LPVOID lpDest,lpSrc;
    INT  nSize;

    TRACE("(%p %d)\n", hdsa, nIndex);

    if (!hdsa)
        return -1;
    if (nIndex < 0 || nIndex >= hdsa->nItemCount)
        return -1;

    /* do we need to move ? */
    if (nIndex < hdsa->nItemCount - 1) {
        lpDest = (char *) hdsa->pData + (hdsa->nItemSize * nIndex);
        lpSrc = (char *) lpDest + hdsa->nItemSize;
        nSize = hdsa->nItemSize * (hdsa->nItemCount - nIndex - 1);
        TRACE("-- move dest=%p src=%p size=%d\n",
               lpDest, lpSrc, nSize);
        memmove (lpDest, lpSrc, nSize);
    }

    hdsa->nItemCount--;

    /* free memory ? */
    if ((hdsa->nMaxCount - hdsa->nItemCount) >= hdsa->nGrow) {
        nSize = hdsa->nItemSize * hdsa->nItemCount;

        lpDest = ReAlloc (hdsa->pData, nSize);
        if (!lpDest)
            return -1;

        hdsa->nMaxCount = hdsa->nItemCount;
        hdsa->pData = lpDest;
    }

    return nIndex;
}


/**************************************************************************
 * DSA_DeleteAllItems [COMCTL32.327]
 *
 * Removes all items and reinitializes the array.
 *
 * PARAMS
 *     hdsa [I] pointer to the array control structure
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI DSA_DeleteAllItems (HDSA hdsa)
{
    TRACE("(%p)\n", hdsa);

    if (!hdsa)
        return FALSE;
    if (hdsa->pData && (!Free (hdsa->pData)))
        return FALSE;

    hdsa->nItemCount = 0;
    hdsa->pData = NULL;
    hdsa->nMaxCount = 0;

    return TRUE;
}


/**************************************************************************
 * DSA_EnumCallback [COMCTL32.387]
 *
 * Enumerates all items in a dynamic storage array.
 *
 * PARAMS
 *     hdsa     [I] handle to the dynamic storage array
 *     enumProc [I]
 *     lParam   [I]
 *
 * RETURNS
 *     none
 */
VOID WINAPI DSA_EnumCallback (HDSA hdsa, PFNDSAENUMCALLBACK enumProc,
                              LPVOID lParam)
{
    INT i;

    TRACE("(%p %p %p)\n", hdsa, enumProc, lParam);

    if (!hdsa)
        return;
    if (hdsa->nItemCount <= 0)
        return;

    for (i = 0; i < hdsa->nItemCount; i++) {
        LPVOID lpItem = DSA_GetItemPtr (hdsa, i);
        if ((enumProc)(lpItem, lParam) == 0)
            return;
    }

    return;
}


/**************************************************************************
 * DSA_DestroyCallback [COMCTL32.388]
 *
 * Enumerates all items in a dynamic storage array and destroys it.
 *
 * PARAMS
 *     hdsa     [I] handle to the dynamic storage array
 *     enumProc [I]
 *     lParam   [I]
 *
 * RETURNS
 *     none
 */
void WINAPI DSA_DestroyCallback (HDSA hdsa, PFNDSAENUMCALLBACK enumProc,
                                 LPVOID lParam)
{
    TRACE("(%p %p %p)\n", hdsa, enumProc, lParam);

    DSA_EnumCallback (hdsa, enumProc, lParam);
    DSA_Destroy (hdsa);
}

#if __WINE_COMCTL32_VERSION == 6
/**************************************************************************
 * DSA_Clone [COMCTL32.@]
 *
 * Creates a copy of a dsa
 *
 * PARAMS
 *     hdsa [I] handle to the dynamic storage array
 *
 * RETURNS
 *     Cloned dsa
 */
HDSA WINAPI DSA_Clone(HDSA hdsa)
{
    HDSA dest;
    INT i;

    TRACE("(%p)\n", hdsa);

    if (!hdsa)
        return NULL;

    dest = DSA_Create (hdsa->nItemSize, hdsa->nGrow);
    if (!dest)
        return NULL;

    for (i = 0; i < hdsa->nItemCount; i++) {
        void *ptr = DSA_GetItemPtr (hdsa, i);
        if (DSA_InsertItem (dest, DA_LAST, ptr) == -1) {
            DSA_Destroy (dest);
            return NULL;
        }
    }

    return dest;
}

/**************************************************************************
 * DSA_GetSize [COMCTL32.@]
 *
 * Returns allocated memory size for this array
 *
 * PARAMS
 *     hdsa [I] handle to the dynamic storage array
 *
 * RETURNS
 *     Size
 */
ULONGLONG WINAPI DSA_GetSize(HDSA hdsa)
{
    TRACE("(%p)\n", hdsa);

    if (!hdsa) return 0;

    return sizeof(*hdsa) + (ULONGLONG)hdsa->nMaxCount*hdsa->nItemSize;
}
#endif /* __WINE_COMCTL32_VERSION == 6 */
