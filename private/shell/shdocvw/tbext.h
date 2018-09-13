///////////////////////////////////////////////////////
// File:   TBBxt.h    (Toolbar Button Extension Headers
// Author: Karim Farouki
//
// We declare here three classes:
// (1) CToolbarExt a base class that takes care of the
//     button work for our custom extensions
// (2) CToolbarExtBand the object which deals with custom
//     buttons that plug into bands
// (3) CToolbarExtExec the object which deals with custom
//     buttons (or tools menu items) that exec stuff.
//
// The latter two are derived from the former 

#ifndef _TBEXT_H
#define _TBEXT_H

#include "priv.h"

//
// Internal interface for accessing ther properties of a button/menu extension.
// This interface will likely go away afer IE5B2 when we move this functionality to
// a browser helper object.
//
typedef enum _tagGetPropertyIDs 
{
    TBEX_BUTTONTEXT     = 100,     // VT_BSTR
    TBEX_TOOLTIPTEXT    = 101,     // VT_BSTR
    TBEX_GRAYICON       = 102,     // HICON as a VT_BYREF
    TBEX_HOTICON        = 103,     // HICON as a VT_BYREF
    TBEX_GRAYICONSM     = 104,     // HICON as a VT_BYREF     
    TBEX_HOTICONSM      = 105,     // HICON as a VT_BYREF
    TBEX_DEFAULTVISIBLE = 106,     // VT_BOOL
    TMEX_MENUTEXT       = 200,     // VT_BSTR   
    TMEX_STATUSBARTEXT  = 201,     // VT_BSTR
    TMEX_CUSTOM_MENU    = 202,     // VT_BSTR
} GETPROPERTYIDS;

interface IBrowserExtension : IUnknown
{
    virtual STDMETHODIMP Init(REFGUID refguid) = 0;
    virtual STDMETHODIMP GetProperty(SHORT iPropID, VARIANTARG * varProperty) = 0;
};

class CToolbarExt : public IBrowserExtension,
                    public IOleCommandTarget,
                    public IObjectWithSite
{
public:
    // Constructor/Destructor
    CToolbarExt();
    virtual ~CToolbarExt();

    // IUnknown Interface Members
    STDMETHODIMP            QueryInterface(const IID& iid, void** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IBrowserExtension Interface Members
    STDMETHODIMP Init(REFGUID rguid);
    STDMETHODIMP GetProperty(SHORT iPropID, VARIANTARG * pvarProperty);

    // IOleCommandTarget Interface Members
    STDMETHODIMP QueryStatus(const GUID * pguidCmdGroup, ULONG  cCmds, OLECMD prgCmds[], OLECMDTEXT * pCmdText);
    STDMETHODIMP Exec(const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT* pvaIn, VARIANT* pvaOut) = 0;

    // IObjectWithSite Interface Members        
    STDMETHODIMP SetSite(IUnknown* pUnkSite);
    STDMETHODIMP GetSite(REFIID riid, void ** ppvSite);

protected:
    BOOL _RegGetBoolValue(LPCWSTR pszPropName, BOOL fDefault);
    BOOL _RegReadString(HKEY hkeyThisExtension, LPCWSTR szPropName, BSTR * pbstrProp, BOOL fExpand = FALSE);
    HICON _ExtractIcon(LPWSTR pszPath, int resid, int cx, int cy);
    HRESULT _GetIcon(LPCWSTR pszIcon, int nWidth, int nHeight, HICON& rhIcon, VARIANTARG * pvarProperty);

    long            _cRef;
    HICON           _hIcon;             // gray icon regular size
    HICON           _hIconSm;           // gray icon small
    HICON           _hHotIcon;          // Hot... are color versions of above
    HICON           _hHotIconSm;
    BSTR            _bstrButtonText;    // The buttons caption
    BSTR            _bstrToolTip;       // This is optional (not supported on our side yet)
    HKEY            _hkeyThisExtension; 
    HKEY            _hkeyCurrentLang;   // optional location for localized strings
    IShellBrowser*  _pisb;              // passed in by IObjectWithSite::SetSite()  Used to load band
};

class CToolbarExtBand : public CToolbarExt
{
public:
    // Constructor / Destructor
    CToolbarExtBand();
    virtual ~CToolbarExtBand();
    
    // Overridden IBrowserExtension Interface Members
    STDMETHODIMP Init(REFGUID rguid);

    // Overridden IOleCommandTarget Interface Members
    STDMETHODIMP QueryStatus(const GUID * pguidCmdGroup, ULONG  cCmds, OLECMD prgCmds[], OLECMDTEXT * pCmdText);
    STDMETHODIMP Exec(const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT* pvaIn, VARIANT* pvaOut);

protected:
    BOOL            _bBandState;        // This is a hack... ideally state will be determined from the browser
    BSTR            _bstrBandCLSID;     // CLSID of band to load.  Kept as BSTR because this is how it is passed
                                        // to load the band
};

class CToolbarExtExec : public CToolbarExt
{
public:
    // Constructor / Destructor
    CToolbarExtExec();
    virtual ~CToolbarExtExec();
    
    // Overridden IBrowserExtension Interface Members
    STDMETHODIMP Init(REFGUID rguid);
    STDMETHODIMP GetProperty(SHORT iPropID, VARIANTARG * pvarProperty);

    // Overridden IObjectWithSite Interface Members        
    STDMETHODIMP SetSite(IUnknown* pUnkSite);

    // Overridden IOleCommandTarget Interface Members
    STDMETHODIMP QueryStatus(const GUID * pguidCmdGroup, ULONG  cCmds, OLECMD prgCmds[], OLECMDTEXT * pCmdText);
    STDMETHODIMP Exec(const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT* pvaIn, VARIANT* pvaOut);

protected:
    BOOL            _bButton;           // Does this object support being a button?
    BOOL            _bMenuItem;         // Does it support being a menu item?
    BOOL            _bExecCalled;       // if Exec was called
    BSTR            _bstrExec;          // Thing to ShellExecute
    BSTR            _bstrScript;        // Script to Execute
    BSTR            _bstrMenuText;
    BSTR            _bstrMenuCustomize; // the menu that is to be customized
    BSTR            _bstrMenuStatusBar;
    IUnknown*       _punkExt;           // (Optional) created when button is first pressed
};

#endif // _TBEXT_H
