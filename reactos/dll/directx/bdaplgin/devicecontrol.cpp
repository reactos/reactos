/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/classfactory.cpp
 * PURPOSE:         ClassFactory interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"

const GUID CLSID_DVBTNetworkProvider = {0x216c62df, 0x6d7f, 0x4e9a, {0x85, 0x71, 0x5, 0xf1, 0x4e, 0xdb, 0x76, 0x6a}};
const GUID IID_IAC3Filter         = {0xe4539501, 0xc609, 0x46ea, {0xad, 0x2a, 0x0e, 0x97, 0x00, 0x24, 0x56, 0x83}};
const GUID IID_IAsyncReader       = {0x56A868AA, 0x0AD4, 0x11CE, {0xB0, 0x3A, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70}};
const GUID IID_IMatrixMixer       = {0xafc57835, 0x2fd1, 0x4541, {0xa6, 0xd9, 0x0d, 0xb7, 0x18, 0x56, 0xe5, 0x89}};
const GUID IID_IBDA_NetworkProvider   = {0xfd501041, 0x8ebe, 0x11ce, {0x81, 0x83, 0x00, 0xaa, 0x00, 0x57, 0x7d, 0xa2}};
const GUID IID_IAMOpenProgress    = {0x8E1C39A1, 0xDE53, 0x11cf, {0xAA, 0x63, 0x00, 0x80, 0xC7, 0x44, 0x52, 0x8D}};
const GUID IID_IDistributorNotify = {0x56a868af, 0x0ad4, 0x11ce, {0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
const GUID IID_IBDA_DeviceControl = {0xFD0A5AF3, 0xB41D, 0x11d2, {0x9C, 0x95, 0x00, 0xC0, 0x4F, 0x79, 0x71, 0xE0}};
const GUID IID_IBDA_Topology      = {0x79B56888, 0x7FEA, 0x4690, {0xB4, 0x5D, 0x38, 0xFD, 0x3C, 0x78, 0x49, 0xBE}};
const GUID IID_IKsObject           = {0x423c13a2, 0x2070, 0x11d0, {0x9e, 0xf7, 0x00, 0xaa, 0x00, 0xa2, 0x16, 0xa1}};
const GUID KSPROPSETID_BdaTopology = {0xa14ee835, 0x0a23, 0x11d3, {0x9c, 0xc7, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0}};
const GUID KSMETHODSETID_BdaDeviceConfiguration = {0x71985f45, 0x1ca1, 0x11d3, {0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0}};

class CBDADeviceControl : public IBDA_DeviceControl,
                          public IBDA_Topology
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

    // IBDA_DeviceControl methods
    HRESULT STDMETHODCALLTYPE StartChanges( void);
    HRESULT STDMETHODCALLTYPE CheckChanges( void);
    HRESULT STDMETHODCALLTYPE CommitChanges( void);
    HRESULT STDMETHODCALLTYPE GetChangeState(ULONG *pState);

    // IBDA_Topology methods
    HRESULT STDMETHODCALLTYPE GetNodeTypes(ULONG *pulcNodeTypes, ULONG ulcNodeTypesMax, ULONG * rgulNodeTypes);
    HRESULT STDMETHODCALLTYPE GetNodeDescriptors(ULONG *ulcNodeDescriptors, ULONG ulcNodeDescriptorsMax, BDANODE_DESCRIPTOR * rgNodeDescriptors);
    HRESULT STDMETHODCALLTYPE GetNodeInterfaces(ULONG ulNodeType, ULONG *pulcInterfaces, ULONG ulcInterfacesMax, GUID * rgguidInterfaces);
    HRESULT STDMETHODCALLTYPE GetPinTypes(ULONG *pulcPinTypes, ULONG ulcPinTypesMax, ULONG *rgulPinTypes);
    HRESULT STDMETHODCALLTYPE GetTemplateConnections(ULONG *pulcConnections, ULONG ulcConnectionsMax, BDA_TEMPLATE_CONNECTION * rgConnections);
    HRESULT STDMETHODCALLTYPE CreatePin(ULONG ulPinType, ULONG *pulPinId);
    HRESULT STDMETHODCALLTYPE DeletePin(ULONG ulPinId);
    HRESULT STDMETHODCALLTYPE SetMediaType(ULONG ulPinId, AM_MEDIA_TYPE *pMediaType);
    HRESULT STDMETHODCALLTYPE SetMedium(ULONG ulPinId, REGPINMEDIUM *pMedium);
    HRESULT STDMETHODCALLTYPE CreateTopology(ULONG ulInputPinId, ULONG ulOutputPinId);
    HRESULT STDMETHODCALLTYPE GetControlNode(ULONG ulInputPinId, ULONG ulOutputPinId, ULONG ulNodeType, IUnknown **ppControlNode);

    CBDADeviceControl(IUnknown * pUnkOuter, HANDLE hFile) : m_Ref(0), m_pUnkOuter(pUnkOuter), m_Handle(hFile){};
    virtual ~CBDADeviceControl(){};

protected:
    LONG m_Ref;
    IUnknown * m_pUnkOuter;
    HANDLE m_Handle;
};

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;

    *Output = NULL;

    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    if (IsEqualGUID(refiid, IID_IBDA_DeviceControl))
    {
        *Output = (IBDA_DeviceControl*)(this);
        reinterpret_cast<IBDA_DeviceControl*>(*Output)->AddRef();
        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_IBDA_Topology))
    {
        *Output = (IBDA_Topology*)(this);
        reinterpret_cast<IBDA_Topology*>(*Output)->AddRef();
        return NOERROR;
    }
