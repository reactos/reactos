// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  DEFAULT.CPP
//
//  This is the default implementation for CAccessible. All other objects
//	usually inherit from this one.
//
//	Implements:
//		IUnknown
//			QueryInterface
//			AddRef
//			Release
//		IDispatch
//			GetTypeInfoCount
//			GetTypeInfo
//			GetIDsOfNames
//			Invoke
//		IAccessible
//			get_accParent
//			get_accChildCount
//			get_accChild
//			get_accName
//			get_accValue
//			get_accDescription
//			get_accRole
//			get_accState
//			get_accHelp
//			get_accHelpTopic
//			get_accKeyboardShortcut
//			get_accFocus
//			get_accSelection
//			get_accDefaultAction
//			accSelect
//			accLocation
//			accNavigate
//			accHitTest
//			accDoDefaultAction
//			put_accName
//			put_accValue
//		IEnumVARIANT
//			Next
//			Skip
//			Reset
//			Clone
//		IOleWindow
//			GetWindow
//			ContextSensitiveHelp
//
//		Helper Functions
//			SetupChildren
//			ValidateChild
//			InitTypeInfo
//			TermTypeInfo
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"

// this is for ClickOnTheRect
typedef struct tagMOUSEINFO
{
    int MouseThresh1;
    int MouseThresh2;
    int MouseSpeed;
}
MOUSEINFO, FAR* LPMOUSEINFO;

//-----------------------------------------------------------------
// [v-jaycl, 4/2/97] Table of registered handler CLSIDs.  
//	TODO:Make dynamic. Place at bottom of file w/ other data?
//-----------------------------------------------------------------
CLSID	rgRegisteredTypes[TOTAL_REG_HANDLERS];

//-----------------------------------------------------------------
// [v-jaycl, 6/7/97] [brendanm 9/4/98]
//  loads and initializes registered handlers.
//	Called by FindAndCreatreWindowClass
//-----------------------------------------------------------------
HRESULT CreateRegisteredHandler( HWND      hwnd,
                                 long      idObject,
                                 int       iHandlerIndex,
                                 REFIID    riid,
                                 LPVOID *  ppvObject );

BOOL LookupWindowClass( HWND          hWnd,
                        BOOL          fWindow,
                        LPFNCREATE *  ppfnCreate,
                        int *         pRegHandlerIndex );

BOOL LookupWindowClassName( LPCTSTR       pClassName,
                            BOOL          fWindow,
                            LPFNCREATE *  ppfnCreate,
                            int *         pRegHandlerIndex );


// --------------------------------------------------------------------------
//
//  _purecall()
//
//  We implement this ourself to avoid pulling in CRT gunk.  We don't
//  need to do anything when a pure virtual function isn't redefined.
//
// --------------------------------------------------------------------------
int __cdecl _purecall(void)
{
    DebugBreak();
    return(FALSE);
}

// --------------------------------------------------------------------------
//
//  SharedRead()
//
//  This reads shared memory. SharedAlloc and SharedFree are in memchk.cpp
//
// --------------------------------------------------------------------------
BOOL SharedRead(LPVOID lpvSharedSource,LPVOID lpvDest,DWORD cbSize,HANDLE hProcess)
{
#ifdef _X86_
    if (fWindows95)
    {
        CopyMemory (lpvDest,lpvSharedSource,cbSize);
        return TRUE;
    }
    else
#endif // _X86_
    {
        return (ReadProcessMemory (hProcess,lpvSharedSource,lpvDest,cbSize,NULL));
    }
}

// --------------------------------------------------------------------------
//
//  SharedWrite()
//
//  This writes into shared memory.
//
// --------------------------------------------------------------------------
BOOL SharedWrite(LPVOID lpvSource,LPVOID lpvSharedDest,DWORD cbSize,HANDLE hProcess)
{
#ifdef _X86_
    if (fWindows95)
    {
        CopyMemory(lpvSharedDest,lpvSource,cbSize);
        return TRUE;
    }
    else
#endif // _X86_
    {
        return (WriteProcessMemory (hProcess,lpvSharedDest,lpvSource,cbSize,NULL));
    }
}


CAccessible::~CAccessible()
{
	// Nothing to do
	// (Dtor only exists so that the base class has a virtual dtor, so that
	// derived class dtors work properly when deleted through a base class ptr)
}


