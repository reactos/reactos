/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/mediasample.cpp
 * PURPOSE:         IMediaSample interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */
#include "precomp.h"

class CMediaSample : public IMediaSample
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
            if (m_Allocator)
            {
                m_Allocator->ReleaseBuffer((IMediaSample*)this);
                return 0;
            }
            delete this;
            return 0;
        }
        return m_Ref;
    }

    HRESULT STDMETHODCALLTYPE GetPointer(BYTE **ppBuffer);
    long STDMETHODCALLTYPE GetSize(void);
    HRESULT STDMETHODCALLTYPE GetTime(REFERENCE_TIME *pTimeStart, REFERENCE_TIME *pTimeEnd);
    HRESULT STDMETHODCALLTYPE SetTime(REFERENCE_TIME *pTimeStart, REFERENCE_TIME *pTimeEnd);
    HRESULT STDMETHODCALLTYPE IsSyncPoint();
    HRESULT STDMETHODCALLTYPE SetSyncPoint(BOOL bIsSyncPoint);
    HRESULT STDMETHODCALLTYPE IsPreroll();
    HRESULT STDMETHODCALLTYPE SetPreroll(BOOL bIsPreroll);
    long STDMETHODCALLTYPE GetActualDataLength();
    HRESULT STDMETHODCALLTYPE SetActualDataLength(long Length);
    HRESULT STDMETHODCALLTYPE GetMediaType(AM_MEDIA_TYPE **ppMediaType);
    HRESULT STDMETHODCALLTYPE SetMediaType(AM_MEDIA_TYPE *pMediaType);
    HRESULT STDMETHODCALLTYPE IsDiscontinuity();
    HRESULT STDMETHODCALLTYPE SetDiscontinuity(BOOL bDiscontinuity);
    HRESULT STDMETHODCALLTYPE GetMediaTime(LONGLONG *pTimeStart, LONGLONG *pTimeEnd);
    HRESULT STDMETHODCALLTYPE SetMediaTime(LONGLONG *pTimeStart, LONGLONG *pTimeEnd);

    CMediaSample(IMemAllocator * Allocator, BYTE * Buffer, LONG BufferSize);
    virtual ~CMediaSample(){}

protected:
    ULONG m_Flags;
    ULONG m_TypeFlags;
    BYTE * m_Buffer;
    LONG m_ActualLength;
    LONG m_BufferSize;
    IMemAllocator * m_Allocator;
    CMediaSample * m_Next;
    REFERENCE_TIME m_StartTime;
    REFERENCE_TIME m_StopTime;
    LONGLONG m_MediaStart;
    LONGLONG m_MediaStop;
    AM_MEDIA_TYPE * m_MediaType;
    ULONG m_StreamId;

public:
    LONG m_Ref;

    BOOL m_bMediaTimeValid;


};

CMediaSample::CMediaSample(
    IMemAllocator * Allocator,
    BYTE * Buffer,
    LONG BufferSize) :
                       m_Flags(0),
                       m_TypeFlags(0),
                       m_Buffer(Buffer),
                       m_ActualLength(BufferSize),
                       m_BufferSize(BufferSize),
                       m_Allocator(Allocator),
                       m_Next(0),
                       m_StartTime(0),
                       m_StopTime(0),
                       m_MediaStart(0),
                       m_MediaStop(0),
                       m_MediaType(0),
                       m_StreamId(0),
                       m_Ref(0),
                       m_bMediaTimeValid(0)
{
}


HRESULT
STDMETHODCALLTYPE
CMediaSample::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown) ||
        IsEqualGUID(refiid, IID_IMediaSample))
    {
        *Output = PVOID(this);
        reinterpret_cast<IMediaSample*>(*Output)->AddRef();
        return NOERROR;
    }
    if (IsEqualGUID(refiid, IID_IMediaSample2))
    {
#if 0
        *Output = (IMediaSample2*)(this);
        reinterpret_cast<IMediaSample2*>(*Output)->AddRef();
        return NOERROR;
#endif
    }

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IMediaSample interface
//
HRESULT
STDMETHODCALLTYPE
CMediaSample::GetPointer(
    BYTE **ppBuffer)
{
    if (!ppBuffer)
        return E_POINTER;

    *ppBuffer = m_Buffer;
    return S_OK;
}

long
STDMETHODCALLTYPE
CMediaSample::GetSize()
{
    return m_BufferSize;
}

