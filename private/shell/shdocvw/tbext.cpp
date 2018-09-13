////////////////////////////////////////////////////////////////////////////
// File:   TBExt.cpp   (toolbar extension classes)
// Author: Karim Farouki
//
// We define here three classes:
// (1) CToolbarExt a base class that takes care of the
//     button work for our custom extensions
// (2) CToolbarExtBand the object which deals with custom
//     buttons that plug into bands
// (3) CToolbarExtExec the object which deals with custom
//     buttons (or tools menu items) that exec stuff.
//
// The latter two are derived from the former 
#include "priv.h"
#include <mshtmcid.h>
#include "tbext.h"


//////////////////////////////
// Class CToolbarExt
//
// This is the base class from which CToolbarExtBand and CToolbarExtExec
// both inherit.  It takes care of all the ToolbarButton specific stuff
// like lazy loading the appropriate icons, and keeping track of the button
// text.

// Constructor / Destructor
//
CToolbarExt::CToolbarExt() : _cRef(1)
{ 
    ASSERT(_hIcon == NULL);
    ASSERT(_hIconSm == NULL);
    ASSERT(_hHotIcon == NULL);
    ASSERT(_hHotIconSm == NULL);
    ASSERT(_bstrButtonText == NULL);
    ASSERT(_bstrToolTip == NULL);
    ASSERT(_hkeyThisExtension == NULL);
    ASSERT(_hkeyCurrentLang == NULL);
    ASSERT(_pisb == NULL);
    
    DllAddRef();
}

// Destructor
//
CToolbarExt::~CToolbarExt() 
{ 
    if (_pisb)
        _pisb->Release();
    
    if (_bstrButtonText)
        SysFreeString(_bstrButtonText);

    if (_bstrToolTip)
        SysFreeString(_bstrToolTip);

    if (_hIcon)
        DestroyIcon(_hIcon);

    if (_hIconSm)
        DestroyIcon(_hIconSm);

    if (_hHotIcon)
        DestroyIcon(_hHotIcon);

    if (_hHotIconSm)
        DestroyIcon(_hHotIconSm);

    if (_hkeyThisExtension)
        RegCloseKey(_hkeyThisExtension);

    if (_hkeyCurrentLang)
        RegCloseKey(_hkeyCurrentLang);

    DllRelease();
}


// IUnknown implementation
//
STDMETHODIMP CToolbarExt::QueryInterface(const IID& iid, void** ppv)
{    
	if (iid == IID_IUnknown)
    	*ppv = static_cast<IBrowserExtension*>(this); 
	else if (iid == IID_IBrowserExtension)
		*ppv = static_cast<IBrowserExtension*>(this);
	else if (iid == IID_IOleCommandTarget)
        *ppv = static_cast<IOleCommandTarget*>(this);
    else if (iid == IID_IObjectWithSite)
        *ppv = static_cast<IObjectWithSite*>(this);
    else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) CToolbarExt::AddRef()
{
	return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CToolbarExt::Release() 
{
	if (InterlockedDecrement(&_cRef) == 0)
	{
		delete this;
		return 0;
	}
	return _cRef;
}

// IBrowserExtension::Init Implementation.  We'll read the ButtonText here but wait on the icons until
// a specific variant of the icon is requested.
STDMETHODIMP CToolbarExt::Init(REFGUID rguid)
{
    HRESULT hr = S_OK;
    LPOLESTR pszGUID;

    if (SUCCEEDED(StringFromCLSID(rguid, &pszGUID)))
    {
        //Open the extension reg key associated with this guid
        WCHAR szKey[MAX_PATH];
        StrCpyN(szKey, TEXT("Software\\Microsoft\\Internet Explorer\\Extensions\\"), ARRAYSIZE(szKey));
        StrCatBuff(szKey, pszGUID, ARRAYSIZE(szKey));

        // We will keep _hkeyThisExtension around... it will be closed in the destructor!
        if (RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_READ, &_hkeyThisExtension) == ERROR_SUCCESS ||
            RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &_hkeyThisExtension) == ERROR_SUCCESS)
        {
            // See if there is a subkey for the current language
            LANGID langid = MLGetUILanguage();
            WCHAR szBuff[MAX_PATH];
            wnsprintf(szBuff, ARRAYSIZE(szBuff), L"Lang%04x", langid);
            RegOpenKeyEx(_hkeyThisExtension, szBuff, 0, KEY_READ, &_hkeyCurrentLang);
            
            // Now get the button text
            _RegReadString(_hkeyThisExtension, TEXT("ButtonText"), &_bstrButtonText);
        }

        CoTaskMemFree(pszGUID);
    }

    if (!_bstrButtonText)
        hr = E_FAIL;

    return hr;
}

