/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _DSOCTRL_HXX
#define _DSOCTRL_HXX

#ifndef _REFERENCE_HXX
#include "_reference.hxx"
#endif

#ifndef _XMLDSO_HXX
#include "xmldso.hxx"
#endif

#ifndef _DISPATCH_HXX
#include "core/com/_dispatch.hxx"
#endif

//---------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CreateDSOControl(REFIID iid, void **ppvObj);

//---------------------------------------------------------------
// The XMLDSOControl is a wrapper on the DSO that implements the
// IXMLDSOControl interface.

class CXMLDSOControl : public _dispatchexport<XMLDSO, IXMLDSOControl, &LIBID_MSXML, ORD_MSXML, &IID_IXMLDSOControl>
{
    typedef _dispatchexport<XMLDSO, IXMLDSOControl, &LIBID_MSXML, ORD_MSXML, &IID_IXMLDSOControl> super;
public:
    CXMLDSOControl(XMLDSO * p);
    ~CXMLDSOControl() { }

    ///////////////////////////////////////////////////////////////////
    // IXMLDSOControl methods
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_XMLDocument( 
        /* [retval][out] */ IXMLDOMDocument __RPC_FAR *__RPC_FAR *ppDoc);
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_XMLDocument( 
        /* [in] */ IXMLDOMDocument __RPC_FAR *ppDoc);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_JavaDSOCompatible( 
        /* [retval][out] */ BOOL __RPC_FAR *fJavaDSOCompatible);
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_JavaDSOCompatible( 
        /* [in] */ BOOL fJavaDSOCompatible);
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_readyState( 
        /* [retval][out] */ long __RPC_FAR *state);

    ///////////////////////////////////////////////////////////////////
    // internal helpers
private:
};

typedef _reference<CXMLDSOControl> RXMLDSOControl;

#endif
