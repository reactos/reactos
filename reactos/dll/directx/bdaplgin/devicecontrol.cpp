/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/classfactory.cpp
 * PURPOSE:         ClassFactory interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"

const GUID IID_IBDA_DeviceControl = {0xFD0A5AF3, 0xB41D, 0x11d2, {0x9C, 0x95, 0x00, 0xC0, 0x4F, 0x79, 0x71, 0xE0}};
const GUID IID_IBDA_Topology      = {0x79B56888, 0x7FEA, 0x4690, {0xB4, 0x5D, 0x38, 0xFD, 0x3C, 0x78, 0x49, 0xBE}};

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

    CBDADeviceControl(HANDLE hFile) : m_Ref(0), m_Handle(hFile){};
    virtual ~CBDADeviceControl(){};

protected:
    LONG m_Ref;
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
    OutputDebugStringW(L"CBDADeviceControl::GetNodeTypes: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetNodeDescriptors(ULONG *ulcNodeDescriptors, ULONG ulcNodeDescriptorsMax, BDANODE_DESCRIPTOR * rgNodeDescriptors)
{
    OutputDebugStringW(L"CBDADeviceControl::GetNodeDescriptors: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetNodeInterfaces(ULONG ulNodeType, ULONG *pulcInterfaces, ULONG ulcInterfacesMax, GUID * rgguidInterfaces)
{
    OutputDebugStringW(L"CBDADeviceControl::GetNodeInterfaces: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetPinTypes(ULONG *pulcPinTypes, ULONG ulcPinTypesMax, ULONG *rgulPinTypes)
{
    OutputDebugStringW(L"CBDADeviceControl::GetPinTypes: NotImplemented\n");
    return E_NOTIMPL;
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
CBDADeviceControl::CreateTopology(ULONG ulInputPinId, ULONG ulOutputPinId){
    OutputDebugStringW(L"CBDADeviceControl::CreateTopology: NotImplemented\n");
    return E_NOTIMPL;
}


HRESULT
STDMETHODCALLTYPE
CBDADeviceControl::GetControlNode(ULONG ulInputPinId, ULONG ulOutputPinId, ULONG ulNodeType, IUnknown **ppControlNode)
{
    OutputDebugStringW(L"CBDADeviceControl::GetControlNode: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
CBDADeviceControl_fnConstructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    CBDADeviceControl * handler = new CBDADeviceControl(NULL);

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
