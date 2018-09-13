/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pnpsubs.c

Abstract:

    This module contains the plug-and-play subroutines for the
    I/O system.


Author:

    Shie-Lin Tzong (shielint) 3-Jan-1995

Environment:

    Kernel mode

Revision History:


--*/

#include "iop.h"
#pragma hdrstop

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'uspP')
#endif

//
// Prototype of internal functions
//

VOID
IopDisableDevice(
    IN PDEVICE_NODE DeviceNode,
    IN HANDLE Handle
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopCreateMadeupNode)
#pragma alloc_text(PAGE, IopRemoveStringFromValueKey)
#pragma alloc_text(PAGE, IopAppendStringToValueKey)
#pragma alloc_text(PAGE, IopConcatenateUnicodeStrings)
#pragma alloc_text(PAGE, IopPrepareDriverLoading)
#pragma alloc_text(PAGE, IopServiceInstanceToDeviceInstance)
#pragma alloc_text(PAGE, IopOpenRegistryKeyPersist)
#pragma alloc_text(PAGE, IopOpenServiceEnumKeys)
#pragma alloc_text(PAGE, IopOpenCurrentHwProfileDeviceInstanceKey)
#pragma alloc_text(PAGE, IopGetDeviceInstanceCsConfigFlags)
#pragma alloc_text(PAGE, IopSetDeviceInstanceCsConfigFlags)
#pragma alloc_text(PAGE, IopApplyFunctionToSubKeys)
#pragma alloc_text(PAGE, IopRegMultiSzToUnicodeStrings)
#pragma alloc_text(PAGE, IopApplyFunctionToServiceInstances)
#pragma alloc_text(PAGE, IopIsDuplicatedDevices)
#pragma alloc_text(PAGE, IopMarkDuplicateDevice)
#pragma alloc_text(PAGE, IopFreeUnicodeStringList)
#pragma alloc_text(PAGE, IopDriverLoadingFailed)
#pragma alloc_text(PAGE, IopIsAnyDeviceInstanceEnabled)
#pragma alloc_text(PAGE, IopIsDeviceInstanceEnabled)
#pragma alloc_text(PAGE, IopDetermineResourceListSize)
#pragma alloc_text(PAGE, IopReferenceDriverObjectByName)
#pragma alloc_text(PAGE, IopDeviceObjectFromDeviceInstance)
#pragma alloc_text(PAGE, IopDeviceObjectToDeviceInstance)
#pragma alloc_text(PAGE, IopCleanupDeviceRegistryValues)
#pragma alloc_text(PAGE, IopGetDeviceResourcesFromRegistry)
#pragma alloc_text(PAGE, IopCmResourcesToIoResources)
#pragma alloc_text(PAGE, IopReadDeviceConfiguration)
#pragma alloc_text(PAGE, IopGetGroupOrderIndex)
#pragma alloc_text(PAGE, IopDeleteLegacyKey)
#pragma alloc_text(PAGE, IopIsLegacyDriver)
#pragma alloc_text(PAGE, IopFilterResourceRequirementsList)
#pragma alloc_text(PAGE, IopMergeFilteredResourceRequirementsList)
#pragma alloc_text(PAGE, IopDisableDevice)
#pragma alloc_text(PAGE, IopMergeCmResourceLists)
#pragma alloc_text(PAGE, IopDeviceCapabilitiesToRegistry)
#pragma alloc_text(PAGE, IopRestartDeviceNode)
#endif

NTSTATUS
IopCreateMadeupNode(
    IN PUNICODE_STRING ServiceKeyName,
    OUT PHANDLE ReturnedHandle,
    OUT PUNICODE_STRING KeyName,
    OUT PULONG InstanceNumber,
    IN BOOLEAN ResourceOwned
    )

/*++

Routine Description:

    This routine creates a new instance node under System\Enum\Root\*Madeup<Name>
    key and all the required default value entries.  Also a value entry under
    Service\ServiceKeyName\Enum is created to point to the newly created madeup
    entry.  A handle and the keyname of the new key are returned to caller.
    Caller must free the unicode string when he is done with it.

Parameters:

    ServiceKeyName - Supplies a pointer to the name of the subkey in the
        system service list (HKEY_LOCAL_MACHINE\CurrentControlSet\Services)
        that caused the driver to load. This is the RegistryPath parameter
        to the DriverEntry routine.

    ReturnedHandle - Supplies a variable to receive the handle of the
        newly created key.

    KeyName - Supplies a variable to receive the name of the newly created
        key.

    InstanceNumber - supplies a variable to receive the InstanceNumber value
        entry created under service\name\enum subkey.

    ResourceOwned - supplies a BOOLEAN variable to indicate if caller owns
        the registry resource exclusively.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    UNICODE_STRING tmpKeyName, unicodeInstanceName, unicodeString;
    UNICODE_STRING rootKeyName, unicodeValueName, unicodeKeyName;
    HANDLE handle, enumRootHandle, hTreeHandle;
    ULONG instance;
    UCHAR unicodeBuffer[20];
    ULONG tmpValue, disposition = 0;
    NTSTATUS status;
    PWSTR p;
    BOOLEAN releaseResource = FALSE;

    if (!ResourceOwned) {
        KeEnterCriticalRegion();
        ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);
        releaseResource = TRUE;
    }

    //
    // Open LocalMachine\System\CurrentControlSet\Enum\Root
    //

    status = IopOpenRegistryKey(&enumRootHandle,
                                NULL,
                                &CmRegistryMachineSystemCurrentControlSetEnumRootName,
                                KEY_ALL_ACCESS,
                                FALSE
                                );
    if (!NT_SUCCESS(status)) {
        goto local_exit0;
    }

    //
    // Open, and create if not already exist, System\Enum\Root\LEGACY_<ServiceName>
    // First, try to find the ServiceName by extracting it from user supplied
    // ServiceKeyName.
    //

    PiWstrToUnicodeString(&tmpKeyName, REGSTR_KEY_MADEUP);
    IopConcatenateUnicodeStrings(&unicodeKeyName, &tmpKeyName, ServiceKeyName);
    RtlUpcaseUnicodeString(&unicodeKeyName, &unicodeKeyName, FALSE);
    status = IopOpenRegistryKeyPersist(&handle,
                                       enumRootHandle,
                                       &unicodeKeyName,
                                       KEY_ALL_ACCESS,
                                       TRUE,
                                       NULL
                                       );
    ZwClose(enumRootHandle);
    if (!NT_SUCCESS(status)) {
        RtlFreeUnicodeString(&unicodeKeyName);
        goto local_exit0;
    }

    instance = 1;

    PiWstrToUnicodeString(&unicodeValueName, REGSTR_VALUE_NEXT_INSTANCE);
    status = ZwSetValueKey(
                handle,
                &unicodeValueName,
                TITLE_INDEX_VALUE,
                REG_DWORD,
                &instance,
                sizeof(instance)
                );

    instance--;
    *InstanceNumber = instance;
    PiUlongToInstanceKeyUnicodeString(&unicodeInstanceName,
                                      unicodeBuffer + sizeof(WCHAR), // reserve first WCHAR space
                                      20 - sizeof(WCHAR),
                                      instance
                                      );
    status = IopOpenRegistryKeyPersist(ReturnedHandle,
                                       handle,
                                       &unicodeInstanceName,
                                       KEY_ALL_ACCESS,
                                       TRUE,
                                       &disposition
                                       );
    ZwClose(handle);
    if (!NT_SUCCESS(status)) {
        RtlFreeUnicodeString(&unicodeKeyName);
        goto local_exit0;
    }

    //
    // Prepare newly created registry key name for returning to caller
    //

    *(PWSTR)unicodeBuffer = OBJ_NAME_PATH_SEPARATOR;
    unicodeInstanceName.Buffer = (PWSTR)unicodeBuffer;
    unicodeInstanceName.Length += sizeof(WCHAR);
    unicodeInstanceName.MaximumLength += sizeof(WCHAR);
    PiWstrToUnicodeString(&rootKeyName, REGSTR_KEY_ROOTENUM);
    RtlInitUnicodeString(&tmpKeyName, L"\\");
    IopConcatenateUnicodeStrings(&unicodeString, &tmpKeyName, &unicodeKeyName);
    RtlFreeUnicodeString(&unicodeKeyName);
    IopConcatenateUnicodeStrings(&tmpKeyName, &rootKeyName, &unicodeString);
    RtlFreeUnicodeString(&unicodeString);
    IopConcatenateUnicodeStrings(KeyName, &tmpKeyName, &unicodeInstanceName);

    if (disposition == REG_CREATED_NEW_KEY) {

        //
        // Create all the default value entry for the newly created key.
        // Service = ServiceKeyName
        // FoundAtEnum = 1
        // Class = "LegacyDriver"
        // ClassGUID = GUID for legacy driver class
        // ConfigFlags = 0
        //
        // Create "Control" subkey with "NewlyCreated" value key
        //

        PiWstrToUnicodeString(&unicodeValueName, REGSTR_KEY_CONTROL);
        status = IopOpenRegistryKey(&handle,
                                    *ReturnedHandle,
                                    &unicodeValueName,
                                    KEY_ALL_ACCESS,
                                    TRUE
                                    );
        if (NT_SUCCESS(status)) {
            PiWstrToUnicodeString(&unicodeValueName, REGSTR_VALUE_NEWLY_CREATED);
            tmpValue = 0;
            ZwSetValueKey(handle,
                          &unicodeValueName,
                          TITLE_INDEX_VALUE,
                          REG_DWORD,
                          &tmpValue,
                          sizeof(tmpValue)
                          );
            ZwClose(handle);
        }

        handle = *ReturnedHandle;

        PiWstrToUnicodeString(&unicodeValueName, REGSTR_VALUE_SERVICE);
        p = (PWSTR)ExAllocatePool(PagedPool,
                                  ServiceKeyName->Length + sizeof(UNICODE_NULL));
        if(p) {
            RtlMoveMemory(p, ServiceKeyName->Buffer, ServiceKeyName->Length);
            p[ServiceKeyName->Length / sizeof (WCHAR)] = UNICODE_NULL;
            ZwSetValueKey(
                        handle,
                        &unicodeValueName,
                        TITLE_INDEX_VALUE,
                        REG_SZ,
                        p,
                        ServiceKeyName->Length + sizeof(UNICODE_NULL)
                        );
            //
            // We'll keep the null-terminated service name buffer around for a while,
            // because we may need it later on for the DeviceDesc in case the service
            // has no DisplayName.
            //
            // ExFreePool(p);
        }

        PiWstrToUnicodeString(&unicodeValueName, REGSTR_VALUE_LEGACY);
        tmpValue = 1;
        ZwSetValueKey(
                    handle,
                    &unicodeValueName,
                    TITLE_INDEX_VALUE,
                    REG_DWORD,
                    &tmpValue,
                    sizeof(tmpValue)
                    );

        PiWstrToUnicodeString(&unicodeValueName, REGSTR_VALUE_CONFIG_FLAGS);
        tmpValue = 0;
        ZwSetValueKey(
                    handle,
                    &unicodeValueName,
                    TITLE_INDEX_VALUE,
                    REG_DWORD,
                    &tmpValue,
                    sizeof(tmpValue)
                    );

        //
        // BUGBUG (lonnym)--verify that removal of the following code fragment is OK.
        //
#if 0
        PiWstrToUnicodeString(&unicodeValueName, REGSTR_VALUE_FOUNDATENUM);
        tmpValue = 1;
        ZwSetValueKey(
                    handle,
                    &unicodeValueName,
                    TITLE_INDEX_VALUE,
                    REG_DWORD,
                    &tmpValue,
                    sizeof(tmpValue)
                    );
#endif // (lonnym, verify removal to here)

        PiWstrToUnicodeString(&unicodeValueName, REGSTR_VALUE_CLASS);
        ZwSetValueKey(
                    handle,
                    &unicodeValueName,
                    TITLE_INDEX_VALUE,
                    REG_SZ,
                    REGSTR_VALUE_LEGACY_DRIVER,
                    sizeof(REGSTR_VALUE_LEGACY_DRIVER)
                    );

        PiWstrToUnicodeString(&unicodeValueName, REGSTR_VALUE_CLASSGUID);
        ZwSetValueKey(
                    handle,
                    &unicodeValueName,
                    TITLE_INDEX_VALUE,
                    REG_SZ,
                    REGSTR_VALUE_LEGACY_DRIVER_CLASS_GUID,
                    sizeof(REGSTR_VALUE_LEGACY_DRIVER_CLASS_GUID)
                    );


        //
        // Initialize DeviceDesc= value entry.  If the service key has a "DisplayName"
        // value entry, it is used as the DeviceDesc value.  Otherwise, the service key
        // name is used.
        //

        status = IopOpenServiceEnumKeys(ServiceKeyName,
                                        KEY_READ,
                                        &handle,
                                        NULL,
                                        FALSE
                                        );
        if (NT_SUCCESS(status)) {

            keyValueInformation = NULL;
            unicodeString.Length = 0;
            status = IopGetRegistryValue(handle,
                                         REGSTR_VALUE_DISPLAY_NAME,
                                         &keyValueInformation
                                        );
            if (NT_SUCCESS(status)) {
                if (keyValueInformation->Type == REG_SZ) {
                    if (keyValueInformation->DataLength > sizeof(UNICODE_NULL)) {
                        IopRegistryDataToUnicodeString(&unicodeString,
                                                       (PWSTR)KEY_VALUE_DATA(keyValueInformation),
                                                       keyValueInformation->DataLength
                                                       );
                    }
                }
            }
            if ((unicodeString.Length == 0) && p) {

                //
                // No DisplayName--use the service key name.
                //

                unicodeString.Length = ServiceKeyName->Length;
                unicodeString.MaximumLength = ServiceKeyName->Length + sizeof(UNICODE_NULL);
                unicodeString.Buffer = p;
            }

            if(unicodeString.Length) {
                PiWstrToUnicodeString(&unicodeValueName, REGSTR_VALUE_DEVICE_DESC);
                ZwSetValueKey(*ReturnedHandle,
                              &unicodeValueName,
                              TITLE_INDEX_VALUE,
                              REG_SZ,
                              unicodeString.Buffer,
                              unicodeString.Length + sizeof(UNICODE_NULL)
                              );
            }
            if (keyValueInformation) {
                ExFreePool(keyValueInformation);
            }
            ZwClose(handle);
        }

        if(p) {
            ExFreePool(p);
        }
    }

    //
    // Create new value entry under ServiceKeyName\Enum to reflect the newly
    // added made-up device instance node.
    //

    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();
    releaseResource = FALSE;

    status = PpDeviceRegistration(
                 KeyName,
                 TRUE,
                 NULL,
                 NULL
                 );

    if (ResourceOwned) {
        KeEnterCriticalRegion();
        ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);
    }
    RtlFreeUnicodeString(&tmpKeyName);
    if (!NT_SUCCESS( status )) {

        //
        // There is no registry key for the ServiceKeyName information.
        //

        ZwClose(*ReturnedHandle);
        RtlFreeUnicodeString(KeyName);
    }
local_exit0:
    if (releaseResource) {
        ExReleaseResource(&PpRegistryDeviceResource);
        KeLeaveCriticalRegion();
    }
    return status;
}

NTSTATUS
IopRemoveStringFromValueKey (
    IN HANDLE Handle,
    IN PWSTR ValueName,
    IN PUNICODE_STRING String
    )

/*++

Routine Description:

    This routine remove a string from a value entry specified by ValueName
    under an already opened registry handle.  Note, this routine will not
    delete the ValueName entry even it becomes empty after the removal.

Parameters:

    Handle - Supplies the handle to a registry key whose value entry will
        be modified.

    ValueName - Supplies a unicode string to specify the value entry.

    String - Supplies a unicode string to remove from value entry.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    UNICODE_STRING unicodeString;
    PWSTR nextString, currentString;
    ULONG length, leftLength;
    NTSTATUS status;
    BOOLEAN found = FALSE;

    if (String == NULL || String->Length / sizeof(WCHAR) == 0) {
        return STATUS_SUCCESS;
    }

    //
    // Read registry value entry data
    //

    status = IopGetRegistryValue(Handle, ValueName, &keyValueInformation);

    if (!NT_SUCCESS( status )) {
        return status;
    } else if ((keyValueInformation->Type != REG_MULTI_SZ) ||
               (keyValueInformation->DataLength == 0)) {

        status = (keyValueInformation->Type == REG_MULTI_SZ) ? STATUS_SUCCESS
                                                             : STATUS_INVALID_PARAMETER;
        ExFreePool(keyValueInformation);
        return status;
    }

    //
    // Scan through the multi_sz string to find the matching string
    // and remove it.
    //

    status = STATUS_SUCCESS;
    currentString = (PWSTR)KEY_VALUE_DATA(keyValueInformation);
    leftLength = keyValueInformation->DataLength;
    while (!found && leftLength >= String->Length + sizeof(WCHAR)) {
        unicodeString.Buffer = currentString;
        length = wcslen( currentString ) * sizeof( WCHAR );
        unicodeString.Length = (USHORT)length;
        length += sizeof(UNICODE_NULL);
        unicodeString.MaximumLength = (USHORT)length;
        nextString = currentString + length / sizeof(WCHAR);
        leftLength -= length;

        if (RtlEqualUnicodeString(&unicodeString, String, TRUE)) {
            found = TRUE;
            RtlMoveMemory(currentString, nextString, leftLength);
            RtlInitUnicodeString(&unicodeString, ValueName);
            status = ZwSetValueKey(
                        Handle,
                        &unicodeString,
                        TITLE_INDEX_VALUE,
                        REG_MULTI_SZ,
                        KEY_VALUE_DATA(keyValueInformation),
                        keyValueInformation->DataLength - length
                        );
            break;
        } else {
            currentString = nextString;
        }
    }
    ExFreePool(keyValueInformation);
    return status;
}

NTSTATUS
IopAppendStringToValueKey (
    IN HANDLE Handle,
    IN PWSTR ValueName,
    IN PUNICODE_STRING String,
    IN BOOLEAN Create
    )

/*++

Routine Description:

    This routine appends a string to a value entry specified by ValueName
    under an already opened registry handle.  If the ValueName is not present
    and Create is TRUE, a new value entry will be created using the name
    ValueName.

Parameters:

    Handle - Supplies the handle to a registry key whose value entry will
        be modified.

    ValueName - Supplies a pointer to a string to specify the value entry.

    String - Supplies a unicode string to append to the value entry.

    Create - Supplies a BOOLEAN variable to indicate if the ValueName
        value entry should be created if it is not present.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PWSTR destinationString, p;
    UNICODE_STRING unicodeValueName;
    ULONG size;
    NTSTATUS status;

    if ( !String || (String->Length < sizeof(WCHAR)) ) {
        return STATUS_SUCCESS;
    }

    //
    // Read registry value entry data
    //

    status = IopGetRegistryValue(Handle, ValueName, &keyValueInformation);

    if(!NT_SUCCESS( status )) {
        if (status == STATUS_OBJECT_NAME_NOT_FOUND && Create) {

            //
            // if no valid entry exists and user said ok to create one
            //

            keyValueInformation = NULL;
        } else {
            return status;
        }
    } else if(keyValueInformation->Type != REG_MULTI_SZ) {

        ExFreePool(keyValueInformation);

        if(Create) {
            keyValueInformation = NULL;
        } else {
            return STATUS_INVALID_PARAMETER_2;
        }

    } else if(keyValueInformation->DataLength < sizeof(WCHAR) ||
              *(PWCHAR)KEY_VALUE_DATA(keyValueInformation) == UNICODE_NULL) {  // empty or one NULL WCHAR

        ExFreePool(keyValueInformation);
        keyValueInformation = NULL;
    }

    //
    // Allocate a buffer to hold new data for the specified key value entry
    // Make sure the buffer is at least an empty MULTI_SZ big.
    //

    if (keyValueInformation) {
        size = keyValueInformation->DataLength + String->Length + sizeof (UNICODE_NULL);
    } else {
        size =  String->Length + 2 * sizeof(UNICODE_NULL);
    }

    destinationString = p = (PWSTR)ExAllocatePool(PagedPool, size);
    if (destinationString == NULL) {
        if (keyValueInformation) {
            ExFreePool(keyValueInformation);
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy the existing data to our newly allocated buffer, if any
    //

    if (keyValueInformation) {

        //
        // Note we  need to remove a UNICODE_NULL because the
        // MULTI_SZ has two terminating UNICODE_NULL.
        //

        RtlMoveMemory(p,
                      KEY_VALUE_DATA(keyValueInformation),
                      keyValueInformation->DataLength - sizeof(WCHAR)
                      );
        p += keyValueInformation->DataLength / sizeof(WCHAR) - 1;

        ExFreePool(keyValueInformation);
    }

    //
    // Append the user specified unicode string to our buffer
    //
    RtlMoveMemory(p,
                  String->Buffer,
                  String->Length
                  );
    p += String->Length / sizeof(WCHAR);
    *p = UNICODE_NULL;
    p++;
    *p = UNICODE_NULL;

    //
    // Finally write the data to the specified registy value entry
    //

    RtlInitUnicodeString(&unicodeValueName, ValueName);
    status = ZwSetValueKey(
                Handle,
                &unicodeValueName,
                TITLE_INDEX_VALUE,
                REG_MULTI_SZ,
                destinationString,
                size
                );

    ExFreePool(destinationString);
    return status;
}

BOOLEAN
IopConcatenateUnicodeStrings (
    OUT PUNICODE_STRING Destination,
    IN  PUNICODE_STRING String1,
    IN  PUNICODE_STRING String2  OPTIONAL
    )

/*++

Routine Description:

    This routine returns a buffer containing the concatenation of the
    two specified strings.  Since String2 is optional, this function may
    also be used to make a copy of a unicode string.  Paged pool space
    is allocated for the destination string.  Caller must release the
    space once done with it.

Parameters:

    Destination - Supplies a variable to receive the handle of the
        newly created key.

    String1 - Supplies a pointer to the frist UNICODE_STRING.

    String2 - Supplies an optional pointer to the second UNICODE_STRING.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    ULONG length;
    PWSTR buffer;

    length = String1->Length + sizeof(UNICODE_NULL);
    if (ARGUMENT_PRESENT(String2)) {
        length += String2->Length;
    }
    buffer = (PWSTR)ExAllocatePool(PagedPool, length);
    if (!buffer) {
        return FALSE;
    }
    Destination->Buffer = buffer;
    Destination->Length = (USHORT)length - sizeof(UNICODE_NULL);
    Destination->MaximumLength = (USHORT)length;
    RtlMoveMemory (Destination->Buffer, String1->Buffer, String1->Length);
    if(ARGUMENT_PRESENT(String2)) {
        RtlMoveMemory((PUCHAR)Destination->Buffer + String1->Length,
                      String2->Buffer,
                      String2->Length
                     );
    }
    buffer[length / sizeof(WCHAR) - 1] = UNICODE_NULL;
    return TRUE;
}

NTSTATUS
IopPrepareDriverLoading (
    IN PUNICODE_STRING KeyName,
    IN HANDLE KeyHandle,
    IN PIMAGE_NT_HEADERS Header
    )

/*++

Routine Description:

    This routine first checks if the driver is loadable.  If its a
    PnP driver, it will always be loaded (we trust it to do the right
    things.)  If it is a legacy driver, we need to check if its device
    has been disabled.  Once we decide to load the driver, the Enum
    subkey of the service node will be checked for duplicates, if any.

Parameters:

    KeyName - Supplies a pointer to the driver's service key unicode string

    KeyHandle - Supplies a handle to the driver service node in the registry
        that describes the driver to be loaded.

Return Value:

    The function value is the final status of the load operation.

--*/

