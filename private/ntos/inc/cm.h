/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    cm.h

Abstract:

    This module contains the internal structure definitions and APIs
    used by the NT configuration management system, including the
    registry.

Author:

    Bryan M. Willman (bryanwi)  28-Aug-91


Revision History:


--*/

#ifndef _CM_
#define _CM_

//
// Define Names used to access the regsitry
//

extern UNICODE_STRING CmRegistryRootName;            // \REGISTRY
extern UNICODE_STRING CmRegistryMachineName;         // \REGISTRY\MACHINE
extern UNICODE_STRING CmRegistryMachineHardwareName; // \REGISTRY\MACHINE\HARDWARE
extern UNICODE_STRING CmRegistryMachineHardwareDescriptionName;
                            // \REGISTRY\MACHINE\HARDWARE\DESCRIPTION
extern UNICODE_STRING CmRegistryMachineHardwareDescriptionSystemName;
                            // \REGISTRY\MACHINE\HARDWARE\DESCRIPTION\SYSTEM
extern UNICODE_STRING CmRegistryMachineHardwareDeviceMapName;
                            // \REGISTRY\MACHINE\HARDWARE\DEVICEMAP
extern UNICODE_STRING CmRegistryMachineHardwareResourceMapName;
                            // \REGISTRY\MACHINE\HARDWARE\RESOURCEMAP
extern UNICODE_STRING CmRegistryMachineHardwareOwnerMapName;
                            // \REGISTRY\MACHINE\HARDWARE\OWNERMAP
extern UNICODE_STRING CmRegistryMachineSystemName;
                            // \REGISTRY\MACHINE\SYSTEM
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSet;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetEnumName;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\ENUM
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetEnumRootName;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\ENUM\ROOT
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetServices;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\SERVICES
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetHardwareProfilesCurrent;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\HARDWARE PROFILES\CURRENT
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlClass;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\CONTROL\CLASS
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlSafeBoot;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\CONTROL\SAFEBOOT
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlSessionManagerMemoryManagement;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\CONTROL\SESSION MANAGER\MEMORY MANAGEMENT
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlBootLog;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\CONTROL\BOOTLOG
extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetServicesEventLog;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\SERVICES\EVENTLOG
extern UNICODE_STRING CmRegistryUserName;            // \REGISTRY\USER

#ifdef _WANT_MACHINE_IDENTIFICATION

extern UNICODE_STRING CmRegistryMachineSystemCurrentControlSetControlBiosInfo;
                            // \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\CONTROL\BIOSINFO

#endif

//
// The following strings will be used as the keynames for registry
// nodes.
// The associated enumerated type is CONFIGURATION_TYPE in arc.h
//

extern UNICODE_STRING CmTypeName[];
extern PWSTR CmTypeString[];

//
// CmpClassString - contains strings which are used as the class
//     strings in the keynode.
// The associated enumerated type is CONFIGURATION_CLASS in arc.h
//

extern UNICODE_STRING CmClassName[];
extern PWSTR CmClassString[];


//
// Define structure of boot driver list.
//

typedef struct _BOOT_DRIVER_LIST_ENTRY {
    LIST_ENTRY Link;
    UNICODE_STRING FilePath;
    UNICODE_STRING RegistryPath;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
} BOOT_DRIVER_LIST_ENTRY, *PBOOT_DRIVER_LIST_ENTRY;

PHANDLE
CmGetSystemDriverList(
    VOID
    );

BOOLEAN
CmInitSystem1(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

BOOLEAN
CmInitSystem2(
    );

VOID
CmNotifyRunDown(
    PETHREAD    Thread
    );

VOID
CmShutdownSystem(
    VOID
    );

VOID
CmBootLastKnownGood(
    ULONG ErrorLevel
    );


//
// Stuctures and definitions for use with CmGetSystemControlValues
//

//
// NOTES:
//      KeyPath is relative to currentcontrolset.  So, if the variable
//      of interest is
//      "\registry\machine\system\currentcontrolset\control\fruit\apple:x"
//      the entry is
//      { L"fruit\\apple",
//        L"x",
//        &Xbuffer,
//        sizeof(ULONG),
//        &Xtype
//      }
//
//      *BufferLength is available space on input
//      on output:
//          -1 = no such key or value
//          0  = key and value exist, but have 0 length data
//          > input = buffer too small, filled to available space,
//                    value is actual size of data in registry
//          <= input = number of bytes copied out
//
typedef struct _CM_SYSTEM_CONTROL_VECTOR {
    PWSTR       KeyPath;                // path name relative to
                                        // current control set
    PWSTR       ValueName;              // name of value entry
    PVOID       Buffer;                 // data goes here
    PULONG      BufferLength;           // IN: space allocated
                                        // OUT: space used, -1 for no such
                                        //      key or value, 0 for key/value
                                        //      found but has 0 length data
                                        // if NULL pointer, assume 4 bytes
                                        // (reg DWORD) available and do not
                                        // report actual size
    PULONG      Type;                   // return type of found data, may
                                        // be NULL
} CM_SYSTEM_CONTROL_VECTOR, *PCM_SYSTEM_CONTROL_VECTOR;

VOID
CmGetSystemControlValues(
    PVOID                   SystemHiveBuffer,
    PCM_SYSTEM_CONTROL_VECTOR  ControlVector
    );

VOID
CmQueryRegistryQuotaInformation(
    IN PSYSTEM_REGISTRY_QUOTA_INFORMATION RegistryQuotaInformation
    );

VOID
CmSetRegistryQuotaInformation(
    IN PSYSTEM_REGISTRY_QUOTA_INFORMATION RegistryQuotaInformation
    );



typedef
VOID
(*PCM_TRACE_NOTIFY_ROUTINE)(
    IN NTSTATUS Status,
    IN HANDLE KeyHandle,
    IN LONGLONG  ElapsedTime,
    IN PUNICODE_STRING KeyName,
    IN UCHAR Type
    );

NTSTATUS
CmSetTraceNotifyRoutine(
    IN PCM_TRACE_NOTIFY_ROUTINE NotifyRoutine,
    IN BOOLEAN Remove
    );


#endif // _CM_
