/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware resource management
 * COPYRIGHT:   Copyright 1998 David Welch <welch@mcmail.com>
 *              Copyright 2001 Eric Kohl <eric.kohl@reactos.org>
 *              Copyright 2004-2013 Alex Ionescu <alex.ionescu@reactos.org>
 *              Copyright 2010 Cameron Gutman <cameron.gutman@reactos.org>
 *              Copyright 2010 Pierre Schweitzer <pierre@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#ifndef NDEBUG
    #define IORSRCTRACE(...)    DbgPrint(__VA_ARGS__)
#else
    #if defined(_MSC_VER)
    #define IORSRCTRACE     __noop
    #else
    #define IORSRCTRACE(...)    do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #endif
#endif

/* GLOBALS *******************************************************************/

static CONFIGURATION_INFORMATION
_SystemConfigurationInformation = { 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, 0, 0 };

/* API parameters to pass to IopQueryBusDescription() */
typedef struct _IO_QUERY
{
    PINTERFACE_TYPE BusType;
    PULONG BusNumber;
    PCONFIGURATION_TYPE ControllerType;
    PULONG ControllerNumber;
    PCONFIGURATION_TYPE PeripheralType;
    PULONG PeripheralNumber;
    PIO_QUERY_DEVICE_ROUTINE CalloutRoutine;
    PVOID Context;
} IO_QUERY, *PIO_QUERY;

/* Strings corresponding to CONFIGURATION_TYPE */
PCWSTR ArcTypes[MaximumType + 1] =
{
    L"System",
    L"CentralProcessor",
    L"FloatingPointProcessor",
    L"PrimaryICache",
    L"PrimaryDCache",
    L"SecondaryICache",
    L"SecondaryDCache",
    L"SecondaryCache",
    L"EisaAdapter",
    L"TcAdapter",
    L"ScsiAdapter",
    L"DtiAdapter",
    L"MultifunctionAdapter",
    L"DiskController",
    L"TapeController",
    L"CdRomController",
    L"WormController",
    L"SerialController",
    L"NetworkController",
    L"DisplayController",
    L"ParallelController",
    L"PointerController",
    L"KeyboardController",
    L"AudioController",
    L"OtherController",
    L"DiskPeripheral",
    L"FloppyDiskPeripheral",
    L"TapePeripheral",
    L"ModemPeripheral",
    L"MonitorPeripheral",
    L"PrinterPeripheral",
    L"PointerPeripheral",
    L"KeyboardPeripheral",
    L"TerminalPeripheral",
    L"OtherPeripheral",
    L"LinePeripheral",
    L"NetworkPeripheral",
    L"SystemMemory",
    L"DockingInformation",
    L"RealModeIrqRoutingTable",
    L"RealModePCIEnumeration",
    L"Undefined"
};

/* Strings corresponding to IO_QUERY_DEVICE_DATA_FORMAT */
PCWSTR IoDeviceInfoNames[IoQueryDeviceMaxData] =
{
    L"Identifier",
    L"Configuration Data",
    L"Component Information"
};

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * Reads and returns Hardware information from the appropriate hardware
 * registry key. Helper stub of IopQueryBusDescription().
 *
 * @param[in]   Query
 * What the parent function wants.
 *
 * @param[in]   RootKey
 * Which key to look in.
 *
 * @param[in]   RootKeyHandle
 * Handle to the key.
 *
 * @param[in]   Bus
 * The bus number.
 *
 * @param[in]   BusInformation
 * The configuration information being sent.
 *
 * @return  A status code.
 **/
