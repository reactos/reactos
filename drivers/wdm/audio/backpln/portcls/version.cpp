/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/version.cpp
 * PURPOSE:         Implements IPortClsVersion interface
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#define NDEBUG
#include <debug.h>

class CPortClsVersion : public CUnknownImpl<IPortClsVersion>
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    IMP_IPortClsVersion;

    CPortClsVersion(IUnknown *OuterUnknown)
    {
    }
    virtual ~CPortClsVersion()
    {

    }
};

//---------------------------------------------------------------
// IPortClsVersion interface functions
//

NTSTATUS
NTAPI
CPortClsVersion::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;

    if (IsEqualGUIDAligned(refiid, IID_IPortClsVersion) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PPORTCLSVERSION(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT1("CPortClsVersion::QueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }

    return STATUS_UNSUCCESSFUL;
}

DWORD
NTAPI
CPortClsVersion::GetVersion()
{
    return kVersionWinServer2003_UAAQFE;
}

NTSTATUS NewPortClsVersion(
    OUT PPORTCLSVERSION * OutVersion)
{
    CPortClsVersion * This = new(NonPagedPool, TAG_PORTCLASS) CPortClsVersion(NULL);

    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();

    *OutVersion = (PPORTCLSVERSION)This;

    return STATUS_SUCCESS;
}
