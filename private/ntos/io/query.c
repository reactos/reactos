/*++

Copyright (c) 1989-1994  Microsoft Corporation

Module Name:

    query.c

Abstract:

    This module contains the subroutines to Query Device Descriptions from
    the Hardware tree in the registry

Author:

    Andre Vachon (andreva) 20-Jun-1994

Environment:

    Kernel mode

Revision History:


--*/

#include "iop.h"

typedef struct _IO_QUERY_DESC {
    PINTERFACE_TYPE BusType;
    PULONG BusNumber;
    PCONFIGURATION_TYPE ControllerType;
    PULONG ControllerNumber;
    PCONFIGURATION_TYPE PeripheralType;
    PULONG PeripheralNumber;
    PIO_QUERY_DEVICE_ROUTINE CalloutRoutine;
    PVOID Context;
} IO_QUERY_DESC, *PIO_QUERY_DESC;


NTSTATUS
pIoQueryBusDescription(
    PIO_QUERY_DESC QueryDescription,
    UNICODE_STRING PathName,
    HANDLE RootHandle,
    PULONG BusNum,
    BOOLEAN HighKey
    );

NTSTATUS
pIoQueryDeviceDescription(
    PIO_QUERY_DESC QueryDescription,
    UNICODE_STRING PathName,
    HANDLE RootHandle,
    ULONG BusNum,
    PKEY_VALUE_FULL_INFORMATION *BusValueInfo
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IoQueryDeviceDescription)
#pragma alloc_text(PAGE, pIoQueryBusDescription)
#pragma alloc_text(PAGE, pIoQueryDeviceDescription)
#endif




NTSTATUS
IoQueryDeviceDescription(
    IN PINTERFACE_TYPE BusType OPTIONAL,
    IN PULONG BusNumber OPTIONAL,
    IN PCONFIGURATION_TYPE ControllerType OPTIONAL,
    IN PULONG ControllerNumber OPTIONAL,
    IN PCONFIGURATION_TYPE PeripheralType OPTIONAL,
    IN PULONG PeripheralNumber OPTIONAL,
    IN PIO_QUERY_DEVICE_ROUTINE CalloutRoutine,
    IN PVOID Context
    )

/*++

Routine Description:


Arguments:

    BusType - Supplies an optional bus type being searched for in the
        description tree. Valid types are Mca, Isa, Eisa ... If no bus type
        is specified, the system information (i.e. machine BIOS) is returned.

    BusNumber - Supplies an optional value determining which bus should be
        queried.

    ControllerType - Supplies an optional controller type being searched for.
        If no Controller type is specified, only the Bus information is
        returned.

    ControllerNumber - Supplies an optional value determining which
        controller should be queried.

    PeripheralType - Supplies an optional peripheral type being searched for.
        If no Controller type is specified, only the Bus information and the
        controller information are returned.

    PeripheralNumber - Supplies an optional value determining which
        peripheral should be queried.

    CalloutRoutine - Supplies a pointer to a routine that gets called
       for each successful match of PeripheralType.

    Context - Supplies a context value that is passed back to the callback
        routine.

Return Value:

    The status returned is the final completion status of the operation.

Notes:

--*/

