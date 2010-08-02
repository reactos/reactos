/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/node.cpp
 * PURPOSE:         Control Node
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

class CKsNode : public IKsControl
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

    //IKsControl
    HRESULT STDMETHODCALLTYPE KsProperty(PKSPROPERTY Property, ULONG PropertyLength, LPVOID PropertyData, ULONG DataLength, ULONG* BytesReturned);
    HRESULT STDMETHODCALLTYPE KsMethod(PKSMETHOD Method, ULONG MethodLength, LPVOID MethodData, ULONG DataLength, ULONG* BytesReturned);
    HRESULT STDMETHODCALLTYPE KsEvent(PKSEVENT Event, ULONG EventLength, LPVOID EventData, ULONG DataLength, ULONG* BytesReturned);

    CKsNode(IUnknown * pUnkOuter, HANDLE Handle) : m_Ref(0), m_pUnkOuter(pUnkOuter), m_Handle(Handle){};
    virtual ~CKsNode()
    {
        CloseHandle(m_Handle);
    };

protected:
    LONG m_Ref;
    IUnknown * m_pUnkOuter;
    HANDLE m_Handle;
};

HRESULT
STDMETHODCALLTYPE
CKsNode::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown) ||
        IsEqualGUID(refiid, IID_IKsControl))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IKsControl
//
HRESULT
STDMETHODCALLTYPE
CKsNode::KsProperty(
    PKSPROPERTY Property,
    ULONG PropertyLength,
    LPVOID PropertyData,
    ULONG DataLength,
    ULONG* BytesReturned)
{
    assert(m_Handle != 0);
    return KsSynchronousDeviceControl(m_Handle, IOCTL_KS_PROPERTY, (PVOID)Property, PropertyLength, (PVOID)PropertyData, DataLength, BytesReturned);
}

HRESULT
STDMETHODCALLTYPE
CKsNode::KsMethod(
    PKSMETHOD Method,
    ULONG MethodLength,
    LPVOID MethodData,
    ULONG DataLength,
    ULONG* BytesReturned)
{
    assert(m_Handle != 0);
    return KsSynchronousDeviceControl(m_Handle, IOCTL_KS_METHOD, (PVOID)Method, MethodLength, (PVOID)MethodData, DataLength, BytesReturned);
}

HRESULT
STDMETHODCALLTYPE
CKsNode::KsEvent(
    PKSEVENT Event,
    ULONG EventLength,
    LPVOID EventData,
    ULONG DataLength,
    ULONG* BytesReturned)
{
    assert(m_Handle != 0);

    if (EventLength)
        return KsSynchronousDeviceControl(m_Handle, IOCTL_KS_ENABLE_EVENT, (PVOID)Event, EventLength, (PVOID)EventData, DataLength, BytesReturned);
    else
        return KsSynchronousDeviceControl(m_Handle, IOCTL_KS_DISABLE_EVENT, (PVOID)Event, EventLength, NULL, 0, BytesReturned);
}

HRESULT
WINAPI
CKsNode_Constructor(
    IUnknown * pUnkOuter,
    HANDLE ParentHandle,
    ULONG NodeId,
    ACCESS_MASK DesiredAccess,
    REFIID riid,
    LPVOID * ppv)
{
    HRESULT hr;
    HANDLE handle;
    KSNODE_CREATE NodeCreate;

#ifdef KSPROXY_TRACE
    OutputDebugStringW(L"CKsNode_Constructor\n");
#endif

    //setup request
    NodeCreate.CreateFlags = 0;
    NodeCreate.Node = NodeId;

    hr = KsCreateTopologyNode(ParentHandle, &NodeCreate, DesiredAccess, &handle);
    if (hr != NOERROR)
    {
         OutputDebugString("CKsNode_Constructor failed to open device\n");
         return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, hr);
    }

    CKsNode * quality = new CKsNode(pUnkOuter, handle);

    if (!quality)
    {
        // free clock handle
        CloseHandle(handle);
        return E_OUTOFMEMORY;
    }

    if (FAILED(quality->QueryInterface(riid, ppv)))
    {
        /* not supported */
        delete quality;
        return E_NOINTERFACE;
    }

    return NOERROR;
}
