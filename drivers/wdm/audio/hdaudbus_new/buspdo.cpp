#include "driver.h"
#include "adsp.h"

NTSTATUS
Bus_CreatePdo(
    _In_ WDFDEVICE       Device,
    _In_ PWDFDEVICE_INIT DeviceInit,
    _In_ PPDO_IDENTIFICATION_DESCRIPTION           Desc
);

NTSTATUS
Bus_EvtChildListIdentificationDescriptionDuplicate(
    WDFCHILDLIST DeviceList,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER SourceIdentificationDescription,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER DestinationIdentificationDescription
)
/*++
Routine Description:
    It is called when the framework needs to make a copy of a description.
    This happens when a request is made to create a new child device by
    calling WdfChildListAddOrUpdateChildDescriptionAsPresent.
    If this function is left unspecified, RtlCopyMemory will be used to copy the
    source description to destination. Memory for the description is managed by the
    framework.
    NOTE:   Callback is invoked with an internal lock held.  So do not call out
    to any WDF function which will require this lock
    (basically any other WDFCHILDLIST api)
Arguments:
    DeviceList - Handle to the default WDFCHILDLIST created by the framework.
    SourceIdentificationDescription - Description of the child being created -memory in
                            the calling thread stack.
    DestinationIdentificationDescription - Created by the framework in nonpaged pool.
Return Value:
    NT Status code.
--*/
{
    PPDO_IDENTIFICATION_DESCRIPTION src, dst;

    UNREFERENCED_PARAMETER(DeviceList);

    src = CONTAINING_RECORD(SourceIdentificationDescription,
        PDO_IDENTIFICATION_DESCRIPTION,
        Header);
    dst = CONTAINING_RECORD(DestinationIdentificationDescription,
        PDO_IDENTIFICATION_DESCRIPTION,
        Header);

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "%s\n", __func__);

    dst->FdoContext = src->FdoContext;
    RtlCopyMemory(&dst->CodecIds, &src->CodecIds, sizeof(dst->CodecIds));

    return STATUS_SUCCESS;
}

BOOLEAN
Bus_EvtChildListIdentificationDescriptionCompare(
    WDFCHILDLIST DeviceList,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER FirstIdentificationDescription,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER SecondIdentificationDescription
)
/*++
Routine Description:
    It is called when the framework needs to compare one description with another.
    Typically this happens whenever a request is made to add a new child device.
    If this function is left unspecified, RtlCompareMemory will be used to compare the
    descriptions.
    NOTE:   Callback is invoked with an internal lock held.  So do not call out
    to any WDF function which will require this lock
    (basically any other WDFCHILDLIST api)
Arguments:
    DeviceList - Handle to the default WDFCHILDLIST created by the framework.
Return Value:
   TRUE or FALSE.
--*/
{
    PPDO_IDENTIFICATION_DESCRIPTION lhs, rhs;

    UNREFERENCED_PARAMETER(DeviceList);

    lhs = CONTAINING_RECORD(FirstIdentificationDescription,
        PDO_IDENTIFICATION_DESCRIPTION,
        Header);
    rhs = CONTAINING_RECORD(SecondIdentificationDescription,
        PDO_IDENTIFICATION_DESCRIPTION,
        Header);

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "%s\n", __func__);

    return (lhs->FdoContext == rhs->FdoContext) &&
        (RtlCompareMemory(&lhs->CodecIds, &rhs->CodecIds, sizeof(lhs->CodecIds)) == sizeof(lhs->CodecIds));
}

VOID
Bus_EvtChildListIdentificationDescriptionCleanup(
    _In_ WDFCHILDLIST DeviceList,
    _Inout_ PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
)
/*++
Routine Description:
    It is called to free up any memory resources allocated as part of the description.
    This happens when a child device is unplugged or ejected from the bus.
    Memory for the description itself will be freed by the framework.
Arguments:
    DeviceList - Handle to the default WDFCHILDLIST created by the framework.
    IdentificationDescription - Description of the child being deleted
Return Value:
--*/
{
    UNREFERENCED_PARAMETER(DeviceList);
    UNREFERENCED_PARAMETER(IdentificationDescription);
}

NTSTATUS
Bus_EvtDeviceListCreatePdo(
    WDFCHILDLIST DeviceList,
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    PWDFDEVICE_INIT ChildInit
)
/*++
Routine Description:
    Called by the framework in response to Query-Device relation when
    a new PDO for a child device needs to be created.
Arguments:
    DeviceList - Handle to the default WDFCHILDLIST created by the framework as part
                        of FDO.
    IdentificationDescription - Decription of the new child device.
    ChildInit - It's a opaque structure used in collecting device settings
                    and passed in as a parameter to CreateDevice.
Return Value:
    NT Status code.
--*/
{
    PPDO_IDENTIFICATION_DESCRIPTION pDesc;

    PAGED_CODE();

    pDesc = CONTAINING_RECORD(IdentificationDescription,
        PDO_IDENTIFICATION_DESCRIPTION,
        Header);

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "%s\n", __func__);

    return Bus_CreatePdo(WdfChildListGetDevice(DeviceList),
        ChildInit,
        pDesc);
}

