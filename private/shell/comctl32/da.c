// Dynamic Array APIs
#include "ctlspriv.h"

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
#define DSA_MAGIC   ('S' | ('A' << 8))
#define IsDSA(pdsa) ((pdsa) && (pdsa)->magic == DSA_MAGIC)
#define DPA_MAGIC   ('P' | ('A' << 8))
#define IsDPA(pdpa) ((pdpa) && (pdpa)->magic == DPA_MAGIC)
#else
#define IsDSA(pdsa)
#define IsDPA(pdsa)
#endif


typedef struct {
    void FAR* FAR* pp;
    PFNDPACOMPARE pfnCmp;
    LPARAM lParam;
    int cp;
#ifdef MERGESORT
    void FAR* FAR* ppT;
#endif
} SORTPARAMS;

BOOL NEAR DPA_QuickSort(SORTPARAMS FAR* psp);
BOOL NEAR DPA_QuickSort2(int i, int j, SORTPARAMS FAR* psp);
BOOL NEAR DPA_HeapSort(SORTPARAMS FAR* psp);
void NEAR DPA_HeapSortPushDown(int first, int last, SORTPARAMS FAR* psp);
BOOL NEAR DPA_MergeSort(SORTPARAMS FAR* psp);
void NEAR DPA_MergeSort2(SORTPARAMS FAR* psp, int iFirst, int cItems);



//========== Dynamic structure array ====================================

// Dynamic structure array

typedef struct _DSA {
// NOTE: The following field MUST be defined at the beginning of the
// structure in order for GetItemCount() to work.
//
    int cItem;		// # of elements in dsa

    void FAR* aItem;	// memory for elements
    int cItemAlloc;	// # items which fit in aItem
    int cbItem;		// size of each item
    int cItemGrow;	// # items to grow cItemAlloc by
#ifdef DEBUG
    UINT magic;
#endif
} DSA;

#define DSA_PITEM(pdsa, index)    ((void FAR*)(((BYTE FAR*)(pdsa)->aItem) + ((index) * (pdsa)->cbItem)))


#ifdef DEBUG
#define BF_ONDAVALIDATE     0x00001000

void DABreakFn(void)
{
    if (IsFlagSet(g_dwBreakFlags, BF_ONDAVALIDATE))
        ASSERT(0);
}

#define DABreak()    DABreakFn()
#else
#define DABreak()
#endif


HDSA WINAPI DSA_Create(int cbItem, int cItemGrow)
{
    HDSA pdsa = Alloc(sizeof(DSA));

    ASSERT(cbItem);

    if (pdsa)
    {
        ASSERT(pdsa->cItem == 0);
        ASSERT(pdsa->cItemAlloc == 0);
        pdsa->cbItem = cbItem;
        pdsa->cItemGrow = (cItemGrow == 0 ? 1 : cItemGrow);
        ASSERT(pdsa->aItem == NULL);
#ifdef DEBUG
        pdsa->magic = DSA_MAGIC;
#endif
    }
    return pdsa;
}

BOOL WINAPI DSA_Destroy(HDSA pdsa)
{

    if (pdsa == NULL)       // allow NULL for low memory cases
        return TRUE;

    // Components rely on not having to check for NULL
    ASSERT(IsDSA(pdsa));

#ifdef DEBUG
    pdsa->cItem = 0;
    pdsa->cItemAlloc = 0;
    pdsa->cbItem = 0;
    pdsa->magic = 0;
#endif
    if (pdsa->aItem && !Free(pdsa->aItem))
        return FALSE;

    return Free(pdsa);
}

void WINAPI DSA_EnumCallback(HDSA pdsa, PFNDSAENUMCALLBACK pfnCB, LPVOID pData)
{
    int i;
    
    if (!pdsa)
        return;
    
    ASSERT(IsDSA(pdsa));

    for (i = 0; i < pdsa->cItem; i++) {
        if (!pfnCB(DSA_GetItemPtr(pdsa, i), pData))
            break;
    }
}

void WINAPI DSA_DestroyCallback(HDSA pdsa, PFNDSAENUMCALLBACK pfnCB, LPVOID pData)
{
    DSA_EnumCallback(pdsa, pfnCB, pData);
    DSA_Destroy(pdsa);
}


