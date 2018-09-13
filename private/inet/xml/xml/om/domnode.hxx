/*
 * @(#)IXMLDOMNode.hxx 1.0 2/25/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XML_OM_IXMLDOMNODE
#define _XML_OM_IXMLDOMNODE

#ifndef _XML_OM_NODE_HXX
#include "xml/om/node.hxx"
#endif

#ifndef _XML_OM_W3CDOM_HXX
#include "xml/om/w3cdom.hxx"
#endif

#ifndef _ENUMVARIANT_HXX
#include "enumvariant.hxx"
#endif

DEFINE_CLASS(Document);
DEFINE_CLASS(Hashtable);
class DOMNode;
class Enumeration;


class DOMNode : public _dispatchEx<IXMLDOMNode, &LIBID_MSXML, &IID_IXMLDOMNode, false>
{
    friend class W3CDOMWrapper;
    friend class DOMNodeLock;
    friend class DOMNodeReadLock;
    friend class DOMNodeUpgradableLock;
    friend class NodeDataNodeFactory;
    friend class Node;

    // {6D7FC382-20BF-11d2-836E-0000F87A7782}
    public: static const IID s_IID;

    // DOMNode class methods
    
    public: DOMNode(Node * pNode);

    protected: virtual ~DOMNode();

    public: static DOMNode * newDOMNode(Node * pNode);
    public: static void deleteDOMNode(DOMNode * pDOMNode);
    public: static void classExit();

    protected: void init(Node * pNode);

    protected: void reset();

    public: virtual ULONG STDMETHODCALLTYPE Release();

    // never use empty constructor
    private: DOMNode() { Assert("shouldn't be used"); }

    public: Node * getNodeData()
    {
        return _pNode;
    }

    protected: Document * getDocument();

    // make sure the passed IXMLDOMNode object is a DOMNode.. returning null if not
    public: static DOMNode * Variant2DOMNode( VARIANT *);

    public: bool xmlSpacePreserve();
    
public:

    // IUnknown methods
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **);

    // IDispatch methods

    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( 
        /* [in] */ REFIID riid,
        /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
        /* [in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Invoke( 
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
        /* [out] */ VARIANT __RPC_FAR *pVarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
        /* [out] */ UINT __RPC_FAR *puArgErr);

    // IDispatchEx methods

    HRESULT STDMETHODCALLTYPE GetDispID( 
        /* [in] */ BSTR bstrName,
        /* [in] */ DWORD grfdex,
        /* [out] */ DISPID __RPC_FAR *pid);
    
    // IXMLDOMNode methods

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeType( 
        /* [out][retval] */ DOMNodeType __RPC_FAR *plType);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeName( 
        /* [out][retval] */ BSTR __RPC_FAR *pName);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeValue( 
        /* [out][retval] */ VARIANT __RPC_FAR *pVal);
    
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_nodeValue( 
        /* [in] */ VARIANT bstrVal);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_parentNode( 
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_childNodes( 
        /* [out][retval] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppNodeList);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_firstChild( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppFirstChild);

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_lastChild( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppLastChild);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_previousSibling( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppPrevSib);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nextSibling( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNextSib);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE insertBefore( 
        /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
        /* [in] */ VARIANT refChild,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *outNewChild);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE replaceChild( 
        /* [in] */ IXMLDOMNode __RPC_FAR *pNew,
        /* [in] */ IXMLDOMNode __RPC_FAR *pOld,
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE removeChild( 
        /* [in] */ IXMLDOMNode __RPC_FAR *oldChild,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppOldChild);
    
    virtual HRESULT STDMETHODCALLTYPE appendChild( 
        /* [in] */ IXMLDOMNode __RPC_FAR *newChild,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNewChild);
    
    virtual HRESULT STDMETHODCALLTYPE get_attributes( 
        /* [out][retval] */ IXMLDOMNamedNodeMap __RPC_FAR *__RPC_FAR *ppAttributes);

    virtual HRESULT STDMETHODCALLTYPE hasChildNodes( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pBool);
    
    virtual HRESULT STDMETHODCALLTYPE cloneNode( 
        /* [in] */ VARIANT_BOOL deep,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppCloneRoot);    
        
    virtual  HRESULT STDMETHODCALLTYPE get_ownerDocument( 
        /* [retval][out] */ IXMLDOMDocument __RPC_FAR *__RPC_FAR *DOMDocument);
    
    ////////////////////////////////////////////////
    // IXMLDOMNode

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeTypeString( 
        /* [out][retval] */ BSTR __RPC_FAR *nodeType);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_text( 
        /* [out][retval] */ BSTR __RPC_FAR *text);
    
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_text( 
        /* [in] */ BSTR text);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_specified( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool);

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_definition( 
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_nodeTypedValue( 
        /* [out][retval] */ VARIANT __RPC_FAR *TypedValue);
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_nodeTypedValue( 
        /* [in] */ VARIANT TypedValue);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_dataType( 
        /* [out][retval] */ VARIANT __RPC_FAR *p);
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_dataType( 
        /* [in] */ BSTR p);

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_xml(
        /* [out][retval] */ BSTR __RPC_FAR *pbstrXml);

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE transformNode( 
        /* [in] */ IXMLDOMNode __RPC_FAR *pStyleSheet,
        /* [out][retval] */ BSTR __RPC_FAR *pbstr);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE selectNodes( 
        /* [in] */ BSTR bstrXQL,
        /* [out][retval] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppResult);
        
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE selectSingleNode( 
        /* [in] */ BSTR queryString,
        /* [out][retval] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *resultNode);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_parsed( 
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *isParsed);

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_namespaceURI( 
        /* [out][retval] */ BSTR __RPC_FAR *pURI);

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_prefix( 
        /* [out][retval] */ BSTR __RPC_FAR *pPrefix);

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_baseName( 
        /* [out][retval] */ BSTR __RPC_FAR *pBaseName);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE transformNodeToObject( 
        /* [in] */ IXMLDOMNode __RPC_FAR *pXDNStylesheet,
        /* [in] */ VARIANT varOutputObject);


public:

    W3CDOMWrapper * getW3CWrapper();

    HRESULT STDMETHODCALLTYPE getElementsByTagName( 
        /* [in] */ BSTR tagname,
        /* [retval][out] */ IXMLDOMNodeList __RPC_FAR *__RPC_FAR *ppNodeList);

    // helper functions for Invoke()
    static INVOKEFUNC _invokeDOMNode;
    static INVOKEFUNC _invokeDOMElement;

private:
    Node *                              _pNode;
    W3CDOMWrapper *               _pW3CWrapper;

protected:

    typedef IUnknown * (W3CWrapperCastFunc)(W3CDOMWrapper *);

    struct DispInfo
    {
        DISPATCHINFO dispatchinfo;
        W3CWrapperCastFunc * func;
    };

    static DispInfo aDispInfo[(int)Node::XMLDECL+1];

#define DOMNODECACHESIZE    4

    private: static DOMNode * _apDOMNodes[DOMNODECACHESIZE];
#ifdef RENTAL_MODEL
    private: static DOMNode * _apDOMNodesRental[DOMNODECACHESIZE];
#endif
};


class NOVTABLE DOMNodeList : public EnumVariant
{
    public: DOMNodeList(Node * node);
    protected: virtual ~DOMNodeList();

#if DBG==1
    // never use empty constructor
    protected: DOMNodeList() { Assert("shouldn't be used"); }
#endif

public:
    ////////////////////////////////////////////////////////////////////////////////
    // IXMLDOMNodeList Interface

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE get_item( 
        /* [in] */ long index,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_length( 
        /* [retval][out] */ long __RPC_FAR *pSize);

    ////////////////////////////////////////////////////////////////////////////////
    // IXMLDOMNodeList Interface

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE nextNode( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE reset( void);
    
    ////////////////////////////////////////////////////////////////////////////////
    // EnumVariant Interface

    virtual IDispatch * enumGetIDispatch(Node * pNode) 
    { 
        return pNode ? (IXMLDOMNode *)pNode->getDOMNodeWrapper() : null; 
    }
    virtual Node * enumGetNext(void ** ppv) 
    { 
        return _next(_pParent, ppv); 
    }
    virtual bool enumValidate(void * pv)
    {
        // when ((DWORD_PTR)ppv & 1) != 0, the current node must be a default attribute node
        return ((((DWORD_PTR)pv & 1) != 0) ||
                _pParent->validateHandle(&pv));
    }

protected:
    HRESULT get__newEnum(IUnknown *pUnk, IUnknown **ppUnk);
    // helper method to find next
    virtual Node * _next(Node * pParentNode, void ** ppv) = 0;

    _reference<Node>                    _pParent;
    NodeIteratorState                   _State;
};


#ifdef UNIX
// This is a Apogee (HP and Solaris Unix) compiler bug workaround!
// If you change it you may totally break XLM under Unix
class NOVTABLE _DOMList : public _dispatchEx<IXMLDOMNodeList, &LIBID_MSXML, &IID_IXMLDOMNodeList, true>
{
public: _DOMList();

    HRESULT STDMETHODCALLTYPE GetDispID( 
        /* [in] */ BSTR bstrName,
        /* [in] */ DWORD grfdex,
        /* [out] */ DISPID __RPC_FAR *pid);


};
#endif // UNIX


#ifdef UNIX
// This is a Apogee (HP and Solaris Unix) compiler bug workaround!
// If you change it you may totally break XLM under Unix
class DOMChildList : public _DOMList, public DOMNodeList
#else
class DOMChildList : public _dispatchEx<IXMLDOMNodeList, &LIBID_MSXML, &IID_IXMLDOMNodeList, true>, public DOMNodeList
#endif
{
    public: DOMChildList(Node * node, Element::NodeType = Node::ANY);

#if DBG==1
    // never use empty constructor
    private: DOMChildList() { Assert("shouldn't be used"); }
#endif


public:
    // IUnknown methods

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **);

    ////////////////////////////////////////////////////////////////////////////////

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE get_item( 
        /* [in] */ long index,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
    {
        return DOMNodeList::get_item(index, ppNode);
    }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_length( 
        /* [retval][out] */ long __RPC_FAR *pSize)
    {
        return DOMNodeList::get_length( pSize);
    }

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE nextNode( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
    {
        return DOMNodeList::nextNode(ppNode);
    }
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE reset( void)
    {
        return DOMNodeList::reset();
    }

    virtual /* [id][hidden][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__newEnum( 
        /* [out][retval] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk)
    {
        return DOMNodeList::get__newEnum((IXMLDOMNodeList*)this, ppUnk);
    }

    // helper function for Invoke()
    static INVOKEFUNC _invoke;

protected:

    // helper method to find next
    Node * _next(Node * pParentNode, void ** ppv);

    bool                                _fFilter;
    Element::NodeType                   _eNodeTypeFilter;
};


class DOMNamedNodeMapList : public _dispatchEx<IXMLDOMNamedNodeMap, &LIBID_MSXML, &IID_IXMLDOMNamedNodeMap, true>, public DOMNodeList
{
    public: DOMNamedNodeMapList(Node * node, Element::NodeType eType);

#if DBG==1
    // never use empty constructor
    private: DOMNamedNodeMapList() { Assert("shouldn't be used"); }
#endif

public:
    // IUnknown methods

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **);

    ////////////////////////////////////////////////////////////////////////////////

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE get_item( 
        /* [in] */ long index,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
    {
        return DOMNodeList::get_item(index, ppNode);
    }
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_length( 
        /* [retval][out] */ long __RPC_FAR *pSize)
    {
        return DOMNodeList::get_length( pSize);
    }

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE nextNode( 
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode)
    {
        return DOMNodeList::nextNode(ppNode);
    }
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE reset( void)
    {
        return DOMNodeList::reset();
    }

    virtual /* [id][hidden][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__newEnum( 
        /* [out][retval] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk)
    {
        return DOMNodeList::get__newEnum((IXMLDOMNamedNodeMap*)this, ppUnk);
    }
    ////////////////////////////////////////////////////////////////////////////////
    // IXMLDOMNamedNodeMap Interface

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getNamedItem( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE setNamedItem( 
        /* [in] */ IXMLDOMNode __RPC_FAR *arg,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE removeNamedItem( 
        /* [in] */ BSTR name,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);

    ////////////////////////////////////////////////////////////////////////////////
    // IXMLDOMNamedNodeMap Interface

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE getQualifiedItem( 
        /* [in] */ BSTR name,
        /* [in] */ BSTR nameSpaceURN,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE removeQualifiedItem( 
        /* [in] */ BSTR name,
        /* [in] */ BSTR NameSpace,
        /* [retval][out] */ IXMLDOMNode __RPC_FAR *__RPC_FAR *ppNode);

    // helper function for Invoke()
    static INVOKEFUNC _invoke;

protected:

    // helper method to find next
    Node * _next(Node * pParentNode, void ** ppv);

    Element::NodeType _eNodeType;
};


#endif // _XML_OM_IXMLDOMNODE

