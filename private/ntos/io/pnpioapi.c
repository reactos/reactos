
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pnpsubs.c

Abstract:

    This module contains the plug-and-play IO system APIs.

Author:

    Shie-Lin Tzong (shielint) 3-Jan-1995
    Andrew Thornton (andrewth) 5-Sept-1996
    Paula Tomlinson (paulat) 1-May-1997

Environment:

    Kernel mode

Revision History:


--*/

#include "iop.h"
#pragma hdrstop
#include <stddef.h>
#include <wdmguid.h>
#include <pnpmgr.h>
#include <pnpsetup.h>
#include "..\pnp\pnpi.h"

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'oipP')
#endif

#define PNP_DEVICE_EVENT_ENTRY_TAG 'EEpP'


//
// Define device state work item.
//

typedef struct _DEVICE_WORK_ITEM {
    WORK_QUEUE_ITEM WorkItem;
    PDEVICE_OBJECT DeviceObject;
    PVOID Context;
} DEVICE_WORK_ITEM, *PDEVICE_WORK_ITEM;

typedef struct _ASYNC_TDC_WORK_ITEM {
    WORK_QUEUE_ITEM WorkItem;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_CHANGE_COMPLETE_CALLBACK Callback;
    PVOID Context;
    PTARGET_DEVICE_CUSTOM_NOTIFICATION NotificationStructure;
}   ASYNC_TDC_WORK_ITEM, *PASYNC_TDC_WORK_ITEM;

typedef struct _NOTIFICATION_CALLBACK_PARAM_BLOCK {
    PDRIVER_NOTIFICATION_CALLBACK_ROUTINE Callout;
    PVOID   NotificationStructure;
    PVOID   Context;
} NOTIFICATION_CALLBACK_PARAM_BLOCK, *PNOTIFICATION_CALLBACK_PARAM_BLOCK;

NTSTATUS
IopQueueDeviceWorkItem(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PVOID WorkerRoutine,
    IN PVOID Context
    );

VOID
IopInvalidateDeviceStateWorker(
    PVOID Context
    );

VOID
IopReportTargetDeviceChangeAsyncWorker(
    PVOID Context
    );

VOID
IopRequestDeviceEjectWorker(
    PVOID Context
    );

BOOLEAN
IopIsReportedAlready(
    IN HANDLE Handle,
    IN PUNICODE_STRING ServiceName,
    IN PCM_RESOURCE_LIST ResourceList
    );

NTSTATUS
IopSetupDeviceObjectFromDeviceClass(
    IN PDEVICE_OBJECT Pdo,
    IN HANDLE InterfaceClassKey
    );

NTSTATUS
IopSetSecurityObjectFromRegistry(
    IN PVOID Object,
    IN HANDLE Key
    );


NTSTATUS
IopPnPHydraCallback (
    PVOID CallbackParams
    );
//
// Definitions for IoOpenDeviceRegistryKey
//

#define PATH_CURRENTCONTROLSET_HW_PROFILE_CURRENT TEXT("\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet")
#define PATH_CURRENTCONTROLSET                    TEXT("\\Registry\\Machine\\System\\CurrentControlSet")
#define PATH_ENUM                                 TEXT("Enum\\")
#define PATH_CONTROL_CLASS                        TEXT("Control\\Class\\")
#define MAX_RESTPATH_BUF_LEN            512

//
// Definitions for PnpGetDeviceInterfaces
//

#define INITIAL_INFO_BUFFER_SIZE         512
#define INFO_BUFFER_GROW_SIZE            64
#define INITIAL_SYMLINK_BUFFER_SIZE      1024
#define SYMLINK_BUFFER_GROW_SIZE         128
#define INITIAL_RETURN_BUFFER_SIZE       4096
#define RETURN_BUFFER_GROW_SIZE          512
//
// This should never have to grow, since 200 is the maximum length of a device
// instance name...
//
#define INITIAL_DEVNODE_NAME_BUFFER_SIZE   (FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + (200 * sizeof(WCHAR)))

//
// Definitions for PnpOpenDeviceInterfaceRegistryKey
//

#define KEY_STRING_PREFIX                       TEXT("##?#")
#define KEY_STRING_PREFIX_SIZE                  ( sizeof(KEY_STRING_PREFIX) - sizeof(UNICODE_NULL) )
#define KEY_STRING_PREFIX_LENGTH                ( KEY_STRING_PREFIX_SIZE / sizeof(WCHAR) )
// #define SYMBOLIC_LINK_NAME_PREFIX_SIZE          ( sizeof(L"\\DosDevices\\") - sizeof(UNICODE_NULL) )
// #define SYMBOLIC_LINK_NAME_PREFIX_LENGTH        ( SYMBOLIC_LINK_NAME_PREFIX_SIZE / sizeof(WCHAR) )

//
// Definitions for PnpRegisterDeviceInterface
//

#define SEPERATOR_STRING                   TEXT("\\")
#define SEPERATOR_CHAR                     (L'\\')
#define ALT_SEPERATOR_CHAR                 (L'/')
#define REPLACED_SEPERATOR_STRING          TEXT("#")
#define REPLACED_SEPERATOR_CHAR            (L'#')
#define USER_SYMLINK_STRING_PREFIX         TEXT("\\\\?\\")
#define USER_SYMLINK_STRING_PREFIX_LENGTH  (( sizeof(USER_SYMLINK_STRING_PREFIX) - sizeof(UNICODE_NULL) ) / sizeof(WCHAR) )
#define KERNEL_SYMLINK_STRING_PREFIX       TEXT("\\??\\")
#define KERNEL_SYMLINK_STRING_PREFIX_LENGTH (( sizeof(KERNEL_SYMLINK_STRING_PREFIX) - sizeof(UNICODE_NULL) ) / sizeof(WCHAR) )
#define REFSTRING_PREFIX_CHAR              (L'#')

//
// Definitions for PpCreateLegacyDeviceIds
//

#define LEGACY_COMPATIBLE_ID_BASE           TEXT("DETECTED")

//
// Guid related definitions
//

#define GUID_STRING_LENGTH  38
#define GUID_STRING_SIZE    GUID_STRING_LENGTH * sizeof(WCHAR)

//
// Kernel mode notification data
//

LIST_ENTRY IopDeviceClassNotifyList[NOTIFY_DEVICE_CLASS_HASH_BUCKETS];
FAST_MUTEX IopDeviceClassNotifyLock;
PSETUP_NOTIFY_DATA IopSetupNotifyData = NULL;
FAST_MUTEX IopTargetDeviceNotifyLock;
LIST_ENTRY IopProfileNotifyList;
FAST_MUTEX IopHwProfileNotifyLock;
extern BOOLEAN     PiNotificationInProgress;
extern FAST_MUTEX  PiNotificationInProgressLock;
extern NTSTATUS PiNotifyUserMode(
    PPNP_DEVICE_EVENT_ENTRY DeviceEvent
    );

typedef struct _DEFERRED_REGISTRATION_ENTRY {
    LIST_ENTRY            ListEntry;
    PNOTIFY_ENTRY_HEADER  NotifyEntry;
} DEFERRED_REGISTRATION_ENTRY, *PDEFERRED_REGISTRATION_ENTRY;
LIST_ENTRY IopDeferredRegistrationList;
FAST_MUTEX IopDeferredRegistrationLock;

//
// Prototypes
//
NTSTATUS
IopAppendBuffer(
    IN PBUFFER_INFO Info,
    IN PVOID Data,
    IN ULONG DataSize
    );

NTSTATUS
IopOverwriteBuffer(
    IN PBUFFER_INFO Info,
    IN PVOID Data,
    IN ULONG DataSize
    );

NTSTATUS
IopRealloc(
    IN OUT PVOID *Buffer,
    IN ULONG OldSize,
    IN ULONG NewSize
    );

NTSTATUS
IopDeviceInterfaceKeysFromSymbolicLink(
    IN PUNICODE_STRING SymbolicLinkName,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE DeviceInterfaceClassKey     OPTIONAL,
    OUT PHANDLE DeviceInterfaceKey          OPTIONAL,
    OUT PHANDLE DeviceInterfaceInstanceKey  OPTIONAL
    );

NTSTATUS
IopBuildSymbolicLinkStrings(
    IN PUNICODE_STRING DeviceString,
    IN PUNICODE_STRING GuidString,
    IN PUNICODE_STRING ReferenceString      OPTIONAL,
    OUT PUNICODE_STRING UserString,
    OUT PUNICODE_STRING KernelString
    );

NTSTATUS
IopReplaceSeperatorWithPound(
    OUT PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString
    );

NTSTATUS
IopDropReferenceString(
    OUT PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString
    );

NTSTATUS
IopParseSymbolicLinkName(
    IN  PUNICODE_STRING SymbolicLinkName,
    OUT PUNICODE_STRING PrefixString        OPTIONAL,
    OUT PUNICODE_STRING MungedPathString    OPTIONAL,
    OUT PUNICODE_STRING GuidString          OPTIONAL,
    OUT PUNICODE_STRING RefString           OPTIONAL,
    OUT PBOOLEAN        RefStringPresent    OPTIONAL,
    OUT LPGUID Guid                         OPTIONAL
    );

NTSTATUS
IopSetRegistryStringValue(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN PUNICODE_STRING ValueData
    );

VOID
IopInitializePlugPlayNotification(
    VOID
    );

VOID
IopReferenceNotify(
    PNOTIFY_ENTRY_HEADER notify
    );

VOID
IopDereferenceNotify(
    PNOTIFY_ENTRY_HEADER Notify
    );

NTSTATUS
IopRegisterTargetDeviceNotification(
    IN ULONG Flags,
    IN PFILE_OBJECT FileObject,
    IN PDRIVER_OBJECT DriverObject,
    IN PDRIVER_NOTIFICATION_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID Context,
    IN BOOLEAN AddLast,
    OUT PVOID *NotificationEntry
    );

NTSTATUS
IopOpenOrCreateDeviceInterfaceSubKeys(
    OUT PHANDLE InterfaceKeyHandle           OPTIONAL,
    OUT PULONG InterfaceKeyDisposition       OPTIONAL,
    OUT PHANDLE InterfaceInstanceKeyHandle   OPTIONAL,
    OUT PULONG InterfaceInstanceDisposition  OPTIONAL,
    IN HANDLE InterfaceClassKeyHandle,
    IN PUNICODE_STRING DeviceInterfaceName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN Create
    );

NTSTATUS
PpCreateLegacyDeviceIds(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING DriverName,
    IN PCM_RESOURCE_LIST Resources
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopIsReportedAlready)
#pragma alloc_text(PAGE, IoCreateDriver)
#pragma alloc_text(PAGE, IoDeleteDriver)
#pragma alloc_text(PAGE, IoReportDetectedDevice)
#pragma alloc_text(PAGE, IoOpenDeviceRegistryKey)
#pragma alloc_text(PAGE, IoGetDeviceProperty)
#pragma alloc_text(PAGE, IoRegisterPlugPlayNotification)
#pragma alloc_text(PAGE, IoUnregisterPlugPlayNotification)
#pragma alloc_text(PAGE, IoGetDeviceInterfaces)
#pragma alloc_text(PAGE, IopGetDeviceInterfaces)
#pragma alloc_text(PAGE, IoSetDeviceInterfaceState)
#pragma alloc_text(PAGE, IoRegisterDeviceInterface)
#pragma alloc_text(PAGE, IopRegisterDeviceInterface)
#pragma alloc_text(PAGE, IopUnregisterDeviceInterface)
#pragma alloc_text(PAGE, IopRemoveDeviceInterfaces)
#pragma alloc_text(PAGE, IoOpenDeviceInterfaceRegistryKey)
#pragma alloc_text(PAGE, IoGetDeviceInterfaceAlias)
#pragma alloc_text(PAGE, IopDeviceInterfaceKeysFromSymbolicLink)
#pragma alloc_text(PAGE, IopBuildSymbolicLinkStrings)
#pragma alloc_text(PAGE, IopReplaceSeperatorWithPound)
#pragma alloc_text(PAGE, IopAllocateUnicodeString)
#pragma alloc_text(PAGE, IopFreeAllocatedUnicodeString)
#pragma alloc_text(PAGE, IopDropReferenceString)
#pragma alloc_text(PAGE, IopRealloc)
#pragma alloc_text(PAGE, IopSetRegistryStringValue)
#pragma alloc_text(PAGE, IopRegisterTargetDeviceNotification)
#pragma alloc_text(PAGE, IopNotifyDeviceClassChange)
#pragma alloc_text(PAGE, IopNotifyTargetDeviceChange)
#pragma alloc_text(PAGE, IopNotifyHwProfileChange)
#pragma alloc_text(PAGE, IopNotifySetupDeviceArrival)
#pragma alloc_text(PAGE, IopRequestHwProfileChangeNotification)
#pragma alloc_text(PAGE, IoNotifyPowerOperationVetoed)
#pragma alloc_text(PAGE, IopDereferenceNotify)
#pragma alloc_text(PAGE, IopReferenceNotify)
#pragma alloc_text(PAGE, IopInitializePlugPlayNotification)
#pragma alloc_text(PAGE, IoSynchronousInvalidateDeviceRelations)
#pragma alloc_text(PAGE, IopParseSymbolicLinkName)
#pragma alloc_text(PAGE, IopOverwriteBuffer)
#pragma alloc_text(PAGE, IopAppendBuffer)
#pragma alloc_text(PAGE, IopFreeBuffer)
#pragma alloc_text(PAGE, IopResizeBuffer)
#pragma alloc_text(PAGE, IopAllocateBuffer)
#pragma alloc_text(PAGE, IopOpenOrCreateDeviceInterfaceSubKeys)
#pragma alloc_text(PAGE, IoIsWdmVersionAvailable)
#pragma alloc_text(PAGE, IoGetDmaAdapter)
#pragma alloc_text(PAGE, IopGetRelatedTargetDevice)
#pragma alloc_text(PAGE, IoGetRelatedTargetDevice)
#pragma alloc_text(PAGE, IopResourceRequirementsChanged)
#pragma alloc_text(PAGE, PpCreateLegacyDeviceIds)
#pragma alloc_text(PAGE, IopPnPHydraCallback)
#endif // ALLOC_PRAGMA

NTSTATUS
IoGetDeviceProperty(
    IN PDEVICE_OBJECT DeviceObject,
    IN DEVICE_REGISTRY_PROPERTY DeviceProperty,
    IN ULONG BufferLength,
    OUT PVOID PropertyBuffer,
    OUT PULONG ResultLength
    )

/*++

Routine Description:

    This routine lets drivers query the registry properties associated with the
    specified device.

Parameters:

    DeviceObject - Supplies the device object whoes registry property is to be
                   returned.  This device object should be the one created by
                   a bus driver.

    DeviceProperty - Specifies what device property to get.

    BufferLength - Specifies the length, in byte, of the PropertyBuffer.

    PropertyBuffer - Supplies a pointer to a buffer to receive property data.

    ResultLength - Supplies a pointer to a variable to receive the size of the
                   property data returned.

ReturnValue:

    Status code that indicates whether or not the function was successful.  If
    PropertyBuffer is not big enough to hold requested data, STATUS_BUFFER_TOO_SMALL
    will be returned and ResultLength will be set to the number of bytes actually
    required.

--*/

{
    PDEVICE_NODE deviceNode;
    DEVICE_CAPABILITIES capabilities;
    NTSTATUS status;
    HANDLE handle;
    PWSTR valueName, keyName = NULL;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    ULONG valueType;
    ULONG length;
    POBJECT_NAME_INFORMATION deviceObjectName;
    PWSTR deviceInstanceName;
    PWCHAR enumeratorNameEnd;

    PAGED_CODE();

    ASSERT_PDO(DeviceObject);

    //
    // Initialize out parameters
    //

    *ResultLength = 0;

    //
    // Map Device Property to registry value name and value type.
    //

    switch (DeviceProperty) {

    case DevicePropertyDeviceDescription:
        valueName = REGSTR_VALUE_DEVICE_DESC;
        valueType = REG_SZ;
        break;

    case DevicePropertyHardwareID:
        valueName = REGSTR_VAL_HARDWAREID;
        valueType = REG_MULTI_SZ;
        break;

    case DevicePropertyCompatibleIDs:
        valueName = REGSTR_VAL_COMPATIBLEIDS;
        valueType = REG_MULTI_SZ;
        break;

    case DevicePropertyBootConfiguration:
        keyName   = REGSTR_KEY_LOG_CONF;
        valueName = REGSTR_VAL_BOOTCONFIG;
        valueType = REG_RESOURCE_LIST;
        break;

    case DevicePropertyBootConfigurationTranslated:
        //
        // BUGBUG(andrewth) - support this!
        //
        return STATUS_NOT_SUPPORTED;
        break;

    case DevicePropertyClassName:
        valueName = REGSTR_VALUE_CLASS;
        valueType = REG_SZ;
        break;

    case DevicePropertyClassGuid:
        valueName = REGSTR_VALUE_CLASSGUID;
        valueType = REG_SZ;
        break;

    case DevicePropertyDriverKeyName:
        valueName = REGSTR_VALUE_DRIVER;
        valueType = REG_SZ;
        break;

    case DevicePropertyManufacturer:
        valueName = REGSTR_VAL_MFG;
        valueType = REG_SZ;
        break;

    case DevicePropertyFriendlyName:
        valueName = REGSTR_VALUE_FRIENDLYNAME;
        valueType = REG_SZ;
        break;

    case DevicePropertyLocationInformation:
        valueName = REGSTR_VAL_LOCATION_INFORMATION;
        valueType = REG_SZ;
        break;

    case DevicePropertyUINumber:
        valueName = REGSTR_VAL_UI_NUMBER;
        valueType = REG_DWORD;
        break;

    case DevicePropertyPhysicalDeviceObjectName:

        ASSERT (0 == (1 & BufferLength));  // had better be an even length

        //
        // Create a buffer for the Obj manager.
        //
        length = BufferLength + sizeof (OBJECT_NAME_INFORMATION);

        deviceObjectName = (POBJECT_NAME_INFORMATION)
                            ExAllocatePool(PagedPool, length);

        if (NULL == deviceObjectName) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = ObQueryNameString (DeviceObject,
                                    deviceObjectName,
                                    length,
                                    ResultLength);

        if (STATUS_INFO_LENGTH_MISMATCH == status) {
            status = STATUS_BUFFER_TOO_SMALL;
        }

        if (NT_SUCCESS (status)) {

            if (deviceObjectName->Name.Length == 0)  {

                //
                // PDO has no NAME, probably it's been deleted
                //
                *ResultLength = 0;

            } else {

                *ResultLength = deviceObjectName->Name.Length + sizeof(UNICODE_NULL);
                if (*ResultLength > BufferLength) {
                    status = STATUS_BUFFER_TOO_SMALL;
                } else {

                    RtlCopyMemory(PropertyBuffer,
                                  deviceObjectName->Name.Buffer,
                                  deviceObjectName->Name.Length);
                    *(PWCHAR) (((PUCHAR) PropertyBuffer) + deviceObjectName->Name.Length) = L'\0';
                }
            }
        } else {
            *ResultLength -= sizeof(OBJECT_NAME_INFORMATION);
        }

        ExFreePool (deviceObjectName);
        return status;


    case DevicePropertyBusTypeGuid:

        deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
        if (deviceNode->ChildBusTypeIndex != 0xffff
        &&  deviceNode->ChildBusTypeIndex < IopBusTypeGuidList->Count) {

            *ResultLength = sizeof(GUID);

            if(*ResultLength <= BufferLength) {

                RtlCopyMemory(PropertyBuffer,
                              &(IopBusTypeGuidList->Guid[deviceNode->ChildBusTypeIndex]),
                              sizeof(GUID));

                status = STATUS_SUCCESS;

            } else {

                status = STATUS_BUFFER_TOO_SMALL;

            }

        } else {
            status = STATUS_OBJECT_NAME_NOT_FOUND;
        }

        return status;

    case DevicePropertyLegacyBusType:

        deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
        if (deviceNode->ChildInterfaceType != InterfaceTypeUndefined) {

            *ResultLength = sizeof(INTERFACE_TYPE);

            if(*ResultLength <= BufferLength) {

                *(PINTERFACE_TYPE)PropertyBuffer = deviceNode->ChildInterfaceType;
                status = STATUS_SUCCESS;

            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }

        } else {
            status = STATUS_OBJECT_NAME_NOT_FOUND;
        }

        return status;

    case DevicePropertyBusNumber:

        //
        // Retrieve the property from the parent's devnode field.
        //

        deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
        if ((deviceNode->ChildBusNumber & 0x80000000) != 0x80000000) {

            *ResultLength = sizeof(ULONG);

            if(*ResultLength <= BufferLength) {

                *(PULONG)PropertyBuffer = deviceNode->ChildBusNumber;
                status = STATUS_SUCCESS;

            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }

        } else {
            status = STATUS_OBJECT_NAME_NOT_FOUND;
        }

        return status;

    case DevicePropertyEnumeratorName:

        ASSERT (0 == (1 & BufferLength));  // had better be an even length

        deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
        deviceInstanceName = deviceNode->InstancePath.Buffer;

        //
        // There should always be a string here, except for (possibly)
        // HTREE\Root\0, but no one should ever be calling us with that PDO
        // anyway.
        //
        ASSERT (deviceInstanceName);

        //
        // We know we're going to find a separator character (\) in the string,
        // so the fact that unicode strings may not be null-terminated isn't
        // a problem.
        //
        enumeratorNameEnd = wcschr(deviceInstanceName, OBJ_NAME_PATH_SEPARATOR);
        ASSERT (enumeratorNameEnd);

        //
        // Compute required length, minus null terminating character.
        //
        length = (ULONG)((PUCHAR)enumeratorNameEnd - (PUCHAR)deviceInstanceName);

        //
        // Store required length in caller-supplied OUT parameter.
        //
        *ResultLength = length + sizeof(UNICODE_NULL);

        if(*ResultLength > BufferLength) {
            status = STATUS_BUFFER_TOO_SMALL;
        } else {
            memcpy((PUCHAR)PropertyBuffer, (PUCHAR)deviceInstanceName, length);
            *(PWCHAR)((PUCHAR)PropertyBuffer + length) = UNICODE_NULL;
            status = STATUS_SUCCESS;
        }

        return status;

    case DevicePropertyAddress:

        deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;

        status = IopQueryDeviceCapabilities(deviceNode, &capabilities);

        if (NT_SUCCESS(status) && (capabilities.Address != 0xFFFFFFFF)) {

            *ResultLength = sizeof(ULONG);

            if(*ResultLength <= BufferLength) {

                *(PULONG)PropertyBuffer = capabilities.Address;
                status = STATUS_SUCCESS;

            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }

        } else {
            status = STATUS_OBJECT_NAME_NOT_FOUND;
        }

        return status;

    default:
        status = STATUS_INVALID_PARAMETER_2;
        goto clean0;
    }

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulatio
    // portion solves this problem
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&PpRegistryDeviceResource, TRUE);

    //
    // Based on the PDO specified by caller, find the handle of its device
    // instance registry key.
    //

    status = IopDeviceObjectToDeviceInstance(DeviceObject, &handle, KEY_READ);
    if (!NT_SUCCESS(status)) {
        goto clean1;
    }

    //
    // If the data is stored in a subkey then open this key and close the old one
    //

    if (keyName) {
        HANDLE subKeyHandle;
        UNICODE_STRING unicodeKey;

        RtlInitUnicodeString(&unicodeKey, keyName);
        status = IopOpenRegistryKeyEx( &subKeyHandle,
                                       handle,
                                       &unicodeKey,
                                       KEY_READ
                                       );

        if(NT_SUCCESS(status)){
            ZwClose(handle);
            handle = subKeyHandle;
        } else {
            goto clean2;
        }

    }

    //
    // Read the registry value of the desired value name
    //

    status = IopGetRegistryValue (handle,
                                  valueName,
                                  &keyValueInformation);


    //
    // We have finished using the registry so clean up and release our resources
    //

clean2:
    ZwClose(handle);
clean1:
    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    //
    // If we have been sucessful finding the info hand it back to the caller
    //

    if (NT_SUCCESS(status)) {

        //
        // Return the length of the data (or the required length if the buffer is too small)
        //

        *ResultLength = keyValueInformation->DataLength;

        //
        // Check that the buffer we have been given is big enough and that the value returned is
        // of the correct registry type
        //

        if (keyValueInformation->DataLength <= BufferLength) {
            if (keyValueInformation->Type == valueType) {
                RtlCopyMemory(PropertyBuffer,
                              KEY_VALUE_DATA(keyValueInformation),
                              keyValueInformation->DataLength);

            } else {
               status = STATUS_INVALID_PARAMETER_2;
            }
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }

        ExFreePool(keyValueInformation);
    }

clean0:
    return status;
}

NTSTATUS
IoOpenDeviceRegistryKey(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN ULONG DevInstKeyType,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE DevInstRegKey
    )

/*++

Routine Description:

    This routine returns a handle to an opened registry key that the driver
    may use to store/retrieve configuration information specific to a particular
    device instance.

    The driver must call ZwClose to close the handle returned from this api
    when access is no longer required.

Parameters:

    DeviceObject   - Supples the device object of the physical device instance to
                     open a registry storage key for.  Normally it is a device object
                     created by the hal bus extender.

    DevInstKeyType - Supplies flags specifying which storage key associated with
                     the device instance is to be opened.  May be a combination of
                     the following value:

                     PLUGPLAY_REGKEY_DEVICE - Open a key for storing device specific
                         (driver-independent) information relating to the device instance.
                         The flag may not be specified with PLUGPLAY_REGKEY_DRIVER.

                     PLUGPLAY_REGKEY_DRIVER - Open a key for storing driver-specific
                         information relating to the device instance,  This flag may
                         not be specified with PLUGPLAY_REGKEY_DEVICE.

                     PLUGPLAY_REGKEY_CURRENT_HWPROFILE - If this flag is specified,
                         then a key in the current hardware profile branch will be
                         opened for the specified storage type.  This allows the driver
                         to access configuration information that is hardware profile
                         specific.

    DesiredAccess - Specifies the access mask for the key to be opened.

    DevInstRegKey - Supplies the address of a variable that receives a handle to the
                    opened key for the specified registry storage location.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{

    NTSTATUS status, appendStatus;
    HANDLE hBasePath;
    UNICODE_STRING unicodeBasePath, unicodeRestPath;

    PAGED_CODE();

    //
    // Until SCSIPORT stops passing non PDOs allow the system to boot.
    //
    // ASSERT_PDO(PhysicalDeviceObject);
    //

    //
    // Initialise out parameters
    //

    *DevInstRegKey = NULL;

    //
    // Allocate a buffer to build the RestPath string in
    //

    if(!(unicodeRestPath.Buffer = ExAllocatePool(PagedPool, MAX_RESTPATH_BUF_LEN))) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto clean0;
    }

    unicodeRestPath.Length=0;
    unicodeRestPath.MaximumLength=MAX_RESTPATH_BUF_LEN;

    //
    // Select the base path to the CurrentControlSet based on if we are dealing with
    // a hardware profile or not
    //

    if(DevInstKeyType & PLUGPLAY_REGKEY_CURRENT_HWPROFILE) {
        PiWstrToUnicodeString(&unicodeBasePath, PATH_CURRENTCONTROLSET_HW_PROFILE_CURRENT);

    } else {
        PiWstrToUnicodeString(&unicodeBasePath, PATH_CURRENTCONTROLSET);
    }

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&PpRegistryDeviceResource, TRUE);

    //
    // Open the base registry key
    //

    status = IopOpenRegistryKeyEx( &hBasePath,
                                   NULL,
                                   &unicodeBasePath,
                                   KEY_READ
                                   );

    if(!NT_SUCCESS(status)) {
        goto clean1;
    }

    //
    // Build the RestPath string
    //

    switch (DevInstKeyType) {

    case PLUGPLAY_REGKEY_DEVICE:
    case PLUGPLAY_REGKEY_DEVICE + PLUGPLAY_REGKEY_CURRENT_HWPROFILE:
        {
            PDEVICE_NODE pDeviceNode;

            //
            // Initialise the rest path with Enum\
            //

            appendStatus = RtlAppendUnicodeToString(&unicodeRestPath, PATH_ENUM);
            ASSERT(NT_SUCCESS( appendStatus ));
            //
            // Get the Enumerator\DeviceID\InstanceID path from the DeviceNode
            //

            pDeviceNode = (PDEVICE_NODE) PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;

            //
            // Ensure this is a PDO and not an FDO (only PDO's have a DeviceNode)
            //

            if (pDeviceNode) {
                appendStatus = RtlAppendUnicodeStringToString(&unicodeRestPath, &(pDeviceNode->InstancePath));
                ASSERT(NT_SUCCESS( appendStatus ));
            } else {
                status = STATUS_INVALID_DEVICE_REQUEST;
            }

            break;
        }

    case PLUGPLAY_REGKEY_DRIVER:
    case PLUGPLAY_REGKEY_DRIVER + PLUGPLAY_REGKEY_CURRENT_HWPROFILE:
        {

            HANDLE hDeviceKey;
            PKEY_VALUE_FULL_INFORMATION pDriverKeyInfo;

            //
            // Initialise the rest path with Control\Class\
            //

            appendStatus = RtlAppendUnicodeToString(&unicodeRestPath, PATH_CONTROL_CLASS);
            ASSERT(NT_SUCCESS( appendStatus ));

            //
            // Open the device instance key for this device
            //

            status = IopDeviceObjectToDeviceInstance(PhysicalDeviceObject, &hDeviceKey, KEY_READ);

            if(!NT_SUCCESS(status)){
                goto clean1;
            }

            //
            // See if we have a driver value
            //

            status = IopGetRegistryValue(hDeviceKey, REGSTR_VALUE_DRIVER, &pDriverKeyInfo );

            if(NT_SUCCESS(status)){

                if(pDriverKeyInfo->Type == REG_SZ) {

                //
                // Append <DevInstClass>\<ClassInstanceOrdinal>
                //

                appendStatus = RtlAppendUnicodeToString(&unicodeRestPath, (PWSTR) KEY_VALUE_DATA(pDriverKeyInfo));
                ASSERT(NT_SUCCESS( appendStatus ));

                } else {
                    //
                    // We have a driver key with a non REG_SZ type - something is wrong - blame the PDO!
                    //

                    status = STATUS_INVALID_PARAMETER_1;
                }

                ExFreePool(pDriverKeyInfo);
            }

            ZwClose(hDeviceKey);

            break;
        }
    default:
        status = STATUS_INVALID_PARAMETER_3;
        goto clean2;
    }


    //
    // If we succeeded in building the rest path then open the key and hand it back to the caller
    //

    if (NT_SUCCESS(status)){
        if (DevInstKeyType == PLUGPLAY_REGKEY_DEVICE) {

            status = IopOpenDeviceParametersSubkey(DevInstRegKey,
                                                   hBasePath,
                                                   &unicodeRestPath,
                                                   DesiredAccess);
        } else {

            status = IopCreateRegistryKeyEx( DevInstRegKey,
                                             hBasePath,
                                             &unicodeRestPath,
                                             DesiredAccess,
                                             REG_OPTION_NON_VOLATILE,
                                             NULL
                                             );
        }
    }

    //
    // Free up resources
    //

clean2:
    ZwClose(hBasePath);
clean1:
    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();
    ExFreePool(unicodeRestPath.Buffer);
clean0:
    return status;

}

NTSTATUS
IoCreateDriver(
    IN PUNICODE_STRING DriverName    OPTIONAL,
    IN PDRIVER_INITIALIZE InitializationFunction
    )
/*++

Routine Description:

    This routine creates a driver object for a kernel component that
    was not loaded as a driver.  If the creation of the driver object
    succeeds, Initialization function is invoked with the same parameters
    as passed to DriverEntry.

Parameters:

    DriverName - Supplies the name of the driver for which a driver object
                 is to be created.

    InitializationFunction - Equivalent to DriverEntry().

ReturnValue:

    Status code that indicates whether or not the function was successful.

Notes:

--*/
{
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS status;
    PDRIVER_OBJECT driverObject;
    HANDLE driverHandle;
    ULONG objectSize;
    USHORT length;
    UNICODE_STRING driverName, serviceName;
    WCHAR buffer[60];
    ULONG i;

    PAGED_CODE();

    if (DriverName == NULL) {

        //
        // Madeup a name for the driver object.
        //

        length = (USHORT) _snwprintf(buffer, sizeof(buffer) / sizeof(WCHAR), L"\\Driver\\%08u", KeTickCount);
        driverName.Length = length * sizeof(WCHAR);
        driverName.MaximumLength = driverName.Length + sizeof(UNICODE_NULL);
        driverName.Buffer = buffer;                                                           \
    } else {
        driverName = *DriverName;
    }

    //
    // Attempt to create the driver object
    //

    InitializeObjectAttributes( &objectAttributes,
                                &driverName,
                                OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    objectSize = sizeof( DRIVER_OBJECT ) + sizeof( DRIVER_EXTENSION );
    status = ObCreateObject( KernelMode,
                             IoDriverObjectType,
                             &objectAttributes,
                             KernelMode,
                             NULL,
                             objectSize,
                             0,
                             0,
                             &driverObject );

    if( !NT_SUCCESS( status )){

        //
        // Driver object creation failed
        //

        return status;
    }

    //
    // We've created a driver object, initialize it.
    //

    RtlZeroMemory( driverObject, objectSize );
    driverObject->DriverExtension = (PDRIVER_EXTENSION)(driverObject + 1);
    driverObject->DriverExtension->DriverObject = driverObject;
    driverObject->Type = IO_TYPE_DRIVER;
    driverObject->Size = sizeof( DRIVER_OBJECT );
    driverObject->Flags = DRVO_BUILTIN_DRIVER;
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        driverObject->MajorFunction[i] = IopInvalidDeviceRequest;
    driverObject->DriverInit = InitializationFunction;

    serviceName.Buffer = (PWSTR)ExAllocatePool(PagedPool, driverName.Length + sizeof(WCHAR));
    if (serviceName.Buffer) {
        serviceName.MaximumLength = driverName.Length + sizeof(WCHAR);
        serviceName.Length = driverName.Length;
        RtlMoveMemory(serviceName.Buffer, driverName.Buffer, driverName.Length);
        serviceName.Buffer[serviceName.Length / sizeof(WCHAR)] = UNICODE_NULL;
        driverObject->DriverExtension->ServiceKeyName = serviceName;
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorFreeDriverObject;
    }

    //
    // Insert it into the object table.
    //

    status = ObInsertObject( driverObject,
                             NULL,
                             FILE_READ_DATA,
                             0,
                             NULL,
                             &driverHandle );

    if( !NT_SUCCESS( status )){

        //
        // Couldn't insert the driver object into the table.
        // The object got dereferenced by the object manager. Just exit
        //

        goto errorReturn;
    }

    //
    // Reference the handle and obtain a pointer to the driver object so that
    // the handle can be deleted without the object going away.
    //

    status = ObReferenceObjectByHandle( driverHandle,
                                        0,
                                        IoDriverObjectType,
                                        KernelMode,
                                        (PVOID *) &driverObject,
                                        (POBJECT_HANDLE_INFORMATION) NULL );
    if( !NT_SUCCESS( status )) {
       //
       // Backout here is probably bogus. If the ref didn't work then the handle is probably bad
       // Do this right though just in case there are other common error returns for ObRef...
       //
       ZwMakeTemporaryObject( driverHandle ); // Cause handle close to free the object
       ZwClose( driverHandle ); // Close the handle.
       goto errorReturn;
    }

    ZwClose( driverHandle );

    //
    // Store the name of the device driver in the driver object so that it
    // can be easily found by the error log thread.
    //

    driverObject->DriverName.Buffer = ExAllocatePool( PagedPool,
                                                      driverName.MaximumLength );
    if (driverObject->DriverName.Buffer) {
        driverObject->DriverName.MaximumLength = driverName.MaximumLength;
        driverObject->DriverName.Length = driverName.Length;

        RtlCopyMemory( driverObject->DriverName.Buffer,
                       driverName.Buffer,
                       driverName.MaximumLength );
    }

    //
    // Call the driver initialization routine
    //

    status = (*InitializationFunction)(driverObject, NULL);

    if( !NT_SUCCESS( status )){

errorFreeDriverObject:

        //
        // If we were unsuccessful, we need to get rid of the driverObject
        // that we created.
        //

        ObMakeTemporaryObject( driverObject );
        ObDereferenceObject( driverObject );
    }
errorReturn:
    return status;
}

VOID
IoDeleteDriver(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    This routine deletes a driver object created explicitly through
    IoCreateDriver.

Parameters:

    DriverObject - Supplies a pointer to the driver object to be deleted.

ReturnValue:

    Status code that indicates whether or not the function was successful.

Notes:

--*/
{

    ObDereferenceObject(DriverObject);
}

NTSTATUS
IoSynchronousInvalidateDeviceRelations(
    PDEVICE_OBJECT DeviceObject,
    DEVICE_RELATION_TYPE Type
    )

/*++

Routine Description:

    This API notifies the system that changes have occurred in the device
    relations of the specified type for the supplied DeviceObject.   All
    cached information concerning the relationships must be invalidated,
    and if needed re-obtained via IRP_MN_QUERY_DEVICE_RELATIONS.

    This routine performs device enumeration synchronously.
    Note, A driver can NOT call this IO api while processing pnp irps AND
    A driver can NOT call this api from any system thread except the system
    threads created by the driver itself.

Parameters:

    DeviceObject - the PDEVICE_OBJECT for which the specified relation type
                   information has been invalidated.  This pointer is valid
                   for the duration of the call.

    Type - specifies the type of the relation being invalidated.

ReturnValue:

    Status code that indicates whether or not the function was successful.

--*/

{
    PDEVICE_NODE deviceNode;
    NTSTATUS status = STATUS_SUCCESS;
    KEVENT completionEvent;

    PAGED_CODE();

    ASSERT_PDO(DeviceObject);

    switch (Type) {
    case BusRelations:

        if (PnPInitialized) {

            deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;

            if (deviceNode->Flags & DNF_STARTED) {

                KeInitializeEvent( &completionEvent, NotificationEvent, FALSE );

                status = IopRequestDeviceAction( DeviceObject,
                                                 ReenumerateDeviceTree,
                                                 &completionEvent,
                                                 NULL );

                if (NT_SUCCESS(status)) {

                    status = KeWaitForSingleObject( &completionEvent,
                                                    Executive,
                                                    KernelMode,
                                                    FALSE,
                                                    NULL);
                }
            } else {
                status = STATUS_UNSUCCESSFUL;
            }
        } else {
            //
            // BUGBUG - This check may be too much.  For now ...
            //

            status = STATUS_UNSUCCESSFUL;  // BUGBUG- better status code
        }


        break;

    case EjectionRelations:

        //
        // For Ejection relation change, we will ignore it.  We don't keep track
        // the Ejection relation.  We will query the Ejection relation only when
        // we are requested to eject a device.
        //

        status = STATUS_NOT_SUPPORTED;
        break;

    case PowerRelations:


        //
        // Call off to Po code, which will do the right thing
        //
        PoInvalidateDevicePowerRelations(DeviceObject);
        break;
    }
    return status;
}

VOID
IoInvalidateDeviceRelations(
    PDEVICE_OBJECT DeviceObject,
    DEVICE_RELATION_TYPE Type
    )

/*++

Routine Description:

    This API notifies the system that changes have occurred in the device
    relations of the specified type for the supplied DeviceObject.   All
    cached information concerning the relationships must be invalidated,
    and if needed re-obtained via IRP_MN_QUERY_DEVICE_RELATIONS.

Parameters:

    DeviceObject - the PDEVICE_OBJECT for which the specified relation type
                   information has been invalidated.  This pointer is valid
                   for the duration of the call.

    Type - specifies the type of the relation being invalidated.

ReturnValue:

    none.

--*/

{

    PDEVICE_NODE deviceNode;
    KIRQL        oldIrql;

    ASSERT_PDO(DeviceObject);

    switch (Type) {
    case BusRelations:

        //
        // If the call was made before PnP completes device enumeration
        // we can safely ignore it.  PnP manager will do it without
        // driver's request.
        //

        deviceNode = (PDEVICE_NODE) DeviceObject->DeviceObjectExtension->DeviceNode;
        if (deviceNode) {

            ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);
            if (deviceNode->Flags & DNF_BEING_ENUMERATED) {
                deviceNode->Flags |= DNF_IO_INVALIDATE_DEVICE_RELATIONS_PENDING;
                ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
            } else if ((deviceNode->Flags & DNF_STARTED)                       &&
                !(deviceNode->Flags & DNF_ENUMERATION_REQUEST_QUEUED)) {
                deviceNode->Flags |= DNF_ENUMERATION_REQUEST_QUEUED;
                ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);

                IopRequestDeviceAction( DeviceObject,
                                        ReenumerateDeviceTree,
                                        NULL,
                                        NULL );
            } else {
                ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
            }
        }
        break;

    case EjectionRelations:

        //
        // For Ejection relation change, we will ignore it.  We don't keep track
        // the Ejection relation.  We will query the Ejection relation only when
        // we are requested to eject a device.

        break;

    case PowerRelations:

        //
        // Call off to Po code, which will do the right thing
        //
        PoInvalidateDevicePowerRelations(DeviceObject);
        break;
    }
}

