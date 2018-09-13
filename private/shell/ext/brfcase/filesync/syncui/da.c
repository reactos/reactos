//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: da.c
//
//  This file contains dynamic array functions.
//
// History:
//  09-27-94 ScottH     Taken from commctrl
//
//---------------------------------------------------------------------------


#include "brfprv.h"         // common headers

//
// Heapsort is a bit slower, but it doesn't use any stack or memory...
// Mergesort takes a bit of memory (O(n)) and stack (O(log(n)), but very fast...
//
#ifdef WIN32
#define MERGESORT
#else
#define USEHEAPSORT
#endif

#ifdef DEBUG
#define DSA_MAGIC   (TEXT('S') | (TEXT('A') << 256))
#define IsDSA(pdsa) ((pdsa) && (pdsa)->magic == DSA_MAGIC)
#define DPA_MAGIC   (TEXT('P') | (TEXT('A') << 256))
#define IsDPA(pdpa) ((pdpa) && (pdpa)->magic == DPA_MAGIC)
#else
#define IsDSA(pdsa)
#define IsDPA(pdsa)
#endif


typedef struct {
    void * * pp;
    PFNDPACOMPARE pfnCmp;
    LPARAM lParam;
    int cp;
#ifdef MERGESORT
    void * * ppT;
#endif
} SORTPARAMS;

BOOL  DPA_QuickSort(SORTPARAMS * psp);
BOOL  DPA_QuickSort2(int i, int j, SORTPARAMS * psp);
BOOL  DPA_HeapSort(SORTPARAMS * psp);
void  DPA_HeapSortPushDown(int first, int last, SORTPARAMS * psp);
BOOL  DPA_MergeSort(SORTPARAMS * psp);
void  DPA_MergeSort2(SORTPARAMS * psp, int iFirst, int cItems);



//========== Dynamic structure array ====================================

// Dynamic structure array

typedef struct _DSA {
// NOTE: The following field MUST be defined at the beginning of the
// structure in order for GetItemCount() to work.
//
    int cItem;          // # of elements in dsa

    void * aItem;       // memory for elements
    int cItemAlloc;     // # items which fit in aItem
    int cbItem;         // size of each item
    int cItemGrow;      // # items to grow cItemAlloc by
#ifdef DEBUG
    UINT magic;
#endif
} DSA;

#define DSA_PITEM(pdsa, index)    ((void *)(((BYTE *)(pdsa)->aItem) + ((index) * (pdsa)->cbItem)))


HDSA PUBLIC DSA_Create(int cbItem, int cItemGrow)
{
    HDSA pdsa = SharedAlloc(sizeof(DSA));

    ASSERT(cbItem);

    if (pdsa)
    {
        pdsa->cItem = 0;
        pdsa->cItemAlloc = 0;
        pdsa->cbItem = cbItem;
        pdsa->cItemGrow = (cItemGrow == 0 ? 1 : cItemGrow);
        pdsa->aItem = NULL;
#ifdef DEBUG
        pdsa->magic = DSA_MAGIC;
#endif
    }
    return pdsa;
}

BOOL PUBLIC DSA_Destroy(HDSA pdsa)
{
    ASSERT(IsDSA(pdsa));

    if (pdsa == NULL)       // allow NULL for low memory cases, still assert
        return TRUE;

#ifdef DEBUG
    pdsa->cItem = 0;
    pdsa->cItemAlloc = 0;
    pdsa->cbItem = 0;
    pdsa->magic = 0;
#endif
    if (pdsa->aItem && !SharedFree(&pdsa->aItem))
        return FALSE;

    return SharedFree(&pdsa);
}

BOOL PUBLIC DSA_GetItem(HDSA pdsa, int index, void * pitem)
{
    ASSERT(IsDSA(pdsa));
    ASSERT(pitem);

    if (index < 0 || index >= pdsa->cItem)
    {
        DebugMsg(DM_ERROR, TEXT("DSA: Invalid index: %d"), index);
        return FALSE;
    }

    hmemcpy(pitem, DSA_PITEM(pdsa, index), pdsa->cbItem);
    return TRUE;
}

