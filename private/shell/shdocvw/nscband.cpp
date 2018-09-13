/**************************************************************\
    FILE: NSCBand.cpp

    DESCRIPTION:  implementation of CNSCBand.  the class CNSCBand 
        exists to support name space control bands.  A name 
        space control uses IShellFolder rooted in various 
        namespaces including Favorites, history, Shell Name 
        Space, etc. to depict a hierarchical UI 
        representation of the given name space.  
    
    AUTHOR:  chrisny

\**************************************************************/
#include "priv.h"
#include "sccls.h"
#include "util.h"
#include "resource.h"
#include "inpobj.h"
#include "dhuihand.h"
#include "nscband.h"

#include <mluisupp.h>

HRESULT CNSCBand::_Init(LPCITEMIDLIST pidl)
{
    // further initialization happens in ShowDW
    _fInited = FALSE;
    _fVisible = FALSE;
    _fCanFocus = TRUE;
    _haccTree = LoadAccelerators(MLGetHinst(), MAKEINTRESOURCE(ACCEL_FAVBAR));

    // pidl can be real or a CSIDL_ constant
    if (HIWORD(pidl))
        _pidl = ILClone(pidl);
    else
        SHGetSpecialFolderLocation(NULL, LOWORD(pidl), &_pidl);

    return _pidl ? S_OK : E_FAIL;
}

CNSCBand::~CNSCBand()
{
    if (_pidl)
        ILFree(_pidl);
    ATOMICRELEASE(_pns);
    ATOMICRELEASE(_pweh);

    if (_himlNormal)
        ImageList_Destroy(_himlNormal);
    if (_himlHot)  
        ImageList_Destroy(_himlHot);
}

HRESULT CNSCBand::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CNSCBand, IContextMenu),       // IID_IContextMenu
        QITABENT(CNSCBand, IWinEventHandler),   // IID_IWinEventHandler
        QITABENT(CNSCBand, IBandNavigate),      // IID_IBandNavigate
        { 0 },
    };
    HRESULT hres = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hres))
        hres = CToolBand::QueryInterface(riid, ppvObj);
    return hres;
}

#ifndef ENABLE_CCHANNELBAND
HRESULT CNSCBand_CreateInstanceEx(IUnknown *punkOuter, IUnknown **ppunk, 
                                  LPCOBJECTINFO poi, LPCITEMIDLIST pidl)
{
    // aggregation checking is handled in class factory
    HRESULT hres;
    CNSCBand * p = new CNSCBand();
    if (p)
    {
        hres = p->_Init(pidl);
        if (SUCCEEDED(hres))
        {
            p->_pns = CNscTree_CreateInstance();
            if (p->_pns)
            {
                p->_poi = poi;   
                // if you change this cast, fix up CChannelBand_CreateInstance
                *ppunk = SAFECAST(p, IDeskBand *);

                IUnknown_SetSite(p->_pns, *ppunk);
                hres = S_OK;
            }
        }
        p->Release();
    }
    else
        hres = E_OUTOFMEMORY;
    return hres;
}
#endif

#ifdef ENABLE_CHANNELS

extern LPITEMIDLIST Channel_GetFolderPidl();

HRESULT CChannelBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
#ifndef ENABLE_CCHANNELBAND
    ASSERT(FALSE);
    return E_FAIL;
#else
    HRESULT hres = CNSCBand_CreateInstanceEx(punkOuter, ppunk, poi, Channel_GetFolderPidl());
    if (*ppunk) {
        CNSCBand* p = (CNSCBand*)(IDeskBand*)*ppunk;
        p->SetNscMode(MODE_CHANNELS);
    }
    return hres;
#endif
}

#endif  // ENABLE_CHANNELS

extern HRESULT GetHistoryPIDL(LPITEMIDLIST *ppidlHistory);