//
// Gets the icon closest to the desired size from an .ico file or from the 
// resource in a .dll of .exe file
//
HICON CToolbarExt::_ExtractIcon
(
    LPWSTR pszPath, // file to get icon from
    int resid,      // resource id (0 if unused)
    int cx,         // desired icon width
    int cy          // desired icon height
)
{
    HICON hIcon = NULL;

    WCHAR szPath[MAX_PATH];
    SHExpandEnvironmentStrings(pszPath, szPath, ARRAYSIZE(szPath));

    // If no resource id, assume it's an ico file
    if (resid == 0)
    {
        hIcon = (HICON)LoadImage(0, szPath, IMAGE_ICON, cx, cy, LR_LOADFROMFILE);
    }

    // Otherwise, see if it's a resouce
    if (hIcon == NULL)
    {
        HINSTANCE hInst = LoadLibraryEx(szPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hInst)
        {
            hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(resid), IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR);
            FreeLibrary(hInst);
        }
    }

    return hIcon;
}

//
// Returns the desired icon in pvarProperty
//
HRESULT CToolbarExt::_GetIcon
(
    LPCWSTR pszIcon,            // Name of icon value in registry
    int nWidth,                 // icon width
    int nHeight,                // icon height
    HICON& rhIcon,              // location to cached icon
    VARIANTARG * pvarProperty   // used for return icon
)
{
    HRESULT hr = S_OK;
    if (pvarProperty)
    {
        if (rhIcon == NULL)
        {
            BSTR bstrIconName;
            if (_RegReadString(_hkeyThisExtension, pszIcon, &bstrIconName, TRUE))
            {
                // Parse entry such as "file.ext,1" to get the icon index
                int nIconIndex = PathParseIconLocation(bstrIconName);

                // If the entry was ",#" then it's an index into our built-in button bitmap
                if (*bstrIconName == L'\0')
                {
                    pvarProperty->vt = VT_I4;
                    pvarProperty->lVal = nIconIndex;
                    SysFreeString(bstrIconName);
                    return hr;
                }
                else
                {
                    rhIcon = _ExtractIcon(bstrIconName, nIconIndex, nWidth, nHeight);
                }
                SysFreeString(bstrIconName);
            }
        }
        pvarProperty->vt = VT_BYREF;
        pvarProperty->byref = rhIcon;
    }
    return hr;
}

//
// Implementation of IBrowserExtension::GetProperty().  There are two important points here:
// (1) We are lazy loading the appropriate icons.  This way if the user never goes into small icon
//     mode we never create the images...
// (2) If we are called with a NULL pvarProperty then we must still return S_OK if the iPropID
//     is for a property that we support and E_NOTIMPL if we do not.  This is why the if (pvarProperty)
//     check is done for each case rather tan outside the case block.  This behavior is important
//     for CBrowserExtension::Update() who passes in a NULL pvarProperty but still is trying to determine
//     what kind of extension this is!
//
STDMETHODIMP CToolbarExt::GetProperty(SHORT iPropID, VARIANTARG * pvarProperty)
{
    HRESULT hr = S_OK;
    
    switch (iPropID)
    {
        case TBEX_BUTTONTEXT:
            if (pvarProperty)
            {
                pvarProperty->vt = VT_BSTR;
                pvarProperty->bstrVal = SysAllocString(_bstrButtonText);
                if (NULL == pvarProperty->bstrVal)
                    hr = E_OUTOFMEMORY;
            }
            break;

        case TBEX_GRAYICON:
            hr = _GetIcon(TEXT("Icon"), 20, 20, _hIcon, pvarProperty);
            break;

        case TBEX_GRAYICONSM:
            hr = _GetIcon(TEXT("Icon"), 16, 16, _hIconSm, pvarProperty);
            break;

        case TBEX_HOTICON:
            hr = _GetIcon(TEXT("HotIcon"), 20, 20, _hHotIcon, pvarProperty);
            break;

        case TBEX_HOTICONSM:
            hr = _GetIcon(TEXT("HotIcon"), 16, 16, _hHotIcon, pvarProperty);
            break;

        case TBEX_DEFAULTVISIBLE:
            if (pvarProperty)
            {
                BOOL fVisible = _RegGetBoolValue(L"Default Visible", FALSE);
                pvarProperty->vt = VT_BOOL;
                pvarProperty->boolVal = fVisible ? VARIANT_TRUE : VARIANT_FALSE;
            }
            break;

        default:
            hr = E_NOTIMPL;
    }

    return hr;
}

