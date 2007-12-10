/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmconfig.c
 * PURPOSE:         Configuration Manager - System Configuration Routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#include "cm.h"
#define NDEBUG
#include "debug.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
CmpInitializeRegistryNode(IN PCONFIGURATION_COMPONENT_DATA CurrentEntry,
                          IN HANDLE NodeHandle,
                          OUT PHANDLE NewHandle,
                          IN INTERFACE_TYPE InterfaceType,
                          IN ULONG BusNumber,
                          IN PUSHORT DeviceIndexTable)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName, ValueName, ValueData;
    HANDLE KeyHandle, ParentHandle;
    ANSI_STRING TempString;
    CHAR TempBuffer[12];
    WCHAR Buffer[12];
    PCONFIGURATION_COMPONENT Component;
    ULONG Disposition, Length = 0;

    /* Get the component */
    Component = &CurrentEntry->ComponentEntry;

    /* Set system class components to ARC system type */
    if (Component->Class == SystemClass) Component->Type = ArcSystem;

    /* Create a key for the component */
    InitializeObjectAttributes(&ObjectAttributes,
                               &CmTypeName[Component->Type],
                               OBJ_CASE_INSENSITIVE,
                               NodeHandle,
                               NULL);
    Status = NtCreateKey(&KeyHandle,
                         KEY_READ | KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         &Disposition);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if this is anything but a system class component */
    if (Component->Class != SystemClass)
    {
        /* Build the sub-component string */
        RtlIntegerToChar(DeviceIndexTable[Component->Type]++,
                         10,
                         12,
                         TempBuffer);
        RtlInitAnsiString(&TempString, TempBuffer);

        /* Convert it to Unicode */
        RtlInitEmptyUnicodeString(&KeyName, Buffer, sizeof(Buffer));
        RtlAnsiStringToUnicodeString(&KeyName, &TempString, FALSE);

        /* Create the key */
        ParentHandle = KeyHandle;
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   ParentHandle,
                                   NULL);
        Status = NtCreateKey(&KeyHandle,
                             KEY_READ | KEY_WRITE,
                             &ObjectAttributes,
                             0,
                             NULL,
                             0,
                             &Disposition);
        NtClose(ParentHandle);

        /* Fail if the key couldn't be created, and make sure it's a new key */
        if (!NT_SUCCESS(Status)) return Status;
        //ASSERT(Disposition == REG_CREATED_NEW_KEY);
    }

    /* Setup the component information key */
    RtlInitUnicodeString(&ValueName, L"Component Information");
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_BINARY,
                           &Component->Flags,
                           FIELD_OFFSET(CONFIGURATION_COMPONENT,
                                        ConfigurationDataLength) -
                           FIELD_OFFSET(CONFIGURATION_COMPONENT, Flags));
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        NtClose(KeyHandle);
        return Status;
    }

    /* Check if we have an identifier */
    if (Component->IdentifierLength)
    {
        /* Build the string and convert it to Unicode */
        RtlInitUnicodeString(&ValueName, L"Identifier");
        RtlInitAnsiString(&TempString, Component->Identifier);
        Status = RtlAnsiStringToUnicodeString(&ValueData,
                                              &TempString,
                                              TRUE);
        RtlCreateUnicodeString(&ValueData, (PWCHAR)Component->Identifier);
        if (NT_SUCCESS(Status))
        {
            /* Save the identifier in the registry */
            Status = NtSetValueKey(KeyHandle,
                                   &ValueName,
                                   0,
                                   REG_SZ,
                                   ValueData.Buffer,
                                   ValueData.Length + sizeof(UNICODE_NULL));
            RtlFreeUnicodeString(&ValueData);
        }

        /* Check for failure during conversion or registry write */
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            NtClose(KeyHandle);
            return Status;
        }
    }

    /* Setup the configuration data string */
    RtlInitUnicodeString(&ValueName, L"Configuration Data");

    /* Check if we got configuration data */
    if (CurrentEntry->ConfigurationData)
    {
        /* Calculate the total length and check if it fits into our buffer */
        Length = Component->ConfigurationDataLength +
                 FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList);
        if (Length > CmpConfigurationAreaSize)
        {
            ASSERTMSG("Component too large -- need reallocation!", FALSE);
        }
        else
        {
            /* Copy the data */
            RtlCopyMemory(&CmpConfigurationData->PartialResourceList.Version,
                          CurrentEntry->ConfigurationData,
                          Component->ConfigurationDataLength);
        }
    }
    else
    {
        /* No configuration data, setup defaults */
        CmpConfigurationData->PartialResourceList.Version = 0;
        CmpConfigurationData->PartialResourceList.Revision = 0;
        CmpConfigurationData->PartialResourceList.Count = 0;
        Length = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors) +
                 FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList);
    }

    /* Set the interface type and bus number */
    CmpConfigurationData->InterfaceType = InterfaceType;
    CmpConfigurationData->BusNumber = BusNumber;

    /* Save the actual data */
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_FULL_RESOURCE_DESCRIPTOR,
                           CmpConfigurationData,
                           Length);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        NtClose(KeyHandle);
    }
    else
    {
        /* Return the new handle */
        *NewHandle = KeyHandle;
    }

    /* Return status */
    return Status;
}

