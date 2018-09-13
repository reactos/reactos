//	Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
//	Notes on m_bfModeSwitched and m_bfReloadAttempted.
//	IE5 bug 52818 was punted; pages containing IFrames don't refresh when changing
//	browse/edit modes, because the stream is seen as dirty (because the IFrame
//	considers itself dirty.)  In response, we set m_bfModeSwitched when changing mode,
//	m_bfReloadAttempted when and ATTEMPT is made to reload the page, and check for BOTH
//	in OnReadyStateChanged.  If the mode was changed but the page wasn't reloaded,
//	we have to reload it manually.


#include "stdafx.h"
#include "DHTMLEd.h"
#include "DHTMLEdit.h"
#include "site.h"
#include "proxyframe.h"
#include <TRIEDIID.h>
#include <mshtmdid.h>
#include "dispexa.h"
#include <wchar.h>
#include <string.h>


// HTML to initialize Trident with if the host didn't supply any
// The <P>&nbsp;</P> works around a nasty Trident bug.
// Change: now there is one with paragraphs, one with DIVs.
//
static WCHAR* g_initialHTMLwithP = \
L"<HTML>\r\n\
<HEAD>\r\n\
<META NAME=\"GENERATOR\" Content=\"Microsoft DHTML Editing Control\">\r\n\
<TITLE></TITLE>\r\n\
</HEAD>\r\n\
<BODY>\r\n\
<P>&nbsp;</P>\r\n\
</BODY>\r\n\
</HTML>\r\n";

static WCHAR* g_initialHTMLwithDIV = \
L"<HTML>\r\n\
<HEAD>\r\n\
<META NAME=\"GENERATOR\" Content=\"Microsoft DHTML Editing Control\">\r\n\
<TITLE></TITLE>\r\n\
</HEAD>\r\n\
<BODY>\r\n\
<DIV>&nbsp;</DIV>\r\n\
</BODY>\r\n\
</HTML>\r\n";


//	Text, numbers and constants used to construct a unique-per-process protocol ID
//
static WCHAR* g_wszProtocolPrefix = L"DHTMLEd";
static int	s_iProtocolSuffix = 0;
#define MAX_PROTOCOL_SUFFIX	999999


// Name of the Title property which we get from the IHtmlDocument2 interface.
static WCHAR* 	g_wszHTMLTitlePropName = L"title";


//	Maps private DHTMLEdit command IDs to Triedit command IDs.
//	The third field is true if the command includes an out parameter.
//
static CommandMap cmdMap[] = 
{
	{DECMD_BOLD,				IDM_TRIED_BOLD,				FALSE},
	{DECMD_COPY,				IDM_TRIED_COPY,				FALSE},
	{DECMD_CUT,					IDM_TRIED_CUT,				FALSE},
	{DECMD_DELETE,				IDM_TRIED_DELETE,			FALSE},
	{DECMD_DELETECELLS,			IDM_TRIED_DELETECELLS,		FALSE},
	{DECMD_DELETECOLS,			IDM_TRIED_DELETECOLS,		FALSE},
	{DECMD_DELETEROWS,			IDM_TRIED_DELETEROWS,		FALSE},
	{DECMD_FINDTEXT,			IDM_TRIED_FIND,				FALSE},
	{DECMD_FONT,				IDM_TRIED_FONT,				FALSE},
	{DECMD_GETBACKCOLOR,		IDM_TRIED_BACKCOLOR,		TRUE},
	{DECMD_GETBLOCKFMT,			IDM_TRIED_BLOCKFMT,			TRUE},
	{DECMD_GETBLOCKFMTNAMES,	IDM_TRIED_GETBLOCKFMTS,		TRUE},
	{DECMD_GETFONTNAME,			IDM_TRIED_FONTNAME,			TRUE},
	{DECMD_GETFONTSIZE,			IDM_TRIED_FONTSIZE,			TRUE},
	{DECMD_GETFORECOLOR,		IDM_TRIED_FORECOLOR,		TRUE},
	{DECMD_HYPERLINK,			IDM_TRIED_HYPERLINK,		FALSE},
	{DECMD_IMAGE,				IDM_TRIED_IMAGE,			FALSE},
	{DECMD_INDENT,				IDM_TRIED_INDENT,			FALSE},
	{DECMD_INSERTCELL,			IDM_TRIED_INSERTCELL,		FALSE},
	{DECMD_INSERTCOL,			IDM_TRIED_INSERTCOL,		FALSE},
	{DECMD_INSERTROW,			IDM_TRIED_INSERTROW,		FALSE},
	{DECMD_INSERTTABLE,			IDM_TRIED_INSERTTABLE,		FALSE},
	{DECMD_ITALIC,				IDM_TRIED_ITALIC,			FALSE},
	{DECMD_JUSTIFYLEFT,			IDM_TRIED_JUSTIFYLEFT,		FALSE},
	{DECMD_JUSTIFYRIGHT,		IDM_TRIED_JUSTIFYRIGHT,		FALSE},
	{DECMD_JUSTIFYCENTER,		IDM_TRIED_JUSTIFYCENTER,	FALSE},
	{DECMD_LOCK_ELEMENT,		IDM_TRIED_LOCK_ELEMENT,		FALSE},
	{DECMD_MAKE_ABSOLUTE,		IDM_TRIED_MAKE_ABSOLUTE,	FALSE},
	{DECMD_MERGECELLS,			IDM_TRIED_MERGECELLS,		FALSE},
	{DECMD_ORDERLIST,			IDM_TRIED_ORDERLIST,		FALSE},
	{DECMD_OUTDENT,				IDM_TRIED_OUTDENT,			FALSE},
	{DECMD_PASTE,				IDM_TRIED_PASTE,			FALSE},
	{DECMD_REDO,				IDM_TRIED_REDO,				FALSE},
	{DECMD_REMOVEFORMAT,		IDM_TRIED_REMOVEFORMAT,		FALSE},
	{DECMD_SELECTALL,			IDM_TRIED_SELECTALL,		FALSE},
	{DECMD_SEND_BACKWARD,		IDM_TRIED_SEND_BACKWARD,	FALSE},
	{DECMD_BRING_FORWARD,		IDM_TRIED_SEND_FORWARD,		FALSE},
	{DECMD_SEND_BELOW_TEXT,		IDM_TRIED_SEND_BEHIND_1D,	FALSE},
	{DECMD_BRING_ABOVE_TEXT,	IDM_TRIED_SEND_FRONT_1D,	FALSE},
	{DECMD_SEND_TO_BACK,		IDM_TRIED_SEND_TO_BACK,		FALSE},
	{DECMD_BRING_TO_FRONT,		IDM_TRIED_SEND_TO_FRONT,	FALSE},
	{DECMD_SETBACKCOLOR,		IDM_TRIED_BACKCOLOR,		FALSE},
	{DECMD_SETBLOCKFMT,			IDM_TRIED_BLOCKFMT,			FALSE},
	{DECMD_SETFONTNAME,			IDM_TRIED_FONTNAME,			FALSE},
	{DECMD_SETFONTSIZE,			IDM_TRIED_FONTSIZE,			FALSE},
	{DECMD_SETFORECOLOR,		IDM_TRIED_FORECOLOR,		FALSE},
	{DECMD_SPLITCELL,			IDM_TRIED_SPLITCELL,		FALSE},
	{DECMD_UNDERLINE,			IDM_TRIED_UNDERLINE,		FALSE},
	{DECMD_UNDO,				IDM_TRIED_UNDO,				FALSE},
	{DECMD_UNLINK,				IDM_TRIED_UNLINK,			FALSE},
	{DECMD_UNORDERLIST,			IDM_TRIED_UNORDERLIST,		FALSE},
	{DECMD_PROPERTIES,			IDM_TRIED_DOVERB,			FALSE}
};



CProxyFrame::CProxyFrame(CDHTMLSafe* pCtl)
{
	SAFEARRAYBOUND rgsabound[1] = {0};

	_ASSERTE(pCtl);

	m_cRef = 1;

	m_pUnkTriEdit = NULL;
	m_hWndObj = NULL;
	m_pIOleIPActiveObject = NULL;
	m_pSite = NULL;
	m_pCtl = pCtl;

	m_fCreated = FALSE;
	m_fActivated = FALSE;
	m_state = ESTATE_NOTCREATED;
	m_readyState = READYSTATE_UNINITIALIZED;
	m_dwFilterFlags = m_dwFilterOutFlags = filterAll;

	m_fActivateApplets = FALSE;
	m_fActivateControls = FALSE;
	m_fActivateDTCs = TRUE;
	m_fShowAllTags = FALSE;
	m_fShowBorders = FALSE;

	m_fDialogEditing = TRUE;
	m_fDisplay3D = TRUE;
	m_fScrollbars = TRUE;
	m_fDisplayFlatScrollbars = FALSE;
	m_fContextMenu = TRUE;

	m_fPreserveSource = TRUE;

	m_fAbsoluteDropMode = FALSE;
	m_fSnapToGrid = FALSE;
	m_ulSnapToGridX = 50;
	m_ulSnapToGridY = 50;

	rgsabound[0].lLbound = 0;
	rgsabound[0].cElements = 0;

	m_pMenuStrings = NULL;	
	m_pMenuStates = NULL;	

	m_vbBrowseMode = VARIANT_FALSE;

	m_vbUseDivOnCr = VARIANT_FALSE;
	m_bfIsLoading  = FALSE;
	m_bfBaseURLFromBASETag = FALSE;
	m_bfPreserveDirtyFlagAcrossBrowseMode = FALSE;
	m_bstrInitialDoc.Empty ();

	m_bstrCurDocPath.Empty ();

	wcscpy ( m_wszProtocol, g_wszProtocolPrefix );
	WCHAR wszSuffix[8];
	_itow ( s_iProtocolSuffix++, wszSuffix, 10 );
	if ( MAX_PROTOCOL_SUFFIX <= s_iProtocolSuffix )
	{
		s_iProtocolSuffix = 0;	// Roll over.
	}
	wcscat ( m_wszProtocol, wszSuffix );
	wcscpy ( m_wszProtocolPrefix, m_wszProtocol );
	wcscat ( m_wszProtocolPrefix, L":" );

	m_pProtInfo = NULL;
	m_bfIsURL   = FALSE;
	m_bstrBaseURL = L"";
	m_hwndRestoreFocus = NULL;

#ifdef LATE_BIND_URLMON_WININET
	m_hUlrMon					= NULL;
	m_hWinINet					= NULL;
	m_pfnCoInternetCombineUrl	= NULL;
	m_pfnCoInternetParseUrl		= NULL;
	m_pfnCreateURLMoniker		= NULL;
	m_pfnCoInternetGetSession	= NULL;
	m_pfnURLOpenBlockingStream	= NULL;

	m_pfnDeleteUrlCacheEntry	= NULL;
	m_pfnInternetCreateUrl		= NULL;
	m_pfnInternetCrackUrl		= NULL;
#endif // LATE_BIND_URLMON_WININET

	m_bfModeSwitched	= FALSE;
	m_bfReloadAttempted	= FALSE;
	m_bfSFSRedirect		= FALSE;
}

CProxyFrame::~CProxyFrame()
{
	_ASSERTE(FALSE == m_fCreated);
	_ASSERTE(FALSE == m_fActivated);
	_ASSERTE( m_cRef == 0 );

	if (m_pMenuStrings)
	{
		SafeArrayDestroy(m_pMenuStrings);
		m_pMenuStrings = NULL;
	}

	if (m_pMenuStates)
	{
		SafeArrayDestroy(m_pMenuStates);
		m_pMenuStates = NULL;
	}

	// This should never happen: SetActiveObject should take care of it.
	_ASSERTE ( NULL == m_pIOleIPActiveObject );
	if (m_pIOleIPActiveObject)
	{
		m_pIOleIPActiveObject->Release();
		m_pIOleIPActiveObject = NULL;
	}

	UnRegisterPluggableProtocol ();
#ifdef LATE_BIND_URLMON_WININET
	DynUnloadLibraries ();
#endif // LATE_BIND_URLMON_WININET
}


//	Create the TriEdit object and host it.
//	Clean up and return an error if there was any problem.
//
HRESULT
CProxyFrame::Init(IUnknown* pUnk, IUnknown** ppUnkTriEdit)
{
	HRESULT hr = S_OK;

#ifdef LATE_BIND_URLMON_WININET
	if ( ! DynLoadLibraries () )
	{
		return E_FAIL;
	}
#endif // LATE_BIND_URLMON_WININET

	hr = RegisterPluggableProtocol ();
	if ( FAILED ( hr ) )
	{
		return hr;
	}

	_ASSERTE(NULL == m_pSite);
	_ASSERTE(GetState() == ESTATE_NOTCREATED);

	InitializeDocString ();

	if (m_pSite)
		return E_UNEXPECTED;

	if (GetState() != ESTATE_NOTCREATED)
		return E_UNEXPECTED;

	// Create and initialize the site for TriEdit
	m_pSite = new CSite(this);

    if (NULL == m_pSite)
	{
        return E_OUTOFMEMORY;
	}

    m_pSite->AddRef();  // So we can free with Release

	// Ask the site to create TriEdit
    if (SUCCEEDED(hr = m_pSite->HrCreate(pUnk, &m_pUnkTriEdit)))
	{
		ChangeState(ESTATE_CREATED);
		m_fCreated = TRUE;
		if (ppUnkTriEdit)
		{
			m_pUnkTriEdit->AddRef();
			*ppUnkTriEdit = m_pUnkTriEdit;
		}
	}
	else
	{
		m_pSite->Release();
		m_pSite = NULL;
	}

    return hr;        
}


//	Destroy the site and the TriEdit object.
//
HRESULT
CProxyFrame::Close()
{
	HRESULT hr = S_OK;

	_ASSERTE(m_pUnkTriEdit);
	_ASSERTE(m_pSite);
	_ASSERTE(GetState() != ESTATE_NOTCREATED);

	m_bstrCurDocPath.Empty ();

	// triedit must be created
	// any state from created to activated is ok
	if (GetState() == ESTATE_NOTCREATED)
		return E_UNEXPECTED;

	if (m_fActivated)
	{
		hr = HrExecCommand(&CGID_MSHTML, IDM_STOP, MSOCMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
		_ASSERTE(SUCCEEDED(hr));
	}

	ChangeState(ESTATE_NOTCREATED);
	m_fCreated = FALSE;
	m_fActivated = FALSE;
    if (m_pSite != NULL)
	{
		CSite* pSite = m_pSite; // prevents reentry;
		m_pSite = NULL;

		pSite->Close(FALSE);
		ReleaseInterface(pSite)
		pSite = NULL;
	}

	if (m_pUnkTriEdit != NULL)
	{
		LPUNKNOWN pUnkTriEdit = m_pUnkTriEdit;
		m_pUnkTriEdit = NULL;

		ReleaseInterface(pUnkTriEdit);
		pUnkTriEdit = NULL;
	}

	m_hwndRestoreFocus = NULL;

	return S_OK;
}


//	Determine which string constant to use and return a pointer to it.
//
WCHAR* CProxyFrame::GetInitialHTML ()
{
	if ( m_vbUseDivOnCr )
	{
		return g_initialHTMLwithDIV;
	}
	else
	{
		return g_initialHTMLwithP;
	}
}


//	Perform these steps before loading TriEdit's contents
//
HRESULT
CProxyFrame::PreActivate()
{
	HRESULT hr = S_OK;
	_ASSERTE(m_pSite);
	_ASSERTE(m_pCtl);

	_ASSERTE(ESTATE_CREATED == GetState());

	if (GetState() != ESTATE_CREATED)
		return E_UNEXPECTED;


	if (FAILED(hr = m_pSite->HrObjectInitialize()))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto error;
	}

	m_fActivated = TRUE;
	ChangeState(ESTATE_PREACTIVATING);

	if (FAILED(hr = HrSetRuntimeProperties()))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto error;
	}

	ChangeState(ESTATE_ACTIVATING);

error:

	return hr;
}


//	Perform these steps after loading TriEdits contents to go UI active.
//
HRESULT
CProxyFrame::Activate()
{
	HRESULT hr = S_OK;

	_ASSERTE(m_pSite);
	_ASSERTE(m_pCtl);
	_ASSERTE(m_pCtl->m_hWndCD);

	_ASSERTE(GetState() == ESTATE_ACTIVATING);

	if (GetState() != ESTATE_ACTIVATING)
		return E_UNEXPECTED;

    // activate Trident with "Show"
	m_pSite->InitialActivate(OLEIVERB_SHOW, m_pCtl->m_hWndCD);


	ChangeState(ESTATE_ACTIVATED);

	// This may have been deferred, because the site's command target did not yet exist...
	SetBrowseMode ( m_vbBrowseMode );

	return hr;
}


//	Load and activate the control with a minimal, empty page.
//
HRESULT
CProxyFrame::LoadInitialDoc()
{
	HRESULT hr = S_OK;

	_ASSERTE(GetState() == ESTATE_CREATED);

	if (GetState() != ESTATE_CREATED)
		return E_UNEXPECTED;
	
	if (FAILED(hr = PreActivate()))
		goto error;

	if (FAILED(hr = LoadBSTRDeferred ( m_bstrInitialDoc )))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto error;
	}

	if (FAILED(hr = Activate()))
		goto error;

error:

	return hr;
}


// Before getting or setting a property or calling a method on the docobject,
// assure that it's been properly activated.
//
void CProxyFrame::AssureActivated ()
{
	if ( ! m_fActivated )
	{
		if ( m_pCtl->IsUserMode() )
		{
			if ( !m_pCtl->m_bInPlaceActive )
			{
				m_pCtl->DoVerbInPlaceActivate ( NULL, NULL );
			}
			LoadInitialDoc ();
		}
	}
}


