#include "priv.h"

#include "dhuihand.h"


#define DM_DOCHOSTUIHANDLER 0

//==========================================================================
// IDocHostUIHandler implementation
//==========================================================================

HRESULT CDocHostUIHandler::ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::ShowContextMenu called");

    //
    // LATER: WebBand in a DesktBar/BrowserBar needs to hook this event
    // to popup a customized context menu.
    //
    return S_FALSE; // Host did not display any UI. 
}

HRESULT CDocHostUIHandler::GetHostInfo(DOCHOSTUIINFO *pInfo)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::GetHostInfo called");

// Trident does not initialize it. It's defined as [in] parameter. 
#if 0
    if (pInfo->cbSize < SIZEOF(DOCHOSTUIINFO)) {
        return E_INVALIDARG;
    }
#endif
    pInfo->cbSize = SIZEOF(DOCHOSTUIINFO);
    pInfo->dwFlags = DOCHOSTUIFLAG_BROWSER;
// Disable double buffering if low memory machine.
//    if (SHIsLowMemoryMachine(ILMM_IE4))
//        pInfo->dwFlags = pInfo->dwFlags | DOCHOSTUIFLAG_DISABLE_OFFSCREEN;
    
    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;     // default
    return S_OK;
}

HRESULT CDocHostUIHandler::ShowUI( 
    DWORD dwID, IOleInPlaceActiveObject *pActiveObject,
    IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame,
    IOleInPlaceUIWindow *pDoc)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::ShowUI called");

    // Host did not display its own UI. Trident will proceed to display its own. 
    return S_FALSE;
}

HRESULT CDocHostUIHandler::HideUI(void)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::HideUI called");
    // This one is paired with ShowUI
    return S_FALSE;
}

HRESULT CDocHostUIHandler::UpdateUI(void)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::UpdateUI called");
    // LATER: Isn't this equivalent to OLECMDID_UPDATECOMMANDS?
    return S_FALSE;
}

HRESULT CDocHostUIHandler::EnableModeless(BOOL fEnable)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::EnableModeless called");
    // Called from the Trident when the equivalent member of its
    // IOleInPlaceActiveObject is called by the frame. We don't care
    // those cases.
    return S_OK;
}

HRESULT CDocHostUIHandler::OnDocWindowActivate(BOOL fActivate)
{
    // Called from the Trident when the equivalent member of its
    // IOleInPlaceActiveObject is called by the frame. We don't care
    // those cases.
    return S_OK;
}

HRESULT CDocHostUIHandler::OnFrameWindowActivate(BOOL fActivate)
{
    // Called from the Trident when the equivalent member of its
    // IOleInPlaceActiveObject is called by the frame. We don't care
    // those cases.
    return S_OK;
}

HRESULT CDocHostUIHandler::ResizeBorder( 
LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    // Called from the Trident when the equivalent member of its
    // IOleInPlaceActiveObject is called by the frame. We don't care
    // those cases.
    return S_OK;
}

HRESULT CDocHostUIHandler::TranslateAccelerator( 
LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
    // Called from the Trident when the equivalent member of its
    // IOleInPlaceActiveObject is called by the frame. We don't care
    // those cases.
    return S_FALSE; // The message was not translated
}

HRESULT CDocHostUIHandler::GetOptionKeyPath(BSTR *pbstrKey, DWORD dw)
{
    // Trident will default to its own user options.
    *pbstrKey = NULL;
    return S_FALSE;
}

HRESULT CDocHostUIHandler::GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::GetDropTarget called");

    return E_NOTIMPL;
}

HRESULT CDocHostUIHandler::GetAltExternal(IDispatch **ppDisp)
{
    HRESULT hr = E_FAIL;
    
    IServiceProvider  *psp;
    IDocHostUIHandler *pDocHostUIHandler;
    IOleObject        *pOleObject;
    IOleClientSite    *pOleClientSite;

    *ppDisp = NULL;

    //  * QI ourselves for a service provider
    //  * QS for the top level browser's service provider
    //  * Ask for an IOleObject
    //  * Ask the IOleObject for an IOleClientSite
    //  * QI the IOleClientSite for an IDocHostUIHandler
    //  * Call GetExternal on the IDocHostUIHandler to get the IDispatch

    if (SUCCEEDED(IUnknown_QueryService(this, 
                                        SID_STopLevelBrowser,
                                        IID_IServiceProvider, 
                                        (void **)&psp)))
    {
        if (SUCCEEDED(psp->QueryService(SID_SWebBrowserApp, IID_IOleObject,
                                        (void **)&pOleObject)))
        {
            if (SUCCEEDED(pOleObject->GetClientSite(&pOleClientSite)))
            {
                if (SUCCEEDED(pOleClientSite->QueryInterface(IID_IDocHostUIHandler,
                                                             (void **)&pDocHostUIHandler)))
                {
                    hr = pDocHostUIHandler->GetExternal(ppDisp);
                    pDocHostUIHandler->Release();
                }
                pOleClientSite->Release();
            }
            pOleObject->Release();
        }
        psp->Release();
    }

    return hr;
}


HRESULT CDocHostUIHandler::GetExternal(IDispatch **ppDisp)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::GetExternal called");

    HRESULT hr;

    if (ppDisp)
    {
        IDispatch *psuihDisp;
        IDispatch *pAltExternalDisp;

        *ppDisp = NULL;

        GetAltExternal(&pAltExternalDisp);

        hr = CShellUIHelper_CreateInstance2((IUnknown **)&psuihDisp, IID_IDispatch,
                                           (IUnknown *)this, pAltExternalDisp);
        if (SUCCEEDED(hr))
        {
            *ppDisp = psuihDisp;

            if (pAltExternalDisp)
            {
                //  Don't hold a ref - the ShellUIHelper will do it
                pAltExternalDisp->Release();
            }
        }
        else if (pAltExternalDisp)
        {
            //  Couldn't create a ShellUIHelper but we got our host's
            //  external.
            *ppDisp = pAltExternalDisp;
            hr = S_OK;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    ASSERT((SUCCEEDED(hr) && (*ppDisp)) || (FAILED(hr)));
    return hr;
}


HRESULT CDocHostUIHandler::TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::TranslateUrl called");

    return S_FALSE;
}


HRESULT CDocHostUIHandler::FilterDataObject(IDataObject *pDO, IDataObject **ppDORet)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::FilterDataObject called");

    return S_FALSE;
}
