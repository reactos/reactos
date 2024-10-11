/*
 * Dynamic pointer array (DPA) implementation
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

#define COBJMACROS

#include <stdarg.h>
#include <limits.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "commctrl.h"
#include "objbase.h"

#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dpa);

typedef struct _DPA
{
    INT    nItemCount;
    LPVOID   *ptrs;
    HANDLE hHeap;
    INT    nGrow;
    INT    nMaxCount;
} DPA;

typedef struct _STREAMDATA
{
    DWORD dwSize;
    DWORD dwData2;
    DWORD dwItems;
} STREAMDATA, *PSTREAMDATA;

/**************************************************************************
 * DPA_LoadStream [COMCTL32.9]
 *
 * Loads a dynamic pointer array from a stream
 *
 * PARAMS
 *     phDpa    [O] pointer to a handle to a dynamic pointer array
 *     loadProc [I] pointer to a callback function
 *     pStream  [I] pointer to a stream
 *     pData    [I] pointer to callback data
 *
 * RETURNS
 *     Success: S_OK, S_FALSE - partial success
 *     Failure: HRESULT error code
 *
 * NOTES
 *     No more information available yet!
 */
HRESULT WINAPI DPA_LoadStream (HDPA *phDpa, PFNDPASTREAM loadProc,
                               IStream *pStream, LPVOID pData)
{
    HRESULT errCode;
    LARGE_INTEGER position;
    ULARGE_INTEGER initial_pos;
    STREAMDATA  streamData;
    DPASTREAMINFO streamInfo;
    ULONG ulRead;
    HDPA hDpa;
    PVOID *ptr;

    TRACE ("phDpa=%p loadProc=%p pStream=%p pData=%p\n",
           phDpa, loadProc, pStream, pData);

    if (!phDpa || !loadProc || !pStream)
        return E_INVALIDARG;

    *phDpa = NULL;

    position.QuadPart = 0;

    errCode = IStream_Seek (pStream, position, STREAM_SEEK_CUR, &initial_pos);
    if (errCode != S_OK)
        return errCode;

    memset(&streamData, 0, sizeof(STREAMDATA));
    errCode = IStream_Read (pStream, &streamData, sizeof(STREAMDATA), &ulRead);
    if (errCode != S_OK)
        return errCode;

    TRACE ("dwSize=%lu dwData2=%lu dwItems=%lu\n",
           streamData.dwSize, streamData.dwData2, streamData.dwItems);

    if (ulRead < sizeof(STREAMDATA) ||
        streamData.dwSize < sizeof(STREAMDATA) || streamData.dwData2 != 1) {
        /* back to initial position */
        position.QuadPart = initial_pos.QuadPart;
        IStream_Seek (pStream, position, STREAM_SEEK_SET, NULL);
        return E_FAIL;
    }

    if (streamData.dwItems > (UINT_MAX / 2 / sizeof(VOID*))) /* 536870911 */
        return E_OUTOFMEMORY;

    /* create the dpa */
    hDpa = DPA_Create (streamData.dwItems);
    if (!hDpa)
        return E_OUTOFMEMORY;

    if (!DPA_Grow (hDpa, streamData.dwItems)) {
        DPA_Destroy (hDpa);
        return E_OUTOFMEMORY;
    }

    /* load data from the stream into the dpa */
    ptr = hDpa->ptrs;
    for (streamInfo.iPos = 0; streamInfo.iPos < streamData.dwItems; streamInfo.iPos++) {
        errCode = (loadProc)(&streamInfo, pStream, pData);
        if (errCode != S_OK) {
            errCode = S_FALSE;
            break;
        }

        *ptr = streamInfo.pvItem;
        ptr++;
    }

    /* set the number of items */
    hDpa->nItemCount = streamInfo.iPos;

    /* store the handle to the dpa */
    *phDpa = hDpa;
    TRACE ("new hDpa=%p, errorcode %lx\n", hDpa, errCode);

    return errCode;
}


/**************************************************************************
 * DPA_SaveStream [COMCTL32.10]
 *
 * Saves a dynamic pointer array to a stream
 *
 * PARAMS
 *     hDpa     [I] handle to a dynamic pointer array
 *     saveProc [I] pointer to a callback function
 *     pStream  [I] pointer to a stream
 *     pData    [I] pointer to callback data
 *
 * RETURNS
 *     Success: S_OK, S_FALSE - partial success
 *     Failure: HRESULT error code
 *
 * NOTES
 *     No more information available yet!
 */
