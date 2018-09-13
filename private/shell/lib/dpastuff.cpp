#include "priv.h"
#include "dpastuff.h"

//
//  The ORDERITEM structure is exposed via the IOrderList interface.
//  ORDERITEM2 contains our private hidden fields.
//
//  The extra fields contain information about the cached icon location.
//
//  ftModified is the modify-time on the pidl, which is used to detect
//  whether the cache needs to be refreshed.
//
//  If ftModified is nonzero, then { pwszIcon, iIconIndex, pidlTarget }
//  describe the icon that should be displayed for the item.
//
//  If pwszIcon is nonzero, then the item is a shortcut with a custom
//  icon.  pwszIcon points to the file name for the icon, iIconIndex
//  is the icon index within the pwszIcon file.
//
//  If pidlTarget is nonzero, then the item is a shortcut with a default
//  icon.  pidlTarget is the target pidl, whose icon we should use.
//

typedef struct ORDERITEM2 {
    ORDERITEM oi;               // part that clients see - must come first
    DWORD  dwFlags;             // User defined flags.
    LPWSTR pwszIcon;            // for cacheing the icon location
    int iIconIndex;             // for cacheing the icon location
    LPITEMIDLIST pidlTarget;    // use the icon for this pidl
} ORDERITEM2, *PORDERITEM2;

int CALLBACK OrderItem_Compare(LPVOID pv1, LPVOID pv2, LPARAM lParam)
{
    PORDERITEM  poi1 = (PORDERITEM)pv1;
    PORDERITEM  poi2 = (PORDERITEM)pv2;
    PORDERINFO  poinfo = (PORDERINFO)lParam;
    int nRet;

    if (!poinfo)
    {   
        ASSERT(FALSE);
        return 0;
    }
    switch (poinfo->dwSortBy)
    {
    case OI_SORTBYNAME:
    {
        // Make sure they're both non-null
        //
        if ( poi1->pidl && poi2->pidl )
        {
            HRESULT hres = poinfo->psf->CompareIDs(0, poi1->pidl, poi2->pidl);
            nRet = (short)HRESULT_CODE(hres);
        }
        else
        {
            if ( poi1->pidl == poi2->pidl )
                nRet = 0;
            else
                nRet = ((UINT_PTR)poi1->pidl < (UINT_PTR)poi2->pidl ? -1 : 1);
        }

        break;
    }

    case OI_SORTBYORDINAL:
        if (poi1->nOrder == poi2->nOrder)
            nRet = 0;
        else
            // do unsigned compare so -1 goes to end of list
            nRet = ((UINT)poi1->nOrder < (UINT)poi2->nOrder ? -1 : 1);
        break;

    default:
        ASSERT_MSG(0, "Bad dwSortBy passed to OrderItem_Compare");
        nRet = 0;
        break;
    }

    return nRet;
}

void OrderItem_FreeIconInfo(PORDERITEM poi)
{
    PORDERITEM2 poi2 = CONTAINING_RECORD(poi, ORDERITEM2, oi);
    if (poi2->pwszIcon)
    {
        LPWSTR pwszIcon = poi2->pwszIcon;
        poi2->pwszIcon = NULL;
        LocalFree(pwszIcon);
    }

    if (poi2->pidlTarget)
    {
        LPITEMIDLIST pidl = poi2->pidlTarget;
        poi2->pidlTarget = NULL;
        ILFree(pidl);
    }
}


LPVOID CALLBACK OrderItem_Merge(UINT uMsg, LPVOID pvDst, LPVOID pvSrc, LPARAM lParam)
{
    PORDERITEM2 poi2Dst = CONTAINING_RECORD(pvDst, ORDERITEM2, oi);
    PORDERITEM2 poi2Src = CONTAINING_RECORD(pvSrc, ORDERITEM2, oi);
    PORDERINFO  poinfo = (PORDERINFO)lParam;
    LPVOID pvRet = pvDst;

    switch (uMsg)
    {
    case DPAMM_MERGE:
        // Transfer the order field
        poi2Dst->oi.nOrder = poi2Src->oi.nOrder;

        // Propagate any cached icon information too...
        if (poi2Src->pwszIcon || poi2Src->pidlTarget)
        {
            // To avoid useless allocation, we transfer the cache across
            // instead of copying it.
            if (poinfo->psf2 &&
                poinfo->psf2->CompareIDs(SHCIDS_ALLFIELDS, poi2Dst->oi.pidl, poi2Src->oi.pidl) == S_OK)
            {
                OrderItem_FreeIconInfo(&poi2Dst->oi);
                CopyMemory((LPBYTE)poi2Dst + sizeof(ORDERITEM),
                           (LPBYTE)poi2Src  + sizeof(ORDERITEM),
                           sizeof(ORDERITEM2) - sizeof(ORDERITEM));
                ZeroMemory((LPBYTE)poi2Src  + sizeof(ORDERITEM),
                           sizeof(ORDERITEM2) - sizeof(ORDERITEM));
            }
        }
        break;

    case DPAMM_DELETE:
    case DPAMM_INSERT:
        // Don't need to implement this
        ASSERT(0);
        pvRet = NULL;
        break;
    }
    
    return pvRet;
}