VOID
IoRequestDeviceEject(
    PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This API notifies that the device eject button has been pressed. This API must
    be called at IRQL <= DISPATCH_LEVEL.

    This API informs PnP that a device eject has been requested, the device will
    not necessarily be ejected as a result of this API.  The device will only be
    ejected if the drivers associated with it agree to stop and the device is
    successfully powered down.  Note that eject in this context refers to device
    eject, not to media (floppies, cds, tapes) eject.  For example, eject of a
    cd-rom disk drive, not ejection of a cd-rom disk.

Arguments:

    DeviceObject - the PDEVICE_OBJECT for the device whose eject button has
                   been pressed.  This pointer is valid for the duration of
                   the call, if the API wants to keep a copy of it, it
                   should obtain its own reference to the object
                   (ObReferenceObject).

ReturnValue:

    None.

--*/

{
    ASSERT_PDO(DeviceObject);

    IopQueueDeviceWorkItem(DeviceObject, IopRequestDeviceEjectWorker, NULL);
}

VOID
IopRequestDeviceEjectWorker(
    PVOID Context
    )
{
    PDEVICE_WORK_ITEM deviceWorkItem = (PDEVICE_WORK_ITEM)Context;
    PDEVICE_OBJECT deviceObject = deviceWorkItem->DeviceObject;

    ExFreePool(deviceWorkItem);

    //
    // Queue the event, we'll return immediately after it's queued.
    //

    PpSetTargetDeviceRemove( deviceObject,
                             TRUE,
                             TRUE,
                             TRUE,
                             CM_PROB_DEVICE_NOT_THERE,
                             NULL,
                             NULL,
                             NULL,
                             NULL);

    ObDereferenceObject(deviceObject);
}


NTSTATUS
IoReportDetectedDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN INTERFACE_TYPE LegacyBusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PCM_RESOURCE_LIST ResourceList,
    IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements OPTIONAL,
    IN BOOLEAN ResourceAssigned,
    IN OUT PDEVICE_OBJECT *DeviceObject
    )

/*++

Routine Description:

    PnP device drivers call this API to report any device detected.  This routine
    creates a Physical Device object, reference the Physical Device object and
    returns back to the callers.  Once the detected device is reported, the Pnp manager
    considers the device has been fully controlled by the reporting drivers.  Thus it
    will not invoke AddDevice entry and send StartDevice irp to the driver.

    The driver needs to report the resources it used to detect this device such that
    pnp manager can perform duplicates detection on this device.

    The caller must dereference the DeviceObject once it no longer needs it.

Parameters:

    DriverObject - Supplies the driver object of the driver who detected
                   this device.

    ResourceList - Supplies a pointer to the resource list which the driver used
                   to detect the device.

    ResourceRequirements - supplies a pointer to the resource requirements list
                   for the detected device.  This is optional.

    ResourceAssigned - if TRUE, the driver already called IoReportResourceUsage or
                   IoAssignResource to get the ownership of the resources.  Otherwise,
                   the PnP manager will call IoReportResourceUsage to allocate the
                   resources for the driver.

    DeviceObject - if NULL, this routine will create a PDO and return it thru this variable.
                   Otherwise, a PDO is already created and this routine will simply use the supplied
                   PDO.

Return Value:

    Status code that indicates whether or not the function was successful.


--*/

{
    WCHAR buffer[60];
    NTSTATUS status;
    UNICODE_STRING deviceName, instanceName, unicodeName, *serviceName, driverName;
    PDEVICE_NODE deviceNode;
    ULONG length, i = 0, disposition, tmpValue, listSize = 0;
    HANDLE handle, handle1, logConfHandle, controlHandle, hTreeHandle, enumHandle;
    PCM_RESOURCE_LIST cmResource;
    PWSTR p;
    LARGE_INTEGER tickCount;
    PDEVICE_OBJECT deviceObject;
    BOOLEAN newlyCreated = FALSE;

    PAGED_CODE();

    if (*DeviceObject) {

        deviceObject = *DeviceObject;

        //
        // The PDO is already known. simply handle the resourcelist and resreq list.
        // This is a hack for NDIS drivers.
        //
        deviceNode = (PDEVICE_NODE)(*DeviceObject)->DeviceObjectExtension->DeviceNode;
        if (!deviceNode) {
            return STATUS_NO_SUCH_DEVICE;
        }

        KeEnterCriticalRegion();
        ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

        //
        // Write ResourceList and ResReq list to the device instance
        //

        status = IopDeviceObjectToDeviceInstance (*DeviceObject,
                                                  &handle,
                                                  KEY_ALL_ACCESS
                                                  );
        if (!NT_SUCCESS(status)) {
            ExReleaseResource(&PpRegistryDeviceResource);
            KeLeaveCriticalRegion();
            return status;
        }
        if (ResourceAssigned) {
            RtlInitUnicodeString(&unicodeName, REGSTR_VALUE_NO_RESOURCE_AT_INIT);
            tmpValue = 1;
            ZwSetValueKey(handle,
                          &unicodeName,
                          TITLE_INDEX_VALUE,
                          REG_DWORD,
                          &tmpValue,
                          sizeof(tmpValue)
                          );
        }
        RtlInitUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
        status = IopCreateRegistryKeyEx( &logConfHandle,
                                         handle,
                                         &unicodeName,
                                         KEY_ALL_ACCESS,
                                         REG_OPTION_NON_VOLATILE,
                                         NULL
                                         );
        ZwClose(handle);
        if (NT_SUCCESS(status)) {

            //
            // Write the ResourceList and and ResourceRequirements to the logconf key under
            // device instance key.
            //

            if (ResourceList) {
                RtlInitUnicodeString(&unicodeName, REGSTR_VAL_BOOTCONFIG);
                ZwSetValueKey(
                          logConfHandle,
                          &unicodeName,
                          TITLE_INDEX_VALUE,
                          REG_RESOURCE_LIST,
                          ResourceList,
                          listSize = IopDetermineResourceListSize(ResourceList)
                          );
            }
            if (ResourceRequirements) {
                RtlInitUnicodeString(&unicodeName, REGSTR_VALUE_BASIC_CONFIG_VECTOR);
                ZwSetValueKey(
                          logConfHandle,
                          &unicodeName,
                          TITLE_INDEX_VALUE,
                          REG_RESOURCE_REQUIREMENTS_LIST,
                          ResourceRequirements,
                          ResourceRequirements->ListSize
                          );
            }
            ZwClose(logConfHandle);
        }
        ExReleaseResource(&PpRegistryDeviceResource);
        KeLeaveCriticalRegion();
        if (NT_SUCCESS(status)) {
            goto checkResource;
        } else {
            return status;
        }
    }

    //
    // Normal case: *DeviceObject is NULL
    //

    *DeviceObject = NULL;
    serviceName = &DriverObject->DriverExtension->ServiceKeyName;

    //
    // Special handling for driver object created thru IoCreateDriver.
    // When a builtin driver calls IoReportDetectedDevice, the ServiceKeyName of
    // the driver object is set to \Driver\DriverName.  To create a detected device
    // instance key, we will take only the DriverName.
    //

    if (DriverObject->Flags & DRVO_BUILTIN_DRIVER) {
        p = serviceName->Buffer + (serviceName->Length / sizeof(WCHAR)) - 1;
        driverName.Length = 0;
        while (*p != '\\' && (p != serviceName->Buffer)) {
            p--;
            driverName.Length += sizeof(WCHAR);
        }
        if (p == serviceName->Buffer) {
            return STATUS_UNSUCCESSFUL;
        } else {
            p++;
            driverName.Buffer = p;
            driverName.MaximumLength = driverName.Length + sizeof(WCHAR);
        }
    } else {

        //
        // Before doing anything first perform duplicate detection
        //

        status = IopDuplicateDetection(
                     LegacyBusType,
                     BusNumber,
                     SlotNumber,
                     &deviceNode
                 );

        if (NT_SUCCESS(status) && deviceNode) {
            deviceObject = deviceNode->PhysicalDeviceObject;
            if ((deviceNode->Flags & DNF_ADDED) ||
                (IopDoesDevNodeHaveProblem(deviceNode) &&
                 deviceNode->Problem != CM_PROB_NOT_CONFIGURED &&
                 deviceNode->Problem != CM_PROB_REINSTALL &&
                 deviceNode->Problem != CM_PROB_FAILED_INSTALL)) {

                //
                // BUGBUG: This assumption may not be true.
                //

                ObDereferenceObject(deviceObject);

                return STATUS_NO_SUCH_DEVICE;
            }

            deviceNode->Flags &= ~DNF_HAS_PROBLEM;
            deviceNode->Problem = 0;

            IopDeleteLegacyKey(DriverObject);
            goto checkResource;
        }

    }

    //
    // Create a PDO and its DeviceNode
    //

    //
    // Madeup a name for the device object.
    //

    KeQueryTickCount(&tickCount);
    length = _snwprintf(buffer, sizeof(buffer) / sizeof(WCHAR), L"\\Device\\%04u%x", IopNumberDeviceNodes, tickCount.LowPart);
    deviceName.MaximumLength = sizeof(buffer);
    deviceName.Length = (USHORT)(length * sizeof(WCHAR));
    deviceName.Buffer = buffer;                                                           \

    status = IoCreateDevice( IoPnpDriverObject,
                             sizeof(IOPNP_DEVICE_EXTENSION),
                             &deviceName,
                             FILE_DEVICE_CONTROLLER,
                             0,
                             FALSE,
                             &deviceObject );

    if (NT_SUCCESS(status)) {
        deviceObject->Flags |= DO_BUS_ENUMERATED_DEVICE;   // Mark this is a PDO
        deviceNode = IopAllocateDeviceNode(deviceObject);
        if (deviceNode) {

            //
            // First delete the Legacy_DriverName key and subkeys from Enum\Root, if exits.
            //

            if (!(DriverObject->Flags & DRVO_BUILTIN_DRIVER)) {
                IopDeleteLegacyKey(DriverObject);
            }

            //
            // Create the compatible id list we'll use for this made-up device.
            //

            status = PpCreateLegacyDeviceIds(
                        deviceObject,
                        ((DriverObject->Flags & DRVO_BUILTIN_DRIVER) ?
                            &driverName : serviceName),
                        ResourceList);

            if(!NT_SUCCESS(status)) {
                goto exit;
            }

            //
            // Create/Open a registry key for the device instance and
            // write the addr of the device object to registry
            //

            if (DriverObject->Flags & DRVO_BUILTIN_DRIVER) {
                length = _snwprintf(buffer, sizeof(buffer) / sizeof(WCHAR), L"ROOT\\%s", driverName.Buffer);
            } else {
                length = _snwprintf(buffer, sizeof(buffer) / sizeof(WCHAR), L"ROOT\\%s", serviceName->Buffer);
            }
            deviceName.MaximumLength = sizeof(buffer);
            ASSERT(length <= sizeof(buffer) - 10);
            deviceName.Length = (USHORT)(length * sizeof(WCHAR));
            deviceName.Buffer = buffer;

            KeEnterCriticalRegion();
            ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

            status = IopOpenRegistryKeyEx( &enumHandle,
                                           NULL,
                                           &CmRegistryMachineSystemCurrentControlSetEnumName,
                                           KEY_ALL_ACCESS
                                           );
            if (!NT_SUCCESS(status)) {
                goto exit;
            }

            status = IopCreateRegistryKeyEx( &handle1,
                                             enumHandle,
                                             &deviceName,
                                             KEY_ALL_ACCESS,
                                             REG_OPTION_NON_VOLATILE,
                                             &disposition
                                             );

            if (NT_SUCCESS(status)) {
                deviceName.Buffer[deviceName.Length / sizeof(WCHAR)] =
                           OBJ_NAME_PATH_SEPARATOR;
                deviceName.Length += sizeof(WCHAR);
                if (disposition != REG_CREATED_NEW_KEY) {
                    while (TRUE) {
                        PiUlongToInstanceKeyUnicodeString(&instanceName,
                                                          buffer + deviceName.Length / sizeof(WCHAR),
                                                          sizeof(buffer) - deviceName.Length,
                                                          i
                                                          );
                        status = IopCreateRegistryKeyEx( &handle,
                                                         handle1,
                                                         &instanceName,
                                                         KEY_ALL_ACCESS,
                                                         REG_OPTION_NON_VOLATILE,
                                                         &disposition
                                                         );
                        if (NT_SUCCESS(status)) {
                            if (disposition == REG_CREATED_NEW_KEY) {
                                ZwClose(handle1);
                                break;
                            } else {
                                if (IopIsReportedAlready(handle, serviceName, ResourceList)) {

                                    //
                                    // Write the reported resources to registry in case the irq changed
                                    //

                                    RtlInitUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
                                    status = IopCreateRegistryKeyEx( &logConfHandle,
                                                                     handle,
                                                                     &unicodeName,
                                                                     KEY_ALL_ACCESS,
                                                                     REG_OPTION_NON_VOLATILE,
                                                                     NULL
                                                                     );
                                    if (NT_SUCCESS(status)) {

                                        //
                                        // Write the ResourceList and and ResourceRequirements to the device instance key
                                        //

                                        if (ResourceList) {
                                            RtlInitUnicodeString(&unicodeName, REGSTR_VAL_BOOTCONFIG);
                                            ZwSetValueKey(
                                                      logConfHandle,
                                                      &unicodeName,
                                                      TITLE_INDEX_VALUE,
                                                      REG_RESOURCE_LIST,
                                                      ResourceList,
                                                      listSize = IopDetermineResourceListSize(ResourceList)
                                                      );
                                        }
                                        if (ResourceRequirements) {
                                            RtlInitUnicodeString(&unicodeName, REGSTR_VALUE_BASIC_CONFIG_VECTOR);
                                            ZwSetValueKey(
                                                      logConfHandle,
                                                      &unicodeName,
                                                      TITLE_INDEX_VALUE,
                                                      REG_RESOURCE_REQUIREMENTS_LIST,
                                                      ResourceRequirements,
                                                      ResourceRequirements->ListSize
                                                      );
                                        }
                                        ZwClose(logConfHandle);
                                    }

                                    ExReleaseResource(&PpRegistryDeviceResource);
                                    KeLeaveCriticalRegion();
                                    IoDeleteDevice(deviceObject);
                                    ZwClose(handle1);
                                    deviceObject = IopDeviceObjectFromDeviceInstance (
                                                           handle, NULL);  // Add a reference
                                    ZwClose(handle);
                                    ZwClose(enumHandle);
                                    ASSERT(deviceObject);
                                    if (deviceObject == NULL) {
                                        status = STATUS_UNSUCCESSFUL;
                                        return status;
                                    }
                                    deviceNode = (PDEVICE_NODE)
                                                  deviceObject->DeviceObjectExtension->DeviceNode;
                                    goto checkResource;
                                } else {
                                    i++;
                                    ZwClose(handle);
                                    continue;
                                }
                            }
                        } else {
                            ZwClose(handle1);
                            ZwClose(enumHandle);
                            goto exit;
                        }
                    }
                } else {

                    //
                    // This is a new device key.  So, instance is 0.  Create it.
                    //

                    PiUlongToInstanceKeyUnicodeString(&instanceName,
                                                      buffer + deviceName.Length / sizeof(WCHAR),
                                                      sizeof(buffer) - deviceName.Length,
                                                      i
                                                      );
                    status = IopCreateRegistryKeyEx( &handle,
                                                     handle1,
                                                     &instanceName,
                                                     KEY_ALL_ACCESS,
                                                     REG_OPTION_NON_VOLATILE,
                                                     &disposition
                                                     );
                    ZwClose(handle1);
                    if (!NT_SUCCESS(status)) {
                        ZwClose(enumHandle);
                        goto exit;
                    }
                    ASSERT(disposition == REG_CREATED_NEW_KEY);
                }
            } else {
                ZwClose(enumHandle);
                goto exit;
            }

            deviceName.Length += instanceName.Length;
            ASSERT(disposition == REG_CREATED_NEW_KEY);
            newlyCreated = TRUE;

            //
            // Initialize new device instance registry key
            //

            if (ResourceAssigned) {
                RtlInitUnicodeString(&unicodeName, REGSTR_VALUE_NO_RESOURCE_AT_INIT);
                tmpValue = 1;
                ZwSetValueKey(handle,
                              &unicodeName,
                              TITLE_INDEX_VALUE,
                              REG_DWORD,
                              &tmpValue,
                              sizeof(tmpValue)
                              );
            }
            RtlInitUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
            logConfHandle = NULL;
            status = IopCreateRegistryKeyEx( &logConfHandle,
                                             handle,
                                             &unicodeName,
                                             KEY_ALL_ACCESS,
                                             REG_OPTION_NON_VOLATILE,
                                             NULL
                                             );

            ASSERT(status == STATUS_SUCCESS);

            if (NT_SUCCESS(status)) {

                //
                // Write the ResourceList and and ResourceRequirements to the logconf key under
                // device instance key.
                //

                if (ResourceList) {
                    RtlInitUnicodeString(&unicodeName, REGSTR_VAL_BOOTCONFIG);
                    ZwSetValueKey(
                              logConfHandle,
                              &unicodeName,
                              TITLE_INDEX_VALUE,
                              REG_RESOURCE_LIST,
                              ResourceList,
                              listSize = IopDetermineResourceListSize(ResourceList)
                              );
                }
                if (ResourceRequirements) {
                    RtlInitUnicodeString(&unicodeName, REGSTR_VALUE_BASIC_CONFIG_VECTOR);
                    ZwSetValueKey(
                              logConfHandle,
                              &unicodeName,
                              TITLE_INDEX_VALUE,
                              REG_RESOURCE_REQUIREMENTS_LIST,
                              ResourceRequirements,
                              ResourceRequirements->ListSize
                              );
                }
                //ZwClose(logConfHandle);
            }

            RtlInitUnicodeString(&unicodeName, REGSTR_VALUE_CONFIG_FLAGS);
            tmpValue = CONFIGFLAG_FINISH_INSTALL;
            ZwSetValueKey(handle,
                          &unicodeName,
                          TITLE_INDEX_VALUE,
                          REG_DWORD,
                          &tmpValue,
                          sizeof(tmpValue)
                          );

            PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_LEGACY);
            tmpValue = 0;
            ZwSetValueKey(
                        handle,
                        &unicodeName,
                        TITLE_INDEX_VALUE,
                        REG_DWORD,
                        &tmpValue,
                        sizeof(ULONG)
                        );

            RtlInitUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
            controlHandle = NULL;
            IopCreateRegistryKeyEx( &controlHandle,
                                    handle,
                                    &unicodeName,
                                    KEY_ALL_ACCESS,
                                    REG_OPTION_VOLATILE,
                                    NULL
                                    );

            ASSERT(status == STATUS_SUCCESS);

            if (NT_SUCCESS(status)) {

                //
                // Write DeviceObject reference ...
                //

                PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_DEVICE_REFERENCE);
                status = ZwSetValueKey(controlHandle,
                                       &unicodeName,
                                       TITLE_INDEX_VALUE,
                                       REG_DWORD,
                                       (PULONG_PTR)&deviceObject,
                                       sizeof(ULONG_PTR)
                                       );
                PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_DEVICE_REPORTED);
                tmpValue = 1;
                status = ZwSetValueKey(controlHandle,
                                       &unicodeName,
                                       TITLE_INDEX_VALUE,
                                       REG_DWORD,
                                       &tmpValue,
                                       sizeof(ULONG)
                                       );
                status = ZwSetValueKey(handle,
                                       &unicodeName,
                                       TITLE_INDEX_VALUE,
                                       REG_DWORD,
                                       &tmpValue,
                                       sizeof(ULONG)
                                       );

                //ZwClose(controlHandle);
            }

            ZwClose(enumHandle);

            //
            // Create Service value name and set it to the calling driver's service
            // key name.
            //

            PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_SERVICE);
            p = (PWSTR)ExAllocatePool(PagedPool, serviceName->Length + sizeof(UNICODE_NULL));
            if (!p) {
                goto CleanupRegistry;
            }
            RtlMoveMemory(p, serviceName->Buffer, serviceName->Length);
            p[serviceName->Length / sizeof (WCHAR)] = UNICODE_NULL;
            ZwSetValueKey(
                        handle,
                        &unicodeName,
                        TITLE_INDEX_VALUE,
                        REG_SZ,
                        p,
                        serviceName->Length + sizeof(UNICODE_NULL)
                        );
            if (DriverObject->Flags & DRVO_BUILTIN_DRIVER) {
                deviceNode->ServiceName = *serviceName;
            } else {
                ExFreePool(p);
            }

            ExReleaseResource(&PpRegistryDeviceResource);
            KeLeaveCriticalRegion();
            //ZwClose(logConfHandle);
            //ZwClose(controlHandle);
            //ZwClose(handle);

            //
            // Register the device for the driver and save the device
            // instance path in device node.
            //

            if (!(DriverObject->Flags & DRVO_BUILTIN_DRIVER)) {
                PpDeviceRegistration( &deviceName,
                                      TRUE,
                                      &deviceNode->ServiceName
                                      );
            }

            IopConcatenateUnicodeStrings(&deviceNode->InstancePath, &deviceName, NULL);

            deviceNode->Flags = DNF_MADEUP + DNF_ENUMERATED + DNF_PROCESSED;

            IopInsertTreeDeviceNode(IopRootDeviceNode, deviceNode);

            //
            // Add a reference to the DeviceObject for ourself
            //

            ObReferenceObject(deviceObject);

            IopNotifySetupDeviceArrival(deviceObject, NULL, FALSE);

            goto checkResource;
        } else {
            IoDeleteDevice(deviceObject);
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    return status;
checkResource:


    //
    // At this point the *DeviceObject is established.  Check if we need to report resources for
    // the detected device.  If we failed to
    //

    if (ResourceAssigned) {
        //ASSERT(deviceNode->ResourceList == NULL);      // make sure we have not reported resources yet.

        //
        // If the driver specifies it already has acquired the resource.  We will put a flag
        // in the device instance path to not to allocate resources at boot time.  The Driver
        // may do detection and report it again.
        //

        deviceNode->Flags |= DNF_NO_RESOURCE_REQUIRED; // do not need resources for this boot.
        if (ResourceList) {

            //
            // Write the resource list to the reported device instance key.
            //

            listSize = IopDetermineResourceListSize(ResourceList);
            IopWriteAllocatedResourcesToRegistry (deviceNode, ResourceList, listSize);
        }
    } else {
        BOOLEAN conflict;

        if (ResourceList && ResourceList->Count && ResourceList->List[0].PartialResourceList.Count) {
            if (listSize == 0) {
                listSize = IopDetermineResourceListSize(ResourceList);
            }
            cmResource = (PCM_RESOURCE_LIST) ExAllocatePool(PagedPool, listSize);
            if (cmResource) {
                RtlCopyMemory(cmResource, ResourceList, listSize);
                RtlInitUnicodeString(&unicodeName, PNPMGR_STR_PNP_MANAGER);
                status = IoReportResourceUsageInternal(
                             ArbiterRequestLegacyReported,
                             &unicodeName,       // DriverClassName OPTIONAL,
                             IoPnpDriverObject,  // DriverObject,
                             NULL,               // DriverList OPTIONAL,
                             0,                  // DriverListSize OPTIONAL,
                             deviceNode->PhysicalDeviceObject,
                                                 // DeviceObject OPTIONAL,
                             cmResource,         // DeviceList OPTIONAL,
                             listSize,           // DeviceListSize OPTIONAL,
                             FALSE,              // OverrideConflict,
                             &conflict           // ConflictDetected
                             );
                ExFreePool(cmResource);
                if (!NT_SUCCESS(status) || conflict) {
                    status = STATUS_CONFLICTING_ADDRESSES;
                    IopSetDevNodeProblem(deviceNode, CM_PROB_NORMAL_CONFLICT);
                }
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
                IopSetDevNodeProblem(deviceNode, CM_PROB_OUT_OF_MEMORY);
            }
        } else {
            ASSERT(ResourceRequirements == NULL);
            deviceNode->Flags |= DNF_NO_RESOURCE_REQUIRED; // do not need resources for this boot.
        }
    }

    if (NT_SUCCESS(status)) {

        IopDoDeferredSetInterfaceState(deviceNode);

        deviceNode->Flags |= DNF_STARTED | DNF_ADDED | DNF_NEED_ENUMERATION_ONLY | DNF_NEED_QUERY_IDS;
        *DeviceObject = deviceObject;
        if (newlyCreated) {
            if (controlHandle) {
                ZwClose(controlHandle);
            }
            if (logConfHandle) {
                ZwClose(logConfHandle);
            }
            ZwClose(handle);
        }
        return status;

    }
CleanupRegistry:
    IopReleaseDeviceResources(deviceNode, FALSE);
    if (newlyCreated) {
        IoDeleteDevice(deviceObject);
        if (controlHandle) {
            ZwDeleteKey(controlHandle);
        }
        if (logConfHandle) {
            ZwDeleteKey(logConfHandle);
        }
        if (handle) {
            ZwDeleteKey(handle);
        }
    }
    return status;
exit:
    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();
    IoDeleteDevice(*DeviceObject);
    return status;
}

BOOLEAN
IopIsReportedAlready(
    IN HANDLE Handle,
    IN PUNICODE_STRING ServiceName,
    IN PCM_RESOURCE_LIST ResourceList
    )

