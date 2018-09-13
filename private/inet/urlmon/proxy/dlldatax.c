//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       dlldatax.c
//
//  Contents:   wrapper for dlldata.c
//
//  Classes:
//
//  Functions:
//
//  History:    1-08-96   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#define PROXY_CLSID CLSID_PSUrlMonProxy

#define DllMain             PrxDllMain
#define DllGetClassObject   PrxDllGetClassObject
#define DllCanUnloadNow     PrxDllCanUnloadNow
#define DllRegisterServer   PrxDllRegisterServer
#define DllUnregisterServer PrxDllUnregisterServer

#include "dlldata.c"