HRESULT WINAPI DPA_SaveStream (HDPA hDpa, PFNDPASTREAM saveProc,
                               IStream *pStream, LPVOID pData)
{
    LARGE_INTEGER position;
    ULARGE_INTEGER initial_pos, curr_pos;
    STREAMDATA  streamData;
    DPASTREAMINFO streamInfo;
    HRESULT hr;
    PVOID *ptr;

    TRACE ("hDpa=%p saveProc=%p pStream=%p pData=%p\n",
            hDpa, saveProc, pStream, pData);

    if (!hDpa || !saveProc || !pStream) return E_INVALIDARG;

    /* save initial position to write header after completion */
    position.QuadPart = 0;
    hr = IStream_Seek (pStream, position, STREAM_SEEK_CUR, &initial_pos);
    if (hr != S_OK)
        return hr;

    /* write empty header */
    streamData.dwSize  = sizeof(streamData);
    streamData.dwData2 = 1;
    streamData.dwItems = 0;

    hr = IStream_Write (pStream, &streamData, sizeof(streamData), NULL);
    if (hr != S_OK) {
        position.QuadPart = initial_pos.QuadPart;
        IStream_Seek (pStream, position, STREAM_SEEK_SET, NULL);
        return hr;
    }

    /* no items - we're done */
    if (hDpa->nItemCount == 0) return S_OK;

    ptr = hDpa->ptrs;
    for (streamInfo.iPos = 0; streamInfo.iPos < hDpa->nItemCount; streamInfo.iPos++) {
        streamInfo.pvItem = *ptr;
        hr = (saveProc)(&streamInfo, pStream, pData);
        if (hr != S_OK) {
            hr = S_FALSE;
            break;
        }
        ptr++;
    }

    /* write updated header */
    position.QuadPart = 0;
    IStream_Seek (pStream, position, STREAM_SEEK_CUR, &curr_pos);

    streamData.dwSize  = curr_pos.QuadPart - initial_pos.QuadPart;
    streamData.dwData2 = 1;
    streamData.dwItems = streamInfo.iPos;

    position.QuadPart = initial_pos.QuadPart;
    IStream_Seek (pStream, position, STREAM_SEEK_SET, NULL);
    IStream_Write (pStream, &streamData, sizeof(streamData), NULL);

    position.QuadPart = curr_pos.QuadPart;
    IStream_Seek (pStream, position, STREAM_SEEK_SET, NULL);

    return hr;
}


/**************************************************************************
 * DPA_Merge [COMCTL32.11]
 *
 * Merge two dynamic pointers arrays.
 *
 * PARAMS
 *     hdpa1       [I] handle to a dynamic pointer array
 *     hdpa2       [I] handle to a dynamic pointer array
 *     dwFlags     [I] flags
 *     pfnCompare  [I] pointer to sort function
 *     pfnMerge    [I] pointer to merge function
 *     lParam      [I] application specific value
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE 
 *
 * NOTES
 *     No more information available yet!
 */
