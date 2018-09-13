#include "priv.h"

#ifdef FEATURE_PICS

#include "asyncrat.h"
#include <ratings.h>
#include "dochost.h"
#include <mshtmdid.h>


/* There is a PicsQuery structure in the following global array for each
 * outstanding query.  It records the address of the PicsData structure in
 * the corresponding w3doc, the window handle corresponding to the Mwin,
 * and a serial number.  This way, RatingObtainQueryCallback can tell if
 * the page the query corresponds to still exists, before posting a message;
 * and PicsDataMessageLoop can tell if the doc still exists when the message
 * finally gets delivered.
 *
 * The array is dynamically allocated and is protected by the main HTML
 * critical section.
 */


HDSA g_haQueries = NULL;
DWORD g_dwPicsSerial = 1L;
const UINT c_cQueryAllocSize = 8;		/* should be plenty by default */
UINT g_crefQueries = 0;


/* AddPicsQuery - add an outstanding PICS query to the list, given a window
 * handle to send a completion message to.  Returns the serial number of the
 * query for later reference.
 */
DWORD _AddPicsQuery(HWND hwnd)
{
    ENTERCRITICAL;
    
    DWORD dwRet = 0;
    
    if (g_haQueries == NULL) {
        g_haQueries = DSA_Create(sizeof(PicsQuery), c_cQueryAllocSize);
    }
    
    if (g_haQueries != NULL) {
        PicsQuery q;
        
        q.dwSerial = ::g_dwPicsSerial++;
        q.hwnd = hwnd;
        q.lpvRatingDetails = NULL;
        
        if (DSA_InsertItem(g_haQueries, DA_LAST, &q) >= 0)
            dwRet = q.dwSerial;
    }
    
    LEAVECRITICAL;
    
    return dwRet;
}


/* RemovePicsQuery - remove an outstanding query based on its serial number.
*/
void _RemovePicsQuery(DWORD dwSerial)
{
    ENTERCRITICAL;
    
    if (g_haQueries != NULL) {
        UINT cQueries = DSA_GetItemCount(g_haQueries);
        PicsQuery *pq = NULL;
        for (UINT i=0; i<cQueries; i++) {
            pq = (PicsQuery *)DSA_GetItemPtr(g_haQueries, i);
            if (pq != NULL && pq->dwSerial == dwSerial)
                break;
        }
        
        if (pq != NULL) {
            if (pq->lpvRatingDetails != NULL)
                ::RatingFreeDetails(pq->lpvRatingDetails);
            DSA_DeleteItem(g_haQueries, i);
        }
    }
    
    LEAVECRITICAL;
}


/* GetPicsQuery - get a copy of an outstanding PICS query record, given its
 * serial number.  Returns TRUE if found.
 */
BOOL _GetPicsQuery(DWORD dwSerial, PicsQuery *pOut)
{
    ENTERCRITICAL;
    
    PicsQuery *pq = NULL;
    
    if (g_haQueries != NULL) {
        UINT cQueries = DSA_GetItemCount(g_haQueries);
        for (UINT i=0; i<cQueries; i++) {
            pq = (PicsQuery *)DSA_GetItemPtr(g_haQueries, i);
            if (pq != NULL && pq->dwSerial == dwSerial)
                break;
        }
        
        if (pq != NULL) {
            *pOut = *pq;
            pq->lpvRatingDetails = NULL;	/* caller's copy owns this now */
        }
    }
    
    LEAVECRITICAL;
    
    return pq != NULL;
}


/* _RefPicsQueries - add a reference to the async query array */
void _RefPicsQueries(void)
{
    ENTERCRITICAL;

    ++g_crefQueries;

    LEAVECRITICAL;
}


/* _ReleasePicsQueries - cleanup all memory associated with outstanding queries
 */