//
// IOleCommandTarget Implementation
//
STDMETHODIMP CToolbarExt::QueryStatus(const GUID* pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT* pCmdText)
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;
    if (pguidCmdGroup && IsEqualGUID(*pguidCmdGroup, CLSID_ToolbarExtButtons))
    {
        // Default to all commands enabled
        for (ULONG i = 0; i < cCmds; i++)
        {
//            if (prgCmds[i].cmdID == 1)
                // Execing this object is supported and can be done at this point
                rgCmds[i].cmdf = OLECMDF_ENABLED | OLECMDF_SUPPORTED;
//            else
//                prgCmds[i].cmdf = 0;
        }
        hr = S_OK;
    }

    // Return an empty pCmdText
    if (pCmdText != NULL)
    {
        pCmdText->cwActual = 0;
    }
    return hr;
}

//
// IObjectWithSite Implementation
//
STDMETHODIMP CToolbarExt::SetSite(IUnknown* pUnkSite)
{
    if (_pisb != NULL)
    {
        _pisb->Release();
        _pisb = NULL;
    }
    
    if (pUnkSite)
        pUnkSite->QueryInterface(IID_IShellBrowser, (void **)&_pisb);
        
    return S_OK;
}
   
STDMETHODIMP CToolbarExt::GetSite(REFIID riid, void ** ppvSite)
{
    return E_NOTIMPL;
}


BOOL CToolbarExt::_RegGetBoolValue
(
    LPCWSTR         pszPropName,
    BOOL            fDefault
)
{
    WCHAR szData[MAX_PATH];
    DWORD cbData = SIZEOF(szData);

    if ((_hkeyCurrentLang && RegQueryValueEx(_hkeyCurrentLang, pszPropName, NULL, NULL, (unsigned char *)szData, &cbData) == ERROR_SUCCESS) ||
        (_hkeyThisExtension && RegQueryValueEx(_hkeyThisExtension, pszPropName, NULL, NULL, (unsigned char *)szData, &cbData) == ERROR_SUCCESS))
    {
        if ((0 == StrCmpI(L"TRUE", szData)) || 
            (0 == StrCmpI(L"YES", szData)))
        {
            fDefault = TRUE;        // We read TRUE from the registry.
        }
        else if ((0 == StrCmpI(L"FALSE", szData)) || 
            (0 == StrCmpI(L"NO", szData)))
        {
            fDefault = FALSE;        // We read TRUE from the registry.
        }
    }

    return fDefault;
}



// Private Helper Functions
//
// shlwapi has some similar function; however, they all insist on reopening and closing the key in question
// with each read.  It is explicitly suggested that we use our own helper if we are caching the key...
BOOL CToolbarExt::_RegReadString
(
    HKEY hkeyThisExtension,
    LPCWSTR pszPropName,
    BSTR * pbstrProp,
    BOOL fExpand            // = FALSE, Expand Environment strings
    )
{
    WCHAR   szData[MAX_PATH];
    *pbstrProp = NULL;
    BOOL fSuccess = FALSE;
    
    // First try the optional location for localized content
    if (_hkeyCurrentLang)
    {
        if (SUCCEEDED(SHLoadRegUIString(_hkeyCurrentLang, pszPropName, szData, ARRAYSIZE(szData))))
        {
            fSuccess = TRUE;
        }
    }

    // Next try default location
    if (!fSuccess && _hkeyThisExtension)
    {
        if (SUCCEEDED(SHLoadRegUIString(hkeyThisExtension, pszPropName, szData, ARRAYSIZE(szData))))
        {
            fSuccess = TRUE;
        }
    }

    if (fSuccess)
    {
        LPWSTR psz = szData;
        WCHAR szExpand[MAX_PATH];
        if (fExpand)
        {
            SHExpandEnvironmentStrings(szData, szExpand, ARRAYSIZE(szExpand));
            psz = szExpand;
        }
        *pbstrProp = SysAllocString(psz);
    }
    return (NULL != *pbstrProp);
}


