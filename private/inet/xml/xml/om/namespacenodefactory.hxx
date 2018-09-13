/*
 * @(#)NameSpaceNodeFactory.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _NAMESPACENODEFACTORY_HXX
#define _NAMESPACENODEFACTORY_HXX

#include "namespacemgr.hxx"
#include <xmlparser.h>
#include "dtd.hxx"
#include "document.hxx"

typedef _reference<IXMLNodeFactory> RNodeFactory;
class Document;
typedef _weakreference<Document> WDocument;
class ValidationFactory;

class NameSpaceNodeFactory : public _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>
{
protected:

public: 
    NameSpaceNodeFactory(Document* doc, IXMLNodeFactory * f, DTD* dtd, NamespaceMgr * mgr, 
        bool fIgnoreDTD, bool fProcessNamespaces);

    virtual ~NameSpaceNodeFactory();

    HRESULT STDMETHODCALLTYPE NotifyEvent( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ XML_NODEFACTORY_EVENT iEvt);
                           
    HRESULT STDMETHODCALLTYPE BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);
    
    HRESULT STDMETHODCALLTYPE EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ BOOL fEmptyNode,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);
    
    HRESULT STDMETHODCALLTYPE Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo);
    
    HRESULT STDMETHODCALLTYPE CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo);

public:
    const ValidationFactory * getValidationNF() const
    {
        return (ValidationFactory*)(IXMLNodeFactory *)_pValidationFactory;
    }

protected:

    bool    ProcessXMLNSAttributes(IXMLNodeSource* pNodeSource, 
                USHORT cNumRecs, XML_NODE_INFO** apNodeInfo, bool& fLoadedSchema);
    void    LoadSchema(IXMLNodeSource* pNodeSource, Atom* pURN);
    HRESULT HandlePending();


    RNodeFactory _pFactory;          // node factory we are delegating to _right_now_

    RNodeFactory _pDTDNodeFactory;
    RNodeFactory _pValidationFactory;
    RNodeFactory _pFactoryDefault;     // node factory to delegate calls
    RNamespaceMgr _pMgr;               // namespace manager
    WDocument   _pDoc;
    RDTD        _pDTD;

    bool _fHasDTD;
    bool _fEndProlog;                  // whether ended parsing prolog
    bool _fIgnoreDTD;                  // whether ignore loading DTD or not
    bool _fLoadingNamespace;
    long _lInDTD;
    bool _fInAttribute;
    int  _cSchemaPrefixLen;
    void* _pNodePending;     // pending node because of schema download.
};

extern HRESULT AbortParse(IXMLNodeSource * pNodeSource, Exception * e, Document * pDoc);
extern String* GetAttributeValue(XML_NODE_INFO** ppInfo, int * count, DTD * pDTD);

#endif _NAMESPACENODEFACTORY_HXX