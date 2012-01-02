/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/msvidctl/tuningspace.cpp
 * PURPOSE:         ITuningSpace interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

const GUID CLSID_DVBTNetworkProvider = {0x216c62df, 0x6d7f, 0x4e9a, {0x85, 0x71, 0x5, 0xf1, 0x4e, 0xdb, 0x76, 0x6a}};

class CTuningSpace : public IDVBTuningSpace
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

            WCHAR Buffer[100];
            swprintf(Buffer, L"CTuningSpace::Release : %p Ref %lu\n", this, m_Ref);
            OutputDebugStringW(Buffer);

        if (!m_Ref)
        {
            //delete this;
            return 0;
        }
        return m_Ref;
    }

     // IDispatch methods
     HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
     HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
     HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
     HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);


    //ITuningSpace methods
    HRESULT STDMETHODCALLTYPE get_UniqueName(BSTR *Name);
    HRESULT STDMETHODCALLTYPE put_UniqueName(BSTR Name);
    HRESULT STDMETHODCALLTYPE get_FriendlyName(BSTR *Name);
    HRESULT STDMETHODCALLTYPE put_FriendlyName(BSTR Name);
    HRESULT STDMETHODCALLTYPE get_CLSID(BSTR *SpaceCLSID);
    HRESULT STDMETHODCALLTYPE get_NetworkType(BSTR *NetworkTypeGuid);
    HRESULT STDMETHODCALLTYPE put_NetworkType(BSTR NetworkTypeGuid);
    HRESULT STDMETHODCALLTYPE get__NetworkType(GUID *NetworkTypeGuid);
    HRESULT STDMETHODCALLTYPE put__NetworkType(REFCLSID NetworkTypeGuid);
    HRESULT STDMETHODCALLTYPE CreateTuneRequest(ITuneRequest **TuneRequest);
    HRESULT STDMETHODCALLTYPE EnumCategoryGUIDs(IEnumGUID **ppEnum);
    HRESULT STDMETHODCALLTYPE EnumDeviceMonikers(IEnumMoniker **ppEnum);
    HRESULT STDMETHODCALLTYPE get_DefaultPreferredComponentTypes(IComponentTypes **ComponentTypes);
    HRESULT STDMETHODCALLTYPE put_DefaultPreferredComponentTypes(IComponentTypes *NewComponentTypes);
    HRESULT STDMETHODCALLTYPE get_FrequencyMapping(BSTR *pMapping);
    HRESULT STDMETHODCALLTYPE put_FrequencyMapping(BSTR Mapping);
    HRESULT STDMETHODCALLTYPE get_DefaultLocator(ILocator **LocatorVal);
    HRESULT STDMETHODCALLTYPE put_DefaultLocator(ILocator *LocatorVal);
    HRESULT STDMETHODCALLTYPE Clone(ITuningSpace **NewTS);
    // IDVBTuningSpace
    HRESULT STDMETHODCALLTYPE get_SystemType(DVBSystemType *SysType);
    HRESULT STDMETHODCALLTYPE put_SystemType(DVBSystemType SysType);

    CTuningSpace() : m_Ref(0){};

    virtual ~CTuningSpace(){};

protected:
    LONG m_Ref;
};

HRESULT
STDMETHODCALLTYPE
CTuningSpace::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_ITuningSpace))
    {
        *Output = (ITuningSpace*)this;
        reinterpret_cast<ITuningSpace*>(*Output)->AddRef();
        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_IDVBTuningSpace))
    {
        *Output = (IDVBTuningSpace*)this;
        reinterpret_cast<IDVBTuningSpace*>(*Output)->AddRef();
        return NOERROR;
    }


    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CTuningSpace::QueryInterface: NoInterface for %s", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);


    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IDispatch methods