///////////////////////////////////////////////////////////
// Class CToolbarExtBand
//
// This class adds to the base functionality of CToolbarExt
// by storing the CLSID for a registered band, and displaying that
// band upon execution of IOleCommandTarget::Exec
//
//
STDAPI CToolbarExtBand_CreateInstance(
            IUnknown        * punkOuter,
            IUnknown        ** ppunk,
            LPCOBJECTINFO   poi
            )
{
    HRESULT hr = S_OK;

    *ppunk = NULL;

    CToolbarExtBand * lpTEB = new CToolbarExtBand();

    if (lpTEB == NULL)
        hr = E_OUTOFMEMORY;
    else
        *ppunk = SAFECAST(lpTEB, IBrowserExtension *);

    return hr;
}

// Constructor / Destructor
//
CToolbarExtBand::CToolbarExtBand()
{
    ASSERT(_cRef == 1);
    ASSERT(_bBandState == FALSE);
    ASSERT(_bstrBandCLSID == NULL);
}

// Destructor
//
CToolbarExtBand::~CToolbarExtBand() 
{ 
    if (_bstrBandCLSID)
        SysFreeString(_bstrBandCLSID);
}

// IBrowserExtension::Init()   We pass the majroity of the work on to the base class, then we load
// the BandCLSID and cache it.
STDMETHODIMP CToolbarExtBand::Init(REFGUID rguid)
{
    HRESULT hr = CToolbarExt::Init(rguid);
    
    _RegReadString(_hkeyThisExtension, TEXT("BandCLSID"), &_bstrBandCLSID);
    
    if (!(_bstrButtonText && _bstrBandCLSID))
        hr = E_FAIL;

    return hr;
}
    
STDMETHODIMP CToolbarExtBand::QueryStatus
(
    const GUID * pguidCmdGroup,
    ULONG  cCmds,
    OLECMD prgCmds[],
    OLECMDTEXT * pCmdText
    )
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;
    if (pguidCmdGroup && IsEqualGUID(*pguidCmdGroup, CLSID_ToolbarExtButtons))
    {
        VARIANT varClsid;
      
        // Default to all commands enabled
        for (ULONG i = 0; i < cCmds; i++)
        {
            varClsid.vt = VT_BSTR;
            varClsid.bstrVal = _bstrBandCLSID;
            prgCmds[i].cmdf = OLECMDF_ENABLED | OLECMDF_SUPPORTED;

            hr = IUnknown_Exec(_pisb, &CGID_ShellDocView, SHDVID_ISBROWSERBARVISIBLE, 0, &varClsid, NULL);
            if (S_OK == hr)
            {
                prgCmds[i].cmdf |= OLECMDF_LATCHED;
            }
        }
        hr = S_OK;
    }
    return hr;
}

// Take the pIShellBrowser (obtained from IObjectWithSite::SetSite()) and disply the band
STDMETHODIMP CToolbarExtBand::Exec( 
                const GUID              * pguidCmdGroup,
                DWORD                   nCmdID,
                DWORD                   nCmdexecopt,
                VARIANT                 * pvaIn,
                VARIANT                 * pvaOut
                )
{
    HRESULT hr = E_FAIL;
    
    if (_pisb)
    {
        VARIANT varClsid;
        varClsid.vt = VT_BSTR;
        varClsid.bstrVal = _bstrBandCLSID;
      
        _bBandState = !_bBandState;
        IUnknown_Exec(_pisb, &CGID_ShellDocView, SHDVID_SHOWBROWSERBAR, _bBandState, &varClsid, NULL);

        hr = S_OK;
    }
    
    return hr;
}