void _ReleasePicsQueries(void)
{
    ENTERCRITICAL;
    
    if (!--g_crefQueries) {
        if (g_haQueries != NULL) {
            UINT cQueries = DSA_GetItemCount(g_haQueries);
            for (UINT i=0; i<cQueries; i++) {
                PicsQuery *pq = (PicsQuery *)DSA_GetItemPtr(g_haQueries, i);
                if (pq != NULL && pq->lpvRatingDetails != NULL) {
                    RatingFreeDetails(pq->lpvRatingDetails);
                }
            }
            DSA_Destroy(g_haQueries);
            g_haQueries = NULL;
            // leave g_dwPicsSerial as it is, just in case we start up again
        }
    }
    
    LEAVECRITICAL;
}


/* PostPicsMessage - formats up a custom window message to signal that a
 * query is complete.  Format is WM_PICS_STATUS(hresult,dwSerial).  Other
 * information (the rating details blob obtained from RatingCheckUserAccess)
 * is stored in the query record for safekeeping.
 *
 * Returns TRUE if a message was posted successfully to the right window.
 */
BOOL _PostPicsMessage(DWORD dwSerial, HRESULT hr, LPVOID lpvRatingDetails)
{
    BOOL fRet = FALSE;
    
    ENTERCRITICAL;
    
    if (g_haQueries != NULL) {
        PicsQuery *pq = NULL;
        UINT cQueries = DSA_GetItemCount(g_haQueries);
        for (UINT i=0; i<cQueries; i++) {
            pq = (PicsQuery *)DSA_GetItemPtr(g_haQueries, i);
            if (pq != NULL && pq->dwSerial == dwSerial)
                break;
        }
        
        if (pq != NULL) {
            pq->lpvRatingDetails = lpvRatingDetails;
            fRet = PostMessage(pq->hwnd, WM_PICS_ASYNCCOMPLETE, (WPARAM)hr,
                (LPARAM)dwSerial);
            if (!fRet) {	/* oops, couldn't post message, don't keep copy of details */
                pq->lpvRatingDetails = NULL;
            }
        }
    }
    
    LEAVECRITICAL;
    
    return fRet;
}


/* Class CPicsRootDownload manages the download of the root document of a
 * site, to get ratings from it.
 */

CPicsRootDownload::CPicsRootDownload(CDocObjectHost *pdoh, IOleCommandTarget *pctParent, BOOL fFrameIsOffline, BOOL fFrameIsSilent)
{
    m_cRef = 1;
    m_pctParent = pctParent; m_pctParent->AddRef();
    m_pdoh = pdoh; m_pdoh->AddRef();
    m_pole = NULL;
    m_pctObject = NULL;
    m_pBinding = NULL;
    m_fFrameIsOffline = fFrameIsOffline ? TRUE : FALSE;
    m_fFrameIsSilent = fFrameIsSilent ? TRUE : FALSE;
}


CPicsRootDownload::~CPicsRootDownload()
{
    ATOMICRELEASE(m_pctParent);

    CleanUp();

    ATOMICRELEASE(m_pBinding);

    ATOMICRELEASE(m_pBindCtx);
    ATOMICRELEASET(m_pdoh,CDocObjectHost);
}