BOOL WINAPI DPA_Merge (HDPA hdpa1, HDPA hdpa2, DWORD dwFlags,
                       PFNDPACOMPARE pfnCompare, PFNDPAMERGE pfnMerge,
                       LPARAM lParam)
{
    INT nCount;
    LPVOID *pWork1, *pWork2;
    INT nResult, i;
    INT nIndex;

    TRACE("%p, %p, %#lx, %p, %p, %#Ix\n", hdpa1, hdpa2, dwFlags, pfnCompare, pfnMerge, lParam);

    if (IsBadWritePtr (hdpa1, sizeof(*hdpa1)))
        return FALSE;

    if (IsBadWritePtr (hdpa2, sizeof(*hdpa2)))
        return FALSE;

    if (IsBadCodePtr ((FARPROC)pfnCompare))
        return FALSE;

    if (IsBadCodePtr ((FARPROC)pfnMerge))
        return FALSE;

    if (!(dwFlags & DPAM_SORTED)) {
        TRACE("sorting dpa's.\n");
        if (hdpa1->nItemCount > 0)
        DPA_Sort (hdpa1, pfnCompare, lParam);
        TRACE ("dpa 1 sorted.\n");
        if (hdpa2->nItemCount > 0)
        DPA_Sort (hdpa2, pfnCompare, lParam);
        TRACE ("dpa 2 sorted.\n");
    }

    if (hdpa2->nItemCount < 1)
        return TRUE;

    TRACE("hdpa1->nItemCount=%d hdpa2->nItemCount=%d\n",
           hdpa1->nItemCount, hdpa2->nItemCount);


    nIndex = hdpa1->nItemCount - 1;
    nCount = hdpa2->nItemCount - 1;

    do
    {
        pWork1 = &hdpa1->ptrs[nIndex];
        pWork2 = &hdpa2->ptrs[nCount];

        if (nIndex < 0) {
            if ((nCount >= 0) && (dwFlags & DPAM_UNION)) {
                /* Now insert the remaining new items into DPA 1 */
                TRACE("%d items to be inserted at start of DPA 1\n",
                      nCount+1);
                for (i=nCount; i>=0; i--) {
                    PVOID ptr;

                    ptr = (pfnMerge)(DPAMM_INSERT, *pWork2, NULL, lParam);
                    if (!ptr)
                        return FALSE;
                    DPA_InsertPtr (hdpa1, 0, ptr);
                    pWork2--;
                }
            }
            break;
        }
        nResult = (pfnCompare)(*pWork1, *pWork2, lParam);
        TRACE("compare result=%d, dpa1.cnt=%d, dpa2.cnt=%d\n",
              nResult, nIndex, nCount);

        if (nResult == 0)
        {
            PVOID ptr;

            ptr = (pfnMerge)(DPAMM_MERGE, *pWork1, *pWork2, lParam);
            if (!ptr)
                return FALSE;

            nCount--;
            *pWork1 = ptr;
            nIndex--;
        }
        else if (nResult > 0)
        {
            /* item in DPA 1 missing from DPA 2 */
            if (dwFlags & DPAM_INTERSECT)
            {
                /* Now delete the extra item in DPA1 */
                PVOID ptr;

                ptr = DPA_DeletePtr (hdpa1, nIndex);

                (pfnMerge)(DPAMM_DELETE, ptr, NULL, lParam);
            }
            nIndex--;
        }
        else
        {
            /* new item in DPA 2 */
            if (dwFlags & DPAM_UNION)
            {
                /* Now insert the new item in DPA 1 */
                PVOID ptr;

                ptr = (pfnMerge)(DPAMM_INSERT, *pWork2, NULL, lParam);
                if (!ptr)
                    return FALSE;
                DPA_InsertPtr (hdpa1, nIndex+1, ptr);
            }
            nCount--;
        }

    }
    while (nCount >= 0);

    return TRUE;
}


/**************************************************************************
 * DPA_Destroy [COMCTL32.329]
 *
 * Destroys a dynamic pointer array
 *
 * PARAMS
 *     hdpa [I] handle (pointer) to the pointer array
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI DPA_Destroy (HDPA hdpa)
{
    TRACE("(%p)\n", hdpa);

    if (!hdpa)
        return FALSE;

    if (hdpa->ptrs && (!HeapFree (hdpa->hHeap, 0, hdpa->ptrs)))
        return FALSE;

    return HeapFree (hdpa->hHeap, 0, hdpa);
}


/**************************************************************************
 * DPA_Grow [COMCTL32.330]
 *
 * Sets the growth amount.
 *
 * PARAMS
 *     hdpa  [I] handle (pointer) to the existing (source) pointer array
 *     nGrow [I] number of items by which the array grows when it's too small
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI DPA_Grow (HDPA hdpa, INT nGrow)
{
    INT items;
    TRACE("(%p %d)\n", hdpa, nGrow);

    if (!hdpa)
        return FALSE;

    nGrow = max( 8, nGrow );
    items = nGrow * (((hdpa->nMaxCount - 1) / nGrow) + 1);
    if (items > hdpa->nMaxCount)
    {
        void *ptr;

        if (hdpa->ptrs)
            ptr = HeapReAlloc( hdpa->hHeap, HEAP_ZERO_MEMORY, hdpa->ptrs, items * sizeof(LPVOID) );
        else
            ptr = HeapAlloc( hdpa->hHeap, HEAP_ZERO_MEMORY, items * sizeof(LPVOID) );
        if (!ptr) return FALSE;
        hdpa->nMaxCount = items;
        hdpa->ptrs = ptr;
    }
    hdpa->nGrow = nGrow;

    return TRUE;
}


/**************************************************************************
 * DPA_Clone [COMCTL32.331]
 *
 * Copies a pointer array to another one or creates a copy
 *
 * PARAMS
 *     hdpa    [I] handle (pointer) to the existing (source) pointer array
 *     hdpaNew [O] handle (pointer) to the destination pointer array
 *
 * RETURNS
 *     Success: pointer to the destination pointer array.
 *     Failure: NULL
 *
 * NOTES
 *     - If the 'hdpaNew' is a NULL-Pointer, a copy of the source pointer
 *       array will be created and its handle (pointer) is returned.
 *     - If 'hdpa' is a NULL-Pointer, the original implementation crashes,
 *       this implementation just returns NULL.
 */
