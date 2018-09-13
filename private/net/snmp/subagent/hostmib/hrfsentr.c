/*
 *  HrFSEntry.c v0.10
 *  Generated in conjunction with Management Factory scripts: 
 *      script version: SNMPv1, 0.16, Apr 25, 1996
 *      project:        D:\TEMP\EXAMPLE\HOSTMIB
 ****************************************************************************
 *                                                                          *
 *      (C) Copyright 1995 DIGITAL EQUIPMENT CORPORATION                    *
 *                                                                          *
 *      This  software  is  an  unpublished work protected under the        *
 *      the copyright laws of the  United  States  of  America,  all        *
 *      rights reserved.                                                    *
 *                                                                          *
 *      In the event this software is licensed for use by the United        *
 *      States Government, all use, duplication or disclosure by the        *
 *      United States Government is subject to restrictions  as  set        *
 *      forth in either subparagraph  (c)(1)(ii)  of the  Rights  in        *
 *      Technical  Data  And  Computer  Software  Clause  at   DFARS        *
 *      252.227-7013, or the Commercial Computer Software Restricted        *
 *      Rights Clause at FAR 52.221-19, whichever is applicable.            *
 *                                                                          *
 ****************************************************************************
 *
 *  Facility:
 *
 *    Windows NT SNMP Extension Agent
 *
 *  Abstract:
 *  
 *    This module contains the code for dealing with the get, set, and
 *    instance name routines for the HrFSEntry.  Actual instrumentation code is
 *    supplied by the developer.
 *
 *  Functions:
 *
 *    A get and set routine for each attribute in the class.
 *
 *    The routines for instances within the class.
 *
 *  Author:
 *
 *	D. D. Burns @ Webenable Inc
 *
 *  Revision History:
 *
 *    V1.00 - 04/18/97  D. D. Burns     Genned: Thu Nov 07 16:44:44 1996
 *    V1.01 - 06/17/97  D. D. Burns     Fix bug in Gen_HrFSTable_Cache() that
 *                                        precluded finding "hrFSStorageIndex"
 *                                        for drives w/volume labels.
 *	
 */


#include <windows.h>
#include <malloc.h>

#include <snmp.h>

#include "mib.h"
#include "smint.h"
#include "hostmsmi.h"
#include "user.h"         /* Developer supplied include file */
#include "HMCACHE.H"      /* Cache-related definitions */
#include "string.h"       /* For string manipulation in "Gen_Hrstorage_Cache"*/
#include "stdio.h"        /* For sprintf */
#include <winioctl.h>     /* For PARTITION_INFORMATION */


/*
|==============================================================================
| Function prototypes for this module.
|
*/
/* debug_print_hrFSTable - Prints a Row from HrFSTable sub-table */
static void
debug_print_hrFSTable(
                      CACHEROW     *row  /* Row in hrFSTable table */
                      );


/*
|==============================================================================
| Create the list-head for the HrFStable cache.
|
| - Global so that code in "ProcessPartitions()" in "HRDISKST.C" can search
|   this cache.
|
| - This macro is defined in "HMCACHE.H".
*/
CACHEHEAD_INSTANCE(hrFSTable_cache, debug_print_hrFSTable);



/*
 *  GetHrFSIndex
 *    A unique value for each file system local to this host.  The value for 
 *    each file system must remain constant at least from one r
 *    
 *    Gets the value for HrFSIndex.
 *
 *  Arguments:
 *
 *    outvalue                   address to return variable value
 *    accesss                    Reserved for future security use
 *    instance                   address of instance name as ordered native
 *                               data type(s)
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtget.c v0.10
 *
 | =============== From WebEnable Design Spec Rev 3 04/11/97==================
 | hrFSIndex
 | 
 |  ACCESS         SYNTAX
 |  read-only      INTEGER (1..2147483647)
 | 
 | "A unique value for each file system local to this host.  The value for each
 | file system must remain constant at least from one re-initialization of the
 | agent to the next re-initialization."
 | 
 | 
 | DISCUSSION:
 | 
 | An entry is generated for each drive (network or not) returned by
 | "GetLogicalDrives".
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.8.1.1.<instance>
 |                | | | |
 |                | | | *hrFSIndex
 |                | | *hrFSEntry
 |                | *-hrFSTable
 |                *-hrDevice
 */

