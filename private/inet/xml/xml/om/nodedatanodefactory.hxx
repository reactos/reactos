/*
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _XML_OM_NODEDATANODEFACTORY
#define _XML_OM_NODEDATANODEFACTORY

class Node;
class NodeSlotManager;

#include <xmlparser.h>

DEFINE_CLASS( ElementNode);
DEFINE_CLASS( Document);
class ValidationFactory;

#define XMLSPACE_PARENT_FLAG 2
#define XMLSPACE_DEFAULT_DEFINED 4
#define XMLSPACE_DEFAULT_PRESERVE 8

// defined in IE4NodeFactory.cxx
bool ProcessXmlSpace(Node * pNode, bool fIE4);

class NOVTABLE _NDNodeFactory : public _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>
{
public: // (De)Constructors
    _NDNodeFactory( Document * pDocument);
    ~_NDNodeFactory();

protected: // internal methods
    void bufferAttach();
    bool bufferAppend(Node* pParent, const WCHAR *pwc, ULONG ulLen);

protected: // instance vars
    void bufferAppend(const WCHAR *pwc, ULONG ulLen);

    WDocument _pDocument;
    RNodeManager _pNodeMgr;

    // xml:space
    bool _fPreserveWhiteSpace;
    bool _fForcePreserveWhiteSpace;

    // text/append buffer
    Node * _pLastTextNode;
    bool _fTextInBuffer;
    bool _fCollapsed;
    RAWCHAR _pBuffer;
    ULONG _ulBufUsed;
};

class NodeDataNodeFactory : public _NDNodeFactory
{
public: // (De)Constructors
    NodeDataNodeFactory( Document * pDocument) 
        : _NDNodeFactory(pDocument)
        {}
    ~NodeDataNodeFactory() 
        {}

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

protected:
    bool _fXMLSpaceAttr;
    bool _fXMLSpaceAttrValue;
    PVOID _pRoot; 
    bool _fInsideEntity;    // set true when any element/attribute
                            // creation is inside and entity
    _reference<ValidationFactory> _pValidationNF;
#if DBG == 1
    long _cDepth;
#endif
};

#endif _XML_OM_NODEDATANODEFACTORY