{
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation = NULL;
    ULONG tmp, count = 0;
    HANDLE serviceEnumHandle = NULL, sysEnumXxxHandle, controlHandle;
    UNICODE_STRING unicodeKeyName, unicodeValueName;
    BOOLEAN IsPlugPlayDriver = FALSE;

    KeEnterCriticalRegion();
    ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

    //
    // Check should this driver be loaded.  If it is a PnP driver and has at least
    // one device instance enabled, it will be loaded. If it is a legacy driver,
    // we need to check if its device has been disabled.
    //

    if ((Header) &&
        (Header->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_WDM_DRIVER)) {
        IsPlugPlayDriver = TRUE;
    }
    if (IsPlugPlayDriver) {

        //
        // BUGBUG - For SUR, we load the driver only if device instance list is not
        // empty and at least one device instance is enabled.
        //

        if (!IopIsAnyDeviceInstanceEnabled(KeyName, KeyHandle, FALSE)) {
            if (PnPDetectionEnabled) {

                //
                // We need to load WDM driver on TextMode Setup phase.
                //

                status = STATUS_SUCCESS;
            } else {
                status = STATUS_PLUGPLAY_NO_DEVICE;
           }
           goto exit;

        }
    } else {

        //
        // If this is a legacy driver, then we need to check if its
        // device has been disabled. (There should be NO more than one device
        // instance for legacy driver.)
        //

        if (!IopIsAnyDeviceInstanceEnabled(KeyName, KeyHandle, TRUE)) {

            //
            // If the device is not enabled, it may be because there is no device
            // instance.  If this is the case, we need to create a madeup key as
            // the device instance for the legacy driver.
            //

            //
            // First open registry ServiceKeyName\Enum branch
            //

            PiWstrToUnicodeString(&unicodeKeyName, REGSTR_KEY_ENUM);
            status = IopOpenRegistryKey(&serviceEnumHandle,
                                        KeyHandle,
                                        &unicodeKeyName,
                                        KEY_ALL_ACCESS,
                                        TRUE
                                        );

            if (!NT_SUCCESS( status )) {
                goto exit;
            }

            //
            // Find out how many device instances listed in the ServiceName's
            // Enum key.
            //

            status = IopGetRegistryValue ( serviceEnumHandle,
                                           REGSTR_VALUE_COUNT,
                                           &keyValueInformation
                                           );
            if (NT_SUCCESS(status)) {
                if ((keyValueInformation->Type == REG_DWORD) &&
                    (keyValueInformation->DataLength >= sizeof(ULONG))) {

                    count = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
                }
                ExFreePool(keyValueInformation);
            } else if (status != STATUS_OBJECT_PATH_NOT_FOUND &&
                       status != STATUS_OBJECT_NAME_NOT_FOUND) {

                ZwClose(serviceEnumHandle);
                goto exit;
            }

            if (count == 0) {

                //
                // If there is no Enum key or instance under Enum for the
                // legacy driver we will create a madeup node for it.
                //

                status = IopCreateMadeupNode(KeyName,
                                             &sysEnumXxxHandle,
                                             &unicodeKeyName,
                                             &tmp,
                                             TRUE);

                if (!NT_SUCCESS(status)) {
                    ZwClose(serviceEnumHandle);
                    goto exit;
                }
                RtlFreeUnicodeString(&unicodeKeyName);

                //
                // Create and set Control\ActiveService value
                //

                PiWstrToUnicodeString(&unicodeValueName, REGSTR_KEY_CONTROL);
                status = IopOpenRegistryKey(&controlHandle,
                                            sysEnumXxxHandle,
                                            &unicodeValueName,
                                            KEY_ALL_ACCESS,
                                            TRUE
                                            );
                if (NT_SUCCESS(status)) {
                    PiWstrToUnicodeString(&unicodeValueName, REGSTR_VAL_ACTIVESERVICE);
                    ZwSetValueKey(
                                controlHandle,
                                &unicodeValueName,
                                TITLE_INDEX_VALUE,
                                REG_SZ,
                                KeyName->Buffer,
                                KeyName->Length + sizeof(UNICODE_NULL)
                                );

                    ZwClose(controlHandle);
                }
                count++;

                //
                // Don't forget to update the "Count=" and "NextInstance=" value entries
                //

                PiWstrToUnicodeString( &unicodeValueName, REGSTR_VALUE_COUNT);

                ZwSetValueKey(serviceEnumHandle,
                              &unicodeValueName,
                              TITLE_INDEX_VALUE,
                              REG_DWORD,
                              &count,
                              sizeof (count)
                              );
                PiWstrToUnicodeString( &unicodeValueName, REGSTR_VALUE_NEXT_INSTANCE);

                ZwSetValueKey(serviceEnumHandle,
                              &unicodeValueName,
                              TITLE_INDEX_VALUE,
                              REG_DWORD,
                              &count,
                              sizeof (count)
                              );
                ZwClose(sysEnumXxxHandle);
                ZwClose(serviceEnumHandle);
            } else {
                ZwClose(serviceEnumHandle);
                status = STATUS_PLUGPLAY_NO_DEVICE;
                goto exit;
            }
        }
    }

    status = STATUS_SUCCESS;
exit:
    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();
    return status;
}