HRESULT CNSCBand::CloseDW(DWORD dw)
{
    if (_fVisible) 
    {
        _UnregisterBand();
    }

    if (_pns)
    {
        IUnknown_SetSite(_pns, NULL); // Break the ref-count cycle.
    }

    return CToolBand::CloseDW(dw);
}

void CNSCBand::_UnregisterBand()
{
    IBrowserService *pswProxy;
    QueryService(SID_SProxyBrowser, IID_IBrowserService, (LPVOID*)&pswProxy);
    ASSERT(pswProxy);
    if (pswProxy)
    {
        IOleCommandTarget *poctProxy;

        if (SUCCEEDED(pswProxy->QueryInterface(IID_IOleCommandTarget, (void **)&poctProxy)))
        {
            VARIANT var;
            VariantInit(&var);
                
            //  Register ourselves for SBCMDID_SELECTHISTPIDL,SBCMDID_INITFILECTXMENU
            var.vt = VT_UNKNOWN;
            QueryInterface(IID_IUnknown, (void **)&var.punkVal);
            poctProxy->Exec(&CGID_Explorer, SBCMDID_UNREGISTERNSCBAND,  OLECMDEXECOPT_PROMPTUSER, &var, NULL);
            VariantClear(&var);
            poctProxy->Release();
        }
        pswProxy->Release();
    }
}

HRESULT CNSCBand::ShowDW(BOOL fShow)
{
    BOOL fIsHistory = IsEqualCLSID(*_poi->pclsid, CLSID_HistBand);
    if (fShow && _hwnd && !_fVisible)
    {
        IBrowserService *pswProxy;

        QueryService(SID_SProxyBrowser, IID_IBrowserService, (LPVOID*)&pswProxy);
        ASSERT(pswProxy);
        if (!_fInited)
        {
            // this should forward to an overrided method to avoid testing fIsHistory
            //  like _OnRegisterBand, below
            _pns->Initialize(_pidl, (SHCONTF_FOLDERS | SHCONTF_NONFOLDERS), (fIsHistory ? (NSS_NOHISTSELECT | NSS_DROPTARGET | NSS_BROWSERSELECT) : (NSS_DROPTARGET | NSS_BROWSERSELECT)));
        }
        else
        {
            _pns->ShowWindow(TRUE);
        }

        if (pswProxy)
        {
            IOleCommandTarget *poctProxy;

            if (SUCCEEDED(pswProxy->QueryInterface(IID_IOleCommandTarget, (void **)&poctProxy)))
            {
                VARIANT var;
                VariantInit(&var);
                
                //  Register ourselves for SBCMDID_SELECTHISTPIDL,SBCMDID_INITFILECTXMENU
                var.vt = VT_UNKNOWN;
                QueryInterface(IID_IUnknown, (void **)&var.punkVal);

                poctProxy->Exec(&CGID_Explorer, SBCMDID_REGISTERNSCBAND, OLECMDEXECOPT_PROMPTUSER, &var, NULL);

                //clear the variant cheaply
                var.vt = VT_EMPTY;
                Release();

                // do any special registration if necessary
                _OnRegisterBand(poctProxy);
                
                poctProxy->Release();
            }
            pswProxy->Release();
        }
       _fInited = TRUE;
       _fVisible = TRUE;
    }
    else if (!fShow && _fVisible)
    {
        _pns->ShowWindow(FALSE);
        _UnregisterBand();
        _fVisible = FALSE;
    }

    return CToolBand::ShowDW(fShow);
}

HRESULT CNSCBand::GetWindow(HWND * lphwnd)
{
    _pns->CreateTree(_hwndParent, 0, &_hwnd);
    ASSERT(_hwnd);
    return CToolBand::GetWindow(lphwnd);
}