HDPA WINAPI DPA_Clone (const HDPA hdpa, HDPA hdpaNew)
{
    INT nNewItems, nSize;
    HDPA hdpaTemp;

    if (!hdpa)
        return NULL;

    TRACE("(%p %p)\n", hdpa, hdpaNew);

    if (!hdpaNew) {
        /* create a new DPA */
        hdpaTemp = HeapAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY,
                                    sizeof(*hdpaTemp));
        hdpaTemp->hHeap = hdpa->hHeap;
        hdpaTemp->nGrow = hdpa->nGrow;
    }
    else
        hdpaTemp = hdpaNew;

    if (hdpaTemp->ptrs) {
        /* remove old pointer array */
        HeapFree (hdpaTemp->hHeap, 0, hdpaTemp->ptrs);
        hdpaTemp->ptrs = NULL;
        hdpaTemp->nItemCount = 0;
        hdpaTemp->nMaxCount = 0;
    }

    /* create a new pointer array */
    nNewItems = hdpaTemp->nGrow *
                (((hdpa->nItemCount - 1) / hdpaTemp->nGrow) + 1);
    nSize = nNewItems * sizeof(LPVOID);
    hdpaTemp->ptrs = HeapAlloc (hdpaTemp->hHeap, HEAP_ZERO_MEMORY, nSize);
    hdpaTemp->nMaxCount = nNewItems;

    /* clone the pointer array */
    hdpaTemp->nItemCount = hdpa->nItemCount;
    memmove (hdpaTemp->ptrs, hdpa->ptrs,
             hdpaTemp->nItemCount * sizeof(LPVOID));

    return hdpaTemp;
}


/**************************************************************************
 * DPA_GetPtr [COMCTL32.332]
 *
 * Retrieves a pointer from a dynamic pointer array
 *
 * PARAMS
 *     hdpa   [I] handle (pointer) to the pointer array
 *     nIndex [I] array index of the desired pointer
 *
 * RETURNS
 *     Success: pointer
 *     Failure: NULL
 */
LPVOID WINAPI DPA_GetPtr (HDPA hdpa, INT_PTR nIndex)
{
    TRACE("%p, %Id\n", hdpa, nIndex);

    if (!hdpa)
        return NULL;
    if (!hdpa->ptrs) {
        WARN("no pointer array.\n");
        return NULL;
    }
    if ((nIndex < 0) || (nIndex >= hdpa->nItemCount)) {
        WARN("not enough pointers in array (%Id vs %d).\n",nIndex,hdpa->nItemCount);
        return NULL;
    }

    TRACE("-- %p\n", hdpa->ptrs[nIndex]);

    return hdpa->ptrs[nIndex];
}


/**************************************************************************
 * DPA_GetPtrIndex [COMCTL32.333]
 *
 * Retrieves the index of the specified pointer
 *
 * PARAMS
 *     hdpa   [I] handle (pointer) to the pointer array
 *     p      [I] pointer
 *
 * RETURNS
 *     Success: index of the specified pointer
 *     Failure: -1
 */
INT WINAPI DPA_GetPtrIndex (HDPA hdpa, LPCVOID p)
{
    INT i;

    if (!hdpa || !hdpa->ptrs)
        return -1;

    for (i = 0; i < hdpa->nItemCount; i++) {
        if (hdpa->ptrs[i] == p)
            return i;
    }

    return -1;
}


/**************************************************************************
 * DPA_InsertPtr [COMCTL32.334]
 *
 * Inserts a pointer into a dynamic pointer array
 *
 * PARAMS
 *     hdpa [I] handle (pointer) to the array
 *     i    [I] array index
 *     p    [I] pointer to insert
 *
 * RETURNS
 *     Success: index of the inserted pointer
 *     Failure: -1
 */
