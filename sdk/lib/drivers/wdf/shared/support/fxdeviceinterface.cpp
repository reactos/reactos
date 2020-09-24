/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceInterface.cpp

Abstract:

    This module implements the device interface object.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "FxSupportPch.hpp"

extern "C" {
#include "FxDeviceInterface.tmh"
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

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    m_Device = NULL;
#endif

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
        RtlFreeUnicodeString(&m_SymbolicLinkName);
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
    m_State = State;

    //
    // Only set the state if the interface has been registered
    //
    if (m_SymbolicLinkName.Buffer != NULL) {
        Mx::MxSetDeviceInterfaceState(&m_SymbolicLinkName, m_State);
    }
}

