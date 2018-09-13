/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    abios.h

Abstract:

    This module contains the i386 kernel ABIOS specific header file.

Author:

    Shie-Lin Tzong (shielint) 22-May-1991

Revision History:

--*/

//
// Define public portion of the ABIOS Device Block
//

typedef struct _KDEVICE_BLOCK {
    USHORT Length;
    UCHAR Revision;
    UCHAR SecondDeviceId;
    USHORT LogicalId;
    USHORT DeviceId;
    USHORT NumberExclusivePortPairs;
    USHORT NumberCommonPortPairs;
} KDEVICE_BLOCK, *PKDEVICE_BLOCK; 


typedef struct _KABIOS_POINTER {
    USHORT Offset;
    USHORT Selector;
} KABIOS_POINTER, *PKABIOS_POINTER;

#pragma pack(1)

//
// ABIOS Function Transfer Table definition
//

typedef struct _KFUNCTION_TRANSFER_TABLE {
    KABIOS_POINTER CommonRoutine[3];
    USHORT FunctionCount;
    USHORT Reserved;
    KABIOS_POINTER SpecificRoutine;
} KFUNCTION_TRANSFER_TABLE, *PKFUNCTION_TRANSFER_TABLE;


//
// ABIOS Commom Data Area definitions
//

typedef struct _KDB_FTT_SECTION {
    KABIOS_POINTER DeviceBlock;
    KABIOS_POINTER FunctionTransferTable;
} KDB_FTT_SECTION, *PKDB_FTT_SECTION;

typedef struct _KCOMMON_DATA_AREA {
    USHORT DataPointer0Offset;
    USHORT NumberLids;
    ULONG Reserved;
    PKDB_FTT_SECTION DbFttPointer;
} KCOMMON_DATA_AREA, *PKCOMMON_DATA_AREA;

#pragma pack()

//
// Available GDT Entry
//

typedef struct _KFREE_GDT_ENTRY {
    struct _KFREE_GDT_ENTRY *Flink;
    ULONG BaseMid : 8;
    ULONG Type : 5;
    ULONG Dpl : 2;
    ULONG Present : 1;
    ULONG LimitHi : 4;
    ULONG Sys : 1;
    ULONG Reserved_0 : 1;
    ULONG Default_Big : 1;
    ULONG Granularity : 1;
    ULONG BaseHi : 8;
} KFREE_GDT_ENTRY, *PKFREE_GDT_ENTRY;

//
// Logical Id table entry
//

typedef struct _KLID_TABLE_ENTRY {
    ULONG Owner;
    ULONG OwnerCount;
} KLID_TABLE_ENTRY, *PKLID_TABLE_ENTRY;

#define LID_NO_SPECIFIC_OWNER  0xffffffff
#define NUMBER_LID_TABLE_ENTRIES 1024

//
// Macro to extract the high byte of a short offset
//

#define HIGHBYTE(l) ((UCHAR)(((USHORT)(l)>>8) & 0xff))

//
// Macro to extract the low byte of a short offset
//

#define LOWBYTE(l) ((UCHAR)(l))

//
// The following selectors are reserved for 16 bit stack, code and 
// ABIOS Common Data Area.
//

#define KGDT_STACK16 0xf8
#define KGDT_CODE16 0xf0
#define KGDT_CDA16  0xe8         
#define KGDT_GDT_ALIAS 0x70

//
// Misc. definitions
//

#define RESERVED_GDT_ENTRIES  28

//
// External references
//

extern PKFREE_GDT_ENTRY KiAbiosGdtStart;
extern PKFREE_GDT_ENTRY KiAbiosGdtEnd;
extern PUCHAR KiEndOfCode16;
extern ULONG KiStack16GdtEntry;

extern 
VOID
KiI386CallAbios(
    IN KABIOS_POINTER AbiosFunction,
    IN KABIOS_POINTER DeviceBlockPointer,
    IN KABIOS_POINTER FunctionTransferTable,
    IN KABIOS_POINTER RequestBlock
    );

VOID
KiInitializeAbiosGdtEntry (
    OUT PKGDTENTRY GdtEntry,
    IN ULONG Base,
    IN ULONG Limit,
    IN USHORT Type
    );

extern
ULONG
KiAbiosGetGdt (
    VOID
    );


