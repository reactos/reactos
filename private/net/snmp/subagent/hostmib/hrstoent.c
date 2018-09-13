/*
 *  HrStorageEntry.c v0.10
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
 *    instance name routines for the HrStorageEntry.
 *    Actual instrumentation code is supplied by the developer.
 *
 *  Functions:
 *
 *    A get and set routine for each attribute in the class.
 *
 *    The routines for instances within the class, plus the cache
 *    initialization function "Gen_Hrstorage_Cache()".
 *
 *  Author:
 *
 *	D. D. Burns @ Webenable Inc
 *
 *  Revision History:
 *
 *    V1.00 - 04/17/97  D. D. Burns     Genned: Thu Nov 07 16:40:22 1996
 *    V1.01 - 05/15/97  D. D. Burns     Move Disk Label/Size acquisitions
 *                                       to cache from real-time
 *    V1.02 - 06/18/97  D. D. Burns     Add spt to scan event log for
 *                                       allocation failures
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


/*
|==============================================================================
| Function prototypes for this module.
|
*/
/* ScanLog_Failures - Scan Event Log for Storage Allocation Failures */
static UINT
ScanLog_Failures(
                 CHAR   *device
                 );


#if defined(CACHE_DUMP)

/* debug_print_hrstorage - Prints a Row from Hrstorage sub-table */
static void
debug_print_hrstorage(
                      CACHEROW     *row  /* Row in hrstorage table */
                      );
#endif


/*
|==============================================================================
| Create the list-head for the HrStorage table cache.
|
| This list-head is globally accessible so the logic that loads hrFSTable
| can scan this cache for matches (among other reasons).
|
| (This macro is defined in "HMCACHE.H").
*/
CACHEHEAD_INSTANCE(hrStorage_cache, debug_print_hrstorage);


/*
|==============================================================================
| Local string for this kind of "storage".
*/
#define VM            "Virtual Memory"


/*
 * ============================================================================
 *  GetHrStorageIndex
 *    A unique value for each logical storage are contained by the host.
 *
 *    Gets the value for HrStorageIndex.
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
 | hrStorageIndex
 |
 |  ACCESS         SYNTAX
 |  read-only      INTEGER (1..2147483647)
 |
 | "A unique value for each logical storage area contained by the host."
 |
 | DISCUSSION:
 |
 | The value of this attribute is always the number of drives reported by
 | "GetLogicalDrives" (excepting network drives) plus one (for reporting on
 | "Virtual Memory").
 |
 |============================================================================
 | 1.3.6.1.2.1.25.2.3.1.1.<instance>
 |                | | | |
 |                | | | *hrStorageIndex
 |                | | *hrStorageEntry
 |                | *-hrStorageTable
 |                *-hrStorage
 */

