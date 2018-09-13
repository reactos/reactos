//+----------------------------------------------------------------------------
//
//  HTML property pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:      view.cpp
//
//  Contents:  CPropView view object class implimentation
//
//  History:   22-Jan-97 EricB      Created.
//
//-----------------------------------------------------------------------------
#include "pch.h"
#include "view.h"
#include "SiteObj.h"
#pragma hdrstop

#define WM_USER_DOINIT (WM_USER + 1000)
#define WM_USER_CONNECTSINK (WM_USER + 1001)

const TCHAR c_szCtrlClass[] = TEXT("HtmlPropViewClass");
const TCHAR c_szCtrlName[]  = TEXT("HtmlPropViewName");

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::Create
//
//  Sysnopsis:  Creates an instance of the web view host window.
//
//-----------------------------------------------------------------------------
HRESULT
CPropView::Create(HWND hWndOwner, HINSTANCE hInstance, LPCTSTR pszUrl,
                  CPropView ** ppObj)
{
    HRESULT hr;
    WNDCLASS wc;

    CPropView * pView = new CPropView;
    if (pView == NULL)
    {
        TRACE(TEXT("CPropView::Create object allocation failed\n"));
        *ppObj = NULL;
        return E_OUTOFMEMORY;
    }

    pView->m_hInst = hInstance;
    pView->m_pszUrl = pszUrl;

    //
    // The class isn't registered, so register it.
    //
    wc.style         = 0;
    wc.lpfnWndProc   = CPropView::StaticWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = c_szCtrlClass;
    RegisterClass(&wc);

    pView->m_hWnd = CreateWindow(c_szCtrlClass,
                                 c_szCtrlName,
                                 WS_CHILD,
                                 0, 0, 0, 0,
                                 hWndOwner,
                                 (HMENU)1,           // child ID
                                 hInstance,
                                 pView);            // lpParam

    if (pView->m_hWnd == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        TRACE(TEXT("CPropView::Create: CreateWindow failed with error 0x%x\n"), hr);
        delete pView;
        *ppObj = NULL;
        return hr;
    }

    ShowWindow(pView->m_hWnd, SW_SHOW);
    UpdateWindow(pView->m_hWnd);

    *ppObj = pView;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::CPropView
//
//-----------------------------------------------------------------------------
CPropView::CPropView() :
    m_cRef(0),
    m_hWnd(NULL),
    m_pSite(NULL),
    m_pszUrl(NULL),
    m_pIOleIPActiveObject(NULL),
    m_fCreated(FALSE)
{
#ifdef _DEBUG
    strcpy(szClass, "CPropView");
#endif
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::~CPropView
//
//-----------------------------------------------------------------------------
CPropView::~CPropView()
{
    ASSERT(m_cRef == 0);
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::CreateDocObject
//
//  Sysnopsis:  Create and initialise the site for the HTML Doc Object.
//
//-----------------------------------------------------------------------------
BOOL
CPropView::CreateDocObject(LPCTSTR pchPath)
{    
    m_pSite = new CSite(m_hWnd, this);

    if (NULL == m_pSite)
    {
        return FALSE;
    }

    m_pSite->AddRef();  //So we can free with Release

    /*
     * Now tell the site to create an object in it using the filename
     * and the storage we opened.  The site will create a sub-storage
     * for the doc object's use.
     */

    // Ask the Site to Create the Activex Document
    //

    if (!m_pSite->Create(pchPath))
    {
        return FALSE;
    }

    // We created the thing, now activate it with "Show"
    //
    m_pSite->Activate(OLEIVERB_SHOW);

    // Send command to Trident to set it into Browse mode.
    // This may not be needed since it appears to be the default.
    //ExecCommand( IDM_BROWSEMODE );

    // Post a message to connect the event sinks. This has to be done after
    // Trident has fully loaded the URL, hence the post message.
    // BUGBUG: it would be better to detect the load completed event on the
    // document.
    PostMessage(m_hWnd, WM_USER_CONNECTSINK, 0, 0);
    
    return TRUE;        
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IOleInPlaceFrame::GetWindow
//
//  Sysnopsis:  Retrieves the handle of the window associated with the object
//              on which this interface is implemented.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CPropView::GetWindow(HWND * phWnd)
{
    *phWnd = m_hWnd;

    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IOleInPlaceFrame::ContextSensitiveHelp
//
//  Sysnopsis:  Instructs the object on which this interface is implemented to
//              enter or leave a context-sensitive help mode.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CPropView::ContextSensitiveHelp(BOOL fEnterMode)
{
    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IOleInPlaceFrame::GetBorder
//
//  Sysnopsis:  Returns the rectangle in which the container is willing to
//              negotiate about an object's adornments.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CPropView::GetBorder(LPRECT prcBorder)
{
    if (NULL == prcBorder)
	{
        return E_INVALIDARG;
	}

    //We return all the client area space
    //
    GetClientRect(m_hWnd, prcBorder);
    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IOleInPlaceFrame::RequestBorderSpace
//
//  Sysnopsis:  Asks the container if it can surrender the amount of space
//              in pBW that the object would like for it's adornments.  The
//              container does nothing but validate the spaces on this call.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CPropView::RequestBorderSpace(LPCBORDERWIDTHS /*pBW*/)
{
    // We have no border space restrictions
    //
    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IOleInPlaceFrame::SetBorderSpace
//
//  Sysnopsis:  Called when the object now officially requests that the
//              container surrender border space it previously allowed
//              in RequestBorderSpace.  The container should resize windows
//              appropriately to surrender this space.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CPropView::SetBorderSpace(LPCBORDERWIDTHS /*pBW*/)
{
	// We turn off the Trident UI so we ignore all of this.
    //
    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IOleInPlaceFrame::SetActiveObject
//
//  Sysnopsis:  Provides the container with the object's
//              IOleInPlaceActiveObject pointer.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CPropView::SetActiveObject(LPOLEINPLACEACTIVEOBJECT pIIPActiveObj,
                           LPCOLESTR /*pszObj*/)
{
	// If we already have an active Object then release it.
    if (NULL != m_pIOleIPActiveObject)
	{
        m_pIOleIPActiveObject->Release();
	}

    //NULLs m_pIOleIPActiveObject if pIIPActiveObj is NULL
    m_pIOleIPActiveObject = pIIPActiveObj;

    if (NULL != m_pIOleIPActiveObject)
	{
        m_pIOleIPActiveObject->AddRef();
		m_pIOleIPActiveObject->GetWindow(&m_hWndObj);
	}
    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IOleInPlaceFrame::InsertMenus
//
//  Sysnopsis:  Instructs the container to place its in-place menu items where
//              necessary in the given menu and to fill in elements 0, 2, and 4
//              of the OLEMENUGROUPWIDTHS array to indicate how many top-level
//              items are in each group.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CPropView::InsertMenus(HMENU /*hMenu*/, LPOLEMENUGROUPWIDTHS /*pMGW*/)
{
	// Trident Menus are  turned off so we don't expect any merging to go on.
    //
	return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IOleInPlaceFrame::SetMenu
//
//  Sysnopsis:  Instructs the container to replace whatever menu it's currently
//              using with the given menu and to call OleSetMenuDescritor so
//              OLE knows to whom to dispatch messages.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CPropView::SetMenu(HMENU /*hMenu*/, HOLEMENU /*hOLEMenu*/, HWND /*hWndObj*/)
{
	// Trident Menus are  turned off so we don't expect any merging to go on.
    //
	return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IOleInPlaceFrame::RemoveMenus
//
//  Sysnopsis:  Asks the container to remove any menus it put into hMenu in
//              InsertMenus.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CPropView::RemoveMenus(HMENU /*hMenu*/)
{
	// Trident Menus are  turned off so we don't expect any merging to go on.
    //
	return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IOleInPlaceFrame::SetStatusText
//
//  Sysnopsis:  Asks the container to place some text in a status line, if one
//              exists.  If the container does not have a status line it
//              should return E_FAIL here in which case the object could
//              display its own.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CPropView::SetStatusText(LPCOLESTR pszText)
{
    return E_FAIL;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IOleInPlaceFrame::EnableModeless
//
//  Sysnopsis:  Instructs the container to show or hide any modeless popup
//              windows that it may be using.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CPropView::EnableModeless(BOOL /*fEnable*/)
{
    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IOleInPlaceFrame::TranslateAccelerator
//
//  Sysnopsis:  When dealing with an in-place object from an EXE server, this
//              is called to give the container a chance to process accelerators
//              after the server has looked at the message.
//
// Parameters:
//  pMSG            LPMSG for the container to examine.
//  wID             WORD the identifier in the container's
//                  accelerator table (from IOleInPlaceSite
//                  ::GetWindowContext) for this message (OLE does
//                  some translation before calling).
//
// Return Value:
//  HRESULT         NOERROR if the keystroke was used,
//                  S_FALSE otherwise.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CPropView::TranslateAccelerator(LPMSG /*pMSG*/, WORD /*wID*/)
{
	// this could be forwarded to the top level frame
    return S_FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IOleCommandTarget:: QueryStatus
//
//  Sysnopsis:  
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CPropView::QueryStatus(const GUID* pguidCmdGroup, ULONG cCmds,
				       OLECMD * prgCmds, OLECMDTEXT * pCmdText)
{
    if (pguidCmdGroup != NULL)
	{
		// It's a nonstandard group!!
        return OLECMDERR_E_UNKNOWNGROUP;
	}

    MSOCMD*     pCmd;
    INT         c;
    HRESULT     hr = S_OK;

    // By default command text is NOT SUPPORTED.
    if (pCmdText && (pCmdText->cmdtextf != OLECMDTEXTF_NONE))
	{
        pCmdText->cwActual = 0;
	}

    // Loop through each command in the ary, setting the status of each.
    for (pCmd = prgCmds, c = cCmds; --c >= 0; pCmd++)
    {
        // By default command status is NOT SUPPORTED.
        pCmd->cmdf = 0;

        switch (pCmd->cmdID)
        {
			case OLECMDID_SETPROGRESSTEXT:
			case OLECMDID_SETTITLE:
				pCmd->cmdf = OLECMDF_SUPPORTED;
				break;
        }
    }

    return (hr);
}
        
//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IOleCommandTarget::Exec
//
//  Sysnopsis:  
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CPropView::Exec(const GUID * pguidCmdGroup, DWORD nCmdID, DWORD /*nCmdexecopt*/,
                VARIANTARG * pvaIn, VARIANTARG * /*pvaOut*/)
{
    HRESULT hr = S_OK;

    if (pguidCmdGroup == NULL)
    {
        //USES_CONVERSION;
#ifdef YANK
        switch (nCmdID)
        {
			case OLECMDID_SAVE:
				// We don't support any save stuff!
				hr = OLECMDERR_E_NOTSUPPORTED;
				break;

			case OLECMDID_SETPROGRESSTEXT:
				if (pvaIn && V_VT(pvaIn) == VT_BSTR)
				{
					CFrameWnd* pFrame = GetTopLevelFrame();
					if (pFrame != NULL)
					{
						pFrame->SetMessageText(OLE2T(V_BSTR(pvaIn)));
					}
				}
				else
				{
					hr = OLECMDERR_E_NOTSUPPORTED;
				}
				break;

			case OLECMDID_UPDATECOMMANDS:
				// MFC updates stuff in it's idle so we don't bother forcing the update here
				hr = OLECMDERR_E_NOTSUPPORTED;
				break;

			case OLECMDID_SETTITLE:
				if (pvaIn && V_VT(pvaIn) == VT_BSTR)
				{
					CCarrotDoc* pDoc = GetDocument();
					ASSERT_VALID(pDoc);

					pDoc->SetTitle(OLE2T(V_BSTR(pvaIn)));
				}
				else
				{
					hr = OLECMDERR_E_NOTSUPPORTED;
				}
				break;

			default:
				hr = OLECMDERR_E_NOTSUPPORTED;
				break;
        }
#endif // YANK
        hr = OLECMDERR_E_NOTSUPPORTED;
    }
    else
    {
        hr = OLECMDERR_E_UNKNOWNGROUP;
    }
    return hr;
}

//+----------------------------------------------------------------------------
// Helper functions on IOleCommandTarget of the object
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::GetCommandStatus
//
//  Sysnopsis:  
//
//-----------------------------------------------------------------------------
DWORD
CPropView::GetCommandStatus(ULONG ucmdID)
{
	DWORD dwReturn = 0;
	if (m_pSite != NULL)
	{
		LPOLECOMMANDTARGET pCommandTarget = m_pSite->GetCommandTarget();
		if (pCommandTarget != NULL)
		{
			HRESULT hr = S_OK;
			MSOCMD msocmd;
			msocmd.cmdID = ucmdID;
			msocmd.cmdf  = 0;
			hr = pCommandTarget->QueryStatus(&CMDSETID_Forms3, 1, &msocmd, NULL);
			dwReturn = msocmd.cmdf;
		}
	}
	return dwReturn;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::ExecCommand
//
//  Sysnopsis:  
//
//-----------------------------------------------------------------------------
void
CPropView::ExecCommand(ULONG ucmdID)
{
	if (m_pSite != NULL)
	{
		LPOLECOMMANDTARGET pCommandTarget = m_pSite->GetCommandTarget();

		if (pCommandTarget != NULL)
		{
			HRESULT hr = S_OK;
		
			hr = pCommandTarget->Exec(&CMDSETID_Forms3,
						ucmdID,
						MSOCMDEXECOPT_DONTPROMPTUSER,
						NULL,
						NULL);
		}
	}

}

//+----------------------------------------------------------------------------
// CPropView message handlers
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::OnCreate
//
//-----------------------------------------------------------------------------
int CPropView::OnCreate(HWND hWnd) 
{
    //
    // Post a message to complete the initialization.
    //
    PostMessage(hWnd, WM_USER_DOINIT, 0, 0);

	return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::OnDoInit
//
//-----------------------------------------------------------------------------
LRESULT
CPropView::OnDoInit()
{
    // create the Trident ActiveX Document
    //
    if (m_fCreated == FALSE)
    {
        m_fCreated = TRUE;

        if (!CreateDocObject(m_pszUrl))
        {
            MessageBox(m_hWnd, TEXT("Invalid URL"), TEXT("Error"), MB_ICONEXCLAMATION);
        }

        TRACE(TEXT("CPropView::OnDoInit: calling OnSize\n"));
        OnSize();
    }

    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::OnSize
//
//  Sysnopsis:  Tell the site to tell the object.
//
//-----------------------------------------------------------------------------
void
CPropView::OnSize(void)
{
	if (NULL != m_pSite)
	{
        TRACE(TEXT("CPropView::OnSize: calling m_pSite->UpdateObjectRects\n"));
    	m_pSite->UpdateObjectRects();
	}
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::OnSetFocus
//
//-----------------------------------------------------------------------------
void
CPropView::OnSetFocus(void)
{
	// Give the focus to the ActiveX Document window
    if (m_hWndObj != NULL)
	{
		::SetFocus(m_hWndObj);
	}
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::OnPaint
//
//-----------------------------------------------------------------------------
void
CPropView::OnPaint() 
{
#ifdef YANK
    if (m_pSite != NULL)
    {
        HDC hDC = GetDC(m_hWnd);
        m_pSite->Draw(hDC);
        ReleaseDC(m_hWnd, hDC);
    }
#endif // YANK
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::OnDestroy
//
//  Sysnopsis:  On view window shutdown, close the site object.
//
//-----------------------------------------------------------------------------
void
CPropView::OnDestroy(void)
{
    if (m_pSite != NULL)
    {
        CSite *pSite = m_pSite; //Prevents reentry
        m_pSite = NULL;

        pSite->Close(); // Closes object

        ReleaseInterface(pSite);
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::StaticWndProc
//
//  Sysnopsis:  static window procedure
//
//-----------------------------------------------------------------------------
long CALLBACK
CPropView::StaticWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CPropView *pThis = (CPropView *)GetWindowLong(hWnd, GWL_USERDATA);

    if (uMsg == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;

        pThis = (CPropView *) pcs->lpCreateParams;

        SetWindowLong(hWnd, GWL_USERDATA, (LONG)pThis);
    }

    if (pThis != NULL)
    {
        return pThis->WndProc(hWnd, uMsg, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::WndProc
//
//  Sysnopsis:  per-instance window proc
//
//-----------------------------------------------------------------------------
LRESULT
CPropView::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lr;
    switch (uMsg)
    {
    case WM_CREATE:
        lr = OnCreate(hWnd);
        return lr;

    case WM_USER_DOINIT:
        return OnDoInit();

    case WM_USER_CONNECTSINK:
        m_pSite->ConnectSink();
        return 0;

    case WM_SETFOCUS:
        OnSetFocus();
        break;

//    case WM_SHOWWINDOW:
//    case WM_MOVE:
//    case WM_WINDOWPOSCHANGED:
    case WM_SIZE:
        OnSize();
        break;

    case WM_DESTROY:
        OnDestroy();
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IUnknown::QueryInterface
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CPropView::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (IID_IUnknown == riid || IID_IOleInPlaceUIWindow == riid ||
        IID_IOleWindow == riid || IID_IOleInPlaceFrame == riid)
	{
        *ppv = (IOleInPlaceFrame *)this;
	}

	if (IID_IOleCommandTarget == riid)
	{
        *ppv = (IOleCommandTarget *)this;
	}

    if (NULL != *ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IUnknown::AddRef
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CPropView::AddRef(void)
{
    return ++m_cRef;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::IUnknown::Release
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CPropView::Release(void)
{
	// Debug check to see we don't fall below 0
	ASSERT(m_cRef != 0);

    return --m_cRef;

    if (0 != --m_cRef)
    {
        return m_cRef;
    }

    delete this;

    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CPropView::OnApply
//
//  Sysnopsis:  On Apply button press, call the DoApply script subroutine on
//              the loaded HTML page.
//
//-----------------------------------------------------------------------------
void 
CPropView::OnApply(void)
{
    // Note, GetObjectUnknown doesn't increment the refcount, so a release
    // is not needed.
    //
    IUnknown * pUnk = m_pSite->GetObjectUnknown();

    if (pUnk == NULL)
    {
        TRACE(TEXT("CPropView::OnApply: couldn't get Trident IUnknown!\n"));
        return;
    }

    HRESULT hr = S_OK;
    IHTMLDocument * pHTMLDocument;

    hr = pUnk->QueryInterface(IID_IHTMLDocument, (void **)&pHTMLDocument);
    if (FAILED(hr))
    {
        TRACE(TEXT("CPropView::OnApply: QI(IID_IHTMLDocument) failed with error 0x%x\n"), hr);
        return;
    }

    IDispatch * pDispatch;

    hr = pHTMLDocument->get_Script(&pDispatch);
    if (FAILED(hr))
    {
        pHTMLDocument->Release();
        TRACE(TEXT("CPropView::OnApply: get_Script failed with error 0x%x\n"), hr);
        return;
    }

    OLECHAR * pszName = L"DoApply";
    DISPID dispid;

    hr = pDispatch->GetIDsOfNames(IID_NULL, &pszName, 1, LOCALE_SYSTEM_DEFAULT,
                                  &dispid);
    if (FAILED(hr))
    {
        pDispatch->Release();
        pHTMLDocument->Release();
        if (hr == DISP_E_UNKNOWNNAME)
        {
            //
            // No DoApply sub on the page.
            //
            return;
        }
        TRACE(TEXT("CPropView::OnApply: GetIdsOfName failed with error 0x%x\n"), hr);
        return;
    }

    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};

    hr = pDispatch->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                           DISPATCH_METHOD, &dispparamsNoArgs, NULL, NULL,
                           NULL);
    if (FAILED(hr))
    {
        TRACE(TEXT("CPropView::OnApply: Invoke failed with error 0x%x\n"), hr);
    }

    pDispatch->Release();

    pHTMLDocument->Release();
}


