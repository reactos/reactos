/*
 * @(#)securitymanager.hxx 1.0 10/01/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of IInternetHostSecurityManager object for XTL scripting
 * 
 */


#ifndef _SECURITYMANAGER_HXX_
#define _SECURITYMANAGER_HXX_

//+---------------------------------------------------------------------------
//  *** FROM TRIDENT mshtml\src\site\ole\safety.hxx ***
//
//  Enumeration:          SAFETYOPERATION
//  Synopsis:             Indicates which operation is being validated for safety
//  Reason for Inclusion: To match trident security regarding ActiveX Object instantiation
//----------------------------------------------------------------------------
enum SAFETYOPERATION
{
    SAFETY_INIT,
    SAFETY_SCRIPT,
    SAFETY_SCRIPTENGINE
};

//+------------------------------------------------------------------------
//  *** FROM TRIDENT mshtml\src\site\ole\clstab.hxx ***
//
//  Enumeration:          COMPAT
//  Synopsis:             OLE Control compatiblity flags.
//  Reason for Inclusion: To match trident security regarding ActiveX Object instatiation
//                        We only use object safety flags, the rest are commented out for now
//                        Trident maintains a list of ActiveX controls in the registry for which
//                        the following characteristics are stored.
//-------------------------------------------------------------------------
enum
{
//  COMPAT_AGGREGATE =                  0x00000001,   // Aggregate this ocx
    COMPAT_NO_OBJECTSAFETY =            0x00000002,   // Ignore safety info presented by ocx
//  COMPAT_NO_PROPNOTIFYSINK =          0x00000004,   // Don't attach prop notify sink
//  COMPAT_SEND_SHOW =                  0x00000008,   // DoVerb(SHOW) before oVerb(INPLACE)
//  COMPAT_SEND_HIDE =                  0x00000010,   // DoVerb(HIDE) before InPlaceDeactivate()
//  COMPAT_ALWAYS_INPLACEACTIVATE =     0x00000020,   // Baseline state is INPLACE in browse mode even if not visible.
//  COMPAT_NO_SETEXTENT =               0x00000040,   // Don't bother calling SetExtent
//  COMPAT_NO_UIACTIVATE =              0x00000080,   // Never let this ctrl uiactivate
//  COMPAT_NO_QUICKACTIVATE =           0x00000100,   // Don't bother with IQuickActivate
//  COMPAT_NO_BINDF_OFFLINEOPERATION =  0x00000200,   // filter out BINDF_OFFLINEOPERATION flag.
    COMPAT_EVIL_DONT_LOAD =             0x00000400,   // don't load this control at all.
//  COMPAT_PROGSINK_UNTIL_ACTIVATED =   0x00000800,   // delay OnLoad() event firing until this ctrl is inplace activated.
                                                      // ALWAYS USE WITH COMPAT_ALWAYS_INPLACEACTIVATE
//  COMPAT_USE_PROPBAG_AND_STREAM =     0x00001000,   // Call both IPersistPropBag::Load and IPersistStreamInit::Load
//  COMPAT_DISABLEWINDOWLESS      =     0x00002000,   // do not allow control to get inplace activated windowless
//  COMPAT_SETWINDOWRGN           =     0x00004000,   // SetWindowRgn to clip rect before call to SetObjectRects to avoid flickering
//  COMPAT_PRINTPLUGINSITE        =     0x00008000,   // When printing, ask the plugin site to print directly instead
//  COMPAT_INPLACEACTIVATEEVENWHENINVISIBLE =   0x00010000, // Inplace Activate even when not visible
//  COMPAT_NEVERFOCUSSABLE        =     0x00020000,   // Hack for #68793
//  COMPAT_ALWAYSDEFERSETWINDOWRGN  = 0x00040000,     // Hack for #71466
//  COMPAT_INPLACEACTIVATESYNCHRONOUSLY = 0x00080000, // Hack for #71073
};

extern HRESULT CompatFlagsFromClsid(REFCLSID clsid, DWORD *pdwCompat);

class SecurityManager : public _unknown<IInternetHostSecurityManager, &IID_IInternetHostSecurityManager>
{
public:
    SecurityManager();
    virtual ~SecurityManager();

    HRESULT STDMETHODCALLTYPE GetSecurityId( 
        BYTE *pbSecurityId,
        DWORD *pcbSecurityId,
        DWORD_PTR dwReserved);
        
    HRESULT STDMETHODCALLTYPE ProcessUrlAction( 
        DWORD dwAction,
        BYTE *pPolicy,
        DWORD cbPolicy,
        BYTE *pContext,
        DWORD cbContext,
        DWORD dwFlags,
        DWORD dwReserved);
    
    HRESULT STDMETHODCALLTYPE QueryCustomPolicy( 
        REFGUID guidKey,
        BYTE **ppPolicy,
        DWORD *pcbPolicy,
        BYTE *pContext,
        DWORD cbContext,
        DWORD dwReserved);

    HRESULT Init(DWORD dwSafetyOptions, IUnknown *pSite, String *sURL);

private:
    bool CheckSafety(SAFETYOPERATION sOperation, REFIID riid, CLSID clsid, IUnknown *pUnk);
    HRESULT IsAllowableAction(DWORD dwAction, bool *pfAllow);
    
    BSTR _bstrURL;
    bool _fBlanketDisallow;  // we have a security context but don't ask IE security mgr, blanket disallow
    bool _fSecurityContext;  // do we have a security context
};

#endif // _SECURITYMANAGER_HXX_