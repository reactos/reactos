/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/allocator.cpp
 * PURPOSE:         IKsAllocator interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

const GUID IID_IKsAllocatorEx = {0x091bb63a, 0x603f, 0x11d1, {0xb0, 0x67, 0x00, 0xa0, 0xc9, 0x06, 0x28, 0x02}};
const GUID IID_IKsAllocator   = {0x8da64899, 0xc0d9, 0x11d0, {0x84, 0x13, 0x00, 0x00, 0xf8, 0x22, 0xfe, 0x8a}};

class CKsAllocator : public IKsAllocatorEx,
                     public IMemAllocatorCallbackTemp
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }
    //IKsAllocator
    HANDLE STDMETHODCALLTYPE KsGetAllocatorHandle();
    KSALLOCATORMODE STDMETHODCALLTYPE KsGetAllocatorMode();
    HRESULT STDMETHODCALLTYPE KsGetAllocatorStatus(PKSSTREAMALLOCATOR_STATUS AllocatorStatus);
    VOID STDMETHODCALLTYPE KsSetAllocatorMode(KSALLOCATORMODE Mode);

    //IKsAllocatorEx
    PALLOCATOR_PROPERTIES_EX STDMETHODCALLTYPE KsGetProperties();
    VOID STDMETHODCALLTYPE KsSetProperties(PALLOCATOR_PROPERTIES_EX Properties);
    VOID STDMETHODCALLTYPE KsSetAllocatorHandle(HANDLE AllocatorHandle);
    HANDLE STDMETHODCALLTYPE KsCreateAllocatorAndGetHandle(IKsPin* KsPin);

    //IMemAllocator
    HRESULT STDMETHODCALLTYPE SetProperties(ALLOCATOR_PROPERTIES *pRequest, ALLOCATOR_PROPERTIES *pActual);
    HRESULT STDMETHODCALLTYPE GetProperties(ALLOCATOR_PROPERTIES *pProps);
    HRESULT STDMETHODCALLTYPE Commit();
    HRESULT STDMETHODCALLTYPE Decommit();
    HRESULT STDMETHODCALLTYPE GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime, DWORD dwFlags);
    HRESULT STDMETHODCALLTYPE ReleaseBuffer(IMediaSample *pBuffer);

    //IMemAllocatorCallbackTemp
    HRESULT STDMETHODCALLTYPE SetNotify(IMemAllocatorNotifyCallbackTemp *pNotify);
    HRESULT STDMETHODCALLTYPE GetFreeCount(LONG *plBuffersFree);


    CKsAllocator() : m_Ref(0), m_hAllocator(0), m_Mode(KsAllocatorMode_User), m_Notify(0), m_Allocated(0), m_FreeCount(0), m_cbBuffer(0), m_cBuffers(0), m_cbAlign(0), m_cbPrefix(0){}
    virtual ~CKsAllocator(){}

protected:
    LONG m_Ref;
    HANDLE m_hAllocator;
    KSALLOCATORMODE m_Mode;
    ALLOCATOR_PROPERTIES_EX m_Properties;
    IMemAllocatorNotifyCallbackTemp *m_Notify;
    ULONG m_Allocated;
    ULONG m_FreeCount;
    ULONG m_cbBuffer;
    ULONG m_cBuffers;
    ULONG m_cbAlign;
    ULONG m_cbPrefix;
};


