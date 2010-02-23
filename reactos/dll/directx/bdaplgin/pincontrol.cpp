/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/classfactory.cpp
 * PURPOSE:         ClassFactory interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"

const GUID IID_IBDA_PinControl = {0x0DED49D5, 0xA8B7, 0x4d5d, {0x97, 0xA1, 0x12, 0xB0, 0xC1, 0x95, 0x87, 0x4D}};

class CBDAPinControl : public IBDA_PinControl
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
    // IBDA_PinControl methods
    HRESULT STDMETHODCALLTYPE GetPinID(ULONG *pulPinID);
    HRESULT STDMETHODCALLTYPE GetPinType(ULONG *pulPinType);
    HRESULT STDMETHODCALLTYPE RegistrationContext(ULONG *pulRegistrationCtx);


    CBDAPinControl(HANDLE hFile) : m_Ref(0), m_Handle(hFile){};
    virtual ~CBDAPinControl(){};

protected:
    LONG m_Ref;
    HANDLE m_Handle;

};

HRESULT
STDMETHODCALLTYPE
CBDAPinControl::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    if (IsEqualGUID(refiid, IID_IBDA_PinControl))
    {
        *Output = (IBDA_PinControl*)(this);
        reinterpret_cast<IBDA_PinControl*>(*Output)->AddRef();
        return NOERROR;
    }
   OutputDebugStringW(L"CBDAPinControl::QueryInterface: NoInterface!!!\n");
    return E_NOINTERFACE;
}
//-------------------------------------------------------------------
// IBDA_PinControl methods
//
HRESULT
STDMETHODCALLTYPE
CBDAPinControl::GetPinID(ULONG *pulPinID)
{
    OutputDebugStringW(L"CBDAPinControl::GetPinID: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDAPinControl::GetPinType(ULONG *pulPinType)
{
    OutputDebugStringW(L"CBDAPinControl::GetPinType: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CBDAPinControl::RegistrationContext(ULONG *pulRegistrationCtx)
{
    OutputDebugStringW(L"CBDAPinControl::RegistrationContext: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
CBDAPinControl_fnConstructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    CBDAPinControl * handler = new CBDAPinControl(NULL);

    OutputDebugStringW(L"CBDAPinControl_fnConstructor");

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