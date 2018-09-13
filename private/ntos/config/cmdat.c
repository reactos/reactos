/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    cmdat.c

Abstract:

    This module contains registry "static" data, except for data
    also used by setup, which is in cmdat2.c.

Author:

    Bryan Willman (bryanwi) 19-Oct-93


Environment:

    Kernel mode.

Revision History:

--*/

#include "cmp.h"

//
// ***** INIT *****
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#endif

//
// ---------------------------
//


UNICODE_STRING  CmpLoadOptions = { 0 };        // sys options from FW or boot.ini


//
// CmpClassString - contains strings which are used as the class
//     strings in the keynode.
// The associated enumerated type is CONFIGURATION_CLASS in arc.h
//

UNICODE_STRING CmClassName[MaximumClass + 1] = { 0 };

PWCHAR CmClassString[MaximumClass + 1] = {
    L"System",
    L"Processor",
    L"Cache",
    L"Adapter",
    L"Controller",
    L"Peripheral",
    L"MemoryClass",
    L"Undefined"
    };


struct {
    PUCHAR  AscString;
    USHORT  InterfaceType;
    USHORT  Count;
} CmpMultifunctionTypes[] = {
    "ISA",      Isa,            0,
    "MCA",      MicroChannel,   0,
    "PCI",      PCIBus,         0,
    "VME",      VMEBus,         0,
    "PCMCIA",   PCMCIABus,      0,
    "CBUS",     CBus,           0,
    "MPIPI",    MPIBus,         0,
    "MPSA",     MPSABus,        0,
    NULL,       Internal,       0
};


USHORT CmpUnknownBusCount = 0;

ULONG CmpConfigurationAreaSize = 0x4000;        // Initialize size = 16K
PCM_FULL_RESOURCE_DESCRIPTOR CmpConfigurationData = { 0 };


#ifdef ALLOC_DATA_PRAGMA
#pragma  data_seg()
#endif


//
// ***** PAGE *****
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

//
// The following strings will be used as the keynames for registry
// nodes.
// The associated enumerated type is CONFIGURATION_TYPE in arc.h
//

UNICODE_STRING CmTypeName[MaximumType + 1] = { 0 };

//
// CmpTypeCount[] - For each 'type', a count is used to keep track how many
//     keys have been created.
//

ULONG CmpTypeCount[NUMBER_TYPES] = {
            0,                  // ArcSystem
            0,                  // CentralProcessor",
            0,                  // FloatingPointProcessor",
            0,                  // PrimaryICache",
            0,                  // PrimaryDCache",
            0,                  // SecondaryICache",
            0,                  // SecondaryDCache",
            0,                  // SecondaryCache",
            0,                  // EisaAdapter", (8)
            0,                  // TcAdapter",   (9)
            0,                  // ScsiAdapter",
            0,                  // DtiAdapter",
            0,                  // MultifunctionAdapter", (12)
            0,                  // DiskController", (13)
            0,                  // TapeController",
            0,                  // CdRomController",
            0,                  // WormController",
            0,                  // SerialController",
            0,                  // NetworkController",
            0,                  // DisplayController",
            0,                  // ParallelController",
            0,                  // PointerController",
            0,                  // KeyboardController",
            0,                  // AudioController",
            0,                  // OtherController",
            0,                  // DiskPeripheral",
            0,                  // FloppyDiskPeripheral",
            0,                  // TapePeripheral",
            0,                  // ModemPeripheral",
            0,                  // MonitorPeripheral",
            0,                  // PrinterPeripheral",
            0,                  // PointerPeripheral",
            0,                  // KeyboardPeripheral",
            0,                  // TerminalPeripheral",
            0,                  // OtherPeripheral",
            0,                  // LinePeripheral",
            0,                  // NetworkPeripheral",
            0,                  // SystemMemory",
            0,                  // DockingInformation,
            0,					// RealModeIrqRoutingTable
            0                   // Undefined"
            };

UNICODE_STRING nullclass = { 0, 0, NULL };

//
// All names used by the registry
//


