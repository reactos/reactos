/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Internals device init functions
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "common/fxdeviceinit.h"
#include "common/fxdriver.h"
#include "common/fxdevicetext.h"
#include "common/fxdevice.h"

WDFDEVICE_INIT::WDFDEVICE_INIT(
    __in FxDriver* Driver
    ) :
    Driver(Driver)
{
    DriverGlobals = Driver->GetDriverGlobals();

    ReadWriteIoType = WdfDeviceIoBuffered;
    PowerPageable = TRUE;
    Inrush = FALSE;
    DeviceType = FILE_DEVICE_UNKNOWN;
    Characteristics = FILE_DEVICE_SECURE_OPEN;

    RtlZeroMemory(&FileObject, sizeof(FileObject));
    FileObject.AutoForwardCleanupClose = WdfUseDefault;

    DeviceName = NULL;
    CreatedDevice = NULL;

    CreatedOnStack = FALSE;
    Exclusive = FALSE;

    RequiresSelfIoTarget = FALSE;

    RemoveLockOptionFlags = 0;

    RtlZeroMemory(&PnpPower.PnpPowerEventCallbacks, sizeof(PnpPower.PnpPowerEventCallbacks));
    RtlZeroMemory(&PnpPower.PolicyEventCallbacks, sizeof(PnpPower.PolicyEventCallbacks));
    PnpPower.PnpStateCallbacks = NULL;
    PnpPower.PowerStateCallbacks = NULL;
    PnpPower.PowerPolicyStateCallbacks = NULL;

    PnpPower.PowerPolicyOwner = WdfUseDefault;

    InitType = FxDeviceInitTypeFdo;

    RtlZeroMemory(&Fdo.EventCallbacks, sizeof(Fdo.EventCallbacks));
    RtlZeroMemory(&Fdo.ListConfig, sizeof(Fdo.ListConfig));
    RtlZeroMemory(&Fdo.ListConfigAttributes, sizeof(Fdo.ListConfigAttributes));
    Fdo.Filter = FALSE;

    RtlZeroMemory(&Pdo.EventCallbacks, sizeof(Pdo.EventCallbacks));
    Pdo.Raw = FALSE;
    Pdo.Static = FALSE;
    Pdo.DeviceID = NULL;
    Pdo.InstanceID = NULL;
    Pdo.ContainerID = NULL;
    Pdo.DefaultLocale = 0x0;
    Pdo.DescriptionEntry = NULL;
    Pdo.ForwardRequestToParent = FALSE;
    
    RtlZeroMemory(&Security, sizeof(Security));

    RtlZeroMemory(&RequestAttributes, sizeof(RequestAttributes));

    PreprocessInfo = NULL;

    IoInCallerContextCallback = NULL;

    //InitializeListHead(&CxDeviceInitListHead);

    //ReleaseHardwareOrderOnFailure = WdfReleaseHardwareOrderOnFailureEarly;
    
#if (FX_CORE_MODE == FX_CORE_USER_MODE)

    DeviceControlIoType = WdfDeviceIoBuffered;
    DirectTransferThreshold = 0;

    DevStack = NULL;

    KernelDeviceName = NULL;

    PdoKey = NULL;

    ConfigRegistryPath = NULL;

    DevInstanceID = NULL;

    DriverID = 0;

    //
    // Companion related member initilaization
    //
    //Companion = NULL;

    RtlZeroMemory(&CompanionInit.CompanionEventCallbacks, sizeof(CompanionInit.CompanionEventCallbacks));
#endif
}

WDFDEVICE_INIT::~WDFDEVICE_INIT()
{
    //PLIST_ENTRY next;
    
    if (PnpPower.PnpStateCallbacks != NULL)
    {
        delete PnpPower.PnpStateCallbacks;
    }

    if (PnpPower.PowerStateCallbacks != NULL)
    {
        delete PnpPower.PowerStateCallbacks;
    }

    if (PnpPower.PowerPolicyStateCallbacks != NULL)
    {
        delete PnpPower.PowerPolicyStateCallbacks;
    }

    if (DeviceName != NULL)
    {
        DeviceName->DeleteObject();
    }
    if (Pdo.DeviceID != NULL)
    {
        Pdo.DeviceID->DeleteObject();
    }
    if (Pdo.InstanceID != NULL)
    {
        Pdo.InstanceID->DeleteObject();
    }
    if (Pdo.ContainerID != NULL)
    {
        Pdo.ContainerID->DeleteObject();
    }

    FxDeviceText::_CleanupList(&Pdo.DeviceText);

    if (Security.Sddl != NULL)
    {
        Security.Sddl->DeleteObject();
    }
    if (PreprocessInfo != NULL)
    {
        delete PreprocessInfo;
    }

    //
    // NOTE: not contains this code in WDF 1.9
    //
    /*while(!IsListEmpty(&CxDeviceInitListHead))
    {
        next = RemoveHeadList(&CxDeviceInitListHead);
        PWDFCXDEVICE_INIT cxInit;
        cxInit = CONTAINING_RECORD(next, WDFCXDEVICE_INIT, ListEntry);
        InitializeListHead(next);
        delete cxInit;
    }*/

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    delete [] KernelDeviceName;
    delete [] ConfigRegistryPath;
    delete [] DevInstanceID;

    if (PdoKey != NULL)
    {
         RegCloseKey(PdoKey);
         PdoKey = NULL;
    }
#endif

}

BOOLEAN
WDFDEVICE_INIT::ShouldCreateSecure(
    VOID
    )
{
    //
    // Driver explicitly set a class or SDDL, we have to create a secure
    // device.  This will be true for all control devices (SDDL required)
    // and raw PDOs (class required), could be true for FDOs or filters as
    // well.
    //
    if (Security.DeviceClassSet || Security.Sddl != NULL)
    {
        return TRUE;
    }

    //
    // See if there is a name for the device
    //
    if (HasName())
    {
        if (IsPdoInit())
        {
            ASSERT(Pdo.Raw == FALSE);

            DoTraceLevelMessage(
                DriverGlobals, 
                TRACE_LEVEL_WARNING, 
                TRACINGDEVICE,
                "WDFDRIVER 0x%p asked for a named device object, but the PDO "
                "will be created without a name because an SDDL string has not "
                "been specified for the PDO.",
                Driver
                );
            return FALSE;
        }
        else
        {
            //
            // We are creating a named FDO or filter
            //
            ASSERT(IsFdoInit());
            return TRUE;
        }
    }

    //
    // No name involved (FDO or filter)
    //
    ASSERT(IsFdoInit());

    return FALSE;
}

VOID
WDFDEVICE_INIT::SetPdo(
    __in FxDevice* Parent
    )
{
    InitType = FxDeviceInitTypePdo;

    //
    // Remember the parent so we can store it later in WdfDeviceCreate
    //
    Pdo.Parent = Parent;

    //
    // PDOs *must* have a name.  By setting this flag, the driver writer
    // does not need to know this
    //
    Characteristics |= FILE_AUTOGENERATED_DEVICE_NAME;

    //
    // By default, PDOs are not power pageable b/c they do not know how the
    // stack above them will work.  For a "closed" system where the bus driver
    // knows the stack be loaded on its PDO, this may not be true and it
    // can use WdfDeviceInitSetPowerPageable to set it back.
    //
    // In all current shipping OS's, if the parent is power pageable, the
    // child must be power pagable as well.
    //
    if (Parent->IsPowerPageableCapable() == FALSE)
    {
        PowerPageable = FALSE;
    }
}
