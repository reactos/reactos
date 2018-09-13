#include "priv.h"
#pragma hdrstop

#ifdef UNIX

#include "sccls.h"
#include "bands.h"
#include "util.h"
#include "resource.h"
#include "inpobj.h"
#include "dhuihand.h"
#include "droptgt.h"
#include "iface.h"
#include "stream.h"
#include "isfband.h"
#include "itbdrop.h"
#include "browband.h"
#include "mluisupp.h"

extern "C"  const GUID  CLSID_MsgBand;
STDAPI_(void) unixGetWininetCacheLockStatus (BOOL *pBoolReadOnly, char **ppszLockingHost);
STDAPI_(void) unixCleanupWininetCacheLockFile();


////////////////
///  Msg (BrowserOC) band

#undef SUPERCLASS 
#define SUPERCLASS CBrowserBand

class CMsgBand : public CBrowserBand
{

public:
    // *** IDeskBand methods ***
    virtual STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                   DESKBANDINFO* pdbi) ;
    
    // *** IPersistStream methods ***
    // (others use base class implementation) 
    virtual STDMETHODIMP GetClassID(CLSID *pClassID);
    virtual STDMETHODIMP Load(IStream *pStm);
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);

    // *** IOleCommandTarget methods ***
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // *** IDockingWindow methods ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);

    CMsgBand();
    virtual ~CMsgBand();

    virtual void _InitBrowser(void);
    virtual STDMETHODIMP Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,UINT * puArgErr);

protected:
    void _OnSearchBtnSelect(int x, int y);
    void _CloseBand();
    BOOL _fStrsAdded;
    long _lStrOffset;

};

CMsgBand::CMsgBand() :
    SUPERCLASS()
{
    // _fBlockSIDProxy = FALSE;
    _fBlockDrop = TRUE;
    return;
}

CMsgBand::~CMsgBand()
{
}

void CMsgBand::_OnSearchBtnSelect(int x, int y)
{
    HMENU        hmenu = CreatePopupMenu();
    
    if (hmenu)
    {
        IContextMenu* pcm;

        if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SExplorerToolbar, IID_IContextMenu2, (void**)&pcm)))
        {
            HWND  hwnd;
            
            if (SUCCEEDED(IUnknown_GetWindow(pcm, &hwnd)))
            {
                int   idCmd;
                
                pcm->QueryContextMenu(hmenu, 0, FCIDM_SEARCHFIRST, FCIDM_SEARCHLAST, 0);
            
                idCmd = TrackPopupMenu(hmenu, TPM_RETURNCMD, x, y, 0, hwnd, NULL);
                
                if (idCmd != 0)
                {
                    CMINVOKECOMMANDINFO ici = {0};
    
                    ici.cbSize = SIZEOF(ici);
                    ici.hwnd = _hwnd;
                    ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - FCIDM_SEARCHFIRST);
                    ici.nShow  = SW_NORMAL;
                    pcm->InvokeCommand(&ici);
                }
            }
            pcm->Release();
        }
        DestroyMenu(hmenu);
    }
}

HRESULT CMsgBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    switch (nCmdID)
    {
    case TBIDM_SEARCH:
         if (EVAL(pvarargIn && (pvarargIn->vt == VT_I4)))
             _OnSearchBtnSelect(LOWORD(pvarargIn->lVal), HIWORD(pvarargIn->lVal));
         return S_OK;
    }
    return CBrowserBand::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}

void CMsgBand::_CloseBand()
{
    // Excute SBCMDID_MSGBAND command on the site.
    IUnknown_Exec(_punkSite, &CGID_Explorer, SBCMDID_MSGBAND, FALSE, NULL, NULL);
}

HRESULT CMsgBand::ShowDW(BOOL fShow)
{
    HRESULT hres = CBrowserBand::ShowDW(fShow);
    return hres;
}