UNICODE_STRING CmRegistryRootName = { 0 };
UNICODE_STRING CmRegistryMachineName = { 0 };
UNICODE_STRING CmRegistryMachineHardwareName = { 0 };
UNICODE_STRING CmRegistryMachineHardwareDescriptionName = { 0 };
UNICODE_STRING CmRegistryMachineHardwareDescriptionSystemName = { 0 };
UNICODE_STRING CmRegistryMachineHardwareDeviceMapName = { 0 };
UNICODE_STRING CmRegistryMachineHardwareResourceMapName = { 0 };
UNICODE_STRING CmRegistryMachineHardwareOwnerMapName = { 0 };
UNICODE_STRING CmRegistryMachineSystemName = { 0 };
UNICODE_STRING CmRegistryMachineSystemCurrentControlSet = { 0 };
UNICODE_STRING CmRegistryMachineSystemCurrentControlSetEnumName = { 0 };
UNICODE_STRING CmRegistryMachineSystemCurrentControlSetEnumRootName = { 0 };
UNICODE_STRING CmRegistryMachineSystemCurrentControlSetServices = { 0 };
UNICODE_STRING CmRegistryMachineSystemCurrentControlSetHardwareProfilesCurrent = { 0 };
UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlClass = { 0 };
UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlSafeBoot = { 0 };
UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlSessionManagerMemoryManagement = { 0 };
UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlBootLog = { 0 };
UNICODE_STRING CmRegistryMachineSystemCurrentControlSetServicesEventLog = { 0 };
UNICODE_STRING CmRegistryUserName = { 0 };
UNICODE_STRING CmRegistrySystemCloneName = { 0 };
UNICODE_STRING CmpSystemFileName = { 0 };
UNICODE_STRING CmSymbolicLinkValueName = { 0 };

#ifdef _WANT_MACHINE_IDENTIFICATION
UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlBiosInfo = { 0 };
#endif

PWCHAR CmpRegistryRootString = L"\\REGISTRY";
PWCHAR CmpRegistryMachineString = L"\\REGISTRY\\MACHINE";
PWCHAR CmpRegistryMachineHardwareString =
                    L"\\REGISTRY\\MACHINE\\HARDWARE";
PWCHAR CmpRegistryMachineHardwareDescriptionString =
                    L"\\REGISTRY\\MACHINE\\HARDWARE\\DESCRIPTION";
PWCHAR CmpRegistryMachineHardwareDescriptionSystemString =
                    L"\\REGISTRY\\MACHINE\\HARDWARE\\DESCRIPTION\\SYSTEM";
PWCHAR CmpRegistryMachineHardwareDeviceMapString =
                    L"\\REGISTRY\\MACHINE\\HARDWARE\\DEVICEMAP";
PWCHAR CmpRegistryMachineHardwareResourceMapString =
                    L"\\REGISTRY\\MACHINE\\HARDWARE\\RESOURCEMAP";
PWCHAR CmpRegistryMachineHardwareOwnerMapString =
                    L"\\REGISTRY\\MACHINE\\HARDWARE\\OWNERMAP";
PWCHAR CmpRegistryMachineSystemString =
                    L"\\REGISTRY\\MACHINE\\SYSTEM";
PWCHAR CmpRegistryMachineSystemCurrentControlSetString =
                    L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET";
PWCHAR CmpRegistryMachineSystemCurrentControlSetEnumString =
                    L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\ENUM";
PWCHAR CmpRegistryMachineSystemCurrentControlSetEnumRootString =
                    L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\ENUM\\ROOT";
PWCHAR CmpRegistryMachineSystemCurrentControlSetServicesString =
                    L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\SERVICES";
PWCHAR CmpRegistryMachineSystemCurrentControlSetHardwareProfilesCurrentString =
                    L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\HARDWARE PROFILES\\CURRENT";
PWCHAR CmpRegistryMachineSystemCurrentControlSetControlClassString =
                    L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\CONTROL\\CLASS";
PWCHAR CmpRegistryMachineSystemCurrentControlSetControlSafeBootString =
                    L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\CONTROL\\SAFEBOOT";
PWCHAR CmpRegistryMachineSystemCurrentControlSetControlSessionManagerMemoryManagementString =
                    L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\CONTROL\\SESSION MANAGER\\MEMORY MANAGEMENT";