void * PUBLIC DSA_GetItemPtr(HDSA pdsa, int index)
{
    ASSERT(IsDSA(pdsa));

    if (index < 0 || index >= pdsa->cItem)
    {
        DebugMsg(DM_ERROR, TEXT("DSA: Invalid index: %d"), index);
        return NULL;
    }
    return DSA_PITEM(pdsa, index);
}

BOOL PUBLIC DSA_SetItem(HDSA pdsa, int index, void * pitem)
{
    ASSERT(pitem);
    ASSERT(IsDSA(pdsa));

    if (index < 0)
    {
        DebugMsg(DM_ERROR, TEXT("DSA: Invalid index: %d"), index);
        return FALSE;
    }

    if (index >= pdsa->cItem)
    {
        if (index + 1 > pdsa->cItemAlloc)
        {
            int cItemAlloc = (((index + 1) + pdsa->cItemGrow - 1) / pdsa->cItemGrow) * pdsa->cItemGrow;

            void * aItemNew = SharedReAlloc(pdsa->aItem, cItemAlloc * pdsa->cbItem);
            if (!aItemNew)
                return FALSE;

            pdsa->aItem = aItemNew;
            pdsa->cItemAlloc = cItemAlloc;
        }
        pdsa->cItem = index + 1;
    }

    hmemcpy(DSA_PITEM(pdsa, index), pitem, pdsa->cbItem);

    return TRUE;
}

int PUBLIC DSA_InsertItem(HDSA pdsa, int index, void * pitem)
{
    ASSERT(pitem);
    ASSERT(IsDSA(pdsa));

    if (index < 0)
    {
        DebugMsg(DM_ERROR, TEXT("DSA: Invalid index: %d"), index);
        return -1;
    }

    if (index > pdsa->cItem)
        index = pdsa->cItem;

    if (pdsa->cItem + 1 > pdsa->cItemAlloc)
    {
        void * aItemNew = SharedReAlloc(pdsa->aItem,
                (pdsa->cItemAlloc + pdsa->cItemGrow) * pdsa->cbItem);
        if (!aItemNew)
            return -1;

        pdsa->aItem = aItemNew;
        pdsa->cItemAlloc += pdsa->cItemGrow;
    }

    if (index < pdsa->cItem)
    {
        hmemcpy(DSA_PITEM(pdsa, index + 1), DSA_PITEM(pdsa, index),
            (pdsa->cItem - index) * pdsa->cbItem);
    }
    pdsa->cItem++;
    hmemcpy(DSA_PITEM(pdsa, index), pitem, pdsa->cbItem);

    return index;
}

BOOL PUBLIC DSA_DeleteItem(HDSA pdsa, int index)
{
    ASSERT(IsDSA(pdsa));

    if (index < 0 || index >= pdsa->cItem)
    {
        DebugMsg(DM_ERROR, TEXT("DSA: Invalid index: %d"), index);
        return FALSE;
    }

    if (index < pdsa->cItem - 1)
    {
        hmemcpy(DSA_PITEM(pdsa, index), DSA_PITEM(pdsa, index + 1),
            (pdsa->cItem - (index + 1)) * pdsa->cbItem);
    }
    pdsa->cItem--;

    if (pdsa->cItemAlloc - pdsa->cItem > pdsa->cItemGrow)
    {
        void * aItemNew = SharedReAlloc(pdsa->aItem,
                (pdsa->cItemAlloc - pdsa->cItemGrow) * pdsa->cbItem);

        ASSERT(aItemNew);
        pdsa->aItem = aItemNew;
        pdsa->cItemAlloc -= pdsa->cItemGrow;
    }
    return TRUE;
}

