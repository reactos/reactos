// DHTMLEdit.cpp : Implementation of CDHTMLEdit and CDHTMLSafe
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#include "stdafx.h"
#include "DHTMLEd.h"
#include "DHTMLEdit.h"
#include "proxyframe.h"
#include "site.h"

/////////////////////////////////////////////////////////////////////////////
// CDHTMLSafe


CDHTMLSafe::CDHTMLSafe()
{
	m_bWindowOnly				= TRUE;				// A window is needed when we activate Trident.
	m_pFrame					= NULL;
	m_piControlSite				= NULL;
	m_fJustCreated				= TRUE;
	m_piOuterEditCtl			= (IDHTMLEdit*)-1;	// Crash if we use this without properly initializing it.
	m_bfOuterEditUnknownTested	= NULL;
}

CDHTMLSafe::~CDHTMLSafe()
{

}


HRESULT CDHTMLSafe::FinalConstruct()
{
	HRESULT hr E_FAIL;

	m_pFrame = new CProxyFrame(this);

	_ASSERTE(m_pFrame);

	if (NULL == m_pFrame)
		return E_OUTOFMEMORY;

	// not aggregating TriEdit -- don't get
	// reference to its pUnk;

	hr = m_pFrame->Init(NULL, NULL);

	_ASSERTE(SUCCEEDED(hr));

	if (FAILED(hr))
	{
		m_pFrame->Release ();
		m_pFrame = NULL;
	}

	return hr;
}


void CDHTMLSafe::FinalRelease()
{
	if ( NULL != m_piControlSite )
	{
		m_piControlSite->Release ();
		m_piControlSite = NULL;
	}

	if (NULL != m_pFrame)
	{
		if (m_pFrame->IsCreated())
		{
			_ASSERTE(FALSE == m_pFrame->IsActivated());

			m_pFrame->Close();
		}

		m_pFrame->Release ();
		m_pFrame = NULL;
	}
}


HRESULT CDHTMLSafe::OnDraw(ATL_DRAWINFO& di)
{
	HRESULT hr = S_OK;
	
	_ASSERTE(m_pFrame);

	if (NULL == m_pFrame)
		return E_UNEXPECTED;

	if (IsUserMode() == FALSE)
	{
		HBRUSH hgreyBrush = NULL;

		hgreyBrush = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
		RECT& rc = *(RECT*)di.prcBounds;
		Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);
		FillRect(di.hdcDraw, &rc, hgreyBrush);
        return S_OK;

	}
	else if (IsUserMode() && m_pFrame->IsCreated() == TRUE && m_pFrame->IsActivated() == FALSE)
	{
		_ASSERTE(m_bInPlaceActive == TRUE);

		hr = m_pFrame->LoadInitialDoc();
	}

	return hr;
}


LRESULT
CDHTMLSafe::OnSize(UINT /*nMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& lResult)
{
    m_pFrame->UpdateObjectRects();

	lResult = TRUE;
	return 0;
}


STDMETHODIMP CDHTMLSafe::TranslateAccelerator(LPMSG lpmsg)
{
	HRESULT hr = S_OK;

	hr = m_pFrame->HrTranslateAccelerator(lpmsg);
	return hr;
}	

STDMETHODIMP CDHTMLSafe::OnMnemonic(LPMSG /*pMsg*/)
{
	return S_FALSE;
}


STDMETHODIMP CDHTMLSafe::SetClientSite(IOleClientSite *pClientSite)
{
	HRESULT hr = S_OK;

	if ( NULL == pClientSite )
		{
		_ASSERTE ( m_pFrame );
		if ( NULL != m_pFrame )
		{
			_ASSERTE(m_pFrame->IsCreated());
			hr = m_pFrame->Close();
			_ASSERTE(SUCCEEDED(hr));
		}
	}
	return IOleObject_SetClientSite ( pClientSite );
}


LRESULT
CDHTMLSafe::OnDestroy(UINT /*nMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*lResult*/)
{
	HRESULT hr = S_OK;

	// This would, in turn, destroy the hosted Trident's window.
	if ( NULL != m_pFrame )
	{
		_ASSERTE(m_pFrame->IsCreated());
		_ASSERTE ( m_hWndCD );
		m_pFrame->SetParent ( NULL );
	}

	return hr;
}


LRESULT
CDHTMLSafe::OnCreate(UINT /*nMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*lResult*/)
{
	if ( NULL != m_pFrame )
	{
		_ASSERTE(m_pFrame->IsCreated());
		_ASSERTE ( m_hWndCD );
		m_pFrame->SetParent ( m_hWndCD );
	}

	return 0;
}


LRESULT
CDHTMLSafe::OnShow(UINT /*nMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*lResult*/)
{
	if ( NULL != m_pFrame )
	{
		_ASSERTE(m_pFrame->IsCreated());
		m_pFrame->Show ( wParam );
	}
	return 0;
}


// Do our best to set the focus on the ControlSite.
// m_piControlSite is obtained on demand, and released in FinalRelease.
//
void
CDHTMLSafe::FocusSite ( BOOL bfGetFocus )
{
	if  ( NULL == m_piControlSite )
	{
		_ASSERTE ( m_spClientSite );
		if ( m_spClientSite )
		{
			m_spClientSite->QueryInterface ( IID_IOleControlSite, (void**)&m_piControlSite );
		}
	}

	if ( m_piControlSite )
	{
		m_piControlSite->OnFocus ( bfGetFocus );
	}
}


LRESULT
CDHTMLSafe::OnSetFocus(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult)
{
	lResult = FALSE;
	_ASSERTE ( m_pFrame );
	if ( NULL != m_pFrame )
	{
		FocusSite ( TRUE );
		return m_pFrame->OnSetFocus ( nMsg, wParam, lParam, lResult );
	}
	return 0;
}


//	This message is posted in OnReadyStateChanged.
//	This postpones firing DocumentComplete until MSHTML is actually complete.
//
LRESULT
CDHTMLSafe::OnDocumentComplete(UINT /*nMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& lResult)
{
	_ASSERTE ( DOCUMENT_COMPETE_SIGNATURE == wParam );
	if ( DOCUMENT_COMPETE_SIGNATURE == wParam )
	{
		lResult = TRUE;
		Fire_DocumentComplete();
	}
	return 0;
}


STDMETHODIMP CDHTMLSafe::OnAmbientPropertyChange(DISPID /*dispID*/)
{
	HRESULT hr = S_OK;

	// There may be some VB ambients we may want to handle here
	// in the future for VB debugging.
	return hr;
}


//	IE5 security settings for Paste, and possibly for Copy and Cut, require that we call
//	ITHMLDocument2->execCommand for testing.
//
HRESULT CDHTMLSafe::SpecialEdit ( DHTMLEDITCMDID cmdID, OLECMDEXECOPT cmdexecopt )
{
	HRESULT					hr			= S_OK;
	CComPtr<IHTMLDocument2>	spDOM		= NULL;
	VARIANT_BOOL			vbResult	= VARIANT_FALSE;
	CComBSTR				bstrCommand;
	CComVariant				varValue;

	hr = get_DOM ( &spDOM );
	if ( SUCCEEDED ( hr ) )
	{
		switch ( cmdID )
		{
			case DECMD_CUT:
				bstrCommand = L"Cut";
				break;
			case DECMD_COPY:
				bstrCommand = L"Copy";
				break;
			case DECMD_PASTE:
				bstrCommand = L"Paste";
				break;
			default:
				return E_UNEXPECTED;
		}
		hr = spDOM->execCommand ( bstrCommand, cmdexecopt == OLECMDEXECOPT_PROMPTUSER, varValue, &vbResult );
	}
	return hr;
}


//	To be Safe for Scripting, restrict the range of cmdIDs to a known set.
//	Handle edit commands specially to utilize IE5's security settings.
//
STDMETHODIMP CDHTMLSafe::ExecCommand(DHTMLEDITCMDID cmdID, OLECMDEXECOPT cmdexecopt, LPVARIANT pInVar, LPVARIANT pOutVar)
{
	HRESULT			hr			= S_OK;
	LPVARIANT		_pVarIn		= NULL;
	LPVARIANT		_pVarOut	= NULL;

	// It is valid for pVar to be VT_EMPTY (on a DECMD_GETXXX op) but not VT_ERROR

	if (pInVar && (V_VT(pInVar) != VT_ERROR))
		_pVarIn = pInVar;

	if (pOutVar && (V_VT(pOutVar) != VT_ERROR))
		_pVarOut = pOutVar;

	if ( ( cmdexecopt < OLECMDEXECOPT_DODEFAULT ) ||
		 ( cmdexecopt >  OLECMDEXECOPT_DONTPROMPTUSER ) )
	{
		return E_INVALIDARG;
	}

	//	Special case for editing commands in Safe for Scripting version:
	if ( ( DECMD_CUT == cmdID ) || ( DECMD_COPY == cmdID ) || ( DECMD_PASTE == cmdID ) )
	{
		return SpecialEdit ( cmdID, cmdexecopt );
	}

	hr = m_pFrame->HrMapExecCommand(cmdID, cmdexecopt, _pVarIn, _pVarOut);

	return hr;
}