HRESULT
STDMETHODCALLTYPE
CMediaSample::GetTime(
    REFERENCE_TIME *pTimeStart,
    REFERENCE_TIME *pTimeEnd)
{
    HRESULT hr;

    if (!pTimeStart || !pTimeEnd)
        return E_POINTER;

    if (!(m_Flags & (AM_SAMPLE_TIMEVALID | AM_SAMPLE_STOPVALID)))
    {
        // no time is set
        return VFW_E_SAMPLE_TIME_NOT_SET;
    }

    *pTimeStart = m_StartTime;

    if (m_Flags & AM_SAMPLE_STOPVALID)
    {
        *pTimeEnd = m_StopTime;
        hr = NOERROR;
    }
    else
    {
        *pTimeEnd = m_StartTime + 1;
        hr = VFW_S_NO_STOP_TIME;
    }
    return hr;
}

HRESULT
STDMETHODCALLTYPE
CMediaSample::SetTime(REFERENCE_TIME *pTimeStart, REFERENCE_TIME *pTimeEnd)
{
    if (!pTimeStart)
    {
        m_Flags &= ~(AM_SAMPLE_TIMEVALID | AM_SAMPLE_STOPVALID);
        return NOERROR;
    }

    if (!pTimeEnd)
    {
        m_Flags &= ~(AM_SAMPLE_STOPVALID);
        m_Flags |= AM_SAMPLE_TIMEVALID;
        m_StartTime = *pTimeStart;
        return NOERROR;
    }


    m_Flags |= (AM_SAMPLE_TIMEVALID | AM_SAMPLE_STOPVALID);
    m_StartTime = *pTimeStart;
    m_StopTime = *pTimeEnd;

    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CMediaSample::IsSyncPoint()
{
    return (m_Flags & AM_SAMPLE_SPLICEPOINT) ? S_OK : S_FALSE;
}
HRESULT
STDMETHODCALLTYPE
CMediaSample::SetSyncPoint(BOOL bIsSyncPoint)
{
    if (bIsSyncPoint)
        m_Flags |= AM_SAMPLE_SPLICEPOINT;
    else
        m_Flags &= ~AM_SAMPLE_SPLICEPOINT;

    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CMediaSample::IsPreroll()
{
    return (m_Flags & AM_SAMPLE_PREROLL) ? S_OK : S_FALSE;
}

HRESULT
STDMETHODCALLTYPE
CMediaSample::SetPreroll(BOOL bIsPreroll)
{
    if (bIsPreroll)
        m_Flags |= AM_SAMPLE_PREROLL;
    else
        m_Flags &= ~AM_SAMPLE_PREROLL;

    return NOERROR;
}

long
STDMETHODCALLTYPE
CMediaSample::GetActualDataLength()
{
    return m_ActualLength;
}

HRESULT
STDMETHODCALLTYPE
CMediaSample::SetActualDataLength(long Length)
{
    if (Length > m_BufferSize)
        return VFW_E_BUFFER_OVERFLOW;

    m_ActualLength = Length;
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CMediaSample::GetMediaType(AM_MEDIA_TYPE **ppMediaType)
{
    if (!m_MediaType)
    {
        *ppMediaType = NULL;
        return S_FALSE;
    }

    assert(0);
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CMediaSample::SetMediaType(AM_MEDIA_TYPE *pMediaType)
{
    OutputDebugStringW(L"CMediaSample::SetMediaType NotImplemented\n");
    return E_NOTIMPL;
}


HRESULT
STDMETHODCALLTYPE
CMediaSample::IsDiscontinuity()
{
    return (m_Flags & AM_SAMPLE_DATADISCONTINUITY) ? S_OK : S_FALSE;
}

HRESULT
STDMETHODCALLTYPE
CMediaSample::SetDiscontinuity(BOOL bDiscontinuity)
{
    if (bDiscontinuity)
        m_Flags |= AM_SAMPLE_DATADISCONTINUITY;
    else
        m_Flags &= ~AM_SAMPLE_DATADISCONTINUITY;

    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CMediaSample::GetMediaTime(LONGLONG *pTimeStart, LONGLONG *pTimeEnd)
{
    if (!pTimeStart || !pTimeEnd)
        return E_POINTER;

    if (!m_bMediaTimeValid)
        return VFW_E_MEDIA_TIME_NOT_SET;

    m_MediaStart = *pTimeStart;
    m_MediaStop = *pTimeEnd;

    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CMediaSample::SetMediaTime(LONGLONG *pTimeStart, LONGLONG *pTimeEnd)
{
    if (!pTimeStart || !pTimeEnd)
    {
        m_bMediaTimeValid = false;
        return NOERROR;
    }

    m_MediaStart = *pTimeStart;
    m_MediaStop = *pTimeEnd;

    return NOERROR;
}




HRESULT
WINAPI
CMediaSample_Constructor(
    IMemAllocator* Allocator,
    BYTE* pBuffer,
    ULONG BufferSize,
    REFIID riid,
    LPVOID * ppv)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CMediaSample_Constructor\n");
#endif

    CMediaSample * handler = new CMediaSample(Allocator, pBuffer, BufferSize);

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