INT WINAPI DPA_InsertPtr (HDPA hdpa, INT i, LPVOID p)
{
    TRACE("(%p %d %p)\n", hdpa, i, p);

    if (!hdpa || i < 0) return -1;

    /* append item if index is out of bounds */
    i = min(hdpa->nItemCount, i);

    /* create empty spot at the end */
    if (!DPA_SetPtr(hdpa, hdpa->nItemCount, 0)) return -1;
    
    if (i != hdpa->nItemCount - 1)
        memmove (hdpa->ptrs + i + 1, hdpa->ptrs + i, 
                 (hdpa->nItemCount - i - 1) * sizeof(LPVOID));
    
    hdpa->ptrs[i] = p;
    return i;
}


/**************************************************************************
 * DPA_SetPtr [COMCTL32.335]
 *
 * Sets a pointer in the pointer array
 *
 * PARAMS
 *     hdpa [I] handle (pointer) to the pointer array
 *     i    [I] index of the pointer that will be set
 *     p    [I] pointer to be set
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI DPA_SetPtr (HDPA hdpa, INT i, LPVOID p)
{
    LPVOID *lpTemp;

    TRACE("(%p %d %p)\n", hdpa, i, p);

    if (!hdpa || i < 0)
        return FALSE;

    if (hdpa->nItemCount <= i) {
        /* within the old array */
        if (hdpa->nMaxCount <= i) {
            /* resize the block of memory */
            INT nNewItems =
                hdpa->nGrow * ((((i+1) - 1) / hdpa->nGrow) + 1);
            INT nSize = nNewItems * sizeof(LPVOID);

            if (hdpa->ptrs)
                lpTemp = HeapReAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY, hdpa->ptrs, nSize);
            else
                lpTemp = HeapAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY, nSize);
            
            if (!lpTemp)
                return FALSE;

            hdpa->nMaxCount = nNewItems;
            hdpa->ptrs = lpTemp;
        }
        hdpa->nItemCount = i+1;
    }

    /* put the new entry in */
    hdpa->ptrs[i] = p;

    return TRUE;
}


/**************************************************************************
 * DPA_DeletePtr [COMCTL32.336]
 *
 * Removes a pointer from the pointer array.
 *
 * PARAMS
 *     hdpa [I] handle (pointer) to the pointer array
 *     i    [I] index of the pointer that will be deleted
 *
 * RETURNS
 *     Success: deleted pointer
 *     Failure: NULL
 */
LPVOID WINAPI DPA_DeletePtr (HDPA hdpa, INT i)
{
    LPVOID *lpDest, *lpSrc, lpTemp = NULL;
    INT  nSize;

    TRACE("(%p %d)\n", hdpa, i);

    if ((!hdpa) || i < 0 || i >= hdpa->nItemCount)
        return NULL;

    lpTemp = hdpa->ptrs[i];

    /* do we need to move ?*/
    if (i < hdpa->nItemCount - 1) {
        lpDest = hdpa->ptrs + i;
        lpSrc = lpDest + 1;
        nSize = (hdpa->nItemCount - i - 1) * sizeof(LPVOID);
        TRACE("-- move dest=%p src=%p size=%x\n",
               lpDest, lpSrc, nSize);
        memmove (lpDest, lpSrc, nSize);
    }

    hdpa->nItemCount --;

    /* free memory ?*/
    if ((hdpa->nMaxCount - hdpa->nItemCount) >= hdpa->nGrow) {
        INT nNewItems = max(hdpa->nGrow * 2, hdpa->nItemCount);
        nSize = nNewItems * sizeof(LPVOID);
        lpDest = HeapReAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY,
                                      hdpa->ptrs, nSize);
        if (!lpDest)
            return NULL;

        hdpa->nMaxCount = nNewItems;
        hdpa->ptrs = lpDest;
    }

    return lpTemp;
}


