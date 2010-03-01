#ifndef PRECOMP_H__
#define PRECOMP_H__

#define MSDVBNP_TRACE
#define BUILDING_KS
#define _KSDDK_
#include <dshow.h>
//#include <streams.h>
#include <ks.h>
#define __STREAMS__
#include <ksproxy.h>
#include <stdio.h>
#include <wchar.h>
#include <tchar.h>
#include <uuids.h>
#include <bdatypes.h>
#include <bdaiface.h>
#include <bdamedia.h>
#include <tuner.h>
#include <assert.h>

typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObject);

typedef struct
{
    const GUID* riid;
    LPFNCREATEINSTANCE lpfnCI;
} INTERFACE_TABLE;

/* classfactory.cpp */
IClassFactory *
CClassFactory_fnConstructor(
    LPFNCREATEINSTANCE lpfnCI,
    PLONG pcRefDll,
    IID * riidInst);

/* networkprovider.cpp */
HRESULT
WINAPI
CNetworkProvider_fnConstructor(
    IUnknown *pUnknown,
    REFIID riid,
    LPVOID * ppv);

/* scanningtunner.cpp */
HRESULT
WINAPI
CScanningTunner_fnConstructor(
    IUnknown *pUnknown,
    REFIID riid,
    LPVOID * ppv);


#endif