BOOL PUBLIC DSA_DeleteAllItems(HDSA pdsa)
{
    ASSERT(IsDSA(pdsa));

    if (pdsa->aItem && !SharedFree(&pdsa->aItem))
        return FALSE;

    pdsa->aItem = NULL;
    pdsa->cItem = pdsa->cItemAlloc = 0;
    return TRUE;
}


//================== Dynamic pointer array implementation ===========

typedef struct _DPA {
// NOTE: The following two fields MUST be defined in this order, at
// the beginning of the structure in order for the macro APIs to work.
//
    int cp;
    void * * pp;

    HANDLE hheap;        // Heap to allocate from if NULL use shared

    int cpAlloc;
    int cpGrow;
#ifdef DEBUG
    UINT magic;
#endif
} DPA;



HDPA PUBLIC DPA_Create(int cpGrow)
{
    HDPA pdpa = SharedAlloc(sizeof(DPA));
    if (pdpa)
    {
        pdpa->cp = 0;
        pdpa->cpAlloc = 0;
        pdpa->cpGrow = (cpGrow < 8 ? 8 : cpGrow);
        pdpa->pp = NULL;
#ifdef WIN32
        pdpa->hheap = g_hSharedHeap;   // Defaults to use shared one (for now...)
#else
        pdpa->hheap = NULL;       // Defaults to use shared one (for now...)
#endif
#ifdef DEBUG
        pdpa->magic = DPA_MAGIC;
#endif
    }
    return pdpa;
}

// Should nuke the standard DPA above...
HDPA PUBLIC DPA_CreateEx(int cpGrow, HANDLE hheap)
{
    HDPA pdpa;
    if (hheap == NULL)
    {
        pdpa = SharedAlloc(sizeof(DPA));
#ifdef WIN32
        hheap = g_hSharedHeap;
#endif
    }
    else
        pdpa = MemAlloc(hheap, sizeof(DPA));
    if (pdpa)
    {
        pdpa->cp = 0;
        pdpa->cpAlloc = 0;
        pdpa->cpGrow = (cpGrow < 8 ? 8 : cpGrow);
        pdpa->pp = NULL;
        pdpa->hheap = hheap;
#ifdef DEBUG
        pdpa->magic = DPA_MAGIC;
#endif
    }
    return pdpa;
}

BOOL PUBLIC DPA_Destroy(HDPA pdpa)
{
    ASSERT(IsDPA(pdpa));

    if (pdpa == NULL)       // allow NULL for low memory cases, still assert
        return TRUE;

#ifdef WIN32
    ASSERT (pdpa->hheap);
#endif

#ifdef DEBUG
    pdpa->cp = 0;
    pdpa->cpAlloc = 0;
    pdpa->magic = 0;
#endif
    if (pdpa->pp && !MemFree(pdpa->hheap, pdpa->pp))
        return FALSE;

    return MemFree(pdpa->hheap, pdpa);
}

HDPA PUBLIC DPA_Clone(HDPA pdpa, HDPA pdpaNew)
{
    BOOL fAlloc = FALSE;

    if (!pdpaNew)
    {
        pdpaNew = DPA_CreateEx(pdpa->cpGrow, pdpa->hheap);
        if (!pdpaNew)
            return NULL;

        fAlloc = TRUE;
    }

    if (!DPA_Grow(pdpaNew, pdpa->cpAlloc)) {
        if (!fAlloc)
            DPA_Destroy(pdpaNew);
        return NULL;
    }

    pdpaNew->cp = pdpa->cp;
    hmemcpy(pdpaNew->pp, pdpa->pp, pdpa->cp * sizeof(void *));

    return pdpaNew;
}

void * PUBLIC DPA_GetPtr(HDPA pdpa, int index)
{
    ASSERT(IsDPA(pdpa));

    if (index < 0 || index >= pdpa->cp)
        return NULL;

    return pdpa->pp[index];
}

