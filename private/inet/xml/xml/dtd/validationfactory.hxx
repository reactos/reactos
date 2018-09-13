/*
 * @(#)ValidationFactory.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _VALIDATIONFACTORY_HXX
#define _VALIDATIONFACTORY_HXX

#ifndef _XML_PARSER_DTD
#include "dtd.hxx"
#endif

#ifndef _DTDSTATE_HXX
#include "dtdstate.hxx"
#endif

#ifndef _ATTDEF_HXX
#include "attdef.hxx"
#endif

#ifndef _ELEMENTDECL_HXX
#include "elementdecl.hxx"
#endif

#ifndef _XML_OM_NODE_HXX
#include "node.hxx"
#endif

#include <xmlparser.h>

typedef _reference<IXMLNodeFactory> RNodeFactory;
typedef _reference<IUnknown> RUnknown;
typedef _reference<Node> RNode;

///////////////////////////////////////////////////////////////////////
// ValidationFactory class
//

class ValidationFactory : public _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>
{
protected:
    ValidationFactory() {};

public: 
    ValidationFactory(IXMLNodeFactory * fc, DTD* dtd, Document* doc);
    virtual ~ValidationFactory();

	HRESULT STDMETHODCALLTYPE NotifyEvent( 
			/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
			/* [in] */ XML_NODEFACTORY_EVENT iEvt);

    HRESULT STDMETHODCALLTYPE BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);

    HRESULT STDMETHODCALLTYPE EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ BOOL fEmptyNode,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);
    
    HRESULT STDMETHODCALLTYPE Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
    {
        _fNotifiedError = true;
        return _pFactory->Error(pSource, hrErrorCode, cNumRecs, apNodeInfo);
    }
    
    HRESULT STDMETHODCALLTYPE CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo);

public:
    ElementDecl * getElemDecl() const
    {
        return ((null != (DTDState*)_pCurrent) ? (ElementDecl*)_pCurrent->ed : (ElementDecl*)null);
    }

    AttDef * getAttDef() const
    {
        return _pAttDef;
    }

protected:

    void            init();    
    HRESULT         CheckPending(IXMLNodeSource  *pSource);
    void            CheckAttribute(NameDef* namedef);
    void            CheckGlobalAttribute(NameDef* namedef);

    // These are the main states that the validate node factory goes through.
    enum EState {
                eValidating,    // doing validation
                eAttribute,     // validating an an attribute value.
                ePending,
                eSchema,        // finished prolog and there's no DTD validation needed - except entity checking
    } _nState;

    RNodeFactory _pFactory;         // IXMLNodeFactory to delegate calls
    RDTD         _pDTD;             // The DTD to build & validate against.
    RStack       _pContexts;        // verifying context 
    RDTDState    _pCurrent;         // current context pointer
    RObject      _pCurrentElement;
    RAttDef      _pAttDef;          // attribute name

    bool         _fNotifiedError;        // whether nofitied error
    bool         _fInAttribute;
    bool         _fInPI;
    bool         _fValidateRoot;
    bool         _fForcePreserveWhiteSpace;
    RNode        _pNodePending;     // pending validation because of schema download.
    WDocument    _pDoc;
};


#endif _VALIDATIONFACTORY_HXX