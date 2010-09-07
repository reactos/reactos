/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/miniport_dmus.cpp
 * PURPOSE:         DirectMusic miniport
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

class CMiniportDMus : public IMiniportDMus
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
    IMP_IMiniportDMus;
    CMiniportDMus(IUnknown *OuterUnknown){}
    virtual ~CMiniportDMus(){}

protected:
    LONG m_Ref;

};

// IUnknown methods

NTSTATUS
NTAPI
CMiniportDMus::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUIDAligned(refiid, IID_IMiniportDMus))
    {
        *Output = PVOID(PUNKNOWN(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

// IMiniport methods

NTSTATUS
NTAPI
CMiniportDMus::DataRangeIntersection(
    IN ULONG  PinId,
    IN PKSDATARANGE  DataRange,
    IN PKSDATARANGE  MatchingDataRange,
    IN ULONG OutputBufferLength,
    OUT PVOID ResultantFormat  OPTIONAL,
    OUT PULONG ResultantFormatLength)
{
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CMiniportDMus::GetDescription(
    OUT PPCFILTER_DESCRIPTOR  *Description)
{
    return STATUS_UNSUCCESSFUL;
}

HRESULT
NTAPI
CMiniportDMus::Init(
    IN PUNKNOWN  pUnknownAdapter,
    IN PRESOURCELIST  pResourceList,
    IN PPORTDMUS  pPort,
    OUT PSERVICEGROUP  *ppServiceGroup
    )
{
    return STATUS_UNSUCCESSFUL;
}

HRESULT
NTAPI
CMiniportDMus::NewStream(
    OUT PMXF  *ppMXF,
    IN PUNKNOWN  pOuterUnknown  OPTIONAL,
    IN POOL_TYPE  PoolType,
    IN ULONG  uPinId,
    IN DMUS_STREAM_TYPE  StreamType,
    IN PKSDATAFORMAT  pDataFormat,
    OUT PSERVICEGROUP  *ppServiceGroup,
    IN PAllocatorMXF  pAllocatorMXF,
    IN PMASTERCLOCK  pMasterClock,
    OUT PULONGLONG  puuSchedulePreFetch
    )
{
    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
CMiniportDMus::Service()
{

}

NTSTATUS
NewMiniportDMusUART(
    OUT PMINIPORT* OutMiniport,
    IN  REFCLSID ClassId)
{
    CMiniportDMus * This;

    This = new(NonPagedPool, TAG_PORTCLASS)CMiniportDMus(NULL);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    *OutMiniport = (PMINIPORT)This;
    This->AddRef();

    return STATUS_SUCCESS;
}


