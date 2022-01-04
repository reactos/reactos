/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxUsbInterface.cpp

Abstract:

Author:

Environment:

    Both kernel and user mode

Revision History:

--*/

#include "fxusbpch.hpp"

extern "C" {
#include "FxUsbInterface.tmh"
}

FxUsbInterface::FxUsbInterface(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxUsbDevice* UsbDevice,
    __in PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor
    ) :
    FxNonPagedObject(FX_TYPE_USB_INTERFACE ,sizeof(FxUsbInterface), FxDriverGlobals),
    m_UsbDevice(UsbDevice)
{
    m_UsbDevice->ADDREF(this);

    m_InterfaceNumber = InterfaceDescriptor->bInterfaceNumber;
    m_Protocol = InterfaceDescriptor->bInterfaceProtocol;
    m_Class = InterfaceDescriptor->bInterfaceClass;
    m_SubClass = InterfaceDescriptor->bInterfaceSubClass;

    m_CurAlternateSetting = 0;
    m_NumSettings = 0;
    m_NumberOfConfiguredPipes = 0;
    m_ConfiguredPipes = NULL;
    m_Settings = NULL;

    MarkNoDeleteDDI(ObjectDoNotLock);
}

FxUsbInterface::~FxUsbInterface()
{
    ULONG i;

    m_UsbDevice->RemoveDeletedInterface(this);

    for (i = 0; i < m_NumberOfConfiguredPipes; i++) {
        ASSERT(m_ConfiguredPipes[i] == NULL);
    }

    if (m_ConfiguredPipes != NULL) {
        FxPoolFree(m_ConfiguredPipes);
        m_ConfiguredPipes = NULL;
    }

    m_NumberOfConfiguredPipes = 0;

    if (m_Settings != NULL) {
        FxPoolFree(m_Settings);
        m_Settings = NULL;
    }

    //
    // Release the reference taken in the constructor
    //
    m_UsbDevice->RELEASE(this);
}

VOID
FxUsbInterface::CleanUpAndDelete(
    __in BOOLEAN Failure
    )
/*++

Routine Description:
    Deletes the pipes contained on the interface.

Arguments:
    Failure - if TRUE, the pipes were never exposed to the client, so any attributes
              are cleared before deletion (via DeleteFromFailedCreate)

Assumes:
    FxUsbDevice::InterfaceIterationLock is held by the caller

Return Value:
    None

  --*/
{
    FxUsbPipe** pPipes;
    ULONG numPipes;
    KIRQL irql;
    ULONG i;

    //
    // Capture the values, clear them out of the object and then clean them up
    // outside of the lock.
    //
    m_UsbDevice->Lock(&irql);

    pPipes = m_ConfiguredPipes;
    numPipes = m_NumberOfConfiguredPipes;

    m_ConfiguredPipes = NULL;
    m_NumberOfConfiguredPipes = 0;

    m_UsbDevice->Unlock(irql);

    if (pPipes != NULL) {
        for (i = 0; i < numPipes; i++) {
            if (pPipes[i] != NULL) {
                if (Failure) {
                    //
                    // FxIoTarget::Remove will be called in FxIoTarget::Dispose()
                    //
                    pPipes[i]->DeleteFromFailedCreate();
                }
                else {
                    pPipes[i]->DeleteObject();
                }
            }
            else {
                //
                // No more pointers to delete, break out of the loop
                //
                break;
            }
        }

        FxPoolFree(pPipes);
        pPipes = NULL;
    }
}

VOID
FxUsbInterface::RemoveDeletedPipe(
    __in FxUsbPipe* Pipe
    )
{
    ULONG i;

    if (m_ConfiguredPipes == NULL) {
        return;
    }

    for (i = 0; i < m_NumberOfConfiguredPipes; i++) {
        if (m_ConfiguredPipes[i] == Pipe) {
            m_ConfiguredPipes[i] = NULL;
            return;
        }
    }
}

VOID
FxUsbInterface::SetInfo(
    __in PUSBD_INTERFACE_INFORMATION InterfaceInfo
    )