int t, tabLevel;

VOID
NTAPI
CmpDumpHardwareTree(IN PCONFIGURATION_COMPONENT_DATA CurrentEntry,
                    IN INTERFACE_TYPE InterfaceType,
                    IN ULONG BusNumber)
{
    PCONFIGURATION_COMPONENT Component;
    USHORT DeviceIndexTable[MaximumType + 1] = {0};
    ULONG Interface = InterfaceType, Bus = BusNumber, i;
    PCHAR InterfaceStrings[MaximumInterfaceType + 1] =
    {
        "Internal",
        "Isa",
        "Eisa",
        "MicroChannel",
        "TurboChannel",
        "PCIBus",
        "VMEBus",
        "NuBus",
        "PCMCIABus",
        "CBus",
        "MPIBus",
        "MPSABus",
        "ProcessorInternal",
        "InternalPowerBus",
        "PNPISABus",
        "PNPBus",
        "Unknown"
    };
        
    while (CurrentEntry)
    {
        for (t = 0; t < tabLevel; t++) DbgPrint("\t");
        DbgPrint("Dumping node @ 0x%p\n", CurrentEntry);
        for (t = 0; t < tabLevel; t++) DbgPrint("\t");
        DbgPrint("Parent @ 0x%p Sibling @ 0x%p Child @ 0x%p\n",
                 CurrentEntry->Parent, CurrentEntry->Sibling, CurrentEntry->Child);
        
        /* Check if this is an adapter */
        Component = &CurrentEntry->ComponentEntry;
        if ((Component->Class == AdapterClass) &&
            (CurrentEntry->Parent->ComponentEntry.Class == SystemClass))
        {
            /* Check what kind of adapter it is */
            switch (Component->Type)
            {
                /* EISA */
                case EisaAdapter:
                    
                    /* Fixup information */
                    Interface = Eisa;
                    Bus = CmpTypeCount[EisaAdapter]++;
                    break;
                
                /* Turbo-channel */
                case TcAdapter:
                    
                    /* Fixup information */
                    Interface = TurboChannel;
                    Bus = CmpTypeCount[TurboChannel]++;
                    break;
                
                /* ISA, PCI, etc busses */
                case MultiFunctionAdapter:
                    
                    /* Check if we have an  identifier */
                    if (Component->Identifier)
                    {
                        /* Loop each multi-function adapter type */
                        for (i = 0; CmpMultifunctionTypes[i].Identifier; i++)
                        {
                            /* Check for a name match */
                            if (!_wcsicmp(CmpMultifunctionTypes[i].Identifier,
                                          (PWCHAR)Component->Identifier))
                            {
                                /* Match found */
                                break;
                            }
                        }
                        
                        /* Fix up information */
                        Interface = CmpMultifunctionTypes[i].InterfaceType;
                        Bus = CmpMultifunctionTypes[i].Count++;
                    }
                    break;
                
                /* SCSI Bus */
                case ScsiAdapter:

                    /* Fix up */
                    Interface = Internal;
                    Bus = CmpTypeCount[ScsiAdapter]++;
                    break;
                
                /* Unknown */
                default:
                    Interface = -1;
                    Bus = CmpUnknownBusCount++;
                    break;
            }
        }

        /* Convert from NT to ARC class */
        if (Component->Class == SystemClass) Component->Type = ArcSystem;
        
        /* Dump information on the component */
        for (t = 0; t < tabLevel; t++) DbgPrint("\t");
        DbgPrint("Component Type: %wZ\n", &CmTypeName[Component->Type]);
        for (t = 0; t < tabLevel; t++) DbgPrint("\t");
        DbgPrint("Class: %wZ\n", &CmClassName[Component->Class]);
        if (Component->Class != SystemClass)
        {
            for (t = 0; t < tabLevel; t++) DbgPrint("\t");
            DbgPrint("Device Index: %lx\n", DeviceIndexTable[Component->Type]++);    
        }
        for (t = 0; t < tabLevel; t++) DbgPrint("\t");
        DbgPrint("Component Information:\n");
        for (t = 0; t < tabLevel; t++) DbgPrint("\t");
        DbgPrint("\tFlags: %lx\n", Component->Flags);
        for (t = 0; t < tabLevel; t++) DbgPrint("\t");
        DbgPrint("\tVersion: %lx\n", Component->Version);
        for (t = 0; t < tabLevel; t++) DbgPrint("\t");
        DbgPrint("\tAffinity: %lx\n", Component->AffinityMask);
        if (Component->IdentifierLength)
        {
            for (t = 0; t < tabLevel; t++) DbgPrint("\t");
            DbgPrint("Identifier: %S\n", Component->Identifier);
        }
        for (t = 0; t < tabLevel; t++) DbgPrint("\t");
        DbgPrint("Configuration Data: %p\n", CurrentEntry->ConfigurationData);    
        for (t = 0; t < tabLevel; t++) DbgPrint("\t");
        DbgPrint("Interface Type: %s Bus Number: %d\n\n",
                 InterfaceStrings[Interface],
                 Bus);
        
        /* Check for children */
        if (CurrentEntry->Child)
        {
            /* Recurse child */
            tabLevel++;
            CmpDumpHardwareTree(CurrentEntry->Child, Interface, Bus);
            tabLevel--;
        }
        
        /* Get to the next entry */
        CurrentEntry = CurrentEntry->Sibling;
    }
}

