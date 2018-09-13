/*
 * @(#)IDOMError.hxx 1.0
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XML_OM_DOMERROR_HXX
#define _XML_OM_DOMERROR_HXX

#include "xmldom.hxx"

class Exception;
DEFINE_CLASS(String);

class DOMError : public _dispatchEx<IXMLDOMParseError, &LIBID_MSXML, &IID_IXMLDOMParseError, false>
{
public:
    DOMError(Exception* e);
    ~DOMError() { _pException = NULL; };

    ////////////////////////////////////////////////////////////////////////////////
    // IXMLDOMError Interface
    //
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_errorCode( 
        /* [out][retval] */ LONG __RPC_FAR *plErrorCode);

    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_url( 
        /* [out][retval] */ BSTR __RPC_FAR *pbstrUrl);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_reason( 
        /* [out][retval] */ BSTR __RPC_FAR *pbstrReason);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_srcText( 
        /* [out][retval] */ BSTR __RPC_FAR *pbstrText);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_line( 
        /* [out][retval] */ long __RPC_FAR *plLine);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_linepos( 
        /* [out][retval] */ long __RPC_FAR *plLinePos);
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_filepos( 
        /* [out][retval] */ long __RPC_FAR *plFilePos);

    // helper functions for Invoke()
    static INVOKEFUNC _invoke;

protected:

    _reference<Exception> _pException;
};

#endif _XML_OM_DOMERROR_HXX