/*++ 

Copyright (c) 1998  Microsoft Corporation

Module Name:

    smbios.h

Abstract:

    This module contains definitions that describe SMBIOS

Author:

    Alan Warwick (AlanWar) 12-Feb-1998


Revision History:


--*/

#ifndef _SMBIOS_
#define _SMBIOS_

//
// SMBIOS error codes
#define DMI_SUCCESS 0x00
#define DMI_UNKNOWN_FUNCTION 0x81
#define DMI_FUNCTION_NOT_SUPPORTED 0x82
#define DMI_INVALID_HANDLE 0x83
#define DMI_BAD_PARAMETER 0x84
#define DMI_INVALID_SUBFUNCTION 0x85
#define DMI_NO_CHANGE 0x86
#define DMI_ADD_STRUCTURE_FAILED 0x87

// @@BEGIN_DDKSPLIT

//
// SMBIOS registry values
#define SMBIOSPARENTKEYNAME L"\\Registry\\Machine\\Hardware\\Description\\System\\MultifunctionAdapter"

#define SMBIOSIDENTIFIERVALUENAME L"Identifier"
#define SMBIOSIDENTIFIERVALUEDATA L"PNP BIOS"
#define SMBIOSDATAVALUENAME		L"Configuration Data"

#define MAXSMBIOSKEYNAMESIZE 256

// @@END_DDKSPLIT

//
// SMBIOS table search
#define SMBIOS_EPS_SEARCH_SIZE      0x10000
#define SMBIOS_EPS_SEARCH_START     0x000f0000
#define SMBIOS_EPS_SEARCH_INCREMENT 0x10

#pragma pack(push, 1)
typedef struct _SMBIOS_TABLE_HEADER
{
    UCHAR Signature[4];             // _SM_ (ascii)
    UCHAR Checksum;
    UCHAR Length;
    UCHAR MajorVersion;
    UCHAR MinorVersion;
    USHORT MaximumStructureSize;
    UCHAR EntryPointRevision;
    UCHAR Reserved[5];
    UCHAR Signature2[5];           // _DMI_ (ascii)
    UCHAR IntermediateChecksum;
    USHORT StructureTableLength;
    ULONG StructureTableAddress;
    USHORT NumberStructures;
    UCHAR Revision;
} SMBIOS_EPS_HEADER, *PSMBIOS_EPS_HEADER;

#define SMBIOS_EPS_SIGNATURE '_MS_'
#define DMI_EPS_SIGNATURE    'IMD_'

typedef struct _SMBIOS_STRUCT_HEADER
{
    UCHAR Type;
    UCHAR Length;
    USHORT Handle;
    UCHAR Data[];
} SMBIOS_STRUCT_HEADER, *PSMBIOS_STRUCT_HEADER;


typedef struct _DMIBIOS_TABLE_HEADER
{
    UCHAR Signature2[5];           // _DMI_ (ascii)
    UCHAR IntermediateChecksum;
    USHORT StructureTableLength;
    ULONG StructureTableAddress;
    USHORT NumberStructures;
    UCHAR Revision;
} DMIBIOS_EPS_HEADER, *PDMIBIOS_EPS_HEADER;

//
// SMBIOS table search
#define SYSID_EPS_SEARCH_SIZE      0x20000
#define SYSID_EPS_SEARCH_START     0x000e0000
#define SYSID_EPS_SEARCH_INCREMENT 0x10

typedef struct _SYSID_EPS_HEADER
{
    UCHAR Signature[7];           // _SYSID_ (ascii)
    UCHAR Checksum;
    USHORT Length;                // Length of SYSID_EPS_HEADER
    ULONG SysIdTableAddress;      // Physical Address of SYSID table
    USHORT SysIdCount;            // Count of SYSIDs in table
    UCHAR BiosRev;                // SYSID Bios revision
} SYSID_EPS_HEADER, *PSYSID_EPS_HEADER;

typedef struct _SYSID_TABLE_ENTRY
{
	UCHAR Type[6];                // _UUID_ or _1394_ (ascii)
    UCHAR Checksum;
    USHORT Length;                // Length of this table
    UCHAR Data[1];                // Variable length UUID/1394 data
} SYSID_TABLE_ENTRY, *PSYSID_TABLE_ENTRY;

#define SYSID_UUID_DATA_SIZE 16

typedef struct _SYSID_UUID_ENTRY
{
	UCHAR Type[6];                // _UUID_ (ascii)
    UCHAR Checksum;
    USHORT Length;                // Length of this table
    UCHAR UUID[SYSID_UUID_DATA_SIZE];  // UUID
} SYSID_UUID_ENTRY, *PSYSID_UUID_ENTRY;

#define SYSID_1394_DATA_SIZE 8

typedef struct _SYSID_1394_ENTRY
{
	UCHAR Type[6];                // _1394_ (ascii)
    UCHAR Checksum;
    USHORT Length;                // Length of this table
    UCHAR x1394Id[SYSID_1394_DATA_SIZE]; // 1394 ID
} SYSID_1394_ENTRY, *PSYSID_1394_ENTRY;

#define LARGEST_SYSID_TABLE_ENTRY (sizeof(SYSID_UUID_ENTRY))

#define SYSID_TYPE_UUID "_UUID_"
#define SYSID_TYPE_1394 "_1394_"
									   
#pragma pack(pop)
#endif
