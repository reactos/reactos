/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/interface.cpp
 * PURPOSE:         IKsInterfaceHandler interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

const GUID IID_IKsObject           = {0x423c13a2, 0x2070, 0x11d0, {0x9e, 0xf7, 0x00, 0xaa, 0x00, 0xa2, 0x16, 0xa1}};

class CKsInterfaceHandler : public IKsInterfaceHandler
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
    HRESULT STDMETHODCALLTYPE KsSetPin(IKsPin *KsPin);
    HRESULT STDMETHODCALLTYPE KsProcessMediaSamples(IKsDataTypeHandler *KsDataTypeHandler, IMediaSample** SampleList, PLONG SampleCount, KSIOOPERATION IoOperation, PKSSTREAM_SEGMENT *StreamSegment);
    HRESULT STDMETHODCALLTYPE KsCompleteIo(PKSSTREAM_SEGMENT StreamSegment);

    CKsInterfaceHandler() : m_Ref(0), m_Handle(NULL){};
    virtual ~CKsInterfaceHandler(){};

protected:
    LONG m_Ref;
    HANDLE m_Handle;
};

HRESULT
STDMETHODCALLTYPE
CKsInterfaceHandler::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown) ||
        IsEqualGUID(refiid, IID_IKsInterfaceHandler))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CKsInterfaceHandler::KsSetPin(
    IKsPin *KsPin)
{
    HRESULT hr;
    IKsObject * KsObject;

    // check if IKsObject is supported
    hr = KsPin->QueryInterface(IID_IKsObject, (void**)&KsObject);

    if (SUCCEEDED(hr))
    {
        // get pin handle
        m_Handle = KsObject->KsGetObjectHandle();

        // release IKsObject interface
        KsObject->Release();

        if (!m_Handle)
        {
            // expected a file handle
            return E_UNEXPECTED;
        }
    }

    // done
    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsInterfaceHandler::KsProcessMediaSamples(
     IKsDataTypeHandler *KsDataTypeHandler,
     IMediaSample** SampleList,
     PLONG SampleCount,
     KSIOOPERATION IoOperation,
     PKSSTREAM_SEGMENT *StreamSegment)
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CKsInterfaceHandler::KsCompleteIo(
    PKSSTREAM_SEGMENT StreamSegment)
{
    OutputDebugString("UNIMPLEMENTED\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
CKsInterfaceHandler_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    OutputDebugStringW(L"CKsInterfaceHandler_Constructor\n");

    CKsInterfaceHandler * handler = new CKsInterfaceHandler();

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
