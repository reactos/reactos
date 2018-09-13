#include "priv.h"
#include "sccls.h"
#include <shguidp.h>
#define WANT_CBANDSITE_CLASS
#include "bandsite.h"
#include "bandobj.h"
#include "caggunk.h"
#include "droptgt.h"
#include "inpobj.h"
#include "resource.h"
#include "bands.h"
#include "tbmenu.h"

#include "mluisupp.h"

#define TF_BANDDD   0x00400000
#define DM_INIT     0               //
#define DM_PERSIST  0               // trace IPS::Load, ::Save, etc.
#define DM_MENU     0               // menu code
#define DM_DRAG     0               // drag&drop
#define DM_FOCUS    0               // focus
#define DM_PERF     0               // perf tune
#define DM_PERF2    0               // perf tune (verbose)

#define IDM_DRAGDROP    1

#define ISMOVEDDISABLED(dwBandID)   ((S_OK == _IsRestricted(dwBandID, RA_MOVE, BAND_ADMIN_NOMOVE)) ? TRUE : FALSE)
#define ISDDCLOSEDISABLED(dwBandID) ((S_OK == _IsRestricted(dwBandID, RA_DRAG, BAND_ADMIN_NODDCLOSE)) ? TRUE : FALSE)

// drag state (BUGBUG from dockbar.h)
#define DRAG_NIL        0       // nil
#define DRAG_MOVE       1       // moving
#define DRAG_SIZE       2       // sizing

typedef struct {
    UINT cx;
    UINT fStyle;
    UINT cxMinChild;
    UINT cyMinChild;
    UINT cyIntegral;
    UINT cyMaxChild;
    UINT cyChild;
} PERSISTBANDINFO_V3;


typedef struct {
    UINT cx;
    UINT fStyle;
    UINT cxMinChild;  // UNUSED. reclaim!
    UINT cyMinChild;
    UINT cyIntegral;   // UNUSED
    UINT cyMaxChild;    // UNUSED.
    UINT cyChild;
    DWORD dwAdminSettings;
    BITBOOL fNoTitle:1;
} PERSISTBANDINFO;
#define RBBIM_XPERSIST  (RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_STYLE)

#ifdef DEBUG

extern unsigned long DbStreamTell(IStream *pstm);

#else

#define DbStreamTell(pstm)      ((ULONG) 0)

#endif

UINT _FixMenuIndex(HMENU hmenu, UINT indexMenu)
{
    UINT i;

    i = GetMenuItemCount(hmenu);
    if (indexMenu > i)
        indexMenu = i;
    return indexMenu;
}

#define SUPERCLASS CAggregatedUnknown


HRESULT CBandSite::v_InternalQueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        // perf: last tuned 980728
        QITABENT(CBandSite, IBandSite),             // IID_IBandSite
        QITABENT(CBandSite, IInputObject),          // IID_IInputObject
        QITABENT(CBandSite, IServiceProvider),      // IID_IServiceProvider
        QITABENT(CBandSite, IOleCommandTarget),     // IID_IOleCommandTarget
        QITABENTMULTI(CBandSite, IOleWindow, IDeskBarClient),   // IID_IOleWindow
        QITABENT(CBandSite, IWinEventHandler),      // IID_IWinEventHandler
        QITABENT(CBandSite, IInputObjectSite),      // IID_IInputObjectSite
        QITABENT(CBandSite, IDeskBarClient),        // IID_IDeskBarClient
        QITABENTMULTI(CBandSite, IPersist, IPersistStream),     // rare IID_IPersist
        QITABENT(CBandSite, IPersistStream),        // rare IID_IPersistStream
        QITABENT(CBandSite, IBandSiteHelper),       // rare IBandSiteHelper
        QITABENT(CBandSite, IDropTarget),           // rare IID_IDropTarget
        { 0 },
    };
    
    return QISearch(this, qit, riid, ppvObj);
}

/////  impl of IServiceProvider
HRESULT CBandSite::QueryService(REFGUID guidService, REFIID riid, void **ppvObj)
{
    HRESULT hres = E_FAIL;
    *ppvObj = NULL; // assume error

    if (IsEqualIID(guidService, SID_IBandProxy)) 
    {
        hres =  QueryService_SID_IBandProxy(_punkSite, riid, &_pbp, ppvObj);
        if(!_pbp)
        {
            // We need to create it ourselves since our parent couldn't help
            ASSERT(FALSE == _fCreatedBandProxy);

            hres = CreateIBandProxyAndSetSite(_punkSite, riid, &_pbp, ppvObj);
            if(_pbp)
            {
                ASSERT(S_OK == hres);
                _fCreatedBandProxy = TRUE;   
            }
        }
    } 
    else if (IsEqualIID(guidService, SID_ITopViewHost)) 
    {
        return QueryInterface(riid, ppvObj);
    } 
    else if (IsEqualIID(guidService, IID_IBandSite))
    {
        // It is common for bands to save/load pidls for persistence.
        // CShellLink is a robust way to do this, so let's share one
        // among all the bands.
        //
        // NOTE: This is shared between bands, so if you request it
        // you must complete your use of it within the scope of your
        // function call!
        //
        if (IsEqualIID(riid, IID_IShellLinkA))
        {
            if (NULL == _plink)
                CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkA, (void **)&_plink);
            if (_plink)
            {
                *ppvObj = _plink;
                _plink->AddRef();
                hres = S_OK;
            }
        }
    } 
    else if (_punkSite) 
    {
        hres = IUnknown_QueryService(_punkSite, guidService, riid, ppvObj);
    }
    return hres;
}

HRESULT CBandSite::GetWindow(HWND * lphwnd)
{
    *lphwnd = _hwnd;

    return *lphwnd ?  S_OK : E_FAIL;
}

CBandSite::CBandSite(IUnknown* punkAgg) : SUPERCLASS(punkAgg)
{
    DWORD dwData = 0;
    DWORD dwSize = SIZEOF(dwData);

    // We assume this object was zero inited.
    ASSERT(!_pbp);
    ASSERT(FALSE == _fCreatedBandProxy);
    SHRegGetUSValue(SZ_REGKEY_GLOBALADMINSETTINGS, SZ_REGVALUE_GLOBALADMINSETTINGS,
        NULL, (LPVOID) &dwData, &dwSize, FALSE, NULL, 0);

    if (IsFlagSet(dwData, BAND_ADMIN_ADMINMACHINE))
        _fIEAKInstalled = TRUE;
    else
        _fIEAKInstalled = FALSE;

    _dwStyle = BSIS_AUTOGRIPPER;

    //
    //  We check whether or not this succeeded in CBandSite::_Initialize
    //
    _QueryOuterInterface(IID_IBandSite, (void **)&_pbsOuter);
    DllAddRef();
}

void CBandSite::_ReleaseBandItemData(LPBANDITEMDATA pbid, int iIndex)
{
    if (pbid->pdb) 
    {
        REBARBANDINFO rbbi;

        pbid->pdb->CloseDW(0);

        if (-1 != iIndex)
        {
            // The band's hwnd is typically destroyed in CloseDW
            rbbi.cbSize = sizeof(rbbi);
            rbbi.fMask = RBBIM_CHILD | RBBIM_LPARAM;
            rbbi.hwndChild = NULL;
            rbbi.lParam = NULL;
            EVAL( SendMessage(_hwnd, RB_SETBANDINFO, iIndex, (LPARAM) &rbbi) );
        }

        // this is called from remove and the destroy.
        IUnknown_SetSite(pbid->pdb, NULL);
        ATOMICRELEASE(pbid->pdb);
    }

    if (pbid->pweh == _pwehCache)
        ATOMICRELEASE(_pwehCache);

    ATOMICRELEASE(pbid->pweh);
    LocalFree(pbid);
}

CBandSite::~CBandSite()
{
    ATOMICRELEASE(_pdtobj);

    if(_pbp && _fCreatedBandProxy)
        _pbp->SetSite(NULL);
        
    ATOMICRELEASE(_pbp);

    ATOMICRELEASE(_pwehCache);
    _CacheActiveBand(NULL);

    _Close();

    SetDeskBarSite(NULL);

    if (_plink)
        _plink->Release();

    RELEASEOUTERINTERFACE(_pbsOuter);
    DllRelease();
}

//***   _IsBandDeleteable --
// ENTRY/EXIT
//  idBand  band ID
//  ret     TRUE if deletable, o.w. FALSE (also FALSE on bogus band)
BOOL CBandSite::_IsBandDeleteable(DWORD dwBandID)
{
    DWORD dwState;

    if (FAILED(_pbsOuter->QueryBand(dwBandID, NULL, &dwState, NULL, 0))
      || (dwState & BSSF_UNDELETEABLE)) {
        return FALSE;
    }

    ASSERT(dwBandID != (DWORD)-1);  // make sure QueryBand catches this

    return TRUE;
}

DWORD CBandSite::_GetAdminSettings(DWORD dwBandID)
{
    LPBANDITEMDATA pbid = _GetBandItem(_BandIDToIndex(dwBandID)); 

    if (EVAL(pbid))
        return pbid->dwAdminSettings;

    return BAND_ADMIN_NORMAL;
}


void CBandSite::_SetAdminSettings(DWORD dwBandID, DWORD dwNewAdminSettings)
{
    LPBANDITEMDATA pbid = _GetBandItem(_BandIDToIndex(dwBandID)); 

    if (EVAL(pbid))
        pbid->dwAdminSettings = dwNewAdminSettings;
}


//***   CBandSite::IBandSite::* {

/*----------------------------------------------------------
Purpose: IBandSite::EnumBands method

*/
HRESULT CBandSite::EnumBands(UINT uBand, DWORD* pdwBandID)
{
    ASSERT((NULL == pdwBandID && (UINT)-1 == uBand) || 
           IS_VALID_WRITE_PTR(pdwBandID, DWORD));

    if (uBand == (UINT)-1)
        return _GetBandItemCount();      // query count

    LPBANDITEMDATA pbid = _GetBandItem(uBand);
    if (pbid)
    {
        *pdwBandID = pbid->dwBandID;
        return S_OK;
    }
    return E_FAIL;
}


/*----------------------------------------------------------
Purpose: IBandSite::QueryBand method

*/
HRESULT CBandSite::QueryBand(DWORD dwBandID, IDeskBand** ppstb, DWORD* pdwState, LPWSTR pszName, int cchName)
{
    ASSERT(NULL == ppstb || IS_VALID_WRITE_PTR(ppstb, IDeskBand));
    ASSERT(NULL == pdwState || IS_VALID_WRITE_PTR(pdwState, DWORD));
    ASSERT(NULL == pszName || IS_VALID_WRITE_BUFFER(pszName, WCHAR, cchName));

    if (ppstb)
        *ppstb = NULL;

    LPBANDITEMDATA pbid = _GetBandItemDataStructByID(dwBandID);
    if (!pbid)
        return E_FAIL;
    if (pszName) {
        StrCpyNW(pszName, pbid->szTitle, cchName);
    }

    if (ppstb) {
        *ppstb = pbid->pdb;
        pbid->pdb->AddRef();
    }
    
    if (pdwState) {
        *pdwState = 0;
        if (pbid->fShow)
            *pdwState = BSSF_VISIBLE;
        if (pbid->fNoTitle)
            *pdwState |= BSSF_NOTITLE;
        if (pbid->dwModeFlags & DBIMF_UNDELETEABLE)
            *pdwState |= BSSF_UNDELETEABLE;
    }

    return S_OK;
}


/*----------------------------------------------------------
Purpose: IBandSite::SetBandState

* NOTES
*   failure handling is inconsistent (1 band vs. all bands case)
*/
HRESULT CBandSite::SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
    LPBANDITEMDATA pbid;
    HRESULT hr;

    if (dwBandID == (DWORD) -1)
    {
        BOOL fChange = FALSE;
        for (int i = _GetBandItemCount() - 1; i >= 0; i--)
        {
            pbid = _GetBandItem(i);
            if (pbid)
            {
                hr = _SetBandStateHelper(pbid->dwBandID, dwMask, dwState);
                ASSERT(SUCCEEDED(hr));
                fChange |= (hr != S_OK);
            }
            else
            {
                return E_FAIL;
            }
        }
        if (fChange)
            _UpdateAllBands(FALSE, FALSE);
        return S_OK;
    }
    else
    {
        hr = _SetBandStateHelper(dwBandID, dwMask, dwState);
        if (SUCCEEDED(hr) && hr != S_OK)
        {
            _UpdateBand(dwBandID);
            return S_OK;
        }
    }
    return E_FAIL;
}

