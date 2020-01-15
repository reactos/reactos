#include "common/fxdeviceinit.h"
#include "common/fxdriver.h"
#include "common/fxdevicetext.h"

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

    InitializeListHead(&CxDeviceInitListHead);

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
    PLIST_ENTRY next;
    
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