/*++

Routine Description:

    This routine determines if the reported device instance is already reported
    or not.

Parameters:

    Handle - Supplies a handle to the reported device instance key.

    ServiceName - supplies a pointer to a the unicode service key name.

    ResourceList - supplies a pointer to the reported Resource list.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PKEY_VALUE_FULL_INFORMATION keyValueInfo1 = NULL, keyValueInfo2 = NULL;
    NTSTATUS status;
    UNICODE_STRING unicodeName;
    HANDLE logConfHandle, controlHandle = NULL;
    BOOLEAN returnValue = FALSE;
    PCM_RESOURCE_LIST cmResource = NULL;
    ULONG tmpValue;

    PAGED_CODE();

    //
    // If this registry key is for a device reported during the same boot
    // this is not a duplicate.
    //

    RtlInitUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
    status = IopOpenRegistryKeyEx( &controlHandle,
                                   Handle,
                                   &unicodeName,
                                   KEY_ALL_ACCESS
                                   );
    if (NT_SUCCESS(status)) {
        status = IopGetRegistryValue(controlHandle,
                                     REGSTR_VALUE_DEVICE_REPORTED,
                                     &keyValueInfo1);
        if (NT_SUCCESS(status)) {
            goto exit;
        }

        //
        // Check if "Service" value matches what the caller passed in.
        //

        status = IopGetRegistryValue(Handle, REGSTR_VALUE_SERVICE, &keyValueInfo1);
        if (NT_SUCCESS(status)) {
            if ((keyValueInfo1->Type == REG_SZ) &&
                (keyValueInfo1->DataLength != 0)) {
                unicodeName.Buffer = (PWSTR)KEY_VALUE_DATA(keyValueInfo1);
                unicodeName.MaximumLength = unicodeName.Length = (USHORT)keyValueInfo1->DataLength;
                if (unicodeName.Buffer[(keyValueInfo1->DataLength / sizeof(WCHAR)) - 1] == UNICODE_NULL) {
                    unicodeName.Length -= sizeof(WCHAR);
                }
                if (RtlEqualUnicodeString(ServiceName, &unicodeName, TRUE)) {

                    //
                    // Next check if resources are the same
                    //

                    RtlInitUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
                    status = IopOpenRegistryKeyEx( &logConfHandle,
                                                   Handle,
                                                   &unicodeName,
                                                   KEY_READ
                                                   );
                    if (NT_SUCCESS(status)) {
                        status = IopGetRegistryValue(logConfHandle,
                                                     REGSTR_VAL_BOOTCONFIG,
                                                     &keyValueInfo2);
                        ZwClose(logConfHandle);
                        if (NT_SUCCESS(status)) {
                            if ((keyValueInfo2->Type == REG_RESOURCE_LIST) &&
                                (keyValueInfo2->DataLength != 0)) {
                                cmResource = (PCM_RESOURCE_LIST)KEY_VALUE_DATA(keyValueInfo2);
                                if (ResourceList && cmResource &&
                                    IopIsDuplicatedDevices(ResourceList, cmResource, NULL, NULL)) {
                                    returnValue = TRUE;
                                }
                            }
                        }
                    }
                    if (!ResourceList && !cmResource) {
                        returnValue = TRUE;
                    }
                }
            }
        }
        if (returnValue == TRUE) {

            //
            // Mark this key has been used.
            //

            PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_DEVICE_REPORTED);
            tmpValue = 1;
            status = ZwSetValueKey(controlHandle,
                                   &unicodeName,
                                   TITLE_INDEX_VALUE,
                                   REG_DWORD,
                                   &tmpValue,
                                   sizeof(ULONG)
                                   );
            if (!NT_SUCCESS(status)) {
                returnValue = FALSE;
            }
        }
    }

exit:
    if (controlHandle) {
        ZwClose(controlHandle);
    }

    if (keyValueInfo1) {
        ExFreePool(keyValueInfo1);
    }
    if (keyValueInfo2) {
        ExFreePool(keyValueInfo2);
    }
    return returnValue;
}


NTSTATUS
IopAllocateBuffer(
    IN PBUFFER_INFO Info,
    IN ULONG Size
    )

/*++

Routine Description:

    Allocates a buffer of Size bytes and initialises the BUFFER_INFO
    structure so the current position is at the start of the buffer.

Parameters:

    Info - Pointer to a buffer info structure to be used to manage the new
           buffer

    Size - The number of bytes to be allocated for the buffer

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    ASSERT(Info);

    if (!(Info->Buffer = ExAllocatePool(PagedPool, Size))) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Info->Current = Info->Buffer;
    Info->MaxSize = Size;

    return STATUS_SUCCESS;
}


NTSTATUS
IopResizeBuffer(
    IN PBUFFER_INFO Info,
    IN ULONG NewSize,
    IN BOOLEAN CopyContents
    )

/*++

Routine Description:

    Allocates a new buffer of NewSize bytes and associates it with Info, freeing the
    old buffer.  It will optionally copy the data stored in the old buffer into the
    new buffer and update the current position.

Parameters:

    Info - Pointer to a buffer info structure to be used to manage the buffer

    NewSize - The new size of the buffer in bytes

    CopyContents - If TRUE indicates that the contents of the old buffer should be
                   copied to the new buffer

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    ULONG used;
    PCHAR newBuffer;

    ASSERT(Info);

    used = (ULONG)(Info->Current - Info->Buffer);

    if (!(newBuffer = ExAllocatePool(PagedPool, NewSize))) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (CopyContents) {

        //
        // Assert there is room in the buffer
        //

        ASSERT(used < NewSize);

        RtlCopyMemory(newBuffer,
                      Info->Buffer,
                      used);

        Info->Current = newBuffer + used;

    } else {

        Info->Current = newBuffer;
    }

    ExFreePool(Info->Buffer);

    Info->Buffer = newBuffer;
    Info->MaxSize = NewSize;

    return STATUS_SUCCESS;
}

VOID
IopFreeBuffer(
    IN PBUFFER_INFO Info
    )

/*++

Routine Description:

    Frees the buffer associated with Info and resets all Info fields

Parameters:

    Info - Pointer to a buffer info structure to be used to manage the buffer

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    ASSERT(Info);

    //
    // Free the buffer
    //

    ExFreePool(Info->Buffer);

    //
    // Zero out the info parameters so we can't accidently used the free buffer
    //

    Info->Buffer = NULL;
    Info->Current = NULL;
    Info->MaxSize = 0;
}

NTSTATUS
IopAppendBuffer(
    IN PBUFFER_INFO Info,
    IN PVOID Data,
    IN ULONG DataSize
    )

/*++

Routine Description:

    Copies the data to the end of the buffer, resizing if necessary.  The current
    position is set to the end of the data just added.

Parameters:

    Info - Pointer to a buffer info structure to be used to manage the buffer

    Data - Pointer to the data to be added to the buffer

    DataSize - The size of the data pointed to by Data in bytes

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{

    NTSTATUS status = STATUS_SUCCESS;
    ULONG free, used;

    ASSERT(Info);

    used = (ULONG)(Info->Current - Info->Buffer);
    free = Info->MaxSize - used;

    if (free < DataSize) {
        status = IopResizeBuffer(Info, used + DataSize, TRUE);

        if (!NT_SUCCESS(status)) {
            goto clean0;
        }

    }

    //
    // Copy the data into the buffer
    //

    RtlCopyMemory(Info->Current,
                  Data,
                  DataSize);

    //
    // Advance down the buffer
    //

    Info->Current += DataSize;

clean0:
    return status;

}

NTSTATUS
IopOverwriteBuffer(
    IN PBUFFER_INFO Info,
    IN PVOID Data,
    IN ULONG DataSize
    )

/*++

Routine Description:

    Copies data into the buffer, overwriting what is currently present,
    resising if necessary.  The current position is set to the end of the
    data just added.

Parameters:

    Info - Pointer to a buffer info structure to be used to manage the buffer

    Data - Pointer to the data to be added to the buffer

    DataSize - The size of the data pointed to by Data in bytes

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG free;

    ASSERT(Info);

    free = Info->MaxSize;


    if (free < DataSize) {
        status = IopResizeBuffer(Info, DataSize, FALSE);

        if (!NT_SUCCESS(status)) {
            goto clean0;
        }

    }

    //
    // Copy the data into the buffer
    //

    RtlCopyMemory(Info->Buffer,
                  Data,
                  DataSize);

    //
    // Advance down the buffer
    //

    Info->Current += DataSize;

clean0:
    return status;
}

#define DBG_GET_ASSOC 0

NTSTATUS
IopGetDeviceInterfaces(
        IN CONST GUID *InterfaceClassGuid,
        IN PUNICODE_STRING DevicePath   OPTIONAL,
        IN ULONG Flags,
        IN BOOLEAN UserModeFormat,
        OUT PWSTR *SymbolicLinkList,
        OUT PULONG SymbolicLinkListSize OPTIONAL
        )

/*++

Routine Description:

    This API allows a WDM driver to get a list of paths that represent all
    devices registered for the specified interface class.

Parameters:

    InterfaceClassGuid - Supplies a pointer to a GUID representing the interface class
        for whom a list of members is to be retrieved

    DevicePath - Optionally, supplies a pointer to a unicode string containing the
        enumeration path for a device for whom interfaces of the specified class are
        to be re-trieved.  If this parameter  is not supplied, then all interface
        devices (regardless of what physical device exposes them) will be returned.

    Flags - Supplies flags that modify the behavior of list retrieval.
        The following flags are presently defined:

        DEVICE_INTERFACE_INCLUDE_NONACTIVE -- If this flag is specified, then all
            interface devices, whether currently active or not, will be returned
            (potentially filtered based on the Physi-calDeviceObject, if specified).

    UserModeFormat - If TRUE the multi-sz returned will have user mode prefixes
        (\\?\) otherwise they will have kernel mode prefixes (\??\).

    SymbolicLinkList - Supplies the address of a character pointer, that on
        success will contain a multi-sz list of \??\ symbolic link
        names that provide the requested functionality.  The caller is
        responsible for freeing the memory via ExFreePool.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    UNICODE_STRING guidString, tempString, defaultString, symLinkString, devnodeString;
    BUFFER_INFO returnBuffer, infoBuffer, symLinkBuffer, devnodeNameBuffer;
    PKEY_VALUE_FULL_INFORMATION pDefaultInfo;
    ULONG tempLong, keyIndex, instanceKeyIndex, resultSize;
    HANDLE hDeviceClasses, hClass, hKey, hInstanceKey, hControl;
    BOOLEAN defaultPresent = FALSE;

    PAGED_CODE();

    //
    // Initialise out parameters
    //

    *SymbolicLinkList = NULL;

    //
    // Convert the GUID into a string
    //

    status = RtlStringFromGUID(InterfaceClassGuid, &guidString);
    if(!NT_SUCCESS(status)) {
        goto finalClean;
    }

#if DBG_GET_ASSOC
    DbgPrint("Getting associations for class %wZ\n", &guidString);
#endif

    //
    // Allocate initial buffers
    //

    status = IopAllocateBuffer(&returnBuffer,
                               INITIAL_RETURN_BUFFER_SIZE
                               );

    if (!NT_SUCCESS(status)) {
        goto clean0;
    }

    status = IopAllocateBuffer(&infoBuffer,
                               INITIAL_INFO_BUFFER_SIZE
                               );

    if (!NT_SUCCESS(status)) {
        goto clean1;
    }

    status = IopAllocateBuffer(&symLinkBuffer,
                               INITIAL_SYMLINK_BUFFER_SIZE
                               );

    if (!NT_SUCCESS(status)) {
        goto clean2;
    }

    status = IopAllocateBuffer(&devnodeNameBuffer,
                               INITIAL_DEVNODE_NAME_BUFFER_SIZE
                               );

    if (!NT_SUCCESS(status)) {
        goto clean2a;
    }

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&PpRegistryDeviceResource, TRUE);

    //
    // Open HKLM\System\CurrentControlSet\Control\DeviceClasses key
    //

    PiWstrToUnicodeString(&tempString, REGSTR_FULL_PATH_DEVICE_CLASSES);
    status = IopCreateRegistryKeyEx( &hDeviceClasses,
                                     NULL,
                                     &tempString,
                                     KEY_ALL_ACCESS,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );

    if (!NT_SUCCESS(status)) {
        goto clean3;
    }

    //
    // Open function class GUID key
    //

    status = IopOpenRegistryKeyEx( &hClass,
                                   hDeviceClasses,
                                   &guidString,
                                   KEY_ALL_ACCESS
                                   );
    ZwClose(hDeviceClasses);

    if(status == STATUS_OBJECT_NAME_NOT_FOUND || status == STATUS_OBJECT_PATH_NOT_FOUND) {

        //
        // The path does not exist - return a single null character buffer
        //

        status = STATUS_SUCCESS;
        goto clean5;
    } else if (!NT_SUCCESS(status)) {
        goto clean3;
    }

    //
    // Get the default value if it exists
    //

    status = IopGetRegistryValue(hClass,
                                 REGSTR_VAL_DEFAULT,
                                 &pDefaultInfo
                                 );


    if (NT_SUCCESS(status)
        && pDefaultInfo->Type == REG_SZ
        && pDefaultInfo->DataLength >= sizeof(WCHAR)) {

        //
        // We have a default - construct a counted string from the default
        //

        defaultPresent = TRUE;
        defaultString.Buffer = (PWSTR) KEY_VALUE_DATA(pDefaultInfo);
        defaultString.Length = (USHORT) pDefaultInfo->DataLength - sizeof(UNICODE_NULL);
        defaultString.MaximumLength = defaultString.Length;

#if DBG_GET_ASSOC
        DbgPrint("Class default: %wZ\n", &defaultString);
#endif

        //
        // Open the device interface instance key for the default name.
        //
        status = IopOpenOrCreateDeviceInterfaceSubKeys(NULL,
                                                       NULL,
                                                       &hKey,
                                                       NULL,
                                                       hClass,
                                                       &defaultString,
                                                       KEY_READ,
                                                       FALSE
                                                      );

        if (!NT_SUCCESS(status)) {
            defaultPresent = FALSE;
            ExFreePool(pDefaultInfo);
            //
            // Continue with the call but ignore the invalid default entry
            //
#if DBG_GET_ASSOC
            DbgPrint("WDM Warning: Default entry for class %zW is invalid\n", &guidString);
#endif
        } else {

            //
            // If we are just supposed to return live interfaces, then make sure this default
            // interface is linked.
            //

            if (!(Flags & DEVICE_INTERFACE_INCLUDE_NONACTIVE)) {

                defaultPresent = FALSE;

                //
                // Open the control subkey
                //

                PiWstrToUnicodeString(&tempString, REGSTR_KEY_CONTROL);
                status = IopOpenRegistryKeyEx( &hControl,
                                               hKey,
                                               &tempString,
                                               KEY_ALL_ACCESS
                                               );

                if (NT_SUCCESS(status)) {
                    //
                    // Get the linked value
                    //

                    PiWstrToUnicodeString(&tempString, REGSTR_VAL_LINKED);
                    status = ZwQueryValueKey(hControl,
                                             &tempString,
                                             KeyValuePartialInformation,
                                             (PVOID) infoBuffer.Buffer,
                                             infoBuffer.MaxSize,
                                             &resultSize
                                             );

                    ZwClose(hControl);

                    //
                    // We don't need to check the buffer was big enough because it starts
                    // off that way and doesn't get any smaller!
                    //

                    if (NT_SUCCESS(status)
                        && (((PKEY_VALUE_PARTIAL_INFORMATION)(infoBuffer.Buffer))->Type == REG_DWORD)
                        && (((PKEY_VALUE_PARTIAL_INFORMATION)(infoBuffer.Buffer))->DataLength == sizeof(ULONG))) {

                        defaultPresent = *(PULONG)(((PKEY_VALUE_PARTIAL_INFORMATION)(infoBuffer.Buffer))->Data)
                                       ? TRUE
                                       : FALSE;
                    }
                }
            }

            ZwClose(hKey);

            if(defaultPresent) {
                //
                // Add the default as the first entry in the return buffer and patch to usermode if necessary
                //
                status = IopAppendBuffer(&returnBuffer,
                                         defaultString.Buffer,
                                         defaultString.Length + sizeof(UNICODE_NULL)
                                        );

                if (!UserModeFormat) {

                    RtlCopyMemory(returnBuffer.Buffer,
                                  KERNEL_SYMLINK_STRING_PREFIX,
                                  KERNEL_SYMLINK_STRING_PREFIX_LENGTH
                                  );
                }

            } else {
                //
                // The default device interface isn't active--free the memory for the name buffer now.
                //
                ExFreePool(pDefaultInfo);
            }
        }

    } else if (status == STATUS_OBJECT_NAME_NOT_FOUND || status == STATUS_OBJECT_PATH_NOT_FOUND) {
        //
        // Do nothing - there is no default
        //
    } else {
        //
        // An unexpected error occured - clean up
        //
        if (NT_SUCCESS(status)) {

            ExFreePool(pDefaultInfo);
            status = STATUS_UNSUCCESSFUL;
        }

        ZwClose(hClass);
        goto clean4;
    }

    //
    // Iterate through the subkeys under this interface class key.
    //

    keyIndex = 0;

    while((status = ZwEnumerateKey(hClass,
                                   keyIndex,
                                   KeyBasicInformation,
                                   (PVOID) infoBuffer.Buffer,
                                   infoBuffer.MaxSize,
                                   &resultSize
                                   )) != STATUS_NO_MORE_ENTRIES)
    {

        if (status == STATUS_BUFFER_TOO_SMALL) {

            status = IopResizeBuffer(&infoBuffer, resultSize, FALSE);

            continue;

        } else if (!NT_SUCCESS(status)) {
            ZwClose(hClass);
            goto clean4;
        }

        //
        // Open up this interface key.
        //
        tempString.Length = (USHORT) ((PKEY_BASIC_INFORMATION)(infoBuffer.Buffer))->NameLength;
        tempString.MaximumLength = tempString.Length;
        tempString.Buffer = ((PKEY_BASIC_INFORMATION)(infoBuffer.Buffer))->Name;

#if DBG_GET_ASSOC
        DbgPrint("Key %u enumerated %wZ\n", keyIndex, &tempString);
#endif

        //
        // Open the associated key
        //

        status = IopOpenRegistryKeyEx( &hKey,
                                       hClass,
                                       &tempString,
                                       KEY_READ
                                       );

        if (!NT_SUCCESS(status)) {
            //
            // For some reason we couldn't open this key--skip it and move on.
            //
#if DBG_GET_ASSOC
            DbgPrint("\tCouldn't open interface key!\n");
#endif
            keyIndex++;
            continue;
        }

        //
        // If we're filtering on a particular PDO, then retrieve the owning device
        // instance name for this interface key, and make sure they match.
        //
        PiWstrToUnicodeString(&tempString, REGSTR_VAL_DEVICE_INSTANCE);
        while ((status = ZwQueryValueKey(hKey,
                                         &tempString,
                                         KeyValuePartialInformation,
                                         devnodeNameBuffer.Buffer,
                                         devnodeNameBuffer.MaxSize,
                                         &resultSize
                                         )) == STATUS_BUFFER_TOO_SMALL ) {

            status = IopResizeBuffer(&devnodeNameBuffer, resultSize, FALSE);

            if (!NT_SUCCESS(status)) {
                ZwClose(hKey);
                ZwClose(hClass);
                goto clean4;
            }
        }

        if (!(NT_SUCCESS(status)
              && ((PKEY_VALUE_PARTIAL_INFORMATION)(devnodeNameBuffer.Buffer))->Type == REG_SZ
              && ((PKEY_VALUE_PARTIAL_INFORMATION)(devnodeNameBuffer.Buffer))->DataLength > sizeof(WCHAR) ) ) {
#if DBG_GET_ASSOC
            DbgPrint("\tDevice instance entry corrupt\n");
#endif
            goto CloseInterfaceKeyAndContinue;
        }

        //
        // Build counted string
        //

        devnodeString.Length = (USHORT) ((PKEY_VALUE_PARTIAL_INFORMATION)(devnodeNameBuffer.Buffer))->DataLength - sizeof(UNICODE_NULL);
        devnodeString.MaximumLength = tempString.Length;
        devnodeString.Buffer = (PWSTR) ((PKEY_VALUE_PARTIAL_INFORMATION)(devnodeNameBuffer.Buffer))->Data;

        //
        // Enumerate each interface instance subkey under this PDO's interface key.
        //
        instanceKeyIndex = 0;

        while((status = ZwEnumerateKey(hKey,
                                       instanceKeyIndex,
                                       KeyBasicInformation,
                                       (PVOID) infoBuffer.Buffer,
                                       infoBuffer.MaxSize,
                                       &resultSize
                                       )) != STATUS_NO_MORE_ENTRIES)
        {

            if (status == STATUS_BUFFER_TOO_SMALL) {

                status = IopResizeBuffer(&infoBuffer, resultSize, FALSE);

                continue;

            } else if (!NT_SUCCESS(status)) {
                ZwClose(hKey);
                ZwClose(hClass);
                goto clean4;
            }

            //
            // Open up this interface instance key.
            //
            tempString.Length = (USHORT) ((PKEY_BASIC_INFORMATION)(infoBuffer.Buffer))->NameLength;
            tempString.MaximumLength = tempString.Length;
            tempString.Buffer = ((PKEY_BASIC_INFORMATION)(infoBuffer.Buffer))->Name;

#if DBG_GET_ASSOC
            DbgPrint("Instance key %u enumerated %wZ\n", instanceKeyIndex, &tempString);
#endif

            //
            // Open the associated key
            //

            status = IopOpenRegistryKeyEx( &hInstanceKey,
                                           hKey,
                                           &tempString,
                                           KEY_READ
                                           );

            if (!NT_SUCCESS(status)) {
                //
                // For some reason we couldn't open this key--skip it and move on.
                //
#if DBG_GET_ASSOC
                DbgPrint("\tCouldn't open interface key!\n");
#endif
                instanceKeyIndex++;
                continue;
            }

            if (!(Flags & DEVICE_INTERFACE_INCLUDE_NONACTIVE)) {

                //
                // Open the control subkey
                //

                PiWstrToUnicodeString(&tempString, REGSTR_KEY_CONTROL);
                status = IopOpenRegistryKeyEx( &hControl,
                                               hInstanceKey,
                                               &tempString,
                                               KEY_READ
                                               );

                if (!NT_SUCCESS(status)) {

                    //
                    // We have no control subkey so can't be linked -
                    // continue enumerating the keys ignoring this one
                    //

#if DBG_GET_ASSOC
                    DbgPrint("\tNo control subkey\n");
#endif
                    goto CloseInterfaceInstanceKeyAndContinue;
                }

                //
                // Get the linked value
                //

                PiWstrToUnicodeString(&tempString, REGSTR_VAL_LINKED);
                status = ZwQueryValueKey(hControl,
                                         &tempString,
                                         KeyValuePartialInformation,
                                         (PVOID) infoBuffer.Buffer,
                                         infoBuffer.MaxSize,
                                         &resultSize
                                         );

                ZwClose(hControl);

                //
                // We don't need to check the buffer was big enough because it starts
                // off that way and doesn't get any smaller!
                //

                if (!NT_SUCCESS(status)
                    || (((PKEY_VALUE_PARTIAL_INFORMATION)(infoBuffer.Buffer))->Type != REG_DWORD)
                    || (((PKEY_VALUE_PARTIAL_INFORMATION)(infoBuffer.Buffer))->DataLength != sizeof(ULONG))
                    || !*(PULONG)(((PKEY_VALUE_PARTIAL_INFORMATION)(infoBuffer.Buffer))->Data)) {

                    //
                    // We are NOT linked so continue enumerating the keys ignoring this one
                    //

#if DBG_GET_ASSOC
                    DbgPrint("\tNot Linked\n");
#endif
                    goto CloseInterfaceInstanceKeyAndContinue;
                }
            }

            //
            // Open the "SymbolicLink" value and place the information into the symLink buffer
            //

            PiWstrToUnicodeString(&tempString, REGSTR_VAL_SYMBOLIC_LINK);
            while ((status = ZwQueryValueKey(hInstanceKey,
                                             &tempString,
                                             KeyValuePartialInformation,
                                             symLinkBuffer.Buffer,
                                             symLinkBuffer.MaxSize,
                                             &resultSize
                                             )) == STATUS_BUFFER_TOO_SMALL ) {

                status = IopResizeBuffer(&symLinkBuffer, resultSize, FALSE);

                if (!NT_SUCCESS(status)) {
                    ZwClose(hInstanceKey);
                    ZwClose(hKey);
                    ZwClose(hClass);
                    goto clean4;
                }
            }

            if (!(NT_SUCCESS(status)
                && ((PKEY_VALUE_PARTIAL_INFORMATION)(symLinkBuffer.Buffer))->Type == REG_SZ
                && ((PKEY_VALUE_PARTIAL_INFORMATION)(symLinkBuffer.Buffer))->DataLength > sizeof(WCHAR) ) ) {
#if DBG_GET_ASSOC
                DbgPrint("\tSymbolic link entry corrupt\n");
#endif
                goto CloseInterfaceInstanceKeyAndContinue;
            }

            //
            // Build counted string from value data
            //

            symLinkString.Length = (USHORT) ((PKEY_VALUE_PARTIAL_INFORMATION)(symLinkBuffer.Buffer))->DataLength - sizeof(UNICODE_NULL);
            symLinkString.MaximumLength = symLinkString.Length;
            symLinkString.Buffer = (PWSTR) ((PKEY_VALUE_PARTIAL_INFORMATION)(symLinkBuffer.Buffer))->Data;

            //
            // If we have a default, check this is not it
            //

            if (defaultPresent) {

                if (RtlCompareUnicodeString(&defaultString, &symLinkString, TRUE) == 0) {

                    //
                    // We have already added the default to the beginning of the buffer so skip it
                    //

#if DBG_GET_ASSOC
                    DbgPrint("\tDefault entry skipped\n");
#endif
                    goto CloseInterfaceInstanceKeyAndContinue;
                }
            }

            //
            // If we are only returning interfaces for a particular PDO then check
            // this is from that PDO
            //
            if (ARGUMENT_PRESENT(DevicePath)) {
                //
                // Check if it is from the same PDO
                //
                if (RtlCompareUnicodeString(DevicePath, &devnodeString, TRUE) != 0) {
                    //
                    // If not then go onto the next key
                    //
#if DBG_GET_ASSOC
                    DbgPrint("\tNot from the correct PDO\n");
#endif
                    goto CloseInterfaceInstanceKeyAndContinue;
                }
            }

            //
            // Copy the symLink string to the return buffer including the NULL termination
            //

            status = IopAppendBuffer(&returnBuffer,
                                     symLinkString.Buffer,
                                     symLinkString.Length + sizeof(UNICODE_NULL)
                                     );

            ASSERT(((PWSTR) returnBuffer.Current)[-1] == UNICODE_NULL);

#if DBG_GET_ASSOC
            DbgPrint("\tAdded to return buffer\n");
#endif

            //
            // If we are returning KM strings then patch the prefix
            //

            if (!UserModeFormat) {

                RtlCopyMemory(returnBuffer.Current - (symLinkString.Length + sizeof(UNICODE_NULL)),
                              KERNEL_SYMLINK_STRING_PREFIX,
                              KERNEL_SYMLINK_STRING_PREFIX_LENGTH
                              );
            }

CloseInterfaceInstanceKeyAndContinue:
            ZwClose(hInstanceKey);
            instanceKeyIndex++;
        }

CloseInterfaceKeyAndContinue:
        ZwClose(hKey);
        keyIndex++;
    }

    ZwClose(hClass);

clean5:
    //
    // We've got then all!  Resize to leave space for a terminating NULL.
    //

    status = IopResizeBuffer(&returnBuffer,
                             (ULONG) (returnBuffer.Current - returnBuffer.Buffer + sizeof(UNICODE_NULL)),
                             TRUE
                             );

    if (NT_SUCCESS(status)) {

        //
        // Terminate the buffer
        //
        *((PWSTR) returnBuffer.Current) = UNICODE_NULL;
    }

clean4:
    if (defaultPresent) {
        ExFreePool(pDefaultInfo);
    }

clean3:
    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();
    IopFreeBuffer(&devnodeNameBuffer);

clean2a:
    IopFreeBuffer(&symLinkBuffer);

clean2:
    IopFreeBuffer(&infoBuffer);

clean1:
    if (!NT_SUCCESS(status)) {
        IopFreeBuffer(&returnBuffer);
    }

clean0:
    RtlFreeUnicodeString(&guidString);

finalClean:
    if (NT_SUCCESS(status)) {

        *SymbolicLinkList = (PWSTR) returnBuffer.Buffer;

        if (ARGUMENT_PRESENT(SymbolicLinkListSize)) {
            *SymbolicLinkListSize = returnBuffer.MaxSize;
        }

    } else {

        *SymbolicLinkList = NULL;

        if (ARGUMENT_PRESENT(SymbolicLinkListSize)) {
            *SymbolicLinkListSize = 0;
        }

    }

    return status;
}

NTSTATUS
IoGetDeviceInterfaces(
    IN CONST GUID *InterfaceClassGuid,
    IN PDEVICE_OBJECT PhysicalDeviceObject OPTIONAL,
    IN ULONG Flags,
    OUT PWSTR *SymbolicLinkList
    )

/*++

Routine Description:

    This API allows a WDM driver to get a list of paths that represent all
    device interfaces registered for the specified interface class.

Parameters:

    InterfaceClassGuid - Supplies a pointer to a GUID representing the interface class
        for whom a list of members is to be retrieved

    PhysicalDeviceObject - Optionally, supplies a pointer to the PDO for whom
        interfaces of the specified class are to be re-trieved.  If this parameter
        is not supplied, then all interface devices (regardless of what physical
        device exposes them) will be returned.

    Flags - Supplies flags that modify the behavior of list retrieval.
        The following flags are presently defined:

        DEVICE_INTERFACE_INCLUDE_NONACTIVE -- If this flag is specified, then all
            device interfaces, whether currently active or not, will be returned
            (potentially filtered based on the PhysicalDeviceObject, if specified).

    SymbolicLinkList - Supplies the address of a character pointer, that on
        success will contain a multi-sz list of \DosDevices\ symbolic link
        names that provide the requested functionality.  The caller is
        responsible for freeing the memory via ExFreePool

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    PUNICODE_STRING pDeviceName = NULL;
    PDEVICE_NODE pDeviceNode;

    PAGED_CODE();

    //
    // Check we have a PDO and if so extract the instance path from it
    //

    if (ARGUMENT_PRESENT(PhysicalDeviceObject)) {

        ASSERT_PDO(PhysicalDeviceObject);
        pDeviceNode = (PDEVICE_NODE) PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;
        pDeviceName = &pDeviceNode->InstancePath;
    }

    status = IopGetDeviceInterfaces(InterfaceClassGuid,
                                    pDeviceName,
                                    Flags,
                                    FALSE,
                                    SymbolicLinkList,
                                    NULL
                                    );
    return status;
}


NTSTATUS
IopRealloc(
    IN OUT PVOID *Buffer,
    IN ULONG OldSize,
    IN ULONG NewSize
)

/*++

Routine Description:

    This implements a variation of the traditional C realloc routine.

Parameters:

    Buffer - Supplies a pointer to a pointer to the buffer that is being
        reallocated.  On sucessful completion it the pointer will be updated
        to point to the new buffer, on failure it will still point to the old
        buffer.

    OldSize - The size in bytes of the memory block referenced by Buffer

    NewSize - The desired new size in bytes of the buffer.  This can be larger
        or smaller than the OldSize

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{

    PVOID newBuffer;

    PAGED_CODE();

    ASSERT(*Buffer);

    //
    // Allocate a new buffer
    //

    if (!(newBuffer = ExAllocatePool(PagedPool, NewSize))) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy the contents of the old buffer
    //

    if(OldSize <= NewSize) {
        RtlCopyMemory(newBuffer, *Buffer , OldSize);
    } else {
        RtlCopyMemory(newBuffer, *Buffer , NewSize);
    }
    //
    // Free up the old buffer
    //

    ExFreePool(*Buffer);

    //
    // Hand the new buffer back to the caller
    //

    *Buffer = newBuffer;

    return STATUS_SUCCESS;

}

NTSTATUS
IoSetDeviceInterfaceState(
    IN PUNICODE_STRING SymbolicLinkName,
    IN BOOLEAN Enable
    )

/*++

Routine Description:

    This DDI allows a device class to activate and deactivate an association
    previously registered using IoRegisterDeviceInterface

Parameters:

    SymbolicLinkName - Supplies a pointer to the symbolic link name which was
        returned by IoRegisterDeviceInterface when the interface was registered,
        or as returned by IoGetDeviceInterfaces.

    Enable - If TRUE (non-zero), the interface will be enabled.  If FALSE, it
        will be disabled.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;

    PAGED_CODE();

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&PpRegistryDeviceResource, TRUE);

    status = IopProcessSetInterfaceState(SymbolicLinkName, Enable, TRUE);

    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    if (!NT_SUCCESS(status) && !Enable) {
        //
        // If we failed to disable an interface (most likely because the
        // interface keys have already been deleted) report success.
        //
        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
IoOpenDeviceInterfaceRegistryKey(
    IN PUNICODE_STRING SymbolicLinkName,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE DeviceInterfaceKey
    )

/*++

Routine Description:

    This routine will open the registry key where the data associated with a
    specific device interface can be stored.

Parameters:

    SymbolicLinkName - Supplies a pointer to the symbolic link name which was
        returned by IoRegisterDeviceInterface when the device class was registered.

    DesiredAccess - Supplies the access privileges to the key the caller wants.

    DeviceInterfaceKey - Supplies a pointer to a handle which on success will
        contain the handle to the requested registry key.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    HANDLE hKey;
    UNICODE_STRING unicodeString;

    PAGED_CODE();

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&PpRegistryDeviceResource, TRUE);

    //
    // Open the interface device key
    //

    status = IopDeviceInterfaceKeysFromSymbolicLink(SymbolicLinkName,
                                                    KEY_READ,
                                                    NULL,
                                                    NULL,
                                                    &hKey
                                                    );
    if(!NT_SUCCESS(status)) {
        goto clean0;
    }

    //
    // Open the "Device Parameters" subkey.
    //

    PiWstrToUnicodeString(&unicodeString, REGSTR_KEY_DEVICEPARAMETERS);
    status = IopCreateRegistryKeyEx( DeviceInterfaceKey,
                                     hKey,
                                     &unicodeString,
                                     DesiredAccess,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );
    ZwClose(hKey);

clean0:
    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    return status;
}

NTSTATUS
IopDeviceInterfaceKeysFromSymbolicLink(
    IN PUNICODE_STRING SymbolicLinkName,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE DeviceInterfaceClassKey    OPTIONAL,
    OUT PHANDLE DeviceInterfaceKey         OPTIONAL,
    OUT PHANDLE DeviceInterfaceInstanceKey OPTIONAL
    )

/*++

Routine Description:

    This routine will open the registry key where the data associated with the
    device pointed to by SymbolicLinkName is stored.  If the path does not exist
    it will not be created.

Parameters:

    SymbolicLinkName - Supplies a pointer to the symbolic link name.

    DesiredAccess - Supplies the access privto the function class instance key the
        caller wants.

    DeviceInterfaceClassKey - Optionally, supplies the address of a variable that
        receives a handle to the device class key for the interface.

    DeviceInterfaceKey - Optionally, supplies the address of a variable that receives
        a handle to the device interface (parent) key.

    DeviceInterfaceInstanceKey - Optionally, Supplies the address of a variable that
        receives a handle to the device interface instance key (i.e., the
        refstring-specific one).

Return Value:

    Status code that indicates whether or not the function was successful.


--*/

{
    NTSTATUS status;
    UNICODE_STRING guidString, tempString;
    HANDLE hDeviceClasses, hFunctionClass;

    PAGED_CODE();

    //
    // Get guid from symbolic link name
    //
    status = IopParseSymbolicLinkName(SymbolicLinkName, NULL, NULL, &guidString, NULL, NULL, NULL);

    if(!NT_SUCCESS(status)){
        goto clean0;
    }

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&PpRegistryDeviceResource, TRUE);

    //
    // Open HKLM\System\CurrentControlSet\Control\DeviceClasses key
    //

    PiWstrToUnicodeString(&tempString, REGSTR_FULL_PATH_DEVICE_CLASSES);
    status = IopOpenRegistryKeyEx( &hDeviceClasses,
                                   NULL,
                                   &tempString,
                                   KEY_READ
                                   );

    if( !NT_SUCCESS(status) ){
        goto clean1;
    }

    //
    // Open function class GUID key
    //

    status = IopOpenRegistryKeyEx( &hFunctionClass,
                                   hDeviceClasses,
                                   &guidString,
                                   KEY_READ
                                   );

    if( !NT_SUCCESS(status) ){
        goto clean2;
    }

    //
    // Open device interface instance key
    //
    status = IopOpenOrCreateDeviceInterfaceSubKeys(DeviceInterfaceKey,
                                                   NULL,
                                                   DeviceInterfaceInstanceKey,
                                                   NULL,
                                                   hFunctionClass,
                                                   SymbolicLinkName,
                                                   DesiredAccess,
                                                   FALSE
                                                  );

    if((!NT_SUCCESS(status)) || (!ARGUMENT_PRESENT(DeviceInterfaceClassKey))) {
        ZwClose(hFunctionClass);
    } else {
        *DeviceInterfaceClassKey = hFunctionClass;
    }

clean2:
    ZwClose(hDeviceClasses);
clean1:
    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();
clean0:
    return status;

}

NTSTATUS
IoRegisterDeviceInterface(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN CONST GUID *InterfaceClassGuid,
    IN PUNICODE_STRING ReferenceString      OPTIONAL,
    OUT PUNICODE_STRING SymbolicLinkName
    )