/**************************************************************************
 * DPA_DeleteAllPtrs [COMCTL32.337]
 *
 * Removes all pointers and reinitializes the array.
 *
 * PARAMS
 *     hdpa [I] handle (pointer) to the pointer array
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI DPA_DeleteAllPtrs (HDPA hdpa)
{
    TRACE("(%p)\n", hdpa);

    if (!hdpa)
        return FALSE;

    if (hdpa->ptrs && (!HeapFree (hdpa->hHeap, 0, hdpa->ptrs)))
        return FALSE;

    hdpa->nItemCount = 0;
    hdpa->nMaxCount = hdpa->nGrow * 2;
    hdpa->ptrs = HeapAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY,
                                     hdpa->nMaxCount * sizeof(LPVOID));

    return TRUE;
}


/**************************************************************************
 * DPA_QuickSort [Internal]
 *
 * Ordinary quicksort (used by DPA_Sort).
 *
 * PARAMS
 *     lpPtrs     [I] pointer to the pointer array
 *     l          [I] index of the "left border" of the partition
 *     r          [I] index of the "right border" of the partition
 *     pfnCompare [I] pointer to the compare function
 *     lParam     [I] user defined value (3rd parameter in compare function)
 *
 * RETURNS
 *     NONE
 */
static VOID DPA_QuickSort (LPVOID *lpPtrs, INT l, INT r,
                           PFNDPACOMPARE pfnCompare, LPARAM lParam)
{
    INT m;
    LPVOID t;

    TRACE("l=%i r=%i\n", l, r);

    if (l==r)    /* one element is always sorted */
        return;
    if (r<l)     /* oops, got it in the wrong order */
        {
        DPA_QuickSort(lpPtrs, r, l, pfnCompare, lParam);
        return;
        }
    m = (l+r)/2; /* divide by two */
    DPA_QuickSort(lpPtrs, l, m, pfnCompare, lParam);
    DPA_QuickSort(lpPtrs, m+1, r, pfnCompare, lParam);

    /* join the two sides */
    while( (l<=m) && (m<r) )
    {
        if(pfnCompare(lpPtrs[l],lpPtrs[m+1],lParam)>0)
        {
            t = lpPtrs[m+1];
            memmove(&lpPtrs[l+1],&lpPtrs[l],(m-l+1)*sizeof(lpPtrs[l]));
            lpPtrs[l] = t;

            m++;
        }
        l++;
    }
}


/**************************************************************************
 * DPA_Sort [COMCTL32.338]
 *
 * Sorts a pointer array using a user defined compare function
 *
 * PARAMS
 *     hdpa       [I] handle (pointer) to the pointer array
 *     pfnCompare [I] pointer to the compare function
 *     lParam     [I] user defined value (3rd parameter of compare function)
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI DPA_Sort (HDPA hdpa, PFNDPACOMPARE pfnCompare, LPARAM lParam)
{
    if (!hdpa || !pfnCompare)
        return FALSE;

    TRACE("%p, %p, %#Ix\n", hdpa, pfnCompare, lParam);

    if ((hdpa->nItemCount > 1) && (hdpa->ptrs))
        DPA_QuickSort (hdpa->ptrs, 0, hdpa->nItemCount - 1,
                       pfnCompare, lParam);

    return TRUE;
}


/**************************************************************************
 * DPA_Search [COMCTL32.339]
 *
 * Searches a pointer array for a specified pointer
 *
 * PARAMS
 *     hdpa       [I] handle (pointer) to the pointer array
 *     pFind      [I] pointer to search for
 *     nStart     [I] start index
 *     pfnCompare [I] pointer to the compare function
 *     lParam     [I] user defined value (3rd parameter of compare function)
 *     uOptions   [I] search options
 *
 * RETURNS
 *     Success: index of the pointer in the array.
 *     Failure: -1
 */
INT WINAPI DPA_Search (HDPA hdpa, LPVOID pFind, INT nStart,
                       PFNDPACOMPARE pfnCompare, LPARAM lParam, UINT uOptions)
{
    if (!hdpa || !pfnCompare || !pFind)
        return -1;

    TRACE("%p, %p, %d, %p, %#Ix, %#x\n", hdpa, pFind, nStart, pfnCompare, lParam, uOptions);

    if (uOptions & DPAS_SORTED) {
        /* array is sorted --> use binary search */
        INT l, r, x, n;
        LPVOID *lpPtr;

        /* for binary search ignore start index */
        l = 0;
        r = hdpa->nItemCount - 1;
        lpPtr = hdpa->ptrs;
        while (r >= l) {
            x = l + (r - l) / 2;
            n = (pfnCompare)(pFind, lpPtr[x], lParam);
            if (n == 0)
                return x;
            else if (n < 0)
                r = x - 1;
            else /* (n > 0) */
                l = x + 1;
        }
        if (uOptions & (DPAS_INSERTBEFORE|DPAS_INSERTAFTER)) return l;
    }
    else {
        /* array is not sorted --> use linear search */
        LPVOID *lpPtr;
        INT  nIndex;

        nIndex = (nStart == -1)? 0 : nStart;
        lpPtr = hdpa->ptrs;
        for (; nIndex < hdpa->nItemCount; nIndex++) {
            if ((pfnCompare)(pFind, lpPtr[nIndex], lParam) == 0)
                return nIndex;
        }
    }

    return -1;
}