STDMETHODIMP CDHTMLSafe::QueryStatus(DHTMLEDITCMDID cmdID, DHTMLEDITCMDF* retval)
{
	HRESULT hr = S_OK;

	hr = m_pFrame->HrMapQueryStatus(cmdID, retval);

	return hr;
}


// Get Document Object Model
//
STDMETHODIMP CDHTMLSafe::get_DOM(IHTMLDocument2 ** pVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(pVal);

	if (NULL == pVal)
		return E_INVALIDARG;

	*pVal = NULL;
	hr = m_pFrame->HrGetDoc(pVal);

	return hr;
}


STDMETHODIMP CDHTMLSafe::get_DocumentHTML(BSTR * pVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(pVal);
	_ASSERTE(m_pFrame);

	if (!pVal)
		return E_INVALIDARG;

	hr = m_pFrame->HrGetDocumentHTML(pVal);
	return hr;
}


STDMETHODIMP CDHTMLSafe::put_DocumentHTML(BSTR newVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(m_pFrame);

	hr = m_pFrame->HrSetDocumentHTML(newVal);
	if ( SUCCEEDED ( hr ) )
	{
		SetDirty ( TRUE );
		FireOnChanged ( DISPID_DOCUMENTHTML );
	}
	return hr;
}


STDMETHODIMP CDHTMLSafe::get_ActivateApplets(VARIANT_BOOL * pVal)
{
	HRESULT hr = S_OK;
	BOOL bVal = FALSE;

	_ASSERTE(pVal);
	_ASSERTE(m_pFrame);

	if (!pVal)
		return E_INVALIDARG;

	hr = m_pFrame->HrGetPropActivateApplets(bVal);

#pragma warning(disable: 4310) // cast truncates constant value
	*pVal = (bVal) ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
	return hr;
}

STDMETHODIMP CDHTMLSafe::put_ActivateApplets(VARIANT_BOOL newVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(m_pFrame);

	hr = m_pFrame->HrSetPropActivateApplets(newVal);
	if ( SUCCEEDED ( hr ) )
	{
		SetDirty ( TRUE );
		FireOnChanged ( DISPID_ACTIVATEAPPLETS );
	}
	return hr;
}

STDMETHODIMP CDHTMLSafe::get_ActivateActiveXControls(VARIANT_BOOL * pVal)
{
	HRESULT hr = S_OK;
	BOOL bVal = FALSE;

	_ASSERTE(pVal);
	_ASSERTE(m_pFrame);

	if (!pVal)
		return E_INVALIDARG;

	hr = m_pFrame->HrGetPropActivateControls(bVal);
#pragma warning(disable: 4310) // cast truncates constant value
	*pVal = (bVal) ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
	return hr;
}

STDMETHODIMP CDHTMLSafe::put_ActivateActiveXControls(VARIANT_BOOL newVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(m_pFrame);

	hr = m_pFrame->HrSetPropActivateControls(newVal);
	if ( SUCCEEDED ( hr ) )
	{
		SetDirty ( TRUE );
		FireOnChanged ( DISPID_ACTIVATEACTIVEXCONTROLS );
	}
	return hr;
}

STDMETHODIMP CDHTMLSafe::get_ActivateDTCs(VARIANT_BOOL * pVal)
{
	HRESULT hr = S_OK;
	BOOL bVal = FALSE;

	_ASSERTE(pVal);
	_ASSERTE(m_pFrame);

	if (!pVal)
		return E_INVALIDARG;

	hr = m_pFrame->HrGetPropActivateDTCs(bVal);
#pragma warning(disable: 4310) // cast truncates constant value
	*pVal = (bVal) ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
	return hr;
}

STDMETHODIMP CDHTMLSafe::put_ActivateDTCs(VARIANT_BOOL newVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(m_pFrame);

	hr = m_pFrame->HrSetPropActivateDTCs(newVal);
	if ( SUCCEEDED ( hr ) )
	{
		SetDirty ( TRUE );
		FireOnChanged ( DISPID_ACTIVATEDTCS );
	}
	return hr;
}


STDMETHODIMP CDHTMLSafe::get_ShowDetails(VARIANT_BOOL * pVal)
{
	HRESULT hr = S_OK;
	BOOL bVal = FALSE;

	_ASSERTE(pVal);
	_ASSERTE(m_pFrame);

	if (!pVal)
		return E_INVALIDARG;

	 hr = m_pFrame->HrGetPropShowAllTags(bVal);
#pragma warning(disable: 4310) // cast truncates constant value
	*pVal = (bVal) ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
	 return hr;
}

STDMETHODIMP CDHTMLSafe::put_ShowDetails(VARIANT_BOOL newVal)
{
	_ASSERTE(m_pFrame);
	SetDirty ( TRUE );
	FireOnChanged ( DISPID_SHOWDETAILS );
	
	return m_pFrame->HrSetPropShowAllTags(newVal);
}

STDMETHODIMP CDHTMLSafe::get_ShowBorders(VARIANT_BOOL * pVal)
{
	HRESULT hr = S_OK;
	BOOL bVal = FALSE;

	_ASSERTE(pVal);
	_ASSERTE(m_pFrame);

	if (!pVal)
		return E_INVALIDARG;

	hr = m_pFrame->HrGetPropShowBorders(bVal);
#pragma warning(disable: 4310) // cast truncates constant value
	*pVal = (bVal) ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value

	return hr;
}

STDMETHODIMP CDHTMLSafe::put_ShowBorders(VARIANT_BOOL newVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(m_pFrame);

	hr = m_pFrame->HrSetPropShowBorders(newVal);
	if ( SUCCEEDED ( hr ) )
	{
		SetDirty ( TRUE );
		FireOnChanged ( DISPID_SHOWBORDERS );
	}
	return hr;
}



STDMETHODIMP CDHTMLSafe::get_Appearance(DHTMLEDITAPPEARANCE * pVal)
{
	HRESULT hr = S_OK;
	BOOL bVal = FALSE;

	_ASSERTE(pVal);
	_ASSERTE(m_pFrame);

	if (!pVal)
		return E_INVALIDARG;

	hr = m_pFrame->HrGetDisplay3D(bVal);
	*pVal = (bVal) ? DEAPPEARANCE_3D : DEAPPEARANCE_FLAT;
	return hr;
}

STDMETHODIMP CDHTMLSafe::put_Appearance(DHTMLEDITAPPEARANCE newVal)
{
	HRESULT hr = S_OK;
	BOOL bVal = (newVal == DEAPPEARANCE_3D) ? TRUE : FALSE;

	_ASSERTE(m_pFrame);

	hr = m_pFrame->HrSetDisplay3D(bVal);
	if ( SUCCEEDED ( hr ) )
	{
		SetDirty ( TRUE );
		FireOnChanged ( DISPID_DHTMLEDITAPPEARANCE );
	}
	return hr;
}

STDMETHODIMP CDHTMLSafe::get_Scrollbars(VARIANT_BOOL * pVal)
{
	HRESULT hr = S_OK;
	BOOL bVal = FALSE;

	_ASSERTE(pVal);
	_ASSERTE(m_pFrame);

	if (!pVal)
		return E_INVALIDARG;

	hr = m_pFrame->HrGetScrollbars(bVal);
#pragma warning(disable: 4310) // cast truncates constant value
	*pVal = (bVal) ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
	return hr;
}

STDMETHODIMP CDHTMLSafe::put_Scrollbars(VARIANT_BOOL newVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(m_pFrame);

	hr = m_pFrame->HrSetScrollbars(newVal);
	if ( SUCCEEDED ( hr ) )
	{
		SetDirty ( TRUE );
		FireOnChanged ( DISPID_DHTMLEDITSCROLLBARS );
	}
	return hr;
}

STDMETHODIMP CDHTMLSafe::get_ScrollbarAppearance(DHTMLEDITAPPEARANCE * pVal)
{
	HRESULT hr = S_OK;
	BOOL bVal = FALSE;

	_ASSERTE(pVal);
	_ASSERTE(m_pFrame);

	if (!pVal)
		return E_INVALIDARG;

	hr = m_pFrame->HrGetDisplayFlatScrollbars(bVal);
	*pVal = (bVal) ? DEAPPEARANCE_FLAT : DEAPPEARANCE_3D;
	return hr;
}