int OrderItem_UpdatePos(LPVOID p, LPVOID pData)
{
    PORDERITEM poi = (PORDERITEM)p;

    if (-1 == poi->nOrder)
    {
        poi->nOrder = (int)(INT_PTR)pData;
    }
    else if ((int)(INT_PTR)pData >= poi->nOrder)
    {
        poi->nOrder++;
    }

    return 1;
}

// OrderList_Merge sorts hdpaNew to match hdpaOld order,
// putting any items in hdpaNew that were not in hdpaOld
// at position iInsertPos (-1 means end of list).
//
// Assumes hdpaOld is already sorted by sort order in lParam (OI_SORTBYNAME by default)
// (if hdpaOld is specified)
//
void OrderList_Merge(HDPA hdpaNew, HDPA hdpaOld, int iInsertPos, LPARAM lParam,
                     LPFNORDERMERGENOMATCH pfn, LPVOID pvParam)
{
    PORDERINFO poinfo = (PORDERINFO)lParam;

    BOOL fMergeOnly = FALSE;
    if (poinfo->dwSortBy == OI_MERGEBYNAME)
    {
        poinfo->dwSortBy = OI_SORTBYNAME;
        fMergeOnly = TRUE;
    }

    // hdpaNew has not been sorted, sort by name
    DPA_Sort(hdpaNew, OrderItem_Compare, lParam);
    BOOL fForceNoMatch = FALSE;

    if (FAILED(poinfo->psf->QueryInterface(IID_IShellFolder2, (LPVOID *)&poinfo->psf2))) {
        // 239390: Network Connections folder doesn't implement QI correctly.  Its psf 
        // fails QI for IID_IShellFolder2, but doesn't null out ppvObj.  So do it for them.
        poinfo->psf2 = NULL;
    }

    // Copy order preferences over from old list to new list
    if (hdpaOld)
    {
        DPA_Merge(hdpaNew, hdpaOld, DPAM_SORTED | DPAM_NORMAL, OrderItem_Compare, OrderItem_Merge, lParam);

        // If we're waiting for the notify from a drag&drop operation,
        // update the new items (they will have a -1) to the insert position.
        if (-1 != iInsertPos)
        {
            DPA_EnumCallback(hdpaNew, OrderItem_UpdatePos, (LPVOID)(INT_PTR)iInsertPos);
        }

        if (poinfo->dwSortBy != OI_SORTBYORDINAL && !fMergeOnly)
        {
            poinfo->dwSortBy = OI_SORTBYORDINAL;
            DPA_Sort(hdpaNew, OrderItem_Compare, lParam);
        }
    }
    else
        fForceNoMatch = TRUE;

    // If the caller passed a NoMatch callback, then call it with
    // each item that is not matched.
    if (pfn)
    {
        for (int i = DPA_GetPtrCount(hdpaNew)-1 ; i >= 0 ; i--)
        {
            PORDERITEM poi = (PORDERITEM)DPA_FastGetPtr(hdpaNew, i);

            // Does this item have order information?
            if (iInsertPos == poi->nOrder ||
                -1 == poi->nOrder ||
                fForceNoMatch)
            {
                // No; Then pass to the "No Match" callback
                pfn(pvParam, poi->pidl);
            }
        }
    }

    ATOMICRELEASE(poinfo->psf2);

    OrderList_Reorder(hdpaNew);
}

// OrderList_Reorder refreshes the order info
void OrderList_Reorder(HDPA hdpa)
{
    int i;

    for (i = DPA_GetPtrCount(hdpa)-1 ; i >= 0 ; i--)
    {
        PORDERITEM poi = (PORDERITEM)DPA_FastGetPtr(hdpa, i);

        poi->nOrder = i;
    }
}

BOOL OrderList_Append(HDPA hdpa, LPITEMIDLIST pidl, int nOrder)
{
    PORDERITEM poi = OrderItem_Create(pidl, nOrder);
    if (poi)
    {
        if (-1 != DPA_AppendPtr(hdpa, poi))
            return TRUE;

        OrderItem_Free(poi);
    }
    return FALSE;
}

