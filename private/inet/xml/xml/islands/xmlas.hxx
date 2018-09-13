/*++

Copyright (c) 1998 - 1999 Microsoft Corporation. All rights reserved.
Module:
    xmlas.hxx

Abstract:
    Implements CXMLScriptEngine, the ActiveScripting host for the XML parser

Author:
    Simon Bernstein (simonb)

History:
    02-26-1998 Created (simonb)

--*/

#ifndef __XMLAS_HXX__
#define __XMLAS_HXX__

//
// Class definition
// 

const DWORD g_dwInterfaceSecurityOptions = 
    INTERFACESAFE_FOR_UNTRUSTED_CALLER |
    INTERFACESAFE_FOR_UNTRUSTED_DATA   |
    INTERFACE_USES_SECURITY_MANAGER    |
    INTERFACE_USES_DISPEX;

const CLSID CLSID_CXMLScriptEngine = {0x989D1DC0,0xB162,0x11d1,{0xB6, 0xEC,0xD2,0x7D,0xDC,0xF9,0xA9,0x23}};
class __declspec(uuid("989D1DC0-B162-11d1-B6EC-D27DDCF9A923"))
CXMLScriptEngine : 
    public IActiveScript,
    public IActiveScriptParse,
    public IObjectSafety
{   
    
public:
    ////////////////////////////////////////////////
    // Basic class members
    ////////////////////////////////////////////////

    ~CXMLScriptEngine();
    friend LPUNKNOWN CXMLScriptEngineConstruct();

private:

    // Don't want anyone but the CXMLScriptEngineConstruct function to be
    // able to construct one of these
    CXMLScriptEngine(
        HRESULT *phr);

    /////////////////////////////////////////
    // Private functions
    /////////////////////////////////////////

    IHTMLElement *GetMyScriptElement();

    HRESULT AttachToXMLParser(
        IXMLDOMDocument **ppDoc);

    BOOL IsSecure(
        IHTMLElement *pElem);

    HRESULT GetSrcAttrib(
        IHTMLElement *pElem);

    HRESULT RaiseScriptError(
        ULONG   iLineNumber,
        LONG    iCharNumber,
        LPCWSTR pcwszDescription);

    ////////////////////////////////////////////////
    // Class member variables
    ////////////////////////////////////////////////

    ULONG                       m_cRef; // Reference count
    IActiveScriptSite           *m_pScriptSite; 
    IHTMLDocument2              *m_pDocument;
    SCRIPTSTATE                 m_ssenumState;
    bool                        m_fInitNewCalled;
    bool                        m_fWindowAddedToNameSpace;
    DWORD                       m_dwCurrentSafety;
    LPWSTR                      m_pwszSrcAttrib;

public:

    ////////////////////////////////////////////////
    // IUnknown members
    ////////////////////////////////////////////////
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID riid,
        void  **ppvObject);
    
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    
    virtual ULONG STDMETHODCALLTYPE Release(void);

    ////////////////////////////////////////////////
    // IObjectSafety members
    ////////////////////////////////////////////////

    virtual HRESULT STDMETHODCALLTYPE GetInterfaceSafetyOptions( 
        /* [in] */ REFIID riid,
        /* [out] */ DWORD __RPC_FAR *pdwSupportedOptions,
        /* [out] */ DWORD __RPC_FAR *pdwEnabledOptions);
    
    virtual HRESULT STDMETHODCALLTYPE SetInterfaceSafetyOptions( 
        /* [in] */ REFIID riid,
        /* [in] */ DWORD dwOptionSetMask,
        /* [in] */ DWORD dwEnabledOptions);
        
    ////////////////////////////////////////////////
    // IActiveScript members
    ////////////////////////////////////////////////

    virtual HRESULT STDMETHODCALLTYPE SetScriptSite( 
    /* [in] */ IActiveScriptSite __RPC_FAR *pass);
    
    virtual HRESULT STDMETHODCALLTYPE GetScriptSite( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
    
    virtual HRESULT STDMETHODCALLTYPE SetScriptState( 
        /* [in] */ SCRIPTSTATE ss);
    
    virtual HRESULT STDMETHODCALLTYPE GetScriptState( 
        /* [out] */ SCRIPTSTATE __RPC_FAR *pssState);
    
    virtual HRESULT STDMETHODCALLTYPE Close( void);
    
    virtual HRESULT STDMETHODCALLTYPE AddNamedItem( 
        /* [in] */ LPCOLESTR pstrName,
        /* [in] */ DWORD dwFlags);
    
    virtual HRESULT STDMETHODCALLTYPE AddTypeLib( 
        /* [in] */ REFGUID rguidTypeLib,
        /* [in] */ DWORD dwMajor,
        /* [in] */ DWORD dwMinor,
        /* [in] */ DWORD dwFlags);
    
    virtual HRESULT STDMETHODCALLTYPE GetScriptDispatch( 
        /* [in] */ LPCOLESTR pstrItemName,
        /* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdisp);
    
    virtual HRESULT STDMETHODCALLTYPE GetCurrentScriptThreadID( 
        /* [out] */ SCRIPTTHREADID __RPC_FAR *pstidThread);
    
    virtual HRESULT STDMETHODCALLTYPE GetScriptThreadID( 
        /* [in] */ DWORD dwWin32ThreadId,
        /* [out] */ SCRIPTTHREADID __RPC_FAR *pstidThread);
    
    virtual HRESULT STDMETHODCALLTYPE GetScriptThreadState( 
        /* [in] */ SCRIPTTHREADID stidThread,
        /* [out] */ SCRIPTTHREADSTATE __RPC_FAR *pstsState);
    
    virtual HRESULT STDMETHODCALLTYPE InterruptScriptThread( 
        /* [in] */ SCRIPTTHREADID stidThread,
        /* [in] */ const EXCEPINFO __RPC_FAR *pexcepinfo,
        /* [in] */ DWORD dwFlags);
    
    virtual HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IActiveScript __RPC_FAR *__RPC_FAR *ppscript);

    
    ////////////////////////////////////////////////
    // IActiveScriptParse members
    ////////////////////////////////////////////////

    virtual HRESULT STDMETHODCALLTYPE InitNew( void);
    
    virtual HRESULT STDMETHODCALLTYPE AddScriptlet( 
        /* [in] */ LPCOLESTR pstrDefaultName,
        /* [in] */ LPCOLESTR pstrCode,
        /* [in] */ LPCOLESTR pstrItemName,
        /* [in] */ LPCOLESTR pstrSubItemName,
        /* [in] */ LPCOLESTR pstrEventName,
        /* [in] */ LPCOLESTR pstrDelimiter,
        /* [in] */ DWORD dwSourceContextCookie,
        /* [in] */ ULONG ulStartingLineNumber,
        /* [in] */ DWORD dwFlags,
        /* [out] */ BSTR __RPC_FAR *pbstrName,
        /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo);
    
    virtual HRESULT STDMETHODCALLTYPE ParseScriptText( 
        /* [in] */ LPCOLESTR pstrCode,
        /* [in] */ LPCOLESTR pstrItemName,
        /* [in] */ IUnknown __RPC_FAR *punkContext,
        /* [in] */ LPCOLESTR pstrDelimiter,
        /* [in] */ DWORD dwSourceContextCookie,
        /* [in] */ ULONG ulStartingLineNumber,
        /* [in] */ DWORD dwFlags,
        /* [out] */ VARIANT __RPC_FAR *pvarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo);

};

class CActiveScriptError : public _unknown<IActiveScriptError, &IID_IActiveScriptError>
{
private:
    ULONG  m_iLineNumber;
    LONG   m_iCharNumber;
    LPWSTR m_pwszDescription;

public:
    CActiveScriptError::CActiveScriptError(
        ULONG iLine,
        LONG iCharNumber,
        LPCWSTR pcwszDescription);

    ~CActiveScriptError();

    ////////////////////////////////////////////////
    // IActiveScriptError members
    ////////////////////////////////////////////////

    virtual HRESULT STDMETHODCALLTYPE GetExceptionInfo( 
        /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo);

    virtual HRESULT STDMETHODCALLTYPE GetSourcePosition( 
        /* [out] */ DWORD __RPC_FAR *pdwSourceContext,
        /* [out] */ ULONG __RPC_FAR *pulLineNumber,
        /* [out] */ LONG __RPC_FAR *plCharacterPosition);

    virtual HRESULT STDMETHODCALLTYPE GetSourceLineText( 
        /* [out] */ BSTR __RPC_FAR *pbstrSourceLine);
};

#endif // __XMLAS_HXX__

// end of file: xmlas.hxx