STDMETHODIMP CDHTMLSafe::put_ScrollbarAppearance(DHTMLEDITAPPEARANCE newVal)
{
	HRESULT hr = S_OK;
	BOOL bVal = (newVal == DEAPPEARANCE_3D) ? FALSE : TRUE;

	_ASSERTE(m_pFrame);

	hr = m_pFrame->HrSetDisplayFlatScrollbars(bVal);
	if ( SUCCEEDED ( hr ) )
	{
		SetDirty ( TRUE );
		FireOnChanged ( DISPID_SCROLLBARAPPEARANCE );
	}
	return hr;
}


STDMETHODIMP CDHTMLSafe::get_SourceCodePreservation(VARIANT_BOOL * pVal)
{
	HRESULT hr = S_OK;
	BOOL bVal = FALSE;

	_ASSERTE(pVal);
	_ASSERTE(m_pFrame);

	if (!pVal)
		return E_INVALIDARG;

	hr = m_pFrame->HrGetPreserveSource(bVal);
#pragma warning(disable: 4310) // cast truncates constant value
	*pVal = (bVal) ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
	return hr;
}

STDMETHODIMP CDHTMLSafe::put_SourceCodePreservation(VARIANT_BOOL newVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(m_pFrame);

	hr = m_pFrame->HrSetPreserveSource(newVal);
	if ( SUCCEEDED ( hr ) )
	{
		SetDirty ( TRUE );
		FireOnChanged ( DISPID_SOURCECODEPRESERVATION );
	}
	return hr;
}

///////////////////////////////////////

STDMETHODIMP CDHTMLSafe::get_AbsoluteDropMode(VARIANT_BOOL* pVal)
{
	HRESULT hr = S_OK;
	BOOL bVal = FALSE;

	_ASSERTE(pVal);
	_ASSERTE(m_pFrame);

	if (!pVal)
		return E_INVALIDARG;

	hr = m_pFrame->HrGetAbsoluteDropMode(bVal);
#pragma warning(disable: 4310) // cast truncates constant value
	*pVal = (bVal) ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
	return hr;
}


STDMETHODIMP CDHTMLSafe::put_AbsoluteDropMode(VARIANT_BOOL newVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(m_pFrame);

	hr = m_pFrame->HrSetAbsoluteDropMode(newVal);
	if ( SUCCEEDED ( hr ) )
	{
		SetDirty ( TRUE );
		FireOnChanged ( DISPID_ABSOLUTEDROPMODE );
	}
	return hr;
}

STDMETHODIMP CDHTMLSafe::get_SnapToGrid(VARIANT_BOOL* pVal)
{
	HRESULT hr = S_OK;
	BOOL bVal = FALSE;

	_ASSERTE(pVal);
	_ASSERTE(m_pFrame);

	if (!pVal)
		return E_INVALIDARG;

	hr = m_pFrame->HrGetSnapToGrid(bVal);
#pragma warning(disable: 4310) // cast truncates constant value
	*pVal = (bVal) ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
	return hr;
}

STDMETHODIMP CDHTMLSafe::put_SnapToGrid(VARIANT_BOOL newVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(m_pFrame);

	hr = m_pFrame->HrSetSnapToGrid(newVal);
	if ( SUCCEEDED ( hr ) )
	{
		SetDirty ( TRUE );
		FireOnChanged ( DISPID_SNAPTOGRID );
	}
	return hr;
}

STDMETHODIMP CDHTMLSafe::get_SnapToGridX(LONG* pVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(pVal);
	_ASSERTE(m_pFrame);

	if (!pVal)
		return E_INVALIDARG;

	hr = m_pFrame->HrGetSnapToGridX(*pVal);
	return hr;
}

STDMETHODIMP CDHTMLSafe::put_SnapToGridX(LONG newVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(m_pFrame);

	hr = m_pFrame->HrSetSnapToGridX(newVal);
	if ( SUCCEEDED ( hr ) )
	{
		SetDirty ( TRUE );
		FireOnChanged ( DISPID_SNAPTOGRIDX );
	}
	return hr;
}

STDMETHODIMP CDHTMLSafe::get_SnapToGridY(LONG* pVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(pVal);
	_ASSERTE(m_pFrame);

	if (!pVal)
		return E_INVALIDARG;

	hr = m_pFrame->HrGetSnapToGridY(*pVal);
	return hr;
}


STDMETHODIMP CDHTMLSafe::put_SnapToGridY(LONG newVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(m_pFrame);

	hr = m_pFrame->HrSetSnapToGridY(newVal);
	if ( SUCCEEDED ( hr ) )
	{
		SetDirty ( TRUE );
		FireOnChanged ( DISPID_SNAPTOGRIDY );
	}
	return hr;
}


STDMETHODIMP CDHTMLSafe::get_CurrentDocumentPath(BSTR * pVal)
{
	HRESULT hr = S_OK;

	_ASSERTE(pVal);
	_ASSERTE(m_pFrame);

	if (!pVal)
		return E_INVALIDARG;

	hr = m_pFrame->HrGetCurrentDocumentPath(pVal);
	return hr;
}


STDMETHODIMP CDHTMLSafe::get_IsDirty(VARIANT_BOOL * pVal)
{
	HRESULT hr = S_OK;
	BOOL bVal = FALSE;

	_ASSERTE(pVal);
	_ASSERTE(m_pFrame);

	if (NULL == pVal)
		return E_INVALIDARG;

	hr = m_pFrame->HrGetIsDirty(bVal);

#pragma warning(disable: 4310) // cast truncates constant value
	*pVal = (TRUE == bVal) ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value

	return hr;
}


STDMETHODIMP CDHTMLSafe::get_BaseURL(/* [retval][out] */ BSTR  *baseURL)
{
	HRESULT hr = S_OK;

	_ASSERTE(m_pFrame);
	_ASSERTE ( baseURL );

	if ( NULL == baseURL )
	{
		return E_INVALIDARG;
	}

	CComBSTR bstr;
	hr = m_pFrame->GetBaseURL ( bstr );

	if ( SUCCEEDED ( hr ) )
	{
		SysReAllocString ( baseURL, bstr );
	}
	return hr;
}


STDMETHODIMP CDHTMLSafe::put_BaseURL(/* [in] */ BSTR baseURL)
{
	HRESULT hr = S_OK;

	_ASSERTE(m_pFrame);
	_ASSERTE ( baseURL );
	if ( NULL == baseURL )
	{
		return E_INVALIDARG;
	}

	CComBSTR bstr = baseURL;
	hr = m_pFrame->SetBaseURL ( bstr );

	if ( SUCCEEDED ( hr ) )
	{
		SetDirty ( TRUE );
		FireOnChanged ( DISPID_BASEURL );
	}

	return hr;
}


STDMETHODIMP CDHTMLSafe::get_DocumentTitle(/* [retval][out] */ BSTR  *docTitle)
{
	HRESULT hr = S_OK;

	_ASSERTE ( docTitle );
	_ASSERTE ( m_pFrame );

	if ( NULL == docTitle )
	{
		return E_INVALIDARG;
	}

	CComBSTR bstr;
	hr = m_pFrame->GetDocumentTitle ( bstr );
	if ( SUCCEEDED ( hr ) )
	{
		SysReAllocString ( docTitle, bstr );
	}

	return hr;
}


STDMETHODIMP CDHTMLSafe::get_UseDivOnCarriageReturn ( VARIANT_BOOL  *pVal )
{
	_ASSERTE ( pVal );
	if ( NULL == pVal )
	{
		return E_INVALIDARG;
	}

	return m_pFrame->GetDivOnCr ( pVal );
}


STDMETHODIMP CDHTMLSafe::get_Busy ( VARIANT_BOOL  *pVal )
{
	_ASSERTE ( pVal );
	if ( NULL == pVal )
	{
		return E_INVALIDARG;
	}

	return m_pFrame->GetBusy ( pVal );
}


STDMETHODIMP CDHTMLSafe::put_UseDivOnCarriageReturn ( VARIANT_BOOL newVal )
{
	HRESULT hr = S_OK;

	hr = m_pFrame->SetDivOnCr( newVal );
	if ( SUCCEEDED ( hr ) )
	{
		SetDirty ( TRUE );
		FireOnChanged ( DISPID_USEDIVONCR );
	}

	return hr;
}


STDMETHODIMP CDHTMLSafe::SetContextMenu(/*[in]*/LPVARIANT menuStrings, /*[in]*/ LPVARIANT menuStates)
{
	HRESULT hr = S_OK;

	hr = m_pFrame->SetContextMenu(menuStrings, menuStates);
	return hr;
}


STDMETHODIMP CDHTMLSafe::NewDocument ()
{
	HRESULT hr = E_FAIL;

	_ASSERTE ( m_pFrame );
	if ( NULL == m_pFrame )
	{
		return E_UNEXPECTED;
	}

	hr = m_pFrame->LoadDocument( NULL );

	return hr;
}