HRESULT CPicsRootDownload::StartDownload(IMoniker *pmk)
{
    IUnknown *punk = NULL;
    HRESULT hr;

    hr = CreateBindCtx(0, &m_pBindCtx);
    if (FAILED(hr))
        goto LErrExit;

    /*
    hr = m_pBindCtx->RegisterObjectParam(BROWSER_OPTIONS_OBJECT_NAME,
                    (IBrowseControl *)this);
    if (FAILED(hr))
        goto LErrExit;
    */

    //
    //  Associate the client site as an object parameter to this
    // bind context so that Trident can pick it up while processing
    // IPersistMoniker::Load().
    //
    m_pBindCtx->RegisterObjectParam(WSZGUID_OPID_DocObjClientSite,
                                    SAFECAST(this, IOleClientSite*));

    hr = RegisterBindStatusCallback(m_pBindCtx,
            (IBindStatusCallback *)this,
            0,
            0L);
    if (FAILED(hr))
        goto LErrExit;

    hr = pmk->BindToObject(m_pBindCtx, NULL, IID_IUnknown, (LPVOID*)&punk);

    if (SUCCEEDED(hr) || hr==E_PENDING)
    {
        hr = S_OK;

        //
        // If moniker happen to return the object synchronously, emulate
        // OnDataAvailable callback and OnStopBinding.
        //
        if (punk)
        {
            OnObjectAvailable(IID_IUnknown, punk);
            OnStopBinding(hr, NULL);
            punk->Release();
        }
    }
    else
    {
        /* OnStopBinding can be called by URLMON within the BindToObject
         * call in some cases.  So, don't call it ourselves if it's
         * already been called (we can tell by looking whether our
         * bind context still exists).
         */
        if (m_pBindCtx != NULL) {
            OnStopBinding(hr, NULL);
        }
    }

LErrExit:
    if (FAILED(hr) && (m_pBindCtx != NULL)) {
        m_pBindCtx->Release();
        m_pBindCtx = NULL;
    }

    return hr;
}


/* _NotifyEndOfDocument is used in all the error cases to make sure the caller
 * gets a notification of some sort.  The case where this function does not
 * send a notification is if we have a valid OLE object -- in that case, we're
 * assuming that we have it because we know it supports PICS, therefore we're
 * expecting it to send such a notification to the parent itself.
 */
void CPicsRootDownload::_NotifyEndOfDocument(void)
{
    if (m_pole == NULL) {
        if (m_pctParent != NULL) {
            m_pctParent->Exec(&CGID_ShellDocView, SHDVID_NOMOREPICSLABELS, 0, NULL, NULL);
        }
    }
}


HRESULT CPicsRootDownload::_Abort()
{
    if (m_pBinding)
    {
        return m_pBinding->Abort();
    }
    return S_FALSE;
}


void CPicsRootDownload::CleanUp()
{
    _Abort();

    if (m_pctObject != NULL) {
        VARIANTARG v;
        v.vt = VT_UNKNOWN;
        v.lVal = (DWORD)0;
        m_pctObject->Exec(&CGID_ShellDocView, SHDVID_CANSUPPORTPICS, 0, &v, NULL);
        m_pctObject->Exec(NULL, OLECMDID_STOP, NULL, NULL, NULL);
        ATOMICRELEASE(m_pctObject);
    }

    LPOLECLIENTSITE pcs;
    if (m_pole && SUCCEEDED(m_pole->GetClientSite(&pcs)) && pcs) 
    {
        if (pcs == SAFECAST(this, LPOLECLIENTSITE)) 
        {
            m_pole->SetClientSite(NULL);
        }
        pcs->Release();
    }

    ATOMICRELEASE(m_pole);
}