HRESULT CNSCBand::GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                DESKBANDINFO* pdbi) 
{
    _dwBandID = dwBandID;
    pdbi->dwModeFlags = DBIMF_FIXEDBMP | DBIMF_VARIABLEHEIGHT;
    
    pdbi->ptMinSize.x = 16;
    pdbi->ptMinSize.y = 0;
    pdbi->ptMaxSize.x = 32000; // random
    pdbi->ptMaxSize.y = 32000; // random
    pdbi->ptActual.y = -1;
    pdbi->ptActual.x = -1;
    pdbi->ptIntegral.y = 1;

    if (_szTitle[0]) 
    {
        StrCpyNW(pdbi->wszTitle, _szTitle, ARRAYSIZE(pdbi->wszTitle));
    } 
    else 
    {
        CLSID clsid;
        UINT  ids;
        GetClassID(&clsid);
        if (IsEqualIID(clsid, CLSID_FavBand))
            ids = IDS_BAND_FAVORITES;
        else if (IsEqualIID(clsid, CLSID_HistBand)) 
            ids = IDS_BAND_HISTORY;

#ifdef ENABLE_CHANNELS
        else if (IsEqualIID(clsid, CLSID_ChannelBand))         
            ids = IDS_BAND_CHANNELS;
#endif  // ENABLE_CHANNELS

        else 
        {
            ASSERT(FALSE);      // BOGUS BAND!!!
            return S_FALSE;
        }
        MLLoadStringW(ids, pdbi->wszTitle, ARRAYSIZE(pdbi->wszTitle));
    }
    return S_OK;
} 

void _InitColors(BOOL fReinit);
//***   CNSCBand::IWinEventHandler::* {
HRESULT CNSCBand::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    HRESULT hr = E_FAIL;

    if (!_pweh && _pns)
        _pns->QueryInterface(IID_IWinEventHandler, (void **) &_pweh);

    // We need to tell the bandsite that we have become active if we're getting a 
    // click focus or something
    if (uMsg == WM_NOTIFY && ((LPNMHDR)lParam)->code == NM_SETFOCUS)
    {
        UnkOnFocusChangeIS(_punkSite, SAFECAST(this, IInputObject*), TRUE);
    }


    if (_pweh)
        hr = _pweh->OnWinEvent(hwnd, uMsg, wParam, lParam, plres);

    return hr;
}

HRESULT CNSCBand::IsWindowOwner(HWND hwnd)
{
    HRESULT hres;

    hres = SHIsChildOrSelf(_hwnd, hwnd);
    ASSERT(hwnd != NULL || hres == S_FALSE);
    ASSERT(_hwnd != NULL || hres == S_FALSE);
    return hres;
}

//***   CNSCBand::IPersistStream::* {

HRESULT CNSCBand::GetClassID(CLSID *pClassID)
{
    ASSERT(_poi->pclsid != NULL);
    *pClassID = *(_poi->pclsid);
    return S_OK;
}

HRESULT CNSCBand::Load(IStream *pstm)
{
    return S_OK;
}

HRESULT CNSCBand::Save(IStream *pstm, BOOL fClearDirty)
{
    return S_OK;
}

// }

//***   CNSCBand::IContextMenu::* {

HRESULT CNSCBand::QueryContextMenu(HMENU hmenu,
    UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    int i = 0;
#if 0
    HMENU hmenuMe = LoadMenuPopup(MENU_IWBBAND);

    i += Shell_MergeMenus(hmenu, hmenuMe, indexMenu, idCmdFirst + i, idCmdLast, MM_ADDSEPARATOR) - (idCmdFirst + i);
    DestroyMenu(hmenuMe);
#endif

    // aka (S_OK|i)
    return MAKE_HRESULT(ERROR_SUCCESS, FACILITY_NULL, i);
}

HRESULT CNSCBand::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
#if 0
    int idCmd = -1;

    if (!HIWORD(pici->lpVerb))
        idCmd = LOWORD(pici->lpVerb);
    switch (idCmd) 
    {
        case default:
            TraceMsg(DM_ERROR, "cbb::ic cmd=%d not handled", idCmd);
            break;
    }