/*++

Routine Description:

    This device driver interface allows a WDM driver to register a particular
    interface of its underlying hardware (ie PDO) as a member of a function class.

Parameters:

    PhysicalDeviceObject - Supplies a pointer to the PDO for the P&P device
        instance associated with the functionality being registered

    InterfaceClassGuid - Supplies a pointer to the GUID representring the functionality
        to be registered

    ReferenceString - Optionally, supplies an additional context string which is
        appended to the enumeration path of the device

    SymbolicLinkName - Supplies a pointer to a string which on success will contain the
        kernel mode path of the symbolic link used to open this device.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PDEVICE_NODE pDeviceNode;
    PUNICODE_STRING pDeviceString;
    NTSTATUS status;
    PWSTR   pRefString;
    USHORT  count;

    PAGED_CODE();

    //
    // Until PartMgr/Disk stop registering non PDOs allow the system to boot.
    //
    // ASSERT_PDO(PhysicalDeviceObject);
    //

    //
    // Ensure we have a PDO - only PDO's have a device node attached
    //

    pDeviceNode = (PDEVICE_NODE) PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;
    if (pDeviceNode) {

        //
        // Get the instance path string
        //
        pDeviceString = &pDeviceNode->InstancePath;

        if (pDeviceNode->InstancePath.Length == 0) {
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        //
        // Make sure the ReferenceString does not contain any path seperator characters
        //
        if (ReferenceString) {
            pRefString = ReferenceString->Buffer;
            count = ReferenceString->Length / sizeof(WCHAR);
            while (count--) {
                if((*pRefString == SEPERATOR_CHAR) || (*pRefString == ALT_SEPERATOR_CHAR)) {
                    status = STATUS_INVALID_DEVICE_REQUEST;
                    KdPrint(("IoRegisterDeviceInterface: Invalid RefString!! failed with status = %8.8X\n", status));
                    return status;
                }
                pRefString++;
            }
        }

        return IopRegisterDeviceInterface(pDeviceString,
                                          InterfaceClassGuid,
                                          ReferenceString,
                                          FALSE,           // kernel-mode format
                                          SymbolicLinkName
                                          );
    } else {

        return STATUS_INVALID_DEVICE_REQUEST;
    }
}

NTSTATUS
IopRegisterDeviceInterface(
    IN PUNICODE_STRING DeviceInstanceName,
    IN CONST GUID *InterfaceClassGuid,
    IN PUNICODE_STRING ReferenceString      OPTIONAL,
    IN BOOLEAN UserModeFormat,
    OUT PUNICODE_STRING SymbolicLinkName
    )

/*++

Routine Description:

    This is the worker routine for PnpRegisterDeviceInterface.  It is also
    called by the user-mode ConfigMgr (via an NtPlugPlayControl), which is why it
    must take a device instance name instead of a PDO (since the device instance
    may not currently be 'live'), and also why it must optionally return the user-
    mode form of the interface device name (i.e., "\\?\" instead of "\??\").

Parameters:

    DeviceInstanceName - Supplies the name of the device instance for which a
        device interface is being registered.

    InterfaceClassGuid - Supplies a pointer to the GUID representring the class
        of the device interface being registered.

    ReferenceString - Optionally, supplies an additional context string which is
        appended to the enumeration path of the device

    UserModeFormat - If non-zero, then the symbolic link name returned for the
        interface device is in user-mode form (i.e., "\\?\").  If zero (FALSE),
        it is in kernel-mode form (i.e., "\??\").

    SymbolicLinkName - Supplies a pointer to a string which on success will contain
        either the kernel-mode or user-mode path of the symbolic link used to open
        this device.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    UNICODE_STRING tempString, guidString, otherString;
    PUNICODE_STRING pUserString, pKernelString;
    HANDLE hTemp1, hTemp2, hInterfaceInstanceKey;
    ULONG InterfaceDisposition, InterfaceInstanceDisposition;

    PAGED_CODE();

    //
    // Convert the class guid into string form
    //

    status = RtlStringFromGUID(InterfaceClassGuid, &guidString);
    if( !NT_SUCCESS(status) ){
        goto clean0;
    }

    //
    // Construct both flavors of symbolic link name (go ahead and store the form
    // that the user wants in the SymbolicLinkName parameter they supplied--this
    // saves us from having to copy the appropriate string over to their string
    // later).
    //
    if(UserModeFormat) {
        pUserString = SymbolicLinkName;
        pKernelString = &otherString;
    } else {
        pKernelString = SymbolicLinkName;
        pUserString = &otherString;
    }

    status = IopBuildSymbolicLinkStrings(DeviceInstanceName,
                                         &guidString,
                                         ReferenceString,
                                         pUserString,
                                         pKernelString
                                         );
    if (!NT_SUCCESS(status)) {
        goto clean1;
    }

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&PpRegistryDeviceResource, TRUE);

    //
    // Open HKLM\System\CurrentControlSet\Control\DeviceClasses key into hTemp1
    //

    PiWstrToUnicodeString(&tempString, REGSTR_FULL_PATH_DEVICE_CLASSES);
    status = IopCreateRegistryKeyEx( &hTemp1,
                                     NULL,
                                     &tempString,
                                     KEY_CREATE_SUB_KEY,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );

    if( !NT_SUCCESS(status) ){
        goto clean2;
    }

    //
    // Open/create function class GUID key into hTemp2
    //

    status = IopCreateRegistryKeyEx( &hTemp2,
                                     hTemp1,
                                     &guidString,
                                     KEY_CREATE_SUB_KEY,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );
    ZwClose(hTemp1);

    if( !NT_SUCCESS(status) ){
        goto clean2;
    }

    //
    // Now open/create the two-level device interface hierarchy underneath this
    // interface class key.
    //
    status = IopOpenOrCreateDeviceInterfaceSubKeys(&hTemp1,
                                                   &InterfaceDisposition,
                                                   &hInterfaceInstanceKey,
                                                   &InterfaceInstanceDisposition,
                                                   hTemp2,
                                                   pUserString,
                                                   KEY_WRITE | DELETE,
                                                   TRUE
                                                  );

    ZwClose(hTemp2);

    if(!NT_SUCCESS(status)) {
        goto clean2;
    }

    //
    // Create the device instance value under the device interface key
    //

    PiWstrToUnicodeString(&tempString, REGSTR_VAL_DEVICE_INSTANCE);
    status = IopSetRegistryStringValue(hTemp1,
                                       &tempString,
                                       DeviceInstanceName
                                       );
    if(!NT_SUCCESS(status)) {
        goto clean3;
    }

    //
    // Create symbolic link value under interface instance subkey
    //

    PiWstrToUnicodeString(&tempString, REGSTR_VAL_SYMBOLIC_LINK);
    status = IopSetRegistryStringValue(hInterfaceInstanceKey,
                                       &tempString,
                                       pUserString
                                       );

clean3:
    if (!NT_SUCCESS(status)) {
        //
        // Since we failed to register the device interface, delete any keys
        // that were newly created in the attempt.
        //
        if(InterfaceInstanceDisposition == REG_CREATED_NEW_KEY) {
            ZwDeleteKey(hInterfaceInstanceKey);
        } else {
            ZwClose(hInterfaceInstanceKey);
        }

        if(InterfaceDisposition == REG_CREATED_NEW_KEY) {
            ZwDeleteKey(hTemp1);
        } else {
            ZwClose(hTemp1);
        }
    } else {
        ZwClose(hInterfaceInstanceKey);
        ZwClose(hTemp1);
    }

clean2:
    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();
    IopFreeAllocatedUnicodeString(&otherString);
    if (!NT_SUCCESS(status)) {
        IopFreeAllocatedUnicodeString(SymbolicLinkName);
    }

clean1:
    RtlFreeUnicodeString(&guidString);
clean0:
    return status;
}

NTSTATUS
IopUnregisterDeviceInterface(
    IN PUNICODE_STRING SymbolicLinkName
    )

/*++

Routine Description:

    This routine removes the interface instance subkey of
    ReferenceString from the interface for DeviceInstanceName to the
    given InterfaceClassGuid.  If the interface instance specified by
    the Reference String portion of SymbolicLinkName is the only
    instance of the interface, the interface subkey is removed from
    the device class key as well.

Parameters:

    SymbolicLinkName - Supplies a pointer to a unicode string which
        contains the symbolic link name of the device to unregister.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS        status = STATUS_SUCCESS, context;
    HANDLE          hInterfaceClassKey=NULL, hInterfaceKey=NULL,
                    hInterfaceInstanceKey=NULL, hControl=NULL;
    UNICODE_STRING  tempString, mungedPathString, guidString, refString;
    BOOLEAN         refStringPresent;
    GUID            guid;
    UNICODE_STRING  interfaceKeyName, instanceKeyName;
    ULONG           linked, remainingSubKeys;
    USHORT          length;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PKEY_FULL_INFORMATION keyInformation;


    PAGED_CODE();

    //
    // Check that the supplied symbolic link is present, and can be parsed
    //
    if ( (!ARGUMENT_PRESENT(SymbolicLinkName)) ||
         (!NT_SUCCESS(IopParseSymbolicLinkName(SymbolicLinkName,
                                              NULL,
                                              &mungedPathString,
                                              &guidString,
                                              &refString,
                                              &refStringPresent,
                                              &guid))) ) {
        status = STATUS_INVALID_PARAMETER;
        goto clean0;
    }

    //
    // Allocate a unicode string for the interface instance key name.
    // (includes the REFSTRING_PREFIX_CHAR, and ReferenceString, if present)
    //
    length = sizeof(WCHAR) + refString.Length;
    status = IopAllocateUnicodeString(&instanceKeyName,
                                      length);
    if(!NT_SUCCESS(status)) {
        goto clean0;
    }

    //
    // Set the MaximumLength of the Buffer, and append the
    // REFSTRING_PREFIX_CHAR to it.
    //
    *instanceKeyName.Buffer = REFSTRING_PREFIX_CHAR;
    instanceKeyName.Length = sizeof(WCHAR);
    instanceKeyName.MaximumLength = length + sizeof(UNICODE_NULL);

    //
    // Append the ReferenceString to the prefix char, if necessary.
    //
    if (refStringPresent) {
        RtlAppendUnicodeStringToString(&instanceKeyName, &refString);
    }

    instanceKeyName.Buffer[instanceKeyName.Length/sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Allocate a unicode string for the interface key name.
    // (includes KEY_STRING_PREFIX, mungedPathString, separating '#'
    //  char, and the guidString)
    //
    length = IopConstStringSize(KEY_STRING_PREFIX) + mungedPathString.Length +
             sizeof(WCHAR) + guidString.Length;

    status = IopAllocateUnicodeString(&interfaceKeyName,
                                      length);
    if(!NT_SUCCESS(status)) {
        goto clean1;
    }

    interfaceKeyName.MaximumLength = length + sizeof(UNICODE_NULL);

    //
    // Copy the symbolic link name (without refString) to the interfaceKeyNam
    //
    RtlCopyMemory(interfaceKeyName.Buffer, SymbolicLinkName->Buffer, length);
    interfaceKeyName.Length = length;
    interfaceKeyName.Buffer[interfaceKeyName.Length/sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Replace the "\??\" or "\\?\" symbolic link name prefix with "##?#"
    //
    RtlCopyMemory(interfaceKeyName.Buffer,
                  KEY_STRING_PREFIX,
                  IopConstStringSize(KEY_STRING_PREFIX));

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&PpRegistryDeviceResource, TRUE);

    //
    // Get class, interface, and instance handles
    //
    status = IopDeviceInterfaceKeysFromSymbolicLink(SymbolicLinkName,
                                                    KEY_ALL_ACCESS,
                                                    &hInterfaceClassKey,
                                                    &hInterfaceKey,
                                                    &hInterfaceInstanceKey
                                                    );
    if (!NT_SUCCESS(status)) {
        goto clean2;
    }

    //
    // Determine whether this interface is currently "enabled"
    //
    linked = 0;
    PiWstrToUnicodeString(&tempString, REGSTR_KEY_CONTROL);
    status = IopOpenRegistryKeyEx( &hControl,
                                   hInterfaceInstanceKey,
                                   &tempString,
                                   KEY_ALL_ACCESS
                                   );
    if (NT_SUCCESS(status)) {
        //
        // Check the "linked" value under the "Control" subkey of this
        // interface instance
        //
        keyValueInformation=NULL;
        status = IopGetRegistryValue(hControl,
                                     REGSTR_VAL_LINKED,
                                     &keyValueInformation);

        if(NT_SUCCESS(status)) {
            if (keyValueInformation->Type == REG_DWORD &&
                keyValueInformation->DataLength == sizeof(ULONG)) {

                linked = *((PULONG) KEY_VALUE_DATA(keyValueInformation));
                ExFreePool(keyValueInformation);
            }
        }

        ZwClose(hControl);
        hControl = NULL;
    }

    //
    // Ignore any status code returned while attempting to retieve the
    // state of the device.  The value of linked will tell us if we
    // need to disable the interface instance first.
    //
    // If no instance "Control" subkey or "linked" value was present
    //     (status == STATUS_OBJECT_NAME_NOT_FOUND), this interface instance
    //     is not currently enabled -- ok to delete.
    //
    // If the attempt to retrieve these values failed with some other error,
    //     any attempt to disable the interface will also likely fail,
    //     so we'll just have to delete this instance anyways.
    //
    status = STATUS_SUCCESS;

    if (linked) {
        //
        // Disabled the active interface before unregistering it, ignore any
        // status returned, we'll delete this interface instance key anyways.
        //
        IoSetDeviceInterfaceState(SymbolicLinkName, FALSE);
    }

    //
    // Recursively delete the interface instance key, if it exists.
    //
    ZwClose(hInterfaceInstanceKey);
    hInterfaceInstanceKey = NULL;
    IopDeleteKeyRecursive (hInterfaceKey, instanceKeyName.Buffer);

    //
    // Find out how many subkeys to the interface key remain.
    //
    status = IopGetRegistryKeyInformation(hInterfaceKey,
                                          &keyInformation);
    if (!NT_SUCCESS(status)) {
        goto clean3;
    }

    remainingSubKeys = keyInformation->SubKeys;

    ExFreePool(keyInformation);

    //
    // See if a volatile "Control" subkey exists under this interface key
    //
    PiWstrToUnicodeString(&tempString, REGSTR_KEY_CONTROL);
    status = IopOpenRegistryKeyEx( &hControl,
                                   hInterfaceKey,
                                   &tempString,
                                   KEY_READ
                                   );
    if (NT_SUCCESS(status)) {
        ZwClose(hControl);
        hControl = NULL;
    }
    if ((remainingSubKeys==0) ||
        ((remainingSubKeys==1) && (NT_SUCCESS(status)))) {
        //
        // If the interface key has no subkeys, or the only the remaining subkey
        // is the volatile interface "Control" subkey, then there are no more
        // instances to this interface.  We should delete the interface key
        // itself also.
        //
        ZwClose(hInterfaceKey);
        hInterfaceKey = NULL;

        IopDeleteKeyRecursive (hInterfaceClassKey, interfaceKeyName.Buffer);
    }

    status = STATUS_SUCCESS;


clean3:
    if (hControl) {
        ZwClose(hControl);
    }
    if (hInterfaceInstanceKey) {
        ZwClose(hInterfaceInstanceKey);
    }
    if (hInterfaceKey) {
        ZwClose(hInterfaceKey);
    }
    if (hInterfaceClassKey) {
        ZwClose(hInterfaceClassKey);
    }

clean2:
    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    IopFreeAllocatedUnicodeString(&interfaceKeyName);

clean1:
    IopFreeAllocatedUnicodeString(&instanceKeyName);

clean0:
    return status;
}

NTSTATUS
IopRemoveDeviceInterfaces(
    IN PUNICODE_STRING DeviceInstancePath
    )

/*++

Routine Description:

    This routine checks all device class keys under
    HKLM\SYSTEM\CCS\Control\DeviceClasses for interfaces for which the
    DeviceInstance value matches the supplied DeviceInstancePath.  Instances of
    such device interfaces are unregistered, and the device interface subkey
    itself is removed.

    Note that a lock on the registry must have already been acquired,
    by the caller of this routine.

Parameters:

    DeviceInterfacePath - Supplies a pointer to a unicode string which
        contains the DeviceInterface name of the device for which
        interfaces to are to be removed.

Return Value:

    Status code that indicates whether or not the function was
    successful.

--*/

{
    NTSTATUS       status, context;
    HANDLE         hDeviceClasses=NULL, hClassGUID=NULL, hInterface=NULL;
    UNICODE_STRING tempString, guidString, interfaceString, deviceInstanceString;
    ULONG          resultSize, classIndex, interfaceIndex;
    ULONG          symbolicLinkListSize;
    PWCHAR         symbolicLinkList, symLink;
    BUFFER_INFO    classInfoBuffer, interfaceInfoBuffer;
    PKEY_VALUE_FULL_INFORMATION deviceInstanceInfo;
    BOOLEAN        deletedInterface;
    GUID           classGUID;

    PAGED_CODE();

    //
    // Allocate initial buffers
    //
    status = IopAllocateBuffer(&classInfoBuffer,
                               INITIAL_INFO_BUFFER_SIZE);
    if (!NT_SUCCESS(status)) {
        goto clean0;
    }

    status = IopAllocateBuffer(&interfaceInfoBuffer,
                               INITIAL_INFO_BUFFER_SIZE);
    if (!NT_SUCCESS(status)) {
        IopFreeBuffer(&classInfoBuffer);
        goto clean0;
    }

    //
    // Open HKLM\System\CurrentControlSet\Control\DeviceClasses
    //
    PiWstrToUnicodeString(&tempString, REGSTR_FULL_PATH_DEVICE_CLASSES);
    status = IopOpenRegistryKeyEx( &hDeviceClasses,
                                   NULL,
                                   &tempString,
                                   KEY_READ
                                   );
    if(!NT_SUCCESS(status)){
        goto clean1;
    }

    //
    // Enumerate all device classes
    //
    classIndex = 0;

    while((status = ZwEnumerateKey(hDeviceClasses,
                                   classIndex,
                                   KeyBasicInformation,
                                   (PVOID) classInfoBuffer.Buffer,
                                   classInfoBuffer.MaxSize,
                                   &resultSize)) != STATUS_NO_MORE_ENTRIES) {

        if (status == STATUS_BUFFER_TOO_SMALL) {
            status = IopResizeBuffer(&classInfoBuffer, resultSize, FALSE);
            continue;
        } else if (!NT_SUCCESS(status)) {
            goto clean1;
        }

        //
        // Get the key name for this device class
        //
        guidString.Length = (USHORT)((PKEY_BASIC_INFORMATION)(classInfoBuffer.Buffer))->NameLength;
        guidString.MaximumLength = guidString.Length;
        guidString.Buffer = ((PKEY_BASIC_INFORMATION)(classInfoBuffer.Buffer))->Name;

        //
        // Open the key for this device class
        //
        status = IopOpenRegistryKeyEx( &hClassGUID,
                                       hDeviceClasses,
                                       &guidString,
                                       KEY_ALL_ACCESS
                                       );
        if (!NT_SUCCESS(status)) {
            //
            // Couldn't open key for this device class -- skip it and move on.
            //
            goto CloseClassKeyAndContinue;
        }

        //
        // Enumerate all device interfaces for this device class
        //
        interfaceIndex = 0;

        while((status = ZwEnumerateKey(hClassGUID,
                                       interfaceIndex,
                                       KeyBasicInformation,
                                       (PVOID) interfaceInfoBuffer.Buffer,
                                       interfaceInfoBuffer.MaxSize,
                                       &resultSize)) != STATUS_NO_MORE_ENTRIES) {

            if (status == STATUS_BUFFER_TOO_SMALL) {
                status = IopResizeBuffer(&interfaceInfoBuffer, resultSize, FALSE);
                continue;
            } else if (!NT_SUCCESS(status)) {
                goto clean1;
            }

            //
            // This interface key has not yet been deleted
            //
            deletedInterface = FALSE;

            //
            // Create a NULL-terminated unicode string for the interface key name
            //
            status = IopAllocateUnicodeString(&interfaceString,
                                              (USHORT)((PKEY_BASIC_INFORMATION)(interfaceInfoBuffer.Buffer))->NameLength);

            if (!NT_SUCCESS(status)) {
                goto clean1;
            }

            interfaceString.Length = (USHORT)((PKEY_BASIC_INFORMATION)(interfaceInfoBuffer.Buffer))->NameLength;
            interfaceString.MaximumLength = interfaceString.Length + sizeof(UNICODE_NULL);
            RtlCopyMemory(interfaceString.Buffer,
                          ((PKEY_BASIC_INFORMATION)(interfaceInfoBuffer.Buffer))->Name,
                          interfaceString.Length);
            interfaceString.Buffer[interfaceString.Length/sizeof(WCHAR)] = UNICODE_NULL;

            //
            // Open the device interface key
            //
            status = IopOpenRegistryKeyEx( &hInterface,
                                           hClassGUID,
                                           &interfaceString,
                                           KEY_ALL_ACCESS
                                           );
            if (!NT_SUCCESS(status)) {
                //
                // Couldn't open the device interface key -- skip it and move on.
                //
                hInterface = NULL;
                goto CloseInterfaceKeyAndContinue;
            }

            //
            // Get the DeviceInstance value for this interface key
            //
            status = IopGetRegistryValue(hInterface,
                                         REGSTR_VAL_DEVICE_INSTANCE,
                                         &deviceInstanceInfo);

            if(!NT_SUCCESS(status)) {
                //
                //  Couldn't get the DeviceInstance for this interface --
                //  skip it and move on.
                //
                goto CloseInterfaceKeyAndContinue;
            }

            if((deviceInstanceInfo->Type == REG_SZ) &&
               (deviceInstanceInfo->DataLength != 0)) {

                IopRegistryDataToUnicodeString(&deviceInstanceString,
                                               (PWSTR)KEY_VALUE_DATA(deviceInstanceInfo),
                                               deviceInstanceInfo->DataLength);

            } else {
                //
                // DeviceInstance value is invalid -- skip it and move on.
                //
                ExFreePool(deviceInstanceInfo);
                goto CloseInterfaceKeyAndContinue;

            }

            //
            // Compare the DeviceInstance of this interface to DeviceInstancePath
            //
            if (RtlEqualUnicodeString(&deviceInstanceString, DeviceInstancePath, TRUE)) {

                ZwClose(hInterface);
                hInterface = NULL;

                //
                // Retrieve all instances of this device interface
                // (active and non-active)
                //
                RtlGUIDFromString(&guidString, &classGUID);

                status = IopGetDeviceInterfaces(&classGUID,
                                                DeviceInstancePath,
                                                DEVICE_INTERFACE_INCLUDE_NONACTIVE,
                                                FALSE,       // kernel-mode format
                                                &symbolicLinkList,
                                                &symbolicLinkListSize);

                if (NT_SUCCESS(status)) {

                    //
                    // Iterate through all instances of the interface
                    //
                    symLink = symbolicLinkList;
                    while(*symLink != UNICODE_NULL) {

                        RtlInitUnicodeString(&tempString, symLink);

                        //
                        // Unregister this instance of the interface.  Since we are
                        // removing the device, ignore any returned status, since
                        // there isn't anything we can do about interfaces which
                        // fail unregistration.
                        //
                        IopUnregisterDeviceInterface(&tempString);

                        symLink += ((tempString.Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR));
                    }
                    ExFreePool(symbolicLinkList);
                }

                //
                // Recursively delete the interface key, if it still exists.
                // While IopUnregisterDeviceInterface will itself delete the
                // interface key if no interface instance subkeys remain, if any
                // of the above calls to IopUnregisterDeviceInterface failed to
                // delete an interface instance key, subkeys will remain, and
                // the interface key will not have been deleted.  We'll catch
                // that here.
                //
                status = IopOpenRegistryKeyEx( &hInterface,
                                               hClassGUID,
                                               &interfaceString,
                                               KEY_READ
                                               );
                if(NT_SUCCESS(status)){
                    if (NT_SUCCESS(IopDeleteKeyRecursive(hClassGUID,
                                                         interfaceString.Buffer))) {
                        deletedInterface = TRUE;
                    }
                    ZwDeleteKey(hInterface);
                    ZwClose(hInterface);
                    hInterface = NULL;
                } else if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
                    //
                    // Interface was already deleted by IopUnregisterDeviceInterface
                    //
                    deletedInterface = TRUE;
                }
            }

            //
            // Free allocated key info structure
            //
            ExFreePool(deviceInstanceInfo);

CloseInterfaceKeyAndContinue:

            if (hInterface != NULL) {
                ZwClose(hInterface);
                hInterface = NULL;
            }

            IopFreeAllocatedUnicodeString(&interfaceString);

            //
            // Only increment the enumeration index for non-deleted keys
            //
            if (!deletedInterface) {
                interfaceIndex++;
            }

        }

CloseClassKeyAndContinue:

        if (hClassGUID != NULL) {
            ZwClose(hClassGUID);
            hClassGUID = NULL;
        }
        classIndex++;
    }

clean1:
    if (hInterface) {
        ZwClose(hInterface);
    }
    if (hClassGUID) {
        ZwClose(hClassGUID);
    }
    if (hDeviceClasses) {
        ZwClose(hDeviceClasses);
    }

    IopFreeBuffer(&interfaceInfoBuffer);
    IopFreeBuffer(&classInfoBuffer);

clean0:
    return status;
}

NTSTATUS
IopOpenOrCreateDeviceInterfaceSubKeys(
    OUT PHANDLE InterfaceKeyHandle           OPTIONAL,
    OUT PULONG InterfaceKeyDisposition       OPTIONAL,
    OUT PHANDLE InterfaceInstanceKeyHandle   OPTIONAL,
    OUT PULONG InterfaceInstanceDisposition  OPTIONAL,
    IN HANDLE InterfaceClassKeyHandle,
    IN PUNICODE_STRING DeviceInterfaceName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN Create
    )

/*++

Routine Description:

    This API opens or creates a two-level registry hierarchy underneath the
    specified interface class key for a particular device interface.  The first
    level is the (munged) symbolic link name (sans RefString).  The second level
    is the refstring, prepended with a '#' sign (if the device interface has no
    refstring, then this key name is simply '#').

Parameters:

    InterfaceKeyHandle - Optionally, supplies the address of a variable that
        receives a handle to the interface key (1st level in the hierarchy).

    InterfaceKeyDisposition - Optionally, supplies the address of a variable that
        receives either REG_CREATED_NEW_KEY or REG_OPENED_EXISTING_KEY indicating
        whether the interface key was newly-created.

    InterfaceInstanceKeyHandle - Optionally, supplies the address of a variable
        that receives a handle to the interface instance key (2nd level in the
        hierarchy).

    InterfaceInstanceDisposition - Optionally, supplies the address of a variable
        that receives either REG_CREATED_NEW_KEY or REG_OPENED_EXISTING_KEY
        indicating whether the interface instance key was newly-created.

    InterfaceClassKeyHandle - Supplies a handle to the interface class key under
        which the device interface keys are to be opened/created.

    DeviceInterfaceName - Supplies the (user-mode or kernel-mode form) device
        interface name.

    DesiredAccess - Specifies the desired access that the caller needs to the keys.

    Create - Determines if the keys are to be created if they do not exist.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    UNICODE_STRING TempString, RefString;
    WCHAR PoundCharBuffer;
    HANDLE hTempInterface, hTempInterfaceInstance;
    ULONG TempInterfaceDisposition;
    BOOLEAN RefStringPresent=FALSE;

    PAGED_CODE();

    //
    // Make a copy of the device interface name, since we're going to munge it.
    //
    status = IopAllocateUnicodeString(&TempString, DeviceInterfaceName->Length);

    if(!NT_SUCCESS(status)) {
        goto clean0;
    }

    RtlCopyUnicodeString(&TempString, DeviceInterfaceName);

    //
    // Find the refstring component (if there is one).
    //
    status = IopParseSymbolicLinkName(&TempString,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &RefString,
                                      &RefStringPresent,
                                      NULL
                                     );
    ASSERT(NT_SUCCESS(status));

    if(RefStringPresent) {
        //
        // Truncate the device interface name before the refstring separator char.
        //
        RefString.Buffer--;
        RefString.Length += sizeof(WCHAR);
        RefString.MaximumLength += sizeof(WCHAR);
        TempString.MaximumLength = TempString.Length = (USHORT)((PUCHAR)RefString.Buffer - (PUCHAR)TempString.Buffer);
    } else {
        //
        // Set up refstring to point to a temporary character buffer that will hold
        // the single '#' used for the key name when no refstring is present.
        //
        RefString.Buffer = &PoundCharBuffer;
        RefString.Length = RefString.MaximumLength = sizeof(PoundCharBuffer);
    }

    //
    // Replace the "\??\" or "\\?\" symbolic link name prefix with ##?#
    //
    RtlCopyMemory(TempString.Buffer, KEY_STRING_PREFIX, IopConstStringSize(KEY_STRING_PREFIX));

    //
    // Munge the string
    //
    IopReplaceSeperatorWithPound(&TempString, &TempString);

    //
    // Now open/create this subkey under the interface class key.
    //

    if (Create) {
        status = IopCreateRegistryKeyEx( &hTempInterface,
                                         InterfaceClassKeyHandle,
                                         &TempString,
                                         DesiredAccess,
                                         REG_OPTION_NON_VOLATILE,
                                         &TempInterfaceDisposition
                                         );
    } else {
        status = IopOpenRegistryKeyEx( &hTempInterface,
                                       InterfaceClassKeyHandle,
                                       &TempString,
                                       DesiredAccess
                                       );
    }

    if(!NT_SUCCESS(status)) {
        goto clean1;
    }

    //
    // Store a '#' as the first character of the RefString, and then we're ready to open the
    // refstring subkey.
    //
    *RefString.Buffer = REFSTRING_PREFIX_CHAR;

    //
    // Now open/create the subkey under the interface key representing this interface instance
    // (i.e., differentiated by refstring).
    //

    if (Create) {
        status = IopCreateRegistryKeyEx( &hTempInterfaceInstance,
                                       hTempInterface,
                                       &RefString,
                                       DesiredAccess,
                                       REG_OPTION_NON_VOLATILE,
                                       InterfaceInstanceDisposition
                                       );
    } else {
        status = IopOpenRegistryKeyEx( &hTempInterfaceInstance,
                                       hTempInterface,
                                       &RefString,
                                       DesiredAccess
                                       );
    }

    if(NT_SUCCESS(status)) {
        //
        // Store any requested return values in the caller-supplied buffers.
        //
        if(InterfaceKeyHandle) {
            *InterfaceKeyHandle = hTempInterface;
        } else {
            ZwClose(hTempInterface);
        }
        if(InterfaceKeyDisposition) {
            *InterfaceKeyDisposition = TempInterfaceDisposition;
        }
        if(InterfaceInstanceKeyHandle) {
            *InterfaceInstanceKeyHandle = hTempInterfaceInstance;
        } else {
            ZwClose(hTempInterfaceInstance);
        }
        //
        // (no need to set InterfaceInstanceDisposition--we already set it above)
        //
    } else {
        //
        // If the interface key was newly-created above, then delete it.
        //
        if(TempInterfaceDisposition == REG_CREATED_NEW_KEY) {
            ZwDeleteKey(hTempInterface);
        } else {
            ZwClose(hTempInterface);
        }
    }

clean1:
    IopFreeAllocatedUnicodeString(&TempString);

clean0:
    return status;
}

NTSTATUS
IoGetDeviceInterfaceAlias(
    IN PUNICODE_STRING SymbolicLinkName,
    IN CONST GUID *AliasInterfaceClassGuid,
    OUT PUNICODE_STRING AliasSymbolicLinkName
    )

/*++

Routine Description:

    This API returns a symbolic link name (i.e., device interface) of a
    particular interface class that 'aliases' the specified device interface.
    Two device interfaces are considered aliases of each other if the
    following two criteria are met:

        1.  Both interfaces are exposed by the same PDO (devnode).
        2.  Both interfaces share the same RefString.

Parameters:

    SymbolicLinkName - Supplies the name of the device interface whose alias is
        to be retrieved.

    AliasInterfaceClassGuid - Supplies a pointer to the GUID representing the interface
        class for which an alias is to be retrieved.

    AliasSymbolicLinkName - Supplies a pointer to a string which, upon success,
        will contain the name of the device interface in the specified class that
        aliases the SymbolicLinkName interface.  (This symbolic link name will be
        returned in either kernel-mode or user-mode form, depeding upon the form
        of the SymbolicLinkName path).

        It is the caller's responsibility to free the buffer allocated for this
        string via ExFreePool().

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    HANDLE hKey;
    PKEY_VALUE_FULL_INFORMATION pDeviceInstanceInfo;
    UNICODE_STRING deviceInstanceString, refString, guidString, otherString;
    PUNICODE_STRING pUserString, pKernelString;
    BOOLEAN refStringPresent, userModeFormat;

    PAGED_CODE();

    //
    // Convert the class guid into string form
    //

    status = RtlStringFromGUID(AliasInterfaceClassGuid, &guidString);
    if( !NT_SUCCESS(status) ){
        goto clean0;
    }

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&PpRegistryDeviceResource, TRUE);

    //
    // Open the (parent) device interface key--not the refstring-specific one.
    //

    status = IopDeviceInterfaceKeysFromSymbolicLink(SymbolicLinkName,
                                                    KEY_READ,
                                                    NULL,
                                                    &hKey,
                                                    NULL
                                                    );
    if(!NT_SUCCESS(status)) {
        goto clean1;
    }

    //
    // Get the name of the device instance that 'owns' this interface.
    //

    status = IopGetRegistryValue(hKey, REGSTR_VAL_DEVICE_INSTANCE, &pDeviceInstanceInfo);

    ZwClose(hKey);

    if(!NT_SUCCESS(status)) {
        goto clean1;
    }

    if(pDeviceInstanceInfo->Type == REG_SZ) {

        IopRegistryDataToUnicodeString(&deviceInstanceString,
                                       (PWSTR)KEY_VALUE_DATA(pDeviceInstanceInfo),
                                       pDeviceInstanceInfo->DataLength
                                      );

    } else {

        status = STATUS_INVALID_PARAMETER_1;
        goto clean2;

    }

    //
    // Now parse out the refstring, so that we can construct the name of the interface device's
    // alias.  (NOTE: we have not yet verified that the alias actually exists, we're only
    // constructing what its name would be, if it did exist.)
    //
    // Don't bother to check the return code.  If this were a bad string, we'd have already
    // failed above when we called IopDeviceInterfaceKeysFromSymbolicLink.
    //
    IopParseSymbolicLinkName(SymbolicLinkName,
                             NULL,
                             NULL,
                             NULL,
                             &refString,
                             &refStringPresent,
                             NULL
                            );

    //
    // Did the caller supply us with a user-mode or kernel-mode format path?
    //
    userModeFormat = (IopConstStringSize(USER_SYMLINK_STRING_PREFIX) ==
                          RtlCompareMemory(SymbolicLinkName->Buffer,
                                           USER_SYMLINK_STRING_PREFIX,
                                           IopConstStringSize(USER_SYMLINK_STRING_PREFIX)
                                          ));

    if(userModeFormat) {
        pUserString = AliasSymbolicLinkName;
        pKernelString = &otherString;
    } else {
        pKernelString = AliasSymbolicLinkName;
        pUserString = &otherString;
    }

    status = IopBuildSymbolicLinkStrings(&deviceInstanceString,
                                         &guidString,
                                         refStringPresent ? &refString : NULL,
                                         pUserString,
                                         pKernelString
                                         );
    if (!NT_SUCCESS(status)) {
        goto clean2;
    }

    //
    // OK, we now have the symbolic link name of the alias, but we don't yet know whether
    // it actually exists.  Check this by attempting to open the associated registry key.
    //
    status = IopDeviceInterfaceKeysFromSymbolicLink(AliasSymbolicLinkName,
                                                    KEY_READ,
                                                    NULL,
                                                    NULL,
                                                    &hKey
                                                    );

    if(NT_SUCCESS(status)) {
        //
        // Alias exists--close the key handle.
        //
        ZwClose(hKey);
    } else {
        IopFreeAllocatedUnicodeString(AliasSymbolicLinkName);
    }

    IopFreeAllocatedUnicodeString(&otherString);

clean2:
    ExFreePool(pDeviceInstanceInfo);

clean1:
    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();
    RtlFreeUnicodeString(&guidString);

clean0:
    return status;
}

NTSTATUS
IopBuildSymbolicLinkStrings(
    IN PUNICODE_STRING DeviceString,
    IN PUNICODE_STRING GuidString,
    IN PUNICODE_STRING ReferenceString      OPTIONAL,
    OUT PUNICODE_STRING UserString,
    OUT PUNICODE_STRING KernelString
)
/*++

Routine Description:

    This routine will construct various strings used in the registration of
    function device class associations (IoRegisterDeviceClassAssociation).
    The specific strings are detailed below

Parameters:

    DeviceString - Supplies a pointer to the instance path of the device.
        It is of the form <Enumerator>\<Device>\<Instance>.

    GuidString - Supplies a pointer to the string representation of the
        function class guid.

    ReferenceString - Supplies a pointer to the reference string for the given
        device to exhibit the given function.  This is optional

    UserString - Supplies a pointer to an uninitialised string which on success
        will contain the string to be assigned to the "SymbolicLink" value under the
        KeyString.  It is of the format \\?\<MungedDeviceString>\<GuidString>\<Reference>
        When no longer required it should be freed using IopFreeAllocatedUnicodeString.

    KernelString - Supplies a pointer to an uninitialised string which on success
        will contain the kernel mode path of the device and is of the format
        \??\<MungedDeviceString>\<GuidString>\<Reference>. When no longer required it
        should be freed using IopFreeAllocatedUnicodeString.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    USHORT length, count;
    UNICODE_STRING mungedDeviceString;

    PAGED_CODE();

    //
    // The code is optimised to use the fact that \\.\ and \??\ are the same size - if
    // these prefixes change then we need to change the code.
    //

    ASSERT(IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX) == IopConstStringSize(USER_SYMLINK_STRING_PREFIX));

    //
    // Calculate the lengths of the strings
    //

    length = IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX) + DeviceString->Length +
             IopConstStringSize(REPLACED_SEPERATOR_STRING) + GuidString->Length;

    if(ARGUMENT_PRESENT(ReferenceString)) {
        length += IopConstStringSize(SEPERATOR_STRING) + ReferenceString->Length;
    }

    //
    // Allocate space for the strings
    //

    status = IopAllocateUnicodeString(KernelString, length);
    if (!NT_SUCCESS(status)) {
        goto clean0;
    }

    status = IopAllocateUnicodeString(UserString, length);
    if (!NT_SUCCESS(status)) {
        goto clean1;
    }

    //
    // Allocate a temporary string to hold the munged device string
    //

    status = IopAllocateUnicodeString(&mungedDeviceString, DeviceString->Length);
    if (!NT_SUCCESS(status)) {
        goto clean2;
    }

    //
    // Copy and munge the device string
    //

    status = IopReplaceSeperatorWithPound(&mungedDeviceString, DeviceString);
    if (!NT_SUCCESS(status)) {
        goto clean3;
    }

    //
    // Construct the user mode string
    //

    RtlAppendUnicodeToString(UserString, USER_SYMLINK_STRING_PREFIX);
    RtlAppendUnicodeStringToString(UserString, &mungedDeviceString);
    RtlAppendUnicodeToString(UserString, REPLACED_SEPERATOR_STRING);
    RtlAppendUnicodeStringToString(UserString, GuidString);

    if (ARGUMENT_PRESENT(ReferenceString)) {
        RtlAppendUnicodeToString(UserString, SEPERATOR_STRING);
        RtlAppendUnicodeStringToString(UserString, ReferenceString);
    }

    ASSERT( UserString->Length == length );

    //
    // Construct the kernel mode string by replacing the prefix on the value string
    //

    RtlCopyUnicodeString(KernelString, UserString);
    RtlCopyMemory(KernelString->Buffer,
                  KERNEL_SYMLINK_STRING_PREFIX,
                  IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX)
                 );

clean3:
    IopFreeAllocatedUnicodeString(&mungedDeviceString);

clean2:
    if (!NT_SUCCESS(status)) {
        IopFreeAllocatedUnicodeString(UserString);
    }

clean1:
    if (!NT_SUCCESS(status)) {
        IopFreeAllocatedUnicodeString(KernelString);
    }

clean0:
    return status;
}

NTSTATUS
IopReplaceSeperatorWithPound(
    OUT PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString
    )

/*++

Routine Description:

    This routine will copy a string from InString to OutString replacing any occurence of
    '\' or '/' with '#' as it goes.

Parameters:

    OutString - Supplies a pointer to a string which has already been initialised to
        have a buffer large enough to accomodate the string.  The contents of this
        string will be over written

    InString - Supplies a pointer to the string to be munged

Return Value:

    Status code that indicates whether or not the function was successful.

Remarks:

    In place munging can be performed - ie. the In and Out strings can be the same.

--*/