// IUnknown members
STDMETHODIMP CPicsRootDownload::QueryInterface(REFIID riid, void **punk)
{
    *punk = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IsPicsBrowser))
        *punk = (IUnknown *)(IBindStatusCallback *)this;
    else if (IsEqualIID(riid, IID_IBindStatusCallback))
        *punk = (IBindStatusCallback *)this;
    else if (IsEqualIID(riid, IID_IOleClientSite))
        *punk = (IOleClientSite *)this;
    else if (IsEqualIID(riid, IID_IServiceProvider))
        *punk = (IServiceProvider *)this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *punk = (IDispatch *)this;

    if (*punk != NULL) {
        ((IUnknown *)(*punk))->AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CPicsRootDownload::AddRef(void)
{
    ++m_cRef;
    TraceMsg(TF_SHDREF, "CPicsRootDownload(%x)::AddRef called, new m_cRef=%d", this, m_cRef);
    return m_cRef;
}


STDMETHODIMP_(ULONG) CPicsRootDownload::Release(void)
{
    UINT crefNew = --m_cRef;

    TraceMsg(TF_SHDREF, "CPicsRootDownload(%x)::Release called, new m_cRef=%d", this, m_cRef);

    if (!crefNew)
        delete this;

    return crefNew;
}

// IBindStatusCallback methods
STDMETHODIMP CPicsRootDownload::OnStartBinding(DWORD dwReserved, IBinding* pbinding)
{
    if (m_pBinding != NULL)
        m_pBinding->Release();

    m_pBinding = pbinding;

    if (m_pBinding != NULL)
        m_pBinding->AddRef();

    return S_OK;
}


STDMETHODIMP CPicsRootDownload::GetPriority(LONG* pnPriority)
{
    return E_NOTIMPL;
}


STDMETHODIMP CPicsRootDownload::OnLowResource(DWORD dwReserved)
{
    return E_NOTIMPL;
}


STDMETHODIMP CPicsRootDownload::OnProgress(ULONG ulProgress, ULONG ulProgressMax,
                                           ULONG ulStatusCode, LPCWSTR pwzStatusText)
{
    /* If the root document's data type is not HTML, don't try to get any
     * ratings out of it, just abort.
     */
    if (ulStatusCode == BINDSTATUS_CLASSIDAVAILABLE) {
        BOOL fContinueDownload = FALSE;

        CLSID clsid;
        // CLSIDFromString is prototyped wrong, non const first param
        HRESULT hresT = CLSIDFromString((WCHAR *)pwzStatusText, &clsid);
        if (SUCCEEDED(hresT)) {
            LPWSTR pwzProgID = NULL;
            hresT = ProgIDFromCLSID(clsid, &pwzProgID);
            if (SUCCEEDED(hresT)) {
                if (StrCmp(pwzProgID, L"htmlfile") == 0)
                {
                    fContinueDownload = TRUE;
                }
                OleFree(pwzProgID);
            }
        }

        if (!fContinueDownload) {
            _Abort();
        }
    }

    return S_OK;
}


STDMETHODIMP CPicsRootDownload::OnStopBinding(HRESULT hrResult, LPCWSTR szError)
{
    /* Some of the cleanup we do in here (RevokeObjectParam is suspect?) could
     * remove our last reference, causing the Releases at the end to fault.
     * Guard against this with an AddRef/Release.  Dochost does this too.
     *
     * BUGBUG - if URLMON is calling back through this object, shouldn't he
     * have a reference to us?  If so, where is it?
     */
    AddRef();

    /* Notify the caller that we've got to the end of the document */
    _NotifyEndOfDocument();
    m_pBindCtx->RevokeObjectParam(WSZGUID_OPID_DocObjClientSite);
    ::RevokeBindStatusCallback(m_pBindCtx, (IBindStatusCallback *)this);
    ATOMICRELEASE(m_pBinding);
    ATOMICRELEASE(m_pBindCtx);

    /* Undo above AddRef(). */
    Release();

    return S_OK;
}

void SetBindfFlagsBasedOnAmbient(BOOL fAmbientOffline, DWORD *pgrfBindf);

STDMETHODIMP CPicsRootDownload::GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindInfo)
{
    if ( !pgrfBINDF || !pbindInfo || !pbindInfo->cbSize )
        return E_INVALIDARG;

    *pgrfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE;
    *pgrfBINDF |= BINDF_GETNEWESTVERSION;

    if(m_fFrameIsSilent)
    {
        *pgrfBINDF |= BINDF_NO_UI;  
    }
    else
    {
        *pgrfBINDF &= ~BINDF_NO_UI;
    }

    SetBindfFlagsBasedOnAmbient(BOOLIFY(m_fFrameIsOffline), pgrfBINDF);
    
    // clear BINDINFO except cbSize
    DWORD cbSize = pbindInfo->cbSize;
    ZeroMemory( pbindInfo, cbSize );
    pbindInfo->cbSize = cbSize;

    pbindInfo->dwBindVerb = BINDVERB_GET;

    return S_OK;
}