///////////////////////////////////////////////////////////////////////
// Class CToolbarExtExec
//
// Expands on the base class by adding support for tools menu plug-ins.
// An instance of this class can be a button OR a menu OR BOTH.  It also
// keeps track of a BSTR which it ShellExecutes in its IOleCommandTarget::Exec()
//
STDAPI CToolbarExtExec_CreateInstance(
            IUnknown        * punkOuter,
            IUnknown        ** ppunk,
            LPCOBJECTINFO   poi
            )
{
    HRESULT hr = S_OK;

    *ppunk = NULL;

    CToolbarExtExec * lpTEE = new CToolbarExtExec();

    if (lpTEE == NULL)
        hr = E_OUTOFMEMORY;
    else
        *ppunk = SAFECAST(lpTEE, IBrowserExtension *);

    return hr;
}

CToolbarExtExec::CToolbarExtExec()
{
    ASSERT(_cRef == 1);
    ASSERT(_bstrToolTip == NULL);
    ASSERT(_bstrExec == NULL);
    ASSERT(_bstrScript == NULL);
    ASSERT(_bstrMenuText == NULL);
    ASSERT(_bstrMenuCustomize == NULL);
    ASSERT(_bstrMenuStatusBar == NULL);
    ASSERT(_punkExt == NULL);
}

CToolbarExtExec::~CToolbarExtExec()
{
    if (_bstrToolTip)
        SysFreeString(_bstrToolTip);

    if (_bstrExec)
        SysFreeString(_bstrExec);

    if (_bstrScript)
        SysFreeString(_bstrScript);

    if (_bstrMenuText)
        SysFreeString(_bstrMenuText);

    if (_bstrMenuCustomize)
        SysFreeString(_bstrMenuCustomize);

    if (_bstrMenuStatusBar)
        SysFreeString(_bstrMenuStatusBar);

    if (_punkExt)
        _punkExt->Release();
}

// Pass on the work for the toolbar button intiaztion to the base class then determine the object
// type and initialize the menu information if necessary...
STDMETHODIMP CToolbarExtExec::Init(REFGUID rguid)
{
    HRESULT hr = CToolbarExt::Init(rguid);

    // If the baseclass initialization went OK, then we have a working button
    if (hr == S_OK)
        _bButton = TRUE;

    // Get app and/or script to execute (optional)
    _RegReadString(_hkeyThisExtension, TEXT("Exec"), &_bstrExec, TRUE);
    _RegReadString(_hkeyThisExtension, TEXT("Script"), &_bstrScript, TRUE);

        
    // See if we have a menu item
    if (_RegReadString(_hkeyThisExtension, TEXT("MenuText"), &_bstrMenuText))
    {
        _RegReadString(_hkeyThisExtension, TEXT("MenuCustomize"), &_bstrMenuCustomize);
        _RegReadString(_hkeyThisExtension, TEXT("MenuStatusBar"), &_bstrMenuStatusBar);
        _bMenuItem = TRUE;
    }

    if (_bMenuItem || _bButton)
    {
        hr = S_OK;
    }

    return hr;
}

