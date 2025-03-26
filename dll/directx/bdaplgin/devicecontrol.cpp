/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/devicecontrol.cpp
 * PURPOSE:         ClassFactory interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

const GUID IID_IKsObject           = {0x423c13a2, 0x2070, 0x11d0, {0x9e, 0xf7, 0x00, 0xaa, 0x00, 0xa2, 0x16, 0xa1}};

#ifndef _MSC_VER
const GUID CLSID_DVBTNetworkProvider = {0x216c62df, 0x6d7f, 0x4e9a, {0x85, 0x71, 0x5, 0xf1, 0x4e, 0xdb, 0x76, 0x6a}};

const GUID KSPROPSETID_BdaTopology = {0xa14ee835, 0x0a23, 0x11d3, {0x9c, 0xc7, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0}};
const GUID KSMETHODSETID_BdaDeviceConfiguration = {0x71985f45, 0x1ca1, 0x11d3, {0x9c, 0xc8, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0}};
const GUID KSMETHODSETID_BdaChangeSync = {0xfd0a5af3, 0xb41d, 0x11d2, {0x9c, 0x95, 0x0, 0xc0, 0x4f, 0x79, 0x71, 0xe0}};
const GUID IID_IBaseFilter         = {0x56a86895, 0x0ad4, 0x11ce, {0xb0,0x3a, 0x00,0x20,0xaf,0x0b,0xa7,0x70}};
const GUID IID_IAsyncReader       = {0x56A868AA, 0x0AD4, 0x11CE, {0xB0, 0x3A, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70}};
const GUID IID_IAMOpenProgress    = {0x8E1C39A1, 0xDE53, 0x11cf, {0xAA, 0x63, 0x00, 0x80, 0xC7, 0x44, 0x52, 0x8D}};
const GUID IID_IBDA_Topology      = {0x79B56888, 0x7FEA, 0x4690, {0xB4, 0x5D, 0x38, 0xFD, 0x3C, 0x78, 0x49, 0xBE}};
const GUID IID_IBDA_NetworkProvider   = {0xfd501041, 0x8ebe, 0x11ce, {0x81, 0x83, 0x00, 0xaa, 0x00, 0x57, 0x7d, 0xa2}};
const GUID IID_IBDA_DeviceControl = {0xFD0A5AF3, 0xB41D, 0x11d2, {0x9C, 0x95, 0x00, 0xC0, 0x4F, 0x79, 0x71, 0xE0}};

const GUID IID_IDistributorNotify = {0x56a868af, 0x0ad4, 0x11ce, {0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};

#endif

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

    CBDADeviceControl(IUnknown * pUnkOuter, IBaseFilter *pFilter, HANDLE hFile) : m_Ref(0), m_pUnkOuter(pUnkOuter), m_Handle(hFile), m_pFilter(pFilter){};
    virtual ~CBDADeviceControl(){};

protected:
    LONG m_Ref;
    IUnknown * m_pUnkOuter;
    HANDLE m_Handle;
    IBaseFilter * m_pFilter;
};

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
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

    return E_NOINTERFACE;
}


//-------------------------------------------------------------------
// IBDA_DeviceControl methods
//
HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::StartChanges( void)
{
    KSMETHOD Method;
    HRESULT hr;
    ULONG BytesReturned;

    /* setup request */
    Method.Set = KSMETHODSETID_BdaChangeSync;
    Method.Id = KSMETHOD_BDA_START_CHANGES;
    Method.Flags = KSMETHOD_TYPE_NONE;

    /* execute request */
    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_METHOD, (PVOID)&Method, sizeof(KSMETHOD), NULL, 0, &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDADeviceControl::StartChanges: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}


HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::CheckChanges( void)
{
    KSMETHOD Method;
    HRESULT hr;
    ULONG BytesReturned;

    /* setup request */
    Method.Set = KSMETHODSETID_BdaChangeSync;
    Method.Id = KSMETHOD_BDA_CHECK_CHANGES;
    Method.Flags = KSMETHOD_TYPE_NONE;

    /* execute request */
    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_METHOD, (PVOID)&Method, sizeof(KSMETHOD), NULL, 0, &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDADeviceControl::CheckChanges: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}


HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::CommitChanges( void)
{
    KSMETHOD Method;
    HRESULT hr;
    ULONG BytesReturned;

    /* setup request */
    Method.Set = KSMETHODSETID_BdaChangeSync;
    Method.Id = KSMETHOD_BDA_COMMIT_CHANGES;
    Method.Flags = KSMETHOD_TYPE_NONE;

    /* execute request */
    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_METHOD, (PVOID)&Method, sizeof(KSMETHOD), NULL, 0, &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDADeviceControl::CommitChanges: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetChangeState(ULONG *pState)
{
    if (pState)
    {
        *pState = BDA_CHANGES_COMPLETE;
        return S_OK;
    }
    else
    {
        return E_POINTER;
    }
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

    // setup request
    Property.Set = KSPROPSETID_BdaTopology;
    Property.Id = KSPROPERTY_BDA_NODE_TYPES;
    Property.Flags = KSPROPERTY_TYPE_GET;

    // perform request
    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), rgulNodeTypes, sizeof(ULONG) * ulcNodeTypesMax, &BytesReturned);

    *pulcNodeTypes = (BytesReturned / sizeof(ULONG));

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDADeviceControl::GetNodeTypes: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);

    if (SUCCEEDED(hr))
    {
        for(ULONG Index = 0; Index < *pulcNodeTypes; Index++)
        {
            swprintf(Buffer, L"CBDADeviceControl::GetPinTypes: Index %lu Value %lx\n", Index, rgulNodeTypes[Index]);
            OutputDebugStringW(Buffer);
        }
    }
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetNodeDescriptors(ULONG *ulcNodeDescriptors, ULONG ulcNodeDescriptorsMax, BDANODE_DESCRIPTOR * rgNodeDescriptors)
{
    KSPROPERTY Property;
    HRESULT hr;
    ULONG BytesReturned;


    // setup request
    Property.Set = KSPROPSETID_BdaTopology;
    Property.Id = KSPROPERTY_BDA_NODE_DESCRIPTORS;
    Property.Flags = KSPROPERTY_TYPE_GET;

    // perform request
    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), rgNodeDescriptors, sizeof(BDANODE_DESCRIPTOR) * ulcNodeDescriptorsMax, &BytesReturned);

    *ulcNodeDescriptors = (BytesReturned / sizeof(BDANODE_DESCRIPTOR));

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[1000];
    swprintf(Buffer, L"CBDADeviceControl::GetNodeDescriptors: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);


    if (SUCCEEDED(hr))
    {
        for(ULONG Index = 0; Index < min(*ulcNodeDescriptors, ulcNodeDescriptorsMax); Index++)
        {
            LPOLESTR pGUIDFunction, pGUIDName;

            StringFromCLSID(rgNodeDescriptors[Index].guidFunction, &pGUIDFunction);
            StringFromCLSID(rgNodeDescriptors[Index].guidName, &pGUIDName);

            swprintf(Buffer, L"CBDADeviceControl::GetPinTypes: Index %lu Value %lx\nFunction %s\n Name %s\n-----\n", Index, rgNodeDescriptors[Index].ulBdaNodeType, pGUIDFunction, pGUIDName);
            OutputDebugStringW(Buffer);
        }
    }
#endif


    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetNodeInterfaces(ULONG ulNodeType, ULONG *pulcInterfaces, ULONG ulcInterfacesMax, GUID * rgguidInterfaces)
{
    KSP_NODE Property;
    HRESULT hr;
    ULONG BytesReturned;


    // setup request
    Property.Property.Set = KSPROPSETID_BdaTopology;
    Property.Property.Id = KSPROPERTY_BDA_NODE_PROPERTIES;
    Property.Property.Flags = KSPROPERTY_TYPE_GET;
    Property.NodeId = ulNodeType;
    Property.Reserved = 0;

    // perform request
    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSP_NODE), rgguidInterfaces, sizeof(GUID) * ulcInterfacesMax, &BytesReturned);

    *pulcInterfaces = (BytesReturned / sizeof(GUID));

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDADeviceControl::GetNodeInterfaces: hr %lx, BytesReturned %lu ulNodeType %lu\n", hr, BytesReturned, ulNodeType);
    OutputDebugStringW(Buffer);

    if (SUCCEEDED(hr))
    {
        for(ULONG Index = 0; Index < min(*pulcInterfaces, ulcInterfacesMax); Index++)
        {
            LPOLESTR pstr;

            StringFromCLSID(rgguidInterfaces[Index], &pstr);

            swprintf(Buffer, L"CBDADeviceControl::GetNodeInterfaces: Index %lu Name %s\n", Index, pstr);
            OutputDebugStringW(Buffer);
        }
    }
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetPinTypes(ULONG *pulcPinTypes, ULONG ulcPinTypesMax, ULONG *rgulPinTypes)
{
    KSPROPERTY Property;
    HRESULT hr;
    ULONG BytesReturned;

    // setup request
    Property.Set = KSPROPSETID_BdaTopology;
    Property.Id = KSPROPERTY_BDA_PIN_TYPES;
    Property.Flags = KSPROPERTY_TYPE_GET;

    // perform request
    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), rgulPinTypes, sizeof(ULONG) * ulcPinTypesMax, &BytesReturned);

    *pulcPinTypes = (BytesReturned / sizeof(ULONG));

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDADeviceControl::GetPinTypes: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);

    if (SUCCEEDED(hr))
    {
        for(ULONG Index = 0; Index < *pulcPinTypes; Index++)
        {
            swprintf(Buffer, L"CBDADeviceControl::GetPinTypes: Index %lu Value %lx\n", Index, rgulPinTypes[Index]);
            OutputDebugStringW(Buffer);
        }
    }
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetTemplateConnections(ULONG *pulcConnections, ULONG ulcConnectionsMax, BDA_TEMPLATE_CONNECTION * rgConnections)
{
#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CBDADeviceControl::GetTemplateConnections: NotImplemented\n");
#endif

    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::CreatePin(ULONG ulPinType, ULONG *pulPinId)
{
#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CBDADeviceControl::CreatePin: NotImplemented\n");
#endif

    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::DeletePin(ULONG ulPinId)
{
#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CBDADeviceControl::DeletePin: NotImplemented\n");
#endif

    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::SetMediaType(ULONG ulPinId, AM_MEDIA_TYPE *pMediaType)
{
#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CBDADeviceControl::SetMediaType: NotImplemented\n");
#endif

    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::SetMedium(ULONG ulPinId, REGPINMEDIUM *pMedium)
{
#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CBDADeviceControl::SetMedium: NotImplemented\n");
#endif

    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::CreateTopology(ULONG ulInputPinId, ULONG ulOutputPinId)
{
    KSM_BDA_PIN_PAIR Method;
    HRESULT hr;
    ULONG BytesReturned = 0;

    Method.Method.Flags  = KSMETHOD_TYPE_NONE;
    Method.Method.Id = KSMETHOD_BDA_CREATE_TOPOLOGY;
    Method.Method.Set = KSMETHODSETID_BdaDeviceConfiguration;
    Method.InputPinId = ulInputPinId;
    Method.OutputPinId = ulOutputPinId;

    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_METHOD, (PVOID)&Method, sizeof(KSM_BDA_PIN_PAIR), NULL, 0, &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CBDADeviceControl::CreateTopology: hr %lx, BytesReturned %lu\n", hr, BytesReturned);
    OutputDebugStringW(Buffer);
#endif

    return hr;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetControlNode(ULONG ulInputPinId, ULONG ulOutputPinId, ULONG ulNodeType, IUnknown **ppControlNode)
{
    HRESULT hr;
    ULONG PinId = 0;
    ULONG BytesReturned;
    KSP_BDA_NODE_PIN Property;

    //setup request
    Property.Property.Set = KSPROPSETID_BdaTopology;
    Property.Property.Id = KSPROPERTY_BDA_CONTROLLING_PIN_ID;
    Property.Property.Flags = KSPROPERTY_TYPE_GET;
    Property.ulInputPinId = ulInputPinId;
    Property.ulOutputPinId = ulOutputPinId;
    Property.ulNodeType = ulNodeType;

    // perform request
    // WinXP SP3 expects minimum sizeof(KSP_BDA_NODE_PIN) + sizeof(ULONG)
    // seems a driver to be a driver bug

    hr = KsSynchronousDeviceControl(m_Handle, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSP_BDA_NODE_PIN) + sizeof(ULONG), &PinId, sizeof(ULONG), &BytesReturned);

#ifdef BDAPLGIN_TRACE
    WCHAR Buffer[200];
    swprintf(Buffer, L"CBDADeviceControl::GetControlNode: hr %lx, BytesReturned %lu PinId %lu ulInputPinId %lu ulOutputPinId %lu ulNodeType %lu\n", hr, BytesReturned, PinId, ulInputPinId, ulOutputPinId, ulNodeType);
    OutputDebugStringW(Buffer);
#endif

    if (FAILED(hr))
        return hr;

    hr = CControlNode_fnConstructor(m_pFilter, ulNodeType, PinId, IID_IUnknown, (LPVOID*)ppControlNode);

#ifdef BDAPLGIN_TRACE
    swprintf(Buffer, L"CBDADeviceControl::GetControlNode: hr %lx\n", hr);
    OutputDebugStringW(Buffer);
#endif

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
    IBaseFilter *pFilter = NULL;
    HANDLE hFile;

#ifdef BDAPLGIN_TRACE
    OutputDebugStringW(L"CBDADeviceControl_fnConstructor\n");
#endif

    //DebugBreak();

    // sanity check
    assert(pUnkOuter);

    // query for IKsObject
    hr = pUnkOuter->QueryInterface(IID_IKsObject, (void**)&pObject);

    if (FAILED(hr))
        return E_NOINTERFACE;

    // sanity check
    assert(hr == NOERROR);

    // query for IBaseFilter interface support
    hr = pUnkOuter->QueryInterface(IID_IBaseFilter, (void**)&pFilter);

    if (FAILED(hr))
    {
        // release
       pObject->Release();
       return E_NOINTERFACE;
    }

    // another sanity check
    assert(pObject != NULL);

    // get file handle
    hFile = pObject->KsGetObjectHandle();

    // one more sanity check
    assert(hFile != NULL && hFile != INVALID_HANDLE_VALUE);

    // release IKsObject interface
    pObject->Release();

    // release filter
    pFilter->Release();

    // construct device control
    CBDADeviceControl * handler = new CBDADeviceControl(pUnkOuter, pFilter, hFile);

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