//***
// ENTRY/EXIT
//  ret     S_OK|changed on success, o.w. E_*.
// NOTES
//  only a helper for SetBandState, don't call directly
HRESULT CBandSite::_SetBandStateHelper(DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
    LPBANDITEMDATA pbid;

    pbid = _GetBandItem(_BandIDToIndex(dwBandID));
    if (pbid) {
        DWORD dwOldState;

        if (FAILED(QueryBand(dwBandID, NULL, &dwOldState, NULL, 0))) {
            ASSERT(0);  // 'impossible'
            dwOldState = (DWORD)-1;
        }

        if (dwMask & BSSF_VISIBLE)
            _ShowBand(pbid, dwState & BSSF_VISIBLE);

        if (dwMask & BSSF_NOTITLE)
            pbid->fNoTitle = BOOLIFY(dwState & BSSF_NOTITLE);
            
        // BUGBUG (kkahl): BSSF_UNDELETABLE cannot currently be modified with
        // this interface.

        return ResultFromShort((dwOldState ^ dwState) & dwMask);
    }
    return E_FAIL;
}

//***   _CheckNotifyOnAddRemove -- handle notifies for add/remove/empty
// DESCRIPTION
//  add/remove always sends a BSID_BANDADDED/BSID_BANDREMOVED.
//  remove of last always sends a DBCID_EMPTY.
//  in floating mode, a transition to/from 1 band does a refresh.
//
void CBandSite::_CheckNotifyOnAddRemove(DWORD dwBandID, int iCode)
{
    int cBands;
    if (!_pct)
        return;

    if (iCode == CNOAR_CLOSEBAR) {
        // Shut down the whole thing
        cBands = 0;
    } else {
        VARIANTARG var;
        int nCmdID;

        cBands = _GetBandItemCount();   // post-op # (since op happened in caller)

        VariantInit(&var);
        var.vt = VT_UI4;
        var.ulVal = dwBandID;

        BOOL fOne = FALSE;
        switch (iCode) {
        case CNOAR_ADDBAND:
            fOne = (cBands == 2);   // 1->2
            nCmdID = BSID_BANDADDED;
            break;
        case CNOAR_REMOVEBAND:
            fOne = (cBands == 1);   // 2->1
            nCmdID = BSID_BANDREMOVED;
            break;
        default:
            ASSERT(0);
            return;
        }

        if ((fOne && (_dwMode & DBIF_VIEWMODE_FLOATING))) {
            // n.b. fBSOnly *must* be TRUE for perf
            _UpdateAllBands(TRUE, TRUE);  // force refresh of optional gripper/title
        }

        _pct->Exec(&CGID_BandSite, nCmdID, 0, &var, NULL);
    }

    if (cBands == 0) {
        ASSERT(iCode != CNOAR_ADDBAND);     // sanity check
        _pct->Exec(&CGID_DeskBarClient, DBCID_EMPTY, 0, NULL, NULL);
    }

    return;
}

/*----------------------------------------------------------
Purpose: IBandSite::RemoveBand method

*/
HRESULT CBandSite::RemoveBand(DWORD dwBandID)
{
    int iIndex = _BandIDToIndex(dwBandID);
    LPBANDITEMDATA pbid = _GetBandItem(iIndex);
    if (pbid)
    {
        // Release the banditem data first, while it can still
        // receive cleanup notifications from its control.  *Then*
        // delete the band item.
        _ReleaseBandItemData(pbid, iIndex);
        _DeleteBandItem(iIndex);    // unhook from host (rebar)
        _CheckNotifyOnAddRemove(dwBandID, CNOAR_REMOVEBAND);
        return S_OK;
    }
    return E_FAIL;
}


void CBandSite::_OnCloseBand(DWORD dwBandID)
{
    if (dwBandID == -1) {
        // Close everything
        _CheckNotifyOnAddRemove(dwBandID, CNOAR_CLOSEBAR);
    } else {
        // Close just this band

        LPBANDITEMDATA pbid = _GetBandItemDataStructByID(dwBandID);
        USES_CONVERSION;

        if (EVAL(pbid) && ConfirmRemoveBand(_hwnd, IDS_CONFIRMCLOSEBAND,W2T(pbid->szTitle)))
            RemoveBand(dwBandID);
    }
}

void CBandSite::_MaximizeBand(DWORD dwBandID)
{
    SendMessage(_hwnd, RB_MAXIMIZEBAND, _BandIDToIndex(dwBandID), TRUE);
}

//
// private insert a band into the container control by ID
// returns the band ID in ShortFromResult(hres)
//

