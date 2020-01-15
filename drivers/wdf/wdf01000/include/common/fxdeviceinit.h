#ifndef _FXDEVICEINIT_H_
#define _FXDEVICEINIT_H_

#include <ntddk.h>
#include "common/fxstump.h"
#include "common/fxstring.h"
#include "common/fxirppreprocessinfo.h"
#include "common/fxpnpcallbacks.h"
#include "common/fxchildlist.h"


enum FxDeviceInitType {
    FxDeviceInitTypeFdo = 0,
    FxDeviceInitTypePdo,
    FxDeviceInitTypeControlDevice,
    FxDeviceInitTypeCompanion
};

struct FileObjectInit {
    WDF_FILEOBJECT_CLASS Class;

    WDF_OBJECT_ATTRIBUTES Attributes;

    WDF_FILEOBJECT_CONFIG Callbacks;

    WDF_TRI_STATE AutoForwardCleanupClose;

    BOOLEAN Set;
};

struct SecurityInit {
    FxString* Sddl;

    GUID DeviceClass;

    BOOLEAN DeviceClassSet;
};

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
//struct CompanionInit
//{
//    WDF_COMPANION_EVENT_CALLBACKS CompanionEventCallbacks;
//};
#endif

struct PnpPowerInit {
    WDF_PNPPOWER_EVENT_CALLBACKS PnpPowerEventCallbacks;

    WDF_POWER_POLICY_EVENT_CALLBACKS PolicyEventCallbacks;

    FxPnpStateCallback* PnpStateCallbacks;

    FxPowerStateCallback* PowerStateCallbacks;

    FxPowerPolicyStateCallback* PowerPolicyStateCallbacks;

    WDF_TRI_STATE PowerPolicyOwner;
};

struct FdoInit {
    WDF_FDO_EVENT_CALLBACKS EventCallbacks;

    WDF_CHILD_LIST_CONFIG ListConfig;

    WDF_OBJECT_ATTRIBUTES ListConfigAttributes;

    BOOLEAN Filter;

    MdDeviceObject PhysicalDevice;
};

struct PdoInit {

    PdoInit(
        VOID
        )
    {
        DeviceText.Next = NULL;
        LastDeviceTextEntry = &DeviceText.Next;
    }

    WDF_PDO_EVENT_CALLBACKS EventCallbacks;

    CfxDevice* Parent;

    FxString* DeviceID;

    FxString* InstanceID;

    //FxCollectionInternal HardwareIDs;

    //FxCollectionInternal CompatibleIDs;

    FxString* ContainerID;

    SINGLE_LIST_ENTRY DeviceText;
    PSINGLE_LIST_ENTRY* LastDeviceTextEntry;

    FxDeviceDescriptionEntry* DescriptionEntry;

    LCID DefaultLocale;

    BOOLEAN Raw;

    BOOLEAN Static;

    BOOLEAN ForwardRequestToParent;
};

struct ControlInit {
    ControlInit(
        VOID
        )
    {
        ShutdownNotification = NULL;
        Flags = 0;
    }

    PFN_WDF_DEVICE_SHUTDOWN_NOTIFICATION ShutdownNotification;

    UCHAR Flags;
};

//
// The typedef for a pointer to this structure is exposed in wdfdevice.h
//
struct WDFDEVICE_INIT : public FxStump {
public:
    WDFDEVICE_INIT(
        __in FxDriver* Driver
        );

    ~WDFDEVICE_INIT();

public:
    PFX_DRIVER_GLOBALS DriverGlobals;

    FxDriver* Driver;

    PVOID CreatedDevice;

    BOOLEAN CreatedOnStack;

    BOOLEAN Exclusive;

    BOOLEAN PowerPageable;

    BOOLEAN Inrush;

    //
    // If set, the Cx/Client intends to leverage Self IO Target
    //
    BOOLEAN RequiresSelfIoTarget;

    ULONG RemoveLockOptionFlags;

    FxDeviceInitType InitType;

    WDF_DEVICE_IO_TYPE ReadWriteIoType;

    DEVICE_TYPE DeviceType;

    FxString* DeviceName;

    ULONG Characteristics;

    FileObjectInit FileObject;

    SecurityInit Security;

    WDF_OBJECT_ATTRIBUTES RequestAttributes;

    FxIrpPreprocessInfo* PreprocessInfo;

    PFN_WDF_IO_IN_CALLER_CONTEXT IoInCallerContextCallback;

    // NOTE: Minimum WDF version - 1.11
    //WDF_RELEASE_HARDWARE_ORDER_ON_FAILURE ReleaseHardwareOrderOnFailure;

    PnpPowerInit    PnpPower;
    FdoInit         Fdo;
    PdoInit         Pdo;
    ControlInit     Control;

    //
    // Class extension's device init.
    //
    LIST_ENTRY      CxDeviceInitListHead;

#if (FX_CORE_MODE == FX_CORE_USER_MODE)

    //CompanionInit   CompanionInit;

    //
    // IoType preference for IOCTL
    //
    WDF_DEVICE_IO_TYPE DeviceControlIoType;

    //
    // Direct I/O threshold
    //
    ULONG DirectTransferThreshold;
    
    //
    // Weak reference to host side device stack
    //
    IWudfDeviceStack * DevStack;

    //
    // Weak reference to host side companion
    //
    //IWudfCompanion * Companion;

    //
    // Kernel redirector's side object name.
    //
    PWSTR KernelDeviceName;

    //
    // PnP devinode hw key handle
    //
    HKEY PdoKey;

    //
    // Registry sub-path name containing driver configuration information. 
    // The registry path name is relative to the "device key".
    //
    PWSTR ConfigRegistryPath;

    //
    // PDO Instance ID
    //
    PWSTR DevInstanceID;

    //
    // This ID is used by Host to associated host device and its driver info.
    //
    ULONG DriverID;
#endif
};

#endif //_FXDEVICEINIT_H_