// This differes from DPA_Clone in that it allocates new items!
HDPA OrderList_Clone(HDPA hdpa)
{
    HDPA hdpaNew = NULL;

    if (EVAL(hdpa))
    {
        hdpaNew = DPA_Create(DPA_GetPtrCount(hdpa));
    
        if (hdpaNew)
        {
            int i;
    
            for (i = 0 ; i < DPA_GetPtrCount(hdpa) ; i++)
            {
                PORDERITEM poi = (PORDERITEM)DPA_FastGetPtr(hdpa, i);
                LPITEMIDLIST pidl = ILClone(poi->pidl);
                if (pidl)
                {
                    if (!OrderList_Append(hdpaNew, pidl, poi->nOrder))
                    {
                        ILFree(pidl);
                    }
                }
            }
        }
    }

    return hdpaNew;
}

// Does not clone the pidl but will free it.
// Does not addref the psf nor release it.
PORDERITEM OrderItem_Create(LPITEMIDLIST pidl, int nOrder)
{
    PORDERITEM2 poi = (PORDERITEM2)LocalAlloc(LPTR, SIZEOF(ORDERITEM2));

    if (poi)
    {
        poi->oi.pidl = pidl;
        poi->oi.nOrder = nOrder;
        return &poi->oi;
    }

    return NULL;
}

void OrderItem_Free(PORDERITEM poi, BOOL fKillPidls /* = TRUE */)
{
    if (fKillPidls)
        ILFree(poi->pidl);
    OrderItem_FreeIconInfo(poi);
    LocalFree(poi);
}

int OrderItem_FreeItem(LPVOID p, LPVOID pData)
{
    PORDERITEM poi = (PORDERITEM)p;

    OrderItem_Free(poi, (BOOL)(INT_PTR)pData);

    return 1;
}

void OrderList_Destroy(HDPA* phdpa, BOOL fKillPidls /* = fTrue */)
{
    if (*phdpa) {
        DPA_DestroyCallback(*phdpa, OrderItem_FreeItem, (LPVOID) (INT_PTR)fKillPidls);
        *phdpa = NULL;
    }
}

//
//  Return values:
//
//  S_OK    - icon obtained successfully
//  S_FALSE - icon not obtained, don't waste time trying
//  E_FAIL  - no cached icon, need to do more work
//
HRESULT OrderItem_GetSystemImageListIndexFromCache(PORDERITEM poi,
                                        IShellFolder *psf, int *piOut)
{
    PORDERITEM2 poi2 = CONTAINING_RECORD(poi, ORDERITEM2, oi);
    IShellFolder *psfT;
    LPCITEMIDLIST pidlItem;
    HRESULT hr;

    // Do we have a cached icon location?
    if (poi2->pwszIcon)
    {
        *piOut = 0;
        // Validate Path existance.
        if (PathFileExistsW(poi2->pwszIcon))
        {
            *piOut = _WorA_Shell_GetCachedImageIndex(poi2->pwszIcon, poi2->iIconIndex, GIL_PERINSTANCE);
        }

        return (*piOut > 0)? S_OK : E_FAIL;
    }

    // Do we have a cached pidlTarget?
    if (poi2->pidlTarget)
    {
        hr = SHBindToIDListParent(poi2->pidlTarget, IID_IShellFolder, (void**)&psfT, &pidlItem);
        if (SUCCEEDED(hr))
        {
            // Make sure the pidl exsists before binding. because the bind does succeed if it does not exist.
            DWORD dwAttrib = SFGAO_VALIDATE;
            hr = psfT->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlItem, &dwAttrib);
            if (SUCCEEDED(hr))
            {
                *piOut = SHMapPIDLToSystemImageListIndex(psfT, pidlItem, NULL);
            }
            psfT->Release();
            return hr;
        }

        // Bind failed - shortcut target was deleted
        // Keep the cache valid because we don't want to whack the disk
        // all the time only to discover it's busted.
        return E_FAIL;
    }

    return E_FAIL;
}

DWORD OrderItem_GetFlags(PORDERITEM poi)
{
    PORDERITEM2 poi2 = CONTAINING_RECORD(poi, ORDERITEM2, oi);
    return poi2->dwFlags;
}

void OrderItem_SetFlags(PORDERITEM poi, DWORD dwFlags)
{
    PORDERITEM2 poi2 = CONTAINING_RECORD(poi, ORDERITEM2, oi);
    poi2->dwFlags = dwFlags;
}


