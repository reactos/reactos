/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/datatype.cpp
 * PURPOSE:         IKsDataTypeHandler interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */
#include "precomp.h"

/* FIXME guid mess */
const GUID IID_IUnknown           = {0x00000000, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
const GUID IID_IClassFactory      = {0x00000001, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

class CKsDataTypeHandler : public IKsDataTypeHandler
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

    HRESULT STDMETHODCALLTYPE KsCompleteIoOperation(IN OUT IMediaSample *Sample, IN OUT PVOID StreamHeader, IN KSIOOPERATION IoOperation, IN BOOL Cancelled);
    HRESULT STDMETHODCALLTYPE KsIsMediaTypeInRanges(IN PVOID DataRanges);
    HRESULT STDMETHODCALLTYPE KsPrepareIoOperation(IN OUT IMediaSample *Sample, IN OUT PVOID StreamHeader, IN KSIOOPERATION IoOperation);
    HRESULT STDMETHODCALLTYPE KsQueryExtendedSize(OUT ULONG* ExtendedSize);
    HRESULT STDMETHODCALLTYPE KsSetMediaType(IN const AM_MEDIA_TYPE* AmMediaType);

    CKsDataTypeHandler() : m_Ref(0), m_Type(0){};
    virtual ~CKsDataTypeHandler()
    {
        if (m_Type)
        {
            if (m_Type->pbFormat)
                CoTaskMemFree(m_Type->pbFormat);

            if (m_Type->pUnk)
                m_Type->pUnk->Release();

            CoTaskMemFree(m_Type);
        }

    };

protected:
    LONG m_Ref;
    AM_MEDIA_TYPE * m_Type;
};


HRESULT
STDMETHODCALLTYPE
CKsDataTypeHandler::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown) ||
        IsEqualGUID(refiid, IID_IKsDataTypeHandler))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}


HRESULT
STDMETHODCALLTYPE
CKsDataTypeHandler::KsCompleteIoOperation(
    IN OUT IMediaSample *Sample,
    IN OUT PVOID StreamHeader,
    IN KSIOOPERATION IoOperation,
    IN BOOL Cancelled)
{
    return NOERROR;
}


HRESULT
STDMETHODCALLTYPE
CKsDataTypeHandler::KsIsMediaTypeInRanges(
    IN PVOID DataRanges)
{
    PKSMULTIPLE_ITEM DataList;
    PKSDATARANGE DataRange;
    ULONG Index;
    //HRESULT hr = S_FALSE;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsDataTypeHandler::KsIsMediaTypeInRanges\n");
#endif

    DataList = (PKSMULTIPLE_ITEM)DataRanges;
    DataRange = (PKSDATARANGE)(DataList + 1);

    for(Index = 0; Index < DataList->Count; Index++)
    {
        BOOL bMatch = FALSE;

        if (DataRange->FormatSize >= sizeof(KSDATARANGE))
        {
            bMatch = IsEqualGUID(DataRange->MajorFormat, GUID_NULL);
        }

        if (!bMatch && DataRange->FormatSize >= sizeof(KSDATARANGE_AUDIO))
        {
            bMatch = IsEqualGUID(DataRange->MajorFormat, MEDIATYPE_Audio);
        }

        if (bMatch)
        {
            if (IsEqualGUID(DataRange->SubFormat, m_Type->subtype) ||
                IsEqualGUID(DataRange->SubFormat, GUID_NULL))
            {
                if (IsEqualGUID(DataRange->Specifier, m_Type->formattype) ||
                    IsEqualGUID(DataRange->Specifier, GUID_NULL))
                {
                    if (!IsEqualGUID(m_Type->formattype, FORMAT_WaveFormatEx) && !IsEqualGUID(DataRange->Specifier, FORMAT_WaveFormatEx))
                    {
                        //found match
                        //hr = S_OK;
                        break;
                    }

                    if (DataRange->FormatSize >= sizeof(KSDATARANGE_AUDIO) && m_Type->cbFormat >= sizeof(WAVEFORMATEX))
                    {
                        LPWAVEFORMATEX Format = (LPWAVEFORMATEX)m_Type->pbFormat;
                        PKSDATARANGE_AUDIO AudioRange = (PKSDATARANGE_AUDIO)DataRange;

                        if (Format->nSamplesPerSec >= AudioRange->MinimumSampleFrequency &&
                            Format->nSamplesPerSec <= AudioRange->MaximumSampleFrequency &&
                            Format->wBitsPerSample >= AudioRange->MinimumSampleFrequency &&
                            Format->wBitsPerSample <= AudioRange->MaximumBitsPerSample &&
                            Format->nChannels <= AudioRange->MaximumChannels)
                        {
                            // found match
                            //hr = S_OK;
                            break;
                        }
                    }
                }
            }
        }

        DataRange = (PKSDATARANGE)(((ULONG_PTR)DataRange + DataRange->FormatSize + 7) & ~7);
    }
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CKsDataTypeHandler::KsPrepareIoOperation(
    IN OUT IMediaSample *Sample,
    IN OUT PVOID StreamHeader,
    IN KSIOOPERATION IoOperation)
{
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CKsDataTypeHandler::KsQueryExtendedSize(
    OUT ULONG* ExtendedSize)
{
    /* no header extension required */
    *ExtendedSize = 0;
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CKsDataTypeHandler::KsSetMediaType(
    IN const AM_MEDIA_TYPE* AmMediaType)
{
#ifdef KSPROXY_TRACE
    OutputDebugString("CKsDataTypeHandler::KsSetMediaType\n");
#endif

    if (m_Type)
    {
        /* media type can only be set once */
        return E_FAIL;
    }

    m_Type = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (!m_Type)
        return E_OUTOFMEMORY;

    CopyMemory(m_Type, AmMediaType, sizeof(AM_MEDIA_TYPE));

    if (m_Type->cbFormat)
    {
        m_Type->pbFormat = (BYTE*)CoTaskMemAlloc(m_Type->cbFormat);

        if (!m_Type->pbFormat)
        {
            CoTaskMemFree(m_Type);
            return E_OUTOFMEMORY;
        }

        CopyMemory(m_Type->pbFormat, AmMediaType->pbFormat, m_Type->cbFormat);
    }

    if (m_Type->pUnk)
        m_Type->pUnk->AddRef();


    return S_OK;
}

HRESULT
WINAPI
CKsDataTypeHandler_Constructor (
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsDataTypeHandler_Constructor\n");
#endif

    CKsDataTypeHandler * handler = new CKsDataTypeHandler();

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
