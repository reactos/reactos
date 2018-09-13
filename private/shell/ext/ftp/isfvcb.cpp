/*****************************************************************************\
    FILE: isfvcb.cpp

    DESCRIPTION:
        This is a base class that implements the default behavior of 
    IShellFolderViewCallBack.  This allows default DefView implementation with this
    callback to override specific behavior.
\*****************************************************************************/

#include "priv.h"
#include "isfvcb.h"


//===========================
// *** IShellFolderViewCB Interface ***
//===========================

/*****************************************************************************\
    FUNCTION: _OnSetISFV

    DESCRIPTION:
        Same as ::SetSite();
\*****************************************************************************/
HRESULT CBaseFolderViewCB::_OnSetISFV(IShellFolderView * psfv)
{
    IUnknown_Set((IUnknown **) &m_psfv, (IUnknown *) psfv);
    return S_OK;
}

/*****************************************************************************\
    FUNCTION: IShellFolderViewCB::MessageSFVCB

    DESCRIPTION:
\*****************************************************************************/
#define NOTHANDLED(m) case m: hr = E_NOTIMPL; break

HRESULT CBaseFolderViewCB::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = E_FAIL;

    switch (uMsg)
    {
    case DVM_GETDETAILSOF:
        hr = _OnGetDetailsOf((UINT)wParam, (PDETAILSINFO)lParam);
        break;

    case DVM_COLUMNCLICK:
        hr = _OnColumnClick((UINT)wParam);
        break;

    case DVM_MERGEMENU:
        hr = _OnMergeMenu((LPQCMINFO)lParam);
        break;

    case DVM_UNMERGEMENU:
        hr = _OnUnMergeMenu((HMENU)lParam);
        break;

    case DVM_INVOKECOMMAND:
        hr = _OnInvokeCommand((UINT)wParam);
        break;

    case DVM_GETHELPTEXT:
        hr = _OnGetHelpText(lParam, wParam);
        break;

    case SFVM_GETHELPTOPIC:
        hr = _OnGetHelpTopic((SFVM_HELPTOPIC_DATA *) lParam);
        break;

    case DVM_GETTOOLTIPTEXT:
        // TODO: Implement
        hr = E_NOTIMPL;
        break;

    case DVM_UPDATESTATUSBAR:
        // TODO: Implement
        hr = _OnUpdateStatusBar();
        break;

    case DVM_WINDOWCREATED:
        hr = _OnWindowCreated();
        break;

    case SFVM_BACKGROUNDENUMDONE:
        hr = _OnBackGroundEnumDone();
        break;

    case DVM_INITMENUPOPUP:
        hr = _OnInitMenuPopup((HMENU) lParam, (UINT) HIWORD(wParam), (UINT) LOWORD(wParam));
        break;

    case DVM_RELEASE:
    {
        CBaseFolderViewCB * pfv = (CBaseFolderViewCB *) lParam;
        if (pfv)
            hr = pfv->Release();
    }
    break;

    case DVM_DEFITEMCOUNT:
        hr = _OnDefItemCount((LPINT)lParam);
        break;

    case DVM_DIDDRAGDROP:
        hr = _OnDidDragDrop((DROPEFFECT)wParam, (IDataObject *)lParam);
        break;

    case DVM_REFRESH:
        hr = _OnRefresh((BOOL) wParam);
        break;

    case SFVM_ADDPROPERTYPAGES:
        hr = _OnAddPropertyPages((SFVM_PROPPAGE_DATA *)lParam);
        break;

    case DVM_BACKGROUNDENUM:
        //  WARNING!  If we return S_OK from DVM_BACKGROUNDENUM, we also
        //  are promising that we support free threading on our IEnumIDList
        //  interface!  This allows the shell to do enumeration on our
        //  IEnumIDList on a separate background thread.
        hr = S_OK;                    // Always enum in background
        break;

    case SFVM_DONTCUSTOMIZE:
        if (lParam)
            *((BOOL *) lParam) = FALSE;  // Yes, we are customizable.
        hr = S_OK;
        break;

    case SFVM_GETZONE:
        hr = _OnGetZone((DWORD *) lParam, wParam);
        break;

    case SFVM_GETPANE:
        hr = _OnGetPane((DWORD) wParam, (DWORD *)lParam);
        break;

    case SFVM_SETISFV:
        hr = _OnSetISFV((IShellFolderView *)lParam);
        break;

    case SFVM_GETNOTIFY:
        hr = _OnGetNotify((LPITEMIDLIST *) wParam, (LONG *) lParam);
        break;

    case SFVM_FSNOTIFY:
        hr = _OnFSNotify((LPITEMIDLIST *) wParam, (LONG *) lParam);
        break;

    case SFVM_QUERYFSNOTIFY:
        hr = _OnQueryFSNotify((SHChangeNotifyEntry *) lParam);
        // BUGBUG: Do we need to implement this? Maybe it will fix #173045
        break;

    case SFVM_SIZE:
        hr = _OnSize((LONG) wParam, (LONG) lParam);
        break;

    case SFVM_THISIDLIST:
        hr = _OnThisIDList((LPITEMIDLIST *) lParam);
        break;


    // We need to do the following BUGBUG/TODO
    // Do we want to implement these later?
    // SFVM_HWNDMAIN

    // Others that aren't currently handled.
    NOTHANDLED(DVM_GETBUTTONINFO);
    NOTHANDLED(DVM_GETBUTTONS);
    NOTHANDLED(DVM_SELCHANGE);
    NOTHANDLED(DVM_DRAWITEM);
    NOTHANDLED(DVM_MEASUREITEM);
    NOTHANDLED(DVM_EXITMENULOOP);
    NOTHANDLED(DVM_GETCCHMAX);
    NOTHANDLED(DVM_WINDOWDESTROY);
    NOTHANDLED(DVM_SETFOCUS);
    NOTHANDLED(DVM_KILLFOCUS);
    NOTHANDLED(DVM_QUERYCOPYHOOK);
    NOTHANDLED(DVM_NOTIFYCOPYHOOK);
    NOTHANDLED(DVM_DEFVIEWMODE);
#if 0
    NOTHANDLED(DVM_INSERTITEM);         // Too verbose
    NOTHANDLED(DVM_DELETEITEM);
#endif

    NOTHANDLED(DVM_GETWORKINGDIR);
    NOTHANDLED(DVM_GETCOLSAVESTREAM);
    NOTHANDLED(DVM_SELECTALL);
    NOTHANDLED(DVM_SUPPORTSIDENTIFY);
    NOTHANDLED(DVM_FOLDERISPARENT);
    default:
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}



