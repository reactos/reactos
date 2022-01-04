/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceInitUm.cpp

Abstract:
    Internals for WDFDEVICE_INIT

Author:




Environment:

    User mode only

Revision History:

--*/

#include "coreprivshared.hpp"

extern "C" {
#include "FxDeviceInitUm.tmh"
}

VOID
WDFDEVICE_INIT::SetPdo(
    __in FxDevice* Parent
    )
{
    UNREFERENCED_PARAMETER(Parent);

    UfxVerifierTrapNotImpl();
}

VOID
WDFDEVICE_INIT::AssignIoType(
    _In_ PWDF_IO_TYPE_CONFIG IoTypeConfig
    )
{
    NTSTATUS status;

    if (IoTypeConfig->ReadWriteIoType == WdfDeviceIoUndefined ||
        IoTypeConfig->ReadWriteIoType > WdfDeviceIoBufferedOrDirect) {
        status= STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Out of range ReadWriteIoType %d, %!status!",
            IoTypeConfig->ReadWriteIoType, status);
        FxVerifierDbgBreakPoint(DriverGlobals);
        return;
    }

    if (IoTypeConfig->DeviceControlIoType == WdfDeviceIoUndefined ||
        IoTypeConfig->DeviceControlIoType > WdfDeviceIoBufferedOrDirect) {
        status= STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Out of range DeviceControlIoType %d, %!status!",
            IoTypeConfig->DeviceControlIoType, status);
        FxVerifierDbgBreakPoint(DriverGlobals);
        return;
    }

    if (IoTypeConfig->ReadWriteIoType == WdfDeviceIoNeither ||
        IoTypeConfig->DeviceControlIoType == WdfDeviceIoNeither) {
        status= STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WdfDeviceIoNeither not supported for ReadWriteIoType or "
            "DeviceControlIoType, %!status!", status);
        FxVerifierDbgBreakPoint(DriverGlobals);
        return;
    }

    ReadWriteIoType = IoTypeConfig->ReadWriteIoType;
    DeviceControlIoType = IoTypeConfig->DeviceControlIoType;
    DirectTransferThreshold = IoTypeConfig->DirectTransferThreshold;

    return;
}

