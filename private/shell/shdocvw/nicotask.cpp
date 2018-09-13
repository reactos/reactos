#include "priv.h"
#include "nicotask.h"
#include "nsc.h"
/////////////////////////////////////////////////////////////////////////
// COPY AND PASTE ALERT

// this code is mostly copied and pasted from browseui/icotask.cpp
// see lamadio and/or davemi for why these aren't combined or shared
/////////////////////////////////////////////////////////////////////////

// {EB30900C-1AC4-11d2-8383-00C04FD918D0}
EXTERN_C static const GUID TASKID_IconExtraction = 
{ 0xeb30900c, 0x1ac4, 0x11d2, { 0x83, 0x83, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0xd0 } };

CNscIconTask::CNscIconTask(LPITEMIDLIST pidl, PFNNSCICONTASKBALLBACK pfn, LPVOID pvData, UINT_PTR uId, UINT uSynchId):
    _pidl(pidl), _pfn(pfn), _pvData(pvData), _uId(uId), _uSynchId(uSynchId), CRunnableTask(RTF_DEFAULT)
{ 
    
}

CNscIconTask::~CNscIconTask()
{
    if (_pidl)
        ILFree(_pidl);
}

// IRunnableTask methods (override)
STDMETHODIMP CNscIconTask::RunInitRT(void)
{
    int           iIcon = -1, iIconOpen = -1;
    IShellFolder* psf = NULL;
    LPCITEMIDLIST pidlItem;

    // We need to rebind because shell folders may not be thread safe.
    HRESULT hres = IEBindToParentFolder(_pidl, &psf, &pidlItem);

    if (SUCCEEDED(hres))
    {
#ifdef IE5_36825
        HRESULT hr = E_FAIL;
        IShellIcon *psi;
        if (SUCCEEDED(psf->QueryInterface(IID_IShellIcon, (void **)&psi)))
        {
            hr = psi->GetIconOf(pidlItem, 0, &iIcon); 
            if (hr == S_OK)
            {
                ULONG ulAttrs = SFGAO_FOLDER;
                psf->GetAttributesOf(1, &pidlItem, &ulAttrs);

                if ( !(ulAttrs & SFGAO_FOLDER) || FAILED(psi->GetIconOf(pidlItem, GIL_OPENICON, &iIconOpen)))
                    iIconOpen = iIcon;
            }
            psi->Release();
        }

        if (hr != S_OK)
        {
#endif
            // slow way...
            iIcon = IEMapPIDLToSystemImageListIndex(psf, pidlItem, &iIconOpen);
#ifdef IE5_36825
        }
#endif

        // BUGBUG/TODO: This is no good.  We are attempted to see if the content is offline.  That should
        // be done by using IQueryInfo::xxx().  This should go in the InternetShortcut object.
        // IShellFolder2::GetItemData or can also be used.
        //
        // See if it is a link. If it is not, then it can't be in the wininet cache and can't
        // be pinned (sticky cache entry) or greyed (unavailable when offline)
        DWORD dwFlags = 0;
        BOOL fAvailable;
        BOOL fSticky;
        
        // GetLinkInfo() will fail if the SFGAO_FOLDER or SFGAO_BROWSER bits aren't set.
        if (pidlItem && SUCCEEDED(hres) &&
            SUCCEEDED(GetLinkInfo(psf, pidlItem, &fAvailable, &fSticky)))
        {
            if (!fAvailable)
            {
                dwFlags |= NSCICON_GREYED;
            }

            if (fSticky)
            {
                dwFlags |= NSCICON_PINNED;
            }
        }
        else
        {
            //item is not a link
            dwFlags |= NSCICON_DONTREFETCH;
        }

        ATOMICRELEASE(psf);

        _pfn(_pvData, _uId, iIcon, iIconOpen, dwFlags, _uSynchId);
    }
    return S_OK;        // return S_OK even if we don't get an icon.
}


HRESULT AddNscIconTask(IShellTaskScheduler* pts, LPCITEMIDLIST pidl, 
                    PFNNSCICONTASKBALLBACK pfn, LPVOID pvData, UINT_PTR uId, UINT uSynchId)
{
    if (!pts)
        return E_INVALIDARG;

    CNscIconTask* pit = new CNscIconTask((LPITEMIDLIST)pidl, pfn, pvData, uId, uSynchId);

    // Don't ILFree(pidl) because CNscIconTask takes ownership.
    // BUGBUG (lamadio) Remove this from the memory list. Ask Saml how to do this
    // for the IMallocSpy stuff.

    HRESULT hres = E_OUTOFMEMORY;
    if (pit)
    {
        hres = pts->AddTask(SAFECAST(pit, IRunnableTask*), TASKID_IconExtraction, 
            ITSAT_DEFAULT_LPARAM, ITSAT_DEFAULT_PRIORITY);

#ifdef DEBUG
        if(SUCCEEDED(hres))
        {
            remove_from_memlist((LPVOID)pit);
            remove_from_memlist((LPVOID)pidl);
        }
#endif // DEBUG
        pit->Release();
    }

    return hres;
}