//	Loading MSHTML shifts the focus to its document window.
//	This is not desirable in a control.  Experimentation has demonstrated
//	that the focus shifts between various QIs from MSHTML (probably in response
//	to posted messages.)  There is no routine in DHTMLEdit which enters with the	
//	focus outside the control and exits with the focus within the control.
//	Therefore, a member variable is used to preserve the appropriate focus
//	across calls to OnReadyStateChanged, which are called in response to events
//	fired by the control.  m_hwndRestoreFocus is used to preserve the appropriate
//	window to receive the focus.  Note that NULL may be appropriate, but is not honored.
//	If no window had focus, the document will gain focus.
//
void 
CProxyFrame::OnReadyStateChanged(READYSTATE readyState)
{
	_ASSERTE(m_pCtl);

	m_readyState = readyState;

	switch (m_readyState)
	{	
	case READYSTATE_UNINITIALIZED:
		{
			m_hwndRestoreFocus = NULL;
		}
		break;

	case READYSTATE_LOADING:
		{
			m_hwndRestoreFocus = ::GetFocus ();
		}
		break;
		
	case READYSTATE_LOADED:
		{
		}
		break;

	case READYSTATE_INTERACTIVE:
		{
			if ( NULL != m_hwndRestoreFocus )
			{
				_ASSERTE ( ::IsWindow ( m_hwndRestoreFocus ) );
				if ( ::IsWindow ( m_hwndRestoreFocus ) )
				{
					::SetFocus ( m_hwndRestoreFocus );
				}
			}

			// See if we failed to get a refresh on a mode change.  This happens if
			// there are IFrames on the page, perhaps in other cases as well.
			if ( m_bfModeSwitched && !m_bfReloadAttempted )
			{
				HRESULT	hr	= S_OK;

				CComPtr<IMoniker> srpMoniker;
				CComPtr<IBindCtx> srpBindCtx;
				CComQIPtr<IPersistMoniker, &IID_IPersistMoniker> srpPM (m_pUnkTriEdit);
				_ASSERTE ( srpPM );

				if ( srpPM )
				{
					CComBSTR	bstrProtocol = m_wszProtocolPrefix;

#ifdef LATE_BIND_URLMON_WININET
					_ASSERTE ( m_pfnCreateURLMoniker );
					hr = (*m_pfnCreateURLMoniker)( NULL, bstrProtocol, &srpMoniker );
#else
					hr = CreateURLMoniker ( NULL, bstrProtocol, &srpMoniker );
#endif // LATE_BIND_URLMON_WININET

					_ASSERTE ( SUCCEEDED( hr ) );
					if ( SUCCEEDED ( hr ) )
					{
						hr = ::CreateBindCtx(NULL, &srpBindCtx);
						_ASSERTE ( SUCCEEDED( hr ) );
						if ( SUCCEEDED ( hr ) )
						{
							hr = srpPM->Load(FALSE, srpMoniker,  srpBindCtx, STGM_READ);
						}
					}
				}
			}
			m_bfModeSwitched	= FALSE;
			m_bfReloadAttempted	= FALSE;
		}
		break;

	case READYSTATE_COMPLETE:
		{
			HRESULT hr		= S_OK;

			m_hwndRestoreFocus = NULL;
			if ( ! m_vbBrowseMode )
			{
				hr = HrSetDocLoadedProperties();
				_ASSERTE(SUCCEEDED(hr));
			}

			_ASSERTE ( m_pCtl->m_hWnd );
			_ASSERTE ( ::IsWindow ( m_pCtl->m_hWnd ) );

			if ( m_bfPreserveDirtyFlagAcrossBrowseMode && !m_vbBrowseMode )
			{
				m_bfPreserveDirtyFlagAcrossBrowseMode = FALSE;
				SetDirtyFlag ( TRUE );
			}
			// Post a user message to fire the DocumentComplete event.
			// Otherwise, calling things like LoadURL from DocumentComplete behaves strangely.
			::PostMessage ( m_pCtl->m_hWnd, DOCUMENT_COMPETE_MESSAGE, DOCUMENT_COMPETE_SIGNATURE, 0L );
			HrSetRuntimeProperties ();
			m_bfIsLoading = FALSE;
			SetBaseURLFromBaseHref ();	// Must be called after clearing m_bfIsLoading
		}
		break;
	}
}


/*
 * IUnknown implementation
 */
/*
 * CProxyFrame::QueryInterface
 * CProxyFrame::AddRef
 * CProxyFrame::Release
 */
