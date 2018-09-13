/*
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _IE4NODEFACTORY_HXX
#define _IE4NODEFACTORY_HXX

#ifndef _XML_OM_NODEDATANODEFACTORY
#include "nodedatanodefactory.hxx"
#endif

class IE4NodeFactory : public _NDNodeFactory
{
public: // (De)Constructors
    IE4NodeFactory( Document * pDocument);
    ~IE4NodeFactory();

public:
    virtual HRESULT STDMETHODCALLTYPE NotifyEvent( 
			/* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
			/* [in] */ XML_NODEFACTORY_EVENT iEvt);
    
    virtual HRESULT STDMETHODCALLTYPE BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);
    
    virtual HRESULT STDMETHODCALLTYPE EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ BOOL fEmptyNode,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);

    virtual HRESULT STDMETHODCALLTYPE Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo);
    
    virtual HRESULT STDMETHODCALLTYPE CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pNodeSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo);

protected: // instance vars

    RNamespaceMgr _pNamespaceMgr;
};

extern HRESULT AbortParse(IXMLNodeSource * pNodeSource, Exception * e, Document * pDoc);

#endif _IE4NODEFACTORY_HXX