HRESULT CBandSite::_AddBandByID(IUnknown *punk, DWORD dwID)
{
    IDeskBand *pdb;
    HRESULT hres = punk->QueryInterface(IID_IDeskBand, (void **)&pdb);
    if (SUCCEEDED(hres)) 
    {
        BANDITEMDATA *pbid = (BANDITEMDATA *)LocalAlloc(LPTR, sizeof(BANDITEMDATA));
        if (pbid)
        {
            pbid->dwBandID = dwID;
            pbid->pdb = pdb;      // ref held by QI above
            pbid->fShow = TRUE;     // initially visible

            pbid->pdb->QueryInterface(IID_IWinEventHandler, (void **)&pbid->pweh);
            IUnknown_SetSite(pbid->pdb, SAFECAST(this, IBandSite*));
            pbid->pdb->GetWindow(&pbid->hwnd);
            
            if (_AddBandItem(pbid))
            {
                if (_dwShowState == DBC_SHOW) 
                {
                    ASSERT(pbid->fShow);
                    pbid->pdb->ShowDW(TRUE);
                    _MaximizeBand(pbid->dwBandID);
                }
            
                _CheckNotifyOnAddRemove(pbid->dwBandID, CNOAR_ADDBAND);
                hres = ResultFromShort(pbid->dwBandID); // success
            }
            else
            {
                _ReleaseBandItemData(pbid, -1);
            }

            //
            // Now that we've added the band, clear the _SendToToolband cache.
            //
            // We need to do this because we might have gotten a message for
            // the band before it was inserted, in which case we'll have cached
            // a NULL handler for the band's hwnd (preventing the band from
            // getting any messages thereafter).
            //
            ATOMICRELEASE(_pwehCache);
            _hwndCache = NULL;
        } 
        else
        {
            hres = E_OUTOFMEMORY;
            pdb->Release();    // don't hold on to this
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IBandSite::AddBand method.

         Insert a band into the container control.

Returns: the band ID in ShortFromResult(hres)

*/
HRESULT CBandSite::AddBand(IUnknown *punk)
{
    HRESULT hres = _AddBandByID(punk, _dwBandIDNext);
    if (SUCCEEDED(hres)) {
        ASSERT(ShortFromResult(hres) == (int)_dwBandIDNext);
        _dwBandIDNext++;
    }
    return hres;
}

void CBandSite::_UpdateBand(DWORD dwBandID)
{
    LPBANDITEMDATA pbid = _GetBandItem(_BandIDToIndex(dwBandID)); 
    if (pbid) {
        _UpdateBandInfo(pbid, FALSE);
        _OnRBAutoSize(NULL);
    }
}

void CBandSite::_UpdateAllBands(BOOL fBSOnly, BOOL fNoAutoSize)
{
    BOOL_PTR fRedraw = SendMessage(_hwnd, WM_SETREDRAW, FALSE, 0);

    for (int i = _GetBandItemCount() - 1; i >= 0; i--)
    {
        LPBANDITEMDATA pbid = _GetBandItem(i);
        if (pbid)
            _UpdateBandInfo(pbid, fBSOnly);
    }    

    SendMessage(_hwnd, WM_SETREDRAW, fRedraw, 0);

    if (!fNoAutoSize) {
        SendMessage(_hwnd, RB_SIZETORECT, 0, 0);
        _OnRBAutoSize(NULL);
    }
}

// *** IOleCommandTarget ***
HRESULT CBandSite::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    HRESULT hres = OLECMDERR_E_UNKNOWNGROUP;

    if (pguidCmdGroup)
    {
        if (IsEqualIID(*pguidCmdGroup, IID_IDockingWindow))
        {
            for (ULONG i=0 ; i<cCmds ; i++)
            {
                switch (rgCmds[i].cmdID)
                {
                case DBID_BANDINFOCHANGED:
                case DBID_PUSHCHEVRON:
                    rgCmds[i].cmdf = OLECMDF_ENABLED;
                    break;
    
                default:
                    rgCmds[i].cmdf = 0;
                    break;
                }
            }
            hres = S_OK;
            goto Lret;
        }
        else if (IsEqualIID(*pguidCmdGroup, CGID_Explorer))
        {
            hres = IUnknown_QueryStatus(_ptbActive, pguidCmdGroup, cCmds, rgCmds, pcmdtext);
            goto Lret;
        }
    }

    // if we got here, we didn't handle it
    // forward it down
    hres = MayQSForward(_ptbActive, OCTD_DOWN, pguidCmdGroup, cCmds, rgCmds, pcmdtext);

Lret:
    return hres;
}


int _QueryServiceCallback(LPBANDITEMDATA pbid, void *pv)
{
    QSDATA* pqsd = (QSDATA*)pv;

    if (pbid->fShow)
        pqsd->hres = IUnknown_QueryService(pbid->pdb, *(pqsd->pguidService), *(pqsd->piid), pqsd->ppvObj);

    // stop if we found the service
    return SUCCEEDED(pqsd->hres) ? FALSE : TRUE;
}


typedef struct {
    HRESULT hres;
    const GUID *pguidCmdGroup;
    DWORD nCmdID;
    DWORD nCmdexecopt;
    VARIANTARG *pvarargIn;
    VARIANTARG *pvarargOut;
} EXECDATA;

int _ExecCallback(LPBANDITEMDATA pbid, void *pv)
{
    EXECDATA* ped = (EXECDATA*)pv;
    
    ped->hres = IUnknown_Exec(pbid->pdb, ped->pguidCmdGroup, ped->nCmdID, ped->nCmdexecopt,
        ped->pvarargIn, ped->pvarargOut);
    return 1;
}

HRESULT CBandSite::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hres = OLECMDERR_E_UNKNOWNGROUP;
    HRESULT hresTmp;

    if (pguidCmdGroup == NULL) {
        /*NOTHING*/
        ;
    }
    else if (IsEqualIID(*pguidCmdGroup, CGID_DeskBand)) {
        switch (nCmdID) {
        case DBID_BANDINFOCHANGED:
            if (!pvarargIn)
                _UpdateAllBands(FALSE, FALSE);
            else if (pvarargIn->vt == VT_I4) 
                _UpdateBand(pvarargIn->lVal);
            hres = S_OK;
            
            // forward this up.
            if (_pct) {
                _pct->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            }
            goto Lret;

        case DBID_PUSHCHEVRON:
            if (pvarargIn && pvarargIn->vt == VT_I4) {
                int iIndex = _BandIDToIndex(nCmdexecopt);
                SendMessage(_hwnd, RB_PUSHCHEVRON, iIndex, pvarargIn->lVal);
                hres = S_OK;
            }
            goto Lret;

        case DBID_MAXIMIZEBAND:
            if (pvarargIn && pvarargIn->vt == VT_UI4)
                _MaximizeBand(pvarargIn->ulVal);
            hres = S_OK;
            goto Lret;
#if 1 // { BUGBUG temporary until add cbs::Select() mfunc
        case DBID_SHOWONLY:
            {
                int iCount = _GetBandItemCount();
                
                // pvaIn->punkVal:
                //  punk hide everyone except me
                //  0    hide everyone
                //  1    show everyone
                // BUGBUG we should use pvaIn->lVal not punkVal since we're
                // allowing 0 & 1 !!! (and not doing addref/release)
                ASSERT(pvarargIn && pvarargIn->vt == VT_UNKNOWN);
                if (pvarargIn->punkVal == NULL || pvarargIn->punkVal == (IUnknown*)1)
                    TraceMsg(TF_BANDDD, "cbs.e: (id=DBID_SHOWONLY, punk=%x)", pvarargIn->punkVal);
                // show myself, hide everyone else
                TraceMsg(TF_BANDDD, "cbs.Exec(DBID_SHOWONLY): n=%d", _GetBandItemCount());

                // wait to show this band until we've hidden the others
                LPBANDITEMDATA pbidShow = NULL;
                // BUGBUG: this (IUnknown*)1 crap is bogus!
                BOOL bShowAll = (pvarargIn->punkVal == (IUnknown*)1);
                for (int i = iCount - 1; i >= 0; i--) {
                    LPBANDITEMDATA pbid = _GetBandItem(i);

                    if (pbid) {
                        BOOL fShow;

                        fShow = bShowAll || SHIsSameObject(pbid->pdb, pvarargIn->punkVal);
                        if (!fShow || bShowAll)
                            _ShowBand(pbid, fShow);
                        else
                            pbidShow = pbid;
                    }
                }
                if (pbidShow) {
                    _ShowBand(pbidShow, TRUE);
                    // nash:37290 set focus to band on open
                    if (_dwShowState == DBC_SHOW)
                        UnkUIActivateIO(pbidShow->pdb, TRUE, NULL);
                    else
                        ASSERT(0);
                }
            }
            break;
#endif // }
        }
    }
    else if (IsEqualIID(*pguidCmdGroup, CGID_Explorer)) {
        return IUnknown_Exec(_ptbActive, pguidCmdGroup, nCmdID, nCmdexecopt,
                pvarargIn, pvarargOut);
    }
    else if (IsEqualIID(*pguidCmdGroup, CGID_DeskBarClient)) {
        switch (nCmdID) {
        case DBCID_ONDRAG:
            if (EVAL(pvarargIn->vt == VT_I4)) {
                ASSERT(pvarargIn->lVal == 0 || pvarargIn->lVal == DRAG_MOVE);
                TraceMsg(DM_TRACE, "cbs.e: DBCID_ONDRAG i=%d", pvarargIn->lVal);
                _fDragging = pvarargIn->lVal;
            }
            break;
        }
    }


    // if we got here, we didn't handle it
    // see if we should forward it down
    hresTmp = IsExecForward(pguidCmdGroup, nCmdID);
    if (SUCCEEDED(hresTmp) && HRESULT_CODE(hresTmp) > 0) {
        // down (singleton or broadcast)
        if (HRESULT_CODE(hresTmp) == OCTD_DOWN) {
            // down (singleton)

            hres = IUnknown_Exec(_ptbActive, pguidCmdGroup, nCmdID, nCmdexecopt,
                pvarargIn, pvarargOut);
        }
        else {
            // down (broadcast)
            // n.b. hres is a bit weird: 'last one wins'
            // BUGBUG should we just return S_OK?
            ASSERT(HRESULT_CODE(hresTmp) == OCTD_DOWNBROADCAST);

            EXECDATA ctd = { hres, pguidCmdGroup, nCmdID, nCmdexecopt,
                pvarargIn, pvarargOut };

            _BandItemEnumCallback(1, _ExecCallback, &ctd);
            hres = ctd.hres;
        }
    }

Lret:
    return hres;
}

/***    _ShowBand -- show/hide band (cached state, band, and rebar band)
 */
void CBandSite::_ShowBand(LPBANDITEMDATA pbid, BOOL fShow)
{
    int i;

    pbid->fShow = BOOLIFY(fShow);
    pbid->pdb->ShowDW(fShow && (_dwShowState == DBC_SHOW));

    i = _BandIDToIndex(pbid->dwBandID);
    SendMessage(_hwnd, RB_SHOWBAND, i, fShow);

    // get me a window to draw D&D curosors on. . .
    SHGetTopBrowserWindow(SAFECAST(this, IBandSite*), &_hwndDD);
}


/*----------------------------------------------------------
Purpose: IBandSite::GetBandSiteInfo

*/
HRESULT CBandSite::GetBandSiteInfo(BANDSITEINFO * pbsinfo)
{
    ASSERT(IS_VALID_WRITE_PTR(pbsinfo, BANDSITEINFO));

    if (pbsinfo->dwMask & BSIM_STATE)
        pbsinfo->dwState = _dwMode;

    if (pbsinfo->dwMask & BSIM_STYLE)
        pbsinfo->dwStyle = _dwStyle;

    return S_OK;
}


/*----------------------------------------------------------
Purpose: IBandSite::SetBandSiteInfo

*/
HRESULT CBandSite::SetBandSiteInfo(const BANDSITEINFO * pbsinfo)
{
    ASSERT(IS_VALID_READ_PTR(pbsinfo, BANDSITEINFO));

    if (pbsinfo->dwMask & BSIM_STATE)
        _dwMode = pbsinfo->dwState;

    if (pbsinfo->dwMask & BSIM_STYLE)
    {
        // If the BSIS_SINGLECLICK style changed, change the rebar style
        if ( _hwnd && ((_dwStyle ^ pbsinfo->dwStyle) & BSIS_SINGLECLICK) )
            SHSetWindowBits(_hwnd, GWL_STYLE, RBS_DBLCLKTOGGLE, (pbsinfo->dwStyle & BSIS_SINGLECLICK)?0:RBS_DBLCLKTOGGLE);
            
        _dwStyle = pbsinfo->dwStyle;
    }

    return S_OK;
}


/*----------------------------------------------------------
Purpose: IBandSite::GetBandObject

*/
HRESULT CBandSite::GetBandObject(DWORD dwBandID, REFIID riid, void **ppvObj)
{
    HRESULT hres = E_FAIL;
    
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IDataObject)) 
    {
        *ppvObj = _DataObjForBand(dwBandID);
        if (*ppvObj)
            hres = S_OK;
    }
    else 
    {
        LPBANDITEMDATA pbid = _GetBandItemDataStructByID(dwBandID);
        if (pbid)
        {
            hres = pbid->pdb->QueryInterface(riid, ppvObj);
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: Returns a pointer to the band item data given an
         externally known band ID.

Returns: NULL if band ID is illegal
*/
LPBANDITEMDATA CBandSite::_GetBandItemDataStructByID(DWORD uID)
{
    int iBand = _BandIDToIndex(uID);
    if (iBand == -1)
        return NULL;
    return _GetBandItem(iBand);
}


__inline HRESULT _FwdWinEvent(IWinEventHandler* pweh, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    ASSERT(pweh);
    ASSERT(hwnd == HWND_BROADCAST || pweh->IsWindowOwner(hwnd) == S_OK);

    return pweh->OnWinEvent(hwnd, uMsg, wParam, lParam, plres);
}

/*----------------------------------------------------------
Purpose: Forwards messages to the band that owns the window.

Returns: TRUE if the message was forwarded

*/
BOOL CBandSite::_SendToToolband(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    BOOL fSent = FALSE;
    LRESULT lres = 0;

    if (hwnd)
    {
        if (hwnd == _hwndCache)
        {
            ASSERT(hwnd != HWND_BROADCAST);

            if (_pwehCache)
            {
                _FwdWinEvent(_pwehCache, hwnd, uMsg, wParam, lParam, &lres);
                fSent = TRUE;
            }
        }
        else
        {
            LPBANDITEMDATA pbid;
            int i;

            for (i = _GetBandItemCount() - 1; i >= 0; i--)
            {
                pbid = _GetBandItem(i);
                if (pbid)
                {
                    if (pbid->pweh)
                    {
                        if (hwnd == HWND_BROADCAST || 
                          (pbid->pweh->IsWindowOwner(hwnd) == S_OK))
                        {
                            _FwdWinEvent(pbid->pweh, hwnd, uMsg, wParam, lParam, &lres);
                            fSent = TRUE;

                            if (hwnd != HWND_BROADCAST)
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (hwnd == HWND_BROADCAST && pbid->hwnd)
                        {
                            lres = SendMessage(pbid->hwnd, uMsg, wParam, lParam);
                            fSent = TRUE;
                        }
                    }
                }
            }

            if (hwnd != HWND_BROADCAST)
            {
                ATOMICRELEASE(_pwehCache);
                _hwndCache = hwnd;
                if (fSent)
                {
                    _pwehCache = pbid->pweh;
                    _pwehCache->AddRef();
                }
            }
        }
    }

    if (plres)
        *plres = lres;
    
    return fSent;
}

typedef struct {
    HWND hwnd;
    HRESULT hres;
} WINDOWOWNERDATA;

int _IsWindowOwnerCallback(LPBANDITEMDATA pbid, void *pv)
{
    WINDOWOWNERDATA* pwod = (WINDOWOWNERDATA*)pv;
    
    if (pbid->pweh && (pbid->pweh->IsWindowOwner(pwod->hwnd) == S_OK)) 
    {
        pwod->hres = S_OK;
        return 0;
    }
    return 1;
}

HRESULT CBandSite::IsWindowOwner(HWND hwnd)
{
    if (hwnd == _hwnd)
        return S_OK;
    
    WINDOWOWNERDATA wod = { hwnd, S_FALSE };
    _BandItemEnumCallback(1, _IsWindowOwnerCallback, &wod);
    return wod.hres;
}

//***   CBandSite::IDeskBarClient::* {
HRESULT CBandSite::GetSize(DWORD dwWhich, LPRECT prc)
{
    HRESULT hres = E_FAIL;
    switch (dwWhich) {
    case DBC_GS_IDEAL:
        prc->right = 0;
        prc->bottom = 0;

        BOOL_PTR fRedraw = SendMessage(_hwnd, WM_SETREDRAW, FALSE, 0);
        for (int i = _GetBandItemCount() - 1; i >= 0; i--)
        {
            LPBANDITEMDATA pbid = _GetBandItem(i);
            if (pbid)
            {
                RECT rc;
            
                SendMessage(_hwnd, RB_GETBANDBORDERS, _BandIDToIndex(pbid->dwBandID), (LPARAM) &rc);
                _UpdateBandInfo(pbid, FALSE);

                if (pbid->fShow) {
                    if (_dwMode & (DBIF_VIEWMODE_FLOATING | DBIF_VIEWMODE_VERTICAL)) {

                        prc->right = max(prc->right, pbid->ptActual.x + (rc.left + rc.right));
                        prc->bottom += pbid->ptActual.y + rc.top + rc.bottom;
                    } else {
                        prc->bottom = max(prc->right, pbid->ptActual.x + (rc.left + rc.right));
                        prc->right += pbid->ptActual.y + rc.top + rc.bottom;
                    }
                }
                hres = S_OK;
            }
        }
        SendMessage(_hwnd, WM_SETREDRAW, fRedraw, 0);
        
        break;
    }
    return hres;
    
}


void CBandSite::_Close() 
{        
    if (_hwnd) {
        // BUGBUG (scotth): This method is getting called by the destructor,
        //  and calls _DeleteAllBandItems, which sends messages to _hwnd.
        //  _hwnd is already destroyed by this time.  If you hit this assert
        //  it is because in debug windows it RIPs like crazy.
        // 970508 (adp): pblm was that we weren't doing DestroyWnd etc.
        //  
        //  Do no remove this assert....please fix the root problem.
        ASSERT(IS_VALID_HANDLE(_hwnd, WND));
        SendMessage(_hwnd, WM_SETREDRAW, 0, 0);
        _DeleteAllBandItems(); 

        DestroyWindow(_hwnd);
        _hwnd = 0;
    }
}

    
HRESULT CBandSite::UIActivateDBC(DWORD dwState)
{
    if (dwState != _dwShowState) {
        BOOL fShow = dwState;

        _dwShowState = dwState;
        // map UIActivateDBC to ShowDW
        if (DBC_SHOWOBSCURE == dwState)
            fShow = FALSE;

        BOOL_PTR fRedraw = SendMessage(_hwnd, WM_SETREDRAW, FALSE, 0);
        for (int i = _GetBandItemCount() - 1; i >= 0; i--)
        {
            LPBANDITEMDATA pbid = _GetBandItem(i);
            if (pbid)
                pbid->pdb->ShowDW(fShow && pbid->fShow);
        }

        // do this now intead of at creation so that 
        // rebar doesn't keep trying to autosize us while
        // we're not even visible
        SHSetWindowBits(_hwnd, GWL_STYLE, RBS_AUTOSIZE, RBS_AUTOSIZE);
        SendMessage(_hwnd, WM_SIZE, 0, 0);
        SendMessage(_hwnd, WM_SETREDRAW, (DBC_SHOW == dwState) ? TRUE : fRedraw, 0);
    }
    return S_OK;
}

DWORD CBandSite::_GetWindowStyle(DWORD* pdwExStyle)
{
    *pdwExStyle = WS_EX_TOOLWINDOW;
    DWORD dwStyle = RBS_REGISTERDROP | RBS_VARHEIGHT | RBS_BANDBORDERS |
                    WS_VISIBLE |  WS_CHILD | WS_CLIPCHILDREN |
                    WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN;
    if (_dwStyle & BSIS_LEFTALIGN) {
        dwStyle |= RBS_VERTICALGRIPPER;
    }

    if (!(_dwStyle & BSIS_SINGLECLICK)) {
        dwStyle |= RBS_DBLCLKTOGGLE;
    }

    return dwStyle;
}

HRESULT CBandSite::_Initialize(HWND hwndParent)
{
    //
    //  I hope we have an IBandSite to talk to.
    //
    if (!_pbsOuter)
        return E_FAIL;

    if (!_hwnd) 
    {
        DWORD dwExStyle;
        DWORD dwStyle = _GetWindowStyle(&dwExStyle);

        _hwnd = CreateWindowEx(dwExStyle, REBARCLASSNAME, NULL, dwStyle,
                               0, 0, 0, 0, hwndParent, (HMENU) FCIDM_REBAR, HINST_THISDLL, NULL);

        if (_hwnd)
        {
            SendMessage(_hwnd, RB_SETTEXTCOLOR, 0, CLR_DEFAULT);
            SendMessage(_hwnd, RB_SETBKCOLOR, 0, CLR_DEFAULT);
            SendMessage(_hwnd, WM_SETREDRAW, FALSE, 0);
        }
    }
    
    return _hwnd ? S_OK : E_OUTOFMEMORY;
}

HRESULT CBandSite::SetDeskBarSite(IUnknown* punkSite)
{
    HRESULT hr = S_OK;

    if (!punkSite)
    {
        // Time to tell the bands to free their
        // back pointers on us or we never get freed...

        // 970325 for now bs::SetDeskBarSite(NULL) is 'overloaded'
        // to mean do both a CloseDW and a SetSite.
        // when we clean up our act and have a bs::Close iface
        // we'll go back to the '#else' code below.
        if (_hwnd)
            _Close();
    }

    ATOMICRELEASE(_pct);
    ATOMICRELEASE(_pdb);
    ATOMICRELEASE(_punkSite);

    if (_pbp && _fCreatedBandProxy)
        _pbp->SetSite(punkSite);

    if (punkSite)
    {
        _punkSite = punkSite;
        _punkSite->AddRef();

        if (!_hwnd) 
        {
            HWND hwndParent;
            IUnknown_GetWindow(punkSite, &hwndParent);
            hr = _Initialize(hwndParent);
        }

        punkSite->QueryInterface(IID_IOleCommandTarget, (void **)&_pct);
        punkSite->QueryInterface(IID_IDeskBar, (void **)&_pdb);
    }
    
    return hr;
}

HRESULT CBandSite::SetModeDBC(DWORD dwMode)
{
    if (dwMode != _dwMode) {
        _dwMode = dwMode;

        if (_hwnd) {
            DWORD dwStyle = 0;
            if (dwMode & (DBIF_VIEWMODE_FLOATING | DBIF_VIEWMODE_VERTICAL)) {
                dwStyle |= CCS_VERT;
            }
            SHSetWindowBits(_hwnd, GWL_STYLE, CCS_VERT, dwStyle);
        }

        _UpdateAllBands(FALSE, FALSE);
    }
    return S_OK;
}

// }

IDropTarget* CBandSite::_WrapDropTargetForBand(IDropTarget* pdtBand)
{
    if (!pdtBand || (_dwStyle & BSIS_NODROPTARGET)) {
        // addref it for the new pointer
        if (pdtBand)
            pdtBand->AddRef();
        return pdtBand;
    } else {
        return DropTargetWrap_CreateInstance(pdtBand, SAFECAST(this, IDropTarget*), _hwndDD);
    }
}

LRESULT CBandSite::_OnNotify(LPNMHDR pnm)
{
    NMOBJECTNOTIFY *pnmon = (NMOBJECTNOTIFY *)pnm;
    
    switch (pnm->code) {
    case RBN_GETOBJECT:
    {
        pnmon->hResult = E_FAIL;
        
        // if we're the drag source, then a band is dragging... we want to only
        // give out the bandsite's drop target
        if (pnmon->iItem != -1 && !_fDragSource) 
        {
            LPBANDITEMDATA pbid = _GetBandItemDataStructByID(pnmon->iItem);
            if (EVAL(pbid))
            {
                pnmon->hResult = pbid->pdb->QueryInterface(*pnmon->piid, &pnmon->pObject);

                // give a wrapped droptarget instead of the band's droptarget
                if (IsEqualIID(*pnmon->piid, IID_IDropTarget))
                {
                    IDropTarget* pdtBand;
                    BOOL fNeedReleasePdtBand = FALSE;

                    if (SUCCEEDED(pnmon->hResult)) {
                        pdtBand = (IDropTarget*)(pnmon->pObject);
                    } else {
                        CDropDummy *pdtgt = new CDropDummy(_hwndDD);
                        pdtBand = SAFECAST(pdtgt, IDropTarget*);
                        fNeedReleasePdtBand = TRUE;
                    }

                    IDropTarget* pdt = _WrapDropTargetForBand(pdtBand);
                    if (pdt)
                    {
                        pnmon->pObject = pdt;
                        pnmon->hResult = S_OK;

                        // we've handed off pdtBand to pdt
                        fNeedReleasePdtBand = TRUE;
                    }

                    if (fNeedReleasePdtBand && pdtBand)
                        pdtBand->Release();
                }

                if (FAILED(pnmon->hResult) && !(_dwStyle & BSIS_NODROPTARGET)) 
                    pnmon->hResult = QueryInterface(*pnmon->piid, &pnmon->pObject);
            }
        } 
        break;
    }

    case RBN_BEGINDRAG:
        return _OnBeginDrag((NMREBAR*)pnm);

    case RBN_AUTOSIZE:
        _OnRBAutoSize((NMRBAUTOSIZE*)pnm);
        break;

    case RBN_CHEVRONPUSHED:
    {
        LPNMREBARCHEVRON pnmch = (LPNMREBARCHEVRON) pnm;
        LPBANDITEMDATA pbid = _GetBandItem(pnmch->uBand);
        if (EVAL(pbid)) {
            MapWindowPoints(_hwnd, HWND_DESKTOP, (LPPOINT)&pnmch->rc, 2);
            ToolbarMenu_Popup(_hwnd, &pnmch->rc, pbid->pdb, pbid->hwnd, 0, (DWORD)pnmch->lParamNM);
        }
        break;
    }
    }

    return 0;
}

void CBandSite::_OnRBAutoSize(NMRBAUTOSIZE* pnm)
{
    // DRAG_MOVE: we turn off autosize during (most of) a move because
    // fVertical is out of sync until the very end
    if (_pdb && _GetBandItemCount() && _fDragging != DRAG_MOVE) {
        RECT rc;
        int iHeightCur;
        int iHeight = (int)SendMessage(_hwnd, RB_GETBARHEIGHT, 0, 0);

#ifdef DEBUG
        DWORD dwStyle = GetWindowLong(_hwnd, GWL_STYLE);
#endif

        GetWindowRect(_hwnd, &rc);
        MapWindowRect(HWND_DESKTOP, GetParent(_hwnd), &rc);

        if (_dwMode & (DBIF_VIEWMODE_FLOATING | DBIF_VIEWMODE_VERTICAL))
        {
            ASSERT((dwStyle & CCS_VERT));
            iHeightCur = RECTWIDTH(rc);
            rc.right = rc.left + iHeight;
        }
        else
        {
            ASSERT(!(dwStyle & CCS_VERT));
            iHeightCur = RECTHEIGHT(rc);
            rc.bottom = rc.top + iHeight;
        }

        if (iHeightCur != iHeight)
            _pdb->OnPosRectChangeDB(&rc);
    }
}

IDataObject* CBandSite::_DataObjForBand(DWORD dwBandID)
{
    IDataObject* pdtobjReturn = NULL;

    LPBANDITEMDATA pbid = _GetBandItemDataStructByID(dwBandID);
    if (EVAL(pbid) && pbid->pdb)
    {
        HRESULT hres;
        
        CBandDataObject* pdtobj = new CBandDataObject();
        if (pdtobj) {
            hres = pdtobj->Init(pbid->pdb, this, dwBandID);

            if (SUCCEEDED(hres)) {
                pdtobjReturn = pdtobj;
                pdtobjReturn->AddRef();
            }
            
            pdtobj->Release();
        }
    }
    return pdtobjReturn;
}

LRESULT CBandSite::_OnBeginDrag(NMREBAR* pnm)
{
    LRESULT lres = 0;
    DWORD dwBandID = _IndexToBandID(pnm->uBand);

    IDataObject* pdtobj = _DataObjForBand(dwBandID);

    ATOMICRELEASE(_pdtobj);

    _uDragBand = pnm->uBand;
    _pdtobj = pdtobj;
    // because the RBN_BEGINDRAG is synchronous and so is SHDoDragDrop
    // post this message to ourselves instead of calling dragdrop directly.
    // note that we don't have a window of our own, so we post to our parent
    // and let the message reflector send it back to us
    PostMessage(GetParent(_hwnd), WM_COMMAND, MAKELONG(0, IDM_DRAGDROP), (LPARAM)_hwnd);
    return 1;
}

// return TRUE if the user drags out of the rect of the rebar meaning that we should
// go into ole drag drop.
BOOL CBandSite::_PreDragDrop()
{
    BOOL f = FALSE;
    RECT rc;
    POINT pt;
    DWORD dwBandID = _IndexToBandID(_uDragBand);    // Find the BandID before an reordering that may happen.
    
    GetWindowRect(_hwnd, &rc);
    SetCapture(_hwnd);

    InflateRect(&rc, GetSystemMetrics(SM_CXEDGE) * 3, GetSystemMetrics(SM_CYEDGE) * 3);
    while (GetCapture() == _hwnd) {
        MSG msg;
        
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            switch (msg.message) {
                
            case WM_MOUSEMOVE:
                GetCursorPos(&pt);
                if (!ISMOVEDDISABLED(dwBandID))
                {
                    if (PtInRect(&rc, pt))
                    {
                        SendMessage(_hwnd, RB_DRAGMOVE, 0, (LPARAM)-1);
                    } else if (!ISDDCLOSEDISABLED(dwBandID) && _pdtobj)
                    {
                        // we've moved out of the bounds of the rebar..  switch to ole drag
                        f = TRUE;
                        SetCapture(NULL);
                    }
                }
                break;

            case WM_LBUTTONUP:
            case WM_LBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_RBUTTONDOWN:
                // bail on any mouse button action
                SetCapture(NULL);
                break;
                
            case WM_KEYDOWN:
                switch (msg.wParam) {
                case VK_ESCAPE:
                    SetCapture(NULL);
                    break;
                }
                // fall through
                
            default:
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    if (ISDDCLOSEDISABLED(dwBandID) || !_IsBandDeleteable(dwBandID))
    {
        /// if don't allow close, never return true for ole drag.
        f = FALSE;
    }

    return f;
}

void CBandSite::_DoDragDrop()
{
    DWORD dwBandID = _IndexToBandID(_uDragBand);
    DWORD dwEffect = DROPEFFECT_MOVE;

    _fDragSource = TRUE;

    SendMessage(_hwnd, RB_BEGINDRAG, _uDragBand, (LPARAM)-2);

    HRESULT hres = S_OK;

    // first check to see if we even need to go into Ole drag, or if
    // it can all be contained within the rebar
    if (_PreDragDrop()) {
        SHLoadOLE(SHELLNOTIFY_OLELOADED); // Browser Only - our shell32 doesn't know ole has been loaded
        hres = SHDoDragDrop(_hwnd, _pdtobj, NULL, dwEffect, &dwEffect);
    } else {
        // if we kept it all within win32 dragging, then set no drop effect
        dwEffect = DROPEFFECT_NONE;
    }

    SendMessage(_hwnd, RB_ENDDRAG, 0, 0);
    _fDragSource = FALSE;
    if (dwEffect & DROPEFFECT_MOVE) 
    {
        RemoveBand(dwBandID);
    } 
    else if (!dwEffect && hres == DRAGDROP_S_DROP) 
    {
        // if the drop was done, but the target didn't allow
        // then we float the band.
    }

    ATOMICRELEASE(_pdtobj);
}

HMENU CBandSite::_LoadContextMenu()
{
    return LoadMenuPopup_PrivateNoMungeW(MENU_BANDSITE1);
}

HRESULT CBandSite::_OnBSCommand(int idCmd, DWORD idBandActive, LPBANDITEMDATA pbid)
{
    HRESULT hr = S_OK;

    switch (idCmd) {
    case BSIDM_CLOSEBAND:
        _OnCloseBand(idBandActive);
        break;

    case BSIDM_SHOWTITLEBAND:
        ASSERT(idBandActive != (DWORD)-1 && pbid);
        if (pbid) {
            pbid->fNoTitle = !pbid->fNoTitle;
            _UpdateBandInfo(pbid, FALSE);
        }
        break;

    case BSIDM_IEAK_DISABLE_MOVE:
    case BSIDM_IEAK_DISABLE_DDCLOSE:
        ASSERT(idBandActive != (DWORD)-1);
        if (idBandActive != (DWORD)-1) {
            static const int idCmds[]  = { BSIDM_IEAK_DISABLE_MOVE,   BSIDM_IEAK_DISABLE_DDCLOSE  };
            static const int idFlags[] = { BAND_ADMIN_NOMOVE,         BAND_ADMIN_NODDCLOSE        };

            DWORD dwFlag = SHSearchMapInt(idCmds, idFlags, ARRAYSIZE(idCmds), idCmd);
            DWORD dwAdminSettings = _GetAdminSettings(idBandActive);

            // Toggle Setting.
            ToggleFlag(dwAdminSettings, dwFlag);

            // Set Menu Item Check Mark appropriately.
            _SetAdminSettings(idBandActive, dwAdminSettings);
        }
        break;

    default:
        ASSERT(0);
        hr = E_FAIL;
        break;
    }

    return hr;
}

// returns the index of the band hit by lParam using context menu semantics (lParam == -1 for keyboard)
int CBandSite::_ContextMenuHittest(LPARAM lParam, POINT* ppt)
{
    int iBandIndex;

    if (lParam == (LPARAM) -1)
    {
        // Keyboard activation.  Use active band.
        DWORD dwBandID = _BandIDFromPunk(_ptbActive);
        iBandIndex = _BandIDToIndex(dwBandID);

        LPBANDITEMDATA pbid = _GetBandItem(iBandIndex);
        if (pbid)
        {
            RECT rc;
            GetWindowRect(pbid->hwnd, &rc);
            ppt->x = rc.left;
            ppt->y = rc.top;
        }
    }
    else
    {
        // Mouse activation.  Figure out which band got clicked.
        RBHITTESTINFO rbht;

        ppt->x = GET_X_LPARAM(lParam);
        ppt->y = GET_Y_LPARAM(lParam);
        rbht.pt = *ppt;
        ScreenToClient(_hwnd, &rbht.pt);
        SendMessage(_hwnd, RB_HITTEST, 0, (LPARAM)&rbht);
        iBandIndex = rbht.iBand;
    }

    return iBandIndex;
}

HRESULT CBandSite::_OnContextMenu(WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = S_OK;
    HMENU hmenu = CreatePopupMenu();

    if (hmenu)
    {
        HRESULT hresT;
        int idCmd = 1;
        IContextMenu *pcm, *pcmParent = NULL, *pcmChild = NULL;

        POINT pt;
        int iBandIndex = _ContextMenuHittest(lParam, &pt);

        // map rebar index to band id
        // get band info for that band id
        DWORD idBandActive = _IndexToBandID(iBandIndex);
        LPBANDITEMDATA pbid = _GetBandItemDataStructByID(idBandActive);

        //
        // self (top)
        //
        int idCmdBS1 = idCmd;

        HMENU hmenuMe = _LoadContextMenu();
        if (hmenuMe) {
            if (pbid) {
                DESKBANDINFO dbi;

                CheckMenuItem(hmenuMe, BSIDM_SHOWTITLEBAND,
                    pbid->fNoTitle ? MF_BYCOMMAND|MF_UNCHECKED : MF_BYCOMMAND|MF_CHECKED);
                dbi.dwMask = 0;     // paranoia (and needed for taskband!)
                _GetBandInfo(pbid, &dbi);
                // make sure pbid in sync
                ASSERT((dbi.dwMask & DBIM_TITLE) || pbid->fNoTitle);
                if (!(dbi.dwMask & DBIM_TITLE) || !_IsEnableTitle(pbid)) {
        Lnotitle:
                    DeleteMenu(hmenuMe, BSIDM_SHOWTITLEBAND, MF_BYCOMMAND);
                }
            }
            else {
                goto Lnotitle;
            }

            idCmd += Shell_MergeMenus(hmenu, hmenuMe, 0, idCmd, 0x7fff, MM_ADDSEPARATOR) - (idCmd);
            DestroyMenu(hmenuMe);
        }

        //
        // child
        //
        int idCmdChild = idCmd;

        if (pbid && pbid->pdb) {
            // merge in band's menu (at front)
            hresT = pbid->pdb->QueryInterface(IID_IContextMenu, (void **)&pcmChild);
            if (SUCCEEDED(hresT)) {
                // 0=at front
                hresT = pcmChild->QueryContextMenu(hmenu, 0, idCmd, 0x7fff, 0);
                ASSERT(SUCCEEDED(hresT));
                idCmd += HRESULT_CODE(hresT);
            }
        }

        //
        // parent
        //
        int idCmdParent = idCmd;
        
        if (_punkSite) {
            
            _punkSite->QueryInterface(IID_IContextMenu, (void **)&pcmParent);
            if (pcmParent) {
                // BUGBUG todo: fix parents and kids to handle...
                // we'd like to pass in -1 but not everyone handles that.
                // workaround: use _FixMenuIndex...
                hresT = pcmParent->QueryContextMenu(hmenu, _FixMenuIndex(hmenu, (UINT)-1), idCmd, 0x7fff, 0);
                ASSERT(SUCCEEDED(hresT));
                idCmd += HRESULT_CODE(hresT);
            }
        }
        
        //
        // self (bottom)
        //
        int idCmdBS2 = idCmd;

        if (!(_dwStyle & BSIS_NOCONTEXTMENU))
        {
            hmenuMe = LoadMenuPopup_PrivateNoMungeW(MENU_BANDSITE2);
            if (hmenuMe) {
                // disable 'Close Band' if it's marked undeleteable
                // nash:17821: don't disable when 0 bands (so user can easily
                // get out of toasted mode)
                if ((idBandActive == (DWORD)-1) || // if mouse not over a band, delete close menu item
                    (!_IsBandDeleteable(idBandActive) ||
                     ISDDCLOSEDISABLED(idBandActive)))
                {
                    DeleteMenu(hmenuMe, BSIDM_CLOSEBAND, MF_BYCOMMAND);
                }

                if (!_fIEAKInstalled) {
                    DeleteMenu(hmenuMe, BSIDM_IEAK_DISABLE_DDCLOSE, MF_BYCOMMAND);
                    DeleteMenu(hmenuMe, BSIDM_IEAK_DISABLE_MOVE, MF_BYCOMMAND);
                }
                else {
                    DWORD dwAdminSettings = _GetAdminSettings(idBandActive);

                    if (IsFlagSet(dwAdminSettings, BAND_ADMIN_NODDCLOSE))
                        _CheckMenuItem(hmenuMe, BSIDM_IEAK_DISABLE_DDCLOSE, TRUE);

                    if (IsFlagSet(dwAdminSettings, BAND_ADMIN_NOMOVE))
                        _CheckMenuItem(hmenuMe, BSIDM_IEAK_DISABLE_MOVE, TRUE);
                }

                idCmd += Shell_MergeMenus(hmenu, hmenuMe, (UINT) -1, idCmd, 0x7fff, MM_ADDSEPARATOR) - (idCmd);
                DestroyMenu(hmenuMe);
            }
        }

        //
        // do it
        //
        {
            HWND hwndParent = GetParent(_hwnd);
            if (!hwndParent)
                hwndParent = _hwnd;
            idCmd = TrackPopupMenu(hmenu,
                            TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                                   pt.x, pt.y, 0, hwndParent, NULL);
        }

        if (idCmd) {
            // must test from smallest to largest
            ASSERT(idCmdBS1 <= idCmdChild);
            ASSERT(idCmdChild <= idCmdParent);    // o.w. test in wrong order
            ASSERT(idCmdParent <= idCmdBS2);

            if (idCmd < idCmdChild || idCmd >= idCmdBS2)
            {
                // A bandsite command
                if (idCmd < idCmdChild)
                    idCmd -= idCmdBS1;
                else
                    idCmd -= idCmdBS2;

                hres = _OnBSCommand(idCmd, idBandActive, pbid);
            }
            else
            {
                // A parent or child command
                if (idCmd < idCmdParent) {
                    pcm = pcmChild;
                    idCmd -= idCmdChild;
                } else {
                    pcm = pcmParent;
                    idCmd -= idCmdParent;
                }

                ASSERT(pcm);
                BANDSITEINVOKEPARAM bsip = { idBandActive, _GetOuter() };
            
                //
                // Call InvokeCommand
                //
                CMINVOKECOMMANDINFOEX ici = {
                    SIZEOF(CMINVOKECOMMANDINFOEX),
                    0L,
                    _hwnd,
                    (LPSTR)MAKEINTRESOURCE(idCmd),
                    (LPSTR) &bsip, NULL,
                    SW_NORMAL,
                };

                hres = pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
            }
        }

        if (pcmParent)
            pcmParent->Release();
        if (pcmChild)
            pcmChild->Release();
        
        DestroyMenu(hmenu);
    }
    
    return hres;
}


/*----------------------------------------------------------
Purpose: IWinEventHandler::OnWinEvent

         Processes messages passed on from the bar.  Forward
         messages to the bands as appropriate.

*/
HRESULT CBandSite::OnWinEvent(HWND h, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    HRESULT hres = E_FAIL;
    HWND hwnd = HWND_BROADCAST;
    
    switch (uMsg) {
    case WM_WININICHANGE:
        _UpdateAllBands(FALSE, FALSE);
        // fall through

    case WM_SYSCOLORCHANGE:
    case WM_PALETTECHANGED:
        // propagate to rebar
        if (_hwnd)
            SendMessage(_hwnd, uMsg, wParam, lParam);

#if 0 // COM side propagates, don't double-propagate to win32 side!
        // propagate it to child windows incase they handle it,
        PropagateMessage(_hwnd, uMsg, wParam, lParam, TRUE);
#endif

        // by not returning here, it will get forwarded to the bands also... 
        break;
        
    case WM_CONTEXTMENU:
        // if it came from the keyboard, wParam is somewhat useless.  it's always out hwnd
        if (IS_WM_CONTEXTMENU_KEYBOARD(lParam))
            hwnd = GetFocus();
        else
            hwnd = (HWND)wParam;
        break;

    case WM_COMMAND:
        hwnd = GET_WM_COMMAND_HWND(wParam, lParam);
        break;
            
    case WM_NOTIFY:
        hwnd = ((LPNMHDR)lParam)->hwndFrom;
        break;
        
    default:
        return E_FAIL;
    }
    
    LRESULT lres = 0;
    
    if (hwnd) {

        if(_hwnd == hwnd) {

            // a message for us
            switch (uMsg) {
            case WM_NOTIFY:
                lres = _OnNotify((LPNMHDR)lParam);
                hres = S_OK;
                break;

            case WM_COMMAND:
                switch (GET_WM_COMMAND_CMD(wParam, lParam)) {
                case IDM_DRAGDROP:
                    _DoDragDrop();
                    break;
                }
                break;
            }
            
        } else {
            if (_SendToToolband(hwnd, uMsg, wParam, lParam, &lres))
                hres = S_OK;
        }
    }
    
    
    switch (uMsg) {
    case WM_WININICHANGE:
        SendMessage(_hwnd, WM_SIZE, 0, 0);
        break;

    case WM_CONTEXTMENU:
        if (!lres)
            return _OnContextMenu(wParam, lParam);
        break;
    }

    if (plres)
        *plres = lres;
    
    return hres;
}

HRESULT CBandSite_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CBandSite *pbs = new CBandSite(pUnkOuter);
    if (pbs)
    {
        *ppunk = pbs->_GetInner();
        return S_OK;
    }
    *ppunk = NULL;
    return E_OUTOFMEMORY;
}

#ifdef DEBUG
void DbPrBandInfo(REBARBANDINFO* prbbi)
{
    TraceMsg(DM_PERSIST,
        "fMask=%x fStyle=%x clrFore=%x clrBack=%x",
        prbbi->fMask, prbbi->fStyle, prbbi->clrFore, prbbi->clrBack);
    TraceMsg(DM_PERSIST, "lpText=%s", prbbi->lpText ? prbbi->lpText : TEXT("<nil>"));
    TraceMsg(DM_PERSIST,
        "cch=%d iImage=%d hwndChild=%x",
        prbbi->cch, prbbi->iImage, prbbi->hwndChild);
    TraceMsg(DM_PERSIST,
        "cxMinChild=%d cyMinChild=%d cx=%d hbmBack=%x",
        prbbi->cxMinChild, prbbi->cyMinChild, prbbi->cx, prbbi->hbmBack);
    TraceMsg(DM_PERSIST,
        "wID=%d cyMaxChild=%d cyIntegral=%d cyChild=%d",
        prbbi->wID, prbbi->cyMaxChild, prbbi->cyIntegral, prbbi->cyChild);
    return;
}
#else

#define DbPrBandInfo(prbbi) 0

#endif

//*** CBandSite::IPersistStream*::* {
//

HRESULT CBandSite::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_RebarBandSite;
    return S_OK;
}

HRESULT CBandSite::IsDirty(void)
{
    ASSERT(0);
    return S_FALSE; // BUGBUG: never be dirty?
}

HRESULT CBandSite::_AddBand(IUnknown* punk)
{
    if (_pbsOuter)
    {
        // Give the outer guy first crack
        return _pbsOuter->AddBand(punk);
    }
    else
    {
        return AddBand(punk);
    }
}

//
// Persisted CBandSite, use types that have fixes sizes
//
struct SBandSite
{
    DWORD   cbSize;
    DWORD   cbVersion;
    DWORD   cBands;
    // ...followed by length-preceded bands
};

#define SBS_WOADMIN_VERSION 3   // Before we added admin settings.
#define SBS_VERSION 8

//***   CBandSite::Load, Save -- 
// DESCRIPTION
//  for each band...
//  Load            Read (i); OLFS(obj)+AddBand; Read (rbbi); RB_SBI
//  Save    RB_GBI; Write(i); OSTS(obj)+nil    ; Write(rbbi)
// NOTES
//  BUGBUG needs error recovery
//  BUGBUG we might have done a CreateBand w/o an AddBand; if so our
//  assumption about the rebar bands and the iunknowns being 'parallel'
//  is bogus.

HRESULT CBandSite::Load(IStream *pstm)
{
    HRESULT hres;
    SBandSite sfoo;

    hres = IStream_Read(pstm, &sfoo, SIZEOF(sfoo));     // pstm->Read
    if (hres == S_OK)
    {
        if (!(sfoo.cbSize == SIZEOF(SBandSite) &&
          (sfoo.cbVersion == SBS_VERSION || sfoo.cbVersion == SBS_WOADMIN_VERSION)))
        {
            hres = E_FAIL;
        }

        IBandSiteHelper *pbsh;
        hres = QueryInterface(IID_IBandSiteHelper, (void **)&pbsh);
        if (EVAL(SUCCEEDED(hres)))
        {
            BOOL_PTR fRedraw = SendMessage(_hwnd, WM_SETREDRAW, FALSE, 0);
            for (DWORD i = 0; i < sfoo.cBands && SUCCEEDED(hres); ++i)
            {
                DWORD j;
                hres = IStream_Read(pstm, &j, SIZEOF(j));   // pstm->Read
                if (hres == S_OK)
                {
                    if (j == i)             // for sanity check
                    {
                        IUnknown* punk;
                        hres = pbsh->LoadFromStreamBS(pstm, IID_IUnknown, (void **)&punk);
                        if (SUCCEEDED(hres))
                        {
                            hres = _AddBand(punk);
                            if (SUCCEEDED(hres))
                            {
                                hres = _LoadBandInfo(pstm, i, sfoo.cbVersion);
                            }
                            punk->Release();
                        }
                    }
                    else
                    {
                        hres = E_FAIL;
                    }
                }
            }
            SendMessage(_hwnd, WM_SETREDRAW, fRedraw, 0);

            pbsh->Release();
        }

        _UpdateAllBands(FALSE, TRUE);     // force refresh
    }

    return hres;
}

HRESULT CBandSite::Save(IStream *pstm, BOOL fClearDirty)
{
    HRESULT hres;
    SBandSite sfoo;
    
    TraceMsg(DM_PERSIST, "cbs.s enter(this=%x pstm=%x) tell()=%x", this, pstm, DbStreamTell(pstm));

    sfoo.cbSize = SIZEOF(SBandSite);
    sfoo.cbVersion = SBS_VERSION;
    sfoo.cBands = _GetBandItemCount();
    TraceMsg(DM_PERSIST, "cdb.s: cbands=%d", sfoo.cBands);

    hres = pstm->Write(&sfoo, SIZEOF(sfoo), NULL);
    if (SUCCEEDED(hres))
    {
        for (DWORD i = 0; i < sfoo.cBands; i++) 
        {
            // BUGBUG todo: put seek ptr so can resync after bogus streams
            hres = pstm->Write(&i, SIZEOF(i), NULL);    // for sanity check
            if (SUCCEEDED(hres))
            {
                LPBANDITEMDATA pbid = _GetBandItem(i);
                if (EVAL(pbid) && pbid->pdb)
                {
                    IBandSiteHelper *pbsh;
                    hres = QueryInterface(IID_IBandSiteHelper, (void **)&pbsh);
                    if (SUCCEEDED(hres)) 
                    {
                        hres = pbsh->SaveToStreamBS(SAFECAST(pbid->pdb, IUnknown*), pstm);
                        pbsh->Release();
                    }
                }

                hres = _SaveBandInfo(pstm, i);
                ASSERT(SUCCEEDED(hres));
            }
        }
    }

    TraceMsg(DM_PERSIST, "cbs.s leave tell()=%x", DbStreamTell(pstm));
    return hres;
}

HRESULT CBandSite::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    static const ULARGE_INTEGER cbMax = { SIZEOF(SBandSite), 0 };
    *pcbSize = cbMax;
    return S_OK;
}

// returns: IStream::Read() semantics, S_OK means complete read

HRESULT CBandSite::_LoadBandInfo(IStream *pstm, int i, DWORD dwVersion)
{
    PERSISTBANDINFO bi;
    HRESULT hres;
    DWORD dwSize = SIZEOF(bi);
    bi.dwAdminSettings = BAND_ADMIN_NORMAL;     // Assume Normal since it's not specified

    if (SBS_WOADMIN_VERSION == dwVersion)
        dwSize = SIZEOF(PERSISTBANDINFO_V3);

    hres = IStream_Read(pstm, &bi, dwSize);     // pstm->Read
    if (hres == S_OK)
    {
        REBARBANDINFO rbbi;
        LPBANDITEMDATA pbid;

        rbbi.cbSize = SIZEOF(rbbi);
        rbbi.fMask = RBBIM_XPERSIST;
        rbbi.cx = bi.cx;
        rbbi.fStyle = bi.fStyle;
        
        // these things can change from instantiation to instantiation.
        // we want to restore the visual state, not the sizing rules.
        // the sizing rules re retreived each time in getbandinfo
        rbbi.cyMinChild = -1;
        rbbi.cyMaxChild = -1;
        rbbi.cyIntegral = -1;
        rbbi.cxMinChild = -1;

        if (rbbi.fStyle & RBBS_VARIABLEHEIGHT) {
            rbbi.cyChild = bi.cyChild;
        } else {
            rbbi.cyMinChild = bi.cyMinChild;
        }

        if (!SendMessage(_hwnd, RB_SETBANDINFO, i, (LPARAM) &rbbi)) 
        {
            TraceMsg(DM_PERSIST, "cbs.l: RB_SETBANDINFO failed");
            ASSERT(0);
            hres = E_FAIL;
        }

        pbid = _GetBandItem(i);
        if (pbid) {
            pbid->dwAdminSettings = bi.dwAdminSettings;
            pbid->fNoTitle = bi.fNoTitle;
        }
    }
    return hres;
}

HRESULT CBandSite::_SaveBandInfo(IStream *pstm, int i)
{
    REBARBANDINFO rbbi;
    PERSISTBANDINFO bi = {0};
    LPBANDITEMDATA pbid;

    rbbi.cbSize = SIZEOF(rbbi);
    rbbi.fMask = RBBIM_XPERSIST;
    SendMessage(_hwnd, RB_GETBANDINFO, i, (LPARAM) &rbbi);

    ASSERT((rbbi.fMask & RBBIM_XPERSIST) == RBBIM_XPERSIST);

    bi.cx = rbbi.cx;
    bi.fStyle = rbbi.fStyle;
    bi.cyMinChild = rbbi.cyMinChild;
    bi.cyChild = rbbi.cyChild;

    pbid = _GetBandItem(i);
    if (pbid) {
        bi.dwAdminSettings = pbid->dwAdminSettings;
        bi.fNoTitle = pbid->fNoTitle;
    }

    return pstm->Write(&bi, SIZEOF(bi), NULL);
}

// }

void CBandSite::_CacheActiveBand(IUnknown *ptb)
{
    if (ptb == _ptbActive)
        return;

    if (SHIsSameObject(ptb, _ptbActive))
        return;

    ATOMICRELEASE(_ptbActive);

    if (ptb != NULL) {
#ifdef DEBUG
        {
        IUnknown *pxTmp;

        // better be an IInputObject or else why did you call us?
        if (EVAL(SUCCEEDED(ptb->QueryInterface(IID_IInputObject, (void **)&pxTmp))))
            pxTmp->Release();

        // overly strict, but in our case it's true...
        if (EVAL(SUCCEEDED(ptb->QueryInterface(IID_IDeskBand, (void **)&pxTmp))))
            pxTmp->Release();
        }
#endif
        _ptbActive = ptb;
        _ptbActive->AddRef();
    }

    return;
}

DWORD CBandSite::_BandIDFromPunk(IUnknown* punk)
{
    DWORD dwBandID = -1;
    DWORD dwBandIDTest;
    int cBands = EnumBands(-1, NULL);
    IUnknown* punkTest;

    if (punk)
    {
        for (int i = 0; i < cBands; i++)
        {
            if (SUCCEEDED(EnumBands(i, &dwBandIDTest)))
            {
                if (SUCCEEDED(GetBandObject(dwBandIDTest, IID_IUnknown, (void**)&punkTest)))
                {
                    BOOL fEq = SHIsSameObject(punk, punkTest);

                    punkTest->Release();

                    if (fEq)
                    {
                        dwBandID = dwBandIDTest;
                        break;
                    }
                }
            }
        }
    }

    return dwBandID;
}

//*** IInputObjectSite methods ***

HRESULT CBandSite::OnFocusChangeIS(IUnknown *punk, BOOL fSetFocus)
{
    if (_ptbActive)
    {
        if (!SHIsSameObject(_ptbActive, punk))
        {
            // Deactivate current band since the current band is 
            // not the caller
            UIActivateIO(FALSE, NULL);
        }
    }

    if (fSetFocus)
        _CacheActiveBand(punk);

    return UnkOnFocusChangeIS(_punkSite, SAFECAST(this, IInputObject*), fSetFocus);
}


//*** IInputObject methods ***

HRESULT CBandSite::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    HRESULT hres = E_FAIL;

    ASSERT(NULL == lpMsg || IS_VALID_WRITE_PTR(lpMsg, MSG));

    if (_ptbActive)
    {
        hres = UnkUIActivateIO(_ptbActive, fActivate, lpMsg);
    }
    else
    {
        hres = OnFocusChangeIS(NULL, fActivate);
    }

    if (fActivate)
    {
        if (!_ptbActive)
        {
            if (IsVK_TABCycler(lpMsg))
                hres = _CycleFocusBS(lpMsg);
            else
                hres = S_OK;
        }
    }
    else
    {
        _CacheActiveBand(NULL);
    }

    return hres;
}

HRESULT CBandSite::HasFocusIO()
{
    // Rebar should never get focus
    // NT #288832 Is one case where (GetFocus() == _hwnd)
    //    which is caused when the "Folder Bar" disappears.
    //    CExplorerBand::ShowDW() calls ShowWindow(hwndTreeView, SW_HIDE)
    //    which by default sets focus to the parent (us).
    //    This is ok because when this function is called,
    //    it will return E_FAIL which the caller will treat
    //    as S_FALSE and give the focus to the next deserving
    //    dude in line.
    return UnkHasFocusIO(_ptbActive);
}

HRESULT CBandSite::TranslateAcceleratorIO(LPMSG lpMsg)
{
    if (UnkTranslateAcceleratorIO(_ptbActive, lpMsg) == S_OK)
    {
        // active band handled it
        return S_OK;
    }
    else if (IsVK_TABCycler(lpMsg))
    {
        // it's a tab; cycle focus
        return _CycleFocusBS(lpMsg);
    }

    return S_FALSE;
}

int CBandSite::_BandIndexFromPunk(IUnknown *punk)
{
    for (int i = 0; i < _GetBandItemCount(); i++)
    {
        LPBANDITEMDATA pbid = _GetBandItem(i);

        if (EVAL(pbid) && SHIsSameObject(pbid->pdb, punk))
            return i;
    }

    return -1;
}

BOOL CBandSite::_IsBandTabstop(LPBANDITEMDATA pbid)
{
    // A band is a tabstop if it is visible and has WS_TABSTOP

    if (pbid->fShow && pbid->hwnd)
    {
        ASSERT(IsWindowVisible(pbid->hwnd));

        if (WS_TABSTOP & GetWindowStyle(pbid->hwnd))
            return TRUE;
    }

    return FALSE;
}

#define INCDEC(i, fDec)   (fDec ? i - 1 : i + 1)

IUnknown* CBandSite::_GetNextTabstopBand(IUnknown* ptb, BOOL fBackwards)
{
    // Find the first tabstop candidate
    int iBandCount = _GetBandItemCount();
    int iBand = _BandIndexFromPunk(ptb);
    
    if (iBand == -1)
    {
        // Start at the end/beginning
        if (fBackwards)
            iBand = iBandCount - 1;
        else
            iBand = 0;
    }
    else
    {
        // Start one off the current band
        iBand = INCDEC(iBand, fBackwards);
    }

    // Loop til we find a tabstop band or we run off the end
    while (0 <= iBand && iBand < iBandCount)
    {
        LPBANDITEMDATA pbid = _GetBandItem(iBand);
        if (EVAL(pbid))
        {
            if (_IsBandTabstop(pbid))
                return pbid->pdb;
        }

        // Try the next band
        iBand = INCDEC(iBand, fBackwards);
    }

    return NULL;
}

HRESULT CBandSite::_CycleFocusBS(LPMSG lpMsg)
{
    HRESULT hr = S_FALSE;

    IUnknown* ptbSave = NULL;

    if (_ptbActive)
    {
        // Save off the active band in ptbSave
        ptbSave = _ptbActive;
        ptbSave->AddRef();

        // Deactivate active band and clear cache
        UnkUIActivateIO(_ptbActive, FALSE, NULL);
        _CacheActiveBand(NULL);
    }

    if (ptbSave && IsVK_CtlTABCycler(lpMsg))
    {
        // If ctl-tab and a band was active, then reject focus
        ASSERT(hr == S_FALSE);
    }
    else
    {
        BOOL fShift = (GetKeyState(VK_SHIFT) < 0);

        // Loop til we find a tabstop and successfully activate it
        // or til we run out of bands.

        // BUGBUG: todo -- call SetFocus if UIActivateIO fails?

        IUnknown* ptbNext = ptbSave;
        while (ptbNext = _GetNextTabstopBand(ptbNext, fShift))
        {
            if (UnkUIActivateIO(ptbNext, TRUE, lpMsg) == S_OK)
            {
                hr = S_OK;
                break;
            }
        }
    }

    ATOMICRELEASE(ptbSave);

    return hr;
}

//*** CBandSite::IBandSiteHelper::* {

// stuff to make it possible to overload the OleLoad/Save stuff so the
// taskbar band does not have to be CoCreat able. kinda a hack...

HRESULT CBandSite::LoadFromStreamBS(IStream *pstm, REFIID riid, void **ppv)
{
    return OleLoadFromStream(pstm, riid, ppv);
}

HRESULT CBandSite::SaveToStreamBS(IUnknown *punk, IStream *pstm)
{
    IPersistStream *ppstm;
    HRESULT hres = punk->QueryInterface(IID_IPersistStream, (void **)&ppstm);
    if (SUCCEEDED(hres)) 
    {
        hres = OleSaveToStream(ppstm, pstm);
        ppstm->Release();
    }
    return hres;
}

// }


// *** IDropTarget *** {

HRESULT CBandSite::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    TraceMsg(TF_BANDDD, "BandSite::DragEnter %d %d", pt.x, pt.y);

    if (!_fDragSource) {
        FORMATETC fmte = {g_cfDeskBand, NULL, 0, -1, TYMED_ISTREAM};
        _dwDropEffect = DROPEFFECT_NONE;
        
        if (pdtobj->QueryGetData(&fmte) == S_OK) {
            _dwDropEffect = DROPEFFECT_MOVE;
        } else {

            LPITEMIDLIST pidl;

            if (SUCCEEDED(SHPidlFromDataObject(pdtobj, &pidl, NULL, 0))) {
                ASSERT(pidl && IS_VALID_PIDL(pidl));

                DWORD dwAttrib = SFGAO_FOLDER | SFGAO_BROWSABLE;
                IEGetAttributesOf(pidl, &dwAttrib);
                ILFree(pidl);

                DWORD   dwRAction;
        
                if (FAILED(IUnknown_HandleIRestrict(_punkSite, &RID_RDeskBars, RA_DROP, NULL, &dwRAction)))
                    dwRAction = RR_ALLOW;

                if (dwRAction == RR_DISALLOW)
                    _dwDropEffect = DROPEFFECT_NONE;
                else {                
                    // if it's not a folder nor a browseable object, we can't host it.
                    if ((dwAttrib & SFGAO_FOLDER) ||
                        (dwAttrib & SFGAO_BROWSABLE) && (grfKeyState & (MK_CONTROL | MK_SHIFT)) == (MK_CONTROL | MK_SHIFT)) 
                        _dwDropEffect = DROPEFFECT_LINK | DROPEFFECT_COPY;

                    _dwDropEffect |= GetPreferedDropEffect(pdtobj);
                }
            }
        }
        *pdwEffect &= _dwDropEffect;
    }
    
    return S_OK;
}

