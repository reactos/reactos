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
#include <ksmedia.h>
#include <stdio.h>
#include <wchar.h>
#include <tchar.h>
#include <uuids.h>
#include <bdatypes.h>
#include <bdaiface.h>
#include <bdatif.h>
#include <bdamedia.h>
#include <tuner.h>
#include <assert.h>
#include <vector>

typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObject);

typedef struct
{
    const GUID* riid;
    LPFNCREATEINSTANCE lpfnCI;
    LPCWSTR ProviderName;
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
    std::vector<IUnknown*> & m_DeviceFilter,
    REFIID riid,
    LPVOID * ppv);

/* enumpins.cpp */
HRESULT
WINAPI
CEnumPins_fnConstructor(
    IUnknown *pUnknown,
    ULONG NumPins,
    IPin ** pins,
    REFIID riid,
    LPVOID * ppv);

/* pin.cpp */
HRESULT
WINAPI
CPin_fnConstructor(
    IUnknown *pUnknown,
    IBaseFilter * ParentFilter,
    REFIID riid,
    LPVOID * ppv);

/* enum_mediatypes.cpp */
HRESULT
WINAPI
CEnumMediaTypes_fnConstructor(
    IUnknown *pUnknown,
    ULONG MediaTypeCount,
    AM_MEDIA_TYPE * MediaTypes,
    REFIID riid,
    LPVOID * ppv);

/* ethernetfilter.cpp */
HRESULT
WINAPI
CEthernetFilter_fnConstructor(
    IBDA_NetworkProvider * pNetworkProvider,
    REFIID riid,
    LPVOID * ppv);

/* ipv6.cpp */
HRESULT
WINAPI
CIPV6Filter_fnConstructor(
    IBDA_NetworkProvider * pNetworkProvider,
    REFIID riid,
    LPVOID * ppv);

/* ipv4.cpp */
HRESULT
WINAPI
CIPV4Filter_fnConstructor(
    IBDA_NetworkProvider * pNetworkProvider,
    REFIID riid,
    LPVOID * ppv);

#ifndef _MSC_VER
extern const GUID CLSID_DVBTNetworkProvider;
#endif

#endif