int OrderItem_GetSystemImageListIndex(PORDERITEM poi, IShellFolder *psf, BOOL fUseCache)
{
    PORDERITEM2 poi2 = CONTAINING_RECORD(poi, ORDERITEM2, oi);
    HRESULT hr;
    int iBitmap;
    DWORD dwAttr;

    if (fUseCache)
    {
        hr = OrderItem_GetSystemImageListIndexFromCache(poi, psf, &iBitmap);
        if (SUCCEEDED(hr))
        {
            return iBitmap;
        }
        else
        {
            goto Fallback;
        }
    }
    else
    {
        //
        // Free any pointers we cached previously
        //
        if (poi2->pidlTarget)
        {
            ILFree(poi2->pidlTarget);
            poi2->pidlTarget = NULL;
        }

        Str_SetPtr(&poi2->pwszIcon, NULL);
    }

    //
    //  Go find the icon.
    //
    ASSERT(poi2->pidlTarget == NULL);
    ASSERT(poi2->pwszIcon == NULL);

    //
    //  Is this item shortcutlike at all?
    //
    dwAttr = SFGAO_LINK;
    hr = psf->GetAttributesOf(1, (LPCITEMIDLIST*)&poi->pidl, &dwAttr);
    if (FAILED(hr) || !(dwAttr & SFGAO_LINK))
        goto Fallback;                  // not a shortcut; use the fallback

    //
    // Must go for ANSI version first because client might not support
    // UNICODE.
    //
    // BUGBUG - should QI for IExtractIcon to see if we get GIL_DONTCACHE
    // back.

    IShellLinkA *pslA;
    hr = psf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*)&poi->pidl,
                            IID_IShellLinkA, 0, (LPVOID *)&pslA);

    if (FAILED(hr))
        goto Fallback;

    //
    //  If there's a UNICODE version, that's even better.
    //
    IShellLinkW *pslW;
    WCHAR wszIconPath[MAX_PATH];

    hr = pslA->QueryInterface(IID_IShellLinkW, (LPVOID *)&pslW);
    if (SUCCEEDED(hr))
    {
        hr = pslW->GetIconLocation(wszIconPath, ARRAYSIZE(wszIconPath), &poi2->iIconIndex);
        pslW->Release();
    }
    else
    {
        // Only IShellLinkA supported.  Thunk to UNICODE manually.
        CHAR szIconPath[ARRAYSIZE(wszIconPath)];
        hr = pslA->GetIconLocation(szIconPath, ARRAYSIZE(szIconPath), &poi2->iIconIndex);
        if (SUCCEEDED(hr))
            SHAnsiToUnicode(szIconPath, wszIconPath, ARRAYSIZE(wszIconPath));
    }

    // If we have a custom icon path, then save that
    if (SUCCEEDED(hr) && wszIconPath[0])
    {
        Str_SetPtr(&poi2->pwszIcon, wszIconPath);
    }
    else
    {
        // No icon path, get the target instead
        pslA->GetIDList(&poi2->pidlTarget);

        if (IsURLChild(poi2->pidlTarget, TRUE))
        {
            // If this is a url, we want to go to the "Fallback" case. The reason for this
            // is that the fallback case will go through 
            // where we will end up with the generic icon for .url files
            ILFree(poi2->pidlTarget);
            poi2->pidlTarget = NULL;

            pslA->Release();
            goto Fallback;
        }
    }

    pslA->Release();

    //
    //  Aw-right, the cache is all loaded up.  Let's try that again.
    //
    hr = OrderItem_GetSystemImageListIndexFromCache(poi, psf, &iBitmap);
    if (hr == S_OK)
    {
        return iBitmap;
    }

Fallback:
    return SHMapPIDLToSystemImageListIndex(psf, poi->pidl, NULL);
}


// Header for file menu streams
//
// The file menu stream consists of an IOSTREAMHEADER followed by
// a DPA_SaveStream of the order DPA.  Each item in the DPA consists
// of an OISTREAMITEM.
//
// To keep roaming profiles working between NT4 (IE4) and NT5 (IE5),
// the dwVersion used by NT5 must be the same as that used by NT4.
// I.e., it must be 2.

typedef struct tagOISTREAMHEADER
{
    DWORD cbSize;           // Size of header
    DWORD dwVersion;        // Version of header
} OISTREAMHEADER;

#define OISTREAMHEADER_VERSION  2

//
//  Each item in a persisted order DPA consists of an OISTREAMITEM
//  followed by additional goo.  All pidls stored include the
//  terminating (USHORT)0.
//
//  IE4:
//      OISTREAMITEM
//      pidl                    - the item itself
//
//  IE5 - shortcut has custom icon
//      OISTREAMITEM
//      pidl                    - the item itself (last-modify time implied)
//      <optional padding>      - for WCHAR alignment
//      dwFlags                 - User defined Flags
//      dwStringLen             - Length of the icon path 
//      UNICODEZ iconpath       - icon path
//      iIconIndex              - icon index
//
//  IE5 - shortcut takes its icon from another pidl
//      OISTREAMITEM
//      pidl                    - the item itself (last-modify time implied)
//      <optional padding>      - for WCHAR alignment
//      dwFlags                 - User defined Flags
//      (DWORD)0                - null string indicates "no custom icon"
//      pidlTarget              - use the icon for this pidl
//

typedef struct tagOISTREAMITEM
{
    DWORD cbSize;           // Size including trailing goo
    int   nOrder;           // User-specified order

    // variable-sized trailing goo comes here.
    //
    // See above for description of trailing goo.

} OISTREAMITEM;

