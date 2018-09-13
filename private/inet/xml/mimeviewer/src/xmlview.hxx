/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _XMLVIEW_HXX
#define _XMLVIEW_HXX

#include <urlmon.h>
#include <perhist.h>
#include <msxml.h>
#include "xmlmimeguid.hxx"

class MIMEBufferedStream;

EXTERN_C const IID IID_IXMLViewerIdentity;
EXTERN_C const IID IID_IXMLViewerScriptMask;


interface NOVTABLE IXMLViewerIdentity : public IUnknown
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_XMLViewerObject( 
           /* [out][retval] */ VARIANT __RPC_FAR *pObj) = 0;
    };

interface NOVTABLE IXMLViewerScriptMask : public IUnknown
    {
    public:
    };

class Viewer : public IPersistMoniker, public IOleCommandTarget, public IPersistHistory, public IXMLViewerIdentity, public IXMLViewerScriptMask
{
public:
    
    Viewer();
    virtual ~Viewer();
 
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void ** ppv);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();


    // ====================== IPersist =====================
    virtual HRESULT STDMETHODCALLTYPE GetClassID( 
           /* [out] */ CLSID *pClassID);


    // ====================== IPersistMoniker ===================== 
    virtual HRESULT STDMETHODCALLTYPE IsDirty(void);

    virtual HRESULT STDMETHODCALLTYPE Load( 
                /* [in] */ BOOL fFullyAvailable,
                /* [in] */ IMoniker __RPC_FAR *pimkName,
                /* [in] */ LPBC pibc,
                /* [in] */ DWORD grfMode);

    virtual HRESULT STDMETHODCALLTYPE Save( 
                /* [in] */ IMoniker __RPC_FAR *pimkName,
                /* [in] */ LPBC pbc,
                /* [in] */ BOOL fRemember);

    virtual HRESULT STDMETHODCALLTYPE SaveCompleted( 
                /* [in] */ IMoniker __RPC_FAR *pimkName,
                /* [in] */ LPBC pibc);

    virtual HRESULT STDMETHODCALLTYPE GetCurMoniker( 
               /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppimkName);


    // ====================== IPersistHistory ===================== 
    virtual HRESULT STDMETHODCALLTYPE LoadHistory( 
        /* [in] */ IStream __RPC_FAR *pStream,
        /* [in] */ IBindCtx __RPC_FAR *pbc);
        
    virtual HRESULT STDMETHODCALLTYPE SaveHistory( 
        /* [in] */ IStream __RPC_FAR *pStream);

    virtual HRESULT STDMETHODCALLTYPE SetPositionCookie( 
        /* [in] */ DWORD dwPositioncookie);

    virtual HRESULT STDMETHODCALLTYPE GetPositionCookie( 
        /* [out] */ DWORD __RPC_FAR *pdwPositioncookie);

    // ====================== IOLECommandTarget ===================== 
    virtual /* [input_sync] */ HRESULT STDMETHODCALLTYPE QueryStatus( 
        /* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
        /* [in] */ ULONG cCmds,
        /* [out][in][size_is] */ OLECMD __RPC_FAR prgCmds[  ],
        /* [unique][out][in] */ OLECMDTEXT __RPC_FAR *pCmdText);
        
    virtual HRESULT STDMETHODCALLTYPE Exec( 
        /* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
        /* [in] */ DWORD nCmdID,
        /* [in] */ DWORD nCmdexecopt,
        /* [unique][in] */ VARIANT __RPC_FAR *pvaIn,
        /* [unique][out][in] */ VARIANT __RPC_FAR *pvaOut);

    // ====================== IXMLViewerIdentity =====================  
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_XMLViewerObject( 
       /* [out][retval] */ VARIANT __RPC_FAR *pObj);
    
public:
    HRESULT reloadFromTrident(MIMEBufferedStream *pStm, IMoniker *pimk, LPBC pbcTrident);
    HRESULT reload(BOOL fFullyAvailable, IMoniker *pimk, LPBC pbcTrident, LPBC pbcXML, DWORD grfMode, MIMEBufferedStream *pStm);
    HRESULT init(void);
#if MIMEASYNC
    HANDLE  getLoadCompleteEvent(void) { return _evtLoadComplete; }
#endif
    HRESULT stopDownload(void);

    IUnknown* getTrident();
private:
    HRESULT OleCmdSaveXML(DWORD nCmdexecOpt, VARIANT *pvaIn);
//  void    disableCmds(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD *prgCmds, const GUID *pguidCmdGroupDisable, DWORD cmdFirst, DWORD cmdLast);
    void    disableEncodingMenu(HMENU hMenu);
    void    saveGetNameFromURL(LPTSTR pchPath);
    HRESULT savePromptFileName(LPTSTR pchPath, int cchPath);
    HRESULT saveXMLFile(LPCTSTR pchPath);
    HWND    saveGetHWND(void);
    HRESULT alert(ResourceID resID);
    HRESULT alert(BSTR bstrMsg);

    void    cleanupBinding();

private:
    long _ulRefs;
    IUnknown * _pTrident;
    IXMLDOMDocument* _pDocument;
    IMoniker* _pimkCurrent;
    HANDLE _evtLoadComplete;
    CBinding* _pBinding;
#if MIMEASYNC
    static long s_objCount;
#endif
};


#endif