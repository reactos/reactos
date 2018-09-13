/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    udfs_rec.h

Abstract:

    This module contains the mini-file system recognizer for UDFS.

Author:

    Dan Lovinger (danlo) 13-Feb-1997

Environment:

    Kernel mode, local to I/O system

Revision History:


--*/

//
//  NOTE CAREFULLY: the canonical location for this information is in the UDFS
//      driver source.
//

//
//  Aligning this byte offset to a sector boundary by rounding up will
//  yield the starting offset of the Volume Recognition Area (2/8.3)
//

#define VRA_BOUNDARY_LOCATION (32767 + 1)

/***    ISO 13346 Part 2: Volume and Boot Block Recognition
 *
 *
 */


/***    vsd_generic - Generic Volume Structure Descriptor (2/9.1)
 *
 */

typedef struct  VSD_GENERIC {
    UCHAR       Type;                   // Structure Type
    UCHAR       Ident[5];               // Standard Identifier
    UCHAR       Version;                // Standard Version
    UCHAR       Data[2041];             // Structure Data
} VSD_GENERIC, *PVSD_GENERIC;

//  VSD_LENGTH_... - vsd field lengths

#define VSD_LENGTH_IDENT        5       // Length of regid_Identifier (bytes)

//  VSD_IDENT_... - Values for vsd_generic_Ident

#define VSD_IDENT_BEA01     "BEA01"     // Begin Extended Area
#define VSD_IDENT_TEA01     "TEA01"     // Terminate Extended Area
#define VSD_IDENT_CDROM     "CDROM"     // High Sierra Group (pre-ISO 9660)
#define VSD_IDENT_CD001     "CD001"     // ISO 9660
#define VSD_IDENT_CDW01     "CDW01"     // ECMA 168
#define VSD_IDENT_CDW02     "CDW02"     // ISO 13490
#define VSD_IDENT_NSR01     "NSR01"     // ECMA 167
#define VSD_IDENT_NSR02     "NSR02"     // ISO 13346
#define VSD_IDENT_BOOT2     "BOOT2"     // Boot Descriptor

typedef enum _VSD_IDENT {
    VsdIdentBad = 0,
    VsdIdentBEA01,
    VsdIdentTEA01,
    VsdIdentCDROM,
    VsdIdentCD001,
    VsdIdentCDW01,
    VsdIdentCDW02,
    VsdIdentNSR01,
    VsdIdentNSR02,
    VsdIdentBOOT2
} VSD_IDENT, *PVSD_IDENT;

//
//  Following structure is used to build up static data for parse tables
//

typedef struct _PARSE_KEYVALUE {
    PCHAR Key;
    ULONG Value;
} PARSE_KEYVALUE, *PPARSE_KEYVALUE;

//
// Define the functions provided by this driver.
//

BOOLEAN
IsUdfsVolume (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize
    );

ULONG
UdfsFindInParseTable (
    IN PPARSE_KEYVALUE ParseTable,
    IN PCHAR Id,
    IN ULONG MaxIdLen
    );

#define SectorAlignN(SECTORSIZE, L) (                                           \
    ((((ULONG)(L)) + ((SECTORSIZE) - 1)) & ~((SECTORSIZE) - 1))                 \
)

