/*
 * @(#)domimplementation.cxx 1.0
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#include "domimplementation.hxx"

DISPATCHINFO _dispatch<IXMLDOMImplementation, &LIBID_MSXML, &IID_IXMLDOMImplementation>::s_dispatchinfo = 
{
    NULL, &IID_IXMLDOMImplementation, &LIBID_MSXML, ORD_MSXML
};

DOMImplementation::DOMImplementation() 
{
    // This must be here to correctly initialize s_dispatchinfo under Unix
};

HRESULT STDMETHODCALLTYPE 
DOMImplementation::hasFeature( 
        /* [in] */ BSTR feature,
        /* [in] */ BSTR version,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *hasFeature)
{
    HRESULT hr;

    if (feature && hasFeature)
    {
        *hasFeature = VARIANT_FALSE;
        if (0 == StrCmpIC(feature, _T("XML")))
        {
            if (!version || (0 == StrCmpC(version, _T("1.0"))))
                *hasFeature = VARIANT_TRUE;
        }
        else if (0 == StrCmpIC(feature, _T("DOM")))
        {
            if (!version || (0 == StrCmpC(version, _T("1.0"))))
                *hasFeature = VARIANT_TRUE;
        }
        else if (0 == StrCmpIC(feature, _T("MS-DOM")))
        {
            if (!version || (0 == StrCmpC(version, _T("1.0"))))
                *hasFeature = VARIANT_TRUE;
        }
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}