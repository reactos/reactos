#include "priv.h"

//***   FDSA -- small/fast DSA routines
// DESCRIPTION
//  We attempt to behave as much like an array as possible (semantics
//  and performance and to some extent allocation).  In particular,
//  - indexing (FDSA_GetItemXxx) is done entirely inline; and...
//  - ... involves only 1 extra indirection vs. a true array; and...
//  - ... knows the data type so sizeof is a constant; and...
//  - ... does no range checks (except possibly in DEBUG versions).
//  - part of the FDSA is statically alloc'ed, so one can put critical
//  items in the static part to ensure that they'll be there even under
//  low memory conditions.
// NOTES
//  For now we only implement:
//      Initialize, Destroy, GetItemPtr, GetItemCount, InsertItem, AppendItem

// BUGBUG eventually these come from comctl...
#define DABreak()   /*NOTHING*/
// ReAlloc and Free come from inc/heapaloc.h

//***   SDSA_PITEM -- get item the hard way
//
#define SDSA_PITEM(pfdsa, i) \
    ((void *)(((BYTE *)(pfdsa)->aItem) + ((i) * (pfdsa)->cbItem)))

//***   FDSA_Initialize -- initialize
//
BOOL WINAPI FDSA_Initialize(int cbItem, int cItemGrow,
    PFDSA pfdsa, void * aItemStatic, int cItemStatic)
{
    ASSERT(pfdsa != NULL);      // BUGBUG how handle?

    if (cItemGrow == 0)
        cItemGrow = 1;

    // for implementation simplicity, cItemStatic must be a multiple of
    // cItemGrow.  o.w. our 1st grow from static->dynamic is messy and
    // probably buggy.
    if (cItemStatic % cItemGrow != 0) {
        AssertMsg(0, TEXT("CItemStatic must be a multiple of cItemGrow"));
        return FALSE;
    }

    if (aItemStatic != NULL) {
        // since we're (eventually) in comctl, we can't assume caller's
        // buffer was 0'ed
        ZeroMemory(aItemStatic, cItemStatic * cbItem);
    }

    if (pfdsa) {
        pfdsa->cItem = 0;
        pfdsa->cItemAlloc = cItemStatic;
        pfdsa->aItem = aItemStatic;
        pfdsa->fAllocated = FALSE;

        pfdsa->cbItem = cbItem;
        ASSERT(pfdsa->cbItem == cbItem);        // bitfield overflow

        pfdsa->cItemGrow = cItemGrow;
        ASSERT(pfdsa->cItemGrow == cItemGrow);  // bitfield overflow
    }

    return TRUE;
}

BOOL WINAPI FDSA_Destroy(PFDSA pdsa)
{
    if (pdsa == NULL)       // allow NULL for low memory cases
        return TRUE;

    if (pdsa->fAllocated && pdsa->aItem && !LocalFree(pdsa->aItem))
        return FALSE;

    return TRUE;
}

void* _LocalReAlloc(void* p, UINT uBytes)
{
    if (uBytes) {
        if (p) {
            return LocalReAlloc(p, uBytes, LMEM_MOVEABLE | LMEM_ZEROINIT);
        } else {
            return LocalAlloc(LPTR, uBytes);
        }
    } else {
        if (p)
            LocalFree(p);
        return NULL;
    }
}

//***   FDSA_InsertItem -- insert an item
// ENTRY/EXIT
//  index   insertion point; index > count (e.g. DA_LAST) means append
// NOTES
//  param called 'pdsa' (vs. pfdsa) for easy diff'ing w/ DSA_InsertItem
int WINAPI FDSA_InsertItem(PFDSA pdsa, int index, void FAR* pitem)
{
    ASSERT(pitem);

    if (index < 0) {
        TraceMsg(DM_ERROR, "FDSA: InsertItem: Invalid index: %d", index);
        DABreak();
        return -1;
    }

    if (index > pdsa->cItem)
        index = pdsa->cItem;

    if (pdsa->cItem + 1 > pdsa->cItemAlloc) {
        void FAR* aItemNew = _LocalReAlloc(pdsa->fAllocated ? pdsa->aItem : NULL,
                (pdsa->cItemAlloc + pdsa->cItemGrow) * pdsa->cbItem);
        if (!aItemNew)
            return -1;

        if (!pdsa->fAllocated) {
            // when we go from static->dynamic, we need to copy
            pdsa->fAllocated = TRUE;
            hmemcpy(aItemNew, pdsa->aItem, pdsa->cItem * pdsa->cbItem);
        }

        pdsa->aItem = aItemNew;
        pdsa->cItemAlloc += pdsa->cItemGrow;
    }

    if (index < pdsa->cItem) {
        hmemcpy(SDSA_PITEM(pdsa, index + 1), SDSA_PITEM(pdsa, index),
            (pdsa->cItem - index) * pdsa->cbItem);
    }
    pdsa->cItem++;
    hmemcpy(SDSA_PITEM(pdsa, index), pitem, pdsa->cbItem);

    return index;
}


BOOL WINAPI FDSA_DeleteItem(PFDSA pdsa, int index)
{
    ASSERT(pdsa);

    if (index < 0 || index >= pdsa->cItem)
    {
        TraceMsg(TF_ERROR, "FDSA: DeleteItem: Invalid index: %d", index);
        DABreak();
        return FALSE;
    }

    if (index < pdsa->cItem - 1)
    {
        hmemcpy(SDSA_PITEM(pdsa, index), SDSA_PITEM(pdsa, index + 1),
            (pdsa->cItem - (index + 1)) * pdsa->cbItem);
    }
    pdsa->cItem--;

#if 0
    // BUGBUG: this doesn't work yet for FDsA
    if (pdsa->cItemAlloc - pdsa->cItem > pdsa->cItemGrow)
    {
        void FAR* aItemNew = ReAlloc(pdsa->aItem,
                (pdsa->cItemAlloc - pdsa->cItemGrow) * pdsa->cbItem);

        ASSERT(aItemNew);
        pdsa->aItem = aItemNew;
        pdsa->cItemAlloc -= pdsa->cItemGrow;
    }
#endif
    return TRUE;
}

