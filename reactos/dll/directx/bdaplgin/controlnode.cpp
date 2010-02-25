/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS BDA Proxy
 * FILE:            dll/directx/bdaplgin/controlnode.cpp
 * PURPOSE:         ControlNode interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"

class CControlNode : public IUnknown
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

    CControlNode(HANDLE hFile, ULONG NodeType, ULONG PinId) : m_Ref(0), m_hFile(hFile), m_NodeType(NodeType), m_PinId(PinId){};
    virtual ~CControlNode(){};

protected:
    LONG m_Ref;
    HANDLE m_hFile;
    ULONG m_NodeType;
    ULONG m_PinId;
};

HRESULT
STDMETHODCALLTYPE
CControlNode::QueryInterface(
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
    else if(IsEqualGUID(refiid, IID_IBDA_FrequencyFilter))
    {
        return CBDAFrequencyFilter_fnConstructor(m_hFile, refiid, Output);
    }
    else if(IsEqualGUID(refiid, IID_IBDA_SignalStatistics))
    {
        return CBDASignalStatistics_fnConstructor(m_hFile, refiid, Output);
    }
    else if(IsEqualGUID(refiid, IID_IBDA_LNBInfo))
    {
        return CBDALNBInfo_fnConstructor(m_hFile, refiid, Output);
    }
    else if(IsEqualGUID(refiid, IID_IBDA_DigitalDemodulator))
    {
        return CBDADigitalDemodulator_fnConstructor(m_hFile, refiid, Output);
    }

    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CControlNode::QueryInterface: NoInterface for %s", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);

    return E_NOINTERFACE;
}


HRESULT
WINAPI
CControlNode_fnConstructor(
    HANDLE hFile,
    ULONG NodeType,
    ULONG PinId,
    REFIID riid,
    LPVOID * ppv)
{
    // construct device control
    CControlNode * handler = new CControlNode(hFile, NodeType, PinId);

    OutputDebugStringW(L"CControlNode_fnConstructor\n");

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