#define CB_OISTREAMITEM     (sizeof(OISTREAMITEM))

//
//  Save a component of the orderitem to the stream.  If an error has
//  already occurred on the stream, *phrRc contains the old error code,
//  and we write nothing.
//
//  If pstm == NULL, then we are not actually writing anything.  We are
//  merely doing a dry run.
//
//  Otherwise, *phrRc accumulates the number of bytes actually written,
//  or receives an error code on failure.
//

void
OrderItem_SaveSubitemToStream(IStream *pstm, LPCVOID pvData, ULONG cb, HRESULT* phrRc)
{
    HRESULT hres;

    if (SUCCEEDED(*phrRc))
    {
        if (pstm)
        {
            hres = IStream_Write(pstm, (LPVOID)pvData, cb);
            if (SUCCEEDED(hres))
            {
                *phrRc += cb;           // successful write - accumulate
            }
            else
            {
                *phrRc = hres;          // error - return error code
            }
        }
        else
        {
            *phrRc += cb;               // no output stream - accumulate
        }
    }
}

//
//  This worker function (1) computes the numer of bytes we will actually
//  write out, and (2) actually writes it if pstm != NULL.
//
//  Return value is the number of bytes written (or would have been
//  written), or a COM error code on failure.
//

const BYTE c_Zeros[2] = { 0 };    // a bunch of zeros

HRESULT
OrderItem_SaveToStreamWorker(PORDERITEM2 poi2, OISTREAMITEM *posi,
                             IStream *pstm, IShellFolder2 *psf2)
{
    HRESULT hrRc = 0;           // no bytes, no error

    ASSERT(poi2->oi.pidl);

    //
    //  First comes the header.
    //
    OrderItem_SaveSubitemToStream(pstm, posi, CB_OISTREAMITEM, &hrRc);

    //
    //  Then the pidl.
    //

    // We're assuming this is an immediate child pidl.  If it's not,
    // the pidl is being truncated!
    ASSERT(0 == _ILNext(poi2->oi.pidl)->mkid.cb);

    OrderItem_SaveSubitemToStream(pstm, poi2->oi.pidl,
                                  poi2->oi.pidl->mkid.cb + sizeof(USHORT),
                                  &hrRc);
    // Insert padding to get back to WCHAR alignment.
    if (hrRc % sizeof(WCHAR)) 
    {
        OrderItem_SaveSubitemToStream(pstm, &c_Zeros, 1, &hrRc);
    }

    OrderItem_SaveSubitemToStream(pstm, &poi2->dwFlags, sizeof(DWORD), &hrRc);

    //
    //  If we haven't barfed yet and the IShellFolder supports identity
    //  and there is icon information, then save it.
    //
    if (SUCCEEDED(hrRc) && psf2 && (poi2->pwszIcon || poi2->pidlTarget))
    {
        // Optional icon is present. 

        if (poi2->pwszIcon)
        {
            // UNICODEZ path
            DWORD cbString = (lstrlenW(poi2->pwszIcon) + 1) * sizeof(WCHAR);

            // Save the String len
            OrderItem_SaveSubitemToStream(pstm, &cbString,
                      sizeof(DWORD) , &hrRc);

            OrderItem_SaveSubitemToStream(pstm, poi2->pwszIcon,
                      (lstrlenW(poi2->pwszIcon) + 1) * sizeof(WCHAR), &hrRc);

            // icon index
            OrderItem_SaveSubitemToStream(pstm, &poi2->iIconIndex,
                      sizeof(poi2->iIconIndex), &hrRc);
        }
        else
        {
            DWORD cbString = 0;
            OrderItem_SaveSubitemToStream(pstm, &cbString, sizeof(DWORD), &hrRc);

            // pidlTarget
            OrderItem_SaveSubitemToStream(pstm, poi2->pidlTarget,
                      ILGetSize(poi2->pidlTarget), &hrRc);
        }
    }
    return hrRc;
}

HRESULT 
CALLBACK 
OrderItem_SaveToStream(DPASTREAMINFO * pinfo, IStream * pstm, LPVOID pvData)
{
    PORDERITEM2 poi2 = (PORDERITEM2)pinfo->pvItem;
    HRESULT hres = S_FALSE;
    IShellFolder2 *psf2 = (IShellFolder2 *)pvData;

    if (poi2->oi.pidl)
    {
        OISTREAMITEM osi;

        // First a dry run to compute the size of this item.
        hres = OrderItem_SaveToStreamWorker(poi2, NULL, NULL, psf2);

        // Nothing actually got written, so this should always succeed.
        ASSERT(SUCCEEDED(hres));

        osi.cbSize = hres;
        osi.nOrder = poi2->oi.nOrder;

        // Now write it out for real
        hres = OrderItem_SaveToStreamWorker(poi2, &osi, pstm, psf2);

        // On success, we must return exactly S_OK or DPA will blow us off
        if (SUCCEEDED(hres))
            hres = S_OK;
    }

    return hres;
}   

