#ifndef PRECOMP_H__
#define PRECOMP_H__

#define MSVIDCTL_TRACE
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

/* tuningspace_container.cpp */
HRESULT
WINAPI
CTuningSpaceContainer_fnConstructor(
    IUnknown *pUnknown,
    REFIID riid,
    LPVOID * ppv);

/* tuningspace.cpp */
HRESULT
WINAPI
CTuningSpace_fnConstructor(
    IUnknown *pUnknown,
    REFIID riid,
    LPVOID * ppv);

/* tunerequest.cpp */
HRESULT
WINAPI
CTuneRequest_fnConstructor(
    IUnknown *pUnknown,
    ITuningSpace * TuningSpace,
    REFIID riid,
    LPVOID * ppv);

/* enumtuningspaces.cpp */
HRESULT
WINAPI
CEnumTuningSpaces_fnConstructor(
    IUnknown *pUnknown,
    REFIID riid,
    LPVOID * ppv);



#endif