PWCHAR CmpRegistryMachineSystemCurrentControlSetControlBootLogString =
                    L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\CONTROL\\BOOTLOG";
PWCHAR CmpRegistryMachineSystemCurrentControlSetServicesEventLogString =
                    L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\SERVICES\\EVENTLOG";
PWCHAR CmpRegistryUserString = L"\\REGISTRY\\USER";
PWCHAR CmpRegistrySystemCloneString = L"\\REGISTRY\\MACHINE\\CLONE";
PWCHAR CmpRegistrySystemFileNameString = L"SYSTEM";
PWCHAR CmpRegistryPerflibString = L"\\REGISTRY\\MACHINE\\SOFTWARE\\MICROSOFT\\WINDOWS NT\\CURRENTVERSION\\PERFLIB";

PWCHAR CmpProcessorControl = L"ProcessorControl";
PWCHAR CmpControlSessionManager = L"Control\\Session Manager";
PWCHAR CmpSymbolicLinkValueName = L"SymbolicLinkValue";

#ifdef _WANT_MACHINE_IDENTIFICATION
PWCHAR CmpRegistryMachineSystemCurrentControlSetControlBiosInfoString =
                    L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\CONTROL\\BIOSINFO";
#endif

//
// N.B. The CLONE hive is left out of the machine Hive list if 
//      we will not be using it to clone the current control set, 
//      since that is that Hive's only purpose.
//

HIVE_LIST_ENTRY CmpMachineHiveList[] = {
    { L"HARDWARE", L"MACHINE\\", NULL, HIVE_VOLATILE },
    { L"SECURITY", L"MACHINE\\", NULL, 0},
    { L"SOFTWARE", L"MACHINE\\", NULL, 0},
    { L"SYSTEM",   L"MACHINE\\", NULL, 0},
    { L"DEFAULT",  L"USER\\.DEFAULT", NULL, 0},
    { L"SAM",      L"MACHINE\\", NULL, HIVE_NOLAZYFLUSH },

#if CLONE_CONTROL_SET
    { L"CLONE",    L"MACHINE\\", NULL, HIVE_VOLATILE },
#endif

//  { L"TEST",     L"MACHINE\\", NULL, HIVE_NOLAZYFLUSH },
    { NULL,        NULL,         0, 0}
    };



//
// Master Hive
//
//  The KEY_NODEs for \REGISTRY, \REGISTRY\MACHINE, and \REGISTRY\USER
//  are stored in a small memory only hive called the Master Hive.
//  All other hives have link nodes in this hive which point to them.
//
PCMHIVE CmpMasterHive = { 0 };
BOOLEAN CmpNoMasterCreates = FALSE;     // Set TRUE after we're done to
                                        // prevent random creates in the
                                        // master hive, which is not backed
                                        // by a file.


LIST_ENTRY  CmpHiveListHead = { 0 };            // List of CMHIVEs


//
// Addresses of object type descriptors:
//

POBJECT_TYPE CmpKeyObjectType = { 0 };

//
// Write-Control:
//  CmpNoWrite is initially true.  When set this way write and flush
//  do nothing, simply returning success.  When cleared to FALSE, I/O
//  is enabled.  This change is made after the I/O system is started
//  AND autocheck (chkdsk) has done its thing.
//

BOOLEAN CmpNoWrite = TRUE;


//
// NtInitializeRegistry global status flags
//

// 
// If CmFirstTime is TRUE, then NtInitializeRegistry has not yet been
// called to perform basic registry initialization
//

BOOLEAN CmFirstTime = TRUE;       

//
// If CmBootAcceptFirstTime is TRUE, then NtInitializeRegistry has not 
// yet been called to accept the current Boot and save the boot
// control set as the LKG control set.
//

BOOLEAN CmBootAcceptFirstTime = TRUE;   

//
// CmpWasSetupBoot indicates whether or not the boot
// is into text mode setup.  If so, we do not turn
// on global quotas.
//
BOOLEAN CmpWasSetupBoot;

#ifdef ALLOC_DATA_PRAGMA
#pragma  data_seg()
#endif


//
// ***** FIXED *****
//


PWCHAR CmTypeString[MaximumType + 1] = {
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
    L"Undefined"
    };