NTSTATUS
IopServiceInstanceToDeviceInstance (
    IN  HANDLE ServiceKeyHandle OPTIONAL,
    IN  PUNICODE_STRING ServiceKeyName OPTIONAL,
    IN  ULONG ServiceInstanceOrdinal,
    OUT PUNICODE_STRING DeviceInstanceRegistryPath OPTIONAL,
    OUT PHANDLE DeviceInstanceHandle OPTIONAL,
    IN  ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This routine reads the service node enum entry to find the desired device instance
    under the System\Enum tree.  It then optionally returns the registry path of the
    specified device instance (relative to HKLM\System\Enum) and an open handle
    to that registry key.

    It is the caller's responsibility to close the handle returned if
    DeviceInstanceHandle is supplied, and also to free the (PagedPool) memory
    allocated for the unicode string buffer of DeviceInstanceRegistryPath, if
    supplied.

Parameters:

    ServiceKeyHandle - Optionally, supplies a handle to the driver service node in the
        registry that controls this device instance.  If this argument is not specified,
        then ServiceKeyName is used to specify the service entry.

    ServiceKeyName - Optionally supplies the name of the service entry that controls
        the device instance. This must be specified if ServiceKeyHandle isn't given.

    ServiceInstanceOrdinal - Supplies the instance value under the service entry's
        volatile Enum subkey that references the desired device instance.

    DeviceInstanceRegistryPath - Optionally, supplies a pointer to a unicode string
        that will be initialized with the registry path (relative to HKLM\System\Enum)
        to the device instance key.

    DeviceInstanceHandle - Optionally, supplies a pointer to a variable that will
        receive a handle to the opened device instance registry key.

    DesiredAccess - If DeviceInstanceHandle is specified (i.e., the device instance
        key is to be opened), then this variable specifies the access that is needed
        to this key.

Return Value:

    NT status code indicating whether the function was successful.

--*/

{
    WCHAR unicodeBuffer[20];
    UNICODE_STRING unicodeKeyName;
    NTSTATUS status;
    HANDLE handle;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;

    //
    // Open registry ServiceKeyName\Enum branch
    //
    if(ARGUMENT_PRESENT(ServiceKeyHandle)) {

        PiWstrToUnicodeString(&unicodeKeyName, REGSTR_KEY_ENUM);
        status = IopOpenRegistryKey(&handle,
                                    ServiceKeyHandle,
                                    &unicodeKeyName,
                                    KEY_READ,
                                    FALSE
                                    );
    } else {

        status = IopOpenServiceEnumKeys(ServiceKeyName,
                                        KEY_READ,
                                        NULL,
                                        &handle,
                                        FALSE
                                       );
    }

    if (!NT_SUCCESS( status )) {

        //
        // There is no registry key for the ServiceKeyName\Enum information.
        //

        return status;
    }

    //
    // Read a path to System\Enum hardware tree branch specified by the service
    // instance ordinal
    //

    swprintf(unicodeBuffer, REGSTR_VALUE_STANDARD_ULONG_FORMAT, ServiceInstanceOrdinal);
    status = IopGetRegistryValue ( handle,
                                   unicodeBuffer,
                                   &keyValueInformation
                                   );

    ZwClose(handle);
    if (!NT_SUCCESS( status )) {
        return status;
    } else {
        if(keyValueInformation->Type == REG_SZ) {
            IopRegistryDataToUnicodeString(&unicodeKeyName,
                                           (PWSTR)KEY_VALUE_DATA(keyValueInformation),
                                           keyValueInformation->DataLength
                                          );
            if(!unicodeKeyName.Length) {
                status = STATUS_OBJECT_PATH_NOT_FOUND;
            }
        } else {
            status = STATUS_INVALID_PLUGPLAY_DEVICE_PATH;
        }

        if(!NT_SUCCESS(status)) {
            goto PrepareForReturn;
        }
    }

    //
    // If the DeviceInstanceHandle argument was specified, open the device instance
    // key under HKLM\System\CurrentControlSet\Enum
    //

    if (ARGUMENT_PRESENT(DeviceInstanceHandle)) {

        status = IopOpenRegistryKey(&handle,
                                    NULL,
                                    &CmRegistryMachineSystemCurrentControlSetEnumName,
                                    KEY_READ,
                                    FALSE
                                    );

        if (NT_SUCCESS( status )) {

            status = IopOpenRegistryKey (DeviceInstanceHandle,
                                         handle,
                                         &unicodeKeyName,
                                         DesiredAccess,
                                         FALSE
                                         );
            ZwClose(handle);
        }

        if (!NT_SUCCESS( status )) {
            goto PrepareForReturn;
        }
    }

    //
    // If the DeviceInstanceRegistryPath argument was specified, then store a
    // copy of the device instance path in the supplied unicode string variable.
    //
    if (ARGUMENT_PRESENT(DeviceInstanceRegistryPath)) {

        if (!IopConcatenateUnicodeStrings(DeviceInstanceRegistryPath,
                                          &unicodeKeyName,
                                          NULL)) {

            if(ARGUMENT_PRESENT(DeviceInstanceHandle)) {
                ZwClose(*DeviceInstanceHandle);
            }
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

PrepareForReturn:

    ExFreePool(keyValueInformation);
    return status;
}

NTSTATUS
IopOpenRegistryKeyPersist(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN Create,
    OUT PULONG Disposition OPTIONAL
    )

/*++

Routine Description:

    Opens or creates a PERSIST (non-volatile) registry key using the name
    passed in based at the BaseHandle node. This name may specify a key
    that is actually a registry path, in which case each intermediate subkey
    will be created (if Create is TRUE).

    NOTE: Creating a registry path (i.e., more than one of the keys in the path
    do not presently exist) requires that a BaseHandle be specified.

Arguments:

    Handle - Pointer to the handle which will contain the registry key that
        was opened.

    BaseHandle - Optional handle to the base path from which the key must be opened.
        If KeyName specifies a registry path that must be created, then this parameter
        must be specified, and KeyName must be a relative path.

    KeyName - Name of the Key that must be opened/created (possibly a registry path)

    DesiredAccess - Specifies the desired access that the caller needs to
        the key.

    Create - Determines if the key is to be created if it does not exist.

    Disposition - If Create is TRUE, this optional pointer receives a ULONG indicating
        whether the key was newly created:

            REG_CREATED_NEW_KEY - A new Registry Key was created
            REG_OPENED_EXISTING_KEY - An existing Registry Key was opened

Return Value:

   The function value is the final status of the operation.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG disposition, baseHandleIndex = 0, keyHandleIndex = 1, closeBaseHandle;
    HANDLE handles[2];
    BOOLEAN continueParsing;
    PWCHAR pathEndPtr, pathCurPtr, pathBeginPtr;
    ULONG pathComponentLength;
    UNICODE_STRING unicodeString;
    NTSTATUS status;

    PAGED_CODE();

    InitializeObjectAttributes(&objectAttributes,
                               KeyName,
                               OBJ_CASE_INSENSITIVE,
                               BaseHandle,
                               (PSECURITY_DESCRIPTOR) NULL
                              );
    if(Create) {
        //
        // Attempt to create the path as specified. We have to try it this
        // way first, because it allows us to create a key without a BaseHandle
        // (if only the last component of the registry path is not present).
        //
        status = ZwCreateKey(&(handles[keyHandleIndex]),
                             DesiredAccess,
                             &objectAttributes,
                             0,
                             (PUNICODE_STRING) NULL,
                             REG_OPTION_NON_VOLATILE,
                             &disposition
                            );

        if(!((status == STATUS_OBJECT_NAME_NOT_FOUND) && ARGUMENT_PRESENT(BaseHandle))) {
            //
            // Then either we succeeded, or failed, but there's nothing we can do
            // about it. In either case, prepare to return.
            //
            goto PrepareForReturn;
        }

    } else {
        //
        // Simply attempt to open the path, as specified.
        //
        return ZwOpenKey(Handle,
                         DesiredAccess,
                         &objectAttributes
                        );
    }

    //
    // If we get to here, then there must be more than one element of the
    // registry path that does not currently exist.  We will now parse the
    // specified path, extracting each component and doing a ZwCreateKey on it.
    //
    handles[baseHandleIndex] = NULL;
    handles[keyHandleIndex] = BaseHandle;
    closeBaseHandle = 0;
    continueParsing = TRUE;
    pathBeginPtr = KeyName->Buffer;
    pathEndPtr = (PWCHAR)((PCHAR)pathBeginPtr + KeyName->Length);
    status = STATUS_SUCCESS;

    while(continueParsing) {
        //
        // There's more to do, so close the previous base handle (if necessary),
        // and replace it with the current key handle.
        //
        if(closeBaseHandle > 1) {
            ZwClose(handles[baseHandleIndex]);
        }
        baseHandleIndex = keyHandleIndex;
        keyHandleIndex = (keyHandleIndex + 1) & 1;  // toggle between 0 and 1.
        handles[keyHandleIndex] = NULL;

        //
        // Extract next component out of the specified registry path.
        //
        for(pathCurPtr = pathBeginPtr;
            ((pathCurPtr < pathEndPtr) && (*pathCurPtr != OBJ_NAME_PATH_SEPARATOR));
            pathCurPtr++);

        if((pathComponentLength = (ULONG)((PCHAR)pathCurPtr - (PCHAR)pathBeginPtr))) {
            //
            // Then we have a non-empty path component (key name).  Attempt
            // to create this key.
            //
            unicodeString.Buffer = pathBeginPtr;
            unicodeString.Length = unicodeString.MaximumLength = (USHORT)pathComponentLength;

            InitializeObjectAttributes(&objectAttributes,
                                       &unicodeString,
                                       OBJ_CASE_INSENSITIVE,
                                       handles[baseHandleIndex],
                                       (PSECURITY_DESCRIPTOR) NULL
                                      );
            status = ZwCreateKey(&(handles[keyHandleIndex]),
                                 DesiredAccess,
                                 &objectAttributes,
                                 0,
                                 (PUNICODE_STRING) NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 &disposition
                                );
            if(NT_SUCCESS(status)) {
                //
                // Increment the closeBaseHandle value, which basically tells us whether
                // the BaseHandle passed in has been 'shifted out' of our way, so that
                // we should start closing our base handles when we're finished with them.
                //
                closeBaseHandle++;
            } else {
                continueParsing = FALSE;
                continue;
            }
        } else {
            //
            // Either a path separator ('\') was included at the beginning of
            // the path, or we hit 2 consecutive separators.
            //
            status = STATUS_INVALID_PARAMETER;
            continueParsing = FALSE;
            continue;
        }

        if((pathCurPtr == pathEndPtr) ||
           ((pathBeginPtr = pathCurPtr + 1) == pathEndPtr)) {
            //
            // Then we've reached the end of the path
            //
            continueParsing = FALSE;
        }
    }

    if(closeBaseHandle > 1) {
        ZwClose(handles[baseHandleIndex]);
    }

PrepareForReturn:

    if(NT_SUCCESS(status)) {
        *Handle = handles[keyHandleIndex];

        if(ARGUMENT_PRESENT(Disposition)) {
            *Disposition = disposition;
        }
    }

    return status;
}

NTSTATUS
IopOpenServiceEnumKeys (
    IN PUNICODE_STRING ServiceKeyName,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE ServiceHandle OPTIONAL,
    OUT PHANDLE ServiceEnumHandle OPTIONAL,
    IN BOOLEAN CreateEnum
    )

/*++

Routine Description:

    This routine opens the HKEY_LOCAL_MACHINE\CurrentControlSet\Services\
    ServiceKeyName and its Enum subkey and returns handles for both key.
    It is caller's responsibility to close the returned handles.

Arguments:

    ServiceKeyName - Supplies a pointer to the name of the subkey in the
        system service list (HKEY_LOCAL_MACHINE\CurrentControlSet\Services)
        that caused the driver to load. This is the RegistryPath parameter
        to the DriverEntry routine.

    DesiredAccess - Specifies the desired access to the keys.

    ServiceHandle - Supplies a variable to receive a handle to ServiceKeyName.
        A NULL ServiceHandle indicates caller does not want need the handle to
        the ServiceKeyName.

    ServiceEnumHandle - Supplies a variable to receive a handle to ServiceKeyName\Enum.
        A NULL ServiceEnumHandle indicates caller does not need the handle to
        the ServiceKeyName\Enum.

    CreateEnum - Supplies a BOOLEAN variable to indicate should the Enum subkey be
        created if not present.

Return Value:

    status

--*/

{
    HANDLE handle, serviceHandle, enumHandle;
    UNICODE_STRING enumName;
    NTSTATUS status;

    //
    // Open System\CurrentControlSet\Services
    //

    status = IopOpenRegistryKey(&handle,
                                NULL,
                                &CmRegistryMachineSystemCurrentControlSetServices,
                                DesiredAccess,
                                FALSE
                                );

    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // Open the registry ServiceKeyName key.
    //

    status = IopOpenRegistryKey(&serviceHandle,
                                handle,
                                ServiceKeyName,
                                DesiredAccess,
                                FALSE
                                );

    ZwClose(handle);
    if (!NT_SUCCESS( status )) {

        //
        // There is no registry key for the ServiceKeyName information.
        //

        return status;
    }

    if (ARGUMENT_PRESENT(ServiceEnumHandle) || CreateEnum) {

        //
        // Open registry ServiceKeyName\Enum branch if caller wants
        // the handle or wants to create it.
        //

        PiWstrToUnicodeString(&enumName, REGSTR_KEY_ENUM);
        status = IopOpenRegistryKey(&enumHandle,
                                    serviceHandle,
                                    &enumName,
                                    DesiredAccess,
                                    CreateEnum
                                    );

        if (!NT_SUCCESS( status )) {

            //
            // There is no registry key for the ServiceKeyName\Enum information.
            //

            ZwClose(serviceHandle);
            return status;
        }
        if (ARGUMENT_PRESENT(ServiceEnumHandle)) {
            *ServiceEnumHandle = enumHandle;
        } else {
            ZwClose(enumHandle);
        }
    }

    //
    // if caller wants to have the ServiceKey handle, we return it.  Otherwise
    // we close it.
    //

    if (ARGUMENT_PRESENT(ServiceHandle)) {
        *ServiceHandle = serviceHandle;
    } else {
        ZwClose(serviceHandle);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IopGetDeviceInstanceCsConfigFlags(
    IN PUNICODE_STRING ServiceKeyName,
    IN ULONG Instance,
    OUT PULONG CsConfigFlags
    )

/*++

Routine Description:

    This routine retrieves the csconfig flags for the specified device
    which is specified by the instance number under ServiceKeyName\Enum.

Arguments:

    ServiceKeyName - Supplies a pointer to the name of the subkey in the
        system service list (HKEY_LOCAL_MACHINE\CurrentControlSet\Services)
        that caused the driver to load.

    Instance - Supplies the instance value under ServiceKeyName\Enum key

    CsConfigFlags - Supplies a variable to receive the device's CsConfigFlags

Return Value:

    status

--*/

{
    NTSTATUS status;
    HANDLE handle;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;

    *CsConfigFlags = 0;

    status = IopOpenCurrentHwProfileDeviceInstanceKey(&handle,
                                                      ServiceKeyName,
                                                      Instance,
                                                      KEY_READ,
                                                      FALSE
                                                     );
    if(NT_SUCCESS(status)) {
        status = IopGetRegistryValue(handle,
                                     REGSTR_VALUE_CSCONFIG_FLAGS,
                                     &keyValueInformation
                                    );
        if(NT_SUCCESS(status)) {
            if((keyValueInformation->Type == REG_DWORD) &&
               (keyValueInformation->DataLength >= sizeof(ULONG))) {
                *CsConfigFlags = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
            }
            ExFreePool(keyValueInformation);
        }
        ZwClose(handle);
    }
    return status;
}

NTSTATUS
IopSetDeviceInstanceCsConfigFlags(
    IN PUNICODE_STRING ServiceKeyName,
    IN ULONG Instance,
    IN ULONG CsConfigFlags
    )

/*++

Routine Description:

    This routine sets the csconfig flags for the specified device
    which is specified by the instance number under ServiceKeyName\Enum.

Arguments:

    ServiceKeyName - Supplies a pointer to the name of the subkey in the
        system service list (HKEY_LOCAL_MACHINE\CurrentControlSet\Services)
        that caused the driver to load. This is the RegistryPath parameter
        to the DriverEntry routine.

    Instance - Supplies the instance value under ServiceKeyName\Enum key

    CsConfigFlags - Supplies the device instance's new CsConfigFlags

Return Value:

    status

--*/

{
    HANDLE handle;
    NTSTATUS status;
    UNICODE_STRING tempUnicodeString;

    status = IopOpenCurrentHwProfileDeviceInstanceKey(&handle,
                                                      ServiceKeyName,
                                                      Instance,
                                                      KEY_READ | KEY_WRITE,
                                                      TRUE
                                                     );
    if(NT_SUCCESS(status)) {

        PiWstrToUnicodeString(&tempUnicodeString, REGSTR_VALUE_CSCONFIG_FLAGS);
        status = ZwSetValueKey(handle,
                               &tempUnicodeString,
                               TITLE_INDEX_VALUE,
                               REG_DWORD,
                               &CsConfigFlags,
                               sizeof(CsConfigFlags)
                              );
        ZwClose(handle);
    }
    return status;
}

NTSTATUS
IopOpenCurrentHwProfileDeviceInstanceKey(
    OUT PHANDLE Handle,
    IN  PUNICODE_STRING ServiceKeyName,
    IN  ULONG Instance,
    IN  ACCESS_MASK DesiredAccess,
    IN  BOOLEAN Create
    )

/*++

Routine Description:

    This routine sets the csconfig flags for the specified device
    which is specified by the instance number under ServiceKeyName\Enum.

Arguments:

    ServiceKeyName - Supplies a pointer to the name of the subkey in the
        system service list (HKEY_LOCAL_MACHINE\CurrentControlSet\Services)
        that caused the driver to load. This is the RegistryPath parameter
        to the DriverEntry routine.

    Instance - Supplies the instance value under ServiceKeyName\Enum key

    DesiredAccess - Specifies the desired access that the caller needs to
        the key.

    Create - Determines if the key is to be created if it does not exist.

Return Value:

    status

--*/

{
    NTSTATUS status;
    UNICODE_STRING tempUnicodeString;
    HANDLE profileHandle, profileEnumHandle, tmpHandle;

    //
    // See if we can open current hardware profile
    //
    status = IopOpenRegistryKey(&profileHandle,
                                NULL,
                                &CmRegistryMachineSystemCurrentControlSetHardwareProfilesCurrent,
                                KEY_READ,
                                Create
                                );
    if(NT_SUCCESS(status)) {
        //
        // Now, we must open the System\CCS\Enum key under this.
        //
        //
        // Open system\CurrentControlSet under current hardware profile key
        //

        PiWstrToUnicodeString(&tempUnicodeString, REGSTR_PATH_CURRENTCONTROLSET);
        status = IopOpenRegistryKey(&tmpHandle,
                                    profileHandle,
                                    &tempUnicodeString,
                                    DesiredAccess,
                                    FALSE
                                    );
        ZwClose(profileHandle);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        PiWstrToUnicodeString(&tempUnicodeString, REGSTR_KEY_ENUM);
        status = IopOpenRegistryKey(&profileEnumHandle,
                                    tmpHandle,
                                    &tempUnicodeString,
                                    KEY_READ,
                                    Create
                                    );
        ZwClose(tmpHandle);
        if(NT_SUCCESS(status)) {

            status = IopServiceInstanceToDeviceInstance(NULL,
                                                        ServiceKeyName,
                                                        Instance,
                                                        &tempUnicodeString,
                                                        NULL,
                                                        0
                                                       );
            if(NT_SUCCESS(status)) {
                status = IopOpenRegistryKey(Handle,
                                            profileEnumHandle,
                                            &tempUnicodeString,
                                            DesiredAccess,
                                            Create
                                            );
                RtlFreeUnicodeString(&tempUnicodeString);
            }
            ZwClose(profileEnumHandle);
        }
    }
    return status;
}

NTSTATUS
IopApplyFunctionToSubKeys(
    IN     HANDLE BaseHandle OPTIONAL,
    IN     PUNICODE_STRING KeyName OPTIONAL,
    IN     ACCESS_MASK DesiredAccess,
    IN     BOOLEAN IgnoreNonCriticalErrors,
    IN     PIOP_SUBKEY_CALLBACK_ROUTINE SubKeyCallbackRoutine,
    IN OUT PVOID Context
    )

/*++

Routine Description:

    This routine enumerates all subkeys under the specified key, and calls
    the specified callback routine for each subkey.

Arguments:

    BaseHandle - Optional handle to the base registry path. If KeyName is also
        specified, then KeyName represents a subkey under this path.  If KeyName
        is not specified, the subkeys are enumerated under this handle.  If this
        parameter is not specified, then the full path to the base key must be
        given in KeyName.

    KeyName - Optional name of the key whose subkeys are to be enumerated.

    DesiredAccess - Specifies the desired access that the callback routine
        needs to the subkeys.  If no desired access is specified (i.e.,
        DesiredAccess is zero), then no handle will be opened for the
        subkeys, and the callback will be passed a NULL for its SubKeyHandle
        parameter.

    IgnoreNonCriticalErrors - Specifies whether this function should
        immediately terminate on all errors, or only on critical ones.
        An example of a non-critical error is when an enumerated subkey
        cannot be opened for the desired access.

    SubKeyCallbackRoutine - Supplies a pointer to a function that will
        be called for each subkey found under the
        specified key.  The prototype of the function
        is as follows:

            typedef BOOLEAN (*PIOP_SUBKEY_CALLBACK_ROUTINE) (
                IN     HANDLE SubKeyHandle,
                IN     PUNICODE_STRING SubKeyName,
                IN OUT PVOID Context
                );

        where SubKeyHandle is the handle to an enumerated subkey under the
        specified key, SubKeyName is its name, and Context is a pointer to
        user-defined data.

        This function should return TRUE to continue enumeration, or
        FALSE to terminate it.

    Context - Supplies a pointer to user-defined data that will be passed
        in to the callback routine at each subkey invocation.

Return Value:

    NT status code indicating whether the subkeys were successfully
    enumerated.  Note that this does not provide information on the
    success or failure of the callback routine--if desired, this
    information should be stored in the Context structure.

--*/

{
    NTSTATUS Status;
    BOOLEAN CloseHandle = FALSE, ContinueEnumeration;
    HANDLE Handle, SubKeyHandle;
    ULONG i, RequiredBufferLength;
    PKEY_BASIC_INFORMATION KeyInformation = NULL;
    // Use an initial key name buffer size large enough for a 20-character key
    // (+ terminating NULL)
    ULONG KeyInformationLength = sizeof(KEY_BASIC_INFORMATION) + (20 * sizeof(WCHAR));
    UNICODE_STRING SubKeyName;

    if(ARGUMENT_PRESENT(KeyName)) {

        Status = IopOpenRegistryKey(&Handle,
                                    BaseHandle,
                                    KeyName,
                                    KEY_READ,
                                    FALSE
                                   );
        if(!NT_SUCCESS(Status)) {
            return Status;
        } else {
            CloseHandle = TRUE;
        }

    } else {

        Handle = BaseHandle;
    }

    //
    // Enumerate the subkeys until we run out of them.
    //
    i = 0;
    SubKeyHandle = NULL;

    while(TRUE) {

        if(!KeyInformation) {

            KeyInformation = (PKEY_BASIC_INFORMATION)ExAllocatePool(PagedPool,
                                                                    KeyInformationLength
                                                                   );
            if(!KeyInformation) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
        }

        Status = ZwEnumerateKey(Handle,
                                i,
                                KeyBasicInformation,
                                KeyInformation,
                                KeyInformationLength,
                                &RequiredBufferLength
                               );

        if(!NT_SUCCESS(Status)) {
            if(Status == STATUS_BUFFER_OVERFLOW) {
                //
                // Try again with larger buffer.
                //
                ExFreePool(KeyInformation);
                KeyInformation = NULL;
                KeyInformationLength = RequiredBufferLength;
                continue;

            } else if(Status == STATUS_NO_MORE_ENTRIES) {
                //
                // break out of loop
                //
                Status = STATUS_SUCCESS;
                break;

            } else {
                //
                // This is a non-critical error.
                //
                if(IgnoreNonCriticalErrors) {
                    goto ContinueWithNextSubKey;
                } else {
                    break;
                }
            }
        }

        //
        // Initialize a unicode string with this key name.  Note that this string
        // WILL NOT be NULL-terminated.
        //
        SubKeyName.Length = SubKeyName.MaximumLength = (USHORT)KeyInformation->NameLength;
        SubKeyName.Buffer = KeyInformation->Name;

        //
        // If DesiredAccess is non-zero, open a handle to this subkey.
        //
        if(DesiredAccess) {
            Status = IopOpenRegistryKey(&SubKeyHandle,
                                        Handle,
                                        &SubKeyName,
                                        DesiredAccess,
                                        FALSE
                                       );
            if(!NT_SUCCESS(Status)) {
                //
                // This is a non-critical error.
                //
                if(IgnoreNonCriticalErrors) {
                    goto ContinueWithNextSubKey;
                } else {
                    break;
                }
            }
        }

        //
        // Invoke the supplied callback function for this subkey.
        //
        ContinueEnumeration = SubKeyCallbackRoutine(SubKeyHandle, &SubKeyName, Context);

        if(DesiredAccess) {
            ZwClose(SubKeyHandle);
        }

        if(!ContinueEnumeration) {
            //
            // Enumeration has been aborted.
            //
            Status = STATUS_SUCCESS;
            break;

        }

ContinueWithNextSubKey:

        i++;
    }

    if(KeyInformation) {
        ExFreePool(KeyInformation);
    }

    if(CloseHandle) {
        ZwClose(Handle);
    }

    return Status;
}

NTSTATUS
IopRegMultiSzToUnicodeStrings(
    IN  PKEY_VALUE_FULL_INFORMATION KeyValueInformation,
    OUT PUNICODE_STRING *UnicodeStringList,
    OUT PULONG UnicodeStringCount
    )

/*++

Routine Description:

    This routine takes a KEY_VALUE_FULL_INFORMATION structure containing
    a REG_MULTI_SZ value, and allocates an array of UNICODE_STRINGs,
    initializing each one to a copy of one of the strings in the value entry.
    All the resulting UNICODE_STRINGs will be NULL terminated
    (MaximumLength = Length + sizeof(UNICODE_NULL)).

    It is the responsibility of the caller to free the buffers for each
    unicode string, as well as the buffer containing the UNICODE_STRING
    array. This may be done by calling IopFreeUnicodeStringList.

Arguments:

    KeyValueInformation - Supplies the buffer containing the REG_MULTI_SZ
        value entry data.

    UnicodeStringList - Receives a pointer to an array of UNICODE_STRINGs, each
        initialized with a copy of one of the strings in the REG_MULTI_SZ.

    UnicodeStringCount - Receives the number of strings in the
        UnicodeStringList.

Returns:

    NT status code indicating whether the function was successful.

--*/

{
    PWCHAR p, BufferEnd, StringStart;
    ULONG StringCount, i, StringLength;

    //
    // First, make sure this is really a REG_MULTI_SZ value.
    //
    if(KeyValueInformation->Type != REG_MULTI_SZ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make a preliminary pass through the buffer to count the number of strings
    // There will always be at least one string returned (possibly empty).
    //
    StringCount = 0;
    p = (PWCHAR)KEY_VALUE_DATA(KeyValueInformation);
    BufferEnd = (PWCHAR)((PUCHAR)p + KeyValueInformation->DataLength);
    while(p != BufferEnd) {
        if(!*p) {
            StringCount++;
            if(((p + 1) == BufferEnd) || !*(p + 1)) {
                break;
            }
        }
        p++;
    }
    if(p == BufferEnd) {
        StringCount++;
    }

    *UnicodeStringList = ExAllocatePool(PagedPool, sizeof(UNICODE_STRING) * StringCount);
    if(!(*UnicodeStringList)) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now, make a second pass through the buffer making copies of each string.
    //
    i = 0;
    StringStart = p = (PWCHAR)KEY_VALUE_DATA(KeyValueInformation);
    while(p != BufferEnd) {
        if(!*p) {
            StringLength = (ULONG)((PUCHAR)p - (PUCHAR)StringStart) + sizeof(UNICODE_NULL);
            (*UnicodeStringList)[i].Buffer = ExAllocatePool(PagedPool, StringLength);

            if(!((*UnicodeStringList)[i].Buffer)) {
                IopFreeUnicodeStringList(*UnicodeStringList, i);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlMoveMemory((*UnicodeStringList)[i].Buffer, StringStart, StringLength);

            (*UnicodeStringList)[i].Length =
                ((*UnicodeStringList)[i].MaximumLength = (USHORT)StringLength)
                - sizeof(UNICODE_NULL);

            i++;

            if(((p + 1) == BufferEnd) || !*(p + 1)) {
                break;
            } else {
                StringStart = p + 1;
            }
        }
        p++;
    }
    if(p == BufferEnd) {
        StringLength = (ULONG)((PUCHAR)p - (PUCHAR)StringStart);
        (*UnicodeStringList)[i].Buffer = ExAllocatePool(PagedPool,
                                                        StringLength + sizeof(UNICODE_NULL)
                                                       );
        if(!((*UnicodeStringList)[i].Buffer)) {
            IopFreeUnicodeStringList(*UnicodeStringList, i);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        if(StringLength) {
            RtlMoveMemory((*UnicodeStringList)[i].Buffer, StringStart, StringLength);
        }
        (*UnicodeStringList)[i].Buffer[CB_TO_CWC(StringLength)] = UNICODE_NULL;

        (*UnicodeStringList)[i].MaximumLength =
                ((*UnicodeStringList)[i].Length = (USHORT)StringLength)
                + sizeof(UNICODE_NULL);
    }

    *UnicodeStringCount = StringCount;

    return STATUS_SUCCESS;
}

NTSTATUS
IopApplyFunctionToServiceInstances(
    IN     HANDLE ServiceKeyHandle OPTIONAL,
    IN     PUNICODE_STRING ServiceKeyName OPTIONAL,
    IN     ACCESS_MASK DesiredAccess,
    IN     BOOLEAN IgnoreNonCriticalErrors,
    IN     PIOP_SUBKEY_CALLBACK_ROUTINE DevInstCallbackRoutine,
    IN OUT PVOID Context,
    OUT    PULONG ServiceInstanceOrdinal OPTIONAL
    )

/*++

Routine Description:

    This routine enumerates all device instances referenced by the instance
    ordinal entries under a service's volatile Enum key, and calls
    the specified callback routine for each instance's corresponding subkey
    under HKLM\System\Enum.

Arguments:

    ServiceKeyHandle - Optional handle to the service entry. If this parameter
        is not specified, then the service key name must be given in
        ServiceKeyName (if both parameters are specified, then ServiceKeyHandle
        is used, and ServiceKeyName is ignored).

    ServiceKeyName - Optional name of the service entry key (under
        HKLM\CurrentControlSet\Services). If this parameter is not specified,
        then ServiceKeyHandle must contain a handle to the desired service key.

    DesiredAccess - Specifies the desired access that the callback routine
        needs to the enumerated device instance keys.  If no desired access is
        specified (i.e., DesiredAccess is zero), then no handle will be opened
        for the device instance keys, and the callback will be passed a NULL for
        its DeviceInstanceHandle parameter.

    IgnoreNonCriticalErrors - Specifies whether this function should
        immediately terminate on all errors, or only on critical ones.
        An example of a non-critical error is when an enumerated device instance
        key cannot be opened for the desired access.

    DevInstCallbackRoutine - Supplies a pointer to a function that will
        be called for each device instance key referenced by a service instance
        entry under the service's volatile Enum subkey. The prototype of the
        function is as follows:

            typedef BOOLEAN (*PIOP_SUBKEY_CALLBACK_ROUTINE) (
                IN     HANDLE DeviceInstanceHandle,
                IN     PUNICODE_STRING DeviceInstancePath,
                IN OUT PVOID Context
                );

        where DeviceInstanceHandle is the handle to an enumerated device instance
        key, DeviceInstancePath is the registry path (relative to
        HKLM\System\Enum) to this device instance, and Context is a pointer to
        user-defined data.

        This function should return TRUE to continue enumeration, or
        FALSE to terminate it.

    Context - Supplies a pointer to user-defined data that will be passed
        in to the callback routine at each device instance key invocation.

    ServiceInstanceOrdinal - Optionally, receives the service instance ordinal (1 based)
        that terminated the enumeration, or the total number of instances enumerated
        if the enumeration completed without being aborted.

Return Value:

    NT status code indicating whether the device instance keys were successfully
    enumerated.  Note that this does not provide information on the success or
    failure of the callback routine--if desired, this information should be
    stored in the Context structure.

--*/

{
    NTSTATUS Status;
    HANDLE ServiceEnumHandle, SystemEnumHandle, DeviceInstanceHandle;
    UNICODE_STRING TempUnicodeString;
    ULONG ServiceInstanceCount, i, junk;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    WCHAR ValueNameString[20];
    BOOLEAN ContinueEnumeration;

    //
    // First, open up the volatile Enum subkey under the specified service entry.
    //

    if(ARGUMENT_PRESENT(ServiceKeyHandle)) {
        PiWstrToUnicodeString(&TempUnicodeString, REGSTR_KEY_ENUM);
        Status = IopOpenRegistryKey(&ServiceEnumHandle,
                                    ServiceKeyHandle,
                                    &TempUnicodeString,
                                    KEY_READ,
                                    FALSE
                                   );
    } else {
        Status = IopOpenServiceEnumKeys(ServiceKeyName,
                                        KEY_READ,
                                        NULL,
                                        &ServiceEnumHandle,
                                        FALSE
                                       );
    }
    if(!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Find out how many instances are referenced in the service's Enum key.
    //

    ServiceInstanceCount = 0;   // assume none.

    Status = IopGetRegistryValue(ServiceEnumHandle,
                                 REGSTR_VALUE_COUNT,
                                 &KeyValueInformation
                                );
    if(NT_SUCCESS(Status)) {

        if((KeyValueInformation->Type == REG_DWORD) &&
           (KeyValueInformation->DataLength >= sizeof(ULONG))) {

            ServiceInstanceCount = *(PULONG)KEY_VALUE_DATA(KeyValueInformation);

        }
        ExFreePool(KeyValueInformation);

    } else if(Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        goto PrepareForReturn;
    } else {
        //
        // If 'Count' value entry not found, consider this to mean there are simply
        // no device instance controlled by this service.
        //
        Status = STATUS_SUCCESS;
    }

    //
    // Now, enumerate each service instance, and call the specified callback function
    // for the corresponding device instance.
    //

    if (ServiceInstanceCount) {

        if (DesiredAccess) {
            Status = IopOpenRegistryKey(&SystemEnumHandle,
                                        NULL,
                                        &CmRegistryMachineSystemCurrentControlSetEnumName,
                                        KEY_READ,
                                        FALSE
                                       );
            if(!NT_SUCCESS(Status)) {
                goto PrepareForReturn;
            }
        } else {
            //
            // Set DeviceInstanceHandle to NULL, since we won't be opening up the
            // device instance keys.
            //
            DeviceInstanceHandle = NULL;
        }
        KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)ExAllocatePool(
                                                              PagedPool,
                                                              PNP_SCRATCH_BUFFER_SIZE);
        if (!KeyValueInformation) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto PrepareForReturn;
        }
        i = 0;
        while (TRUE) {
            Status = ZwEnumerateValueKey (           // i was initialized to zero above.
                            ServiceEnumHandle,
                            i++,
                            KeyValueFullInformation,
                            KeyValueInformation,
                            PNP_SCRATCH_BUFFER_SIZE,
                            &junk
                            );

            if (!NT_SUCCESS (Status)) {
                if (Status == STATUS_NO_MORE_ENTRIES) {
                    Status = STATUS_SUCCESS;
                    break;
                } else if(IgnoreNonCriticalErrors) {
                    continue;
                } else {
                    break;
                }
            }

            if (KeyValueInformation->Type != REG_SZ) {
                continue;
            }

            ContinueEnumeration = TRUE;
            TempUnicodeString.Length = 0;
            IopRegistryDataToUnicodeString(&TempUnicodeString,
                                           (PWSTR)KEY_VALUE_DATA(KeyValueInformation),
                                           KeyValueInformation->DataLength
                                           );
            if (TempUnicodeString.Length) {

                //
                // We have retrieved a (non-empty) string for this service instance.
                // If the user specified a non-zero value for the DesiredAccess
                // parameter, we will attempt to open up the corresponding device
                // instance key under HKLM\System\Enum.
                //
                if (DesiredAccess) {
                    Status = IopOpenRegistryKey(&DeviceInstanceHandle,
                                                SystemEnumHandle,
                                                &TempUnicodeString,
                                                DesiredAccess,
                                                FALSE
                                                );
                }

                if (NT_SUCCESS(Status)) {
                    //
                    // Invoke the specified callback routine for this device instance.
                    //
                    ContinueEnumeration = DevInstCallbackRoutine(DeviceInstanceHandle,
                                                                 &TempUnicodeString,
                                                                 Context
                                                                );
                    if (DesiredAccess) {
                        ZwClose(DeviceInstanceHandle);
                    }
                } else if (IgnoreNonCriticalErrors) {
                    continue;
                } else {
                    break;
                }
            } else {
                continue;
            }
            if (!ContinueEnumeration) {
                break;
            }
        }

        if(DesiredAccess) {
            ZwClose(SystemEnumHandle);
        }
        ExFreePool(KeyValueInformation);
    }

    if(ARGUMENT_PRESENT(ServiceInstanceOrdinal)) {
        *ServiceInstanceOrdinal = i;
    }

PrepareForReturn:

    ZwClose(ServiceEnumHandle);

    return Status;
}

NTSTATUS
IopMarkDuplicateDevice (
    IN PUNICODE_STRING TargetKeyName,
    IN ULONG TargetInstance,
    IN PUNICODE_STRING SourceKeyName,
    IN ULONG SourceInstance
    )

/*++

Routine Description:

    This routine marks the device instance specified by TargetKeyName and TargetInstance
    as DuplicateOf the device specified by SourceKeyName and SourceInstance.

Arguments:

    TargetKeyName - supplies a pointer to the name of service key which will be marked
        as duplicate.

    TargetInstance - the instance number of the target device.

    SourceKeyName - supplies a pointer to the name of service key.

    SourceInstance - the instance number of the source device.


Returns:

    NTSTATUS code.

--*/

{
    HANDLE handle;
    NTSTATUS status;
    UNICODE_STRING sourceDeviceString, unicodeValueName;

    //
    // Open the handle of the target device instance.
    //

    status = IopServiceInstanceToDeviceInstance(
                       NULL,
                       TargetKeyName,
                       TargetInstance,
                       NULL,
                       &handle,
                       0
                       );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Get the name of the source device instance
    //

    status = IopServiceInstanceToDeviceInstance(
                       NULL,
                       SourceKeyName,
                       SourceInstance,
                       &sourceDeviceString,
                       NULL,
                       0
                       );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Write the name of the source device to the DuplicateOf value entry of
    // target device key.
    //

    PiWstrToUnicodeString(&unicodeValueName, REGSTR_VALUE_DUPLICATEOF);
    status = ZwSetValueKey(
                handle,
                &unicodeValueName,
                TITLE_INDEX_VALUE,
                REG_SZ,
                &sourceDeviceString,
                sourceDeviceString.Length + sizeof(WCHAR)
                );
    return status;
}

BOOLEAN
IopIsDuplicatedDevices(
    IN PCM_RESOURCE_LIST Configuration1,
    IN PCM_RESOURCE_LIST Configuration2,
    IN PHAL_BUS_INFORMATION BusInfo1 OPTIONAL,
    IN PHAL_BUS_INFORMATION BusInfo2 OPTIONAL
    )

/*++

Routine Description:

    This routine compares two set of configurations and bus information to
    determine if the resources indicate the same device.  If BusInfo1 and
    BusInfo2 both are absent, it means caller wants to compare the raw
    resources.

Arguments:

    Configuration1 - Supplies a pointer to the first set of resource.

    Configuration2 - Supplies a pointer to the second set of resource.

    BusInfo1 - Supplies a pointer to the first set of bus information.

    BusInfo2 - Supplies a pointer to the second set of bus information.

Return Value:

    returns TRUE if the two set of resources indicate the same device;
    otherwise a value of FALSE is returned.

--*/

{
    PCM_PARTIAL_RESOURCE_LIST list1, list2;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor1, descriptor2;

    ULONG i, j;
    ULONG pass = 0;

    //
    // The BusInfo for both resources must be both present or not present.
    //

    if ((ARGUMENT_PRESENT(BusInfo1) && !ARGUMENT_PRESENT(BusInfo2)) ||
        (!ARGUMENT_PRESENT(BusInfo1) && ARGUMENT_PRESENT(BusInfo2))) {

        //
        // Unable to determine.
        //

        return FALSE;
    }

    //
    // Next check resources used by the two devices.
    // Currently, we *only* check the Io ports.
    //

    if (Configuration1->Count == 0 || Configuration2->Count == 0) {

        //
        // If any one of the configuration data is empty, we assume
        // the devices are not duplicates.
        //

        return FALSE;
    }

RedoScan:

    list1 = &(Configuration1->List[0].PartialResourceList);
    list2 = &(Configuration2->List[0].PartialResourceList);

    for(i = 0, descriptor1 = list1->PartialDescriptors;
        i < list1->Count;
        i++, descriptor1++) {

        //
        // If this is an i/o port or a memory range then look for a match
        // in the other list.
        //

        if((descriptor1->Type == CmResourceTypePort) ||
           (descriptor1->Type == CmResourceTypeMemory)) {

            for(j = 0, descriptor2 = list2->PartialDescriptors;
                j < list2->Count;
                j++, descriptor2++) {

                //
                // If the types match then check to see if both addresses
                // match as well.  If bus info was provided then go ahead
                // and translate the ranges first.
                //

                if(descriptor1->Type == descriptor2->Type) {

                    PHYSICAL_ADDRESS range1, range1Translated;
                    PHYSICAL_ADDRESS range2, range2Translated;
                    ULONG range1IoSpace, range2IoSpace;

                    range1 = descriptor1->u.Generic.Start;
                    range2 = descriptor2->u.Generic.Start;

                    if((range1.QuadPart == 0) ||
                       (BusInfo1 == NULL) ||
                       (HalTranslateBusAddress(
                            BusInfo1->BusType,
                            BusInfo1->BusNumber,
                            range1,
                            &range1IoSpace,
                            &range1Translated) == FALSE)) {

                        range1Translated = range1;
                        range1IoSpace =
                            (descriptor1->Type == CmResourceTypePort) ? TRUE :
                                                                        FALSE;
                    }

                    if((range2.QuadPart == 0) ||
                       (BusInfo2 == NULL) ||
                       (HalTranslateBusAddress(
                            BusInfo2->BusType,
                            BusInfo2->BusNumber,
                            range2,
                            &range2IoSpace,
                            &range2Translated) == FALSE)) {

                        range2Translated = range2;
                        range2IoSpace =
                            (descriptor2->Type == CmResourceTypePort) ? TRUE :
                                                                        FALSE;
                    }

                    //
                    // If the ranges are in the same space and start at the
                    // same location then break out and go on to the next
                    // range
                    //

                    if((range1Translated.QuadPart == range2Translated.QuadPart) &&
                       (range1IoSpace == range2IoSpace)) {

                        break;
                    }
                }
            }

            //
            // If we made it all the way through the resource list without
            // finding a match then these are not duplicates.
            //

            if(j == list2->Count) {
                return FALSE;
            }
        }
    }

    //
    // If every resource in list 1 exists in list 2 then we also need to make
    // sure that every resource in list 2 exists in list 1.
    //

    if(pass == 0) {

        PVOID tmp ;

        tmp = Configuration2;
        Configuration2 = Configuration1;
        Configuration1 = tmp;

        tmp = BusInfo2;
        BusInfo2 = BusInfo1;
        BusInfo1 = tmp;

        pass = 1;

        goto RedoScan;
    }

    return TRUE;
}

VOID
IopFreeUnicodeStringList(
    IN PUNICODE_STRING UnicodeStringList,
    IN ULONG StringCount
    )

/*++

Routine Description:

    This routine frees the buffer for each UNICODE_STRING in the specified list
    (there are StringCount of them), and then frees the memory used for the
    string list itself.

Arguments:

    UnicodeStringList - Supplies a pointer to an array of UNICODE_STRINGs.

    StringCount - Supplies the number of strings in the UnicodeStringList array.

Returns:

    None.

--*/

{
    ULONG i;

    if(UnicodeStringList) {
        for(i = 0; i < StringCount; i++) {
            if(UnicodeStringList[i].Buffer) {
                ExFreePool(UnicodeStringList[i].Buffer);
            }
        }
        ExFreePool(UnicodeStringList);
    }
}

NTSTATUS
IopDriverLoadingFailed(
    IN HANDLE ServiceHandle OPTIONAL,
    IN PUNICODE_STRING ServiceName OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked when driver failed to start.  All the device
    instances controlled by this driver/service are marked as failing to
    start.

Arguments:

    ServiceKeyHandle - Optionally, supplies a handle to the driver service node in the
        registry that controls this device instance.  If this argument is not specified,
        then ServiceKeyName is used to specify the service entry.

    ServiceKeyName - Optionally supplies the name of the service entry that controls
        the device instance. This must be specified if ServiceKeyHandle isn't given.

Returns:

    None.

--*/

{
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    BOOLEAN closeHandle = FALSE, deletePdo;
    HANDLE handle, serviceEnumHandle, controlHandle;
    HANDLE sysEnumHandle = NULL;
    ULONG deviceFlags, count, newCount, i, j;
    UNICODE_STRING unicodeValueName, deviceInstanceName;
    WCHAR unicodeBuffer[20];

    //
    // Open registry ServiceKeyName\Enum branch
    //

    if (!ARGUMENT_PRESENT(ServiceHandle)) {
        status = IopOpenServiceEnumKeys(ServiceName,
                                        KEY_READ,
                                        &ServiceHandle,
                                        &serviceEnumHandle,
                                        FALSE
                                        );
        closeHandle = TRUE;
    } else {
        PiWstrToUnicodeString(&unicodeValueName, REGSTR_KEY_ENUM);
        status = IopOpenRegistryKey(&serviceEnumHandle,
                                    ServiceHandle,
                                    &unicodeValueName,
                                    KEY_READ,
                                    FALSE
                                    );
    }
    if (!NT_SUCCESS( status )) {

        //
        // No Service Enum key? no device instance.  Return FALSE.
        //

        return status;
    }

    //
    // Set "STARTFAILED" flags.  So, we won't load it again.
    //

    RtlInitUnicodeString(&unicodeValueName, L"INITSTARTFAILED");
    deviceFlags = 1;
    ZwSetValueKey(
                serviceEnumHandle,
                &unicodeValueName,
                TITLE_INDEX_VALUE,
                REG_DWORD,
                &deviceFlags,
                sizeof(deviceFlags)
                );

    //
    // Find out how many device instances listed in the ServiceName's
    // Enum key.
    //

    status = IopGetRegistryValue ( serviceEnumHandle,
                                   REGSTR_VALUE_COUNT,
                                   &keyValueInformation
                                   );
    count = 0;
    if (NT_SUCCESS(status)) {
        if ((keyValueInformation->Type == REG_DWORD) &&
            (keyValueInformation->DataLength >= sizeof(ULONG))) {

            count = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
        }
        ExFreePool(keyValueInformation);
    }
    if (count == 0) {
        ZwClose(serviceEnumHandle);
        if (closeHandle) {
            ZwClose(ServiceHandle);
        }
        return status;
    }

    //
    // Open HTREE\ROOT\0 key so later we can remove device instance key
    // from its AttachedComponents value name.
    //

    status = IopOpenRegistryKey(&sysEnumHandle,
                                NULL,
                                &CmRegistryMachineSystemCurrentControlSetEnumName,
                                KEY_ALL_ACCESS,
                                FALSE
                                );

    //
    // Walk through each registered device instance to mark its Problem and
    // StatusFlags as fail to start and reset its ActiveService
    //

    KeEnterCriticalRegion();
    ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

    newCount = count;
    for (i = 0; i < count; i++) {
        deletePdo = FALSE;
        status = IopServiceInstanceToDeviceInstance (
                     ServiceHandle,
                     ServiceName,
                     i,
                     &deviceInstanceName,
                     &handle,
                     KEY_ALL_ACCESS
                     );

        if (NT_SUCCESS(status)) {

            PDEVICE_OBJECT deviceObject;
            PDEVICE_NODE deviceNode;

            //
            // If the device instance is a detected device reported during driver's
            // DriverEntry we need to clean it up.
            //

            deviceObject = IopDeviceObjectFromDeviceInstance(handle, NULL);
            if (deviceObject) {
                deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;
                if (deviceNode) {

                    IopReleaseDeviceResources(deviceNode);

                    if ((deviceNode->Flags & DNF_MADEUP) && (deviceNode->Flags & DNF_STARTED)) {

                        //
                        // Now mark this one deleted.
                        //
                        deviceNode->Flags &= ~DNF_STARTED;

                        IopSetDevNodeProblem(deviceNode, CM_PROB_DEVICE_NOT_THERE);
                        deletePdo = TRUE;
                    }
                }
                ObDereferenceObject(deviceObject);  // added via IopDeviceObjectFromDeviceInstance
            }

            PiWstrToUnicodeString(&unicodeValueName, REGSTR_KEY_CONTROL);
            controlHandle = NULL;
            status = IopOpenRegistryKey(&controlHandle,
                                        handle,
                                        &unicodeValueName,
                                        KEY_ALL_ACCESS,
                                        FALSE
                                        );
            if (NT_SUCCESS(status)) {

                status = IopGetRegistryValue(controlHandle,
                                             REGSTR_VALUE_NEWLY_CREATED,
                                             &keyValueInformation);
                if (NT_SUCCESS(status)) {
                    ExFreePool(keyValueInformation);
                }
                if ((status != STATUS_OBJECT_NAME_NOT_FOUND) &&
                    (status != STATUS_OBJECT_PATH_NOT_FOUND)) {

                    //
                    // Remove the instance value name from service enum key
                    //

                    PiUlongToUnicodeString(&unicodeValueName, unicodeBuffer, 20, i);
                    status = ZwDeleteValueKey (serviceEnumHandle, &unicodeValueName);
                    if (NT_SUCCESS(status)) {

                        //
                        // If we can successfaully remove the instance value entry
                        // from service enum key, we then remove the device instance key
                        // Otherwise, we go thru normal path to mark driver loading failed
                        // in the device instance key.
                        //

                        newCount--;

                        ZwDeleteKey(controlHandle);
                        ZwDeleteKey(handle);


                        //
                        // We also want to delete the ROOT\LEGACY_<driver> key
                        //

                        if (sysEnumHandle) {
                            deviceInstanceName.Length -= 5 * sizeof(WCHAR);
                            deviceInstanceName.Buffer[deviceInstanceName.Length / sizeof(WCHAR)] =
                                                 UNICODE_NULL;
                            status = IopOpenRegistryKey(&handle,
                                                        sysEnumHandle,
                                                        &deviceInstanceName,
                                                        KEY_ALL_ACCESS,
                                                        FALSE
                                                        );
                            if (NT_SUCCESS(status)) {
                                ZwDeleteKey(handle);
                            }
                        }

                        ExFreePool(deviceInstanceName.Buffer);

                        //
                        // If there is a PDO for this device, remove it
                        //

                        if (deletePdo) {
                            IoDeleteDevice(deviceObject);
                        }

                        continue;
                    }
                }
            }

            //
            // Reset Control\ActiveService value name.
            //

            if (controlHandle) {
                PiWstrToUnicodeString(&unicodeValueName, REGSTR_VAL_ACTIVESERVICE);
                ZwDeleteValueKey(controlHandle, &unicodeValueName);
                ZwClose(controlHandle);
            }

            ZwClose(handle);
            ExFreePool(deviceInstanceName.Buffer);
        }
    }

    //
    // If some instance value entry is deleted, we need to update the count of instance
    // value entries and rearrange the instance value entries under service enum key.
    //

    if (newCount != count) {
        if (newCount != 0) {
            j = 0;
            i = 0;
            while (i < count) {
                PiUlongToUnicodeString(&unicodeValueName, unicodeBuffer, 20, i);
                status = IopGetRegistryValue(serviceEnumHandle,
                                             unicodeValueName.Buffer,
                                             &keyValueInformation
                                             );
                if (NT_SUCCESS(status)) {
                    if (i != j) {

                        //
                        // Need to change the instance i to instance j
                        //

                        ZwDeleteValueKey(serviceEnumHandle, &unicodeValueName);

                        PiUlongToUnicodeString(&unicodeValueName, unicodeBuffer, 20, j);
                        ZwSetValueKey (serviceEnumHandle,
                                       &unicodeValueName,
                                       TITLE_INDEX_VALUE,
                                       REG_SZ,
                                       (PVOID)KEY_VALUE_DATA(keyValueInformation),
                                       keyValueInformation->DataLength
                                       );
                    }
                    ExFreePool(keyValueInformation);
                    j++;
                }
                i++;
            }
        }

        //
        // Don't forget to update the "Count=" and "NextInstance=" value entries
        //

        PiWstrToUnicodeString( &unicodeValueName, REGSTR_VALUE_COUNT);

        ZwSetValueKey(serviceEnumHandle,
                      &unicodeValueName,
                      TITLE_INDEX_VALUE,
                      REG_DWORD,
                      &newCount,
                      sizeof (newCount)
                      );
        PiWstrToUnicodeString( &unicodeValueName, REGSTR_VALUE_NEXT_INSTANCE);

        ZwSetValueKey(serviceEnumHandle,
                      &unicodeValueName,
                      TITLE_INDEX_VALUE,
                      REG_DWORD,
                      &newCount,
                      sizeof (newCount)
                      );
    }
    ZwClose(serviceEnumHandle);
    if (closeHandle) {
        ZwClose(ServiceHandle);
    }
    if (sysEnumHandle) {
        ZwClose(sysEnumHandle);
    }

    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    return STATUS_SUCCESS;
}

VOID
IopDisableDevice(
    IN PDEVICE_NODE DeviceNode,
    IN HANDLE Handle
    )

/*++

Routine Description:

    This routine tries to ask a bus driver stopping decoding resources

Arguments:

    DeviceNode - Specifies the device to be disabled.

    Handle - specifies the device instance handle.

Returns:

    None.

--*/

{
    UNICODE_STRING unicodeName;
    NTSTATUS status;
    PCM_RESOURCE_LIST cmResource;
    ULONG cmLength, tmpValue;
    HANDLE logConfHandle;

    //
    // If the device has boot config, we will query-remove and remove the device to free
    // the boot config if possible.
    //

    if (DeviceNode->Flags & DNF_HAS_BOOT_CONFIG) {
        status = IopRemoveDevice (DeviceNode->PhysicalDeviceObject, IRP_MN_QUERY_REMOVE_DEVICE);
        if (NT_SUCCESS(status)) {
            status = IopRemoveDevice (DeviceNode->PhysicalDeviceObject, IRP_MN_REMOVE_DEVICE);
            ASSERT(NT_SUCCESS(status));
            IopReleaseDeviceResources(DeviceNode);

            //
            // If device node still has boot config, the IopReleaseDeviceResource has
            // reserved the boot config for the device again and the boot resources are not
            // changed.
            // Else send query-resource again to make sure the bus driver did disable the
            // device and free the resources.
            //

            if (DeviceNode->BootResources == NULL) {
                PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
                status = IopOpenRegistryKeyPersist(&logConfHandle,
                                                   Handle,
                                                   &unicodeName,
                                                   KEY_ALL_ACCESS,
                                                   TRUE,
                                                   &tmpValue
                                                   );
                if (!NT_SUCCESS(status)) {
                    logConfHandle = NULL;
                }
                status = IopQueryDeviceResources (
                              DeviceNode->PhysicalDeviceObject,
                              QUERY_RESOURCE_LIST,
                              &cmResource,
                              &cmLength
                              );

                if (!NT_SUCCESS(status)) {
                    cmResource = NULL;
                    cmLength = 0;
                }

                if (logConfHandle) {
                    KeEnterCriticalRegion();
                    ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);
                    if (cmResource) {
                        PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_BOOTCONFIG);
                        ZwSetValueKey(
                                  logConfHandle,
                                  &unicodeName,
                                  TITLE_INDEX_VALUE,
                                  REG_RESOURCE_LIST,
                                  cmResource,
                                  cmLength
                                  );
                    } else {
                        ZwDeleteValueKey(logConfHandle, &unicodeName);
                    }
                    ExReleaseResource(&PpRegistryDeviceResource);
                    KeLeaveCriticalRegion();
                    ZwClose(logConfHandle);
                }
                if (cmResource) {
                    DeviceNode->Flags |= DNF_HAS_BOOT_CONFIG;
                    DeviceNode->BootResources = cmResource;

                    //
                    // This device consumes BOOT resources.  Reserve its boot resources
                    //

                    (*IopReserveResourcesRoutine)(ArbiterRequestPnpEnumerated,
                                                  DeviceNode->PhysicalDeviceObject,
                                                  cmResource);
                }
            }
        } else {
            IopRemoveDevice (DeviceNode->PhysicalDeviceObject, IRP_MN_CANCEL_REMOVE_DEVICE);
        }
    }

    if (IopDoesDevNodeHaveProblem(DeviceNode)) {
        ASSERT(IopIsDevNodeProblem(DeviceNode, CM_PROB_NOT_CONFIGURED) ||
               IopIsDevNodeProblem(DeviceNode, CM_PROB_REINSTALL));

        IopClearDevNodeProblem(DeviceNode);
    }

    IopSetDevNodeProblem(DeviceNode, CM_PROB_DISABLED);
}

BOOLEAN
IopIsAnyDeviceInstanceEnabled(
    IN PUNICODE_STRING ServiceKeyName,
    IN HANDLE ServiceHandle OPTIONAL,
    IN BOOLEAN LegacyIncluded
    )

/*++

Routine Description:

    This routine checks if any of the devices instances is turned on for the specified
    service. This routine is used for Pnp Driver only and is temporary function to support
    SUR.

Arguments:

    ServiceKeyName - Specifies the service key unicode name

    ServiceHandle - Optionally supplies a handle to the service key to be checked.

    LegacyIncluded - TRUE, a legacy device instance key is counted as a device instance.
                     FALSE, a legacy device instance key is not counted.

Returns:

    A BOOLEAN value.

--*/

{
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    HANDLE serviceEnumHandle, handle, controlHandle;
    ULONG i, count, deviceFlags;
    UNICODE_STRING unicodeName;
    BOOLEAN enabled, setProblem, closeHandle = FALSE;
    PDEVICE_OBJECT physicalDeviceObject;
    PDEVICE_NODE deviceNode;

    //
    // Open registry ServiceKeyName\Enum branch
    //

    if (!ARGUMENT_PRESENT(ServiceHandle)) {
        status = IopOpenServiceEnumKeys(ServiceKeyName,
                                        KEY_READ,
                                        &ServiceHandle,
                                        &serviceEnumHandle,
                                        FALSE
                                        );
        closeHandle = TRUE;
    } else {
        PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_ENUM);
        status = IopOpenRegistryKey(&serviceEnumHandle,
                                    ServiceHandle,
                                    &unicodeName,
                                    KEY_READ,
                                    FALSE
                                    );
    }
    if (!NT_SUCCESS( status )) {

        //
        // No Service Enum key? no device instance.  Return FALSE.
        //

        return FALSE;
    }

    //
    // Find out how many device instances listed in the ServiceName's
    // Enum key.
    //

    status = IopGetRegistryValue ( serviceEnumHandle,
                                   REGSTR_VALUE_COUNT,
                                   &keyValueInformation
                                   );
    ZwClose(serviceEnumHandle);
    count = 0;
    if (NT_SUCCESS(status)) {
        if ((keyValueInformation->Type == REG_DWORD) &&
            (keyValueInformation->DataLength >= sizeof(ULONG))) {

            count = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
        }
        ExFreePool(keyValueInformation);
    }
    if (count == 0) {
        if (closeHandle) {
            ZwClose(ServiceHandle);
        }
        return FALSE;
    }

    //
    // Walk through each registered device instance to check it is enabled.
    //

    enabled = FALSE;
    for (i = 0; i < count; i++) {

        //
        // Get device instance handle.  If it fails, we will skip this device
        // instance.
        //

        status = IopServiceInstanceToDeviceInstance (
                     ServiceHandle,
                     NULL,
                     i,
                     NULL,
                     &handle,
                     KEY_ALL_ACCESS
                     );
        if (!NT_SUCCESS(status)) {
            continue;
        }

        physicalDeviceObject = IopDeviceObjectFromDeviceInstance(handle, NULL);
        if (physicalDeviceObject) {
            deviceNode = (PDEVICE_NODE)physicalDeviceObject->DeviceObjectExtension->DeviceNode;
            if (deviceNode && IopIsDevNodeProblem(deviceNode, CM_PROB_DISABLED)) {
                ZwClose(handle);
                ObDereferenceObject(physicalDeviceObject);
                continue;
            }
        }

        //
        // Check if the device instance has been disabled.
        // First check global flag: CONFIGFLAG and then CSCONFIGFLAG.
        //

        deviceFlags = 0;
        status = IopGetRegistryValue(handle,
                                     REGSTR_VALUE_CONFIG_FLAGS,
                                     &keyValueInformation);
        if (NT_SUCCESS(status)) {
            if ((keyValueInformation->Type == REG_DWORD) &&
                (keyValueInformation->DataLength >= sizeof(ULONG))) {

                deviceFlags = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
            }
            ExFreePool(keyValueInformation);
        }

        if (deviceFlags & CONFIGFLAG_DISABLED) {

            //
            // Convert this flag into the hardware profile-specific version, so it'll
            // look the same as the CsConfigFlags we retrieve below.
            //

            deviceFlags = CSCONFIGFLAG_DISABLED;

        } else {

            status = IopGetDeviceInstanceCsConfigFlags(
                         ServiceKeyName,
                         i,
                         &deviceFlags
                         );

            if (!NT_SUCCESS(status)) {
                deviceFlags = 0;
            }
        }

        //
        // If the device is disabled (either globally, or specifically for this
        // hardware profile), then mark the devnode as DNF_DISABLED.
        //

        if ((deviceFlags & CSCONFIGFLAG_DISABLED) || (deviceFlags & CSCONFIGFLAG_DO_NOT_START)) {

            if (deviceNode) {
                IopDisableDevice(deviceNode, handle);
            }
        }

        if (physicalDeviceObject) {
            ObDereferenceObject(physicalDeviceObject);
        }

        //
        // Finally, we need to set the STATUSFLAGS of the device instance to
        // indicate if the driver is successfully started.
        //

        if (!(deviceFlags & (CSCONFIGFLAG_DISABLED | CSCONFIGFLAG_DO_NOT_CREATE | CSCONFIGFLAG_DO_NOT_START))) {

            ULONG legacy;

            //
            // Check should legacy instance key be counted as an enabled device
            //

            if (LegacyIncluded == FALSE) {

                //
                // The legacy variable must be initialized to zero.  Because the device
                // instance key may be an enumerated device.  In this case, there is no
                // legacy value name.
                //

                legacy = 0;
                status = IopGetRegistryValue(handle,
                                             REGSTR_VALUE_LEGACY,
                                             &keyValueInformation
                                             );
                if (NT_SUCCESS(status)) {
                    if ((keyValueInformation->Type == REG_DWORD) &&
                        (keyValueInformation->DataLength >= sizeof(ULONG))) {
                        legacy = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
                    }
                    ExFreePool(keyValueInformation);
                }
            } else {
                legacy = 0;
            }

            if (legacy == 0) {

                //
                // Mark that the driver has at least a device instance to work with.
                //

                PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
                status = IopOpenRegistryKey(&controlHandle,
                                            handle,
                                            &unicodeName,
                                            KEY_ALL_ACCESS,
                                            TRUE
                                            );
                if (NT_SUCCESS(status)) {
                    PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_ACTIVESERVICE);
                    ZwSetValueKey(
                                controlHandle,
                                &unicodeName,
                                TITLE_INDEX_VALUE,
                                REG_SZ,
                                ServiceKeyName->Buffer,
                                ServiceKeyName->Length + sizeof(UNICODE_NULL)
                                );

                    ZwClose(controlHandle);
                }
                enabled = TRUE;
            }
        }
        ZwClose(handle);
    }

    if (closeHandle) {
        ZwClose(ServiceHandle);
    }
    return enabled;
}

BOOLEAN
IopIsDeviceInstanceEnabled(
    IN HANDLE DeviceInstanceHandle,
    IN PUNICODE_STRING DeviceInstance,
    IN BOOLEAN DisableIfEnabled
    )

/*++

Routine Description:

    This routine checks if the specified devices instances is enabled.

Arguments:

    DeviceInstanceHandle - Optionally supplies a handle to the device instance
        key to be checked.

    DeviceInstance - Specifies the device instance key unicode name.  Caller
        must at least specified DeviceInstanceHandle or DeviceInstance.

Returns:

    A BOOLEAN value.

--*/

{
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    HANDLE handle, handle1;
    ULONG deviceFlags;
    BOOLEAN enabled, closeHandle = FALSE;
    UNICODE_STRING unicodeString;
    PDEVICE_OBJECT deviceObject = NULL;
    PDEVICE_NODE deviceNode = NULL;;

    //
    // Open registry ServiceKeyName\Enum branch
    //

    if (!ARGUMENT_PRESENT(DeviceInstanceHandle)) {
        status = IopOpenRegistryKey(&handle,
                                    NULL,
                                    &CmRegistryMachineSystemCurrentControlSetEnumName,
                                    KEY_READ,
                                    FALSE
                                    );

        if (NT_SUCCESS( status )) {

            status = IopOpenRegistryKey (
                                    DeviceInstanceHandle,
                                    handle,
                                    DeviceInstance,
                                    KEY_READ,
                                    FALSE
                                    );
            ZwClose(handle);
        }

        if (!NT_SUCCESS( status )) {
            return FALSE;
        }
        closeHandle = TRUE;
    }

    enabled = TRUE;

    //
    // First check the device node
    //

    deviceObject = IopDeviceObjectFromDeviceInstance(DeviceInstanceHandle, NULL);
    if (deviceObject) {
        deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;
        if (deviceNode && IopIsDevNodeProblem(deviceNode, CM_PROB_DISABLED)) {
            enabled = FALSE;
            goto exit;
        }
    }

    //
    // Check if the device instance has been disabled.
    // First check global flag: CONFIGFLAG and then CSCONFIGFLAG.
    //

    deviceFlags = 0;
    status = IopGetRegistryValue(DeviceInstanceHandle,
                                 REGSTR_VALUE_CONFIG_FLAGS,
                                 &keyValueInformation);
    if (NT_SUCCESS(status)) {
        if ((keyValueInformation->Type == REG_DWORD) &&
            (keyValueInformation->DataLength >= sizeof(ULONG))) {
            deviceFlags = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
        }
        ExFreePool(keyValueInformation);
    }
    if (!(deviceFlags & CONFIGFLAG_DISABLED)) {
        enabled = TRUE;

        //
        // See if we can open current hardware profile
        //
        status = IopOpenRegistryKey(&handle1,
                                    NULL,
                                    &CmRegistryMachineSystemCurrentControlSetHardwareProfilesCurrent,
                                    KEY_READ,
                                    FALSE
                                    );
        if (NT_SUCCESS(status) && DeviceInstance != NULL) {

            //
            // Now, we must open the System\CCS\Enum key under this.
            //
            //
            // Open system\CurrentControlSet under current hardware profile key
            //

            PiWstrToUnicodeString(&unicodeString, REGSTR_PATH_CURRENTCONTROLSET);
            status = IopOpenRegistryKey(&handle,
                                        handle1,
                                        &unicodeString,
                                        KEY_READ,
                                        FALSE
                                        );
            ZwClose(handle1);
            if (NT_SUCCESS(status)) {
                PiWstrToUnicodeString(&unicodeString, REGSTR_KEY_ENUM);
                status = IopOpenRegistryKey(&handle1,
                                            handle,
                                            &unicodeString,
                                            KEY_READ,
                                            FALSE
                                            );
                ZwClose(handle);
                if (NT_SUCCESS(status)) {
                    status = IopOpenRegistryKey(&handle,
                                                handle1,
                                                DeviceInstance,
                                                KEY_READ,
                                                FALSE
                                                );
                    ZwClose(handle1);
                    if (NT_SUCCESS(status)) {
                        status = IopGetRegistryValue(
                                        handle,
                                        REGSTR_VALUE_CSCONFIG_FLAGS,
                                        &keyValueInformation
                                        );
                        if (NT_SUCCESS(status)) {
                            if((keyValueInformation->Type == REG_DWORD) &&
                               (keyValueInformation->DataLength >= sizeof(ULONG))) {
                               deviceFlags = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
                            }
                            ExFreePool(keyValueInformation);
                        }
                        ZwClose(handle);
                        if (NT_SUCCESS(status)) {
                            if ((deviceFlags & CSCONFIGFLAG_DISABLED) ||
                                (deviceFlags & CSCONFIGFLAG_DO_NOT_CREATE) ||
                                (deviceFlags & CSCONFIGFLAG_DO_NOT_START)) {
                                enabled = FALSE;
                            }
                        }
                    }
                }
            }
        }
    } else {
        enabled = FALSE;
    }

    //
    // If the device is disabled and has device node associated with it.
    // disable the device.
    //

    if (enabled == FALSE && deviceNode && DisableIfEnabled) {
        IopDisableDevice(deviceNode, DeviceInstanceHandle);
    }
exit:
    if (deviceObject) {
        ObDereferenceObject(deviceObject);
    }
    if (closeHandle) {
        ZwClose(DeviceInstanceHandle);
    }
    return enabled;
}

ULONG
IopDetermineResourceListSize(
    IN PCM_RESOURCE_LIST ResourceList
    )

/*++

Routine Description:

    This routine determines size of the passed in ResourceList
    structure.

Arguments:

    Configuration1 - Supplies a pointer to the resource list.

Return Value:

    size of the resource list structure.

--*/

{
    ULONG totalSize, listSize, descriptorSize, i, j;
    PCM_FULL_RESOURCE_DESCRIPTOR fullResourceDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptor;

    if (!ResourceList) {
        totalSize = 0;
    } else {
        totalSize = FIELD_OFFSET(CM_RESOURCE_LIST, List);
        fullResourceDesc = &ResourceList->List[0];
        for (i = 0; i < ResourceList->Count; i++) {
            listSize = FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR,
                                    PartialResourceList) +
                       FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST,
                                    PartialDescriptors);
            partialDescriptor = &fullResourceDesc->PartialResourceList.PartialDescriptors[0];
            for (j = 0; j < fullResourceDesc->PartialResourceList.Count; j++) {
                descriptorSize = sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
                if (partialDescriptor->Type == CmResourceTypeDeviceSpecific) {
                    descriptorSize += partialDescriptor->u.DeviceSpecificData.DataSize;
                }
                listSize += descriptorSize;
                partialDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
                                        ((PUCHAR)partialDescriptor + descriptorSize);
            }
            totalSize += listSize;
            fullResourceDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)
                                      ((PUCHAR)fullResourceDesc + listSize);
        }
    }
    return totalSize;
}

PDRIVER_OBJECT
IopReferenceDriverObjectByName (
    IN PUNICODE_STRING DriverName
    )

/*++

Routine Description:

    This routine references a driver object by a given driver name.

Arguments:

    DriverName - supplies a pointer to the name of the driver whose driver object is
        to be referenced.

Returns:

    A pointer to a DRIVER_OBJECT if succeeds.  Otherwise, a NULL value.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE driverHandle;
    NTSTATUS status;
    PDRIVER_OBJECT driverObject;

    //
    // Make sure the driver name is valid.
    //

    if (DriverName->Length == 0) {
        return NULL;
    }

    InitializeObjectAttributes(&objectAttributes,
                               DriverName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );
    status = ObOpenObjectByName(&objectAttributes,
                                IoDriverObjectType,
                                KernelMode,
                                NULL,
                                FILE_READ_ATTRIBUTES,
                                NULL,
                                &driverHandle
                                );
    if (NT_SUCCESS(status)) {

        //
        // Now reference the driver object.
        //

        status = ObReferenceObjectByHandle(driverHandle,
                                           0,
                                           IoDriverObjectType,
                                           KernelMode,
                                           &driverObject,
                                           NULL
                                           );
        NtClose(driverHandle);
    }

    if (NT_SUCCESS(status)) {
        return driverObject;
    } else {
        return NULL;
    }
}

PDEVICE_OBJECT
IopDeviceObjectFromDeviceInstance (
    IN HANDLE DeviceInstanceHandle      OPTIONAL,
    IN PUNICODE_STRING DeviceInstance   OPTIONAL
    )

/*++

Routine Description:

    This routine receives a DeviceInstance path (or DeviceInstance handle) and
    returns a reference to a bus device object for the DeviceInstance.

Arguments:

    DeviceInstanceHandle - Supplies a handle to the registry Device Instance key.
        If this parameter is NULL, DeviceInstance must specify the registry path.

    DeviceInstance - supplies a UNICODE_STRING to specify the device instance path.
        if this parameter is NULL, DeviceInstanceHandle must specify the handle
        to its registry key.

Returns:

    A reference to the desired bus device object.

--*/

{
    NTSTATUS status;
    HANDLE handle, deviceHandle = NULL;
    PDEVICE_OBJECT deviceReference = NULL;
    UNICODE_STRING unicodeName;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    if (!ARGUMENT_PRESENT(DeviceInstanceHandle)) {

        //
        // first open System\CCS\Enum key and then the device instance key.
        //

        status = IopOpenRegistryKey(&handle,
                                    NULL,
                                    &CmRegistryMachineSystemCurrentControlSetEnumName,
                                    KEY_READ,
                                    FALSE
                                    );

        if (!NT_SUCCESS( status )) {
            return deviceReference;
        }

        ASSERT(DeviceInstance);
        status = IopOpenRegistryKey (&deviceHandle,
                                     handle,
                                     DeviceInstance,
                                     KEY_READ,
                                     FALSE
                                     );
        ZwClose(handle);
        if (!NT_SUCCESS(status)) {
            return deviceReference;
        }
        DeviceInstanceHandle = deviceHandle;
    }

    //
    // Read the address of device object
    //

    PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
    status = IopOpenRegistryKey(&handle,
                                DeviceInstanceHandle,
                                &unicodeName,
                                KEY_READ,
                                FALSE
                                );
    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    status = IopGetRegistryValue (handle,
                                  REGSTR_VALUE_DEVICE_REFERENCE,
                                  &keyValueInformation);
    ZwClose(handle);
    if (NT_SUCCESS(status)) {
        if ((keyValueInformation->Type == REG_DWORD) &&
            (keyValueInformation->DataLength == sizeof(ULONG_PTR))) {

            deviceReference = *(PDEVICE_OBJECT *)KEY_VALUE_DATA(keyValueInformation);
        }
        ExFreePool(keyValueInformation);
    }
    if (!deviceReference) {
        goto exit;
    }

    //
    // Make sure the address of the device object is not clobbered,
    // unfortunately if the address is truly bogus we will bugcheck since
    // try/except doesn't work for kernel addresses.
    //

    if (deviceReference->Type != IO_TYPE_DEVICE) {
        deviceReference = NULL;
    } else {
        deviceNode = (PDEVICE_NODE)deviceReference->DeviceObjectExtension->DeviceNode;
        if (deviceNode && (deviceNode->PhysicalDeviceObject == deviceReference)) {
            ObReferenceObject(deviceReference);
        } else {
            deviceReference = NULL;
        }
    }

exit:
    if (deviceHandle) {
        ZwClose(deviceHandle);
    }
    return deviceReference;

}

NTSTATUS
IopDeviceObjectToDeviceInstance (
    IN PDEVICE_OBJECT DeviceObject,
    IN PHANDLE DeviceInstanceHandle,
    IN  ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This routine receives a DeviceObject pointer and returns a handle to the device
    instance path under registry System\ENUM key.

    Note, caller must owner the PpRegistryDeviceResource before calling the function,

Arguments:

    DeviceObject - supplies a pointer to a physical device object.

    DeviceInstanceHandle - Supplies a variable to receive the handle to the registry
             device instance key.

    DesiredAccess - specifies the access that is needed to this key.

Returns:

    NTSTATUS code to indicate success or failure.

--*/

{
    NTSTATUS status;
    HANDLE handle;
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    status = IopOpenRegistryKey(&handle,
                                NULL,
                                &CmRegistryMachineSystemCurrentControlSetEnumName,
                                KEY_READ,
                                FALSE
                                );

    if (!NT_SUCCESS( status )) {
        return status;
    }

    deviceNode = (PDEVICE_NODE) DeviceObject->DeviceObjectExtension->DeviceNode;
    if (deviceNode && (deviceNode->InstancePath.Length != 0)) {
        status = IopOpenRegistryKey (DeviceInstanceHandle,
                                     handle,
                                     &deviceNode->InstancePath,
                                     DesiredAccess,
                                     FALSE
                                     );
    } else {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    ZwClose(handle);

    return status;
}

NTSTATUS
IopCleanupDeviceRegistryValues (
    IN PUNICODE_STRING InstancePath,
    IN BOOLEAN KeepReference
    )

/*++

Routine Description:

    This routine cleans up a device instance key when the device is no
    longer present/enumerated.  If the device is registered to a Service
    the Service's enum key will also been cleaned up.

    Note the caller must lock the RegistryDeciceResource

Arguments:

    InstancePath - supplies a pointer to the name of the device instance key.

    KeepReference - supplies a boolean value to indicate if we should remove the
        device instance path to device object mapping.

Return Value:

    status

--*/

{
    HANDLE handle, instanceHandle;
    UNICODE_STRING unicodeValueName;
    NTSTATUS status;
    ULONG count = 0;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;

    PAGED_CODE();

    //
    // Open System\CurrentControlSet\Enum
    //

    status = IopOpenRegistryKey(&handle,
                                NULL,
                                &CmRegistryMachineSystemCurrentControlSetEnumName,
                                KEY_ALL_ACCESS,
                                FALSE
                                );

    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // Open the device instance key
    //

    status = IopOpenRegistryKey(&instanceHandle,
                                handle,
                                InstancePath,
                                KEY_ALL_ACCESS,
                                FALSE
                                );

    ZwClose(handle);
    if (!NT_SUCCESS( status )) {

        //
        // There is no registry key for the ServiceKeyName information.
        //

        return status;
    }

    //
    // Delete the DeviceReference value name under Control key
    //

    if (!KeepReference) {

        //
        // BUGBUG (lonnym)--verify that removal of the following code fragment is valid.
        //
#if 0
        //
        // Set the 'FoundAtEnum' value to false.
        //

        PiWstrToUnicodeString(&unicodeValueName, REGSTR_VALUE_FOUNDATENUM);
        tmpValue = 0;
        ZwSetValueKey(instanceHandle,
                      &unicodeValueName,
                      TITLE_INDEX_VALUE,
                      REG_DWORD,
                      &tmpValue,
                      sizeof(tmpValue)
                      );
#endif // (lonnym, verify removal to here)

        PiWstrToUnicodeString(&unicodeValueName, REGSTR_KEY_CONTROL);
        status = IopOpenRegistryKey(&handle,
                                    instanceHandle,
                                    &unicodeValueName,
                                    KEY_ALL_ACCESS,
                                    FALSE
                                    );
        if (NT_SUCCESS( status )) {
            PiWstrToUnicodeString(&unicodeValueName, REGSTR_VALUE_DEVICE_REFERENCE);
            ZwDeleteValueKey(handle, &unicodeValueName);
            ZwClose(handle);
        }

        //
        // Deregister the device from its controlling service's service enum key
        //

        status = PiDeviceRegistration(InstancePath,
                                      FALSE,
                                      NULL
                                      );
    }
    ZwClose(instanceHandle);

    return status;
}

NTSTATUS
IopGetDeviceResourcesFromRegistry (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ResourceType,
    IN ULONG Preference,
    OUT PVOID *Resource,
    OUT PULONG Length
    )

/*++

Routine Description:

    This routine determines the resources decoded by the device specified.
    If the device object is a madeup device, we will try to read the resources
    from registry.  Otherwise, we need to traverse the internal assigned resource
    list to compose the resource list.

Arguments:

    DeviceObject - supplies a pointer to a device object whose registry
        values are to be cleaned up.

    ResourceType - 0 for CM_RESOURCE_LIST and 1 for IO_RESOURCE_REQUIREMENTS_LIS

    Flags - specify the preference.

    Resource - Specified a variable to receive the required resources.

    Length - Specified a variable to receive the length of the resource structure.

Return Value:

    status

--*/

{
    PDEVICE_NODE deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    HANDLE handle, handlex;
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PCM_RESOURCE_LIST cmResource;
    PIO_RESOURCE_REQUIREMENTS_LIST ioResource;
    ULONG length;
    UNICODE_STRING unicodeName;
    PWCHAR valueName = NULL;

    *Resource = NULL;
    *Length = 0;

    //
    // Open the LogConfig key of the device instance.
    //

    status = IopDeviceObjectToDeviceInstance(DeviceObject, &handlex, KEY_READ);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (ResourceType == QUERY_RESOURCE_LIST) {

        //
        // Caller is asking for CM_RESOURCE_LIST
        //

        if (Preference & REGISTRY_ALLOC_CONFIG) {

            //
            // Try alloc config first
            //

            PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
            status = IopOpenRegistryKey(&handle,
                                        handlex,
                                        &unicodeName,
                                        KEY_READ,
                                        FALSE
                                        );
            if (NT_SUCCESS(status)) {
                status = IopReadDeviceConfiguration (handle, REGISTRY_ALLOC_CONFIG, (PCM_RESOURCE_LIST *)Resource, Length);
                ZwClose(handle);
                if (NT_SUCCESS(status)) {
                    ZwClose(handlex);
                    return status;
                }
            }
        }

        handle = NULL;
        if (Preference & REGISTRY_FORCED_CONFIG) {

            PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
            status = IopOpenRegistryKey(&handle,
                                        handlex,
                                        &unicodeName,
                                        KEY_READ,
                                        FALSE
                                        );
            if (NT_SUCCESS(status)) {
                status = IopReadDeviceConfiguration (handle, REGISTRY_FORCED_CONFIG, (PCM_RESOURCE_LIST *)Resource, Length);
                if (NT_SUCCESS(status)) {
                    ZwClose(handle);
                    ZwClose(handlex);
                    return status;
                }
            } else {
                ZwClose(handlex);
                return status;
            }
        }
        if (Preference & REGISTRY_BOOT_CONFIG) {

            //
            // Try alloc config first
            //

            if (handle == NULL) {
                PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
                status = IopOpenRegistryKey(&handle,
                                            handlex,
                                            &unicodeName,
                                            KEY_READ,
                                            FALSE
                                            );
                if (!NT_SUCCESS(status)) {
                    ZwClose(handlex);
                    return status;
                }
            }
            status = IopReadDeviceConfiguration (handle, REGISTRY_BOOT_CONFIG, (PCM_RESOURCE_LIST *)Resource, Length);
        }
        if (handle) {
            ZwClose(handle);
        }
    } else {

        PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
        status = IopOpenRegistryKey(&handle,
                                    handlex,
                                    &unicodeName,
                                    KEY_READ,
                                    FALSE
                                    );
        if (NT_SUCCESS(status)) {

            if (Preference & REGISTRY_OVERRIDE_CONFIGVECTOR) {
                valueName = REGSTR_VALUE_OVERRIDE_CONFIG_VECTOR;
            } else if (Preference & REGISTRY_BASIC_CONFIGVECTOR) {
                valueName = REGSTR_VALUE_BASIC_CONFIG_VECTOR;
            }
            if (valueName) {

                //
                // Try to read device's configuration vector
                //

                status = IopGetRegistryValue (handle,
                                              valueName,
                                              &keyValueInformation);
                if (NT_SUCCESS(status)) {

                    //
                    // Try to read what caller wants.
                    //

                    if ((keyValueInformation->Type == REG_RESOURCE_REQUIREMENTS_LIST) &&
                        (keyValueInformation->DataLength != 0)) {

                        *Resource = ExAllocatePool(PagedPool,
                                                   keyValueInformation->DataLength);
                        if (*Resource) {
                            PIO_RESOURCE_REQUIREMENTS_LIST ioResource;

                            *Length = keyValueInformation->DataLength;
                            RtlMoveMemory(*Resource,
                                          KEY_VALUE_DATA(keyValueInformation),
                                          keyValueInformation->DataLength);

                            //
                            // Process the io resource requirements list to change undefined
                            // interface type to our default type.
                            //

                            ioResource = *Resource;
                            if (ioResource->InterfaceType == InterfaceTypeUndefined) {
                                ioResource->BusNumber = 0;
                                ioResource->InterfaceType = PnpDefaultInterfaceType;
                            }
                        } else {
                            status = STATUS_INVALID_PARAMETER_2;
                        }
                    }
                    ExFreePool(keyValueInformation);
                }
            }
            ZwClose(handle);
        }
    }
    ZwClose(handlex);
    return status;
}

NTSTATUS
IopReadDeviceConfiguration (
    IN HANDLE Handle,
    IN ULONG Flags,
    OUT PCM_RESOURCE_LIST *CmResource,
    OUT PULONG Length
    )

/*++

Routine Description:

    This routine read the specified ALLOC config or ForcedConfig or Boot config.

Arguments:

    Hanle - supplies a handle to the registry key to read resources.

Return Value:

    status

--*/

{
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PWCHAR valueName;

    *CmResource = NULL;
    *Length = 0;

    if (Flags == REGISTRY_ALLOC_CONFIG) {
        valueName = REGSTR_VALUE_ALLOC_CONFIG;
    } else if (Flags == REGISTRY_FORCED_CONFIG) {
        valueName = REGSTR_VALUE_FORCED_CONFIG;
    } else if (Flags == REGISTRY_BOOT_CONFIG) {
        valueName = REGSTR_VALUE_BOOT_CONFIG;
    } else {
        return STATUS_INVALID_PARAMETER_2;
    }

    //
    // Read the registry value of the desired value name
    //

    status = IopGetRegistryValue (Handle,
                                  valueName,
                                  &keyValueInformation);
    if (NT_SUCCESS(status)) {

        //
        // Try to read what caller wants.
        //

        if ((keyValueInformation->Type == REG_RESOURCE_LIST) &&
            (keyValueInformation->DataLength != 0)) {
            *CmResource = ExAllocatePool(PagedPool,
                                         keyValueInformation->DataLength);
            if (*CmResource) {
                if (*CmResource) {
                    *Length = keyValueInformation->DataLength;
                    RtlMoveMemory(*CmResource,
                                  KEY_VALUE_DATA(keyValueInformation),
                                  keyValueInformation->DataLength);
                } else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            ExFreePool(keyValueInformation);
            if (*CmResource) {
                PCM_RESOURCE_LIST resourceList;
                PCM_FULL_RESOURCE_DESCRIPTOR cmFullDesc;
                PCM_PARTIAL_RESOURCE_DESCRIPTOR cmPartDesc;
                ULONG j, k, size, count;

                //
                // Process the resource list read from Registry to change undefined
                // interface type to our default interface type.
                //

                resourceList = *CmResource;
                cmFullDesc = &resourceList->List[0];
                for (j = 0; j < resourceList->Count; j++) {
                    if (cmFullDesc->InterfaceType == InterfaceTypeUndefined) {
                        cmFullDesc->BusNumber = 0;
                        cmFullDesc->InterfaceType = PnpDefaultInterfaceType;
                    }
                    cmPartDesc = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
                    for (k = 0; k < cmFullDesc->PartialResourceList.Count; k++) {
                        size = 0;
                        switch (cmPartDesc->Type) {
                        case CmResourceTypeDeviceSpecific:
                             size = cmPartDesc->u.DeviceSpecificData.DataSize;
                             break;
                        }
                        cmPartDesc++;
                        cmPartDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmPartDesc + size);
                    }
                    cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmPartDesc;
                }
            }
        } else if (keyValueInformation->Type != REG_RESOURCE_LIST) {
            status = STATUS_UNSUCCESSFUL;
        }
    }
    return status;
}

PIO_RESOURCE_REQUIREMENTS_LIST
IopCmResourcesToIoResources (
    IN ULONG SlotNumber,
    IN PCM_RESOURCE_LIST CmResourceList,
    IN ULONG Priority
    )

/*++

Routine Description:

    This routines converts the input CmResourceList to IO_RESOURCE_REQUIREMENTS_LIST.

Arguments:

    SlotNumber - supplies the SlotNumber the resources refer to.

    CmResourceList - the cm resource list to convert.

    Priority - specifies the priority of the logconfig

Return Value:

    returns a IO_RESOURCE_REQUIREMENTS_LISTST if succeeds.  Otherwise a NULL value is
    returned.

--*/
{
    PIO_RESOURCE_REQUIREMENTS_LIST ioResReqList;
    ULONG count = 0, size, i, j;
    PCM_FULL_RESOURCE_DESCRIPTOR cmFullDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmPartDesc;
    PIO_RESOURCE_DESCRIPTOR ioDesc;

    //
    // First determine number of descriptors required.
    //

    cmFullDesc = &CmResourceList->List[0];
    for (i = 0; i < CmResourceList->Count; i++) {
        count += cmFullDesc->PartialResourceList.Count;
        cmPartDesc = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
        for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {
            size = 0;
            switch (cmPartDesc->Type) {
            case CmResourceTypeDeviceSpecific:
                 size = cmPartDesc->u.DeviceSpecificData.DataSize;
                 count--;
                 break;
            }
            cmPartDesc++;
            cmPartDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmPartDesc + size);
        }
        cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmPartDesc;
    }

    if (count == 0) {
        return NULL;
    }

    //
    // Count the extra descriptors for InterfaceType and BusNumber information.
    //

    count += CmResourceList->Count - 1;

    //
    // Allocate heap space for IO RESOURCE REQUIREMENTS LIST
    //

    count++;           // add one for CmResourceTypeConfigData
    ioResReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)ExAllocatePool(
                       PagedPool,
                       sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
                           count * sizeof(IO_RESOURCE_DESCRIPTOR)
                       );
    if (!ioResReqList) {
        return NULL;
    }

    //
    // Parse the cm resource descriptor and build its corresponding IO resource descriptor
    //

    ioResReqList->InterfaceType = CmResourceList->List[0].InterfaceType;
    ioResReqList->BusNumber = CmResourceList->List[0].BusNumber;
    ioResReqList->SlotNumber = SlotNumber;
    ioResReqList->Reserved[0] = 0;
    ioResReqList->Reserved[1] = 0;
    ioResReqList->Reserved[2] = 0;
    ioResReqList->AlternativeLists = 1;
    ioResReqList->List[0].Version = 1;
    ioResReqList->List[0].Revision = 1;
    ioResReqList->List[0].Count = count;

    //
    // Generate a CmResourceTypeConfigData descriptor
    //

    ioDesc = &ioResReqList->List[0].Descriptors[0];
    ioDesc->Option = IO_RESOURCE_PREFERRED;
    ioDesc->Type = CmResourceTypeConfigData;
    ioDesc->ShareDisposition = CmResourceShareShared;
    ioDesc->Flags = 0;
    ioDesc->Spare1 = 0;
    ioDesc->Spare2 = 0;
    ioDesc->u.ConfigData.Priority = Priority;
    ioDesc++;

    cmFullDesc = &CmResourceList->List[0];
    for (i = 0; i < CmResourceList->Count; i++) {
        if (i != 0) {

            //
            // Set up descriptor to remember the InterfaceType and BusNumber.
            //

            ioDesc->Option = IO_RESOURCE_PREFERRED;
            ioDesc->Type = CmResourceTypeReserved;
            ioDesc->ShareDisposition = CmResourceShareUndetermined;
            ioDesc->Flags = 0;
            ioDesc->Spare1 = 0;
            ioDesc->Spare2 = 0;
            if (cmFullDesc->InterfaceType == InterfaceTypeUndefined) {
                ioDesc->u.DevicePrivate.Data[0] = PnpDefaultInterfaceType;
            } else {
                ioDesc->u.DevicePrivate.Data[0] = cmFullDesc->InterfaceType;
            }
            ioDesc->u.DevicePrivate.Data[1] = cmFullDesc->BusNumber;
            ioDesc->u.DevicePrivate.Data[2] = 0;
            ioDesc++;
        }
        cmPartDesc = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
        for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {
            ioDesc->Option = IO_RESOURCE_PREFERRED;
            ioDesc->Type = cmPartDesc->Type;
            ioDesc->ShareDisposition = cmPartDesc->ShareDisposition;
            ioDesc->Flags = cmPartDesc->Flags;
            ioDesc->Spare1 = 0;
            ioDesc->Spare2 = 0;

            size = 0;
            switch (cmPartDesc->Type) {
            case CmResourceTypePort:
                 ioDesc->u.Port.MinimumAddress = cmPartDesc->u.Port.Start;
                 ioDesc->u.Port.MaximumAddress.QuadPart = cmPartDesc->u.Port.Start.QuadPart +
                                                             cmPartDesc->u.Port.Length - 1;
                 ioDesc->u.Port.Alignment = 1;
                 ioDesc->u.Port.Length = cmPartDesc->u.Port.Length;
                 ioDesc++;
                 break;
            case CmResourceTypeInterrupt:
#if defined(_X86_)
                ioDesc->u.Interrupt.MinimumVector = ioDesc->u.Interrupt.MaximumVector =
                   cmPartDesc->u.Interrupt.Level;
#else
                 ioDesc->u.Interrupt.MinimumVector = ioDesc->u.Interrupt.MaximumVector =
                    cmPartDesc->u.Interrupt.Vector;
#endif
                 ioDesc++;
                 break;
            case CmResourceTypeMemory:
                 ioDesc->u.Memory.MinimumAddress = cmPartDesc->u.Memory.Start;
                 ioDesc->u.Memory.MaximumAddress.QuadPart = cmPartDesc->u.Memory.Start.QuadPart +
                                                               cmPartDesc->u.Memory.Length - 1;
                 ioDesc->u.Memory.Alignment = 1;
                 ioDesc->u.Memory.Length = cmPartDesc->u.Memory.Length;
                 ioDesc++;
                 break;
            case CmResourceTypeDma:
                 ioDesc->u.Dma.MinimumChannel = cmPartDesc->u.Dma.Channel;
                 ioDesc->u.Dma.MaximumChannel = cmPartDesc->u.Dma.Channel;
                 ioDesc++;
                 break;
            case CmResourceTypeDeviceSpecific:
                 size = cmPartDesc->u.DeviceSpecificData.DataSize;
                 break;
            case CmResourceTypeBusNumber:
                 ioDesc->u.BusNumber.MinBusNumber = cmPartDesc->u.BusNumber.Start;
                 ioDesc->u.BusNumber.MaxBusNumber = cmPartDesc->u.BusNumber.Start +
                                                    cmPartDesc->u.BusNumber.Length - 1;
                 ioDesc->u.BusNumber.Length = cmPartDesc->u.BusNumber.Length;
                 ioDesc++;
                 break;
            default:
                 ioDesc->u.DevicePrivate.Data[0] = cmPartDesc->u.DevicePrivate.Data[0];
                 ioDesc->u.DevicePrivate.Data[1] = cmPartDesc->u.DevicePrivate.Data[1];
                 ioDesc->u.DevicePrivate.Data[2] = cmPartDesc->u.DevicePrivate.Data[2];
                 ioDesc++;
                 break;
            }
            cmPartDesc++;
            cmPartDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmPartDesc + size);
        }
        cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmPartDesc;
    }
    ioResReqList->ListSize = (ULONG)((ULONG_PTR)ioDesc - (ULONG_PTR)ioResReqList);
    return ioResReqList;
}

