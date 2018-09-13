// PrevCtrl.cpp : Implementation of CPreview

#include "precomp.h"

#include "shimgvw.h"
#include "PrevCtrl.h"

#define MIN(x,y)    ((x<y)?x:y)
#define MAX(x,y)    ((x>y)?x:y)

/////////////////////////////////////////////////////////////////////////////
// CPreview

/////////////////////////////////////////////////////////////////////////////
// Control Message handlers
/////////////////////////////////////////////////////////////////////////////

LRESULT CPreview::OnCreate(UINT , WPARAM , LPARAM , BOOL& )
{
    ATLTRACE( _T("CPreview::OnCreate\n") );
    RECT rcWnd;
    GetClientRect( &rcWnd );

    // Create the preview window
    if ( m_cwndPreview.Create(m_hWnd, rcWnd, NULL, WS_CHILD|WS_VISIBLE, 0 ) )
    {
        m_cwndPreview.SetNotify( this );
        return 0;
    }
    return -1;
}

LRESULT CPreview::OnActivate(UINT , WPARAM , LPARAM , BOOL& bHandled)
{
    ATLTRACE( _T("CPreview::OnActivate\n") );
    m_cwndPreview.SetFocus();
    bHandled = false;
    return 0;
}

HRESULT CPreview::OnDrawAdvanced(ATL_DRAWINFO& )
{
    ATLTRACE( _T("CPreview::OnDrawAdvanced\n") );
    return S_OK;
}

LRESULT CPreview::OnEraseBkgnd(UINT , WPARAM , LPARAM , BOOL& )
{
    ATLTRACE( _T("CPreview::OnEraseBkgnd\n") );
    return TRUE;
}

LRESULT CPreview::OnSize(UINT , WPARAM , LPARAM lParam, BOOL& )
{
    ATLTRACE( _T("CPreview::OnSize\n") );
    ::SetWindowPos(m_cwndPreview.m_hWnd, NULL, 0,0,
        LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER | SWP_NOACTIVATE);
    return 0;
}

// IObjectSafety::GetInterfaceSafetyOptions
//
// This method never gets called.  We are safe for any and every thing.  There should
// be no possible way that this control could lose, destroy, or expose data.
STDMETHODIMP CPreview::GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions,
                                                  DWORD *pdwEnabledOptions)
{
    ATLTRACE(_T("IObjectSafetyImpl::GetInterfaceSafetyOptions\n"));
    if (pdwSupportedOptions == NULL || pdwEnabledOptions == NULL)
        return E_POINTER;
    HRESULT hr = S_OK;
    if (riid == IID_IDispatch || riid == IID_IPersistPropertyBag)
    {
        *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;
        *pdwEnabledOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;
    }
    else
    {
        *pdwSupportedOptions = 0;
        *pdwEnabledOptions = 0;
        hr = E_NOINTERFACE;
    }
    return hr;
}

STDMETHODIMP CPreview::SetInterfaceSafetyOptions(REFIID riid, DWORD dwSupportedOptions,
                                                  DWORD dwEnabledOptions)
{
	ATLTRACE(_T("IObjectSafetyImpl::SetInterfaceSafetyOptions\n"));
	// We are always safe for any data or script call on both of these interfaces
	if (riid == IID_IDispatch || riid == IID_IPersistPropertyBag)
	{
		return S_OK;
	}
	return E_NOINTERFACE;
}

// IPersistPropertyBag::Load
//
// We have the following properties that we can load from the property bag:
//      Toolbar         false/zero = don't show the toolbar, otherwise show the toolbar
//      Full Screen     false/zero = don't show fullscreen button on toolbar, otherwise show the button
//      Context Menu    false/zero = don't show context menu, otherwise show the context menu when the user right clicks
//      Print Button    false/zero = don't show print button on toolbar, otherwise show the button
STDMETHODIMP CPreview::Load(IPropertyBag * pPropBag, IErrorLog * pErrorLog)
{
    HRESULT hr;
    VARIANT var;
    BOOL bDummy = TRUE;

    var.vt = VT_UI4;
    var.ulVal = TRUE;
    hr = pPropBag->Read(L"Toolbar", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_UI4)
    {
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_TOOLBAR,var.ulVal,bDummy);
    }

    var.vt = VT_UI4;
    var.ulVal = TRUE;
    hr = pPropBag->Read(L"Full Screen", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_UI4)
    {
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_FULLSCREENBTN,var.ulVal,bDummy);
    }

    var.vt = VT_UI4;
    var.ulVal = TRUE;
    hr = pPropBag->Read(L"Print Button", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_UI4)
    {
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_PRINTBTN,var.ulVal,bDummy);
    }

    var.vt = VT_UI4;
    var.ulVal = TRUE;
    hr = pPropBag->Read(L"Context Menu", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_UI4)
    {
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_CONTEXTMENU,var.ulVal,bDummy);
    }

    var.vt = VT_UI4;
    var.ulVal = FALSE;
    hr = pPropBag->Read(L"Allow Online", &var, NULL);
    if (SUCCEEDED(hr) && var.vt==VT_UI4)
    {
        m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_ALLOWGOONLINE,var.ulVal,bDummy);
    }

    return S_OK;
}