// It we're a button try passing the work on to the base class, if that doesn't cut it we'll
// check the menu stuff...
STDMETHODIMP CToolbarExtExec::GetProperty(SHORT iPropID, VARIANTARG * pvarProperty)
{
    HRESULT     hr = S_OK;
    BOOL        fImple = FALSE;

    if (_bButton)
    {
        // If The generic button's getproperty returns S_OK then our job here is done
        if (CToolbarExt::GetProperty(iPropID, pvarProperty) == S_OK)
            fImple = TRUE;
    }
    
    if (_bMenuItem && !fImple)
    {
        fImple = TRUE;

        switch (iPropID)
        {
            case TMEX_CUSTOM_MENU:
            {
                if (pvarProperty != NULL)
                {
                    pvarProperty->vt = VT_BSTR;
                    pvarProperty->bstrVal = SysAllocString(_bstrMenuCustomize);
                    if (pvarProperty->bstrVal == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
            break;

            case TMEX_MENUTEXT:
                if (pvarProperty)
                {
                    pvarProperty->vt = VT_BSTR;
                    pvarProperty->bstrVal = SysAllocString(_bstrMenuText);
                    if (NULL == pvarProperty->bstrVal)
                        hr = E_OUTOFMEMORY;
                }
                break;

            case TMEX_STATUSBARTEXT:
                if (pvarProperty)
                {
                    pvarProperty->vt = VT_BSTR;
                    pvarProperty->bstrVal = SysAllocString(_bstrMenuStatusBar);
                    if (NULL == pvarProperty->bstrVal)
                        hr = E_OUTOFMEMORY;
                }
                break;

            default:
                fImple = FALSE;
        }
    }

    if (!fImple)
        hr = E_NOTIMPL;

    return hr;
}

STDMETHODIMP CToolbarExtExec::SetSite(IUnknown* punkSite)
{
    // Give the external object our site
    IUnknown_SetSite(_punkExt, punkSite);
    
    // Call base class
    return CToolbarExt::SetSite(punkSite);
}

STDMETHODIMP CToolbarExtExec::QueryStatus(const GUID * pguidCmdGroup, ULONG  cCmds, OLECMD rgCmds[], OLECMDTEXT * pCmdText)
{
    HRESULT hr = S_OK;

    // Pass query to external object if it exists
    IOleCommandTarget* pCmd;
    if (_punkExt && SUCCEEDED(_punkExt->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&pCmd)))
    {
        hr = pCmd->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pCmdText);
        pCmd->Release();
    }
    else
    {
        // Let base class handle this
        hr = CToolbarExt::QueryStatus(pguidCmdGroup, cCmds, rgCmds, pCmdText);
    }

    return hr;
}

// Shell execute the _bstrExec 
STDMETHODIMP CToolbarExtExec::Exec(
                const GUID              * pguidCmdGroup,
                DWORD                   nCmdId,
                DWORD                   nCmdexecopt,
                VARIANT                 * pvaIn,
                VARIANT                 * pvaOut
                )
{
    HRESULT hr = S_OK;

    //
    // The first time this is called, we lazy instantiate an external object if
    // one is registered.. This object can JIT in components and provide a
    // command target.
    //
    if (!_bExecCalled)
    {
        // We only do this once
        _bExecCalled = TRUE;

        BSTR bstrExtCLSID;
        if (_RegReadString(_hkeyThisExtension, TEXT("clsidExtension"), &bstrExtCLSID))
        {
            // We have an extension clsid, so create the object.  This gives the object an oportunity
            // to jit in code when its button or menu is invoked.
            CLSID clsidExt;

            if (CLSIDFromString(bstrExtCLSID, &clsidExt) == S_OK)
            {
                if (SUCCEEDED(CoCreateInstance(clsidExt, NULL, CLSCTX_INPROC_SERVER,
                                     IID_IUnknown, (void **)&_punkExt)))
                {
                    // Give the object our site (optional)
                    IUnknown_SetSite(_punkExt, _pisb);
                }
            }
            SysFreeString(bstrExtCLSID);
        }
    }

    // Pass command to external object if it exists
    IOleCommandTarget* pCmd;
    if (_punkExt && SUCCEEDED(_punkExt->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&pCmd)))
    {
        hr = pCmd->Exec(pguidCmdGroup, nCmdId, nCmdexecopt, pvaIn, pvaOut);
        pCmd->Release();
    }

    // Run a script if one was specified
    if(_bstrScript && _pisb)
    {
        IOleCommandTarget *poct = NULL;
        VARIANT varArg;
        varArg.vt = VT_BSTR;
        varArg.bstrVal = _bstrScript;
        hr = _pisb->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&poct);
        if (SUCCEEDED(hr))
        {
            // Tell MSHTML to execute the script
            hr = poct->Exec(&CGID_MSHTML, IDM_RUNURLSCRIPT, 0, &varArg, NULL);
            poct->Release();
        }
    }

    // Launch executable if one was specified
    if (_bstrExec)
    {
        SHELLEXECUTEINFO sei = { 0 };

        sei.cbSize = sizeof(sei);
        sei.lpFile = _bstrExec;
        sei.nShow = SW_SHOWNORMAL;

        // We are using ShellExecuteEx over ShellExecute because the Unicode version of ShellExecute
        // is bogus on 95/98
        if (ShellExecuteExW(&sei) == FALSE)
            hr = E_FAIL;
    }

    return hr;
}