/**************************************************************************
 * DPA_CreateEx [COMCTL32.340]
 *
 * Creates a dynamic pointer array using the specified size and heap.
 *
 * PARAMS
 *     nGrow [I] number of items by which the array grows when it is filled
 *     hHeap [I] handle to the heap where the array is stored
 *
 * RETURNS
 *     Success: handle (pointer) to the pointer array.
 *     Failure: NULL
 *
 * NOTES
 *     The DPA_ functions can be used to create and manipulate arrays of
 *     pointers.
 */
HDPA WINAPI DPA_CreateEx (INT nGrow, HANDLE hHeap)
{
    HDPA hdpa;

    TRACE("(%d %p)\n", nGrow, hHeap);

    if (hHeap)
        hdpa = HeapAlloc (hHeap, HEAP_ZERO_MEMORY, sizeof(*hdpa));
    else
        hdpa = Alloc (sizeof(*hdpa));

    if (hdpa) {
        hdpa->nGrow = max(8, nGrow);
        hdpa->hHeap = hHeap ? hHeap : GetProcessHeap();
        hdpa->nMaxCount = hdpa->nGrow * 2;
        hdpa->ptrs = HeapAlloc (hdpa->hHeap, HEAP_ZERO_MEMORY,
                                hdpa->nMaxCount * sizeof(LPVOID));
    }

    TRACE("-- %p\n", hdpa);

    return hdpa;
}


/**************************************************************************
 * DPA_Create [COMCTL32.328]
 *
 * Creates a dynamic pointer array.
 *
 * PARAMS
 *     nGrow [I] number of items by which the array grows when it is filled
 *
 * RETURNS
 *     Success: handle (pointer) to the pointer array.
 *     Failure: NULL
 *
 * NOTES
 *     The DPA_ functions can be used to create and manipulate arrays of
 *     pointers.
 */
HDPA WINAPI DPA_Create (INT nGrow)
{
    return DPA_CreateEx( nGrow, 0 );
}


/**************************************************************************
 * DPA_EnumCallback [COMCTL32.385]
 *
 * Enumerates all items in a dynamic pointer array.
 *
 * PARAMS
 *     hdpa     [I] handle to the dynamic pointer array
 *     enumProc [I]
 *     lParam   [I]
 *
 * RETURNS
 *     none
 */
VOID WINAPI DPA_EnumCallback (HDPA hdpa, PFNDPAENUMCALLBACK enumProc,
                              LPVOID lParam)
{
    INT i;

    TRACE("(%p %p %p)\n", hdpa, enumProc, lParam);

    if (!hdpa)
        return;
    if (hdpa->nItemCount <= 0)
        return;

    for (i = 0; i < hdpa->nItemCount; i++) {
        if ((enumProc)(hdpa->ptrs[i], lParam) == 0)
            return;
    }

    return;
}


/**************************************************************************
 * DPA_DestroyCallback [COMCTL32.386]
 *
 * Enumerates all items in a dynamic pointer array and destroys it.
 *
 * PARAMS
 *     hdpa     [I] handle to the dynamic pointer array
 *     enumProc [I]
 *     lParam   [I]
 *
 * RETURNS
 *     none
 */
void WINAPI DPA_DestroyCallback (HDPA hdpa, PFNDPAENUMCALLBACK enumProc,
                                 LPVOID lParam)
{
    TRACE("(%p %p %p)\n", hdpa, enumProc, lParam);

    DPA_EnumCallback (hdpa, enumProc, lParam);
    DPA_Destroy (hdpa);
}

/**************************************************************************
 * DPA_GetSize [COMCTL32.@]
 *
 * Returns all array allocated memory size
 *
 * PARAMS
 *     hdpa     [I] handle to the dynamic pointer array
 *
 * RETURNS
 *     Size in bytes
 */
ULONGLONG WINAPI DPA_GetSize(HDPA hdpa)
{
    TRACE("(%p)\n", hdpa);

    if (!hdpa) return 0;

    return sizeof(DPA) + hdpa->nMaxCount*sizeof(PVOID);
}
