//
// isfmenu.cpp
//
// callback for chevron drop-down menu for isfbands
//

#include "priv.h"
#include "sccls.h"
#include "isfmenu.h"
#include "isfband.h"
#include "util.h"

// *** IUnknown methods ***
STDMETHODIMP CISFMenuCallback::QueryInterface (REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = 
    {
        QITABENT(CISFMenuCallback, IShellMenuCallback),
        QITABENT(CISFMenuCallback, IObjectWithSite),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CISFMenuCallback::AddRef ()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CISFMenuCallback::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if( _cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

BOOL CISFMenuCallback::_IsVisible(LPITEMIDLIST pidl)
{
    if (_poct) {
        VARIANTARG v;

        v.vt = VT_INT_PTR;
        v.byref = pidl;

        HRESULT hr = _poct->Exec(&CGID_ISFBand, ISFBID_ISITEMVISIBLE, 0, &v, NULL);
        return (hr == S_OK);
    }

    return FALSE;
}


HRESULT IUnknown_SeekToZero(IUnknown* punk)
{
    HRESULT hres = E_FAIL;
    IStream* pstm;
    if (punk && SUCCEEDED(punk->QueryInterface(IID_IStream, (void**)&pstm)))
    {
        // We need to seek to the beginning of the stream here. We don't do this in
        // the menubands because it's rude: They should not seek to the beginning
        // because there may be information that needs to be saved after them.
        //Set the seek pointer at the beginning.
        const LARGE_INTEGER li0 = {0};
        hres = pstm->Seek(li0, STREAM_SEEK_SET, NULL);
        pstm->Release();
    }

    return hres;
}

HRESULT CISFMenuCallback::_GetObject(LPSMDATA psmd, REFIID riid, void** ppvObj)
{
    HRESULT hres = S_FALSE;
    *ppvObj = NULL;
    if (IsEqualIID(riid, IID_IStream))
    {
        if (_pidl && psmd->pidlFolder && psmd->pidlItem)
        {
            // Verify that the Cascading menuband is ONLY asking for this folder.
            // because if there is a sub menu, It's going to ask again with the
            // pidl of that folder, which we don't have the Stream for, and we
            // can hose things pretty good if we indescriminatly hand out order streams

            LPITEMIDLIST pidlFull = ILCombine(psmd->pidlFolder, psmd->pidlItem);
            if (pidlFull)
            {
                if (_poct && ILIsEqual(pidlFull, _pidl)) 
                {
                    VARIANTARG v;

                    v.vt = VT_UNKNOWN;
                    hres = _poct->Exec(&CGID_ISFBand, ISFBID_GETORDERSTREAM, 0, NULL, &v);

                    IUnknown_SeekToZero(v.punkVal);

                    if (SUCCEEDED(hres))
                    {
                        hres = v.punkVal->QueryInterface(riid, ppvObj);
                        v.punkVal->Release();
                    }
                }
                ILFree(pidlFull);
            }
        }
    }
    return hres;
}

HRESULT CISFMenuCallback::_SetObject(LPSMDATA psmd, REFIID riid, void** ppvObj)
{
    HRESULT hres = E_FAIL;

    if (IsEqualIID(riid, IID_IStream))
    {
        if (_pidl && psmd->pidlFolder && psmd->pidlItem)
        {
            // Verify that the Cascading menuband is ONLY asking for this folder.
            // because if there is a sub menu, It's going to ask again with the
            // pidl of that folder, which we don't have the Stream for, and we
            // can hose things pretty good if we indescriminatly hand out order streams

            LPITEMIDLIST pidlFull = ILCombine(psmd->pidlFolder, psmd->pidlItem);
            if (pidlFull)
            {
                if (_poct && ILIsEqual(pidlFull, _pidl)) 
                {
                    ASSERT(ppvObj);

                    VARIANTARG v;

                    v.vt = VT_UNKNOWN;
                    v.punkVal = *(IUnknown**)ppvObj;

                    IUnknown_SeekToZero(*(IUnknown**)ppvObj);

                    hres = _poct->Exec(&CGID_ISFBand, ISFBID_SETORDERSTREAM, 0, &v, NULL);
                }
                ILFree(pidlFull);
            }
        }
    }

    return hres;
}


HRESULT CISFMenuCallback::_GetSFInfo(LPSMDATA psmd, PSMINFO psminfo)
{
    // We only want to filter pidls if:
    //  1) It's at the root of the links chevron menu
    //  2) It's _IS_ visible in the links bar. We don't want to show links
    //     in this menu that are visible.
    if (psmd->uIdAncestor == ANCESTORDEFAULT &&
        (psminfo->dwMask & SMIM_FLAGS)       && 
        _IsVisible(psmd->pidlItem))
    {
        // not obscured on the subject isfband; exclude from menu
        psminfo->dwFlags |= SMIF_HIDDEN;
    }

    return S_OK;
}

// *** IShellMenuCallback methods ***
STDMETHODIMP CISFMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_FALSE;

    switch (uMsg) 
    {
    case SMC_SFEXEC:
        hr = SHNavigateToFavorite(psmd->psf, psmd->pidlItem, _punkSite, SBSP_DEFBROWSER | SBSP_DEFMODE);
        break;

    case SMC_GETSFINFO:
        hr = _GetSFInfo(psmd, (PSMINFO)lParam);
        break;

    case SMC_GETSFOBJECT:
        hr = _GetObject(psmd, *((GUID*)wParam), (void**)lParam);
        break;

    case SMC_SETSFOBJECT:
        hr = _SetObject(psmd, *((GUID*)wParam), (void**)lParam);
        break;

    }

    return hr;
}

// *** IObjectWithSite methods ***
STDMETHODIMP CISFMenuCallback::SetSite(IUnknown* punkSite)
{
    if (punkSite != _punkSite)
        IUnknown_Set(&_punkSite, punkSite);

    return S_OK;
}


HRESULT CISFMenuCallback::Initialize(IUnknown* punk)
{
    HRESULT hr = E_FAIL;

    if (punk)
        hr = punk->QueryInterface(IID_IOleCommandTarget, (PVOID*)&_poct);

    IShellFolderBand* psfb;
    hr = punk->QueryInterface(IID_IShellFolderBand, (PVOID*)&psfb);

    if (SUCCEEDED(hr)) 
    {
        BANDINFOSFB bi;
        bi.dwMask = ISFB_MASK_IDLIST | ISFB_MASK_SHELLFOLDER;

        hr = psfb->GetBandInfoSFB(&bi);
        _pidl = bi.pidl;
        if (bi.psf)
            bi.psf->Release();
        psfb->Release();
    }

    return hr;
}

CISFMenuCallback::CISFMenuCallback() : _cRef(1)
{
}

CISFMenuCallback::~CISFMenuCallback()
{
    ASSERT(_cRef == 0);

    ILFree(_pidl);

    ATOMICRELEASE(_poct);
}
