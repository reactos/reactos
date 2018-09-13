/*
 * @(#)_reference.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

void _assign(IUnknown ** ppref, IUnknown * pref)
{
    IUnknown *punkRef = *ppref;
    if (pref) ((Object *)pref)->AddRef();
    (*ppref) = (Object *)pref; 
    if (punkRef) punkRef->Release();
}    

void _release(IUnknown ** ppref)
{
    if (*ppref) 
    {
        (*ppref)->Release();
        *ppref = NULL;
    }
}
