/*
 * @(#)islandshared.cxx 1.0 3/24/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * Common code for MSXML Data Islands
 * 
 */
#include "core.hxx"
#pragma hdrstop

#include "islandshared.hxx"

#define GETMEMBER_CASE_SENSITIVE    0x00000001
#define GETMEMBER_AS_SOURCE         0x00000002
#define GETMEMBER_ABSOLUTE          0x00000004


HRESULT
SetExpandoProperty(
    IHTMLElement *pElement,
    String *pstrName,
    VARIANT *pvarValue)
{
    IDispatch *pdispDoc = NULL;
    HRESULT hr;
    VARIANT_BOOL vbExpando;
    BSTR bstrAttribName = pstrName->getBSTR();

    Assert (NULL != pElement);

    // We need the document so that we can toggle its expando state if necessary
    hr = pElement->get_document(&pdispDoc);
    
    if (NULL != pdispDoc && SUCCEEDED(hr))
    {
        IHTMLDocument2 *pDoc = NULL;
        hr = pdispDoc->QueryInterface(IID_IHTMLDocument2, (LPVOID *)&pDoc);
        pdispDoc->Release();
        
        if (NULL != pDoc)
        {
            Assert(SUCCEEDED(hr));

            hr = pDoc->get_expando(&vbExpando);

            Assert (SUCCEEDED(hr) && "get_expando failed !");

            // On change the state of the expando property if necessary
            if (SUCCEEDED(hr) && VARIANT_TRUE != vbExpando)
            {
                hr = pDoc->put_expando(VARIANT_TRUE);
                Assert (SUCCEEDED(hr) && "put_expando failed !");
            }

            if (SUCCEEDED(hr))
            {
                // Attach attribute - it will be created if it doesn't exist
                hr = pElement->setAttribute(bstrAttribName, *pvarValue, GETMEMBER_CASE_SENSITIVE);

                Assert(SUCCEEDED(hr) && "Failed to set XMLDocument expando");

                // Reset the state of the expando property
                if (VARIANT_TRUE != vbExpando)
                {
                    hr = pDoc->put_expando(VARIANT_FALSE);
                    Assert(SUCCEEDED(hr));
                }
            }

            pDoc->Release();
        }
    }
    else
    {
        // Couldn't get the document
        Assert(FALSE);
        hr = E_FAIL;
    }

    SysFreeString(bstrAttribName);

    return hr;
}


HRESULT
GetExpandoProperty(
    IHTMLElement *pElement,
    String *pstrName,
    VARIANT *pvarValue)
{
    IDispatch *pdispDoc = NULL;
    HRESULT hr;
    VARIANT_BOOL vbExpando;
    BSTR bstrAttribName = pstrName->getBSTR();

    VariantInit(pvarValue);
    hr = pElement->getAttribute(bstrAttribName, 0, pvarValue);

    SysFreeString(bstrAttribName);

    return hr;
}