NTSTATUS
NTAPI
CmpInitializeHardwareConfiguration(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    ULONG Disposition;
    UNICODE_STRING KeyName;

    /* Setup the key name */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\Hardware\\DeviceMap");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Create the device map key */
    Status = NtCreateKey(&KeyHandle,
                         KEY_READ | KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         &Disposition);
    if (!NT_SUCCESS(Status)) return Status;
    NtClose(KeyHandle);

    /* Nobody should've created this key yet! */
    //ASSERT(Disposition == REG_CREATED_NEW_KEY);

    /* Setup the key name */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\Hardware\\Description");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Create the description key */
    Status = NtCreateKey(&KeyHandle,
                          KEY_READ | KEY_WRITE,
                          &ObjectAttributes,
                          0,
                          NULL,
                          0,
                          &Disposition);
    if (!NT_SUCCESS(Status)) return Status;

    /* Nobody should've created this key yet! */
    //ASSERT(Disposition == REG_CREATED_NEW_KEY);

    /* Allocate the configuration data buffer */
    CmpConfigurationData = ExAllocatePoolWithTag(PagedPool,
                                                 CmpConfigurationAreaSize,
                                                 TAG_CM);
    if (!CmpConfigurationData) return STATUS_INSUFFICIENT_RESOURCES;

    /* Check if we got anything from NTLDR */
    if (LoaderBlock->ConfigurationRoot)
    {
        /* Dump the hardware tree */
        DPRINT1("ARC Hardware Tree Received @ 0x%p. Dumping HW Info:\n\n",
                LoaderBlock->ConfigurationRoot);
        CmpDumpHardwareTree(LoaderBlock->ConfigurationRoot, -1, -1);
    }
    else
    {
        /* Nothing else to do */
        Status = STATUS_SUCCESS;
    }

    /* Close our handle, free the buffer and return status */
    ExFreePool(CmpConfigurationData);
    NtClose(KeyHandle);
    return Status;
}


