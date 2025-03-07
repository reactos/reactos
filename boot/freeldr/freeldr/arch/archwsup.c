/*
 * PROJECT:         ReactOS Boot Loader (FreeLDR)
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/freeldr/arch/archwsup.c
 * PURPOSE:         Routines for ARC Hardware Tree and Configuration Data
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);

/* GLOBALS ********************************************************************/

/* Strings corresponding to CONFIGURATION_TYPE */
const PCSTR ArcTypes[MaximumType + 1] = // CmTypeName
{
    "System",
    "CentralProcessor",
    "FloatingPointProcessor",
    "PrimaryICache",
    "PrimaryDCache",
    "SecondaryICache",
    "SecondaryDCache",
    "SecondaryCache",
    "EisaAdapter",
    "TcAdapter",
    "ScsiAdapter",
    "DtiAdapter",
    "MultifunctionAdapter",
    "DiskController",
    "TapeController",
    "CdRomController",
    "WormController",
    "SerialController",
    "NetworkController",
    "DisplayController",
    "ParallelController",
    "PointerController",
    "KeyboardController",
    "AudioController",
    "OtherController",
    "DiskPeripheral",
    "FloppyDiskPeripheral",
    "TapePeripheral",
    "ModemPeripheral",
    "MonitorPeripheral",
    "PrinterPeripheral",
    "PointerPeripheral",
    "KeyboardPeripheral",
    "TerminalPeripheral",
    "OtherPeripheral",
    "LinePeripheral",
    "NetworkPeripheral",
    "SystemMemory",
    "DockingInformation",
    "RealModeIrqRoutingTable",
    "RealModePCIEnumeration",
    "Undefined"
};

PCONFIGURATION_COMPONENT_DATA FldrArcHwTreeRoot;

// ARC Disk Information
ULONG reactos_disk_count = 0;
ARC_DISK_SIGNATURE_EX reactos_arc_disk_info[32];

/* FUNCTIONS ******************************************************************/

#define TAG_HW_COMPONENT_DATA   'DCwH'
#define TAG_HW_NAME             'mNwH'

VOID
AddReactOSArcDiskInfo(
    IN PSTR ArcName,
    IN ULONG Signature,
    IN ULONG Checksum,
    IN BOOLEAN ValidPartitionTable)
{
    ASSERT(reactos_disk_count < sizeof(reactos_arc_disk_info)/sizeof(reactos_arc_disk_info[0]));

    /* Fill out the ARC disk block */

    reactos_arc_disk_info[reactos_disk_count].DiskSignature.Signature = Signature;
    reactos_arc_disk_info[reactos_disk_count].DiskSignature.CheckSum = Checksum;
    reactos_arc_disk_info[reactos_disk_count].DiskSignature.ValidPartitionTable = ValidPartitionTable;

    strcpy(reactos_arc_disk_info[reactos_disk_count].ArcName, ArcName);
    reactos_arc_disk_info[reactos_disk_count].DiskSignature.ArcName =
        reactos_arc_disk_info[reactos_disk_count].ArcName;

    reactos_disk_count++;
}

//
// ARC Component Configuration Routines
//

static VOID
FldrSetIdentifier(
    _In_ PCONFIGURATION_COMPONENT Component,
    _In_ PCSTR IdentifierString)
{
    SIZE_T IdentifierLength;
    PCHAR Identifier;

    /* Allocate memory for the identifier */
    IdentifierLength = strlen(IdentifierString) + 1;
    Identifier = FrLdrHeapAlloc(IdentifierLength, TAG_HW_NAME);
    if (!Identifier) return;

    /* Copy the identifier */
    RtlCopyMemory(Identifier, IdentifierString, IdentifierLength);

    /* Set component information */
    Component->IdentifierLength = (ULONG)IdentifierLength;
    Component->Identifier = Identifier;
}

VOID
FldrSetConfigurationData(
    _Inout_ PCONFIGURATION_COMPONENT_DATA ComponentData,
    _In_ PCM_PARTIAL_RESOURCE_LIST ResourceList,
    _In_ ULONG Size)
{
    /* Set component information */
    ComponentData->ConfigurationData = ResourceList;
    ComponentData->ComponentEntry.ConfigurationDataLength = Size;
}