//
HRESULT
STDMETHODCALLTYPE
CTuningSpace::GetTypeInfoCount(UINT *pctinfo)
{
    OutputDebugStringW(L"CTuningSpace::GetTypeInfoCount : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    OutputDebugStringW(L"CTuningSpace::GetTypeInfo : NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CTuningSpace::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    OutputDebugStringW(L"CTuningSpace::GetIDsOfNames : NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CTuningSpace::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    OutputDebugStringW(L"CTuningSpace::Invoke : NotImplemented\n");
    return E_NOTIMPL;
}


//-------------------------------------------------------------------
// ITuningSpace interface
//

HRESULT
STDMETHODCALLTYPE
CTuningSpace::get_UniqueName(BSTR *Name)
{
    OutputDebugStringW(L"CTuningSpace::get_UniqueName : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::put_UniqueName(BSTR Name)
{
    OutputDebugStringW(L"CTuningSpace::put_UniqueName : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::get_FriendlyName(BSTR *Name)
{
    OutputDebugStringW(L"CTuningSpace::get_FriendlyName : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::put_FriendlyName(BSTR Name)
{
    OutputDebugStringW(L"CTuningSpace::put_FriendlyName : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::get_CLSID(BSTR *SpaceCLSID)
{
    OutputDebugStringW(L"CTuningSpace::get_CLSID : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::get_NetworkType(BSTR *NetworkTypeGuid)
{
    OutputDebugStringW(L"CTuningSpace::get_NetworkType : stub\n");
    return StringFromCLSID(CLSID_DVBTNetworkProvider, (LPOLESTR*)NetworkTypeGuid);

}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::put_NetworkType(BSTR NetworkTypeGuid)
{
    OutputDebugStringW(L"CTuningSpace::put_NetworkType : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::get__NetworkType(GUID *NetworkTypeGuid)
{
#ifdef MSVIDCTL_TRACE
    WCHAR Buffer[100];
    swprintf(Buffer, L"CTuningSpace::get__NetworkType : %p stub\n", NetworkTypeGuid);
    OutputDebugStringW(Buffer);
#endif

    CopyMemory(NetworkTypeGuid, &CLSID_DVBTNetworkProvider, sizeof(GUID));
    OutputDebugStringW(L"CTuningSpace::get__NetworkType : done\n");
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::put__NetworkType(REFCLSID NetworkTypeGuid)
{
    OutputDebugStringW(L"CTuningSpace::put__NetworkType : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::CreateTuneRequest(ITuneRequest **TuneRequest)
{
    OutputDebugStringW(L"CTuningSpace::CreateTuneRequest : stub\n");
    return CTuneRequest_fnConstructor(NULL, (ITuningSpace*)this, IID_ITuneRequest, (void**)TuneRequest);
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::EnumCategoryGUIDs(IEnumGUID **ppEnum)
{
    OutputDebugStringW(L"CTuningSpace::EnumCategoryGUIDs : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::EnumDeviceMonikers(IEnumMoniker **ppEnum)
{
    OutputDebugStringW(L"CTuningSpace::EnumDeviceMonikers : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::get_DefaultPreferredComponentTypes(IComponentTypes **ComponentTypes)
{
    OutputDebugStringW(L"CTuningSpace::get_DefaultPreferredComponentTypes : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::put_DefaultPreferredComponentTypes(IComponentTypes *NewComponentTypes)
{
    OutputDebugStringW(L"CTuningSpace::put_DefaultPreferredComponentTypes : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::get_FrequencyMapping(BSTR *pMapping)
{
    OutputDebugStringW(L"CTuningSpace::get_FrequencyMapping : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::put_FrequencyMapping(BSTR Mapping)
{
    OutputDebugStringW(L"CTuningSpace::put_FrequencyMapping : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::get_DefaultLocator(ILocator **LocatorVal)
{
    OutputDebugStringW(L"CTuningSpace::get_DefaultLocator : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::put_DefaultLocator(ILocator *LocatorVal)
{
    OutputDebugStringW(L"CTuningSpace::put_DefaultLocator : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::Clone(ITuningSpace **NewTS)
{
    OutputDebugStringW(L"CTuningSpace::Clone : NotImplemented\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// IDVBTuningSpace
//
HRESULT
STDMETHODCALLTYPE
CTuningSpace::get_SystemType(DVBSystemType *SysType)
{
    OutputDebugStringW(L"CTuningSpace::get_SystemType : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpace::put_SystemType(DVBSystemType SysType)
{
    OutputDebugStringW(L"CTuningSpace::put_SystemType : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
WINAPI
CTuningSpace_fnConstructor(
    IUnknown *pUnknown,
    REFIID riid,
    LPVOID * ppv)
{
    // construct device control
    CTuningSpace * space = new CTuningSpace();

#ifdef MSVIDCTL_TRACE
    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(riid, &lpstr);
    swprintf(Buffer, L"CTuningSpace_fnConstructor riid %s pUnknown %p\n", lpstr, pUnknown);
    OutputDebugStringW(Buffer);
#endif

    if (!space)
        return E_OUTOFMEMORY;

    if (FAILED(space->QueryInterface(riid, ppv)))
    {
        /* not supported */
        delete space;
        return E_NOINTERFACE;
    }

    return NOERROR;
}