HRESULT CBandSite::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    TraceMsg(TF_BANDDD, "BandSite::DragOver %d %d", ptl.x, ptl.y);
    if (_fDragSource) {
        RECT rc;
        POINT pt;
        pt.x = ptl.x;
        pt.y = ptl.y;
        GetWindowRect(_hwnd, &rc);
        if (PtInRect(&rc, pt))
            SendMessage(_hwnd, RB_DRAGMOVE, 0, (LPARAM)-1);
    } else {
        *pdwEffect &= _dwDropEffect;
    }
    return S_OK;    
}

HRESULT CBandSite::DragLeave(void)
{
    return S_OK;
}

HRESULT CBandSite::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hres = E_FAIL;
    
    TraceMsg(TF_BANDDD, "BandSite::Drop");
    if (_fDragSource)
    {
        SendMessage(_hwnd, RB_ENDDRAG, 0, 0);
        *pdwEffect = DROPEFFECT_NONE;
        hres = S_OK;
    }
    else
    {
        FORMATETC fmte = {g_cfDeskBand, NULL, 0, -1, TYMED_ISTREAM};
        STGMEDIUM stg;
        IUnknown *punk = NULL;
        LPITEMIDLIST pidl;
        
        // if it was an object of our type, create it!
        if ((*pdwEffect & DROPEFFECT_MOVE) &&
            SUCCEEDED(pdtobj->GetData(&fmte, &stg)))
        {

            hres = OleLoadFromStream(stg.pstm, IID_IUnknown, (void **)&punk);
            if (SUCCEEDED(hres))
            {
                *pdwEffect = DROPEFFECT_MOVE;
            }

            ReleaseStgMedium(&stg);
        } 
        else if ((*pdwEffect & (DROPEFFECT_COPY | DROPEFFECT_LINK)) &&
                 SUCCEEDED(SHPidlFromDataObject(pdtobj, &pidl, NULL, 0)))
        {

            hres = SHCreateBandForPidl(pidl, &punk, (grfKeyState & (MK_CONTROL | MK_SHIFT)) == (MK_CONTROL | MK_SHIFT));
            ILFree(pidl);

            if (SUCCEEDED(hres))
            {
                if (*pdwEffect & DROPEFFECT_LINK)
                    *pdwEffect = DROPEFFECT_LINK;
                else
                    *pdwEffect = DROPEFFECT_COPY;
            }
        }
    
        if (punk)
        {
            hres = _AddBand(punk);

            if (SUCCEEDED(hres))
            {
                DWORD dwState;

                dwState = IDataObject_GetDeskBandState(pdtobj);
                SetBandState(ShortFromResult(hres), BSSF_NOTITLE, dwState & BSSF_NOTITLE);
            }

            punk->Release();
        }
    }
    
    if (FAILED(hres)) 
        *pdwEffect = DROPEFFECT_NONE;
    return hres;
}

