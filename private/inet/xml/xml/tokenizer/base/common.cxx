/*
 * @(#)common.hxx 1.0 2/28/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

WCHAR * StringDup(const WCHAR * s)
{
    if (s == NULL)
        return NULL;
    int l = StrLen(s);
    WCHAR * s1 = new_ne WCHAR[l + 1];
    if (s1 == NULL)
        return NULL;
    ::memcpy(s1, s, sizeof(WCHAR) * l);
    s1[l] = 0;
    return s1;
}

WCHAR * StringDupHR(const WCHAR * s, HRESULT * pHR)
{
    WCHAR * r = StringDup(s);
    if (!r)
        *pHR = E_OUTOFMEMORY;
    return r;
}

IUnknown* SafeRelease(IUnknown* p)
{
    if (p != NULL) 
    {
        p->Release(); 
        p = NULL;
    }
    return p;
}

