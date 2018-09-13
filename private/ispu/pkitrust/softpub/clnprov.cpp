//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       clnprov.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  SoftpubInitialize
//
//  History:    23-Jul-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"


HRESULT WINAPI SoftpubCleanup(CRYPT_PROVIDER_DATA *pProvData)
{
    return(ERROR_SUCCESS);
}