NTSTATUS
IopFilterResourceRequirementsList (
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoList,
    IN PCM_RESOURCE_LIST CmList,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *FilteredList,
    OUT PBOOLEAN ExactMatch
    )

/*++

Routine Description:

    This routines adjusts the input IoList based on input BootConfig.


Arguments:

    IoList - supplies the pointer to an IoResourceRequirementsList

    CmList - supplies the pointer to a BootConfig.

    FilteredList - Supplies a variable to receive the filtered resource
             requirements list.

Return Value:

    A NTSTATUS code to indicate the result of the function.

--*/
{
    NTSTATUS status;
    PIO_RESOURCE_REQUIREMENTS_LIST ioList, newList;
    PIO_RESOURCE_LIST ioResourceList, newIoResourceList, selectedResourceList = NULL;
    PIO_RESOURCE_DESCRIPTOR ioResourceDescriptor, ioResourceDescriptorEnd;
    PIO_RESOURCE_DESCRIPTOR newIoResourceDescriptor, configDataDescriptor;
    LONG ioResourceDescriptorCount = 0;
    USHORT version;
    PCM_FULL_RESOURCE_DESCRIPTOR cmFullDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDescriptor;
    ULONG cmDescriptorCount = 0;
    ULONG size, i, j, oldCount, phase;
    LONG k, alternativeLists;
    BOOLEAN exactMatch;

    PAGED_CODE();

    *FilteredList = NULL;
    *ExactMatch = FALSE;

    //
    // Make sure there is some resource requirements to be filtered.
    // If no, we will convert CmList/BootConfig to an IoResourceRequirementsList
    //

    if (IoList == NULL || IoList->AlternativeLists == 0) {
        if (CmList && CmList->Count != 0) {
            *FilteredList = IopCmResourcesToIoResources (0, CmList, LCPRI_BOOTCONFIG);
        }
        return STATUS_SUCCESS;
    }

    //
    // Make a copy of the Io Resource Requirements List
    //

    ioList = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool(PagedPool, IoList->ListSize);
    if (ioList == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlMoveMemory(ioList, IoList, IoList->ListSize);

    //
    // If there is no BootConfig, simply return the copy of the input Io list.
    //

    if (CmList == NULL || CmList->Count == 0) {
        *FilteredList = ioList;
        return STATUS_SUCCESS;
    }

    //
    // First determine minimum number of descriptors required.
    //

    cmFullDesc = &CmList->List[0];
    for (i = 0; i < CmList->Count; i++) {
        cmDescriptorCount += cmFullDesc->PartialResourceList.Count;
        cmDescriptor = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
        for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {
            size = 0;
            switch (cmDescriptor->Type) {
            case CmResourceTypeConfigData:
            case CmResourceTypeDevicePrivate:
                 cmDescriptorCount--;
                 break;
            case CmResourceTypeDeviceSpecific:
                 size = cmDescriptor->u.DeviceSpecificData.DataSize;
                 cmDescriptorCount--;
                 break;
            default:

                 //
                 // Invalid cmresource list.  Ignore it and use io resources
                 //

                 if (cmDescriptor->Type == CmResourceTypeNull ||
                     cmDescriptor->Type >= CmResourceTypeMaximum) {
                     cmDescriptorCount--;
                 }
            }
            cmDescriptor++;
            cmDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmDescriptor + size);
        }
        cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmDescriptor;
    }

    if (cmDescriptorCount == 0) {
        *FilteredList = ioList;
        return STATUS_SUCCESS;
    }

    //
    // cmDescriptorCount is the number of BootConfig Descriptors needs.
    //
    // For each IO list Alternative ...
    //

    ioResourceList = ioList->List;
    k = ioList->AlternativeLists;
    while (--k >= 0) {
        ioResourceDescriptor = ioResourceList->Descriptors;
        ioResourceDescriptorEnd = ioResourceDescriptor + ioResourceList->Count;
        while (ioResourceDescriptor < ioResourceDescriptorEnd) {
            ioResourceDescriptor->Spare1 = 0;
            ioResourceDescriptor++;
        }
        ioResourceList = (PIO_RESOURCE_LIST) ioResourceDescriptorEnd;
    }

    ioResourceList = ioList->List;
    k = alternativeLists = ioList->AlternativeLists;
    while (--k >= 0) {
        version = ioResourceList->Version;
        if (version == 0xffff) {  // Convert bogus version to valid number
            version = 1;
        }

        //
        // We use Version field to store number of BootConfig found.
        // Count field to store new number of descriptor in the alternative list.
        //

        ioResourceList->Version = 0;
        oldCount = ioResourceList->Count;

        ioResourceDescriptor = ioResourceList->Descriptors;
        ioResourceDescriptorEnd = ioResourceDescriptor + ioResourceList->Count;

        if (ioResourceDescriptor == ioResourceDescriptorEnd) {

            //
            // An alternative list with zero descriptor count
            //

            ioResourceList->Version = 0xffff;  // Mark it as invalid
            ioList->AlternativeLists--;
            continue;
        }

        exactMatch = TRUE;

        //
        // For each Cm Resource descriptor ... except DevicePrivate and
        // DeviceSpecific...
        //

        cmFullDesc = &CmList->List[0];
        for (i = 0; i < CmList->Count; i++) {
            cmDescriptor = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
            for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {
                size = 0;
                switch (cmDescriptor->Type) {
                case CmResourceTypeDevicePrivate:
                     break;
                case CmResourceTypeDeviceSpecific:
                     size = cmDescriptor->u.DeviceSpecificData.DataSize;
                     break;
                default:
                    if (cmDescriptor->Type == CmResourceTypeNull ||
                        cmDescriptor->Type >= CmResourceTypeMaximum) {
                        break;
                    }

                    //
                    // Check CmDescriptor against current Io Alternative list
                    //

                    for (phase = 0; phase < 2; phase++) {
                        ioResourceDescriptor = ioResourceList->Descriptors;
                        while (ioResourceDescriptor < ioResourceDescriptorEnd) {
                            if ((ioResourceDescriptor->Type == cmDescriptor->Type) &&
                                (ioResourceDescriptor->Spare1 == 0)) {
                                ULONGLONG min1, max1, min2, max2;
                                ULONG len1 = 1, len2 = 1, align1, align2;
                                UCHAR share1, share2;

                                share2 = ioResourceDescriptor->ShareDisposition;
                                share1 = cmDescriptor->ShareDisposition;
                                if ((share1 == CmResourceShareUndetermined) ||
                                    (share1 > CmResourceShareShared)) {
                                    share1 = share2;
                                }
                                if ((share2 == CmResourceShareUndetermined) ||
                                    (share2 > CmResourceShareShared)) {
                                    share2 = share1;
                                }
                                align1 = align2 = 1;

                                switch (cmDescriptor->Type) {
                                case CmResourceTypePort:
                                case CmResourceTypeMemory:
                                    min1 = cmDescriptor->u.Port.Start.QuadPart;
                                    max1 = cmDescriptor->u.Port.Start.QuadPart + cmDescriptor->u.Port.Length - 1;
                                    len1 = cmDescriptor->u.Port.Length;
                                    min2 = ioResourceDescriptor->u.Port.MinimumAddress.QuadPart;
                                    max2 = ioResourceDescriptor->u.Port.MaximumAddress.QuadPart;
                                    len2 = ioResourceDescriptor->u.Port.Length;
                                    align2 = ioResourceDescriptor->u.Port.Alignment;
                                    break;
                                case CmResourceTypeInterrupt:
                                    max1 = min1 = cmDescriptor->u.Interrupt.Vector;
                                    min2 = ioResourceDescriptor->u.Interrupt.MinimumVector;
                                    max2 = ioResourceDescriptor->u.Interrupt.MaximumVector;
                                    break;
                                case CmResourceTypeDma:
                                    min1 = max1 =cmDescriptor->u.Dma.Channel;
                                    min2 = ioResourceDescriptor->u.Dma.MinimumChannel;
                                    max2 = ioResourceDescriptor->u.Dma.MaximumChannel;
                                    break;
                                case CmResourceTypeBusNumber:
                                    min1 = cmDescriptor->u.BusNumber.Start;
                                    max1 = cmDescriptor->u.BusNumber.Start + cmDescriptor->u.BusNumber.Length - 1;
                                    len1 = cmDescriptor->u.BusNumber.Length;
                                    min2 = ioResourceDescriptor->u.BusNumber.MinBusNumber;
                                    max2 = ioResourceDescriptor->u.BusNumber.MaxBusNumber;
                                    len2 = ioResourceDescriptor->u.BusNumber.Length;
                                    break;
                                default:
                                    ASSERT(0);
                                    break;
                                }
                                if (phase == 0) {
                                    if (share1 == share2 && min2 == min1 && max2 >= max1 && len2 >= len1) {

                                        //
                                        // For phase 0 match, we want near exact match...
                                        //

                                        if (max2 != max1) {
                                            exactMatch = FALSE;
                                        }
                                        ioResourceList->Version++;
                                        ioResourceDescriptor->Spare1 = 0x80;
                                        if (ioResourceDescriptor->Option & IO_RESOURCE_ALTERNATIVE) {
                                            PIO_RESOURCE_DESCRIPTOR ioDesc;

                                            ioDesc = ioResourceDescriptor;
                                            ioDesc--;
                                            while (ioDesc >= ioResourceList->Descriptors) {
                                                ioDesc->Type = CmResourceTypeNull;
                                                ioResourceList->Count--;
                                                if (ioDesc->Option == IO_RESOURCE_ALTERNATIVE) {
                                                    ioDesc--;
                                                } else {
                                                    break;
                                                }
                                            }
                                        }
                                        ioResourceDescriptor->Option = IO_RESOURCE_PREFERRED;
                                        ioResourceDescriptor->Flags = cmDescriptor->Flags;
                                        if (ioResourceDescriptor->Type == CmResourceTypePort ||
                                            ioResourceDescriptor->Type == CmResourceTypeMemory) {
                                            ioResourceDescriptor->u.Port.MinimumAddress.QuadPart = min1;
                                            ioResourceDescriptor->u.Port.MaximumAddress.QuadPart = min1 + len2 - 1;
                                            ioResourceDescriptor->u.Port.Alignment = 1;
                                        } else if (ioResourceDescriptor->Type == CmResourceTypeBusNumber) {
                                            ioResourceDescriptor->u.BusNumber.MinBusNumber = (ULONG)min1;
                                            ioResourceDescriptor->u.BusNumber.MaxBusNumber = (ULONG)(min1 + len2 - 1);
                                        }
                                        ioResourceDescriptor++;
                                        while (ioResourceDescriptor < ioResourceDescriptorEnd) {
                                            if (ioResourceDescriptor->Option & IO_RESOURCE_ALTERNATIVE) {
                                                ioResourceDescriptor->Type = CmResourceTypeNull;
                                                ioResourceDescriptor++;
                                                ioResourceList->Count--;
                                            } else {
                                                break;
                                            }
                                        }
                                        phase = 1;   // skip phase 1
                                        break;
                                    } else {
                                        ioResourceDescriptor++;
                                    }
                                } else {
                                    exactMatch = FALSE;
                                    if (share1 == share2 && min2 <= min1 && max2 >= max1 && len2 >= len1 &&
                                        (min1 & (align2 - 1)) == 0) {

                                        //
                                        // Io range covers Cm range ... Change the Io range to what is specified
                                        // in BootConfig.
                                        //
                                        //

                                        switch (cmDescriptor->Type) {
                                        case CmResourceTypePort:
                                        case CmResourceTypeMemory:
                                            ioResourceDescriptor->u.Port.MinimumAddress.QuadPart = min1;
                                            ioResourceDescriptor->u.Port.MaximumAddress.QuadPart = min1 + len2 - 1;
                                            break;
                                        case CmResourceTypeInterrupt:
                                        case CmResourceTypeDma:
                                            ioResourceDescriptor->u.Interrupt.MinimumVector = (ULONG)min1;
                                            ioResourceDescriptor->u.Interrupt.MaximumVector = (ULONG)max1;
                                            break;
                                        case CmResourceTypeBusNumber:
                                            ioResourceDescriptor->u.BusNumber.MinBusNumber = (ULONG)min1;
                                            ioResourceDescriptor->u.BusNumber.MaxBusNumber = (ULONG)(min1 + len2 - 1);
                                            break;
                                        }
                                        ioResourceList->Version++;
                                        ioResourceDescriptor->Spare1 = 0x80;
                                        ioResourceDescriptor->Flags = cmDescriptor->Flags;
                                        if (ioResourceDescriptor->Option & IO_RESOURCE_ALTERNATIVE) {
                                            PIO_RESOURCE_DESCRIPTOR ioDesc;

                                            ioDesc = ioResourceDescriptor;
                                            ioDesc--;
                                            while (ioDesc >= ioResourceList->Descriptors) {
                                                ioDesc->Type = CmResourceTypeNull;
                                                ioResourceList->Count--;
                                                if (ioDesc->Option == IO_RESOURCE_ALTERNATIVE) {
                                                    ioDesc--;
                                                } else {
                                                    break;
                                                }
                                            }
                                        }
                                        ioResourceDescriptor->Option = IO_RESOURCE_PREFERRED;
                                        ioResourceDescriptor++;
                                        while (ioResourceDescriptor < ioResourceDescriptorEnd) {
                                            if (ioResourceDescriptor->Option & IO_RESOURCE_ALTERNATIVE) {
                                                ioResourceDescriptor->Type = CmResourceTypeNull;
                                                ioResourceList->Count--;
                                                ioResourceDescriptor++;
                                            } else {
                                                break;
                                            }
                                        }
                                        break;
                                    } else {
                                        ioResourceDescriptor++;
                                    }
                                }
                            } else {
                                ioResourceDescriptor++;
                            }
                        } // Don't add any instruction after this ...
                    } // phase
                } // switch

                //
                // Move to next Cm Descriptor
                //

                cmDescriptor++;
                cmDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmDescriptor + size);
            }

            //
            // Move to next Cm List
            //

            cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmDescriptor;
        }

        if (ioResourceList->Version != (USHORT)cmDescriptorCount) {

            //
            // If the current alternative list does not cover all the boot config
            // descriptors, make it as invalid.
            //

            ioResourceList->Version = 0xffff;
            ioList->AlternativeLists--;
        } else {
            if ((ioResourceList->Count == cmDescriptorCount) ||
                (ioResourceList->Count == (cmDescriptorCount + 1) &&
                 ioResourceList->Descriptors[0].Type == CmResourceTypeConfigData)) {
                if (selectedResourceList) {
                    ioResourceList->Version = 0xffff;
                    ioList->AlternativeLists--;
                } else {
                    selectedResourceList = ioResourceList;
                    ioResourceDescriptorCount += ioResourceList->Count;
                    ioResourceList->Version = version;
                    if (exactMatch) {
                        *ExactMatch = TRUE;
                    }
                }
            } else {
                ioResourceDescriptorCount += ioResourceList->Count;
                ioResourceList->Version = version;
            }
        }
        ioResourceList->Count = oldCount;

        //
        // Move to next Io alternative list.
        //

        ioResourceList = (PIO_RESOURCE_LIST) ioResourceDescriptorEnd;
    }

    //
    // If there is not any valid alternative, convert CmList to Io list.
    //

    if (ioList->AlternativeLists == 0) {
         *FilteredList = IopCmResourcesToIoResources (0, CmList, LCPRI_BOOTCONFIG);
        ExFreePool(ioList);
        return STATUS_SUCCESS;
    }

    //
    // we have finished filtering the resource requirements list.  Now allocate memory
    // and rebuild a new list.
    //

    size = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
               sizeof(IO_RESOURCE_LIST) * (ioList->AlternativeLists - 1) +
               sizeof(IO_RESOURCE_DESCRIPTOR) * (ioResourceDescriptorCount);
    newList = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool(PagedPool, size);
    if (newList == NULL) {
        ExFreePool(ioList);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Walk through the io resource requirements list and pick up any valid descriptor.
    //

    newList->ListSize = size;
    newList->InterfaceType = CmList->List->InterfaceType;
    newList->BusNumber = CmList->List->BusNumber;
    newList->SlotNumber = ioList->SlotNumber;
    if (ioList->AlternativeLists > 1) {
        *ExactMatch = FALSE;
    }
    newList->AlternativeLists = ioList->AlternativeLists;
    ioResourceList = ioList->List;
    newIoResourceList = newList->List;
    while (--alternativeLists >= 0) {
        ioResourceDescriptor = ioResourceList->Descriptors;
        ioResourceDescriptorEnd = ioResourceDescriptor + ioResourceList->Count;
        if (ioResourceList->Version == 0xffff) {
            ioResourceList = (PIO_RESOURCE_LIST)ioResourceDescriptorEnd;
            continue;
        }
        newIoResourceList->Version = ioResourceList->Version;
        newIoResourceList->Revision = ioResourceList->Revision;

        newIoResourceDescriptor = newIoResourceList->Descriptors;
        if (ioResourceDescriptor->Type != CmResourceTypeConfigData) {
            newIoResourceDescriptor->Option = IO_RESOURCE_PREFERRED;
            newIoResourceDescriptor->Type = CmResourceTypeConfigData;
            newIoResourceDescriptor->ShareDisposition = CmResourceShareShared;
            newIoResourceDescriptor->Flags = 0;
            newIoResourceDescriptor->Spare1 = 0;
            newIoResourceDescriptor->Spare2 = 0;
            newIoResourceDescriptor->u.ConfigData.Priority = LCPRI_BOOTCONFIG;
            configDataDescriptor = newIoResourceDescriptor;
            newIoResourceDescriptor++;
        } else {
            newList->ListSize -= sizeof(IO_RESOURCE_DESCRIPTOR);
            configDataDescriptor = newIoResourceDescriptor;
        }

        while (ioResourceDescriptor < ioResourceDescriptorEnd) {
            if (ioResourceDescriptor->Type != CmResourceTypeNull) {
                *newIoResourceDescriptor = *ioResourceDescriptor;
                newIoResourceDescriptor++;
            }
            ioResourceDescriptor++;
        }
        newIoResourceList->Count = (ULONG)(newIoResourceDescriptor - newIoResourceList->Descriptors);

        //if (newIoResourceList->Count == (cmDescriptorCount + 1)) {
        configDataDescriptor->u.ConfigData.Priority =  LCPRI_BOOTCONFIG;
        //}

        //
        // Move to next Io alternative list.
        //

        newIoResourceList = (PIO_RESOURCE_LIST) newIoResourceDescriptor;
        ioResourceList = (PIO_RESOURCE_LIST) ioResourceDescriptorEnd;
    }
    ASSERT((PUCHAR)newIoResourceList == ((PUCHAR)newList + newList->ListSize));

    *FilteredList = newList;
    ExFreePool(ioList);
    return STATUS_SUCCESS;
}

