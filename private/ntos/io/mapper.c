
/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    registry.c

Abstract:

    This module contains the code that manipulates the ARC firmware
    tree and other elements in the registry.

Author:

    Bob Rinne (BobRi) 15-Oct-1994

Environment:

    Kernel mode

Revision History :

--*/

#include "iop.h"
#pragma hdrstop

#if UMODETEST
#undef IsNEC_98
#define IsNEC_98 0
#endif

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'rpaM')
#endif


//
// This contains information obtained by checking the firmware
// tree of the registry
//

typedef struct _FIRMWARE_CONFIGURATION {
    struct _FIRMWARE_CONFIGURATION *Next;
    INTERFACE_TYPE     BusType;
    ULONG              BusNumber;
    CONFIGURATION_TYPE ControllerType;
    ULONG              ControllerNumber;
    CONFIGURATION_TYPE PeripheralType;
    ULONG              PeripheralNumber;
    ULONG              NumberBases;
    ULONG              ResourceDescriptorSize;
    PVOID              ResourceDescriptor;
    ULONG              IdentifierLength;
    ULONG              IdentifierType;
    PVOID              Identifier;
    PWCHAR             PnPId;
    BOOLEAN            NewlyCreated;
} FIRMWARE_CONFIGURATION, *PFIRMWARE_CONFIGURATION;

//
// Device extension information
//

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT     DeviceObject;
    PDRIVER_OBJECT     DriverObject;
    INTERFACE_TYPE     InterfaceType;
    ULONG              BusNumber;
    PFIRMWARE_CONFIGURATION FirmwareList;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;
//
// mapping table from firmware to enum
//

typedef struct _FIRMWARE_IDENT_TO_PNP_ID {
    PWCHAR  FirmwareName;
    PWCHAR  PnPId;
} FIRMWARE_IDENT_TO_PNP_ID, *PFIRMWARE_IDENT_TO_PNP_ID;

//
// table to hold seed information for a firmware tree entry.
//

#define OPTIONS_NONE                    0x00000000
#define OPTIONS_INSERT_PNP_ID           0x00000001
#define OPTIONS_INSERT_DEVICEDESC       0x00000002
#define OPTIONS_INSERT_COMPATIBLE_IDS   0x00000004
#define OPTIONS_INSERT_PHANTOM_MARKER   0x00000008
typedef struct _MAPPER_SEED {
    PWCHAR  ValueName;
    ULONG   ValueType;
    ULONG   DwordValueContent;
    ULONG   Options;
} MAPPER_SEED, *PMAPPER_SEED;

//
// table to hold key names and attributes for construction
// in the root enumerator tree
//

#define KEY_SEED_REQUIRED               0x00000000
#define KEY_SEED_DEVICE_PARAMETERS      0x00000001
typedef struct _KEY_SEED {
    PWCHAR  KeyName;
    ULONG   Attribute;
    ULONG   Options;
} KEY_SEED, *PKEY_SEED;


//
// All the data here is INIT only
//

//#ifdef ALLOC_DATA_PRAGMA
//#pragma data_seg("INIT")
//#endif

DEVICE_EXTENSION MapperDeviceExtension;

//
// This table is used to translate the firmware tree information
// to the root enumerator PNP id for keyboard devices.
//

FIRMWARE_IDENT_TO_PNP_ID KeyboardMap[] = {
    L"XT_83KEY",        L"*PNP0300",
    L"PCAT_86KEY",      L"*PNP0301",
    L"PCXT_84KEY",      L"*PNP0302",
    L"XT_84KEY",        L"*PNP0302",
    L"101-KEY",         L"*PNP0303",
    L"OLI_83KEY",       L"*PNP0304",
    L"ATT_301",         L"*PNP0304",
    L"OLI_102KEY",      L"*PNP0305",
    L"OLI_86KEY",       L"*PNP0306",
    L"OLI_A101_102KEY", L"*PNP0309",
    L"ATT_302",         L"*PNP030a",
    L"PCAT_ENHANCED",   L"*PNP030b",
    L"PC98_106KEY",     L"*nEC1300",
    L"PC98_LaptopKEY",  L"*nEC1300",
    L"PC98_N106KEY",    L"*PNP0303",
    NULL, NULL
};

#define PS2_KEYBOARD_COMPATIBLE_ID  L"PS2_KEYBOARD"
#define PS2_MOUSE_COMPATIBLE_ID     L"PS2_MOUSE"

//
// This table is used to translate the firmware tree information
// to the root enumerator PNP id for pointer devices.
//

FIRMWARE_IDENT_TO_PNP_ID PointerMap[] = {
    L"PS2 MOUSE",                        L"*PNP0F0E",
    L"SERIAL MOUSE",                     L"*PNP0F0C",
    L"MICROSOFT PS2 MOUSE",              L"*PNP0F03",
    L"LOGITECH PS2 MOUSE",               L"*PNP0F12",
    L"MICROSOFT INPORT MOUSE",           L"*PNP0F02",
    L"MICROSOFT SERIAL MOUSE",           L"*PNP0F01",
    L"MICROSOFT BALLPOINT SERIAL MOUSE", L"*PNP0F09",
    L"LOGITECH SERIAL MOUSE",            L"*PNP0F08",
    L"MICROSOFT BUS MOUSE",              L"*PNP0F00",
    L"NEC PC-9800 BUS MOUSE",            L"*nEC1F00",
    NULL, NULL
};

//
// the MapperValueSeed table is a NULL terminated table (i.e. the name
// pointer is NULL) that contains the list of values and their type
// for insertion in a newly created root enumerator key.
//

MAPPER_SEED MapperValueSeed[] = {
    REGSTR_VAL_HARDWAREID,         REG_MULTI_SZ, 0, OPTIONS_INSERT_PNP_ID,
    REGSTR_VAL_COMPATIBLEIDS,      REG_MULTI_SZ, 0, OPTIONS_INSERT_COMPATIBLE_IDS,
    REGSTR_VAL_FIRMWAREIDENTIFIED, REG_DWORD,    1, OPTIONS_NONE,
    REGSTR_VAL_DEVDESC,            REG_SZ,       0, OPTIONS_INSERT_DEVICEDESC,
    REGSTR_VAL_PHANTOM,            REG_DWORD,    1, OPTIONS_INSERT_PHANTOM_MARKER,
    NULL, 0, 0, 0
};

//
// the MapperKeySeed table is a NULL terminated table (i.e. the name
// pointer is NULL) that contains the list of keys to and their
// attributes (volatile or non-volatile) for keys to be created under
// a newly created root enumerator key.
//
// The preceeding backslash is required on all entries in this table.
//

KEY_SEED MapperKeySeed[] = {
    L"\\Control",           REG_OPTION_VOLATILE,     KEY_SEED_REQUIRED,
    L"\\LogConf",           REG_OPTION_NON_VOLATILE, KEY_SEED_REQUIRED,
    L"",                    REG_OPTION_NON_VOLATILE, KEY_SEED_DEVICE_PARAMETERS,
    NULL, 0, 0
};

//
// SerialId is used as the PNP id for all serial controllers.
// NOTE: there is no code to detect presense of a 16550.
//

PWSTR SerialId = L"*PNP0501";   // RDR should be two entries.  *PNP0501 is 16550

PWSTR SerialIdNEC[] = {         // NEC98 need some DeviceId for Serial Controller.
    L"*nEC1500",
    L"*nEC1501",
    L"*nEC1502",
    L"*nEC1503",
    L"*nEC8071",
    L"*nEC0C01",
     NULL
};

//
// ParallelId is used as the PNP id for all parallel controllers.
// NOTE: there is no code to detect presense of ECP support.
//

PWSTR ParallelId = L"*PNP0400"; // RDR should be two entries.  *PNP0401 is ECP

