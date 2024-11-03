/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/msvidctl/tuningspace_container.cpp
 * PURPOSE:         ITuningSpaceContainer interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */
#define _FORCENAMELESSUNION
#include "precomp.h"


class CTuningSpaceContainer : public ITuningSpaceContainer
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
            OutputDebugStringW(L"CTuningSpaceContainer::Release : delete\n");
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

     //ITuningSpaceContainer methods
     HRESULT STDMETHODCALLTYPE get_Count(long *Count);
     HRESULT STDMETHODCALLTYPE get__NewEnum(IEnumVARIANT **NewEnum);
     HRESULT STDMETHODCALLTYPE get_Item(VARIANT varIndex, ITuningSpace **TuningSpace);
     HRESULT STDMETHODCALLTYPE put_Item(VARIANT varIndex, ITuningSpace *TuningSpace);
     HRESULT STDMETHODCALLTYPE TuningSpacesForCLSID(BSTR SpaceCLSID, ITuningSpaces **NewColl);
     HRESULT STDMETHODCALLTYPE _TuningSpacesForCLSID(REFCLSID SpaceCLSID, ITuningSpaces **NewColl);
     HRESULT STDMETHODCALLTYPE TuningSpacesForName(BSTR Name, ITuningSpaces **NewColl);
     HRESULT STDMETHODCALLTYPE FindID(ITuningSpace *TuningSpace, long *ID);
     HRESULT STDMETHODCALLTYPE Add(ITuningSpace *TuningSpace, VARIANT *NewIndex);
     HRESULT STDMETHODCALLTYPE get_EnumTuningSpaces(IEnumTuningSpaces **ppEnum);
     HRESULT STDMETHODCALLTYPE Remove(VARIANT Index);
     HRESULT STDMETHODCALLTYPE get_MaxCount(long *MaxCount);
     HRESULT STDMETHODCALLTYPE put_MaxCount(long MaxCount);

    CTuningSpaceContainer() : m_Ref(0){};

    virtual ~CTuningSpaceContainer(){};

protected:
    LONG m_Ref;

};

HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_ITuningSpaceContainer))
    {
        *Output = (ITuningSpaceContainer*)this;
        reinterpret_cast<ITuningSpaceContainer*>(*Output)->AddRef();
        return NOERROR;
    }

    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CTuningSpaceContainer::QueryInterface: NoInterface for %s", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);


    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
// IDispatch methods
//
HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::GetTypeInfoCount(UINT *pctinfo)
{
    OutputDebugStringW(L"CTuningSpaceContainer::GetTypeInfoCount : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    OutputDebugStringW(L"CTuningSpaceContainer::GetTypeInfo : NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    OutputDebugStringW(L"CTuningSpaceContainer::GetIDsOfNames : NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    OutputDebugStringW(L"CTuningSpaceContainer::Invoke : NotImplemented\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
// ITuningSpaceContainer methods
//

HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::get_Count(long *Count)
{
    OutputDebugStringW(L"CTuningSpaceContainer::get_Count : NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::get__NewEnum(IEnumVARIANT **NewEnum)
{
    OutputDebugStringW(L"CTuningSpaceContainer::get__NewEnum : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::get_Item(VARIANT varIndex, ITuningSpace **TuningSpace)
{
#ifdef MSVIDCTL_TRACE
    WCHAR Buffer[100];
	swprintf(Buffer, L"CTuningSpaceContainer::get_Item : type %x value %s stub\n", varIndex.vt, varIndex.bstrVal);
    OutputDebugStringW(Buffer);
#endif

    return CTuningSpace_fnConstructor(NULL, IID_ITuningSpace, (void**)TuningSpace);
}
HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::put_Item(VARIANT varIndex, ITuningSpace *TuningSpace)
{
    OutputDebugStringW(L"CTuningSpaceContainer::put_Item : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::TuningSpacesForCLSID(BSTR SpaceCLSID, ITuningSpaces **NewColl)
{
    OutputDebugStringW(L"CTuningSpaceContainer::TuningSpacesForCLSID : NotImplemented\n");
    return E_NOTIMPL;
}
HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::_TuningSpacesForCLSID(REFCLSID SpaceCLSID, ITuningSpaces **NewColl)
{
    OutputDebugStringW(L"CTuningSpaceContainer::_TuningSpacesForCLSID : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::TuningSpacesForName(BSTR Name, ITuningSpaces **NewColl)
{
    OutputDebugStringW(L"CTuningSpaceContainer::TuningSpacesForName : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::FindID(ITuningSpace *TuningSpace, long *ID)
{
    OutputDebugStringW(L"CTuningSpaceContainer::FindID : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::Add(ITuningSpace *TuningSpace, VARIANT *NewIndex)
{
    OutputDebugStringW(L"CTuningSpaceContainer::Add : stub\n");
    TuningSpace->AddRef();
    NewIndex->vt = VT_BSTR;
    InterlockedIncrement(&m_Ref);
    return TuningSpace->get_FriendlyName(&NewIndex->bstrVal);
}
HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::get_EnumTuningSpaces(IEnumTuningSpaces **ppEnum)
{
    OutputDebugStringW(L"CTuningSpaceContainer::get_EnumTuningSpaces : stub\n");
    return CEnumTuningSpaces_fnConstructor(NULL, IID_IEnumTuningSpaces, (void**)ppEnum);
}

HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::Remove(VARIANT Index)
{
    OutputDebugStringW(L"CTuningSpaceContainer::Remove: NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::get_MaxCount(long *MaxCount)
{
    OutputDebugStringW(L"CTuningSpaceContainer::get_MaxCount : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CTuningSpaceContainer::put_MaxCount(long MaxCount)
{
    OutputDebugStringW(L"CTuningSpaceContainer::put_MaxCount : NotImplemented\n");
    return E_NOTIMPL;
}


HRESULT
WINAPI
CTuningSpaceContainer_fnConstructor(
    IUnknown *pUnknown,
    REFIID riid,
    LPVOID * ppv)
{
    // construct device control
    CTuningSpaceContainer * provider = new CTuningSpaceContainer();

#ifdef MSVIDCTL_TRACE
    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(riid, &lpstr);
    swprintf(Buffer, L"CTuningSpaceContainer_fnConstructor riid %s pUnknown %p\n", lpstr, pUnknown);
    OutputDebugStringW(Buffer);
#endif

    if (!provider)
        return E_OUTOFMEMORY;

    if (FAILED(provider->QueryInterface(riid, ppv)))
    {
        /* not supported */
        delete provider;
        return E_NOINTERFACE;
    }

    return NOERROR;
}
