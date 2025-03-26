/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Configuration Manager - Boot Initialization Internal header
 * COPYRIGHT:   Copyright 2010 ReactOS Portable Systems Group
 *
 * NOTE: This module is shared by both the kernel and the bootloader.
 */

//
// Boot Driver Node
//
typedef struct _BOOT_DRIVER_NODE
{
    BOOT_DRIVER_LIST_ENTRY ListEntry;
    UNICODE_STRING Group;
    UNICODE_STRING Name;
    ULONG Tag;
    ULONG ErrorControl;
} BOOT_DRIVER_NODE, *PBOOT_DRIVER_NODE;


//
// Boot Routines
//
CODE_SEG("INIT")
HCELL_INDEX
NTAPI
CmpFindControlSet(
    _In_ PHHIVE SystemHive,
    _In_ HCELL_INDEX RootCell,
    _In_ PCUNICODE_STRING SelectKeyName,
    _Out_ PBOOLEAN AutoSelect);


//
// Driver List Routines
//
#ifdef _BLDR_

CODE_SEG("INIT")
BOOLEAN
NTAPI
CmpIsDriverInList(
    _In_ PLIST_ENTRY DriverListHead,
    _In_ PCUNICODE_STRING DriverName,
    _Out_opt_ PBOOT_DRIVER_NODE* FoundDriver);

#endif /* _BLDR_ */

CODE_SEG("INIT")
BOOLEAN
NTAPI
CmpFindDrivers(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX ControlSet,
    _In_ SERVICE_LOAD_TYPE LoadType,
    _In_opt_ PCWSTR BootFileSystem,
    _Inout_ PLIST_ENTRY DriverListHead);

CODE_SEG("INIT")
BOOLEAN
NTAPI
CmpSortDriverList(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX ControlSet,
    _Inout_ PLIST_ENTRY DriverListHead);

CODE_SEG("INIT")
BOOLEAN
NTAPI
CmpResolveDriverDependencies(
    _Inout_ PLIST_ENTRY DriverListHead);

CODE_SEG("INIT")
VOID
NTAPI
CmpFreeDriverList(
    _In_ PHHIVE Hive,
    _Inout_ PLIST_ENTRY DriverListHead);