//
//  Check if a pidl we read out of a stream is a simple child pidl.
//  The pidl must be exactly cb bytes in length.
//  The pointer is known to be valid;
//  we just want to check that the contents are good, too.
//
BOOL
IsValidPersistedChildPidl(LPCITEMIDLIST pidl, UINT cb)
{
    // Must have at least room for one byte of pidl plus the terminating
    // zero.
    if (cb < 1 + sizeof(USHORT))
        return FALSE;

    // Make sure size is at least what it's supposed to be.
    if (pidl->mkid.cb + sizeof(USHORT) > cb)
        return FALSE;

    // Make sure there's a zero right after it.
    pidl = _ILNext(pidl);
    return pidl->mkid.cb == 0;
}

//
//  Just like ILGetSize, but returns (UINT)-1 if the pidl is corrupt.
//  We use (UINT)-1 as the return value because it will be bigger than
//  the buffer size we eventually compare it against.
UINT
SafeILGetSize(LPCITEMIDLIST pidl)
{
    _try {
        return ILGetSize(pidl);
    } _except (EXCEPTION_EXECUTE_HANDLER) {
    }
    __endexcept
    return (UINT)-1;
}

HRESULT
CALLBACK 
OrderItem_LoadFromStream(DPASTREAMINFO * pinfo, IStream * pstm, LPVOID /*pvData*/)
{
    HRESULT hres;
    OISTREAMITEM osi;

    hres = IStream_Read(pstm, &osi, CB_OISTREAMITEM);
    if (SUCCEEDED(hres))
    {
        ASSERT(CB_OISTREAMITEM < osi.cbSize);
        if (CB_OISTREAMITEM < osi.cbSize)
        {
            UINT cb = osi.cbSize - CB_OISTREAMITEM;
            LPITEMIDLIST pidl = IEILCreate(cb);
            if ( !pidl )
                hres = E_OUTOFMEMORY;
            else
            {
                hres = IStream_Read(pstm, pidl, cb);
                if (SUCCEEDED(hres) && IsValidPersistedChildPidl(pidl, cb))
                {
                    PORDERITEM poi = OrderItem_Create(pidl, osi.nOrder);

                    if (poi)
                    {
                        PORDERITEM2 poi2 = CONTAINING_RECORD(poi, ORDERITEM2, oi);
                        pinfo->pvItem = poi;
                        // cbPos = offset to trailing gunk after pidl
                        UINT cbPos = pidl->mkid.cb + sizeof(USHORT);
                        cbPos = ROUNDUP(cbPos, sizeof(WCHAR));

                        // Do we have a DWORD hanging off the end of the pidl? This should be the flags.
                        if (cb >= cbPos + sizeof(DWORD))
                        {
                            poi2->dwFlags = *(UNALIGNED DWORD*)((LPBYTE)pidl + cbPos);
                        }

                        // Make sure there's at least a WCHAR to test against.
                        if (cb >= cbPos + sizeof(WCHAR) + 2 * sizeof(DWORD))
                        {
                            DWORD cbString = *(UNALIGNED DWORD*)((LPBYTE)pidl + cbPos + sizeof(DWORD));
                            LPWSTR pwszIcon = (LPWSTR)((LPBYTE)pidl + cbPos + 2 * sizeof(DWORD));

                            // Do we have a string lenght?
                            if (pwszIcon && cbString != 0)
                            {
                                // Yes, then this is a string not a pidl. We want to make sure this is a
                                // fully qualified path.
                                if (IS_VALID_STRING_PTRW(pwszIcon, cbString) &&
                                    !PathIsRelative(pwszIcon))
                                {
                                    poi2->pwszIcon = StrDup(pwszIcon);
                                    pwszIcon += lstrlenW(pwszIcon) + 1;
                                    poi2->iIconIndex = *(UNALIGNED int *)pwszIcon;
                                }
                            }
                            else
                            {
                                // A string length of zero is 
                                LPITEMIDLIST pidlTarget = (LPITEMIDLIST)(pwszIcon);
                                // We want to write
                                // cbPos + sizeof(WCHAR) + SafeILGetSize(pidlTarget) <= cb
                                // but SafeILGetSize returns (UINT)-1 on error, so we need
                                // to do some algebra to avoid overflows
                                if (SafeILGetSize(pidlTarget) <= cb - cbPos - 2 * sizeof(DWORD))
                                {
                                    poi2->pidlTarget = ILClone(pidlTarget);
                                }
                            }
                        }

                        hres = E_OUTOFMEMORY;

                        // pidl Contains extranious information. Take the hit of stripping it so that
                        // our working set doesn't bloat.
                        LPITEMIDLIST pidlNew = ILClone(poi2->oi.pidl);
                        if (pidlNew)
                        {
                            ILFree(poi2->oi.pidl);
                            poi2->oi.pidl = pidlNew;
                            hres = S_OK;
                        }
                    }
                    else
                        hres = E_OUTOFMEMORY;
                }
                else
                    hres = E_FAIL;

                // Cleanup
                if (FAILED(hres))
                    ILFree(pidl);
            }
        }
        else
            hres = E_FAIL;

    }

    ASSERT((S_OK == hres && pinfo->pvItem) || FAILED(hres));
    return hres;
}    