// }

//***   ::_MergeBS -- merge two bandsites into one
// ENTRY/EXIT
//  pdtDst  [INOUT] destination DropTarget (always from bandsite)
//  pbsSrc  [INOUT] source bandsite; deleted if all bands moved successfully
//  ret     S_OK if all bands moved; S_FALSE if some moved; E_* o.w.
// NOTES
//  note that if all the bands are moved successfully, pbsSrc will be deleted
//  as a side-effect.
//  pdtDst is assumed to accept multiple drops (bandsite does).
//  pdtDst may be from marshal/unmarshal (tray bandsite).
HRESULT _MergeBS(IDropTarget *pdtDst, IBandSite *pbsSrc)
{
    HRESULT hres = E_FAIL;
    DWORD idBand;

    pbsSrc->AddRef();           // don't go away until we're all done!

    // drag each band in turn
    while (SUCCEEDED(pbsSrc->EnumBands(0, &idBand))) {

        // note our (bogus?) assumption that bands which can't be
        // dragged will percolate down to a contiguous range of
        // iBands 0..n.  if that's bogus i'm not sure how we can
        // keep track of where we are.

        IDataObject *pdoSrc;
        hres = pbsSrc->GetBandObject(idBand, IID_IDataObject, (void **)&pdoSrc);
        if (SUCCEEDED(hres)) {
            DWORD dwEffect = DROPEFFECT_MOVE | DROPEFFECT_COPY;
            hres = SHSimulateDrop(pdtDst, pdoSrc, 0, NULL, &dwEffect);
            pdoSrc->Release();

            if (SUCCEEDED(hres) && (dwEffect & DROPEFFECT_MOVE)) {
                hres = pbsSrc->RemoveBand(idBand);
                ASSERT(SUCCEEDED(hres));
            }
        }

        // we failed to move the band, bail

        if (FAILED(hres)) {
            ASSERT(0);
            break;
        }
    }

    pbsSrc->Release();

    TraceMsg(DM_DRAG, "dba.ms: ret hres=%x", hres);
    return hres;
}