UINT
GetHrFSIndex( 
        OUT Integer *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
ULONG           index;          /* As fetched from instance structure */
CACHEROW        *row;           /* Row entry fetched from cache       */


/*
| Grab the instance information
*/
index = GET_INSTANCE(0);

/*
| Use it to find the right entry in the cache
*/
if ((row = FindTableRow(index, &hrFSTable_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| Return the "hrFSIndex" value from this entry
*/
*outvalue = row->attrib_list[HRFS_INDEX].u.unumber_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrFSIndex() */


/*
 *  GetHrFSMountPoint
 *    The path name of the root of this file system.
 *    
 *    Gets the value for HrFSMountPoint.
 *
 *  Arguments:
 *
 *    outvalue                   address to return variable value
 *    accesss                    Reserved for future security use
 *    instance                   address of instance name as ordered native
 *                               data type(s)
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtget.c v0.10
 *
 | =============== From WebEnable Design Spec Rev 3 04/11/97==================
 | hrFSMountPoint
 | 
 |  ACCESS         SYNTAX
 |  read-only      InternationalDisplayString (SIZE(0..128))
 | 
 | "The path name of the root of this file system."
 | 
 | DISCUSSION:
 | 
 | The value of this attribute is the proper value returned by
 | "GetLogicalDriveStrings" for the selected entry.
 | 
 | RESOLVED >>>>>>>>
 | <POA-15>  Just return an empty string for the Mount Point variables.
 | RESOLVED >>>>>>>>
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.8.1.2.<instance>
 |                | | | |
 |                | | | *hrFSMountPoint
 |                | | *hrFSEntry
 |                | *-hrFSTable
 |                *-hrDevice
 */

UINT
GetHrFSMountPoint( 
        OUT InternationalDisplayString *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{

ULONG           index;          /* As fetched from instance structure */
CACHEROW        *row;           /* Row entry fetched from cache       */


/*
| Grab the instance information
*/
index = GET_INSTANCE(0);

/*
| Use it to find the right entry in the cache
*/
if ((row = FindTableRow(index, &hrFSTable_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| The cache has the device string in it, but we return the empty string
| per the spec.
*/
#if 1
    /* Return the empty string */
    outvalue->string = "";
    outvalue->length = 0;
#else
    /* Return the cached string */
    outvalue->string = row->attrib_list[HRFS_MOUNTPT].u.string_value;
    outvalue->length = strlen(outvalue->string);

    /* "Truncate" here to meet RFC as needed*/
    if ((outvalue->length = strlen(outvalue->string)) > 128) {
        outvalue->length = 128;
        }
#endif

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrFSMountPoint() */


/*
 *  GetHrFSRemoteMountPoint
 *    A description of the name and/or address of the server that this file 
 *    system is mounted from.  This may also include parameters 
 *    
 *    Gets the value for HrFSRemoteMountPoint.
 *
 *  Arguments:
 *
 *    outvalue                   address to return variable value
 *    accesss                    Reserved for future security use
 *    instance                   address of instance name as ordered native
 *                               data type(s)
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtget.c v0.10
 *
 | =============== From WebEnable Design Spec Rev 3 04/11/97==================
 | hrFSRemoteMountPoint
 | 
 |  ACCESS         SYNTAX
 |  read-only      InternationalDisplayString (SIZE(0..128))
 | 
 | "A description of the name and/or address of the server that this file system
 | is mounted from.  This may also include parameters such as the mount point on
 | the remote file system.  If this is not a remote file system, this string
 | should have a length of zero."
 | 
 | DISCUSSION:
 | 
 | <POA-15> The starting point for deriving this attribute's value is the logical
 | drive name, which would already be known to represent a network drive.  I can
 | find no Win32 API function that maps a network logical drive to it's server.
 | 
 | RESOLVED >>>>>>>>
 | <POA-15>  Just return an empty string for the Mount Point variables.
 | RESOLVED >>>>>>>>
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.8.1.3.<instance>
 |                | | | |
 |                | | | *hrFSRemoteMountPoint
 |                | | *hrFSEntry
 |                | *-hrFSTable
 |                *-hrDevice
 */

UINT
GetHrFSRemoteMountPoint( 
        OUT InternationalDisplayString *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{

/* Return the empty string */
outvalue->string = "";
outvalue->length = 0;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrFSRemoteMountPoint() */


/*
 *  GetHrFSType
 *    The value of this object identifies the type of this file system.
 *    
 *    Gets the value for HrFSType.
 *
 *  Arguments:
 *
 *    outvalue                   address to return variable value
 *    accesss                    Reserved for future security use
 *    instance                   address of instance name as ordered native
 *                               data type(s)
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtget.c v0.10
 *
 | =============== From WebEnable Design Spec Rev 3 04/11/97==================
 | hrFSType
 | 
 |  ACCESS         SYNTAX
 |  read-only      OBJECT IDENTIFIER
 | 
 | "The value of this object identifies the type of this file system."
 | 
 | DISCUSSION:
 | 
 |    -- Registration for some popular File System types,
 |    -- for use with hrFSType.
 | 
 |    hrFSTypes               OBJECT IDENTIFIER ::= { hrDevice 9 }
 | 
 |    hrFSOther               OBJECT IDENTIFIER ::= { hrFSTypes 1 }
 |    hrFSUnknown             OBJECT IDENTIFIER ::= { hrFSTypes 2 }
 |    hrFSBerkeleyFFS         OBJECT IDENTIFIER ::= { hrFSTypes 3 }
 |    hrFSSys5FS              OBJECT IDENTIFIER ::= { hrFSTypes 4 }
 |    -- DOS
 |    hrFSFat                 OBJECT IDENTIFIER ::= { hrFSTypes 5 }
 |    -- OS/2 High Performance File System
 |    hrFSHPFS                OBJECT IDENTIFIER ::= { hrFSTypes 6 }
 |    --  Macintosh Hierarchical File System
 |    hrFSHFS                 OBJECT IDENTIFIER ::= { hrFSTypes 7 }
 | 
 | 
 |    -- Macintosh File System
 |    hrFSMFS                 OBJECT IDENTIFIER ::= { hrFSTypes 8 }
 |    -- Windows NT
 |    hrFSNTFS                OBJECT IDENTIFIER ::= { hrFSTypes 9 }
 |    hrFSVNode               OBJECT IDENTIFIER ::= { hrFSTypes 10 }
 |    hrFSJournaled           OBJECT IDENTIFIER ::= { hrFSTypes 11 }
 |    -- CD File systems
 |    hrFSiso9660             OBJECT IDENTIFIER ::= { hrFSTypes 12 }
 |    hrFSRockRidge           OBJECT IDENTIFIER ::= { hrFSTypes 13 }
 | 
 |    hrFSNFS                 OBJECT IDENTIFIER ::= { hrFSTypes 14 }
 |    hrFSNetware             OBJECT IDENTIFIER ::= { hrFSTypes 15 }
 |    -- Andrew File System
 |    hrFSAFS                 OBJECT IDENTIFIER ::= { hrFSTypes 16 }
 |    -- OSF DCE Distributed File System
 |    hrFSDFS                 OBJECT IDENTIFIER ::= { hrFSTypes 17 }
 |    hrFSAppleshare          OBJECT IDENTIFIER ::= { hrFSTypes 18 }
 |    hrFSRFS                 OBJECT IDENTIFIER ::= { hrFSTypes 19 }
 |    -- Data General
 |    hrFSDGCFS               OBJECT IDENTIFIER ::= { hrFSTypes 20 }
 |    -- SVR4 Boot File System
 |    hrFSBFS                 OBJECT IDENTIFIER ::= { hrFSTypes 21 }
 | 
 | Win32 API function "GetVolumeInformation" can provide us with the 
 | information needed to select the correct OID for this attribute's value.
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.9.n
 |                | | |
 |                | | *-Type indicator
 |                | *-hrFSTypes
 |                *-hrDevice
 |
 | 1.3.6.1.2.1.25.3.8.1.4.<instance>
 |                | | | |
 |                | | | *hrFSType
 |                | | *hrFSEntry
 |                | *-hrFSTable
 |                *-hrDevice
 */

UINT
GetHrFSType( 
        OUT ObjectIdentifier *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
ULONG           index;          /* As fetched from instance structure */
CACHEROW        *row;           /* Row entry fetched from cache       */


/*
| Grab the instance information
*/
index = GET_INSTANCE(0);

/*
| Use it to find the right entry in the cache
*/
if ((row = FindTableRow(index, &hrFSTable_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| By convention with the cache-building function "Gen_HrFSTable_Cache()",
| the cached value is the right-most arc we must return as the value.
|
| Hence whatever cache entry we retrieve, we tack the number retrieved
| from the cache for this attribute onto { hrFSTypes ... }.
*/
if ( (outvalue->ids = SNMP_malloc(10 * sizeof( UINT ))) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }
outvalue->idLength = 10;


/*
| Load in the full hrFSType OID:
|
| 1.3.6.1.2.1.25.3.9.n
|                | | |
|                | | *-Type indicator
|                | *-hrFSTypes
|                *-hrDevice
*/
outvalue->ids[0] = 1;
outvalue->ids[1] = 3;
outvalue->ids[2] = 6;
outvalue->ids[3] = 1;
outvalue->ids[4] = 2;
outvalue->ids[5] = 1;
outvalue->ids[6] = 25;
outvalue->ids[7] = 3;
outvalue->ids[8] = 9;

/* Cached FS Type indicator */
outvalue->ids[9] = row->attrib_list[HRFS_TYPE].u.unumber_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrFSType() */


/*
 *  GetHrFSAccess
 *    An indication if this file system is logically configured by the 
 *    operating system to be readable and writable or only readable. 
 *    
 *    Gets the value for HrFSAccess.
 *
 *  Arguments:
 *
 *    outvalue                   address to return variable value
 *    accesss                    Reserved for future security use
 *    instance                   address of instance name as ordered native
 *                               data type(s)
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtget.c v0.10
 *
 | =============== From WebEnable Design Spec Rev 3 04/11/97==================
 | hrFSAccess
 | 
 |  ACCESS         SYNTAX
 |  read-only      INTEGER {readWrite(1),readOnly(2)}
 | 
 | "An indication if this file system is logically configured by the operating
 | system to be readable and writable or only readable.  This does not represent
 | any local access-control policy, except one that is applied to the file system
 | as a whole."
 | 
 | DISCUSSION:
 | 
 | Win32 API function "GetVolumeInformation" can provide us with the information
 | needed to select the correct OID for this attribute's value.
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.8.1.5.<instance>
 |                | | | |
 |                | | | *hrFSAccess
 |                | | *hrFSEntry
 |                | *-hrFSTable
 |                *-hrDevice
 */

UINT
GetHrFSAccess( 
        OUT INTAccess *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
ULONG           index;          /* As fetched from instance structure */
CACHEROW        *row;           /* Row entry fetched from cache       */


/*
| Grab the instance information
*/
index = GET_INSTANCE(0);

/*
| Use it to find the right entry in the cache
*/
if ((row = FindTableRow(index, &hrFSTable_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| Return the "hrFSAccess" value from this entry
*/
*outvalue = row->attrib_list[HRFS_ACCESS].u.unumber_value;


return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrFSAccess() */


/*
 *  GetHrFSBootable
 *    A falg indicating whether this file system is bootable.
 *    
 *    Gets the value for HrFSBootable.
 *
 *  Arguments:
 *
 *    outvalue                   address to return variable value
 *    accesss                    Reserved for future security use
 *    instance                   address of instance name as ordered native
 *                               data type(s)
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtget.c v0.10
 *
 | =============== From WebEnable Design Spec Rev 3 04/11/97==================
 | hrFSBootable
 | 
 |  ACCESS         SYNTAX
 |  read-only      Boolean
 | 
 | "A flag indicating whether this file system is bootable."
 | 
 | DISCUSSION:
 | 
 | Win32 API function "CreatFile" and DeviceIoControlcan provide us with the
 | information needed to select the correct OID for this attribute's value.
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.8.1.6.<instance>
 |                | | | |
 |                | | | *hrFSBootable
 |                | | *hrFSEntry
 |                | *-hrFSTable
 |                *-hrDevice
 */

UINT
GetHrFSBootable( 
        OUT Boolean *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
ULONG           index;          /* As fetched from instance structure */
CACHEROW        *row;           /* Row entry fetched from cache       */


/*
| Grab the instance information
*/
index = GET_INSTANCE(0);

/*
| Use it to find the right entry in the cache
*/
if ((row = FindTableRow(index, &hrFSTable_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| Return the "hrFSBootable" value from this entry
*/
*outvalue = row->attrib_list[HRFS_BOOTABLE].u.unumber_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrFSBootable() */


/*
 *  GetHrFSStorageIndex
 *    The index of the hrStorageEntry that represents information about this 
 *    file system.  If there is no such information available, 
 *    
 *    Gets the value for HrFSStorageIndex.
 *
 *  Arguments:
 *
 *    outvalue                   address to return variable value
 *    accesss                    Reserved for future security use
 *    instance                   address of instance name as ordered native
 *                               data type(s)
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtget.c v0.10
 *
 | =============== From WebEnable Design Spec Rev 3 04/11/97==================
 | hrFSStorageIndex
 | 
 |  ACCESS         SYNTAX
 |  read-only      INTEGER (0..2147483647)
 | 
 | "The index of the hrStorageEntry that represents information about this file
 | system.  If there is no such information available, then this value shall be
 | zero.  The relevant storage entry will be useful in tracking the percent usage
 | of this file system and diagnosing errors that may occur when it runs out of
 | space."
 | 
 | DISCUSSION:
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.8.1.7.<instance>
 |                | | | |
 |                | | | *hrFSStorageIndex
 |                | | *hrFSEntry
 |                | *-hrFSTable
 |                *-hrDevice
 */

UINT
GetHrFSStorageIndex( 
        OUT Integer *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
ULONG           index;          /* As fetched from instance structure */
CACHEROW        *row;           /* Row entry fetched from cache       */


/*
| Grab the instance information
*/
index = GET_INSTANCE(0);

/*
| Use it to find the right entry in the cache
*/
if ((row = FindTableRow(index, &hrFSTable_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| Return the "hrFSStorageIndex" value from this entry
*/
*outvalue = row->attrib_list[HRFS_STORINDX].u.unumber_value;


return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrFSStorageIndex() */


/*
 *  GetHrFSLastFullBackupDate
 *    The last date at which this complete file system was copied to another 
 *    storage device for backup.  This information is useful fo
 *    
 *    Gets the value for HrFSLastFullBackupDate.
 *
 *  Arguments:
 *
 *    outvalue                   address to return variable value
 *    accesss                    Reserved for future security use
 *    instance                   address of instance name as ordered native
 *                               data type(s)
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtget.c v0.10
 *
 | =============== From WebEnable Design Spec Rev 3 04/11/97==================
 | hrFSLastFullBackupDate
 | 
 |  ACCESS         SYNTAX
 |  read-write     DateAndTime
 | 
 | "The last date at which this complete file system was copied to another
 | storage device for backup.  This information is useful for ensuring that
 | backups are being performed regularly.  If this information is not known, then
 | this variable shall have the value corresponding to January 1, year 0000,
 | 00:00:00.0, which is encoded as (hex)'00 00 01 01 00 00 00 00'."
 | 
 | DISCUSSION:
 | 
 | This metric is apparently not recorded and is not made available through
 | any documented Win32 API function.  Consequently, we return the appropriate
 | "not known" value.
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.8.1.8.<instance>
 |                | | | |
 |                | | | *hrFSLastFullBackupDate
 |                | | *hrFSEntry
 |                | *-hrFSTable
 |                *-hrDevice
 */

UINT
GetHrFSLastFullBackupDate( 
        OUT DateAndTime *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{

outvalue->length = 8;
outvalue->string = "\0\0\1\1\0\0\0\0";


return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrFSLastFullBackupDate() */


/*
 *  SetHrFSLastFullBackupDate
 *    The last date at which this complete file system was copied to another 
 *    storage device for backup.  This information is useful fo
 *    
 *    Sets the HrFSLastFullBackupDate value.
 *
 *  Arguments:
 *
 *    invalue                    address of value to set the variable
 *    outvalue                   address to return the set variable value
 *    access                     Reserved for future security use
 *    instance                   address of instance name as ordered native
 *                               data type(s)
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_BADVALUE   Set value not in range
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtset.ntc v0.10
 */

UINT
SetHrFSLastFullBackupDate( 
        IN DateAndTime *invalue ,
        OUT DateAndTime *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{

    return SNMP_ERRORSTATUS_NOSUCHNAME ;

} /* end of SetHrFSLastFullBackupDate() */


/*
 *  GetHrFSLastPartialBackupDate
 *    The last date at which a portion of thes file system was copied to 
 *    another storage device for backup.  This information is usefu
 *    
 *    Gets the value for HrFSLastPartialBackupDate.
 *
 *  Arguments:
 *
 *    outvalue                   address to return variable value
 *    accesss                    Reserved for future security use
 *    instance                   address of instance name as ordered native
 *                               data type(s)
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtget.c v0.10
 *
 | =============== From WebEnable Design Spec Rev 3 04/11/97==================
 | hrFSLastPartialBackupDate
 | 
 |  ACCESS         SYNTAX
 |  read-write     DateAndTime
 | 
 | "The last date at which a portion of this file system was copied to another
 | storage device for backup.  This information is useful for ensuring that
 | backups are being performed regularly.  If this information is not known, 
 | then this variable shall have the value corresponding to 
 | January 1, year 0000, 00:00:00.0, which is encoded as
 | (hex)'00 00 01 01 00 00 00 00'."
 | 
 | DISCUSSION:
 | 
 | This metric is apparently not recorded and is not made available through
 | any documented Win32 API function.  Consequently, we return the appropriate
 | "not known" value.
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.8.1.9.<instance>
 |                | | | |
 |                | | | *hrFSLastPartialBackupDate
 |                | | *hrFSEntry
 |                | *-hrFSTable
 |                *-hrDevice
 */

UINT
GetHrFSLastPartialBackupDate( 
        OUT DateAndTime *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{

outvalue->length = 8;
outvalue->string = "\0\0\1\1\0\0\0\0";

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrFSLastPartialBackupDate() */


/*
 *  SetHrFSLastPartialBackupDate
 *    The last date at which a portion of thes file system was copied to 
 *    another storage device for backup.  This information is usefu
 *    
 *    Sets the HrFSLastPartialBackupDate value.
 *
 *  Arguments:
 *
 *    invalue                    address of value to set the variable
 *    outvalue                   address to return the set variable value
 *    access                     Reserved for future security use
 *    instance                   address of instance name as ordered native
 *                               data type(s)
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_BADVALUE   Set value not in range
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtset.ntc v0.10
 */

UINT
SetHrFSLastPartialBackupDate( 
        IN DateAndTime *invalue ,
        OUT DateAndTime *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{

    return SNMP_ERRORSTATUS_NOSUCHNAME ;

} /* end of SetHrFSLastPartialBackupDate() */


/*
 *  HrFSEntryFindInstance
 *
 *     This routine is used to verify that the specified instance is
 *     valid.
 *
 *  Arguments:
 *
 *     FullOid                 Address for the full oid - group, variable,
 *                             and instance information
 *     instance                Address for instance specification as an oid
 *
 *  Return Codes:
 *
 *     SNMP_ERRORSTATUS_NOERROR     Instance found and valid
 *     SNMP_ERRORSTATUS_NOSUCHNAME  Invalid instance
 *
 */

UINT
HrFSEntryFindInstance( IN ObjectIdentifier *FullOid ,
                       IN OUT ObjectIdentifier *instance )
{
    UINT tmp_instance ;

    //
    //  Developer instrumentation code to find appropriate instance goes here.
    //  For non-tables, it is not necessary to modify this routine.  However, if
    //  there is any context that needs to be set, it can be done here.
    //

    if ( FullOid->idLength <= HRFSENTRY_VAR_INDEX )
	// No instance was specified
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else  if ( FullOid->idLength != HRFSENTRY_VAR_INDEX + 1 )
	// Instance length is more than 1
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else
	// The only valid instance for a non-table are instance 0.  If this
	// is a non-table, the following code validates the instances.  If this
	// is a table, developer modification is necessary below.

	tmp_instance = FullOid->ids[ HRFSENTRY_VAR_INDEX ] ;

        /*
        | For hrFSTable, the instance arc(s) is a single arc, and it must
        | correctly select an entry in the hrFSTable cache.
        | Check that here.
        */
	if ( FindTableRow(tmp_instance, &hrFSTable_cache) == NULL ) {
	    return SNMP_ERRORSTATUS_NOSUCHNAME ;
            }
	else
	{
	    // the instance is valid.  Create the instance portion of the OID
	    // to be returned from this call.
	    instance->ids[ 0 ] = tmp_instance ;
	    instance->idLength = 1 ;
	}

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of HrFSEntryFindInstance() */



/*
 *  HrFSEntryFindNextInstance
 *
 *     This routine is called to get the next instance.  If no instance
 *     was passed than return the first instance (1).
 *
 *  Arguments:
 *
 *     FullOid                 Address for the full oid - group, variable,
 *                             and instance information
 *     instance                Address for instance specification as an oid
 *
 *  Return Codes:
 *
 *     SNMP_ERRORSTATUS_NOERROR     Instance found and valid
 *     SNMP_ERRORSTATUS_NOSUCHNAME  Invalid instance
 *
 */

UINT
HrFSEntryFindNextInstance( IN ObjectIdentifier *FullOid ,
                           IN OUT ObjectIdentifier *instance )
{
    //
    //  Developer supplied code to find the next instance of class goes here.
    //  If this is a class with cardinality 1, no modification of this routine
    //  is necessary unless additional context needs to be set.
    //  If the FullOid does not specify an instance, then the only instance
    //  of the class is returned.  If this is a table, the first row of the
    //  table is returned.
    //
    //  If an instance is specified and this is a non-table class, then NOSUCHNAME
    //  is returned so that correct MIB rollover processing occurs.  If this is
    //  a table, then the next instance is the one following the current instance.
    //  If there are no more instances in the table, return NOSUCHNAME.
    //

    CACHEROW        *row;
    ULONG           tmp_instance;


    if ( FullOid->idLength <= HRFSENTRY_VAR_INDEX )
    {
        /*
        | Too short: must return the instance arc that selects the first 
        |            entry in the table if there is one.
        */
        tmp_instance = 0;
    }
    else {
        /*
        | There is at least one instance arc.  Even if it is the only arc
        | we use it as the "index" in a request for the "NEXT" one.
        */
        tmp_instance = FullOid->ids[ HRFSENTRY_VAR_INDEX ] ;
        }

    /* Now go off and try to find the next instance in the table */
    if ((row = FindNextTableRow(tmp_instance, &hrFSTable_cache)) == NULL) {
        return SNMP_ERRORSTATUS_NOSUCHNAME ;
        }

    instance->ids[ 0 ] = row->index ;
    instance->idLength = 1 ;

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of HrFSEntryFindNextInstance() */



/*
 *  HrFSEntryConvertInstance
 *
 *     This routine is used to convert the object id specification of an
 *     instance into an ordered native representation.  The object id format
 *     is that object identifier that is returned from the Find Instance
 *     or Find Next Instance routines.  It is NOT the full object identifier
 *     that contains the group and variable object ids as well.  The native
 *     representation is an argc/argv-like structure that contains the
 *     ordered variables that define the instance.  This is specified by
 *     the MIB's INDEX clause.  See RFC 1212 for information about the INDEX
 *     clause.
 *
 *
 *  Arguments:
 *
 *     oid_spec                Address of the object id instance specification
 *     native_spec             Address to return the ordered native instance
 *                             specification
 *
 *  Return Codes:
 *
 *     SUCCESS                 Conversion complete successfully
 *     FAILURE                 Unable to convert object id into native format
 *
 */

UINT
HrFSEntryConvertInstance( IN ObjectIdentifier *oid_spec ,
                          IN OUT InstanceName *native_spec )
{
static char    *array;  /* The address of this (char *) is passed back     */
                        /* as though it were an array of length 1 of these */
                        /* types.                                          */

static ULONG    inst;   /* The address of this ULONG is passed back  */
                        /* (Obviously, no "free()" action is needed) */

    /* We only expect the one arc in "oid_spec" */
    inst = oid_spec->ids[0];
    array = (char *) &inst;

    native_spec->count = 1;
    native_spec->array = &array;
    return SUCCESS ;

} /* end of HrFSEntryConvertInstance() */




/*
 *  HrFSEntryFreeInstance
 *
 *     This routine is used to free an ordered native representation of an
 *     instance name.
 *
 *  Arguments:
 *
 *     instance                Address to return the ordered native instance
 *                             specification
 *
 *  Return Codes:
 *
 *
 */

void
HrFSEntryFreeInstance( IN OUT InstanceName *instance )
{

  /* No action needed for hrFSTable */
} /* end of HrFSEntryFreeInstance() */

/*
| End of Generated Code
*/

/* Gen_HrFSTable_Cache - Generate a initial cache for HrFSTable */
/* Gen_HrFSTable_Cache - Generate a initial cache for HrFSTable */
/* Gen_HrFSTable_Cache - Generate a initial cache for HrFSTable */

BOOL
Gen_HrFSTable_Cache(
                    void
                    )

/*
|  EXPLICIT INPUTS:
|
|       None.
|
|  IMPLICIT INPUTS:
|
|       The module-local head of the cache for the HrFSTable,
|       "hrFSTable_cache".
|
|  OUTPUTS:
|
|     On Success:
|       Function returns TRUE indicating that the cache has been fully
|       populated with all "static" cache-able values.
|
|     On any Failure:
|       Function returns FALSE (indicating "not enough storage").
|
|  THE BIG PICTURE:
|
|       At subagent startup time, the cache for each table in the MIB is
|       populated with rows for each row in the table.  This function is
|       invoked by the start-up code in "UserMibInit()" ("MIB.C") to 
|       populate the cache for the HrStorage table.
|
|  OTHER THINGS TO KNOW:
|
|       There is one of these function for every table that has a cache.
|       Each is found in the respective source file.
|
|=============== From WebEnable Design Spec Rev 3 04/11/97==================
| DISCUSSION:
| 
| It appears that there is a one-to-one correspondence between NT logical drives
| and "file systems" as defined by this table.  As a consequence, the contents
| of this table is almost identical to the hrStorageTable except that remote
| network drives (in unix-speak, remotely mounted file systems) are included
| in this table while excluded from hrStorageTable.
| 
| To this end, a combination of Win32 API functions "GetLogicalDrives",
| "GetVolumeInformation", "GetDriveType" and "GetDiskFreeSpace" are used to
| acquire the information for the SNMP attributes in this table.
| 
|============================================================================
| 1.3.6.1.2.1.25.3.8.1...
|                | | |
|                | | *hrFSEntry
|                | *-hrFSTable
|                *-hrDevice
|
*/
{
CHAR    temp[8];                /* Temporary buffer for first call         */
LPSTR   pDrvStrings;            /* --> allocated storage for drive strings */
LPSTR   pOriginal_DrvStrings;   /* (Needed for final deallocation          */
DWORD   DS_request_len;         /* Storage actually needed                 */
DWORD   DS_current_len;         /* Storage used on 2nd call                */
ULONG   table_index=0;          /* hrFSTable index counter                 */
CACHEROW *row;                  /* --> Cache structure for row-being built */
UINT    i;                      /* Handy-Dandy loop index                  */

#define PHYS_SIZE 32
CHAR    phys_name[PHYS_SIZE];   /* Buffer where a string like "\\.C:" (for */
                                /*  example) is built for drive access.    */


/*
| We're going to call GetLogicalDriveStrings() twice, once to get the proper
| buffer size, and the second time to actually get the drive strings.
*/
if ((DS_request_len = GetLogicalDriveStrings(2, temp)) == 0) {

    /* Request failed altogether, can't initialize */
    return ( FALSE );
    }

/*
| Grab enough storage for the drive strings plus one null byte at the end
*/

if ( (pOriginal_DrvStrings = pDrvStrings = malloc( (DS_request_len + 1) ) )
    == NULL) {

    /* Storage Request failed altogether, can't initialize */
    return ( FALSE );
    }

/* Go for all of the strings */
if ((DS_current_len = GetLogicalDriveStrings(DS_request_len, pDrvStrings))
    == 0) {

    /* Request failed altogether, can't initialize */
    free( pOriginal_DrvStrings );
    return ( FALSE );
    }


/*
|==============================================================================
| As long as we've got an unprocessed drive-string which may correspond to
| a File-System for which we need a table-row entry . . . 
*/
while ( strlen(pDrvStrings) > 0 ) {

    UINT        drivetype;      /* Type of the drive from "GetDriveType()"   */
    ULONG       fs_type;        /* derived File-System type (last-arc value) */
    UINT        bootable;       /* derived "bootable" value (Boolean)        */
    UINT        readwrite;      /* derived "readwrite" value (0 or 1)        */
    UINT        storage_index;  /* Index into hrStorageTable to matching slot*/
    CACHEROW   *row_hrstorage;  /* As returned by FindNextTableRow()         */


    /*
    | Get the drive-type so we can decide whether it should participate in
    | this table.  We do both locals and remotes (unlike hrStorage, from
    | which this code was ripped off).
    */
    drivetype = GetDriveType(pDrvStrings);

    if (   drivetype == DRIVE_UNKNOWN
        || drivetype == DRIVE_NO_ROOT_DIR
        ) {

        /* Step to next string, if any */
        pDrvStrings += strlen(pDrvStrings) + 1;

        continue;
        }

    /*
    | OK, we want this one in the table, get a row-entry created.
    */
    if ((row = CreateTableRow( HRFS_ATTRIB_COUNT ) ) == NULL) {
        return ( FALSE );       // Out of memory
        }

    /* =========== hrFSIndex ==========*/
    row->attrib_list[HRFS_INDEX].attrib_type = CA_NUMBER;
    row->attrib_list[HRFS_INDEX].u.unumber_value = (table_index += 1) ;


    /* =========== hrFSMountPoint ==========
    | Note: We store the device string in the cache, but the "GET" function
    |       always returns an empty string, per the spec.
    */
    row->attrib_list[HRFS_MOUNTPT].attrib_type = CA_STRING;

    /* Get some space */
    if ( (row->attrib_list[HRFS_MOUNTPT].u.string_value
          = ( LPSTR ) malloc(strlen(pDrvStrings) + 1)) == NULL) {
        return ( FALSE );       /* out of memory */
        }
    /* Copy the Value into the space */
    strcpy(row->attrib_list[HRFS_MOUNTPT].u.string_value, pDrvStrings);

    /*
    | The GET functions for "computed" attributes expect to be able to use
    | the value of the "hrFSMountPoint" cache value stored above to lookup
    | (using system calls) their respective values.  We may or may not actually
    | report this stored-value as the value of the attribute in initial
    | release.
    */


    /* =========== hrFSRemoteMountPoint ==========*/
    row->attrib_list[HRFS_RMOUNTPT].attrib_type = CA_COMPUTED;


    /* =========== hrFSType     ==========
    |  =========== hrFSAccess   ==========
    |  =========== hrFSBootable ==========
    |
    | We use the first two characters of the drive string (e.g. "C:") to
    | create the special string (e.g. "\\.\C:") needed to obtain a "CreateFile"
    | handle to the device "C" or partition "C".
    |
    | With that, a DeviceIoControl call for partition information gives
    | us an idea as to the type and bootability of the device or partition.
    |
    | If any step in this process fails, the type is "hrFSUnknown", bootable
    | is "FALSE" and Access is presumed "read-write".
    |
    | For hrFSType we store a single number as the cached value of the 
    | hrFSType attribute.  When this attribute is fetched, the cached number 
    | forms the last arc in the OBJECT IDENTIFIER that actually specifies the
    | type: { hrFSTypes x }, where "x" is what gets stored.
    */
    fs_type = 2;        /* 2 = last arc value for "hrFSUnknown" */
    bootable = 0;       /* if unknown, "not bootable"           */
    readwrite = 1;      /* if unknown, "read-write"             */

    /* If we have room in the buffer to build the handle-name string */
    if ((strlen(pDrvStrings) + strlen("\\\\.\\")) < PHYS_SIZE) {

        HANDLE                  hdrv;       /* Handle to device           */
        PARTITION_INFORMATION   part_info;  /* Partition Info from device */
        DWORD                   bytes_out;  /* Bytes retnd into part_info */


        /* Build it for device A: "\\.\A:" */
        sprintf(phys_name, "\\\\.\\%2.2s", pDrvStrings);

        /*
        | Suppress any attempt by the system to make the user put a volume in a
        | removable drive ("CreateFile" will just fail).
        */
        SetErrorMode(SEM_FAILCRITICALERRORS);

        /* Attempt to get a handle using this physical name string */
        if ((hdrv = CreateFile(phys_name,             // Device
                               GENERIC_READ,          // Access
                               FILE_SHARE_READ   |
                               FILE_SHARE_WRITE,      // Share Mode
                               NULL,                  // Security
                               OPEN_EXISTING,         // CreationDistribution
                               FILE_ATTRIBUTE_NORMAL, // FlagsandAttributes
                               NULL                   // Template file
                               )) != INVALID_HANDLE_VALUE) {
            /*
            | Device is Open
            |
            | Try for Partition Information on the "device" we opened
            |
            | (Not supported by Floppy drivers, so this'll probably fail).
            */
            if (DeviceIoControl(hdrv,           // device handle
                                                // IoControlCode (op-code)
                                IOCTL_DISK_GET_PARTITION_INFO,

                                NULL,           // "input buffer"
                                0,              // "input buffer size"
                                &part_info,     // "output buffer"
                                                // "output buffer size"
                                sizeof(PARTITION_INFORMATION),

                                &bytes_out,     // bytes written to part_info
                                NULL            // no Overlapped I/o
                                )) {

                /*
                | We've got Partition Information for the device: use it
                */
                bootable = part_info.BootIndicator;

                /*
                | Assign an OID Type "last-arc number" for those file system
                | types we recognize.
                */
                switch (part_info.PartitionType) {

                    case PARTITION_UNIX:             // Unix
                        fs_type = 3;    // "hrFSBerkeleyFFS"
                        break;

                    case PARTITION_FAT_12:           // 12-bit FAT entries
                    case PARTITION_FAT_16:           // 16-bit FAT entries
                    case PARTITION_HUGE:             // Huge partition MS-DOS V4
                    case PARTITION_FAT32:            // FAT32
                    case PARTITION_FAT32_XINT13:     // FAT32 using extended int13 services
                        fs_type = 5;    // "hrFSFat"
                        break;

                    case PARTITION_IFS:              // IFS Partition
                    case VALID_NTFT:                 // NTFT uses high order bits
                        fs_type = 9;    // "hrFSNTFS"
                        break;

                    case PARTITION_XENIX_1:          // Xenix
                    case PARTITION_XENIX_2:          // Xenix
                    case PARTITION_XINT13:           // Win95 partition using extended int13 services
                    case PARTITION_XINT13_EXTENDED:  // Same as type 5 but uses extended int13 services
                    case PARTITION_EXTENDED:         // Extended partition entry
                    case PARTITION_PREP:             // PowerPC Reference Platform (PReP) Boot Partition
                        fs_type = 1;    // "hrFSOther"
                        break;

                    case PARTITION_ENTRY_UNUSED:     // Entry unused
                    default:
                        /* This will translate to fs_type = 2 "unknown" */
                        break;
                    }
                }   /* If (we managed to get partition information) */

            CloseHandle(hdrv);
            }   /* if (we managed to "CreateFile" the device) */

        SetErrorMode(0);        /* Turn error suppression mode off */
        }   /* if (we managed to build a device name) */

    /* =========== hrFSType     ========== */
    row->attrib_list[HRFS_TYPE].attrib_type = CA_NUMBER;
    row->attrib_list[HRFS_TYPE].u.unumber_value = fs_type;


    /* =========== hrFSAccess   ========== */
    /* Quick check: if its a CD-ROM, we presume it is readonly */
    if (drivetype == DRIVE_CDROM) {
        readwrite = 2;
        }
    row->attrib_list[HRFS_ACCESS].attrib_type = CA_NUMBER;
    row->attrib_list[HRFS_ACCESS].u.unumber_value = readwrite;


    /* =========== hrFSBootable ========== */
    row->attrib_list[HRFS_BOOTABLE].attrib_type = CA_NUMBER;
    row->attrib_list[HRFS_BOOTABLE].u.unumber_value = bootable;


    /* =========== hrFSStorageIndex ==========
    | Strategy:
    |
    | We wander up the hrStorage table looking for an exact match between
    | the storage attribute "hrStorageDescr" (which contains the DOS drive
    | string as returned by GetLogicalDriveStrings()) and the current drive
    | string.
    |
    | The first exact match: the index of that hrStorageTable row gets
    | stored here as the value of "hrFSStorageIndex".
    |
    | No Match: Store zero per the RFC spec.
    |
    | Come PnP, this attribute has to become "computed", as entries may come
    | and go from the hrStorage table.
    |
    | NOTE: The length of the comparison of the match is limited by the
    |       drive string we generate in this function, as the "description"
    |       from the hrStorage table may have other stuff appended to the
    |       end of the drive-string.
    */
    row->attrib_list[HRFS_STORINDX].attrib_type = CA_NUMBER;
    storage_index = 0;  /* Presume failure */

    /* Scan the hrStorageTable cache */
    for (row_hrstorage = FindNextTableRow(0, &hrStorage_cache);
         row_hrstorage != NULL;
         row_hrstorage = FindNextTableRow(i, &hrStorage_cache)
         ) {

        /* Obtain the actual row index */
        i = row_hrstorage->index;

        /* If (this entry has an exact match on drive-strings) */
        if (strncmp(row_hrstorage->attrib_list[HRST_DESCR].u.string_value,
                   pDrvStrings,strlen(pDrvStrings)) == 0) {

            /* We found a match, record it and break out */
            storage_index = i;
            break;
            }
        }

    row->attrib_list[HRFS_STORINDX].u.unumber_value = storage_index;


    /* =========== hrFSLastFullBackupDate ==========*/
    row->attrib_list[HRFS_LASTFULL].attrib_type = CA_COMPUTED;


    /* =========== hrFSLastPartialBackupDate ==========*/
    row->attrib_list[HRFS_LASTPART].attrib_type = CA_COMPUTED;


    /*
    | ======================================================
    | Now insert the filled-in CACHEROW structure into the
    | cache-list for the hrFSTable.
    */
    if (AddTableRow(row->attrib_list[HRFS_INDEX].u.unumber_value,  /* Index */
                    row,                                           /* Row   */
                    &hrFSTable_cache                               /* Cache */
                    ) == FALSE) {
        return ( FALSE );       /* Internal Logic Error! */
        }

    /* Step to next string, if any */
    pDrvStrings += strlen(pDrvStrings) + 1;

    } /* while (drive-strings remain . . .) */


free( pOriginal_DrvStrings );


#if defined(CACHE_DUMP)
PrintCache(&hrFSTable_cache);
#endif


/*
| Initialization of this table's cache succeeded
*/

return (TRUE);
}


/* PartitionTypeToLastArc - Convert Partition Type to Last OID Arc value */
/* PartitionTypeToLastArc - Convert Partition Type to Last OID Arc value */
/* PartitionTypeToLastArc - Convert Partition Type to Last OID Arc value */

ULONG
PartitionTypeToLastArc (
                        BYTE p_type
                        )
/*
|  EXPLICIT INPUTS:
|
|       Disk Partition Type as returned in PARTITION_INFORMATINO
|
|  IMPLICIT INPUTS:
|
|       None.
|
|  OUTPUTS:
|
|     On Success:
|       Function returns the value that should be used as the Last-Arc "x"
|       in a "hrFSTypes" Object Identifier.
|
|     On any Failure:
|       Function returns the last-arc value for "UNKNOWN".
|
|  THE BIG PICTURE:
|
|       In more than one spot we need to be able to translate from
|       a Partition Type to our "Last-Arc" value.
|
|  OTHER THINGS TO KNOW:
|
*/
{
ULONG   last_arc = 2;           /* "2" = "UNKNOWN" */

/*
| Assign an OID Type "last-arc number" for those file system
| types we recognize.
*/
switch ( p_type ) {

    case PARTITION_UNIX:             // Unix
        last_arc = 3;    // "hrFSBerkeleyFFS"
        break;

    case PARTITION_FAT_12:           // 12-bit FAT entries
    case PARTITION_FAT_16:           // 16-bit FAT entries
    case PARTITION_HUGE:             // Huge partition MS-DOS V4
    case PARTITION_FAT32:            // FAT32
    case PARTITION_FAT32_XINT13:     // FAT32 using extended int13 services
        last_arc = 5;    // "hrFSFat"
        break;

    case PARTITION_IFS:              // IFS Partition
    case VALID_NTFT:                 // NTFT uses high order bits
        last_arc = 9;    // "hrFSNTFS"
        break;

    case PARTITION_XENIX_1:          // Xenix
    case PARTITION_XENIX_2:          // Xenix
    case PARTITION_XINT13:           // Win95 partition using extended int13 services
    case PARTITION_XINT13_EXTENDED:  // Same as type 5 but uses extended int13 services
    case PARTITION_EXTENDED:         // Extended partition entry
    case PARTITION_PREP:             // PowerPC Reference Platform (PReP) Boot Partition
        last_arc = 1;    // "hrFSOther"
        break;

    case PARTITION_ENTRY_UNUSED:     // Entry unused
    default:
        /* This will translate to "unknown" */
        break;
    }

return ( last_arc );
}

#if defined(CACHE_DUMP)

/* debug_print_hrFSTable - Prints a Row from HrFSTable sub-table */
/* debug_print_hrFSTable - Prints a Row from HrFSTable sub-table */
/* debug_print_hrFSTable - Prints a Row from HrFSTable sub-table */

static void
debug_print_hrFSTable(
                      CACHEROW     *row  /* Row in hrFSTable table */
                      )
/*
|  EXPLICIT INPUTS:
|
|       "row" - points to the row to be dumped, if NULL, the function
|       merely prints a suitable title.
|
|  IMPLICIT INPUTS:
|
|       - Symbols used to reference the attributes in the row entry.
|       - File handle defined by OFILE, presumed to be open.
|
|  OUTPUTS:
|
|     On Success:
|       Function prints a dump of the row in ASCII for debugging purposes
|       on file handle OFILE.
|
|  THE BIG PICTURE:
|
|     Debugging only.
|
|  OTHER THINGS TO KNOW:
*/
{

if (row == NULL) {
    fprintf(OFILE, "=====================\n");
    fprintf(OFILE, "hrFSTable Table Cache\n");
    fprintf(OFILE, "=====================\n");
    return;
    }


fprintf(OFILE, "hrFSIndex. . . . . . . . %d\n",
        row->attrib_list[HRFS_INDEX].u.unumber_value);

fprintf(OFILE, "hrFSMountPoint . . . . . \"%s\" (ALWAYS RETURNED AS EMPTY STRING) \n",
        row->attrib_list[HRFS_MOUNTPT].u.string_value);

fprintf(OFILE, "hrFSRemoteMountPoint . . \"%s\"\n",
        row->attrib_list[HRFS_RMOUNTPT].u.string_value);

fprintf(OFILE, "hrFSType . . . . . . . . %d ",
        row->attrib_list[HRFS_TYPE].u.unumber_value);

switch (row->attrib_list[HRFS_TYPE].u.unumber_value) {
    case 1: fprintf(OFILE, "(hrFSOther)\n");            break;
    case 2: fprintf(OFILE, "(hrFSUnknown)\n");          break;
    case 3: fprintf(OFILE, "(hrFSBerkeleyFFS)\n");      break;
    case 5: fprintf(OFILE, "(hrFSFat)\n");              break;
    case 9: fprintf(OFILE, "(hrFSNTFS)\n");             break;
    default:
            fprintf(OFILE, "(???)\n");
    }


fprintf(OFILE, "hrFSAccess . . . . . . . %d ",
        row->attrib_list[HRFS_ACCESS].u.number_value);
switch (row->attrib_list[HRFS_ACCESS].u.unumber_value) {
    case 1: fprintf(OFILE, "(readWrite)\n"); break;
    case 2: fprintf(OFILE, "(readOnly)\n"); break;
    default: 
            fprintf(OFILE, "(???)\n"); break;
    }


fprintf(OFILE, "hrFSBootable . . . . . . %d ",
        row->attrib_list[HRFS_BOOTABLE].u.number_value);

switch (row->attrib_list[HRFS_BOOTABLE].u.unumber_value) {
    case 0: fprintf(OFILE, "(FALSE)\n"); break;
    case 1: fprintf(OFILE, "(TRUE)\n"); break;
    default: 
            fprintf(OFILE, "(???)\n"); break;
    }

fprintf(OFILE, "hrFSStorageIndex . . . . %d\n",
        row->attrib_list[HRFS_STORINDX].u.number_value);
}
#endif