HRESULT OrderList_LoadFromStream(IStream* pstm, HDPA * phdpa, IShellFolder * psfParent)
{
    HDPA hdpa = NULL;
    OISTREAMHEADER oish;

    ASSERT(phdpa);
    ASSERT(pstm);

    // Read the header for more info
    if (SUCCEEDED(IStream_Read(pstm, &oish, sizeof(oish))) &&
        sizeof(oish) == oish.cbSize)
    {
        // Load the stream.  (Should be ordered by name.)
        DPA_LoadStream(&hdpa, OrderItem_LoadFromStream, pstm, psfParent);
        
        // if this is the wrong version, throw away the pidls.
        // we go through the load anyways to make suret he read pointer is set right
        if (OISTREAMHEADER_VERSION != oish.dwVersion)
            OrderList_Destroy(&hdpa, TRUE);
        
    }

    *phdpa = hdpa;

    return (NULL != hdpa) ? S_OK : E_FAIL;
}

HRESULT OrderList_SaveToStream(IStream* pstm, HDPA hdpaSave, IShellFolder *psf)
{
    HRESULT hres = E_OUTOFMEMORY;
    OISTREAMHEADER oish;
    HDPA hdpa;

    // Clone the array and sort by name for the purpose of persisting it
    hdpa = DPA_Clone(hdpaSave, NULL);
    if (hdpa)
    {
        ORDERINFO   oinfo = {0};
#ifdef DEBUG
        // use QI to help track down leaks
        if (psf)
            EVAL(SUCCEEDED(psf->QueryInterface(IID_IShellFolder, (LPVOID *)&oinfo.psf)));
#else
        oinfo.psf = psf;
        if (psf)
            oinfo.psf->AddRef();
#endif
        oinfo.dwSortBy = OI_SORTBYNAME;
        DPA_Sort(hdpa, OrderItem_Compare, (LPARAM)&oinfo);

        // Save the header
        oish.cbSize = sizeof(oish);
        oish.dwVersion = OISTREAMHEADER_VERSION;

        hres = IStream_Write(pstm, &oish, sizeof(oish));
        if (SUCCEEDED(hres))
        {
            if (psf)
                oinfo.psf->QueryInterface(IID_IShellFolder2, (LPVOID *)&oinfo.psf2);
            hres = DPA_SaveStream(hdpa, OrderItem_SaveToStream, pstm, oinfo.psf2);
            ATOMICRELEASE(oinfo.psf2);
        }
        ATOMICRELEASE(oinfo.psf);
        DPA_Destroy(hdpa);
    }

    return hres;
}    

/////////////
//
// COrderList impl for export to channel installer
//

class COrderList  : public IPersistFolder, 
                    public IOrderList2
{
public:
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IPersistFolder
    virtual STDMETHODIMP GetClassID(CLSID *pClassID);
    virtual STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IOrderList
    virtual STDMETHODIMP GetOrderList(HDPA * phdpa);
    virtual STDMETHODIMP SetOrderList(HDPA hdpa, IShellFolder *psf);
    virtual STDMETHODIMP FreeOrderList(HDPA hdpa);

    virtual STDMETHODIMP SortOrderList(HDPA hdpa, DWORD dw);

    virtual STDMETHODIMP AllocOrderItem(PORDERITEM * ppoi, LPCITEMIDLIST pidl);
    virtual STDMETHODIMP FreeOrderItem(PORDERITEM poi);

    // IOrderList 2
    virtual STDMETHODIMP LoadFromStream(IStream* pstm, HDPA* hdpa, IShellFolder* psf);
    virtual STDMETHODIMP SaveToStream(IStream* pstm, HDPA hdpa);

protected:
    COrderList(IUnknown* punkOuter, LPCOBJECTINFO poi);
    friend IUnknown * COrderList_Create();

    COrderList();
    ~COrderList();

    int _cRef;
    IShellFolder *_psf;
    LPITEMIDLIST  _pidl;
    LPITEMIDLIST  _pidlFavorites;
};

COrderList::COrderList()
{
    _cRef = 1;
    DllAddRef();
}

COrderList::~COrderList()
{
    ILFree(_pidl);
    ILFree(_pidlFavorites);
    ATOMICRELEASE(_psf);
    DllRelease();
}

