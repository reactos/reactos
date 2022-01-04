/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxWmiProviderUm.cpp

Abstract:

    This module implements the FxWmiProvider object

Author:




Environment:

    User mode only

Revision History:


--*/

#include <fxmin.hpp>
#include <wdfwmi.h>
#include <FxCallback.hpp>
#include <FxPackage.hpp>
#include <FxWmiIrpHandler.hpp>
#include <FxWmiProvider.hpp>

#pragma warning(push)
#pragma warning(disable:4100) //unreferenced parameter

FxWmiProvider::FxWmiProvider(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_WMI_PROVIDER_CONFIG Config,
    __in CfxDevice* Device
    ) :
    FxNonPagedObject(FX_TYPE_WMI_PROVIDER,
                     sizeof(FxWmiProvider),
                     FxDriverGlobals),
    m_FunctionControl(FxDriverGlobals)
{
    UfxVerifierTrapNotImpl();
}

FxWmiProvider::~FxWmiProvider()
{
    UfxVerifierTrapNotImpl();
}

BOOLEAN
FxWmiProvider::Dispose(
    VOID
    )
{
    UfxVerifierTrapNotImpl();
    return FALSE;
}

_Must_inspect_result_
NTSTATUS
FxWmiProvider::_Create(
    __in PFX_DRIVER_GLOBALS CallersGlobals,
    __in WDFDEVICE Device,
    __in_opt PWDF_OBJECT_ATTRIBUTES ProviderAttributes,
    __in PWDF_WMI_PROVIDER_CONFIG WmiProviderConfig,
    __out WDFWMIPROVIDER* WmiProvider,
    __out FxWmiProvider** Provider
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
NTSTATUS
FxWmiProvider::AddInstanceLocked(
    __in  FxWmiInstance* Instance,
    __in  BOOLEAN NoErrorIfPresent,
    __out PBOOLEAN Update,
    __in  AddInstanceAction Action
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
NTSTATUS
FxWmiProvider::AddInstance(
     __in FxWmiInstance* Instance,
     __in BOOLEAN NoErrorIfPresent
     )
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

VOID
FxWmiProvider::RemoveInstance(
    __in FxWmiInstance* Instance
    )
{
    UfxVerifierTrapNotImpl();
}

ULONG
FxWmiProvider::GetInstanceIndex(
    __in FxWmiInstance* Instance
    )
{
    UfxVerifierTrapNotImpl();
    return ((ULONG) -1);
}

_Must_inspect_result_
FxWmiInstance*
FxWmiProvider::GetInstanceReferenced(
    __in ULONG Index,
    __in PVOID Tag
    )
{
    UfxVerifierTrapNotImpl();
    return NULL;
}

_Must_inspect_result_
FxWmiInstance*
FxWmiProvider::GetInstanceReferencedLocked(
    __in ULONG Index,
    __in PVOID Tag
    )
{
    UfxVerifierTrapNotImpl();
    return NULL;
}

_Must_inspect_result_
NTSTATUS
FxWmiProvider::FunctionControl(
    __in WDF_WMI_PROVIDER_CONTROL Control,
    __in BOOLEAN Enable
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

ULONG
FxWmiProvider::GetRegistrationFlagsLocked(
    VOID
    )
{
    UfxVerifierTrapNotImpl();
    return ((ULONG) -1);
}

#pragma warning(pop)