/////////////////////////////////////////////////////////////////////////////
// IDispatch::Invoke
/////////////////////////////////////////////////////////////////////////////
HRESULT CMsgBand::Invoke
(
    DISPID          dispidMember,
    REFIID          riid,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pdispparams,
    VARIANT *       pvarResult,
    EXCEPINFO *     pexcepinfo,
    UINT *          puArgErr
)
{
    HRESULT hr = S_OK;

    ASSERT(pdispparams);
    if(!pdispparams)
        return E_INVALIDARG;

    //
    // Big HACK!  Do this right after RTW.  Basically, I didn't have time to
    // implement new DISPIDs and connect them up to the MsgBand class correctly.
    // Instead, I overload the title name to be a function call.  The problem
    // with this scheme is that the title is updated all the time so you can
    // only call each function once realiably (of course, for us that's fine :)
    // davidd
    //

    switch (dispidMember)
    {
    case DISPID_TITLECHANGE:
    {
        int iArg = pdispparams->cArgs -1;

        if (iArg == 0 && (pdispparams->rgvarg[iArg].vt == VT_BSTR)) 
        {
            static BOOL s_fLockDeleted = FALSE;
            BSTR pArg = pdispparams->rgvarg[iArg].bstrVal;

            if ( !s_fLockDeleted &&
                 StrCmpW( pArg, L"close" ) == 0 )
            {
                s_fLockDeleted = TRUE;
                _CloseBand();
            }

            if ( !s_fLockDeleted &&
                 StrCmpW( pArg, L"deleteLock" ) == 0 )
            {
                s_fLockDeleted = TRUE;
                unixCleanupWininetCacheLockFile();
                _CloseBand();
            }

            if ( !s_fLockDeleted &&
                 StrCmpW( pArg, L"deleteLockAndShutdown" ) == 0 )
            {
                IOleCommandTarget *poct;

                s_fLockDeleted = TRUE;
                unixCleanupWininetCacheLockFile();

                // Initiate shutdown
                PostMessage( GetActiveWindow(), WM_QUIT, 10, 0 );
            }

            if ( StrCmpW( pArg, L"Loaded" ) == 0) {

                //
                // Change all elements of ID "lockedMachine" to the actual
                // machine name.
                //

                static IHTMLElementCollection *s_pIElementCollection = NULL;

                IDispatch *pIDispatch;
                IHTMLDocument2 *pIDocument;
                IHTMLElement *pIElement;
                CHAR *pszLockingHostAnsi;
                HRESULT hr;
                BSTR bstr;
                SA_BSTR strLockingHost;

                unixGetWininetCacheLockStatus( NULL, &pszLockingHostAnsi );
                if ( NULL == pszLockingHostAnsi ) 
                    break;

                SHAnsiToUnicode( pszLockingHostAnsi, strLockingHost.wsz, ARRAYSIZE(strLockingHost.wsz));
                strLockingHost.cb = lstrlenW(strLockingHost.wsz) * SIZEOF(WCHAR);

                if ( !s_pIElementCollection ) {
                    if ( !_pauto )
                        break;

                    hr = _pauto->get_Document( &pIDispatch );
                    // IEUNIX
                    // WebOC returns hr = S_OK and also sets pIDispatch to NULL
                    if ( !SUCCEEDED( hr ) || !pIDispatch) 
                        break;

                    hr = pIDispatch->QueryInterface( IID_IHTMLDocument2, (void**)&pIDocument );
                    pIDispatch->Release();
                    if ( !SUCCEEDED( hr ))
                        break;

                    hr = pIDocument->get_all( &s_pIElementCollection );
                    pIDocument->Release();
                    if ( !SUCCEEDED( hr )) {
                        s_pIElementCollection = NULL;
                        break;
                    }
                }

                bstr = SysAllocString(L"lockedMachine");
                if (bstr)
                {
                    VARIANT va;
                    VARIANT vaIndex;

                    va.vt = VT_BSTR;
                    va.bstrVal = bstr;

                    vaIndex.vt = VT_I4;
                    vaIndex.intVal = 0;

                    do {
                        hr = s_pIElementCollection->item( va, vaIndex, &pIDispatch );
                        if ( !SUCCEEDED( hr ) || !pIDispatch)
                            break;

                        hr = pIDispatch->QueryInterface( IID_IHTMLElement, (void**) &pIElement );
                        pIDispatch->Release();
                        if ( !SUCCEEDED( hr ))
                            break;

                        pIElement->put_innerText( strLockingHost.wsz );
                        pIElement->Release();

                        vaIndex.intVal++;
                    } while ( TRUE );

                    SysFreeString( bstr );
                }
                break;
            }
        }
        break;
    }
    default:
        SUPERCLASS::Invoke( dispidMember, riid, lcid, wFlags, pdispparams,
                            pvarResult, pexcepinfo, puArgErr );
    }

    return hr;
}

STDAPI CMsgBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    TCHAR szResURL[MAX_URL_STRING];
    LPITEMIDLIST pidlNew;
    HRESULT hr;

    *ppunk = NULL;
    hr = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                           HINST_THISDLL,
                           ML_CROSSCODEPAGE,
                           TEXT("cachewrn.htm"),
                           szResURL,
                           ARRAYSIZE(szResURL),
                           TEXT("shdocvw.dll"));
    if (SUCCEEDED(hr))
    {
        hr = IECreateFromPath(szResURL, &pidlNew);
        if (SUCCEEDED(hr))
        {
            CMsgBand *p = new CMsgBand();

            if (p)
            {
                p->_pidl = pidlNew;
                *ppunk = SAFECAST(p, IDeskBand*);
                hr = NOERROR;
            }
            else
            {
                ILFree(pidlNew);
                hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}


void CMsgBand::_InitBrowser(void)
{
    SUPERCLASS::_InitBrowser();
    _pauto->put_Silent(VARIANT_FALSE);
}

HRESULT CMsgBand::GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                DESKBANDINFO* pdbi) 
{
    _dwBandID = dwBandID;
    pdbi->dwModeFlags = DBIMF_FIXEDBMP;
    
    pdbi->ptMinSize.x = 16;
    pdbi->ptMinSize.y = 0;
    pdbi->ptMaxSize.x = 32000; // random
    pdbi->ptMaxSize.y = 32000; // random
    pdbi->ptActual.y = -1;
    pdbi->ptActual.x = -1;
    pdbi->ptIntegral.y = 1;
    pdbi->dwModeFlags |= DBIMF_VARIABLEHEIGHT;

    MLLoadStringW(IDS_BAND_MESSAGE, pdbi->wszTitle, ARRAYSIZE(pdbi->wszTitle));
    
    return S_OK;
} 


//***   CMsgBand::IPersistStream::* {

        extern "C"  const GUID     CLSID_MsgBand  ;

HRESULT CMsgBand::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_MsgBand;

    return S_OK;
}

HRESULT CMsgBand::Load(IStream *pstm)
{
    _NavigateOC();
    
    return S_OK;
}

HRESULT CMsgBand::Save(IStream *pstm, BOOL fClearDirty)
{
    return S_OK;
}

// }

IDeskBand* CMsgBand_Create()
{
    HRESULT hr;
    IUnknown *punk;
    IDeskBand* pistb = NULL;

    hr = CMsgBand_CreateInstance(NULL, &punk, NULL);
    if (SUCCEEDED(hr))
    {
        EVAL(punk->QueryInterface(IID_IDeskBand,(LPVOID *)&pistb));

        // if we succeeded, release the 2nd refcnt and return non-NULL;
        // if we failed   , release the 1st refcnt and return NULL
        punk->Release();
    }
    return pistb;
}

// }
// 


#endif // UNIX


