/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    cmconfig.c

Abstract:

    This module is responsible to build the hardware tree of the
    registry data base.

Author:

    Shie-Lin Tzong (shielint) 23-Jan-1992


Environment:

    Kernel mode.

Revision History:

--*/

#include "cmp.h"

//
// Title Index - Never used for Product 1, set to 0 for now.
//

#define TITLE_INDEX_VALUE 0


extern ULONG CmpTypeCount[];


#define EISA_ADAPTER_INDEX EisaAdapter
#define TURBOCHANNEL_ADAPTER_INDEX TcAdapter

//
// The following variables are used to cross-reference multifunction
// adapters to their corresponding NT interface type.
//

extern struct {
    PUCHAR  AscString;
    USHORT  InterfaceType;
    USHORT  Count;
} CmpMultifunctionTypes[];

extern USHORT CmpUnknownBusCount;


//
// CmpConfigurationData - A pointer to the area reserved for the purpose
//     of reconstructing Configuration Data.
//
// CmpConfigurationAreaSize - Record the size of the Configuration Data
//     area.

extern ULONG CmpConfigurationAreaSize;
extern PCM_FULL_RESOURCE_DESCRIPTOR CmpConfigurationData;

//
// Function prototypes for internal erferences
//

NTSTATUS
CmpSetupConfigurationTree(
     IN PCONFIGURATION_COMPONENT_DATA CurrentEntry,
     IN HANDLE ParentHandle,
     IN INTERFACE_TYPE InterfaceType,
     IN ULONG BusNumber
     );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,CmpInitializeHardwareConfiguration)
#pragma alloc_text(INIT,CmpSetupConfigurationTree)
#pragma alloc_text(INIT,CmpInitializeRegistryNode)
#endif


