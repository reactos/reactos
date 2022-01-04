/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxUsbInterfaceUm.cpp

Abstract:

Author:

Environment:

    user mode only

Revision History:

--*/

#include "fxusbpch.hpp"

extern "C" {
#include "FxUsbInterfaceUm.tmh"
}

NTSTATUS
FxUsbInterface::SetWinUsbHandle(
    _In_ UCHAR FrameworkInterfaceIndex
)
{
    NTSTATUS status = STATUS_SUCCESS;
    UMURB urb;

    if (m_InterfaceNumber != FrameworkInterfaceIndex) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
                            "Composite device detected: Converting absolute interface "
                            "index %d to relative interface index %d", m_InterfaceNumber,
                            FrameworkInterfaceIndex);
    }

    if (FrameworkInterfaceIndex == 0) {
        m_WinUsbHandle = m_UsbDevice->m_WinUsbHandle;
    }
    else {
        RtlZeroMemory(&urb, sizeof(UMURB));

        urb.UmUrbGetAssociatedInterface.Hdr.InterfaceHandle = m_UsbDevice->m_WinUsbHandle;
        urb.UmUrbGetAssociatedInterface.Hdr.Function = UMURB_FUNCTION_GET_ASSOCIATED_INTERFACE;
        urb.UmUrbGetAssociatedInterface.Hdr.Length = sizeof(_UMURB_GET_ASSOCIATED_INTERFACE);

        //
        // If this is using the WinUSB dispatcher, this will ultimately call
        // WinUsb_GetAssociatedInterface which expects a 0-based index but starts
        // counting from index 1. To get the handle for interface n, we pass n-1
        // and WinUSB will return the handle for (n-1)+1.
        //
        // The NativeUSB dispatcher ultimately calls WdfUsbTargetDeviceGetInterface.
        // Unlike WinUSB.sys, this starts counting from index zero. The NativeUSB
        // dispatcher expects this framework quirk and adjusts accordingly. See
        // WudfNativeUsbDispatcher.cpp for more information.
        //
        // The actual interface number may differ from the interface index in
        // composite devices. In all cases, the interface index (starting from zero)
        // must be used.
        //
        urb.UmUrbGetAssociatedInterface.InterfaceIndex = FrameworkInterfaceIndex - 1;

        status = m_UsbDevice->SendSyncUmUrb(&urb, 5);

        if (NT_SUCCESS(status)) {
            m_WinUsbHandle = urb.UmUrbGetAssociatedInterface.InterfaceHandle;
        }
        else {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                "Failed to retrieve WinUsb interface handle");
            m_WinUsbHandle = NULL;
        }
    }

    return status;
}

NTSTATUS
FxUsbInterface::MakeAndConfigurePipes(
    __in PWDF_OBJECT_ATTRIBUTES PipesAttributes,
    __in UCHAR NumPipes
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UCHAR iPipe;
    FxUsbPipe* pPipe;
    FxUsbPipe** ppPipes;
    //
    // Zero pipes are a valid configuration, this simplifies the code below.
    //
    ULONG size = (NumPipes == 0 ? 1 : NumPipes) * sizeof(FxUsbPipe*);
    UMURB urb;

    ppPipes = (FxUsbPipe**)FxPoolAllocate(GetDriverGlobals(), NonPagedPool, size);
    if (ppPipes == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Unable to allocate memory %!STATUS!", status);
        goto Done;
    }

    RtlZeroMemory(ppPipes, size);

    for (iPipe = 0; iPipe < NumPipes; iPipe++) {
        ppPipes[iPipe] = new (GetDriverGlobals(), PipesAttributes)
            FxUsbPipe(GetDriverGlobals(), m_UsbDevice);

        if (ppPipes[iPipe] == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Unable to allocate memory for the pipes %!STATUS!", status);
            goto Done;
        }

        pPipe = ppPipes[iPipe];

        status = pPipe->Init(m_UsbDevice->m_Device);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Init pipe failed  %!STATUS!", status);
            goto Done;
        }

        status = pPipe->Commit(PipesAttributes, NULL, this);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Commit pipe failed  %!STATUS!", status);
            goto Done;
        }
    }

    if (IsInterfaceConfigured()) {
        //
        // Delete the old pipes
        //
        m_UsbDevice->CleanupInterfacePipesAndDelete(this);
    }

    SetNumConfiguredPipes(NumPipes);
    SetConfiguredPipes(ppPipes);

    for (iPipe = 0; iPipe < m_NumberOfConfiguredPipes ; iPipe++) {
        RtlZeroMemory(&urb, sizeof(UMURB));

        urb.UmUrbQueryPipe.Hdr.InterfaceHandle = m_WinUsbHandle;
        urb.UmUrbQueryPipe.Hdr.Function = UMURB_FUNCTION_QUERY_PIPE;
        urb.UmUrbQueryPipe.Hdr.Length = sizeof(_UMURB_QUERY_PIPE);

        urb.UmUrbQueryPipe.AlternateSetting = m_CurAlternateSetting;
        urb.UmUrbQueryPipe.PipeID = iPipe;

        status = m_UsbDevice->SendSyncUmUrb(&urb, 2);

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Send UMURB_FUNCTION_QUERY_PIPE failed  %!STATUS!", status);
            goto Done;
        }

        m_ConfiguredPipes[iPipe]->InitPipe(&urb.UmUrbQueryPipe.PipeInformation,
                                           m_InterfaceNumber,
                                           this);
    }

Done:
    if (!NT_SUCCESS(status)) {
        if (ppPipes != NULL) {
            ASSERT(ppPipes != m_ConfiguredPipes);

            for (iPipe = 0; iPipe < NumPipes; iPipe++) {
                if (ppPipes[iPipe] != NULL) {
                    ppPipes[iPipe]->DeleteFromFailedCreate();
                }
            }

            FxPoolFree(ppPipes);
            ppPipes = NULL;
        }
    }

    return status;
}

NTSTATUS
FxUsbInterface::UpdatePipeAttributes(
    __in PWDF_OBJECT_ATTRIBUTES PipesAttributes
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UCHAR iPipe;
    FxUsbPipe** ppPipes;
    UCHAR numberOfPipes;

    numberOfPipes = m_NumberOfConfiguredPipes;
    ppPipes = m_ConfiguredPipes;

    for (iPipe = 0; iPipe < numberOfPipes; iPipe++) {
        status = FxObjectAllocateContext(ppPipes[iPipe],
                                         PipesAttributes,
                                         TRUE,
                                         NULL);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "UpdatePipeAttributes failed  %!STATUS!", status);
            break;
        }
    }

    //
    // Pipe attributes are updated as part of select
    // config and it is ok for the client driver to configure
    // twice with the same attributes. In a similar scenario,
    // KMDF will return STATUS_SUCCESS, so we should do the
    // same for UMDF for consistency
    //
    if (status == STATUS_OBJECT_NAME_EXISTS) {
        status = STATUS_SUCCESS;
    }

    return status;
}

