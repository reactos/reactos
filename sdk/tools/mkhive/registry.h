/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/registry.h
 * PURPOSE:         Registry code
 */

#pragma once

typedef struct _REPARSE_POINT
{
    LIST_ENTRY ListEntry;
    PCMHIVE SourceHive;
    HCELL_INDEX SourceKeyCellOffset;
    PCMHIVE DestinationHive;
    HCELL_INDEX DestinationKeyCellOffset;
} REPARSE_POINT, *PREPARSE_POINT;

typedef struct _MEMKEY
{
    /* Information on hard disk structure */
    HCELL_INDEX KeyCellOffset;
    PCMHIVE RegistryHive;
} MEMKEY, *PMEMKEY;

#define HKEY_TO_MEMKEY(hKey) ((PMEMKEY)(hKey))
#define MEMKEY_TO_HKEY(memKey) ((HKEY)(memKey))

typedef struct _HIVE_LIST_ENTRY
{
    PCSTR   HiveName;
    PCWSTR  HiveRegistryPath;
    PCMHIVE CmHive;
    PUCHAR  SecurityDescriptor;
    ULONG   SecurityDescriptorLength;
} HIVE_LIST_ENTRY, *PHIVE_LIST_ENTRY;

#define MAX_NUMBER_OF_REGISTRY_HIVES    7
extern HIVE_LIST_ENTRY RegistryHives[];

#define ERROR_SUCCESS                    0L
#define ERROR_UNSUCCESSFUL               1L
#define ERROR_FILE_NOT_FOUND             2L
#define ERROR_OUTOFMEMORY                14L
#define ERROR_INVALID_PARAMETER          87L
#define ERROR_MORE_DATA                  234L
#define ERROR_NO_MORE_ITEMS              259L

#define REG_NONE                           0
#define REG_SZ                             1
#define REG_EXPAND_SZ                      2
#define REG_BINARY                         3
#define REG_DWORD                          4
#define REG_DWORD_LITTLE_ENDIAN            4
#define REG_DWORD_BIG_ENDIAN               5
#define REG_LINK                           6
#define REG_MULTI_SZ                       7
#define REG_RESOURCE_LIST                  8
#define REG_FULL_RESOURCE_DESCRIPTOR       9
#define REG_RESOURCE_REQUIREMENTS_LIST     10
#define REG_QWORD                          11
#define REG_QWORD_LITTLE_ENDIAN            11

VOID
RegInitializeRegistry(
    IN PCSTR HiveList);

VOID
RegShutdownRegistry(VOID);

/* EOF */