STDMETHODIMP CDHTMLSafe::Refresh ()
{
	HRESULT hr = E_FAIL;

	_ASSERTE ( m_pFrame );
	if ( NULL == m_pFrame )
	{
		return E_UNEXPECTED;
	}

	hr = m_pFrame->RefreshDoc ();

	return hr;
}


//	In the safe for scripting version, only the http: protocol is permitted.
//
STDMETHODIMP CDHTMLSafe::LoadURL ( BSTR url )
{
	HRESULT		hr = S_OK;
	CComBSTR	rbstrSafeProtocols[] = { L"http://", L"https://", L"ftp://" };

	_ASSERTE(url);

	_ASSERTE ( m_pFrame );
	if (  NULL == m_pFrame )
	{
		return E_UNEXPECTED;
	}

	if ( ( NULL == url ) || ( 0 == SysStringLen ( url ) ) )
		return E_INVALIDARG;

	// Check for the protocol:
	CComBSTR bstrURL = url;
	_wcslwr ( bstrURL.m_str );

	BOOL bfSafe = FALSE;
	for ( int iProtocol = 0;
		iProtocol < ( sizeof ( rbstrSafeProtocols ) / sizeof ( CComBSTR ) );
		iProtocol++ )
	{
		if ( 0 == wcsncmp ( bstrURL.m_str, rbstrSafeProtocols[iProtocol],
			rbstrSafeProtocols[iProtocol].Length () ) )
		{
			bfSafe = TRUE;
			break;
		}
	}

	hr = m_pFrame->CheckCrossZoneSecurity ( url );
	if ( SUCCEEDED ( hr ) )
	{
		hr = DE_E_UNKNOWN_PROTOCOL;
		if ( bfSafe )
		{
			hr = m_pFrame->LoadDocument( url, TRUE );
		}
	}

	return hr;
}


STDMETHODIMP CDHTMLSafe::FilterSourceCode(BSTR sourceCodeIn, BSTR* sourceCodeOut)
{
	HRESULT	hr;

	_ASSERTE ( sourceCodeIn );
	_ASSERTE ( sourceCodeOut );

	if ( ( NULL == sourceCodeIn ) || ( NULL == sourceCodeOut ) )
	{
		return E_INVALIDARG;
	}

	*sourceCodeOut = NULL;

	hr = m_pFrame->FilterSourceCode ( sourceCodeIn, sourceCodeOut );
	return hr;
}


//	Override handler for IOleInPlaceObject->UIDeactivate to fire the blur event.
//
HRESULT CDHTMLSafe::IOleInPlaceObject_UIDeactivate ( void )
{
	Fire_onblur();
	return CComControlBase::IOleInPlaceObject_UIDeactivate ();
}

// Override IOleObjectImpl methods
// We must set the object as dirty when resized
//
HRESULT CDHTMLSafe::IOleObject_SetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
	if ((m_sizeExtent.cx != psizel->cx || m_sizeExtent.cy != psizel->cy) && !m_fJustCreated)
		SetDirty(TRUE);
	m_fJustCreated = FALSE;
	return CComControlBase::IOleObject_SetExtent(dwDrawAspect, psizel);
}


HRESULT CDHTMLSafe::IPersistStreamInit_Save(LPSTREAM pStm, BOOL fClearDirty, ATL_PROPMAP_ENTRY*)
{
	return CComControlBase::IPersistStreamInit_Save ( pStm, fClearDirty, ProperPropMap() );
}


HRESULT CDHTMLSafe::IPersistStreamInit_Load(LPSTREAM pStm, ATL_PROPMAP_ENTRY*)
{
	return CComControlBase::IPersistStreamInit_Load ( pStm, ProperPropMap() );
}

HRESULT CDHTMLSafe::IPersistPropertyBag_Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties, ATL_PROPMAP_ENTRY* )
{
	return CComControlBase::IPersistPropertyBag_Save(pPropBag, fClearDirty, fSaveAllProperties, ProperPropMap());
}


HRESULT CDHTMLSafe::IPersistPropertyBag_Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog, ATL_PROPMAP_ENTRY*)
{
	return CComControlBase::IPersistPropertyBag_Load(pPropBag, pErrorLog, ProperPropMap());
}


//	We cannot QI for the OuterEditControl in the FinalConstruct, or we crash whenever
//	we're aggregated.  So, we QI on demand.
//	Call this routine to get the outer control's unknown, never use m_piOuterEditCtl
//	directly.
//	NOTE:
//	This routine DOES NOT addref the interface returned!  Do not release it!
//
IDHTMLEdit * CDHTMLSafe::GetOuterEditControl ()
{
	if ( ! m_bfOuterEditUnknownTested )
	{
		m_bfOuterEditUnknownTested = TRUE;

		// Keep an un-addreffed pointer to the aggregating DHTMLEdit control, if it exists.
		if ( SUCCEEDED ( GetControllingUnknown()->QueryInterface ( IID_IDHTMLEdit, (void**)&m_piOuterEditCtl ) ) )
		{
			_ASSERTE ( m_piOuterEditCtl );
			m_piOuterEditCtl->Release ();
		}
	}
	_ASSERTE ( (IDHTMLEdit*)-1 != m_piOuterEditCtl );
	return m_piOuterEditCtl;
}


//	There are two property maps to choose from.
//	Return the one for the DHTMLEdit control if it's aggregating us,
//	else return our own.
//
ATL_PROPMAP_ENTRY* CDHTMLSafe::ProperPropMap ()
{
	IDHTMLEdit *piOuterEditControl = GetOuterEditControl ();

	if ( NULL == piOuterEditControl )
	{
		return CDHTMLSafe::GetPropertyMap();
	}
	else
	{
		return CDHTMLEdit::GetPropertyMap();
	}
}


//	Return the appropriate CLSID, depending on whether we're the safe or unsafe control.
//
HRESULT CDHTMLSafe::GetClassID( CLSID *pClassID )
{
	IDHTMLEdit *piOuterEditControl = GetOuterEditControl ();

	if ( NULL == piOuterEditControl )
	{
		*pClassID = CLSID_DHTMLSafe;
	}
	else
	{
		*pClassID = CLSID_DHTMLEdit;
	}
	return S_OK;
}


//	The above redirecting of the PropertyMap doesn't work unless we override this method,
//	We keep an un-addref'd pointer to the aggregating DHTMLEdit control if available.
//	Addreffing it would cause a circular reference.
//
HRESULT CDHTMLSafe::ControlQueryInterface(const IID& iid, void** ppv)
{
	HRESULT	hr = S_OK;
	IDHTMLEdit *piOuterEditControl = GetOuterEditControl ();

	if ( NULL == piOuterEditControl )
	{
		hr = GetUnknown()->QueryInterface ( iid, ppv );
	}
	else
	{
		hr = piOuterEditControl->QueryInterface ( iid, ppv );
	}
	return hr;
}




////////////////////////////////////////////////////
//
//	Event sink
//

