/*
 * @(#)msxmlCOM.hxx 1.0 6/3/97
 * 
 * Implementation of msxml2.idl interfaces.
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XML_OM_MSXMLCOM
#define _XML_OM_MSXMLCOM

#include "document.hxx"

#ifndef _ENUMVARIANT_HXX
#include "enumvariant.hxx"
#endif

class IDocumentWrapper : public _dispatchexport<Document, IXMLDocument2, &LIBID_MSXML, ORD_MSXML, &IID_IXMLDocument2>
{
    public: IDocumentWrapper(Document * p);

    public: virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppv);

    public: virtual ULONG STDMETHODCALLTYPE Release();

    //========================================================================= 
    // Implementation of the IXMLDocument2 interface.

    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_root( 
        /* [out][retval] */ IXMLElement2 __RPC_FAR *__RPC_FAR *p);

    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_root( 
        /* [in] */ IXMLElement2 __RPC_FAR *p);
        
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_fileSize( 
        /* [out][retval] */ BSTR __RPC_FAR *p);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_fileModifiedDate( 
        /* [out][retval] */ BSTR __RPC_FAR *p);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_fileUpdatedDate( 
        /* [out][retval] */ BSTR __RPC_FAR *p);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_URL( 
        /* [out][retval] */ BSTR __RPC_FAR *p);
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_URL( 
        /* [in] */ BSTR p);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_mimeType( 
        /* [out][retval] */ BSTR __RPC_FAR *p);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_readyState( 
        /* [out][retval] */ long __RPC_FAR *pl);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_charset( 
        /* [out][retval] */ BSTR __RPC_FAR *p);
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_charset( 
        /* [in] */ BSTR p);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_version( 
        /* [out][retval] */ BSTR __RPC_FAR *p);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_doctype( 
        /* [out][retval] */ BSTR __RPC_FAR *p);
        
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_dtdURL( 
        /* [out][retval] */ BSTR __RPC_FAR *p);
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE createElement( 
        /* [in] */ VARIANT vType,
        /* [in][optional] */ VARIANT var1,
        /* [out][retval] */ IXMLElement2 __RPC_FAR *__RPC_FAR *ppElem);

    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_async( 
        /* [out][retval] */ VARIANT_BOOL __RPC_FAR *p);
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_async( 
        /* [in] */ VARIANT_BOOL p);

};


class IElementWrapper : public _dispatchexport<ElementNode, IXMLElement2, &LIBID_MSXML, ORD_MSXML, &IID_IXMLElement2>
{
    public: IElementWrapper(ElementNode * p);
    public: ~IElementWrapper();

    public: virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppv);

    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_tagName( 
        /* [out][retval] */ BSTR __RPC_FAR *p);
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_tagName( 
        /* [in] */ BSTR p);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_parent( 
        /* [out][retval] */ IXMLElement2 __RPC_FAR *__RPC_FAR *ppParent);
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE setAttribute( 
        /* [in] */ BSTR strPropertyName,
        /* [in] */ VARIANT PropertyValue);
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE getAttribute( 
        /* [in] */ BSTR strPropertyName,
        /* [out][retval] */ VARIANT __RPC_FAR *PropertyValue);
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE removeAttribute( 
        /* [in] */ BSTR strPropertyName);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_children( 
        /* [out][retval] */ IXMLElementCollection __RPC_FAR *__RPC_FAR *pp);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_type( 
        /* [out][retval] */ long __RPC_FAR *plType);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_text( 
        /* [out][retval] */ BSTR __RPC_FAR *p);
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_text( 
        /* [in] */ BSTR p);
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE addChild( 
        /* [in] */ IXMLElement2 __RPC_FAR *pChildElem,
        long lIndex,
        long lReserved);
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE removeChild( 
        /* [in] */ IXMLElement2 __RPC_FAR *pChildElem);

    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_attributes( 
        /* [out][retval] */ IXMLElementCollection __RPC_FAR *__RPC_FAR *pp);

};

class ElementCollection : public _dispatch<IXMLElementCollection, &LIBID_MSXML, &IID_IXMLElementCollection>,
                          public EnumVariant
{
protected:

    ElementCollection();
    ~ElementCollection() { _pRoot = NULL; _pName = NULL; };

public:

    static ElementCollection * newElementCollection(Element * root, Name * tag = null, bool fEnumAttributes = false);
    
    // IUnknown methods
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **);

    // IXMLElementCollection methods

    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_length( 
        /* [in] */ long v);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_length( 
        /* [out][retval] */ long __RPC_FAR *p);
    
    virtual /* [id][hidden][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__newEnum( 
        /* [out][retval] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk);
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE item( 
        /* [in][optional] */ VARIANT var1,
        /* [in][optional] */ VARIANT var2,
        /* [out][retval] */ IDispatch __RPC_FAR *__RPC_FAR *ppDisp);

    ////////////////////////////////////////////////////////////////////////////////
    // EnumVariant Interface

    virtual IDispatch * enumGetIDispatch(Node *);
    virtual Node * enumGetNext(void ** ppv) { return nextNode(ppv); };
    virtual bool enumValidate(void * pv)
    {
        return CAST_TO(ElementNode*, _pRoot)->validateHandle(&pv);
    }

    Node * nextNode(void ** ppv);

protected:
     
    Node * _next(Node * pNode, void ** ppv, Name * tag);

    RElement            _pRoot;
    RName               _pName;
    bool                _fAttributes;
};


class IAttributeWrapper : public _dispatchexport<ElementNode, IXMLAttribute, &LIBID_MSXML, ORD_MSXML, &IID_IXMLAttribute>
{
    public: IAttributeWrapper(ElementNode * p);
    public: ~IAttributeWrapper();

    public: virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppv);

    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_name( 
        /* [out][retval] */ BSTR __RPC_FAR *n);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_value( 
        /* [out][retval] */ BSTR __RPC_FAR *v);

};

#endif _XML_OM_MSXMLCOM
