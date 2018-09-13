#ifndef I_DOM_HXX_
#define I_DOM_HXX_
#pragma INCMSG("--- Beg 'dom.hxx'")

#define _hxx_
#include "dom.hdl"

MtExtern(CAttribute)
MtExtern(CDOMTextNode)

class CElement;

//+------------------------------------------------------------
//
//  Class : CAttribute
//
//-------------------------------------------------------------

class CAttribute : public CBase
{
    DECLARE_CLASS_TYPES(CAttribute, CBase)
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CAttribute))

    CAttribute(const PROPERTYDESC * const *ppPropDesc, CElement *pElem);
    ~CAttribute();

    // IUnknown and IDispatch
    DECLARE_PLAIN_IUNKNOWN(CAttribute)
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    // IHTMLDOMAttribute methods
    #define _CAttribute_
    #include "dom.hdl"

    const PROPERTYDESC * const *_ppPropDesc;
    CElement *_pElem;

protected:
    DECLARE_CLASSDESC_MEMBERS;
};

//+------------------------------------------------------------
//
//  Class : CDOMTextNode
//
//-------------------------------------------------------------

class CDOMTextNode : public CBase
{
    DECLARE_CLASS_TYPES(CDOMTextNode, CBase)
protected:
    CDoc *_pDoc;
    long _lTextID;
    CMarkupPointer *_pMkpPtr;
    CDOMChildrenCollection *EnsureDOMChildrenCollectionPtr();
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDOMTextNode))

    HRESULT MoveTo ( CMarkupPointer *pmkptrTarget );
    HRESULT GetMarkupPointer( CMarkupPointer **ppMkp );
    HRESULT Remove();
    CDoc *Doc() { return _pDoc; }
    CMarkupPointer *MarkupPtr() { return _pMkpPtr; }

    CDOMTextNode ( long lTextID, CDoc *pDoc, CMarkupPointer *pmkptr );
    ~CDOMTextNode () ;
    // IUnknown and IDispatch
    DECLARE_PLAIN_IUNKNOWN(CDOMTextNode)
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    // IHTMLDOMTextNode methods
    #define _CDOMTextNode_
    #include "dom.hdl"

    // IHTMLDOMNode methods
	NV_DECLARE_TEAROFF_METHOD(get_nodeType, GET_nodeType, (long*p));
	NV_DECLARE_TEAROFF_METHOD(get_parentNode, GET_parentNode, (IHTMLDOMNode**p));
	NV_DECLARE_TEAROFF_METHOD_(HRESULT, hasChildNodes, haschildnodes, (VARIANT_BOOL*p));
	NV_DECLARE_TEAROFF_METHOD(get_childNodes, GET_childNodes, (IDispatch**p));
	NV_DECLARE_TEAROFF_METHOD(get_attributes, GET_attributes, (IDispatch**p));
	NV_DECLARE_TEAROFF_METHOD_(HRESULT, insertBefore, insertbefore, (IHTMLDOMNode* newChild,VARIANT refChild, IHTMLDOMNode**));
	NV_DECLARE_TEAROFF_METHOD_(HRESULT, removeChild, removechild, (IHTMLDOMNode* oldChild,IHTMLDOMNode**));
	NV_DECLARE_TEAROFF_METHOD_(HRESULT, replaceChild, replacechild, (IHTMLDOMNode* newChild,IHTMLDOMNode* oldChild,IHTMLDOMNode**));
	NV_DECLARE_TEAROFF_METHOD_(HRESULT, cloneNode, clonenode, (VARIANT_BOOL fDeep,IHTMLDOMNode** ppnewNode));
	NV_DECLARE_TEAROFF_METHOD_(HRESULT, removeNode, removenode, (VARIANT_BOOL fDeep,IHTMLDOMNode** ppnewNode));
	NV_DECLARE_TEAROFF_METHOD_(HRESULT, swapNode, swapnode, ( IHTMLDOMNode *pNode, IHTMLDOMNode** ppnewNode));
	NV_DECLARE_TEAROFF_METHOD_(HRESULT, replaceNode, replacenode, ( IHTMLDOMNode *pNode, IHTMLDOMNode** ppnewNode));
	NV_DECLARE_TEAROFF_METHOD_(HRESULT, appendChild, appendchild, (IHTMLDOMNode* newChild,IHTMLDOMNode**));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, get_nodeName, GET_nodename, (BSTR*));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, get_nodeValue, GET_nodevalue, (VARIANT*));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, put_nodeValue, PUT_nodevalue, (VARIANT));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, get_firstChild, GET_firstchild, (IHTMLDOMNode**));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, get_lastChild, GET_lastchild, (IHTMLDOMNode**));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, get_previousSibling, GET_previoussibling, (IHTMLDOMNode**));
    NV_DECLARE_TEAROFF_METHOD_(HRESULT, get_nextSibling, GET_nextsibling, (IHTMLDOMNode**));

    // IObjectIdentity Methods
    NV_DECLARE_TEAROFF_METHOD(IsEqualObject, isequalobject, (IUnknown *ppunk));

protected:
    DECLARE_CLASSDESC_MEMBERS;
};

#pragma INCMSG("--- End 'dom.hxx'")
#else
#pragma INCMSG("*** Dup 'dom.hxx'")
#endif
