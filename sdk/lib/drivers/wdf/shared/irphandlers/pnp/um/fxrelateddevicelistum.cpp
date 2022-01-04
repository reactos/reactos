/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    FxRelatedDeviceListUm.cpp

Abstract:
    This object derives from the transactioned list and provides a unique
    object check during the addition of an item.

Author:




Environment:

    User mode only

Revision History:

--*/

#include <fxmin.hpp>

_Must_inspect_result_
NTSTATUS
FxRelatedDeviceList::Add(
    __in PFX_DRIVER_GLOBALS /*FxDriverGlobals*/,
    __inout FxRelatedDevice* /*Entry*/
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

VOID
FxRelatedDeviceList::Remove(
    __in PFX_DRIVER_GLOBALS /*FxDriverGlobals*/,
    __in MdDeviceObject /*Device*/
    )
{
    UfxVerifierTrapNotImpl();
    return;
}

_Must_inspect_result_
FxRelatedDevice*
FxRelatedDeviceList::GetNextEntry(
    __in_opt FxRelatedDevice* /*Entry*/
    )
{
    UfxVerifierTrapNotImpl();
    return NULL;
}

_Must_inspect_result_
NTSTATUS
FxRelatedDeviceList::ProcessAdd(
    __in FxTransactionedEntry * /*NewEntry*/
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
FxRelatedDeviceList::Compare(
    __in FxTransactionedEntry* /*Entry*/,
    __in PVOID /*Data*/
    )
{
    UfxVerifierTrapNotImpl();
    return FALSE;
}

VOID
FxRelatedDeviceList::EntryRemoved(
    __in FxTransactionedEntry* /*Entry*/
    )
{
    UfxVerifierTrapNotImpl();
    return;
}
