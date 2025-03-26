#pragma once

#define WIN32_NO_STATUS
#define WIN32_LEAN_AND_MEAN

#define _FORCENAMELESSUNION
#define BUILDING_KS
#define _KSDDK_
#define KSPROXY_TRACE
#include <dshow.h>
//#include <streams.h>
#include <ks.h>
#define __STREAMS__
#include <ksproxy.h>
#include <devioctl.h>
#include <stdio.h>
//#include <wchar.h>
//#include <tchar.h>
//#include <uuids.h>
#include <dvp.h>
#include <vptype.h>
#include <vpconfig.h>
#include <setupapi.h>
#include <vector>
#include <stack>
#include <list>
#include <assert.h>
#include <ksmedia.h>
//#include <debug.h>

interface DECLSPEC_UUID("877E4351-6FEA-11D0-B863-00AA00A216A1") IKsClock;

#undef INTERFACE
#define INTERFACE IKsClock

DECLARE_INTERFACE_(IKsClock, IUnknown)
{
    STDMETHOD_(HANDLE, KsGetClockHandle)(
        THIS
    ) PURE;
};


typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObject);

typedef struct {
    const GUID* riid;
    LPFNCREATEINSTANCE lpfnCI;
} INTERFACE_TABLE;

/* classfactory.cpp */

IClassFactory *
CClassFactory_fnConstructor(
    LPFNCREATEINSTANCE lpfnCI,
    PLONG pcRefDll,
    IID * riidInst);

/* datatype.cpp */
HRESULT
WINAPI
CKsDataTypeHandler_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv);

/* interface.cpp */
HRESULT
WINAPI
CKsInterfaceHandler_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv);

/* clockforward.cpp */
HRESULT
WINAPI
CKsClockForwarder_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv);

/* qualityforward.cpp */
HRESULT
WINAPI
CKsQualityForwarder_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv);

/* cvpconfig.cpp */
HRESULT
WINAPI
CVPConfig_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv);

/* cvpvbiconfig.cpp */
HRESULT
WINAPI
CVPVBIConfig_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv);

/* basicaudio.cpp */
HRESULT
WINAPI
CKsBasicAudio_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv);

/* proxy.cpp */
HRESULT
WINAPI
CKsProxy_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv);

/* input_pin.cpp */
HRESULT
WINAPI
CInputPin_Constructor(
    IBaseFilter * ParentFilter,
    LPCWSTR PinName,
    HANDLE hFilter,
    ULONG PinId,
    KSPIN_COMMUNICATION Communication,
    REFIID riid,
    LPVOID * ppv);

/* output_pin.cpp */
HRESULT
WINAPI
COutputPin_Constructor(
    IBaseFilter * ParentFilter,
    LPCWSTR PinName,
    ULONG PinId,
    KSPIN_COMMUNICATION Communication,
    REFIID riid,
    LPVOID * ppv);

HRESULT
STDMETHODCALLTYPE
COutputPin_SetState(
    IPin * Pin,
    KSSTATE State);

/* enumpins.cpp */
HRESULT
WINAPI
CEnumPins_fnConstructor(
    std::vector<IPin*> Pins,
    REFIID riid,
    LPVOID * ppv);

/* enum_mediatypes.cpp */
HRESULT
WINAPI
CEnumMediaTypes_fnConstructor(
    ULONG MediaTypeCount,
    AM_MEDIA_TYPE * MediaTypes,
    REFIID riid,
    LPVOID * ppv);

/* node.cpp */
HRESULT
WINAPI
CKsNode_Constructor(
    IUnknown * pUnkOuter,
    HANDLE ParentHandle,
    ULONG NodeId,
    ACCESS_MASK DesiredAccess,
    REFIID riid,
    LPVOID * ppv);

/* allocator.cpp */
HRESULT
WINAPI
CKsAllocator_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv);

/* mediasample.cpp */
HRESULT
WINAPI
CMediaSample_Constructor(
    IMemAllocator* Allocator,
    BYTE* pBuffer,
    ULONG BufferSize,
    REFIID riid,
    LPVOID * ppv);


extern const GUID IID_IKsObject;
extern const GUID IID_IKsPinEx;
extern const GUID IID_IKsAggregateControl;
extern const GUID IID_IKsPinPipe;
extern const GUID IID_IKsPinFactory;
extern const GUID IID_IKsAllocatorEx;
extern KSPIN_INTERFACE StandardPinInterface;
extern KSPIN_MEDIUM StandardPinMedium;