int PUBLIC DPA_GetPtrIndex(HDPA pdpa, void * p)
{
    void * * pp;
    void * * ppMax;

    ASSERT(IsDPA(pdpa));
    if (pdpa->pp)
    {
        pp = pdpa->pp;
        ppMax = pp + pdpa->cp;
        for ( ; pp < ppMax; pp++)
        {
            if (*pp == p)
                return (pp - pdpa->pp);
        }
    }
    return -1;
}

BOOL PUBLIC DPA_Grow(HDPA pdpa, int cpAlloc)
{
    ASSERT(IsDPA(pdpa));

    if (cpAlloc > pdpa->cpAlloc)
    {
        void * * ppNew;

        cpAlloc = ((cpAlloc + pdpa->cpGrow - 1) / pdpa->cpGrow) * pdpa->cpGrow;

        if (pdpa->pp)
            ppNew = (void * *)MemReAlloc(pdpa->hheap, pdpa->pp, cpAlloc * sizeof(void *));
        else
            ppNew = (void * *)MemAlloc(pdpa->hheap, cpAlloc * sizeof(void *));
        if (!ppNew)
            return FALSE;

        pdpa->pp = ppNew;
        pdpa->cpAlloc = cpAlloc;
    }
    return TRUE;
}

BOOL PUBLIC DPA_SetPtr(HDPA pdpa, int index, void * p)
{
    ASSERT(IsDPA(pdpa));

    if (index < 0)
    {
        DebugMsg(DM_ERROR, TEXT("DPA: Invalid index: %d"), index);
        return FALSE;
    }

    if (index >= pdpa->cp)
    {
        if (!DPA_Grow(pdpa, index + 1))
            return FALSE;
        pdpa->cp = index + 1;
    }

    pdpa->pp[index] = p;

    return TRUE;
}

int PUBLIC DPA_InsertPtr(HDPA pdpa, int index, void * p)
{
    ASSERT(IsDPA(pdpa));

    if (index < 0)
    {
        DebugMsg(DM_ERROR, TEXT("DPA: Invalid index: %d"), index);
        return -1;
    }
    if (index > pdpa->cp)
        index = pdpa->cp;

    // Make sure we have room for one more item
    //
    if (pdpa->cp + 1 > pdpa->cpAlloc)
    {
        if (!DPA_Grow(pdpa, pdpa->cp + 1))
            return -1;
    }

    // If we are inserting, we need to slide everybody up
    //
    if (index < pdpa->cp)
    {
        hmemcpy(&pdpa->pp[index + 1], &pdpa->pp[index],
            (pdpa->cp - index) * sizeof(void *));
    }

    pdpa->pp[index] = p;
    pdpa->cp++;

    return index;
}

void * PUBLIC DPA_DeletePtr(HDPA pdpa, int index)
{
    void * p;

    ASSERT(IsDPA(pdpa));

    if (index < 0 || index >= pdpa->cp)
    {
        DebugMsg(DM_ERROR, TEXT("DPA: Invalid index: %d"), index);
        return NULL;
    }

    p = pdpa->pp[index];

    if (index < pdpa->cp - 1)
    {
        hmemcpy(&pdpa->pp[index], &pdpa->pp[index + 1],
            (pdpa->cp - (index + 1)) * sizeof(void *));
    }
    pdpa->cp--;

    if (pdpa->cpAlloc - pdpa->cp > pdpa->cpGrow)
    {
        void * * ppNew;
        ppNew = MemReAlloc(pdpa->hheap, pdpa->pp, (pdpa->cpAlloc - pdpa->cpGrow) * sizeof(void *));

        ASSERT(ppNew);
        pdpa->pp = ppNew;
        pdpa->cpAlloc -= pdpa->cpGrow;
    }
    return p;
}