UINT
GetHrStorageIndex(
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
if ((row = FindTableRow(index, &hrStorage_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| Return the "hrStorageIndex" value from this entry
*/
*outvalue = row->attrib_list[HRST_INDEX].u.unumber_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrStorageIndex() */


/*
 * ============================================================================
 *  GetHrStorageType
 *    The type of strage represented by this entry.
 *
 *    Gets the value for HrStorageType.
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
 | hrStorageType
 |
 |  ACCESS         SYNTAX
 |  read-only      OBJECT IDENTIFIER
 |
 | "The type of storage represented by this entry."
 |
 |    -- Registration for some storage types, for use with hrStorageType
 |
 |    hrStorageOther          OBJECT IDENTIFIER ::= { hrStorageTypes 1 }
 |    hrStorageRam            OBJECT IDENTIFIER ::= { hrStorageTypes 2 }
 |    -- hrStorageVirtualMemory is temporary storage of swapped
 |    -- or paged memory
 |    hrStorageVirtualMemory  OBJECT IDENTIFIER ::= { hrStorageTypes 3 }
 |    hrStorageFixedDisk      OBJECT IDENTIFIER ::= { hrStorageTypes 4 }
 |    hrStorageRemovableDisk  OBJECT IDENTIFIER ::= { hrStorageTypes 5 }
 |    hrStorageFloppyDisk     OBJECT IDENTIFIER ::= { hrStorageTypes 6 }
 |    hrStorageCompactDisc    OBJECT IDENTIFIER ::= { hrStorageTypes 7 }
 |    hrStorageRamDisk        OBJECT IDENTIFIER ::= { hrStorageTypes 8 }
 |
 | DISCUSSION:
 |
 | The value returned for this attribute is determined by indications from
 | "GetDriveType" for disks.  For the "Virtual Memory" entry, the OID for
 | "hrStorageVirtualMemory" is returned.
 |
 |============================================================================
 | 1.3.6.1.2.1.25.2.3.1.2.<instance>
 |                | | | |
 |                | | | *hrStorageType
 |                | | *hrStorageEntry
 |                | *-hrStorageTable
 |                *-hrStorage
 */

UINT
GetHrStorageType(
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
if ((row = FindTableRow(index, &hrStorage_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| By convention with the cache-building function "Gen_Hrstorage_Cache()",
| the cached value is the right-most arc we must return as the value.
|
| Hence whatever cache entry we retrieve, we tack the number retrieved
| from the cache for this attribute onto { hrStorageTypes ... }.
*/
if ( (outvalue->ids = SNMP_malloc(10 * sizeof( UINT ))) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }
outvalue->idLength = 10;


/*
| Load in the full hrStorageType OID:
|
| 1.3.6.1.2.1.25.2.1.n
|                | | |
|                | | *-Type indicator
|                | *-hrStorageTypes (OIDs specifying storage types)
|                *-hrStorage
|
*/
outvalue->ids[0] = 1;
outvalue->ids[1] = 3;
outvalue->ids[2] = 6;
outvalue->ids[3] = 1;
outvalue->ids[4] = 2;
outvalue->ids[5] = 1;
outvalue->ids[6] = 25;
outvalue->ids[7] = 2;
outvalue->ids[8] = 1;

/* Cached Type indicator */
outvalue->ids[9] = row->attrib_list[HRST_TYPE].u.unumber_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrStorageType() */


/*
 *  GetHrStorageDesc
 *    A description of the type and instance of the storage described by this
 *    entry.
 *
 *    Gets the value for HrStorageDesc.
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
 | hrStorageDescr
 |
 |  ACCESS         SYNTAX
 |  read-only      DisplayString
 |
 | "A description of the type and instance of the storage described by this
 | entry."
 |
 | DISCUSSION:
 |
 | For the Virtual Memory entry, the string "Virtual Memory" is returned.
 |
 | For disks, a string composed of:
 |         + the logical drive letter followed by
 |         + the Volume Identification (for drives containing a volume)
 |           in double quotes
 |         + the Volume Serial Number
 |
 | For instance, the value of this variable for drive C might be
 |                          C: Label="Main Disk"  Serial #=0030-34FE
 |
 | For speed, the label acquisition is done at cache-build time, and
 | as a consequence the removable drives are sampled only once.
 | ===========================================================================
 | 1.3.6.1.2.1.25.2.3.1.3.<instance>
 |                | | | |
 |                | | | *hrStorageDescr
 |                | | *hrStorageEntry
 |                | *-hrStorageTable
 |                *-hrStorage
 */

UINT
GetHrStorageDesc(
        OUT Simple_DisplayString *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
ULONG           index;          /* As fetched from instance structure     */
CACHEROW       *row;            /* Row entry fetched from cache           */


/*
| Grab the instance information
*/
index = GET_INSTANCE(0);

/*
| Use it to find the right entry in the cache
*/
if ((row = FindTableRow(index, &hrStorage_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/* Return the description that was computed at cache-build time */
outvalue->length = strlen(row->attrib_list[HRST_DESCR].u.string_value);
outvalue->string = row->attrib_list[HRST_DESCR].u.string_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrStorageDesc() */


/*
 *  GetHrStorageAllocationUnits
 *    The size, in bytes, of the data objects allocated from this pool.  If
 *    this entry is monitoring sectors, blocks, buffers, or pack
 *
 *    Gets the value for HrStorageAllocationUnits.
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
 | hrStorageAllocationUnits
 |
 |  ACCESS         SYNTAX
 |  read-only      INTEGER (1..2147483647)
 |
 | "The size, in bytes, of the data objects allocated from this pool.  If this
 | entry is monitoring sectors, blocks, buffers, or packets, for example, this
 | number will commonly be greater than one.  Otherwise this number will
 | typically be one."
 |
 | DISCUSSION:
 |
 | For Virtual Memory, the value returned is that provided as
 | "AllocationGranularity" after a call to "GetSystemInfo".
 |
 | For disks, the size of the "hrStorageAllocationUnits" value is computed as
 | the quantity "BytesPerSector * SectorsPerCluster" as returned by Win32 API
 | function "GetDiskFreeSpace".
 |
 | =============================================================================
 | 1.3.6.1.2.1.25.2.3.1.4.<instance>
 |                | | | |
 |                | | | *hrStorageAllocationUnits
 |                | | *hrStorageEntry
 |                | *-hrStorageTable
 |                *-hrStorage
 */

UINT
GetHrStorageAllocationUnits(
        OUT Integer *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
ULONG           index;          /* As fetched from instance structure     */
CACHEROW       *row;            /* Row entry fetched from cache           */


/*
| Grab the instance information
*/
index = GET_INSTANCE(0);

/*
| Use it to find the right entry in the cache
*/
if ((row = FindTableRow(index, &hrStorage_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

*outvalue = row->attrib_list[HRST_ALLOC].u.number_value;
return ( SNMP_ERRORSTATUS_NOERROR );

} /* end of GetHrStorageAllocationUnits() */


/*
 *  GetHrStorageSize
 *    The size of the storage represented by this entry, in units of
 *    hrStorageAllocationUnits.
 *
 *    Gets the value for HrStorageSize.
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
 | hrStorageSize
 |
 |  ACCESS         SYNTAX
 |  read-write     INTEGER (0..2147483647)
 |
 | "The size of the storage represented by this entry, in units of
 | hrStorageAllocationUnits."
 |
 | DISCUSSION:
 |
 | For Virtual Memory, the value returned is computed as "TotalPageFile" (as
 | returned by "GlobalMemoryStatus") divided by "AllocationGranularity" from
 | "GetSystemInfo".
 |
 | For disks, the "hrStorageSize" value is the value of "TotalNumberOfClusters"
 | as returned by Win32 API function "GetDiskFreeSpace".
 |
 | <POA-4> This variable is marked as ACCESS="read-write".  It is unclear to me
 | what effect can be expected from a SET operation on this variable.  I propose
 | making a SET operation have no effect.
 |
 | RESOLVED >>>>>>>
 | <POA-4> Leaving this read-only is fine.
 | RESOLVED >>>>>>>
 |
 | =============================================================================
 | 1.3.6.1.2.1.25.2.3.1.5.<instance>
 |                | | | |
 |                | | | *hrStorageSize
 |                | | *hrStorageEntry
 |                | *-hrStorageTable
 |                *-hrStorage
 */

UINT
GetHrStorageSize(
        OUT Integer *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
ULONG           index;          /* As fetched from instance structure     */
CACHEROW       *row;            /* Row entry fetched from cache           */


/*
| Grab the instance information
*/
index = GET_INSTANCE(0);

/*
| Use it to find the right entry in the cache
*/
if ((row = FindTableRow(index, &hrStorage_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

*outvalue = row->attrib_list[HRST_SIZE].u.number_value;
return ( SNMP_ERRORSTATUS_NOERROR )  ;

} /* end of GetHrStorageSize() */


/*
 *  SetHrStorageSize
 *    The size of the storage represented by this entry, in units of
 *    hrStorageAllocationUnits.
 *
 *    Sets the HrStorageSize value.
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
 *
 | =============================================================================
 | 1.3.6.1.2.1.25.2.3.1.5.<instance>
 |                | | | |
 |                | | | *hrStorageSize
 |                | | *hrStorageEntry
 |                | *-hrStorageTable
 |                *-hrStorage
 */

UINT
SetHrStorageSize(
        IN Integer *invalue ,
        OUT Integer *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{

#if 0
//debug debug debug debug debug
static int x=0;
if (x==0) {
    /*
    | If it is invoked here, the invocation
    | of it in mib.c must be removed.
    */
    Gen_HrDevice_Cache();
    x =1;
    }
//debug debug debug debug debug
#endif

    return SNMP_ERRORSTATUS_NOSUCHNAME ;

} /* end of SetHrStorageSize() */


/*
 *  GetHrStorageUsed
 *    The amount of the storage represented by this entry that is allocated,
 *    in units of hrStorageAllocationUnits.
 *
 *    Gets the value for HrStorageUsed.
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
 | hrStorageUsed
 |
 |  ACCESS         SYNTAX
 |  read-only      INTEGER (0..2147483647)
 |
 | "The amount of the storage represented by this entry that is allocated, in
 | units of hrStorageAllocationUnits."
 |
 | DISCUSSION:
 |
 | For Virtual Memory, the value returned is computed as the quantity
 | "TotalPageFile" less "AvailPageFile" (as returned by "GlobalMemoryStatus")
 | divided by "AllocationGranularity" (as returned by "GetSystemInfo".
 |
 | For disks, the "hrStorageUsed" value is computed as the quantity
 | "TotalNumberOfClusters - NumberOfFreeClusters" as returned by Win32 API
 | function "GetDiskFreeSpace".
 |
 | ===========================================================================
 | 1.3.6.1.2.1.25.2.3.1.6.<instance>
 |                | | | |
 |                | | | *hrStorageUsed
 |                | | *hrStorageEntry
 |                | *-hrStorageTable
 |                *-hrStorage
 */

UINT
GetHrStorageUsed(
        OUT Integer *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{

ULONG           index;          /* As fetched from instance structure     */
CACHEROW       *row;            /* Row entry fetched from cache           */



/*
| Grab the instance information
*/
index = GET_INSTANCE(0);

/*
| Use it to find the right entry in the cache
*/
if ((row = FindTableRow(index, &hrStorage_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

*outvalue = row->attrib_list[HRST_USED].u.number_value;
return ( SNMP_ERRORSTATUS_NOERROR )  ;

} /* end of GetHrStorageUsed() */


/*
 *  GetHrStorageAllocationFailures
 *    The number of requests for storage represented by this entry that could
 *    not be honored due to not enough storage.  It should be
 *
 *    Gets the value for HrStorageAllocationFailures.
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
 |=============================================================================
 |hrStorageAllocationFailures
 |
 | ACCESS         SYNTAX
 | read-only      Counter
 |
 |"The number of requests for storage represented by this entry that could not
 |be honored due to not enough storage.  It should be noted that as this object
 |has a SYNTAX of Counter, that it does not have a defined initial value.
 |However, it is recommended that this object be initialized to zero."
 |
 |DISCUSSION:
 |
 |<POA-5> This value as very problematical for both Virtual Memory and Disk
 |storage.  There appear to be no Win32 APIs that report allocation failures for
 |either virtual memory or disk storage.  I presume there may be performance
 |monitoring counters stored in the registry, however I'm not able to find the
 |documentation that describes where such information might be stored.  For
 |disks, we need to be able to map whatever counters are stored into logical
 |drives (as that is how this table is organized).
 |
 |RESOLVED >>>>>>>
 |<POA-5> You would have to scan to event log looking for the out of virtual
 |memory and out of disk space events and count them.
 |RESOLVED >>>>>>>
 |=============================================================================
 | 1.3.6.1.2.1.25.2.3.1.7.<instance>
 |                | | | |
 |                | | | *hrStorageAllocationFailures
 |                | | *hrStorageEntry
 |                | *-hrStorageTable
 |                *-hrStorage
 */

UINT
GetHrStorageAllocationFailures(
        OUT Counter *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
CHAR            device[3];      /* Device Name build-buffer             */
ULONG           index;          /* As fetched from instance structure   */
CACHEROW       *row;            /* Row entry fetched from cache         */


/*
| Grab the instance information
*/
index = GET_INSTANCE(0);

/*
| Use it to find the right entry in the cache
*/
if ((row = FindTableRow(index, &hrStorage_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| Load the "device[]" array with two characters, either something like "C:"
| (indicating we're interested in allocation failures for "C:") or "VM"
| if we want VM storage allocation failures.... from the HRST_DESCR string.
*/
if (strcmp(row->attrib_list[HRST_DESCR].u.string_value, VM) == 0) {
    /* Storage is really "Virtual Memory" */
    device[0] = 'V';
    device[1] = 'M';
    }
else {
    device[0] = row->attrib_list[HRST_DESCR].u.string_value[0];
    device[1] = row->attrib_list[HRST_DESCR].u.string_value[1];
    }
device[2] = '\0';       /* Null-terminate */

/* Riffle thru the Event Log looking for this device's failures */
*outvalue = ScanLog_Failures( device );

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrStorageAllocationFailures() */


/*
 *  HrStorageEntryFindInstance
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
HrStorageEntryFindInstance( IN ObjectIdentifier *FullOid ,
                       IN OUT ObjectIdentifier *instance )
{
    UINT tmp_instance ;

    //
    //  Developer instrumentation code to find appropriate instance goes here.
    //  For non-tables, it is not necessary to modify this routine.  However, if
    //  there is any context that needs to be set, it can be done here.
    //

    if ( FullOid->idLength <= HRSTORAGEENTRY_VAR_INDEX )
	// No instance was specified
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else  if ( FullOid->idLength != HRSTORAGEENTRY_VAR_INDEX + 1 )
	// Instance length is more than 1
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else
	// The only valid instance for a non-table are instance 0.  If this
	// is a non-table, the following code validates the instances.  If this
	// is a table, developer modification is necessary below.

	tmp_instance = FullOid->ids[ HRSTORAGEENTRY_VAR_INDEX ] ;

        /*
        | For hrStorage, the instance arc(s) is a single arc, and it must
        | correctly select an entry in the hrStorageTable cache.
        | Check that here.
        */
	if ( FindTableRow(tmp_instance, &hrStorage_cache) == NULL ) {
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

} /* end of HrStorageEntryFindInstance() */



/*
 *  HrStorageEntryFindNextInstance
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
HrStorageEntryFindNextInstance( IN ObjectIdentifier *FullOid ,
                           IN OUT ObjectIdentifier *instance )
{
    //  Developer supplied code to find the next instance of class goes here.
    //
    //  The purpose of this function is to indicate to the rest of the system
    //  what the exact OID of the "next instance" is GIVEN THAT:
    //
    //          a) The "FullOid" passed in will have enough arcs to specify
    //             both the TABLE and the ATTRIBUTE in the table
    //
    //          b) The "instance" OID array is always big enough to have
    //             as many arcs as needed loaded into it by this function
    //             to specify (exactly) the "next instance"
    //
    //
    //  If this is a class with cardinality 1, no modification of this routine
    //  is necessary unless additional context needs to be set.
    //
    //  If the FullOid is so short that it does not specify an instance,
    //  then the only instance of the class should be returned.  If this is a
    //  table, the first row of the table is returned.  To do these things,
    //  set the "instance" OID to just the right arcs such that when
    //  concatenated onto the FullOid, the concatenation exactly specifies
    //  the first instance of this attribute in the table.
    //
    //  If an instance is specified and this is a non-table class, then
    //  NOSUCHNAME is returned so that correct MIB rollover processing occurs.
    //
    //  If this is a table, then the next instance is the one following the
    //  current instance.
    //
    //  If there are no more instances in the table, return NOSUCHNAME.
    //

    CACHEROW        *row;
    ULONG           tmp_instance;


    if ( FullOid->idLength <= HRSTORAGEENTRY_VAR_INDEX )
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
        tmp_instance = FullOid->ids[ HRSTORAGEENTRY_VAR_INDEX ] ;
        }

    /* Now go off and try to find the next instance in the table */
    if ((row = FindNextTableRow(tmp_instance, &hrStorage_cache)) == NULL) {
        return SNMP_ERRORSTATUS_NOSUCHNAME ;
        }

    instance->ids[ 0 ] = row->index ;
    instance->idLength = 1 ;

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of HrStorageEntryFindNextInstance() */



/*
 *  HrStorageEntryConvertInstance
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
HrStorageEntryConvertInstance( IN ObjectIdentifier *oid_spec ,
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

} /* end of HrStorageEntryConvertInstance() */




/*
 *  HrStorageEntryFreeInstance
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
HrStorageEntryFreeInstance( IN OUT InstanceName *instance )
{

  /* No action needed for hrStorageTable */
} /* end of HrStorageEntryFreeInstance() */

/*
| End of Generated Code
*/

/* Gen_Hrstorage_Cache - Generate a initial cache for HrStorage Table */
/* Gen_Hrstorage_Cache - Generate a initial cache for HrStorage Table */
/* Gen_Hrstorage_Cache - Generate a initial cache for HrStorage Table */

BOOL
Gen_Hrstorage_Cache(
                    void
                    )

/*
|  EXPLICIT INPUTS:
|
|       None.
|
|  IMPLICIT INPUTS:
|
|       The module-local head of the cache for the HrStorage table,
|       "hrStorage_cache".
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
|  DISCUSSION:
|
|  Since this table is meant for diagnosing "out-of-storage" situations and
|  given that the information is meant to be regarded from the point of view
|  of an application, we simply report every instance of what would appear
|  on the "Drive Bar" of the File Manager (excepting network drives) plus
|  one more table entry to reflect information on "Virtual Memory".
|
|  To this end, a combination of Win32 API functions "GetLogicalDrives",
|  "GetVolumeInformation", "GetDriveType" and "GetDiskFreeSpace" are used to
|  acquire the information for the SNMP attributes in this table.
|
|  For reporting on "Virtual Memory", functions "GlobalMemoryStatus" and
|  "GetSystemInfo" are invoked.
|============================================================================
| 1.3.6.1.2.1.25.2.1....
|                | |
|                | *-hrStorageTypes (OIDs specifying storage types)
|                *-hrStorage
|
| 1.3.6.1.2.1.25.2.2....
|                | |
|                | *-hrMemorySize   (standalone attribute)
|                *-hrStorage
|
| 1.3.6.1.2.1.25.2.3....
|                | |
|                | *-hrStorageTable (the table)
|                *-hrStorage
*/

#define VOL_NAME_SIZE 256
#define DESCR_SIZE    384

{
CHAR    temp[8];                /* Temporary buffer for first call         */
LPSTR   pDrvStrings;            /* --> allocated storage for drive strings */
LPSTR   pOriginal_DrvStrings;   /* (Needed for final deallocation          */
DWORD   DS_request_len;         /* Storage actually needed                 */
DWORD   DS_current_len;         /* Storage used on 2nd call                */
ULONG   table_index=0;          /* hrStorageTable index counter            */
CACHEROW *row;                  /* --> Cache structure for row-being built */

SYSTEM_INFO     sys_info;       /* Filled in by GetSystemInfo for VM       */

LPSTR   str_descr;              /* String for disk label/serial description*/
TCHAR   volname[VOL_NAME_SIZE+2];            /* Volume Name returned here  */
DWORD   volnamesize=VOL_NAME_SIZE;           /* Size of volname buffer     */
DWORD   serial_number;          /* Volume Serial Number                    */
DWORD   max_comp_len;           /* File system file-name component length  */
DWORD   filesys_flags;          /* File System flags (GetVolInformation)   */
CHAR    descr[DESCR_SIZE];      /* Full description possibly built here    */

DWORD   SectorsPerCluster;      /* GetDiskFreeSpace() cells */
DWORD   BytesPerSector;
DWORD   NumberOfFreeClusters;
DWORD   TotalNumberOfClusters;

MEMORYSTATUS    mem_status;     /* Filled in by GlobalMemoryStatus        */


/*
| We're going to call GetLogicalDriveStrings() twice, once to get the proper
| buffer size, and the second time to actually get the drive strings.
|
| Bogus:
*/
if ((DS_request_len = GetLogicalDriveStrings(2, temp)) == 0) {

    /* Request failed altogether, can't initialize */
    return ( FALSE );
    }

/*
| Grab enough storage for the drive strings plus one null byte at the end
*/

if ( (pOriginal_DrvStrings = pDrvStrings = malloc( (DS_request_len + 2) ) )
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
| As long as we've got an unprocessed drive-string. . .
*/
while ( strlen(pDrvStrings) > 0 ) {

    UINT        drivetype;      /* Type of the drive from "GetDriveType()" */


    /*
    | Get the drive-type so we can decide whether it should participate in
    | this table.
    */
    drivetype = GetDriveType(pDrvStrings);

    if (   drivetype == DRIVE_UNKNOWN
        || drivetype == DRIVE_NO_ROOT_DIR
        || drivetype == DRIVE_REMOTE            /* No Remotes in HrStorage */
        ) {

        /* Step to next string, if any */
        pDrvStrings += strlen(pDrvStrings) + 1;

        continue;
        }

    /*
    | OK, we want this one in the table, get a row-entry created.
    */
    if ((row = CreateTableRow( HRST_ATTRIB_COUNT ) ) == NULL) {
        return ( FALSE );       // Out of memory
        }

    /* =========== hrStorageIndex ==========*/
    row->attrib_list[HRST_INDEX].attrib_type = CA_NUMBER;
    row->attrib_list[HRST_INDEX].u.unumber_value = ++table_index;


    /* =========== hrStorageType ==========*/
    row->attrib_list[HRST_TYPE].attrib_type = CA_NUMBER;

    /*
    | Based on the drive-type returned, we store a single number as
    | the cached value of the hrStorageType attribute.  When this attribute
    | is fetched, the cached number forms the last arc in the OBJECT IDENTIFIER
    | that actually specifies the type: { hrStorageTypes x }, where "x" is
    | what gets stored.
    */
    switch (drivetype) {

        case DRIVE_REMOVABLE:
            row->attrib_list[HRST_TYPE].u.unumber_value = 5;
            break;

        case DRIVE_FIXED:
            row->attrib_list[HRST_TYPE].u.unumber_value = 4;
            break;

        case DRIVE_CDROM:
            row->attrib_list[HRST_TYPE].u.unumber_value = 7;
            break;

        case DRIVE_RAMDISK:
            row->attrib_list[HRST_TYPE].u.unumber_value = 8;
            break;

        default:
            row->attrib_list[HRST_TYPE].u.unumber_value = 1;  // "Other"
            break;
        }


    /* =========== hrStorageDescr ==========
    |
    |  We try and fetch the volume label here, to get a string
    |  that may look like:
    |           C: Label="Main Disk"  Serial #=0030-34FE
    |
    | Handle all kinds of disk storage here:
    |
    |   Try to get volume label and serial number.  If we fail, we just give
    |   'em the root-path name.
    |
    |   Presume that we'll fail, and just return the root-path string.
    */
    str_descr = pDrvStrings;

    /*
    | Suppress any attempt by the system to make the user put a volume in a
    | removable drive.
    */
    SetErrorMode(SEM_FAILCRITICALERRORS);

    if (GetVolumeInformation(pDrvStrings,       /* drive name           */
                             volname,           /* Volume Name Buffer   */
                             volnamesize,       /* Size of buffer       */
                             &serial_number,    /* Vol. # Returned here */
                             &max_comp_len,     /* Max filename length  */
                             &filesys_flags,    /* File System flags    */
                             NULL,              /* Name of Filesystem   */
                             0                  /* Length of name       */
                             )) {
        /*
        | We got something back.
        |
        | If we have room given string lengths, build a description:
        |
        |    <root path> Label:<volume name>  Serial Number: <#>
        */
        #define SPRINTF_FORMAT "%s Label:%s  Serial Number %x"

        if ((strlen(SPRINTF_FORMAT) + strlen(volname) + strlen(str_descr))
            < DESCR_SIZE) {
            sprintf(descr,
                    SPRINTF_FORMAT,
                    str_descr,          // root-path
                    volname,            // volume name
                    serial_number);     // volume serial #
            str_descr = descr;
            }
        }


    row->attrib_list[HRST_DESCR].attrib_type = CA_STRING;

    /*
    | Note:
    |       The convention is established that the first characters of
    |       this description string is always the device-string (e.g. "C:")
    |       or the value of local symbol "VM" ("Virtual Memory").
    |
    |       Code in "GetHrStorageAllocationFailures()" attempts to extract
    |       the drive letter (or "Virtual Memory") from the beginning of this
    |       string in order to determine allocation failures from the
    |       Event Log(!).
    */
    if ( (row->attrib_list[HRST_DESCR].u.string_value
          = ( LPSTR ) malloc(strlen(str_descr) + 1)) == NULL) {
        return ( FALSE );       /* out of memory */
        }
    /* Copy the Value into the space */
    strcpy(row->attrib_list[HRST_DESCR].u.string_value, str_descr);

    row->attrib_list[HRST_ALLOC].attrib_type = CA_NUMBER;
    row->attrib_list[HRST_SIZE].attrib_type = CA_NUMBER;
    row->attrib_list[HRST_USED].attrib_type = CA_NUMBER;

    /*
    | Handle all kinds of disk storage info here:
    |
    |   Try to get volume statistics via GetDiskFreeSpace().
    */
    if (GetDiskFreeSpace(pDrvStrings,           // drive
                         &SectorsPerCluster,
                         &BytesPerSector,
                         &NumberOfFreeClusters,
                         &TotalNumberOfClusters
                         )) {
        /* Success */

        /* =========== hrStorageAllocationUnits ==========*/
        row->attrib_list[HRST_ALLOC].u.unumber_value =
                          BytesPerSector * SectorsPerCluster;

        /* =========== hrStorageSize ==========*/
        row->attrib_list[HRST_SIZE].u.unumber_value = TotalNumberOfClusters;

        /* =========== hrStorageUsed ==========*/
        row->attrib_list[HRST_USED].u.unumber_value =
            TotalNumberOfClusters - NumberOfFreeClusters;
        }
    else {
        /* Failure */

        /* =========== hrStorageAllocationUnits ==========*/
        row->attrib_list[HRST_ALLOC].u.unumber_value = 0;

        /* =========== hrStorageSize ==========*/
        row->attrib_list[HRST_SIZE].u.unumber_value = 0;

        /* =========== hrStorageUsed ==========*/
        row->attrib_list[HRST_USED].u.unumber_value = 0;
        }

    /* =========== hrStorageAllocationFailures ==========*/
    row->attrib_list[HRST_FAILS].attrib_type = CA_COMPUTED;

    SetErrorMode(0);        /* Turn error suppression mode off */

    /*
    | Now insert the filled-in CACHEROW structure into the
    | cache-list for the hrStorageTable.
    */
    if (AddTableRow(row->attrib_list[HRST_INDEX].u.unumber_value,  /* Index */
                    row,                                           /* Row   */
                    &hrStorage_cache                               /* Cache */
                    ) == FALSE) {
        return ( FALSE );       /* Internal Logic Error! */
        }

    /* Step to next string, if any */
    pDrvStrings += strlen(pDrvStrings) + 1;
    }


free( pOriginal_DrvStrings );

/*
|==============================================================================
| Now handle Virtual Memory as a special case.
|==============================================================================
*/
if ((row = CreateTableRow( HRST_ATTRIB_COUNT ) ) == NULL) {
    return ( FALSE );       // Out of memory
    }

/* =========== hrStorageIndex ==========*/
row->attrib_list[HRST_INDEX].attrib_type = CA_NUMBER;
row->attrib_list[HRST_INDEX].u.unumber_value = ++table_index;

/* =========== hrStorageType ==========*/
row->attrib_list[HRST_TYPE].attrib_type = CA_NUMBER;
row->attrib_list[HRST_TYPE].u.unumber_value = 3;        /* Virtual Memory */

/* =========== hrStorageDescr ==========*/
row->attrib_list[HRST_DESCR].attrib_type = CA_STRING;
if ( (row->attrib_list[HRST_DESCR].u.string_value
      = ( LPSTR ) malloc(strlen(VM) + 1)) == NULL) {
    return ( FALSE );       /* out of memory */
    }
strcpy(row->attrib_list[HRST_DESCR].u.string_value, VM);


/* =========== hrStorageAllocationUnits ==========*/
GetSystemInfo(&sys_info);
row->attrib_list[HRST_ALLOC].attrib_type = CA_NUMBER;
row->attrib_list[HRST_ALLOC].u.unumber_value =
                                             sys_info.dwAllocationGranularity;

/* =========== hrStorageSize ==========*/
/* Acquire current memory statistics */
GlobalMemoryStatus(&mem_status);

row->attrib_list[HRST_SIZE].attrib_type = CA_NUMBER;
row->attrib_list[HRST_SIZE].u.unumber_value =
    (DWORD)(mem_status.dwTotalPageFile / sys_info.dwAllocationGranularity);


/* =========== hrStorageUsed ==========*/
row->attrib_list[HRST_USED].attrib_type = CA_NUMBER;
row->attrib_list[HRST_USED].attrib_type =
    (int)((mem_status.dwTotalPageFile - mem_status.dwAvailPageFile)
                           / sys_info.dwAllocationGranularity);

/* =========== hrStorageAllocationFailures ==========*/
row->attrib_list[HRST_FAILS].attrib_type = CA_COMPUTED;

/*
| Now insert the filled-in CACHEROW structure into the
| cache-list for the hrStorageTable.
*/
if (AddTableRow(row->attrib_list[HRST_INDEX].u.unumber_value,  /* Index */
                row,                                           /* Row   */
                &hrStorage_cache                               /* Cache */
                ) == FALSE) {
    return ( FALSE );       /* Internal Logic Error! */
    }
/*
|==============================================================================
| End of Virtual Memory
|==============================================================================
*/


#if defined(CACHE_DUMP)
PrintCache(&hrStorage_cache);
#endif

/*
| Initialization of this table's cache succeeded
*/
return (TRUE);
}

/* ScanLog_Failures - Scan Event Log for Storage Allocation Failures */
/* ScanLog_Failures - Scan Event Log for Storage Allocation Failures */
/* ScanLog_Failures - Scan Event Log for Storage Allocation Failures */

static UINT
ScanLog_Failures(
                 CHAR   *device
                 )

/*
|  EXPLICIT INPUTS:
|
|       "device" is either the string "VM" (for "Virtual Memory") or
|       the logical device for which we're looking for failures (e.g. "C:").
|
|  IMPLICIT INPUTS:
|
|       The System Event log file.
|
|  OUTPUTS:
|
|     On Success/Failure:
|       Function returns the number of storage allocation failures
|       found for the specified device.
|
|  THE BIG PICTURE:
|
|       This is a "helper" function for routine "GetHrStorageAllocationFailures"
|       within this module.
|
|  OTHER THINGS TO KNOW:
|
|       We scan backwards (in time) thru the Event Log until we hit the
|       "Event Logging Started" event record (because, presumably, we don't
|       care about failures that happened before the system last came up).
*/

/*
| These symbols select the "Event Log Started" informational message.
*/
#define EVENTLOG_START_ID   0x80001775
#define EVENTLOG_START_TYPE 4
#define EVENTLOG_START_SRC  "EventLog"

{
#define EVL_BUFFER_SIZE 2048
EVENTLOGRECORD *pevlr;
BYTE            bBuffer[EVL_BUFFER_SIZE];
DWORD           dwRead, dwNeeded, cRecords;
HANDLE          h;
BOOL            keep_scanning = TRUE;
UINT            alloc_failures = 0;

/*
| Open the System event log
*/
h = OpenEventLog(NULL,      /* local computer */
                 "System"   /* source name    */
                 );

if (h == NULL) {
    return ( alloc_failures );
    }

pevlr = (EVENTLOGRECORD *) &bBuffer;

/*
| Read records sequentially "Backward in Time" until there
| are no more, or we hit the "Event Logging Started" event.
|
| Read a "Slug 'o Records":
*/
while (ReadEventLog(h,                      // event log handle
                    EVENTLOG_BACKWARDS_READ | // reads backward
                    EVENTLOG_SEQUENTIAL_READ, // sequential read
                    0,                      // ignored for sequential reads
                    pevlr,                  // address of buffer
                    EVL_BUFFER_SIZE,        // size of buffer
                    &dwRead,                // count of bytes read
                    &dwNeeded)              // bytes in next record
       && keep_scanning == TRUE) {


    /* Wind down thru this "slug" . . . */
    while (dwRead > 0) {

        /*
        | Check for "Event Logging Started"
        |
        | (The source name is just past the end of the formal structure).
        */
        if (   pevlr->EventID == EVENTLOG_START_ID
            && pevlr->EventType == EVENTLOG_START_TYPE
            && strcmp( ((LPSTR) ((LPBYTE) pevlr + sizeof(EVENTLOGRECORD))),
                      EVENTLOG_START_SRC) == 0
            ) {
            keep_scanning = FALSE;
            break;
            }

//============================================================================
// INSERT RECORD CHECKING LOGIC OF THIS SORT HERE:
//
//      IF (    <eventrecordID>           == pevlr->EventID
//           && <eventrecordtype>         == pevlr->EventType
//           && <eventrecordsourcestring> == (is the same as..)
//                     ( (LPSTR) ((LPBYTE) pevlr + sizeof(EVENTLOGRECORD)) )
//         ) {
//              // It's an allocation-failure record, if it is for device
//              // "device", then count it.
//              IF (strcmp(device, <eventrecordinstance-data>) {
//                  alloc_failures += 1;
//                  }
//           }
//============================================================================

        dwRead -= pevlr->Length;
        pevlr = (EVENTLOGRECORD *)
            ((LPBYTE) pevlr + pevlr->Length);
        }

    pevlr = (EVENTLOGRECORD *) &bBuffer;
    }

CloseEventLog(h);

/* Give 'em the count */
return (alloc_failures);
}

#if defined(CACHE_DUMP)

/* debug_print_hrstorage - Prints a Row from Hrstorage sub-table */
/* debug_print_hrstorage - Prints a Row from Hrstorage sub-table */
/* debug_print_hrstorage - Prints a Row from Hrstorage sub-table */

static void
debug_print_hrstorage(
                      CACHEROW     *row  /* Row in hrstorage table */
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
    fprintf(OFILE, "hrStorage Table Cache\n");
    fprintf(OFILE, "=====================\n");
    return;
    }


fprintf(OFILE, "hrStorageIndex . . . . . %d\n",
        row->attrib_list[HRST_INDEX].u.unumber_value);

fprintf(OFILE, "hrStorageType. . . . . . %d ",
        row->attrib_list[HRST_TYPE].u.unumber_value);

switch (row->attrib_list[HRST_TYPE].u.unumber_value) {
    case 1: fprintf(OFILE, "(Other)\n");        break;
    case 2: fprintf(OFILE, "(RAM)\n");        break;
    case 3: fprintf(OFILE, "(Virtual Memory)\n");        break;
    case 4: fprintf(OFILE, "(Fixed Disk)\n");        break;
    case 5: fprintf(OFILE, "(Removable Disk)\n");        break;
    case 6: fprintf(OFILE, "(Floppy Disk)\n");        break;
    case 7: fprintf(OFILE, "(Compact Disk)\n");        break;
    case 8: fprintf(OFILE, "(RAM Disk)\n");        break;
    default:
            fprintf(OFILE, "(Unknown)\n");
    }


fprintf(OFILE, "hrStorageDescr . . . . . \"%s\"\n",
        row->attrib_list[HRST_DESCR].u.string_value);

fprintf(OFILE, "hrStorageAllocationUnits %d\n",
        row->attrib_list[HRST_ALLOC].u.number_value);

fprintf(OFILE, "hrStorageSize. . . . . . %d\n",
        row->attrib_list[HRST_SIZE].u.number_value);

fprintf(OFILE, "hrStorageUsed. . . . . . %d\n",
        row->attrib_list[HRST_USED].u.number_value);

fprintf(OFILE, "hrStorageAllocationFails (Computed)\n");
}
#endif