// IPreview Methods:
STDMETHODIMP CPreview::ShowFile(BSTR bstrFileName, int iSelectCount)
{
    m_cwndPreview.SendMessage( IV_SHOWFILEW, iSelectCount, (LPARAM)bstrFileName );
    return S_OK;
}

STDMETHODIMP CPreview::Show(VARIANT var)
{
    HRESULT hr;
    FolderItem * pfi;
    FolderItems * pfis;

    switch (var.vt)
    {
    case VT_UNKNOWN:
    case VT_DISPATCH:
        // QI for Folder Item
        if ( var.punkVal )
        {
            hr = var.punkVal->QueryInterface(IID_FolderItem, (void**)&pfi);
            if ( SUCCEEDED(hr) )
            {
                // If the item is a link we want to get the link's target:
                VARIANT_BOOL vbool;
                hr = pfi->get_IsLink(&vbool);
                if ( SUCCEEDED(hr) && (VARIANT_FALSE != vbool) )    // IsLink returns TRUE, not VARIANT_TRUE
                {
                    IDispatch * pdisp;

                    hr = pfi->get_GetLink(&pdisp);
                    if ( SUCCEEDED(hr) && pdisp )
                    {
                        IShellLinkDual2 * psl2;
                        hr = pdisp->QueryInterface( IID_IShellLinkDual2, (void**)&psl2 );
                        if ( SUCCEEDED(hr) && psl2 )
                        {
                            FolderItem * pfiTarg;
                            hr = psl2->get_Target(&pfiTarg);
                            if ( SUCCEEDED(hr) && pfiTarg )
                            {
                                pfi->Release();
                                pfi = pfiTarg;
                            }
                            psl2->Release();
                        }
                        pdisp->Release();
                    }
                }

                // We now have the folder item for the target, get it's context menu.  We
                // look at the context menu to see if the item is printable.
                FolderItemVerbs * pfivs;
                BOOL bPrintable = FALSE;        // assume not printable to start with

                hr = pfi->Verbs( &pfivs );
                if ( SUCCEEDED(hr) )
                {
                    long cVerbs;

                    hr = pfivs->get_Count(&cVerbs);
                    if ( SUCCEEDED(hr) )
                    {
                        FolderItemVerb * pverb;
                        VARIANT varItem;
                        varItem.vt = VT_I4;

                        for ( --cVerbs; cVerbs >= 0; cVerbs-- )
                        {
                            varItem.lVal = cVerbs;
                            hr = pfivs->Item( varItem, &pverb );
                            if ( SUCCEEDED(hr) )
                            {
                                // BUGBUG: Need to add a method to get the canonical verb name
                                BSTR bstr;
                                hr = pverb->get_Name( &bstr );
                                if ( SUCCEEDED(hr) )
                                {
                                    // BUGBUG: should check against the canonical string "&Print" which will avoid all
                                    // this load and convert to WCHAR crap
                                    WCHAR szPrint[256];
                                    if ( LoadStringWrapW(_Module.GetModuleInstance(), IDS_PRINT, szPrint, ARRAYSIZE(szPrint)) )
                                    {
                                        if ( 0 == StrCmpW( bstr, szPrint ) )
                                        {
                                            bPrintable = TRUE;
                                            cVerbs = -1;
                                        }
                                    }
                                    SysFreeString(bstr);
                                }
                                pverb->Release();
                            }
                        }
                    }
                    pfivs->Release();
                }

                // Now we need to know the path for this item.  We can only view items if
                // we can get a path or URL to the target so some namespaces aren't viewable.
                BSTR bstr;
                hr = pfi->get_Path(&bstr);
                if ( SUCCEEDED(hr) )
                {
                    m_cwndPreview.SendMessage( IV_SHOWFILEW, 1, (LPARAM)bstr );
                    SysFreeString(bstr);
                    hr = S_OK;
                }
                else
                {
                    // we couldn't get the path so we will display the "No Preview" message
                    bPrintable = FALSE;
                    m_cwndPreview.SendMessage( IV_SHOWFILEW, 1, NULL );
                    hr = S_FALSE;
                }

                // Finally we need to set the printable verb.  We do this after setting the
                // path above to avoid the possibility of processing a print when we don't
                // know the path.
                put_printable(bPrintable);

                // now release the Folder Item pointer
                pfi->Release();

                return hr;
            }
            else if ( SUCCEEDED( var.punkVal->QueryInterface(IID_FolderItems, (void**)&pfis) ) )
            {
                // currently in the multi-select case we just show the multi-select message.
                // eventually this should go to slideshow mode
                m_cwndPreview.SendMessage( IV_SHOWFILEW, 2, NULL );
                pfis->Release();
                return S_FALSE;
            }
        }

        // the unknown pointer isn't for an object type that we know about
        return E_INVALIDARG;

    case VT_BSTR:
        m_cwndPreview.SendMessage( IV_SHOWFILEW, 1, (LPARAM)var.bstrVal );
        break;

    case VT_BOOL:
        // show(false) will hide the currently previewed item
        if ( VARIANT_FALSE == var.boolVal )
        {
            m_cwndPreview.SendMessage( IV_SHOWFILEW, 0, NULL );
            return S_OK;
        }
        else
        {
            return E_INVALIDARG;
        }

    default:
        return E_INVALIDARG;
    }

	return S_OK;
}