{

#define UNICODE_NUM_LENGTH 14
#define UNICODE_REGISTRY_PATH_LENGTH 1024

    IO_QUERY_DESC queryDesc;

    NTSTATUS status;
    UNICODE_STRING registryPathName;
    HANDLE rootHandle;
    ULONG busNumber = (ULONG) -1;


    PAGED_CODE();

    ASSERT( CalloutRoutine != NULL );

    //
    // Check if we need to return the machine information
    //

    if (!ARGUMENT_PRESENT( BusType )) {
        return STATUS_NOT_IMPLEMENTED;
    }

    queryDesc.BusType = BusType;
    queryDesc.BusNumber = BusNumber;
    queryDesc.ControllerType = ControllerType;
    queryDesc.ControllerNumber = ControllerNumber;
    queryDesc.PeripheralType = PeripheralType;
    queryDesc.PeripheralNumber = PeripheralNumber;
    queryDesc.CalloutRoutine = CalloutRoutine;
    queryDesc.Context = Context;


    //
    // Set up a string with the pathname to the hardware description
    // portion of the registry.
    //

    registryPathName.Length = 0;
    registryPathName.MaximumLength = UNICODE_REGISTRY_PATH_LENGTH *
                                     sizeof(WCHAR);

    registryPathName.Buffer = ExAllocatePoolWithTag( PagedPool,
                                                     UNICODE_REGISTRY_PATH_LENGTH,
                                                     'mNoI' );

    if (!registryPathName.Buffer) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    RtlAppendUnicodeStringToString( &registryPathName,
                                    &CmRegistryMachineHardwareDescriptionSystemName );


    //
    // Open a handle to the root path we have.
    //

    status = IopOpenRegistryKey( &rootHandle,
                                 (HANDLE) NULL,
                                 &registryPathName,
                                 KEY_READ,
                                 FALSE );

    if (NT_SUCCESS( status )) {

        status = pIoQueryBusDescription(&queryDesc,
                                        registryPathName,
                                        rootHandle,
                                        &busNumber,
                                        TRUE );

        ZwClose( rootHandle );

    }

    ExFreePool( registryPathName.Buffer );

    //
    // For compatibility with old version of the function.
    //

    if (status == STATUS_NO_MORE_ENTRIES) {

        return STATUS_OBJECT_NAME_NOT_FOUND;


    } else {

        return status;

    }
}


NTSTATUS
pIoQueryBusDescription(
    PIO_QUERY_DESC QueryDescription,
    UNICODE_STRING PathName,
    HANDLE RootHandle,
    PULONG BusNum,
    BOOLEAN HighKey
    )

/*++

Routine Description:


Arguments:

    QueryDescription - Buffer containing all the query information requested
        by the driver.

    PathName - Registry path name of the key we are dealing with.  This is
        a unicode strig so that we don't have to bother with resetting NULLs
        at the end of the string - the length determines how much of the
        string is valid.

    RootHandle - Handle equivalent to the registry path.

    BusNum - Pointer to a variable that keeps track of the bus number we are
        searching for (buses have to be accumulated.

    HighKey - Determines is this is a high key (a root key with a list of
        bus types) or a low level key (under which the number of the various
        buses will be little).

Return Value:

    The status returned is the final completion status of the operation.

Notes:

--*/