VOID
FldrCreateSystemKey(
    _Out_ PCONFIGURATION_COMPONENT_DATA* SystemNode,
    _In_ PCSTR IdentifierString)
{
    PCONFIGURATION_COMPONENT Component;

    /* Allocate the root */
    FldrArcHwTreeRoot = FrLdrHeapAlloc(sizeof(CONFIGURATION_COMPONENT_DATA),
                                       TAG_HW_COMPONENT_DATA);
    if (!FldrArcHwTreeRoot) return;

    /* Set it up */
    Component = &FldrArcHwTreeRoot->ComponentEntry;
    Component->Class = SystemClass;
    Component->Type = MaximumType;
    Component->ConfigurationDataLength = 0;
    Component->Identifier = 0;
    Component->IdentifierLength = 0;
    Component->Flags = 0;
    Component->Version = 0;
    Component->Revision = 0;
    Component->Key = 0;
    Component->AffinityMask = 0xFFFFFFFF;

    /* Set identifier */
    if (IdentifierString)
        FldrSetIdentifier(Component, IdentifierString);

    /* Return the node */
    *SystemNode = FldrArcHwTreeRoot;
}

static VOID
FldrLinkToParent(
    _In_ PCONFIGURATION_COMPONENT_DATA Parent,
    _In_ PCONFIGURATION_COMPONENT_DATA Child)
{
    PCONFIGURATION_COMPONENT_DATA Sibling;

    /* Get the first sibling */
    Sibling = Parent->Child;

    /* If no sibling exists, then we are the first child */
    if (!Sibling)
    {
        /* Link us in */
        Parent->Child = Child;
    }
    else
    {
        /* Loop each sibling */
        do
        {
            /* This is now the parent */
            Parent = Sibling;
        } while ((Sibling = Sibling->Sibling));

        /* Found the lowest sibling; mark us as its sibling too */
        Parent->Sibling = Child;
    }
}

VOID
FldrCreateComponentKey(
    _In_ PCONFIGURATION_COMPONENT_DATA SystemNode,
    _In_ CONFIGURATION_CLASS Class,
    _In_ CONFIGURATION_TYPE Type,
    _In_ IDENTIFIER_FLAG Flags,
    _In_ ULONG Key,
    _In_ ULONG Affinity,
    _In_ PCSTR IdentifierString,
    _In_ PCM_PARTIAL_RESOURCE_LIST ResourceList,
    _In_ ULONG Size,
    _Out_ PCONFIGURATION_COMPONENT_DATA* ComponentKey)
{
    PCONFIGURATION_COMPONENT_DATA ComponentData;
    PCONFIGURATION_COMPONENT Component;

    /* Allocate the node for this component */
    ComponentData = FrLdrHeapAlloc(sizeof(CONFIGURATION_COMPONENT_DATA),
                                   TAG_HW_COMPONENT_DATA);
    if (!ComponentData) return;

    /* Now save our parent */
    ComponentData->Parent = SystemNode;

    /* Link us to the parent */
    if (SystemNode)
        FldrLinkToParent(SystemNode, ComponentData);

    /* Set us up */
    Component = &ComponentData->ComponentEntry;
    Component->Class = Class;
    Component->Type = Type;
    Component->Flags = Flags;
    Component->Key = Key;
    Component->AffinityMask = Affinity;

    /* Set identifier */
    if (IdentifierString)
        FldrSetIdentifier(Component, IdentifierString);

    /* Set configuration data */
    if (ResourceList)
        FldrSetConfigurationData(ComponentData, ResourceList, Size);

    TRACE("Created key: %s\\%d\n", ArcTypes[min(Type, MaximumType)], Key);

    /* Return the child */
    *ComponentKey = ComponentData;
}

ULONG ArcGetDiskCount(VOID)
{
    return reactos_disk_count;
}

PARC_DISK_SIGNATURE_EX ArcGetDiskInfo(ULONG Index)
{
    if (Index >= reactos_disk_count)
    {
        return NULL;
    }

    return &reactos_arc_disk_info[Index];
}