{
    PWSTR pInPosition, pOutPosition;
    USHORT count;

    PAGED_CODE();

    ASSERT(InString);
    ASSERT(OutString);

    //
    // Ensure we have enough space in the output string
    //

    if(InString->Length > OutString->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    pInPosition = InString->Buffer;
    pOutPosition = OutString->Buffer;
    count = InString->Length / sizeof(WCHAR);

    //
    // Traverse the in string copying and replacing all occurences of '\' or '/'
    // with '#'
    //

    while (count--) {
        if((*pInPosition == SEPERATOR_CHAR) || (*pInPosition == ALT_SEPERATOR_CHAR)) {
            *pOutPosition = REPLACED_SEPERATOR_CHAR;
        } else {
            *pOutPosition = *pInPosition;
        }
        pInPosition++;
        pOutPosition++;
    }

    OutString->Length = InString->Length;

    return STATUS_SUCCESS;

}

NTSTATUS
IopDropReferenceString(
    OUT PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString
    )

/*++

Routine Description:

    This routine removes the reference string from a symbolic link name.  No space
    is allocated for the out string so no attempt should be made to free the buffer
    of OutString.

Parameters:

    SymbolicLinkName - Supplies a pointer to a symbolic link name string.
        Both the prefixed strings are valid.

    GuidReferenceString - Supplies a pointer to an uninitialised string which on
        success will contain the symbolic link name without the reference string.
        See the note on storage allocation above.

Return Value:

    Status code that indicates whether or not the function was successful.

Remarks:

    The string returned in OutString is dependant on the buffer of
    InString and is only valid as long as InString is valid.

--*/

{
    UNICODE_STRING refString;
    NTSTATUS status;
    BOOLEAN refStringPresent;

    PAGED_CODE();

    ASSERT(InString);
    ASSERT(OutString);

    OutString->Buffer = InString->Buffer;

    status = IopParseSymbolicLinkName(InString, NULL, NULL, NULL, &refString, &refStringPresent, NULL);

    if (NT_SUCCESS(status)) {
        //
        // If we have a refstring then subtract it's length
        //
        if (refStringPresent) {
            OutString->Length = InString->Length - (refString.Length + sizeof(WCHAR));
        } else {
            OutString->Length = InString->Length;
        }

    } else {
        //
        // Invalidate the returned string
        //
        OutString->Buffer = NULL;
        OutString->Length = 0;
    }

    OutString->MaximumLength = OutString->Length;

    return status;
}

NTSTATUS
IopParseSymbolicLinkName(
    IN  PUNICODE_STRING SymbolicLinkName,
    OUT PUNICODE_STRING PrefixString        OPTIONAL,
    OUT PUNICODE_STRING MungedPathString    OPTIONAL,
    OUT PUNICODE_STRING GuidString          OPTIONAL,
    OUT PUNICODE_STRING RefString           OPTIONAL,
    OUT PBOOLEAN        RefStringPresent    OPTIONAL,
    OUT LPGUID Guid                         OPTIONAL
    )

/*++

Routine Description:

    This routine breaks apart a symbolic link name constructed by
    IopBuildSymbolicLinkNames.  Both formats of name are valid - user
    mode \\?\ and kernel mode \??\.

Parameters:

    SymbolicLinkName - Supplies a pointer to the symbolic link name to
        be analysed.

    PrefixString - Optionally contains a pointer to a string which will contain
        the prefix of the string.

    MungedPathString - Optionally contains a pointer to a string which will contain
        the enumeration path of the device with all occurences of '\' replaced with '#'.

    GuidString - Optionally contains a pointer to a string which will contain
        the device class guid in string format from the string.

    RefString - Optionally contains a pointer to a string which will contain
        the refstring of the string if one is present, otherwise it is undefined.

    RefStringPresent - Optionally contains a pointer to a boolean value which will
        be set to true if a refstring is present.

    Guid - Optionally contains a pointer to a guid which will contain
        the function class guid of the string.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pCurrent;
    USHORT last, current, path, guid, reference = 0;
    UNICODE_STRING tempString;
    GUID tempGuid;
    BOOLEAN haveRefString;

    PAGED_CODE();

    ASSERT(IopConstStringSize(USER_SYMLINK_STRING_PREFIX) == IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX));

    //
    // Sanity check on the incoming string - if it does not have a \\?\ or \??\ prefix then fail
    //

    if (!(RtlCompareMemory(SymbolicLinkName->Buffer,
                           USER_SYMLINK_STRING_PREFIX,
                           IopConstStringSize(USER_SYMLINK_STRING_PREFIX)
                          ) != IopConstStringSize(USER_SYMLINK_STRING_PREFIX)
       || RtlCompareMemory(SymbolicLinkName->Buffer,
                           KERNEL_SYMLINK_STRING_PREFIX,
                           IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX)
                          ) != IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX)
       )) {

        status = STATUS_INVALID_PARAMETER;
        goto clean0;
    }


    //
    // check that the input buffer really is big enough
    //
    if (SymbolicLinkName->Length <(IopConstStringLength(KERNEL_SYMLINK_STRING_PREFIX)+GUID_STRING_SIZE+1)) {
        status = STATUS_INVALID_PARAMETER;
        goto clean0;

    }

    //
    // Break apart the string into it's constituent parts
    //

    path = IopConstStringSize(USER_SYMLINK_STRING_PREFIX) + 1;

    //
    // Find the '\' seperator
    //

    pCurrent = SymbolicLinkName->Buffer + IopConstStringLength(KERNEL_SYMLINK_STRING_PREFIX);

    for (current = 0;
         current < (SymbolicLinkName->Length / sizeof(WCHAR)) - IopConstStringLength(KERNEL_SYMLINK_STRING_PREFIX);
         current++, pCurrent++) {

        if(*pCurrent == SEPERATOR_CHAR) {
            reference = current + 1 + IopConstStringLength(KERNEL_SYMLINK_STRING_PREFIX);
            break;
        }

    }

    //
    // If we don't have a reference string fake it to where it would have been
    //

    if (reference == 0) {
        haveRefString = FALSE;
        reference = SymbolicLinkName->Length / sizeof(WCHAR) + 1;

    } else {
        haveRefString = TRUE;
    }

    //
    // Check the guid looks plausable
    //

    tempString.Length = GUID_STRING_SIZE;
    tempString.MaximumLength = GUID_STRING_SIZE;
    tempString.Buffer = SymbolicLinkName->Buffer + reference - GUID_STRING_LENGTH - 1;

    if (!NT_SUCCESS( RtlGUIDFromString(&tempString, &tempGuid) )) {
        status = STATUS_INVALID_PARAMETER;
        goto clean0;
    }

    guid = reference - GUID_STRING_LENGTH - 1;

    //
    // Setup return strings
    //

    if (ARGUMENT_PRESENT(PrefixString)) {
        PrefixString->Length = IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX);
        PrefixString->MaximumLength = PrefixString->Length;
        PrefixString->Buffer = SymbolicLinkName->Buffer;
    }

    if (ARGUMENT_PRESENT(MungedPathString)) {
        MungedPathString->Length = (reference - 1 - GUID_STRING_LENGTH - 1 -
                                   IopConstStringLength(KERNEL_SYMLINK_STRING_PREFIX)) *
                                   sizeof(WCHAR);
        MungedPathString->MaximumLength = MungedPathString->Length;
        MungedPathString->Buffer = SymbolicLinkName->Buffer +
                                   IopConstStringLength(KERNEL_SYMLINK_STRING_PREFIX);

    }

    if (ARGUMENT_PRESENT(GuidString)) {
        GuidString->Length = GUID_STRING_SIZE;
        GuidString->MaximumLength = GuidString->Length;
        GuidString->Buffer = SymbolicLinkName->Buffer + reference -
                             GUID_STRING_LENGTH - 1;
    }

    if (ARGUMENT_PRESENT(RefString)) {
        //
        // Check if we have a refstring
        //
        if (haveRefString) {
            RefString->Length = SymbolicLinkName->Length -
                                  (reference * sizeof(WCHAR));
            RefString->MaximumLength = RefString->Length;
            RefString->Buffer = SymbolicLinkName->Buffer + reference;
        } else {
            RefString->Length = 0;
            RefString->MaximumLength = 0;
            RefString->Buffer = NULL;
        }
    }

    if (ARGUMENT_PRESENT(RefStringPresent)) {
        *RefStringPresent = haveRefString;
    }

    if(ARGUMENT_PRESENT(Guid)) {
        *Guid = tempGuid;
    }

clean0:

    return status;

}

NTSTATUS
IopAllocateUnicodeString(
    IN OUT PUNICODE_STRING String,
    IN USHORT Length
    )

/*++

Routine Description:

    This routine allocates a buffer for a unicode string of a given length
    and initialises the UNICODE_STRING structure appropriately. When the
    string is no longer required it can be freed using IopFreeAllocatedString.
    The buffer also be directly deleted by ExFreePool and so can be handed
    back to a caller.

Parameters:

    String - Supplies a pointer to an uninitialised unicode string which will
        be manipulated by the function.

    Length - The number of BYTES long that the string will be.

Return Value:

    Either STATUS_INSUFFICIENT_RESOURCES indicating paged pool is exhausted or
    STATUS_SUCCESS.

Remarks:

    The buffer allocated will be one character (2 bytes) more than length specified.
    This is to allow for easy null termination of the strings - eg for registry
    storage.

--*/

{
    PAGED_CODE();

    String->Length = 0;
    String->MaximumLength = Length + sizeof(UNICODE_NULL);

    if(!(String->Buffer = ExAllocatePool(PagedPool, Length + sizeof(UNICODE_NULL)))) {
        return STATUS_INSUFFICIENT_RESOURCES;
    } else {
        return STATUS_SUCCESS;
    }
}

VOID
IopFreeAllocatedUnicodeString(
    PUNICODE_STRING String
    )

/*++

Routine Description:

    This routine frees a string previously allocated with IopAllocateUnicodeString.

Parameters:

    String - Supplies a pointer to the string that has been previously allocated.

Return Value:

    None

--*/

{
    PAGED_CODE();

    ASSERT(String);

    //
    // If we have a buffer free it
    //

    if(String->Buffer) {

        ExFreePool(String->Buffer);

    }

    //
    // Blank out the string
    //

    String->Buffer = NULL;
    String->Length = 0;
    String->MaximumLength = 0;

}

NTSTATUS
IopSetRegistryStringValue(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN PUNICODE_STRING ValueData
    )

/*++

Routine Description:

    Sets a value key in the registry to a specific value of string (REG_SZ) type.

Parameters:

    KeyHandle - A handle to the key under which the value is stored.

    ValueName - Supplies a pointer to the name of the value key

    ValueData - Supplies a pointer to the string to be stored in the key.  The data
        will automatically be null terminated for storage in the registry.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;

    PAGED_CODE();

    ASSERT(ValueName);
    ASSERT(ValueData);
    ASSERT(ValueName->Buffer);
    ASSERT(ValueData->Buffer);

    //
    // Null terminate the string
    //

    if ((ValueData->MaximumLength - ValueData->Length) >= sizeof(UNICODE_NULL)) {

        //
        // There is room in the buffer so just append a null
        //

        ValueData->Buffer[(ValueData->Length / sizeof(WCHAR))] = UNICODE_NULL;

        //
        // Set the registry value
        //

        status = ZwSetValueKey(KeyHandle,
                               ValueName,
                               0,
                               REG_SZ,
                               (PVOID) ValueData->Buffer,
                               ValueData->Length + sizeof(UNICODE_NULL)
                               );

    } else {

        UNICODE_STRING tempString;

        //
        // There is no room so allocate a new buffer and so we need to build
        // a new string with room
        //

        status = IopAllocateUnicodeString(&tempString, ValueData->Length);

        if (!NT_SUCCESS(status)) {
            goto clean0;
        }

        //
        // Copy the input string to the output string
        //

        tempString.Length = ValueData->Length;
        RtlCopyMemory(tempString.Buffer, ValueData->Buffer, ValueData->Length);

        //
        // Add the null termination
        //

        tempString.Buffer[tempString.Length / sizeof(WCHAR)] = UNICODE_NULL;

        //
        // Set the registry value
        //

        status = ZwSetValueKey(KeyHandle,
                               ValueName,
                               0,
                               REG_SZ,
                               (PVOID) tempString.Buffer,
                               tempString.Length + sizeof(UNICODE_NULL)
                               );

        //
        // Free the temporary string
        //

        IopFreeAllocatedUnicodeString(&tempString);

    }

clean0:
    return status;

}

NTSTATUS
IoUnregisterPlugPlayNotification(
    IN PVOID NotificationEntry
    )

/*++

Routine Description:

    This routine unregisters a notification previously registered via
    IoRegisterPlugPlayNotification.  A driver cannot be unloaded until it has
    unregistered all of its notification handles.

Parameters:

    NotificationEntry - This provices the cookie returned by IoRegisterPlugPlayNotification
        which identifies the registration in question.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PNOTIFY_ENTRY_HEADER entry;
    PFAST_MUTEX lock;
    BOOLEAN wasDeferred = FALSE;

    PAGED_CODE();

    ASSERT(NotificationEntry);

    entry = (PNOTIFY_ENTRY_HEADER)NotificationEntry;

    lock = entry->Lock;

    ExAcquireFastMutex(&PiNotificationInProgressLock);
    if (PiNotificationInProgress) {
        //
        // Before unregistering the entry, we need to make sure that it's not sitting
        // around in the deferred registration list.
        //
        IopAcquireNotifyLock(&IopDeferredRegistrationLock);

        if (!IsListEmpty(&IopDeferredRegistrationList)) {

            PLIST_ENTRY link;
            PDEFERRED_REGISTRATION_ENTRY deferredNode;

            link = IopDeferredRegistrationList.Flink;
            deferredNode = (PDEFERRED_REGISTRATION_ENTRY)link;

            while (link != (PLIST_ENTRY)&IopDeferredRegistrationList) {
                ASSERT(deferredNode->NotifyEntry->Unregistered);
                if (deferredNode->NotifyEntry == entry) {
                    wasDeferred = TRUE;
                    if (lock) {
                        IopAcquireNotifyLock(lock);
                    }
                    link = link->Flink;
                    RemoveEntryList((PLIST_ENTRY)deferredNode);
                    IopDereferenceNotify((PNOTIFY_ENTRY_HEADER)deferredNode->NotifyEntry);
                    if (lock) {
                        IopReleaseNotifyLock(lock);
                    }
                    ExFreePool(deferredNode);
                    deferredNode = (PDEFERRED_REGISTRATION_ENTRY)link;
                } else {
                    link = link->Flink;
                    deferredNode = (PDEFERRED_REGISTRATION_ENTRY)link;
                }
            }
        }

        IopReleaseNotifyLock(&IopDeferredRegistrationLock);
    } else {
        //
        // If there is currently no notification in progress, the deferred
        // registration list must be empty.
        //
        ASSERT(IsListEmpty(&IopDeferredRegistrationList));
    }
    ExReleaseFastMutex(&PiNotificationInProgressLock);

    //
    // Acquire lock
    //
    if (lock) {
        IopAcquireNotifyLock(lock);
    }

    ASSERT(wasDeferred == entry->Unregistered);

    if (!entry->Unregistered || wasDeferred) {
        //
        // Dereference the entry if it is currently registered, or had its
        // registration pending completion of the notification in progress.
        //

        //
        // Mark the entry as unregistered so we don't notify on it
        //

        entry->Unregistered = TRUE;

        //
        // Dereference it thus deleting if no longer required
        //

        IopDereferenceNotify(entry);
    }

    //
    // Release the lock
    //

    if (lock) {
        IopReleaseNotifyLock(lock);
    }

    return STATUS_SUCCESS;

}

VOID
IopProcessDeferredRegistrations(
    VOID
    )
/*++

Routine Description:

    This routine removes notification entries from the deferred registration
    list, marking them as "registered" so that they can receive notifications.

Parameters:

    None.

Return Value:

    None.

  --*/
{
    PDEFERRED_REGISTRATION_ENTRY deferredNode;
    PFAST_MUTEX lock;

    IopAcquireNotifyLock(&IopDeferredRegistrationLock);

    while (!IsListEmpty(&IopDeferredRegistrationList)) {

        deferredNode = (PDEFERRED_REGISTRATION_ENTRY)RemoveHeadList(&IopDeferredRegistrationList);

        //
        // Acquire this entry's list lock.
        //
        lock = deferredNode->NotifyEntry->Lock;
        if (lock) {
            IopAcquireNotifyLock(lock);
        }

        //
        // Mark this entry as registered.
        //
        deferredNode->NotifyEntry->Unregistered = FALSE;

        //
        // Dereference the notification entry when removing it from the deferred
        // list, and free the node.
        //
        IopDereferenceNotify((PNOTIFY_ENTRY_HEADER)deferredNode->NotifyEntry);
        ExFreePool(deferredNode);

        //
        // Release this entry's list lock.
        //
        if (lock) {
            IopReleaseNotifyLock(lock);
            lock = NULL;
        }
    }

    IopReleaseNotifyLock(&IopDeferredRegistrationLock);
}


NTSTATUS
IoReportTargetDeviceChange(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PVOID NotificationStructure  // always begins with a PLUGPLAY_NOTIFICATION_HEADER
    )

/*++

Routine Description:

    This routine may be used to give notification of 3rd-party target device
    change events.  This API will notify every driver that has registered for
    notification on a file object associated with PhysicalDeviceObject about
    the event indicated in the NotificationStructure.

Parameters:

    PhysicalDeviceObject - Provides a pointer to the PDO that the change begin
        reported is associated with.

    NotificationStructure - Provides a pointer to the notification structure to be
        sent to all parties registered for notifications about changes to
        PhysicalDeviceObject.

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    This API may only be used to report non-PnP target device changes.  In particular,
    it will fail if it's called with the NotificationStructure->Event field set to
    GUID_TARGET_DEVICE_QUERY_REMOVE, GUID_TARGET_DEVICE_REMOVE_CANCELLED, or
    GUID_TARGET_DEVICE_REMOVE_COMPLETE.

--*/
{

    NTSTATUS status = STATUS_SUCCESS;
    KEVENT completionEvent;
    NTSTATUS completionStatus;
    PTARGET_DEVICE_CUSTOM_NOTIFICATION notifyStruct;
    LONG                   dataSize;

    PAGED_CODE();

    notifyStruct = (PTARGET_DEVICE_CUSTOM_NOTIFICATION)NotificationStructure;

    ASSERT(notifyStruct);

    ASSERT_PDO(PhysicalDeviceObject);

    ASSERT(NULL == notifyStruct->FileObject);


    if (IopCompareGuid(&notifyStruct->Event, &GUID_TARGET_DEVICE_QUERY_REMOVE) ||
        IopCompareGuid(&notifyStruct->Event, &GUID_TARGET_DEVICE_REMOVE_CANCELLED) ||
        IopCompareGuid(&notifyStruct->Event, &GUID_TARGET_DEVICE_REMOVE_COMPLETE)) {

        //
        //  Passed in an illegal value
        //

#if DBG
        DbgPrint("Illegal Event type passed as custom notification\n");
#endif
        return STATUS_INVALID_DEVICE_REQUEST;

    }

    if (notifyStruct->Size < FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer)) {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    dataSize = notifyStruct->Size - FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer);

    if (notifyStruct->NameBufferOffset != -1 && notifyStruct->NameBufferOffset > dataSize)  {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    KeInitializeEvent(&completionEvent, NotificationEvent, FALSE);

    status = PpSetCustomTargetEvent( PhysicalDeviceObject,
                                     &completionEvent,
                                     &completionStatus,
                                     NULL,
                                     NULL,
                                     notifyStruct);

    if (NT_SUCCESS(status))  {

        KeWaitForSingleObject( &completionEvent, Executive, KernelMode, FALSE, NULL );

        status = completionStatus;
    }

    return status;
}

NTSTATUS
IoReportTargetDeviceChangeAsynchronous(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PVOID NotificationStructure,  // always begins with a PLUGPLAY_NOTIFICATION_HEADER
    IN PDEVICE_CHANGE_COMPLETE_CALLBACK Callback        OPTIONAL,
    IN PVOID Context    OPTIONAL
    )

/*++

Routine Description:

    This routine may be used to give notification of 3rd-party target device
    change events.  This API will notify every driver that has registered for
    notification on a file object associated with PhysicalDeviceObject about
    the event indicated in the NotificationStructure.

Parameters:

    PhysicalDeviceObject - Provides a pointer to the PDO that the change begin
        reported is associated with.

    NotificationStructure - Provides a pointer to the notification structure to be
        sent to all parties registered for notifications about changes to
        PhysicalDeviceObject.

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    This API may only be used to report non-PnP target device changes.  In particular,
    it will fail if it's called with the NotificationStructure->Event field set to
    GUID_TARGET_DEVICE_QUERY_REMOVE, GUID_TARGET_DEVICE_REMOVE_CANCELLED, or
    GUID_TARGET_DEVICE_REMOVE_COMPLETE.

--*/
{
    PASYNC_TDC_WORK_ITEM    asyncWorkItem;
    PWORK_QUEUE_ITEM        workItem;
    NTSTATUS                status;
    LONG                    dataSize;

    PTARGET_DEVICE_CUSTOM_NOTIFICATION   notifyStruct;

    notifyStruct = (PTARGET_DEVICE_CUSTOM_NOTIFICATION)NotificationStructure;

    ASSERT(notifyStruct);

    ASSERT_PDO(PhysicalDeviceObject);

    ASSERT(NULL == notifyStruct->FileObject);

    if (IopCompareGuid(&notifyStruct->Event, &GUID_TARGET_DEVICE_QUERY_REMOVE) ||
        IopCompareGuid(&notifyStruct->Event, &GUID_TARGET_DEVICE_REMOVE_CANCELLED) ||
        IopCompareGuid(&notifyStruct->Event, &GUID_TARGET_DEVICE_REMOVE_COMPLETE)) {

        //
        //  Passed in an illegal value
        //

#if DBG
        DbgPrint("Illegal Event type passed as custom notification\n");
#endif
        return STATUS_INVALID_DEVICE_REQUEST;

    }

    if (notifyStruct->Size < FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer)) {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    dataSize = notifyStruct->Size - FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer);

    if (notifyStruct->NameBufferOffset != -1 && notifyStruct->NameBufferOffset > dataSize)  {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Since this routine can be called at DPC level we need to queue
    // a work item and process it when the irql drops.
    //

    asyncWorkItem = ExAllocatePool( NonPagedPool,
                                    sizeof(ASYNC_TDC_WORK_ITEM) + notifyStruct->Size);

    if (asyncWorkItem != NULL) {

        ObReferenceObject(PhysicalDeviceObject);

        asyncWorkItem->DeviceObject = PhysicalDeviceObject;
        asyncWorkItem->NotificationStructure =
            (PTARGET_DEVICE_CUSTOM_NOTIFICATION)((PUCHAR)asyncWorkItem + sizeof(ASYNC_TDC_WORK_ITEM));

        RtlCopyMemory( asyncWorkItem->NotificationStructure,
                       notifyStruct,
                       notifyStruct->Size);

        asyncWorkItem->Callback = Callback;
        asyncWorkItem->Context = Context;
        workItem = &asyncWorkItem->WorkItem;

        ExInitializeWorkItem(workItem, IopReportTargetDeviceChangeAsyncWorker, asyncWorkItem);

        //
        // Queue a work item to do the enumeration
        //

        ExQueueWorkItem(workItem, DelayedWorkQueue);
        status = STATUS_PENDING;
    } else {
        //
        // Failed to allocate memory for work item.  Nothing we can do ...
        //

        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

VOID
IopReportTargetDeviceChangeAsyncWorker(
    PVOID Context
    )

/*++

Routine Description:

    This routine is the worker routine of IoInvalidateDeviceState.
    Its main purpose is to invoke IopSynchronousQueryDeviceState and release
    work item space.

Parameters:

    Context - Supplies a pointer to the ASYNC_TDC_WORK_ITEM.

ReturnValue:

    None.

--*/

{
    PASYNC_TDC_WORK_ITEM asyncWorkItem = (PASYNC_TDC_WORK_ITEM)Context;

    PpSetCustomTargetEvent( asyncWorkItem->DeviceObject,
                            NULL,
                            NULL,
                            asyncWorkItem->Callback,
                            asyncWorkItem->Context,
                            asyncWorkItem->NotificationStructure);

    ObDereferenceObject(asyncWorkItem->DeviceObject);
    ExFreePool(asyncWorkItem);
}



VOID
IoInvalidateDeviceState(
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

    This API will cause the PnP manager to send the specified PDO an IRP_MN_QUERY_PNP_DEVICE_STATE
    IRP.

Parameters:

    PhysicalDeviceObject - Provides a pointer to the PDO who's state is to be invalidated.

Return Value:

    none.

--*/
{
    PDEVICE_NODE deviceNode;

    ASSERT_PDO(PhysicalDeviceObject);

    //
    // If the call was made before PnP completes device enumeration
    // we can safely ignore it.  PnP manager will do it without
    // driver's request.  If the device was already removed or surprised
    // removed then ignore it as well since this is only valid for started
    // devices.
    //

    deviceNode = (PDEVICE_NODE)PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;

    if ((deviceNode->Flags & (DNF_STARTED | DNF_REMOVE_PENDING_CLOSES)) != DNF_STARTED) {
        return;
    }

    IopQueueDeviceWorkItem( PhysicalDeviceObject,
                            IopInvalidateDeviceStateWorker,
                            NULL);
}


NTSTATUS
IopQueueDeviceWorkItem(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PVOID WorkerRoutine,
    IN PVOID Context
    )

/*++

Routine Description:

    This API will cause the PnP manager to send the specified PDO an
    IRP_MN_QUERY_PNP_DEVICE_STATE IRP.

Parameters:

    PhysicalDeviceObject - Provides a pointer to the PDO who's state is to be
    invalidated.

Return Value:

    none.

--*/

{
    PDEVICE_WORK_ITEM deviceWorkItem;

    //
    // Since this routine can be called at DPC level we need to queue
    // a work item and process it when the irql drops.
    //

    deviceWorkItem = ExAllocatePool(NonPagedPool, sizeof(DEVICE_WORK_ITEM));
    if (deviceWorkItem == NULL) {

        //
        // Failed to allocate memory for work item.  Nothing we can do ...
        //

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ObReferenceObject(PhysicalDeviceObject);
    deviceWorkItem->DeviceObject = PhysicalDeviceObject;
    deviceWorkItem->Context = Context;

    ExInitializeWorkItem( &deviceWorkItem->WorkItem,
                          WorkerRoutine,
                          deviceWorkItem);

    //
    // Queue a work item to do the enumeration
    //

    ExQueueWorkItem( &deviceWorkItem->WorkItem, DelayedWorkQueue );

    return STATUS_SUCCESS;
}

VOID
IopInvalidateDeviceStateWorker(
    PVOID Context
    )

/*++

Routine Description:

    This routine is the worker routine of IoInvalidateDeviceState.
    Its main purpose is to invoke IopSynchronousQueryDeviceState and release
    work item space.

Parameters:

    Context - Supplies a pointer to the DEVICE_WORK_ITEM.

ReturnValue:

    None.

--*/

{
    PDEVICE_WORK_ITEM deviceWorkItem = (PDEVICE_WORK_ITEM)Context;
    PDEVICE_OBJECT deviceObject = deviceWorkItem->DeviceObject;
    PDEVICE_NODE deviceNode;

    ExFreePool(deviceWorkItem);

    //
    // If the device was removed or surprised removed while the work item was
    // queued then ignore it.
    //

    deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;

    if ((deviceNode->Flags & (DNF_STARTED | DNF_REMOVE_PENDING_CLOSES)) == DNF_STARTED) {

        IopQueryDeviceState(deviceObject);

        //
        // PCMCIA driver uses this when switching between Cardbus and R2 cards.
        //
        IopUncacheInterfaceInformation(deviceObject);
    }

    ObDereferenceObject(deviceObject);
}
//
// Private routines
//
VOID
IopResourceRequirementsChanged(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN BOOLEAN StopRequired
    )

/*++

Routine Description:

    This routine handles request of device resource requirements list change.

Parameters:

    PhysicalDeviceObject - Provides a pointer to the PDO who's state is to be invalidated.

    StopRequired - Supplies a BOOLEAN value to indicate if the resources reallocation needs
                   to be done after device stopped.

Return Value:

    none.

--*/

{
    PDEVICE_NODE deviceNode;
    PDEVICE_OBJECT device = NULL;

    PAGED_CODE();

    deviceNode = (PDEVICE_NODE)PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;

    //
    // Clear the NO_RESOURCE_REQUIRED flags.
    //

    deviceNode->Flags &= ~DNF_NO_RESOURCE_REQUIRED;

    //
    // If for some reason this device did not start, we need to clear some flags
    // such that it can be started later.  In this case, we call IopRequestDeviceEnumeration
    // with NULL device object, so the devices will be handled in non-started case.  They will
    // be assigned resources, started and enumerated.
    //

    deviceNode->Flags |= DNF_RESOURCE_REQUIREMENTS_CHANGED;

    IopClearDevNodeProblem(deviceNode);

    //
    // If the device is already started, we call IopRequestDeviceEnumeration with
    // the device object.
    //

    if (deviceNode->Flags & DNF_STARTED) {
        device = PhysicalDeviceObject;
        if (StopRequired == FALSE) {
            deviceNode->Flags |= DNF_NON_STOPPED_REBALANCE;
        } else {

            //
            // Explicitly clear it.
            //

            deviceNode->Flags &= ~DNF_NON_STOPPED_REBALANCE;
        }
    }

    IopRequestDeviceAction( device, ResourceRequirementsChanged, NULL, NULL );
}

VOID
IopInitializePlugPlayNotification(
    VOID
    )

/*++

Routine Description:

    This routine performs initialization required before any of the notification
    APIs can be called.

Parameters:

    None

Return Value:

    None

--*/

{
    ULONG count;

    PAGED_CODE();

    //
    // Initialize the notification structures
    //

    for (count = 0; count < NOTIFY_DEVICE_CLASS_HASH_BUCKETS; count++) {

        InitializeListHead(&IopDeviceClassNotifyList[count]);

    }

    //
    // Initialize the profile notification list
    //
    InitializeListHead(&IopProfileNotifyList);

    //
    // Initialize the deferred registration list
    //
    InitializeListHead(&IopDeferredRegistrationList);

    ExInitializeFastMutex(&IopDeviceClassNotifyLock);
    ExInitializeFastMutex(&IopTargetDeviceNotifyLock);
    ExInitializeFastMutex(&IopHwProfileNotifyLock);
    ExInitializeFastMutex(&IopDeferredRegistrationLock);
}

VOID
IopReferenceNotify(
    PNOTIFY_ENTRY_HEADER Notify
    )

/*++

Routine Description:

    This routine increments the reference count for a notification entry.

Parameters:

    Notify - Supplies a pointer to the notification entry to be referenced

Return Value:

    None

Note:

    The appropriate symchronization lock must be held on the notification
    list before this routine can be called

--*/

{
    PAGED_CODE();

    ASSERT(Notify);
    ASSERT(Notify->RefCount > 0);

    Notify->RefCount++;

}

VOID
IopDereferenceNotify(
    PNOTIFY_ENTRY_HEADER Notify
    )

/*++

Routine Description:

    This routine decrements the reference count for a notification entry, removing
    the entry from the list and freeing the associated memory if there are no
    outstanding reference counts.

Parameters:

    Notify - Supplies a pointer to the notification entry to be referenced

Return Value:

    None

Note:

    The appropriate symchronization lock must be held on the notification
    list before this routine can be called

--*/

{
    PAGED_CODE();

    ASSERT(Notify);
    ASSERT(Notify->RefCount > 0);

    Notify->RefCount--;

    if (Notify->RefCount == 0) {

        //
        // If the refcount is zero then the node should have been deregisterd
        // and is no longer needs to be in the list so remove and free it
        //

        ASSERT(Notify->Unregistered);

        //
        // Dereference the driver object that registered for notifications
        //

        ObDereferenceObject(Notify->DriverObject);

        //
        // If this notification entry is for target device change, dereference
        // the PDO upon which this notification entry was hooked.
        //

        if (Notify->EventCategory == EventCategoryTargetDeviceChange) {
            PTARGET_DEVICE_NOTIFY_ENTRY entry = (PTARGET_DEVICE_NOTIFY_ENTRY)Notify;

            if (entry->PhysicalDeviceObject) {
                ObDereferenceObject(entry->PhysicalDeviceObject);
                entry->PhysicalDeviceObject = NULL;
            }
        }

        RemoveEntryList((PLIST_ENTRY)Notify);

        ExFreePool(Notify);

    }
}

NTSTATUS
IopRequestHwProfileChangeNotification(
    IN   LPGUID                         EventGuid,
    IN   PROFILE_NOTIFICATION_TIME      NotificationTime,
    OUT  PPNP_VETO_TYPE                 VetoType            OPTIONAL,
    OUT  PUNICODE_STRING                VetoName            OPTIONAL
    )

/*++

Routine Description:

    This routine is used to notify all registered drivers of a hardware profile
    change.  If the operation is a HW provile change query then the operation
    is synchronous and the veto information is propagated.  All other operations
    are asynchronous and veto information is not returned.

Parameters:

    EventTypeGuid       - The event that has occured

    NotificationTime    - This is used to tell if we are already in an event
                          when delivering a synchronous notification (ie,
                          querying profile change to eject). It is one of
                          three values:
                              PROFILE_IN_PNPEVENT
                              PROFILE_NOT_IN_PNPEVENT
                              PROFILE_PERHAPS_IN_PNPEVENT

    VetoType            - Type of vetoer.

    VetoName            - Name of vetoer.

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    The contents of the notification structure *including* all pointers is only
    valid during the callback routine to which it was passed.  If the data is
    required after the duration of the callback then it must be physically copied
    by the callback routine.

--*/

{
    NTSTATUS status=STATUS_SUCCESS,completionStatus;
    KEVENT completionEvent;
    ULONG dataSize,totalSize;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;

    PAGED_CODE();

    if (
         (!IopCompareGuid(EventGuid,
                       (LPGUID)&GUID_HWPROFILE_QUERY_CHANGE)) &&
         (!IopCompareGuid(EventGuid,
                       (LPGUID)&GUID_HWPROFILE_CHANGE_CANCELLED)) &&
         (!IopCompareGuid(EventGuid,
                       (LPGUID)&GUID_HWPROFILE_CHANGE_COMPLETE)) ) {

         //
         //  Passed in an illegal value
         //

 #if DBG
         DbgPrint ("Illegal Event type passed as profile notification\n");
 #endif
         return STATUS_INVALID_DEVICE_REQUEST;
     }


     //
     // Only the query changes are synchronous, and in that case we must
     // know definitely whether we are nested within a Pnp event or not.
     //
     ASSERT((!IopCompareGuid(EventGuid, (LPGUID)&GUID_HWPROFILE_QUERY_CHANGE))||
            (NotificationTime != PROFILE_PERHAPS_IN_PNPEVENT)) ;

     if (!IopCompareGuid(EventGuid, (LPGUID)&GUID_HWPROFILE_QUERY_CHANGE) ) {

         //
         // Asynchronous case. Very easy.
         //
         ASSERT(!ARGUMENT_PRESENT(VetoName));
         ASSERT(!ARGUMENT_PRESENT(VetoType));

         return PpSetHwProfileChangeEvent( EventGuid,
                                           NULL,
                                           NULL,
                                           NULL,
                                           NULL
                                           );
     }

     //
     // Query notifications are synchronous. Determine if we are currently
     // within an event, in which case we must do the notify here instead
     // of queueing it up.
     //
     if (NotificationTime == PROFILE_NOT_IN_PNPEVENT) {

         //
         // Queue up and block on the notification.
         //
         KeInitializeEvent(&completionEvent, NotificationEvent, FALSE);

         status = PpSetHwProfileChangeEvent( EventGuid,
                                             &completionEvent,
                                             &completionStatus,
                                             VetoType,
                                             VetoName
                                             );

         if (NT_SUCCESS(status))  {

             KeWaitForSingleObject( &completionEvent, Executive, KernelMode, FALSE, NULL );

             status = completionStatus;
         }

         return status ;
     }

     //
     // Synchronous notify inside our Pnp event.
     //
     // ADRIAO BUGBUG 11/12/98 -
     //     GROSS, UGLY, SINFUL HACK - We are MANUALLY sending the profile
     // query change notification because we are blocking inside a PnPEvent and
     // thus can't queue/wait on another!
     //
     ASSERT(PiNotificationInProgress == TRUE);

     dataSize =  sizeof(PLUGPLAY_EVENT_BLOCK);

     totalSize = dataSize + FIELD_OFFSET (PNP_DEVICE_EVENT_ENTRY,Data);

     deviceEvent = ExAllocatePoolWithTag (PagedPool,
                                          totalSize,
                                          PNP_DEVICE_EVENT_ENTRY_TAG);

     if (NULL == deviceEvent) {
         return STATUS_INSUFFICIENT_RESOURCES;
     }

     //
     //Setup the PLUGPLAY_EVENT_BLOCK
     //
     RtlZeroMemory ((PVOID)deviceEvent,totalSize);
     deviceEvent->Data.EventCategory = HardwareProfileChangeEvent;
     RtlCopyMemory(&deviceEvent->Data.EventGuid, EventGuid, sizeof(GUID));
     deviceEvent->Data.TotalSize = dataSize;
     deviceEvent->CallerEvent = &completionEvent;
     deviceEvent->Data.Result = &completionStatus;
     deviceEvent->VetoType = VetoType;
     deviceEvent->VetoName = VetoName;

     //
     // Notify K-Mode
     //
     status = IopNotifyHwProfileChange(&deviceEvent->Data.EventGuid,
                                       VetoType,
                                       VetoName);

     if (!NT_SUCCESS(status)) {
         return status;
     }

     //
     // Notify user-mode (synchronously).
     //
     status = PiNotifyUserMode(deviceEvent);

     if (!NT_SUCCESS(status)) {
         //
         // Notify K-mode that the query has been cancelled.
         //
         IopNotifyHwProfileChange((LPGUID)&GUID_HWPROFILE_CHANGE_CANCELLED,
                                  NULL,
                                  NULL);
     }
     return status;
}

NTSTATUS
IopNotifyHwProfileChange(
    IN  LPGUID           EventGuid,
    OUT PPNP_VETO_TYPE   VetoType    OPTIONAL,
    OUT PUNICODE_STRING  VetoName    OPTIONAL
    )
/*++

Routine Description:

    This routine is used to deliver the HWProfileNotifications. It is
    called from the worker thread only
    It does not return until all interested parties have been notified.

Parameters:

    EventTypeGuid - The event that has occured

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    The contents of the notification structure *including* all pointers is only
    valid during the callback routine to which it was passed.  If the data is
    required after the duration of the callback then it must be physically copied
    by the callback routine.

--*/
{
    NTSTATUS status=STATUS_SUCCESS;
    PHWPROFILE_NOTIFY_ENTRY  pNotifyList;
    PLIST_ENTRY link;
#if DBG
    ULONG originalApcDisable;
#endif


    PAGED_CODE();

    //Lock the Profile Notification List
    IopAcquireNotifyLock (&IopHwProfileNotifyLock);

    //
    //  Grab the list head (inside the lock)
    //
    link = IopProfileNotifyList.Flink;
    pNotifyList=(PHWPROFILE_NOTIFY_ENTRY)link;

    //
    //circular list
    //
    while (link != (PLIST_ENTRY)&IopProfileNotifyList) {
        if (!pNotifyList->Unregistered) {

            HWPROFILE_CHANGE_NOTIFICATION notification;

            NOTIFICATION_CALLBACK_PARAM_BLOCK callparams;
            ULONG Console=0;

            //
            // Reference the entry so that no one deletes during the callback
            // and then release the lock
            //
            IopReferenceNotify((PNOTIFY_ENTRY_HEADER)pNotifyList);
            IopReleaseNotifyLock(&IopHwProfileNotifyLock);

            //
            // Fill in the notification
            //

            notification.Version = PNP_NOTIFICATION_VERSION;
            notification.Size = sizeof(HWPROFILE_CHANGE_NOTIFICATION);
            notification.Event = *EventGuid;

#if DBG
            originalApcDisable = KeGetCurrentThread()->KernelApcDisable;
#endif
            //
            // Reference the notify and call back
            //
            callparams.Callout=(pNotifyList->Callback);
            callparams.NotificationStructure=&notification;
            callparams.Context=pNotifyList->Context;


            //
            // Dispatch this function via the memory manager.
            // Win32K is a driver that can have multiple copies. If Hydra
            // is running, the Mem. manager will check if the callback exists
            // in "per session" space. If that is the case, it will attach to the
            // console (hence the 3rd param of PULONG containing 0) session and deliver
            // it. If either Hydra is not running, or the callback is outside session space
            // then the callback is called directly.
            //
            status = MmDispatchWin32Callout ((PKWIN32_CALLOUT)callparams.Callout,&IopPnPHydraCallback,&callparams,&Console);
#if DBG
            if (originalApcDisable != KeGetCurrentThread()->KernelApcDisable) {
                DbgPrint("IopNotifyHwProfileChange: Driver %Z, notification handler @ 0x%p returned with different KernelApcDisable = %d, original = %d\n",
                         &pNotifyList->DriverObject->DriverName, pNotifyList->Callback, KeGetCurrentThread()->KernelApcDisable, originalApcDisable);
                DbgBreakPoint();
            }
#endif

            //
            // If the caller returned anything other than success and it was a
            // query hardware profile change, we veto the query and send cancels
            // to all callers that already got the query.
            //

            if (!NT_SUCCESS(status) &&
                IopCompareGuid(EventGuid, (LPGUID)&GUID_HWPROFILE_QUERY_CHANGE)) {

                if (VetoType) {
                    *VetoType = PNP_VetoDriver;
                }
                if (VetoName) {
                    VetoName->Length = 0;
                    RtlCopyUnicodeString(VetoName, &pNotifyList->DriverObject->DriverName);
                }

                notification.Event = GUID_HWPROFILE_CHANGE_CANCELLED;
                notification.Size = sizeof(GUID_HWPROFILE_CHANGE_CANCELLED);

                //
                // Dereference the entry which vetoed the query change.
                //
                IopAcquireNotifyLock(&IopHwProfileNotifyLock);
                IopDereferenceNotify((PNOTIFY_ENTRY_HEADER)pNotifyList);

                do {
                    pNotifyList = (PHWPROFILE_NOTIFY_ENTRY)link;
                    if (!pNotifyList->Unregistered) {
                        IopReferenceNotify((PNOTIFY_ENTRY_HEADER)pNotifyList);
                        IopReleaseNotifyLock(&IopHwProfileNotifyLock);

#if DBG
                        originalApcDisable = KeGetCurrentThread()->KernelApcDisable;
#endif
                        callparams.Callout=(pNotifyList->Callback);
                        callparams.NotificationStructure=&notification;
                        callparams.Context=pNotifyList->Context;

                        MmDispatchWin32Callout ((PKWIN32_CALLOUT)callparams.Callout,&IopPnPHydraCallback,&callparams,&Console);
#if DBG
                        if (originalApcDisable != KeGetCurrentThread()->KernelApcDisable) {
                            DbgPrint("IopNotifyHwProfileChange: Driver %Z, notification handler @ 0x%p returned with different KernelApcDisable = %d, original = %d\n",
                                     &pNotifyList->DriverObject->DriverName, pNotifyList->Callback, KeGetCurrentThread()->KernelApcDisable, originalApcDisable);
                            DbgBreakPoint();
                        }
#endif

                        IopAcquireNotifyLock(&IopHwProfileNotifyLock);
                        link = link->Blink;
                        IopDereferenceNotify((PNOTIFY_ENTRY_HEADER)pNotifyList);

                    } else {
                        link = link->Blink;
                    }
                } while (link != (PLIST_ENTRY)&IopProfileNotifyList);

                goto Clean0;
            }

            //
            // Reacquire the lock, walk forward, and dereference
            //
            IopAcquireNotifyLock (&IopHwProfileNotifyLock);
            link = link->Flink;
            IopDereferenceNotify((PNOTIFY_ENTRY_HEADER)pNotifyList);
            pNotifyList=(PHWPROFILE_NOTIFY_ENTRY)link;

        } else {
            //
            //Walk forward if we hit an unregistered node
            //
            if (pNotifyList) {
                //
                //walk forward
                //
                link = link->Flink;
                pNotifyList=(PHWPROFILE_NOTIFY_ENTRY)link;
            }
        }
    }

 Clean0:

    //UnLock the Profile Notification List
    IopReleaseNotifyLock (&IopHwProfileNotifyLock);

    return status;
}



NTSTATUS
IopNotifyTargetDeviceChange(
    LPCGUID EventGuid,
    PDEVICE_OBJECT DeviceObject,
    PVOID NotificationStructure,
    PDRIVER_OBJECT *VetoingDriver
    )

/*++

Routine Description:

    This routine is used to notify all registered drivers of a change to a
    particular device. It does not return until all interested parties have
    been notified.

Parameters:

    EventGuid - The event that has occured

    DeviceObject - The device object that has changed.  The devnode for this
        device object contains a list of callback routines that have registered
        for notification of any changes on this device object.

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    The contents of the notification structure *including* all pointers is only
    valid during the callback routine to which it was passed.  If the data is
    required after the duration of the callback then it must be physically copied
    by the callback routine.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PLIST_ENTRY link, lastLink;
    PTARGET_DEVICE_NOTIFY_ENTRY entry;
    TARGET_DEVICE_REMOVAL_NOTIFICATION notification;
    PDEVICE_NODE deviceNode;
    BOOLEAN reverse;
#if DBG
    KIRQL originalIrql;
    ULONG originalApcDisable;
#endif

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);
    ASSERT(EventGuid != NULL);

    //
    // Reference the device object so it can't go away while we're doing notification
    //
    ObReferenceObject(DeviceObject);

    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;

    ASSERT(deviceNode != NULL);


    if (NotificationStructure) {
        //
        //We're handling a custom notification
        //

        ((PTARGET_DEVICE_CUSTOM_NOTIFICATION)NotificationStructure)->Version = PNP_NOTIFICATION_VERSION;

    } else {
        //
        // Fill in the notification
        //

        notification.Version = PNP_NOTIFICATION_VERSION;
        notification.Size = sizeof(TARGET_DEVICE_REMOVAL_NOTIFICATION);
        notification.Event = *EventGuid;
    }

    //
    // Lock the notify list
    //

    IopAcquireNotifyLock(&IopTargetDeviceNotifyLock);

    //
    // Get the first entry
    //

    reverse = IopCompareGuid(EventGuid, (LPGUID)&GUID_TARGET_DEVICE_REMOVE_CANCELLED);

    if (reverse) {
        link = deviceNode->TargetDeviceNotify.Blink;
    } else {
        link = deviceNode->TargetDeviceNotify.Flink;
    }

    //
    // Iterate through the list
    //

    while (link != &deviceNode->TargetDeviceNotify) {

        entry = (PTARGET_DEVICE_NOTIFY_ENTRY)link;

        //
        // Only callback on registered nodes
        //

        if (!entry->Unregistered) {

            NOTIFICATION_CALLBACK_PARAM_BLOCK callparams;
            ULONG Console=0;

            //
            // Reference the entry so that no one deletes during the callback
            // and then release the lock
            //

            IopReferenceNotify((PNOTIFY_ENTRY_HEADER)entry);
            IopReleaseNotifyLock(&IopTargetDeviceNotifyLock);

#if DBG
            originalIrql = KeGetCurrentIrql();
            originalApcDisable = KeGetCurrentThread()->KernelApcDisable;
#endif

            //
            // Callback the entity that registered and examine return value
            //

            if (NotificationStructure) {
                TARGET_DEVICE_CUSTOM_NOTIFICATION *notificationStructure = NotificationStructure;

                notificationStructure->FileObject = entry->FileObject;

                callparams.Callout=(entry->Callback);
                callparams.NotificationStructure=NotificationStructure;
                callparams.Context=entry->Context;


                //
                // Dispatch this function via the memory manager.
                // Win32K is a driver that can have multiple copies. If Hydra
                // is running, the Mem. manager will check if the callback exists
                // in "per session" space. If that is the case, it will attach to the
                // console (hence the 3rd param of PULONG containing 0) session and deliver
                // it. If either Hydra is not running, or the callback is outside session space
                // then the callback is called directly.
                //
                status = MmDispatchWin32Callout ((PKWIN32_CALLOUT)callparams.Callout,&IopPnPHydraCallback,&callparams,&Console);


            } else {
                notification.FileObject = entry->FileObject;

                callparams.Callout=(entry->Callback);
                callparams.NotificationStructure=&notification;
                callparams.Context=entry->Context;

                status = MmDispatchWin32Callout ((PKWIN32_CALLOUT)callparams.Callout,&IopPnPHydraCallback,&callparams,&Console);

            }

#if DBG
            if (originalIrql != KeGetCurrentIrql()) {
                DbgPrint("IopNotifyTargetDeviceChange: Driver %Z, notification handler @ 0x%p returned at raised IRQL = %d, original = %d\n",
                         &entry->DriverObject->DriverName, entry->Callback, KeGetCurrentIrql(), originalIrql);
                DbgBreakPoint();
            }
            if (originalApcDisable != KeGetCurrentThread()->KernelApcDisable) {
                DbgPrint("IopNotifyTargetDeviceChange: Driver %Z, notification handler @ 0x%p returned with different KernelApcDisable = %d, original = %d\n",
                         &entry->DriverObject->DriverName, entry->Callback, KeGetCurrentThread()->KernelApcDisable, originalApcDisable);
                DbgBreakPoint();
            }
#endif

            //
            // If the caller returned anything other than success and it was
            // a query remove, we veto the query remove and send cancels to
            // all callers that already got the query remove.
            //

            if (!NT_SUCCESS(status) &&
                IopCompareGuid(EventGuid, (LPGUID)&GUID_TARGET_DEVICE_QUERY_REMOVE)) {

                if (VetoingDriver != NULL) {
                    *VetoingDriver = entry->DriverObject;
                }

                notification.Event = GUID_TARGET_DEVICE_REMOVE_CANCELLED;

                //
                // Dereference the entry which vetoed the query remove.
                //
                IopAcquireNotifyLock(&IopTargetDeviceNotifyLock);
                IopDereferenceNotify((PNOTIFY_ENTRY_HEADER)entry);

                do {
                    entry = (PTARGET_DEVICE_NOTIFY_ENTRY)link;
                    if (!entry->Unregistered) {
                        IopReferenceNotify((PNOTIFY_ENTRY_HEADER)entry);
                        IopReleaseNotifyLock(&IopTargetDeviceNotifyLock);

                        notification.FileObject = entry->FileObject;
#if DBG
                        originalApcDisable = KeGetCurrentThread()->KernelApcDisable;
#endif

                        callparams.Callout=(entry->Callback);
                        callparams.NotificationStructure=&notification;
                        callparams.Context=entry->Context;

                        MmDispatchWin32Callout ((PKWIN32_CALLOUT)callparams.Callout,&IopPnPHydraCallback,&callparams,&Console);

#if DBG
                        if (originalApcDisable != KeGetCurrentThread()->KernelApcDisable) {
                            DbgPrint("IopNotifyTargetDeviceChange: Driver %Z, notification handler @ 0x%p returned with different KernelApcDisable = %d, original = %d\n",
                                     &entry->DriverObject->DriverName, entry->Callback, KeGetCurrentThread()->KernelApcDisable, originalApcDisable);
                            DbgBreakPoint();
                        }
#endif

                        IopAcquireNotifyLock(&IopTargetDeviceNotifyLock);
                        link = link->Blink;
                        IopDereferenceNotify( (PNOTIFY_ENTRY_HEADER) entry );

                    } else {
                        link = link->Blink;
                    }
                } while (link != &deviceNode->TargetDeviceNotify);

                goto Clean0;
            }

            //
            // Reacquire the lock and dereference
            //
            IopAcquireNotifyLock(&IopTargetDeviceNotifyLock);
            if (reverse) {
                link = link->Blink;
            } else {
                link = link->Flink;
            }
            IopDereferenceNotify((PNOTIFY_ENTRY_HEADER)entry);

        } else {

            //
            // Advance down the list
            //
            if (reverse) {
                link = link->Blink;
            } else {
                link = link->Flink;
            }
        }
    }

Clean0:

    //
    // Release the lock and dereference the object
    //

    IopReleaseNotifyLock(&IopTargetDeviceNotifyLock);

    ObDereferenceObject(DeviceObject);

    return status;
}


NTSTATUS
IopNotifyDeviceClassChange(
    LPGUID EventGuid,
    LPGUID ClassGuid,
    PUNICODE_STRING SymbolicLinkName
    )

/*++

Routine Description:

    This routine is used to notify all registered drivers of a changes to a
    particular class of device. It does not return until all interested parties have
    been notified.

Parameters:

    EventTypeGuid - The event that has occured

    ClassGuid - The device class this change has occured in

    SymbolicLinkName - The kernel mode symbolic link name of the interface device
        that changed

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    The contents of the notification structure *including* all pointers is only
    valid during the callback routine to which it was passed.  If the data is
    required after the duration of the callback then it must be physically copied
    by the callback routine.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PLIST_ENTRY link;
    PDEVICE_CLASS_NOTIFY_ENTRY entry;
    DEVICE_INTERFACE_CHANGE_NOTIFICATION notification;
    ULONG hash;
#if DBG
    KIRQL originalIrql;
    ULONG originalApcDisable;
#endif

    PAGED_CODE();

    //
    // Fill in the notification
    //

    notification.Version = PNP_NOTIFICATION_VERSION;
    notification.Size = sizeof(DEVICE_INTERFACE_CHANGE_NOTIFICATION);
    notification.Event = *EventGuid;
    notification.InterfaceClassGuid = *ClassGuid;
    notification.SymbolicLinkName = SymbolicLinkName;

    //
    // Lock the notify list
    //

    IopAcquireNotifyLock(&IopDeviceClassNotifyLock);

    //
    // Get the first entry
    //

    hash = IopHashGuid(ClassGuid);
    link = IopDeviceClassNotifyList[hash].Flink;

    //
    // Iterate through the list
    //

    while (link != &IopDeviceClassNotifyList[hash]) {

        entry = (PDEVICE_CLASS_NOTIFY_ENTRY) link;

        //
        // Only callback on registered nodes of the correct device class
        //

        if ( !entry->Unregistered && IopCompareGuid(&(entry->ClassGuid), ClassGuid) ) {

            NOTIFICATION_CALLBACK_PARAM_BLOCK callparams;
            ULONG Console=0;
            //
            // Reference the entry so that no one deletes during the callback
            // and then release the lock
            //

            IopReferenceNotify( (PNOTIFY_ENTRY_HEADER) entry );
            IopReleaseNotifyLock(&IopDeviceClassNotifyLock);

#if DBG
            originalIrql = KeGetCurrentIrql();
            originalApcDisable = KeGetCurrentThread()->KernelApcDisable;
#endif

            //
            // Callback the entity that registered and ignore the return value as
            // we arn't interested in it
            //

            callparams.Callout=(entry->Callback);
            callparams.NotificationStructure=&notification;
            callparams.Context=entry->Context;


            //
            // Dispatch this function via the memory manager.
            // Win32K is a driver that can have multiple copies. If Hydra
            // is running, the Mem. manager will check if the callback exists
            // in "per session" space. If that is the case, it will attach to the
            // console (hence the 3rd param of PULONG containing 0) session and deliver
            // it. If either Hydra is not running, or the callback is outside session space
            // then the callback is called directly.
            //
            MmDispatchWin32Callout ((PKWIN32_CALLOUT)callparams.Callout,&IopPnPHydraCallback,&callparams,&Console);



#if DBG
            if (originalIrql != KeGetCurrentIrql()) {
                DbgPrint("IopNotifyDeviceClassChange: Driver %Z, notification handler @ 0x%p returned at raised IRQL = %d, original = %d\n",
                         &entry->DriverObject->DriverName, entry->Callback, KeGetCurrentIrql(), originalIrql);
                DbgBreakPoint();
            }
            if (originalApcDisable != KeGetCurrentThread()->KernelApcDisable) {
                DbgPrint("IopNotifyDeviceClassChange: Driver %Z, notification handler @ 0x%p returned with different KernelApcDisable = %d, original = %d\n",
                         &entry->DriverObject->DriverName, entry->Callback, KeGetCurrentThread()->KernelApcDisable, originalApcDisable);
                DbgBreakPoint();
            }
#endif
            //
            // Reacquire the lock and dereference
            //

            IopAcquireNotifyLock(&IopDeviceClassNotifyLock);
            link = link->Flink;
            IopDereferenceNotify( (PNOTIFY_ENTRY_HEADER) entry );

        } else {

            //
            // Advance down the list
            //

            link = link->Flink;
        }
    }

    //
    // Release the lock
    //

    IopReleaseNotifyLock(&IopDeviceClassNotifyLock);

    return status;
}


NTSTATUS
IoRegisterPlugPlayNotification(
    IN IO_NOTIFICATION_EVENT_CATEGORY EventCategory,
    IN ULONG EventCategoryFlags,
    IN PVOID EventCategoryData OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDRIVER_NOTIFICATION_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID Context,
    OUT PVOID *NotificationEntry
    )
/*++

Routine Description:

    IoRegisterPlugPlayNotification provides a mechanism by which WDM drivers may
    receive notification (via callback) for a variety of Plug&Play events.

Arguments:

    EventCategory - Specifies the event category being registered for.  WDM drivers
        may currently register for hard-ware profile changes, device class changes
        (instance arrivals and removals), and target device changes (query-removal,
        cancel-removal, removal-complete, as well as 3rd-party extensible events).

    EventCategoryFlags - Supplies flags that modify the behavior of event registration.
        There is a separate group of flags defined for each event category.  Presently,
        only the interface device change event category has any flags defined:

            DEVICE_CLASS_NOTIFY_FOR_EXISTING_DEVICES -- Drivers wishing to retrieve a
                complete list of all interface devices presently available, and keep
                the list up-to-date (i.e., receive notification of interface device
                arrivals and removals), may specify this flag.  This will cause the
                PnP manager to immediately notify the driver about every currently-existing
                device of the specified interface class.

    EventCategoryData - Used to  'filter' events of the desired category based on the
        supplied criteria.  Not all event categories will use this parameter.  The
        event categories presently defined use this information as fol-lows:

        EventCategoryHardwareProfileChange -- this parameter is unused, and should be NULL.
        EventCategoryDeviceClassChange -- LPGUID representing the interface class of interest
        EventCategoryTargetDeviceChange -- PFILE_OBJECT of interest

    DriverObject - The caller must supply a reference to its driver object (obtained via
        ObReferenceObject), to prevent the driver from being unloaded while registered for
        notification.  The PnP Manager will dereference the driver object when the driver
        unregisters for notification via IoUnregisterPlugPlayNotification).

    CallbackRoutine - Entry point within the driver that the PnP manager should call
        whenever an applicable PnP event occurs.  The entry point must have the
        following prototype:

            typedef
            NTSTATUS
            (*PDRIVER_NOTIFICATION_CALLBACK_ROUTINE) (
                IN PVOID NotificationStructure,
                IN PVOID Context
                );

        where NotificationStructure contains information about the event.  Each event
        GUID within an event category may potentially have its own notification structure
        format, but the buffer must al-ways begin with a PLUGPLAY_NOTIFICATION_HEADER,
        which indicates the size and ver-sion of the structure, as well as the GUID for
        the event.

        The Context parameter provides the callback with the same context data that the
        caller passed in during registration.

    Context - Points to the context data passed to the callback upon event notification.

    NotificationEntry - Upon success, receives a handle representing the notification
        registration.  This handle may be used to unregister for notification via
        IoUnregisterPlugPlayNotification.

--*/
{

    NTSTATUS status = STATUS_SUCCESS;
#if DBG
    ULONG originalApcDisable;
#endif

    PAGED_CODE();

    ASSERT(NotificationEntry);

    //
    // Initialize out parameters
    //

    *NotificationEntry = NULL;

    //
    // Reference the driver object so it doesn't go away while we still have
    // a pointer outstanding
    //
    status = ObReferenceObjectByPointer(DriverObject,
                                        0,
                                        IoDriverObjectType,
                                        KernelMode
                                        );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    switch (EventCategory) {

    case EventCategoryReserved:
        {

            PSETUP_NOTIFY_DATA setupData;

            //
            // Allocate space for the setup data
            //

            setupData = ExAllocatePool(PagedPool, sizeof(SETUP_NOTIFY_DATA));
            if (!setupData) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto clean0;
            }

            //
            // Store the required information
            //
            InitializeListHead(&(setupData->ListEntry));
            setupData->EventCategory = EventCategory;
            setupData->Callback = CallbackRoutine;
            setupData->Context = Context;
            setupData->RefCount = 1;
            setupData->Unregistered = FALSE;
            setupData->Lock = NULL;
            setupData->DriverObject = DriverObject;

            //
            // Activate the notifications
            //

            IopSetupNotifyData = setupData;

            //
            // Explicitly NULL out the returned entry as you can *NOT* unregister
            // for setup notifications
            //

            *NotificationEntry = NULL;

            break;

        }

    case EventCategoryHardwareProfileChange:
        {
            PHWPROFILE_NOTIFY_ENTRY entry;

            //
            // new entry
            //
            entry =ExAllocatePool (PagedPool,sizeof (HWPROFILE_NOTIFY_ENTRY));
            if (!entry) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto clean0;
            }

            //
            // grab the fields
            //

            entry->EventCategory = EventCategory;
            entry->Callback = CallbackRoutine;
            entry->Context = Context;
            entry->RefCount = 1;
            entry->Unregistered = FALSE;
            entry->Lock = &IopHwProfileNotifyLock;
            entry->DriverObject = DriverObject;

            ExAcquireFastMutex(&PiNotificationInProgressLock);
            if (PiNotificationInProgress) {
                //
                // If a notification is in progress, mark the entry as
                // Unregistered until after the current notification is
                // complete.
                //

                PDEFERRED_REGISTRATION_ENTRY deferredNode;

                deferredNode = ExAllocatePool(PagedPool, sizeof(DEFERRED_REGISTRATION_ENTRY));
                if (!deferredNode) {
                    ExReleaseFastMutex(&PiNotificationInProgressLock);
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto clean0;
                }

                deferredNode->NotifyEntry = (PNOTIFY_ENTRY_HEADER)entry;

                //
                // Consider this entry unregistered during the current
                // notification
                //
                entry->Unregistered = TRUE;

                //
                // Reference the entry so that it doesn't go away until it has
                // been removed from the deferred registration list
                //
                IopReferenceNotify((PNOTIFY_ENTRY_HEADER)entry);

                //
                // Add this entry to the deferred registration list
                //
                IopAcquireNotifyLock(&IopDeferredRegistrationLock);
                InsertTailList(&IopDeferredRegistrationList, (PLIST_ENTRY)deferredNode);
                IopReleaseNotifyLock(&IopDeferredRegistrationLock);
            } else {
                //
                // If there is currently no notification in progress, the deferred
                // registration list must be empty.
                //
                ASSERT(IsListEmpty(&IopDeferredRegistrationList));
            }
            ExReleaseFastMutex(&PiNotificationInProgressLock);

            //
            // Lock the list, insert the new entry, and unlock it.
            //

            IopAcquireNotifyLock(&IopHwProfileNotifyLock);
            InsertTailList(&IopProfileNotifyList, (PLIST_ENTRY)entry);
            IopReleaseNotifyLock(&IopHwProfileNotifyLock);

            *NotificationEntry = entry;

            break;
        }
    case EventCategoryTargetDeviceChange:
        {
            PTARGET_DEVICE_NOTIFY_ENTRY entry;
            IO_STACK_LOCATION irpSp;
            PDEVICE_NODE deviceNode;

            ASSERT(EventCategoryData);

            //
            // Allocate a new list entry
            //

            entry = ExAllocatePool(PagedPool, sizeof(TARGET_DEVICE_NOTIFY_ENTRY));
            if (!entry) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto clean0;
            }

            //
            // Retrieve the device object associated with this file handle.
            //
            status = IopGetRelatedTargetDevice((PFILE_OBJECT)EventCategoryData,
                                               &deviceNode);
            if (!NT_SUCCESS(status)) {
                ExFreePool(entry);
                goto clean0;
            }

            //
            // Fill out the entry
            //

            entry->EventCategory = EventCategory;
            entry->Callback = CallbackRoutine;
            entry->Context = Context;
            entry->DriverObject = DriverObject;
            entry->RefCount = 1;
            entry->Unregistered = FALSE;
            entry->Lock = &IopTargetDeviceNotifyLock;
            entry->FileObject = (PFILE_OBJECT)EventCategoryData;

            //
            // The PDO associated with the devnode we got back from
            // IopGetRelatedTargetDevice has already been referenced by that
            // routine.  Store this reference away in the notification entry,
            // so we can deref it later when the notification entry is unregistered.
            //

            ASSERT(deviceNode->PhysicalDeviceObject);
            entry->PhysicalDeviceObject = deviceNode->PhysicalDeviceObject;

            ExAcquireFastMutex(&PiNotificationInProgressLock);
            if (PiNotificationInProgress) {
                //
                // If a notification is in progress, mark the entry as
                // Unregistered until after the current notification is
                // complete.
                //

                PDEFERRED_REGISTRATION_ENTRY deferredNode;

                deferredNode = ExAllocatePool(PagedPool, sizeof(DEFERRED_REGISTRATION_ENTRY));
                if (!deferredNode) {
                    ExReleaseFastMutex(&PiNotificationInProgressLock);
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto clean0;
                }

                deferredNode->NotifyEntry = (PNOTIFY_ENTRY_HEADER)entry;

                //
                // Consider this entry unregistered during the current
                // notification
                //
                entry->Unregistered = TRUE;

                //
                // Reference the entry so that it doesn't go away until it has
                // been removed from the deferred registration list
                //
                IopReferenceNotify((PNOTIFY_ENTRY_HEADER)entry);

                //
                // Add this entry to the deferred registration list
                //
                IopAcquireNotifyLock(&IopDeferredRegistrationLock);
                InsertTailList(&IopDeferredRegistrationList, (PLIST_ENTRY)deferredNode);
                IopReleaseNotifyLock(&IopDeferredRegistrationLock);
            } else {
                //
                // If there is currently no notification in progress, the deferred
                // registration list must be empty.
                //
                ASSERT(IsListEmpty(&IopDeferredRegistrationList));
            }
            ExReleaseFastMutex(&PiNotificationInProgressLock);

            //
            // Lock the list, insert the new entry, and unlock it.
            //

            IopAcquireNotifyLock(&IopTargetDeviceNotifyLock);
            InsertTailList(&deviceNode->TargetDeviceNotify, (PLIST_ENTRY)entry);
            IopReleaseNotifyLock(&IopTargetDeviceNotifyLock);

            *NotificationEntry = entry;
            break;
        }

    case EventCategoryDeviceInterfaceChange:
        {
            PDEVICE_CLASS_NOTIFY_ENTRY entry;

            ASSERT(EventCategoryData);

            //
            // Allocate a new list entry
            //

            entry = ExAllocatePool(PagedPool, sizeof(DEVICE_CLASS_NOTIFY_ENTRY));
            if (!entry) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto clean0;
            }

            //
            // Fill out the entry
            //

            entry->EventCategory = EventCategory;
            entry->Callback = CallbackRoutine;
            entry->Context = Context;
            entry->ClassGuid = *((LPGUID) EventCategoryData);
            entry->RefCount = 1;
            entry->Unregistered = FALSE;
            entry->Lock = &IopDeviceClassNotifyLock;
            entry->DriverObject = DriverObject;

            ExAcquireFastMutex(&PiNotificationInProgressLock);
            if (PiNotificationInProgress) {
                //
                // If a notification is in progress, mark the entry as
                // Unregistered until after the current notification is
                // complete.
                //

                PDEFERRED_REGISTRATION_ENTRY deferredNode;

                deferredNode = ExAllocatePool(PagedPool, sizeof(DEFERRED_REGISTRATION_ENTRY));
                if (!deferredNode) {
                    ExReleaseFastMutex(&PiNotificationInProgressLock);
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto clean0;
                }

                deferredNode->NotifyEntry = (PNOTIFY_ENTRY_HEADER)entry;

                //
                // Consider this entry unregistered during the current
                // notification
                //
                entry->Unregistered = TRUE;

                //
                // Reference the entry so that it doesn't go away until it has
                // been removed from the deferred registration list
                //
                IopReferenceNotify((PNOTIFY_ENTRY_HEADER)entry);

                //
                // Add this entry to the deferred registration list
                //
                IopAcquireNotifyLock(&IopDeferredRegistrationLock);
                InsertTailList(&IopDeferredRegistrationList, (PLIST_ENTRY)deferredNode);
                IopReleaseNotifyLock(&IopDeferredRegistrationLock);
            } else {
                //
                // If there is currently no notification in progress, the deferred
                // registration list must be empty.
                //
                ASSERT(IsListEmpty(&IopDeferredRegistrationList));
            }
            ExReleaseFastMutex(&PiNotificationInProgressLock);

            //
            // Lock the list
            //

            IopAcquireNotifyLock(&IopDeviceClassNotifyLock);

            //
            // Insert it at the tail
            //

            InsertTailList( (PLIST_ENTRY) &(IopDeviceClassNotifyList[ IopHashGuid(&(entry->ClassGuid)) ]),
                            (PLIST_ENTRY) entry
                          );

            //
            // Unlock the list
            //

            IopReleaseNotifyLock(&IopDeviceClassNotifyLock);

            //
            // See if we need to notify for all the device classes already present
            //

            if (EventCategoryFlags & PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES) {

                PWCHAR pSymbolicLinks, pCurrent;
                DEVICE_INTERFACE_CHANGE_NOTIFICATION notification;
                UNICODE_STRING unicodeString;

                //
                // Fill in the notification structure
                //

                notification.Version = PNP_NOTIFICATION_VERSION;
                notification.Size = sizeof(DEVICE_INTERFACE_CHANGE_NOTIFICATION);
                notification.Event = GUID_DEVICE_INTERFACE_ARRIVAL;
                notification.InterfaceClassGuid = entry->ClassGuid;

                //
                // Get the list of all the devices of this function class that are
                // already in the system
                //

                status = IoGetDeviceInterfaces(&(entry->ClassGuid),
                                                NULL,
                                                0,
                                                &pSymbolicLinks
                                                );
                if (!NT_SUCCESS(status)) {
                    //
                    // No buffer will have been returned so just return status
                    //
                    goto clean0;
                }

                //
                // Callback for each device currently in the system
                //

                pCurrent = pSymbolicLinks;
                while(*pCurrent != UNICODE_NULL) {
                    NOTIFICATION_CALLBACK_PARAM_BLOCK callparams;
                    ULONG Console=0;

                    RtlInitUnicodeString(&unicodeString, pCurrent);
                    notification.SymbolicLinkName = &unicodeString;

#if DBG
                    originalApcDisable = KeGetCurrentThread()->KernelApcDisable;
#endif
                    //
                    // Call back on the registered notification routine
                    //
                    callparams.Callout=(*CallbackRoutine);
                    callparams.NotificationStructure=&notification;
                    callparams.Context=Context;


                    //
                    // Dispatch this function via the memory manager.
                    // Win32K is a driver that can have multiple copies. If Hydra
                    // is running, the Mem. manager will check if the callback exists
                    // in "per session" space. If that is the case, it will attach to the
                    // console (hence the 3rd param of PULONG containing 0) session and deliver
                    // it. If either Hydra is not running, or the callback is outside session space
                    // then the callback is called directly.
                    //
                    MmDispatchWin32Callout ((PKWIN32_CALLOUT)callparams.Callout,&IopPnPHydraCallback,&callparams,&Console);

#if DBG
                    if (originalApcDisable != KeGetCurrentThread()->KernelApcDisable) {
                        DbgPrint("IoRegisterPlugPlayNotification: Driver %Z, notification handler @ 0x%p returned with different KernelApcDisable = %d, original = %d\n",
                                 &entry->DriverObject->DriverName, entry->Callback, KeGetCurrentThread()->KernelApcDisable, originalApcDisable);
                        DbgBreakPoint();
                    }
#endif

                    pCurrent += (unicodeString.Length / sizeof(WCHAR)) + 1;

                }

                ExFreePool(pSymbolicLinks);

            }

            *NotificationEntry = entry;
        }

        break;
    }

clean0:

    if (!NT_SUCCESS(status)) {
        ObDereferenceObject(DriverObject);
    }

    return status;
}


NTSTATUS
IopGetRelatedTargetDevice(
    IN PFILE_OBJECT FileObject,
    OUT PDEVICE_NODE *DeviceNode
    )

/*++

Routine Description:

    IopGetRelatedTargetDevice retrieves the device object associated with
    the specified file object and then sends a query device relations irp
    to that device object.

    NOTE: The PDO associated with the returned device node has been referenced,
    and must be dereferenced when no longer needed.

Arguments:

    FileObject - Specifies the file object that is associated with the device
                 object that will receive the query device relations irp.

    DeviceNode - Returns the related target device node.

ReturnValue

    Returns an NTSTATUS value.

--*/

{
    NTSTATUS status;
    IO_STACK_LOCATION irpSp;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_RELATIONS deviceRelations;

    ASSERT(FileObject);

    //
    // Retrieve the device object associated with this file handle.
    //

    deviceObject = IoGetRelatedDeviceObject(FileObject);
    if (!deviceObject) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Query what the "actual" target device node should be for
    // this file object. Initialize the stack location to pass to
    // IopSynchronousCall() and then send the IRP to the device
    // object that's associated with the file handle.
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));
    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
    irpSp.Parameters.QueryDeviceRelations.Type = TargetDeviceRelation;
    irpSp.DeviceObject = deviceObject;
    irpSp.FileObject = FileObject;

    status = IopSynchronousCall(deviceObject, &irpSp, &deviceRelations);
    if (!NT_SUCCESS(status)) {
#if 0
        DbgPrint("PnpMgr: Contact dev owner for %WZ, which may not correctly support\n",
                 &deviceObject->DriverObject->DriverExtension->ServiceKeyName);
        DbgPrint("        IRP_MN_QUERY_DEVICE_RELATIONS:TargetDeviceRelation\n");
        //ASSERT(0);
#endif
        return status;
    }

    ASSERT(deviceRelations);
    ASSERT(deviceRelations->Count == 1);

    *DeviceNode = (PDEVICE_NODE)deviceRelations->Objects[0]->DeviceObjectExtension->DeviceNode;
    if (!*DeviceNode) {
        status = STATUS_NO_SUCH_DEVICE;
    }

    ExFreePool(deviceRelations);
    return status;
}