HRESULT
STDMETHODCALLTYPE
CKsAllocator::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown) ||
        IsEqualGUID(refiid, IID_IKsAllocator) ||
        IsEqualGUID(refiid, IID_IKsAllocatorEx))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    if (IsEqualGUID(refiid, IID_IMemAllocator) ||
        IsEqualGUID(refiid, IID_IMemAllocatorCallbackTemp))
    {
        *Output = (IDistributorNotify*)(this);
        reinterpret_cast<IDistributorNotify*>(*Output)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IMemAllocator
//
HRESULT
STDMETHODCALLTYPE
CKsAllocator::SetProperties(
    ALLOCATOR_PROPERTIES *pRequest,
    ALLOCATOR_PROPERTIES *pActual)
{
    SYSTEM_INFO SystemInfo;

    OutputDebugStringW(L"CKsAllocator::SetProperties Stub\n");

    if (!pRequest || !pActual)
        return E_POINTER;

    // zero output properties
    ZeroMemory(pActual, sizeof(ALLOCATOR_PROPERTIES));

    // get system info
    GetSystemInfo(&SystemInfo);

    if (!pRequest->cbAlign || (pRequest->cbAlign - 1) & SystemInfo.dwAllocationGranularity)
    {
        // bad alignment
        return VFW_E_BADALIGN;
    }

    if (m_Mode == KsAllocatorMode_Kernel)
    {
        // u cannt change a kernel allocator
        return VFW_E_ALREADY_COMMITTED;
    }

    if (m_Allocated != m_FreeCount)
    {
        // outstanding buffers
        return VFW_E_BUFFERS_OUTSTANDING;
    }

    pActual->cbAlign = m_cbAlign = pRequest->cbAlign;
    pActual->cbBuffer = m_cbBuffer = pRequest->cbBuffer;
    pActual->cbPrefix = m_cbPrefix = pRequest->cbPrefix;
    pActual->cBuffers = m_cBuffers = pRequest->cBuffers;

    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CKsAllocator::GetProperties(
    ALLOCATOR_PROPERTIES *pProps)
{
    if (!pProps)
        return E_POINTER;

    pProps->cbBuffer = m_cbBuffer;
    pProps->cBuffers = m_cBuffers;
    pProps->cbAlign = m_cbAlign;
    pProps->cbPrefix = m_cbPrefix;

    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CKsAllocator::Commit()
{
    if (m_Mode == KsAllocatorMode_Kernel)
    {
        /* no-op for kernel allocator */
       return NOERROR;
    }

    OutputDebugStringW(L"CKsAllocator::Commit NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsAllocator::Decommit()
{
    if (m_Mode == KsAllocatorMode_Kernel)
    {
        /* no-op for kernel allocator */
       return NOERROR;
    }

    OutputDebugStringW(L"CKsAllocator::Decommit NotImplemented\n");
    return E_NOTIMPL;
}


HRESULT
STDMETHODCALLTYPE
CKsAllocator::GetBuffer(
    IMediaSample **ppBuffer,
    REFERENCE_TIME *pStartTime,
    REFERENCE_TIME *pEndTime,
    DWORD dwFlags)
{
    OutputDebugStringW(L"CKsAllocator::GetBuffer NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsAllocator::ReleaseBuffer(
    IMediaSample *pBuffer)
{
    OutputDebugStringW(L"CKsAllocator::ReleaseBuffer NotImplemented\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IMemAllocatorCallbackTemp
//
HRESULT
STDMETHODCALLTYPE
CKsAllocator::SetNotify(
    IMemAllocatorNotifyCallbackTemp *pNotify)
{
    OutputDebugStringW(L"CKsAllocator::SetNotify\n");

    if (pNotify)
        pNotify->AddRef();

    if (m_Notify)
        m_Notify->Release();

    m_Notify = pNotify;
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CKsAllocator::GetFreeCount(
    LONG *plBuffersFree)
{
    OutputDebugStringW(L"CKsAllocator::GetFreeCount NotImplemented\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IKsAllocator
//
HANDLE
STDMETHODCALLTYPE
CKsAllocator::KsGetAllocatorHandle()
{
    return m_hAllocator;
}

KSALLOCATORMODE
STDMETHODCALLTYPE
CKsAllocator::KsGetAllocatorMode()
{
    return m_Mode;
}

HRESULT
STDMETHODCALLTYPE
CKsAllocator::KsGetAllocatorStatus(
    PKSSTREAMALLOCATOR_STATUS AllocatorStatus)
{
    return NOERROR;
}
VOID
STDMETHODCALLTYPE
CKsAllocator::KsSetAllocatorMode(
    KSALLOCATORMODE Mode)
{
    m_Mode = Mode;
}

//-------------------------------------------------------------------
// IKsAllocatorEx
//
PALLOCATOR_PROPERTIES_EX
STDMETHODCALLTYPE
CKsAllocator::KsGetProperties()
{
    return &m_Properties;
}

VOID
STDMETHODCALLTYPE
CKsAllocator::KsSetProperties(
    PALLOCATOR_PROPERTIES_EX Properties)
{
    CopyMemory(&m_Properties, Properties, sizeof(ALLOCATOR_PROPERTIES_EX));
}

VOID
STDMETHODCALLTYPE
CKsAllocator::KsSetAllocatorHandle(
    HANDLE AllocatorHandle)
{
    m_hAllocator = AllocatorHandle;
}


HANDLE
STDMETHODCALLTYPE
CKsAllocator::KsCreateAllocatorAndGetHandle(
    IKsPin* KsPin)
{
    HRESULT hr;
    IKsObject * pObject;
    KSALLOCATOR_FRAMING AllocatorFraming;
    HANDLE hPin;

    OutputDebugStringW(L"CKsAllocator::KsCreateAllocatorAndGetHandle\n");

    if (m_hAllocator)
    {
        CloseHandle(m_hAllocator);
        m_hAllocator = NULL;
    }

    // get pin IKsObject interface
    hr = KsPin->QueryInterface(IID_IKsObject, (void**)&pObject);
    if (FAILED(hr))
        return NULL;

    // get pin handle
    hPin = pObject->KsGetObjectHandle();

    //release IKsObject interface
    pObject->Release();

    if (!hPin || hPin == INVALID_HANDLE_VALUE)
        return NULL;

    //setup allocator framing
    AllocatorFraming.Frames = m_Properties.cBuffers;
    AllocatorFraming.FrameSize = m_Properties.cbBuffer;
    AllocatorFraming.FileAlignment = (m_Properties.cbAlign -1);
    AllocatorFraming.OptionsFlags = KSALLOCATOR_OPTIONF_SYSTEM_MEMORY;
    AllocatorFraming.PoolType = (m_Properties.LogicalMemoryType == KS_MemoryTypeKernelPaged);

    DWORD dwError = KsCreateAllocator(hPin, &AllocatorFraming, &m_hAllocator);
    if (dwError)
        return NULL;

    return m_hAllocator;
}

HRESULT
WINAPI
CKsAllocator_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    OutputDebugStringW(L"CKsAllocator_Constructor\n");

    CKsAllocator * handler = new CKsAllocator();

    if (!handler)
        return E_OUTOFMEMORY;

    if (FAILED(handler->QueryInterface(riid, ppv)))
    {
        /* not supported */
        delete handler;
        return E_NOINTERFACE;
    }

    return NOERROR;
}