PWSTR ParallelIdNEC = L"*nEC1401";  // This DeviceId has NEC98 only.

//
// FloppyId is used as the PNP id for all floppy peripherals.
//

PWSTR FloppyId = L"*PNP0700";

//
// ATAId is here, but not used - there is nothing in the firmware
// tree for the IDE controller.
//

PWSTR ATAId = L"*PNP0600";

//
// Debugging stuff
//

#if DBG

#define MAPPER_ERROR       0x00000001
#define MAPPER_INFORMATION 0x00000002
#define MAPPER_PNP_ID      0x00000004
#define MAPPER_RESOURCES   0x00000008
#define MAPPER_REGISTRY    0x00000010
#define MAPPER_VERBOSE     0x00008000

ULONG MapperDebugMask =    MAPPER_ERROR       |
                           // MAPPER_INFORMATION |
                           // MAPPER_REGISTRY    |
                           // MAPPER_PNP_ID      |
                           // MAPPER_RESOURCES   |
                           // MAPPER_VERBOSE |
                           0;

#define DebugPrint(X) MapperDebugPrint X

VOID
MapperDebugPrint(
    ULONG  DebugMask,
    PCCHAR DebugMessage,
    ...
    );

#else

#define DebugPrint(X)

#endif // DBG

//
// Proto type declarations
//

PFIRMWARE_IDENT_TO_PNP_ID
MapperFindIdentMatch(
    PFIRMWARE_IDENT_TO_PNP_ID IdentTable,
    PWCHAR String
    );

PWSTR
MapperTranslatePnPId(
    CONFIGURATION_TYPE PeripheralType,
    PKEY_VALUE_FULL_INFORMATION Identifier
    );

NTSTATUS
MapperPeripheralCallback(
    IN PVOID                        Context,
    IN PUNICODE_STRING              PathName,
    IN INTERFACE_TYPE               BusType,
    IN ULONG                        BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE           ControllerType,
    IN ULONG                        ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE           PeripheralType,
    IN ULONG                        PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    );

NTSTATUS
MapperCallback(
    IN PVOID                        Context,
    IN PUNICODE_STRING              PathName,
    IN INTERFACE_TYPE               BusType,
    IN ULONG                        BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE           ControllerType,
    IN ULONG                        ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE           PeripheralType,
    IN ULONG                        PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    );

VOID
MapperMarkKey(
    IN HANDLE Handle,
    IN PUNICODE_STRING  PathName,
    IN PFIRMWARE_CONFIGURATION FirmwareEntry
    );

VOID
MapperSeedKey(
    IN HANDLE                  Handle,
    IN PUNICODE_STRING         PathName,
    IN PFIRMWARE_CONFIGURATION FirmwareEntry,
    IN BOOLEAN                 DeviceIsPhantom
    );

PCM_RESOURCE_LIST
MapperAdjustResourceList (
    IN     PCM_RESOURCE_LIST ResourceList,
    IN     PWCHAR            PnPId,
    IN OUT PULONG            Size
    );

NTSTATUS
ComPortDBAdd(
    IN  HANDLE  DeviceParamKey,
    IN  PWSTR   PortName
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, MapperFindIdentMatch)
#pragma alloc_text(INIT, MapperTranslatePnPId)
#pragma alloc_text(INIT, MapperPeripheralCallback)
#pragma alloc_text(INIT, MapperCallback)
#pragma alloc_text(INIT, MapperMarkKey)
#pragma alloc_text(INIT, MapperSeedKey)
#pragma alloc_text(INIT, MapperFreeList)
#pragma alloc_text(INIT, MapperAdjustResourceList)
#pragma alloc_text(INIT, ComPortDBAdd)
#pragma alloc_text(INIT, MapperPhantomizeDetectedComPorts)
#if DBG
#pragma alloc_text(INIT, MapperDebugPrint)
#endif
#endif

PFIRMWARE_IDENT_TO_PNP_ID
MapperFindIdentMatch(
    PFIRMWARE_IDENT_TO_PNP_ID IdentTable,
    PWCHAR                    String
    )

/*++

Routine Description:

    Given a table of strings to match, find the match for
    the identifier given.

Arguments:

Return Value:

    A pointer to the ident table entry for the match if found
    NULL if not found.

--*/

{
    PFIRMWARE_IDENT_TO_PNP_ID entry;

    entry = IdentTable;
    while (entry->FirmwareName) {
        if (!wcscmp(String, entry->FirmwareName)) {
            return entry;
        }
        entry++;
    }
    return NULL;
}

PWSTR
MapperTranslatePnPId(
    CONFIGURATION_TYPE          PeripheralType,
    PKEY_VALUE_FULL_INFORMATION Identifier
    )

/*++

Routine Description:

    Given the peripheral type and a location in the firmware tree
    this routine will determine the PnP Id to be used when constructing
    the root enumeration portion of the registry.

Arguments:

    PeripheralType - the type of item being translated (keyboard, mouse, etc)
    PathName       - the registry path name into the firmware tree for
                     this device.

Return Value:

    A pointer to the PnP Id string if a map is found.

--*/

{
    PFIRMWARE_IDENT_TO_PNP_ID identMap;
    PWSTR identifierString;
    PWSTR idStr;

    if (Identifier) {
        identifierString = (PWSTR)((PUCHAR)Identifier + Identifier->DataOffset);
        DebugPrint((MAPPER_PNP_ID,
                    "Mapper: identifier = %ws\n\tType = ",
                    identifierString));
    }

    idStr = NULL;
    switch (PeripheralType) {
    case DiskController:
        DebugPrint((MAPPER_PNP_ID,
                    "%s (%d)\n",
                    "DiskController",
                    PeripheralType));
        idStr = FloppyId;
        break;

    case SerialController:
        DebugPrint((MAPPER_PNP_ID,
                    "%s (%d)\n",
                    "SerialController",
                    PeripheralType));
        idStr = SerialId;
        break;

    case ParallelController:
        DebugPrint((MAPPER_PNP_ID,
                    "%s (%d)\n",
                    "ParallelController",
                    PeripheralType));
        idStr = ParallelId;
        break;

    case PointerController:
        DebugPrint((MAPPER_PNP_ID,
                    "%s (%d)\n",
                    "PointerController",
                    PeripheralType));
        idStr = PointerMap[0].PnPId;
        break;

    case KeyboardController:
        DebugPrint((MAPPER_PNP_ID,
                    "%s (%d)\n",
                    "KeyboardController",
                    PeripheralType));
        idStr = KeyboardMap[0].PnPId;
        break;

    case DiskPeripheral:
        DebugPrint((MAPPER_PNP_ID,
                    "%s (%d)\n",
                    "DiskPeripheral",
                    PeripheralType));
        break;

    case FloppyDiskPeripheral:
        DebugPrint((MAPPER_PNP_ID,
                    "%s (%d)\n",
                    "FloppyDiskPeripheral",
                    PeripheralType));
        idStr = FloppyId;
        break;

    case PointerPeripheral:
        identMap = MapperFindIdentMatch(PointerMap, identifierString);
        if (identMap) {
            DebugPrint((MAPPER_PNP_ID,
                        "%ws\n",
                        identMap->PnPId));
            idStr = identMap->PnPId;
        } else {
            DebugPrint((MAPPER_ERROR,
                        "Mapper: No pointer match found\n"));
        }
        break;

    case KeyboardPeripheral:
        identMap = MapperFindIdentMatch(KeyboardMap, identifierString);

        if (identMap) {
            DebugPrint((MAPPER_PNP_ID,
                        "%ws\n",
                        identMap->PnPId));
            idStr = identMap->PnPId;
        } else {
            DebugPrint((MAPPER_ERROR,
                        "Mapper: No keyboard match found\n"));
        }

        break;

    default:
        DebugPrint((MAPPER_ERROR,
                    "Mapper: Unknown device (%d)\n",
                    PeripheralType));
        break;
    }
    return idStr;
}


