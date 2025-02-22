/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/allocator.cpp
 * PURPOSE:         IKsAllocator interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */
#include "precomp.h"

const GUID IID_IKsAllocatorEx = {0x091bb63a, 0x603f, 0x11d1, {0xb0, 0x67, 0x00, 0xa0, 0xc9, 0x06, 0x28, 0x02}};
const GUID IID_IKsAllocator   = {0x8da64899, 0xc0d9, 0x11d0, {0x84, 0x13, 0x00, 0x00, 0xf8, 0x22, 0xfe, 0x8a}};

class CKsAllocator : public IKsAllocatorEx,
                     public IMemAllocatorCallbackTemp
{
public:
    typedef std::stack<IMediaSample *>MediaSampleStack;
    typedef std::list<IMediaSample *>MediaSampleList;

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


    CKsAllocator();
    virtual ~CKsAllocator(){}
    VOID STDMETHODCALLTYPE FreeMediaSamples();
protected:
    LONG m_Ref;
    HANDLE m_hAllocator;
    KSALLOCATORMODE m_Mode;
    ALLOCATOR_PROPERTIES_EX m_Properties;
    IMemAllocatorNotifyCallbackTemp *m_Notify;
    ULONG m_Allocated;
    LONG m_cbBuffer;
    LONG m_cBuffers;
    LONG m_cbAlign;
    LONG m_cbPrefix;
    BOOL m_Committed;
    CRITICAL_SECTION m_CriticalSection;
    MediaSampleStack m_FreeList;
    MediaSampleList m_UsedList;
    LPVOID m_Buffer;
    BOOL m_FreeSamples;
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
        *Output = (IMemAllocatorCallbackTemp*)(this);
        reinterpret_cast<IMemAllocatorCallbackTemp*>(*Output)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

CKsAllocator::CKsAllocator() : m_Ref(0),
                               m_hAllocator(0),
                               m_Mode(KsAllocatorMode_User),
                               m_Notify(0),
                               m_Allocated(0),
                               m_cbBuffer(0),
                               m_cBuffers(0),
                               m_cbAlign(0),
                               m_cbPrefix(0),
                               m_Committed(FALSE),
                               m_FreeList(),
                               m_UsedList(),
                               m_Buffer(0),
                               m_FreeSamples(FALSE)
{
   InitializeCriticalSection(&m_CriticalSection);

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

    EnterCriticalSection(&m_CriticalSection);

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsAllocator::SetProperties\n");
#endif

    if (!pRequest || !pActual)
        return E_POINTER;

    // zero output properties
    ZeroMemory(pActual, sizeof(ALLOCATOR_PROPERTIES));

    // get system info
    GetSystemInfo(&SystemInfo);

    if (!pRequest->cbAlign || (pRequest->cbAlign - 1) & SystemInfo.dwAllocationGranularity)
    {
        // bad alignment
        LeaveCriticalSection(&m_CriticalSection);
        return VFW_E_BADALIGN;
    }

    if (m_Mode == KsAllocatorMode_Kernel)
    {
        // u can't change a kernel allocator
        LeaveCriticalSection(&m_CriticalSection);
        return VFW_E_ALREADY_COMMITTED;
    }

    if (m_Committed)
    {
        // need to decommit first
        LeaveCriticalSection(&m_CriticalSection);
        return VFW_E_ALREADY_COMMITTED;
    }

    if (m_Allocated != m_FreeList.size())
    {
        // outstanding buffers
        LeaveCriticalSection(&m_CriticalSection);
        return VFW_E_BUFFERS_OUTSTANDING;
    }

    pActual->cbAlign = m_cbAlign = pRequest->cbAlign;
    pActual->cbBuffer = m_cbBuffer = pRequest->cbBuffer;
    pActual->cbPrefix = m_cbPrefix = pRequest->cbPrefix;
    pActual->cBuffers = m_cBuffers = pRequest->cBuffers;

    LeaveCriticalSection(&m_CriticalSection);
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
    LONG Index;
    PUCHAR CurrentBuffer;
    IMediaSample * Sample;
    HRESULT hr;

    //TODO integer overflow checks
    EnterCriticalSection(&m_CriticalSection);

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsAllocator::Commit\n");
#endif

    if (m_Mode == KsAllocatorMode_Kernel)
    {
        /* no-op for kernel allocator */
       LeaveCriticalSection(&m_CriticalSection);
       return NOERROR;
    }

    if (m_Committed)
    {
        // already committed
        LeaveCriticalSection(&m_CriticalSection);
        return NOERROR;
    }

    if (m_cbBuffer < 0 || m_cBuffers < 0 || m_cbPrefix < 0)
    {
        // invalid parameter
        LeaveCriticalSection(&m_CriticalSection);
        return E_OUTOFMEMORY;
    }

    LONG Size = m_cbBuffer + m_cbPrefix;

    if (m_cbAlign > 1)
    {
        //check alignment
        LONG Mod = Size % m_cbAlign;
        if (Mod)
        {
            // calculate aligned size
            Size += m_cbAlign - Mod;
        }
    }

    LONG TotalSize = Size * m_cBuffers;

    assert(TotalSize);
    assert(m_cBuffers);
    assert(Size);

    // now allocate buffer
    m_Buffer = VirtualAlloc(NULL, TotalSize, MEM_COMMIT, PAGE_READWRITE);
    if (!m_Buffer)
    {
        LeaveCriticalSection(&m_CriticalSection);
        return E_OUTOFMEMORY;
    }

    ZeroMemory(m_Buffer, TotalSize);

    CurrentBuffer = (PUCHAR)m_Buffer;

    for (Index = 0; Index < m_cBuffers; Index++)
    {
        // construct media sample
        hr = CMediaSample_Constructor((IMemAllocator*)this, CurrentBuffer + m_cbPrefix, m_cbBuffer, IID_IMediaSample, (void**)&Sample);
        if (FAILED(hr))
        {
            LeaveCriticalSection(&m_CriticalSection);
            return E_OUTOFMEMORY;
        }

        // add to free list
        m_FreeList.push(Sample);
        m_Allocated++;

        //next sample buffer
        CurrentBuffer += Size;
    }

    // we are now committed
    m_Committed = true;

    LeaveCriticalSection(&m_CriticalSection);
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsAllocator::Decommit()
{
    EnterCriticalSection(&m_CriticalSection);

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsAllocator::Decommit\n");
#endif

    if (m_Mode == KsAllocatorMode_Kernel)
    {
        /* no-op for kernel allocator */
        LeaveCriticalSection(&m_CriticalSection);
        return NOERROR;
    }

    m_Committed = false;

    if (m_Allocated != m_FreeList.size())
    {
        // outstanding buffers
        m_FreeSamples = true;
        LeaveCriticalSection(&m_CriticalSection);
        return NOERROR;
    }
    else
    {
        // no outstanding buffers
        // free to free them
        FreeMediaSamples();
    }

    LeaveCriticalSection(&m_CriticalSection);
    return NOERROR;
}


HRESULT
STDMETHODCALLTYPE
CKsAllocator::GetBuffer(
    IMediaSample **ppBuffer,
    REFERENCE_TIME *pStartTime,
    REFERENCE_TIME *pEndTime,
    DWORD dwFlags)
{
    IMediaSample * Sample = NULL;

    if (!m_Committed)
        return VFW_E_NOT_COMMITTED;

    do
    {
        EnterCriticalSection(&m_CriticalSection);

        if (!m_FreeList.empty())
        {
            OutputDebugStringW(L"CKsAllocator::GetBuffer HACK\n");
            Sample = m_FreeList.top();
            m_FreeList.pop();
        }

        LeaveCriticalSection(&m_CriticalSection);

        if (dwFlags & AM_GBF_NOWAIT)
        {
            // never wait untill a buffer becomes available
            break;
        }
    }
    while(Sample == NULL);

    if (!Sample)
    {
        // no sample acquired
        //HACKKKKKKK
        Sample = m_UsedList.back();
        m_UsedList.pop_back();

        if (!Sample)
            return VFW_E_TIMEOUT;
    }

    // store result
    *ppBuffer = Sample;

   // store sample in used list
   m_UsedList.push_front(Sample);

    // done
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CKsAllocator::ReleaseBuffer(
    IMediaSample *pBuffer)
{
    EnterCriticalSection(&m_CriticalSection);

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsAllocator::ReleaseBuffer\n");
#endif

    // media sample always 1 ref count in free list
    pBuffer->AddRef();

    // add the sample to the free list
    m_FreeList.push(pBuffer);


    if (m_FreeSamples)
    {
        // pending de-commit
        if (m_FreeList.size () == m_Allocated)
        {
            FreeMediaSamples();
        }
    }

    if (m_Notify)
    {
        //notify caller of an available buffer
        m_Notify->NotifyRelease();
    }

    LeaveCriticalSection(&m_CriticalSection);
    return S_OK;
}

//-------------------------------------------------------------------
// IMemAllocatorCallbackTemp
//
HRESULT
STDMETHODCALLTYPE
CKsAllocator::SetNotify(
    IMemAllocatorNotifyCallbackTemp *pNotify)
{
    EnterCriticalSection(&m_CriticalSection);

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsAllocator::SetNotify\n");
#endif

    if (pNotify)
        pNotify->AddRef();

    if (m_Notify)
        m_Notify->Release();

    m_Notify = pNotify;

    LeaveCriticalSection(&m_CriticalSection);
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CKsAllocator::GetFreeCount(
    LONG *plBuffersFree)
{
    *plBuffersFree = m_Allocated - m_FreeList.size();
    return S_OK;
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

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsAllocator::KsCreateAllocatorAndGetHandle\n");
#endif

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

//-------------------------------------------------------------------
VOID
STDMETHODCALLTYPE
CKsAllocator::FreeMediaSamples()
{
    ULONG Index;

    for(Index = 0; Index < m_FreeList.size(); Index++)
    {
        IMediaSample * Sample = m_FreeList.top();
        m_FreeList.pop();
        Sample->Release();
    }

    m_FreeSamples = false;
    m_Allocated = 0;

    if (m_Buffer)
    {
        // release buffer
        VirtualFree(m_Buffer, 0, MEM_RELEASE);

        m_Buffer = NULL;
    }
}

HRESULT
WINAPI
CKsAllocator_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsAllocator_Constructor\n");
#endif

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