STDMETHODIMP CPicsRootDownload::OnDataAvailable(DWORD grfBSCF, DWORD dwSize,
                                                FORMATETC *pfmtetc,
                                                STGMEDIUM* pstgmed)
{
    return E_NOTIMPL;
}


STDMETHODIMP CPicsRootDownload::OnObjectAvailable(REFIID riid, IUnknown* punk)
{
    if (SUCCEEDED(punk->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&m_pctObject))) {
        VARIANTARG v;
        v.vt = VT_UNKNOWN;
        v.punkVal = (IOleCommandTarget *)m_pctParent;
        HRESULT hresT = m_pctObject->Exec(&CGID_ShellDocView, SHDVID_CANSUPPORTPICS, 0, &v, NULL);
        if (hresT == S_OK) {
            hresT = punk->QueryInterface(IID_IOleObject, (LPVOID *)&m_pole);
            if (FAILED(hresT))
                m_pole = NULL;
        }
    }

    if (m_pole == NULL) {
        ATOMICRELEASE(m_pctObject);
        _Abort();
    }

    return S_OK;
}


// IOleClientSite
STDMETHODIMP CPicsRootDownload::SaveObject(void)
{
    return E_NOTIMPL;
}


STDMETHODIMP CPicsRootDownload::GetMoniker(DWORD, DWORD, IMoniker **)
{
    return E_NOTIMPL;
}


STDMETHODIMP CPicsRootDownload::GetContainer(IOleContainer **)
{
    return E_NOTIMPL;
}


STDMETHODIMP CPicsRootDownload::ShowObject(void)
{
    return E_NOTIMPL;
}


STDMETHODIMP CPicsRootDownload::OnShowWindow(BOOL fShow)
{
    return E_NOTIMPL;
}


STDMETHODIMP CPicsRootDownload::RequestNewObjectLayout(void)
{
    return E_NOTIMPL;
}


// IServiceProvider (must be QI'able from IOleClientSite)
STDMETHODIMP CPicsRootDownload::QueryService(REFGUID guidService,
                                    REFIID riid, void **ppvObj)
{
    if (IsEqualGUID(guidService, SID_STopLevelBrowser)) {
        if (IsEqualIID(riid, IID_IsPicsBrowser))
            return QueryInterface(riid, ppvObj);
        return E_NOINTERFACE;
    }

    return E_FAIL;
}


// IDispatch
HRESULT CPicsRootDownload::Invoke(DISPID dispidMember, REFIID iid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pdispparams,
                        VARIANT FAR* pVarResult,EXCEPINFO FAR* pexcepinfo,UINT FAR* puArgErr)
{
    if (!pVarResult)
        return E_INVALIDARG;

    if (wFlags == DISPATCH_PROPERTYGET)
    {
        switch (dispidMember)
        {
        case DISPID_AMBIENT_DLCONTROL :
            // We support IDispatch so that Trident can ask us to control the
            // download.  By specifying all the following flags, and by NOT
            // specifying DLCTL_DLIMAGES, DLCTL_VIDEOS, or DLCTL_BGSOUNDS,
            // we ensure we only download the HTML doc itself, and not a lot
            // of associated things that aren't going to help us find a META
            // tag.

            pVarResult->vt = VT_I4;
            pVarResult->lVal = DLCTL_SILENT | DLCTL_NO_SCRIPTS | 
                               DLCTL_NO_JAVA | DLCTL_NO_RUNACTIVEXCTLS |
                               DLCTL_NO_DLACTIVEXCTLS | DLCTL_NO_FRAMEDOWNLOAD |
                               DLCTL_NO_CLIENTPULL;
            break;
        default:
            return DISP_E_MEMBERNOTFOUND;
        }
        return S_OK;
    }

    return DISP_E_MEMBERNOTFOUND;
}


#endif  /* FEATURE_PICS */
