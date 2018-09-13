/*
 * @(#)SchemaNodeFactory.cxx 1.0 8/7/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#ifndef _SCHEMANODEFACTORY_HXX
#include "schemanodefactory.hxx"
#endif


SchemaNodeFactory::SchemaNodeFactory(IXMLNodeFactory * fc, DTD * pDTD, NamespaceMgr * pPrevNSMgr, 
                                     NamespaceMgr * pThisNSMgr, Atom * pURN, Document* pDoc)
{
    _pFactory = fc;     
    _pPrevNSMgr = pPrevNSMgr;
    _pNamespaceMgr = pThisNSMgr;

    _pURN = pURN;
    _pDoc = pDoc;

    _pSchemaBuilder = new SchemaBuilder(fc, pDTD, pPrevNSMgr, pThisNSMgr, pURN, pDoc);
    
    SchemaNames::classInit();
}


SchemaNodeFactory::~SchemaNodeFactory()
{
    delete _pSchemaBuilder;

    _pFactory = null;

    _pPrevNSMgr = null;
    _pNamespaceMgr = null;

    _pURN = null;
    _pDoc = null;
}


void 
SchemaNodeFactory::init()
{   
//    _pSchemaBuilder->init();
}

//
// interface
HRESULT STDMETHODCALLTYPE 
SchemaNodeFactory::NotifyEvent( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODEFACTORY_EVENT iEvt)
{    
    switch (iEvt)
    {
    case XMLNF_STARTDOCUMENT:
        init();
        _pSchemaBuilder->start();
        break;
    case XMLNF_ENDDOCUMENT: 
        _pSchemaBuilder->finish();
        break;
    }
    
    return _pFactory->NotifyEvent(pSource, iEvt);
}


HRESULT STDMETHODCALLTYPE 
SchemaNodeFactory::BeginChildren(
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
    HRESULT hr = S_OK;

    if (XML_ELEMENT == pNodeInfo->dwType)
    {
        hr = _pSchemaBuilder->ProcessAttributes(pSource, CAST_TO(Node*, (Object*)pNodeInfo->pNode));
        if (hr)
            goto Cleanup;
    }      
    hr =  _pFactory->BeginChildren(pSource, pNodeInfo);

Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE 
SchemaNodeFactory::EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ BOOL fEmpty,
        /* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
    HRESULT hr = S_OK;

    if (XML_ELEMENT == pNodeInfo->dwType)
    {
        if (fEmpty)
        {
            hr = _pSchemaBuilder->ProcessAttributes(pSource, CAST_TO(Node*, (Object*)pNodeInfo->pNode));
        }
        if (!hr)
        {
            hr = _pSchemaBuilder->ProcessEndChildren(pSource, CAST_TO(Node*, (Object*)pNodeInfo->pNode));
        }
        if (hr)
            goto Cleanup;
    }
    
    hr = _pFactory->EndChildren(pSource, fEmpty, pNodeInfo);

Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE 
SchemaNodeFactory::Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
{
    return _pFactory->Error(pSource, hrErrorCode, cNumRecs, apNodeInfo);
}


HRESULT STDMETHODCALLTYPE SchemaNodeFactory::CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
{
    XML_NODE_INFO* pNodeInfo = *apNodeInfo;
    HRESULT hr = _pFactory->CreateNode(pSource, pNodeParent, cNumRecs, apNodeInfo);

    if (!hr)
    {
        switch (pNodeInfo->dwType)
        {
        case XML_ELEMENT:
            hr = _pSchemaBuilder->ProcessElementNode(CAST_TO(Node*, (Object*)pNodeInfo->pNode));
            break;

        case XML_PCDATA:
            hr = _pSchemaBuilder->ProcessPCDATA(CAST_TO(Node*, (Object*)pNodeInfo->pNode), pNodeParent);
            break;
        }
    }

Cleanup:
    return hr;
}