// --------------------------------------------------------------------------
//
//  CAccessible::GetWindow()
//
//  This is from IOleWindow, to let us get the HWND from an IAccessible*.
//
// ---------------------------------------------------------------------------
STDMETHODIMP CAccessible::GetWindow(HWND* phwnd)
{
    *phwnd = m_hwnd;
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CAccessible::ContextSensitiveHelp()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::ContextSensitiveHelp(BOOL fEnterMode)
{
	UNUSED(fEnterMode);
    return(E_NOTIMPL);
}


// --------------------------------------------------------------------------
//
//  CAccessible::InitTypeInfo()
//
//  This initializes our type info when we need it for IDispatch junk.
//
// --------------------------------------------------------------------------
HRESULT CAccessible::InitTypeInfo(void)
{
    HRESULT     hr;
    ITypeLib    *piTypeLib;

    if (m_pTypeInfo)
        return(S_OK);

    // Try getting the typelib from the registry
    hr = LoadRegTypeLib(LIBID_Accessibility, 1, 0, 0, &piTypeLib);

    if (FAILED(hr))
    {
        OLECHAR wszPath[MAX_PATH];

        // Try loading directly.
#ifdef UNICODE
        MyGetModuleFileName(NULL, wszPath, ARRAYSIZE(wszPath));
#else
        TCHAR   szPath[MAX_PATH];

        MyGetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
        MultiByteToWideChar(CP_ACP, 0, szPath, -1, wszPath, ARRAYSIZE(wszPath));
#endif

        hr = LoadTypeLib(wszPath, &piTypeLib);
    }

    if (SUCCEEDED(hr))
    {
        hr = piTypeLib->GetTypeInfoOfGuid(IID_IAccessible, &m_pTypeInfo);
        piTypeLib->Release();

        if (!SUCCEEDED(hr))
            m_pTypeInfo = NULL;
    }

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CAccessible::TermTypeInfo()
//
//  This frees the type info if it is around
//
// --------------------------------------------------------------------------
void CAccessible::TermTypeInfo(void)
{
    if (m_pTypeInfo)
    {
        m_pTypeInfo->Release();
        m_pTypeInfo = NULL;
    }
}



// --------------------------------------------------------------------------
//
//  CAccessible::QueryInterface()
//
//  This responds to 
//          * IUnknown 
//          * IDispatch 
//          * IEnumVARIANT
//          * IAccessible
//
//  Some code will also respond to IText.  That code must override our
//  QueryInterface() implementation.
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if ((riid == IID_IUnknown)  ||
        (riid == IID_IDispatch) ||
        (riid == IID_IAccessible))
        *ppv = (IAccessible *)this;
    else if (riid == IID_IEnumVARIANT)
        *ppv = (IEnumVARIANT *)this;
    else if (riid == IID_IOleWindow)
        *ppv = (IOleWindow*)this;
    else
        return(E_NOINTERFACE);

    ((LPUNKNOWN) *ppv)->AddRef();
    return(NOERROR);
}


// --------------------------------------------------------------------------
//
//  CAccessible::AddRef()
//
// --------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAccessible::AddRef()
{
    return(++m_cRef);
}


// --------------------------------------------------------------------------
//
//  CAccessible::Release()
//
// --------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAccessible::Release()
{
    if ((--m_cRef) == 0)
    {
        TermTypeInfo();
        delete this;
        return 0;
    }

    return(m_cRef);
}



// --------------------------------------------------------------------------
//
//  CAccessible::GetTypeInfoCount()
//
//  This hands off to our typelib for IAccessible().  Note that
//  we only implement one type of object for now.  BOGUS!  What about IText?
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::GetTypeInfoCount(UINT * pctInfo)
{
    HRESULT hr;

    InitPv(pctInfo);

    hr = InitTypeInfo();

    if (SUCCEEDED(hr))
        *pctInfo = 1;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CAccessible::GetTypeInfo()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::GetTypeInfo(UINT itInfo, LCID lcid,
    ITypeInfo ** ppITypeInfo)
{
    HRESULT hr;

	UNUSED(lcid);	// locale id is unused

    if (ppITypeInfo == NULL)
        return(E_POINTER);

    InitPv(ppITypeInfo);

    if (itInfo != 0)
        return(TYPE_E_ELEMENTNOTFOUND);

    hr = InitTypeInfo();
    if (SUCCEEDED(hr))
    {
        m_pTypeInfo->AddRef();
        *ppITypeInfo = m_pTypeInfo;
    }

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CAccessible::GetIDsOfNames()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::GetIDsOfNames(REFIID riid,
    OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgDispID)
{
    HRESULT hr;

	UNUSED(lcid);	// locale id is unused
	UNUSED(riid);	// riid is unused

    hr = InitTypeInfo();
    if (!SUCCEEDED(hr))
        return(hr);

    return(m_pTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispID));
}



// --------------------------------------------------------------------------
//
//  CAccessible::Invoke()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::Invoke(DISPID dispID, REFIID riid,
    LCID lcid, WORD wFlags, DISPPARAMS * pDispParams,
    VARIANT* pvarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    HRESULT hr;

	UNUSED(lcid);	// locale id is unused
	UNUSED(riid);	// riid is unused

    hr = InitTypeInfo();
    if (!SUCCEEDED(hr))
        return(hr);

    return(m_pTypeInfo->Invoke((IAccessible *)this, dispID, wFlags,
        pDispParams, pvarResult, pExcepInfo, puArgErr));
}




// --------------------------------------------------------------------------
//
//  TCharSysAllocString()
//
//  Pillaged from SHELL source, does ANSI BSTR stuff.
//
// --------------------------------------------------------------------------
BSTR TCharSysAllocString(LPTSTR pszString)
{
#ifdef UNICODE
    return(SysAllocString(pszString));
#else
    LPOLESTR    pwszOleString;
    BSTR        bstrReturn;
    int         cChars;

    // do the call first with 0 to get the size needed
    cChars = MultiByteToWideChar(CP_ACP, 0, pszString, -1, NULL, 0);
    pwszOleString = (LPOLESTR)LocalAlloc(LPTR,sizeof(OLECHAR)*cChars);
    if (pwszOleString == NULL)
    {
        return (NULL);
    }

	cChars = MultiByteToWideChar(CP_ACP, 0, pszString, -1, pwszOleString, cChars);
    bstrReturn = SysAllocString(pwszOleString);
	LocalFree (pwszOleString);
    return (bstrReturn);
#endif
}



// --------------------------------------------------------------------------
//
//  CAccessible::get_accParent()
//
//  NOTE:  Not only is this the default handler, it can also serve as
//  parameter checking for overriding implementations.
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accParent(IDispatch ** ppdispParent)
{
    InitPv(ppdispParent);

    if (m_hwnd)
        return(AccessibleObjectFromWindow(m_hwnd, OBJID_WINDOW,
            IID_IDispatch, (void **)ppdispParent));
    else
        return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CAccessible::get_accChildCount()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accChildCount(long* pChildCount)
{
    SetupChildren();
    *pChildCount = m_cChildren;
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CAccessible::get_accChild()
//
//  No children.
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accChild(VARIANT varChild, IDispatch** ppdispChild)
{
    InitPv(ppdispChild);

    if (! ValidateChild(&varChild) || !varChild.lVal)
        return(E_INVALIDARG);

    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CAccessible::get_accValue()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accValue(VARIANT varChild, BSTR * pszValue)
{
    InitPv(pszValue);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CAccessible::get_accDescription()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accDescription(VARIANT varChild, BSTR * pszDescription)
{
    InitPv(pszDescription);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CAccessible::get_accHelp()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accHelp(VARIANT varChild, BSTR* pszHelp)
{
    InitPv(pszHelp);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CAccessible::get_accHelpTopic()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accHelpTopic(BSTR* pszHelpFile,
    VARIANT varChild, long* pidTopic)
{
    InitPv(pszHelpFile);
    InitPv(pidTopic);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CAccessible::get_accKeyboardShortcut()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accKeyboardShortcut(VARIANT varChild,
    BSTR* pszShortcut)
{
    InitPv(pszShortcut);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CAccessible::get_accFocus()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accFocus(VARIANT *pvarFocus)
{
    InitPvar(pvarFocus);
    return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CAccessible::get_accSelection()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accSelection(VARIANT* pvarSelection)
{
    InitPvar(pvarSelection);
    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CAccessible::get_accDefaultAction()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::get_accDefaultAction(VARIANT varChild,
    BSTR* pszDefaultAction)
{
    InitPv(pszDefaultAction);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CAccessible::accSelect()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::accSelect(long flagsSel, VARIANT varChild)
{
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (! ValidateSelFlags(flagsSel))
        return(E_INVALIDARG);

    return(S_FALSE);
}


#if 0
// --------------------------------------------------------------------------
//
//  CAccessible::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::accLocation(long* pxLeft, long* pyTop,
    long* pcxWidth, long* pcyHeight, VARIANT varChild)
{
    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_OK);
}
#endif


// --------------------------------------------------------------------------
//
//  CAccessible::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::accNavigate(long navFlags, VARIANT varStart,
    VARIANT *pvarEnd)
{
    InitPvar(pvarEnd);

    if (! ValidateChild(&varStart))
        return(E_INVALIDARG);

    if (!ValidateNavDir(navFlags, varStart.lVal))
        return(E_INVALIDARG);

    return(S_FALSE);
}


#if 0
// --------------------------------------------------------------------------
//
//  CAccessible::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::accHitTest(long xLeft, long yTop,
    VARIANT* pvarChild)
{
    InitPvar(pvarChild);
    return(S_FALSE);
}
#endif


// --------------------------------------------------------------------------
//
//  CAccessible::accDoDefaultAction()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::accDoDefaultAction(VARIANT varChild)
{
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CAccessible::put_accName()
//
//  CALLER frees the string
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::put_accName(VARIANT varChild, BSTR szName)
{
	UNUSED(szName);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CAccessible::put_accValue()
//
//  CALLER frees the string
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::put_accValue(VARIANT varChild, BSTR szValue)
{
	UNUSED(szValue);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CAccessible::Next
//
//  Handles simple Next, where we return back indeces for child elements.
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::Next(ULONG celt, VARIANT* rgvar,
    ULONG* pceltFetched)
{
    VARIANT* pvar;
    long    cFetched;
    long    iCur;

    SetupChildren();

    // Can be NULL
    if (pceltFetched)
        *pceltFetched = 0;

    pvar = rgvar;
    cFetched = 0;
    iCur = m_idChildCur;

    //
    // Loop through our items
    //
    while ((cFetched < (long)celt) && (iCur < m_cChildren))
    {
        cFetched++;
        iCur++;

        //
        // Note this gives us (index)+1 because we incremented iCur
        //
        pvar->vt = VT_I4;
        pvar->lVal = iCur;
        ++pvar;
    }

    //
    // Advance the current position
    //
    m_idChildCur = iCur;

    //
    // Fill in the number fetched
    //
    if (pceltFetched)
        *pceltFetched = cFetched;

    //
    // Return S_FALSE if we grabbed fewer items than requested
    //
    return((cFetched < (long)celt) ? S_FALSE : S_OK);
}



// --------------------------------------------------------------------------
//
//  CAccessible::Skip()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::Skip(ULONG celt)
{
    SetupChildren();

    m_idChildCur += celt;
    if (m_idChildCur > m_cChildren)
        m_idChildCur = m_cChildren;

    //
    // We return S_FALSE if at the end
    //
    return((m_idChildCur >= m_cChildren) ? S_FALSE : S_OK);
}



// --------------------------------------------------------------------------
//
//  CAccessible::Reset()
//
// --------------------------------------------------------------------------
STDMETHODIMP CAccessible::Reset(void)
{
    m_idChildCur = 0;
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  MyGetGUIThreadInfo()
//
//  Calls USER32 function if present.  Fills in cbSize field to save callers
//  some code.
//
// --------------------------------------------------------------------------
BOOL MyGetGUIThreadInfo(DWORD idThread, PGUITHREADINFO lpGui)
{
    if (! lpfnGuiThreadInfo)
        return(FALSE);

    lpGui->cbSize = sizeof(GUITHREADINFO);
    return((* lpfnGuiThreadInfo)(idThread, lpGui));
}


// --------------------------------------------------------------------------
//
//  MyGetCursorInfo()
//
//  Calls USER32 function if present.  Fills in cbSize field to save callers
//  some code.
//
// --------------------------------------------------------------------------
BOOL MyGetCursorInfo(LPCURSORINFO lpci)
{
    if (! lpfnCursorInfo)
        return(FALSE);

    lpci->cbSize = sizeof(CURSORINFO);
    return((* lpfnCursorInfo)(lpci));
}


// --------------------------------------------------------------------------
//
//  MyGetWindowInfo()
//
//  Calls USER32 function if present.  Fills in cbSize field to save callers
//  some code.
//
// --------------------------------------------------------------------------
BOOL MyGetWindowInfo(HWND hwnd, LPWINDOWINFO lpwi)
{
    if (!IsWindow(hwnd))
    {
        DBPRINTF (TEXT("OLEACC: warning - calling MyGetWindowInfo for bad hwnd 0x%x\r\n"),hwnd);
        return (FALSE);
    }

    if (! lpfnWindowInfo)
    {
        // BOGUS
        // beginning of a hack for NT4
        {
            GetWindowRect(hwnd,&lpwi->rcWindow);
            GetClientRect( hwnd, & lpwi->rcClient );
			// Convert client rect to screen coords...
			MapWindowPoints( hwnd, NULL, (POINT *) & lpwi->rcClient, 2 );
            lpwi->dwStyle = GetWindowLong (hwnd,GWL_STYLE);
            lpwi->dwExStyle = GetWindowLong (hwnd,GWL_EXSTYLE);
            lpwi->dwWindowStatus = 0; // should have WS_ACTIVECAPTION in here if active
            lpwi->cxWindowBorders = 0; // wrong
            lpwi->cyWindowBorders = 0; // wrong
            lpwi->atomWindowType = 0;  // wrong, but not used anyways
            lpwi->wCreatorVersion = 0; // wrong, only used in SDM proxy. The "WINVER"
            return (TRUE);

        } // end hack for NT4
        return(FALSE);
    }

    lpwi->cbSize = sizeof(WINDOWINFO);
    return((* lpfnWindowInfo)(hwnd, lpwi));
}



// --------------------------------------------------------------------------
//
//  MyGetMenuBarInfo()
//
//  Calls USER32 function if present.  Fills in cbSize field to save callers
//  some code.
//
// --------------------------------------------------------------------------
BOOL MyGetMenuBarInfo(HWND hwnd, long idObject, long idItem, LPMENUBARINFO lpmbi)
{
HMENU	hMenu;

    if (! lpfnMenuBarInfo)
        return(FALSE);
	
	hMenu = GetMenu(hwnd);
	if (hMenu && IsMenu(hMenu))
	{
		lpmbi->cbSize = sizeof(MENUBARINFO);
		return((* lpfnMenuBarInfo)(hwnd, idObject, idItem, lpmbi));
	}
	else
	{
		hMenu = (HMENU)SendMessage (hwnd,MN_GETHMENU,0,0);
		if (hMenu && IsMenu(hMenu))
		{
			lpmbi->cbSize = sizeof(MENUBARINFO);
			return((* lpfnMenuBarInfo)(hwnd, idObject, idItem, lpmbi));
		}
	}
	// zero out struct
	SetRectEmpty (&lpmbi->rcBar);
    lpmbi->hMenu = NULL;
    lpmbi->hwndMenu = NULL;
    lpmbi->fBarFocused = FALSE;
    lpmbi->fFocused = FALSE;
	return FALSE;
}



// --------------------------------------------------------------------------
//
//  MyGetTitleBarInfo()
//
//  Calls USER32 function if present.  Fills in cbSize field to save callers
//  some code.
//
// --------------------------------------------------------------------------
BOOL MyGetTitleBarInfo(HWND hwnd, LPTITLEBARINFO lpti)
{
    if (! lpfnTitleBarInfo)
        return(FALSE);

    lpti->cbSize = sizeof(TITLEBARINFO);
    return((* lpfnTitleBarInfo)(hwnd, lpti));
}


// --------------------------------------------------------------------------
//
//  MyGetScrollBarInfo
//
//  Calls USER32 function if present.  Fills in cbSize field to save callers
//  some code.
//
// --------------------------------------------------------------------------
BOOL MyGetScrollBarInfo(HWND hwnd, LONG idObject, LPSCROLLBARINFO lpsbi)
{
    if (! lpfnScrollBarInfo)
        return(FALSE);

    lpsbi->cbSize = sizeof(SCROLLBARINFO);
    return((* lpfnScrollBarInfo)(hwnd, idObject, lpsbi));
}


// --------------------------------------------------------------------------
//
//  MyGetComboBoxInfo()
//
//  Calls USER32 if present.  Fills in cbSize field for callers.
//
// --------------------------------------------------------------------------
BOOL MyGetComboBoxInfo(HWND hwnd, LPCOMBOBOXINFO lpcbi)
{
    if (! lpfnComboBoxInfo)
        return(FALSE);

    lpcbi->cbSize = sizeof(COMBOBOXINFO);
    BOOL b = ((* lpfnComboBoxInfo)(hwnd, lpcbi));

    // ComboEx's have their own child edit that the real COMBO doesn't
    // know about - try and find it...
    // (This may also be called on a ComboLBox list - but we're safe here
    // since it won't have children anyway.)
    if( b && lpcbi->hwndItem == NULL )
    {
        lpcbi->hwndItem = FindWindowEx( hwnd, NULL, TEXT("EDIT"), NULL );
        if( lpcbi->hwndItem )
        {
            // Get real item area from area of Edit.
            // (In a ComboEx, there's a gap between the left edge of the
            // combo and the left edge of the Edit, where an icon is drawn)
            GetWindowRect( lpcbi->hwndItem, & lpcbi->rcItem );
            MapWindowPoints( HWND_DESKTOP, hwnd, (POINT*)& lpcbi->rcItem, 2 );
        }
    }

    return b;
}


// --------------------------------------------------------------------------
//
//  MyGetAncestor()
//
//  This gets the ancestor window where
//      GA_PARENT   gets the "real" parent window
//      GA_ROOT     gets the "real" top level parent window (not inc. owner)
//      GA_ROOTOWNER    gets the "real" top level parent owner
//
//      * The _real_ parent.  This does NOT include the owner, unlike
//          GetParent().  Stops at a top level window unless we start with
//          the desktop.  In which case, we return the desktop.
//      * The _real_ root, caused by walking up the chain getting the
//          ancestor.
//      * The _real_ owned root, caused by GetParent()ing up.
//
//  Note: On Win98, USER32's winable.c:GetAncestor(GA_ROOT) faults is called
//  on the invisible alt-tab or system pupop windows. To work-around, we're
//  simulating GA_ROOT by looping GA_PARENT (which is actually what winable.c
//  does, only we're more careful about checking for NULL handles...)
//  - see MSAA bug #891
// --------------------------------------------------------------------------
HWND MyGetAncestor(HWND hwnd, UINT gaFlags)
{
    if (! lpfnGetAncestor)
    {
        // BOGUS        
        // This block is here to work around the lack of this function in NT4.
        // It is modeled on the code in winable2.c in USER. 
        {
            HWND	hwndParent;
            HWND	hwndDesktop;
            DWORD   dwStyle;
            
            if (!IsWindow(hwnd))
            {
                //DebugErr(DBF_ERROR, "MyGetAncestor: Bogus window");
                return(NULL);
            }
            
            if ((gaFlags < GA_MIN) || (gaFlags > GA_MAX))
            {
                //DebugErr(DBF_ERROR, "MyGetAncestor: Bogus flags");
                return(NULL);
            }
            
            hwndDesktop = GetDesktopWindow();
            if (hwnd == hwndDesktop)
                return(NULL);
            dwStyle = GetWindowLong (hwnd,GWL_STYLE);
            
            switch (gaFlags)
            {
            case GA_PARENT:
                if (dwStyle & WS_CHILD)
                    hwndParent = GetParent(hwnd);
                else
                    hwndParent = GetWindow (hwnd,GW_OWNER);
				hwnd = hwndParent;
                break;
                
            case GA_ROOT:
                if (dwStyle & WS_CHILD)
                    hwndParent = GetParent(hwnd);
                else
                    hwndParent = GetWindow (hwnd,GW_OWNER);
                while (hwndParent != hwndDesktop &&
                    hwndParent != NULL)
                {
                    hwnd = hwndParent;
                    dwStyle = GetWindowLong(hwnd,GWL_STYLE);
                    if (dwStyle & WS_CHILD)
                        hwndParent = GetParent(hwnd);
                    else
                        hwndParent = GetWindow (hwnd,GW_OWNER);
                }
                break;
                
            case GA_ROOTOWNER:
                while (hwndParent = GetParent(hwnd))
                    hwnd = hwndParent;
                break;
            }
            
            return(hwnd);
        } // end of the workaround block for NT4
        
        return(FALSE);
    }
	else if( gaFlags == GA_ROOT )
	{
		// BOGUS
		// work-around for win98-user inability to handle GA_ROOT
		// correctly on alt-tab (WinSwitch) and Popup windows
		// - see MSAA bug #891

		// (Asise: we *could* special-case 98vs95 - ie. call
		// GA_ROOT as usual on 95 and special case only on 98...
		// Non special-case-ing may be slightly more inefficient, but
		// means that when testing, there's only *one* code path,
		// so we don't have to worry about ensuring that the
		// win95 version behaves the same as the win98 one.)
        HWND hwndDesktop = GetDesktopWindow();

        if( ! IsWindow( hwnd ) )
            return NULL;

		// Climb up through parents - stop if parent is desktop - or NULL...
		for( ; ; )
		{
			HWND hwndParent = lpfnGetAncestor( hwnd, GA_PARENT );
			if( hwndParent == NULL || hwndParent == hwndDesktop )
				break;
			hwnd = hwndParent;
		}

		return hwnd;
	}
	else
	{
        return lpfnGetAncestor(hwnd, gaFlags);
	}
}


// --------------------------------------------------------------------------
//
//  MyRealChildWindowFromPoint()
//
// --------------------------------------------------------------------------
#if 0
// Old version - called USER's 'RealChildWindowFromPoint'.
HWND MyRealChildWindowFromPoint(HWND hwnd, POINT pt)
{
    if (! lpfnRealChildWindowFromPoint)
    {
        // BOGUS
        // beginning of a hack for NT4
        {
            return (ChildWindowFromPoint(hwnd,pt));
        } // end of a hack for NT4
        return(NULL);
    }

    return((* lpfnRealChildWindowFromPoint)(hwnd, pt));
}
#endif

/*
 *  Similar to USER's ChildWindowFromPoint, except this
 *  checks the HT_TRANSPARENT bit.
 *  USER's ChildWindowFromPoint can't "see through" groupboxes or
 *      other HTTRANSPARENT things,
 *  USER's RealChildWindowFromPoint can "see through" groupboxes but
 *      not other HTTRANSPARENT things (it special cases only groupboxes!)
 *  This can see through anything that responds to WM_NCHITTEST with
 *      HTTRANSPARENT.
 */
HWND MyRealChildWindowFromPoint( HWND hwnd,
                                 POINT pt )
{
    HWND hBestFitTransparent = NULL;
    RECT rcBest;

    // Translate hwnd-relative points to screen-relative...
    MapWindowPoints( hwnd, NULL, & pt, 1 );

    // Infinite looping is 'possible' (though unlikely) when
    // using GetWindow(...NEXT), so we counter-limit this loop...
    int SanityLoopCount = 1024;
    for( HWND hChild = GetWindow( hwnd, GW_CHILD ) ;
         hChild && --SanityLoopCount ;
         hChild = GetWindow( hChild, GW_HWNDNEXT ) )
    {
        // Skip invisible...
        if( ! IsWindowVisible( hChild ) )
            continue;

        // Check for rect...
        RECT rc;
        GetWindowRect( hChild, & rc );
        if( ! PtInRect( & rc, pt ) )
            continue;

        // Try for transparency...
        LRESULT lr = SendMessage( hChild, WM_NCHITTEST, 0, MAKELPARAM( pt.x, pt.y ) );
        if( lr == HTTRANSPARENT )
        {
            // For reasons best known to the writers of USER, statics - used
            // as labels - claim to be transparent. So that we do hit-test
            // to these, we remember the hwnd here, so if nothing better
            // comes along, we'll use this.

            // If we come accross two or more of these, we remember the
            // one that fts inside the other - if any. That way,
            // we hit-test to siblings 'within' siblings - eg. statics in
            // a groupbox.

            if( ! hBestFitTransparent )
            {
                hBestFitTransparent = hChild;
                GetWindowRect( hChild, & rcBest );
            }
            else
            {
                // Is this child within the last remembered transparent?
                // If so, remember it instead.
                RECT rcChild;
                GetWindowRect( hChild, & rcChild );
                if( rcChild.left >= rcBest.left &&
                    rcChild.top >= rcBest.top &&
                    rcChild.right <= rcBest.right &&
                    rcChild.bottom <= rcBest.bottom )
                {
                    hBestFitTransparent = hChild;
                    rcBest = rcChild;
                }
            }

            continue;
        }

        // Got the window!
        return hChild;
    }

    if( SanityLoopCount == 0 )
        return NULL;

    // Did we find a transparent (eg. a static) on our travels? If so, since
    // we couldn't find anything better, may as well use it.
    if( hBestFitTransparent )
        return hBestFitTransparent;

    // Otherwise return the original window (not NULL!) if no child found...
    return hwnd;
}


// --------------------------------------------------------------------------
//
//  MyGetFocus()
//
//  Gets the focus on this window's VWI.
//
// --------------------------------------------------------------------------
HWND MyGetFocus()
{
    GUITHREADINFO     gui;

    //
    // Use the foreground thread.  If nobody is the foreground, nobody has
    // the focus either.
    //
    if (!MyGetGUIThreadInfo(0, &gui))
        return(NULL);

    return(gui.hwndFocus);
}


// --------------------------------------------------------------------------
//
//  MyGetRect()
//
//  This initializes the rectangle to empty, then makes a GetClientRect()
//  or GetWindowRect() call.  These APIs will leave the rect alone if they
//  fail, hence the zero'ing out ahead of time.  They don't return a useful
//  value in Win '95.
//
// --------------------------------------------------------------------------
void MyGetRect(HWND hwnd, LPRECT lprc, BOOL fWindowRect)
{
    SetRectEmpty(lprc);

    if (fWindowRect)
        GetWindowRect(hwnd, lprc);
    else
        GetClientRect(hwnd, lprc);
}



// --------------------------------------------------------------------------
//
//  MyGetWindowClass()
//
//  Gets the "real" window type, works for superclassers like "ThunderEdit32"
//  and so on.
//
// --------------------------------------------------------------------------
UINT MyGetWindowClass(HWND hwnd, LPTSTR lpszName, UINT cchName)
{
    *lpszName = 0;

    if (! lpfnRealGetWindowClass)
	{
		// BOGUS 
        // Hack for NT 4
        {
		    return (GetClassName(hwnd,lpszName,cchName));
        } // end of hack for NT 4
        return(0);
	}

    return((* lpfnRealGetWindowClass)(hwnd, lpszName, cchName));
}



// --------------------------------------------------------------------------
//
//  MyGetAltTabInfo()
//
//  Gets the alt tab information
//
// --------------------------------------------------------------------------
BOOL MyGetAltTabInfo(HWND hwnd, int iItem, LPALTTABINFO lpati, LPTSTR lpszItem,
    UINT cchItem)
{
    if (! lpfnAltTabInfo)
        return(FALSE);

    lpati->cbSize = sizeof(ALTTABINFO);

    return((* lpfnAltTabInfo)(hwnd, iItem, lpati, lpszItem, cchItem));
}



// --------------------------------------------------------------------------
//
//  MyGetListBoxInfo()
//
//  Gets the # of items per column currently in a listbox
//
// --------------------------------------------------------------------------
DWORD MyGetListBoxInfo(HWND hwnd)
{
    if (! lpfnGetListBoxInfo)
        return(0);

    return((* lpfnGetListBoxInfo)(hwnd));
}
                                         

// --------------------------------------------------------------------------
//
//  MySendInput()
//
//  Calls USER32 function if present.
//
// --------------------------------------------------------------------------
BOOL MySendInput(UINT cInputs, LPINPUT pInputs, INT cbSize)
{
    if (! lpfnSendInput)
        return(FALSE);

    return((* lpfnSendInput)(cInputs,pInputs,cbSize));
}

//--------------------------------------------------------
// [v-jaycl, 6/7/97] Added MyBlockInput support for NT 4.0 
//--------------------------------------------------------

// --------------------------------------------------------------------------
//
//  MyBlockInput()
//
//  Calls USER32 function if present.
//
// --------------------------------------------------------------------------
BOOL MyBlockInput(BOOL bBlock)
{
    if (! lpfnBlockInput)
        return(FALSE);

    return((* lpfnBlockInput)( bBlock ) );
}

// --------------------------------------------------------------------------
//  MyInterlockedCompareExchange
//
//  Calls the function when we are running on NT
// --------------------------------------------------------------------------
PVOID MyInterlockedCompareExchange(PVOID *Destination,PVOID Exchange,PVOID Comperand)
{
    if (!lpfnInterlockedCompareExchange)
        return (NULL);

    return ((* lpfnInterlockedCompareExchange)(Destination,Exchange,Comperand));
}

// --------------------------------------------------------------------------
//  MyVirtualAllocEx
//
//  Calls the function when we are running on NT
// --------------------------------------------------------------------------
LPVOID MyVirtualAllocEx(HANDLE hProcess,LPVOID lpAddress,DWORD dwSize,DWORD flAllocationType,DWORD flProtect)
{
    if (!lpfnVirtualAllocEx)
        return (NULL);

    return ((* lpfnVirtualAllocEx)(hProcess,lpAddress,dwSize,flAllocationType,flProtect));
}

// --------------------------------------------------------------------------
//  MyVirtualFreeEx
//
//  Calls the function when we are running on NT.
// --------------------------------------------------------------------------
BOOL MyVirtualFreeEx(HANDLE hProcess,LPVOID lpAddress,DWORD dwSize,DWORD dwFreeType)
{
    if (!lpfnVirtualFreeEx)
        return (FALSE);

    return ((* lpfnVirtualFreeEx)(hProcess,lpAddress,dwSize,dwFreeType));
}

// --------------------------------------------------------------------------
//  MyGetModuleFileName
// --------------------------------------------------------------------------
DWORD MyGetModuleFileName(HMODULE hModule,LPTSTR lpFilename,DWORD nSize)
{
    if (!lpfnGetModuleFileName)
        return (0);

    return ((* lpfnGetModuleFileName)(hModule,lpFilename,nSize));
}

// --------------------------------------------------------------------------
//
//  CAccessible::ValidateChild()
//
// --------------------------------------------------------------------------
BOOL CAccessible::ValidateChild(VARIANT *pvar)
{
    //
    // This validates a VARIANT parameter and translates missing/empty
    // params.
    //
    SetupChildren();

    // Missing parameter, a la VBA
TryAgain:
    switch (pvar->vt)
    {
        case VT_VARIANT | VT_BYREF:
            VariantCopy(pvar, pvar->pvarVal);
            goto TryAgain;

        case VT_ERROR:
            if (pvar->scode != DISP_E_PARAMNOTFOUND)
                return(FALSE);
            // FALL THRU

        case VT_EMPTY:
            pvar->vt = VT_I4;
            pvar->lVal = 0;
            break;

// remove this! VT_I2 is not valid!!
#ifdef  VT_I2_IS_VALID  // it isn't now...
        case VT_I2:
            pvar->vt = VT_I4;
            pvar->lVal = (long)pvar->iVal;
            // FALL THROUGH
#endif

        case VT_I4:
            if ((pvar->lVal < 0) || (pvar->lVal > m_cChildren))
                return(FALSE);
            break;

        default:
            return(FALSE);
    }

    return(TRUE);
}



// --------------------------------------------------------------------------
//
//  SetupChildren()
//
//  Default implementation of SetupChildren, does nothing.
//
// --------------------------------------------------------------------------
void CAccessible::SetupChildren(void)
{

}



// --------------------------------------------------------------------------
//
//  HrCreateString()
//
//  Loads a string from the resource file and makes a BSTR from it.
//
// --------------------------------------------------------------------------
HRESULT HrCreateString(int istr, BSTR* pszResult)
{
    TCHAR   szString[CCH_STRING_MAX];

    Assert(pszResult);
    *pszResult = NULL;

    if (!LoadString(hinstResDll, istr, szString, CCH_STRING_MAX))
        return(E_OUTOFMEMORY);

    *pszResult = TCharSysAllocString(szString);
    if (!*pszResult)
        return(E_OUTOFMEMORY);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  GetWindowObject()
//
//  Gets an immediate child object.
//
// --------------------------------------------------------------------------
HRESULT GetWindowObject(HWND hwndChild, VARIANT * pvar)
{
    HRESULT hr;
    IDispatch * pdispChild;

    pvar->vt = VT_EMPTY;

    pdispChild = NULL;

    hr = AccessibleObjectFromWindow(hwndChild, OBJID_WINDOW, IID_IDispatch,
        (void **)&pdispChild);

    if (!SUCCEEDED(hr))
        return(hr);
    if (! pdispChild)
        return(E_FAIL);

    pvar->vt = VT_DISPATCH;
    pvar->pdispVal = pdispChild;

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  GetNoncObject()
//
// --------------------------------------------------------------------------
HRESULT GetNoncObject(HWND hwnd, LONG idFrameEl, VARIANT *pvar)
{
    IDispatch * pdispEl;
    HRESULT hr;

    pvar->vt = VT_EMPTY;

    pdispEl = NULL;

    hr = AccessibleObjectFromWindow(hwnd, idFrameEl, IID_IDispatch,
        (void **)&pdispEl);
    if (!SUCCEEDED(hr))
        return(hr);
    if (!pdispEl)
        return(E_FAIL);

    pvar->vt = VT_DISPATCH;
    pvar->pdispVal = pdispEl;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  GetParentToNavigate()
//
//  Gets the parent IAccessible object, and forwards the navigation request
//  to it using the child's ID.
//
// --------------------------------------------------------------------------
HRESULT GetParentToNavigate(long idChild, HWND hwnd, long idParent, long dwNav,
    VARIANT* pvarEnd)
{
    HRESULT hr;
    IAccessible* poleacc;
    VARIANT varStart;

    //
    // Get our parent
    //
    poleacc = NULL;
    hr = AccessibleObjectFromWindow(hwnd, idParent, IID_IAccessible,
        (void**)&poleacc);
    if (!SUCCEEDED(hr))
        return(hr);

    //
    // Ask it to navigate
    //
    VariantInit(&varStart);
    varStart.vt = VT_I4;
    varStart.lVal = idChild;

    hr = poleacc->accNavigate(dwNav, varStart, pvarEnd);

    //
    // Release our parent
    //
    poleacc->Release();

    return(hr);
}



// --------------------------------------------------------------------------
//
//  ValidateNavDir()
//
//  Validates navigation flags.
//
// --------------------------------------------------------------------------
BOOL ValidateNavDir(long navDir, LONG idChild)
{
	
#ifdef MAX_DEBUG
    DBPRINTF (TEXT("Navigate "));
	switch (navDir)
	{
        case NAVDIR_RIGHT:
            DBPRINTF(TEXT("Right"));
			break;
        case NAVDIR_NEXT:
            DBPRINTF (TEXT("Next"));
			break;
        case NAVDIR_LEFT:
            DBPRINTF (TEXT("Left"));
			break;
        case NAVDIR_PREVIOUS:
            DBPRINTF (TEXT("Previous"));
			break;
        case NAVDIR_UP:
            DBPRINTF (TEXT("Up"));
			break;
        case NAVDIR_DOWN:
            DBPRINTF (TEXT("Down"));
			break;
		case NAVDIR_FIRSTCHILD:
            DBPRINTF (TEXT("First Child"));
			break;
		case NAVDIR_LASTCHILD:
            DBPRINTF (TEXT("Last Child"));
			break;
		default:
            DBPRINTF (TEXT("ERROR"));
	}
    if (idChild <= OBJID_WINDOW)
    {
    TCHAR szChild[50];

        switch (idChild)
        {
            case OBJID_WINDOW:
                lstrcpy (szChild,TEXT("SELF"));
                break;
            case OBJID_SYSMENU:
                lstrcpy (szChild,TEXT("SYS MENU"));
                break;
            case OBJID_TITLEBAR:
                lstrcpy (szChild,TEXT("TITLE BAR"));
                break;
            case OBJID_MENU:
                lstrcpy (szChild,TEXT("MENU"));
                break;
            case OBJID_CLIENT:
                lstrcpy (szChild,TEXT("CLIENT"));
                break;
            case OBJID_VSCROLL:
                lstrcpy (szChild,TEXT("V SCROLL"));
                break;
            case OBJID_HSCROLL:
                lstrcpy (szChild,TEXT("H SCROLL"));
                break;
            case OBJID_SIZEGRIP:
                lstrcpy (szChild,TEXT("SIZE GRIP"));
                break;
            default:
                wsprintf (szChild,TEXT("UNKNOWN 0x%lX"),idChild);
                break;
        }
        DBPRINTF(TEXT(" from child %s\r\n"),szChild);
    }
    else
        DBPRINTF(TEXT(" from child %ld\r\n"),idChild);
#endif

    if ((navDir <= NAVDIR_MIN) || (navDir >= NAVDIR_MAX))
        return(FALSE);

    switch (navDir)
    {
        case NAVDIR_FIRSTCHILD:
        case NAVDIR_LASTCHILD:
            return(idChild == 0);
    }

    return(TRUE);
}

// --------------------------------------------------------------------------
//
//  ValidateSelFlags()
//
//  Validates selection flags.
// this makes sure the only bits set are in the valid range and that you don't
// have any invalid combinations.
// Invalid combinations are
// ADDSELECTION and REMOVESELECTION
// ADDSELECTION and TAKESELECTION
// REMOVESELECTION and TAKESELECTION
// EXTENDSELECTION and TAKESELECTION
//
// --------------------------------------------------------------------------
BOOL ValidateSelFlags(long flags)
{
    if (!ValidateFlags((flags), SELFLAG_VALID))
        return (FALSE);

    if ((flags & SELFLAG_ADDSELECTION) && 
        (flags & SELFLAG_REMOVESELECTION))
        return FALSE;

    if ((flags & SELFLAG_ADDSELECTION) && 
        (flags & SELFLAG_TAKESELECTION))
        return FALSE;

    if ((flags & SELFLAG_REMOVESELECTION) && 
        (flags & SELFLAG_TAKESELECTION))
        return FALSE;

    if ((flags & SELFLAG_EXTENDSELECTION) && 
        (flags & SELFLAG_TAKESELECTION))
        return FALSE;

    return TRUE;
}


// --------------------------------------------------------------------------
//
//  InitWindowClasses()
//
//  Adds a whole lot of classes into the Global atom table for comparison
//  purposes.
//
// --------------------------------------------------------------------------
void InitWindowClasses(void)
{
    int     istr;
    TCHAR   szClassName[128];

    for (istr = 0; istr < CSTR_CLIENT_CLASSES; istr++)
    {
        if( rgClassNames[ istr ] == NULL )
        {
            rgAtomClasses[istr] = NULL;
        }
        else
        {
            rgAtomClasses[istr] = GlobalAddAtom( rgClassNames[ istr ] );
        }
    }

	//-----------------------------------------------------------------
	// [v-jaycl, 4/2/97] Retrieve info for registered handlers from 
	//	 registry and add to global atom table. 
	//	 TODO: remove hard-wired strings.
	//-----------------------------------------------------------------

	const TCHAR  szRegHandlers[]   = TEXT("SOFTWARE\\Microsoft\\Active Accessibility\\Handlers");
	TCHAR		 szHandler[255], szHandlerClassKey[255];
    LONG		 lRetVal, lBuffSize;
	HKEY		 hKey;


	lRetVal = RegOpenKey( HKEY_LOCAL_MACHINE, szRegHandlers, &hKey );

	if ( lRetVal != ERROR_SUCCESS )
		return;

	for ( istr = 0; istr < TOTAL_REG_HANDLERS; istr++ )
	{
		lRetVal = RegEnumKey( hKey, istr, szHandler, sizeof(szHandler)/sizeof(TCHAR));

		if ( lRetVal != ERROR_SUCCESS ) 
			break;		

		//-----------------------------------------------------------------
		// [v-jaycl, 4/2/97] Translate string into CLSID, then get info
		//	 on specific handler from HKEY_CLASSES_ROOT\CLSID  subkey.
		//-----------------------------------------------------------------

		//-----------------------------------------------------------------
		//	Get proxied window class name from 
		//	HKEY_CLASSES_ROOT\CLSID\{clsid}\AccClassName
		//-----------------------------------------------------------------

		lstrcpy( szHandlerClassKey, TEXT("CLSID\\"));
		lstrcat( szHandlerClassKey, szHandler );
		lstrcat( szHandlerClassKey, TEXT("\\AccClassName"));

		lBuffSize = sizeof(szClassName)/sizeof(TCHAR);
		lRetVal	= RegQueryValue( HKEY_CLASSES_ROOT, szHandlerClassKey, szClassName, &lBuffSize );

		if ( lRetVal == ERROR_SUCCESS )
		{

			//-------------------------------------------------------------
			// Add CLSID to registered types table and associated class
			//	name to global atom table and class types table.
			//-------------------------------------------------------------

#ifdef UNICODE
			
			if ( CLSIDFromString( szHandler, &rgRegisteredTypes[istr] ) == NOERROR )
			{
				rgAtomClasses[istr + CSTR_CLIENT_CLASSES] = GlobalAddAtom( szClassName );
			}

#else

			OLECHAR wszString[MAX_PATH];
			MultiByteToWideChar(CP_ACP, 0, szHandler, -1, wszString, ARRAYSIZE(wszString));

			if ( CLSIDFromString( wszString, &rgRegisteredTypes[istr] ) == NOERROR )
			{
				rgAtomClasses[istr + CSTR_CLIENT_CLASSES] = GlobalAddAtom( szClassName );
			}

#endif

		}
	}

	RegCloseKey( hKey );

	return;
}


// --------------------------------------------------------------------------
//
//  UnInitWindowClasses()
//
//  Cleans up the Global Atom Table.
//
// --------------------------------------------------------------------------
void UnInitWindowClasses(void)
{
    int     istr;

	//-----------------------------------------------------------------
	// [v-jaycl, 4/2/97] Clean up registered handler atoms after
	//  class and window atoms have been removed.
	//-----------------------------------------------------------------

    for (istr = 0; istr < CSTR_CLIENT_CLASSES + TOTAL_REG_HANDLERS; istr++)
    {
        if (rgAtomClasses[istr])
            GlobalDeleteAtom (rgAtomClasses[istr]);
    }
}


// --------------------------------------------------------------------------
//
//  FindWindowClass()
// - has been replaced by:
//
//  CompareWindowClass
//  FindAndCreateWindowClass
//  LookupWindowClass
//  LookupWindowClassName
//
//  See comments on each function for more infotmation.
//
// --------------------------------------------------------------------------



// --------------------------------------------------------------------------
//
//  CompareWindowClass()
//
//  Checks if a window matches the class of the creation function passed
//  in. Eg. CompareWindowClass( hWnd, CreateButton ) would return TRUE
//  if hWnd was a Button window.
//
//  Paremeters:
//		hwnd        The window handle we are checking
//		pfnQuery    Class creation function (effectively representing that
//                  class a class) to compare against
//
//	Returns:
//		TRUE if the window matches the class represented by the creation
//		function.
//
// --------------------------------------------------------------------------

BOOL CompareWindowClass( HWND hWnd, LPFNCREATE pfnQuery )
{
    int RegHandlerIndex;
    LPFNCREATE pfnCreate;

    // fWindow param is FALSE - only interested in client classes...
    if( ! LookupWindowClass( hWnd, FALSE, & pfnCreate, & RegHandlerIndex ) )
        return FALSE;

    // pfnCreate == NULL means it's a registered handler - not interested
    // in those here...
    if( ! pfnCreate )
        return FALSE;

    return pfnCreate == pfnQuery;
}


// --------------------------------------------------------------------------
//
//  FindAndCreateWindowClass()
//
//  Create an object of the appropriate class for given window.
//  If no suitable class found, use the default object creation given,
//  if any.
//  
//  Paremeters:
//		hwnd	    Handle of window to create object to represent/proxy
//		fWindow	    TRUE if we're interested in a window (as opposed to
//                  client) -type class
//      pfnDefault  Function to use to create object if no suitable class
//                  found.
//      riid        Interface to pass to object creation function
//      idObject    object id to pass to object creation function
//      ppvObject   Object is returned through this
//
//	Returns:
//		HRESULT resulting from object creation.
//      S_OK or other success value on success,
//      failure value on failure (surprise surprise!)
//
//      If no suitable class found, and no default creation function
//      supplied, returns E_FAIL.
//      (Note that return of E_FAIL doesn't necessarilly mean no suitable
//      class found, since it can be returned for other reasons - eg.
//      error during creation of object.)
//
// --------------------------------------------------------------------------

HRESULT FindAndCreateWindowClass( HWND        hWnd,
                                  BOOL        fWindow,
                                  LPFNCREATE  pfnDefault,
                                  REFIID      riid,
                                  long        idObject,
                                  void **     ppvObject )
{
    int RegHandlerIndex;
    LPFNCREATE pfnCreate;

    // Try and find a native proxy or registered handler for this window/client...
    if( ! LookupWindowClass( hWnd, fWindow, & pfnCreate, & RegHandlerIndex ) )
    {
        // Unknown class - do we have a default fn to use instead?
        if( pfnDefault )
        {
            // Yup - use it...
            pfnCreate = pfnDefault;
        }
        else
        {
            // Nope - fail!
            ppvObject = NULL;
            return E_FAIL;
        }
    }

    // At this point, lpfnCreate != NULL means we've either found a class above,
    // or we're using the supplied default.
    // lpfnCreate == NULL means it's a registered handler class, using index
    // RegHandlerIndex...

    // Now create the object...
    if( pfnCreate )
    {
        return pfnCreate( hWnd, idObject, riid, ppvObject );
    }
    else
    {
        return CreateRegisteredHandler( hWnd, idObject, RegHandlerIndex, riid, ppvObject );
    }
}


// --------------------------------------------------------------------------
//
//  LookupWindowClass()
//
//  Tries to find an internal proxy or a registered handler for the
//  window, based on class name.
//
//  If no suitable match found, it sends the window a WM_GETOBJECT
//  message with OBJID_QUERYCLASSNAMEIDX - window can respond to
//  indicate its real name. If so, a class name match is tried on
//  that new name.
//
//  If that fails, or the window doesn't respond to the QUERY message,
//  FALSE is returned.
//
//  Paremeters:
//		hwnd	        The window handle we are checking
//		fWindow	        This is true if...
//      ppfnCreate      ptr to value to receive result ptr to creation function
//      pRegHandlerIndex    ptr to value to receive reg.handler index
//
//	Returns:
//      Returns TRUE if match found, FALSE if none found.
//
//      When TRUE returned:
//      If internal proxy found, *ppfnCreate points to function to
//      call to create the proxy.
//      If reg handler found, *ppfnCreate is set to NULL, and
//      *pRegHandlerIndex is set to a value that can be passed to
//      CreateRegisteredHandler to create a suitable object.
//
// --------------------------------------------------------------------------

BOOL LookupWindowClass( HWND          hWnd,
                        BOOL          fWindow,
                        LPFNCREATE *  ppfnCreate,
                        int *         pRegHandlerIndex )
{
    TCHAR   szClassName[128];

    //  This works by looking at the class name.  It uses a private function in
    //  USER to get the "real" class name, so that we see superclassed controls
    //	like VB's 'ThunderButton' as a button. (This only works for USER controls,
    //  though...)
    if( ! MyGetWindowClass( hWnd, szClassName, ARRAYSIZE( szClassName ) ) )
        return NULL;

    // First do lookup on 'apparent' class name - this allows us to reg-handler
    // even subclassed comctrls...
    if( LookupWindowClassName( szClassName, fWindow, ppfnCreate, pRegHandlerIndex ) )
    {
        // Found a match for the (possibly wrapped) class name - use it...
        return TRUE;
    }

    // Try sending a WM_GETOBJECT / OBJID_QUERYCLASSNAMEIDX...
    LPTSTR pClassName = szClassName;
    DWORD_PTR ref = 0;
    SendMessageTimeout( hWnd, WM_GETOBJECT, 0, OBJID_QUERYCLASSNAMEIDX,
                            SMTO_ABORTIFHUNG, 10000, &ref );

    if( ! ref )
    {
        // No response - no match found, then, so return FALSE...
        return FALSE;
    }

    // Valid / in-range response?
    // (Remember, that we go from base..base+numclasses-1 instead of
    // 0..numclasses-1 to avoid running afoul of Notes and other apps
    // that return small LRESULTS to WM_GETOBJECT...)
    if( ref >= CSTR_QUERYCLASSNAME_BASE &&
         ref - CSTR_QUERYCLASSNAME_BASE < CSTR_QUERYCLASSNAME_CLASSES )
    {
        // Yup - valid:
        pClassName = rgClassNames[ ref - CSTR_QUERYCLASSNAME_BASE ];

        if( ! pClassName )
        {
            DBPRINTF( TEXT("Warning: reply to OBJID_QUERYCLASSNAMEIDX refers to unsupported class") );
            return FALSE;
        }

        // Now try again, using 'real' COMCTRL class name.
        return LookupWindowClassName( pClassName, fWindow, ppfnCreate, pRegHandlerIndex );
    }
    else
    {
        DBPRINTF( TEXT("Warning: out-of-range reply to OBJID_QUERYCLASSNAMEIDX received") );
        return FALSE; // TODO - add debug output
    }
}



// --------------------------------------------------------------------------
//
//  LookupWindowClassName()
//
//  Tries to find an internal proxy or a registered handler for the
//  window, based on class name.
//
//  Does so by converting class name to an 'atom', and looking through
//  our reg handler and proxy tables.
//
//  Paremeters:
//		pClassName          name of class to lookup
//		fWindow	            This is true if...
//      ppfnCreate          ptr to value to receive result ptr to creation function
//      pRegHandlerIndex    ptr to value to receive reg.handler index
//
//	Returns:
//      Returns TRUE if match found, FALSE if none found.
//
//      When TRUE returned:
//      If internal proxy found, *ppfnCreate points to function to
//      call to create the proxy.
//      If reg handler found, *ppfnCreate is set to NULL, and
//      *pRegHandlerIndex is set to a value that can be passed to
//      CreateRegisteredHandler to create a suitable object.
//
// --------------------------------------------------------------------------


BOOL LookupWindowClassName( LPCTSTR       pClassName,
                            BOOL          fWindow,
                            LPFNCREATE *  ppfnCreate,
                            int *         pRegHandlerIndex )
{
    // Get atom from classname - use it to lookup name in registered and
    // internal proxy tables...
    ATOM atom = GlobalFindAtom( pClassName );
    if( ! atom )
        return FALSE;

    // Search registered handler table first...
    int istr;
    for( istr = CSTR_CLIENT_CLASSES ; istr < CSTR_CLIENT_CLASSES + TOTAL_REG_HANDLERS ; istr++ )
	{
		if( rgAtomClasses[ istr ] == atom )
		{
			*pRegHandlerIndex = istr - CSTR_CLIENT_CLASSES;
            *ppfnCreate = NULL;
            return TRUE;
		}
	}

    // Search internal proxy client/window table...
    int cstr = fWindow ? CSTR_WINDOW_CLASSES : CSTR_CLIENT_CLASSES;

    for( istr = 0; istr < cstr ; istr++ )
    {
        if( rgAtomClasses[ istr ] == atom )
		{
            *ppfnCreate = fWindow ? rgWindowTypes[ istr ] : rgClientTypes[ istr ];
            // Only want to return TRUE if the fn is actually non-NULL...
            return *ppfnCreate != NULL;
		}
    }

    return FALSE;
}





// --------------------------------------------------------------------------
//
//  CreateRegisteredHandler()
//
//  This function takes an HWND, OBJID, RIID, and a PPVOID, same as the
//  other CreateXXX functions (like CreateButtonClient, etc.) This function
//  is used by calling FindWindowClass, which sees if a registered handler 
//  for the window class of HWND is installed. If so, it sets a global variable
//  s_iHandlerIndex that is an index into the global rgRegisteredTypes
//  array. That array contains CLSID's that are used to call CoCreateInstance,
//  to create an instance of an object that supports the interface
//  IAccessibleHandler. 
//  After creating this object, this function calls the object's
//  AccesibleObjectFromId method, using the HWND and the OBJID, and filling 
//  in the PPVOID to be an IAccessible interface.
//
//  [v-jaycl, 4/2/97] Special function returned by FindWindowClass()
//	for creating registered handlers.  
//
//  [v-jaycl, 5/15/97] Renamed second parameter from idChildCur to idObject
//	because I believe that what the parameter really is, or at least how I
//	intend to use it.
//
//  [v-jaycl, 8/7/97] Changed logic such that we now get an accessible
//	factory pointer back from CoCreateInstance() which supports
//	IAccessibleHandler. This interface provides the means for getting an
//	IAccessible ptr from a HWND/OBJID pair.
//	NOTE: To support any number of IIDs requested from the caller, we 
//	try QIing on the caller-specified riid parameter if our explicit QI on
//	IID_IAccessibleHandler fails.
// 
//  [BrendanM, 9/4/98]
//  Index now passed by parameter, so global var and mutex no longer needed.
//  Called by FindAndCreateWindowClass and CreateStdAccessibleProxyA.
//
//---------------------------------------------------------------------------

HRESULT CreateRegisteredHandler( HWND      hwnd,
                                 long      idObject,
                                 int       iHandlerIndex,
                                 REFIID    riid,
                                 LPVOID *  ppvObject )
{
    HRESULT		hr;
	LPVOID		pv;

	//------------------------------------------------------------
	// TODO: optimize by caching the proxy's object factory pointer.
	//	CoCreateInstance() only needs to be called once per
	//	proxy, not for each request for an object within a proxy.
	//	For satisfying the caller-specific IID, we can just QI
	//	on the cached object factory pointer.
	//------------------------------------------------------------


	//------------------------------------------------------------
	// First QI on IAccessibleHandler directly to retrieve
	// a pointer to the proxy object factory that
	// manufactures accessible objects from object IDs.
	//------------------------------------------------------------

	hr = CoCreateInstance( rgRegisteredTypes[ iHandlerIndex ], 
                           NULL, 
                           CLSCTX_INPROC_SERVER, 
                           IID_IAccessibleHandler, 
                           &pv );

	if ( SUCCEEDED( hr ) )
	{
		//------------------------------------------------------------
		// We must have a qualified proxy since it supports 
		//	IAccessibleHandler, so get the accessible object.
		//------------------------------------------------------------
#ifndef _WIN64
		hr = ((LPACCESSIBLEHANDLER)pv)->AccessibleObjectFromID( (UINT_PTR)hwnd, 
                                                                idObject, 
                                                                (LPACCESSIBLE *)ppvObject );
#else // _WIN64
        hr = E_NOTIMPL;
#endif // _WIN64
		((LPACCESSIBLEHANDLER)pv)->Release();
	}
	else
	{
		//------------------------------------------------------------
		// Else try using the caller-specific IID
		//------------------------------------------------------------

		hr = CoCreateInstance( rgRegisteredTypes[ iHandlerIndex ], 
                               NULL, 
                               CLSCTX_INPROC_SERVER, 
                               riid, 
                               ppvObject );
	}


    return hr;
}



// --------------------------------------------------------------------------
//
//  CreateStdAccessibleProxyA()
//
//  See Also: CreateStdAccessibleObject() in Api.cpp
//
//  Similar to CreateStdAccessibleObject, but this version allows you to
//  give a classname to use to specify the type of proxy you want - 
//  eg. "Button" for a button proxy, and so on.
//
//  This function takes a class name and an OBJID.  If the OBJID is one of the
//  system reserved IDs (OBJID_WINDOW, OBJID_CURSOR, OBJID_MENU, etc.)
//  we create a default object that implements the interface whose IID we
//  ask for. This is usually IAccessible, but might also be IDispatch, IText,
//  IEnumVARIANT...
//
//  If ObjID is 
//
//  (Why is this in Default.cpp and CreateStdAccessibleObject in
//  API.cpp? Well, despite the name similarities, this is a lot
//  closer to FindAndCreateWindowClass() - which is a few functions
//  above. Actually, that and this should both be moved to api.cpp
//  eventually, but for the moment...)
//
// --------------------------------------------------------------------------

#ifdef UNICODE

STDAPI
CreateStdAccessibleProxyW( HWND     hWnd,
                           LPCWSTR  pClassName, // UNICODE, not TCHAR
                           LONG     idObject,
                           REFIID   riid,
                           void **  ppvObject )

#else

STDAPI
CreateStdAccessibleProxyA( HWND     hWnd,
                           LPCSTR   pClassName, // ANSI, not TCHAR
                           LONG     idObject,
                           REFIID   riid,
                           void **  ppvObject )

#endif

{
    int RegHandlerIndex;
    LPFNCREATE pfnCreate;

    // Try and find a native proxy or registered handler for this window/client...
    if( ! LookupWindowClassName( pClassName, FALSE, & pfnCreate, & RegHandlerIndex ) )
    {
        // Nope - fail!
        ppvObject = NULL;
        return E_FAIL;
    }

    // At this point, lpfnCreate != NULL means we've found a class above,
    // lpfnCreate == NULL means it's a registered handler class, using index
    // RegHandlerIndex...

    // Now create the object...
    if( pfnCreate )
    {
        return pfnCreate( hWnd, idObject, riid, ppvObject );
    }
    else
    {
        return CreateRegisteredHandler( hWnd, idObject, RegHandlerIndex, riid, ppvObject );
    }
}



// --------------------------------------------------------------------------
//
//  CreateStdAccessibleProxyW()
//
//  See Also: CreateStdAccessibleObject() in Api.cpp
//
//  UNICODE wrapper for CreateStdAccessibleProxyA - see that
//  for details of params/return values.
//
// --------------------------------------------------------------------------

#ifdef UNICODE

STDAPI
CreateStdAccessibleProxyA( HWND     hWnd,
                           LPCSTR   pClassName, // ANSI, not TCHAR
                           LONG     idObject,
                           REFIID   riid,
                           void **  ppvObject )
{
    WCHAR szClassNameW[ 256 ];

    if( ! MultiByteToWideChar( CP_ACP, 0, pClassName, -1, szClassNameW,
								ARRAYSIZE( szClassNameW ) ) )
        return E_FAIL;

    return CreateStdAccessibleProxyW( hWnd, szClassNameW, idObject, riid, ppvObject );
}

#else

STDAPI
CreateStdAccessibleProxyW( HWND     hWnd,
                           LPCWSTR  pClassName, // UNICODE, not TCHAR
                           LONG     idObject,
                           REFIID   riid,
                           void **  ppvObject )
{
    CHAR szClassNameA[ 256 ];

    if( ! WideCharToMultiByte( CP_ACP, 0, pClassName, -1, szClassNameA,
                    ARRAYSIZE( szClassNameA ), NULL, NULL ) )
        return E_FAIL;

    return CreateStdAccessibleProxyA( hWnd, szClassNameA, idObject, riid, ppvObject );
}

#endif


// --------------------------------------------------------------------------
//
// This function takes a pointer to a rectangle that contains coordinates
// in the form (top,left) (width,height). These are screen coordinates. It
// then finds the center of that rectangle and checks that the window handle
// given is in fact the window at that point. If so, it uses the SendInput
// function to move the mouse to the center of the rectangle, do a single
// click of the default button, and then move the cursor back where it
// started. In order to be super-robust, it checks the Async state of the 
// shift keys (Shift, Ctrl, and Alt) and turns them off while doing the 
// click, then back on if they were on. if fDblClick is TRUE, it will do
// a double click instead of a single click.
//
// We have to make sure we are not interrupted while doing this!
//
// Returns TRUE if it did it, FALSE if there was some bad error.
//
// --------------------------------------------------------------------------
BOOL ClickOnTheRect(LPRECT lprcLoc,HWND hwndToCheck,BOOL fDblClick)
{
POINT		ptCursor;
POINT		ptClick;
HWND		hwndAtPoint;
MOUSEINFO	miSave;
MOUSEINFO   miNew;
int			nButtons;
INPUT		rgInput[6];
int         i;
DWORD		dwMouseDown;
DWORD		dwMouseUp;
#ifdef THIS_DOESNT_WORK
BOOL        fCtrlPressed,
            fAltPressed,
            fShiftPressed;
#endif

    // Find Center of rect
	ptClick.x = lprcLoc->left + (lprcLoc->right/2);
	ptClick.y = lprcLoc->top + (lprcLoc->bottom/2);

	// check if hwnd at point is same as hwnd to check
	hwndAtPoint = WindowFromPoint (ptClick);
	if (hwndAtPoint != hwndToCheck)
		return FALSE;

    MyBlockInput (TRUE);
    // Get current cursor pos.
    GetCursorPos(&ptCursor);
	if (GetSystemMetrics(SM_SWAPBUTTON))
	{
		dwMouseDown = MOUSEEVENTF_RIGHTDOWN;
		dwMouseUp = MOUSEEVENTF_RIGHTUP;
	}
	else
	{
		dwMouseDown = MOUSEEVENTF_LEFTDOWN;
		dwMouseUp = MOUSEEVENTF_LEFTUP;
	}

    // Get delta to move to center of rectangle from current
    // cursor location.
    ptCursor.x = ptClick.x - ptCursor.x;
    ptCursor.y = ptClick.y - ptCursor.y;

    // NOTE:  For relative moves, USER actually multiplies the
    // coords by any acceleration.  But accounting for it is too
    // hard and wrap around stuff is weird.  So, temporarily turn
    // acceleration off; then turn it back on after playback.

    // Save mouse acceleration info
    if (!SystemParametersInfo(SPI_GETMOUSE, 0, &miSave, 0))
    {
        MyBlockInput (FALSE);
        return (FALSE);
    }

    if (miSave.MouseSpeed)
    {
        miNew.MouseThresh1 = 0;
        miNew.MouseThresh2 = 0;
        miNew.MouseSpeed = 0;

        if (!SystemParametersInfo(SPI_SETMOUSE, 0, &miNew, 0))
        {
            MyBlockInput (FALSE);
            return (FALSE);
        }
    }

    // Get # of buttons
    nButtons = GetSystemMetrics(SM_CMOUSEBUTTONS);

    // BUGBUG!
    // Get state of shift keys and if they are down, send an up
    //
    // MOREBUGBUG
    // Putting this code in caused the shift keys to be in an 
    // indeteriminate state at the end of the function. This
    // is too hard to fix correctly, so we are going to bail.

#ifdef THIS_DOESNT_WORK
    fCtrlPressed = GetKeyState(VK_CONTROL);
    fAltPressed = GetKeyState(VK_MENU);
    fShiftPressed = GetKeyState(VK_SHIFT);
    if (fCtrlPressed)
        SendKey (KEYRELEASE,VK_VIRTUAL,VK_CONTROL,0);
    if (fAltPressed)
        SendKey (KEYRELEASE,VK_VIRTUAL,VK_MENU,0);
    if (fShiftPressed)
        SendKey (KEYRELEASE,VK_VIRTUAL,VK_SHIFT,0);
#endif

    DWORD time = GetTickCount();

    // mouse move to center of start button
    rgInput[0].type = INPUT_MOUSE;
    rgInput[0].mi.dwFlags = MOUSEEVENTF_MOVE;
    rgInput[0].mi.dwExtraInfo = 0;
    rgInput[0].mi.dx = ptCursor.x;
    rgInput[0].mi.dy = ptCursor.y;
    rgInput[0].mi.mouseData = nButtons;
    rgInput[0].mi.time = time;

    i = 1;

DBL_CLICK:
    // Mouse click down, left button
    rgInput[i].type = INPUT_MOUSE;
    rgInput[i].mi.dwFlags = dwMouseDown;
    rgInput[i].mi.dwExtraInfo = 0;
    rgInput[i].mi.dx = 0;
    rgInput[i].mi.dy = 0;
    rgInput[i].mi.mouseData = nButtons;
    rgInput[i].mi.time = time;

    i++;
    // Mouse click up, left button
    rgInput[i].type = INPUT_MOUSE;
    rgInput[i].mi.dwFlags = dwMouseUp;
    rgInput[i].mi.dwExtraInfo = 0;
    rgInput[i].mi.dx = 0;
    rgInput[i].mi.dy = 0;
    rgInput[i].mi.mouseData = nButtons;
    rgInput[i].mi.time = time;

    i++;
    if (fDblClick)
    {
        fDblClick = FALSE;
        goto DBL_CLICK;
    }
	// move mouse back to starting location
    rgInput[i].type = INPUT_MOUSE;
    rgInput[i].mi.dwFlags = MOUSEEVENTF_MOVE;
    rgInput[i].mi.dwExtraInfo = 0;
    rgInput[i].mi.dx = -ptCursor.x;
    rgInput[i].mi.dy = -ptCursor.y;
    rgInput[i].mi.mouseData = nButtons;
    rgInput[i].mi.time = time;

    i++;
    if (!MySendInput(i, rgInput,sizeof(INPUT)))
        MessageBeep(0);

    // BUGBUG!
    // send shift key down events if they were down before
#ifdef THIS_DOESNT_WORK
    if (fCtrlPressed)
        SendKey (KEYPRESS,VK_VIRTUAL,VK_CONTROL,0);
    if (fAltPressed)
        SendKey (KEYPRESS,VK_VIRTUAL,VK_MENU,0);
    if (fShiftPressed)
        SendKey (KEYPRESS,VK_VIRTUAL,VK_SHIFT,0);
#endif

    //
    // Restore Mouse Acceleration
    //
    if (miSave.MouseSpeed)
        SystemParametersInfo(SPI_SETMOUSE, 0, &miSave, 0);

    MyBlockInput (FALSE);

	return TRUE;
}

//--------------------------------------------------------------------------
// This is a private function. Sends the key event specified by 
// the parameters - down or up, plus a virtual key code or character. 
//
// Parameters:
//  nEvent          either KEYPRESS or KEYRELEASE
//  nKeyType        either VK_VIRTUAL or VK_CHAR
//  wKeyCode        a Virtual Key code if KeyType is VK_VIRTUAL,
//                  ignored otherwise
//  cChar           a Character if KeyType is VK_CHAR, ignored otherwise.
//
// Returns:
//  BOOL indicating success (TRUE) or failure (FALSE)
//--------------------------------------------------------------------------
BOOL SendKey (int nEvent,int nKeyType,WORD wKeyCode,TCHAR cChar)
{
INPUT		Input;

    Input.type = INPUT_KEYBOARD;
    if (nKeyType == VK_VIRTUAL)
    {
        Input.ki.wVk = wKeyCode;
        Input.ki.wScan = LOWORD(MapVirtualKey(wKeyCode,0));
    }
    else // must be a character
    {
        Input.ki.wVk = VkKeyScan (cChar);
        Input.ki.wScan = LOWORD(OemKeyScan (cChar));
    }
    Input.ki.dwFlags = nEvent;
    Input.ki.time = GetTickCount();;
    Input.ki.dwExtraInfo = 0;

    return MySendInput(1, &Input,sizeof(INPUT));
}

#ifdef _DEBUG
//--------------------------------------------------------------------------
//
// This new PrintIt function takes the arguments and then uses wvsprintf to
// format the string into a buffer. It then uses OutputDebugString to show 
// the string.
//
//--------------------------------------------------------------------------
void FAR CDECL PrintIt(LPTSTR strFmt, ...)
{
static TCHAR	StringBuf[4096] = {0};
static int  len;
va_list		marker;

	va_start(marker, strFmt);
	len = wvsprintf (StringBuf,strFmt,marker);
	if (len > 0)
		OutputDebugString (StringBuf);
	return;
}

#endif // _DEBUG

//
// These three arrays (rgAtomClasses, rgClientTypes, and rgWindowTypes)
// are used by the FindWindowClass function above. 
//
// The rgAtomClass array is filled in by the InitWindowClasses function.
// InitWindowClasses iterates through the resources, loading strings that 
// are the names of the window classes that we recognize, putting them in 
// the Global Atom Table, and putting the Atom numbers in rgAtomClass so
// that rgAtomClass[StringN] = GlobalAddAtom ("StringTable[StringN]")
//
// When FindWindowClass is called, it gets the "real" class name of the 
// window, does a GlobalFindAtom of that string, then walks though 
// rgAtomClasses to see if the atom is in the table. If it is, then we 
// use the index where we found the atom to index into either the 
// rgClientTypes or rgWindowTypes array, where a pointer to the object 
// creation function is stored. These two arrays are static arrays
// initialized below. The elements in the array must correspond to the
// elements in the string table. 
//
// The rgClientTypes array is where most of the classes are. Currently,
// all types of controls that we create have a parent control that is
// a CWindow object, except for Dropdowns and menu popups, which provide 
// a window handler as well since they do something nonstandard for 
// get_accParent().  The former returns the combobox it is in, the 
// latter returns the menu item it comes from.
//

//
// NB - ordering of these should be considered fixed - as an offset
// referring to Listbox through RichEdit20W can be returned in response
// to WM_GETOBJECT/OBJID_QUERYCLASSINDEX.
// (The index is currently used to index directly into this table,
// but if the table order must change, another mapping table can
// be created and it used instead.)
//
//

//-----------------------------------------------------------------
// [v-jaycl, 4/1/97] Grow to accomodate registered handlers. 
//	TODO: Kludge!  Make this dynamic.
//-----------------------------------------------------------------
ATOM    rgAtomClasses[CSTR_CLIENT_CLASSES + TOTAL_REG_HANDLERS] = {0};

LPFNCREATE rgClientTypes[CSTR_CLIENT_CLASSES] =
{
    CreateListBoxClient,
    CreateMenuPopupClient,
    CreateButtonClient,
    CreateStaticClient,
    CreateEditClient,
    CreateComboClient,
    CreateDialogClient,
    CreateSwitchClient,
    CreateMDIClient,
    CreateDesktopClient,
    CreateScrollBarClient,
    CreateStatusBarClient,
    CreateToolBarClient,
    CreateProgressBarClient,
    CreateAnimatedClient,
    CreateTabControlClient,
    CreateHotKeyClient,
    CreateHeaderClient,
    CreateSliderClient,
    CreateListViewClient,
	CreateListViewClient,
    CreateUpDownClient,     // msctls_updown
    CreateUpDownClient,     // msctls_updown32
    CreateToolTipsClient,   // tooltips_class
    CreateToolTipsClient,   // tooltips_class32
    CreateTreeViewClient,
    NULL,                   // SysMonthCal32
    NULL,                   // SysDateTimePick32
#ifdef _X86_
    CreateHtmlClient,       // HTML_InternetExplorer
#else
    NULL,
#endif
    CreateEditClient,       // RichEdit
    CreateEditClient,       // RichEdit20A
    CreateEditClient,       // RichEdit20W
    CreateSdmClientA,       // Word '95 #1
    CreateSdmClientA,       // Word '95 #2
    CreateSdmClientA,       // Word '95 #3
    CreateSdmClientA,       // Word '95 #4
    CreateSdmClientA,       // Word '95 #5
    CreateSdmClientA,       // Excel '95 #1
    CreateSdmClientA,       // Excel '95 #2
    CreateSdmClientA,       // Excel '95 #3
    CreateSdmClientA,       // Excel '95 #4
    CreateSdmClientA,       // Excel '95 #5
    CreateSdmClientA,       // Word '97 #1
    CreateSdmClientA,       // Word '97 #2
    CreateSdmClientA,       // Word '97 #3
    CreateSdmClientA,       // Word '97 #4
    CreateSdmClientA,       // Word '97 #5
    CreateSdmClientA,       // Word 3.1 #1
    CreateSdmClientA,       // Word 3.1 #2
    CreateSdmClientA,       // Word 3.1 #3
    CreateSdmClientA,       // Word 3.1 #4
    CreateSdmClientA,       // Word 3.1 #5
    CreateSdmClientA,       // Office '97 #1
    CreateSdmClientA,       // Office '97 #2
    CreateSdmClientA,       // Office '97 #3
    CreateSdmClientA,       // Office '97 #4
    CreateSdmClientA,       // Office '97 #5
    CreateSdmClientA,       // Excel '97 #1
    CreateSdmClientA,       // Excel '97 #2
    CreateSdmClientA,       // Excel '97 #3
    CreateSdmClientA,       // Excel '97 #4
    CreateSdmClientA        // Excel '97 #5
};


LPFNCREATE rgWindowTypes[CSTR_WINDOW_CLASSES] =
{
    CreateListBoxWindow,
    CreateMenuPopupWindow
};


LPTSTR rgClassNames[CSTR_CLIENT_CLASSES] =
{
	TEXT( "ListBox" ),
	TEXT( "#32768" ),
	TEXT( "Button" ),
	TEXT( "Static" ),
	TEXT( "Edit" ),
	TEXT( "ComboBox" ),
	TEXT( "#32770" ),
	TEXT( "#32771" ),
	TEXT( "MDIClient" ),
	TEXT( "#32769" ),
	TEXT( "ScrollBar" ),
	TEXT( "msctls_statusbar32" ),
	TEXT( "ToolbarWindow32" ),
	TEXT( "msctls_progress32" ),
	TEXT( "SysAnimate32" ),
	TEXT( "SysTabControl32" ),
	TEXT( "msctls_hotkey32" ),
	TEXT( "SysHeader32" ),
	TEXT( "msctls_trackbar32" ),
	TEXT( "SysListView32" ),
	TEXT( "OpenListView" ),
	TEXT( "msctls_updown" ),
	TEXT( "msctls_updown32" ),
	TEXT( "tooltips_class" ),
	TEXT( "tooltips_class32" ),
	TEXT( "SysTreeView32" ),
	TEXT( "SysMonthCal32" ),
	TEXT( "SysDateTimePick32" ),
#ifdef _X86_
	TEXT( "HTML_Internet Explorer" ),
#else
    NULL,
#endif
	TEXT( "RICHEDIT" ),
	TEXT( "RichEdit20A" ),
	TEXT( "RichEdit20W" ),

// The above CSTR_QUERYCLASSNAME_CLASSES classes can be referred
// to by the WM_GETOBJECT/OBJID_QUERYCLASSNAMEIDX message.
// See LookupWindowClassName() for more details.

	TEXT( "bosa_sdm_Microsoft Word for Windows 95" ),
	TEXT( "osa_sdm_Microsoft Word for Windows 95" ),
	TEXT( "sa_sdm_Microsoft Word for Windows 95" ),
	TEXT( "a_sdm_Microsoft Word for Windows 95" ),
	TEXT( "_sdm_Microsoft Word for Windows 95" ),
	TEXT( "bosa_sdm_XL" ),
	TEXT( "osa_sdm_XL" ),
	TEXT( "sa_sdm_XL" ),
	TEXT( "a_sdm_XL" ),
	TEXT( "_sdm_XL" ),
	TEXT( "bosa_sdm_Microsoft Word 8.0" ),
	TEXT( "osa_sdm_Microsoft Word 8.0" ),
	TEXT( "sa_sdm_Microsoft Word 8.0" ),
	TEXT( "a_sdm_Microsoft Word 8.0" ),
	TEXT( "_sdm_Microsoft Word 8.0" ),
	TEXT( "bosa_sdm_Microsoft Word 6.0" ),
	TEXT( "osa_sdm_Microsoft Word 6.0" ),
	TEXT( "sa_sdm_Microsoft Word 6.0" ),
	TEXT( "a_sdm_Microsoft Word 6.0" ),
	TEXT( "_sdm_Microsoft Word 6.0" ),
	TEXT( "bosa_sdm_Mso96" ),
	TEXT( "osa_sdm_Mso96" ),
	TEXT( "sa_sdm_Mso96" ),
	TEXT( "a_sdm_Mso96" ),
	TEXT( "_sdm_Mso96" ),
	TEXT( "bosa_sdm_XL8" ),
	TEXT( "osa_sdm_XL8" ),
	TEXT( "sa_sdm_XL8" ),
	TEXT( "a_sdm_XL8" ),
	TEXT( "_sdm_XL8" )
};