BOOL PUBLIC DPA_DeleteAllPtrs(HDPA pdpa)
{
    ASSERT(IsDPA(pdpa));

    if (pdpa->pp && !MemFree(pdpa->hheap, pdpa->pp))
        return FALSE;
    pdpa->pp = NULL;
    pdpa->cp = pdpa->cpAlloc = 0;
    return TRUE;
}

BOOL PUBLIC DPA_Sort(HDPA pdpa, PFNDPACOMPARE pfnCmp, LPARAM lParam)
{
    SORTPARAMS sp;

    sp.cp = pdpa->cp;
    sp.pp = pdpa->pp;
    sp.pfnCmp = pfnCmp;
    sp.lParam = lParam;

#ifdef USEQUICKSORT
    return DPA_QuickSort(&sp);
#endif
#ifdef USEHEAPSORT
    return DPA_HeapSort(&sp);
#endif
#ifdef MERGESORT
    return DPA_MergeSort(&sp);
#endif
}

#ifdef USEQUICKSORT

BOOL  DPA_QuickSort(SORTPARAMS * psp)
{
    return DPA_QuickSort2(0, psp->cp - 1, psp);
}

BOOL  DPA_QuickSort2(int i, int j, SORTPARAMS * psp)
{
    void * * pp = psp->pp;
    LPARAM lParam = psp->lParam;
    PFNDPACOMPARE pfnCmp = psp->pfnCmp;

    int iPivot;
    void * pFirst;
    int k;
    int result;

    iPivot = -1;
    pFirst = pp[i];
    for (k = i + 1; k <= j; k++)
    {
        result = (*pfnCmp)(pp[k], pFirst, lParam);

        if (result > 0)
        {
            iPivot = k;
            break;
        }
        else if (result < 0)
        {
            iPivot = i;
            break;
        }
    }

    if (iPivot != -1)
    {
        int l = i;
        int r = j;
        void * pivot = pp[iPivot];

        do
        {
            void * p;

            p = pp[l];
            pp[l] = pp[r];
            pp[r] = p;

            while ((*pfnCmp)(pp[l], pivot, lParam) < 0)
                l++;
            while ((*pfnCmp)(pp[r], pivot, lParam) >= 0)
                r--;
        } while (l <= r);

        if (l - 1 > i)
            DPA_QuickSort2(i, l - 1, psp);
        if (j > l)
            DPA_QuickSort2(l, j, psp);
    }
    return TRUE;
}
#endif  // USEQUICKSORT

#ifdef USEHEAPSORT

void  DPA_HeapSortPushDown(int first, int last, SORTPARAMS * psp)
{
    void * * pp = psp->pp;
    LPARAM lParam = psp->lParam;
    PFNDPACOMPARE pfnCmp = psp->pfnCmp;
    int r;
    int r2;
    void * p;

    r = first;
    while (r <= last / 2)
    {
        int wRTo2R;
        r2 = r * 2;

        wRTo2R = (*pfnCmp)(pp[r-1], pp[r2-1], lParam);

        if (r2 == last)
        {
            if (wRTo2R < 0)
            {
                p = pp[r-1]; pp[r-1] = pp[r2-1]; pp[r2-1] = p;
            }
            break;
        }
        else
        {
            int wR2toR21 = (*pfnCmp)(pp[r2-1], pp[r2+1-1], lParam);

            if (wRTo2R < 0 && wR2toR21 >= 0)
            {
                p = pp[r-1]; pp[r-1] = pp[r2-1]; pp[r2-1] = p;
                r = r2;
            }
            else if ((*pfnCmp)(pp[r-1], pp[r2+1-1], lParam) < 0 && wR2toR21 < 0)
            {
                p = pp[r-1]; pp[r-1] = pp[r2+1-1]; pp[r2+1-1] = p;
                r = r2 + 1;
            }
            else
            {
                break;
            }
        }
    }
}