NTSTATUS
IopMergeFilteredResourceRequirementsList (
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoList1,
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoList2,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *MergedList
    )

/*++

Routine Description:

    This routines merges two IoLists into one.


Arguments:

    IoList1 - supplies the pointer to the first IoResourceRequirementsList

    IoList2 - supplies the pointer to the second IoResourceRequirementsList

    MergedList - Supplies a variable to receive the merged resource
             requirements list.

Return Value:

    A NTSTATUS code to indicate the result of the function.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_RESOURCE_REQUIREMENTS_LIST ioList, newList;
    ULONG size;
    PUCHAR p;

    PAGED_CODE();

    *MergedList = NULL;

    //
    // First handle the easy cases that both IO Lists are empty or any one of
    // them is empty.
    //

    if ((IoList1 == NULL || IoList1->AlternativeLists == 0) &&
        (IoList2 == NULL || IoList2->AlternativeLists == 0)) {
        return status;
    }
    ioList = NULL;
    if (IoList1 == NULL || IoList1->AlternativeLists == 0) {
        ioList = IoList2;
    } else if (IoList2 == NULL || IoList2->AlternativeLists == 0) {
        ioList = IoList1;
    }
    if (ioList) {
        newList = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool(PagedPool, ioList->ListSize);
        if (newList == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlMoveMemory(newList, ioList, ioList->ListSize);
        *MergedList = newList;
        return status;
    }

    //
    // Do real work...
    //

    size = IoList1->ListSize + IoList2->ListSize - FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List);
    newList = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool(
                          PagedPool,
                          size
                          );
    if (newList == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    p = (PUCHAR)newList;
    RtlMoveMemory(p, IoList1, IoList1->ListSize);
    p += IoList1->ListSize;
    RtlMoveMemory(p,
                  &IoList2->List[0],
                  size - IoList1->ListSize
                  );
    newList->ListSize = size;
    newList->AlternativeLists += IoList2->AlternativeLists;
    *MergedList = newList;
    return status;

}

NTSTATUS
IopMergeCmResourceLists (
    IN PCM_RESOURCE_LIST List1,
    IN PCM_RESOURCE_LIST List2,
    IN OUT PCM_RESOURCE_LIST *MergedList
    )

/*++

Routine Description:

    This routines merges two IoLists into one.


Arguments:

    IoList1 - supplies the pointer to the first CmResourceList

    IoList2 - supplies the pointer to the second CmResourceList

    MergedList - Supplies a variable to receive the merged resource
             list.

Return Value:

    A NTSTATUS code to indicate the result of the function.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PCM_RESOURCE_LIST cmList, newList;
    ULONG size, size1, size2;
    PUCHAR p;

    PAGED_CODE();

    *MergedList = NULL;

    //
    // First handle the easy cases that both IO Lists are empty or any one of
    // them is empty.
    //

    if ((List1 == NULL || List1->Count == 0) &&
        (List2 == NULL || List2->Count == 0)) {
        return status;
    }

    cmList = NULL;
    if (List1 == NULL || List1->Count == 0) {
        cmList = List2;
    } else if (List2 == NULL || List2->Count == 0) {
        cmList = List1;
    }
    if (cmList) {
        size =  IopDetermineResourceListSize(cmList);
        newList = (PCM_RESOURCE_LIST) ExAllocatePool(PagedPool, size);
        if (newList == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlMoveMemory(newList, cmList, size);
        *MergedList = newList;
        return status;
    }

    //
    // Do real work...
    //

    size1 =  IopDetermineResourceListSize(List1);
    size2 =  IopDetermineResourceListSize(List2);
    size = size1 + size2;
    newList = (PCM_RESOURCE_LIST) ExAllocatePool(
                          PagedPool,
                          size
                          );
    if (newList == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    p = (PUCHAR)newList;
    RtlMoveMemory(p, List1, size1);
    p += size1;
    RtlMoveMemory(p,
                  &List2->List[0],
                  size2 - FIELD_OFFSET(CM_RESOURCE_LIST, List)
                  );
    newList->Count = List1->Count + List2->Count;
    *MergedList = newList;
    return status;

}

BOOLEAN
IopIsLegacyDriver (
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine checks if the driver object specifies a legacy driver.

Arguments:

    DriverObject - supplies a pointer to the driver object to be checked.

Return Value:

    BOOLEAN

--*/