IUnknown * COrderList_Create()
{
    COrderList * pcol = new COrderList;
    if (pcol)
    {
        return SAFECAST(pcol, IPersistFolder*);
    }
    return NULL;
}

STDAPI COrderList_CreateInstance(IUnknown * pUnkOuter, IUnknown ** punk, LPCOBJECTINFO poi)
{
    *punk = COrderList_Create();

    return *punk ? S_OK : E_OUTOFMEMORY;
}

ULONG COrderList::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG COrderList::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT COrderList::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(COrderList, IPersistFolder),
        QITABENT(COrderList, IOrderList),
        QITABENTMULTI(COrderList, IOrderList2, IOrderList),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

HRESULT COrderList::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_OrderListExport;

    return S_OK;
}


// This is the directory setup wants to re-order
HRESULT COrderList::Initialize(LPCITEMIDLIST pidl)
{
    if (!_pidlFavorites)
    {
        SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &_pidlFavorites);
        if (!_pidlFavorites)
            return E_OUTOFMEMORY;
    }

    if (!pidl || !ILIsParent(_pidlFavorites, pidl, FALSE))
        return E_INVALIDARG;

    // Initialize can be called multiple times
    ATOMICRELEASE(_psf);

    Pidl_Set(&_pidl, pidl);

    if (_pidl)
        IEBindToObject(_pidl, &_psf);

    if (!_psf)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT COrderList_GetOrderList(HDPA * phdpa, LPCITEMIDLIST pidl, IShellFolder * psf)
{
    IStream* pstm = OpenPidlOrderStream((LPCITEMIDLIST)CSIDL_FAVORITES, pidl, REG_SUBKEY_FAVORITESA, STGM_READ);
    if (pstm)
    {
        HRESULT hres = OrderList_LoadFromStream(pstm, phdpa, psf);
        pstm->Release();
        return hres;
    }
    *phdpa = NULL;
    return E_OUTOFMEMORY;
}

HRESULT COrderList::GetOrderList(HDPA * phdpa)
{
    HRESULT hres = E_FAIL;

    *phdpa = NULL;

    if (_psf)
        hres = COrderList_GetOrderList(phdpa, _pidl, _psf);

    return hres;
}

HRESULT COrderList_SetOrderList(HDPA hdpa, LPCITEMIDLIST pidl, IShellFolder *psf)
{
    IStream* pstm = OpenPidlOrderStream((LPCITEMIDLIST)CSIDL_FAVORITES, pidl, REG_SUBKEY_FAVORITESA, STGM_WRITE);
    if (EVAL(pstm))
    {
        HRESULT hres = OrderList_SaveToStream(pstm, hdpa, psf);
        pstm->Release();
        return hres;
    }
    return E_OUTOFMEMORY;
}

HRESULT COrderList::SetOrderList(HDPA hdpa, IShellFolder *psf)
{
    if (!_psf)
        return E_FAIL;

    return COrderList_SetOrderList(hdpa, _pidl, psf);
}

HRESULT COrderList::FreeOrderList(HDPA hdpa)
{
    OrderList_Destroy(&hdpa);
    return S_OK;
}

HRESULT COrderList::SortOrderList(HDPA hdpa, DWORD dw)
{
    if (OI_SORTBYNAME != dw && OI_SORTBYORDINAL != dw)
        return E_INVALIDARG;

    if (!_psf)
        return E_FAIL;

    ORDERINFO oinfo;
    oinfo.dwSortBy = dw;
    oinfo.psf = _psf;
#ifdef DEBUG
    oinfo.psf2 = (IShellFolder2 *)INVALID_HANDLE_VALUE; // force fault if someone uses it
#endif

    DPA_Sort(hdpa, OrderItem_Compare, (LPARAM)&oinfo);

    return S_OK;
}

HRESULT COrderList::AllocOrderItem(PORDERITEM * ppoi, LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlClone = ILClone(pidl);

    *ppoi = NULL;

    if (pidlClone)
    {
        *ppoi = OrderItem_Create(pidlClone, -1);
        if (*ppoi)
            return S_OK;

        ILFree(pidlClone);
    }

    return E_OUTOFMEMORY;
}



HRESULT COrderList::FreeOrderItem(PORDERITEM poi)
{
    OrderItem_Free(poi);

    return S_OK;
}

// IOrderList2::LoadFromStream
STDMETHODIMP COrderList::LoadFromStream(IStream* pstm, HDPA* phdpa, IShellFolder* psf)
{
    ASSERT(_psf == NULL);
    _psf = psf;
    if (_psf)
        _psf->AddRef();
    return OrderList_LoadFromStream(pstm, phdpa, _psf);
}

// IOrderList2::SaveToStream
STDMETHODIMP COrderList::SaveToStream(IStream* pstm, HDPA hdpa)
{
    return OrderList_SaveToStream(pstm, hdpa, _psf);
}