STDMETHODIMP CProxyFrame::QueryInterface( REFIID riid, void **ppv )
{
    /*
     * We provide IOleInPlaceFrame and IOleCommandTarget
	 *   interfaces here for the ActiveX Document hosting
	 */
    *ppv = NULL;

    if ( IID_IUnknown == riid || IID_IOleInPlaceUIWindow == riid
        || IID_IOleWindow == riid || IID_IOleInPlaceFrame == riid )
	{
        *ppv = static_cast<IOleInPlaceFrame *>(this);
	}

	else if ( IID_IOleCommandTarget == riid )
	{
        *ppv = static_cast<IOleCommandTarget *>(this);
	}
	else if ( IID_IBindStatusCallback == riid )
	{
        *ppv = static_cast<IBindStatusCallback *>(this);
	}
	else if ( IID_IAuthenticate == riid )
	{
        *ppv = static_cast<IAuthenticate *>(this);
	}
	else if ( IID_IServiceProvider == riid )
	{
		// Ask the control for a security manager IF in edit mode:
		if ( ! m_vbBrowseMode )
		{
			return m_pCtl->GetUnknown()->QueryInterface ( riid, ppv );
		}
	}

    if ( NULL != *ppv )
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CProxyFrame::AddRef( void )
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CProxyFrame::Release( void )
{
    //Nothing special happening here-- life if user-controlled.
	// Debug check to see we don't fall below 0
	_ASSERTE( m_cRef != 0 );

	ULONG ulRefCount = --m_cRef;
	if ( 0 == ulRefCount )
	{
		delete this;	// Do not refer to any member variables after this.
	}
    return ulRefCount;
}


/*
 * IOleInPlaceFrame implementation
 */
/*
 * CProxyFrame::GetWindow
 *
 * Purpose:
 *  Retrieves the handle of the window associated with the object
 *  on which this interface is implemented.
 *
 * Parameters:
 *  phWnd           HWND * in which to store the window handle.
 *
 * Return Value:
 *  HRESULT         S_OK if successful, E_FAIL if there is no
 *                  window.
 */
STDMETHODIMP CProxyFrame::GetWindow( HWND* phWnd )
{
	if ( m_pCtl != NULL )
	{
		*phWnd = m_pCtl->m_hWnd;
	}
	return S_OK;
}



/*
 * CProxyFrame::ContextSensitiveHelp
 *
 * Purpose:
 *  Instructs the object on which this interface is implemented to
 *  enter or leave a context-sensitive help mode.
 *
 * Parameters:
 *  fEnterMode      BOOL TRUE to enter the mode, FALSE otherwise.
 *
 * Return Value:
 *  HRESULT         S_OK
 */
STDMETHODIMP CProxyFrame::ContextSensitiveHelp( BOOL /*fEnterMode*/ )
{
    return S_OK;
}



/*
 * CProxyFrame::GetBorder
 *
 * Purpose:
 *  Returns the rectangle in which the container is willing to
 *  negotiate about an object's adornments.
 *
 * Parameters:
 *  prcBorder       LPRECT in which to store the rectangle.
 *
 * Return Value:
 *  HRESULT         S_OK if all is well, INPLACE_E_NOTOOLSPACE
 *                  if there is no negotiable space.
 */
STDMETHODIMP CProxyFrame::GetBorder( LPRECT prcBorder )
{
    if ( NULL == prcBorder )
	{
        return E_INVALIDARG;
	}

    //We return all the client area space
    m_pCtl->GetClientRect( prcBorder );
    return S_OK;
}


/*
 * CProxyFrame::RequestBorderSpace
 *
 * Purpose:
 *  Asks the container if it can surrender the amount of space
 *  in pBW that the object would like for it's adornments.  The
 *  container does nothing but validate the spaces on this call.
 *
 * Parameters:
 *  pBW             LPCBORDERWIDTHS containing the requested space.
 *                  The values are the amount of space requested
 *                  from each side of the relevant window.
 *
 * Return Value:
 *  HRESULT         S_OK if we can give up space,
 *                  INPLACE_E_NOTOOLSPACE otherwise.
 */
STDMETHODIMP CProxyFrame::RequestBorderSpace( LPCBORDERWIDTHS /*pBW*/ )
{
    // We have no border space restrictions
    return S_OK;
}


/*
 * CProxyFrame::SetBorderSpace
 *
 * Purpose:
 *  Called when the object now officially requests that the
 *  container surrender border space it previously allowed
 *  in RequestBorderSpace.  The container should resize windows
 *  appropriately to surrender this space.
 *
 * Parameters:
 *  pBW             LPCBORDERWIDTHS containing the amount of space
 *                  from each side of the relevant window that the
 *                  object is now reserving.
 *
 * Return Value:
 *  HRESULT         S_OK
 */
STDMETHODIMP CProxyFrame::SetBorderSpace( LPCBORDERWIDTHS /*pBW*/ )
{
	// We turn off the MSHTML.DLL UI so we ignore all of this.

    return S_OK;
}




/*
 * CProxyFrame::SetActiveObject
 *
 * Purpose:
 *  Provides the container with the object's IOleInPlaceActiveObject
 *  pointer
 *
 * Parameters:
 *  pIIPActiveObj   LPOLEINPLACEACTIVEOBJECT of interest.
 *  pszObj          LPCOLESTR naming the object.  Not used.
 *
 * Return Value:
 *  HRESULT         S_OK
 */
STDMETHODIMP CProxyFrame::SetActiveObject( LPOLEINPLACEACTIVEOBJECT pIIPActiveObj,
											LPCOLESTR /*pszObj*/)
{
	// If we already have an active Object then release it.
    if ( NULL != m_pIOleIPActiveObject )
	{
        m_pIOleIPActiveObject->Release();
	}

    //NULLs m_pIOleIPActiveObject if pIIPActiveObj is NULL
    m_pIOleIPActiveObject = pIIPActiveObj;

    if ( NULL != m_pIOleIPActiveObject )
	{
        m_pIOleIPActiveObject->AddRef();
		m_pIOleIPActiveObject->GetWindow( &m_hWndObj );
	}
    return S_OK;
}



/*
 * CProxyFrame::InsertMenus
 *
 * Purpose:
 *  Instructs the container to place its in-place menu items where
 *  necessary in the given menu and to fill in elements 0, 2, and 4
 *  of the OLEMENUGROUPWIDTHS array to indicate how many top-level
 *  items are in each group.
 *
 * Parameters:
 *  hMenu           HMENU in which to add popups.
 *  pMGW            LPOLEMENUGROUPWIDTHS in which to store the
 *                  width of each container menu group.
 *
 * Return Value:
 *  HRESULT         E_NOTIMPL
 */
STDMETHODIMP CProxyFrame::InsertMenus( HMENU /*hMenu*/, LPOLEMENUGROUPWIDTHS /*pMGW*/ )
{
	// We've turned off the MSHTML.DLL Menus so we don't expect any merging to go on!
	return E_NOTIMPL;
}


/*
 * CProxyFrame::SetMenu
 *
 * Purpose:
 *  Instructs the container to replace whatever menu it's currently
 *  using with the given menu and to call OleSetMenuDescritor so OLE
 *  knows to whom to dispatch messages.
 *
 * Parameters:
 *  hMenu           HMENU to show.
 *  hOLEMenu        HOLEMENU to the menu descriptor.
 *  hWndObj         HWND of the active object to which messages are
 *                  dispatched.
 *
 * Return Value:
 *  HRESULT         NOERROR
 */
STDMETHODIMP CProxyFrame::SetMenu( HMENU /*hMenu*/, HOLEMENU /*hOLEMenu*/, HWND /*hWndObj*/ )
{
	// We've turned off the MSHTML.DLL Menus so we don't expect any merging to go on!
	return E_NOTIMPL;
}



/*
 * CProxyFrame::RemoveMenus
 *
 * Purpose:
 *  Asks the container to remove any menus it put into hMenu in
 *  InsertMenus.
 *
 * Parameters:
 *  hMenu           HMENU from which to remove the container's
 *                  items.
 *
 * Return Value:
 *  HRESULT         NOERROR
 */
STDMETHODIMP CProxyFrame::RemoveMenus( HMENU /*hMenu*/ )
{
	// We've turned off the MSHTML.DLL Menus so we don't expect any merging to go on!
	return E_NOTIMPL;
}


/*
 * CProxyFrame::SetStatusText
 *
 * Purpose:
 *  Asks the container to place some text in a status line, if one
 *  exists.  If the container does not have a status line it
 *  should return E_FAIL here in which case the object could
 *  display its own.
 *
 * Parameters:
 *  pszText         LPCOLESTR to display.
 *
 * Return Value:
 *  HRESULT         S_OK if successful, S_TRUNCATED if not all
 *                  of the text could be displayed, or E_FAIL if
 *                  the container has no status line.
 */
STDMETHODIMP CProxyFrame::SetStatusText( LPCOLESTR /*pszText*/ )
{
    return S_OK;
}



/*
 * CProxyFrame::EnableModeless
 *
 * Purpose:
 *  Instructs the container to show or hide any modeless popup
 *  windows that it may be using.
 *
 * Parameters:
 *  fEnable         BOOL indicating to enable/show the windows
 *                  (TRUE) or to hide them (FALSE).
 *
 * Return Value:
 *  HRESULT         S_OK
 */

STDMETHODIMP CProxyFrame::EnableModeless( BOOL /*fEnable*/ )
{
    return S_OK;
}


/*
 * CProxyFrame::TranslateAccelerator
 *
 * Purpose:
 *  When dealing with an in-place object from an EXE server, this
 *  is called to give the container a chance to process accelerators
 *  after the server has looked at the message.
 *
 * Parameters:
 *  pMSG            LPMSG for the container to examine.
 *  wID             WORD the identifier in the container's
 *                  accelerator table (from IOleInPlaceSite
 *                  ::GetWindowContext) for this message (OLE does
 *                  some translation before calling).
 *
 * Return Value:
 *  HRESULT         NOERROR if the keystroke was used,
 *                  S_FALSE otherwise.
 */
STDMETHODIMP CProxyFrame::TranslateAccelerator( LPMSG /*pMSG*/, WORD /*wID*/ )
{
    return S_FALSE;
}


/*
 * IOleCommandTarget::QueryStatus
 */
STDMETHODIMP CProxyFrame::QueryStatus( const GUID* pguidCmdGroup, ULONG cCmds,
				OLECMD* prgCmds, OLECMDTEXT* pCmdText )
{
    if ( pguidCmdGroup != NULL )
	{
		// It's a nonstandard group!!
        return OLECMDERR_E_UNKNOWNGROUP;
	}

    MSOCMD*     pCmd;
    INT         c;
    HRESULT     hr = S_OK;

    // By default command text is NOT SUPPORTED.
    if ( pCmdText && ( pCmdText->cmdtextf != OLECMDTEXTF_NONE ) )
	{
        pCmdText->cwActual = 0;
	}

    // Loop through each command in the array, setting the status of each.
    for ( pCmd = prgCmds, c = cCmds; --c >= 0; pCmd++ )
    {
        // By default command status is NOT SUPPORTED.
        pCmd->cmdf = 0;

        switch ( pCmd->cmdID )
        {
			case OLECMDID_UPDATECOMMANDS:
				pCmd->cmdf = OLECMDF_SUPPORTED;
				break;

			case OLECMDID_NEW:
			case OLECMDID_OPEN:
			case OLECMDID_SAVE:
				pCmd->cmdf = (MSOCMDF_SUPPORTED | MSOCMDF_ENABLED);
				break;
        }
    }

    return (hr);
}


/*
 * IOleCommandTarget::Exec
 */

STDMETHODIMP CProxyFrame::Exec( const GUID* pguidCmdGroup, DWORD nCmdID,
    DWORD /*nCmdexecopt*/, VARIANTARG* /*pvaIn*/, VARIANTARG* /*pvaOut*/ )
{
    HRESULT hr = S_OK;

    if ( pguidCmdGroup == NULL )
    {
        switch (nCmdID)
        {

			case OLECMDID_UPDATECOMMANDS:
				{
					// Fires event to container.
					m_pCtl->Fire_DisplayChanged();
					hr = S_OK;
				}
				break;

			default:
				hr = OLECMDERR_E_NOTSUPPORTED;
				break;
        }
    }
    else
    {
        hr = OLECMDERR_E_UNKNOWNGROUP;
    }
    return (hr);
}


//	Connector from control to site.
//
void
CProxyFrame::UpdateObjectRects()
{
	_ASSERTE ( m_pSite );
	if ( NULL != m_pSite )
	{
		m_pSite->UpdateObjectRects();
	}
}


//	Called from the control's TranslateAccelerator.
//	Try our own (VID-like) acclerators first, and if not handled pass them along to TriEdit.
//
HRESULT
CProxyFrame::HrTranslateAccelerator(LPMSG lpmsg)
{
	HRESULT hr = S_OK;

	if (NULL != m_pIOleIPActiveObject)
	{
		_ASSERTE(lpmsg);

		hr = HrHandleAccelerator(lpmsg);

		if (hr != S_OK)
		{
			_ASSERTE(SUCCEEDED(hr));
			hr = m_pIOleIPActiveObject->TranslateAccelerator(lpmsg);
		}
	}

	return hr;
}


//	A lot of time was lost here in scenarios like clicking on/tabbing to a control
//	embedded in a VB OCX, tabbing to a control on a page, etc.
//	Exercise great caution and perform a lot of testing if this is changed.
//
LRESULT
CProxyFrame::OnSetFocus(UINT /*nMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if ( ! m_pCtl->m_bUIActive )
	{
		m_pCtl->DoVerbUIActivate ( NULL, NULL );
	}

	// Give the focus to the ActiveX Document window
    if ( m_hWndObj != NULL )
	{
		::SetFocus( m_hWndObj );
	}

	return 0;
}


//	Sets the Trident window's parent correctly when created and destroyed.
//
void
CProxyFrame::SetParent ( HWND hwndParent )
{
	// This may be called before the control has been drawn.
	if ( NULL != m_hWndObj )
	{
		HWND hwndOldParent = ::SetParent ( m_hWndObj, hwndParent );
		if ( NULL == hwndOldParent )
		{
			DWORD dwErr = 0;
			dwErr = GetLastError ();
		}
		_ASSERTE ( m_pSite );
		m_pSite->SetWindow ( hwndParent );
	}
}


//	Handles WM_SHOWWINDOW messages directed to the control.
//
void
CProxyFrame::Show ( WPARAM nCmdShow )
{
	// This may be called before the control has been drawn.
	// Hide or show the hosted Trident
	if ( NULL != m_hWndObj )
	{
		::ShowWindow ( m_hWndObj, (int)nCmdShow );
	}
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//	ExecCommand mechanism
//
///////////////////////////////////////////////////////////////////////////////////////////


//	Convert a command ID into a TriEdit command ID.
//	Some commands used to represent other command groups as well, thus the ppguidCmdGroup parameter.
//	While this does little now, it may be useful again in the future.
//
HRESULT
CProxyFrame::HrMapCommand(DHTMLEDITCMDID typeLibCmdID,
	ULONG* cmdID, const GUID** ppguidCmdGroup, BOOL* pbOutParam)
{

	_ASSERTE(cmdID);
	_ASSERTE(ppguidCmdGroup);
	_ASSERTE(pbOutParam);

	*cmdID = 0;
	*ppguidCmdGroup = NULL;
	*pbOutParam = FALSE;

	for (UINT i=0; i < sizeof(cmdMap)/sizeof(CommandMap); ++i)
	{
		if (typeLibCmdID == cmdMap[i].typeLibCmdID)
		{
			*cmdID = cmdMap[i].cmdID;
			*ppguidCmdGroup = &GUID_TriEditCommandGroup;
			*pbOutParam = cmdMap[i].bOutParam;

			return S_OK;
		}
	}

	return OLECMDERR_E_NOTSUPPORTED ;
}


//	Helper routine for calling Exec.
//
HRESULT
CProxyFrame::HrExecCommand(const GUID* pguidCmdGroup, ULONG ucmdID,
	OLECMDEXECOPT cmdexecopt, VARIANT* pVarIn, VARIANT* pVarOut)
{
	HRESULT hr = E_FAIL;
	LPOLECOMMANDTARGET pCommandTarget = NULL;

	// note that it is valid for pguidCmdGroup to be NULL

	_ASSERTE(m_pSite);

	if (NULL == m_pSite)
		return E_UNEXPECTED;

	pCommandTarget = m_pSite->GetCommandTarget();

	_ASSERTE(pCommandTarget);

	if (pCommandTarget != NULL)
	{
		hr = pCommandTarget->Exec(pguidCmdGroup, ucmdID, cmdexecopt, pVarIn, pVarOut);
	}

	return hr;
}


//	Main command dispatcher; called from the control's ExecCommand method.
//	Handle our unique commands here, pass the rest onto HrExecGenericCommands.
//
HRESULT
CProxyFrame::HrMapExecCommand(DHTMLEDITCMDID deCommand, OLECMDEXECOPT cmdexecopt,
	VARIANT* pVarInput, VARIANT* pVarOutput)
{
	HRESULT hr = S_OK;
	LPOLECOMMANDTARGET pCmdTgt = NULL;
	ULONG ulMappedCommand = 0;
	const GUID* pguidCmdGroup = NULL;
	BOOL bOutParam = FALSE;

	if (FALSE == m_fActivated)
		return E_UNEXPECTED;

	_ASSERTE(m_pSite);
	if (NULL == m_pSite)
		return E_UNEXPECTED;

	pCmdTgt = m_pSite->GetCommandTarget();
	_ASSERTE(pCmdTgt);

	if (NULL == pCmdTgt)
		return E_UNEXPECTED;

	// Its valid for pVarInput to be NULL

	if (FAILED(hr = HrMapCommand(deCommand, &ulMappedCommand, &pguidCmdGroup, &bOutParam)))
		return hr;

	AssureActivated();

	switch ( deCommand )
	{
		case DECMD_GETBLOCKFMTNAMES:
			hr = HrExecGetBlockFmtNames(pVarInput);
			break;

		case DECMD_INSERTTABLE:
			hr = HrExecInsertTable(pVarInput);
			break;

		case DECMD_GETFORECOLOR:
		case DECMD_GETBACKCOLOR:
			hr = HrExecGetColor(deCommand, ulMappedCommand, pVarOutput);
			break;

		case DECMD_SETFONTSIZE:
			hr = HrExecSetFontSize(pVarInput);
			break;

		case DECMD_GETBLOCKFMT:
			// Trident inconsistancy: GetBlockFmt fails if outparam isn't a BSTR.  GetFontName is OK with VT_EMPTY
			VariantChangeType ( pVarOutput, pVarOutput, 0, VT_BSTR );
			// Fall through; do not break!
		case DECMD_GETFONTNAME:
		case DECMD_GETFONTSIZE:
			hr = HrExecGenericCommands(pguidCmdGroup, ulMappedCommand, cmdexecopt, pVarOutput, TRUE );
			break;

		// Because our QueryStatus on DECMD_PROPERTIES returns TRUE for anything with IOleObject, executing the properties
		// verb can return an unexpected error.  Therefore, we ALWAYS return S_OK from this command to avoid causing VB and
		// script to terminate.
		case DECMD_PROPERTIES:
		{
			CComVariant	varParam;
			varParam.vt		= VT_I4;
			varParam.lVal	= OLEIVERB_PROPERTIES;
			hr = HrExecGenericCommands(pguidCmdGroup, ulMappedCommand, cmdexecopt, &varParam, FALSE );
			hr = S_OK;
		}
		break;

		default:
			hr = HrExecGenericCommands(pguidCmdGroup, ulMappedCommand, cmdexecopt, pVarInput, bOutParam);
			break;
	}

	if (FAILED(hr))
	{
		if (DISP_E_BADVARTYPE == hr || DISP_E_MEMBERNOTFOUND == hr)
		{
		// Map these Trident errors to something more general.
		// These errors can occur if Trident expected the element
		// it was trying to operate on to support certain interfaces.
		// The caller was trying to perform an operation not valid
		// for the current selection. Probably didn't call QueryStatus
		// first.

			hr = OLECMDERR_E_NOTSUPPORTED;
		}
	}

	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//	ExecCommand handler implementations
//
///////////////////////////////////////////////////////////////////////////////////////////


//	Helper routine for calling Exec and dealing with results.
//
HRESULT
CProxyFrame::HrExecGenericCommands(const GUID* pguidCmdGroup, ULONG cmdID,
	OLECMDEXECOPT cmdexecopt, LPVARIANT pVarInput, BOOL bOutParam)
{
	HRESULT hr = S_OK;
	LPOLECOMMANDTARGET pCmdTgt = NULL;
	LPVARIANT _pVar = NULL;
	VARIANT	varCopy;

	pCmdTgt = m_pSite->GetCommandTarget();

	if (pVarInput && V_VT(pVarInput) & VT_BYREF)
	{
		// convert VARIANTARGs to Variant for use by Trident
		// this occurs in VB if the user specified a basic type
		// as an arg, i.e., String or Long, instead of Variant

		VariantInit(&varCopy);
		if (FAILED(hr = VariantCopyInd(&varCopy, pVarInput)))
		{
			_ASSERTE(SUCCEEDED(hr));
			return hr;
		}

		_pVar = &varCopy;
	}
	else if (pVarInput)
		_pVar = pVarInput;

	if (bOutParam)
	{
		hr = pCmdTgt->Exec(pguidCmdGroup, cmdID, cmdexecopt, NULL, _pVar);
	}
	else
	{
		hr = pCmdTgt->Exec(pguidCmdGroup, cmdID, cmdexecopt, _pVar, NULL);
	}

	if (FAILED(hr))
		goto cleanup;

	// if a VARIANTARG was passed in for a command with output then
	// fill it in with the result from the Exec
	if (bOutParam && pVarInput && (V_VT(pVarInput) & VT_BYREF))
	{
		_ASSERTE(_pVar);	// _pVar should always be non NULL here
							// if there was an input arg that was byref,
							// then it should have been mapped to _pVar

		if (NULL == _pVar)
			return E_UNEXPECTED; // the catch all error return for "we are in a weird state"

		// if the type of return is different that the type the caller
		// passed in then do nothing and return
		if (V_VT(_pVar) != (V_VT(pVarInput) ^ VT_BYREF))
			return hr;

		switch(V_VT(_pVar))
		{
		case VT_BSTR:
			_ASSERTE(V_VT(pVarInput) == (VT_BSTR|VT_BYREF));

			if (V_BSTRREF(pVarInput))
				hr = SysReAllocString(V_BSTRREF(pVarInput), V_BSTR(_pVar));
			break;

		case VT_BOOL:
			_ASSERTE(V_VT(pVarInput) == (VT_BOOL|VT_BYREF));

			if (V_BOOLREF(pVarInput))
				*(V_BOOLREF(pVarInput)) = V_BOOL(_pVar);
			break;

		case VT_I4:
			_ASSERTE(V_VT(pVarInput) == (VT_I4|VT_BYREF));

			if (V_I4REF(pVarInput))
				*(V_I4REF(pVarInput)) = V_I4(_pVar);
			break;

		default:
			_ASSERTE(0);
			break;
		}
	}

cleanup:
	// Our documentation replaces E_FAIL with DE_E_UNEXPECTED: different values.
	if ( E_FAIL == hr )
	{
		hr = DE_E_UNEXPECTED;
	}

	return hr;
}


//	Handler for command DECMD_GETBLOCKFMTNAMES.
//	There are plenty of possible types of arrays to be handled.
//
HRESULT
CProxyFrame::HrExecGetBlockFmtNames(LPVARIANT pVarInput)
{
	HRESULT hr = S_OK;
	LPOLECOMMANDTARGET pCmdTgt = NULL;
	VARIANT varArray;
	LPUNKNOWN pUnk = NULL;
	CComPtr<IDEGetBlockFmtNamesParam> piNamesParam;

	pCmdTgt = m_pSite->GetCommandTarget();

	if (NULL == pVarInput)
		return E_INVALIDARG;

	if (V_VT(pVarInput) == (VT_BYREF|VT_DISPATCH))
	{
		if (V_DISPATCHREF(pVarInput))
			pUnk = *(V_DISPATCHREF(pVarInput));
		else
			return E_INVALIDARG;
	}
	else if (V_VT(pVarInput) == VT_DISPATCH)
	{
		if (V_DISPATCH(pVarInput))
			pUnk = V_DISPATCH(pVarInput);
		else
			return E_INVALIDARG;
	}
	else if (V_VT(pVarInput) == (VT_BYREF|VT_UNKNOWN))
	{
		if (V_UNKNOWNREF(pVarInput))
			pUnk = *(V_UNKNOWNREF(pVarInput));
		else
			return E_INVALIDARG;
	}
	else if (V_VT(pVarInput) == VT_UNKNOWN)
	{
		if (V_UNKNOWN(pVarInput))
			pUnk = V_UNKNOWN(pVarInput);
		else
			return E_INVALIDARG;
	}
	else
		return E_INVALIDARG;

	// This can happen in VB if an object that has not
	// been set with CreateObject has been passed in
	if (NULL == pUnk)
		return E_INVALIDARG;

	// Try to get the names object before
	// performing the command

	if (FAILED(hr = pUnk->QueryInterface(IID_IDEGetBlockFmtNamesParam, (LPVOID*) &piNamesParam)))
		return E_INVALIDARG;

	_ASSERTE((!piNamesParam) == FALSE);

	// Trident wants the vt to be specifically VT_ARRAY with
	// no type qualifer -- if you give one it fails even though
	// an array of BSTRs is returned

	VariantInit(&varArray);
	V_VT(&varArray) = VT_ARRAY;

	hr = pCmdTgt->Exec(&GUID_TriEditCommandGroup, IDM_TRIED_GETBLOCKFMTS,
		MSOCMDEXECOPT_DONTPROMPTUSER, NULL, &varArray);

	if (FAILED(hr))
		goto cleanup;

	piNamesParam->put_Names(&varArray);

cleanup:

	return hr;
}


//	Handler for command DECMD_INSERTTABLE.
//
HRESULT
CProxyFrame::HrExecInsertTable(LPVARIANT pVarInput)
{
	HRESULT hr = S_OK;
	LPOLECOMMANDTARGET pCmdTgt = NULL;
	VARIANT varTableArray;
	LPUNKNOWN pUnk = NULL;
	CComPtr<IDEInsertTableParam> piTableParam;

	pCmdTgt = m_pSite->GetCommandTarget();

	VariantInit(&varTableArray);

	if (NULL == pVarInput)
		return E_INVALIDARG;

	if (V_VT(pVarInput) == (VT_BYREF|VT_DISPATCH))
	{
		if (V_DISPATCHREF(pVarInput))
			pUnk = *(V_DISPATCHREF(pVarInput));
		else
			return E_INVALIDARG;
	}
	else if (V_VT(pVarInput) == VT_DISPATCH)
	{
		if (V_DISPATCH(pVarInput))
			pUnk = V_DISPATCH(pVarInput);
		else
			return E_INVALIDARG;
	}
	else if (V_VT(pVarInput) == (VT_BYREF|VT_UNKNOWN))
	{
		if (V_UNKNOWNREF(pVarInput))
			pUnk = *(V_UNKNOWNREF(pVarInput));
		else
			return E_INVALIDARG;
	}
	else if (V_VT(pVarInput) == VT_UNKNOWN)
	{
		if (V_UNKNOWN(pVarInput))
			pUnk = V_UNKNOWN(pVarInput);
		else
			return E_INVALIDARG;
	}
	else
		return E_INVALIDARG;

	// This can happen in VB if an object that has not
	// been set with CreateObject has been passed in
	if (NULL == pUnk)
		return E_INVALIDARG;

	if (FAILED(hr = pUnk->QueryInterface(IID_IDEInsertTableParam, (LPVOID*) &piTableParam)))
		return E_INVALIDARG;

	_ASSERTE((!piTableParam) == FALSE);

	if (FAILED(hr = HrGetTableSafeArray(piTableParam, &varTableArray)))
	{
		_ASSERTE(SUCCEEDED(hr));
		return hr;
	}


	hr = pCmdTgt->Exec(&GUID_TriEditCommandGroup, IDM_TRIED_INSERTTABLE,
		MSOCMDEXECOPT_DONTPROMPTUSER, &varTableArray, NULL);

	return hr;
}


//	Hanlder for commands DECMD_GETFORECOLOR and DECMD_GETBACKCOLOR.
//	Reply with a string in the format #RRGGBB or an empty string.
//
HRESULT
CProxyFrame::HrExecGetColor(DHTMLEDITCMDID deCommand, ULONG ulMappedCommand, LPVARIANT pVarOutput)
{
	USES_CONVERSION;
	HRESULT hr = S_OK;
	LPOLECOMMANDTARGET pCmdTgt = NULL;
	VARIANT varColorOut;
	TCHAR buf[32];
	WCHAR* oleStr = NULL;

	pCmdTgt = m_pSite->GetCommandTarget();

	if (NULL == pVarOutput)
		return E_INVALIDARG;

	// validate the command
	if (DECMD_GETFORECOLOR != deCommand && DECMD_GETBACKCOLOR != deCommand)
		return E_INVALIDARG;

	// validate the args
	if (V_VT(pVarOutput) == (VT_BYREF|VT_BSTR))
	{
		if (NULL == V_BSTRREF(pVarOutput))
			return E_INVALIDARG;
	}
	else if (V_VT(pVarOutput) == VT_BSTR)
	{
		if (NULL == V_BSTR(pVarOutput))
			return E_INVALIDARG;
	}
	else if (V_VT(pVarOutput) != (VT_EMPTY) && V_VT(pVarOutput) != (VT_NULL))
		return E_INVALIDARG;

	VariantInit(&varColorOut);
	V_VT(&varColorOut) = VT_I4;

	hr = pCmdTgt->Exec(&GUID_TriEditCommandGroup, ulMappedCommand,
		MSOCMDEXECOPT_DONTPROMPTUSER, NULL, &varColorOut);

	// Trident will return VT_NULL if color selection
	// was mixed or no text is selected, we return empty
	// string ("") in that case.

	buf[0] = 0;

	if (VT_I4 == V_VT(&varColorOut))
	{
		ULONG ulColor = 0;
		ULONG r=0;
		ULONG g=0;
		ULONG b=0;

		ulColor = V_I4(&varColorOut);
		r = 0x000000ff & ulColor;
		g = (0x0000ff00 & ulColor) >> 8;
		b = (0x00ff0000 & ulColor) >> 16;

		wsprintf(buf, TEXT("#%02X%02X%02X"), r, g, b);
	}
	
	oleStr = T2OLE(buf);

	if (V_VT(pVarOutput) == (VT_BSTR|VT_BYREF))
		hr = SysReAllocString(V_BSTRREF(pVarOutput), oleStr);
	else if (V_VT(pVarOutput) == (VT_BSTR))
		hr = SysReAllocString(&(V_BSTR(pVarOutput)), oleStr);
	else if (V_VT(pVarOutput) == (VT_EMPTY) || V_VT(pVarOutput) == (VT_NULL))
	{
		V_VT(pVarOutput) = VT_BSTR;
		V_BSTR(pVarOutput) = SysAllocString(oleStr);
	}

	return hr;
}


//	Handler for command DECMD_SETFONTSIZE.
//
HRESULT
CProxyFrame::HrExecSetFontSize(LPVARIANT pVarInput)
{
	HRESULT hr = S_OK;
	LPOLECOMMANDTARGET pCmdTgt = NULL;
	VARIANT varSizeIn;
	

	pCmdTgt = m_pSite->GetCommandTarget();

	if (NULL == pVarInput)
		return E_INVALIDARG;

	VariantInit(&varSizeIn);

	if (FAILED(hr = VariantChangeType(&varSizeIn, pVarInput, 0, VT_I4)))
		return E_INVALIDARG;

	if (varSizeIn.lVal < 0 || varSizeIn.lVal > 7)
		return E_INVALIDARG;

	if (0 == varSizeIn.lVal)
		varSizeIn.lVal = varSizeIn.lVal + 1;


	hr = pCmdTgt->Exec(&GUID_TriEditCommandGroup, IDM_TRIED_FONTSIZE,
		MSOCMDEXECOPT_DONTPROMPTUSER, &varSizeIn, NULL);

	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//	QueryStatus mechanism
//
///////////////////////////////////////////////////////////////////////////////////////////

//	Map the control specific command ID to a TriEdit command ID and call QueryStatus.
//
HRESULT
CProxyFrame::HrMapQueryStatus( DHTMLEDITCMDID ucmdID, DHTMLEDITCMDF* cmdf)
{
	LPOLECOMMANDTARGET pCommandTarget = NULL;

	_ASSERTE(cmdf);

	HRESULT hr = E_FAIL;

	if (FALSE == m_fActivated)
		return E_UNEXPECTED;

	if (NULL == cmdf)
		return E_INVALIDARG;

	*cmdf = (DHTMLEDITCMDF) 0;

	_ASSERTE(m_pSite);

	if (NULL == m_pSite)
		return E_UNEXPECTED;

	pCommandTarget = m_pSite->GetCommandTarget();
	_ASSERTE(pCommandTarget);

	if ( pCommandTarget != NULL )
	{

		AssureActivated ();

		ULONG cmdID = 0;
		const GUID* pguidCmdGroup = NULL;
		BOOL bOutParam = FALSE;

		if (SUCCEEDED(hr = HrMapCommand(ucmdID, &cmdID, &pguidCmdGroup, &bOutParam)))
		{
			MSOCMD msocmd;
			msocmd.cmdID = cmdID;
			msocmd.cmdf  = 0;

			hr = pCommandTarget->QueryStatus(pguidCmdGroup, 1, &msocmd, NULL);

			*cmdf = (DHTMLEDITCMDF) msocmd.cmdf;
		}
	}

	return hr;
}


//	General routine for determining the status of a command.
//	Should resolve to not supported, disabled, enabled, latched or ninched.
//
HRESULT
CProxyFrame::HrQueryStatus(const GUID* pguidCmdGroup, ULONG ucmdID, OLECMDF* cmdf)
{
	HRESULT hr = E_FAIL;

	_ASSERTE(cmdf);

	// Note that it is valid for pguidCmdGroup to be NULL

	if (NULL == cmdf)
		return E_INVALIDARG;

	*cmdf = (OLECMDF) 0;

	_ASSERTE(m_pSite);

	if ( m_pSite != NULL ) // m_pSite should always be set
	{
		LPOLECOMMANDTARGET pCommandTarget = m_pSite->GetCommandTarget();

		if ( pCommandTarget != NULL )
		{
			MSOCMD msocmd;
			msocmd.cmdID = ucmdID;
			msocmd.cmdf  = 0;

			hr = pCommandTarget->QueryStatus(pguidCmdGroup, 1, &msocmd, NULL);

			*cmdf = (OLECMDF) msocmd.cmdf;
		}
	}

	return hr;
}


//	A tragic FAT16 compatibility problem: file names in the specific form:
//	[a-zA-z]\:[^\\].+ cause various, severe problems.  NTFS "forgives".
//	We must detect these, both in file names and file:// URL and return an error.
//
BOOL
CProxyFrame::IsMissingBackSlash ( BSTR path, BOOL bfIsURL )
{
	BOOL bfMissing = FALSE;

	if ( bfIsURL )
	{
		WCHAR	wszFileProtocol[] = L"file://";
		int		cchProtocol		= wcslen ( wszFileProtocol );

		if ( 0 == _wcsnicmp ( path, wszFileProtocol, cchProtocol ) )
		{
			if ( OLECHAR(':') == path[cchProtocol+1] )
			{
				if ( OLECHAR('\\') != path[cchProtocol+2] )
				{
					bfMissing = TRUE;
				}
			}
		}
	}
	else
	{
		// Path name.  chec for drive letter, colon, non-backslash.
		if ( OLECHAR(':') == path[1] )
		{
			if ( OLECHAR('\\') != path[2] )
			{
				bfMissing = TRUE;
			}
		}
	}
	return bfMissing;
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//	Control methods and properties
//
///////////////////////////////////////////////////////////////////////////////////////////

//	Handles NewDocument, LoadURL and LoadDocument control methods.
//	The document is loaded indirectly via the pluggable protocol handler.
//	If "path" is NULL, do NewDocument.  TestbfURL to see if it's a URL or UNC path.
//
HRESULT
CProxyFrame::LoadDocument(BSTR path, BOOL bfIsURL )
{
	USES_CONVERSION;

	HRESULT hr			= S_OK;
	UINT pathLen		= 0;

	AssureActivated ();	// This can set m_bstrLoadText as a side effect in unactivated controls!  Be careful!

	if (FALSE == m_fActivated)
		return E_UNEXPECTED;

	m_bstrLoadText.Empty ();	// Clear the text to be added directly, or it will be used instead!
	m_bstrCurDocPath	= L"";
	m_bstrBaseURL		= L"";

	if (path)
		pathLen = ::SysStringLen(path);
	else
		pathLen = 0;

	// We've resetting the contents of the control.  Go back to default save mechanism.
	// If we load Unicode it will be reset.
	m_pSite->SetSaveAsUnicode ( FALSE );

	if (path && pathLen)
	{
		_ASSERTE(path);
		_ASSERTE(pathLen > 0);

		// First, look out for a wicked error: X:FileName with no '\' is BAD on FAT16.
		if ( IsMissingBackSlash ( path, bfIsURL ) )
		{
			hr = DE_E_PATH_NOT_FOUND;
			LoadBSTRDeferred ( m_bstrInitialDoc );
			goto error;
		}

		// Try to open the file -- stop the sequence
		// if its bogus or we don't have access
		if ( !bfIsURL )
		{
			if (FAILED(hr = m_pSite->HrTestFileOpen(path)))
			{
				LoadBSTRDeferred ( m_bstrInitialDoc );
				goto error;
			}
		}
		m_bfIsURL = bfIsURL;

		m_bstrCurDocPath = path;	// This needs to be set before loading, because base url is needed durring load.
		SetBaseURLFromCurDocPath ( bfIsURL );
		m_bfPreserveDirtyFlagAcrossBrowseMode = FALSE;

		CComPtr<IMoniker> srpMoniker;
		CComPtr<IBindCtx> srpBindCtx;
		CComQIPtr<IPersistMoniker, &IID_IPersistMoniker> srpPM (m_pUnkTriEdit);
		_ASSERTE ( srpPM );

		if ( srpPM )
		{
			CComBSTR	bstrProtocol = m_wszProtocolPrefix;
			bstrProtocol += L"(";
			bstrProtocol += path;
			bstrProtocol += L")";

#ifdef LATE_BIND_URLMON_WININET
			_ASSERTE ( m_pfnCreateURLMoniker );
			hr = (*m_pfnCreateURLMoniker)( NULL, bstrProtocol, &srpMoniker );
#else
			hr = CreateURLMoniker ( NULL, bstrProtocol, &srpMoniker );
#endif // LATE_BIND_URLMON_WININET

			_ASSERTE ( SUCCEEDED( hr ) );
			if ( SUCCEEDED ( hr ) )
			{
				hr = ::CreateBindCtx(NULL, &srpBindCtx);
				_ASSERTE ( SUCCEEDED( hr ) );
				if ( SUCCEEDED ( hr ) )
				{
					// Delete the cache entry before downloading.
					// This assures that loading, posting, and reloading works.
					// Bug 18544.
					// NOTE: Inexact match fails!  http://www.microsoft.com fails,
					// because this actually loads/caches a specific default page.
					if ( bfIsURL )
					{
						LPTSTR szURL = OLE2T ( m_bstrCurDocPath );
#ifdef LATE_BIND_URLMON_WININET
						_ASSERTE ( m_pfnDeleteUrlCacheEntry );
						(*m_pfnDeleteUrlCacheEntry)( szURL );
#else
						DeleteUrlCacheEntry ( szURL );
#endif // LATE_BIND_URLMON_WININET
					}
					m_bfIsLoading = TRUE;
					m_hrDeferredLoadError = S_OK;	// URLs: don't let Trident get the error!

					hr = srpPM->Load(FALSE, srpMoniker,  srpBindCtx, STGM_READ);

					if ( SUCCEEDED ( hr ) && FAILED ( m_hrDeferredLoadError ) )
					{
						hr = m_hrDeferredLoadError;	// In case we stashed a result
					}
					if ( FAILED ( hr ) )
					{
						m_bfIsLoading = FALSE;
					}
				}
			}
		}
	}
	else
	{
		if (FAILED(hr = LoadBSTRDeferred ( m_bstrInitialDoc )))
		{
			_ASSERTE(SUCCEEDED(hr));
			goto error;
		}
	}

error:
	return hr;
}


//	Implements FilterSourceCode control method
//	Used to restore filtered content extracted directly from DOM.
//
HRESULT
CProxyFrame::FilterSourceCode ( BSTR bsSourceIn, BSTR* pbsSourceOut )
{
	HRESULT				hr;
	CComPtr<IStream>	spStreamIn;
	IStream*			piStreamOut;

	hr = m_pSite->HrBstrToStream(bsSourceIn, &spStreamIn);
	if ( SUCCEEDED ( hr ) )
	{
		if ( m_vbBrowseMode )
		{
			spStreamIn->AddRef ();
			piStreamOut = spStreamIn;
		}
		else
		{
			hr = m_pSite->HrFilter ( FALSE, spStreamIn, &piStreamOut, m_dwFilterOutFlags | dwFilterSourceCode);
		}
		if ( SUCCEEDED ( hr ) )
		{
			hr = m_pSite->HrStreamToBstr ( piStreamOut, pbsSourceOut );
			piStreamOut->Release ();
		}
	}
	return hr;
}


//	Implements the control's Print method
//
HRESULT
CProxyFrame::Print ( BOOL bfWithUI )
{
	AssureActivated ();

	if (FALSE == m_fActivated)
		return E_UNEXPECTED;
	return HrExecCommand ( &GUID_TriEditCommandGroup, IDM_TRIED_PRINT,
		bfWithUI ? MSOCMDEXECOPT_PROMPTUSER : MSOCMDEXECOPT_DONTPROMPTUSER, NULL, NULL );
}


//	Implements the control's Refresh method
//
HRESULT
CProxyFrame::RefreshDoc ()
{
	if ( NULL != m_hWndObj )
	{
		if ( ::IsWindow ( m_hWndObj ) )
		{
			::InvalidateRect ( m_hWndObj, NULL, TRUE );
			return S_OK;
		}
	}
	return S_FALSE;
}


//	Implements the control's SaveDocument method
//
HRESULT
CProxyFrame::SaveDocument(BSTR path)
{
	HRESULT hr = S_OK;
	ULONG pathLen = 0;

	if (FALSE == m_fActivated)
		return E_UNEXPECTED;

	_ASSERTE(GetState() == ESTATE_ACTIVATED);

	AssureActivated ();

	if (GetState() != ESTATE_ACTIVATED)
		return E_UNEXPECTED;

	_ASSERTE(path);

	if (path)
		pathLen = ::SysStringLen(path);
	else
		pathLen = 0;

	if (0 == pathLen)
		return E_INVALIDARG;

	_ASSERTE(pathLen);

	// First, look out for a wicked error: X:FileName with no '\' is BAD on FAT16.
	if ( IsMissingBackSlash ( path, FALSE ) )
	{
		return DE_E_PATH_NOT_FOUND;
	}

	hr = m_pSite->HrSaveToFile(path, m_dwFilterOutFlags);

	if ( SUCCEEDED ( hr ) )
	{
		m_bstrCurDocPath = path;
	}

	return hr;
}


//	Implements the control's SetContextMenu method
//	One routine handles javascript arrays, the other simple arrays.
//
HRESULT
CProxyFrame::SetContextMenu(LPVARIANT pVarMenuStrings, LPVARIANT pVarMenuStates)
{
	if (V_VT(pVarMenuStrings) == VT_DISPATCH || V_VT(pVarMenuStates) == VT_DISPATCH)
		return SetContextMenuDispEx(pVarMenuStrings, pVarMenuStates);
	else
		return SetContextMenuSA(pVarMenuStrings, pVarMenuStates);
}


// Get menu strings from SafeArray
//
HRESULT
CProxyFrame::SetContextMenuSA(LPVARIANT pVarMenuStrings, LPVARIANT pVarMenuStates)
{
	HRESULT hr = S_OK;
	SAFEARRAY* psaStrings = NULL;
	SAFEARRAY* psaStates = NULL;
    LONG lLBound, lUBound, lLBoundState, lUBoundState;

	if (NULL == pVarMenuStrings || NULL == pVarMenuStates)
		return E_INVALIDARG;

	if ((VT_ARRAY|VT_BSTR) != V_VT(pVarMenuStrings) &&
		((VT_ARRAY|VT_BSTR)|VT_BYREF) != V_VT(pVarMenuStrings) &&
		((VT_ARRAY|VT_VARIANT)|VT_BYREF) != V_VT(pVarMenuStrings) &&
		(VT_ARRAY|VT_VARIANT) != V_VT(pVarMenuStrings))
		return E_INVALIDARG;

	if ((VT_ARRAY|VT_I4) != V_VT(pVarMenuStates) &&
		((VT_ARRAY|VT_I4)|VT_BYREF) != V_VT(pVarMenuStates) &&
		((VT_ARRAY|VT_VARIANT)|VT_BYREF) != V_VT(pVarMenuStates) &&
		(VT_ARRAY|VT_VARIANT) != V_VT(pVarMenuStates))
		return E_INVALIDARG;

	if ((VT_ARRAY|VT_BSTR) == V_VT(pVarMenuStrings))
	{
		psaStrings = V_ARRAY(pVarMenuStrings);
	}
	if ((VT_ARRAY|VT_VARIANT) == V_VT(pVarMenuStrings))
	{
		psaStrings = V_ARRAY(pVarMenuStrings);
	}
	else if ((VT_ARRAY|VT_BSTR|VT_BYREF) == V_VT(pVarMenuStrings))
	{
		if (NULL == V_ARRAYREF(pVarMenuStrings))
			return E_INVALIDARG;

		psaStrings = *(V_ARRAYREF(pVarMenuStrings));
	}
	else if ((VT_ARRAY|VT_VARIANT|VT_BYREF) == V_VT(pVarMenuStrings))
	{
		if (NULL == V_ARRAYREF(pVarMenuStrings))
			return E_INVALIDARG;
		
		psaStrings = *(V_ARRAYREF(pVarMenuStrings));
	}

	if ((VT_ARRAY|VT_I4) == V_VT(pVarMenuStates))
	{
		psaStates = V_ARRAY(pVarMenuStates);
	}
	if ((VT_ARRAY|VT_VARIANT) == V_VT(pVarMenuStates))
	{
		psaStates = V_ARRAY(pVarMenuStates);
	}
	else if ((VT_ARRAY|VT_I4|VT_BYREF) == V_VT(pVarMenuStates))
	{
		if (NULL == V_ARRAYREF(pVarMenuStates))
			return E_INVALIDARG;

		psaStates = *(V_ARRAYREF(pVarMenuStates));
	}
	else if ((VT_ARRAY|VT_VARIANT|VT_BYREF) == V_VT(pVarMenuStates))
	{
		if (NULL == V_ARRAYREF(pVarMenuStates))
			return E_INVALIDARG;

		psaStates = *(V_ARRAYREF(pVarMenuStates));
	}


	if (NULL == psaStrings || NULL == psaStates)
		return E_INVALIDARG;

	SafeArrayGetLBound(psaStrings, 1, &lLBound);
	SafeArrayGetUBound(psaStrings, 1, &lUBound);

	SafeArrayGetLBound(psaStates, 1, &lLBoundState);
	SafeArrayGetUBound(psaStates, 1, &lUBoundState);

	if (lLBound != lLBoundState || lUBound != lUBoundState)
		return E_INVALIDARG;

	if (m_pMenuStrings)
	{
		SafeArrayDestroy(m_pMenuStrings);
		m_pMenuStrings = NULL;
	}

	if (m_pMenuStates)
	{
		SafeArrayDestroy(m_pMenuStates);
		m_pMenuStates = NULL;
	}

	// An empty array was passed in 
	// The context menu has been cleared
	if (lLBound ==lUBound )
		goto cleanup;

	if (FAILED(hr = SafeArrayCopy(psaStrings, &m_pMenuStrings)))
		goto cleanup;

	if (FAILED(hr = SafeArrayCopy(psaStates, &m_pMenuStates)))
		goto cleanup;

cleanup:

	if (FAILED(hr))
	{
		if (m_pMenuStrings)
		{
			SafeArrayDestroy(m_pMenuStrings);
			m_pMenuStrings = NULL;
		}

		if (m_pMenuStates)
		{
			SafeArrayDestroy(m_pMenuStates);
			m_pMenuStates = NULL;	
		}
	}

    return hr;
}


// Get menu strings from JScript array, or object that supports IDispatchEx
// For iterating through JScript arrays, we expect the elements
// to be accessable by ordinals starting at 0, i.e., a 0 based array
//
HRESULT
CProxyFrame::SetContextMenuDispEx(LPVARIANT pVarMenuStrings, LPVARIANT pVarMenuStates)
{
	HRESULT hr = S_OK;
	ULONG i=0;
	ULONG ulStringsLen = 0;
	ULONG ulStatesLen = 0;
	IDispatch* pdStrings = NULL;
	IDispatch* pdStates = NULL;
	IDispatchEx* pdexStrings = NULL;
	IDispatchEx* pdexStates = NULL;
	CDispExArray dispStrings;
	CDispExArray dispStates;
	VARIANT varString;
	VARIANT varState;
	SAFEARRAYBOUND rgsabound[1] = {0};
	LONG ix[1]					= {0};

	if (VT_DISPATCH != V_VT(pVarMenuStrings) || VT_DISPATCH != V_VT(pVarMenuStates))
		return E_INVALIDARG;

	VariantInit(&varString);
	VariantInit(&varState);

	pdStrings = V_DISPATCH(pVarMenuStrings);
	pdStates = V_DISPATCH(pVarMenuStates);

	_ASSERTE(pdStrings);
	_ASSERTE(pdStates);

	if (FAILED(hr = pdStrings->QueryInterface(IID_IDispatchEx, (LPVOID*) &pdexStrings)))
	{
		return E_INVALIDARG;
	}
	dispStrings.Attach(pdexStrings);

	if (FAILED(hr = pdStates->QueryInterface(IID_IDispatchEx, (LPVOID*) &pdexStates)))
	{
		return E_INVALIDARG;
	}
	dispStates.Attach(pdexStates);

	if (FAILED(dispStrings.HrGetLength(&ulStringsLen)))
		goto cleanup;

	if (FAILED(dispStates.HrGetLength(&ulStatesLen)))
		goto cleanup;

	// Make sure that arrays are equal length
	if (ulStringsLen != ulStatesLen)
		return E_INVALIDARG;

	if (m_pMenuStrings)
	{
		SafeArrayDestroy(m_pMenuStrings);
		m_pMenuStrings = NULL;
	}

	if (m_pMenuStates)
	{
		SafeArrayDestroy(m_pMenuStates);
		m_pMenuStates = NULL;
	}

	// An empty array was passed in 
	// The context menu has been cleared
	if (ulStringsLen <= 0)
		goto cleanup;

	rgsabound[0].lLbound = 0;
	rgsabound[0].cElements = ulStringsLen;

	m_pMenuStrings = SafeArrayCreate(VT_BSTR, 1, rgsabound);	
	_ASSERTE(m_pMenuStrings);
	if (NULL == m_pMenuStrings)
	{
		hr = E_OUTOFMEMORY;
		goto cleanup;
	}

	m_pMenuStates = SafeArrayCreate(VT_I4, 1, rgsabound);	
	_ASSERTE(m_pMenuStates);
	if (NULL == m_pMenuStates)
	{
		hr = E_OUTOFMEMORY;
		goto cleanup;
	}

	// For iterating through JScript arrays, we expect the elements
	// to be accessable by ordinals starting at 0, i.e., a 0 based array
	hr = S_OK;
	for (i=0; i < ulStringsLen && hr != S_FALSE; i++)
	{		
		if (FAILED(hr = dispStrings.HrGetElement(i, &varString)))
			goto cleanup;

		if (FAILED(hr = dispStates.HrGetElement(i, &varState)))
			goto cleanup;

		if (VT_BSTR != V_VT(&varString) || VT_I4 != V_VT(&varState))
		{
			hr = E_INVALIDARG;
			goto cleanup;
		}

		ix[0] = i;
		if (FAILED(hr = SafeArrayPutElement(m_pMenuStrings, ix, (LPVOID) V_BSTR(&varString))))
			goto cleanup;

		if (FAILED(hr = SafeArrayPutElement(m_pMenuStates, ix, (LPVOID) &(V_I4(&varState)))))
			goto cleanup;

		VariantClear ( &varString );
		VariantClear ( &varState );
	}

cleanup:

	if (FAILED(hr))
	{
		if (m_pMenuStrings)
		{
			SafeArrayDestroy(m_pMenuStrings);
			m_pMenuStrings = NULL;
		}

		if (m_pMenuStates)
		{
			SafeArrayDestroy(m_pMenuStates);
			m_pMenuStates = NULL;	
		}
	}

    return hr;
}


//	DocumentTitle property implementation; read only.
//	Get the property from the HTML document.
//
HRESULT
CProxyFrame::GetDocumentTitle ( CComBSTR&  bstrTitle )
{
	HRESULT		hr = S_OK;
	DISPID		dispid;
	DISPPARAMS	dispparamsNoArgs = {NULL, NULL, 0, 0};
	CComVariant	varResult;

	CComPtr<IHTMLDocument2> piHtmlDoc = NULL;
	hr = HrGetDoc( &piHtmlDoc );

	if ( SUCCEEDED ( hr ) )
	{
		AssureActivated();

		hr = piHtmlDoc->GetIDsOfNames ( IID_NULL, &g_wszHTMLTitlePropName, 1, LOCALE_SYSTEM_DEFAULT, &dispid );
		_ASSERTE ( SUCCEEDED ( hr ) );
		if ( FAILED ( hr ) )
		{
			return hr;
		}

		hr = piHtmlDoc->Invoke ( dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET,
			&dispparamsNoArgs, &varResult, NULL, NULL );
		_ASSERTE ( SUCCEEDED ( hr ) );
		if ( FAILED ( hr ) )
		{
			return hr;
		}
		
		hr = varResult.ChangeType ( VT_BSTR );
		_ASSERTE ( SUCCEEDED ( hr ) );
		if ( FAILED ( hr ) )
		{
			return hr;
		}

		bstrTitle = varResult.bstrVal;
	}

	return hr;
}


//	Implements getting the control's BrowseMode property
//
HRESULT
CProxyFrame::GetBrowseMode ( VARIANT_BOOL  *pVal )
{
	*pVal = m_vbBrowseMode;
	return S_OK;
}


//	Implements setting the control's BrowseMode property
//
HRESULT
CProxyFrame::SetBrowseMode ( VARIANT_BOOL  newVal )
{
	HRESULT hr = S_FALSE;	// Indicates value was set, but actual mode was not changed.

	_ASSERTE ( m_pSite );

	// If we're still reading the property bag, just set the value, don't change the text;
	// it hasn't been loaded yet.
	if ( NULL == m_pSite->GetCommandTarget() )
	{
		m_vbBrowseMode = newVal;
		hr = S_OK;
	}
	else
	{
		if ( m_vbBrowseMode != newVal )
		{
			AssureActivated ();

			m_bfModeSwitched = TRUE;

			if ( newVal && m_pCtl->IsUserMode () )	// newVal means "switching to browse mode"
			{
				CComPtr<IStream>	spStream	= NULL;

				HrGetIsDirty ( m_bfPreserveDirtyFlagAcrossBrowseMode );
				hr = m_pSite->HrSaveToStreamAndFilter ( &spStream, m_dwFilterOutFlags );
				if ( SUCCEEDED ( hr ) )
				{
					m_bstrLoadText.Empty ();
					// Preserve the byte order mark, or else it will not be reloaded properly
					hr = m_pSite->HrStreamToBstr ( spStream, &m_bstrLoadText, TRUE );
				}
			}

			m_vbBrowseMode = newVal;

			// Let Trident know the ambient property has changed.
			CComQIPtr<IOleControl,&IID_IOleControl>spioc ( m_pSite->GetObjectUnknown() );
			if ( spioc )
			{
				m_bfIsLoading = TRUE;
				spioc->OnAmbientPropertyChange ( DISPID_AMBIENT_USERMODE );
			}
		}
	}
	return hr;
}


//	Implements getting the control's UseDivOnCarriageReturn property
//
HRESULT
CProxyFrame::GetDivOnCr ( VARIANT_BOOL  *pVal )
{
	*pVal = m_vbUseDivOnCr;
	return S_OK;
}


//	Implements setting the control's UseDivOnCarriageReturn property
//
HRESULT
CProxyFrame::SetDivOnCr ( VARIANT_BOOL  newVal )
{
	HRESULT		hr	= S_OK;
	CComVariant varDefBlock;

	m_vbUseDivOnCr = newVal;

	// Reinitialize if we haven't loaded our properties before this point.
	if ( READYSTATE_UNINITIALIZED == m_readyState )
	{
		// InitializeDocString takes m_vbUseDivOnCr into account
		InitializeDocString ();
	}
	return hr;
}


//	Implements getting the control's read-only Busy property
//
HRESULT
CProxyFrame::GetBusy ( VARIANT_BOOL *pVal )
{
#pragma warning(disable: 4310) // cast truncates constant value
	*pVal = ( m_bfIsLoading ) ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
	return S_OK;
}


// Implements setting the control's ActivateActiveXControls property
//
HRESULT
CProxyFrame::HrSetPropActivateControls(BOOL activateControls)
{
	HRESULT hr = S_OK;

	if (m_fActivated)
	{
		if (SUCCEEDED(hr = HrTridentSetPropBool(IDM_NOACTIVATENORMALOLECONTROLS, !activateControls)))
			m_fActivateControls = activateControls;
	}
	else
		m_fActivateControls = activateControls;

	_ASSERTE(SUCCEEDED(hr));
	return hr;
}


// Implements getting the control's ActivateActiveXControls property
//
HRESULT
CProxyFrame::HrGetPropActivateControls(BOOL& activateControls)
{
	HRESULT hr = S_OK;

	activateControls = m_fActivateControls;

	_ASSERTE(SUCCEEDED(hr));
	return hr;
}


// Implements setting the control's ActivateApplets property
//
HRESULT
CProxyFrame::HrSetPropActivateApplets(BOOL activateApplets)
{
	HRESULT hr = S_OK;

	if (m_fActivated)
	{
		if (SUCCEEDED(hr = HrTridentSetPropBool(IDM_NOACTIVATEJAVAAPPLETS, !activateApplets)))
			m_fActivateApplets = activateApplets;
	}
	else
		m_fActivateApplets = activateApplets;

	_ASSERTE(SUCCEEDED(hr));
	return hr;
}


// Implements getting the control's ActivateApplets property
//
HRESULT
CProxyFrame::HrGetPropActivateApplets(BOOL& activateApplets)
{
	HRESULT hr = S_OK;

	activateApplets = m_fActivateApplets;

	_ASSERTE(SUCCEEDED(hr));
	return hr;
}


// Implements setting the control's ActivateDTCs property
//
HRESULT
CProxyFrame::HrSetPropActivateDTCs(BOOL activateDTCs)
{
	HRESULT hr = S_OK;

	if (m_fActivated)
	{
		if (SUCCEEDED(hr = HrTridentSetPropBool(IDM_NOACTIVATEDESIGNTIMECONTROLS, !activateDTCs)))
			m_fActivateDTCs = activateDTCs;
	}
	else
		m_fActivateDTCs = activateDTCs;

	_ASSERTE(SUCCEEDED(hr));
	return hr;
}


// Implements getting the control's ActivateDTCs property
//
HRESULT
CProxyFrame::HrGetPropActivateDTCs(BOOL& activateDTCs)
{
	HRESULT hr = S_OK;

	activateDTCs = m_fActivateDTCs;

	_ASSERTE(SUCCEEDED(hr));
	return hr;
}



// Implements setting the control's ShowDetails property
//
HRESULT
CProxyFrame::HrSetPropShowAllTags(BOOL showAllTags)
{
	HRESULT hr = S_OK;

	if (m_fActivated)
	{
		if (SUCCEEDED(hr = HrTridentSetPropBool(IDM_SHOWALLTAGS, showAllTags)))
			m_fShowAllTags = showAllTags;
	}
	else
		m_fShowAllTags = showAllTags;

	_ASSERTE(SUCCEEDED(hr));
	return hr;
}


// Implements getting the control's ShowDetails property
//
HRESULT
CProxyFrame::HrGetPropShowAllTags(BOOL& showAllTags)
{
	HRESULT hr = S_OK;

	showAllTags = m_fShowAllTags;

	_ASSERTE(SUCCEEDED(hr));
	return hr;
}


// Implements setting the control's ShowBorders property
//
HRESULT
CProxyFrame::HrSetPropShowBorders(BOOL showBorders)
{
	HRESULT hr = S_OK;

	if (m_fActivated)
	{
		if (SUCCEEDED(hr = HrTridentSetPropBool(IDM_SHOWZEROBORDERATDESIGNTIME, showBorders)))
			m_fShowBorders = showBorders;
	}
	else
		m_fShowBorders = showBorders;

	_ASSERTE(SUCCEEDED(hr));
	return hr;
}


// Implements getting the control's ShowBorders property
//
HRESULT
CProxyFrame::HrGetPropShowBorders(BOOL& showBorders)
{
	HRESULT hr = S_OK;

	showBorders = m_fShowBorders;

	_ASSERTE(SUCCEEDED(hr));
	return hr;
}


// Implements setting the control's Appearance property
//
HRESULT
CProxyFrame::HrSetDisplay3D(BOOL bVal)
{
	m_fDisplay3D = bVal;
	return S_OK;
}


// Implements getting the control's Appearance property
//
HRESULT
CProxyFrame::HrGetDisplay3D(BOOL& bVal)
{
	bVal = m_fDisplay3D;
	return S_OK;
}


// Implements setting the control's Scrollbars property
//
HRESULT
CProxyFrame::HrSetScrollbars(BOOL bVal)
{
	m_fScrollbars = bVal;
	return S_OK;
}


// Implements getting the control's Scrollbars property
//
HRESULT
CProxyFrame::HrGetScrollbars(BOOL& bVal)
{
	bVal = m_fScrollbars;
	return S_OK;
}


// Implements setting the control's ScrollbarAppearance property
//
HRESULT
CProxyFrame::HrSetDisplayFlatScrollbars(BOOL bVal)
{
	m_fDisplayFlatScrollbars = bVal;
	return S_OK;
}


// Implements getting the control's ScrollbarAppearance property
//
HRESULT
CProxyFrame::HrGetDisplayFlatScrollbars(BOOL& bVal)
{
	bVal = m_fDisplayFlatScrollbars;
	return S_OK;
}


// Implements setting the control's AbsoluteDropMode property
//
HRESULT
CProxyFrame::HrSetAbsoluteDropMode(BOOL dropMode)
{
	HRESULT hr = S_OK;

	if (m_fActivated)
	{
		VARIANT var;

		VariantInit(&var);

		V_VT(&var) = VT_BOOL;
#pragma warning(disable: 4310) // cast truncates constant value
		V_BOOL(&var) = (dropMode) ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value

		if (SUCCEEDED(hr = HrExecCommand(&GUID_TriEditCommandGroup, IDM_TRIED_SET_2D_DROP_MODE,
			MSOCMDEXECOPT_DONTPROMPTUSER, &var, NULL)))
			m_fAbsoluteDropMode = dropMode;
	}
	else
		m_fAbsoluteDropMode = dropMode;

	_ASSERTE(SUCCEEDED(hr));
	return hr;
}


// Implements getting the control's AbsoluteDropMode property
//
HRESULT
CProxyFrame::HrGetAbsoluteDropMode(BOOL& dropMode)
{
	HRESULT hr = S_OK;

	dropMode = m_fAbsoluteDropMode;
	return hr;
}


// Implements setting the control's SnapToGrid property
//
HRESULT
CProxyFrame::HrSetSnapToGrid(BOOL snapToGrid)
{
	HRESULT hr = S_OK;

	if (m_fActivated)
	{
		VARIANT var;
		POINT pt = {0};

		VariantInit(&var);
		if ( snapToGrid )
		{
			pt.y = m_ulSnapToGridY;
			pt.x = m_ulSnapToGridX;
		}
		else
		{
			pt.y = 0;
			pt.x = 0;
		}

		V_VT(&var) = VT_BYREF;
		V_BYREF(&var) = &pt;

		if (SUCCEEDED(hr = HrExecCommand(&GUID_TriEditCommandGroup, IDM_TRIED_SET_ALIGNMENT,
			MSOCMDEXECOPT_DONTPROMPTUSER, &var, NULL)))
			m_fSnapToGrid = snapToGrid;

	}
	else
		m_fSnapToGrid = snapToGrid;

	_ASSERTE(SUCCEEDED(hr));
	return hr;
}


// Implements getting the control's SnapToGrid property
//
HRESULT
CProxyFrame::HrGetSnapToGrid(BOOL& snapToGrid)
{
	HRESULT hr = S_OK;

	snapToGrid = m_fSnapToGrid;
	return hr;
}


// Implements setting the control's SnapToGridX property
//
HRESULT
CProxyFrame::HrSetSnapToGridX(LONG snapToGridX)
{
	HRESULT hr = S_OK;

	if ( 0 >= snapToGridX )
	{
		return DE_E_INVALIDARG;
	}

	if (m_fActivated)
	{
		VARIANT var;
		POINT pt = {0};

		VariantInit(&var);

		pt.x = snapToGridX;
		pt.y = m_ulSnapToGridY;

		V_VT(&var) = VT_BYREF;
		V_BYREF(&var) = &pt;

		if (SUCCEEDED(hr = HrExecCommand(&GUID_TriEditCommandGroup, IDM_TRIED_SET_ALIGNMENT,
			MSOCMDEXECOPT_DONTPROMPTUSER, &var, NULL)))
			m_ulSnapToGridX = snapToGridX;
	}
	else
		m_ulSnapToGridX = snapToGridX;

	_ASSERTE(SUCCEEDED(hr));
	return hr;
}


// Implements getting the control's SnapToGridX property
//
HRESULT
CProxyFrame::HrGetSnapToGridX(LONG& snapToGridX)
{
	HRESULT hr = S_OK;

	snapToGridX = m_ulSnapToGridX;
	return hr;
}


// Implements setting the control's SnapToGridY property
//
HRESULT
CProxyFrame::HrSetSnapToGridY(LONG snapToGridY)
{
	HRESULT hr = S_OK;

	if ( 0 >= snapToGridY )
	{
		return DE_E_INVALIDARG;
	}

	if (m_fActivated)
	{
		VARIANT var;
		POINT pt = {0};

		VariantInit(&var);
		pt.y = snapToGridY;
		pt.x = m_ulSnapToGridX;

		V_VT(&var) = VT_BYREF;
		V_BYREF(&var) = &pt;

		if (SUCCEEDED(hr = HrExecCommand(&GUID_TriEditCommandGroup, IDM_TRIED_SET_ALIGNMENT,
			MSOCMDEXECOPT_DONTPROMPTUSER, &var, NULL)))
			m_ulSnapToGridY = snapToGridY;
	}
	else
		m_ulSnapToGridY = snapToGridY;

	_ASSERTE(SUCCEEDED(hr));
	return hr;
}


// Implements getting the control's SnapToGridY property
//
HRESULT
CProxyFrame::HrGetSnapToGridY(LONG& snapToGridY)
{
	HRESULT hr = S_OK;

	snapToGridY = m_ulSnapToGridY;
	return hr;
}


// Implements setting the control's DocumentHTML property
//
HRESULT
CProxyFrame::HrSetDocumentHTML(BSTR bVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(bVal);

	if (NULL == bVal)
		return E_INVALIDARG;

	if (m_pCtl->IsUserMode ())
	{
		hr = DE_E_UNEXPECTED;

		AssureActivated ();
		if ( m_fActivated )
		{
			m_bstrBaseURL = L"";
			m_bfPreserveDirtyFlagAcrossBrowseMode = FALSE;
			if ( 0 == SysStringLen ( bVal ) )
			{
				CComBSTR bstrMT = GetInitialHTML ();
				hr = LoadBSTRDeferred ( bstrMT );
			}
			else
			{
				hr = LoadBSTRDeferred ( bVal );
			}

			if ( FAILED ( hr ) )
			{
				goto error;
			}

			// We've reset the contents of the control.  Go back to default save mechanism.
			m_pSite->SetSaveAsUnicode ( FALSE );
		}
	}

error:

	return hr;
}


// Implements getting the control's DocumentHTML property
//
HRESULT
CProxyFrame::HrGetDocumentHTML(BSTR* bVal)
{
	HRESULT hr			= S_OK;
	BOOL	bfWasDirty	= FALSE;

	_ASSERTE(bVal);

	if (NULL == bVal)
		return E_INVALIDARG;

	if ( m_bfIsLoading )
		return DE_E_UNEXPECTED;	// This is invalid while document is still loading.

	if ( FAILED ( hr = m_pSite->HrIsDirtyIPersistStreamInit(bfWasDirty) ) )
	{
		_ASSERTE ( SUCCEEDED ( hr ) );
		bfWasDirty = FALSE;	// what else can we do in a situation like this?
	}

	AssureActivated ();

	if (m_fActivated)
	{
		_ASSERTE(m_pSite);

		hr = m_pSite->HrSaveToBstr(bVal, m_dwFilterOutFlags );

		// Preserve original dirty state.
		if ( bfWasDirty )
		{
			SetDirtyFlag ( TRUE );
		}
	}

	return hr;
}


// Implements setting the control's SourceCodePreservation property
//
HRESULT
CProxyFrame::HrSetPreserveSource(BOOL bVal)
{
	m_fPreserveSource = bVal;
	if (m_fPreserveSource)
		m_dwFilterFlags = filterAll;
	else
		m_dwFilterFlags = filterDTCs | filterASP;

	return S_OK;
}


// Implements getting the control's SourceCodePreservation property
//
HRESULT
CProxyFrame::HrGetPreserveSource(BOOL& bVal)
{
	bVal = m_fPreserveSource;
	return S_OK;
}


// Implements getting the control's read-only IsDirty property
//
HRESULT
CProxyFrame::HrGetIsDirty(BOOL& bVal)
{
	HRESULT hr = S_OK;

	bVal = FALSE;

	AssureActivated ();

	if (m_fActivated)
	{
		hr = m_pSite->HrIsDirtyIPersistStreamInit(bVal);
	}

	return hr;
}


//	Implements getting the BaseURL property
//
HRESULT
CProxyFrame::GetBaseURL ( CComBSTR& bstrBaseURL )
{
	AssureActivated ();

	if ( NULL == m_bstrBaseURL.m_str )
	{
		bstrBaseURL = L"";
	}
	else
	{
		bstrBaseURL = m_bstrBaseURL;
	}
	return S_OK;
}


//	Implements setting the BaseURL property.
//	NOTE:
//	The BaseURL can't be (effectively) changed if there's a <BASE HREF=XXX> tag in
//	the document.  Our pluggable Protocol's CombineURL is never called in this case,
//	so don't misguide the user by changing the property.
//
//	Pay attention to m_bfBaseURLFromBASETag before calling to set the value from
//	the routine parsing the <BASE> tag!
//
HRESULT
CProxyFrame::SetBaseURL ( CComBSTR& bstrBaseURL )
{
	HRESULT hr = S_OK;

	_ASSERTE ( bstrBaseURL );

	// Non-persisted property.  Ignore if not in UserMode.
	if ( m_pCtl->IsUserMode () )
	{
		if ( m_bfBaseURLFromBASETag )
		{
			return S_FALSE;
		}
		else
		{
			if ( NULL == m_bstrBaseURL.m_str )
			{
				m_bstrBaseURL = L"";
			}

			// If this test succeedes, the user has done something like x.BaseURL = x.DOM.url or
			// x.BaseURL = y.DOM.url.
			// Response: bstrBaseURL may be the bare protocol prefix, or a prefix with a URL attached
			// for example: dhtmled0:(http://www.microsoft.com).
			// Strip off the prefix and parens (if they exist) and use the interior URL.
			if ( 0 == _wcsnicmp ( bstrBaseURL.m_str, g_wszProtocolPrefix, wcslen ( g_wszProtocolPrefix ) ) )
			{
				CComBSTR bstrNew = bstrBaseURL.m_str;

				// There must be a colon; it would be possibe to have a legitimate base url beginning with g_wszProtocolPrefix
				WCHAR* pwcURL = wcschr ( bstrNew, (WCHAR)':' );
				if ( NULL != pwcURL )
				{
					// Find the first open paren:
					pwcURL = wcschr ( pwcURL, (WCHAR)'(' );
					
					if ( NULL == pwcURL )
					{
						bstrBaseURL = L"";	// No (...)? Set the Base to empty.  Input must have been bare protocol ID.
					}
					else
					{
						pwcURL++;	// Step past the paren.

						// Strip of dhtmledXXX:( ...to... ) and set the BaseURL to what remains.
						_ASSERTE ( (WCHAR)')' == pwcURL[wcslen(pwcURL)-1] );
						if ( (WCHAR)')' == pwcURL[wcslen(pwcURL)-1] )
						{
							pwcURL[wcslen(pwcURL)-1] = (WCHAR)'\0';
							bstrBaseURL = pwcURL;
						}
						else
						{
							// Unexpected:  ill formed pluggable protocol id:
							// starts with dhtml[n[n]]:( but does not end with ).
							// If we skipped it, we would crash.  Best to use an empty base URL.
							bstrBaseURL = L"";
						}
					}
				}
			}

			if ( 0 != wcscmp ( m_bstrBaseURL.m_str, bstrBaseURL.m_str ) )
			{
				m_bstrBaseURL = bstrBaseURL;
				m_bfIsLoading = TRUE;

				// Can't Exec without a command target:
				if ( NULL != m_pSite->GetCommandTarget() )
				{
					// Reload the page, revaluating relative links.
					hr = HrExecCommand(&CGID_MSHTML, IDM_REFRESH, MSOCMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
				}
			}
		}
	}
	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//	Accelerator handler implementations
//
///////////////////////////////////////////////////////////////////////////////////////////


//	Nudge accelerator handler
//	Nudge the selection in the given direction by one pixle if SnaptoGrid is off, or by
//	the SnaptoGridX/Y quantity if SnapToGrid is on.
//
HRESULT
CProxyFrame::HrNudge(DENudgeDirection dir)
{
	HRESULT		hr		= S_FALSE;
	OLECMDF		cmdf	= (OLECMDF) 0;
	VARIANT		var;
	LPVARIANT	pVarIn	= &var;
	LONG		lXDelta	= m_fSnapToGrid ? m_ulSnapToGridX : 1;
	LONG		lYDelta	= m_fSnapToGrid ? m_ulSnapToGridY : 1;

	if (FAILED(hr = HrQueryStatus(&GUID_TriEditCommandGroup, IDM_TRIED_NUDGE_ELEMENT, &cmdf)))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto cleanup;
	}

	if (cmdf & OLECMDF_SUPPORTED && cmdf & OLECMDF_ENABLED)
	{
		LPPOINT lpPoint = new POINT;

		if (NULL == lpPoint)
		{
			hr = E_OUTOFMEMORY;
			goto cleanup;
		}
		_ASSERTE(lpPoint);

		lpPoint->x = 0;
		lpPoint->y = 0;

		// Set increment to snap to absolute grid, not relative grid.
		// Find the selections current position and set increment modulo that position.
		// This assures the first nudge snaps to a grid corner.
		if ( m_fSnapToGrid )
		{
			POINT	ptSelPos;
			if ( SUCCEEDED ( GetSelectionPos ( &ptSelPos ) ) )
			{
				LONG lXNorm = ptSelPos.x % lXDelta;
				LONG lYNorm = ptSelPos.y % lYDelta;
				lXDelta = lXNorm ? lXNorm : lXDelta;
				lYDelta = lYNorm ? lYNorm : lYDelta;
			}
		}

		switch(dir)
		{
		case deNudgeUp:
			{
				lpPoint->x = 0;
				lpPoint->y = -lYDelta;
			}
			break;

		case deNudgeDown:
			{
				lpPoint->x = 0;
				lpPoint->y = lYDelta;
			}
			break;

		case deNudgeLeft:
			{
				lpPoint->x = -lXDelta;
				lpPoint->y = 0;
			}
			break;

		case deNudgeRight:
			{
				lpPoint->x = lXDelta;
				lpPoint->y = 0;
			}
			break;

		default: // move right by default
			{
				lpPoint->x = lXDelta;
				lpPoint->y = 0;
			}
			break;
		}

		VariantInit(pVarIn);
		V_VT(pVarIn) = VT_BYREF;
		V_BYREF(pVarIn) = lpPoint;

		if (FAILED(hr = HrExecCommand(&GUID_TriEditCommandGroup, IDM_TRIED_NUDGE_ELEMENT, MSOCMDEXECOPT_DONTPROMPTUSER, pVarIn, NULL)))
		{
			_ASSERTE(SUCCEEDED(hr));
			goto cleanup;
		}

		hr = S_OK;
	}
	else
		hr = S_FALSE;

cleanup:
	return hr;
}


//	Accelerator handler
//	Toggle the absolute positioned property of the selected object
//
HRESULT
CProxyFrame::HrToggleAbsolutePositioned()
{
	HRESULT hr = S_FALSE;
	OLECMDF cmdf = (OLECMDF) 0;

	if (FAILED(hr = HrQueryStatus(&GUID_TriEditCommandGroup, IDM_TRIED_MAKE_ABSOLUTE, &cmdf)))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto cleanup;
	}

	if (cmdf & OLECMDF_SUPPORTED && cmdf & OLECMDF_ENABLED)
	{
		if (FAILED(hr = HrExecCommand(&GUID_TriEditCommandGroup, IDM_TRIED_MAKE_ABSOLUTE,
			MSOCMDEXECOPT_DONTPROMPTUSER, NULL, NULL)))
		{
			_ASSERTE(SUCCEEDED(hr));
			goto cleanup;
		}

		hr = S_OK;
	}

cleanup:
	return hr;
}


//	Accelerator handler
//	Make a link out of the current selection (with UI.)
//
HRESULT
CProxyFrame::HrHyperLink()
{
	HRESULT hr = S_FALSE;
	OLECMDF cmdf = (OLECMDF) 0;

	if (FAILED(hr = HrQueryStatus(&GUID_TriEditCommandGroup, IDM_TRIED_HYPERLINK, &cmdf)))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto cleanup;
	}

	if (cmdf & OLECMDF_SUPPORTED && cmdf & OLECMDF_ENABLED)
	{
		if (FAILED(hr = HrExecCommand(&GUID_TriEditCommandGroup, IDM_TRIED_HYPERLINK,
			MSOCMDEXECOPT_PROMPTUSER, NULL, NULL)))
		{
			_ASSERTE(SUCCEEDED(hr));
			goto cleanup;
		}

		hr = S_OK;
	}

cleanup:
	return hr;
}


//	Accelerator handler
//	Increase the indent of the current selection.
//
HRESULT
CProxyFrame::HrIncreaseIndent()
{
	HRESULT hr = S_FALSE;
	OLECMDF cmdf = (OLECMDF) 0;

	if (FAILED(hr = HrQueryStatus(&GUID_TriEditCommandGroup, IDM_TRIED_INDENT, &cmdf)))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto cleanup;
	}

	if (cmdf & OLECMDF_SUPPORTED && cmdf & OLECMDF_ENABLED)
	{
		if (FAILED(hr = HrExecCommand(&GUID_TriEditCommandGroup, IDM_TRIED_INDENT,
			MSOCMDEXECOPT_DONTPROMPTUSER, NULL, NULL)))
		{
			_ASSERTE(SUCCEEDED(hr));
			goto cleanup;
		}

		hr = S_OK;
	}

cleanup:
	return hr;
}


//	Accelerator handler
//	Decrease the indent of the current selection.
//
HRESULT
CProxyFrame::HrDecreaseIndent()
{
	HRESULT hr = S_FALSE;
	OLECMDF cmdf = (OLECMDF) 0;

	if (FAILED(hr = HrQueryStatus(&GUID_TriEditCommandGroup, IDM_TRIED_OUTDENT, &cmdf)))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto cleanup;
	}

	if (cmdf & OLECMDF_SUPPORTED && cmdf & OLECMDF_ENABLED)
	{
		if (FAILED(hr = HrExecCommand(&GUID_TriEditCommandGroup, IDM_TRIED_OUTDENT,
			MSOCMDEXECOPT_DONTPROMPTUSER, NULL, NULL)))
		{
			_ASSERTE(SUCCEEDED(hr));
			goto cleanup;
		}

		hr = S_OK;
	}

cleanup:
	return hr;
}


//	Check for and handle control-specific accelerators.  If none is found, call TriEdit to handle it.
//	
HRESULT
CProxyFrame::HrHandleAccelerator(LPMSG lpmsg)
{
	HRESULT hr = S_FALSE;
	BOOL fControl = (0x8000 & GetKeyState(VK_CONTROL));
	BOOL fShift = (0x8000 & GetKeyState(VK_SHIFT));

    if (lpmsg->message == WM_KEYDOWN && lpmsg->wParam == VK_UP)
	{
		hr = HrNudge(deNudgeUp);
	}
    else if (lpmsg->message == WM_KEYDOWN && lpmsg->wParam == VK_DOWN)
	{
		hr = HrNudge(deNudgeDown);
	}
    else if (lpmsg->message == WM_KEYDOWN && lpmsg->wParam == VK_LEFT)
	{
		hr = HrNudge(deNudgeLeft);
	}
    else if (lpmsg->message == WM_KEYDOWN && lpmsg->wParam == VK_RIGHT)
	{
		hr = HrNudge(deNudgeRight);
	}
	else if (lpmsg->message == WM_KEYDOWN && lpmsg->wParam == 'K' && fControl)
	{
		hr = HrToggleAbsolutePositioned();
	}
	else if (lpmsg->message == WM_KEYDOWN && lpmsg->wParam == 'L' && fControl)
	{
		hr = HrHyperLink();
	}
	else if (lpmsg->message == WM_KEYDOWN && lpmsg->wParam == 'T' && !fShift && fControl)
	{
		hr = HrIncreaseIndent();
	}
	else if (lpmsg->message == WM_KEYDOWN && lpmsg->wParam == 'T' && fShift && fControl)
	{
		hr = HrDecreaseIndent();
	}
	else if (lpmsg->message == WM_KEYDOWN && lpmsg->wParam == VK_TAB && fControl)
	{
		// Process control-tab keys as belonging to the container; this allows the user
		// to tab out of the control in non-MDI apps.  MDI uses control-tab to switch
		// windows, thus these apps (like VID) do not pass them to us.
		IOleControlSite* piControlSite = m_pCtl->GetControlSite ();
		_ASSERTE ( piControlSite );
		if ( NULL != piControlSite )
		{
			// Eat the control key, but preserve shift to perform reverse tabbing.
			// KEYMOD_SHIFT = 0x00000001, but isn't defined in any header...
			DWORD dwModifiers = fShift ? 1 : 0;

			hr = piControlSite->TranslateAccelerator ( lpmsg, dwModifiers );
		}
	}

	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//	BaseURL helper routines
//
///////////////////////////////////////////////////////////////////////////////////////////


//	Override the default BaseURL if there is one or more <BASE HREF=...> tags
//	in the document.  If successful, set m_bfBaseURLFromBASETag to TRUE
//	If multiple BASE tags exist, simply use the last one.
//	Equivilent script:  baseurl = document.all.tags("BASE")[<LAST>].href,
//	where <LAST> is derived.
//
HRESULT
CProxyFrame::SetBaseURLFromBaseHref ()
{
	HRESULT		hr	= S_OK;
	CComBSTR	bstrBase;

	if ( !m_bfBaseURLFromBASETag )
	{
		if ( SUCCEEDED ( hr ) )
		{
			CComPtr<IHTMLDocument2> spHtmlDoc = NULL;
			hr = HrGetDoc( &spHtmlDoc );
			if ( spHtmlDoc && SUCCEEDED ( hr ) )
			{
				CComPtr<IHTMLElementCollection> spAll = NULL;
				hr = spHtmlDoc->get_all ( &spAll );
				if ( spAll && SUCCEEDED ( hr ) )
				{
					CComVariant varTag		= L"BASE";
					IDispatch*	piDispTags	= NULL;

					hr = spAll->tags ( varTag, &piDispTags );
					if ( piDispTags && SUCCEEDED ( hr ) )
					{
						CComQIPtr<IHTMLElementCollection, &IID_IHTMLElementCollection> spBases (piDispTags);
						piDispTags->Release ();
						piDispTags = NULL;
						if ( spBases )
						{
							long	cBases = 0;
							hr = spBases->get_length ( &cBases );
							if ( SUCCEEDED ( hr ) && ( 0 != cBases ) )
							{
								CComVariant varName;
								varName.vt = VT_I2;

								for ( varName.iVal = 0; varName.iVal < cBases; varName.iVal++ )
								{
									IDispatch*	piDispBase = NULL;
									CComVariant varValue;

									hr = spBases->item ( varName, varName, &piDispBase );
									if ( piDispBase && SUCCEEDED ( hr ) )
									{
										CComQIPtr<IHTMLElement, &IID_IHTMLElement> spElem ( piDispBase );
										piDispBase->Release ();
										piDispBase = NULL;

										if ( spElem )
										{
											varValue.Clear ();
											hr = spElem->getAttribute ( L"HREF", FALSE, &varValue );
											if ( SUCCEEDED ( hr ) )
											{
												hr = varValue.ChangeType ( VT_BSTR );
												if ( SUCCEEDED ( hr ) )
												{
													if ( 0 != SysStringLen ( varValue.bstrVal ) )
													{
														bstrBase = varValue.bstrVal;
													}
												}
											}
										}
									}
								}
								if ( 0 != bstrBase.Length () )
								{
									hr = SetBaseURL ( bstrBase );	// This clears m_bfBaseURLIsDefault
									m_bfBaseURLFromBASETag = TRUE;
								}
							}
						}
					}
				}
			}
		}
	}
	return hr;
}


//	Set the m_bstrBaseURL value using m_bstrCurDocPath.
//	With URLs, it may be impossible to be certain about the correct BaseURL,
//	so make an intellegent guess.  With files, it should be deterministic.
//	
HRESULT
CProxyFrame::SetBaseURLFromCurDocPath ( BOOL bfIsURL )
{
	m_bfBaseURLFromBASETag = FALSE;	// We're reloading: whipe this out.
	if ( bfIsURL )
	{
		return SetBaseURLFromURL ( m_bstrCurDocPath );
	}
	else
	{
		return SetBaseURLFromFileName ( m_bstrCurDocPath );
	}
}


//	Given a URL_COMPONENTS with nScheme set to INTERNET_SCHEME_FILE,
//	modify the path part to reflect the base path, reconstruct the URL,
//	and set m_bstrBaseURL.
//	Separators may be \ or /.
//
HRESULT
CProxyFrame::SetBaseUrlFromFileUrlComponents ( URL_COMPONENTS & urlc )
{
	TCHAR*	pszPath;
	BOOL	bfBackSlash	= TRUE;
	HRESULT	hr			= S_OK;

	_ASSERTE ( INTERNET_SCHEME_FILE == urlc.nScheme );
	_ASSERTE ( urlc.dwUrlPathLength );
	if ( 0 == urlc.dwUrlPathLength )
	{
		return E_UNEXPECTED;
	}
	pszPath = new TCHAR [urlc.dwUrlPathLength + 3];	// Extra room for \0, dot, and /.
	if ( NULL != pszPath )
	{
		TCHAR	c		= 0;
		int		iPos	= 0;

		// Scan backwards and modify in copy (never in BSTR, please) for beginning, '/' or '\'
		memcpy ( pszPath, urlc.lpszUrlPath, ( urlc.dwUrlPathLength + 1 ) * sizeof(TCHAR) );
		for ( iPos = urlc.dwUrlPathLength - 1; iPos >= 0; iPos-- )
		{
			c = pszPath[iPos];
			pszPath[iPos] = '\0';	// Delete first, ask questions later.  '\' must go.
			if ( '\\' == c )
			{
				break;
			}
			if ( '/' == c )
			{
				bfBackSlash = FALSE;
				break;
			}
		}

		// Space was reserved for an additional two characters, if needed.
		// If empty, add a dot.
		if ( 0 == _tcslen ( pszPath ) )
		{
			_tcscat ( pszPath, TEXT(".") );
		}
		// Add a / or \.
		if ( bfBackSlash )
		{
			_tcscat ( pszPath, TEXT("\\") );
		}
		else
		{
			_tcscat ( pszPath, TEXT("/") );
		}

		urlc.lpszUrlPath = pszPath;
		urlc.dwUrlPathLength = _tcslen ( pszPath );

		DWORD	dwLen = 0;
#ifdef LATE_BIND_URLMON_WININET
		_ASSERTE ( m_pfnInternetCreateUrl );
		(*m_pfnInternetCreateUrl)( &urlc, 0, NULL, &dwLen );	// Get the size required.
#else
		InternetCreateUrl ( &urlc, 0, NULL, &dwLen );	// Get the size required.
#endif // LATE_BIND_URLMON_WININET

		_ASSERTE ( 0 != dwLen );
		TCHAR* pszURL = new TCHAR [ dwLen + 1 ];
		_ASSERTE ( pszURL );
		if ( NULL != pszURL )
		{
			// Incredibly, on Win98, the URL is terminated with a single byte \0.
			// Intializing this buffer to zero assures full termination of the string.
			dwLen += 1;
			memset ( pszURL, 0, sizeof(TCHAR) * dwLen );
#ifdef LATE_BIND_URLMON_WININET
			if ( (*m_pfnInternetCreateUrl)( &urlc, 0, pszURL, &dwLen ) )
#else
			if ( InternetCreateUrl ( &urlc, 0, pszURL, &dwLen ) )
#endif // LATE_BIND_URLMON_WININET
			{
				m_bstrBaseURL = pszURL;
			}
			else
			{
				hr = HRESULT_FROM_WIN32 ( GetLastError () );
			}
			delete [] pszURL;
		}

		delete [] pszPath;
	}
	else
	{
		return E_FAIL;
	}

	return hr;
}


//	The most complicated scenario for "guessing" at the base URL.
//	URLs like http://www.x.com/stuff could be either a file or a directory;
//	a default page might actually be loaded.  We guess based on whether or
//	not the last item in the path contains a period.  If so, we eliminate it.
//	We make sure the path ends with a '/'.
//
HRESULT
CProxyFrame::SetBaseUrlFromUrlComponents ( URL_COMPONENTS & urlc )
{
	_ASSERTE ( INTERNET_SCHEME_FILE != urlc.nScheme );

	BOOL	bfPeriodIncluded	= FALSE;
	HRESULT	hr					= S_OK;

	if ( 0 == urlc.dwSchemeLength )
	{
		m_bstrBaseURL = L"";
		return S_FALSE;
	}

	// Scan backwards over path for beginning, '/'
	TCHAR	c		= 0;
	int		iPos	= 0;

	for ( iPos = urlc.dwUrlPathLength - 1; iPos >= 0; iPos-- )
	{
		c = urlc.lpszUrlPath[iPos];
		if ( '/' == c )
		{
			break;
		}
		if ( '.' == c )
		{
			bfPeriodIncluded = TRUE;
		}
	}

	if ( bfPeriodIncluded )
	{
		if ( 0 > iPos ) iPos = 0;
		urlc.lpszUrlPath[iPos] = '\0';	// Truncate at the '/', or beginning
		urlc.dwUrlPathLength = _tcslen ( urlc.lpszUrlPath );
	}

	// Recreate the URL:
	DWORD	dwLen = 0;
#ifdef LATE_BIND_URLMON_WININET
	_ASSERTE ( m_pfnInternetCreateUrl );
	(*m_pfnInternetCreateUrl)( &urlc, 0, NULL, &dwLen );	// Get the size required.
#else
	InternetCreateUrl ( &urlc, 0, NULL, &dwLen );	// Get the size required.
#endif // LATE_BIND_URLMON_WININET

	_ASSERTE ( 0 != dwLen );
	TCHAR* pszURL = new TCHAR [ dwLen + 1 ];
	_ASSERTE ( pszURL );
	if ( NULL != pszURL )
	{
		dwLen += 1;
		memset ( pszURL, 0, sizeof(TCHAR) * dwLen );
#ifdef LATE_BIND_URLMON_WININET
		if ( (*m_pfnInternetCreateUrl)( &urlc, 0, pszURL, &dwLen ) )
#else
		if ( InternetCreateUrl ( &urlc, 0, pszURL, &dwLen ) )
#endif
		{
			m_bstrBaseURL = pszURL;

			// Append a '/' if needed.
			WCHAR wc = m_bstrBaseURL.m_str[m_bstrBaseURL.Length () - 1];
			if ( ( WCHAR('/') != wc ) && ( NULL != urlc.lpszHostName ) )	// hostname: special case for user pluggable protocols
			{
				m_bstrBaseURL += L"/";
			}
		}
		else
		{
			hr = HRESULT_FROM_WIN32 ( GetLastError () );
		}
		delete [] pszURL;
	}
	return hr;
}


//	Crack the URL, determine if it's a file scheme or other, and call the appropriate handler.
//
HRESULT
CProxyFrame::SetBaseURLFromURL ( const CComBSTR& bstrURL )
{
	USES_CONVERSION;
#define SCHEME_BUF_SIZE			64
#define SCHEME_SIZE				(SCHEME_BUF_SIZE-1)
#define	URL_SEG_LEN_BUF_SIZE	1024
#define	URL_SEG_LEN				(URL_SEG_LEN_BUF_SIZE-1)

	HRESULT			hr	= S_OK;
	URL_COMPONENTS	urlc;
	TCHAR			*ptszScheme		= new TCHAR[SCHEME_BUF_SIZE];
	TCHAR			*ptszHostName	= new TCHAR[URL_SEG_LEN_BUF_SIZE];
	TCHAR			*ptszUrlPath	= new TCHAR[URL_SEG_LEN_BUF_SIZE];

	_ASSERTE ( 0 != bstrURL.Length () );

	memset ( &urlc, 0, sizeof ( urlc ) );
	urlc.dwStructSize		= sizeof ( urlc );
	urlc.lpszScheme			= ptszScheme;
	urlc.dwSchemeLength		= SCHEME_BUF_SIZE;
	urlc.lpszHostName		= ptszHostName;
	urlc.dwHostNameLength	= URL_SEG_LEN;
	urlc.lpszUrlPath		= ptszUrlPath;
	urlc.dwUrlPathLength	= URL_SEG_LEN;

#ifdef LATE_BIND_URLMON_WININET
	_ASSERTE ( m_pfnInternetCrackUrl );
	hr = (*m_pfnInternetCrackUrl)( OLE2T ( bstrURL ), 0, 0, &urlc );
#else
	hr = InternetCrackUrl ( OLE2T ( bstrURL ), 0, 0, &urlc );
#endif // LATE_BIND_URLMON_WININET

	// Flaw in InternetCrackUrl and	InternetCreateUrl:
	// For a pluggable protocol, the host name may be empty.  The empty string instead of NULL
	// returned here causes extera '/'s to be appeneded to the created URL.
	if ( 0 == urlc.dwHostNameLength )
	{
		urlc.lpszHostName = NULL;
	}

	if ( SUCCEEDED ( hr ) )
	{
		if ( INTERNET_SCHEME_FILE == urlc.nScheme )
		{
			hr = SetBaseUrlFromFileUrlComponents ( urlc );
		}
		else
		{
			hr = SetBaseUrlFromUrlComponents ( urlc );
		}
	}

	delete [] ptszScheme;
	delete [] ptszHostName;
	delete [] ptszUrlPath;
	return hr;
}


//	Given a UNC file name, set the m_bstrBaseURL member variable.
//	if bstrFName is empty, set m_bstrBaseURL to empty.
//	Else, scan backward to the first "\" or the beginning of the string.
//	Truncate the string at this point.  If the resultant string is empty,
//	add ".".  Then, add "\".
//
HRESULT
CProxyFrame::SetBaseURLFromFileName ( const CComBSTR& bstrFName )
{
	if ( 0 == bstrFName.Length () )
	{
		m_bstrBaseURL = L"";
	}
	else
	{
		WCHAR* pwzstr = new WCHAR[bstrFName.Length () + 1];
		_ASSERTE ( pwzstr );
		if ( NULL != pwzstr )
		{
			WCHAR	wc		= 0;
			int		iPos	= 0;

			// Scan backwards and modify in copy (never in BSTR, please) for beginning or '\'
			memcpy ( pwzstr, bstrFName.m_str, sizeof(WCHAR) * (bstrFName.Length () + 1) );
			for ( iPos = wcslen ( pwzstr ) - 1; iPos >= 0; iPos-- )
			{
				wc = pwzstr[iPos];
				pwzstr[iPos] = WCHAR('\0');	// Delete first, ask questions later.  '\' must go.
				if ( WCHAR('\\') == wc )
				{
					break;
				}
			}
			m_bstrBaseURL = pwzstr;
			delete [] pwzstr;

			// If empty, add a '.'
			if ( 0 == m_bstrBaseURL.Length () )
			{
				m_bstrBaseURL += L".";
			}
			m_bstrBaseURL += L"\\";
		}
		else
		{
			return E_FAIL;
		}
	}
	return S_OK;
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//	Security oriented routines
//
///////////////////////////////////////////////////////////////////////////////////////////


//	This is a critical security issue:
//	The pluggable protocol's ParseURL is called with PARSE_SECURITY_URL in the SFS control.
//	If the BaseURL is empty, and if we're hosted in Trident, we should return the
//	URL of the hosting page.
//	If there is no Trident host, say we're hosted in VB, return the bootdrive + : + /.
//	Bootdrive is not always C.
//
HRESULT
CProxyFrame::GetSecurityURL (CComBSTR& bstrSecurityURL )
{
	HRESULT			hr						= S_OK;
	IOleClientSite	*piClientSiteUnreffed	= NULL;

	bstrSecurityURL = L"";
	
	piClientSiteUnreffed = m_pCtl->m_spClientSite;
	if ( NULL != piClientSiteUnreffed )
	{
		CComPtr<IOleContainer> spContainer = NULL;
		hr = piClientSiteUnreffed->GetContainer ( &spContainer );
		if ( SUCCEEDED ( hr ) && spContainer )
		{
			CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> spHostDoc ( spContainer );
			if ( spHostDoc )
			{
				CComPtr<IHTMLLocation> spHostLoc = NULL;


				spHostDoc->get_location ( &spHostLoc );
				if ( spHostLoc )
				{
					BSTR bsOut;
					hr = spHostLoc->get_href ( &bsOut );
					if ( SUCCEEDED ( hr ) )
					{
						bstrSecurityURL.Empty ();
						bstrSecurityURL.Attach ( bsOut );
					}
				}
			}
			else
			{
				// If we are not hosted in Trident, use local machine access:
				TCHAR	tszDrive[4];
				GetModuleFileName ( _Module.m_hInst, tszDrive, 3 );	// Get X:\.
				_ASSERTE ( TCHAR(':') == tszDrive[1] );
				_ASSERTE ( TCHAR('\\') == tszDrive[2] );
				bstrSecurityURL = tszDrive;
				hr = S_OK;
			}
		}
	}
	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//	Pluggable protocol oriented routines
//
///////////////////////////////////////////////////////////////////////////////////////////


//	Register our pluggable protocol handler so dhtmledN[N...] is loaded by our code.
//
HRESULT
CProxyFrame::RegisterPluggableProtocol()
{
	HRESULT hr;

	// Get InternetSession

	CComPtr<IInternetSession> srpSession;
#ifdef LATE_BIND_URLMON_WININET
	_ASSERTE ( m_pfnCoInternetGetSession );
	hr = (*m_pfnCoInternetGetSession)(0, &srpSession, 0);
#else
	hr = CoInternetGetSession (0, &srpSession, 0);
#endif // LATE_BIND_URLMON_WININET

	if ( FAILED ( hr ) )
	{
		return hr;
	}

	if(m_pProtInfo == NULL)
	{
		hr = CComObject<CDHTMLEdProtocolInfo>::CreateInstance(&m_pProtInfo);
		if ( FAILED ( hr ) )
		{
			return hr;
		}

		// CreateInstance - doesnt AddRef
		m_pProtInfo->GetUnknown()->AddRef();
	}

	hr = srpSession->RegisterNameSpace(
						static_cast<IClassFactory*>(m_pProtInfo),
						CLSID_DHTMLEdProtocol,
						m_wszProtocol,
						0,
						NULL,
						0);

	if ( FAILED ( hr ) )
	{
		return hr;
	}

	CComQIPtr <IProtocolInfoConnector, &IID_IProtocolInfoConnector> piPic ( m_pProtInfo );
	_ASSERTE ( piPic );
	piPic->SetProxyFrame ( (SIZE_T*)this );

	ATLTRACE( _T("CProxyFrame::Registered ProtocolInfo\n"));

	return NOERROR;
}

//	Unregister the pluggable protocol handler installed in RegisterPluggableProtocol
//
HRESULT
CProxyFrame::UnRegisterPluggableProtocol()
{
	if(m_pProtInfo == NULL)
		return E_UNEXPECTED;

	// Get InternetSession

	HRESULT hr;
	CComPtr<IInternetSession> srpSession;

#ifdef LATE_BIND_URLMON_WININET
	_ASSERTE ( m_pfnCoInternetGetSession );
	hr = (*m_pfnCoInternetGetSession)(0, &srpSession, 0);
#else
	hr = CoInternetGetSession (0, &srpSession, 0);
#endif // LATE_BIND_URLMON_WININET

	if(SUCCEEDED(hr))
	{
		// UnRegister Protocol

		srpSession->UnregisterNameSpace(
							static_cast<IClassFactory*>(m_pProtInfo),
							m_wszProtocol);

	}

	m_pProtInfo->GetUnknown()->Release();
	m_pProtInfo = NULL;

	ATLTRACE(_T("CProxyFrame::UnRegistered ProtocolInfo\n"));

	return NOERROR;
}


//	Workhorse routine that actually performs the loading of the control, including filtering.
//	ParseAndBind calls this to retrieve the data to be displayed in the control.
//
HRESULT
CProxyFrame::GetFilteredStream ( IStream** ppStream )
{
	USES_CONVERSION;
	HRESULT hr = S_OK;
	LPTSTR pFileName = NULL;
	CComPtr<IStream> piStream;
	BOOL	bfLoadingFromBSTR = ( 0 != m_bstrLoadText.Length () );

	*ppStream = NULL;
	m_bfReloadAttempted = TRUE;

	if ( !bfLoadingFromBSTR )
	{
		_ASSERTE(m_bstrCurDocPath);

		pFileName = OLE2T(m_bstrCurDocPath);

		_ASSERTE(pFileName);

		if (NULL == pFileName)
			return E_OUTOFMEMORY;
	}

	if ( bfLoadingFromBSTR )
	{
		hr = m_pSite->HrBstrToStream(m_bstrLoadText, &piStream);
	}
	else if ( m_bfIsURL )
	{
		hr = m_pSite->HrURLToStream(pFileName, &piStream);
	}
	else
	{
		hr = m_pSite->HrFileToStream(pFileName, &piStream);
	}

	if (FAILED( hr ))
	{
		m_bstrCurDocPath.Empty ();
		m_bstrBaseURL.Empty ();

		// Get TriEdit into a reasonable state by loading an empty document
		// If we reinstanced successfully, this should never fail
		// Also, this will make ignoring the above assert benign
		if (FAILED(m_pSite->HrBstrToStream(m_bstrInitialDoc, ppStream)))
		{
			_ASSERTE(SUCCEEDED(hr));
		}

	}
	else
	{
		if ( m_vbBrowseMode )
		{
			piStream->AddRef ();
			*ppStream = piStream;
		}
		else
		{
			hr = m_pSite->HrFilter(TRUE, piStream, ppStream, m_dwFilterFlags);
		}

		if (FAILED(hr))
		{
			m_pSite->HrBstrToStream(m_bstrInitialDoc, ppStream);
		}
		else
		{
			m_dwFilterOutFlags = m_dwFilterFlags;
		}
	}

	// Store the result to return from the (indirectly) called routine,
	// but don't return an error to ParseAndBind!
	if ( FAILED(hr) && ( ! bfLoadingFromBSTR ) )
	{
		m_hrDeferredLoadError = hr;	// Stash this away, we'll pic it up in LoadDocument
		hr = S_OK;
	}

	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//	Document event handling routines
//
///////////////////////////////////////////////////////////////////////////////////////////


HRESULT
CProxyFrame::OnTriEditEvent ( const GUID& iidEventInterface, DISPID dispid )
{
	HRESULT hr = S_OK;

	if ( DIID_HTMLDocumentEvents == iidEventInterface )
	{
		switch ( dispid )
		{
			case DISPID_HTMLDOCUMENTEVENTS_ONKEYPRESS:
			case DISPID_HTMLDOCUMENTEVENTS_ONMOUSEDOWN:
				if ( DISPID_HTMLDOCUMENTEVENTS_ONMOUSEDOWN == dispid )
				{
					m_pCtl->Fire_onmousedown();
				}
				else if ( DISPID_HTMLDOCUMENTEVENTS_ONKEYPRESS == dispid )
				{
					m_pCtl->Fire_onkeypress();
				}

				// Make the control UIActive if it was clicked in.  Since the DocObject swallows the clicks,
				// the control isn't activated automatically.
				// Not needed in browse mode.
				if (  !m_pCtl->m_bUIActive && ! m_vbBrowseMode )
				{
					m_pCtl->DoVerbUIActivate ( NULL, NULL );
					if ( m_hWndObj != NULL )
					{
						::SetFocus( m_hWndObj );
					}
				}
				break;

			case DISPID_HTMLDOCUMENTEVENTS_ONMOUSEMOVE:
				m_pCtl->Fire_onmousemove();
				break;

			case DISPID_HTMLDOCUMENTEVENTS_ONMOUSEUP:
				m_pCtl->Fire_onmouseup();
				// onclick is not delivered in edit mode.  First one lost in broswe mode.
				m_pCtl->Fire_onclick();
				break;

			case DISPID_HTMLDOCUMENTEVENTS_ONMOUSEOUT:
				m_pCtl->Fire_onmouseout();
				break;

			case DISPID_HTMLDOCUMENTEVENTS_ONMOUSEOVER:
				m_pCtl->Fire_onmouseover();
				break;

			case DISPID_HTMLDOCUMENTEVENTS_ONCLICK:
				// We do not fire the onclick event in response.
				// It is only delivered in browse mode, and in addition,
				// the first onclick is lost.  We fire on onmouseup.
				//m_pCtl->Fire_onclick();
				break;

			case DISPID_HTMLDOCUMENTEVENTS_ONDBLCLICK:
				m_pCtl->Fire_ondblclick();
				break;

			case DISPID_HTMLDOCUMENTEVENTS_ONKEYDOWN:
				m_pCtl->Fire_onkeydown();
				break;

			case DISPID_HTMLDOCUMENTEVENTS_ONKEYUP:
				m_pCtl->Fire_onkeyup();
				break;

			case DISPID_HTMLDOCUMENTEVENTS_ONREADYSTATECHANGE:
				m_pCtl->Fire_onreadystatechange();
				break;

			default:
				_ASSERTE ( TRUE );
				break;
		}
	}
	else if ( DIID_HTMLWindowEvents == iidEventInterface )
	{
		// I expected to get these, but I'm not...
		switch ( dispid )
		{
			case DISPID_HTMLWINDOWEVENTS_ONLOAD:
			case DISPID_HTMLWINDOWEVENTS_ONUNLOAD:
			case DISPID_HTMLWINDOWEVENTS_ONHELP:
			case DISPID_HTMLWINDOWEVENTS_ONFOCUS:
			case DISPID_HTMLWINDOWEVENTS_ONBLUR:
			case DISPID_HTMLWINDOWEVENTS_ONERROR:
			case DISPID_HTMLWINDOWEVENTS_ONRESIZE:
			case DISPID_HTMLWINDOWEVENTS_ONSCROLL:
			case DISPID_HTMLWINDOWEVENTS_ONBEFOREUNLOAD:
				hr = S_OK;
				break;
			
			default:
				_ASSERTE ( TRUE );
				break;
		}
	}
	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////
//
//	Dynamic loading routines, used in 4.0 versions
//
///////////////////////////////////////////////////////////////////////////////////////////


#ifdef LATE_BIND_URLMON_WININET
//	Load Urlmon and Wininet and get the proc addresses of every routine we use.
//	We must be able to register the control, even if these libraries are not installed.
//	NOTE:
//	This routine loads ANSI versions.  Needs addaptation for UNICODE.
//
BOOL CProxyFrame::DynLoadLibraries ()
{
	m_hUlrMon	= LoadLibrary ( TEXT("URLMON.DLL") );
	m_hWinINet	= LoadLibrary ( TEXT("WININET.DLL") );
	if ( ( NULL == m_hUlrMon ) || ( NULL == m_hWinINet ) )
	{
		DynUnloadLibraries ();
		return FALSE;
	}

	m_pfnCoInternetCombineUrl	= (PFNCoInternetCombineUrl)GetProcAddress (
									m_hUlrMon, "CoInternetCombineUrl" );
									_ASSERTE ( m_pfnCoInternetCombineUrl );
	m_pfnCoInternetParseUrl		= (PFNCoInternetParseUrl)GetProcAddress (
									m_hUlrMon, "CoInternetParseUrl" );
									_ASSERTE ( m_pfnCoInternetParseUrl );
	m_pfnCreateURLMoniker		= (PFNCreateURLMoniker)GetProcAddress (
									m_hUlrMon, "CreateURLMoniker" );
									_ASSERTE ( m_pfnCreateURLMoniker );
	m_pfnCoInternetGetSession	= (PFNCoInternetGetSession)GetProcAddress (
									m_hUlrMon, "CoInternetGetSession" );
									_ASSERTE ( m_pfnCoInternetGetSession );
	m_pfnURLOpenBlockingStream	= (PFNURLOpenBlockingStream)GetProcAddress (
									m_hUlrMon, "URLOpenBlockingStreamA" );
									_ASSERTE ( m_pfnURLOpenBlockingStream );

	m_pfnDeleteUrlCacheEntry	= (PFNDeleteUrlCacheEntry)GetProcAddress (
									m_hWinINet, "DeleteUrlCacheEntry" );
									_ASSERTE ( m_pfnDeleteUrlCacheEntry );
	m_pfnInternetCreateUrl		= (PFNInternetCreateUrl)GetProcAddress (
									m_hWinINet, "InternetCreateUrlA" );
									_ASSERTE ( m_pfnInternetCreateUrl );
	m_pfnInternetCrackUrl		= (PFNInternetCrackURL)GetProcAddress (
									m_hWinINet, "InternetCrackUrlA" );
									_ASSERTE ( m_pfnInternetCrackUrl );

	return ( m_pfnCoInternetCombineUrl && m_pfnCoInternetParseUrl && m_pfnCreateURLMoniker &&
		m_pfnCoInternetGetSession && m_pfnURLOpenBlockingStream && m_pfnDeleteUrlCacheEntry &&
		m_pfnInternetCreateUrl && m_pfnInternetCrackUrl );
}


//	Release the libraries loaded by DynLoadLibraries
//
void CProxyFrame::DynUnloadLibraries ()
{
	if ( NULL != m_hUlrMon )
	{
		FreeLibrary ( m_hUlrMon );
		m_hUlrMon = NULL;
	}
	if ( NULL != m_hWinINet )
	{
		FreeLibrary ( m_hWinINet );
		m_hWinINet = NULL;
	}

	m_pfnCoInternetCombineUrl	= NULL;
	m_pfnCoInternetParseUrl		= NULL;
	m_pfnCreateURLMoniker		= NULL;
	m_pfnCoInternetGetSession	= NULL;
	m_pfnURLOpenBlockingStream	= NULL;

	m_pfnDeleteUrlCacheEntry	= NULL;
	m_pfnInternetCreateUrl		= NULL;
	m_pfnInternetCrackUrl		= NULL;
}
#endif // LATE_BIND_URLMON_WININET


///////////////////////////////////////////////////////////////////////////////////////////
//
//	Utility routines
//
///////////////////////////////////////////////////////////////////////////////////////////

//	Return the IHTMLDocument2 pointer from the hosted doc.
//
HRESULT
CProxyFrame::HrGetDoc(IHTMLDocument2 **ppDoc)
{
	HRESULT hr = E_FAIL;
	IUnknown* lpUnk = m_pSite->GetObjectUnknown();

	if (FALSE == m_fActivated)
		return DE_E_UNEXPECTED;

	_ASSERTE(ppDoc);

	if (NULL == ppDoc)
		return DE_E_INVALIDARG;

	_ASSERTE(lpUnk);

	if ( m_bfIsLoading )
		return DE_E_UNEXPECTED;	// This is invalid while document is still loading.

	if (lpUnk != NULL)
	{
		// Request the "document" object from the MSHTML
		*ppDoc = NULL;
		hr = lpUnk->QueryInterface(IID_IHTMLDocument2, (void **)ppDoc);
	}

	_ASSERTE(SUCCEEDED(hr)); // this should always succeed
	return hr;
}


//	Helper routine to set any Boolean Trident property
//
HRESULT
CProxyFrame::HrTridentSetPropBool(ULONG cmd, BOOL bVal)
{
	HRESULT hr = S_OK;
	VARIANT varIn;

	VariantInit(&varIn);
	V_VT(&varIn) = VT_BOOL;

#pragma warning(disable: 4310) // cast truncates constant value
	bVal ? V_BOOL(&varIn) = VARIANT_TRUE : V_BOOL(&varIn) = VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value

	hr = HrExecCommand(&CGID_MSHTML, cmd, MSOCMDEXECOPT_DONTPROMPTUSER, &varIn, NULL);

	// this should always succeed since all props 
	// should be set in correct phases of Trident creation

	_ASSERTE(SUCCEEDED(hr)); 
	return hr;
}


//	Helper routine to get any Boolean Trident property
//
HRESULT
CProxyFrame::HrTridentGetPropBool(ULONG cmd, BOOL& bVal)
{
	HRESULT hr = S_OK;
	OLECMDF cmdf = (OLECMDF) 0;

	if (SUCCEEDED(HrQueryStatus(&CGID_MSHTML, cmd, &cmdf)))
	{
		bVal = (cmdf & OLECMDF_ENABLED) == OLECMDF_ENABLED ? TRUE : FALSE;
	}

	// this should always succeed since all props 
	// should be set in correct phases of Trident creation

	_ASSERTE(SUCCEEDED(hr));
	return hr;
}


//	Store the BSTR so LoadFilteredStream can access it, and load a URL with our protocol
//	to kick off the load/resolve/display through the pluggable protocol handler.
//
//	Clear the BaseURL, and mark the control "Loading..."
//
HRESULT
CProxyFrame::LoadBSTRDeferred ( BSTR bVal )
{
	HRESULT	hr	= E_FAIL;

	_ASSERTE ( m_pUnkTriEdit );

	m_bstrLoadText = bVal;

	CComPtr<IMoniker> srpMoniker;
	CComPtr<IBindCtx> srpBindCtx;
	CComQIPtr<IPersistMoniker, &IID_IPersistMoniker> srpPM (m_pUnkTriEdit);
	_ASSERTE ( srpPM );

	if ( srpPM )
	{
#ifdef LATE_BIND_URLMON_WININET
		_ASSERTE ( m_pfnCreateURLMoniker );
		hr = (*m_pfnCreateURLMoniker)( NULL, m_wszProtocolPrefix, &srpMoniker );
#else
		hr = CreateURLMoniker ( NULL, m_wszProtocolPrefix, &srpMoniker );
#endif // LATE_BIND_URLMON_WININET

		_ASSERTE ( SUCCEEDED( hr ) );
		if ( SUCCEEDED ( hr ) )
		{
			hr = ::CreateBindCtx(NULL, &srpBindCtx);
			_ASSERTE ( SUCCEEDED( hr ) );
			if ( SUCCEEDED ( hr ) )
			{
				m_bfIsLoading = TRUE;
				m_bfBaseURLFromBASETag = FALSE;

				hr = srpPM->Load(FALSE, srpMoniker,  srpBindCtx, STGM_READ);

				_ASSERTE ( SUCCEEDED( hr ) );
			}
		}
	}
	return hr;
}


//	Set the document stream's dirty flag
//
HRESULT
CProxyFrame::SetDirtyFlag ( BOOL bfMakeDirty )
{
	CComVariant varDirty;

	varDirty = bfMakeDirty ? true : false;

	return HrExecCommand(&CGID_MSHTML, IDM_SETDIRTY, MSOCMDEXECOPT_DONTPROMPTUSER, &varDirty, NULL);
}


// properties that can be set only after TriEdit is in running state
HRESULT
CProxyFrame::HrSetRuntimeProperties()
{
	HRESULT hr = S_OK;

	if (FAILED(hr = HrSetPropActivateControls(m_fActivateControls)))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto error;
	}

	if (FAILED(hr = HrSetPropActivateApplets(m_fActivateApplets)))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto error;
	}

	if (FAILED(hr = HrSetPropActivateDTCs(m_fActivateDTCs)))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto error;
	}

	// toggle properties

	if (FAILED(hr = HrSetPropShowAllTags(m_fShowAllTags)))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto error;
	}


	if (FAILED(hr = HrSetPropShowBorders(m_fShowBorders)))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto error;
	}

error:

	return hr;
}


HRESULT
CProxyFrame::HrGetCurrentDocumentPath(BSTR* bVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(bVal);

	if (NULL == bVal)
		return E_INVALIDARG;

	*bVal = m_bstrCurDocPath.Copy ();
	return hr;
}


// properties that can only be set after UIActivation
HRESULT
CProxyFrame::HrSetDocLoadedProperties()
{
	HRESULT hr = S_OK;
	BOOL bGoodUndoBehavior = TRUE;

	bGoodUndoBehavior = TRUE;
	if (FAILED(HrTridentGetPropBool(IDM_GOOD_UNDO_BEHAVIOR, bGoodUndoBehavior)))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto error;
	}

	if (FAILED(hr = HrSetAbsoluteDropMode(m_fAbsoluteDropMode)))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto error;
	}

	if (FAILED(hr = HrSetSnapToGridX(m_ulSnapToGridX)))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto error;
	}

	if (FAILED(hr = HrSetSnapToGridY(m_ulSnapToGridY)))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto error;
	}

	if (FAILED(hr = HrSetSnapToGrid(m_fSnapToGrid)))
	{
		_ASSERTE(SUCCEEDED(hr));
		goto error;
	}

error:

	return hr;
}


//	HrExecInsertTable helper.  Extract the safearrys
//
HRESULT
CProxyFrame::HrGetTableSafeArray(IDEInsertTableParam* pTable, LPVARIANT pVarIn)
{
	HRESULT hr					= S_OK;
	UINT i						= 0;
	SAFEARRAY FAR* psa			= NULL;
	SAFEARRAYBOUND rgsabound[1] = {0};
	LONG ix[1]					= {0};
	VARIANT varElem;
	LONG  nNumRows				= 0;
	LONG  nNumCols				= 0;
	BSTR bstrTableAttrs			= NULL;
	BSTR bstrCellAttrs			= NULL;
	BSTR bstrCaption			= NULL;

	_ASSERTE(pTable);

	if (FAILED(hr = pTable->get_NumRows(&nNumRows)))
	{
		_ASSERTE(SUCCEEDED(hr));
		return hr;
	}

	if (FAILED(hr = pTable->get_NumCols(&nNumCols)))
	{
		_ASSERTE(SUCCEEDED(hr));
		return hr;
	}

	if (FAILED(hr = pTable->get_TableAttrs(&bstrTableAttrs)))
	{
		_ASSERTE(SUCCEEDED(hr));
		return hr;
	}
	_ASSERTE(bstrTableAttrs);

	if (FAILED(hr = pTable->get_CellAttrs(&bstrCellAttrs)))
	{
		_ASSERTE(SUCCEEDED(hr));
		return hr;
	}
	_ASSERTE(bstrCellAttrs);

	if (FAILED(hr = pTable->get_Caption(&bstrCaption)))
	{
		_ASSERTE(SUCCEEDED(hr));
		return hr;
	}
	_ASSERTE(bstrCaption);

	rgsabound[0].lLbound = 0;
	rgsabound[0].cElements = 5;

	psa = SafeArrayCreate(VT_VARIANT, 1, rgsabound);	
	_ASSERTE(psa);

	if(NULL == psa)
		return E_OUTOFMEMORY;

	VariantInit(pVarIn);
	V_VT(pVarIn) = VT_ARRAY;
	V_ARRAY(pVarIn) = psa;

	i=0;

	// elmement 1: number of rows
	ix[0] = i;
	VariantInit(&varElem);
	V_VT(&varElem) = VT_I4;
	V_I4(&varElem) = nNumRows; 
	hr = SafeArrayPutElement(psa, ix, &varElem);
	VariantClear(&varElem);
	++i;

	// elmement 2: number of columns
	ix[0] = i;
	VariantInit(&varElem);
	V_VT(&varElem) = VT_I4;
	V_I4(&varElem) = nNumCols;
	hr = SafeArrayPutElement(psa, ix, &varElem);
	VariantClear(&varElem);
	++i;

	// elmement 3: table tag attributes
	ix[0] = i;
	VariantInit(&varElem);
	V_VT(&varElem) = VT_BSTR;
	V_BSTR(&varElem) = bstrTableAttrs;
	hr = SafeArrayPutElement(psa, ix, &varElem);
	VariantClear(&varElem);
	++i;

	// elmement 4: cell attributes
	ix[0] = i;
	VariantInit(&varElem);
	V_VT(&varElem) = VT_BSTR;
	V_BSTR(&varElem) = bstrCellAttrs;
	hr = SafeArrayPutElement(psa, ix, &varElem);
	VariantClear(&varElem);
	++i;

	// elmement 5: table caption
	// VK bug 15857: don't include caption if it's empty.
	if ( 0 != SysStringLen ( bstrCaption ) )
	{
		ix[0] = i;
		VariantInit(&varElem);
		V_VT(&varElem) = VT_BSTR;
		V_BSTR(&varElem) = bstrCaption;
		hr = SafeArrayPutElement(psa, ix, &varElem);
		VariantClear(&varElem);
		++i;
	}

	return hr;
}


//	Determine which object is selected, and return its position
//
HRESULT
CProxyFrame::GetSelectionPos ( LPPOINT lpWhere )
{
	HRESULT	hr	= E_FAIL;
	CComPtr<IHTMLDocument2> spHtmlDoc				= NULL;
	CComPtr<IHTMLSelectionObject> spSelectionObj	= NULL;
	CComPtr<IDispatch> spRangeDisp					= NULL;
	CComPtr<IHTMLElement> spElement					= NULL;
	
	lpWhere->x	= 0;
	lpWhere->y	= 0;

	hr = HrGetDoc ( &spHtmlDoc );
	if ( SUCCEEDED ( hr ) )
	{
		hr = spHtmlDoc->get_selection ( &spSelectionObj );
		if ( SUCCEEDED ( hr ) )
		{
			hr = spSelectionObj->createRange ( &spRangeDisp );
			if (SUCCEEDED ( hr ) )
			{
				CComQIPtr<IHTMLTxtRange, &IID_IHTMLTxtRange> spTextRange ( spRangeDisp );
				if ( spTextRange )
				{
					hr = spTextRange->parentElement(&spElement);
				}
				else
				{
					CComQIPtr<IHTMLControlRange, &IID_IHTMLControlRange> spControlRange ( spRangeDisp );
					if ( spControlRange )
					{
						hr = spControlRange->commonParentElement(&spElement);
					}
				}
				if ( spElement )
				{
					CComPtr<IHTMLStyle> spStyle = NULL;
					hr = spElement->get_style ( &spStyle );
					if ( spStyle )
					{
						spStyle->get_pixelTop ( &( lpWhere->y ) );
						spStyle->get_pixelLeft ( &( lpWhere->x ) );
					}
				}
			}
		}
	}
	return hr;
}


//	If the current document is loaded from a URL, return the empty string.
//	If it's loaded from a file, strip the path part off and return just the file name.
//	Return S_FALSE for a URL or no file name.  S_OK if a file name is supplied.
//
HRESULT
CProxyFrame::GetCurDocNameWOPath ( CComBSTR& bstrDocName )
{
	bstrDocName = L"";

	if ( m_bfIsURL )
	{
		return S_FALSE;
	}
	if ( 0 == m_bstrCurDocPath.Length () )
	{
		return S_FALSE;
	}

	bstrDocName = m_bstrCurDocPath;

	// Truncate at first backslash:
	_wcsrev ( bstrDocName );
	wcstok ( bstrDocName, OLESTR( "\\" ) );
	_wcsrev ( bstrDocName );

	return S_OK;
}


//	Used by ShowContextMenu to properly offset the position of the click
// 
HRESULT
CProxyFrame::GetScrollPos ( LPPOINT lpPos )
{
	HRESULT					hr			= E_FAIL;
	CComPtr<IHTMLDocument2>	spHtmlDoc	= NULL;
	CComPtr<IHTMLElement>	spBodyElem	= NULL;

	_ASSERTE ( lpPos );
	
	hr = HrGetDoc ( &spHtmlDoc );
	
	// It's possible that the user clicked while the doc was still loading.
	// If so, just return 0, 0.
	if ( DE_E_UNEXPECTED == hr )
	{
		lpPos->x = lpPos->y = 0;
		return S_FALSE;
	}

	_ASSERTE ( spHtmlDoc );
	if ( SUCCEEDED ( hr ) )
	{
		hr = spHtmlDoc->get_body ( &spBodyElem );
		_ASSERTE ( spBodyElem );
		if ( SUCCEEDED ( hr ) )
		{
			CComQIPtr<IHTMLTextContainer, &IID_IHTMLTextContainer> spHtmlTextCont ( spBodyElem );
			if ( spHtmlTextCont )
			{
				LONG	lxPos	= 0;
				LONG	lyPos	= 0;

				hr = spHtmlTextCont->get_scrollLeft ( &lxPos );
				_ASSERTE ( SUCCEEDED ( hr ) );
				if ( SUCCEEDED ( hr ) )
				{
					hr = spHtmlTextCont->get_scrollTop ( &lyPos );
					_ASSERTE ( SUCCEEDED ( hr ) );
					if ( SUCCEEDED ( hr ) )
					{
						lpPos->x = lxPos;
						lpPos->y = lyPos;
					}
				}
			}
			else
			{
				hr = E_NOINTERFACE;
				_ASSERTE ( SUCCEEDED ( hr ) );
			}
		}
	}
	return hr;
}


HRESULT
CProxyFrame::GetContainer ( LPOLECONTAINER* ppContainer )
{
	_ASSERTE ( m_pCtl );
	_ASSERTE ( m_pCtl->m_spClientSite );
	if ( m_pCtl->m_spClientSite )
	{
		return m_pCtl->m_spClientSite->GetContainer ( ppContainer );
	}
	return E_NOTIMPL;
}


//	For the Safe for Scripting control, make sure the URL specified comes from
//	the same host as the SecurityURL, the URL of the hosting container..
//	Note that this makes the SFS control virtually useless in VB, which returns
//	the Boot Drive Root Folder as the Security URL.
//
HRESULT CProxyFrame::CheckCrossZoneSecurity ( BSTR urlToLoad )
{
	HRESULT		hr	= S_OK;

	CComPtr<IInternetSecurityManager> srpSec;
	CComBSTR	bstrSecURL;

	hr = GetSecurityURL ( bstrSecURL );
	_ASSERTE ( SUCCEEDED ( hr ) );
	if ( SUCCEEDED ( hr ) )
	{
#ifdef LATE_BIND_URLMON_WININET
		hr = (m_pfnCoInternetCreateSecurityManager)( NULL, &srpSec, 0 );
#else
		hr = CoInternetCreateSecurityManager( NULL, &srpSec, 0 );
#endif // LATE_BIND_URLMON_WININET
		if ( SUCCEEDED ( hr ) && srpSec )
		{
			BYTE*	pbSidToLoad		= NULL;
			BYTE*	pbDSidSecURL	= NULL;
			DWORD	dwSizeToLoad	= INTERNET_MAX_URL_LENGTH;
			DWORD	dwSizeSecURL	= INTERNET_MAX_URL_LENGTH;

			pbSidToLoad  = new BYTE [INTERNET_MAX_URL_LENGTH];
			pbDSidSecURL = new BYTE [INTERNET_MAX_URL_LENGTH];

			hr = srpSec->GetSecurityId ( urlToLoad, pbSidToLoad, &dwSizeToLoad, 0 );
			_ASSERTE ( SUCCEEDED ( hr ) );
			if ( SUCCEEDED ( hr ) )
			{
				hr = srpSec->GetSecurityId ( bstrSecURL, pbDSidSecURL, &dwSizeSecURL, 0 );
				_ASSERTE ( SUCCEEDED ( hr ) );
				if ( SUCCEEDED ( hr ) )
				{
					hr = DE_E_ACCESS_DENIED;

					if ( ( dwSizeToLoad == dwSizeSecURL ) &&
						( 0 == memcmp ( pbSidToLoad, pbDSidSecURL, dwSizeToLoad ) ) )
					{
						hr = S_OK;
					}
				}
			}

			delete [] pbSidToLoad;
			delete [] pbDSidSecURL;
		}
	}
	return hr;
}


HRESULT CProxyFrame::OnProgress(ULONG, ULONG, ULONG ulStatusCode, LPCWSTR)
{
	if ( BINDSTATUS_REDIRECTING == ulStatusCode )
	{
		// If we're the SFS control, cancel on Redirect.  Otherwise, ignore it.
		if ( m_pCtl->IsSafeForScripting ())
		{
			m_bfSFSRedirect = TRUE;
		}
	}
	return E_NOTIMPL;
}