/*++

Routine Description:
    Captures the alternate from the interface information into this structure
    and then assigns info to all the created pipes.

Arguments:
    InterfaceInfo - info to capture

Return Value:
    None

  --*/
{
    UCHAR i;

    ASSERT(m_InterfaceNumber == InterfaceInfo->InterfaceNumber);

    m_CurAlternateSetting = InterfaceInfo->AlternateSetting;

    for (i = 0; i < m_NumberOfConfiguredPipes ; i++) {
        m_ConfiguredPipes[i]->InitPipe(&InterfaceInfo->Pipes[i],
                                       InterfaceInfo->InterfaceNumber,
                                       this);
    }
}

_Must_inspect_result_
NTSTATUS
FxUsbInterface::CreateSettings(
    VOID
    )
/*++

Routine Description:
    1)  Find the max number of settings for the interface
    2)  Allocate an array for the settings
    3)  Create a setting object for each setting and initialize

Arguments:
    None

Return Value:
    NTSTATUS

  --*/
{
    PUSB_INTERFACE_DESCRIPTOR  pDescriptor;
    ULONG size;
    UCHAR i;

    //
    // No need to validate the size of the interface descriptor since FxUsbDevice::CreateInterfaces
    // has already done so
    //
    pDescriptor = (PUSB_INTERFACE_DESCRIPTOR) FxUsbFindDescriptorType(
        m_UsbDevice->m_ConfigDescriptor,
        m_UsbDevice->m_ConfigDescriptor->wTotalLength,
        m_UsbDevice->m_ConfigDescriptor,
        USB_INTERFACE_DESCRIPTOR_TYPE
        );

    //
    // Calculate the number of settings for this interface
    //
    while (pDescriptor != NULL) {
        if (m_InterfaceNumber == pDescriptor->bInterfaceNumber) {
           m_NumSettings++;
        }
        pDescriptor = (PUSB_INTERFACE_DESCRIPTOR) FxUsbFindDescriptorType(
            m_UsbDevice->m_ConfigDescriptor,
            m_UsbDevice->m_ConfigDescriptor->wTotalLength,
            WDF_PTR_ADD_OFFSET(pDescriptor, pDescriptor->bLength),
            USB_INTERFACE_DESCRIPTOR_TYPE
            );
    }
    size =  sizeof(FxUsbInterfaceSetting) * m_NumSettings;
    m_Settings = (FxUsbInterfaceSetting *) FxPoolAllocate(
        GetDriverGlobals(), NonPagedPool, size);

    if (m_Settings == NULL) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not allocate memory for %d settings for bInterfaceNumber %d "
            "(Protocol %d, Class %d, SubClass %d), %!STATUS!",
            m_NumSettings, m_InterfaceNumber, m_Protocol, m_Class, m_SubClass,
            STATUS_INSUFFICIENT_RESOURCES);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(m_Settings, size);

    //
    // Add all the settings for this interface
    //
    pDescriptor = (PUSB_INTERFACE_DESCRIPTOR) FxUsbFindDescriptorType(
        m_UsbDevice->m_ConfigDescriptor,
        m_UsbDevice->m_ConfigDescriptor->wTotalLength,
        m_UsbDevice->m_ConfigDescriptor,
        USB_INTERFACE_DESCRIPTOR_TYPE
        );

    while (pDescriptor != NULL) {
        if (m_InterfaceNumber == pDescriptor->bInterfaceNumber) {
            if (pDescriptor->bAlternateSetting < m_NumSettings) {
                m_Settings[pDescriptor->bAlternateSetting].InterfaceDescriptor = pDescriptor;
            }
            else {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "Interface Number %d does not have contiguous alternate settings,"
                    "expected %d settings, found alt setting %d, %!STATUS!",
                    pDescriptor->bInterfaceNumber, m_NumSettings,
                    pDescriptor->bAlternateSetting, STATUS_INVALID_DEVICE_REQUEST);

                return STATUS_INVALID_DEVICE_REQUEST;
            }
        }

        pDescriptor = (PUSB_INTERFACE_DESCRIPTOR) FxUsbFindDescriptorType(
            m_UsbDevice->m_ConfigDescriptor,
            m_UsbDevice->m_ConfigDescriptor->wTotalLength,
            WDF_PTR_ADD_OFFSET(pDescriptor, pDescriptor->bLength),
            USB_INTERFACE_DESCRIPTOR_TYPE
            );
    }

    for (i = 0; i < m_NumSettings; i++) {

        if (m_Settings[i].InterfaceDescriptor == NULL) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Interface Number %d does not have contiguous alternate settings,"
                "expected consecutive %d settings, but alt setting %d missing "
                "%!STATUS!", m_InterfaceNumber, m_NumSettings, i,
                STATUS_INVALID_DEVICE_REQUEST);

            return STATUS_INVALID_DEVICE_REQUEST;
        }

        //
        // Only validate the endpoints if the interface reports it has some. We don't
        // want to validate EP descriptors that may be after the interface descriptor
        // that are never used because bNumEndpoints doesn't indicate they are present.
        //
        if (m_Settings[i].InterfaceDescriptor->bNumEndpoints > 0) {
            PVOID pRelativeEnd;
            NTSTATUS status;

            //
            // Validate that each endpoint descriptor is the correct size for this alt setting.
            // We will use the next inteface descriptor as the end, and if this is the last
            // interface descriptor, use the end of the config descriptor as our end.
            //
            pRelativeEnd = FxUsbFindDescriptorType(
                m_UsbDevice->m_ConfigDescriptor,
                m_UsbDevice->m_ConfigDescriptor->wTotalLength,
                m_Settings[i].InterfaceDescriptor,
                USB_INTERFACE_DESCRIPTOR_TYPE
                );

            if (pRelativeEnd == NULL) {
                //
                // This is the last alt setting in the config descriptor, use the end of the
                // config descriptor as our end
                //
                pRelativeEnd = WDF_PTR_ADD_OFFSET(m_UsbDevice->m_ConfigDescriptor,
                    m_UsbDevice->m_ConfigDescriptor->wTotalLength);
            }

            //
            // Limit the number of endpoints validated to bNumEndpoints. In theory
            // there could be EP descriptors after N EP descriptors that are never
            // used, thus we don't want to risk valdiating them and failing them
            // (ie an app compat concern, in a perfect world we would validate them)
            //
            status = FxUsbValidateDescriptorType(
                GetDriverGlobals(),
                m_UsbDevice->m_ConfigDescriptor,
                m_Settings[i].InterfaceDescriptor,
                pRelativeEnd,
                USB_ENDPOINT_DESCRIPTOR_TYPE,
                sizeof(USB_ENDPOINT_DESCRIPTOR),
                FxUsbValidateDescriptorOpAtLeast,
                m_Settings[i].InterfaceDescriptor->bNumEndpoints
                );

            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "Interface Number %d does not have a valid endpoint descriptor,"
                    "%!STATUS!", m_InterfaceNumber, status);

                return status;
            }
        }
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxUsbInterface::SelectSettingByIndex(
    __in PWDF_OBJECT_ATTRIBUTES PipesAttributes,
    __in UCHAR SettingIndex
    )