void CBandSite::_BandItemEnumCallback(int dincr, PFNBANDITEMENUMCALLBACK pfnCB, void *pv)
{
    UINT iFirst = 0;

    ASSERT(dincr == 1 || dincr == -1);
    if (dincr < 0) {
        iFirst = _GetBandItemCount() - 1;  // start from last
    }

    for (UINT i = iFirst; i < (UINT) _GetBandItemCount(); i += dincr) {
        LPBANDITEMDATA pbid = _GetBandItem(i);
        if (pbid && !pfnCB(pbid, pv))
            break;
    }
}

void CBandSite::_DeleteAllBandItems()
{
    for (int i = _GetBandItemCount() - 1; i >= 0; i--)
    {
        LPBANDITEMDATA pbid = _GetBandItem(i);

        // Release the banditem data first, while it can still
        // receive cleanup notifications from its control.  *Then*
        // delete the band item.
        if (pbid)
            _ReleaseBandItemData(pbid, i);

        // BUGBUG chrisfra 5/13/97 if you skip deleting, rebar can
        // rearrange on delete, moving a band so that it is never seen
        // and consequently we leak BrandBand and much else
        _DeleteBandItem(i);    // unhook from host (rebar)
    }
}

LPBANDITEMDATA CBandSite::_GetBandItem(int i)
{
    REBARBANDINFO rbbi;
    rbbi.cbSize = SIZEOF(rbbi);
    rbbi.fMask = RBBIM_LPARAM;
    rbbi.lParam = NULL; // in case of failure below

    if (_hwnd)
        SendMessage(_hwnd, RB_GETBANDINFO, i, (LPARAM)&rbbi);
    return (LPBANDITEMDATA)rbbi.lParam;
}