#if 0
    if (IsEqualIID(refiid, IID_IDistributorNotify))
    {
        OutputDebugStringW(L"CBDADeviceControl::QueryInterface: No IDistributorNotify interface\n");
        return E_NOINTERFACE;
    }

    if (IsEqualGUID(refiid, IID_IAMOpenProgress))
    {
        OutputDebugStringW(L"CBDADeviceControl::QueryInterface: No IAMOpenProgress interface\n");
        return E_NOINTERFACE;
    }

    if (IsEqualGUID(refiid, IID_IBDA_NetworkProvider))
    {
        HRESULT hr = CoCreateInstance(CLSID_DVBTNetworkProvider, 0, CLSCTX_INPROC, IID_IBDA_NetworkProvider, (void**)Output);
        swprintf(Buffer, L"CBDADeviceControl::QueryInterface: failed to construct IID_IBDA_NetworkProvider interface with %lx", hr);
        OutputDebugStringW(Buffer);
        return hr;
    }

    if (IsEqualGUID(refiid, IID_IMatrixMixer))
    {
        OutputDebugStringW(L"CBDADeviceControl::QueryInterface: No IID_IMatrixMixer interface\n");
        return E_NOINTERFACE;
    }

    if (IsEqualGUID(refiid, IID_IAsyncReader))
    {
        OutputDebugStringW(L"CBDADeviceControl::QueryInterface: No IID_IAsyncReader interface\n");
        return E_NOINTERFACE;
    }

    if (IsEqualGUID(refiid, IID_IAC3Filter))
    {
        OutputDebugStringW(L"CBDADeviceControl::QueryInterface: No IID_IAC3Filter interface\n");
        return E_NOINTERFACE;
    }
#endif

    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CBDADeviceControl::QueryInterface: NoInterface for %s", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);

    return E_NOINTERFACE;
}


//-------------------------------------------------------------------
// IBDA_DeviceControl methods
//
HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::StartChanges( void)
{
    OutputDebugStringW(L"CBDADeviceControl::StartChanges: NotImplemented\n");
    return E_NOTIMPL;
}


HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::CheckChanges( void)
{
    OutputDebugStringW(L"CBDADeviceControl::CheckChanges: NotImplemented\n");
    return E_NOTIMPL;
}


HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::CommitChanges( void)
{
    OutputDebugStringW(L"CBDADeviceControl::CommitChanges: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetChangeState(ULONG *pState)
{
    OutputDebugStringW(L"CBDADeviceControl::GetChangeState: NotImplemented\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IBDA_Topology methods
//
HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetNodeTypes(ULONG *pulcNodeTypes, ULONG ulcNodeTypesMax, ULONG * rgulNodeTypes)
{
    KSPROPERTY Property;
    HRESULT hr;
    ULONG BytesReturned;
    WCHAR Buffer[100];

    // setup request
    Property.Set = KSPROPSETID_BdaTopology;
    Property.Id = KSPROPERTY_BDA_NODE_TYPES;
    Property.Flags = KSPROPERTY_TYPE_GET;

    // perform request
    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), rgulNodeTypes, sizeof(ULONG) * ulcNodeTypesMax, &BytesReturned);

    *pulcNodeTypes = (BytesReturned / sizeof(ULONG));

    swprintf(Buffer, L"CBDADeviceControl::GetNodeTypes: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetNodeDescriptors(ULONG *ulcNodeDescriptors, ULONG ulcNodeDescriptorsMax, BDANODE_DESCRIPTOR * rgNodeDescriptors)
{
    KSPROPERTY Property;
    HRESULT hr;
    ULONG BytesReturned;
    WCHAR Buffer[100];

    // setup request
    Property.Set = KSPROPSETID_BdaTopology;
    Property.Id = KSPROPERTY_BDA_NODE_DESCRIPTORS;
    Property.Flags = KSPROPERTY_TYPE_GET;

    // perform request
    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), rgNodeDescriptors, sizeof(BDANODE_DESCRIPTOR) * ulcNodeDescriptorsMax, &BytesReturned);

    *ulcNodeDescriptors = (BytesReturned / sizeof(BDANODE_DESCRIPTOR));

    swprintf(Buffer, L"CBDADeviceControl::GetNodeDescriptors: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetNodeInterfaces(ULONG ulNodeType, ULONG *pulcInterfaces, ULONG ulcInterfacesMax, GUID * rgguidInterfaces)
{
    KSP_NODE Property;
    HRESULT hr;
    ULONG BytesReturned;
    WCHAR Buffer[100];

    // setup request
    Property.Property.Set = KSPROPSETID_BdaTopology;
    Property.Property.Id = KSPROPERTY_BDA_NODE_PROPERTIES;
    Property.Property.Flags = KSPROPERTY_TYPE_GET;
    Property.NodeId = ulNodeType;
    Property.Reserved = 0;

    // perform request
    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSP_NODE), rgguidInterfaces, sizeof(GUID) * ulcInterfacesMax, &BytesReturned);

    *pulcInterfaces = (BytesReturned / sizeof(GUID));

    swprintf(Buffer, L"CBDADeviceControl::GetNodeInterfaces: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetPinTypes(ULONG *pulcPinTypes, ULONG ulcPinTypesMax, ULONG *rgulPinTypes)
{
    KSPROPERTY Property;
    HRESULT hr;
    ULONG BytesReturned;
    WCHAR Buffer[100];

    // setup request
    Property.Set = KSPROPSETID_BdaTopology;
    Property.Id = KSPROPERTY_BDA_PIN_TYPES;
    Property.Flags = KSPROPERTY_TYPE_GET;

    // perform request
    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), rgulPinTypes, sizeof(ULONG) * ulcPinTypesMax, &BytesReturned);

    *pulcPinTypes = (BytesReturned / sizeof(ULONG));

    swprintf(Buffer, L"CBDADeviceControl::GetPinTypes: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetTemplateConnections(ULONG *pulcConnections, ULONG ulcConnectionsMax, BDA_TEMPLATE_CONNECTION * rgConnections)
{
    OutputDebugStringW(L"CBDADeviceControl::GetTemplateConnections: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::CreatePin(ULONG ulPinType, ULONG *pulPinId)
{
    OutputDebugStringW(L"CBDADeviceControl::CreatePin: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::DeletePin(ULONG ulPinId)
{
    OutputDebugStringW(L"CBDADeviceControl::DeletePin: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::SetMediaType(ULONG ulPinId, AM_MEDIA_TYPE *pMediaType)
{
    OutputDebugStringW(L"CBDADeviceControl::SetMediaType: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::SetMedium(ULONG ulPinId, REGPINMEDIUM *pMedium)
{
    OutputDebugStringW(L"CBDADeviceControl::SetMedium: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::CreateTopology(ULONG ulInputPinId, ULONG ulOutputPinId)
{
    KSM_BDA_PIN_PAIR Method;
    WCHAR Buffer[100];
    HRESULT hr;
    ULONG BytesReturned = 0;

    Method.Method.Flags  = 0;
    Method.Method.Id = KSMETHOD_BDA_CREATE_TOPOLOGY;
    Method.Method.Set = KSMETHODSETID_BdaDeviceConfiguration;
    Method.InputPinId = ulInputPinId;
    Method.OutputPinId = ulOutputPinId;

    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_METHOD, (PVOID)&Method, sizeof(KSM_BDA_PIN_PAIR), NULL, 0, &BytesReturned);

    swprintf(Buffer, L"CBDADeviceControl::CreateTopology: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetControlNode(ULONG ulInputPinId, ULONG ulOutputPinId, ULONG ulNodeType, IUnknown **ppControlNode)
{
    KSP_BDA_NODE_PIN Property;
    ULONG Dummy = 0;
    HRESULT hr;
    ULONG PinId = 0;
    ULONG BytesReturned;
    WCHAR Buffer[100];

    //setup request
    Property.Property.Set = KSPROPSETID_BdaTopology;
    Property.Property.Id = KSPROPERTY_BDA_CONTROLLING_PIN_ID;
    Property.Property.Flags = KSPROPERTY_TYPE_GET;
    Property.ulInputPinId = ulInputPinId;
    Property.ulOutputPinId = ulOutputPinId;
    Property.ulNodeType = ulNodeType;

    // perform request
    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSP_BDA_NODE_PIN) + sizeof(ULONG), &PinId, sizeof(ULONG), &BytesReturned);

    swprintf(Buffer, L"CBDADeviceControl::GetControlNode: hr %lx, BytesReturned %lu PinId %lu Dummy %lu\n", hr, BytesReturned, PinId, Dummy);
    OutputDebugStringW(Buffer);

    if (FAILED(hr))
        return hr;

    hr = CControlNode_fnConstructor(m_pUnkOuter, ulNodeType, PinId, IID_IUnknown, (LPVOID*)ppControlNode);

    swprintf(Buffer, L"CBDADeviceControl::GetControlNode: hr %lx\n", hr);
    OutputDebugStringW(Buffer);
    return hr;
}

HRESULT
WINAPI
CBDADeviceControl_fnConstructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    HRESULT hr;
    IKsObject *pObject = NULL;
    HANDLE hFile;

    // sanity check
    assert(pUnkOuter);

    // query for IKsObject
    hr = pUnkOuter->QueryInterface(IID_IKsObject, (void**)&pObject);

    // sanity check
    assert(hr == NOERROR);

    // another sanity check
    assert(pObject != NULL);

    // get file handle
    hFile = pObject->KsGetObjectHandle();

    // one more sanity check
    assert(hFile != NULL && hFile != INVALID_HANDLE_VALUE);

    // release IKsObject interface
    pObject->Release();

    // construct device control
    CBDADeviceControl * handler = new CBDADeviceControl(pUnkOuter, hFile);

    OutputDebugStringW(L"CBDADeviceControl_fnConstructor\n");

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