NTSTATUS
Bus_CreatePdo(
    _In_ WDFDEVICE       Device,
    _In_ PWDFDEVICE_INIT DeviceInit,
    _In_ PPDO_IDENTIFICATION_DESCRIPTION           Desc
)
{
    UNREFERENCED_PARAMETER(Device);

    NTSTATUS                    status;
    PPDO_DEVICE_DATA            pdoData = NULL;
    WDFDEVICE                   hChild = NULL;
    WDF_QUERY_INTERFACE_CONFIG  qiConfig;
    WDF_OBJECT_ATTRIBUTES       pdoAttributes;
    WDF_DEVICE_PNP_CAPABILITIES pnpCaps;
    WDF_DEVICE_POWER_CAPABILITIES powerCaps;
    DECLARE_CONST_UNICODE_STRING(deviceLocation, L"High Definition Audio Bus");
    DECLARE_UNICODE_STRING_SIZE(buffer, MAX_INSTANCE_ID_LEN);
    DECLARE_UNICODE_STRING_SIZE(deviceId, MAX_INSTANCE_ID_LEN);
    DECLARE_UNICODE_STRING_SIZE(compatId, MAX_INSTANCE_ID_LEN);

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "%s\n", __func__);

    if (Desc->CodecIds.IsDSP) {
        //
        // Set DeviceType
        //
        WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_BUS_EXTENDER);

        //
        // Provide DeviceID, HardwareIDs, CompatibleIDs and InstanceId
        //
        status = RtlUnicodeStringPrintf(&deviceId, L"CSAUDIO\\ADSP&CTLR_VEN_%04X&CTLR_DEV_%04X",
             Desc->CodecIds.CtlrVenId, Desc->CodecIds.CtlrDevId);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        status = WdfPdoInitAssignInstanceID(DeviceInit, &deviceId);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        status = WdfPdoInitAssignDeviceID(DeviceInit, &deviceId);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // NOTE: same string  is used to initialize hardware id too
        //
        status = WdfPdoInitAddHardwareID(DeviceInit, &deviceId);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        status = RtlUnicodeStringPrintf(&compatId, L"CSAUDIO\\ADSP&CTLR_VEN_%04X&CTLR_DEV_%04X",
            Desc->CodecIds.CtlrVenId, Desc->CodecIds.CtlrDevId);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // NOTE: same string  is used to initialize compat id too
        //
        status = WdfPdoInitAddCompatibleID(DeviceInit, &compatId);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }
    else {
        //
        // Set DeviceType
        //
        WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_SOUND);

        PWCHAR prefix = L"HDAUDIO";
        PWCHAR funcPrefix = L"";
        if (Desc->CodecIds.IsGraphicsCodec) {
            funcPrefix = L"SGPC_";
        }

        //
        // Provide DeviceID, HardwareIDs, CompatibleIDs and InstanceId
        //
        status = RtlUnicodeStringPrintf(&deviceId, L"%s\\%sFUNC_%02X&VEN_%04X&DEV_%04X&SUBSYS_%08X&REV_%04X",
            prefix, funcPrefix, Desc->CodecIds.FuncId, Desc->CodecIds.VenId, Desc->CodecIds.DevId, Desc->CodecIds.SubsysId, Desc->CodecIds.RevId);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        status = WdfPdoInitAssignDeviceID(DeviceInit, &deviceId);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // NOTE: same string  is used to initialize hardware id too
        //
        status = WdfPdoInitAddHardwareID(DeviceInit, &deviceId);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        //Add second hardware ID without Rev
        status = RtlUnicodeStringPrintf(&deviceId, L"%s\\%sFUNC_%02X&VEN_%04X&DEV_%04X&SUBSYS_%08X",
            prefix, funcPrefix, Desc->CodecIds.FuncId, Desc->CodecIds.VenId, Desc->CodecIds.DevId, Desc->CodecIds.SubsysId);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        status = WdfPdoInitAddHardwareID(DeviceInit, &deviceId);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        //Add Compatible Ids
        {
            status = RtlUnicodeStringPrintf(&compatId, L"%s\\%sFUNC_%02X&CTLR_VEN_%02X&CTLR_DEV_%02X&VEN_%04X&DEV_%04X&REV_%04X",
                prefix, funcPrefix, Desc->CodecIds.FuncId, Desc->CodecIds.CtlrVenId, Desc->CodecIds.CtlrDevId, Desc->CodecIds.VenId, Desc->CodecIds.DevId, Desc->CodecIds.RevId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = WdfPdoInitAddCompatibleID(DeviceInit, &compatId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = RtlUnicodeStringPrintf(&compatId, L"%s\\%sFUNC_%02X&CTLR_VEN_%02X&VEN_%04X&DEV_%04X&REV_%04X",
                prefix, funcPrefix, Desc->CodecIds.FuncId, Desc->CodecIds.CtlrVenId, Desc->CodecIds.VenId, Desc->CodecIds.DevId, Desc->CodecIds.RevId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = WdfPdoInitAddCompatibleID(DeviceInit, &compatId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = RtlUnicodeStringPrintf(&compatId, L"%s\\%sFUNC_%02X&VEN_%04X&DEV_%04X&REV_%04X",
                prefix, funcPrefix, Desc->CodecIds.FuncId, Desc->CodecIds.VenId, Desc->CodecIds.DevId, Desc->CodecIds.RevId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = WdfPdoInitAddCompatibleID(DeviceInit, &compatId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = RtlUnicodeStringPrintf(&compatId, L"%s\\%sFUNC_%02X&CTLR_VEN_%02X&CTLR_DEV_%02X&VEN_%04X&DEV_%04X",
                prefix, funcPrefix, Desc->CodecIds.FuncId, Desc->CodecIds.CtlrVenId, Desc->CodecIds.CtlrDevId, Desc->CodecIds.VenId, Desc->CodecIds.DevId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = WdfPdoInitAddCompatibleID(DeviceInit, &compatId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = RtlUnicodeStringPrintf(&compatId, L"%s\\%sFUNC_%02X&CTLR_VEN_%02X&VEN_%04X&DEV_%04X",
                prefix, funcPrefix, Desc->CodecIds.FuncId, Desc->CodecIds.CtlrVenId, Desc->CodecIds.VenId, Desc->CodecIds.DevId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = WdfPdoInitAddCompatibleID(DeviceInit, &compatId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = RtlUnicodeStringPrintf(&compatId, L"%s\\%sFUNC_%02X&VEN_%04X&DEV_%04X",
                prefix, funcPrefix, Desc->CodecIds.FuncId, Desc->CodecIds.VenId, Desc->CodecIds.DevId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = WdfPdoInitAddCompatibleID(DeviceInit, &compatId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = RtlUnicodeStringPrintf(&compatId, L"%s\\%sFUNC_%02X&CTLR_VEN_%02X&CTLR_DEV_%02X&VEN_%04X",
                prefix, funcPrefix, Desc->CodecIds.FuncId, Desc->CodecIds.CtlrVenId, Desc->CodecIds.CtlrDevId, Desc->CodecIds.VenId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = WdfPdoInitAddCompatibleID(DeviceInit, &compatId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = RtlUnicodeStringPrintf(&compatId, L"%s\\%sFUNC_%02X&CTLR_VEN_%02X&VEN_%04X",
                prefix, funcPrefix, Desc->CodecIds.FuncId, Desc->CodecIds.CtlrVenId, Desc->CodecIds.VenId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = WdfPdoInitAddCompatibleID(DeviceInit, &compatId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = RtlUnicodeStringPrintf(&compatId, L"%s\\%sFUNC_%02X&VEN_%04X",
                prefix, funcPrefix, Desc->CodecIds.FuncId, Desc->CodecIds.VenId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = WdfPdoInitAddCompatibleID(DeviceInit, &compatId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = RtlUnicodeStringPrintf(&compatId, L"%s\\%sFUNC_%02X&CTLR_VEN_%02X&CTLR_DEV_%02X",
                prefix, funcPrefix, Desc->CodecIds.FuncId, Desc->CodecIds.CtlrVenId, Desc->CodecIds.CtlrDevId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = WdfPdoInitAddCompatibleID(DeviceInit, &compatId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = RtlUnicodeStringPrintf(&compatId, L"%s\\%sFUNC_%02X&CTLR_VEN_%02X",
                prefix, funcPrefix, Desc->CodecIds.FuncId, Desc->CodecIds.CtlrVenId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = WdfPdoInitAddCompatibleID(DeviceInit, &compatId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = RtlUnicodeStringPrintf(&compatId, L"%s\\%sFUNC_%02X",
                prefix, funcPrefix, Desc->CodecIds.FuncId);
            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = WdfPdoInitAddCompatibleID(DeviceInit, &compatId);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }
    }

    status = RtlUnicodeStringPrintf(&buffer, L"%02d", Desc->CodecIds.CodecAddress);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = WdfPdoInitAssignInstanceID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Provide a description about the device. This text is usually read from
    // the device. In the case of USB device, this text comes from the string
    // descriptor. This text is displayed momentarily by the PnP manager while
    // it's looking for a matching INF. If it finds one, it uses the Device
    // Description from the INF file or the friendly name created by
    // coinstallers to display in the device manager. FriendlyName takes
    // precedence over the DeviceDesc from the INF file.
    //
    if (Desc->CodecIds.IsDSP) {
        status = RtlUnicodeStringPrintf(&buffer,
            L"Intel Audio DSP");
    }
    else {
        status = RtlUnicodeStringPrintf(&buffer,
            L"High Definition Audio Device");
    }
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // You can call WdfPdoInitAddDeviceText multiple times, adding device
    // text for multiple locales. When the system displays the text, it
    // chooses the text that matches the current locale, if available.
    // Otherwise it will use the string for the default locale.
    // The driver can specify the driver's default locale by calling
    // WdfPdoInitSetDefaultLocale.
    //
    status = WdfPdoInitAddDeviceText(DeviceInit,
        &buffer,
        &deviceLocation,
        0x409);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    WdfPdoInitSetDefaultLocale(DeviceInit, 0x409);

    //
    // Initialize the attributes to specify the size of PDO device extension.
    // All the state information private to the PDO will be tracked here.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&pdoAttributes, PDO_DEVICE_DATA);

    status = WdfDeviceCreate(&DeviceInit, &pdoAttributes, &hChild);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Get the device context.
    //
    pdoData = PdoGetData(hChild);

    pdoData->FdoContext = Desc->FdoContext;
    RtlCopyMemory(&pdoData->CodecIds, &Desc->CodecIds, sizeof(Desc->CodecIds));

    if (!Desc->CodecIds.IsDSP)
        Desc->FdoContext->codecs[Desc->CodecIds.CodecAddress] = pdoData;

    //
    // Set some properties for the child device.
    //
    WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);
    pnpCaps.Removable = WdfFalse;
    pnpCaps.EjectSupported = Desc->CodecIds.IsGraphicsCodec ? WdfTrue : WdfFalse;
    pnpCaps.SurpriseRemovalOK = Desc->CodecIds.IsGraphicsCodec ? WdfTrue : WdfFalse;

    pnpCaps.Address = Desc->CodecIds.CodecAddress;
    pnpCaps.UINumber = Desc->CodecIds.CodecAddress;

    WdfDeviceSetPnpCapabilities(hChild, &pnpCaps);

    WDF_DEVICE_POWER_CAPABILITIES_INIT(&powerCaps);

    powerCaps.DeviceD1 = WdfTrue;
    powerCaps.WakeFromD1 = WdfTrue;
    powerCaps.DeviceWake = PowerDeviceD1;

    powerCaps.DeviceState[PowerSystemWorking] = PowerDeviceD1;
    powerCaps.DeviceState[PowerSystemSleeping1] = PowerDeviceD1;
    powerCaps.DeviceState[PowerSystemSleeping2] = PowerDeviceD2;
    powerCaps.DeviceState[PowerSystemSleeping3] = PowerDeviceD2;
    powerCaps.DeviceState[PowerSystemHibernate] = PowerDeviceD3;
    powerCaps.DeviceState[PowerSystemShutdown] = PowerDeviceD3;

    WdfDeviceSetPowerCapabilities(hChild, &powerCaps);

    if (Desc->CodecIds.IsDSP) {
        ADSP_BUS_INTERFACE busInterface = ADSP_BusInterface(pdoData);
        WDF_QUERY_INTERFACE_CONFIG_INIT(&qiConfig,
            (PINTERFACE)&busInterface,
            &GUID_ADSP_BUS_INTERFACE,
            NULL);
        status = WdfDeviceAddQueryInterface(hChild, &qiConfig);
    }
    else {
        HDAUDIO_BUS_INTERFACE busInterface = HDA_BusInterface(pdoData);

        WDF_QUERY_INTERFACE_CONFIG_INIT(&qiConfig,
            (PINTERFACE)&busInterface,
            &GUID_HDAUDIO_BUS_INTERFACE,
            NULL);

        status = WdfDeviceAddQueryInterface(hChild, &qiConfig);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        HDAUDIO_BUS_INTERFACE_V2 busInterface2 = HDA_BusInterfaceV2(pdoData);
        WDF_QUERY_INTERFACE_CONFIG_INIT(&qiConfig,
            (PINTERFACE)&busInterface2,
            &GUID_HDAUDIO_BUS_INTERFACE_V2,
            NULL);
        status = WdfDeviceAddQueryInterface(hChild, &qiConfig);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        HDAUDIO_BUS_INTERFACE_V3 busInterface3 = HDA_BusInterfaceV3(pdoData);
        WDF_QUERY_INTERFACE_CONFIG_INIT(&qiConfig,
            (PINTERFACE)&busInterface3,
            &GUID_HDAUDIO_BUS_INTERFACE_V3,
            NULL);
        status = WdfDeviceAddQueryInterface(hChild, &qiConfig);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    return status;
}