int CBandSite::_GetBandItemCount()
{
    int cel = 0;

    if (_hwnd)
    {
        ASSERT(IS_VALID_HANDLE(_hwnd, WND));

        cel = (int)SendMessage(_hwnd, RB_GETBANDCOUNT, 0, 0);
    }
    return cel;
}

void CBandSite::_GetBandInfo(LPBANDITEMDATA pbid, DESKBANDINFO *pdbi)
{
    pdbi->dwMask = DBIM_MINSIZE | DBIM_MAXSIZE | DBIM_INTEGRAL | DBIM_ACTUAL | DBIM_TITLE | DBIM_MODEFLAGS | DBIM_BKCOLOR;
                 
    pdbi->ptMinSize = pbid->ptMinSize;
    pdbi->ptMaxSize = pbid->ptMaxSize;
    pdbi->ptIntegral = pbid->ptIntegral;
    pdbi->ptActual = pbid->ptActual;
    StrCpyW(pdbi->wszTitle, pbid->szTitle);
    pdbi->dwModeFlags = pbid->dwModeFlags;
    pdbi->crBkgnd = pbid->crBkgnd;
    
    pbid->pdb->GetBandInfo(pbid->dwBandID, _dwMode, pdbi);
    if (pdbi->wszTitle[0] == 0) {
        pdbi->dwMask &= ~DBIM_TITLE;
    }

    pbid->ptMinSize = pdbi->ptMinSize;
    pbid->ptMaxSize = pdbi->ptMaxSize;
    pbid->ptIntegral = pdbi->ptIntegral;
    pbid->ptActual = pdbi->ptActual;
    StrCpyW(pbid->szTitle, pdbi->wszTitle);
    pbid->dwModeFlags = pdbi->dwModeFlags;
    pbid->crBkgnd = pdbi->crBkgnd;

    if (!(pdbi->dwMask & DBIM_TITLE))   // title not supported
        pbid->fNoTitle = TRUE;

    ASSERT(pdbi->dwModeFlags & DBIMF_VARIABLEHEIGHT ? pbid->ptIntegral.y : TRUE);
}