/*++

Routine Description:
    Setlects a setting by alternate setting index.

Arguments:
    PipesAttributes - optional attributes to apply to each of the created pipes

    SettingIndex - alternate setting index to use

Return Value:
    NTSTATUS

  --*/
{
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    PURB urb;
#elif (FX_CORE_MODE == FX_CORE_USER_MODE)
    UMURB urb;
    PUSB_INTERFACE_DESCRIPTOR interfaceDesc;
#endif
    FxUsbInterfaceSetting entry;
    UCHAR numEP;
    NTSTATUS status;
    USHORT size;

    status = STATUS_SUCCESS;

    //
    // We should have at least 1 setting on the interface
    //
    ASSERT(m_NumSettings != 0);

    //
    // If m_NumberOfConfiguredPipes == 0 then it also tells us that
    // the interface wasn't configured.  So it can keep track of configuredness
    // of the interface. Could there be a case when the selected setting has 0
    // EP's.  Due to the above case we use m_InterfaceConfigured.
    //
    if (IsInterfaceConfigured() && m_CurAlternateSetting == SettingIndex) {
        return STATUS_SUCCESS;
    }

    //
    // Check for an invalid alternate setting
    //
    if (SettingIndex >= m_NumSettings){
        return STATUS_INVALID_PARAMETER;
    }

    RtlCopyMemory(&entry, &m_Settings[SettingIndex], sizeof(entry));

    //
    // Create the configured pipes
    //
    numEP = entry.InterfaceDescriptor->bNumEndpoints;

    size = GET_SELECT_INTERFACE_REQUEST_SIZE(numEP);

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    urb = (PURB) FxPoolAllocate(GetDriverGlobals(), NonPagedPool, size);

    if (urb == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {
        FormatSelectSettingUrb(urb, numEP, SettingIndex);

        status = SelectSetting(PipesAttributes, urb);

        FxPoolFree(urb);
        urb = NULL;
    }
#elif (FX_CORE_MODE == FX_CORE_USER_MODE)
    RtlZeroMemory(&urb, sizeof(UMURB));

    urb.UmUrbSelectInterface.Hdr.InterfaceHandle = m_WinUsbHandle;
    urb.UmUrbSelectInterface.Hdr.Function = UMURB_FUNCTION_SELECT_INTERFACE;
    urb.UmUrbSelectInterface.Hdr.Length = sizeof(_UMURB_SELECT_INTERFACE);

    urb.UmUrbSelectInterface.AlternateSetting = SettingIndex;

    status = m_UsbDevice->SendSyncUmUrb(&urb, 2);

    if (NT_SUCCESS(status)) {
        RtlZeroMemory(&urb, sizeof(UMURB));

        urb.UmUrbInterfaceInformation.Hdr.InterfaceHandle = m_WinUsbHandle;
        urb.UmUrbInterfaceInformation.Hdr.Function = UMURB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE;
        urb.UmUrbInterfaceInformation.Hdr.Length = sizeof(_UMURB_INTERFACE_INFORMATION);

        urb.UmUrbInterfaceInformation.AlternateSetting = SettingIndex;

        status = m_UsbDevice->SendSyncUmUrb(&urb, 2);

        if (NT_SUCCESS(status)) {
            interfaceDesc = &urb.UmUrbInterfaceInformation.UsbInterfaceDescriptor;

            m_Settings[SettingIndex].InterfaceDescriptorAlloc = *interfaceDesc;

            m_CurAlternateSetting = SettingIndex;

            MakeAndConfigurePipes(PipesAttributes, interfaceDesc->bNumEndpoints);
        }
    }
#endif

    return status;
}

_Must_inspect_result_
NTSTATUS
FxUsbInterface::SelectSettingByDescriptor(
    __in PWDF_OBJECT_ATTRIBUTES PipesAttributes,
    __in PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor
    )
/*++

Routine Description:
    Selects an alternate setting by using a USB interface descriptor

Arguments:
    PipesAttributes - optional attributes to apply to each of the created pipes

Return Value:
    NTSTATUS

  --*/
{
    PURB urb;
    NTSTATUS status;
    USHORT size;

    if (IsInterfaceConfigured() &&
        (m_CurAlternateSetting == InterfaceDescriptor->bAlternateSetting)) {
        //
        // Don't do anything
        //
        return STATUS_SUCCESS;
    }

    if (InterfaceDescriptor->bInterfaceNumber != m_InterfaceNumber) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDFUSBINTERFACE %p has interface num %d, select setting by "
            "descriptor specified interface num %d, %!STATUS!",
            GetHandle(), m_InterfaceNumber,
            InterfaceDescriptor->bInterfaceNumber, status
            );

        return status;
    }

    size = GET_SELECT_INTERFACE_REQUEST_SIZE(InterfaceDescriptor->bNumEndpoints);

    urb = (PURB) FxPoolAllocate(GetDriverGlobals(), NonPagedPool, size);

    if (urb == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {
        FormatSelectSettingUrb(
            urb,
            InterfaceDescriptor->bNumEndpoints,
            InterfaceDescriptor->bAlternateSetting
            );

        status = SelectSetting(PipesAttributes, urb);

        FxPoolFree(urb);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxUsbInterface::CheckAndSelectSettingByIndex(
    __in UCHAR SettingIndex
    )
/*++

Routine Description:
    Checks if the give SettingIndex is the current alternate setting
    and if not, selects that setting

Arguments:
    SettingIndex - Alternate setting

Return Value:
    NTSTATUS

  --*/
{
     if (GetConfiguredSettingIndex() != SettingIndex) {
         return SelectSettingByIndex(NULL,
                                     SettingIndex);
     }
     else {
         return STATUS_SUCCESS;
     }
}

_Must_inspect_result_
NTSTATUS
FxUsbInterface::SelectSetting(
    __in PWDF_OBJECT_ATTRIBUTES PipesAttributes,
    __in PURB Urb
    )
/*++

Routine Description:
    Worker function which all top level SelectSetting DDIs call into.  This will
    1)  preallocate all required objects so that we can return failure before
        we make any undoable changes to the device's state
    2)  send the select interface URB to the device
    3)  initialize all created objects

Arguments:
    PipesAttributes - optional attributes to apply to all created pipes

    Urb - Urb to send to the device

Return Value:
    NTSTATUS

  --*/
{
    FxSyncRequest request(GetDriverGlobals(), NULL);
    LIST_ENTRY pendHead;
    FxUsbPipe* pPipe;
    NTSTATUS status;
    UCHAR iPipe, numPipes;
    WDF_REQUEST_SEND_OPTIONS options;
    FxUsbPipe ** ppPipes;
    ULONG size;

    //
    // FxSyncRequest always succeesds for KM but can fail for UM.
    //
    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest");
        return status;
    }

    //
    // Subtract the size of the embedded pipe.
    //
    const ULONG interfaceStructSize = sizeof(Urb->UrbSelectInterface.Interface) -
                                      sizeof(USBD_PIPE_INFORMATION);

    //
    // This check will happen twice for SelectSettingByInterface/Descriptor.
    //
    // We could just do it here, but the above two functions will unnecessarily
    // have to build an URB.
    //
    if (IsInterfaceConfigured() &&
        m_CurAlternateSetting ==
                          Urb->UrbSelectInterface.Interface.AlternateSetting) {
        //
        // don't do anything
        //
        return STATUS_SUCCESS;
    }

    InitializeListHead(&pendHead);
    numPipes = 0;
    ppPipes = NULL;

    if (Urb->UrbSelectInterface.Hdr.Length < interfaceStructSize) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Urb header length 0x%x is less than expected 0x%x"
            "%!STATUS!", Urb->UrbSelectInterface.Hdr.Length, interfaceStructSize,status
            );
        return status;
    }

    status = request.m_TrueRequest->ValidateTarget(m_UsbDevice);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    //
    // Urb->UrbSelectInterface.Interface.NumberOfPipes is set when the URB
    // completes.  So, we must compute the number of pipes being requested based
    // on the size of the structure and its Length (as set by the caller).
    // To calculate the number of pipes we need to account for the
    // embedded pipe in the structure.
    //
    numPipes = (UCHAR) ((Urb->UrbSelectInterface.Interface.Length -
                         interfaceStructSize) /
                       sizeof(USBD_PIPE_INFORMATION)
                       );

    if (numPipes > 0) {
        size =  numPipes * sizeof(FxUsbPipe *);
    }
    else {
        //
        // It is valid to have an interface with zero pipes in it.  In that
        // case, we just allocate one entry so that we have a valid array
        // and keep the remaining code simple.
        //
        size = sizeof(FxUsbPipe*);
    }

    //
    // If the interface is already configured don't do anything with the old
    // settings till we allocate new.
    //
    ppPipes = (FxUsbPipe **) FxPoolAllocate(GetDriverGlobals(), NonPagedPool, size);

    if (ppPipes == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Unable to allocate memory %!STATUS!"
            , status);
        goto Done;
    }

    RtlZeroMemory(ppPipes, size);

    for (iPipe = 0; iPipe < numPipes; iPipe++) {
        ppPipes[iPipe] = new (GetDriverGlobals(), PipesAttributes)
            FxUsbPipe(GetDriverGlobals(),m_UsbDevice);

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


    WDF_REQUEST_SEND_OPTIONS_INIT(&options,
                                  WDF_REQUEST_SEND_OPTION_IGNORE_TARGET_STATE);

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&options, WDF_REL_TIMEOUT_IN_SEC(2));

    FxFormatUsbRequest(request.m_TrueRequest, Urb, FxUrbTypeLegacy, NULL);
    status = m_UsbDevice->SubmitSync(request.m_TrueRequest, &options, NULL);

    //
    // If select interface URB fails we are at the point of no return and we
    // will end up with no configured pipes.
    //
    if (NT_SUCCESS(status)) {
        SetNumConfiguredPipes(numPipes);
        SetConfiguredPipes(ppPipes);
        SetInfo(&Urb->UrbSelectInterface.Interface);
    }

