/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _VIEWERFACTORY_HXX
#define _VIEWERFACTORY_HXX

#include <xmlparser.h>
#include "mimedownload.hxx"
#include "callback.hxx"
#include "bufferedstream.hxx"

#define XSLASYNC 1

typedef enum {
    SSTYPE_NONE,
    SSTYPE_CSS,
    SSTYPE_XSL,
} SSTYPE;

typedef enum {
    PITYPE_NONE,
    PITYPE_OTHER,
    PITYPE_BODY,
    PITYPE_XML,
    PITYPE_STYLESHEET,
//  PITYPE_OBJECT,
//  PITYPE_SCRIPT,
} PITYPE;

#if XSLASYNC
class IXTLProcessor;
#endif

class ViewerFactory : public _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>
{
public:
    ViewerFactory::ViewerFactory(MIMEBufferedStream* pStm, IXMLNodeFactory *pDefaultFactory, IXMLDOMDocument *pDoc, IMoniker *pimkXML, CallbackMonitor *pMonitor, HANDLE evtLoad);

    ViewerFactory::~ViewerFactory();

    virtual HRESULT STDMETHODCALLTYPE NotifyEvent( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODEFACTORY_EVENT iEvt);
        
    virtual HRESULT STDMETHODCALLTYPE BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo);
        
    virtual HRESULT STDMETHODCALLTYPE EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ BOOL fEmpty,
        /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo);
        
    virtual HRESULT STDMETHODCALLTYPE Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo);
    
    virtual HRESULT STDMETHODCALLTYPE CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo);

    void    closeXSL(void);

private:
    BOOL    IsCSSWrite(void);
    void    CSSWriteCDATA(XML_NODE_INFO *pInfo);
    void    CSSWriteInfo(XML_NODE_INFO *pInfo);
    void    CSSWrite(const WCHAR* text, ULONG len = 0);
    void    write(const WCHAR* text, ULONG len = 0);

    HRESULT generateNamespaces(USHORT cNumRecs, XML_NODE_INFO **apNodeInfo);
    HRESULT generateTag(USHORT cNumRecs, XML_NODE_INFO **apNodeInfo);
    HRESULT expandEntity(XML_NODE_INFO *pNodeInfo);

    void    detectStylesheet(const WCHAR *pwcText, ULONG ulLen);
    HRESULT addExpandos(void);

    HRESULT loadSS(IXMLDOMDocument *pXSLDocument);
    HRESULT loadSSDefault(IXMLDOMDocument *pXSLDocument);
    HRESULT setSecurity(IXMLDOMDocument *pBaseDocument, IXMLDOMDocument *pDocument);
    HRESULT setDocSecurity(IXMLDOMDocument *pDocument);
    
    HRESULT setGenericParse(VARIANT_BOOL f);

    void    freeStream(void);

#if XSLASYNC
    HRESULT initXSLAsync(void);
    HRESULT processXSLAsync(IXMLNodeSource *pSource, BOOL fLast);
    HRESULT signalAsync(IXMLNodeSource *pSource);

    IXTLProcessor      *_pIXTL;
    UINT               _nSignals;
    bool               _fInitAsync;
    BSTR               _bXSLAsyncErr;
#endif

#if MIMEASYNC
    // communicate to the callback monitor to prevent URLCallback from
    // running the parser on the GUI thread
    CallbackMonitor *_pbcsMonitor;

    // event which is used to signal end of download, passed to the async thread
    HANDLE _evtLoad;

    // indicates download in the worker thread is active for this factory
    BOOL  _fIsAsync;

    // indicates that we should stop processing tokens in the worker thread
    BOOL  _fStopAsync;

    // Try to start async.  pSource is the parser to use.
    // Returns success or failure
    BOOL  StartAsync(IXMLNodeSource *pSource);

#endif

    MIMEBufferedStream *_pStm;
    IXMLNodeFactory *   _pDefaultFactory;
    IXMLDOMDocument *   _pDOMDocument;
    IMoniker *          _pimkXML;
    IXMLDOMDocument *   _pXSLDocument;
    bool                _fXSLError;

    SSTYPE              _ssType;
    PITYPE              _piType;
    WCHAR *             _wURL;
    long                _lcDTD;
    long                _lcEntity;

    bool                _fDocType;
    bool                _fElement;
    bool                _fInAttr;
    WCHAR               _chQuote;
    bool                _fInProlog;
    bool                _fBuildTree;
    bool                _fRootElem;
    HRESULT             _dwErrorCode;
    bool                _fErrReported;
    bool                _fGenericParse;

public:
    BOOL isErrReported(void) { return _fErrReported; }
    void writeError(BSTR bstrError, BSTR bstrSource);
    void reportError(HRESULT hr);
    void reportError2(HRESULT hr);

#if MIMEASYNC
    BOOL isAsync(void)       { return _fIsAsync; }
    // this is how the original thread signals the worker thread to stop
    // would normally come through destructor or stop button stuff
    // sets flag in the factory.  When the factory is called into in the other
    // thread _fStopAsync is set and we return error, which will then stop
    // the parser, we unwind, etc.
    void StopAsync(void)
    {
        if (_fIsAsync)
        {
            _fStopAsync = true;
            if (_pStm)
                _pStm->StopAsync();
        }
    }

    void releaseDocs(void);
#endif
};

class DetectFactory : public _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>
{
public:
    DetectFactory::DetectFactory();

    DetectFactory::~DetectFactory();

    virtual HRESULT STDMETHODCALLTYPE NotifyEvent( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODEFACTORY_EVENT iEvt)
    {
        return S_OK;
    }
        
    virtual HRESULT STDMETHODCALLTYPE BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
    {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ BOOL fEmpty,
        /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
    {
        return S_OK;
    }
        
    virtual HRESULT STDMETHODCALLTYPE Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo);
        
    virtual HRESULT STDMETHODCALLTYPE CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo);

public:
    SSTYPE GetType(void) { return _ssType; }
    BSTR   GetURL(void);

private:
    BSTR _bAttribute;
    BSTR _bURL;
    ULONG _ulAttrLen;
    SSTYPE _ssType;

};

#endif