NTSTATUS
CmpInitializeHardwareConfiguration(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This routine creates \\Registry\Machine\Hardware node in
    the registry and calls SetupTree routine to put the hardware
    information to the registry.

Arguments:

    LoaderBlock - supplies a pointer to the LoaderBlock passed in from the
                  OS Loader.

Returns:

    NTSTATUS code for sucess or reason of failure.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE BaseHandle;
    PCONFIGURATION_COMPONENT_DATA ConfigurationRoot;
    ULONG Disposition;

    ConfigurationRoot = (PCONFIGURATION_COMPONENT_DATA)LoaderBlock->ConfigurationRoot;
    if (ConfigurationRoot) {

#ifdef _X86_

        //
        //  The following strings found in the registry identify obscure,
        //  yet market dominant, non PC/AT compatible i386 machine in Japan.
        //

#define MACHINE_TYPE_FUJITSU_FMR_NAME_A    "FUJITSU FMR-"
#define MACHINE_TYPE_NEC_PC98_NAME_A       "NEC PC-98"

        {
            PCONFIGURATION_COMPONENT_DATA SystemNode;
            ULONG JapanMachineId;

            //
            // For Japan, we have to special case some non PC/AT machines, so
            // determine at this time what kind of platform we are on:
            //
            // NEC PC9800 Compatibles/Fujitsu FM-R Compatibles/IBM PC/AT Compatibles
            //
            // Default is PC/AT compatible.
            //

            JapanMachineId = MACHINE_TYPE_PC_AT_COMPATIBLE;

            //
            // Find the hardware description node
            //

            SystemNode = KeFindConfigurationEntry(ConfigurationRoot,
                                                  SystemClass,
                                                  MaximumType,
                                                  NULL);

            //
            // Did we find something?
            //

            if (SystemNode) {

                //
                // Check platform from identifier string
                //

                if (RtlCompareMemory(SystemNode->ComponentEntry.Identifier,
                                     MACHINE_TYPE_NEC_PC98_NAME_A,
                                     sizeof(MACHINE_TYPE_NEC_PC98_NAME_A) - 1) ==
                    sizeof(MACHINE_TYPE_NEC_PC98_NAME_A) - 1) {

                    //
                    // we are running on NEC PC-9800 comaptibles.
                    //

                    JapanMachineId = MACHINE_TYPE_PC_9800_COMPATIBLE;
                    SetNEC_98;

                } else if (RtlCompareMemory(SystemNode->ComponentEntry.Identifier,
                                            MACHINE_TYPE_FUJITSU_FMR_NAME_A,
                                            sizeof(MACHINE_TYPE_FUJITSU_FMR_NAME_A) - 1) ==
                           sizeof(MACHINE_TYPE_FUJITSU_FMR_NAME_A) - 1) {

                    //
                    // we are running on Fujitsu FMR comaptibles.
                    //

                    JapanMachineId = MACHINE_TYPE_FMR_COMPATIBLE;
                }
            }

            //
            //  Now 'or' this value into the kernel global.
            //

            KeI386MachineType |= JapanMachineId;
        }
#endif //_X86_

        //
        // Create \\Registry\Machine\Hardware\DeviceMap
        //

        InitializeObjectAttributes(
            &ObjectAttributes,
            &CmRegistryMachineHardwareDeviceMapName,
            0,
            (HANDLE)NULL,
            NULL
            );
        ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

        Status = NtCreateKey(                   // Paht may already exist
                    &BaseHandle,
                    KEY_READ | KEY_WRITE,
                    &ObjectAttributes,
                    TITLE_INDEX_VALUE,
                    NULL,
                    0,
                    &Disposition
                    );

        if (!NT_SUCCESS(Status)) {
            return(Status);
        }

        NtClose(BaseHandle);

        ASSERT(Disposition == REG_CREATED_NEW_KEY);

        //
        // Create \\Registry\Machine\Hardware\Description and use the
        // returned handle as the BaseHandle to build the hardware tree.
        //

        InitializeObjectAttributes(
            &ObjectAttributes,
            &CmRegistryMachineHardwareDescriptionName,
            0,
            (HANDLE)NULL,
            NULL
            );
        ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

        Status = NtCreateKey(                   // Path may already exist
                    &BaseHandle,
                    KEY_READ | KEY_WRITE,
                    &ObjectAttributes,
                    TITLE_INDEX_VALUE,
                    NULL,
                    0,
                    &Disposition
                    );

        if (!NT_SUCCESS(Status)) {
            return(Status);
        }

        ASSERT(Disposition == REG_CREATED_NEW_KEY);

        //
        // Allocate 16K bytes memory from paged pool for constructing
        // configuration data for controller component.
        // NOTE:  The configuration Data for controller component
        //    usually takes less than 100 bytes.  But on EISA machine, the
        //    EISA configuration information takes more than 10K and up to
        //    64K.  I believe 16K is the reasonable number to handler 99.9%
        //    of the machines.  Therefore, 16K is the initial value.
        //

        CmpConfigurationData = (PCM_FULL_RESOURCE_DESCRIPTOR)ExAllocatePool(
                                            PagedPool,
                                            CmpConfigurationAreaSize
                                            );

        if (CmpConfigurationData == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        //
        // Call SetupConfigurationTree routine to go over each component
        // of the tree and add component information to registry database.
        //

        Status = CmpSetupConfigurationTree(ConfigurationRoot,
                                           BaseHandle,
                                           -1,
                                           (ULONG)-1
                                           );
        ExFreePool((PVOID)CmpConfigurationData);
        NtClose(BaseHandle);
        return(Status);
    } else {
        return(STATUS_SUCCESS);
    }
}

NTSTATUS
CmpSetupConfigurationTree(
     IN PCONFIGURATION_COMPONENT_DATA CurrentEntry,
     IN HANDLE ParentHandle,
     IN INTERFACE_TYPE InterfaceType,
     IN ULONG BusNumber
     )
/*++

Routine Description:

    This routine traverses loader configuration tree and register
    the hardware information to the registry data base.

    Note to reduce the stack usage on machines with large number of PCI buses,
    we do not recursively process the sibling nodes.  We only recursively
    process the child trees.

Arguments:

    CurrentEntry - Supplies a pointer to a loader configuration
        tree or subtree.

    ParentHandle - Supplies the parent handle of CurrentEntry node.

    InterfaceType - Specify the Interface type of the bus that the
        CurrentEntry component resides.

    BusNumber - Specify the Bus Number of the bus that the CurrentEntry
        component resides.  If Bus number is -1, it means InterfaceType
        and BusNumber are meaningless for this component.

Returns:

    None.

--*/
{
    NTSTATUS Status;
    HANDLE NewHandle;
    USHORT i;
    CONFIGURATION_COMPONENT *Component;
    INTERFACE_TYPE LocalInterfaceType = InterfaceType;
    ULONG LocalBusNumber = BusNumber;
    USHORT DeviceIndexTable[NUMBER_TYPES];

    for (i = 0; i < NUMBER_TYPES; i++) {
        DeviceIndexTable[i] = 0;
    }

    //
    // Process current entry and its siblings
    //

    while (CurrentEntry) {

        //
        // Register current entry first before going down to its children
        //

        Component = &CurrentEntry->ComponentEntry;

        //
        // If the current component is a bus component, we will set up
        // its bus number and Interface type and use them to initialize
        // its subtree.
        //

        if (Component->Class == AdapterClass &&
            CurrentEntry->Parent->ComponentEntry.Class == SystemClass) {

            switch (Component->Type) {

            case EisaAdapter:
                LocalInterfaceType = Eisa;
                LocalBusNumber = CmpTypeCount[EISA_ADAPTER_INDEX]++;
                break;
            case TcAdapter:
                LocalInterfaceType = TurboChannel;
                LocalBusNumber = CmpTypeCount[TURBOCHANNEL_ADAPTER_INDEX]++;
                break;
            case MultiFunctionAdapter:

                //
                // Here we try to distinguish if the Multifunction adapter is
                // Isa, Mca, Internal bus and assign BusNumber based on
                // its interface type (bus type.)
                //

                if (Component->Identifier) {
                    for (i=0; CmpMultifunctionTypes[i].AscString; i++) {
                        if (_stricmp(CmpMultifunctionTypes[i].AscString,
                                    Component->Identifier) == 0) {
                                        break;
                        }
                    }

                    LocalInterfaceType = CmpMultifunctionTypes[i].InterfaceType;
                    LocalBusNumber = CmpMultifunctionTypes[i].Count++;
                }
                break;

            case ScsiAdapter:

                //
                // Set the bus type to internal.
                //

                LocalInterfaceType = Internal;
                LocalBusNumber = CmpTypeCount[ScsiAdapter]++;
                break;

            default:
                LocalInterfaceType = -1;
                LocalBusNumber = CmpUnknownBusCount++;
                break;
            }
        }

        //
        // Initialize and copy current component to hardware registry
        //

        Status = CmpInitializeRegistryNode(
                     CurrentEntry,
                     ParentHandle,
                     &NewHandle,
                     LocalInterfaceType,
                     LocalBusNumber,
                     DeviceIndexTable
                     );

        if (!NT_SUCCESS(Status)) {
            return(Status);
        }

        //
        // Once we are going one level down, we need to clear the TypeCount
        // table for everything under the current component class ...
        //

        if (CurrentEntry->Child) {

            //
            // Process the child entry of current entry
            //

            Status = CmpSetupConfigurationTree(CurrentEntry->Child,
                                               NewHandle,
                                               LocalInterfaceType,
                                               LocalBusNumber
                                               );
            if (!NT_SUCCESS(Status)) {
                NtClose(NewHandle);
                return(Status);
            }
        }
        NtClose(NewHandle);
        CurrentEntry = CurrentEntry->Sibling;
    }
    return(STATUS_SUCCESS);
}


NTSTATUS
CmpInitializeRegistryNode(
    IN PCONFIGURATION_COMPONENT_DATA CurrentEntry,
    IN HANDLE ParentHandle,
    OUT PHANDLE NewHandle,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN PUSHORT DeviceIndexTable
    )

/*++

Routine Description:

    This routine creates a node for the current firmware component
    and puts component data to the data part of the node.

Arguments:

    CurrentEntry - Supplies a pointer to a configuration component.

    Handle - Supplies the parent handle of CurrentEntry node.

    NewHandle - Suppiles a pointer to a HANDLE to receive the handle of
        the newly created node.

    InterfaceType - Specify the Interface type of the bus that the
        CurrentEntry component resides. (See BusNumber also)

    BusNumber - Specify the Bus Number of the bus that the CurrentEntry
        component resides on.  If Bus number is -1, it means InterfaceType
        and BusNumber are meaningless for this component.

Returns:

    None.

--*/
{

    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    UNICODE_STRING ValueData;
    HANDLE Handle;
    HANDLE OldHandle;
    ANSI_STRING AnsiString;
    UCHAR Buffer[12];
    WCHAR UnicodeBuffer[12];
    CONFIGURATION_COMPONENT *Component;
    ULONG Disposition;
    ULONG ConfigurationDataLength;
    PCM_FULL_RESOURCE_DESCRIPTOR NewArea;

    Component = &CurrentEntry->ComponentEntry;

    //
    // If the component class is SystemClass, we set its Type to be
    // ArcSystem.  The reason is because the detection code sets
    // its type to MaximumType to indicate it is NOT ARC compatible.
    // Here, we are only interested in building a System Node.  So we
    // change its Type to ArcSystem to ease the setup.
    //

    if (Component->Class == SystemClass) {
        Component->Type = ArcSystem;
    }

    //
    // Create a new key to describe the Component.
    //
    // The type of the component will be used as the keyname of the
    // registry node.  The class is the class of the component.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &(CmTypeName[Component->Type]),
        0,
        ParentHandle,
        NULL
        );
    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

    Status = NtCreateKey(                   // Paht may already exist
                &Handle,
                KEY_READ | KEY_WRITE,
                &ObjectAttributes,
                TITLE_INDEX_VALUE,
                &(CmClassName[Component->Class]),
                0,
                &Disposition
                );

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // If this component is NOT a SystemClass component, we will
    // create a subkey to identify the component's ordering.
    //

    if (Component->Class != SystemClass) {

        RtlIntegerToChar(
            DeviceIndexTable[Component->Type]++,
            10,
            12,
            Buffer
            );

        RtlInitAnsiString(
            &AnsiString,
            Buffer
            );

        KeyName.Buffer = (PWSTR)UnicodeBuffer;
        KeyName.Length = 0;
        KeyName.MaximumLength = sizeof(UnicodeBuffer);

        RtlAnsiStringToUnicodeString(
            &KeyName,
            &AnsiString,
            FALSE
            );

        OldHandle = Handle;

        InitializeObjectAttributes(
            &ObjectAttributes,
            &KeyName,
            0,
            OldHandle,
            NULL
            );
        ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

        Status = NtCreateKey(
                    &Handle,
                    KEY_READ | KEY_WRITE,
                    &ObjectAttributes,
                    TITLE_INDEX_VALUE,
                    &(CmClassName[Component->Class]),
                    0,
                    &Disposition
                    );

        NtClose(OldHandle);

        if (!NT_SUCCESS(Status)) {
            return(Status);
        }

        ASSERT(Disposition == REG_CREATED_NEW_KEY);
    }

    //
    // Create a value which describes the following component information:
    //     Flags, Cersion, Key, AffinityMask.
    //

    RtlInitUnicodeString(
        &ValueName,
        L"Component Information"
        );

    Status = NtSetValueKey(
                Handle,
                &ValueName,
                TITLE_INDEX_VALUE,
                REG_BINARY,
                &Component->Flags,
                FIELD_OFFSET(CONFIGURATION_COMPONENT, ConfigurationDataLength) -
                    FIELD_OFFSET(CONFIGURATION_COMPONENT, Flags)
                );

    if (!NT_SUCCESS(Status)) {
        NtClose(Handle);
        return(Status);
    }

    //
    // Create a value which describes the component identifier, if any.
    //

    if (Component->IdentifierLength) {

        RtlInitUnicodeString(
            &ValueName,
            L"Identifier"
            );

        RtlInitAnsiString(
            &AnsiString,
            Component->Identifier
            );

        RtlAnsiStringToUnicodeString(
            &ValueData,
            &AnsiString,
            TRUE
            );

        Status = NtSetValueKey(
                    Handle,
                    &ValueName,
                    TITLE_INDEX_VALUE,
                    REG_SZ,
                    ValueData.Buffer,
                    ValueData.Length + sizeof( UNICODE_NULL )
                    );

        RtlFreeUnicodeString(&ValueData);

        if (!NT_SUCCESS(Status)) {
            NtClose(Handle);
            return(Status);
        }
    }

    //
    // Create a value entry for component configuration data.
    //

    RtlInitUnicodeString(
        &ValueName,
        L"Configuration Data"
        );

    //
    // Create the configuration data based on CM_FULL_RESOURCE_DESCRIPTOR.
    //
    // Note the configuration data in firmware tree may be in the form of
    // CM_PARTIAL_RESOURCE_LIST or nothing.  In both cases, we need to
    // set up the registry configuration data to be in the form of
    // CM_FULL_RESOURCE_DESCRIPTOR.
    //

    if (CurrentEntry->ConfigurationData) {

        //
        // This component has configuration data, we copy the data
        // to our work area, add some more data items and copy the new
        // configuration data to the registry.
        //

        ConfigurationDataLength = Component->ConfigurationDataLength +
                      FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR,
                      PartialResourceList);

        //
        // Make sure our reserved area is big enough to hold the data.
        //

        if (ConfigurationDataLength > CmpConfigurationAreaSize) {

            //
            // If reserved area is not big enough, we resize our reserved
            // area.  If, unfortunately, the reallocation fails, we simply
            // loss the configuration data of this particular component.
            //

            NewArea = (PCM_FULL_RESOURCE_DESCRIPTOR)ExAllocatePool(
                                            PagedPool,
                                            ConfigurationDataLength
                                            );

            if (NewArea) {
                CmpConfigurationAreaSize = ConfigurationDataLength;
                ExFreePool(CmpConfigurationData);
                CmpConfigurationData = NewArea;
                RtlMoveMemory(
                    (PUCHAR)&CmpConfigurationData->PartialResourceList.Version,
                    CurrentEntry->ConfigurationData,
                    Component->ConfigurationDataLength
                    );
            } else {
                Component->ConfigurationDataLength = 0;
                CurrentEntry->ConfigurationData = NULL;
            }
        } else {
            RtlMoveMemory(
                (PUCHAR)&CmpConfigurationData->PartialResourceList.Version,
                CurrentEntry->ConfigurationData,
                Component->ConfigurationDataLength
                );
        }

    }

    if (CurrentEntry->ConfigurationData == NULL) {

        //
        // This component has NO configuration data (or we can't resize
        // our reserved area to hold the data), we simple add whatever
        // is required to set up a CM_FULL_RESOURCE_LIST.
        //

        CmpConfigurationData->PartialResourceList.Version = 0;
        CmpConfigurationData->PartialResourceList.Revision = 0;
        CmpConfigurationData->PartialResourceList.Count = 0;
        ConfigurationDataLength = FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR,
                                               PartialResourceList) +
                                  FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST,
                                               PartialDescriptors);
    }

    //
    // Set up InterfaceType and BusNumber for the component.
    //

    CmpConfigurationData->InterfaceType = InterfaceType;
    CmpConfigurationData->BusNumber = BusNumber;

    //
    // Write the newly constructed configuration data to the hardware registry
    //

    Status = NtSetValueKey(
                Handle,
                &ValueName,
                TITLE_INDEX_VALUE,
                REG_FULL_RESOURCE_DESCRIPTOR,
                CmpConfigurationData,
                ConfigurationDataLength
                );

    if (!NT_SUCCESS(Status)) {
        NtClose(Handle);
        return(Status);
    }

    *NewHandle = Handle;
    return(STATUS_SUCCESS);

}
