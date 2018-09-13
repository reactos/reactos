/*
 * @(#)hresult.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop 

// check hr and throw exception if necessary

void checkhr(HRESULT hr)
{
    if (hr != S_OK)
        Exception::throwE(hr);
}

// check hr and throw exception if necessary

HRESULT succeeded(HRESULT hr)
{
    if (!SUCCEEDED(hr))
        Exception::throwE(hr);
    return hr;
}