//***   IsVK_TABCycler -- is key a TAB-equivalent
// ENTRY/EXIT
//  dir     0 if not a TAB, non-0 if a TAB
// NOTES
//  NYI: -1 for shift+tab, 1 for tab
//  cloned from browseui/util.cpp (BUGBUG move to shlwapi?)
//
int IsVK_TABCycler(MSG *pMsg)
{
    if (!pMsg)
        return 0;

    if (pMsg->message != WM_KEYDOWN)
        return 0;
    if (! (pMsg->wParam == VK_TAB || pMsg->wParam == VK_F6))
        return 0;

#if 0 // todo?
    return (GetAsyncKeyState(VK_SHIFT) < 0) ? -1 : 1;
#endif
    return 1;
}

//***
// NOTES
//  hard-coded 1/2/4 (vs. KEYMOD_*) is same thing atlctl.h does.  go figure...
DWORD GetGrfMods()
{
    DWORD dwMods;

    dwMods = 0;
    if (GetAsyncKeyState(VK_SHIFT) < 0)
        dwMods |= 1;    // KEYMOD_SHIFT
    if (GetAsyncKeyState(VK_CONTROL) < 0)
        dwMods |= 2;    // KEYMOD_CONTROL
    if (GetAsyncKeyState(VK_MENU) < 0)
        dwMods |= 4;    // KEYMOD_MENU
    return dwMods;
}

STDMETHODIMP CPreview::TranslateAccelerator( LPMSG lpmsg )
{
    ATLTRACE( _T("CPreview::TranslateAccelerator\n") );

    if ( m_cwndPreview.TranslateAccelerator(lpmsg) )
    {
        return S_OK;
    }

    if ( IsVK_TABCycler(lpmsg) )
    {
        // REVIEW: looks like newer versions of ATL might do this for us so
        // possibly we can replace w/ call to SUPER::TA when we upgrade.
        CComQIPtr <IOleControlSite, &IID_IOleControlSite> spOCS(m_spClientSite);
        if (spOCS) {
            return spOCS->TranslateAccelerator(lpmsg, GetGrfMods());
        }
    }

    return S_FALSE;
}

STDMETHODIMP CPreview::OnFrameWindowActivate( BOOL fActive )
{
    if ( fActive )
    {
        m_cwndPreview.SetFocus();
    }
    return S_OK;
}

LRESULT CPreview::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ATLTRACE( _T("CPreview::OnSetFocus\n") );
 
    LRESULT ret = CComControl<CPreview>::OnSetFocus(uMsg,wParam,lParam, bHandled);
    m_cwndPreview.SetFocus();
    return ret;
}

STDMETHODIMP CPreview::get_printable(BOOL * pVal)
{
    *pVal = m_cwndPreview.GetPrintable();

    return S_OK;
}

STDMETHODIMP CPreview::put_printable(BOOL newVal)
{
    BOOL bDummy = TRUE;
 
    m_cwndPreview.IV_OnSetOptions(IV_SETOPTIONS,IVO_PRINTABLE,newVal,bDummy);

    return S_OK;
}

STDMETHODIMP CPreview::get_cxImage(long * pVal)
{
    // REVIEW: Return an error and set output to zero if no image is currently displayed?
    *pVal = m_cwndPreview.m_ctlPreview.m_cxImage;

    return S_OK;
}

STDMETHODIMP CPreview::get_cyImage(long * pVal)
{
    // REVIEW: Return an error and set output to zero if no image is currently displayed?
    *pVal = m_cwndPreview.m_ctlPreview.m_cyImage;

    return S_OK;
}
