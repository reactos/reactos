/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceInterfaceUM.cpp

Abstract:

    This module implements the device interface object.

Author:



Environment:

    User mode only

Revision History:

--*/

#include "FxSupportPch.hpp"

extern "C" {
#include "FxDeviceInterfaceUM.tmh"
}

FxDeviceInterface::FxDeviceInterface(
    )
/*++

Routine Description:
    Constructor for the object.  Initializes all fields

Arguments:
    None

Return Value:
    None

  --*/
{
    RtlZeroMemory(&m_InterfaceClassGUID, sizeof(m_InterfaceClassGUID));

    RtlZeroMemory(&m_SymbolicLinkName, sizeof(m_SymbolicLinkName));
    RtlZeroMemory(&m_ReferenceString, sizeof(m_ReferenceString));

    m_Entry.Next = NULL;

    m_State = FALSE;
}

FxDeviceInterface::~FxDeviceInterface()
/*++

Routine Description:
    Destructor for FxDeviceInterface.  Cleans up any allocations previously
    allocated.

Arguments:
    None

Return Value:
    None

  --*/
{
    // the device interface should be off now
    ASSERT(m_State == FALSE);

    // should no longer be in any list
    ASSERT(m_Entry.Next == NULL);

    if (m_ReferenceString.Buffer != NULL) {
        FxPoolFree(m_ReferenceString.Buffer);
        RtlZeroMemory(&m_ReferenceString, sizeof(m_ReferenceString));
    }

    if (m_SymbolicLinkName.Buffer != NULL) {
        MxMemory::MxFreePool(m_SymbolicLinkName.Buffer);
    }
}

_Must_inspect_result_
NTSTATUS
FxDeviceInterface::Initialize(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in CONST GUID* InterfaceGUID,
    __in_opt PCUNICODE_STRING ReferenceString
    )
/*++

Routine Description:
    Initializes the object with the interface GUID and optional reference string

Arguments:
    InterfaceGUID - GUID describing the interface

    ReferenceString - string used to differentiate between 2 interfaces on the
        same PDO

Return Value:
    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES

  --*/
{
    RtlCopyMemory(&m_InterfaceClassGUID, InterfaceGUID, sizeof(GUID));

    if (ReferenceString != NULL) {
        return FxDuplicateUnicodeString(FxDriverGlobals,
                                        ReferenceString,
                                        &m_ReferenceString);
    }
    else {
        return STATUS_SUCCESS;
    }
}


VOID
FxDeviceInterface::SetState(
    __in BOOLEAN State
    )
/*++

Routine Description:
    Sets the state of the device interface

Arguments:
    State - the state to set


Return Value:
    None.

  --*/
{
    HRESULT hr;
    NTSTATUS status;
    IWudfDeviceStack *pDeviceStack;




    //
    // Get the IWudfDeviceStack interface
    //
    pDeviceStack = m_Device->GetDeviceStackInterface();

    //
    // Enable the interface
    //
    hr = pDeviceStack->SetDeviceInterfaceState(&this->m_InterfaceClassGUID,
                                               this->m_ReferenceString.Buffer,
                                               State);

    if (SUCCEEDED(hr)) {
        m_State = State;
    }
    else {
        status = FxDevice::NtStatusFromHr(pDeviceStack, hr);
        DoTraceLevelMessage(
            FxDevice::GetFxDevice(m_Device)->GetDriverGlobals(),
            TRACE_LEVEL_WARNING, TRACINGPNP,
            "Failed to %s device interface %!STATUS!",
            (State ? "enable" : "disable"), status);





    }
}

_Must_inspect_result_
NTSTATUS
FxDeviceInterface::Register(
    __in MdDeviceObject DeviceObject
    )
/*++

Routine Description:
    Registers the device interface for a given PDO

Arguments:
    DeviceObject - FDO for the device stack in case of UM, and PDO for
                   in case of KM.

Return Value:
    returned by IWudfDeviceStack::CreateDeviceInterface

  --*/
{
    HRESULT hr;
    NTSTATUS status;
    IWudfDeviceStack *pDeviceStack;

    m_Device = DeviceObject;

    //
    // Get the IWudfDeviceStack interface
    //
    pDeviceStack = m_Device->GetDeviceStackInterface();

    hr = pDeviceStack->CreateDeviceInterface(&m_InterfaceClassGUID,
                                             m_ReferenceString.Buffer);

    if (SUCCEEDED(hr)) {
        status = STATUS_SUCCESS;
    }
    else {
        status = FxDevice::NtStatusFromHr(pDeviceStack, hr);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDeviceInterface::Register(
    _In_ FxDevice* Device
    )
{
    NTSTATUS status;

    //
    // For UMDF, PDO is already known so no reason to defer registration.
    // Also, note that Register takes fdo as parameter for UMDF.
    //
    status = Register(Device->GetDeviceObject());

    return status;
}

NTSTATUS
FxDeviceInterface::GetSymbolicLinkName(
    _In_ FxString* LinkString
    )
{
    NTSTATUS status;
    PCWSTR symLink = NULL;

    if (m_SymbolicLinkName.Buffer == NULL) {
        IWudfDeviceStack *pDeviceStack;
        IWudfDeviceStack2 *pDeviceStack2;

        //
        // Get the IWudfDeviceStack interface
        //
        pDeviceStack = m_Device->GetDeviceStackInterface();
        HRESULT hrQI;
        HRESULT hr;

        hrQI = pDeviceStack->QueryInterface(IID_IWudfDeviceStack2,
                                            (PVOID*)&pDeviceStack2);
        FX_VERIFY(INTERNAL, CHECK_QI(hrQI, pDeviceStack2));
        pDeviceStack->Release();

        //
        // Get the symbolic link
        //
        hr = pDeviceStack2->GetInterfaceSymbolicLink(&m_InterfaceClassGUID,
                                                     m_ReferenceString.Buffer,
                                                     &symLink);
        if (FAILED(hr)) {
            status = FxDevice::GetFxDevice(m_Device)->NtStatusFromHr(hr);
        }
        else {
            RtlInitUnicodeString(&m_SymbolicLinkName, symLink);
            status = STATUS_SUCCESS;
        }
    }
    else {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {
        //
        // Attempt a copy
        //
        status = LinkString->Assign(&m_SymbolicLinkName);
    }

    return status;
}