NTSTATUS
MapperPeripheralCallback(
    IN PVOID                        Context,
    IN PUNICODE_STRING              PathName,
    IN INTERFACE_TYPE               BusType,
    IN ULONG                        BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE           ControllerType,
    IN ULONG                        ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE           PeripheralType,
    IN ULONG                        PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    )

/*++

Routine Description:

    This routine is used to acquire firmware tree information about
    pointer devices in the system.

Arguments:

    Context               - Pointer to the device extension.
    PathName              - unicode registry path.
    BusType               - Internal, Isa, ...
    BusNumber             - Which bus if we are on a multibus system.
    BusInformation        - Configuration information about the bus. Not Used.
    ControllerType        - serial or ata disk.
    ControllerNumber      - Which controller if there is more than one
                            controller in the system.
    ControllerInformation - Array of pointers to the three pieces of
                            registry information.
    PeripheralType        - Undefined for this call.
    PeripheralNumber      - Undefined for this call.
    PeripheralInformation - Undefined for this call.

Return Value:

    STATUS_SUCCESS if everything went ok, or STATUS_INSUFFICIENT_RESOURCES
    if it couldn't map the base csr or acquire the device object, or
    all of the resource information couldn't be acquired.

--*/

{
    PFIRMWARE_CONFIGURATION     firmwareEntry = Context;
    PKEY_VALUE_FULL_INFORMATION information;
    ULONG                       dataLength;
    PWCHAR                      ptr;
    PVOID                       temp;

    DebugPrint((MAPPER_REGISTRY,
                "Mapper: peripheral registry location is\n %ws\n",
                PathName->Buffer));

    if (!ControllerInformation) {
        DebugPrint((MAPPER_VERBOSE,
                    "Mapper: No component information\n"));
    }
    if (!PeripheralInformation) {
        DebugPrint((MAPPER_VERBOSE,
                    "Mapper: No peripheral information\n"));
        return STATUS_SUCCESS;
    }

    //
    // Map the PnP Id for this device.
    //

    if (PeripheralInformation[IoQueryDeviceIdentifier]) {
        information = PeripheralInformation[IoQueryDeviceIdentifier];
        firmwareEntry->PnPId = MapperTranslatePnPId(PeripheralType, information);

        if (firmwareEntry->PnPId) {
            //
            // Remember the peripheral's identifier (if it has one, and it's a REG_SZ value)
            // for use as the default PnP device description.
            //

            if (((dataLength = information->DataLength) > sizeof(WCHAR)) &&
                (information->Type == REG_SZ)) {

                ptr = (PWCHAR) ((PUCHAR)information + information->DataOffset);

                if (*ptr) {
                    temp = ExAllocatePool(NonPagedPool, dataLength);
                    if (temp) {

                        //
                        // If there's already an identifier here (from the peripheral's
                        // controller) then wipe it out.
                        //

                        if(firmwareEntry->Identifier) {
                            ExFreePool(firmwareEntry->Identifier);
                        }

                        //
                        // Move the data
                        //

                        firmwareEntry->Identifier = temp;
                        firmwareEntry->IdentifierType = information->Type;
                        firmwareEntry->IdentifierLength = dataLength;
                        RtlMoveMemory(firmwareEntry->Identifier, ptr, dataLength);
                    }
                }
            }
        }
    }

    //
    // Save the ordinals for the peripheral type and number
    //

    firmwareEntry->PeripheralType = PeripheralType;
    firmwareEntry->PeripheralNumber = PeripheralNumber;

    return STATUS_SUCCESS;
}

NTSTATUS
MapperCallback(
    IN PVOID                        Context,
    IN PUNICODE_STRING              PathName,
    IN INTERFACE_TYPE               BusType,
    IN ULONG                        BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE           ControllerType,
    IN ULONG                        ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE           PeripheralType,
    IN ULONG                        PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    )

/*++

Routine Description:

    This routine is used to acquire firmware tree information about
    pointer devices in the system.

Arguments:

    Context               - Pointer to the device extension.
    PathName              - unicode registry path.
    BusType               - Internal, Isa, ...
    BusNumber             - Which bus if we are on a multibus system.
    BusInformation        - Configuration information about the bus. Not Used.
    ControllerType        - serial or ata disk.
    ControllerNumber      - Which controller if there is more than one
                            controller in the system.
    ControllerInformation - Array of pointers to the three pieces of
                            registry information.
    PeripheralType        - Undefined for this call.
    PeripheralNumber      - Undefined for this call.
    PeripheralInformation - Undefined for this call.

Return Value:

    STATUS_SUCCESS if everything went ok, or STATUS_INSUFFICIENT_RESOURCES
    if it couldn't map the base csr or acquire the device object, or
    all of the resource information couldn't be acquired.

--*/

