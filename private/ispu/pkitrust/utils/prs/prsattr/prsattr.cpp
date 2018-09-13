//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       prsattr.cpp
//
//  Contents:   Microsoft Internet Security Internal Utility
//
//  Functions:  DllMain
//              DllRegisterServer
//              DLLUnregisterServer
//              InitAttr
//              GetAttr
//              ReleasseAttr
//              ExitAttr
//
//  History:    16-Sep-1997 pberkman   created
//
//--------------------------------------------------------------------------


#include "global.hxx"
#include <stdio.h>

HINSTANCE   hinst = NULL;

BOOL WINAPI DllMain(HANDLE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    hinst = (HINSTANCE)hInstDLL;

    return(TRUE);
}

STDAPI DllRegisterServer(void)
{
    return(S_OK);
}

STDAPI DllUnregisterServer(void)
{
    return(S_OK);
}

HRESULT WINAPI InitAttr(LPWSTR pwszInitString)
{
	return(S_OK);
}

HRESULT WINAPI GetAttr(PCRYPT_ATTRIBUTES *ppsAuthenticated,		
					   PCRYPT_ATTRIBUTES *ppsUnauthenticated)
{
   if (!(ppsAuthenticated) || !(ppsUnauthenticated))
   {
       return(ERROR_INVALID_ARGUMENT);
   }

   if (!(*ppsAuthenticated = (PCRYPT_ATTRIBUTES)new BYTE[sizeof(CRYPT_ATTRIBUTES)]))
   {
       return(ERROR_NOT_ENOUGH_MEMORY);
   }

   memset(*ppsAthenticated, 0x00, sizeof(CRYPT_ATTRIBUTES));

   return(ERROR_SUCCESS);
}

HRESULT WINAPI ReleaseAttr(PCRYPT_ATTRIBUTES  psAuthenticated,		
			               PCRYPT_ATTRIBUTES  psUnauthenticated)
{
    if (psAuthenticated)
    {
        delete psAuthentcated;
    }

    if (psUnauthenticated)
    {
        delete psUnauthenticated;
    }

    return(ERROR_SUCCESS);
}


HRESULT	WINAPI ExitAttr(void)
{
	return(ERROR_SUCCESS);

}


	

