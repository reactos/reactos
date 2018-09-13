/*
 * @(#)SchemaNodeFactory.hxx 1.0 8/7/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _SCHEMANODEFACTORY_HXX
#define _SCHEMANODEFACTORY_HXX

#include <xmlparser.h>

#ifndef _PARSER_OM_ELEMENT
#include "xml/om/element.hxx"  
#endif

#ifndef _XML_DOM_NAMESPACEMGR
#include "xml/om/namespacemgr.hxx" 
#endif

#ifndef _CORE_UTIL_NAME
#include "core/util/name.hxx" 
#endif

#ifndef _XML_PARSER_DTD
#include "xml/dtd/dtd.hxx" 
#endif

#ifndef _SCHEMABUILDER_HXX
#include "schemabuilder.hxx"
#endif

#ifndef _SCHEMANAMES_HXX
#include "schemanames.hxx"
#endif


class SchemaNodeFactory : public _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>
{
public:

    SchemaNodeFactory(IXMLNodeFactory * fc, DTD * pDTD, NamespaceMgr * pPrevNSMgr, 
                      NamespaceMgr * pThisNSMgr, Atom * pURN, Document* pDoc);
    ~SchemaNodeFactory();

#ifdef _DEBUG
    virtual ULONG STDMETHODCALLTYPE AddRef()
            {
                return _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>::AddRef();
            }

    virtual ULONG STDMETHODCALLTYPE Release()
            {
                return _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>::Release();
            }
#endif

    //
    // IXMLNodeFactory interface
    //
    HRESULT STDMETHODCALLTYPE NotifyEvent( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODEFACTORY_EVENT iEvt);

    HRESULT STDMETHODCALLTYPE BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo);

    HRESULT STDMETHODCALLTYPE EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ BOOL fEmpty,
        /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo);
    
    HRESULT STDMETHODCALLTYPE Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo);

    HRESULT STDMETHODCALLTYPE CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo);

private:

    void init();

private:
            
    RXMLNodeFactory _pFactory;

    RNamespaceMgr   _pPrevNSMgr;              // since we are creating a new document for each schema
    RNamespaceMgr   _pNamespaceMgr;           // the PrevNSMgr is the NSMgr from the parent document

    SchemaBuilder * _pSchemaBuilder;

    WDocument       _pDoc;
    RAtom           _pURN;
};

#endif