{
    PDEVICE_EXTENSION               deviceExtension = Context;
    PCM_FULL_RESOURCE_DESCRIPTOR    controllerData;
    PKEY_VALUE_FULL_INFORMATION     information;
    PFIRMWARE_CONFIGURATION         firmwareEntry;
    CONFIGURATION_TYPE              peripheralType;
    PUCHAR                          buffer;
    PUCHAR                          identifier;
    ULONG                           dataLength;

    //
    // If entry is found, but there is no information just return
    //

    if (!(information = ControllerInformation[IoQueryDeviceConfigurationData])) {

        return STATUS_SUCCESS;
    }

    if (!(dataLength = information->DataLength)) {

        return STATUS_SUCCESS;
    }

    //
    // Setup to capture the information from the firmware tree
    //

    firmwareEntry = ExAllocatePool(NonPagedPool, sizeof(FIRMWARE_CONFIGURATION));
    if (!firmwareEntry) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(firmwareEntry, sizeof(FIRMWARE_CONFIGURATION));

    //
    // Save information concerning the controller
    //

    firmwareEntry->ControllerType   = ControllerType;
    firmwareEntry->ControllerNumber = ControllerNumber;
    firmwareEntry->BusNumber = BusNumber;
    firmwareEntry->BusType   = BusType;

    //
    // Save the resource descriptor
    //

    buffer = firmwareEntry->ResourceDescriptor = ExAllocatePool(NonPagedPool,
                                                                dataLength);

    if (!buffer) {
        ExFreePool(firmwareEntry);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Save the configuration information on this controller.
    //

    controllerData = (PCM_FULL_RESOURCE_DESCRIPTOR)
        ((PUCHAR)information + information->DataOffset);
    RtlMoveMemory(buffer, controllerData, dataLength);
    firmwareEntry->ResourceDescriptorSize = dataLength;

    //
    // If there is a device identifier save it.
    //

    if (information = ControllerInformation[IoQueryDeviceIdentifier]) {
        PWCHAR ptr;

        if (dataLength = information->DataLength) {

            ptr = (PWCHAR) ((PUCHAR)information + information->DataOffset);
            if (ControllerType == ParallelController) {
                PWCHAR tmpChar;

                //
                // Some extra mapping is performed here to
                // translate the firmware names to LPT names.
                //

                *ptr++ = (WCHAR) 'L';
                *ptr++ = (WCHAR) 'P';
                *ptr++ = (WCHAR) 'T';

                //
                // Find the number.
                //

                tmpChar = ptr;
                while (*tmpChar) {
                    if ((*tmpChar >= (WCHAR) '0') &&
                        (*tmpChar <= (WCHAR) '9')) {
                        break;
                    }
                    tmpChar++;
                }

                if (*tmpChar) {
                    while (*tmpChar) {
                        *ptr++ = *tmpChar++;
                    }
                    *ptr = (WCHAR) 0;

                    //
                    // Update the datalength to be 4 wchars and eos and
                    // restore the pointer.
                    //

                    ptr = (PWCHAR) ((PUCHAR)information + information->DataOffset);
                    dataLength = 10;
                } else {
                    dataLength = 0;
                    DebugPrint((MAPPER_ERROR,
                                "Mapper: no parallel port number!\n"));
                }
            }

            if (dataLength) {
                firmwareEntry->Identifier = ExAllocatePool(NonPagedPool,
                                                           dataLength);
                if (firmwareEntry->Identifier) {

                    //
                    // Move the data
                    //

                    firmwareEntry->IdentifierType = information->Type;
                    firmwareEntry->IdentifierLength = dataLength;
                    RtlMoveMemory(firmwareEntry->Identifier, ptr, dataLength);
                }
            }
        }
    }

    //
    // For some controllers, search the peripheral information
    //

    switch (ControllerType) {
    case SerialController:
    case ParallelController:
        //
        // Don't look for a peripheral.
        //
        peripheralType = (CONFIGURATION_TYPE) 0;
        break;
    case DiskController:
        peripheralType = FloppyDiskPeripheral;
        break;
    case KeyboardController:
        peripheralType = KeyboardPeripheral;
        break;
    case PointerController:
        peripheralType = PointerPeripheral;
        break;
    default:
        peripheralType = (CONFIGURATION_TYPE) 0;
        break;
    }

    DebugPrint((MAPPER_REGISTRY,
                "Mapper: registry location is\n %ws\n",
                PathName->Buffer));

    DebugPrint((MAPPER_INFORMATION,
                "Mapper: ControllerInformation[] -\n\tIdent: %x -\n\tData: %x -\n\tInformation: %x\n",
                ControllerInformation[0],
                ControllerInformation[1],
                ControllerInformation[2]));

    if (peripheralType) {
        DebugPrint((MAPPER_PNP_ID,
                    "Mapper: searching for peripheral type %d\n",
                    peripheralType));

        IoQueryDeviceDescription(&BusType,
                                 &BusNumber,
                                 &ControllerType,
                                 &ControllerNumber,
                                 &peripheralType,
                                 NULL,
                                 MapperPeripheralCallback,
                                 firmwareEntry);
    }

    //
    // firmwareEntry->PnPId will be NULL if there are no peripherals of this
    // type in the tree or if the peripheral's description doesn't match one of
    // those in our table.
    //
    // firmwareEntry->PeripheralType will be equal to peripheralType if we found
    // one of the proper type regardless of whether or not it is in the table.
    //
    // So this test just ensures that we fallback to the controller IDs in the
    // case were there is no peripheral entry.  If there is a peripheral entry
    // that we don't understand we will suppress the entire node.
    //
    // This prevents creating devices with hw ids of bogus as we were seeing on
    // the SGI x86 ARC machines.
    //

    if (!firmwareEntry->PnPId && firmwareEntry->PeripheralType == 0) {

        //
        // Attempt to get PnPId from the controller type.
        //

        firmwareEntry->PnPId = MapperTranslatePnPId(ControllerType, NULL);

        if (!firmwareEntry->PnPId) {
            DebugPrint((MAPPER_PNP_ID,
                        "Mapper: NO PnP Id for\n ==> %ws\n",
                        PathName->Buffer));
        }
    }

    // NEC98 needs DeviceId which differs with the other H/W.
    if (IsNEC_98){

        switch (ControllerType) {
        case SerialController:{
            ULONG   i,j;
            PWSTR   serialFirmwareName;
            serialFirmwareName = ExAllocatePool(NonPagedPool, sizeof(USHORT)*10);
            if (!serialFirmwareName) {
                if (buffer)
                    ExFreePool(buffer);
                if (firmwareEntry) {
                    if (firmwareEntry->Identifier)
                        ExFreePool(firmwareEntry->Identifier);
                    ExFreePool(firmwareEntry);
                }
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            for (
                i = 0;
                i < controllerData->PartialResourceList.Count;
                i++
                ) {
                PCM_PARTIAL_RESOURCE_DESCRIPTOR partial =
                    &controllerData->PartialResourceList.PartialDescriptors[i];

                if (partial->Type == CmResourceTypeDeviceSpecific) {
                    PCM_SERIAL_DEVICE_DATA sDeviceData;

                    sDeviceData = (PCM_SERIAL_DEVICE_DATA)(partial + 1);

                    swprintf(serialFirmwareName, L"*nEC%04X\0", sDeviceData->Revision);
                    j = 0;
                    while (SerialIdNEC[j]) {
                        if (!wcscmp(serialFirmwareName, SerialIdNEC[j])) {
                            firmwareEntry->PnPId = SerialIdNEC[j];
                        }
                        j++;
                    }
                }

            }
            ExFreePool(serialFirmwareName);
            break;
        }
        case ParallelController:
            firmwareEntry->PnPId = ParallelIdNEC;
            break;
        default:
            break;
        }
    }

    DebugPrint((MAPPER_PNP_ID,
                "Mapper: constructed name %d_%d_%d_%d_%d_%d\n",
                firmwareEntry->BusType,
                firmwareEntry->BusNumber,
                firmwareEntry->ControllerType,
                firmwareEntry->ControllerNumber,
                firmwareEntry->PeripheralType,
                firmwareEntry->PeripheralNumber));

    if (firmwareEntry->PnPId) {

        //
        // Link into chain of entries.
        //

        firmwareEntry->Next = deviceExtension->FirmwareList;
        deviceExtension->FirmwareList = firmwareEntry;
    } else {

        //
        // No map found - don't remember this entry.
        //

        ExFreePool(buffer);
        if(firmwareEntry->Identifier) {
            ExFreePool(firmwareEntry->Identifier);
        }
        ExFreePool(firmwareEntry);
    }
    return STATUS_SUCCESS;
}


VOID
MapperProcessFirmwareTree(
    IN BOOLEAN OnlyProcessSerialPorts
    )

/*++

Routine Description:

    Query the information in the firmware tree to know what
    system board devices were located.  This will cause a FirmwareList
    to be created on the device extention passed.

Arguments:

    OnlyProcessSerialPorts - if non-zero, then we'll only look at serial ports.
        This is done on ACPI machines where, in general, we don't want to pay
        attention to ntdetect/firmware information (but we have to for serial
        ports so that legacy add-in ISA serial ports and modems are detected
        automatically as in previous versions of NT as well as Win9x).

Return Value:

    None

--*/

{
    INTERFACE_TYPE     interfaceType;
    ULONG              index;
    CONFIGURATION_TYPE sc;
    CONFIGURATION_TYPE controllerTypes[] = { PointerController,
                                             KeyboardController,
                                             ParallelController,
                                             DiskController,
                                             FloppyDiskPeripheral,
                                             SerialController   // must be last
                                           };
#define CONTROLLER_TYPES_COUNT (sizeof(controllerTypes) / sizeof(controllerTypes[0]))

    //
    // Locate all firmware controller information and save its resource usage.
    //
    // BUGBUG (lonnym)--it's pretty inefficient to be going through all
    // interface types, when we really only care about a very small subset of
    // non-PnP buses (e.g., ISA, EISA, maybe Internal).
    //

    for (interfaceType = 0; interfaceType < MaximumInterfaceType; interfaceType++) {

        DebugPrint((MAPPER_VERBOSE,
                    "Mapper: searching on interface ===> %d\n",
                    interfaceType));

        if(OnlyProcessSerialPorts) {

            //
            // Start out at the last element of the array, so we only process
            // SerialControllers.
            //

            index = CONTROLLER_TYPES_COUNT - 1;
        } else {
            index = 0;
        }

        for ( ; index < CONTROLLER_TYPES_COUNT; index++) {
            sc = controllerTypes[index];

            IoQueryDeviceDescription(&interfaceType,
                                     NULL,
                                     &sc,
                                     NULL,
                                     NULL,
                                     NULL,
                                     MapperCallback,
                                     &MapperDeviceExtension);
        }
    }
}


VOID
MapperMarkKey(
    IN HANDLE           Handle,
    IN PUNICODE_STRING  PathName,
    IN PFIRMWARE_CONFIGURATION FirmwareEntry
    )

/*++

Routine Description:

    Record in the root enum key that the firmware mapper found this entry.
    Migrate configuration information entries.

Arguments:

    Handle   - handle to the key
    PathName - base path name to this key
    FirmwareEntry - information from the firmware tree.

Return Value:

    None

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    PCM_RESOURCE_LIST resourceList;
    UNICODE_STRING    unicodeName;
    NTSTATUS          status;
    HANDLE            subKeyHandle;
    PWCHAR            wcptr;
    ULONG             disposition;
    ULONG             buffer;
    USHORT            originalLength;

    //
    // Mark that this entry was in the firmware tree.
    //

    buffer = 1;
    RtlInitUnicodeString(&unicodeName, REGSTR_VAL_FIRMWAREIDENTIFIED);

    ZwSetValueKey(Handle,
                  &unicodeName,
                  0,
                  REG_DWORD,
                  &buffer,
                  sizeof(ULONG));

    //
    // Create the control subkey
    //

    DebugPrint((MAPPER_INFORMATION,
                "Mapper: marking existing key\n"));
    originalLength = PathName->Length;
    wcptr = (PWCHAR) ((PUCHAR)PathName->Buffer + PathName->Length);
    wcptr++; // locate eos

    //
    // Build the volatile control key
    //

    InitializeObjectAttributes(&objectAttributes,
                               PathName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    RtlAppendUnicodeToString(PathName, L"\\Control");
    status = ZwCreateKey(&subKeyHandle,
                         KEY_READ | KEY_WRITE,
                         &objectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         &disposition);

    if (NT_SUCCESS(status)) {

        //
        // Create the found by firmware volatile.
        //

        buffer = 1;
        RtlInitUnicodeString(&unicodeName, REGSTR_VAL_FIRMWAREMEMBER);

        ZwSetValueKey(subKeyHandle,
                      &unicodeName,
                      0,
                      REG_DWORD,
                      &buffer,
                      sizeof(ULONG));
        ZwClose(subKeyHandle);

    } else {

        //
        // ignore failures
        //

        DebugPrint((MAPPER_ERROR,
                    "Mapper: failed to mark control key %x\n",
                    status));
    }

    //
    // if there is a resource descriptor, restore path and open LogConf key.
    //

    if (FirmwareEntry->ResourceDescriptor) {
        PathName->Length = originalLength;
        *wcptr = (WCHAR) 0;

        InitializeObjectAttributes(&objectAttributes,
                                   PathName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        RtlAppendUnicodeToString(PathName, L"\\LogConf");
        status = ZwCreateKey(&subKeyHandle,
                             KEY_READ | KEY_WRITE,
                             &objectAttributes,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             &disposition);

        if (NT_SUCCESS(status)) {
            ULONG size;

            //
            // two entries need to be made:
            // BootConfig:REG_RESOURCE_LIST
            // BasicConfigVector:REG_RESOURCE_REQUIREMENTS_LIST
            //

            size = sizeof(CM_RESOURCE_LIST) -
                   sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
                   FirmwareEntry->ResourceDescriptorSize;

            resourceList = ExAllocatePool(NonPagedPool, size);

            if (resourceList) {
                PIO_RESOURCE_REQUIREMENTS_LIST reqList;

                resourceList->Count = 1;
                RtlMoveMemory(&resourceList->List[0],
                              FirmwareEntry->ResourceDescriptor,
                              FirmwareEntry->ResourceDescriptorSize);

                resourceList = MapperAdjustResourceList (
                                   resourceList,
                                   FirmwareEntry->PnPId,
                                   &size
                                   );

                RtlInitUnicodeString(&unicodeName,
                                     L"BootConfig");
                ZwSetValueKey(subKeyHandle,
                              &unicodeName,
                              0,
                              REG_RESOURCE_LIST,
                              resourceList,
                              size);
#if 0
                //
                // Now do the resource requirements list.
                //

                reqList = IopCmResourcesToIoResources(0, resourceList);

                if (reqList) {
                    RtlInitUnicodeString(&unicodeName,
                                         L"BasicConfigVector");
                    ZwSetValueKey(subKeyHandle,
                                  &unicodeName,
                                  0,
                                  REG_RESOURCE_REQUIREMENTS_LIST,
                                  reqList,
                                  reqList->ListSize);
                    ExFreePool(reqList);
                }
#endif
                ExFreePool(resourceList);
            }
        } else {

            //
            // ignore errors
            //

            DebugPrint((MAPPER_ERROR,
                        "Mapper: failed to update logconf key %x\n",
                        status));
        }
    }

    //
    // Restore path passed in.
    //

    PathName->Length = originalLength;
    *wcptr = (WCHAR) 0;
}

VOID
MapperSeedKey(
    IN HANDLE                  Handle,
    IN PUNICODE_STRING         PathName,
    IN PFIRMWARE_CONFIGURATION FirmwareEntry,
    IN BOOLEAN                 DeviceIsPhantom
    )

/*++

Routine Description:

    This routine seeds a registry key with enough information
    to get PnP to run the class installer on the devnode.

Arguments:

    Handle          - handle to the key

    PathName        - base path name to this key

    FirmwareEntry   - information from the firmware tree

    DeviceIsPhantom - if non-zero, add "Phantom" value entry so the root
        enumerator will skip this device instance (i.e., not turn it into a
        devnode)

Return Value:

    None

--*/

{
#define SEED_BUFFER_SIZE (512 * sizeof(WCHAR))
    UNICODE_STRING    unicodeName;
    UNICODE_STRING    unicodeValue;
    OBJECT_ATTRIBUTES objectAttributes;
    PMAPPER_SEED      valueSeed;
    PKEY_SEED         keySeed;
    NTSTATUS          status;
    HANDLE            subKeyHandle;
    PWCHAR            pnpid;
    PWCHAR            buffer;
    PWCHAR            wcptr;
    ULONG             disposition;
    ULONG             size;
    USHORT            originalLength;

    buffer = ExAllocatePool(NonPagedPool, SEED_BUFFER_SIZE);
    if (!buffer) {
        return;
    }
    RtlZeroMemory(buffer, SEED_BUFFER_SIZE);

    //
    // Create subkeys.
    //

    originalLength = PathName->Length;
    wcptr = (PWCHAR) ((PUCHAR)PathName->Buffer + PathName->Length);

    for (keySeed = MapperKeySeed; keySeed->KeyName; keySeed++) {

        //
        // Reset the base path for the next key to seed.
        //

        *wcptr = (WCHAR) 0;
        PathName->Length = originalLength;
        RtlAppendUnicodeToString(PathName, keySeed->KeyName);

        //
        // Only build a device parameters key if there is something
        // to put in the key (i.e., this is a serial or parallel port).
        //

        if (keySeed->Options & KEY_SEED_DEVICE_PARAMETERS) {
            if (((FirmwareEntry->ControllerType != SerialController) && (FirmwareEntry->ControllerType != ParallelController)) ||
                !FirmwareEntry->Identifier) {
                continue;
            }

            status = IopOpenDeviceParametersSubkey( &subKeyHandle,
                                                    NULL,
                                                    PathName,
                                                    KEY_READ | KEY_WRITE
                                                    );
            if (NT_SUCCESS(status)) {
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_UNSUCCESSFUL;
            }
        } else {

            //
            // need to construct this key.
            //

            InitializeObjectAttributes(&objectAttributes,
                                       PathName,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);
            status = ZwCreateKey(&subKeyHandle,
                                 KEY_READ | KEY_WRITE,
                                 &objectAttributes,
                                 0,
                                 NULL,
                                 keySeed->Attribute,
                                 &disposition);
        }

        if (NT_SUCCESS(status)) {

            //
            // Check to see if this is the parameters key and
            // migrate the parameter information.
            //

            if (keySeed->Options & KEY_SEED_DEVICE_PARAMETERS) {

                if (FirmwareEntry->ControllerType == SerialController)  {

                    ComPortDBAdd(subKeyHandle, (PWSTR)FirmwareEntry->Identifier);
                } else {
                    //
                    // to get here there must be identifier information
                    // in the FirmwareEntry, so that check is not performed.
                    //
                    // NOTE: this will only happen once - when the key is
                    // created -- perhaps this needs to happen on every
                    // boot.
                    //

                    RtlInitUnicodeString(&unicodeName,
                                        L"PortName");
                    ZwSetValueKey(subKeyHandle,
                                &unicodeName,
                                0,
                                FirmwareEntry->IdentifierType,
                                FirmwareEntry->Identifier,
                                FirmwareEntry->IdentifierLength);
                }
            }
            ZwClose(subKeyHandle);
        } else {

            //
            // ignore failures
            //

            DebugPrint((MAPPER_ERROR,
                        "Mapper: failed to build control key %x\n",
                        status));
        }
    }

    //
    // Undo the mangling of the path name performed in the loop above.
    //

    *wcptr = (WCHAR) 0;
    PathName->Length = originalLength;

    //
    // Create values.
    //

    pnpid = FirmwareEntry->PnPId;
    for (valueSeed = MapperValueSeed; valueSeed->ValueName; valueSeed++) {

        if (valueSeed->ValueType == REG_DWORD) {

            if ((valueSeed->Options == OPTIONS_INSERT_PHANTOM_MARKER) &&
                !DeviceIsPhantom) {

                //
                // Device isn't a phantom--we don't want to mark it as such.
                //

                continue;
            }

            size = sizeof(ULONG);
            RtlMoveMemory(buffer, &valueSeed->DwordValueContent, size);

        } else if (valueSeed->Options == OPTIONS_INSERT_PNP_ID) {

            size = (wcslen(pnpid) + 2) * sizeof(WCHAR); // eos multi_sz
            if (FirmwareEntry->BusType == Eisa) {

                //
                // need a mult_sz of EISA\PNPblah *PNPblah
                //

                RtlZeroMemory(buffer, SEED_BUFFER_SIZE);
                wcptr = pnpid;
                wcptr++;
                swprintf(buffer, L"EISA\\%s", wcptr);

                wcptr = buffer;
                while (*wcptr) {
                    wcptr++;
                }
                wcptr++; // step past eos for 1st string

                RtlMoveMemory(wcptr, pnpid, size);

                size += (ULONG)((PUCHAR)wcptr - (PUCHAR)buffer);
            } else {
                RtlMoveMemory(buffer, pnpid, size - sizeof(WCHAR));
                buffer[size / sizeof(WCHAR) - 1] = L'\0';
            }
        } else if (valueSeed->Options == OPTIONS_INSERT_COMPATIBLE_IDS) {
            if (FirmwareEntry->PeripheralType == KeyboardPeripheral)  {
                size = sizeof(PS2_KEYBOARD_COMPATIBLE_ID);
                RtlMoveMemory(buffer, PS2_KEYBOARD_COMPATIBLE_ID, size);
            } else if (FirmwareEntry->PeripheralType == PointerPeripheral &&
                       (wcscmp(pnpid, L"*PNP0F0E") == 0 ||
                        wcscmp(pnpid, L"*PNP0F03") == 0 ||
                        wcscmp(pnpid, L"*PNP0F12") == 0)) {
                size = sizeof(PS2_MOUSE_COMPATIBLE_ID);
                RtlMoveMemory(buffer, PS2_MOUSE_COMPATIBLE_ID, size);
            } else {
                continue;
            }
            buffer[size / 2] = L'\0';  // 2nd NUL for MULTI_SZ
            size += sizeof(L'\0');
        } else if (valueSeed->Options == OPTIONS_INSERT_DEVICEDESC) {
            size = FirmwareEntry->IdentifierLength;
            RtlMoveMemory(buffer, FirmwareEntry->Identifier, size);
        } else {
            DebugPrint((MAPPER_ERROR, "Mapper: NO VALUE TYPE!\n"));
            ASSERT(FALSE);
            continue;
        }

        RtlInitUnicodeString(&unicodeName,
                             valueSeed->ValueName);
        ZwSetValueKey(Handle,
                      &unicodeName,
                      0,
                      valueSeed->ValueType,
                      buffer,
                      size);
    }
    ExFreePool(buffer);
}


VOID
MapperFreeList(
    VOID
    )

/*++

Routine Description:

    This routine walks through the list of firmware entries
    and frees all allocated memory.

Arguments:

    None

Return Value:

    None

--*/

{
    PDEVICE_EXTENSION       deviceExtension = &MapperDeviceExtension;
    PFIRMWARE_CONFIGURATION tempEntry;
    PFIRMWARE_CONFIGURATION firmwareEntry;

    firmwareEntry = deviceExtension->FirmwareList;
    while (firmwareEntry) {

        //
        // free allocated structures associated with the firmware entry
        //

        if (firmwareEntry->ResourceDescriptor) {
            ExFreePool(firmwareEntry->ResourceDescriptor);
        }
        if (firmwareEntry->Identifier) {
            ExFreePool(firmwareEntry->Identifier);
        }

        //
        // free this entry and move to the next
        //

        tempEntry = firmwareEntry->Next;
        ExFreePool(firmwareEntry);
        firmwareEntry = tempEntry;
    }
}

VOID
MapperConstructRootEnumTree(
    IN BOOLEAN CreatePhantomDevices
    )

/*++

Routine Description:

    This routine walks through the list of firmware entries
    in the device extension and migrates the information into
    the root enumerator's tree in the registry.

Arguments:

    CreatePhantomDevices - If non-zero, then the device instances are created
        as "phantoms" (i.e., they are marked with the "Phantom" value entry so
        that the root enumerator will ignore them).  The only time these device
        instance registry keys will ever turn into real live devnodes is if the
        class installer (in response to DIF_FIRSTTIMESETUP or DIF_DETECT)
        decides that these devices aren't duplicates of any PnP-enumerated
        devnodes, and subsequently registers and installs them.

Return Value:

    None

--*/

{
#define ENUM_KEY_BUFFER_SIZE (1024 * sizeof(WCHAR))
#define INSTANCE_BUFFER_SIZE (256 * sizeof(WCHAR))
    UNICODE_STRING          enumKey;
    PFIRMWARE_CONFIGURATION firmwareEntry;
    OBJECT_ATTRIBUTES       objectAttributes;
    NTSTATUS                status;
    BOOLEAN                 keyPresent;
    PWCHAR                  registryBase;
    PWCHAR                  instanceBuffer;
    HANDLE                  handle;
    ULONG                   disposition;
    PVOID                   buffer;
    PDEVICE_EXTENSION       DeviceExtension = &MapperDeviceExtension;

    //
    // allocate space needed for the registry path into the root
    // enumerator tree.  Note, limited size on path length.
    //

    buffer = ExAllocatePool(NonPagedPool, ENUM_KEY_BUFFER_SIZE);

    if (!buffer) {
        MapperFreeList();
        DebugPrint((MAPPER_ERROR,
                    "Mapper: could not allocate memory for registry update\n"));
        return;
    }

    instanceBuffer = ExAllocatePool(NonPagedPool, INSTANCE_BUFFER_SIZE);
    if (!instanceBuffer) {
        MapperFreeList();
        ExFreePool(buffer);
        DebugPrint((MAPPER_ERROR,
                    "Mapper: could not allocate memory for instance buffer\n"));
        return;
    }

    InitializeObjectAttributes(&objectAttributes,
                               &enumKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

#if UMODETEST
    registryBase = L"\\Registry\\Machine\\System\\TestControlSet\\Enum\\Root\\";
#else
    registryBase = L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\Root\\";
#endif

    firmwareEntry = DeviceExtension->FirmwareList;
    while (firmwareEntry) {

        //
        // Construct the base for the path for this entry.
        //


        RtlInitUnicodeString(&enumKey, NULL);
        enumKey.MaximumLength = ENUM_KEY_BUFFER_SIZE;
        enumKey.Buffer = buffer;
        RtlZeroMemory(buffer, ENUM_KEY_BUFFER_SIZE);
        RtlAppendUnicodeToString(&enumKey, registryBase);
        RtlAppendUnicodeToString(&enumKey, firmwareEntry->PnPId);

        //
        // Build the pnp Key.
        //

        status = ZwCreateKey(&handle,
                             KEY_READ | KEY_WRITE,
                             &objectAttributes,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             &disposition);

        if (NT_SUCCESS(status)) {

            //
            // Do not need the handle, so close it
            // Remember if the key was present prior to call
            //

            ZwClose(handle);
            keyPresent = (disposition == REG_OPENED_EXISTING_KEY) ? TRUE : FALSE;
            DebugPrint((MAPPER_INFORMATION,
                        "Mapper: Key was %s\n",
                        keyPresent ? "Present" : "Created"));

            //
            // Construct the instance name.
            //

            RtlZeroMemory(instanceBuffer, INSTANCE_BUFFER_SIZE);
            swprintf(instanceBuffer,
                     L"\\%d_%d_%d_%d_%d_%d",
                     firmwareEntry->BusType,
                     firmwareEntry->BusNumber,
                     firmwareEntry->ControllerType,
                     firmwareEntry->ControllerNumber,
                     firmwareEntry->PeripheralType,
                     firmwareEntry->PeripheralNumber);
            RtlAppendUnicodeToString(&enumKey, instanceBuffer);

            status = ZwCreateKey(&handle,
                                 KEY_READ | KEY_WRITE,
                                 &objectAttributes,
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 &disposition);

            if (NT_SUCCESS(status)) {

                if (firmwareEntry->ResourceDescriptor) {
                    DebugPrint((MAPPER_INFORMATION,
                                "Mapper: firmware entry has resources %x\n",
                                firmwareEntry->ResourceDescriptor));
                }

                if (firmwareEntry->Identifier) {
                    DebugPrint((MAPPER_INFORMATION,
                                "Mapper: firmware entry has identifier %x\n",
                                firmwareEntry->Identifier));
                }

                //
                // Only if this is a new entry do we see the key.
                //

                if (disposition == REG_CREATED_NEW_KEY) {

                    //
                    // Remember the fact that the key was newly-created for the
                    // PnP BIOS case where we need to come along and "phantomize"
                    // all newly-created ntdetect COM ports.
                    //

                    firmwareEntry->NewlyCreated = TRUE;

                    //
                    // Create enough information to get pnp to
                    // install drivers
                    //

                    MapperSeedKey(handle,
                                  &enumKey,
                                  firmwareEntry,
                                  CreatePhantomDevices
                                 );
                }
                MapperMarkKey(handle,
                              &enumKey,
                              firmwareEntry);
                ZwClose(handle);

            } else {
                DebugPrint((MAPPER_ERROR,
                            "Mapper: create of instance key failed %x\n",
                            status));
            }

        } else {
            DebugPrint((MAPPER_ERROR,
                        "Mapper: create pnp key failed %x\n",
                        status));
        }

        firmwareEntry = firmwareEntry->Next;
    }
    ExFreePool(instanceBuffer);
}

PCM_RESOURCE_LIST
MapperAdjustResourceList (
    IN     PCM_RESOURCE_LIST ResourceList,
    IN     PWCHAR            PnPId,
    IN OUT PULONG            Size
    )
{
    PCM_PARTIAL_RESOURCE_LIST       partialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR problemPartialDescriptors;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptors;
    PCM_RESOURCE_LIST               newResourceList;
    ULONG                           i;

    newResourceList = ResourceList;

#if _X86_
    if (KeI386MachineType == MACHINE_TYPE_EISA) {

        PCM_FULL_RESOURCE_DESCRIPTOR    fullDescriptor;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptor;
        PUCHAR                          nextDescriptor;
        ULONG                           j;
        ULONG                           lastResourceIndex;

        fullDescriptor = &ResourceList->List[0];

        for (i = 0; i < ResourceList->Count; i++) {

            partialResourceList = &fullDescriptor->PartialResourceList;

            for (j = 0; j < partialResourceList->Count; j++) {
                partialDescriptor = &partialResourceList->PartialDescriptors[j];

                if (partialDescriptor->Type == CmResourceTypePort) {
                    if (partialDescriptor->u.Port.Start.HighPart == 0 &&
                        (partialDescriptor->u.Port.Start.LowPart & 0x00000300) == 0) {
                        partialDescriptor->Flags |= CM_RESOURCE_PORT_16_BIT_DECODE;
                    }
                }
            }

            nextDescriptor = (PUCHAR)fullDescriptor + sizeof(CM_FULL_RESOURCE_DESCRIPTOR);

            //
            // account for any resource descriptors in addition to the single
            // imbedded one I've already accounted for (if there aren't any,
            // then I'll end up subtracting off the extra imbedded descriptor
            // from the previous step)
            //
            //
            // finally, account for any extra device specific data at the end of
            // the last partial resource descriptor (if any)
            //
            if (partialResourceList->Count > 0) {

                nextDescriptor += (partialResourceList->Count - 1) *
                     sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

                lastResourceIndex = partialResourceList->Count - 1;

                if (partialResourceList->PartialDescriptors[lastResourceIndex].Type ==
                          CmResourceTypeDeviceSpecific) {

                    nextDescriptor += partialResourceList->PartialDescriptors[lastResourceIndex].
                               u.DeviceSpecificData.DataSize;
                }
            }

            fullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)nextDescriptor;
        }
    }
#endif

    if (wcscmp(PnPId, FloppyId) == 0) {

        if (ResourceList->Count == 1) {

            partialResourceList = &ResourceList->List->PartialResourceList;

            partialDescriptors = partialResourceList->PartialDescriptors;

            //
            // Look for the one and only one 8 byte port resource
            //
            problemPartialDescriptors = NULL;
            for (i=0; i<partialResourceList->Count; i++) {

                if ((partialDescriptors[i].Type == CmResourceTypePort) &&
                    (partialDescriptors[i].u.Port.Length == 8)) {

                    if (problemPartialDescriptors == NULL) {

                        problemPartialDescriptors = partialDescriptors + i;
                    } else {

                        problemPartialDescriptors = NULL;
                        break;
                    }
                }
            }

            if (problemPartialDescriptors) {

                problemPartialDescriptors->u.Port.Length = 6;

                newResourceList = ExAllocatePool (
                                      NonPagedPool,
                                      *Size + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
                                      );
                if (newResourceList) {

                    RtlMoveMemory (
                        newResourceList,
                        ResourceList,
                        *Size
                        );

                    //
                    // pick out the new partial resource descriptor
                    //
                    partialDescriptors = newResourceList->List->
                                             PartialResourceList.PartialDescriptors;
                    partialDescriptors += newResourceList->List->PartialResourceList.Count;

                    RtlMoveMemory (
                        partialDescriptors,
                        problemPartialDescriptors,
                        sizeof(*partialDescriptors)
                        );

                    partialDescriptors->u.Port.Start.QuadPart += 7;
                    partialDescriptors->u.Port.Length = 1;

                    //
                    // we got one more now
                    //
                    newResourceList->List->PartialResourceList.Count++;
                    *Size += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

                    ExFreePool (ResourceList);

                } else {

                    newResourceList = ResourceList;
                }
            }
        }
    }

    return newResourceList;
}

NTSTATUS
ComPortDBAdd(
    IN  HANDLE  DeviceParamKey,
    IN  PWSTR   PortName
    )
{
    UNICODE_STRING                  portNameString;
    UNICODE_STRING                  portPrefixString;
    UNICODE_STRING                  comDBName;
    UNICODE_STRING                  valueName;
    PKEY_VALUE_PARTIAL_INFORMATION  valueInfo;
    ULONG                           valueInfoLength;
    ULONG                           returnedLength;
    HANDLE                          comDBKey;
    ULONG                           portNo;
    NTSTATUS                        status;

    RtlInitUnicodeString(&portNameString, PortName);

    if (portNameString.Length > 3 * sizeof(WCHAR)) {
        portNameString.Length = 3 * sizeof(WCHAR);
    }

    RtlInitUnicodeString(&portPrefixString, L"COM");

    if (RtlCompareUnicodeString(&portNameString, &portPrefixString, TRUE) == 0) {
        portNo = _wtol(&PortName[3]);

        if (portNo > 0 && portNo <= 256) {

#if UMODETEST
            RtlInitUnicodeString(&comDBName, L"\\Registry\\Machine\\System\\TestControlSet\\Control\\COM Name Arbiter");
#else
            RtlInitUnicodeString(&comDBName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\COM Name Arbiter");
#endif

            status = IopCreateRegistryKeyEx( &comDBKey,
                                             NULL,
                                             &comDBName,
                                             KEY_ALL_ACCESS,
                                             REG_OPTION_NON_VOLATILE,
                                             NULL
                                             );

            if (NT_SUCCESS(status)) {

                RtlInitUnicodeString(&valueName, L"ComDB Merge");

#define COMPORT_DB_MERGE_SIZE    32           //  256 / 8

                valueInfoLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + COMPORT_DB_MERGE_SIZE;
                valueInfo = ExAllocatePool(PagedPool, valueInfoLength);

                if (valueInfo != NULL) {

                    status = ZwQueryValueKey( comDBKey,
                                              &valueName,
                                              KeyValuePartialInformation,
                                              valueInfo,
                                              valueInfoLength,
                                              &returnedLength);

                    if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

                        valueInfo->Type = REG_BINARY;
                        valueInfo->DataLength = COMPORT_DB_MERGE_SIZE;
                        RtlZeroMemory(valueInfo->Data, valueInfo->DataLength);
                        status = STATUS_SUCCESS;
                    }

                    if (NT_SUCCESS(status)) {
                        portNo--;
                        valueInfo->Data[ portNo / 8 ] |= 1 << (portNo % 8);

                        status = ZwSetValueKey( comDBKey,
                                                &valueName,
                                                0,
                                                valueInfo->Type,
                                                valueInfo->Data,
                                                valueInfo->DataLength );

                        ASSERT(NT_SUCCESS(status));
                    }

                    ExFreePool(valueInfo);
                }

                ZwClose(comDBKey);
            }
        }
    }

    RtlInitUnicodeString( &valueName, L"PortName" );

    status = ZwSetValueKey( DeviceParamKey,
                            &valueName,
                            0,
                            REG_SZ,
                            PortName,
                            (wcslen(PortName) + 1) * sizeof(WCHAR) );

    return status;
}


VOID
MapperPhantomizeDetectedComPorts (
    VOID
    )
/*++

Routine Description:

    This routine turns all newly-created firmware/ntdetect COM ports into
    phantoms.

Arguments:

    None

Return Value:

    None

--*/
{
    PFIRMWARE_CONFIGURATION firmwareEntry;
    NTSTATUS                status;
    PWCHAR                  registryBase;
    PWCHAR                  instanceBuffer;
    HANDLE                  handle;
    PWCHAR                  buffer;
    PDEVICE_EXTENSION       DeviceExtension = &MapperDeviceExtension;
    UNICODE_STRING          enumKey;
    OBJECT_ATTRIBUTES       objectAttributes;
    UNICODE_STRING          unicodeName;
    ULONG                   regValue;

    //
    // allocate space needed for the registry path into the root
    // enumerator tree.  Note, limited size on path length.
    //

    buffer = ExAllocatePool(NonPagedPool, ENUM_KEY_BUFFER_SIZE);

    if (!buffer) {
        DebugPrint((MAPPER_ERROR,
                    "Mapper: could not allocate memory for registry update\n"));
        return;
    }

    instanceBuffer = ExAllocatePool(NonPagedPool, INSTANCE_BUFFER_SIZE);
    if (!instanceBuffer) {
        ExFreePool(buffer);
        DebugPrint((MAPPER_ERROR,
                    "Mapper: could not allocate memory for instance buffer\n"));
        return;
    }

    InitializeObjectAttributes(&objectAttributes,
                               &enumKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

#if UMODETEST
    registryBase = L"\\Registry\\Machine\\System\\TestControlSet\\Enum\\Root\\";
#else
    registryBase = L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\Root\\";
#endif

    firmwareEntry = DeviceExtension->FirmwareList;
    while (firmwareEntry) {

        //
        // Construct the base for the path for this entry.
        //


        if ((firmwareEntry->ControllerType == SerialController) &&
            firmwareEntry->NewlyCreated) {

            RtlInitUnicodeString(&enumKey, NULL);
            enumKey.MaximumLength = ENUM_KEY_BUFFER_SIZE;
            enumKey.Buffer = buffer;
            RtlZeroMemory(buffer, ENUM_KEY_BUFFER_SIZE);
            RtlAppendUnicodeToString(&enumKey, registryBase);
            RtlAppendUnicodeToString(&enumKey, firmwareEntry->PnPId);

            //
            // Construct the instance name.
            //

            RtlZeroMemory(instanceBuffer, INSTANCE_BUFFER_SIZE);
            swprintf(instanceBuffer,
                     L"\\%d_%d_%d_%d_%d_%d",
                     firmwareEntry->BusType,
                     firmwareEntry->BusNumber,
                     firmwareEntry->ControllerType,
                     firmwareEntry->ControllerNumber,
                     firmwareEntry->PeripheralType,
                     firmwareEntry->PeripheralNumber);
            RtlAppendUnicodeToString(&enumKey, instanceBuffer);

            status = ZwOpenKey(&handle,
                               KEY_READ | KEY_WRITE,
                               &objectAttributes
                              );

            if (NT_SUCCESS(status)) {

                RtlInitUnicodeString(&unicodeName, REGSTR_VAL_PHANTOM);
                regValue = 1;
                ZwSetValueKey(handle,
                              &unicodeName,
                              0,
                              REG_DWORD,
                              &regValue,
                              sizeof(regValue)
                             );

                ZwClose(handle);
            }
        }

        firmwareEntry = firmwareEntry->Next;
    }

    ExFreePool (buffer);
    ExFreePool (instanceBuffer);
}


#if DBG
VOID
MapperDebugPrint(
    ULONG  DebugMask,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for the firmware mapper

Arguments:

    Check the mask value to see if the debug message is requested.

Return Value:

    None

--*/

{
    va_list ap;
    CHAR    buffer[256];

    va_start(ap, DebugMessage);

    if (DebugMask & MapperDebugMask) {
        vsprintf(buffer, DebugMessage, ap);
        DbgPrint(buffer);
    }

    va_end(ap);

}
#endif