BOOL  DPA_HeapSort(SORTPARAMS * psp)
{
    void * * pp = psp->pp;
    int c = psp->cp;
    int i;

    for (i = c / 2; i >= 1; i--)
        DPA_HeapSortPushDown(i, c, psp);

    for (i = c; i >= 2; i--)
    {
        void * p = pp[0]; pp[0] = pp[i-1]; pp[i-1] = p;

        DPA_HeapSortPushDown(1, i - 1, psp);
    }
    return TRUE;
}
#endif  // USEHEAPSORT

#if defined(MERGESORT) && defined(WIN32)

#define SortCompare(psp, pp1, i1, pp2, i2) \
        (psp->pfnCmp(pp1[i1], pp2[i2], psp->lParam))

//
//  This function merges two sorted lists and makes one sorted list.
//   psp->pp[iFirst, iFirst+cItes/2-1], psp->pp[iFirst+cItems/2, iFirst+cItems-1]
//
void  DPA_MergeThem(SORTPARAMS * psp, int iFirst, int cItems)
{
    //
    // Notes:
    //  This function is separated from DPA_MergeSort2() to avoid comsuming
    // stack variables. Never inline this.
    //
    int cHalf = cItems/2;
    int iIn1, iIn2, iOut;
    LPVOID * ppvSrc = &psp->pp[iFirst];

    // Copy the first part to temp storage so we can write directly into
    // the final buffer.  Note that this takes at most psp->cp/2 DWORD's
    hmemcpy(psp->ppT, ppvSrc, cHalf*sizeof(LPVOID));

    for (iIn1=0, iIn2=cHalf, iOut=0;;)
    {
        if (SortCompare(psp, psp->ppT, iIn1, ppvSrc, iIn2) <= 0) {
            ppvSrc[iOut++] = psp->ppT[iIn1++];

            if (iIn1==cHalf) {
                // We used up the first half; the rest of the second half
                // should already be in place
                break;
            }
        } else {
            ppvSrc[iOut++] = ppvSrc[iIn2++];
            if (iIn2==cItems) {
                // We used up the second half; copy the rest of the first half
                // into place
                hmemcpy(&ppvSrc[iOut], &psp->ppT[iIn1], (cItems-iOut)*sizeof(LPVOID));
                break;
            }
        }
    }
}

//
//  This function sorts a give list (psp->pp[iFirst,iFirst-cItems-1]).
//
void  DPA_MergeSort2(SORTPARAMS * psp, int iFirst, int cItems)
{
    //
    // Notes:
    //   This function is recursively called. Therefore, we should minimize
    //  the number of local variables and parameters. At this point, we
    //  use one local variable and three parameters.
    //
    int cHalf;

    switch(cItems)
    {
    case 1:
        return;

    case 2:
        // Swap them, if they are out of order.
        if (SortCompare(psp, psp->pp, iFirst, psp->pp, iFirst+1) > 0)
        {
            psp->ppT[0] = psp->pp[iFirst];
            psp->pp[iFirst] = psp->pp[iFirst+1];
            psp->pp[iFirst+1] = psp->ppT[0];
        }
        break;

    default:
        cHalf = cItems/2;
        // Sort each half
        DPA_MergeSort2(psp, iFirst, cHalf);
        DPA_MergeSort2(psp, iFirst+cHalf, cItems-cHalf);
        // Then, merge them.
        DPA_MergeThem(psp, iFirst, cItems);
        break;
    }
}

BOOL  DPA_MergeSort(SORTPARAMS * psp)
{
    if (psp->cp==0)
        return TRUE;

    // Note that we divide by 2 below; we want to round down
    psp->ppT = LocalAlloc(LPTR, psp->cp/2 * sizeof(LPVOID));
    if (!psp->ppT)
        return FALSE;

    DPA_MergeSort2(psp, 0, psp->cp);
    LocalFree(psp->ppT);
    return TRUE;
}
#endif // MERGESORT

