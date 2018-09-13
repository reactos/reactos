//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       urlapi.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-25-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#ifndef _URLAPI_HXX_DEFINED_
#define _URLAPI_HXX_DEFINED_

//helper apis
STDAPI  CreateURLBinding(LPCWSTR lpszUrl, IBindCtx *pbc, IBinding **ppBdg);
STDAPI  RegisterAsyncBindCtx(LPBC pBC, IBindStatusCallback *pBSCb, DWORD reserved);

// Function prototypes for internal functions defined in this module
HRESULT InternalParseURL(LPBC pbc, LPCWSTR pszName, ULONG FAR * pchEaten,
            LPMONIKER FAR * ppmk);


#endif //_URLAPI_HXX_DEFINED_