void CBandSite::_BandInfoFromBandItem(REBARBANDINFO *prbbi, LPBANDITEMDATA pbid, BOOL fBSOnly)
{
    // REVIEW: could be optimized more
    DESKBANDINFO dbi;

    if (!fBSOnly)
        _GetBandInfo(/*INOUT*/ pbid, &dbi);

    // now add the view as a band in the rebar
    // add links band
    prbbi->fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_ID | RBBIM_IDEALSIZE | RBBIM_TEXT;
    if (fBSOnly)
        prbbi->fMask = RBBIM_STYLE|RBBIM_TEXT;

    // clear the bits the are band settable
    prbbi->fStyle |= RBBS_FIXEDBMP;
    prbbi->fStyle &= ~(RBBS_GRIPPERALWAYS | RBBS_VARIABLEHEIGHT | RBBS_USECHEVRON);

    if (_dwStyle & BSIS_NOGRIPPER)    
        prbbi->fStyle |= RBBS_NOGRIPPER;
    else if (_dwStyle & BSIS_ALWAYSGRIPPER)    
        prbbi->fStyle |= RBBS_GRIPPERALWAYS;
    else {
        // BSIS_AUTOGRIPPER...
        if (!(prbbi->fStyle & RBBS_FIXEDSIZE) &&
            !(_dwMode & DBIF_VIEWMODE_FLOATING))
            prbbi->fStyle |= RBBS_GRIPPERALWAYS;
    }

    if (pbid->dwModeFlags & DBIMF_VARIABLEHEIGHT) 
        prbbi->fStyle |= RBBS_VARIABLEHEIGHT;

    if (pbid->dwModeFlags & DBIMF_USECHEVRON)
        prbbi->fStyle |= RBBS_USECHEVRON;

    if (pbid->dwModeFlags & DBIMF_BREAK)
        prbbi->fStyle |= RBBS_BREAK;

    if (!fBSOnly) {
        prbbi->hwndChild = pbid->hwnd;
        prbbi->wID = pbid->dwBandID;

        // set up the geometries
        prbbi->cxMinChild = pbid->ptMinSize.x;
        prbbi->cyMinChild = pbid->ptMinSize.y;
        prbbi->cyMaxChild = pbid->ptMaxSize.y;
        prbbi->cyIntegral = pbid->ptIntegral.y;

        if (_dwMode & (DBIF_VIEWMODE_FLOATING | DBIF_VIEWMODE_VERTICAL)) 
        {
            // after we're up, it's the "ideal" point
            prbbi->cxIdeal = pbid->ptActual.y;
        } 
        else 
        {
            // after we're up, it's the "ideal" point
            prbbi->cxIdeal = pbid->ptActual.x;
        }

        if (prbbi->cxIdeal == (UINT)-1)
            prbbi->cxIdeal = 0;

        if (pbid->dwModeFlags & DBIMF_BKCOLOR)
        {
            if (dbi.dwMask & DBIM_BKCOLOR)
            {
                prbbi->fMask |= RBBIM_COLORS;
                prbbi->clrFore = CLR_DEFAULT;
                prbbi->clrBack = dbi.crBkgnd;
            }
        }
        ASSERT(pbid->fNoTitle || (dbi.dwMask & DBIM_TITLE));    // pbid in sync?
    }

    SHUnicodeToTChar(pbid->szTitle, prbbi->lpText, prbbi->cch);
    if (!pbid->fNoTitle && _IsEnableTitle(pbid))
    {
        prbbi->fStyle &= ~RBBS_HIDETITLE;
    }
    else
    {
        // No text please
        prbbi->fStyle |= RBBS_HIDETITLE;
    }
        

    // Make this band a tabstop.  Itbar will override v_SetTabstop
    // since for the browser we don't want every band to be a tabstop.
    v_SetTabstop(prbbi);
}

void CBandSite::v_SetTabstop(LPREBARBANDINFO prbbi)
{
    // We specify that a band should be a tabstop by setting the WS_TABSTOP
    // bit.  Never make RBBS_FIXEDSIZE bands (i.e., the brand) tabstops.
    if (prbbi && prbbi->hwndChild && !(prbbi->fStyle & RBBS_FIXEDSIZE))
        SHSetWindowBits(prbbi->hwndChild, GWL_STYLE, WS_TABSTOP, WS_TABSTOP);
}

//***   cbs::_IsEnableTitle -- should we enable (ungray) title
// DESCRIPTION
//  used for handing back title and for enabling menu
// NOTES
//  pbid unused...
//
#ifndef UNIX
_inline
#endif
BOOL CBandSite::_IsEnableTitle(LPBANDITEMDATA pbid)
{
    ASSERT(pbid);
    return (/*pbid && !pbid->fNoTitle &&*/
      !((_dwMode & DBIF_VIEWMODE_FLOATING) && _GetBandItemCount() <= 1));
}

BOOL CBandSite::_UpdateBandInfo(LPBANDITEMDATA pbid, BOOL fBSOnly)
{
    REBARBANDINFO rbbi = {SIZEOF(rbbi)};
    int iRB = _BandIDToIndex(pbid->dwBandID);

    // now update the info
    rbbi.fMask = RBBIM_ID | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE;
    if (fBSOnly)
        rbbi.fMask = RBBIM_STYLE;

    SendMessage(_hwnd, RB_GETBANDINFO, iRB, (LPARAM)&rbbi);

    if (!fBSOnly) {
        if (_dwMode & (DBIF_VIEWMODE_FLOATING | DBIF_VIEWMODE_VERTICAL)) 
        {
            pbid->ptActual.x = rbbi.cyChild;
            pbid->ptActual.y = rbbi.cxIdeal;
        } 
        else 
        {
            pbid->ptActual.x = rbbi.cxIdeal;
            pbid->ptActual.y = rbbi.cyChild;
        }
        pbid->ptMinSize.x = rbbi.cxMinChild;
        pbid->ptMinSize.y = rbbi.cyMinChild;
        pbid->ptMaxSize.y = rbbi.cyMaxChild;
    }

    TCHAR szBand[40];
    rbbi.lpText = szBand;
    rbbi.cch = ARRAYSIZE(szBand);

    _BandInfoFromBandItem(&rbbi, pbid, fBSOnly);
    
    return BOOLFROMPTR(SendMessage(_hwnd, RB_SETBANDINFO, (UINT)iRB, (LPARAM)&rbbi));
}

BOOL CBandSite::_AddBandItem(LPBANDITEMDATA pbid)
{
    REBARBANDINFO rbbi = {SIZEOF(rbbi)};

    pbid->ptActual.x = -1;
    pbid->ptActual.y = -1;

    TCHAR szBand[40];
    rbbi.lpText = szBand;
    rbbi.cch = ARRAYSIZE(szBand);

    _BandInfoFromBandItem(&rbbi, pbid, FALSE);

    rbbi.cyChild = pbid->ptActual.y;
    rbbi.fMask |= RBBIM_LPARAM;
    rbbi.lParam = (LPARAM)pbid;

    ASSERT(rbbi.fMask & RBBIM_ID);

    return BOOLFROMPTR(SendMessage(_hwnd, RB_INSERTBAND, (UINT) -1, (LPARAM)&rbbi));
}

void CBandSite::_DeleteBandItem(int i)
{
    SendMessage(_hwnd, RB_DELETEBAND, i, 0);
}

DWORD CBandSite::_IndexToBandID(int i)
{
    REBARBANDINFO rbbi = {SIZEOF(rbbi)};
    rbbi.fMask = RBBIM_ID;

    if (SendMessage(_hwnd, RB_GETBANDINFO, i, (LPARAM)&rbbi))
        return rbbi.wID;
    else
        return -1;
}


/*----------------------------------------------------------
Purpose: Given the band ID, returns the internal band index.

*/
int CBandSite::_BandIDToIndex(DWORD dwBandID)
{
    int nRet = -1;

    if (_hwnd)
        nRet = (int)SendMessage(_hwnd, RB_IDTOINDEX, (WPARAM) dwBandID, (LPARAM) 0);
    return nRet;
}


/*----------------------------------------------------------
Purpose: The Parent Site may want to override what the admin
         specified.

Return Values:
    S_OK: Do lock band.
    S_FALSE: Do NOT Lock band.

*/
HRESULT CBandSite::_IsRestricted(DWORD dwBandID, DWORD dwRestrictAction, DWORD dwBandFlags)
{
    HRESULT hr;
    DWORD dwRestrictionAction;

    hr = IUnknown_HandleIRestrict(_punkSite, &RID_RDeskBars, dwRestrictAction, NULL, &dwRestrictionAction);
    if (RR_NOCHANGE == dwRestrictionAction)    // If our parent didn't handle it, we will.
        dwRestrictionAction = IsFlagSet(_GetAdminSettings(dwBandID), dwBandFlags) ? RR_DISALLOW : RR_ALLOW;

    if (RR_DISALLOW == dwRestrictionAction)
        hr = S_OK;
    else
        hr = S_FALSE;

    ASSERT(SUCCEEDED(hr));  // FAIL(hr) other than hr == E_NOTIMPLE; is not good.
    return hr;
}

BOOL ConfirmRemoveBand(HWND hwnd, UINT uID, LPCTSTR pszName)
{
    TCHAR szTemp[1024], *pszTemp2, *pszStr, szTitle[80];
    BOOL bRet = TRUE;
    DWORD dwLen;


    MLLoadString(IDS_CONFIRMCLOSETITLE, szTitle, ARRAYSIZE(szTitle));

    // Calling FormatMessage with FORMAT_MESSAGE_FROM_HMODULE fails
    MLLoadString(uID, szTemp, ARRAYSIZE(szTemp));

    dwLen = (lstrlen(szTemp) + lstrlen(pszName) + 1) * sizeof(TCHAR);
    if((pszTemp2 = (TCHAR *)LocalAlloc(LPTR, dwLen)) != NULL)
    {
        _FormatMessage(szTemp, pszTemp2, dwLen, pszName);

        MLLoadString(IDS_CONFIRMCLOSETEXT, szTemp, ARRAYSIZE(szTemp));

        dwLen = (lstrlen(szTemp) + lstrlen(pszTemp2) + 1) * sizeof(TCHAR);
        if((pszStr = (TCHAR *)LocalAlloc(LPTR, dwLen)) != NULL)
        {
            _FormatMessage(szTemp, pszStr, dwLen, pszTemp2);

            bRet = (IDOK == SHMessageBoxCheck(hwnd, pszStr, szTitle, MB_OKCANCEL, IDOK, TEXT("WarnBeforeCloseBand")));

            LocalFree(pszStr);
        }

        LocalFree(pszTemp2);
    }

    return bRet;
}
