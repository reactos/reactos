/*
 * @(#)IDOMImplementation.hxx 1.0
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XML_OM_DOMIMPLEMENTATION_HXX
#define _XML_OM_DOMIMPLEMENTATION_HXX

class DOMImplementation : public _dispatchEx<IXMLDOMImplementation, &LIBID_MSXML, &IID_IXMLDOMImplementation, false>
{
public:

    /////////////////////////////////////////////////////////
    // Constructor & Destructor
    DOMImplementation();
    ~DOMImplementation() {};

    /////////////////////////////////////////////////////////
    // IDOMImplemetation interfaces

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE hasFeature( 
        /* [in] */ BSTR feature,
        /* [in] */ BSTR version,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *hasFeature);   
};

#endif //_XML_OM_DOMIMPLEMENTATION_HXX
