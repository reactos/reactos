/*
 * PROJECT:         ReactOS Boot Loader (FreeLDR)
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/freeldr/reactos/archwsup.c
 * PURPOSE:         Routines for ARC Hardware Tree and Configuration Data
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCONFIGURATION_COMPONENT_DATA FldrArcHwTreeRoot;
ULONG FldrBusTypeCount[MaximumType + 1] = {0};
const PWCHAR FldrBusTypeString[MaximumType + 1] =
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
const PWCHAR FldrClassString[MaximumClass + 1] =
{
    L"System",
    L"Processor",
    L"Cache",
    L"Adapter",
    L"Controller",
    L"Peripheral",
    L"MemoryClass",
    L"Undefined"
};

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
FldrSetComponentInformation(IN FRLDRHKEY ComponentKey,
                            IN IDENTIFIER_FLAG Flags,
                            IN ULONG Key,
                            IN ULONG Affinity)
{
    LONG Error;
    CONFIGURATION_COMPONENT_DATA Data = {0}; // This would be "ComponentKey"
    PCONFIGURATION_COMPONENT_DATA ComponentData = &Data;
    PCONFIGURATION_COMPONENT Component = &ComponentData->ComponentEntry;
    
    /* Set component information */
    Component->Flags = Flags;
    Component->Version = 0;
    Component->Revision = 0;
    Component->Key = Key;
    Component->AffinityMask = Affinity;
    
    /* Set the value */
    Error = RegSetValue(ComponentKey,
                        L"Component Information",
                        REG_BINARY,
                        (PVOID)&Component->Flags,
                        FIELD_OFFSET(CONFIGURATION_COMPONENT, ConfigurationDataLength) -
                        FIELD_OFFSET(CONFIGURATION_COMPONENT, Flags));
    if (Error != ERROR_SUCCESS)
    {
        DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", Error));
    }
}

VOID
NTAPI
FldrSetIdentifier(IN FRLDRHKEY ComponentKey,
                  IN PWCHAR Identifier)
{
    LONG Error;
    ULONG IdentifierLength = (wcslen(Identifier) + 1) * sizeof(WCHAR);
    CONFIGURATION_COMPONENT_DATA Data = {0}; // This would be "ComponentKey"
    PCONFIGURATION_COMPONENT_DATA ComponentData = &Data;
    PCONFIGURATION_COMPONENT Component = &ComponentData->ComponentEntry;
    
    /* Set component information */
    Component->IdentifierLength = IdentifierLength;
    Component->Identifier = (PCHAR)Identifier; // We need to use ASCII instead

    /* Set the key */
    Error = RegSetValue(ComponentKey,
                        L"Identifier",
                        REG_SZ,
                        (PCHAR)Identifier,
                        IdentifierLength);
    if (Error != ERROR_SUCCESS)
    {
        DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", Error));
        return;
    }
}

VOID
NTAPI
FldrCreateSystemKey(OUT FRLDRHKEY *SystemKey)
{
    LONG Error;
    PCONFIGURATION_COMPONENT Component;
    
    /* Allocate the root */
    FldrArcHwTreeRoot = MmAllocateMemory(sizeof(CONFIGURATION_COMPONENT_DATA));
    if (!FldrArcHwTreeRoot) return;
    
    /* Set it up */
    Component = &FldrArcHwTreeRoot->ComponentEntry;
    Component->Class = SystemClass;
    Component->Type = MaximumType;
    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0;
    Component->ConfigurationDataLength = 0;
    Component->Identifier = 0;
    Component->IdentifierLength = 0;
    
    /* Create the key */
    Error = RegCreateKey(NULL,
                         L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System",
                         SystemKey);
    if (Error != ERROR_SUCCESS)
    {
        DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", Error));
        return;
    }
}

VOID
NTAPI
FldrCreateComponentKey(IN FRLDRHKEY SystemKey,
                       IN PWCHAR BusName,
                       IN ULONG BusNumber,
                       IN CONFIGURATION_CLASS Class,
                       IN CONFIGURATION_TYPE Type,
                       OUT FRLDRHKEY *ComponentKey)
{
    LONG Error;
    WCHAR Buffer[80];
    CONFIGURATION_COMPONENT_DATA Root = {0}; // This would be "SystemKey"
    PCONFIGURATION_COMPONENT_DATA SystemNode = &Root;
    PCONFIGURATION_COMPONENT_DATA ComponentData;
    PCONFIGURATION_COMPONENT Component;
    
    /* Allocate the node for this component */
    ComponentData = MmAllocateMemory(sizeof(CONFIGURATION_COMPONENT_DATA));
    if (!ComponentData) return;
    
    /* Now save our parent */
    ComponentData->Parent = SystemNode;
    
    /* Now we need to figure out if the parent already has a child entry */
    if (SystemNode->Child)
    {
        /* It does, so we'll be a sibling of the child instead */
        SystemNode->Child->Sibling = ComponentData;
    }
    else
    {
        /* It doesn't, so we will be the first child */
        SystemNode->Child = ComponentData;
    }
    
    /* Set us up (need to use class/type instead of name/number) */
    Component = &FldrArcHwTreeRoot->ComponentEntry;
    Component->Class = Class;
    Component->Type = Type;
    
    /* FIXME: Use Class/Type to build key name */
    
    /* Build the key name */
    swprintf(Buffer, L"%s\\%u", BusName, BusNumber);
    
    /* Create the key */
    Error = RegCreateKey(SystemKey, Buffer, ComponentKey);
    if (Error != ERROR_SUCCESS)
    {
        DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", Error));
        return;
    }    
}

VOID
NTAPI
FldrSetConfigurationData(IN FRLDRHKEY ComponentKey,
                         IN PVOID ConfigurationData,
                         IN ULONG Size)
{
    LONG Error;
    CONFIGURATION_COMPONENT_DATA Data = {0}; // This would be "ComponentKey"
    PCONFIGURATION_COMPONENT_DATA ComponentData = &Data;
    PCONFIGURATION_COMPONENT Component = &ComponentData->ComponentEntry;
    
    /* Set component information */
    ComponentData->ConfigurationData = ConfigurationData;
    Component->ConfigurationDataLength = Size;
    
    /* Set 'Configuration Data' value */
    Error = RegSetValue(ComponentKey,
                        L"Configuration Data",
                        REG_FULL_RESOURCE_DESCRIPTOR,
                        ConfigurationData,
                        Size);
    if (Error != ERROR_SUCCESS)
    {
        DbgPrint((DPRINT_HWDETECT,
                  "RegSetValue(Configuration Data) failed (Error %u)\n",
                  Error));
    }    
}