{

    PAGED_CODE();

    //
    // If AddDevice entry is not empty it is a wdm driver
    //

    if (DriverObject->DriverExtension->AddDevice) {
        return FALSE;
    }

    //
    // Else if LEGACY flag is set in the driver object, it's a legacy driver.
    //

    if (DriverObject->Flags & DRVO_LEGACY_DRIVER) {
        return TRUE;
    } else {
        return FALSE;
    }
}

USHORT
IopGetGroupOrderIndex (
    IN HANDLE ServiceHandle
    )

/*++

Routine Description:

    This routine reads the Group value of the service key, finds its position in the
    ServiceOrderList.  If ServiceHandle is NULL or unrecognized group value, it returns
    a value with max group order + 1.
    if any.

Parameters:

    ServiceHandle - supplies a handle to the service key.

Return Value:

    group order index.

--*/

{
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    UNICODE_STRING *groupTable, group;
    HANDLE handle;
    ULONG count, index;

    PAGED_CODE();

    //
    // Open System\CurrentControlSet\Control\ServiceOrderList
    //

    PiWstrToUnicodeString(&group, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ServiceGroupOrder");
    status = IopOpenRegistryKey(&handle,
                                NULL,
                                &group,
                                KEY_READ,
                                FALSE
                                );

    if (!NT_SUCCESS( status )) {
        return NO_MORE_GROUP;
    }

    //
    // Read and build a unicode string array containing all the group names.
    //

    status = IopGetRegistryValue (handle,
                                  L"List",
                                  &keyValueInformation);
    ZwClose(handle);
    if (NT_SUCCESS(status)) {

        if ((keyValueInformation->Type == REG_MULTI_SZ) &&
            (keyValueInformation->DataLength != 0)) {
             status = IopRegMultiSzToUnicodeStrings(keyValueInformation, &groupTable, &count);
             if (!NT_SUCCESS(status)) {
                 ExFreePool(keyValueInformation);
                 return NO_MORE_GROUP;
             }
        }
        ExFreePool(keyValueInformation);
    }

    if (ServiceHandle == NULL) {
        IopFreeUnicodeStringList(groupTable, count);
        return (USHORT)(count + 1);
    }


    //
    // Read service key's Group value
    //

    status = IopGetRegistryValue (ServiceHandle,
                                  L"Group",
                                  &keyValueInformation);
    if (NT_SUCCESS(status)) {

        //
        // Try to read what caller wants.
        //

        if ((keyValueInformation->Type == REG_SZ) &&
            (keyValueInformation->DataLength != 0)) {
            IopRegistryDataToUnicodeString(&group,
                                           (PWSTR)KEY_VALUE_DATA(keyValueInformation),
                                           keyValueInformation->DataLength
                                           );
        }
    } else {

        //
        // If we failed to read the Group value, or no Group value...
        //

        IopFreeUnicodeStringList(groupTable, count);
        return (USHORT)count;
    }

    index = 0;
    for (index = 0; index < count; index++) {
        if (RtlEqualUnicodeString(&group, &groupTable[index], TRUE)) {
            break;
        }
    }
    ExFreePool(keyValueInformation);
    IopFreeUnicodeStringList(groupTable, count);
    return (USHORT)index;
}

VOID
IopDeleteLegacyKey(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine checks if the Legacy= value of the driver's legacy_xxx key
    is one.  If yes, it deletes the Legacy key.

Parameters:

    DriverObject - supplies a pointer to the driver object.

Return Value:

    None.  If anything fails in this routine, the legacy key stays.

--*/

{
    WCHAR buffer[100];
    NTSTATUS status;
    UNICODE_STRING deviceName, instanceName, unicodeName, *serviceName;
    ULONG length;
    HANDLE handle, handle1, handlex, enumHandle;
    ULONG legacy;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_NODE deviceNode;

    serviceName = &DriverObject->DriverExtension->ServiceKeyName;

    KeEnterCriticalRegion();
    ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

    status = IopOpenRegistryKey(&enumHandle,
                                NULL,
                                &CmRegistryMachineSystemCurrentControlSetEnumName,
                                KEY_ALL_ACCESS,
                                FALSE
                                );

    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    length = _snwprintf(buffer, sizeof(buffer) / sizeof(WCHAR), L"ROOT\\LEGACY_%s", serviceName->Buffer);
    deviceName.MaximumLength = sizeof(buffer);
    ASSERT(length <= sizeof(buffer) - 10);
    deviceName.Length = (USHORT)(length * sizeof(WCHAR));
    deviceName.Buffer = buffer;

    status = IopOpenRegistryKey(&handle1,
                                enumHandle,
                                &deviceName,
                                KEY_ALL_ACCESS,
                                FALSE
                                );

    if (NT_SUCCESS(status)) {

        deviceName.Buffer[deviceName.Length / sizeof(WCHAR)] =
                   OBJ_NAME_PATH_SEPARATOR;
        deviceName.Length += sizeof(WCHAR);
        PiUlongToInstanceKeyUnicodeString(
                                &instanceName,
                                buffer + deviceName.Length / sizeof(WCHAR),
                                sizeof(buffer) - deviceName.Length,
                                0
                                );
        deviceName.Length += instanceName.Length;


        status = IopOpenRegistryKey(
                                &handle,
                                handle1,
                                &instanceName,
                                KEY_ALL_ACCESS,
                                FALSE
                                );
        if (NT_SUCCESS(status)) {
            legacy = 1;
            status = IopGetRegistryValue (handle,
                                          REGSTR_VALUE_LEGACY,
                                          &keyValueInformation);
            if (NT_SUCCESS(status)) {
                if ((keyValueInformation->Type == REG_DWORD) &&
                    (keyValueInformation->DataLength >= sizeof(ULONG))) {
                    legacy = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
                }
                ExFreePool(keyValueInformation);
            }
            if (legacy != 0) {

                //
                // We also want to delete the madeup device node
                //

                deviceObject = IopDeviceObjectFromDeviceInstance(handle, NULL);
                if (deviceObject) {

                    PDEVICE_NODE devNodex, devNodey;

                    deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;
                    if (deviceNode) {
                        if (deviceNode->Flags & DNF_MADEUP) {

                            //
                            // Now mark this one deleted.
                            //
                            deviceNode->Flags &= ~DNF_STARTED;
                            IopSetDevNodeProblem(deviceNode, CM_PROB_DEVICE_NOT_THERE);

                            //
                            // This is actually doing nothing because DeviceNode->ResourceList is NULL.
                            //

                            IopReleaseDeviceResources(deviceNode);
                            devNodex = deviceNode;
                            while (devNodex) {
                                devNodey = devNodex;
                                devNodex = (PDEVICE_NODE)devNodey->OverUsed2.NextResourceDeviceNode;
                                devNodey->OverUsed2.NextResourceDeviceNode = NULL;
                                devNodey->OverUsed1.LegacyDeviceNode = NULL;
                            }

                            deviceNode->Flags &= ~DNF_MADEUP;  // remove its boot config if any
                            IoDeleteDevice(deviceObject);
                        }
                    }
                    ObDereferenceObject(deviceObject);  // added via IopDeviceObjectFromDeviceInstance
                }

                PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
                status = IopOpenRegistryKey(&handlex,
                                            handle,
                                            &unicodeName,
                                            KEY_ALL_ACCESS,
                                            FALSE
                                            );
                if (NT_SUCCESS(status)) {
                    ZwDeleteKey(handlex);
                }
                PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
                status = IopOpenRegistryKey(&handlex,
                                            handle,
                                            &unicodeName,
                                            KEY_ALL_ACCESS,
                                            FALSE
                                            );
                if (NT_SUCCESS(status)) {
                    ZwDeleteKey(handlex);
                }

                ZwClose(enumHandle);

                //
                // We need to call IopCleanupDeviceRegistryValue even we are going to
                // delete it.  Because, it also cleans up related value names in other
                // keys.
                //

                IopCleanupDeviceRegistryValues(&deviceName, FALSE);
                ZwDeleteKey(handle);
                ZwDeleteKey(handle1);
            } else {
                ZwClose(handle);
                ZwClose(handle1);
                ZwClose(enumHandle);
            }
        } else {
            ZwClose(handle1);
            ZwClose(enumHandle);
        }
    } else {
        ZwClose(enumHandle);
    }
exit:
    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();
    return;
}

NTSTATUS
IopDeviceCapabilitiesToRegistry (
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine queries and updates device capabilities after device received a start irp.

Arguments:

    DeviceObject - supplies a pointer to a device object whose registry
        values are to be updated.

Return Value:

    status

--*/

{
    PDEVICE_NODE deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    HANDLE handle;
    NTSTATUS status;
    UNICODE_STRING unicodeName;
    ULONG tmpValue;
    DEVICE_CAPABILITIES capabilities;

    PAGED_CODE();

    //
    // Open the device instance key
    //

    status = IopDeviceObjectToDeviceInstance(DeviceObject, &handle, KEY_ALL_ACCESS);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = IopQueryDeviceCapabilities(deviceNode, &capabilities);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (deviceNode->Flags & DNF_HAS_BOOT_CONFIG) {
        capabilities.SurpriseRemovalOK = 0;
    }

    PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_CAPABILITIES);
    tmpValue = (capabilities.LockSupported)          |
               (capabilities.EjectSupported    << 1) |
               (capabilities.Removable         << 2) |
               (capabilities.DockDevice        << 3) |
               (capabilities.UniqueID          << 4) |
               (capabilities.SilentInstall     << 5) |
               (capabilities.RawDeviceOK       << 6) |
               (capabilities.SurpriseRemovalOK << 7);

    status = ZwSetValueKey(
                  handle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_DWORD,
                  &tmpValue,
                  sizeof(tmpValue)
                  );

    ZwClose(handle);
    return status;
}

NTSTATUS
IopRestartDeviceNode(
    IN PDEVICE_NODE DeviceNode
    )
{
    PAGED_CODE();

    ASSERT(!(IopDoesDevNodeHaveProblem(DeviceNode) ||
            (DeviceNode->Flags & (DNF_STARTED |
                                 DNF_ADDED |
                                 DNF_RESOURCE_ASSIGNED |
                                 DNF_RESOURCE_REPORTED)) ||
            (DeviceNode->UserFlags & DNUF_WILL_BE_REMOVED)));

    ASSERT(DeviceNode->Flags & DNF_ENUMERATED);

    if (!(DeviceNode->Flags & DNF_ENUMERATED)) {
        return STATUS_UNSUCCESSFUL;
    }

    DeviceNode->UserFlags &= ~DNUF_NEED_RESTART;

#if DBG_SCOPE
    DeviceNode->FailureStatus = 0;
    if (DeviceNode->PreviousResourceList) {
        ExFreePool(DeviceNode->PreviousResourceList);
        DeviceNode->PreviousResourceList = NULL;
    }
    if (DeviceNode->PreviousResourceRequirements) {
        ExFreePool(DeviceNode->PreviousResourceRequirements);
        DeviceNode->PreviousResourceRequirements = NULL;
    }
#endif

    //
    // Free any existing devnode strings so we can recreate them
    // during enumeration.
    //

    if (DeviceNode->Flags & DNF_PROCESSED) {

        DeviceNode->Flags &= ~(DNF_PROCESSED | DNF_ENUMERATION_REQUEST_QUEUED | DNF_RESOURCE_REQUIREMENTS_CHANGED);

        if (DeviceNode->ServiceName.Length != 0) {
            ExFreePool(DeviceNode->ServiceName.Buffer);
            RtlInitUnicodeString(&DeviceNode->ServiceName, NULL);
        }

        if (DeviceNode->InstanceOrdinal.Length != 0) {
            ExFreePool(DeviceNode->InstanceOrdinal.Buffer);
            RtlInitUnicodeString(&DeviceNode->InstanceOrdinal, NULL);
        }

        if (DeviceNode->ResourceRequirements != NULL) {
            ExFreePool(DeviceNode->ResourceRequirements);
            DeviceNode->ResourceRequirements = NULL;
            DeviceNode->Flags &= ~DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED;
        }
    }

    ASSERT(DeviceNode->ServiceName.Length == 0 &&
           DeviceNode->ServiceName.MaximumLength == 0 &&
           DeviceNode->ServiceName.Buffer == NULL);

    ASSERT(DeviceNode->InstanceOrdinal.Length == 0 &&
           DeviceNode->InstanceOrdinal.MaximumLength == 0 &&
           DeviceNode->InstanceOrdinal.Buffer == NULL);

    ASSERT(!(DeviceNode->Flags &
           ~(DNF_MADEUP | DNF_ENUMERATED | DNF_HAS_BOOT_CONFIG |
             DNF_BOOT_CONFIG_RESERVED | DNF_NO_RESOURCE_REQUIRED)));

    return STATUS_SUCCESS;
}