#endif
    return S_OK;
}

HRESULT CNSCBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    if (pguidCmdGroup && IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        for (UINT i=0; i < cCmds; i++)
        {
            rgCmds[i].cmdf = 0;
            switch (rgCmds[i].cmdID)
            {
                case SBCMDID_INITFILECTXMENU:
                {
                    if (_hwnd && _fVisible)
                    {
                        rgCmds->cmdf = 0;
                        if (pcmdtext) 
                            pcmdtext->cmdtextf = 0;

                        rgCmds->cmdf = OLECMDF_ENABLED;
                        if (pcmdtext)
                        {
                            if (SUCCEEDED(_pns->GetSelectedItemName(pcmdtext->rgwz, pcmdtext->cwBuf)))
                            {
                                pcmdtext->cmdtextf = OLECMDTEXTF_NAME;
                                pcmdtext->cwActual = lstrlenW(pcmdtext->rgwz) + 1;
                            }
                        }
                    }
                }
                case SBCMDID_FILERENAME:
                case SBCMDID_FILEDELETE:
                case SBCMDID_FILEPROPERTIES:
                {
                    LPITEMIDLIST  pidl;

                    // get selected item can return NULL pidl and S_FALSE
                    if (_pns->GetSelectedItem(&pidl, 0) == S_OK)
                    {
                        DWORD rgfAttrib = SFGAO_CANDELETE | SFGAO_CANRENAME | SFGAO_HASPROPSHEET; // CAN_LINK
                        if (SUCCEEDED(IEGetAttributesOf(pidl, &rgfAttrib)))
                        {
                            DWORD nCmdID;
        
                            static const DWORD tbtab[] = {
                                    SBCMDID_FILEDELETE, SBCMDID_FILEPROPERTIES, SBCMDID_FILERENAME };
                            static const DWORD cttab[] = {
                                    SFGAO_CANDELETE,    SFGAO_HASPROPSHEET,     SFGAO_CANRENAME };

                            nCmdID = SHSearchMapInt((int*)tbtab, (int*)cttab, ARRAYSIZE(tbtab), rgCmds[i].cmdID);

                            if (nCmdID != -1 && (rgfAttrib & nCmdID))
                                rgCmds[i].cmdf = OLECMDF_ENABLED;
                        }
                        ILFree(pidl);
                    }
                    break;
                }
                    
                default:
                    break;
            }
        }

        return S_OK;
    }
    return CToolBand::QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
}

HRESULT CNSCBand::_InvokeCommandOnItem(LPCTSTR pszVerb)
{
    HRESULT hr;
    IContextMenu *pcm;
    
    hr = _QueryContextMenuSelection(&pcm);
    if (SUCCEEDED(hr))
    {
        CMINVOKECOMMANDINFOEX ici = 
        {
            SIZEOF(CMINVOKECOMMANDINFOEX),
            0L,
            _hwnd,
            NULL,
            NULL, NULL,
            SW_NORMAL,
        };
        
#ifdef UNICODE
        CHAR szVerbAnsi[MAX_PATH];
        SHUnicodeToAnsi(pszVerb, szVerbAnsi, ARRAYSIZE(szVerbAnsi));
        ici.lpVerb = szVerbAnsi;
        ici.lpVerbW = pszVerb;
        ici.fMask |= CMIC_MASK_UNICODE;
#else
        ici.lpVerb = pszVerb;
#endif
        hr = pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
        pcm->Release();
    }
    
    return hr;
}

