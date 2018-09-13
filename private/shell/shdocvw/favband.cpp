//
// favband.cpp
//
// favorites band implementation
//

#include "priv.h"
#include "sccls.h"
#include "nscband.h"
#include "resource.h"
#include "favorite.h"
#include "uemapp.h"   // KMTF: Included for instrumentation

#include <mluisupp.h>

#define SUPERCLASS CNSCBand

#define TF_FAVBAND  0

class CFavBand : public CNSCBand
{
public:

    // *** IOleCommandTarget methods ***
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // *** IDockingWindow methods ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);


protected:
    virtual void _AddButtons(BOOL fAdd);
    HRESULT _BrowserExec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
    void _OrganizeFavorites();
    friend HRESULT CFavBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

    BOOL       _fStrsAdded;
    LONG_PTR       _lStrOffset;
};

HRESULT CFavBand::_BrowserExec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hres = E_FAIL;
    IOleCommandTarget* pct;

    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IOleCommandTarget, (void**)&pct)))
    {
        hres = pct->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
        pct->Release();
    }
    return hres;
}

void CFavBand::_OrganizeFavorites()
{
    DoOrganizeFavDlgW(_hwnd, NULL);
}

// *** IOleCommandTarget methods ***
HRESULT CFavBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (pguidCmdGroup && IsEqualGUID(CLSID_FavBand, *pguidCmdGroup))
    {
        TraceMsg(TF_FAVBAND, "CFavBand::Exec CLSID_FavBand -- nCmdID == %x", nCmdID);

        switch(nCmdID)
        {
        case FCIDM_ADDTOFAVORITES:
        {
            _BrowserExec(&CGID_Explorer, SBCMDID_ADDTOFAVORITES, OLECMDEXECOPT_PROMPTUSER, NULL, NULL);
            // Instrument addition to favorites by pane
            UEMFireEvent(&UEMIID_BROWSER, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_ADDTOFAV, UIBL_PANE);     
            return S_OK;
        }

        case FCIDM_ORGANIZEFAVORITES:
        {
            _OrganizeFavorites();
            return S_OK;
        }
        }
    }

    return SUPERCLASS::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}

static const TBBUTTON c_tbFavorites[] =
{
    {  0, FCIDM_ADDTOFAVORITES,     TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0,0}, 0, 0 },
    {  1, FCIDM_ORGANIZEFAVORITES,  TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0,0}, 0, 1 },
};

void CFavBand::_AddButtons(BOOL fAdd)
{
    IExplorerToolbar* piet;

    // BUGBUG: maybe QueryService would be better here ...
    if (SUCCEEDED(_punkSite->QueryInterface(IID_IExplorerToolbar, (void**)&piet)))
    {
        if (fAdd)
        {
            piet->SetCommandTarget((IUnknown*)SAFECAST(this, IOleCommandTarget*), &CLSID_FavBand, 0);

            if (!_fStrsAdded)
            {
                piet->AddString(&CLSID_FavBand, MLGetHinst(), IDS_FAVS_BAR_LABELS, &_lStrOffset);
                _fStrsAdded = TRUE;
            }

            _EnsureImageListsLoaded();
            piet->SetImageList(&CLSID_FavBand, _himlNormal, _himlHot, NULL);

            TBBUTTON tbFavorites[ARRAYSIZE(c_tbFavorites)];
            memcpy(tbFavorites, c_tbFavorites, SIZEOF(TBBUTTON) * ARRAYSIZE(c_tbFavorites));
            for (int i = 0; i < ARRAYSIZE(c_tbFavorites); i++)
                tbFavorites[i].iString += (long) _lStrOffset;

            piet->AddButtons(&CLSID_FavBand, ARRAYSIZE(tbFavorites), tbFavorites);
        }
        else
            piet->SetCommandTarget(NULL, NULL, 0);

        piet->Release();
    }
}

// *** IDockingWindow methods ***
HRESULT CFavBand::ShowDW(BOOL fShow)
{
    HRESULT hres = SUPERCLASS::ShowDW(fShow);
    _AddButtons(fShow);
    return hres;
}


HRESULT CFavBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory
    CFavBand * pfb = new CFavBand();
    if (!pfb)
        return E_OUTOFMEMORY;

    if (SUCCEEDED(pfb->_Init((LPCITEMIDLIST)CSIDL_FAVORITES)))
    {
        pfb->_pns = CNscTree_CreateInstance();
        if (pfb->_pns)
        {
            ASSERT(poi);
            pfb->_poi = poi;   
            // if you change this cast, fix up CChannelBand_CreateInstance
            *ppunk = SAFECAST(pfb, IDeskBand *);

            IUnknown_SetSite(pfb->_pns, *ppunk);
            pfb->SetNscMode(MODE_FAVORITES);
            return S_OK;
        }
    }
    pfb->Release();

    return E_FAIL;
}