NTSTATUS
IoGetRelatedTargetDevice(
    IN PFILE_OBJECT FileObject,
    OUT PDEVICE_OBJECT *DeviceObject
    )

/*++

Routine Description:

    IoGetRelatedTargetDevice retrieves the device object associated with
    the specified file object and then sends a query device relations irp
    to that device object.

    NOTE: The PDO associated with the returned device node has been referenced,
    and must be dereferenced when no longer needed.

Arguments:

    FileObject - Specifies the file object that is associated with the device
                 object that will receive the query device relations irp.

    DeviceObject - Returns the related target device object.

ReturnValue

    Returns an NTSTATUS value.

--*/

{
    NTSTATUS status;
    PDEVICE_NODE deviceNode = NULL;

    status = IopGetRelatedTargetDevice( FileObject, &deviceNode );
    if (NT_SUCCESS(status) && deviceNode != NULL) {
        *DeviceObject = deviceNode->PhysicalDeviceObject;
    }
    return status;
}


NTSTATUS
IopNotifySetupDeviceArrival(
    PDEVICE_OBJECT PhysicalDeviceObject,    // PDO of the device
    HANDLE EnumEntryKey,                    // Handle into the enum branch of the registry for this device
    BOOLEAN InstallDriver
    )

/*++

Routine Description:

    This routine is used to notify setup (during text-mode setup) of arrivals
    of a particular device. It does not return until all interested parties have
    been notified.

Parameters:

    PhysicalDeviceObject - Supplies a pointer to the PDO of the newly arrived
        device.

    EnumEntryKey - Supplies a handle to the key associated with the devide under
        the Enum\ branch of the registry.  Can be NULL in which case the key
        will be opened here.

    InstallDriver - Indicates whether setup should attempt to install a driver
                    for this object.  Device objects created through
                    IoReportDetectedDevice() already have a driver but we want
                    to indicate them to setup anyway.

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    The contents of the notification structure *including* all pointers is only
    valid during the callback routine to which it was passed.  If the data is
    required after the duration of the callback then it must be physically copied
    by the callback routine.

--*/