/****************************************************\
    Constructor
\****************************************************/
CBaseFolderViewCB::CBaseFolderViewCB() : m_cRef(1), m_dwSignature(c_dwSignature)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_psfv);

}


/****************************************************\
    Destructor
\****************************************************/
CBaseFolderViewCB::~CBaseFolderViewCB()
{
    m_dwSignature = 0;                  // Turn off _IShellFolderViewCallBack
    IUnknown_Set((IUnknown **)&m_psfv, NULL);
    DllRelease();
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CBaseFolderViewCB::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CBaseFolderViewCB::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

// {7982F251-C37A-11d1-9823-006097DF5BD4}
static const GUID CIID_PrivateThis = { 0x7982f251, 0xc37a, 0x11d1, { 0x98, 0x23, 0x0, 0x60, 0x97, 0xdf, 0x5b, 0xd4 } };

HRESULT CBaseFolderViewCB::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IShellFolderViewCB))
    {
        *ppvObj = SAFECAST(this, IShellFolderViewCB*);
    }
    else
    if (IsEqualIID(riid, IID_IObjectWithSite))
    {
        *ppvObj = SAFECAST(this, IObjectWithSite*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


HRESULT CBaseFolderViewCB::_IShellFolderViewCallBack(IShellView * psvOuter, IShellFolder * psf, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    IShellFolderViewCB * psfvcb = NULL;
    HRESULT hr = E_FAIL;


    // Now this is a total hack.  I am bastardizing the pszOuter param to really be the this pointer that
    // is the IShellFolderViewCB interface of the CBaseFolderViewCB object.  I use the SFVM_WINDOWDESTROY event to
    // release the object but DefView calls us back with one more message before we completely go away
    // and that message is SFVM_SETISFV.  Everytime the SFVM_SETISFV is called with a NULL lParam, it's
    // equivalent to calling ::SetSite(NULL).  We can ignore this because we release the back pointer in our
    // destructor.
    if (((SFVM_SETISFV == uMsg) && !lParam) ||
        (SFVM_PRERELEASE == uMsg))
    {
        return S_OK;
    }

    // psvOuter is really our CBaseFolderViewCB.  Sniff around to make sure.
    // Note that this casting must exactly invert the casts that we do in
    // CBaseFolder::_CreateShellView.

    CBaseFolderViewCB *pbfvcb = (CBaseFolderViewCB *)(IShellFolderViewCB *)psvOuter;

    if (EVAL(!IsBadReadPtr(pbfvcb, sizeof(CBaseFolderViewCB))) &&
        EVAL(pbfvcb->m_dwSignature == c_dwSignature))
    {

        // psvOuter is really our CBaseFolderViewCB and let's make sure with this QI.
        hr = psvOuter->QueryInterface(IID_IShellFolderViewCB, (void **) &psfvcb);
        if (EVAL(psfvcb))
        {
            hr = psfvcb->MessageSFVCB(uMsg, wParam, lParam);

            if ((SFVM_WINDOWDESTROY == uMsg)) // (DVM_WINDOWDESTROY == SFVM_WINDOWDESTROY)
            {
                ASSERT(!lParam);    // Sometimes callers want this to be freed.
                psvOuter->Release();     // We are releasing the psvOuter that DefView is holding.  We shouldn't be called again.
            }

            psfvcb->Release();
        }
    }

    return hr;
}