{
    NTSTATUS status;
    ULONG i;
    UNICODE_STRING unicodeString;

    UNICODE_STRING registryPathName;

    ULONG keyBasicInformationSize;
    PKEY_BASIC_INFORMATION keyBasicInformation = NULL;
    HANDLE handle;

    PKEY_FULL_INFORMATION keyInformation;
    ULONG size;

    PKEY_VALUE_FULL_INFORMATION busValueInfo[IoQueryDeviceMaxData];


    PAGED_CODE();

    status = IopGetRegistryKeyInformation( RootHandle,
                                           &keyInformation );

    if (NT_SUCCESS( status )) {

        //
        // With the keyInformation, allocate a buffer that will be large
        // enough for all the subkeys
        //

        keyBasicInformationSize = keyInformation->MaxNameLen +
                                  sizeof(KEY_NODE_INFORMATION);

        keyBasicInformation = ExAllocatePoolWithTag( PagedPool,
                                                     keyBasicInformationSize,
                                                     'mNoI' );

        ExFreePool(keyInformation);

        if (keyBasicInformation == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
    }

    //
    // Now we need to enumerate the keys and see if one of them is a bus
    //

    for (i = 0; NT_SUCCESS( status ); i++) {


        //
        // If we have found the Bus we are looking for, break
        //

        if ((ARGUMENT_PRESENT( QueryDescription->BusNumber )) &&
            (*(QueryDescription->BusNumber) == *BusNum)) {

            break;

        }

        status = ZwEnumerateKey( RootHandle,
                                 i,
                                 KeyBasicInformation,
                                 keyBasicInformation,
                                 keyBasicInformationSize,
                                 &size );

        //
        // If the sub function enumerated all the buses till the end, then
        // treat that as success.
        //

        if (!NT_SUCCESS( status )) {

            break;

        }

        //
        // Only if this is a high key (otherwise we are in the callback
        // pass which we will process later on).
        //
        // If the string is any valid bus string, then we have to go down
        // the tree recursively.
        // Otherwise, go on to the next key.
        //

        if (HighKey) {

            if (wcsncmp( keyBasicInformation->Name,
                         CmTypeString[MultiFunctionAdapter],
                         keyBasicInformation->NameLength / sizeof(WCHAR) )  &&
                wcsncmp( keyBasicInformation->Name,
                         CmTypeString[EisaAdapter],
                         keyBasicInformation->NameLength / sizeof(WCHAR) )  &&
                wcsncmp( keyBasicInformation->Name,
                         CmTypeString[TcAdapter],
                         keyBasicInformation->NameLength / sizeof(WCHAR) )) {

                //
                // All the comparisons returned 1 (which means they all were
                // unsuccessful) so we do not have a bus.
                //
                // Go on to the next key.
                //

                continue;
            }
        }

        //
        // We have a bus. Open that key and enumerate it's clidren
        // (which should be numbers)
        //

        unicodeString.Buffer = keyBasicInformation->Name;
        unicodeString.Length = (USHORT) keyBasicInformation->NameLength;
        unicodeString.MaximumLength = (USHORT) keyBasicInformation->NameLength;

        if (!NT_SUCCESS( IopOpenRegistryKey( &handle,
                                             RootHandle,
                                             &unicodeString,
                                             KEY_READ,
                                             FALSE ) )) {

            //
            // The key could not be opened. Go to the next key
            //

            continue;

        }

        //
        // We have the key. now build the name for this path.
        //
        // Reset the string to its original value
        //

        registryPathName = PathName;

        RtlAppendUnicodeToString( &registryPathName,
                                  L"\\" );

        RtlAppendUnicodeStringToString( &registryPathName,
                                        &unicodeString );


        if (!HighKey) {

            //
            // We have a Key. Get the information for that key
            //

            status = IopGetRegistryValues( handle,
                                           &busValueInfo[0] );

            if (NT_SUCCESS( status )) {

                //
                // Verify that the identifier value for this bus
                // sub-key matches the user-specified bus type.
                // If not, do not increment the number of *found*
                // buses.
                //

                if (( busValueInfo[IoQueryDeviceConfigurationData] != NULL ) &&
                    ( busValueInfo[IoQueryDeviceConfigurationData]->DataLength != 0 ) &&
                    ( ((PCM_FULL_RESOURCE_DESCRIPTOR)
                        ((PCCHAR) busValueInfo[IoQueryDeviceConfigurationData] +
                        busValueInfo[IoQueryDeviceConfigurationData]->DataOffset))
                        ->InterfaceType == *(QueryDescription->BusType) )) {

                    //
                    // Increment the number of buses of desired type we
                    // have found.
                    //

                    (*BusNum)++;

                    //
                    // If we are looking for a specific bus number,
                    // check to see if we are at the right number.
                    // If we are not goto the next bus.  Otherwise
                    // (i.e we have the right bus number, or we
                    // specified all buses), then go on so the
                    // information can be reported.
                    //

                    if ( (QueryDescription->BusNumber == NULL) ||
                         (*(QueryDescription->BusNumber) == *BusNum) ) {


                        //
                        // If we want controller information, call
                        // the controller function.
                        // Otherwise just return the bus information.
                        //

                        if (QueryDescription->ControllerType != NULL) {

                            status = pIoQueryDeviceDescription(
                                         QueryDescription,
                                         registryPathName,
                                         handle,
                                         *BusNum,
                                         (PKEY_VALUE_FULL_INFORMATION *) busValueInfo );

                        } else {

                            status = QueryDescription->CalloutRoutine(
                                         QueryDescription->Context,
                                         &registryPathName,
                                         *(QueryDescription->BusType),
                                         *BusNum,
                                         (PKEY_VALUE_FULL_INFORMATION *) busValueInfo,
                                         0,
                                         0,
                                         NULL,
                                         0,
                                         0,
                                         NULL );

                        }
                    }
                }

                //
                // Free the pool allocated for the controller value data.
                //

                if (busValueInfo[0]) {
                    ExFreePool( busValueInfo[0] );
                    busValueInfo[0] = NULL;
                }
                if (busValueInfo[1]) {
                    ExFreePool( busValueInfo[1] );
                    busValueInfo[1] = NULL;
                }
                if (busValueInfo[2]) {
                    ExFreePool( busValueInfo[2] );
                    busValueInfo[2] = NULL;
                }

            }


            //
            // Shortcurt exit to avoid the recursive call.
            //

            if ((QueryDescription->BusNumber !=NULL ) &&
                (*(QueryDescription->BusNumber) == *BusNum)) {
                ZwClose( handle );
                handle = NULL;
                continue;

            }
        }

        //
        // If we have the key handle, do recursive enumeration.
        // enumaration (for both high and low keys)
        //

        status = pIoQueryBusDescription(
                     QueryDescription,
                     registryPathName,
                     handle,
                     BusNum,
                     (BOOLEAN)!HighKey );

        //
        // If the sub function enumerated all the buses till the end, then
        // treat that as success.
        //

        if (status == STATUS_NO_MORE_ENTRIES) {

            status = STATUS_SUCCESS;

        }

        ZwClose( handle );
        handle = NULL;

    }

    if (keyBasicInformation) {
        ExFreePool( keyBasicInformation );
    }

    return status;
}




NTSTATUS
pIoQueryDeviceDescription(
    PIO_QUERY_DESC QueryDescription,
    UNICODE_STRING PathName,
    HANDLE RootHandle,
    ULONG BusNum,
    PKEY_VALUE_FULL_INFORMATION *BusValueInfo
    )

{

    NTSTATUS status;
    UNICODE_STRING registryPathName = PathName;
    UNICODE_STRING controllerBackupRegistryPathName;
    UNICODE_STRING peripheralBackupRegistryPathName;
    HANDLE controllerHandle = NULL;
    HANDLE peripheralHandle = NULL;
    PKEY_FULL_INFORMATION controllerTypeInfo = NULL;
    PKEY_FULL_INFORMATION peripheralTypeInfo = NULL;
    ULONG maxControllerNum;
    ULONG maxPeripheralNum;
    ULONG controllerNum;
    ULONG peripheralNum;
    WCHAR numBuffer[UNICODE_NUM_LENGTH];
    UNICODE_STRING bufferUnicodeString;
    PKEY_VALUE_FULL_INFORMATION controllerValueInfo[IoQueryDeviceMaxData];
    PKEY_VALUE_FULL_INFORMATION peripheralValueInfo[IoQueryDeviceMaxData];


    //
    // Set up a string for the number translation.
    //

    bufferUnicodeString.MaximumLength = UNICODE_NUM_LENGTH * sizeof(WCHAR);
    bufferUnicodeString.Buffer = &numBuffer[0];


    //         For each controller of the specified type (subkeys 0..M)
    //             if we are looking for controller information
    //                 call the specified callout routine
    //             else
    //                 For each peripheral of the specified type (subkeys 0..N)
    //                     call the specified callout routine

    //
    // Add the controller name to the registry path name.
    //

    status = RtlAppendUnicodeToString( &registryPathName,
                                       L"\\" );

    if (NT_SUCCESS( status )) {

        status = RtlAppendUnicodeToString( &registryPathName,
                                           CmTypeString[*(QueryDescription->ControllerType)] );

    }

    if (!NT_SUCCESS( status )) {
        return status;
    }

    //
    // If a Contoller number was specified by the caller, use that
    // controller number. Otherwise, find out how many buses are present
    // by querying the key.
    //

    if (ARGUMENT_PRESENT( QueryDescription->ControllerNumber )) {

        controllerNum = *(QueryDescription->ControllerNumber);
        maxControllerNum = controllerNum + 1;

    } else {

        //
        // Open the registry key for the controller and
        // Get the full key information for the controller key to
        // determine the number of sub-keys (controller numbers).
        // And we fail, then go on to the next bus.
        // Note the memory allocated by the query must be freed.
        //

        status = IopOpenRegistryKey( &controllerHandle,
                                     (HANDLE) NULL,
                                     &registryPathName,
                                     KEY_READ,
                                     FALSE );

        if (NT_SUCCESS( status )) {

            status = IopGetRegistryKeyInformation( controllerHandle,
                                                   &controllerTypeInfo );

            ZwClose( controllerHandle );
            controllerHandle = NULL;
        }

        //
        // If no controller of this type was found on the bus, go on to
        // the next bus; goto the end of the loop with a successful status
        // so that the memory gets freed, but we continue looping.
        //

        if (!NT_SUCCESS( status )) {

            return status;

        }

        //
        // Get the number of controller sub-keys for this controller
        // type and free the pool.
        //

        maxControllerNum = controllerTypeInfo->SubKeys;
        controllerNum = 0;

        ExFreePool( controllerTypeInfo );
        controllerTypeInfo = NULL;
    }

    //
    // Make a backup of the string since we want to start where we were
    // on the next loop iteration.
    //

    controllerBackupRegistryPathName = registryPathName;

    //
    // For each controller of the specified type (subkeys 0..M).
    // We use BusNumber as the initial value since it is zero if we want
    // all buses, and we only want the bus specified if the value  is not
    // zero.
    //

    for ( ; controllerNum < maxControllerNum; controllerNum++) {

        //
        // Reset the string to its original value
        //

        registryPathName = controllerBackupRegistryPathName;

        //
        // Convert the controller number to a unicode string and append
        // it to the registry path name.
        //

        bufferUnicodeString.Length = (UNICODE_NUM_LENGTH-1) * sizeof(WCHAR);
        status = RtlIntegerToUnicodeString( controllerNum,
                                            10,
                                            &bufferUnicodeString );

        if (NT_SUCCESS( status )) {

            status = RtlAppendUnicodeToString( &registryPathName,
                                               L"\\" );

            if (NT_SUCCESS( status )) {

                status = RtlAppendUnicodeStringToString(
                                                     &registryPathName,
                                                     &bufferUnicodeString );

            }
        }

        if (!NT_SUCCESS( status )) {
            break;
        }

        //
        // Open the registry key for the controller number and
        // Get the value data for this controller and save it for later.
        //


        status = IopOpenRegistryKey( &controllerHandle,
                                     (HANDLE) NULL,
                                     &registryPathName,
                                     KEY_READ,
                                     FALSE );

        if (NT_SUCCESS( status )) {

            status = IopGetRegistryValues( controllerHandle,
                                           &controllerValueInfo[0] );

            ZwClose( controllerHandle );
            controllerHandle = NULL;
        }

        //
        // If we could not open the key and get the info, just continue
        // since there is no memory to free and we are using the for
        // loop to determine when we get to the last controller.
        //

        if (!NT_SUCCESS( status )) {
            continue;
        }

        //
        // Check if we want the controller and bus information only. If
        // it is the case, invoque the callout routine and go on to the
        // next loop (unless an error occurs in the callout).
        //

        if (!ARGUMENT_PRESENT( (QueryDescription->PeripheralType) )) {

            status = QueryDescription->CalloutRoutine(
                         QueryDescription->Context,
                         &registryPathName,
                         *(QueryDescription->BusType),
                         BusNum,
                         BusValueInfo,
                         *(QueryDescription->ControllerType),
                         controllerNum,
                         (PKEY_VALUE_FULL_INFORMATION *) controllerValueInfo,
                         0,
                         0,
                         NULL );

            goto IoQueryDeviceControllerLoop;
        }

        //
        // Add the peripheral name to the registry path name.
        //

        status = RtlAppendUnicodeToString( &registryPathName,
                                           L"\\" );

        if (NT_SUCCESS( status )) {

            status = RtlAppendUnicodeToString(
                                             &registryPathName,
                                             CmTypeString[*(QueryDescription->PeripheralType)] );

        }

        if (!NT_SUCCESS( status )) {
            goto IoQueryDeviceControllerLoop;
        }

        //
        // If a Peripheralnumber was specified by the caller, use that
        // peripheral number. Otherwise, find out how many buses are
        // present by querying the key.
        //

        if (ARGUMENT_PRESENT( (QueryDescription->PeripheralNumber) )) {

            peripheralNum = *(QueryDescription->PeripheralNumber);
            maxPeripheralNum = peripheralNum + 1;

        } else {

            //
            // Open the registry key for the peripheral and
            // Get the full key information for the peripheral key to
            // determine the number of sub-keys (peripheral numbers).
            // And we fail, then go on to the next controller.
            // Note the memory allocated by the query must be freed.
            //

            status = IopOpenRegistryKey( &peripheralHandle,
                                         (HANDLE) NULL,
                                         &registryPathName,
                     KEY_READ,
                     FALSE );

            if (NT_SUCCESS( status )) {

                status = IopGetRegistryKeyInformation( peripheralHandle,
                           &peripheralTypeInfo );

                ZwClose( peripheralHandle );
                peripheralHandle = NULL;
            }

            //
            // If no controller of this type was found on the bus, go on to
            // the next bus; goto the end of the loop with a successful
            // status so that the memory gets freed, but we continue looping.
            //

            if (!NT_SUCCESS( status )) {
                status = STATUS_SUCCESS;
                goto IoQueryDeviceControllerLoop;
            }

            //
            // Get the number of peripheral sub-keys for this peripheral
            // type and free the pool.
            //

            maxPeripheralNum = peripheralTypeInfo->SubKeys;
            peripheralNum = 0;

            ExFreePool( peripheralTypeInfo );
            peripheralTypeInfo = NULL;
        }

        //
        // Make a backup of the string since we want to start where we
        // were on the next loop iteration.
        //

        peripheralBackupRegistryPathName = registryPathName;

        //
        // For each peripheral of the specified type (subkeys 0..N).
        // We use BusNumber as the initial value since it is zero if we
        // want all buses, and we only want the bus specified if the
        // value is not zero.
        //

        for ( ; peripheralNum < maxPeripheralNum; peripheralNum++) {

            //
            // Reset the string to its original value.
            //

            registryPathName = peripheralBackupRegistryPathName;

            //
            // Convert the peripheral number to a unicode string and append
            // it to the registry path name.
            //

            bufferUnicodeString.Length =
                (UNICODE_NUM_LENGTH-1) * sizeof(WCHAR);
            status = RtlIntegerToUnicodeString( peripheralNum,
                                                10,
                                                &bufferUnicodeString );

            if (NT_SUCCESS( status )) {

                status = RtlAppendUnicodeToString( &registryPathName,
                                                   L"\\" );

                if (NT_SUCCESS( status )) {

                    status = RtlAppendUnicodeStringToString(
                                                     &registryPathName,
                                                     &bufferUnicodeString );

                }
            }

            if (!NT_SUCCESS( status )) {
                break;
            }

            //
            // Open the registry key for the peripheral number and
            // Get the value data for this peripheral and save it for
            // later.
            //

            status = IopOpenRegistryKey( &peripheralHandle,
                                         (HANDLE) NULL,
                                         &registryPathName,
                                         KEY_READ,
                                         FALSE );

            if (NT_SUCCESS( status )) {

                status = IopGetRegistryValues( peripheralHandle,
                                               &peripheralValueInfo[0] );

                ZwClose( peripheralHandle );
                peripheralHandle = NULL;
            }

            //
            // If getting the peripheral information worked properly,
            // call the user-specified callout routine.
            //

            if (NT_SUCCESS( status )) {

                status = QueryDescription->CalloutRoutine(
                             QueryDescription->Context,
                             &registryPathName,
                             *(QueryDescription->BusType),
                             BusNum,
                             BusValueInfo,
                             *(QueryDescription->ControllerType),
                             controllerNum,
                             (PKEY_VALUE_FULL_INFORMATION *) controllerValueInfo,
                             *(QueryDescription->PeripheralType),
                             peripheralNum,
                             (PKEY_VALUE_FULL_INFORMATION *) peripheralValueInfo );

                //
                // Free the pool allocated for the peripheral value data.
                //

                if (peripheralValueInfo[0]) {
                    ExFreePool( peripheralValueInfo[0] );
                    peripheralValueInfo[0] = NULL;
                }
                if (peripheralValueInfo[1]) {
                    ExFreePool( peripheralValueInfo[1] );
                    peripheralValueInfo[1] = NULL;
                }
                if (peripheralValueInfo[2]) {
                    ExFreePool( peripheralValueInfo[2] );
                    peripheralValueInfo[2] = NULL;
                }

                //
                // If the user-specified callout routine returned with
                // an unsuccessful status, quit.
                //

                if (!NT_SUCCESS( status )) {
                    break;
               }
            }

        } // for ( ; peripheralNum < maxPeripheralNum ...

IoQueryDeviceControllerLoop:

        //
        // Free the pool allocated for the controller value data.
        //

        if (controllerValueInfo[0]) {
            ExFreePool( controllerValueInfo[0] );
            controllerValueInfo[0] = NULL;
        }
        if (controllerValueInfo[1]) {
            ExFreePool( controllerValueInfo[1] );
            controllerValueInfo[1] = NULL;
        }
        if (controllerValueInfo[2]) {
            ExFreePool( controllerValueInfo[2] );
            controllerValueInfo[2] = NULL;
        }

        if (!NT_SUCCESS( status )) {
            break;
        }

    } // for ( ; controllerNum < maxControllerNum...


    return( status );
}