BOOL WINAPI DSA_GetItem(HDSA pdsa, int index, void FAR* pitem)
{
    ASSERT(IsDSA(pdsa));
    ASSERT(pitem);

    if (index < 0 || index >= pdsa->cItem)
    {
#ifdef DEBUG
        // Don't assert if index == pdsa->cItems as some clients simply want to walk the list and no need to call getcount...

        if (index != pdsa->cItem)
        {
            DebugMsg(DM_ERROR, TEXT("DSA: GetItem: Invalid index: %d"), index);
            DABreak();
        }
#endif
        return FALSE;
    }

    hmemcpy(pitem, DSA_PITEM(pdsa, index), pdsa->cbItem);
    return TRUE;
}

void FAR* WINAPI DSA_GetItemPtr(HDSA pdsa, int index)
{
    ASSERT(IsDSA(pdsa));

    if (index < 0 || index >= pdsa->cItem)
    {
        DebugMsg(DM_ERROR, TEXT("DSA: GetItemPtr: Invalid index: %d"), index);
        // DABreak();   // caller knows 
        return NULL;
    }
    return DSA_PITEM(pdsa, index);
}

BOOL WINAPI DSA_SetItem(HDSA pdsa, int index, void FAR* pitem)
{
    ASSERT(pitem);
    ASSERT(IsDSA(pdsa));

    if (index < 0)
    {
        DebugMsg(DM_ERROR, TEXT("DSA: SetItem: Invalid index: %d"), index);
        DABreak();
        return FALSE;
    }

    if (index >= pdsa->cItem)
    {
        if (index + 1 > pdsa->cItemAlloc)
        {
            int cItemAlloc = (((index + 1) + pdsa->cItemGrow - 1) / pdsa->cItemGrow) * pdsa->cItemGrow;

            void FAR* aItemNew = ReAlloc(pdsa->aItem, cItemAlloc * pdsa->cbItem);
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

int WINAPI DSA_InsertItem(HDSA pdsa, int index, void FAR* pitem)
{
    ASSERT(pitem);
    ASSERT(IsDSA(pdsa));

    if (index < 0)
    {
        DebugMsg(DM_ERROR, TEXT("DSA: InsertItem: Invalid index: %d"), index);
        DABreak();
        return -1;
    }

    if (index > pdsa->cItem)
        index = pdsa->cItem;

    if (pdsa->cItem + 1 > pdsa->cItemAlloc)
    {
        void FAR* aItemNew = ReAlloc(pdsa->aItem,
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

BOOL WINAPI DSA_DeleteItem(HDSA pdsa, int index)
{
    ASSERT(IsDSA(pdsa));

    if (index < 0 || index >= pdsa->cItem)
    {
        DebugMsg(DM_ERROR, TEXT("DSA: DeleteItem: Invalid index: %d"), index);
        DABreak();
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
        void FAR* aItemNew = ReAlloc(pdsa->aItem,
                (pdsa->cItemAlloc - pdsa->cItemGrow) * pdsa->cbItem);
        if (aItemNew)
            pdsa->aItem = aItemNew;
        else
        {
            // If the shrink fails, then just continue with the old (slightly
            // too big) allocation.  Go ahead and let cItemAlloc decrease
            // so we don't keep trying to realloc smaller
        }
        pdsa->cItemAlloc -= pdsa->cItemGrow;
    }
    return TRUE;
}

BOOL WINAPI DSA_DeleteAllItems(HDSA pdsa)
{
    ASSERT(IsDSA(pdsa));

    if (pdsa->aItem && !Free(pdsa->aItem))
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
    void FAR* FAR* pp;

    HANDLE hheap;        // Heap to allocate from if NULL use shared

    int cpAlloc;
    int cpGrow;
#ifdef DEBUG
    UINT magic;
#endif
} DPA;



HDPA WINAPI DPA_Create(int cpGrow)
{
    return DPA_CreateEx(cpGrow, NULL);
}

// Should nuke the standard DPA above...
HDPA WINAPI DPA_CreateEx(int cpGrow, HANDLE hheap)
{
    HDPA pdpa;
    if (hheap == NULL)
    {
#ifdef WIN32
#ifdef WINNT
        hheap = GetProcessHeap();
#else
        hheap = GetSharedHeapHandle();
#endif
#endif

        pdpa = ALLOC_NULLHEAP(hheap, sizeof(DPA));
    }
    else
        pdpa = ControlAlloc(hheap, sizeof(DPA));
    if (pdpa)
    {
        ASSERT(pdpa->cp == 0);
        ASSERT(pdpa->cpAlloc == 0);
        pdpa->cpGrow = (cpGrow < 8 ? 8 : cpGrow);
        ASSERT(pdpa->pp == NULL);
        pdpa->hheap = hheap;
#ifdef DEBUG
        pdpa->magic = DPA_MAGIC;
#endif
    }
    return pdpa;
}

BOOL WINAPI DPA_Destroy(HDPA pdpa)
{
    if (pdpa == NULL)       // allow NULL for low memory cases, still assert
        return TRUE;

    ASSERT(IsDPA(pdpa));

#ifndef UNIX
    ASSERT(pdpa->hheap);
#endif

#ifdef DEBUG
    pdpa->cp = 0;
    pdpa->cpAlloc = 0;
    pdpa->magic = 0;
#endif
    if (pdpa->pp && !ControlFree(pdpa->hheap, pdpa->pp))
        return FALSE;

    return ControlFree(pdpa->hheap, pdpa);
}

HDPA WINAPI DPA_Clone(HDPA pdpa, HDPA pdpaNew)
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
    hmemcpy(pdpaNew->pp, pdpa->pp, pdpa->cp * sizeof(void FAR*));

    return pdpaNew;
}

void FAR* WINAPI DPA_GetPtr(HDPA pdpa, INT_PTR index)
{
    ASSERT(IsDPA(pdpa));

    if (!pdpa || index < 0 || index >= pdpa->cp)
        return NULL;

    return pdpa->pp[index];
}

int WINAPI DPA_GetPtrIndex(HDPA pdpa, void FAR* p)
{
    void FAR* FAR* pp;
    void FAR* FAR* ppMax;

    ASSERT(IsDPA(pdpa));
    if (pdpa && pdpa->pp)
    {
        pp = pdpa->pp;
        ppMax = pp + pdpa->cp;
        for ( ; pp < ppMax; pp++)
        {
            if (*pp == p)
                return (int) (pp - pdpa->pp);
        }
    }
    return -1;
}

BOOL WINAPI DPA_Grow(HDPA pdpa, int cpAlloc)
{
    ASSERT(IsDPA(pdpa));

    if (!pdpa)
        return FALSE;

    if (cpAlloc > pdpa->cpAlloc)
    {
        void FAR* FAR* ppNew;

        cpAlloc = ((cpAlloc + pdpa->cpGrow - 1) / pdpa->cpGrow) * pdpa->cpGrow;

        if (pdpa->pp)
            ppNew = (void FAR* FAR*)ControlReAlloc(pdpa->hheap, pdpa->pp, cpAlloc * sizeof(void FAR*));
        else
            ppNew = (void FAR* FAR*)ControlAlloc(pdpa->hheap, cpAlloc * sizeof(void FAR*));
        if (!ppNew)
            return FALSE;

        pdpa->pp = ppNew;
        pdpa->cpAlloc = cpAlloc;

        //
        // Grow more agressively as we get bigger, up to a maximum of
        // 512 at a time.  Note, we'll only hit our outer bound growth
        // at a time limit once we've already got that many items in the
        // DPA anyway...
        //
        if (pdpa->cpGrow < 256)
        {
            pdpa->cpGrow = pdpa->cpGrow << 1;
        }
    }
    return TRUE;
}

BOOL WINAPI DPA_SetPtr(HDPA pdpa, int index, void FAR* p)
{
    ASSERT(IsDPA(pdpa));

    if (!pdpa)
        return FALSE;

    if (index < 0)
    {
        DebugMsg(DM_ERROR, TEXT("DPA: SetPtr: Invalid index: %d"), index);
        DABreak();
        return FALSE;
    }

    if (index >= pdpa->cp)
    {
        if (!DPA_Grow(pdpa, index + 1))
            return FALSE;
        // If we grew by more than one, must zero-init all the stuff in the middle
        ZeroMemory(pdpa->pp + pdpa->cp, sizeof(LPVOID) * (index - pdpa->cp));
        pdpa->cp = index + 1;
    }

    pdpa->pp[index] = p;

    return TRUE;
}

int WINAPI DPA_InsertPtr(HDPA pdpa, int index, void FAR* p)
{
    ASSERT(IsDPA(pdpa));

    if (!pdpa)
        return -1;

    if (index < 0)
    {
        DebugMsg(DM_ERROR, TEXT("DPA: InsertPtr: Invalid index: %d"), index);
        DABreak();
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
            (pdpa->cp - index) * sizeof(void FAR*));
    }

    pdpa->pp[index] = p;
    pdpa->cp++;

    return index;
}

void FAR* WINAPI DPA_DeletePtr(HDPA pdpa, int index)
{
    void FAR* p;

    ASSERT(IsDPA(pdpa));

    if (!pdpa)
        return FALSE;

    if (index < 0 || index >= pdpa->cp)
    {
        DebugMsg(DM_ERROR, TEXT("DPA: DeltePtr: Invalid index: %d"), index);
        DABreak();
        return NULL;
    }

    p = pdpa->pp[index];

    if (index < pdpa->cp - 1)
    {
        hmemcpy(&pdpa->pp[index], &pdpa->pp[index + 1],
            (pdpa->cp - (index + 1)) * sizeof(void FAR*));
    }
    pdpa->cp--;

    if (pdpa->cpAlloc - pdpa->cp > pdpa->cpGrow)
    {
        void FAR* FAR* ppNew;
        ppNew = ControlReAlloc(pdpa->hheap, pdpa->pp, (pdpa->cpAlloc - pdpa->cpGrow) * sizeof(void FAR*));

        if (ppNew)
            pdpa->pp = ppNew;
        else
        {
            // If the shrink fails, then just continue with the old (slightly
            // too big) allocation.  Go ahead and let cpAlloc decrease
            // so we don't keep trying to realloc smaller
        }
        pdpa->cpAlloc -= pdpa->cpGrow;
    }
    return p;
}

BOOL WINAPI DPA_DeleteAllPtrs(HDPA pdpa)
{
    if (!pdpa)
        return FALSE;

    ASSERT(IsDPA(pdpa));

    if (pdpa->pp && !ControlFree(pdpa->hheap, pdpa->pp))
        return FALSE;
    pdpa->pp = NULL;
    pdpa->cp = pdpa->cpAlloc = 0;
    return TRUE;
}

void WINAPI DPA_EnumCallback(HDPA pdpa, PFNDPAENUMCALLBACK pfnCB, LPVOID pData)
{
    int i;
    
    if (!pdpa)
        return;
    
    ASSERT(IsDPA(pdpa));

    for (i = 0; i < pdpa->cp; i++) {
        if (!pfnCB(DPA_FastGetPtr(pdpa, i), pData))
            break;
    }
}

void WINAPI DPA_DestroyCallback(HDPA pdpa, PFNDPAENUMCALLBACK pfnCB, LPVOID pData)
{
    DPA_EnumCallback(pdpa, pfnCB, pData);
    DPA_Destroy(pdpa);
}


typedef struct _DPASTREAMHEADER
{
    DWORD cbSize;       // Size of entire stream
    DWORD dwVersion;    // For versioning
    int   celem;
} DPASTREAMHEADER;

#define DPASTREAM_VERSION   1


/*----------------------------------------------------------
Purpose: Saves the DPA to a stream by writing out a header,
         and then calling the given callback to write each
         element.

         The callback can end the write early by returning 
         something other than S_OK.  Returning an error will
         cancel the entire write.  Returning S_FALSE will 
         stop the write.

Returns: S_OK or S_FALSE for success.  
         S_FALSE only if callback stops early
         errors
*/
HRESULT
WINAPI
DPA_SaveStream(
    IN HDPA         pdpa,
    IN PFNDPASTREAM pfn,
    IN IStream *    pstm,
    IN LPVOID       pvInstData)
{
    HRESULT hres = E_INVALIDARG;

    if (IS_VALID_HANDLE(pdpa, DPA) &&
        IS_VALID_CODE_PTR(pstm, IStream *) &&
        IS_VALID_CODE_PTR(pfn, PFNDPASTREAM))
    {
        DPASTREAMHEADER header;
        LARGE_INTEGER dlibMove = { 0 };
        ULARGE_INTEGER ulPosBegin;

        // Get the current seek position, so we can update the header
        // once we know how much we've written
        hres = pstm->lpVtbl->Seek(pstm, dlibMove, STREAM_SEEK_CUR, &ulPosBegin);
        if (SUCCEEDED(hres))
        {
            // Write the header (we will update some of this once we're
            // finished)
            header.cbSize = 0;
            header.dwVersion = DPASTREAM_VERSION;
            header.celem = 0;

            // First write out the header
            hres = pstm->lpVtbl->Write(pstm, &header, sizeof(header), NULL);

            if (SUCCEEDED(hres))
            {
                DPASTREAMINFO info;
                int cel = DPA_GetPtrCount(pdpa);
                LPVOID * ppv = DPA_GetPtrPtr(pdpa);

                // This keeps the count of what is actually written
                info.iPos = 0;

                // Write each element
                for (; 0 < cel; cel--, ppv++) 
                {
                    info.pvItem = *ppv;
                    hres = pfn(&info, pstm, pvInstData);

                    // Returning S_FALSE from callback means it didn't
                    // write anything for this element, so don't increment 
                    // the iPos (which refers to the count written).

                    if (S_OK == hres)
                        info.iPos++;
                    else if (FAILED(hres))
                    {
                        hres = S_FALSE;
                        break;
                    }
                }

                if (FAILED(hres))
                {
                    // Reposition pointer to beginning
                    dlibMove.LowPart = ulPosBegin.LowPart;
                    dlibMove.HighPart = ulPosBegin.HighPart;
                    pstm->lpVtbl->Seek(pstm, dlibMove, STREAM_SEEK_SET, NULL);
                }
                else
                {
                    ULARGE_INTEGER ulPosEnd;

                    // Calculate how much was written
                    hres = pstm->lpVtbl->Seek(pstm, dlibMove, STREAM_SEEK_CUR, 
                                              &ulPosEnd);
                    if (SUCCEEDED(hres))
                    {
                        // We only save the low part
                        ASSERT(ulPosEnd.HighPart == ulPosBegin.HighPart);

                        // Update the header
                        header.celem = info.iPos;
                        header.cbSize = ulPosEnd.LowPart - ulPosBegin.LowPart;

                        dlibMove.LowPart = ulPosBegin.LowPart;
                        dlibMove.HighPart = ulPosBegin.HighPart;
                        pstm->lpVtbl->Seek(pstm, dlibMove, STREAM_SEEK_SET, NULL);
                        pstm->lpVtbl->Write(pstm, &header, sizeof(header), NULL);

                        // Reposition pointer
                        dlibMove.LowPart = ulPosEnd.LowPart;
                        dlibMove.HighPart = ulPosEnd.HighPart;
                        pstm->lpVtbl->Seek(pstm, dlibMove, STREAM_SEEK_SET, NULL);
                    }
                }
            }
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: Loads the DPA from a stream by calling the given callback 
         to read each element.

         The callback can end the read early by returning 
         something other than S_OK.  

Returns: S_OK on success
         S_FALSE if the callback aborted early or the stream ended
           abruptly. DPA is partially filled.
         error on anything else
*/
HRESULT
WINAPI
DPA_LoadStream(
    OUT HDPA *      ppdpa,
    IN PFNDPASTREAM pfn,
    IN IStream *    pstm,
    IN LPVOID       pvInstData)
{
    HRESULT hres = E_INVALIDARG;

    if (IS_VALID_WRITE_PTR(ppdpa, HDPA) &&
        IS_VALID_CODE_PTR(pstm, IStream *) &&
        IS_VALID_CODE_PTR(pfn, PFNDPASTREAM))
    {
        DPASTREAMHEADER header;
        LARGE_INTEGER dlibMove = { 0 };
        ULARGE_INTEGER ulPosBegin;
        ULONG cbRead;

        *ppdpa = NULL;

        // Get the current seek position so we can position pointer 
        // correctly upon error.
        hres = pstm->lpVtbl->Seek(pstm, dlibMove, STREAM_SEEK_CUR, &ulPosBegin);
        if (SUCCEEDED(hres))
        {
            // Read the header
            hres = pstm->lpVtbl->Read(pstm, &header, sizeof(header), &cbRead);
            if (SUCCEEDED(hres))
            {
                if (sizeof(header) > cbRead ||
                    sizeof(header) > header.cbSize ||
                    DPASTREAM_VERSION != header.dwVersion)
                {
                    hres = E_FAIL;
                }
                else
                {
                    // Create the list 
                    HDPA pdpa = DPA_Create(header.celem);
                    if ( !pdpa || !DPA_Grow(pdpa, header.celem))
                        hres = E_OUTOFMEMORY;
                    else
                    {
                        // Read each element
                        DPASTREAMINFO info;
                        LPVOID * ppv = DPA_GetPtrPtr(pdpa);

                        for (info.iPos = 0; info.iPos < header.celem; ) 
                        {
                            info.pvItem = NULL;
                            hres = pfn(&info, pstm, pvInstData);

                            // Returning S_FALSE from the callback means
                            // it skipped this stream element.
                            // Don't increment iPos (which refers to the
                            // count read).
                            if (S_OK == hres)
                            {
                                *ppv = info.pvItem;

                                info.iPos++;
                                ppv++;    
                            }
                            else if (FAILED(hres))
                            {
                                hres = S_FALSE;
                                break;
                            }
                        }

                        pdpa->cp = info.iPos;
                        *ppdpa = pdpa;
                    }
                }

                // Reposition pointer if we failed
                if (S_OK != hres)
                {
                    if (S_FALSE == hres)
                    {
                        // Position pointer to the end
                        dlibMove.LowPart = ulPosBegin.LowPart + header.cbSize;
                    }
                    else
                    {
                        // Position pointer to beginning 
                        dlibMove.LowPart = ulPosBegin.LowPart;
                    }
                    dlibMove.HighPart = ulPosBegin.HighPart;
                    pstm->lpVtbl->Seek(pstm, dlibMove, STREAM_SEEK_SET, NULL);
                }
            }
        }

        ASSERT(SUCCEEDED(hres) && *ppdpa ||
               FAILED(hres) && NULL == *ppdpa);
    }

    return hres;
}



/*----------------------------------------------------------
Purpose: Merge two DPAs.  This takes two arrays and merges the
         source array into the destination.   

         Merge options:

          DPAM_SORTED       The arrays are already sorted; don't sort
          DPAM_UNION        The resulting array is the union of all elements
                            in both arrays.
          DPAM_INTERSECT    Only elements in the source array that intersect
                            with the dest array are merged.  
          DPAM_NORMAL       Like DPAM_INTERSECT except the dest array 
                            also maintains its original, additional elements.


Returns: S_OK for success.  
         errors if merge fails

Cond:    --
*/
BOOL
WINAPI
DPA_Merge(
    IN HDPA          pdpaDest,
    IN HDPA          pdpaSrc,
    IN DWORD         dwFlags,
    IN PFNDPACOMPARE pfnCompare,
    IN PFNDPAMERGE   pfnMerge,
    IN LPARAM        lParam)
{
    BOOL bRet = FALSE;

    if (IS_VALID_HANDLE(pdpaSrc, DPA) &&
        IS_VALID_HANDLE(pdpaDest, DPA) &&
        IS_VALID_CODE_PTR(pfnCompare, PFNDPACOMPARE) &&
        IS_VALID_CODE_PTR(pfnMerge, PFNDPAMERGE))
    {
        int iSrc;
        int iDest;
        int nCmp;
        LPVOID * ppvSrc;
        LPVOID * ppvDest;

        bRet = TRUE;

        // Are the arrays already sorted?
        if ( !(dwFlags & DPAM_SORTED) )
        {
            // No; sort them
            DPA_Sort(pdpaSrc, pfnCompare, lParam);
            DPA_Sort(pdpaDest, pfnCompare, lParam);
        }

        // This merges in-place. The size of the resulting DPA 
        // depends on the options:
        //
        //   DPAM_NORMAL    Same size as the dest DPA before 
        //                  the merge.  
        //
        //   DPAM_UNION     Min size is the larger of the two.
        //                  Max size is the sum of the two.
        //
        //   DPAM_INTERSECT Min size is zero.
        //                  Max size is the smaller of the two.
        // 
        // We iterate backwards to minimize the amount of moves we 
        // incur by calling DPA_DeletePtr.
        //

        iSrc = pdpaSrc->cp - 1;
        iDest = pdpaDest->cp - 1;
        ppvSrc = &DPA_FastGetPtr(pdpaSrc, iSrc);
        ppvDest = &DPA_FastGetPtr(pdpaDest, iDest);

        while (0 <= iSrc && 0 <= iDest)
        {
            LPVOID pv;

            nCmp = pfnCompare(*ppvDest, *ppvSrc, lParam);

            if (0 == nCmp)
            {
                // Elements match; merge them.  
                pv = pfnMerge(DPAMM_MERGE, *ppvDest, *ppvSrc, lParam);
                if (NULL == pv)
                {
                    bRet = FALSE;
                    break;
                }
                *ppvDest = pv;

                iSrc--;
                ppvSrc--;
                iDest--;
                ppvDest--;
            }
            else if (0 < nCmp)
            {
                // pvSrc < pvDest. The source array doesn't have pvDest.
                if (dwFlags & DPAM_INTERSECT)
                {
                    // Delete pvDest 
                    pfnMerge(DPAMM_DELETE, DPA_DeletePtr(pdpaDest, iDest), NULL, lParam);
                }
                else
                {
                    ; // Keep it (do nothing)
                }

                // Move onto the next element in the dest array
                iDest--;
                ppvDest--;
            }
            else
            {
                // pvSrc > pvDest. The dest array doesn't have pvSrc.
                if (dwFlags & DPAM_UNION)
                {
                    // Add pvSrc
                    pv = pfnMerge(DPAMM_INSERT, *ppvSrc, NULL, lParam);
                    if (NULL == pv)
                    {
                        bRet = FALSE;
                        break;
                    }

                    DPA_InsertPtr(pdpaDest, iDest+1, pv);
                    // DPA_InsertPtr may end up reallocating the pointer array
                    // thus making ppvDest invalid
                    ppvDest = &DPA_FastGetPtr(pdpaDest, iDest);
                }
                else
                {
                    ;  // Skip it (do nothing)
                }

                // Move onto the next element in the source array
                iSrc--;
                ppvSrc--;
            }
        }
        // there are some items left in src
        if ((dwFlags & DPAM_UNION) && 0 <= iSrc)
        {
            for (; 0 <= iSrc; iSrc--, ppvSrc--)
            {
                LPVOID pv = pfnMerge(DPAMM_INSERT, *ppvSrc, NULL, lParam);
                if (NULL == pv)
                {
                    bRet = FALSE;
                    break;
                }
                DPA_InsertPtr(pdpaDest, 0, pv);
            }
        }
    }

    return bRet;
}


BOOL WINAPI DPA_Sort(HDPA pdpa, PFNDPACOMPARE pfnCmp, LPARAM lParam)
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

BOOL NEAR DPA_QuickSort(SORTPARAMS FAR* psp)
{
    return DPA_QuickSort2(0, psp->cp - 1, psp);
}

BOOL NEAR DPA_QuickSort2(int i, int j, SORTPARAMS FAR* psp)
{
    void FAR* FAR* pp = psp->pp;
    LPARAM lParam = psp->lParam;
    PFNDPACOMPARE pfnCmp = psp->pfnCmp;

    int iPivot;
    void FAR* pFirst;
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
        void FAR* pivot = pp[iPivot];

        do
        {
            void FAR* p;

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

void NEAR DPA_HeapSortPushDown(int first, int last, SORTPARAMS FAR* psp)
{
    void FAR* FAR* pp = psp->pp;
    LPARAM lParam = psp->lParam;
    PFNDPACOMPARE pfnCmp = psp->pfnCmp;
    int r;
    int r2;
    void FAR* p;

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

BOOL NEAR DPA_HeapSort(SORTPARAMS FAR* psp)
{
    void FAR* FAR* pp = psp->pp;
    int c = psp->cp;
    int i;

    for (i = c / 2; i >= 1; i--)
        DPA_HeapSortPushDown(i, c, psp);

    for (i = c; i >= 2; i--)
    {
        void FAR* p = pp[0]; pp[0] = pp[i-1]; pp[i-1] = p;

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
void NEAR DPA_MergeThem(SORTPARAMS FAR* psp, int iFirst, int cItems)
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
void NEAR DPA_MergeSort2(SORTPARAMS FAR* psp, int iFirst, int cItems)
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

BOOL NEAR DPA_MergeSort(SORTPARAMS FAR* psp)
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
int WINAPI DPA_Search(HDPA pdpa, void FAR* pFind, int iStart,
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
// Warning:  this same code is duplicated below.
//
int WINAPI Str_GetPtr(LPCTSTR pszCurrent, LPTSTR pszBuf, int cchBuf)
{
    int cchToCopy;

    if (!pszCurrent)
    {
        ASSERT(FALSE);
        
        if (cchBuf > 0)
            *pszBuf = TEXT('\0');

        return 0;
    }

    cchToCopy = lstrlen(pszCurrent);

    // if pszBuf is NULL, or they passed cchBuf = 0, return the needed buff size
    if (!pszBuf || !cchBuf)
        return cchToCopy + 1;
    
    if (cchToCopy >= cchBuf)
        cchToCopy = cchBuf - 1;

    hmemcpy(pszBuf, pszCurrent, cchToCopy * SIZEOF(TCHAR));
    pszBuf[cchToCopy] = TEXT('\0');

    return cchToCopy + 1;
}

#ifdef DEBUG
//
//  Str_GetPtr0 is just like Str_GetPtr except that it doesn't assert if
//  pszCurrent = NULL.
//
int WINAPI Str_GetPtr0(LPCTSTR pszCurrent, LPTSTR pszBuf, int cchBuf)
{
    return Str_GetPtr(pszCurrent ? pszCurrent : c_szNULL, pszBuf, cchBuf);
}
#endif

#ifdef UNICODE
//
// If we are build Unicode, then this is the ANSI version
// of the above function.
//

int WINAPI Str_GetPtrA(LPCSTR pszCurrent, LPSTR pszBuf, int cchBuf)
{
    int cchToCopy;

    if (!pszCurrent)
    {
        ASSERT(FALSE);

        if (cchBuf > 0)
            *pszBuf = '\0';

        return 0;
    }

    cchToCopy = lstrlenA(pszCurrent);

    // if pszBuf is NULL, or they passed cchBuf = 0, return the needed buff size
    if (!pszBuf || !cchBuf)
        return cchToCopy + 1;
    
    if (cchToCopy >= cchBuf)
        cchToCopy = cchBuf - 1;

    // BUGBUG: Must call TruncateString, as we may be in the middle of DBCS char
    hmemcpy(pszBuf, pszCurrent, cchToCopy * SIZEOF(CHAR));
    pszBuf[cchToCopy] = TEXT('\0');

    return cchToCopy + 1;
}

#else

//
// Unicode stub if this code is built ANSI
//

int WINAPI Str_GetPtrW(LPCWSTR psz, LPWSTR pszBuf, int cchBuf)
{
    SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
    return -1;
}

#endif

#ifdef WIN32

//
// This function is not exported.
//

BOOL Str_Set(LPTSTR *ppsz, LPCTSTR psz)
{
    if (!psz || (psz == LPSTR_TEXTCALLBACK))
    {
        if (*ppsz)
        {
            if (*ppsz != (LPSTR_TEXTCALLBACK))
                LocalFree(*ppsz);
        }
        *ppsz = (LPTSTR)psz;
    }
    else
    {
        LPTSTR pszNew = *ppsz;
        UINT cbSize = (lstrlen(psz) + 1) * sizeof(TCHAR);

        if (pszNew == LPSTR_TEXTCALLBACK)
            pszNew = NULL;
        
        pszNew = CCLocalReAlloc(pszNew, cbSize);

        if (!pszNew)
            return FALSE;

        lstrcpy(pszNew, psz);
        *ppsz = pszNew;
    }
    return TRUE;
}
#endif

// Set *ppszCurrent to a copy of pszNew, and free the previous value, if necessary
//
// WARNING:  This same code is duplicated below
//
BOOL WINAPI Str_SetPtr(LPTSTR * ppszCurrent, LPCTSTR pszNew)
{
    int cchLength;
    LPTSTR pszOld;
    LPTSTR pszNewCopy = NULL;

    if (pszNew)
    {
        cchLength = lstrlen(pszNew);

        // alloc a new buffer w/ room for the null terminator
        pszNewCopy = (LPTSTR) Alloc((cchLength + 1) * SIZEOF(TCHAR));

        if (!pszNewCopy)
            return FALSE;

        lstrcpyn(pszNewCopy, pszNew, cchLength + 1);
    }
    
    pszOld = InterlockedExchangePointer((LPVOID *)ppszCurrent, pszNewCopy);

    if (pszOld)
        Free(pszOld);

    return TRUE;
}

#ifdef UNICODE
//
// ANSI stub when built Unicode.
//

BOOL WINAPI Str_SetPtrA(LPSTR * ppszCurrent, LPCSTR pszNew)
{
    int cchLength;
    LPSTR pszOld;
    LPSTR pszNewCopy = NULL;

    if (pszNew)
    {
        cchLength = lstrlenA(pszNew);

        // alloc a new buffer w/ room for the null terminator
        pszNewCopy = (LPSTR) Alloc((cchLength + 1) * SIZEOF(CHAR));

        if (!pszNewCopy)
            return FALSE;

        lstrcpynA(pszNewCopy, pszNew, cchLength + 1);
    }

    pszOld = InterlockedExchangePointer((LPVOID *)ppszCurrent, pszNewCopy);

    if (pszOld)
        Free(pszOld);

    return TRUE;
}

#else
// Unicode stub if this is built ANSI

BOOL WINAPI Str_SetPtrW(LPWSTR *ppwzCurrent, LPCWSTR pszNew)
{
    int cchLength;
    LPWSTR pwzOld;
    LPWSTR pwzNewCopy = NULL;

    if (pszNew)
    {
        cchLength = lstrlenW(pszNew);       // Yes this is implemented on Win95.

        // alloc a new buffer w/ room for the null terminator
        pwzNewCopy = (LPWSTR) Alloc((cchLength + 1) * SIZEOF(WCHAR));

        if (!pwzNewCopy)
            return FALSE;

        // lstrcpynW is thunked in unicwrap.cpp for Win95 machines.
        StrCpyNW(pwzNewCopy, pszNew, cchLength + 1);
    }

    pwzOld = InterlockedExchangePointer((LPVOID *)ppwzCurrent, pwzNewCopy);

    if (pwzOld)
        Free(pwzOld);

    return TRUE;
}
#endif