class ATL_NO_VTABLE CEventXferSink :
	public CComObjectRootEx<CComSingleThreadModel>,
	public _DHTMLSafeEvents
{
public:
BEGIN_COM_MAP(CEventXferSink)
	COM_INTERFACE_ENTRY_IID(DIID__DHTMLSafeEvents, _DHTMLSafeEvents)
END_COM_MAP()

	CEventXferSink ()
	{
		m_pCtl = NULL;
	}

	void SetOwner ( CDHTMLEdit* pCtl )
	{
		_ASSERTE ( pCtl );
		_ASSERTE ( NULL == m_pCtl );
		if ( NULL == m_pCtl )
		{
			m_pCtl = pCtl;
		}
	}

	STDMETHOD(GetTypeInfoCount) ( UINT * )
	{
		_ASSERTE ( FALSE );
		return E_NOTIMPL;
	}

	STDMETHOD(GetTypeInfo) ( UINT, LCID, ITypeInfo ** )
	{
		_ASSERTE ( FALSE );
		return E_NOTIMPL;
	}

	STDMETHOD(GetIDsOfNames) ( REFIID, OLECHAR **, UINT, LCID, DISPID * )
	{
		_ASSERTE ( FALSE );
		return E_NOTIMPL;
	}

	STDMETHOD(Invoke) ( DISPID dispid, REFIID, LCID, USHORT, DISPPARAMS *pDispParams, VARIANT* /*pVarResult*/, EXCEPINFO *, UINT * )
	{
		HRESULT	hr = E_UNEXPECTED;
		_ASSERTE ( m_pCtl );
		if ( NULL != m_pCtl )
		{
			switch ( dispid )
			{
				case DISPID_DOCUMENTCOMPLETE:
					m_pCtl->Fire_DocumentComplete();
					break;

				case DISPID_DISPLAYCHANGED:
					m_pCtl->Fire_DisplayChanged ();
					break;

				case DISPID_SHOWCONTEXTMENU:
				{
					CComVariant		varParam;
					long			xPos = 0;
					long			yPos = 0;
					unsigned int	uiErr;

					// There should be exactly two parameters.
					_ASSERTE ( 2 == pDispParams->cArgs );
					if (2 == pDispParams->cArgs )
					{
						hr = DispGetParam( pDispParams, 1, VT_I4, &varParam, &uiErr );
						_ASSERTE ( SUCCEEDED ( hr ) );
						if ( SUCCEEDED ( hr ) )
						{
							yPos = varParam.lVal;
							hr = DispGetParam( pDispParams, 0, VT_I4, &varParam, &uiErr );
							_ASSERTE ( SUCCEEDED ( hr ) );
							if ( SUCCEEDED ( hr ) )
							{
								xPos = varParam.lVal;
								m_pCtl->Fire_ShowContextMenu ( xPos, yPos );
							}
						}
					}
					break;
				}

				case DISPID_CONTEXTMENUACTION:
				{
					CComVariant	varMenuIndex;
					unsigned int uiErr;

					// There should be exactly one parameter.
					_ASSERTE ( 1 == pDispParams->cArgs );
					if (1 == pDispParams->cArgs )
					{
						hr = DispGetParam( pDispParams, 0, VT_I4, &varMenuIndex, &uiErr );
						_ASSERTE ( SUCCEEDED ( hr ) );
						if ( SUCCEEDED ( hr ) )
						{
							long lMenuIndex = varMenuIndex.lVal;
							m_pCtl->Fire_ContextMenuAction ( lMenuIndex );
						}
					}
					break;
				}

				case DISPID_ONMOUSEDOWN:
					m_pCtl->Fire_onmousedown ();
					break;

				case DISPID_ONMOUSEMOVE:
					m_pCtl->Fire_onmousemove ();
					break;

				case DISPID_ONMOUSEUP:
					m_pCtl->Fire_onmouseup ();
					break;

				case DISPID_ONMOUSEOUT:
					m_pCtl->Fire_onmouseout ();
					break;

				case DISPID_ONMOUSEOVER:
					m_pCtl->Fire_onmouseover ();
					break;

				case DISPID_ONCLICK:
					m_pCtl->Fire_onclick ();
					break;

				case DISPID_ONDBLCLICK:
					m_pCtl->Fire_ondblclick ();
					break;

				case DISPID_ONKEYDOWN:
					m_pCtl->Fire_onkeydown ();
					break;

				case DISPID_ONKEYPRESS:
					{
						m_pCtl->Fire_onkeypress ();
#if 0
						VARIANT_BOOL	vbCancel;
						vbCancel = m_pCtl->Fire_onkeypress ();
						if ( NULL != pVarResult )
						{
							VariantClear ( pVarResult );
							pVarResult->vt = VT_BOOL;
							pVarResult->boolVal = vbCancel;
						}
#endif
					}
					break;

				case DISPID_ONKEYUP:
					m_pCtl->Fire_onkeyup ();
					break;

				case DISPID_ONBLUR:
					m_pCtl->Fire_onblur ();
					break;

				case DISPID_ONREADYSTATECHANGE:
					m_pCtl->Fire_onreadystatechange ();
					break;

				default:
					break;
			}
		}
		return S_OK;
	}

private:
	CDHTMLEdit*		m_pCtl;
};


////////////////////////////////////////////////////
//
//	CDHTMLEdit implementation
//

CDHTMLEdit::CDHTMLEdit()
{
	m_punkInnerCtl		= NULL;		// Aggregated control's IUnknown
	m_pInnerCtl			= NULL;		// Aggregated control's custome interface
	m_pInnerIOleObj		= NULL;		// Aggregated control's IOleObject
	m_pXferSink			= NULL;		// Event sink for aggregated control
	m_piInnerCtlConPt	= NULL;		// Connection point to aggregated control
	m_pInterconnect		= NULL;		// Interface on inner control for communication
	m_dwXferCookie		= 0;		// Cookie for aggregated control's connection point.
}

CDHTMLEdit::~CDHTMLEdit()
{
}


HRESULT CDHTMLEdit::FinalConstruct()
{
	// Aggregate DHTMLSafe control:
	HRESULT		hr			= E_FAIL;
	IUnknown*	punkContUnk	= NULL;

	punkContUnk = GetControllingUnknown ();
	_ASSERTE ( punkContUnk );

	hr = CoCreateInstance ( CLSID_DHTMLSafe, punkContUnk, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&m_punkInnerCtl );

	if ( SUCCEEDED ( hr ) )
	{
		_ASSERTE ( m_punkInnerCtl );

		hr = m_punkInnerCtl->QueryInterface ( IID_IOleObject, (void**)&m_pInnerIOleObj);
		_ASSERTE ( SUCCEEDED ( hr ) );
		_ASSERTE ( m_pInnerIOleObj );
		punkContUnk->Release ();

		hr = m_punkInnerCtl->QueryInterface ( IID_IDHTMLSafe, (void**)&m_pInnerCtl );	// This addrefs my unknown
		_ASSERTE ( SUCCEEDED ( hr ) );
		_ASSERTE ( m_pInnerCtl );
		punkContUnk->Release ();

		hr = m_punkInnerCtl->QueryInterface ( IID_IInterconnector, (void**)&m_pInterconnect );	// This addrefs my unknown
		_ASSERTE ( SUCCEEDED ( hr ) );
		_ASSERTE ( m_pInterconnect );
		punkContUnk->Release ();

		// Sink events from the aggregated control:
		m_pXferSink = new CComObject<CEventXferSink>;
		_ASSERTE ( m_pXferSink );
		m_pXferSink->AddRef ();
		m_pXferSink->SetOwner ( this );
		
		// Hook the sink up to the aggregated control:
		CComQIPtr<IConnectionPointContainer, &IID_IConnectionPointContainer>picpc ( m_punkInnerCtl );
		if ( picpc )
		{
			punkContUnk->Release ();
			hr = picpc->FindConnectionPoint ( DIID__DHTMLSafeEvents, &m_piInnerCtlConPt );
			if ( SUCCEEDED ( hr ) )
			{
				hr = m_piInnerCtlConPt->Advise ( static_cast<IDispatch *>(m_pXferSink), &m_dwXferCookie);
				_ASSERTE ( SUCCEEDED ( hr ) );
			}
		}
	}

	_ASSERTE ( SUCCEEDED ( hr ) );
	return hr;
}

void CDHTMLEdit::FinalRelease()
{
	IUnknown*	punkContUnk	= NULL;

	punkContUnk = GetControllingUnknown ();
	_ASSERTE ( punkContUnk );

	// Unadvise the event sink:
	_ASSERTE ( m_pXferSink );
	_ASSERTE ( m_piInnerCtlConPt );
	if ( NULL != m_piInnerCtlConPt )
	{
		punkContUnk->AddRef ();
		m_piInnerCtlConPt->Unadvise ( m_dwXferCookie );
		m_piInnerCtlConPt->Release ();
		m_piInnerCtlConPt = NULL;
	}
	if ( NULL != m_pXferSink )
	{
		m_pXferSink->Release ();
		m_pXferSink = NULL;
	}

	if ( m_pInnerCtl )
	{
		// Releasing the cached interface will release my unknown, which has already been ballanced.
		punkContUnk->AddRef ();
		m_pInnerCtl->Release ();
	}
	if ( m_pInnerIOleObj )
	{
		punkContUnk->AddRef ();
		m_pInnerIOleObj->Release ();
	}
	if ( m_pInterconnect )
	{
		punkContUnk->AddRef ();
		m_pInterconnect->Release ();
	}
	if ( m_punkInnerCtl )
	{
		punkContUnk->AddRef ();
		m_punkInnerCtl->Release ();
	}
}



