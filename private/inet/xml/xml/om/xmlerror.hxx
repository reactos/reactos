/*
 * @(#)XMLError.hxx 1.0 11/17/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _XML_OM_XMLERROR
#define _XML_OM_XMLERROR

class Document;

class IXMLErrorWrapper : public _comexport<Document, IXMLError, &IID_IXMLError>
{
    public: 
      IXMLErrorWrapper(Document * p)
                 : _comexport<Document, IXMLError, &IID_IXMLError>(p) 
            {}

    public: virtual HRESULT STDMETHODCALLTYPE GetErrorInfo( 
        /* [retval][out] */ XML_ERROR *pError);

};

#endif _XML_OM_XMLERROR

