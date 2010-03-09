/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/datatype.cpp
 * PURPOSE:         IKsDataTypeHandler interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
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

    CKsDataTypeHandler() : m_Ref(0){};
    virtual ~CKsDataTypeHandler(){};

protected:
    //CMediaType * m_Type;
    LONG m_Ref;
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
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
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
#if 0
    if (m_Type)
    {
        /* media type can only be set once */
        return E_FAIL;
    }
#endif

    /*
     * TODO: allocate CMediaType and copy parameters
     */
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
CKsDataTypeHandler_Constructor (
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    OutputDebugStringW(L"CKsDataTypeHandler_Constructor\n");

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