Done:
    if (!NT_SUCCESS(status)) {
        if (ppPipes != NULL) {
            ASSERT(ppPipes != m_ConfiguredPipes);

            for (iPipe = 0; iPipe < numPipes; iPipe++) {
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

VOID
FxUsbInterface::FormatSelectSettingUrb(
    __in_bcount(GET_SELECT_INTERFACE_REQUEST_SIZE(NumEndpoints)) PURB Urb,
    __in USHORT NumEndpoints,
    __in UCHAR SettingNumber
    )
/*++

Routine Description:
    Format a URB for selecting an interface's new setting.  Note this will setup
    the URB header and all pipes' information in the URB's array of pipe infos.

    This function exists as a method of FxUsbDevice instead of FxUsbInterface
    because in the case of a select config where we manually select setting 0
    on interfaces 2...N we don't have an FxUsbInterface pointer

Arguments:
    Urb - the URB to format.  It is assumed the caller allocated a large enough
          URB to contain NumEndpoints

    NumEndpoints - number of endpoints in the new setting

    InterfaceNum - interface number for the interface

    SettingNumber - setting number on the interface

  --*/
{
    ULONG defaultMaxTransferSize;
    USHORT size;
    UCHAR i;

    size = GET_SELECT_INTERFACE_REQUEST_SIZE(NumEndpoints);

    RtlZeroMemory(Urb, size);

    //
    // Setup the URB, format the request, and send it
    //
    UsbBuildSelectInterfaceRequest(Urb,
                                   size,
                                   m_UsbDevice->m_ConfigHandle,
                                   m_InterfaceNumber,
                                   SettingNumber);

    defaultMaxTransferSize = m_UsbDevice->GetDefaultMaxTransferSize();

    Urb->UrbSelectInterface.Interface.Length =
        GET_USBD_INTERFACE_SIZE(NumEndpoints);

    Urb->UrbSelectInterface.Interface.NumberOfPipes = NumEndpoints;

    for (i = 0; i < NumEndpoints; i++) {

        //
        // Make sure that the Interface Length conveys the exact number of EP's
        //
        ASSERT(
            &Urb->UrbSelectInterface.Interface.Pipes[i] <
            WDF_PTR_ADD_OFFSET(&Urb->UrbSelectInterface.Interface,
                               Urb->UrbSelectInterface.Interface.Length)
            );

        Urb->UrbSelectInterface.Interface.Pipes[i].PipeFlags = 0x0;
        Urb->UrbSelectInterface.Interface.Pipes[i].MaximumTransferSize =
            defaultMaxTransferSize;
    }
}

VOID
FxUsbInterface::GetEndpointInformation(
    __in UCHAR SettingIndex,
    __in UCHAR EndpointIndex,
    __in PWDF_USB_PIPE_INFORMATION PipeInfo
    )
/*++

Routine Description:
    The layout of the config descriptor is such that each interface+setting pair
    is followed by the endpoints for that interface+setting pair.  Keep track of
    the index.

Arguments:
    SettingIndex - alternate setting to get info for

    EndpointIndex  - index into the number endpoints for this interface+setting

    PipeInfo - Info to return

Return Value:
    None

  --*/
{
    PUCHAR pEnd, pCur;
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDesc;
    PUSB_ENDPOINT_DESCRIPTOR pEndpointDesc;
    UCHAR curEndpointIndex;
    BOOLEAN endPointFound;

    pInterfaceDesc = NULL;
    curEndpointIndex = 0;
    endPointFound = FALSE;

    //
    // Extract the interface descriptor for the alternate setting for the interface
    //
    pInterfaceDesc = GetSettingDescriptor(SettingIndex);

    if (pInterfaceDesc == NULL) {
        return;
    }

    pEnd = (PUCHAR) WDF_PTR_ADD_OFFSET(
        m_UsbDevice->m_ConfigDescriptor,
        m_UsbDevice->m_ConfigDescriptor->wTotalLength
        );

    //
    // Start from the descriptor after current one
    //
    pCur = (PUCHAR) WDF_PTR_ADD_OFFSET(pInterfaceDesc, pInterfaceDesc->bLength);

    //
    // Iterate through the list of EP descriptors following the interface descriptor
    // we just found and get the endpoint descriptor which matches the endpoint
    // index or we hit another interface descriptor
    //
    // We have already validated that the descriptor headers are well formed and within
    // the config descriptor bounds
    //
    while (pCur < pEnd) {
        PUSB_COMMON_DESCRIPTOR pCommonDesc = (PUSB_COMMON_DESCRIPTOR) pCur;

        //
        // If we hit the next interface no way we can find the EndPoint
        //
        if (pCommonDesc->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE) {
            break;
        }

        if (pCommonDesc->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE) {
            //
            // Size of pEndpointDesc has been validated by CreateSettings() and
            // is within the config descriptor
            //
            pEndpointDesc = (PUSB_ENDPOINT_DESCRIPTOR) pCommonDesc;

            if (EndpointIndex == curEndpointIndex) {
                CopyEndpointFieldsFromDescriptor(PipeInfo,
                                                 pEndpointDesc,
                                                 SettingIndex);
                break;
            }

            curEndpointIndex++;
        }

        //
        // Advance past this descriptor
        //
        pCur +=  pCommonDesc->bLength;
    }
}

ULONG
FxUsbInterface::DetermineDefaultMaxTransferSize(
    VOID
    )
/*++

Routine Description:
    Returns the maximum transfer size of an endpoint

Arguments:
    None

Return Value:
    max transfer size

  --*/
{
    if (m_UsbDevice->m_Traits & WDF_USB_DEVICE_TRAIT_AT_HIGH_SPEED) {
        return FxUsbPipeHighSpeedMaxTransferSize;
    }
    else {
        return FxUsbPipeLowSpeedMaxTransferSize;
    }
}

VOID
FxUsbInterface::CopyEndpointFieldsFromDescriptor(
    __in PWDF_USB_PIPE_INFORMATION PipeInfo,
    __in PUSB_ENDPOINT_DESCRIPTOR EndpointDesc,
    __in UCHAR SettingIndex
    )
/*++

Routine Description:
    Copy informatoin out of the usb endpoint descriptor into this object

Arguments:
    PipeInfo - information  to return

    EndpointDesc - descriptor to copy from

    SettingIndex - alternate setting this information is for

Return Value:
    None

  --*/
{
    PipeInfo->MaximumPacketSize  = EndpointDesc->wMaxPacketSize;
    PipeInfo->EndpointAddress  = EndpointDesc->bEndpointAddress;
    PipeInfo->Interval  = EndpointDesc->bInterval;

    //
    // Extract the lower 2 bits which contain the EP type
    //
    PipeInfo->PipeType  = FxUsbPipe::_UsbdPipeTypeToWdf(
        (USBD_PIPE_TYPE) (EndpointDesc->bmAttributes & 0x03)
        );

    //
    // Filling in a default value since the EndpointDescriptor doesn't contain it
    //
    if (PipeInfo->PipeType == WdfUsbPipeTypeControl) {
        PipeInfo->MaximumTransferSize = FxUsbPipeControlMaxTransferSize;
    }
    else {
        PipeInfo->MaximumTransferSize  = DetermineDefaultMaxTransferSize();
    }

    PipeInfo->SettingIndex  = SettingIndex;
}

WDFUSBPIPE
FxUsbInterface::GetConfiguredPipe(
    __in UCHAR PipeIndex,
    __out_opt PWDF_USB_PIPE_INFORMATION PipeInfo
    )
/*++

Routine Description:
    Return the WDFUSBPIPE for the given index

Arguments:
    PipeIndex - index into the number of configured pipes for the interface

    PipeInfo - optional information to return about the returned pipe

Return Value:
    valid WDFUSBPIPE handle or NULL on error

  --*/
{
    if (PipeIndex >= m_NumberOfConfiguredPipes) {
        return NULL;
    }
    else {
        if (PipeInfo != NULL) {
            m_ConfiguredPipes[PipeIndex]->GetInformation(PipeInfo);
        }

        return m_ConfiguredPipes[PipeIndex]->GetHandle();
    }
}

VOID
FxUsbInterface::GetDescriptor(
    __in PUSB_INTERFACE_DESCRIPTOR  UsbInterfaceDescriptor,
    __in UCHAR SettingIndex
    )
/*++

Routine Description:
    Copies the descriptor back to the caller

Arguments:
    UsbInterfaceDescriptor - descriptor pointer to fill in

    SettingIndex - alternate setting that the caller is interested in

Return Value:
    None

  --*/
{
    if (SettingIndex >= m_NumSettings) {
        RtlZeroMemory(UsbInterfaceDescriptor,
                      sizeof(*UsbInterfaceDescriptor));
    }
    else {
        RtlCopyMemory(UsbInterfaceDescriptor,
                      m_Settings[SettingIndex].InterfaceDescriptor,
                      sizeof(*UsbInterfaceDescriptor));
    }
}

UCHAR
FxUsbInterface::GetConfiguredSettingIndex(
    VOID
    )
/*++

Routine Description:
    Returns the currently configured setting index for the interface

Arguments:
    None

Return Value:
    Currently configured Index

  --*/

{
    if (IsInterfaceConfigured()) {
        return m_CurAlternateSetting;
    }
    else {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDFUSBINTERFACE %p not configured, cannot retrieve configured "
            "setting index", GetHandle());

        FxVerifierDbgBreakPoint(GetDriverGlobals());

        return 0;
    }
}

UCHAR
FxUsbInterface::GetNumEndpoints(
    __in UCHAR SettingIndex
    )
/*++

Routine Description:
    Returns the number of endpoints on a given alternate interface

Arguments:
    SettingIndex - index of the alternate setting

Return Value:
    Number of endpoints or 0 on error

  --*/
{
    if (SettingIndex >= m_NumSettings) {
        return 0;
    }
    else {
        return m_Settings[SettingIndex].InterfaceDescriptor->bNumEndpoints;
    }
}

PUSB_INTERFACE_DESCRIPTOR
FxUsbInterface::GetSettingDescriptor(
    __in UCHAR Setting
    )
/*++

Routine Description:
    Returns the device's interface descriptor for the given alternate setting

Arguments:
    Setting - AlternateSetting desired

Return Value:
    USB interface descriptor or NULL on failure

  --*/
{
    UCHAR i;

    for (i = 0; i < m_NumSettings; i++) {
        if (m_Settings[i].InterfaceDescriptor->bAlternateSetting == Setting) {
            return m_Settings[i].InterfaceDescriptor;
        }
    }

    return NULL;
}