static NTSTATUS
IopQueryDeviceDescription(
    _In_ PIO_QUERY Query,
    _In_ UNICODE_STRING RootKey,
    _In_ HANDLE RootKeyHandle,
    _In_ ULONG Bus,
    _In_ PKEY_VALUE_FULL_INFORMATION* BusInformation)
{
    NTSTATUS Status = STATUS_SUCCESS;

    /* Controller data */
    UNICODE_STRING ControllerString;
    UNICODE_STRING ControllerRootRegName = RootKey;
    UNICODE_STRING ControllerRegName;
    HANDLE ControllerKeyHandle;
    PKEY_FULL_INFORMATION ControllerFullInformation = NULL;
    PKEY_VALUE_FULL_INFORMATION ControllerInformation[IoQueryDeviceMaxData] =
        {NULL, NULL, NULL};
    ULONG ControllerNumber;
    ULONG ControllerLoop;
    ULONG MaximumControllerNumber;

    /* Peripheral data */
    UNICODE_STRING PeripheralString;
    HANDLE PeripheralKeyHandle;
    PKEY_FULL_INFORMATION PeripheralFullInformation;
    PKEY_VALUE_FULL_INFORMATION PeripheralInformation[IoQueryDeviceMaxData] =
        {NULL, NULL, NULL};
    ULONG PeripheralNumber;
    ULONG PeripheralLoop;
    ULONG MaximumPeripheralNumber;

    /* Global Registry data */
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG LenFullInformation;
    ULONG LenKeyFullInformation;
    UNICODE_STRING TempString;
    WCHAR TempBuffer[14];

    IORSRCTRACE("\nIopQueryDeviceDescription(Query: 0x%p)\n"
                "    RootKey: '%wZ'\n"
                "    RootKeyHandle: 0x%p\n"
                "    Bus: %lu\n",
                Query,
                &RootKey, RootKeyHandle,
                Bus);

    /* Temporary string */
    TempString.MaximumLength = sizeof(TempBuffer);
    TempString.Length = 0;
    TempString.Buffer = TempBuffer;

    /* Append controller name to string */
    RtlAppendUnicodeToString(&ControllerRootRegName, L"\\");
    RtlAppendUnicodeToString(&ControllerRootRegName, ArcTypes[*Query->ControllerType]);

    /* Set the controller number if specified */
    if (Query->ControllerNumber && *(Query->ControllerNumber))
    {
        ControllerNumber = *Query->ControllerNumber;
        MaximumControllerNumber = ControllerNumber + 1;
        IORSRCTRACE("    Getting controller #%lu\n", ControllerNumber);
    }
    else
    {
        IORSRCTRACE("    Enumerating controllers in '%wZ'...\n", &ControllerRootRegName);

        /* Find out how many controllers there are */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &ControllerRootRegName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = ZwOpenKey(&ControllerKeyHandle, KEY_READ, &ObjectAttributes);

        if (NT_SUCCESS(Status))
        {
            /* Retrieve the necessary buffer space */
            ZwQueryKey(ControllerKeyHandle,
                       KeyFullInformation,
                       NULL, 0,
                       &LenFullInformation);

            /* Allocate it */
            ControllerFullInformation = ExAllocatePoolWithTag(PagedPool, LenFullInformation, TAG_IO_RESOURCE);

            /* Get the information */
            Status = ZwQueryKey(ControllerKeyHandle,
                                KeyFullInformation,
                                ControllerFullInformation,
                                LenFullInformation,
                                &LenFullInformation);
            ZwClose(ControllerKeyHandle);
            ControllerKeyHandle = NULL;
        }

        /* No controller was found, bail out */
        if (!NT_SUCCESS(Status))
        {
            if (ControllerFullInformation != NULL)
                ExFreePoolWithTag(ControllerFullInformation, TAG_IO_RESOURCE);
            return Status;
        }

        /* Find out the controllers */
        ControllerNumber = 0;
        MaximumControllerNumber = ControllerFullInformation->SubKeys;

        /* Cleanup */
        ExFreePoolWithTag(ControllerFullInformation, TAG_IO_RESOURCE);
        ControllerFullInformation = NULL;
    }

    /* Save string */
    ControllerRegName = ControllerRootRegName;

    /* Loop through controllers */
    for (; ControllerNumber < MaximumControllerNumber; ControllerNumber++)
    {
        /* Load string */
        ControllerRootRegName = ControllerRegName;

        /* Convert controller number to registry string */
        Status = RtlIntegerToUnicodeString(ControllerNumber, 10, &TempString);

        /* Create string */
        Status |= RtlAppendUnicodeToString(&ControllerRootRegName, L"\\");
        Status |= RtlAppendUnicodeStringToString(&ControllerRootRegName, &TempString);

        /* Something messed up */
        if (!NT_SUCCESS(Status))
            break;

        IORSRCTRACE("    Retrieving controller '%wZ'\n", &ControllerRootRegName);

        /* Open the registry key */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &ControllerRootRegName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = ZwOpenKey(&ControllerKeyHandle, KEY_READ, &ObjectAttributes);

        /* Read the configuration data */
        if (NT_SUCCESS(Status))
        {
            for (ControllerLoop = 0; ControllerLoop < RTL_NUMBER_OF(IoDeviceInfoNames); ControllerLoop++)
            {
                /* Identifier string first */
                RtlInitUnicodeString(&ControllerString, IoDeviceInfoNames[ControllerLoop]);

                /* Retrieve the necessary buffer space */
                Status = ZwQueryValueKey(ControllerKeyHandle,
                                         &ControllerString,
                                         KeyValueFullInformation,
                                         NULL, 0,
                                         &LenKeyFullInformation);

                if (!NT_SUCCESS(Status) &&
                    (Status != STATUS_BUFFER_TOO_SMALL) &&
                    (Status != STATUS_BUFFER_OVERFLOW))
                {
                    continue;
                }

                /* Allocate it */
                ControllerInformation[ControllerLoop] = ExAllocatePoolWithTag(PagedPool, LenKeyFullInformation, TAG_IO_RESOURCE);

                /* Get the information */
                Status = ZwQueryValueKey(ControllerKeyHandle,
                                         &ControllerString,
                                         KeyValueFullInformation,
                                         ControllerInformation[ControllerLoop],
                                         LenKeyFullInformation,
                                         &LenKeyFullInformation);
            }

            /* Cleanup */
            ZwClose(ControllerKeyHandle);
            ControllerKeyHandle = NULL;
        }

        /* Something messed up */
        if (!NT_SUCCESS(Status))
            goto EndLoop;

        /* We now have bus *AND* controller information, is it enough? */
        if (!Query->PeripheralType || !(*Query->PeripheralType))
        {
            IORSRCTRACE("    --> Bus #%lu Controller #%lu Callout: '%wZ'\n",
                        Bus, ControllerNumber, &ControllerRootRegName);

            Status = Query->CalloutRoutine(Query->Context,
                                           &ControllerRootRegName,
                                           *Query->BusType,
                                           Bus,
                                           BusInformation,
                                           *Query->ControllerType,
                                           ControllerNumber,
                                           ControllerInformation,
                                           0,
                                           0,
                                           NULL);
            goto EndLoop;
        }

        /* Not enough: the caller also wants peripheral name */
        Status = RtlAppendUnicodeToString(&ControllerRootRegName, L"\\");
        Status |= RtlAppendUnicodeToString(&ControllerRootRegName, ArcTypes[*Query->PeripheralType]);

        /* Something messed up */
        if (!NT_SUCCESS(Status))
            goto EndLoop;

        /* Set the peripheral number if specified */
        if (Query->PeripheralNumber && *Query->PeripheralNumber)
        {
            PeripheralNumber = *Query->PeripheralNumber;
            MaximumPeripheralNumber = PeripheralNumber + 1;
            IORSRCTRACE("    Getting peripheral #%lu\n", PeripheralNumber);
        }
        else
        {
            IORSRCTRACE("    Enumerating peripherals in '%wZ'...\n", &ControllerRootRegName);

            /* Find out how many peripherals there are */
            InitializeObjectAttributes(&ObjectAttributes,
                                       &ControllerRootRegName,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            Status = ZwOpenKey(&PeripheralKeyHandle, KEY_READ, &ObjectAttributes);

            if (NT_SUCCESS(Status))
            {
                /* Retrieve the necessary buffer space */
                ZwQueryKey(PeripheralKeyHandle,
                           KeyFullInformation,
                           NULL, 0,
                           &LenFullInformation);

                /* Allocate it */
                PeripheralFullInformation = ExAllocatePoolWithTag(PagedPool, LenFullInformation, TAG_IO_RESOURCE);

                /* Get the information */
                Status = ZwQueryKey(PeripheralKeyHandle,
                                    KeyFullInformation,
                                    PeripheralFullInformation,
                                    LenFullInformation,
                                    &LenFullInformation);
                ZwClose(PeripheralKeyHandle);
                PeripheralKeyHandle = NULL;
            }

            /* No controller was found, cleanup and bail out */
            if (!NT_SUCCESS(Status))
            {
                Status = STATUS_SUCCESS;
                goto EndLoop;
            }

            /* Find out peripheral number */
            PeripheralNumber = 0;
            MaximumPeripheralNumber = PeripheralFullInformation->SubKeys;

            /* Cleanup */
            ExFreePoolWithTag(PeripheralFullInformation, TAG_IO_RESOURCE);
            PeripheralFullInformation = NULL;
        }

        /* Save name */
        ControllerRegName = ControllerRootRegName;

        /* Loop through peripherals */
        for (; PeripheralNumber < MaximumPeripheralNumber; PeripheralNumber++)
        {
            /* Restore name */
            ControllerRootRegName = ControllerRegName;

            /* Convert peripheral number to registry string */
            Status = RtlIntegerToUnicodeString(PeripheralNumber, 10, &TempString);

            /* Create string */
            Status |= RtlAppendUnicodeToString(&ControllerRootRegName, L"\\");
            Status |= RtlAppendUnicodeStringToString(&ControllerRootRegName, &TempString);

            /* Something messed up */
            if (!NT_SUCCESS(Status))
                break;

            IORSRCTRACE("    Retrieving peripheral '%wZ'\n", &ControllerRootRegName);

            /* Open the registry key */
            InitializeObjectAttributes(&ObjectAttributes,
                                       &ControllerRootRegName,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            Status = ZwOpenKey(&PeripheralKeyHandle, KEY_READ, &ObjectAttributes);

            if (NT_SUCCESS(Status))
            {
                for (PeripheralLoop = 0; PeripheralLoop < RTL_NUMBER_OF(IoDeviceInfoNames); PeripheralLoop++)
                {
                    /* Identifier string first */
                    RtlInitUnicodeString(&PeripheralString, IoDeviceInfoNames[PeripheralLoop]);

                    /* Retrieve the necessary buffer space */
                    Status = ZwQueryValueKey(PeripheralKeyHandle,
                                             &PeripheralString,
                                             KeyValueFullInformation,
                                             NULL, 0,
                                             &LenKeyFullInformation);

                    if (!NT_SUCCESS(Status) &&
                        (Status != STATUS_BUFFER_TOO_SMALL) &&
                        (Status != STATUS_BUFFER_OVERFLOW))
                    {
                        PeripheralInformation[PeripheralLoop] = NULL;
                        continue;
                    }

                    /* Allocate it */
                    PeripheralInformation[PeripheralLoop] = ExAllocatePoolWithTag(PagedPool, LenKeyFullInformation, TAG_IO_RESOURCE);

                    /* Get the information */
                    Status = ZwQueryValueKey(PeripheralKeyHandle,
                                             &PeripheralString,
                                             KeyValueFullInformation,
                                             PeripheralInformation[PeripheralLoop],
                                             LenKeyFullInformation,
                                             &LenKeyFullInformation);
                }

                /* Cleanup */
                ZwClose(PeripheralKeyHandle);
                PeripheralKeyHandle = NULL;

                /* We now have everything the caller could possibly want */
                if (NT_SUCCESS(Status))
                {
                    IORSRCTRACE("    --> Bus #%lu Controller #%lu Peripheral #%lu Callout: '%wZ'\n",
                                Bus, ControllerNumber, PeripheralNumber, &ControllerRootRegName);

                    Status = Query->CalloutRoutine(Query->Context,
                                                   &ControllerRootRegName,
                                                   *Query->BusType,
                                                   Bus,
                                                   BusInformation,
                                                   *Query->ControllerType,
                                                   ControllerNumber,
                                                   ControllerInformation,
                                                   *Query->PeripheralType,
                                                   PeripheralNumber,
                                                   PeripheralInformation);
                }

                /* Free the allocated memory */
                for (PeripheralLoop = 0; PeripheralLoop < RTL_NUMBER_OF(IoDeviceInfoNames); PeripheralLoop++)
                {
                    if (PeripheralInformation[PeripheralLoop])
                    {
                        ExFreePoolWithTag(PeripheralInformation[PeripheralLoop], TAG_IO_RESOURCE);
                        PeripheralInformation[PeripheralLoop] = NULL;
                    }
                }

                /* Something messed up */
                if (!NT_SUCCESS(Status))
                    break;
            }
        }

EndLoop:
        /* Free the allocated memory */
        for (ControllerLoop = 0; ControllerLoop < RTL_NUMBER_OF(IoDeviceInfoNames); ControllerLoop++)
        {
            if (ControllerInformation[ControllerLoop])
            {
                ExFreePoolWithTag(ControllerInformation[ControllerLoop], TAG_IO_RESOURCE);
                ControllerInformation[ControllerLoop] = NULL;
            }
        }

        /* Something messed up */
        if (!NT_SUCCESS(Status))
            break;
    }

    return Status;
}

/**
 * @brief
 * Reads and returns Hardware information from the appropriate hardware
 * registry key. Helper stub of IoQueryDeviceDescription(). Has two modes
 * of operation, either looking for root bus types or for sub-bus
 * information.
 *
 * @param[in]   Query
 * What the parent function wants.
 *
 * @param[in]   RootKey
 * Which key to look in.
 *
 * @param[in]   RootKeyHandle
 * Handle to the key.
 *
 * @param[in,out]   Bus
 * Pointer to the current bus number.
 *
 * @param[in]   KeyIsRoot
 * Whether we are looking for root bus types or information under them.
 *
 * @return  A status code.
 **/
static NTSTATUS
IopQueryBusDescription(
    _In_ PIO_QUERY Query,
    _In_ UNICODE_STRING RootKey,
    _In_ HANDLE RootKeyHandle,
    _Inout_ PULONG Bus,
    _In_ BOOLEAN KeyIsRoot)
{
    NTSTATUS Status;
    ULONG BusLoop;
    UNICODE_STRING SubRootRegName;
    UNICODE_STRING BusString;
    UNICODE_STRING SubBusString;
    ULONG LenBasicInformation = 0;
    ULONG LenFullInformation;
    ULONG LenKeyFullInformation;
    ULONG LenKey;
    HANDLE SubRootKeyHandle;
    PKEY_FULL_INFORMATION FullInformation;
    PKEY_BASIC_INFORMATION BasicInformation = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_FULL_INFORMATION BusInformation[IoQueryDeviceMaxData] =
        {NULL, NULL, NULL};

    IORSRCTRACE("\nIopQueryBusDescription(Query: 0x%p)\n"
                "    RootKey: '%wZ'\n"
                "    RootKeyHandle: 0x%p\n"
                "    KeyIsRoot: %s\n"
                "    Bus: 0x%p (%lu)\n",
                Query,
                &RootKey, RootKeyHandle,
                KeyIsRoot ? "TRUE" : "FALSE",
                Bus, Bus ? *Bus : -1);

    /* Retrieve the necessary buffer space */
    Status = ZwQueryKey(RootKeyHandle,
                        KeyFullInformation,
                        NULL, 0,
                        &LenFullInformation);

    if (!NT_SUCCESS(Status) &&
        (Status != STATUS_BUFFER_TOO_SMALL) &&
        (Status != STATUS_BUFFER_OVERFLOW))
    {
        return Status;
    }

    /* Allocate it */
    FullInformation = ExAllocatePoolWithTag(PagedPool, LenFullInformation, TAG_IO_RESOURCE);

    if (!FullInformation)
        return STATUS_NO_MEMORY;

    /* Get the information */
    Status = ZwQueryKey(RootKeyHandle,
                        KeyFullInformation,
                        FullInformation,
                        LenFullInformation,
                        &LenFullInformation);
    if (NT_SUCCESS(Status))
    {
        /* Buffer needed for all the keys under this one */
        LenBasicInformation = FullInformation->MaxNameLen + sizeof(KEY_BASIC_INFORMATION);

        /* Allocate it */
        BasicInformation = ExAllocatePoolWithTag(PagedPool, LenBasicInformation, TAG_IO_RESOURCE);
    }

    /* Deallocate the old buffer */
    ExFreePoolWithTag(FullInformation, TAG_IO_RESOURCE);

    /* Try to find a bus */
    for (BusLoop = 0; NT_SUCCESS(Status); BusLoop++)
    {
        /* Bus parameter was passed and number was matched */
        if (Query->BusNumber && (*(Query->BusNumber) == *Bus))
            break;

        /* Enumerate the Key */
        Status = ZwEnumerateKey(RootKeyHandle,
                                BusLoop,
                                KeyBasicInformation,
                                BasicInformation,
                                LenBasicInformation,
                                &LenKey);

        /* Stop if everything was enumerated */
        if (!NT_SUCCESS(Status))
            break;

        IORSRCTRACE("    Seen: '%.*ws'\n", BasicInformation->NameLength/sizeof(WCHAR), BasicInformation->Name);

        /* What bus are we going to go down? (only check if this is a root key) */
        if (KeyIsRoot)
        {
            if (wcsncmp(BasicInformation->Name, L"MultifunctionAdapter", BasicInformation->NameLength / sizeof(WCHAR)) &&
                wcsncmp(BasicInformation->Name, L"EisaAdapter", BasicInformation->NameLength / sizeof(WCHAR)) &&
                wcsncmp(BasicInformation->Name, L"TcAdapter", BasicInformation->NameLength / sizeof(WCHAR)))
            {
                /* Nothing found, check next */
                continue;
            }
        }

        /* Enumerate the bus */
        BusString.Buffer = BasicInformation->Name;
        BusString.Length = (USHORT)BasicInformation->NameLength;
        BusString.MaximumLength = (USHORT)BasicInformation->NameLength;

        /* Open a handle to the root registry key */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &BusString,
                                   OBJ_CASE_INSENSITIVE,
                                   RootKeyHandle,
                                   NULL);

        Status = ZwOpenKey(&SubRootKeyHandle, KEY_READ, &ObjectAttributes);

        /* Go on if we failed */
        if (!NT_SUCCESS(Status))
            continue;

        /* Key opened, create the path */
        SubRootRegName = RootKey;
        RtlAppendUnicodeToString(&SubRootRegName, L"\\");
        RtlAppendUnicodeStringToString(&SubRootRegName, &BusString);

        IORSRCTRACE("    SubRootRegName: '%wZ'\n", &SubRootRegName);

        if (!KeyIsRoot)
        {
            /* Parsing a sub-bus key */
            ULONG SubBusLoop;
            for (SubBusLoop = 0; SubBusLoop < RTL_NUMBER_OF(IoDeviceInfoNames); SubBusLoop++)
            {
                /* Identifier string first */
                RtlInitUnicodeString(&SubBusString, IoDeviceInfoNames[SubBusLoop]);

                IORSRCTRACE("    Getting bus value: '%wZ'\n", &SubBusString);

                /* Retrieve the necessary buffer space */
                ZwQueryValueKey(SubRootKeyHandle,
                                &SubBusString,
                                KeyValueFullInformation,
                                NULL, 0,
                                &LenKeyFullInformation);

                /* Allocate it */
                BusInformation[SubBusLoop] = ExAllocatePoolWithTag(PagedPool, LenKeyFullInformation, TAG_IO_RESOURCE);

                /* Get the information */
                Status = ZwQueryValueKey(SubRootKeyHandle,
                                         &SubBusString,
                                         KeyValueFullInformation,
                                         BusInformation[SubBusLoop],
                                         LenKeyFullInformation,
                                         &LenKeyFullInformation);
            }

            if (NT_SUCCESS(Status))
            {
                PKEY_VALUE_FULL_INFORMATION BusConfigData =
                    BusInformation[IoQueryDeviceConfigurationData];

                /* Do we have something? */
                if (BusConfigData != NULL &&
                    BusConfigData->DataLength != 0 &&
                    /* Does it match what we want? */
                    (((PCM_FULL_RESOURCE_DESCRIPTOR)((ULONG_PTR)BusConfigData +
                        BusConfigData->DataOffset))->InterfaceType == *(Query->BusType)))
                {
                    /* Found a bus */
                    (*Bus)++;

                    /* Is it the bus we wanted? */
                    if (Query->BusNumber == NULL || *(Query->BusNumber) == *Bus)
                    {
                        if (Query->ControllerType == NULL)
                        {
                            IORSRCTRACE("    --> Bus #%lu Callout: '%wZ'\n", *Bus, &SubRootRegName);

                            /* We don't want controller information: call the callback */
                            Status = Query->CalloutRoutine(Query->Context,
                                                           &SubRootRegName,
                                                           *(Query->BusType),
                                                           *Bus,
                                                           BusInformation,
                                                           0,
                                                           0,
                                                           NULL,
                                                           0,
                                                           0,
                                                           NULL);
                        }
                        else
                        {
                            IORSRCTRACE("    --> Getting device on Bus #%lu : '%wZ'\n", *Bus, &SubRootRegName);

                            /* We want controller information: get it */
                            Status = IopQueryDeviceDescription(Query,
                                                               SubRootRegName,
                                                               RootKeyHandle,
                                                               *Bus,
                                                               (PKEY_VALUE_FULL_INFORMATION*)BusInformation);
                        }
                    }
                }
            }

            /* Free the allocated memory */
            for (SubBusLoop = 0; SubBusLoop < RTL_NUMBER_OF(IoDeviceInfoNames); SubBusLoop++)
            {
                if (BusInformation[SubBusLoop])
                {
                    ExFreePoolWithTag(BusInformation[SubBusLoop], TAG_IO_RESOURCE);
                    BusInformation[SubBusLoop] = NULL;
                }
            }

            /* Exit the loop if we found the bus */
            if (Query->BusNumber && (*(Query->BusNumber) == *Bus))
            {
                ZwClose(SubRootKeyHandle);
                SubRootKeyHandle = NULL;
                continue;
            }
        }

        /* Enumerate the buses below us recursively if we haven't found the bus yet */
        Status = IopQueryBusDescription(Query, SubRootRegName, SubRootKeyHandle, Bus, !KeyIsRoot);

        /* Everything enumerated */
        if (Status == STATUS_NO_MORE_ENTRIES) Status = STATUS_SUCCESS;

        ZwClose(SubRootKeyHandle);
        SubRootKeyHandle = NULL;
    }

    /* Free the last remaining allocated memory */
    if (BasicInformation)
        ExFreePoolWithTag(BasicInformation, TAG_IO_RESOURCE);

    return Status;
}

NTSTATUS
IopFetchConfigurationInformation(
    _Out_ PWSTR* SymbolicLinkList,
    _In_ GUID Guid,
    _In_ ULONG ExpectedInterfaces,
    _Out_ PULONG Interfaces)
{
    NTSTATUS Status;
    ULONG interfaces = 0;
    PWSTR symbolicLinkList;

    /* Get the associated enabled interfaces with the given GUID */
    Status = IoGetDeviceInterfaces(&Guid, NULL, 0, SymbolicLinkList);
    if (!NT_SUCCESS(Status))
    {
        /* Zero output and leave */
        if (SymbolicLinkList)
            *SymbolicLinkList = NULL;

        return STATUS_UNSUCCESSFUL;
    }

    symbolicLinkList = *SymbolicLinkList;

    /* Count the number of enabled interfaces by counting the number of symbolic links */
    while (*symbolicLinkList != UNICODE_NULL)
    {
        interfaces++;
        symbolicLinkList += (wcslen(symbolicLinkList) + 1);
    }

    /* Matching result will define the result */
    Status = (interfaces >= ExpectedInterfaces) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    /* Finally, give back to the caller the number of found interfaces */
    *Interfaces = interfaces;

    return Status;
}

VOID
IopStoreSystemPartitionInformation(
    _In_ PUNICODE_STRING NtSystemPartitionDeviceName,
    _In_ PUNICODE_STRING OsLoaderPathName)
{
    NTSTATUS Status;
    UNICODE_STRING LinkTarget, KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE LinkHandle, RegistryHandle, KeyHandle;
    WCHAR LinkTargetBuffer[256];
    UNICODE_STRING CmRegistryMachineSystemName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM");

    ASSERT(NtSystemPartitionDeviceName->MaximumLength >= NtSystemPartitionDeviceName->Length + sizeof(WCHAR));
    ASSERT(NtSystemPartitionDeviceName->Buffer[NtSystemPartitionDeviceName->Length / sizeof(WCHAR)] == UNICODE_NULL);
    ASSERT(OsLoaderPathName->MaximumLength >= OsLoaderPathName->Length + sizeof(WCHAR));
    ASSERT(OsLoaderPathName->Buffer[OsLoaderPathName->Length / sizeof(WCHAR)] == UNICODE_NULL);

    /* First define needed stuff to open NtSystemPartitionDeviceName symbolic link */
    InitializeObjectAttributes(&ObjectAttributes,
                               NtSystemPartitionDeviceName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Open NtSystemPartitionDeviceName symbolic link */
    Status = ZwOpenSymbolicLinkObject(&LinkHandle,
                                      SYMBOLIC_LINK_QUERY,
                                      &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to open symlink %wZ, Status=%lx\n", NtSystemPartitionDeviceName, Status);
        return;
    }

    /* Prepare the string that will receive where symbolic link points to */
    LinkTarget.Length = 0;
    /* We will zero the end of the string after having received it */
    LinkTarget.MaximumLength = sizeof(LinkTargetBuffer) - sizeof(UNICODE_NULL);
    LinkTarget.Buffer = LinkTargetBuffer;

    /* Query target */
    Status = ZwQuerySymbolicLinkObject(LinkHandle, &LinkTarget, NULL);

    /* We are done with symbolic link */
    ObCloseHandle(LinkHandle, KernelMode);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed querying symlink %wZ, Status=%lx\n", NtSystemPartitionDeviceName, Status);
        return;
    }

    /* As promised, we zero the end */
    LinkTarget.Buffer[LinkTarget.Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Open registry to save data (HKLM\SYSTEM) */
    Status = IopOpenRegistryKeyEx(&RegistryHandle,
                                  NULL,
                                  &CmRegistryMachineSystemName,
                                  KEY_ALL_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to open HKLM\\SYSTEM, Status=%lx\n", Status);
        return;
    }

    /* Open or create the Setup subkey where we'll store in */
    RtlInitUnicodeString(&KeyName, L"Setup");

    Status = IopCreateRegistryKeyEx(&KeyHandle,
                                    RegistryHandle,
                                    &KeyName,
                                    KEY_ALL_ACCESS,
                                    REG_OPTION_NON_VOLATILE,
                                    NULL);

    /* We're done with HKLM\SYSTEM */
    ObCloseHandle(RegistryHandle, KernelMode);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed opening/creating Setup key, Status=%lx\n", Status);
        return;
    }

    /* Prepare first data writing */
    RtlInitUnicodeString(&KeyName, L"SystemPartition");

    /* Write SystemPartition value which is the target of the symbolic link */
    Status = ZwSetValueKey(KeyHandle,
                           &KeyName,
                           0,
                           REG_SZ,
                           LinkTarget.Buffer,
                           LinkTarget.Length + sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed writing SystemPartition value, Status=%lx\n", Status);
    }

    /* Prepare for second data writing */
    RtlInitUnicodeString(&KeyName, L"OsLoaderPath");

    /* Remove trailing slash if any (one slash only excepted) */
    if (OsLoaderPathName->Length > sizeof(WCHAR) &&
        OsLoaderPathName->Buffer[(OsLoaderPathName->Length / sizeof(WCHAR)) - 1] == OBJ_NAME_PATH_SEPARATOR)
    {
        OsLoaderPathName->Length -= sizeof(WCHAR);
        OsLoaderPathName->Buffer[OsLoaderPathName->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }

    /* Then, write down data */
    Status = ZwSetValueKey(KeyHandle,
                           &KeyName,
                           0,
                           REG_SZ,
                           OsLoaderPathName->Buffer,
                           OsLoaderPathName->Length + sizeof(UNICODE_NULL));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed writing OsLoaderPath value, Status=%lx\n", Status);
    }

    /* We're finally done! */
    ObCloseHandle(KeyHandle, KernelMode);
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Returns a pointer to the I/O manager's global configuration
 * information structure.
 *
 * This structure contains the current values for how many physical storage
 * media, SCSI HBA, serial, and parallel devices have device objects created
 * to represent them by drivers as they are loaded.
 **/
PCONFIGURATION_INFORMATION
NTAPI
IoGetConfigurationInformation(VOID)
{
    return &_SystemConfigurationInformation;
}

/**
 * @halfplemented
 *
 * @brief
 * Reports hardware resources in the \Registry\Machine\Hardware\ResourceMap
 * tree, so that a subsequently loaded driver cannot attempt to use the
 * same resources.
 *
 * @param[in]   DriverClassName
 * The driver class under which the resource information should be stored.
 *
 * @param[in]   DriverObject
 * The driver object that was provided to the DriverEntry routine.
 *
 * @param[in]   DriverList
 * Resources claimed for the all the driver's devices, rather than per-device.
 *
 * @param[in]   DriverListSize
 * Size in bytes of the DriverList.
 *
 * @param[in]   DeviceObject
 * The device object for which resources should be claimed.
 *
 * @param[in]   DeviceList
 * List of resources that should be claimed for the device.
 *
 * @param[in]   DeviceListSize
 * Size of the per-device resource list in bytes.
 *
 * @param[in]   OverrideConflict
 * TRUE if the resources should be claimed even if a conflict is found.
 *
 * @param[out]  ConflictDetected
 * Points to a variable that receives TRUE if a conflict is detected
 * with another driver.
 **/
NTSTATUS
NTAPI
IoReportResourceUsage(
    _In_opt_ PUNICODE_STRING DriverClassName,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_reads_bytes_opt_(DriverListSize) PCM_RESOURCE_LIST DriverList,
    _In_opt_ ULONG DriverListSize,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_reads_bytes_opt_(DeviceListSize) PCM_RESOURCE_LIST DeviceList,
    _In_opt_ ULONG DeviceListSize,
    _In_ BOOLEAN OverrideConflict,
    _Out_ PBOOLEAN ConflictDetected)
{
    NTSTATUS Status;
    PCM_RESOURCE_LIST ResourceList;

    DPRINT1("IoReportResourceUsage is halfplemented!\n");

    if (!DriverList && !DeviceList)
        return STATUS_INVALID_PARAMETER;

    if (DeviceList)
        ResourceList = DeviceList;
    else
        ResourceList = DriverList;

    Status = IopDetectResourceConflict(ResourceList, FALSE, NULL);
    if (Status == STATUS_CONFLICTING_ADDRESSES)
    {
        *ConflictDetected = TRUE;

        if (!OverrideConflict)
        {
            DPRINT1("Denying an attempt to claim resources currently in use by another device!\n");
            return STATUS_CONFLICTING_ADDRESSES;
        }
        else
        {
            DPRINT1("Proceeding with conflicting resources\n");
        }
    }
    else if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* TODO: Claim resources in registry */

    *ConflictDetected = FALSE;

    return STATUS_SUCCESS;
}

static NTSTATUS
IopLegacyResourceAllocation(
    _In_ ARBITER_REQUEST_SOURCE AllocationType,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements,
    _Inout_ PCM_RESOURCE_LIST* AllocatedResources)
{
    NTSTATUS Status;

    DPRINT1("IopLegacyResourceAllocation is halfplemented!\n");

    if (!ResourceRequirements)
    {
        /* We can get there by calling IoAssignResources() with RequestedResources = NULL.
         * TODO: not sure what we should do, but we shouldn't crash.
         */
        UNIMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    Status = IopFixupResourceListWithRequirements(ResourceRequirements,
                                                  AllocatedResources);
    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_CONFLICTING_ADDRESSES)
        {
            DPRINT1("Denying an attempt to claim resources currently in use by another device!\n");
        }

        return Status;
    }

    /* TODO: Claim resources in registry */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoAssignResources(
    _In_ PUNICODE_STRING RegistryPath,
    _In_opt_ PUNICODE_STRING DriverClassName,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PIO_RESOURCE_REQUIREMENTS_LIST RequestedResources,
    _Inout_ PCM_RESOURCE_LIST* AllocatedResources)
{
    PDEVICE_NODE DeviceNode;

    UNREFERENCED_PARAMETER(RegistryPath);
    UNREFERENCED_PARAMETER(DriverClassName);

    /* Do we have a DO? */
    if (DeviceObject)
    {
        /* Get its device node */
        DeviceNode = IopGetDeviceNode(DeviceObject);
        if (DeviceNode && !(DeviceNode->Flags & DNF_LEGACY_RESOURCE_DEVICENODE))
        {
            /* New drivers should not call this API */
            KeBugCheckEx(PNP_DETECTED_FATAL_ERROR,
                         0,
                         0,
                         (ULONG_PTR)DeviceObject,
                         (ULONG_PTR)DriverObject);
        }
    }

    /* Did the driver supply resources? */
    if (RequestedResources)
    {
        /* Make sure there's actually something useful in them */
        if (!(RequestedResources->AlternativeLists) || !(RequestedResources->List[0].Count))
        {
            /* Empty resources are no resources */
            RequestedResources = NULL;
        }
    }

    /* Initialize output if given */
    if (AllocatedResources)
        *AllocatedResources = NULL;

    /* Call internal helper function */
    return IopLegacyResourceAllocation(ArbiterRequestLegacyAssigned,
                                       DriverObject,
                                       DeviceObject,
                                       RequestedResources,
                                       AllocatedResources);
}

/**
 * @brief
 * Reads and returns Hardware information from the appropriate
 * hardware registry key.
 *
 * @param[in]   BusType
 * Specifies the bus type, for example: MCA, ISA, EISA, etc.
 *
 * @param[in]   BusNumber
 * The number of the specified bus type to query.
 *
 * @param[in]   ControllerType
 * Specifies the controller type
 *
 * @param[in]   ControllerNumber
 * The number of the specified controller type to query.
 *
 * @param[in]   CalloutRoutine
 * A user-provided callback function to call for each valid query.
 *
 * @param[in]   Context
 * A callback-specific context value.
 *
 * @return  A status code.
 **/
NTSTATUS
NTAPI
IoQueryDeviceDescription(
    _In_opt_ PINTERFACE_TYPE BusType,
    _In_opt_ PULONG BusNumber,
    _In_opt_ PCONFIGURATION_TYPE ControllerType,
    _In_opt_ PULONG ControllerNumber,
    _In_opt_ PCONFIGURATION_TYPE PeripheralType,
    _In_opt_ PULONG PeripheralNumber,
    _In_ PIO_QUERY_DEVICE_ROUTINE CalloutRoutine,
    _In_opt_ PVOID Context)
{
    NTSTATUS Status;
    ULONG BusLoopNumber = -1; /* Root Bus */
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING RootRegKey;
    HANDLE RootRegHandle;
    IO_QUERY Query;

    IORSRCTRACE("\nIoQueryDeviceDescription()\n"
                "    BusType:          0x%p (%lu)\n"
                "    BusNumber:        0x%p (%lu)\n"
                "    ControllerType:   0x%p (%lu)\n"
                "    ControllerNumber: 0x%p (%lu)\n"
                "    PeripheralType:   0x%p (%lu)\n"
                "    PeripheralNumber: 0x%p (%lu)\n"
                "    CalloutRoutine:   0x%p\n"
                "    Context:          0x%p\n"
                "--> Query: 0x%p\n",
                BusType, BusType ? *BusType : -1,
                BusNumber, BusNumber ? *BusNumber : -1,
                ControllerType, ControllerType ? *ControllerType : -1,
                ControllerNumber, ControllerNumber ? *ControllerNumber : -1,
                PeripheralType, PeripheralType ? *PeripheralType : -1,
                PeripheralNumber, PeripheralNumber ? *PeripheralNumber : -1,
                CalloutRoutine, Context,
                &Query);

    /* Set up the string */
    RootRegKey.Length = 0;
    RootRegKey.MaximumLength = 2048;
    RootRegKey.Buffer = ExAllocatePoolWithTag(PagedPool, RootRegKey.MaximumLength, TAG_IO_RESOURCE);
    RtlAppendUnicodeToString(&RootRegKey, L"\\REGISTRY\\MACHINE\\HARDWARE\\DESCRIPTION\\SYSTEM");

    /* Open a handle to the Root Registry Key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &RootRegKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&RootRegHandle, KEY_READ, &ObjectAttributes);

    if (NT_SUCCESS(Status))
    {
        /* Use a helper function to loop though this key and get the information */
        Query.BusType = BusType;
        Query.BusNumber = BusNumber;
        Query.ControllerType = ControllerType;
        Query.ControllerNumber = ControllerNumber;
        Query.PeripheralType = PeripheralType;
        Query.PeripheralNumber = PeripheralNumber;
        Query.CalloutRoutine = CalloutRoutine;
        Query.Context = Context;
        Status = IopQueryBusDescription(&Query,
                                        RootRegKey,
                                        RootRegHandle,
                                        &BusLoopNumber,
                                        TRUE);

        /* Close registry key */
        ZwClose(RootRegHandle);
    }

    /* Cleanup */
    ExFreePoolWithTag(RootRegKey.Buffer, TAG_IO_RESOURCE);

    return Status;
}

/**
 * @brief
 * Reports hardware resources of the HAL in the
 * \Registry\Machine\Hardware\ResourceMap tree.
 *
 * @param[in]   HalName
 * Descriptive name of the HAL.
 *
 * @param[in]   RawResourceList
 * List of raw (bus specific) resources which should be claimed
 * for the HAL.
 *
 * @param[in]   TranslatedResourceList
 * List of translated (system wide) resources which should be claimed
 * for the HAL.
 *
 * @param[in]   ResourceListSize
 * Size in bytes of the raw and translated resource lists.
 * Both lists have the same size.
 *
 * @return  A status code.
 **/
NTSTATUS
NTAPI
IoReportHalResourceUsage(
    _In_ PUNICODE_STRING HalName,
    _In_ PCM_RESOURCE_LIST RawResourceList,
    _In_ PCM_RESOURCE_LIST TranslatedResourceList,
    _In_ ULONG ResourceListSize)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    ULONG Disposition;
    HANDLE ResourceMapKey;
    HANDLE HalKey;
    HANDLE DescriptionKey;

    /* Open/Create 'RESOURCEMAP' key */
    RtlInitUnicodeString(&Name, L"\\Registry\\Machine\\HARDWARE\\RESOURCEMAP");
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                               0,
                               NULL);
    Status = ZwCreateKey(&ResourceMapKey,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         &Disposition);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Open/Create 'Hardware Abstraction Layer' key */
    RtlInitUnicodeString(&Name, L"Hardware Abstraction Layer");
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                               ResourceMapKey,
                               NULL);
    Status = ZwCreateKey(&HalKey,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         &Disposition);
    ZwClose(ResourceMapKey);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Create 'HalName' key */
    InitializeObjectAttributes(&ObjectAttributes,
                               HalName,
                               OBJ_CASE_INSENSITIVE,
                               HalKey,
                               NULL);
    Status = ZwCreateKey(&DescriptionKey,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         &Disposition);
    ZwClose(HalKey);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Add '.Raw' value */
    RtlInitUnicodeString(&Name, L".Raw");
    Status = ZwSetValueKey(DescriptionKey,
                           &Name,
                           0,
                           REG_RESOURCE_LIST,
                           RawResourceList,
                           ResourceListSize);
    if (!NT_SUCCESS(Status))
    {
        ZwClose(DescriptionKey);
        return Status;
    }

    /* Add '.Translated' value */
    RtlInitUnicodeString(&Name, L".Translated");
    Status = ZwSetValueKey(DescriptionKey,
                           &Name,
                           0,
                           REG_RESOURCE_LIST,
                           TranslatedResourceList,
                           ResourceListSize);
    ZwClose(DescriptionKey);

    return Status;
}

/* EOF */