{
    PAGED_CODE();

    //
    // Only perform notifications if someone has registered
    //

    if (IopSetupNotifyData) {

        NTSTATUS status;
        SETUP_DEVICE_ARRIVAL_NOTIFICATION notification;
        NOTIFICATION_CALLBACK_PARAM_BLOCK callparams;
        ULONG Console=0;
        PDEVICE_NODE deviceNode;
        HANDLE enumKey = NULL;

        //
        // Fill in the notification
        //

        if (!EnumEntryKey) {
            status = IopDeviceObjectToDeviceInstance(PhysicalDeviceObject,
                                                     &enumKey,
                                                     KEY_WRITE);
            if (!NT_SUCCESS(status)) {
                return status;
            }
            EnumEntryKey = enumKey;
        }

        notification.Version = PNP_NOTIFICATION_VERSION;
        notification.Size = sizeof(SETUP_DEVICE_ARRIVAL_NOTIFICATION);
        notification.Event = GUID_SETUP_DEVICE_ARRIVAL;
        notification.PhysicalDeviceObject = PhysicalDeviceObject;
        notification.EnumEntryKey = EnumEntryKey;
        deviceNode = (PDEVICE_NODE) PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;
        notification.EnumPath = &deviceNode->InstancePath;
        notification.InstallDriver = InstallDriver;

        //
        // Reference the notify and call back
        //

        callparams.Callout=(IopSetupNotifyData->Callback);
        callparams.NotificationStructure=&notification;
        callparams.Context=IopSetupNotifyData->Context;


        //
        // Dispatch this function via the memory manager.
        // Win32K is a driver that can have multiple copies. If Hydra
        // is running, the Mem. manager will check if the callback exists
        // in "per session" space. If that is the case, it will attach to the
        // console (hence the 3rd param of PULONG containing 0) session and deliver
        // it. If either Hydra is not running, or the callback is outside session space
        // then the callback is called directly.
        //
        status = MmDispatchWin32Callout ((PKWIN32_CALLOUT)callparams.Callout,&IopPnPHydraCallback,&callparams,&Console);
        if (enumKey) {
            ZwClose(enumKey);
        }

        return status;

    } else {

        return STATUS_OBJECT_NAME_NOT_FOUND;

    }
}

BOOLEAN
IoIsWdmVersionAvailable(
    IN UCHAR MajorVersion,
    IN UCHAR MinorVersion
    )

/*++

Routine Description:

    This routine reports whether WDM functionality is available that
    is greater than or equal to the specified major and minor version.

Parameters:

    MajorVersion - Supplies the WDM major version that is required.

    MinorVersion - Supplies the WDM minor version that is required.

Return Value:

    If WDM support is available at _at least_ the requested level, the
    return value is TRUE, otherwise it is FALSE.

--*/

{
    return ((MajorVersion < WDM_MAJORVERSION) ||
            ((MajorVersion == WDM_MAJORVERSION) && (MinorVersion <= WDM_MINORVERSION)));
}

NTKERNELAPI
PDMA_ADAPTER
IoGetDmaAdapter(
    IN PDEVICE_OBJECT PhysicalDeviceObject    OPTIONAL,
    IN PDEVICE_DESCRIPTION DeviceDescription,
    IN OUT PULONG NumberOfMapRegisters
    )
/*++

Routine Description:

    This function returns the appropriate DMA adapter object for the device
    defined in the device description structure.  This code is a wrapper
    which queries the bus interface standard and then calls the returned
    get DMA adapter function.   If an adapter object was not retrieved then
    a legecy function is attempted.

Arguments:

    PhysicalDeviceObject - Optionally, supplies the PDO for the device
        requesting the DMA adapter.  If not supplied, this routine performs the
        function of the non-PnP HalGetDmaAdapter routine.

    DeviceDescriptor - Supplies a description of the deivce.

    NumberOfMapRegisters - Returns the maximum number of map registers which
        may be allocated by the device driver.

Return Value:

    A pointer to the requested adapter object or NULL if an adapter could not
    be created.

--*/

{
    KEVENT event;
    NTSTATUS status;
    PIRP irp;
    IO_STATUS_BLOCK ioStatusBlock;
    PIO_STACK_LOCATION irpStack;
    BUS_INTERFACE_STANDARD busInterface;
    PDMA_ADAPTER dmaAdapter = NULL;
    PDEVICE_DESCRIPTION deviceDescriptionToUse;
    DEVICE_DESCRIPTION privateDeviceDescription;
    ULONG resultLength;
    PDEVICE_OBJECT targetDevice;

    PAGED_CODE();

    if (PhysicalDeviceObject != NULL) {

        ASSERT_PDO(PhysicalDeviceObject);

        //
        // First off, determine whether or not the caller has requested that we
        // automatically fill in the proper InterfaceType value into the
        // DEVICE_DESCRIPTION structure used in retrieving the DMA adapter object.
        // If so, then retrieve that interface type value into our own copy of
        // the DEVICE_DESCRIPTION buffer.
        //
        if ((DeviceDescription->InterfaceType == InterfaceTypeUndefined) ||
            (DeviceDescription->InterfaceType == PNPBus)) {
            //
            // Make a copy of the caller-supplied device description, so
            // we can modify it to fill in the correct interface type.
            //
            RtlCopyMemory(&privateDeviceDescription,
                          DeviceDescription,
                          sizeof(DEVICE_DESCRIPTION)
                         );

            status = IoGetDeviceProperty(PhysicalDeviceObject,
                                         DevicePropertyLegacyBusType,
                                         sizeof(privateDeviceDescription.InterfaceType),
                                         (PVOID)&(privateDeviceDescription.InterfaceType),
                                         &resultLength
                                        );

            if (!NT_SUCCESS(status)) {

                ASSERT(status == STATUS_OBJECT_NAME_NOT_FOUND);

                //
                // Since the enumerator didn't tell us what interface type to
                // use for this PDO, we'll fall back to our default.  This is
                // ISA for machines where the legacy bus is ISA or EISA, and it
                // is MCA for machines whose legacy bus is MicroChannel.
                //
                privateDeviceDescription.InterfaceType = PnpDefaultInterfaceType;
            }

            //
            // Use our private device description buffer from now on.
            //
            deviceDescriptionToUse = &privateDeviceDescription;

        } else {
            //
            // Use the caller-supplied device description.
            //
            deviceDescriptionToUse = DeviceDescription;
        }

        //
        // Now, query for the BUS_INTERFACE_STANDARD interface from the PDO.
        //
        KeInitializeEvent( &event, NotificationEvent, FALSE );

        targetDevice = IoGetAttachedDeviceReference(PhysicalDeviceObject);

        irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP,
                                            targetDevice,
                                            NULL,
                                            0,
                                            NULL,
                                            &event,
                                            &ioStatusBlock );

        if (irp == NULL) {
            return NULL;
        }

        RtlZeroMemory( &busInterface, sizeof( BUS_INTERFACE_STANDARD ));

        irpStack = IoGetNextIrpStackLocation( irp );
        irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
        irpStack->Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_BUS_INTERFACE_STANDARD;
        irpStack->Parameters.QueryInterface.Size = sizeof( BUS_INTERFACE_STANDARD );
        irpStack->Parameters.QueryInterface.Version = 1;
        irpStack->Parameters.QueryInterface.Interface = (PINTERFACE) &busInterface;
        irpStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

        //
        // Initialize the status to error in case the ACPI driver decides not to
        // set it correctly.
        //

        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        status = IoCallDriver(targetDevice, irp);

        if (status == STATUS_PENDING) {

            KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
            status = ioStatusBlock.Status;

        }

        ObDereferenceObject(targetDevice);

        if (NT_SUCCESS( status)) {

            if (busInterface.GetDmaAdapter != NULL) {


                dmaAdapter = busInterface.GetDmaAdapter( busInterface.Context,
                                                         deviceDescriptionToUse,
                                                         NumberOfMapRegisters );

            }

            //
            // Dereference the interface
            //

            busInterface.InterfaceDereference( busInterface.Context );
        }

    } else {
        //
        // The caller didn't specify the PDO, so we'll just use the device
        // description exactly as they specified it (i.e., we can't attempt to
        // make our own determination of what interface type to use).
        //
        deviceDescriptionToUse = DeviceDescription;
    }

#if !defined(NO_LEGACY_DRIVERS)
    //
    // If there is no DMA adapter, try the legacy mode code.
    //

    if (dmaAdapter == NULL) {

        dmaAdapter = HalGetDmaAdapter( PhysicalDeviceObject,
                                       deviceDescriptionToUse,
                                       NumberOfMapRegisters );

    }
#endif // NO_LEGACY_DRIVERS

    return( dmaAdapter );
}