// Search function
//
int PUBLIC DPA_Search(HDPA pdpa, void * pFind, int iStart,
            PFNDPACOMPARE pfnCompare, LPARAM lParam, UINT options)
{
    int cp = DPA_GetPtrCount(pdpa);

    ASSERT(pfnCompare);
    ASSERT(0 <= iStart);

    // Only allow these wierd flags if the list is sorted
    ASSERT((options & DPAS_SORTED) || !(options & (DPAS_INSERTBEFORE | DPAS_INSERTAFTER)));

    if (!(options & DPAS_SORTED))
    {
        // Not sorted: do linear search.
        int i;

        for (i = iStart; i < cp; i++)
        {
            if (0 == pfnCompare(pFind, DPA_FastGetPtr(pdpa, i), lParam))
                return i;
        }
        return -1;
    }
    else
    {
        // Search the array using binary search.  If several adjacent 
        // elements match the target element, the index of the first
        // matching element is returned.

        int iRet = -1;      // assume no match
        BOOL bFound = FALSE;
        int nCmp = 0;
        int iLow = 0;       // Don't bother using iStart for binary search
        int iMid = 0;
        int iHigh = cp - 1;

        // (OK for cp == 0)
        while (iLow <= iHigh)
        {
            iMid = (iLow + iHigh) / 2;

            nCmp = pfnCompare(pFind, DPA_FastGetPtr(pdpa, iMid), lParam);

            if (0 > nCmp)
                iHigh = iMid - 1;       // First is smaller
            else if (0 < nCmp)
                iLow = iMid + 1;        // First is larger
            else
            {
                // Match; search back for first match
                bFound = TRUE;
                while (0 < iMid)
                {
                    if (0 != pfnCompare(pFind, DPA_FastGetPtr(pdpa, iMid-1), lParam))
                        break;
                    else
                        iMid--;
                }
                break;
            }
        }

        if (bFound)
        {
            ASSERT(0 <= iMid);
            iRet = iMid;
        }

        // Did the search fail AND
        // is one of the strange search flags set?
        if (!bFound && (options & (DPAS_INSERTAFTER | DPAS_INSERTBEFORE)))
        {
            // Yes; return the index where the target should be inserted
            // if not found
            if (0 < nCmp)       // First is larger
                iRet = iLow;
            else
                iRet = iMid;
            // (We don't distinguish between the two flags anymore)
        }
        else if ( !(options & (DPAS_INSERTAFTER | DPAS_INSERTBEFORE)) )
        {
            // Sanity check with linear search
            ASSERT(DPA_Search(pdpa, pFind, iStart, pfnCompare, lParam, options & ~DPAS_SORTED) == iRet);
        }
        return iRet;
    }
}

//===========================================================================
//
// String ptr management routines
//
// Copy as much of *psz to *pszBuf as will fit
//
int PUBLIC Str_GetPtr(LPCTSTR psz, LPTSTR pszBuf, int cchBuf)
{
    int cch = 0;

    // if pszBuf is NULL, just return length of string.
    //
    if (!pszBuf && psz)
        return lstrlen(psz);

    if (cchBuf)
    {
        if (psz)
        {
            cch = lstrlen(psz);

            if (cch > cchBuf - 1)
                cch = cchBuf - 1;

            hmemcpy(pszBuf, psz, cch * sizeof(TCHAR));
        }
        pszBuf[cch] = 0;
    }
    return cch;
}

// Set *ppsz to a copy of psz, reallocing as needed
//
BOOL PUBLIC Str_SetPtr(LPTSTR * ppsz, LPCTSTR psz)
{
    if (!psz)
    {
        if (*ppsz)
        {
            SharedFree(ppsz);
            *ppsz = NULL;
        }
    }
    else
    {
        LPTSTR pszNew = (LPTSTR)SharedReAlloc(*ppsz, (lstrlen(psz) + 1) * sizeof(TCHAR));
        if (!pszNew)
            return FALSE;

        lstrcpy(pszNew, psz);
        *ppsz = pszNew;
    }
    return TRUE;
}