HRESULT CNSCBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (pguidCmdGroup == NULL)
    {
        switch (nCmdID)
        {
        case OLECMDID_REFRESH:
            if (_pns && (GetFocus() == _hwnd))
                _pns->Refresh();
            return S_OK;
        }

    }
    else if (pguidCmdGroup && IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        HRESULT hr = S_OK;
        
        switch (nCmdID)
        {
        case SBCMDID_SELECTHISTPIDL:
            if (IsEqualCLSID(*_poi->pclsid, CLSID_HistBand) && _hwnd && _fVisible)
            {
                // If you're not visible do nothing.  On becoming visible
                // use Exec to proxy to get last pidlSelect that you would
                // have shown, had you been visible
                LPCITEMIDLIST pidlSelect = VariantToConstIDList(pvarargIn);
                if (pidlSelect)
                {
                    _pns->SetSelectedItem(pidlSelect, TRUE, FALSE, 0);
                }
            }
            break;

        case SBCMDID_INITFILECTXMENU:
            if (_hwnd && _fVisible)
            {
                if (pvarargOut)
                {
                    VariantClearLazy(pvarargOut);

                    HRESULT hres = _QueryContextMenuSelection((IContextMenu **)&(pvarargOut->punkVal));
                    if (SUCCEEDED(hres))
                    {
                        pvarargOut->vt = VT_UNKNOWN;
                    }
                }
            }
            break;

        case SBCMDID_FILERENAME:
        {
            IShellFavoritesNameSpace *psfns;

            hr = _pns->QueryInterface(IID_IShellFavoritesNameSpace, (void **)&psfns);
            if (SUCCEEDED(hr))
            {
                hr = psfns->InvokeContextMenuCommand(L"rename");
                psfns->Release();
            }
            break;
        }
            
        case SBCMDID_FILEDELETE:
            hr = _InvokeCommandOnItem(TEXT("delete"));
            break;
            
        case SBCMDID_FILEPROPERTIES:
            hr = _InvokeCommandOnItem(TEXT("properties"));
            break;

        default:
            hr = E_FAIL;
            break;
        }
        if (SUCCEEDED(hr))
            return hr;
    }

    return CToolBand::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}


HRESULT CNSCBand::_QueryContextMenuSelection(IContextMenu ** ppcm)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidlSelected;

    *ppcm = NULL;
    hr = _pns->GetSelectedItem(&pidlSelected, 0);
    if (SUCCEEDED(hr))
    {
        LPCITEMIDLIST pidlRelative;
        IShellFolder * psf;

        hr = IEBindToParentFolder(pidlSelected, &psf, &pidlRelative);
        if (SUCCEEDED(hr))
        {
            hr = psf->GetUIObjectOf(NULL, 1, &pidlRelative, IID_IContextMenu, NULL, (void **)ppcm);
        }
        ILFree(pidlSelected);
    }
    
    return hr;
}


HRESULT CNSCBand::Select(LPCITEMIDLIST pidl)
{
    _pns->SetSelectedItem(pidl, TRUE, FALSE, 0);
    return S_OK;
}


// *** IInputObject Methods ***
HRESULT CNSCBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    HWND hwndFocus = GetFocus();
    if (_pns->InLabelEdit())
        return EditBox_TranslateAcceleratorST(lpMsg);
    else if ( lpMsg && lpMsg->hwnd && SendMessage(lpMsg->hwnd, TVM_TRANSLATEACCELERATOR, 0, (LPARAM)lpMsg))
        return S_OK;
    else if (hwndFocus == _hwnd && TranslateAcceleratorWrap(_hwnd, _haccTree, lpMsg))
        return S_OK;

    return S_FALSE;
}


void CNSCBand::_EnsureImageListsLoaded()
{
    if (_himlNormal == NULL)
        _himlNormal = ImageList_LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDB_HISTORYANDFAVBANDSDEF), 18, 3, RGB(255, 0, 255), IMAGE_BITMAP, LR_CREATEDIBSECTION);

    if (_himlHot == NULL)
        _himlHot = ImageList_LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDB_HISTORYANDFAVBANDSHOT), 18, 3, RGB(255, 0, 255), IMAGE_BITMAP, LR_CREATEDIBSECTION);
}