HRESULT
CDHTMLEdit::PromptOpenFile(LPTSTR pPath, ULONG ulPathLen)
{
    HRESULT         hr = S_OK;
    OPENFILENAME    ofn = {0};
    BOOL            bResult = FALSE;
	HWND			hWndCD	= NULL;
    
	_ASSERTE(pPath);

	if (NULL == pPath)
		return E_INVALIDARG;

	hr = m_pInterconnect->GetCtlWnd ( (SIZE_T*)&hWndCD );
	_ASSERTE ( SUCCEEDED ( hr ) );
	if ( FAILED ( hr ) )
	{
		return ( SUCCEEDED ( hr ) ) ? E_UNEXPECTED : hr;
	}

    memset(&ofn, 0, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
    ofn.hwndOwner = hWndCD;
	ofn.lpstrTitle = NULL;
    ofn.lpstrFilter = TEXT("HTML Documents (*.htm, *.html)\0*.htm;*.html\0");
    ofn.lpstrFile = pPath;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrDefExt = TEXT("htm");
    ofn.nMaxFile = ulPathLen;
    ofn.Flags = OFN_EXPLORER |
				OFN_FILEMUSTEXIST |
				OFN_PATHMUSTEXIST |
				OFN_OVERWRITEPROMPT |
				OFN_HIDEREADONLY;

    bResult = GetOpenFileName(&ofn);

    if (!bResult)
        return S_FALSE;

	return S_OK;
}


HRESULT
CDHTMLEdit::PromptSaveAsFile(LPTSTR pPath, ULONG ulPathLen)
{
    HRESULT         hr = S_OK;
    OPENFILENAME    ofn = {0};
    BOOL            bResult = FALSE;
	HWND			hWndCD	= NULL;
    
	_ASSERTE(pPath);

	if (NULL == pPath)
		return E_INVALIDARG;

	hr = m_pInterconnect->GetCtlWnd ( (SIZE_T*)&hWndCD );
	_ASSERTE ( SUCCEEDED ( hr ) );
	_ASSERTE ( hWndCD );
	if ( FAILED ( hr ) || ( NULL == hWndCD ) )
	{
		return ( SUCCEEDED ( hr ) ) ? E_UNEXPECTED : hr;
	}

    memset(&ofn, 0, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
    ofn.hwndOwner = hWndCD;
	ofn.lpstrTitle = NULL;
    ofn.lpstrFilter = TEXT("HTML Documents (*.htm, *.html)\0*.htm;*.html\0");
    ofn.lpstrFile = pPath;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrDefExt = TEXT("htm");
    ofn.nMaxFile = ulPathLen;
	ofn.Flags           =   OFN_OVERWRITEPROMPT |
							OFN_CREATEPROMPT    |
							OFN_HIDEREADONLY    |
							OFN_EXPLORER;

    bResult = GetSaveFileName(&ofn);

    if (!bResult)
        return S_FALSE;

	return S_OK;
}



STDMETHODIMP CDHTMLEdit::LoadDocument(LPVARIANT path, LPVARIANT promptUser)
{
	USES_CONVERSION;

	HRESULT hr = S_OK;
	BOOL bPromptUser = NULL;
	TCHAR promptPath[MAX_PATH] = {0};
	CComBSTR bstrPath;
	BSTR _path = NULL;

	_ASSERTE(path);

	CProxyFrame* pFrame = NULL;
	hr = m_pInterconnect->GetInterconnector ( (SIZE_T*)&pFrame );
	_ASSERTE ( SUCCEEDED ( hr ) );
	_ASSERTE ( pFrame );
	if ( FAILED ( hr ) || ( NULL == pFrame ) )
	{
		return ( SUCCEEDED ( hr ) ) ? E_UNEXPECTED : hr;
	}

	if (NULL == path || !(V_VT(path) ==  VT_BSTR || V_VT(path) == (VT_BSTR|VT_BYREF)))
		return E_INVALIDARG;

	// Note that it is valid for path to be NULL,
	// In automation an empty string (BSTR) is a NULL pointer
	// Passing in an emtpy string here allows for initializing TriEdit with
	// an empty document (IPersistStreamInit->InitNew)

	if (promptUser && (V_VT(promptUser) != VT_EMPTY && V_VT(promptUser) != VT_ERROR))
	{
		// note that if promptUser is not type VT_BOOL or VT_BOOL|VT_BYREF
		// then user is not prompted

#pragma warning(disable: 4310) // cast truncates constant value
		if (VT_BOOL == V_VT(promptUser))
			bPromptUser = (VARIANT_TRUE == V_BOOL(promptUser)) ? TRUE : FALSE;
		else if ((VT_BOOL|VT_BYREF) == V_VT(promptUser))
		{
			_ASSERTE(V_BOOLREF(promptUser));

			if (V_BOOLREF(promptUser))
				bPromptUser = (BOOL) (*(V_BOOLREF(promptUser)) == VARIANT_TRUE) ? TRUE : FALSE;
		}
#pragma warning(default: 4310) // cast truncates constant value
	}

	// prompt user overrides any doc name that is specified
	// Change VK:
	// ...but the provided doc name is used as the default.
	if (bPromptUser)
	{
		if ( NULL != path->bstrVal )
		{
			_tcsncpy ( promptPath, OLE2T(path->bstrVal), MAX_PATH );
		}
		hr = PromptOpenFile(promptPath, MAX_PATH);

		if (S_FALSE == hr)
			return S_OK;

		bstrPath = promptPath;
		_path = bstrPath;
	}
	else
	{	
		if ((VT_BSTR|VT_BYREF) == V_VT(path) && V_BSTRREF(path))
			_path = *(V_BSTRREF(path));
		else if (VT_BSTR == V_VT(path) && V_BSTR(path))
			_path = V_BSTR(path);
	}

	if ( 0 == SysStringLen ( _path ) )
	{
		return DE_E_INVALIDARG;
	}

	hr = pFrame->LoadDocument(_path);

	return hr;
}


STDMETHODIMP CDHTMLEdit::SaveDocument(LPVARIANT path, LPVARIANT promptUser)
{
	USES_CONVERSION;

	HRESULT hr= S_OK;
	TCHAR promptPath[MAX_PATH] = {0};
	CComBSTR bstrPath;
	BOOL bPromptUser = FALSE;
	BSTR _path = NULL;

	_ASSERTE(path);

	CProxyFrame* pFrame = NULL;
	hr = m_pInterconnect->GetInterconnector ( (SIZE_T*)&pFrame );
	_ASSERTE ( SUCCEEDED ( hr ) );
	_ASSERTE ( pFrame );
	if ( FAILED ( hr ) || ( NULL == pFrame ) )
	{
		return ( SUCCEEDED ( hr ) ) ? E_UNEXPECTED : hr;
	}

	if (NULL == path || !(V_VT(path) ==  VT_BSTR || V_VT(path) == (VT_BSTR|VT_BYREF)))
		return E_INVALIDARG;

	// prompt user overrides any doc name that is specified
	if (promptUser && (V_VT(promptUser) != VT_EMPTY && V_VT(promptUser) != VT_ERROR))
	{
		// note that if promptUser is not type VT_BOOL or VT_BOOL|VT_BYREF
		// then user is not prompted

#pragma warning(disable: 4310) // cast truncates constant value
		if (VT_BOOL == V_VT(promptUser))
			bPromptUser = (VARIANT_TRUE == V_BOOL(promptUser)) ? TRUE : FALSE;
		else if ((VT_BOOL|VT_BYREF) == V_VT(promptUser))
		{
			_ASSERTE(V_BOOLREF(promptUser));

			if (V_BOOLREF(promptUser))
				bPromptUser = (BOOL) (*(V_BOOLREF(promptUser)) == VARIANT_TRUE) ? TRUE : FALSE;
		}
#pragma warning(default: 4310) // cast truncates constant value
	}

	// prompt user overrides any doc name that is specified
	// Change VK:
	// ...but the provided doc name is used as the default.  If doc name is empty,
	// and the doc was opened from a file, the original file name is provided as a default.
	if (bPromptUser)
	{
		if ( NULL != path->bstrVal )
		{
			_tcsncpy ( promptPath, OLE2T(path->bstrVal), MAX_PATH );
			if ( 0 == _tcslen ( promptPath ) )
			{
				CComBSTR bstrFileName;

				if ( SUCCEEDED ( pFrame->GetCurDocNameWOPath ( bstrFileName ) ) )
				{
					_tcsncpy ( promptPath, OLE2T(bstrFileName), MAX_PATH );
				}
			}
		}
		hr = PromptSaveAsFile(promptPath, MAX_PATH);

		if (S_FALSE == hr)
			return S_OK;

		bstrPath = promptPath;
		_path = bstrPath;
	}
	else
	{	
		if ((VT_BSTR|VT_BYREF) == V_VT(path) && V_BSTRREF(path))
			_path = *(V_BSTRREF(path));
		else if (VT_BSTR == V_VT(path) && V_BSTR(path))
			_path = V_BSTR(path);
	}

	hr = pFrame->SaveDocument(_path);
	return hr;
}


STDMETHODIMP CDHTMLEdit::LoadURL ( BSTR url )
{
	HRESULT			hr		= E_FAIL;
	CProxyFrame*	pFrame	= NULL;

	hr = m_pInterconnect->GetInterconnector ( (SIZE_T*)&pFrame );
	_ASSERTE ( SUCCEEDED ( hr ) );
	_ASSERTE ( pFrame );
	if ( FAILED ( hr ) || ( NULL == pFrame ) )
	{
		return ( SUCCEEDED ( hr ) ) ? E_UNEXPECTED : hr;
	}

	if ( ( NULL == url ) || ( 0 == SysStringLen ( url ) ) )
		return E_INVALIDARG;

	hr = pFrame->LoadDocument( url, TRUE );

	return hr;
}


STDMETHODIMP CDHTMLEdit::PrintDocument ( VARIANT* pvarWithUI )
{
	BOOL	bfWithUI	= FALSE;
	HRESULT	hr			= E_FAIL;
	CProxyFrame*	pFrame	= NULL;

	hr = m_pInterconnect->GetInterconnector ( (SIZE_T*)&pFrame );
	_ASSERTE ( SUCCEEDED ( hr ) );
	_ASSERTE ( pFrame );
	if ( FAILED ( hr ) || ( NULL == pFrame ) )
	{
		return ( SUCCEEDED ( hr ) ) ? E_UNEXPECTED : hr;
	}

	if ( NULL != pvarWithUI )
	{
		CComVariant	varLocal = *pvarWithUI;

		hr = varLocal.ChangeType ( VT_BOOL );
		if ( SUCCEEDED ( hr ) )
		{
			bfWithUI = varLocal.boolVal;	// VariantBool to Bool is safe, not the reverse.
		}
	}

	hr = pFrame->Print ( bfWithUI );
	return S_OK;  // We can't return anything meaningful, because w/UI, Cancel returns E_FAIL.
}


STDMETHODIMP CDHTMLEdit::get_BrowseMode(/* [retval][out] */ VARIANT_BOOL  *pVal)
{
	HRESULT			hr		= S_OK;
	CProxyFrame*	pFrame	= NULL;

	hr = m_pInterconnect->GetInterconnector ( (SIZE_T*)&pFrame );
	_ASSERTE ( SUCCEEDED ( hr ) );
	_ASSERTE ( pFrame );
	if ( FAILED ( hr ) || ( NULL == pFrame ) )
	{
		return ( SUCCEEDED ( hr ) ) ? E_UNEXPECTED : hr;
	}

	_ASSERTE ( pVal );
	if ( NULL == pVal )
	{
		return E_INVALIDARG;
	}

	return pFrame->GetBrowseMode ( pVal );
}


STDMETHODIMP CDHTMLEdit::put_BrowseMode(/* [in] */ VARIANT_BOOL newVal)
{
	HRESULT			hr		= S_OK;
	CProxyFrame*	pFrame	= NULL;

	hr = m_pInterconnect->GetInterconnector ( (SIZE_T*)&pFrame );
	_ASSERTE ( SUCCEEDED ( hr ) );
	_ASSERTE ( pFrame );
	if ( FAILED ( hr ) || ( NULL == pFrame ) )
	{
		return ( SUCCEEDED ( hr ) ) ? E_UNEXPECTED : hr;
	}

	hr = pFrame->SetBrowseMode ( newVal );
	if ( SUCCEEDED ( hr ) )
	{
		m_pInterconnect->MakeDirty ( DISPID_BROWSEMODE );
	}

	return hr;
}


//	To be safe, restrict the range of cmdIDs to a known set.
//
STDMETHODIMP CDHTMLEdit::ExecCommand(DHTMLEDITCMDID cmdID, OLECMDEXECOPT cmdexecopt, LPVARIANT pInVar, LPVARIANT pOutVar)
{
	HRESULT			hr			= S_OK;
	LPVARIANT		_pVarIn		= NULL;
	LPVARIANT		_pVarOut	= NULL;
	CProxyFrame*	pFrame	= NULL;

	hr = m_pInterconnect->GetInterconnector ( (SIZE_T*)&pFrame );
	_ASSERTE ( SUCCEEDED ( hr ) );
	_ASSERTE ( pFrame );
	if ( FAILED ( hr ) || ( NULL == pFrame ) )
	{
		return ( SUCCEEDED ( hr ) ) ? E_UNEXPECTED : hr;
	}

	// It is valid for pVar to be VT_EMPTY (on a DECMD_GETXXX op) but not VT_ERROR

	if (pInVar && (V_VT(pInVar) != VT_ERROR))
		_pVarIn = pInVar;

	if (pOutVar && (V_VT(pOutVar) != VT_ERROR))
		_pVarOut = pOutVar;

	if ( ( cmdexecopt < OLECMDEXECOPT_DODEFAULT ) ||
		 ( cmdexecopt >  OLECMDEXECOPT_DONTPROMPTUSER ) )
	{
		return E_INVALIDARG;
	}

	hr = pFrame->HrMapExecCommand(cmdID, cmdexecopt, _pVarIn, _pVarOut);

	return hr;
}


/*
 * IServiceProvider implementation
 */
STDMETHODIMP CDHTMLEdit::QueryService( REFGUID guidService, REFIID riid, void** ppvService )
{
	*ppvService = NULL;
	if ( SID_SInternetSecurityManager == guidService )
	{
		return GetUnknown()->QueryInterface ( riid, ppvService );
	}
	return E_NOINTERFACE;
}



/*
 * IInternetSecurityManager implementation
 *
 * The purpose of this implementation is to OVERRIDE security and reduce it to the minimum.
 * This should only be provided in Edit mode, not in browse mode. (Browse mode edits scripts.)
 * This prevents warnings about unsafe for scripting DTCs, etc.
 *
 * From HTMED/TriSite, by Carlos Gomes.
 *
 */

STDMETHODIMP CDHTMLEdit::GetSecurityId ( LPCWSTR /*pwszUrl*/, BYTE* /*pbSecurityId*/,
	DWORD* /*pcbSecurityId*/, DWORD_PTR /*dwReserved*/ )
{
	return INET_E_DEFAULT_ACTION;
}


STDMETHODIMP CDHTMLEdit::GetSecuritySite ( IInternetSecurityMgrSite** /*ppSite*/ )
{
	return INET_E_DEFAULT_ACTION;
}


STDMETHODIMP CDHTMLEdit::GetZoneMappings ( DWORD /*dwZone*/, IEnumString** /*ppenumString*/, DWORD /*dwFlags*/ )
{
	return INET_E_DEFAULT_ACTION;
}


STDMETHODIMP CDHTMLEdit::MapUrlToZone ( LPCWSTR /*pwszUrl*/, DWORD *pdwZone, DWORD /*dwFlags*/ )
{
	if ( pdwZone != NULL )
	{
		*pdwZone = URLZONE_LOCAL_MACHINE;
		return NOERROR;
	}
	return INET_E_DEFAULT_ACTION;
}


STDMETHODIMP CDHTMLEdit::ProcessUrlAction ( LPCWSTR /*pwszUrl*/, DWORD dwAction, BYTE* pPolicy, DWORD cbPolicy,
	BYTE* /*pContext*/, DWORD /*cbContext*/, DWORD /*dwFlags*/, DWORD /*dwReserved*/ )
{
	_ASSERTE ( pPolicy );
	if ( NULL == pPolicy )
	{
		return E_INVALIDARG;
	}

	// Handle
	// URLACTION_DOWNLOAD_SIGNED_ACTIVEX
	// URLACTION_ACTIVEX_OVERRIDE_OBJECT_SAFETY
	// URLACTION_ACTIVEX_OVERRIDE_DATA_SAFETY
	// URLACTION_ACTIVEX_OVERRIDE_SCRIPT_SAFETY
	// URLACTION_SCRIPT_OVERRIDE_SAFETY
	// URLACTION_ACTIVEX_RUN
	// URLACTION_ACTIVEX_CONFIRM_NOOBJECTSAFETY
	// URLACTION_SCRIPT_SAFE_ACTIVEX
	//
	if(dwAction >= URLACTION_ACTIVEX_MIN && dwAction <= URLACTION_ACTIVEX_MAX)
	{
		if (cbPolicy >= sizeof(DWORD))
		{
			*(DWORD *)pPolicy = URLPOLICY_ALLOW;
			return S_OK;
		}
		return S_FALSE;
	}
	//
	// Handle
	// URLACTION_DOWNLOAD_SIGNED_ACTIVEX
	// URLACTION_DOWNLOAD_UNSIGNED_ACTIVEX
	//
	else if(dwAction >= URLACTION_DOWNLOAD_MIN && dwAction <= URLACTION_DOWNLOAD_MAX)
	{
		if (cbPolicy >= sizeof(DWORD))
		{
			*(DWORD *)pPolicy = URLPOLICY_ALLOW;
			return S_OK;
		}
		return S_FALSE;
	}
	//
	// Handle
	// URLACTION_SCRIPT_RUN
	// URLACTION_SCRIPT_JAVA_USE
	// URLACTION_SCRIPT_SAFE_ACTIVEX
	//
	else if(dwAction >= URLACTION_SCRIPT_MIN && dwAction <= URLACTION_SCRIPT_MAX)
	{
		if (cbPolicy >= sizeof(DWORD))
		{
			*(DWORD *)pPolicy = URLPOLICY_ALLOW;
			return S_OK;
		}
		return S_FALSE;
	}
	//
	// Allow applets to do anything they want.
	// Provide the java permissions.
	//
	else if(dwAction == URLACTION_JAVA_PERMISSIONS)
	{
		if (cbPolicy >= sizeof(DWORD))
		{
			//
			// URLPOLICY_JAVA_LOW
			// Set low Java security. Java applets will be allowed to
			// do high-capability operations, such as file I/O.
			//
			*(DWORD *)pPolicy = URLPOLICY_JAVA_LOW;
			return S_OK;
		}
		return S_FALSE;
	}
	return INET_E_DEFAULT_ACTION;
}


STDMETHODIMP CDHTMLEdit::QueryCustomPolicy ( LPCWSTR /*pwszUrl*/, REFGUID /*guidKey*/,
	BYTE** /*ppPolicy*/, DWORD* /*pcbPolicy*/, BYTE* /*pContext*/, DWORD /*cbContext*/, DWORD /*dwReserved*/ )
{
	return INET_E_DEFAULT_ACTION;
}


STDMETHODIMP CDHTMLEdit::SetSecuritySite ( IInternetSecurityMgrSite* /*pSite*/ )
{
	return INET_E_DEFAULT_ACTION;
}


STDMETHODIMP CDHTMLEdit::SetZoneMapping ( DWORD /*dwZone*/, LPCWSTR /*lpszPattern*/, DWORD /*dwFlags*/ )
{
	return INET_E_DEFAULT_ACTION;
}


// Map to aggregated control's methods:
//
STDMETHODIMP CDHTMLEdit::get_IsDirty(VARIANT_BOOL *pVal) {return m_pInnerCtl->get_IsDirty ( pVal );}
STDMETHODIMP CDHTMLEdit::get_SourceCodePreservation(VARIANT_BOOL *pVal) {return m_pInnerCtl->get_SourceCodePreservation ( pVal );}
STDMETHODIMP CDHTMLEdit::put_SourceCodePreservation(VARIANT_BOOL newVal) {return m_pInnerCtl->put_SourceCodePreservation ( newVal );}
STDMETHODIMP CDHTMLEdit::get_ScrollbarAppearance(DHTMLEDITAPPEARANCE *pVal) {return m_pInnerCtl->get_ScrollbarAppearance ( pVal );}
STDMETHODIMP CDHTMLEdit::put_ScrollbarAppearance(DHTMLEDITAPPEARANCE newVal) {return m_pInnerCtl->put_ScrollbarAppearance ( newVal );}
STDMETHODIMP CDHTMLEdit::get_Scrollbars(VARIANT_BOOL *pVal) {return m_pInnerCtl->get_Scrollbars ( pVal );}
STDMETHODIMP CDHTMLEdit::put_Scrollbars(VARIANT_BOOL newVal) {return m_pInnerCtl->put_Scrollbars ( newVal );}
STDMETHODIMP CDHTMLEdit::get_Appearance(DHTMLEDITAPPEARANCE *pVal) {return m_pInnerCtl->get_Appearance ( pVal );}
STDMETHODIMP CDHTMLEdit::put_Appearance(DHTMLEDITAPPEARANCE newVal) {return m_pInnerCtl->put_Appearance ( newVal );}
STDMETHODIMP CDHTMLEdit::get_ShowBorders(VARIANT_BOOL *pVal) {return m_pInnerCtl->get_ShowBorders ( pVal );}
STDMETHODIMP CDHTMLEdit::put_ShowBorders(VARIANT_BOOL newVal) {return m_pInnerCtl->put_ShowBorders ( newVal );}
STDMETHODIMP CDHTMLEdit::get_ShowDetails(VARIANT_BOOL *pVal) {return m_pInnerCtl->get_ShowDetails ( pVal );}
STDMETHODIMP CDHTMLEdit::put_ShowDetails(VARIANT_BOOL newVal) {return m_pInnerCtl->put_ShowDetails ( newVal );}
STDMETHODIMP CDHTMLEdit::get_ActivateDTCs(VARIANT_BOOL *pVal) {return m_pInnerCtl->get_ActivateDTCs ( pVal );}
STDMETHODIMP CDHTMLEdit::put_ActivateDTCs(VARIANT_BOOL newVal) {return m_pInnerCtl->put_ActivateDTCs ( newVal );}
STDMETHODIMP CDHTMLEdit::get_ActivateActiveXControls(VARIANT_BOOL *pVal) {return m_pInnerCtl->get_ActivateActiveXControls ( pVal );}
STDMETHODIMP CDHTMLEdit::put_ActivateActiveXControls(VARIANT_BOOL newVal) {return m_pInnerCtl->put_ActivateActiveXControls ( newVal );}
STDMETHODIMP CDHTMLEdit::get_ActivateApplets(VARIANT_BOOL *pVal) {return m_pInnerCtl->get_ActivateApplets ( pVal );}
STDMETHODIMP CDHTMLEdit::put_ActivateApplets(VARIANT_BOOL newVal) {return m_pInnerCtl->put_ActivateApplets ( newVal );}
STDMETHODIMP CDHTMLEdit::get_DOM(IHTMLDocument2 **pVal) {return m_pInnerCtl->get_DOM ( pVal );}
STDMETHODIMP CDHTMLEdit::get_DocumentHTML(BSTR *pVal) {return m_pInnerCtl->get_DocumentHTML ( pVal );}
STDMETHODIMP CDHTMLEdit::put_DocumentHTML(BSTR newVal) {return m_pInnerCtl->put_DocumentHTML ( newVal );}
STDMETHODIMP CDHTMLEdit::get_AbsoluteDropMode(VARIANT_BOOL *pVal) {return m_pInnerCtl->get_AbsoluteDropMode ( pVal );}
STDMETHODIMP CDHTMLEdit::put_AbsoluteDropMode(VARIANT_BOOL newVal) {return m_pInnerCtl->put_AbsoluteDropMode ( newVal );}
STDMETHODIMP CDHTMLEdit::get_SnapToGridX(LONG  *pVal) {return m_pInnerCtl->get_SnapToGridX ( pVal );}
STDMETHODIMP CDHTMLEdit::put_SnapToGridX(LONG newVal) {return m_pInnerCtl->put_SnapToGridX ( newVal );}
STDMETHODIMP CDHTMLEdit::get_SnapToGridY(LONG  *pVal) {return m_pInnerCtl->get_SnapToGridY ( pVal );}
STDMETHODIMP CDHTMLEdit::put_SnapToGridY(LONG newVal) {return m_pInnerCtl->put_SnapToGridY ( newVal );}
STDMETHODIMP CDHTMLEdit::get_SnapToGrid(VARIANT_BOOL  *pVal) {return m_pInnerCtl->get_SnapToGrid ( pVal );}
STDMETHODIMP CDHTMLEdit::put_SnapToGrid(VARIANT_BOOL newVal) {return m_pInnerCtl->put_SnapToGrid ( newVal );}
STDMETHODIMP CDHTMLEdit::get_CurrentDocumentPath(BSTR  *pVal) {return m_pInnerCtl->get_CurrentDocumentPath ( pVal );}
STDMETHODIMP CDHTMLEdit::QueryStatus(DHTMLEDITCMDID cmdID, DHTMLEDITCMDF* retval) {return m_pInnerCtl->QueryStatus ( cmdID, retval );}
STDMETHODIMP CDHTMLEdit::SetContextMenu(LPVARIANT menuStrings,LPVARIANT menuStates) {return m_pInnerCtl->SetContextMenu ( menuStrings, menuStates );}
STDMETHODIMP CDHTMLEdit::get_BaseURL(BSTR  *baseURL) {return m_pInnerCtl->get_BaseURL(baseURL);}
STDMETHODIMP CDHTMLEdit::put_BaseURL(BSTR baseURL) {return m_pInnerCtl->put_BaseURL(baseURL);}
STDMETHODIMP CDHTMLEdit::get_DocumentTitle(BSTR  *docTitle) {return m_pInnerCtl->get_DocumentTitle(docTitle);}
STDMETHODIMP CDHTMLEdit::NewDocument() {return m_pInnerCtl->NewDocument();}
STDMETHODIMP CDHTMLEdit::get_UseDivOnCarriageReturn(VARIANT_BOOL *pVal) {return m_pInnerCtl->get_UseDivOnCarriageReturn(pVal);}
STDMETHODIMP CDHTMLEdit::put_UseDivOnCarriageReturn(VARIANT_BOOL newVal) {return m_pInnerCtl->put_UseDivOnCarriageReturn(newVal);}
STDMETHODIMP CDHTMLEdit::FilterSourceCode(BSTR sourceCodeIn, BSTR* sourceCodeOut) {return m_pInnerCtl->FilterSourceCode(sourceCodeIn, sourceCodeOut);}
STDMETHODIMP CDHTMLEdit::Refresh() {return m_pInnerCtl->Refresh();}
STDMETHODIMP CDHTMLEdit::get_Busy(VARIANT_BOOL *pVal) {return m_pInnerCtl->get_Busy(pVal);}

// End of DHTMLEdit.cpp