NTSTATUS
IopOpenDeviceParametersSubkey(
    OUT HANDLE *ParamKeyHandle,
    IN  HANDLE ParentKeyHandle,
    IN  PUNICODE_STRING SubKeyString,
    IN  ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This routine reports whether WDM functionality is available that
    is greater than or equal to the specified major and minor version.

Parameters:

    MajorVersion - Supplies the WDM major version that is required.

    MinorVersion - Supplies the WDM minor version that is required.

Return Value:

    If WDM support is available at _at least_ the requested level, the
    return value is TRUE, otherwise it is FALSE.

--*/

{
    NTSTATUS                    status;
    ULONG                       disposition;
    ULONG                       lengthSD;
    PSECURITY_DESCRIPTOR        oldSD = NULL;
    SECURITY_DESCRIPTOR         newSD;
    ACL_SIZE_INFORMATION        aclSizeInfo;
    PACL                        oldDacl;
    PACL                        newDacl = NULL;
    ULONG                       sizeDacl;
    BOOLEAN                     daclPresent, daclDefaulted;
    PACCESS_ALLOWED_ACE         ace;
    ULONG                       aceIndex;
    HANDLE                      deviceKeyHandle;
    UNICODE_STRING              deviceParamString;

    //
    // First try and open the device key
    //
    status = IopOpenRegistryKeyEx( &deviceKeyHandle,
                                   ParentKeyHandle,
                                   SubKeyString,
                                   KEY_WRITE
                                   );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    RtlInitUnicodeString(&deviceParamString, REGSTR_KEY_DEVICEPARAMETERS);

    status = IopCreateRegistryKeyEx( ParamKeyHandle,
                                     deviceKeyHandle,
                                     &deviceParamString,
                                     DesiredAccess | READ_CONTROL | WRITE_DAC,
                                     REG_OPTION_NON_VOLATILE,
                                     &disposition
                                     );

    ZwClose(deviceKeyHandle);

    if (!NT_SUCCESS(status)) {
        KdPrint(("IopOpenDeviceParametersSubkey: IopCreateRegistryKeyEx failed, status = %8.8X\n", status));
        return status;
    }

    if (disposition == REG_CREATED_NEW_KEY) {

        //
        // Need to set an ACL on the key if it was created
        //
        //
        // Get the security descriptor from the key so we can add the
        // administrator.
        //
        status = ZwQuerySecurityObject(*ParamKeyHandle,
                                       DACL_SECURITY_INFORMATION,
                                       NULL,
                                       0,
                                       &lengthSD);

        if (status == STATUS_BUFFER_TOO_SMALL) {
            oldSD = ExAllocatePool( PagedPool, lengthSD );

            if (oldSD != NULL) {

                status = ZwQuerySecurityObject(*ParamKeyHandle,
                                               DACL_SECURITY_INFORMATION,
                                               oldSD,
                                               lengthSD,
                                               &lengthSD);
                if (!NT_SUCCESS(status)) {
                    KdPrint(("IopOpenDeviceParametersSubkey: ZwQuerySecurityObject failed, status = %8.8X\n", status));
                    goto Cleanup0;
                }
            } else  {

                KdPrint(("IopOpenDeviceParametersSubkey: Failed to allocate memory, status = %8.8X\n", status));
                status = STATUS_NO_MEMORY;
                goto Cleanup0;
            }
        } else {
           KdPrint(("IopOpenDeviceParametersSubkey: ZwQuerySecurityObject failed %8.8X\n",status));
           status = STATUS_UNSUCCESSFUL;
           goto Cleanup0;
        }

        status = RtlCreateSecurityDescriptor( (PSECURITY_DESCRIPTOR) &newSD,
                                              SECURITY_DESCRIPTOR_REVISION );
        ASSERT( NT_SUCCESS( status ) );

        if (!NT_SUCCESS(status)) {

            KdPrint(("IopOpenDeviceParametersSubkey: RtlCreateSecurityDescriptor failed, status = %8.8X\n", status));
            goto Cleanup0;
        }
        //
        // get the current DACL
        //
        status = RtlGetDaclSecurityDescriptor(oldSD, &daclPresent, &oldDacl, &daclDefaulted);

        ASSERT( NT_SUCCESS( status ) );

        //
        // calculate the size of the new DACL
        //

        if (daclPresent) {

            status = RtlQueryInformationAcl( oldDacl,
                                             &aclSizeInfo,
                                             sizeof(ACL_SIZE_INFORMATION),
                                             AclSizeInformation);


            if (!NT_SUCCESS(status)) {

                KdPrint(("IopOpenDeviceParametersSubkey: RtlQueryInformationAcl failed, status = %8.8X\n", status));
                goto Cleanup0;
            }

            sizeDacl = aclSizeInfo.AclBytesInUse;
        } else {
            sizeDacl = sizeof(ACL);
        }

        sizeDacl += sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(SeAliasAdminsSid) - sizeof(ULONG);

        //
        // create and initialize the new DACL
        //
        newDacl = ExAllocatePool(PagedPool, sizeDacl);

        if (newDacl == NULL) {

            KdPrint(("IopOpenDeviceParametersSubkey: ExAllocatePool failed\n"));
            goto Cleanup0;
        }

        status = RtlCreateAcl(newDacl, sizeDacl, ACL_REVISION);

        if (!NT_SUCCESS(status)) {

            KdPrint(("IopOpenDeviceParametersSubkey: RtlCreateAcl failed, status = %8.8X\n", status));
            goto Cleanup0;
        }

        //
        // copy the current (original) DACL into this new one
        //
        if (daclPresent) {

            for (aceIndex = 0; aceIndex < aclSizeInfo.AceCount; aceIndex++) {

                status = RtlGetAce(oldDacl, aceIndex, (PVOID *)&ace);

                if (!NT_SUCCESS(status)) {

                    KdPrint(("IopOpenDeviceParametersSubkey: RtlGetAce failed, status = %8.8X\n", status));
                    goto Cleanup0;
                }

                //
                // We need to skip copying any ACEs which refer to the Administrator
                // to ensure that our full control ACE is the one and only.
                //
                if ((ace->Header.AceType != ACCESS_ALLOWED_ACE_TYPE &&
                     ace->Header.AceType != ACCESS_DENIED_ACE_TYPE) ||
                     !RtlEqualSid((PSID)&ace->SidStart, SeAliasAdminsSid)) {

                    status = RtlAddAce( newDacl,
                                        ACL_REVISION,
                                        ~0U,
                                        ace,
                                        ace->Header.AceSize
                                        );

                    if (!NT_SUCCESS(status)) {

                        KdPrint(("IopOpenDeviceParametersSubkey: RtlAddAce failed, status = %8.8X\n", status));
                        goto Cleanup0;
                    }
                }
            }
        }

        //
        // and my new admin-full ace to this new DACL
        //
        status = RtlAddAccessAllowedAceEx( newDacl,
                                           ACL_REVISION,
                                           CONTAINER_INHERIT_ACE,
                                           KEY_ALL_ACCESS,
                                           SeAliasAdminsSid
                                           );
        if (!NT_SUCCESS(status)) {

            KdPrint(("IopOpenDeviceParametersSubkey: RtlAddAccessAllowedAceEx failed, status = %8.8X\n", status));
            goto Cleanup0;
        }

        //
        // Set the new DACL in the absolute security descriptor
        //
        status = RtlSetDaclSecurityDescriptor( (PSECURITY_DESCRIPTOR) &newSD,
                                               TRUE,
                                               newDacl,
                                               FALSE
                                               );

        if (!NT_SUCCESS(status)) {

            KdPrint(("IopOpenDeviceParametersSubkey: RtlSetDaclSecurityDescriptor failed, status = %8.8X\n", status));
            goto Cleanup0;
        }

        //
        // validate the new security descriptor
        //
        status = RtlValidSecurityDescriptor(&newSD);

        if (!NT_SUCCESS(status)) {

            KdPrint(("IopOpenDeviceParametersSubkey: RtlValidSecurityDescriptor failed, status = %8.8X\n", status));
            goto Cleanup0;
        }


        status = ZwSetSecurityObject( *ParamKeyHandle,
                                      DACL_SECURITY_INFORMATION,
                                      &newSD
                                      );
        if (!NT_SUCCESS(status)) {

            KdPrint(("IopOpenDeviceParametersSubkey: ZwSetSecurityObject failed, status = %8.8X\n", status));
            goto Cleanup0;
        }
    }

    //
    // If we encounter an error updating the DACL we still return success.
    //

Cleanup0:

    if (oldSD != NULL) {
        ExFreePool(oldSD);
    }

    if (newDacl != NULL) {
        ExFreePool(newDacl);
    }

    return STATUS_SUCCESS;
}

#if 0

NTSTATUS
IopSetupDeviceObjectFromDeviceClass(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN HANDLE DeviceClassKey
    )
/*++

Routine Description:

    This routine will migrate device object settings from the device class
    key in the registry to the physical device object.  The possible settings
    are:

        * DeviceType - the I/O system type for the device object
        * DeviceCharacteristics - the I/O system characteristic flags to be
                                  set for the device object
        * DefaultAcl - the default ACL for the device object.  This can be
                       overridden by setting a device specific ACL in the
                       device node in the registry.

    The routine will then use the DeviceType and DeviceCharacteristics specified
    to determine whether a VPB should be allocated as well as to set default
    security if none is specified in the registry.

Arguments:

    PhysicalDeviceObject - the PDO we are to configure

    DeviceClassKey - a handle to the this particular device class's information
                     in the Control\DeviceClasses section of the registry.

Return Value:

    status

--*/

{
    PDEVICE_OBJECT topOfStack = IoGetAttachedDevice(PhysicalDeviceObject);
    UNICODE_STRING valueName;

    DEVICE_TYPE deviceType = PhysicalDeviceObject->DeviceType;
    ULONG characteristics = PhysicalDeviceObject->Characteristics;

    BOOLEAN deviceTypeChanged;

    PKEY_VALUE_FULL_INFORMATION info;
    PUCHAR data;

    NTSTATUS status;

    //
    // First read from the registry to find the device type and characteristics
    // values.
    //

    RtlInitUnicodeString(&valueName, REGSTR_VAL_DEVICE_TYPE);
    status = IopGetRegistryValue(DeviceClassKey,
                                 valueName.Buffer,
                                 &info);

    if(NT_SUCCESS(status)) {
        data = ((PUCHAR) info) + info->DataOffset;
        deviceType = *((PULONG) data);
        ExFreePool(info);
    }

    RtlInitUnicodeString(&valueName, REGSTR_VAL_DEVICE_CHARACTERISTICS);
    status = IopGetRegistryValue(DeviceClassKey,
                                 valueName.Buffer,
                                 &info);

    if(NT_SUCCESS(status)) {
        data = ((PUCHAR) info) + info->DataOffset;
        characteristics = *((PULONG) data);

        ExFreePool(info);
    }

    //
    // Assign the device type to the PDO and OR in the characteristics bits
    // with the ones already set.
    //

    //
    // Make sure no one registered for two incompatible device classes.
    //

    ASSERT((PhysicalDeviceObject->DeviceType == FILE_DEVICE_PNP_WITH_INTERFACE) ||
           (PhysicalDeviceObject->DeviceType == deviceType));

    if(PhysicalDeviceObject->DeviceType != deviceType) {
        deviceTypeChanged = TRUE;
    }

    PhysicalDeviceObject->DeviceType = deviceType;
    PhysicalDeviceObject->Characteristics |= characteristics;

    //
    // Propagate the device type to the top of the stack too.
    //

    topOfStack->DeviceType = deviceType;
    topOfStack->Characteristics |= characteristics;

    //
    // Setup security on the PDO now if the device type was changed.
    //

    if(deviceTypeChanged) {

        status = IopSetSecurityObjectFromRegistry(PhysicalDeviceObject,
                                                  DeviceClassKey);

        if(!NT_SUCCESS(status)) {

            BOOLEAN hasName;
            CHAR buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
            PSECURITY_DESCRIPTOR securityDescriptor;
            SECURITY_INFORMATION securityInformation = 0;

            //
            // Security couldn't be set from the registry.  Build the default
            // security descriptor for this object and we'll set that instead.
            //

            hasName = ((PhysicalDeviceObject->Flags & DO_DEVICE_HAS_NAME) ==
                       DO_DEVICE_HAS_NAME);

            securityDescriptor =
                IopCreateDefaultDeviceSecurityDescriptor(
                    deviceType,
                    PhysicalDeviceObject->Characteristics,
                    hasName,
                    buffer,
                    &securityInformation
                    );

            if(securityDescriptor != NULL) {

                SECURITY_INFORMATION securityInformation = 0;


                status = ObSetSecurityObjectByPointer(PhysicalDeviceObject,
                                                      securityInformation,
                                                      securityDescriptor);
            }
        }

        //
        // If the device is a mass storage type device then we should mark
        // it for VPB allocation later on down the line.
        //

        if((deviceType == FILE_DEVICE_DISK) ||
           (deviceType == FILE_DEVICE_CD_ROM) ||
           (deviceType == FILE_DEVICE_TAPE) ||
           (deviceType == FILE_DEVICE_VIRTUAL_DISK)) {

            IopCreateVpb(PhysicalDeviceObject);
            KeInitializeEvent( &PhysicalDeviceObject->DeviceLock,
                               SynchronizationEvent,
                               TRUE );
            PoVolumeDevice(PhysicalDeviceObject);
        }
    }

    return status;
}
#endif


NTSTATUS
IopSetSecurityObjectFromRegistry(
    IN PVOID Object,
    IN HANDLE Key
    )
/*++

Routine Description:

    This routine will read in the security information stored in the specified
    registry key and assign that security to the specified object.  If the
    registry key contains a SecurityDescriptor value it will be validated and
    assigned to the object if valid.

Arguments:

    Object - the object to be secured.

    Key - the registry key these values are saved in.

Return Value:

    status

--*/

{
    UNICODE_STRING valueName;

    PKEY_VALUE_FULL_INFORMATION info = NULL;

    PSECURITY_DESCRIPTOR descriptor;
    PSECURITY_DESCRIPTOR capturedDescriptor;

    NTSTATUS status;

    RtlInitUnicodeString(&valueName, REGSTR_VAL_DEVICE_SECURITY_DESCRIPTOR);

    status = IopGetRegistryValue(Key,
                                 valueName.Buffer,
                                 &info);

    if(!NT_SUCCESS(status)) {
        return status;
    }

    descriptor = (((PUCHAR) info) + info->DataOffset);

    status = SeCaptureSecurityDescriptor(descriptor,
                                         UserMode,
                                         PagedPool,
                                         FALSE,
                                         &capturedDescriptor);

    ExFreePool(descriptor);

    if(!NT_SUCCESS(status)) {
        return status;
    }

    try {

        SECURITY_INFORMATION securityInformation;

        PSID sid;
        PACL acl;
        BOOLEAN present, tmp;

        RtlZeroMemory(&securityInformation, sizeof(securityInformation));

        //
        // See what information is in the captured descriptor so we can build
        // up a securityInformation block to go with it.
        //

        status = RtlGetOwnerSecurityDescriptor(capturedDescriptor, &sid, &tmp);

        if(NT_SUCCESS(status) && (sid != NULL)) {
            securityInformation |= OWNER_SECURITY_INFORMATION;
        }

        status = RtlGetGroupSecurityDescriptor(capturedDescriptor, &sid, &tmp);

        if(NT_SUCCESS(status) && (sid != NULL)) {
            securityInformation |= GROUP_SECURITY_INFORMATION;
        }

        status = RtlGetSaclSecurityDescriptor(capturedDescriptor,
                                              &present,
                                              &acl,
                                              &tmp);

        if(NT_SUCCESS(status) && (present)) {
            securityInformation |= SACL_SECURITY_INFORMATION;
        }

        status = RtlGetDaclSecurityDescriptor(capturedDescriptor,
                                              &present,
                                              &acl,
                                              &tmp);

        if(NT_SUCCESS(status) && (present)) {
            securityInformation |= DACL_SECURITY_INFORMATION;
        }

        status = ObSetSecurityObjectByPointer(Object,
                                              securityInformation,
                                              capturedDescriptor);
    } finally {
        SeReleaseSecurityDescriptor(capturedDescriptor,
                                    UserMode,
                                    FALSE);
    }

    return status;
}


NTSTATUS
PpCreateLegacyDeviceIds(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING DriverName,
    IN PCM_RESOURCE_LIST Resources
    )
{
    PIOPNP_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PWCHAR buffer;

    ULONG length = 0;

    INTERFACE_TYPE interface;
    PWCHAR interfaceNames[] ={L"",
                              L"Internal",
                              L"Isa",
                              L"Eisa",
                              L"MicroChannel",
                              L"TurboChannel",
                              L"PCIBus",
                              L"VMEBus",
                              L"NuBus",
                              L"PCMCIABus",
                              L"CBus",
                              L"MPIBus",
                              L"MPSABus",
                              L"ProcessorInternal",
                              L"InternalPowerBus",
                              L"PNPISABus",
                              L"PNPBus",
                              L"Other",
                              L"Root"};


     PAGED_CODE();

    if(Resources != NULL) {

        interface = Resources->List[0].InterfaceType;

        if((interface > MaximumInterfaceType) ||
           (interface < InterfaceTypeUndefined)) {
            interface = MaximumInterfaceType;
        }
    } else {
        interface = Internal;
    }

    interface++;

    //
    // The compatible ID generated will be
    // DETECTED<InterfaceName>\<Driver Name>
    //

    length = wcslen(LEGACY_COMPATIBLE_ID_BASE) * sizeof(WCHAR);
    length += wcslen(interfaceNames[interface]) * sizeof(WCHAR);
    length += sizeof(L'\\');
    length += DriverName->Length;
    length += sizeof(UNICODE_NULL);

    length += wcslen(LEGACY_COMPATIBLE_ID_BASE) * sizeof(WCHAR);
    length += sizeof(L'\\');
    length += DriverName->Length;
    length += sizeof(UNICODE_NULL) * 2;

    buffer = ExAllocatePool(PagedPool, length);
    deviceExtension->CompatibleIdList = buffer;

    if(buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(buffer, length);

    swprintf(buffer, L"%ws%ws\\%wZ", LEGACY_COMPATIBLE_ID_BASE,
                                     interfaceNames[interface],
                                     DriverName);

    //
    // Adjust the buffer to point to the end and generate the second
    // compatible id string.
    //

    buffer += wcslen(buffer) + 1;

    swprintf(buffer, L"%ws\\%wZ", LEGACY_COMPATIBLE_ID_BASE, DriverName);

    deviceExtension->CompatibleIdListSize = length;

    return STATUS_SUCCESS;
}


NTSTATUS
IoNotifyPowerOperationVetoed(
    IN POWER_ACTION             VetoedPowerOperation,
    IN PDEVICE_OBJECT           TargetedDeviceObject    OPTIONAL,
    IN PDEVICE_OBJECT           VetoingDeviceObject
    )
/*++

--*/
{
    PDEVICE_NODE deviceNode, vetoingDeviceNode;
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();

    //
    // We have to types of power events, system wide (standby) and device
    // targetted (warm eject). Rather than have two different veto mechanisms,
    // we just retarget the operation against the root device if none is
    // specified (hey, someone's gotta represent the system, right?).
    //
    if (TargetedDeviceObject) {

        deviceObject = TargetedDeviceObject;

    } else {

        deviceObject = IopRootDeviceNode->PhysicalDeviceObject;
    }

    deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;
    if (!deviceNode) {
        return STATUS_INVALID_PARAMETER_2;
    }

    vetoingDeviceNode = (PDEVICE_NODE)VetoingDeviceObject->DeviceObjectExtension->DeviceNode;
    if (!vetoingDeviceNode) {
        return STATUS_INVALID_PARAMETER_3;
    }

    return PpSetPowerVetoEvent(
        VetoedPowerOperation,
        NULL,
        NULL,
        deviceObject,
        PNP_VetoDevice,
        &vetoingDeviceNode->InstancePath
        );
}

ULONG
IoPnPDeliverServicePowerNotification(
    ULONG   PwrNotification,
    BOOLEAN Synchronous
    )
{

    NTSTATUS status = STATUS_SUCCESS;
    KEVENT completionEvent;
    NTSTATUS completionStatus=STATUS_SUCCESS;
    PNP_VETO_TYPE vetoType = PNP_VetoTypeUnknown;
    UNICODE_STRING vetoName;

    PAGED_CODE();

#define MAX_VETO_NAME_LENGTH    512 //From Revent.c make it common

    if (Synchronous) {



        vetoName.Buffer = ExAllocatePool (PagedPool,MAX_VETO_NAME_LENGTH*sizeof (WCHAR));

        if (vetoName.Buffer) {
            vetoName.MaximumLength = MAX_VETO_NAME_LENGTH;
        }else {
            vetoName.MaximumLength = 0;
        }
        vetoName.Length = 0;

        KeInitializeEvent(&completionEvent, NotificationEvent, FALSE);

        status = PpSetPowerEvent(
            PwrNotification,
            &completionEvent,
            &completionStatus,
            &vetoType,&vetoName
            );

        if (NT_SUCCESS(status))  {

            KeWaitForSingleObject( &completionEvent, Executive, KernelMode, FALSE, NULL );
            status = completionStatus;

            if (vetoType == PNP_VetoWindowsService) {
                IoRaiseInformationalHardError (STATUS_DRIVER_FAILED_SLEEP,&vetoName,NULL);
            }

        }
        if (vetoName.Buffer) {
            ExFreePool (vetoName.Buffer);
        }

    } else {
        status = PpSetPowerEvent(
            PwrNotification,
            NULL,
            NULL,
            NULL,
            NULL
            );
    }

    ASSERT ((completionStatus == STATUS_SUCCESS) ||
            (completionStatus == STATUS_UNSUCCESSFUL));
    //
    // The private code in Win32k that calls this, assumes that 0 is failure, !0 is success
    //

    return (completionStatus != STATUS_UNSUCCESSFUL);

}


//
// Release the references to the device object for all the notifications entries
// of a device object. Then fixup the notification node to not point to a physical
// device object. The notification node will be released when IoUnregisterPlugPlayNotification
// is actually called, but the device object will already be gone.
//
VOID
IopOrphanNotification(
    IN PDEVICE_NODE TargetNode
    )
{
    PTARGET_DEVICE_NOTIFY_ENTRY entry;
    PFAST_MUTEX lock;

    IopAcquireNotifyLock(&IopTargetDeviceNotifyLock);

    while (!IsListEmpty(&TargetNode->TargetDeviceNotify)) {
        entry = (PTARGET_DEVICE_NOTIFY_ENTRY)RemoveHeadList(&TargetNode->TargetDeviceNotify);
        if (entry->EventCategory == EventCategoryTargetDeviceChange) {

            if (entry->PhysicalDeviceObject) {
                ObDereferenceObject(entry->PhysicalDeviceObject);
                entry->PhysicalDeviceObject = NULL;
            }
        }
    }

    IopReleaseNotifyLock(&IopTargetDeviceNotifyLock);
}


//
// This is the dispatch routine for plug and play notifications on the
// "far side" of the memory manager. If this notification is destined for
// a routine in "session space" the mmgr will have attached us to the session.
// Otherwise we just dispatch in the context of the system process
//

NTSTATUS
IopPnPHydraCallback (
    PVOID CallbackParams
    )
{
    PNOTIFICATION_CALLBACK_PARAM_BLOCK params=(PNOTIFICATION_CALLBACK_PARAM_BLOCK)CallbackParams;
    NTSTATUS status;

    status = (params->Callout)(params->NotificationStructure,params->Context);

    return status;
}

//
// An IO_GET_LEGACY_VETO_LIST_CONTEXT structure.
//

typedef struct {
    PWSTR *                     VetoList;
    ULONG                       VetoListLength;
    PPNP_VETO_TYPE              VetoType;
    NTSTATUS *                  Status;
} IO_GET_LEGACY_VETO_LIST_CONTEXT, *PIO_GET_LEGACY_VETO_LIST_CONTEXT;

BOOLEAN
IopAppendLegacyVeto(
    IN PIO_GET_LEGACY_VETO_LIST_CONTEXT Context,
    IN PUNICODE_STRING VetoName
    )
/*++

Routine Description:

    This routine appends a veto (driver name or device instance path) to the
    veto list.

Parameters:

    Context - An IO_GET_LEGACY_VETO_LIST_CONTEXT pointer.

    VetoName - The name of the driver/device to append to the veto list.

ReturnValue:

    A BOOLEAN which indicates whether the append operation was successful.

--*/
{
    ULONG Length;
    PWSTR Buffer;

    //
    // Compute the length of the (new) veto list.  This is the length of
    // the old veto list + the size of the new veto + the size of the
    // terminating '\0'.
    //

    Length = Context->VetoListLength + VetoName->Length + sizeof (WCHAR);

    //
    // Allocate the new veto list.
    //

    Buffer = ExAllocatePool(
                 NonPagedPool,
                 Length
             );

    //
    // If we succeeded in allocating the new veto list, copy the old
    // veto list to the new list, append the new veto, and finally,
    // append a terminating '\0'.  Otherwise, update the status to
    // indicate an error; IopGetLegacyVetoList will free the veto list
    // before it returns.
    //

    if (Buffer != NULL) {

        if (*Context->VetoList != NULL) {

            RtlMoveMemory(
                Buffer,
                *Context->VetoList,
                Context->VetoListLength
            );

            ExFreePool(*Context->VetoList);

        }

        RtlMoveMemory(
            &Buffer[Context->VetoListLength / sizeof (WCHAR)],
            VetoName->Buffer,
            VetoName->Length
        );

        Buffer[Length / sizeof (WCHAR) - 1] = L'\0';

        *Context->VetoList = Buffer;
        Context->VetoListLength = Length;

        return TRUE;

    } else {

        *Context->Status = STATUS_INSUFFICIENT_RESOURCES;

        return FALSE;

    }
}

BOOLEAN
IopGetLegacyVetoListDevice(
    IN PDEVICE_NODE DeviceNode,
    IN PIO_GET_LEGACY_VETO_LIST_CONTEXT Context
    )
/*++

Routine Description:

    This routine determines whether the specified device node should be added to
    the veto list, and if so, calls IopAppendLegacyVeto to add it.

Parameters:

    DeviceNode - The device node to be added.

    Context - An IO_GET_LEGACY_VETO_LIST_CONTEXT pointer.

ReturnValue:

    A BOOLEAN value which indicates whether the device node enumeration
    process should be terminated or not.

--*/
{
    PDEVICE_CAPABILITIES DeviceCapabilities;

    //
    // A device node should be added added to the veto list, if it has the
    // NonDynamic capability.
    //

    DeviceCapabilities = IopDeviceNodeFlagsToCapabilities(DeviceNode);

    if (DeviceCapabilities->NonDynamic) {

        //
        // Update the veto type.  If an error occurrs while adding the device
        // node to the veto list, or the caller did not provide a veto list
        // pointer, terminate the enumeration process now.
        //

        *Context->VetoType = PNP_VetoLegacyDevice;

        if (Context->VetoList != NULL) {

            if (!IopAppendLegacyVeto(Context, &DeviceNode->InstancePath)) {
                return FALSE;
            }

        } else {

            return FALSE;

        }

    }

    return TRUE;
}

BOOLEAN
IopGetLegacyVetoListDeviceNode(
    IN PDEVICE_NODE DeviceNode,
    IN PIO_GET_LEGACY_VETO_LIST_CONTEXT Context
    )
/*++

Routine Description:

    This routine recusively walks the device tree, invoking
    IopGetLegacyVetoListDevice to add device nodes to the veto list
    (as appropriate).

Parameters:

    DeviceNode - The device node.

    Context - An IO_GET_LEGACY_VETO_LIST_CONTEXT pointer.


ReturnValue:

    A BOOLEAN value which indicates whether the device tree enumeration
    process should be terminated or not.

--*/
{
    PDEVICE_NODE Child;

    //
    // Determine whether the device node should be added to the veto
    // list and add it.  If an operation is unsuccessful or we determine
    // the veto type but the caller doesn't need the veto list, then we
    // terminate our search now.
    //

    if (!IopGetLegacyVetoListDevice(DeviceNode, Context)) {
        return FALSE;
    }

    //
    // Call ourselves recursively to enumerate our children.  If while
    // enumerating our children we determine we can terminate the search
    // prematurely, do so.
    //

    for (Child = DeviceNode->Child;
         Child != NULL;
         Child = Child->Sibling) {

        if (!IopGetLegacyVetoListDeviceNode(Child, Context)) {
            return FALSE;
        }

    }

    return TRUE;
}

VOID
IopGetLegacyVetoListDrivers(
    IN PIO_GET_LEGACY_VETO_LIST_CONTEXT Context
    )
{
    PDRIVER_OBJECT driverObject;
    OBJECT_ATTRIBUTES attributes;
    UNICODE_STRING driverString;
    POBJECT_DIRECTORY_INFORMATION dirInfo;
    HANDLE directoryHandle;
    ULONG dirInfoLength, neededLength, dirContext;
    NTSTATUS status;
    BOOLEAN restartScan;

    dirInfoLength = 0;
    dirInfo = NULL;
    restartScan = TRUE;

    //
    // Get handle to \\Driver directory
    //

    RtlInitUnicodeString(&driverString, L"\\Driver");

    InitializeObjectAttributes(&attributes,
                               &driverString,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL
                               );

    status = ZwOpenDirectoryObject(&directoryHandle,
                                   DIRECTORY_QUERY,
                                   &attributes
                                   );
    if (!NT_SUCCESS(status)) {
        *Context->Status = status;
        return;
    }

    for (;;) {

        //
        // Get info on next object in directory.  If the buffer is too
        // small, reallocate it and try again.  Otherwise, any failure
        // including STATUS_NO_MORE_ENTRIES breaks us out.
        //

        status = ZwQueryDirectoryObject(directoryHandle,
                                        dirInfo,
                                        dirInfoLength,
                                        TRUE,           // force one at a time
                                        restartScan,
                                        &dirContext,
                                        &neededLength);
        if (status == STATUS_BUFFER_TOO_SMALL) {
            dirInfoLength = neededLength;
            if (dirInfo != NULL) {
                ExFreePool(dirInfo);
            }
            dirInfo = ExAllocatePool(PagedPool, dirInfoLength);
            if (dirInfo == NULL) {
                *Context->Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            status = ZwQueryDirectoryObject(directoryHandle,
                                            dirInfo,
                                            dirInfoLength,
                                            TRUE,       // force one at a time
                                            restartScan,
                                            &dirContext,
                                            &neededLength);
        }
        restartScan = FALSE;

        if (!NT_SUCCESS(status)) {
            break;
        }

        //
        // Have name of object.  Create object path and use
        // ObReferenceObjectByName() to get DriverObject.  This may
        // fail non-fatally if DriverObject has gone away in the interim.
        //

        driverString.MaximumLength = sizeof(L"\\Driver\\") +
            dirInfo->Name.Length;
        driverString.Length = driverString.MaximumLength - sizeof(WCHAR);
        driverString.Buffer = ExAllocatePool(PagedPool,
                                             driverString.MaximumLength);
        if (driverString.Buffer == NULL) {
            *Context->Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        swprintf(driverString.Buffer, L"\\Driver\\%ws", dirInfo->Name.Buffer);
        status = ObReferenceObjectByName(&driverString,
                                         OBJ_CASE_INSENSITIVE,
                                         NULL,                 // access state
                                         0,                    // access mask
                                         IoDriverObjectType,
                                         KernelMode,
                                         NULL,                 // parse context
                                         &driverObject);

        ExFreePool(driverString.Buffer);

        if (NT_SUCCESS(status)) {
            ASSERT(driverObject->Type == IO_TYPE_DRIVER);
            if (driverObject->Flags & DRVO_LEGACY_RESOURCES) {
                //
                // Update the veto type.  If the caller provided a
                // veto list pointer, add the driver to the veto list.
                // If an error occurs while adding the driver to the
                // veto list, or the caller did not provide a veto
                // list pointer, terminate the driver enumeration now.
                //
                // NOTE: Driver may be loaded but not running,
                // distinction is not made here.


                *Context->VetoType = PNP_VetoLegacyDriver;

                if (Context->VetoList != NULL) {
                    IopAppendLegacyVeto(Context, &dirInfo->Name);
                }
            }
            ObDereferenceObject(driverObject);

            //
            // Early out if we have a veto and the caller didn't want a list or
            // we hit some error already
            //
            if (((*Context->VetoType == PNP_VetoLegacyDriver) &&
                (Context->VetoList == NULL)) ||
                !NT_SUCCESS(*Context->Status)) {
                break;
            }
        }
    }
    if (dirInfo != NULL) {
        ExFreePool(dirInfo);
    }

    ZwClose(directoryHandle);
}

NTSTATUS
IoGetLegacyVetoList(
    OUT PWSTR *VetoList OPTIONAL,
    OUT PPNP_VETO_TYPE VetoType
    )
/*++

Routine Description:

    This routine is used by PNP and PO to determine whether legacy drivers and
    devices are installed in the system.  This routine is conceptually a
    QUERY_REMOVE_DEVICE and QUERY_POWER-like interface for legacy drivers
    and devices.

Parameters:

    VetoList - A pointer to a PWSTR. (Optional)  If specified,
        IoGetLegacyVetoList will allocate a veto list, and return a
        pointer to the veto list in VetoList.

    VetoType - A pointer to a PNP_VETO_TYPE.  If no legacy drivers
        or devices are found in the system, VetoType is assigned
        PNP_VetoTypeUnknown.  If one or more legacy drivers are installed,
        VetoType is assigned PNP_VetoLegacyDriver.  If one or more
        legacy devices are installed, VetoType is assigned
        PNP_VetoLegacyDevice.  VetoType is assigned independent of
        whether a VetoList is created.

ReturnValue:

    An NTSTATUS value indicating whether the IoGetLegacyVetoList() operation
    was successful.

--*/
{
    NTSTATUS Status;
    IO_GET_LEGACY_VETO_LIST_CONTEXT Context;
    UNICODE_STRING UnicodeString;

    //
    // Initialize the veto list.
    //

    if (VetoList != NULL) {
        *VetoList = NULL;
    }

    //
    // Initialize the veto type.
    //

    ASSERT(VetoType != NULL);

    *VetoType = PNP_VetoTypeUnknown;

    //
    // Initialize the status.
    //

    Status = STATUS_SUCCESS;

    if (PnPInitialized == FALSE) {

        //
        // Can't touch anything, but nothing is really started either.
        //
        return Status;
    }

    //
    // Initialize our local context.
    //

    Context.VetoList = VetoList;
    Context.VetoListLength = 0;
    Context.VetoType = VetoType;
    Context.Status = &Status;

    //
    // Enumerate all driver objects.  This process can: (1) modify
    // the veto list, (2) modify the veto type and/or (3) modify the
    // status.
    //

    IopGetLegacyVetoListDrivers(&Context);

    //
    // If the driver enumeration process was successful and no legacy
    // drivers were detected, enumerate all device nodes.  The same
    // context values as above may be modified during device enumeration.
    //

    if (NT_SUCCESS(Status)) {

        if (*VetoType == PNP_VetoTypeUnknown) {

            IopAcquireEnumerationLock(NULL);

            IopGetLegacyVetoListDeviceNode(
                IopRootDeviceNode,
                &Context
            );

            IopReleaseEnumerationLock(NULL);

        }

    }

    //
    // If the previous operation(s) was/were successful, and the caller
    // provided a veto list pointer and we have constructed a veto
    // list, terminate the veto list with an empty string, i.e. MULTI_SZ.
    //

    if (NT_SUCCESS(Status)) {

        if (*VetoType != PNP_VetoTypeUnknown) {

            if (VetoList != NULL) {

                RtlInitUnicodeString(
                    &UnicodeString,
                    L""
                );

                IopAppendLegacyVeto(
                    &Context,
                    &UnicodeString
                );

            }

        }

    }

    //
    // If a previous operation was unsuccessful, free any veto list we may have
    // allocated along the way.
    //

    if (!NT_SUCCESS(Status)) {

        if (VetoList != NULL && *VetoList != NULL) {
            ExFreePool(*VetoList);
            *VetoList = NULL;
        }

    }

    return Status;
}

NTSTATUS
IopDoDeferredSetInterfaceState(
    IN PDEVICE_NODE DeviceNode
    )
/*++

Routine Description:

    Process the queued IoSetDeviceInterfaceState calls.

Parameters:

    DeviceNode - Device node which has just been started.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    KIRQL           irql;
    PDEVICE_OBJECT  attachedDevice;

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&PpRegistryDeviceResource, TRUE);

    ExAcquireFastLock( &IopDatabaseLock, &irql );

    for (attachedDevice = DeviceNode->PhysicalDeviceObject;
         attachedDevice;
         attachedDevice = attachedDevice->AttachedDevice) {

        attachedDevice->DeviceObjectExtension->ExtensionFlags &= ~DOE_START_PENDING;
    }

    ExReleaseFastLock( &IopDatabaseLock, irql );

    while (!IsListEmpty(&DeviceNode->PendedSetInterfaceState)) {

        PPENDING_SET_INTERFACE_STATE entry;

        entry = (PPENDING_SET_INTERFACE_STATE)RemoveHeadList(&DeviceNode->PendedSetInterfaceState);

        IopProcessSetInterfaceState(&entry->LinkName, TRUE, FALSE);

        ExFreePool(entry->LinkName.Buffer);

        ExFreePool(entry);
    }

    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    return STATUS_SUCCESS;
}

NTSTATUS
IopProcessSetInterfaceState(
    IN PUNICODE_STRING SymbolicLinkName,
    IN BOOLEAN Enable,
    IN BOOLEAN DeferNotStarted
    )
/*++

Routine Description:

    This DDI allows a device class to activate and deactivate an association
    previously registered using IoRegisterDeviceInterface

Parameters:

    SymbolicLinkName - Supplies a pointer to the symbolic link name which was
        returned by IoRegisterDeviceInterface when the interface was registered,
        or as returned by IoGetDeviceInterfaces.

    Enable - If TRUE (non-zero), the interface will be enabled.  If FALSE, it
        will be disabled.

    DeferNotStarted - If TRUE then enables will be queued if the PDO isn't
        started.  It is FALSE when we've started the PDO and are processing the
        queued enables.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    HANDLE hInterfaceClassKey = NULL;
    HANDLE hInterfaceParentKey= NULL, hInterfaceInstanceKey = NULL;
    HANDLE hInterfaceParentControl = NULL, hInterfaceInstanceControl = NULL;
    UNICODE_STRING tempString, actualSymbolicLinkName, deviceNameString;
    PKEY_VALUE_FULL_INFORMATION pKeyValueInfo;
    ULONG linked, refcount;
    GUID guid;
    PDEVICE_OBJECT physicalDeviceObject;
    PWCHAR deviceNameBuffer = NULL;
    ULONG deviceNameBufferLength;

    PAGED_CODE();

    //
    // Get the symbolic link name without the ref string
    //

    status = IopDropReferenceString(&actualSymbolicLinkName, SymbolicLinkName);
    if (!NT_SUCCESS(status)) {
        goto clean0;
    }

    //
    // Extract the device class guid
    //

    status = IopParseSymbolicLinkName(SymbolicLinkName, NULL, NULL, NULL, NULL, NULL, &guid);

    //
    // Get function class instance handle
    //

    status = IopDeviceInterfaceKeysFromSymbolicLink(SymbolicLinkName,
                                                    KEY_READ | KEY_WRITE,
                                                    &hInterfaceClassKey,
                                                    &hInterfaceParentKey,
                                                    &hInterfaceInstanceKey
                                                    );

    if (!NT_SUCCESS(status)) {
        goto clean1;
    }

    //
    // Open the parent interface control subkey
    //
    PiWstrToUnicodeString(&tempString, REGSTR_KEY_CONTROL);
    status = IopCreateRegistryKeyEx( &hInterfaceParentControl,
                                     hInterfaceParentKey,
                                     &tempString,
                                     KEY_READ,
                                     REG_OPTION_VOLATILE,
                                     NULL
                                     );
    if (!NT_SUCCESS(status)) {
        goto clean1;
    }


    //
    // Find out the name of the device instance that 'owns' this interface.
    //
    status = IopGetRegistryValue(hInterfaceParentKey,
                                 REGSTR_VAL_DEVICE_INSTANCE,
                                 &pKeyValueInfo
                                 );

    if(NT_SUCCESS(status)) {
        //
        // Open the device instance control subkey
        //
        PiWstrToUnicodeString(&tempString, REGSTR_KEY_CONTROL);
        status = IopCreateRegistryKeyEx( &hInterfaceInstanceControl,
                                         hInterfaceInstanceKey,
                                         &tempString,
                                         KEY_READ,
                                         REG_OPTION_VOLATILE,
                                         NULL
                                         );
        if(!NT_SUCCESS(status)) {
            ExFreePool(pKeyValueInfo);
            hInterfaceInstanceControl = NULL;
        }
    }

    if (!NT_SUCCESS(status)) {
        goto clean2;
    }

    //
    // Find the PDO corresponding to this device instance name.
    //
    if (pKeyValueInfo->Type == REG_SZ) {

        IopRegistryDataToUnicodeString(&tempString,
                                        (PWSTR)KEY_VALUE_DATA(pKeyValueInfo),
                                        pKeyValueInfo->DataLength
                                        );

        physicalDeviceObject = IopDeviceObjectFromDeviceInstance(NULL, &tempString);

        if (physicalDeviceObject) {

            //
            // DeferNotStarted is set TRUE if we are being called from
            // IoSetDeviceInterfaceState.  It will be set FALSE if we are
            // processing previously queued operations as we are starting the
            // device.
            //

            if (DeferNotStarted) {

                if (physicalDeviceObject->DeviceObjectExtension->ExtensionFlags & DOE_START_PENDING) {

                    PDEVICE_NODE deviceNode;
                    PPENDING_SET_INTERFACE_STATE pendingSetState;

                    //
                    // The device hasn't been started yet.  We need to queue
                    // any enables and remove items from the queue on a disable.
                    //
                    deviceNode = (PDEVICE_NODE)physicalDeviceObject->DeviceObjectExtension->DeviceNode;

                    if (Enable) {

                        pendingSetState = ExAllocatePool( PagedPool,
                                                          sizeof(PENDING_SET_INTERFACE_STATE));

                        if (pendingSetState != NULL) {

                            pendingSetState->LinkName.Buffer = ExAllocatePool( PagedPool,
                                                                               SymbolicLinkName->Length);

                            if (pendingSetState->LinkName.Buffer != NULL) {

                                //
                                // Capture the callers info and queue it to the
                                // devnode.  Once the device stack is started
                                // we will dequeue and process it.
                                //
                                pendingSetState->LinkName.MaximumLength = SymbolicLinkName->Length;
                                pendingSetState->LinkName.Length = SymbolicLinkName->Length;
                                RtlCopyMemory( pendingSetState->LinkName.Buffer,
                                               SymbolicLinkName->Buffer,
                                               SymbolicLinkName->Length);
                                InsertTailList( &deviceNode->PendedSetInterfaceState,
                                                &pendingSetState->List);

                                ExFreePool(pKeyValueInfo);

                                ObDereferenceObject(physicalDeviceObject);

                                status = STATUS_SUCCESS;
                                goto clean2;

                            } else {
                                //
                                // Couldn't allocate a buffer to hold the
                                // symbolic link name.
                                //

                                ExFreePool(pendingSetState);
                                status = STATUS_INSUFFICIENT_RESOURCES;
                            }

                        } else {
                            //
                            // Couldn't allocate the PENDING_SET_INTERFACE_STATE
                            // structure.
                            //


                            status = STATUS_INSUFFICIENT_RESOURCES;
                        }

                    } else {

                        PLIST_ENTRY entry;

                        //
                        // We are disabling an interface.  Since we aren't
                        // started yet we should have queued the enable.  Now
                        // we go back and find the matching enable and remove
                        // it from the queue.
                        //

                        for (entry = deviceNode->PendedSetInterfaceState.Flink;
                             entry != &deviceNode->PendedSetInterfaceState;
                             entry = entry->Flink)  {

                            pendingSetState = CONTAINING_RECORD( entry,
                                                                 PENDING_SET_INTERFACE_STATE,
                                                                 List );

                            if (RtlEqualUnicodeString( &pendingSetState->LinkName,
                                                       SymbolicLinkName,
                                                       TRUE)) {

                                //
                                // We found it, remove it from the list and
                                // free it.
                                //
                                RemoveEntryList(&pendingSetState->List);

                                ExFreePool(pendingSetState->LinkName.Buffer);
                                ExFreePool(pendingSetState);

                                break;
                            }
                        }

#if 0
                        //
                        // Debug code to catch the case where we couldn't find
                        // the entry to remove.  This could happen if we messed
                        // up adding the entry to the list or the driver disabled
                        // an interface without first enabling it.  Either way
                        // it probably merits some investigation.
                        //
                        if (entry == &deviceNode->PendedSetInterfaceState) {
                            PIDBGMSG(PIDBG_ERROR,
                                     ("IopProcessSetInterfaceState: Disable couldn't find deferred enable, DeviceNode = 0x%p, SymbolicLink = \"%Z\"\n",
                                     deviceNode,
                                     SymbolicLinkName));
                        }

                        ASSERT(entry != &deviceNode->PendedSetInterfaceState);
#endif
                        ExFreePool(pKeyValueInfo);

                        ObDereferenceObject(physicalDeviceObject);

                        status = STATUS_SUCCESS;
                        goto clean2;
                    }
                }
            }

            if (!Enable || !NT_SUCCESS(status)) {
                ObDereferenceObject(physicalDeviceObject);
            }
        } else {

            status = STATUS_INVALID_DEVICE_REQUEST;
        }

    } else {
        //
        // This will only happen if the registry information is screwed up.
        //
        physicalDeviceObject = NULL;
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    if (!Enable) {
        //
        // In the case of Disable we want to continue even if there was an error
        // finding the PDO.  Prior to adding support for deferring the
        // IoSetDeviceInterfaceState calls, we never looked up the PDO for
        // disables.  This will make sure that we continue to behave the same as
        // we used to in the case where we can't find the PDO.
        //
        status = STATUS_SUCCESS;
    }

    ExFreePool(pKeyValueInfo);

    if (!NT_SUCCESS(status)) {
        goto clean2;
    }

    if (Enable) {
        //
        // Retrieve the PDO's device object name.  (Start out with a reasonably-sized
        // buffer so we hopefully only have to retrieve this once.
        //
        deviceNameBufferLength = 256 * sizeof(WCHAR);

        while (TRUE) {

            deviceNameBuffer = ExAllocatePool(PagedPool, deviceNameBufferLength);
            if (!deviceNameBuffer) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            status = IoGetDeviceProperty( physicalDeviceObject,
                                          DevicePropertyPhysicalDeviceObjectName,
                                          deviceNameBufferLength,
                                          deviceNameBuffer,
                                          &deviceNameBufferLength
                                         );

            if (NT_SUCCESS(status)) {
                break;
            } else {
                //
                // Free the current buffer before we figure out what went wrong.
                //
                ExFreePool(deviceNameBuffer);

                if (status != STATUS_BUFFER_TOO_SMALL) {
                    //
                    // Our failure wasn't because the buffer was too small--bail now.
                    //
                    break;
                }

                //
                // Otherwise, loop back and try again with our new buffer size.
                //
            }
        }

        //
        // OK, we don't need the PDO anymore.
        //
        ObDereferenceObject(physicalDeviceObject);

        if (!NT_SUCCESS(status)) {
            goto clean2;
        }

        //
        // Now create a unicode string based on the device object name we just retrieved.
        //

        RtlInitUnicodeString(&deviceNameString, deviceNameBuffer);
    }

    //
    // Retrieve the linked value from the control subkey.
    //
    pKeyValueInfo=NULL;
    status = IopGetRegistryValue(hInterfaceInstanceControl, REGSTR_VAL_LINKED, &pKeyValueInfo);

    if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // The absence of a linked value is taken to mean not linked
        //

        linked = 0;

    } else {
        if (!NT_SUCCESS(status)) {
            //
            // If the call failed, pKeyValueInfo was never allocated
            //
            goto clean3;
        }

        //
        // Check linked is a DWORD
        //

        if(pKeyValueInfo->Type == REG_DWORD && pKeyValueInfo->DataLength == sizeof(ULONG)) {

            linked = *((PULONG) KEY_VALUE_DATA(pKeyValueInfo));

        } else {

            //
            // The registry is screwed up - assume linked is 0 and the registry will be fixed when
            // we update linked in a few moments
            //

            linked = 0;

        }

    }
    if (pKeyValueInfo) {
        ExFreePool (pKeyValueInfo);
    }

    //
    // Retrieve the refcount value from the control subkey.
    //

    RtlInitUnicodeString(&tempString, REGSTR_VAL_REFERENCECOUNT);
    status = IopGetRegistryValue(hInterfaceParentControl,
                                 tempString.Buffer,
                                 &pKeyValueInfo
                                 );

    if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // The absence of a refcount value is taken to mean refcount == 0
        //

        refcount = 0;

    } else {
        if (!NT_SUCCESS(status)) {
            goto clean3;
        }

        //
        // Check refcount is a DWORD
        //

        if(pKeyValueInfo->Type == REG_DWORD && pKeyValueInfo->DataLength == sizeof(ULONG)) {

            refcount = *((PULONG) KEY_VALUE_DATA(pKeyValueInfo));

        } else {

            //
            // The registry is screwed up - assume refcount is 0 and the registry will be fixed when
            // we update refcount in a few moments
            //

            refcount = 0;

        }

        ExFreePool(pKeyValueInfo);
    }


    if (Enable) {

        if (!linked) {
            //
            // check and update the reference count
            //

            if (refcount > 0) {
                //
                // Another device instance has already referenced this interface;
                // just increment the reference count; don't try create a symbolic link.
                //
                refcount += 1;
            } else {
                //
                // According to the reference count, no other device instances currently
                // reference this interface, and therefore no symbolic links should exist,
                // so we should create one.
                //
                refcount = 1;
                status = IoCreateSymbolicLink(&actualSymbolicLinkName, &deviceNameString);

                if (status == STATUS_OBJECT_NAME_COLLISION) {
                    //
                    // The reference count is screwed up.
                    //
                    KdPrint(("IoSetDeviceInterfaceState: symbolic link for %ws already exists! status = %8.8X\n",
                             actualSymbolicLinkName.Buffer, status));
                    status = STATUS_SUCCESS;
                }

            }

            linked = 1;

#if 0
            IopSetupDeviceObjectFromDeviceClass(physicalDeviceObject,
                                                hInterfaceClassKey);
#endif

        } else {

            //
            // The association already exists - don't perform the notification
            //

            status = STATUS_OBJECT_NAME_EXISTS; // Informational message not error
            goto clean3;

        }
    } else {

        if (linked) {

            //
            // check and update the reference count
            //

            if (refcount > 1) {
                //
                // Another device instance already references this interface;
                // just decrement the reference count; don't try to remove the symbolic link.
                //
                refcount -= 1;
            } else {
                //
                // According to the reference count, only this device instance currently
                // references this interface, so it is ok to delete this symbolic link
                //
                refcount = 0;
                status = IoDeleteSymbolicLink(&actualSymbolicLinkName);

                if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
                    //
                    // The reference count is screwed up.
                    //
                    KdPrint(("IoSetDeviceInterfaceState: no symbolic link for %ws to delete! status = %8.8X\n",
                             actualSymbolicLinkName.Buffer, status));
                    status = STATUS_SUCCESS;
                }

            }

            linked = 0;

        } else {

            //
            // The association does not exists - fail and do not perform notification
            //

            status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    if (!NT_SUCCESS(status)) {
        goto clean3;
    }

    //
    // Update the value of linked
    //

    PiWstrToUnicodeString(&tempString, REGSTR_VAL_LINKED);
    status = ZwSetValueKey(hInterfaceInstanceControl,
                           &tempString,
                           0,
                           REG_DWORD,
                           &linked,
                           sizeof(linked)
                          );

    //
    // Update the value of refcount
    //

    RtlInitUnicodeString(&tempString, REGSTR_VAL_REFERENCECOUNT);
    status = ZwSetValueKey(hInterfaceParentControl,
                           &tempString,
                           0,
                           REG_DWORD,
                           &refcount,
                           sizeof(refcount)
                          );


    //
    // Notify anyone that is interested
    //

    if (linked) {

        PpSetDeviceClassChange( (LPGUID) &GUID_DEVICE_INTERFACE_ARRIVAL, &guid, SymbolicLinkName);

    } else {

        PpSetDeviceClassChange( (LPGUID) &GUID_DEVICE_INTERFACE_REMOVAL, &guid, SymbolicLinkName);

    }

clean3:
    if (deviceNameBuffer != NULL) {
        ExFreePool(deviceNameBuffer);
    }

clean2:
    if (hInterfaceParentControl) {
        ZwClose(hInterfaceParentControl);
    }
    if (hInterfaceInstanceControl) {
        ZwClose(hInterfaceInstanceControl);
    }

clean1:
    if (hInterfaceParentKey) {
        ZwClose(hInterfaceParentKey);
    }
    if (hInterfaceInstanceKey) {
        ZwClose(hInterfaceInstanceKey);
    }
    if(hInterfaceClassKey != NULL) {
        ZwClose(hInterfaceClassKey);
    }

clean0:
    if (!NT_SUCCESS(status) && !Enable) {
        //
        // If we failed to disable an interface (most likely because the
        // interface keys have already been deleted) report success.
        //
        status = STATUS_SUCCESS;
    }

    return status;
}
