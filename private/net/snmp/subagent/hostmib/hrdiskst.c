/*
 *  HrDiskStorageEntry.c v0.10
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
 *    instance name routines for the HrDiskStorageEntry.  Actual 
 *    instrumentation code is supplied by the developer.
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
 *    V1.00 - 04/28/97  D. D. Burns     Genned: Thu Nov 07 16:43:17 1996
 *
 */


#include <windows.h>
#include <malloc.h>
#include <stdio.h>        /* For sprintf                        */
#include <string.h>
#include <snmp.h>

#include "mib.h"
#include "smint.h"
#include "hostmsmi.h"
#include "user.h"         /* Developer supplied include file    */
#include "HMCACHE.H"      /* Cache-related definitions          */
#include "HRDEVENT.H"     /* HrDevice Table-related definitions */
#include <winioctl.h>     /* For PARTITION_INFORMATION et. al.  */


/*
|==============================================================================
| Function prototypes for this module.
|
*/
/* Gen_nonFixed_disks - Scan for Floppies and CD-ROMS */
static BOOL Gen_nonFixed_disks ( ULONG type_arc );

/* Gen_Fixed_disks - Scan for Fixed Disks */
static BOOL Gen_Fixed_disks ( ULONG type_arc );

/* ProcessPartitions - Process Partition Information into HrDevice Row */
static BOOL ProcessPartitions(
                  HANDLE        hdrv,   /* Fixed-Disk containing partitions */
                  CACHEROW     *dv_row, /* Row in hrDevice table for disk   */
                  CHAR         *pntdev  /* NT Device name for physical disk */
                  );

/* Process_DS_Row - Process Information into HrDevice and hrDiskStorage Row */
static CACHEROW *Process_DS_Row ( 
                ULONG       type_arc,  /* hrDeviceType last arc value        */
                ULONG       access,    /* hrDiskStorageAccess = readWrite(1) */
                ULONG       media,     /* hrDiskStorageMedia = floppyDisk(4) */
                ULONG       removable, /* hrDiskStorageRemovable = TRUE      */
                ULONG       capacityKB,/* hrDiskStorageCapacity, (kilobytes) */
                ULONG       status,    /* hrDeviceStatus = unknown(1)        */
                CHAR       *descr      /* hrDeviceDescr string               */
                );

/* FindPartitionLabel - Find MS-DOS Device Label for a Fixed-Disk Partition */
static PCHAR
FindPartitionLabel(
                   CHAR   *pntdev, /* NT Device name for physical disk */
                   UINT   part_id  /* Partition Number (1-origined)    */
                   );

/* debug_print_hrdiskstorage - Prints a Row from HrDiskStorage sub-table */
static void
debug_print_hrdiskstorage(
                          CACHEROW     *row  /* Row in hrDiskStorage table */
                          );

/* debug_print_hrpartition - Prints a Row from HrPartition sub-table */
static void
debug_print_hrpartition(
                        CACHEROW     *row  /* Row in hrPartition table */
                        );

/*
|==============================================================================
| Create the list-head for the HrDiskStorage cache.
|
| (This macro is defined in "HMCACHE.H").
*/
CACHEHEAD_INSTANCE(hrDiskStorage_cache, debug_print_hrdiskstorage);




/*
 *  GetHrDiskStorageAccess
 *    An indication if this long-term storage device is readable and writable 
 *    or only readable.  This should reflect the media type, a
 *    
 *    Gets the value for HrDiskStorageAccess.
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
 | hrDiskStorageAccess
 | 
 |  ACCESS         SYNTAX
 |  read-only      INTEGER {readWrite(1), readOnly(2)}
 | 
 | "An indication if this long-term storage device is readable and writable or
 | only readable.  This should reflect the media type, any write-protect
 | mechanism, and any device configuration that affects the entire device."
 | 
 | DISCUSSION:
 | 
 | This information for the entire drive is obtained using Win32 API CreateFile
 | to open the device and DeviceIoControl to retrieve the needed information.
 |
 |============================================================================
 | 1.3.6.1.2.1.25.3.6.1.1.<instance>
 |                | | | |
 |                | | | *-hrDiskStorageAccess
 |                | | *-hrDiskStorageEntry
 |                | *-hrDiskStorageTable (the table)
 |                *-hrDevice
 */

UINT
GetHrDiskStorageAccess( 
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
if ((row = FindTableRow(index, &hrDiskStorage_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| Return the "hrDiskStorageAccess" value from this entry
*/
*outvalue = row->attrib_list[HRDS_ACCESS].u.unumber_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrDiskStorageAccess() */


/*
 *  GetHrDiskStorageMedia
 *    An indication of the type of media used in this long-term storage device.
 *    
 *    Gets the value for HrDiskStorageMedia.
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
 | hrDiskStorageMedia
 | 
 |  ACCESS         SYNTAX
 |  read-only      INTEGER {other(1),unknown(2),hardDisk(3),floppyDisk(4),
 |                          opticalDiskROM(5),opticalDiskWORM(6),opticalDiskRW(7),
 |                          ramDisk(8)}
 | 
 | "An indication of the type of media used in this long-term storage device."
 | 
 | Discussion
 | 
 | This information for the entire drive is obtained using Win32 API CreateFile
 | to open the device and DeviceIoControl to retrieve the needed information.
 |
 |============================================================================
 | 1.3.6.1.2.1.25.3.6.1.2.<instance>
 |                | | | |
 |                | | | *-hrDiskStorageMedia
 |                | | *-hrDiskStorageEntry
 |                | *-hrDiskStorageTable (the table)
 |                *-hrDevice
 */

UINT
GetHrDiskStorageMedia( 
        OUT INThrDiskStorageMedia *outvalue ,
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
if ((row = FindTableRow(index, &hrDiskStorage_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| Return the "hrDiskStorageAccess" value from this entry
*/
*outvalue = row->attrib_list[HRDS_MEDIA].u.unumber_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrDiskStorageMedia() */


/*
 *  GetHrDiskStorageRemoveble
 *    Denotes whether or not the disk media may be removed from the drive.
 *    
 *    Gets the value for HrDiskStorageRemoveble.
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
 | hrDiskStorageRemoveble
 | 
 |  ACCESS         SYNTAX
 |  read-only      Boolean
 | 
 | "Denotes whether or not the disk media may be removed from the drive."
 | 
 | DISCUSSION:
 | 
 | This information for the entire drive is obtained using Win32 API CreateFile
 | to open the device and DeviceIoControl to retrieve the needed information.
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.6.1.3.<instance>
 |                | | | |
 |                | | | *-hrDiskStorageRemovable
 |                | | *-hrDiskStorageEntry
 |                | *-hrDiskStorageTable (the table)
 |                *-hrDevice
 */

UINT
GetHrDiskStorageRemoveble( 
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
if ((row = FindTableRow(index, &hrDiskStorage_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| Return the "hrDiskStorageRemovable" value from this entry
*/
*outvalue = row->attrib_list[HRDS_REMOVABLE].u.unumber_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrDiskStorageRemoveble() */


/*
 *  GetHrDiskStorageCapacity
 *    The total size for this long-term storage device.
 *    
 *    Gets the value for HrDiskStorageCapacity.
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
 | hrDiskStorageCapacity
 | 
 |  ACCESS         SYNTAX
 |  read-only      KBytes
 | 
 | "The total size for this long-term storage device."
 | 
 | DISCUSSION:
 | 
 | This information for the entire drive is obtained using Win32 API CreateFile
 | to open the device and DeviceIoControl to retrieve the needed information.
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.6.1.4.<instance>
 |                | | | |
 |                | | | *-hrDiskStorageCapacity
 |                | | *-hrDiskStorageEntry
 |                | *-hrDiskStorageTable (the table)
 |                *-hrDevice
 */

UINT
GetHrDiskStorageCapacity( 
        OUT KBytes *outvalue ,
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
if ((row = FindTableRow(index, &hrDiskStorage_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| Return the "hrDiskStorageCapacity" value from this entry
*/
*outvalue = row->attrib_list[HRDS_CAPACITY].u.unumber_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrDiskStorageCapacity() */


/*
 *  HrDiskStorageEntryFindInstance
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
HrDiskStorageEntryFindInstance( IN ObjectIdentifier *FullOid ,
                       IN OUT ObjectIdentifier *instance )
{
    UINT tmp_instance ;

    //
    //  Developer instrumentation code to find appropriate instance goes here.
    //  For non-tables, it is not necessary to modify this routine.  However, if
    //  there is any context that needs to be set, it can be done here.
    //

    if ( FullOid->idLength <= HRDISKSTORAGEENTRY_VAR_INDEX )
	// No instance was specified
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else  if ( FullOid->idLength != HRDISKSTORAGEENTRY_VAR_INDEX + 1 )
	// Instance length is more than 1
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else
	// The only valid instance for a non-table are instance 0.  If this
	// is a non-table, the following code validates the instances.  If this
	// is a table, developer modification is necessary below.

	tmp_instance = FullOid->ids[ HRDISKSTORAGEENTRY_VAR_INDEX ] ;

        /*
        | For hrDiskStorage, the instance arc(s) is a single arc, and it must
        | correctly select an entry in the hrDiskStorage cache.
        | Check that here.
        |
        | Note that if there is a row there, there is also one in the
        | hrDevice table with the same index.
        */
	if ( FindTableRow(tmp_instance, &hrDiskStorage_cache) == NULL ) {
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

} /* end of HrDiskStorageEntryFindInstance() */



/*
 *  HrDiskStorageEntryFindNextInstance
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
HrDiskStorageEntryFindNextInstance( IN ObjectIdentifier *FullOid ,
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
    //  If an instance is specified and this is a non-table class, then 
    //  NOSUCHNAME is returned so that correct MIB rollover processing occurs.
    //  If this is a table, then the next instance is the one following the 
    //  current instance.
    //
    //  If there are no more instances in the table, return NOSUCHNAME.
    //

    CACHEROW        *row;
    ULONG           tmp_instance;


    if ( FullOid->idLength <= HRDISKSTORAGEENTRY_VAR_INDEX )
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
        tmp_instance = FullOid->ids[ HRDISKSTORAGEENTRY_VAR_INDEX ] ;
        }

    /* Now go off and try to find the next instance in the table */
    if ((row = FindNextTableRow(tmp_instance, &hrDiskStorage_cache)) == NULL) {
        return SNMP_ERRORSTATUS_NOSUCHNAME ;
        }

    instance->ids[ 0 ] = row->index ;
    instance->idLength = 1 ;

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of HrDiskStorageEntryFindNextInstance() */



/*
 *  HrDiskStorageEntryConvertInstance
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
HrDiskStorageEntryConvertInstance( IN ObjectIdentifier *oid_spec ,
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

} /* end of HrDiskStorageEntryConvertInstance() */




/*
 *  HrDiskStorageEntryFreeInstance
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
HrDiskStorageEntryFreeInstance( IN OUT InstanceName *instance )
{

  /* No action needed for hrDiskStorageTable */
} /* end of HrDiskStorageEntryFreeInstance() */

/*
| End of Generated Code
*/

/* Gen_HrDiskStorage_Cache - Generate a initial cache for HrDiskStorage Table */
/* Gen_HrDiskStorage_Cache - Generate a initial cache for HrDiskStorage Table */
/* Gen_HrDiskStorage_Cache - Generate a initial cache for HrDiskStorage Table */

BOOL
Gen_HrDiskStorage_Cache(
                    ULONG type_arc
                    )

/*
|  EXPLICIT INPUTS:
|
|       "type_arc" is the number "n" to be used as the last arc in the
|       device-type OID:
|
|        1.3.6.1.2.1.25.3.1.n
|                       | | |
|                       | | * Identifying arc for type
|                       | *-hrDeviceTypes (OIDs specifying device types)
|                       *-hrDevice
|
|        for devices created by this cache-population routine.
|
|  IMPLICIT INPUTS:
|
|       The module-local head of the cache for the HrDiskStorage table,
|       "HrDiskStorage_cache".
|
|  OUTPUTS:
|
|     On Success:
|       Function returns TRUE indicating that both caches have been fully
|       populated with all "static" cache-able values.  This function populates
|       not only the hrDevice table cache but the hrDiskStorage cache as well.
|
|     On any Failure:
|       Function returns FALSE (indicating "not enough storage").
|
|  THE BIG PICTURE:
|
|       At subagent startup time, the cache for each table in the MIB is
|       populated with rows for each row in the table.  This function is
|       invoked by the start-up code in "Gen_HrDevice_Cache()" ("HRDEVENT.C") 
|       to populate the cache for the HrDiskStorage table AND the hrDevice
|       Table.
|
|  OTHER THINGS TO KNOW:
|
|       There is one of these function for every table that has a cache.
|       Each is found in the respective source file.
|
|       This function differs from all of the other corresponding sub-table
|       function instances in that this sub-table has its own cache, rather
|       than depending solely on that of the hrDevice table.
|
|       As a consequence, we don't need fancy logic in the FindInstance()
|       and FindNextInstance() functions to determine whether a particular
|       instance is valid: if it is, there is a row entry in the local
|       hrDiskStorage cache.
|
|       As another consequence, this function must load both caches with
|       data (and it must use the same "index" numbers in both caches
|       for each row entered).
|
|       ----
|
|       The strategy on getting all disks goes like this:
|
|       * Since the "\\.\PHYSICALDRIVEn" trick with "CreateFile" doesn't allow
|         access to floppies or CD-ROMs, we process these separately as a
|         first step.
|
|         + We presume "A:" and "B:" may be floppies and we look for them
|           explicitly.  Any found are presumed "readWrite" and Removable.
|           If a medium is present, we may get a full description plus an
|           accurate storage size, otherwise we just don't know for sure,
|           (DeviceIoControl for drive info fails if no disk is in the
|            floppy drive).
|
|         + We then scan all logical drive strings looking for instances of 
|           CD-ROM drives. Any found are presumed "readOnly" and Removable.
|           We can't obtain storage sizes even if a disk is present for these,
|           so storage is left at zero.
|
|       * Then the  "\\.\PHYSICALDRIVEn" trick is used to enumerate the real
|         hard disks, and real storage sizes are obtainable.
|
|============================================================================
| 1.3.6.1.2.1.25.3.6....
|                | |
|                | *-hrDiskStorageTable (the table)
|                *-hrDevice
*/
{

/*
| Do Floppies and CD-ROM drives (non-fixed disks)
*/
if (Gen_nonFixed_disks( type_arc ) == FALSE) {
    return ( FALSE );
    }

/*
| Do Fixed drives
*/
if (Gen_Fixed_disks( type_arc ) == FALSE) {
    return ( FALSE );
    }

/* Success */
return ( TRUE );
}

/* Gen_nonFixed_disks - Scan for Floppies and CD-ROMS */
/* Gen_nonFixed_disks - Scan for Floppies and CD-ROMS */
/* Gen_nonFixed_disks - Scan for Floppies and CD-ROMS */

static BOOL
Gen_nonFixed_disks ( 
                    ULONG type_arc
                    )
/*
|  EXPLICIT INPUTS:
|
|       "type_arc" is the number "n" to be used as the last arc in the
|       device-type OID:
|
|        1.3.6.1.2.1.25.3.1.n
|                       | | |
|                       | | * Identifying arc for type
|                       | *-hrDeviceTypes (OIDs specifying device types)
|                       *-hrDevice
|
|        for devices created by this cache-population routine.
|
|  IMPLICIT INPUTS:
|
|       The module-local head of the cache for the HrDiskStorage table,
|       "HrDiskStorage_cache".
|
|  OUTPUTS:
|
|     On Success:
|       Function returns TRUE indicating that the both cachees have been fully
|       populated with all non-Fixed disks.
|
|     On any Failure:
|       Function returns FALSE (indicating "not enough storage").
|
|  THE BIG PICTURE:
|
|     Part I of hrDiskStorage cache population.
|
|  OTHER THINGS TO KNOW:
|
|     We scan using the list of Logical Disk drive strings from 
|     GetLogicalDriveStrings() formed up into UNC form, (e.g. "\\.\A:"
|     for "A:\" returned from GetLogicalDriveStrings()).
*/
{
CHAR    temp[8];                /* Temporary buffer for first call         */
LPSTR   pDrvStrings;            /* --> allocated storage for drive strings */
LPSTR   pOriginal_DrvStrings;   /* (Needed for final deallocation          */
DWORD   DS_request_len;         /* Storage actually needed                 */
DWORD   DS_current_len;         /* Storage used on 2nd call                */

#define PHYS_SIZE 32
CHAR    phys_name[PHYS_SIZE];   /* Buffer where a string like "\\.C:" (for */
                                /*  example) is built for drive access.    */


/*
| We're going to call GetLogicalDriveStrings() twice, once to get the proper
| buffer size, and the second time to actually get the drive strings.
|
| The Bogus call.
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

/* Go for all of the strings
|
| The Real Call.
*/
if ((DS_current_len = GetLogicalDriveStrings(DS_request_len, pDrvStrings))
    == 0) {

    /* Request failed altogether, can't initialize */
    free( pOriginal_DrvStrings );
    return ( FALSE );
    }

/*
|==============================================================================
| For each logical drive . . . 
*/
while ( strlen(pDrvStrings) > 0 ) {

    UINT        drivetype;      /* Type of the drive from "GetDriveType()"   */

    /*
    | Get the drive-type so we can decide whether it should participate in
    | this population effort.  We do only CD-ROMS and REMOVABLES.
    */
    drivetype = GetDriveType(pDrvStrings);

    /* Skip the stuff we don't want to look at */
    if ( drivetype != DRIVE_REMOVABLE && drivetype != DRIVE_CDROM ) {

        /* Step to next string, if any */
        pDrvStrings += strlen(pDrvStrings) + 1;

        continue;
        }


    /* If we have room in the buffer to build the handle-name string */
    if ((strlen(pDrvStrings) + strlen("\\\\.\\")) < PHYS_SIZE) {

        #define DESCR_BSZ 512
        CHAR                    d_buf[DESCR_BSZ]; /* Dsecription bld buff */
        HANDLE                  hdrv;       /* Handle to device           */
        DWORD                   bytes_out;  /* Bytes retnd into geo_info  */
        DISK_GEOMETRY           geo_info;   /* Geometry Info from drive   */
        char                   *mt;         /* Media Type */

        ULONG       access;     /* hrDiskStorageAccess = readWrite(1) */
        ULONG       media;      /* hrDiskStorageMedia = floppyDisk(4) */
        ULONG       removable;  /* hrDiskStorageRemovable = TRUE      */
        ULONG       capacityKB; /* hrDiskStorageCapacity, (kilobytes) */
        ULONG       status;     /* hrDeviceStatus = unknown(1)        */
        CHAR       *descr;      /* hrDeviceDescr string               */


        /*                         012345
        |  Build it for device A: "\\.\A:" */
        sprintf(phys_name, "\\\\.\\%2.2s", pDrvStrings);

        /*
        |  Set SNMP variables accordingly
        */
        if (drivetype != DRIVE_CDROM) {     /* Floppy */

            access = 1;          /* hrDiskStorageAccess = readWrite(1) */
            media = 4;           /* hrDiskStorageMedia = floppyDisk(4) */
            removable = TRUE;    /* hrDiskStorageRemovable = TRUE      */
            capacityKB = 0;      /* hrDiskStorageCapacity (unknown)    */
            status = 1;          /* hrDeviceStatus = unknown(1)        */
            descr = pDrvStrings; /* hrDeviceDescr, initial (e.g."A:\") */
            }

        else {                              /* CD-ROM */
            /*
            | We can't get much of anything about CD-ROMs except the fact
            | that there is one.  Capacity cannot be presumed as DVD is
            | now available and some drives read both CD-ROMs and DVD.
            */
            access = 2;          /* hrDiskStorageAccess = readOnly(2)  */
            media = 5;           /* hrDiskStorageMedia = opticalDiskROM(5)*/
            removable = TRUE;    /* hrDiskStorageRemovable = TRUE      */
            capacityKB = 0;      /* hrDiskStorageCapacity (unknown)    */
            status = 1;          /* hrDeviceStatus = unknown(1)        */
            descr = pDrvStrings; /* hrDeviceDescr, initial (e.g."D:\") */
            }


        /*
        | Suppress any attempt by the system to make the user put a volume in a
        | removable drive ("CreateFile" will just fail).
        */
        SetErrorMode(SEM_FAILCRITICALERRORS);

        /* Attempt to get a handle using this physical name string */
        hdrv = CreateFile(phys_name,                   // Device
                               GENERIC_READ,           // device query
                                                       // Share Mode
                               FILE_SHARE_READ   |
                               FILE_SHARE_WRITE,
                               NULL,                   // Security
                               OPEN_EXISTING,          // CreationDistribution
                               FILE_ATTRIBUTE_NORMAL,  // FlagsandAttributes
                               NULL                    // Template file
                               );

        /* If we successfully opened the device . . . */
        if (hdrv != INVALID_HANDLE_VALUE) {

            /*
            | Device is Open.
            |
            | If it is NOT a CD-ROM, (ie, a floppy) its worth trying to get
            | DRIVE GEOMETRY (which will come back if there is a floppy in
            | the drive).
            */

            if (drivetype != DRIVE_CDROM) {     /* Floppy */

                /* ==========================================================
                |  IOCTL_DISK_GET_DRIVE_GEOMETRY
                |
                |  If we can get this, we get a better description and an
                |  accurate capacity value.
                */
                if (DeviceIoControl(hdrv,           // device handle
                                                    // IoControlCode (op-code)
                                    IOCTL_DISK_GET_DRIVE_GEOMETRY,

                                    NULL,           // "input buffer"
                                    0,              // "input buffer size"
                                    &geo_info,      // "output buffer"
                                                    // "output buffer size"
                                    sizeof(DISK_GEOMETRY),

                                    &bytes_out,     // bytes written to geo_info
                                    NULL            // no Overlapped I/o
                                    )) {

                    /*
                    | Compute capacity
                    */
                    capacityKB = (ULONG)
                        ((geo_info.Cylinders.QuadPart * // 64 bits

                        (geo_info.TracksPerCylinder *   // 32 bits
                         geo_info.SectorsPerTrack *
                         geo_info.BytesPerSector)

                        ) / 1024);


                    /* hrDeviceStatus = running(2) */
                    status = 2;

                    switch ( geo_info.MediaType ) {

                        case  F5_1Pt2_512:    mt = "5.25, 1.2MB,  512 bytes/sector"; break;
                        case  F3_1Pt44_512:   mt = "3.5,  1.44MB, 512 bytes/sector"; break;
                        case  F3_2Pt88_512:   mt = "3.5,  2.88MB, 512 bytes/sector"; break;
                        case  F3_20Pt8_512:   mt = "3.5,  20.8MB, 512 bytes/sector"; break;
                        case  F3_720_512:     mt = "3.5,  720KB,  512 bytes/sector"; break;
                        case  F5_360_512:     mt = "5.25, 360KB,  512 bytes/sector"; break;
                        case  F5_320_512:     mt = "5.25, 320KB,  512 bytes/sector"; break;
                        case  F5_320_1024:    mt = "5.25, 320KB,  1024 bytes/sector"; break;
                        case  F5_180_512:     mt = "5.25, 180KB,  512 bytes/sector"; break;
                        case  F5_160_512:     mt = "5.25, 160KB,  512 bytes/sector"; break;
                        case  F3_120M_512:    mt = "3.5, 120M Floppy"; break;
                        case  F3_640_512:     mt = "3.5 ,  640KB,  512 bytes/sector"; break;
                        case  F5_640_512:     mt = "5.25,  640KB,  512 bytes/sector"; break;
                        case  F5_720_512:     mt = "5.25,  720KB,  512 bytes/sector"; break;
                        case  F3_1Pt2_512:    mt = "3.5 ,  1.2Mb,  512 bytes/sector"; break;
                        case  F3_1Pt23_1024:  mt = "3.5 ,  1.23Mb, 1024 bytes/sector"; break;
                        case  F5_1Pt23_1024:  mt = "5.25,  1.23MB, 1024 bytes/sector"; break;
                        case  F3_128Mb_512:   mt = "3.5 MO 128Mb   512 bytes/sector"; break;
                        case  F3_230Mb_512:   mt = "3.5 MO 230Mb   512 bytes/sector"; break;
                        case  F8_256_128:     mt = "8in,   256KB,  128 bytes/sector"; break;

                        default:
                        case  RemovableMedia:
                        case  FixedMedia:
                        case  Unknown:        mt = "Format is unknown"; break;
                        }

                    /* Format a better description if it'll all fit */
                    if ((strlen(pDrvStrings) + strlen(mt) + 1) < DESCR_BSZ ) {
                        sprintf(d_buf, "%s%s", pDrvStrings, mt);
                        descr = d_buf;
                        }
                    } /* If (we managed to get geometry information) */
                }

            CloseHandle(hdrv);

            }   /* if (we managed to "CreateFile" the device successfully) */

        /*
        | Create a row in HrDevice Table and a corresponding row in
        | hrDiskStorage sub-table.
        */
        if ( Process_DS_Row ( 
                             type_arc,  /* hrDeviceType last arc  */
                             access,    /* hrDiskStorageAccess    */
                             media,     /* hrDiskStorageMedia     */
                             removable, /* hrDiskStorageRemovable */
                             capacityKB,/* hrDiskStorageCapacity  */
                             status,    /* hrDeviceStatus         */
                             descr      /* hrDeviceDescr          */
                             ) == NULL) {

            /* Something blew */
            free( pOriginal_DrvStrings );
            return ( FALSE );
            }


        SetErrorMode(0);        /* Turn error suppression mode off */
        }   /* if (we managed to build a device name) */

    /* Step to next string, if any */
    pDrvStrings += strlen(pDrvStrings) + 1;
    }


free( pOriginal_DrvStrings );

/* Successful scan */
return ( TRUE );
}

/* Gen_Fixed_disks - Scan for Fixed Disks */
/* Gen_Fixed_disks - Scan for Fixed Disks */
/* Gen_Fixed_disks - Scan for Fixed Disks */

static BOOL
Gen_Fixed_disks ( 
                    ULONG type_arc
                    )
/*
|  EXPLICIT INPUTS:
|
|       "type_arc" is the number "n" to be used as the last arc in the
|       device-type OID:
|
|        1.3.6.1.2.1.25.3.1.n
|                       | | |
|                       | | * Identifying arc for type
|                       | *-hrDeviceTypes (OIDs specifying device types)
|                       *-hrDevice
|
|        for devices created by this cache-population routine.
|
|  IMPLICIT INPUTS:
|
|       The module-local head of the cache for the HrDiskStorage table,
|       "HrDiskStorage_cache".
|
|  OUTPUTS:
|
|     On Success:
|       Function returns TRUE indicating that the both cachees have been fully
|       populated with all non-Fixed disks.  If the device from which the
|       system was booted is encountered, it's hrDevice index is set into
|       "InitLoadDev_index" (defined in "HRDEVENT.C").
|
|     On any Failure:
|       Function returns FALSE (indicating "not enough storage").
|
|  THE BIG PICTURE:
|
|     Part II of hrDiskStorage cache population.
|
|  OTHER THINGS TO KNOW:
|
|     We scan using the "\\.\PHYSICALDRIVEx" syntax permitted to
|     "CreateFile()".  CreateFile seems to allow opens only on disks
|     that are hard-fixed disks (no floppies, no CD-ROMS).
|
|     This function is also (while it is "at it") looking for the drive
|     from which the system was booted (in order to set a global value
|     (hrdevice table index) for the value of "InitLoadDev_index" (defined
|     in "HRDEVENT.C") which becomes the value of "HrSystemInitialLoadDevice".
*/
{
HANDLE  hdrv;                   /* Handle to device                    */
UINT    dev_number;             /* Current "x" in "\\.\PHYSICALDRIVEx" */

#undef  PHYS_SIZE
#define PHYS_SIZE 64
CHAR    phys_name[PHYS_SIZE];   /* Buffer where a string like          */
                                /*  "\\.PHYSICALDRIVE0"  (for example) */
                                /* is built for drive access.          */

DRIVE_LAYOUT_INFORMATION *dl;       /* Drive-layout pointer       */
#define BBSz 768
CHAR                    big[BBSz];  /* Big buffer for layout info */
DWORD                   bytes_out;  /* Bytes retnd into "big"     */

CHAR                    windir[MAX_PATH]; /* Current Windows Directory      */
CHAR                    ntdev[MAX_PATH];  /* NT Device Name for MSDOS drive */
CHAR                    pntdev[MAX_PATH]; /* NT Device Name for PHYSICALDRIVE*/

/*
|==============================================================================
| Compute the NT "device name" we expect is the device from which the
| system was booted.
|
| Strategy:
|
| - Obtain "current windows directory" and truncate to obtain just the MS-DOS
|   device name.
|
| - Do QueryDosDevice to get the underlying "NT Device Name", which will
|   include a "\PartitionX" on the end of it, where "X" is presumed to be
|   the 1-origined partition number.
|
| - Mangle the NT Device Name so that it sez "....\Partition0" (ie "partition
|   zero") which is the NT Device Name we expect to be associated with the
|   "\\.\PHYSICALDRIVEy" string we generate for each valid fixed disk below.
*/
/* If we can get the current Windows Directory */
if (GetWindowsDirectory(windir, MAX_PATH) != 0 ) {

    /* Truncate to just "C:" (or whatever) */
    windir[2] = '\0';

    /* Obtain the NT Device Name associated with MS-DOS logical drive */
    if (QueryDosDevice(windir, ntdev, MAX_PATH) != 0) {

        PCHAR   partition;

        /* If the string "\Partition" appears in this string */
        if ((partition = strstr(ntdev,"\\Partition")) != NULL) {

            /* Convert it to say "\Partition0" regardless */
            strcpy(partition, "\\Partition0");
            }
        else {
            /* Failure: Null-terminate so we fail gracefully */
            ntdev[0] = '\0';
            }
        }
    else {
        /* Failure: Null-terminate so we fail gracefully */
        ntdev[0] = '\0';
        }
    }
else {
    /* Failure: Null-terminate so we fail gracefully */
    ntdev[0] = '\0';
    }

/*
|==============================================================================
| For every physical device we can open successfully. . .
*/
for (dev_number = 0; ; dev_number += 1) {

    /* Build it for device n: "\\.\PHYSICALDRIVEn" */
    sprintf(phys_name, "\\\\.\\PHYSICALDRIVE%d", dev_number);

    /*
    | Suppress any attempt by the system to make the user put a volume in a
    | removable drive ("CreateFile" will just fail).
    */
    SetErrorMode(SEM_FAILCRITICALERRORS);

    /* Attempt to get a handle using this physical name string */
    if ((hdrv = CreateFile(phys_name,                  // Device
                               GENERIC_READ,           // Access
                                                       // Share Mode
                               FILE_SHARE_READ   |
                               FILE_SHARE_WRITE,
                               NULL,                   // Security
                               OPEN_EXISTING,          // CreationDistribution
                               FILE_ATTRIBUTE_NORMAL,  // FlagsandAttributes
                               NULL             // Template file
                           )) != INVALID_HANDLE_VALUE) {

        ULONG       access;     /* hrDiskStorageAccess = readWrite(1) */
        ULONG       media;      /* hrDiskStorageMedia = floppyDisk(4) */
        ULONG       removable;  /* hrDiskStorageRemovable = TRUE      */
        ULONG       capacityKB; /* hrDiskStorageCapacity, (kilobytes) */
        ULONG       status;     /* hrDeviceStatus = unknown(1)        */
        CHAR       *descr;      /* hrDeviceDescr string               */
        DWORD       bytes_out;  /* Bytes retnd into geo_info          */
        DISK_GEOMETRY geo_info; /* Geometry Info from drive           */
        char       *mt;         /* Media Type                         */
        CACHEROW   *dv_row;     /* HrDevice table row created for disk*/


        /*
        | Device is Open, so we can presume it really exists, so it goes
        | into the hrDevice table.
        |
        | It is presumed to be a FIXED disk.
        */

        access = 1;          /* hrDiskStorageAccess = readWrite(1) */
        media = 3;           /* hrDiskStorageMedia = hardDisk(3)   */
        removable = FALSE;   /* hrDiskStorageRemovable = FALSE     */
        capacityKB = 0;      /* hrDiskStorageCapacity (unknown)    */
        status = 1;          /* hrDeviceStatus = unknown(1)        */
        descr = "Fixed Disk";/* hrDeviceDescr                      */


        /* ==========================================================
        |  IOCTL_DISK_GET_DRIVE_GEOMETRY
        |
        |  If we can get this, we get a better description and an
        |  accurate capacity value.
        */
        if (DeviceIoControl(hdrv,           // device handle
                                            // IoControlCode (op-code)
                            IOCTL_DISK_GET_DRIVE_GEOMETRY,
                            NULL,           // "input buffer"
                            0,              // "input buffer size"
                            &geo_info,      // "output buffer"
                                            // "output buffer size"
                            sizeof(DISK_GEOMETRY),
                            &bytes_out,     // bytes written to geo_info
                            NULL            // no Overlapped I/o
                            )) {

            /*
            | Compute capacity
            */
            capacityKB = (ULONG) 
                (geo_info.Cylinders.QuadPart *  // 64 bit

                 (geo_info.TracksPerCylinder *  // 32 bits
                  geo_info.SectorsPerTrack *
                  geo_info.BytesPerSector)

                 ) / 1024;

            /* hrDeviceStatus = running(2) */
            status = 2;

            switch ( geo_info.MediaType ) {

                case  FixedMedia:
                    break;

                default:
                    descr = "Unknown Media";
                }
            }


        /*
        | Create a row in HrDevice Table and a corresponding row in
        | hrDiskStorage sub-table.
        */
        if ((dv_row = Process_DS_Row (type_arc,  /* hrDeviceType last arc  */
                                      access,    /* hrDiskStorageAccess    */
                                      media,     /* hrDiskStorageMedia     */
                                      removable, /* hrDiskStorageRemovable */
                                      capacityKB,/* hrDiskStorageCapacity  */
                                      status,    /* hrDeviceStatus         */
                                      descr      /* hrDeviceDescr          */
                                      )
             ) == NULL) {

            /* Something blew */
            CloseHandle(hdrv);
            return ( FALSE );
            }

        /*
        | If it turns out this is the device from which the system was
        | booted, we need to return the index associated with the "dv_row"
        | row into "InitLoadDev_index" (defined in "HRDEVENT.C") to become
        | the value of "HrSystemInitialLoadDevice".
        |
        | See if the NT Device name associated with this "\\.\PHYSICALDRIVEx"
        | matches the value predicted for the system boot device.
        |
        |  If we can get the NT Device name for "PHYSICALDRIVEn" . . . */
        if (QueryDosDevice(&phys_name[4], pntdev, MAX_PATH) != 0 ) {

            /* If it matches the predicted value for boot device . . . */
            if ( strcmp(pntdev, ntdev) == 0) {

                /* Record the index for the current "physicaldrive" */
                InitLoadDev_index = dv_row->index;
                }
            }
        else {
            /*
            | Fail gracefully so things will still work in
            | "ProcessPartition()" below.
            */
            pntdev[0] = '\0';
            }

        /*
        | Create a hrPartition table (in the HrDevice Table row just created
        | for the disk) to represent the partitions on this fixed disk.
        */
        if ( ProcessPartitions( hdrv, dv_row, pntdev ) != TRUE ) {
            
            /* Something blew */
            CloseHandle(hdrv);
            return ( FALSE );
            }

        /* Fold up shop on this disk */
        CloseHandle(hdrv);
        } /* If we managed to "CreateFile()" the device */

    else {      /* Open failed... give up the scan */
        break;
        }

    SetErrorMode(0);        /* Turn error suppression mode off */
    }   /* For each device */

/* Successful scan */
return ( TRUE );
}

/* Process_DS_Row - Process Information into HrDevice and hrDiskStorage Row */
/* Process_DS_Row - Process Information into HrDevice and hrDiskStorage Row */
/* Process_DS_Row - Process Information into HrDevice and hrDiskStorage Row */

static CACHEROW *
Process_DS_Row ( 
                ULONG       type_arc,  /* hrDeviceType last arc value        */
                ULONG       access,    /* hrDiskStorageAccess = readWrite(1) */
                ULONG       media,     /* hrDiskStorageMedia = floppyDisk(4) */
                ULONG       removable, /* hrDiskStorageRemovable = TRUE      */
                ULONG       capacityKB,/* hrDiskStorageCapacity, (kilobytes) */
                ULONG       status,    /* hrDeviceStatus = unknown(1)        */
                CHAR       *descr      /* hrDeviceDescr string               */
                )
/*
|  EXPLICIT INPUTS:
|
|       "type_arc" is the number "n" to be used as the last arc in the
|       device-type OID:
|
|        1.3.6.1.2.1.25.3.1.n
|                       | | |
|                       | | * Identifying arc for type
|                       | *-hrDeviceTypes (OIDs specifying device types)
|                       *-hrDevice
|
|        for devices created by this cache-population routine.
|
|       The rest of the arguments outline above are used to fill in
|       attribute values in both the hrDevice table row and the corresponding
|       hrDiskStorage row.
|
|  IMPLICIT INPUTS:
|
|       The module-local head of the cache for the HrDiskStorage table,
|       "HrDiskStorage_cache".
|
|  OUTPUTS:
|
|     On Success:
|       Function returns pointer to row entry made to the hrDevice
|       table indicating that the both caches have been fully
|       populated with a new row.
|
|     On any Failure:
|       Function returns NULL (indicating "not enough storage" or other
|       failure.).
|
|  THE BIG PICTURE:
|
|
|  OTHER THINGS TO KNOW:
|
|     This function contains common "row-insertion" code for the
|     Gen_Fixed_disks() and Gen_nonFixed_disks() functions.
*/
{
CACHEROW   *dv_row;     /* Row created in hrDevice table      */
CACHEROW   *ds_row;     /* Row created in hrDiskStorage table */


/*
|==========================
| Create hrDevice Row entry.
|
| Note we're initializing this as though the Hidden Context is always
| going to be a Cache pointer.  It will be for fixed-disks (that have
| Partition Tables), but for other disks the "NULL" signals "No Partition
| Table".
|
| For fixed-disks, the NULL is overwritten later (in "ProcessPartitions()")
| by a pointer to malloced storage containing an instance of  CACHEHEAD
| structure that carries the hrPartition Table for that fixed-disk).
*/
if ((dv_row = AddHrDeviceRow(type_arc,   // DeviceType OID Last-Arc
                             descr,      // Used as description
                             NULL,       // Hidden Ctx (none)
                             CA_CACHE    // Hidden Ctx type
                             )) == NULL ) {
    /* Something blew */
    return ( NULL );
    }

/* Re-Set hrDeviceStatus */
dv_row->attrib_list[HRDV_STATUS].attrib_type = CA_NUMBER;
dv_row->attrib_list[HRDV_STATUS].u.unumber_value = status;

/*
|===============================
| Create hrDiskStorage Row entry
|
| Note: The index is not recorded IN the row, but the entry
|       is "indexed": by the hrDevice row index.  This happens
|       in the AddTableRow() call below.
*/
if ((ds_row = CreateTableRow( HRDS_ATTRIB_COUNT ) ) == NULL) {
    return ( NULL );       // Out of memory
    }

/*
| Set the attribute values for this row
*/

/* =========== hrDiskStorageAccess ==========*/
ds_row->attrib_list[HRDS_ACCESS].attrib_type = CA_NUMBER;
ds_row->attrib_list[HRDS_ACCESS].u.unumber_value = access;

/* =========== hrDiskStorageAccess ==========*/
ds_row->attrib_list[HRDS_MEDIA].attrib_type = CA_NUMBER;
ds_row->attrib_list[HRDS_MEDIA].u.unumber_value = media;

/* =========== hrDiskStorageRemovable ==========*/
ds_row->attrib_list[HRDS_REMOVABLE].attrib_type = CA_NUMBER;
ds_row->attrib_list[HRDS_REMOVABLE].u.unumber_value = removable;

/* =========== hrDiskStorageCapacity ==========*/
ds_row->attrib_list[HRDS_CAPACITY].attrib_type = CA_NUMBER;
ds_row->attrib_list[HRDS_CAPACITY].u.unumber_value = capacityKB;


/*
| Now insert the filled-in CACHEROW structure into the
| cache-list for the hrDiskStorage Table..
|
| Use the same index that was used to specify the row inserted
| into the hrDevice table.
*/


if (AddTableRow(dv_row->attrib_list[HRDV_INDEX].u.unumber_value,  /* Index */
                ds_row,                                           /* Row   */
                &hrDiskStorage_cache                              /* Cache */
                ) == FALSE) {

    return ( NULL );       /* Internal Logic Error! */
    }

/* Processing complete */
return ( dv_row );
}

/* ProcessPartitions - Process Partition Information into HrDevice Row */
/* ProcessPartitions - Process Partition Information into HrDevice Row */
/* ProcessPartitions - Process Partition Information into HrDevice Row */

static BOOL
ProcessPartitions(
                  HANDLE        hdrv,   /* Fixed-Disk containing partitions */
                  CACHEROW     *dv_row, /* Row in hrDevice table for disk   */
                  CHAR         *pntdev  /* NT Device name for physical disk */
                  )
/*
|  EXPLICIT INPUTS:
|
|       "hdrv" - handle open to the fixed disk whose partitions are to be
|       enumerated.
|
|       "dv_row" - the HrDevice row into which the new hrPartition Table
|       is to go.
|
|       "pntdev" - NT Device Name for the physical disk we're dealing with.
|                  We need this to infer the logical device name for any
|                  active partition.
|
|  IMPLICIT INPUTS:
|
|       "HrFSTable_cache" - this gets scanned to allow "hrPartitionFSIndex"
|       to be filled in.
|
|  OUTPUTS:
|
|     On Success:
|       Function returns TRUE indicating that the Partition Information
|       for the given disk has been used to populate an hrPartition Table
|       instance.
|
|     On any Failure:
|       Function returns NULL (indicating "not enough storage" or other
|       failure.).
|
|  THE BIG PICTURE:
|
|     This is the function that instantiates hrPartition tables.
|
|  OTHER THINGS TO KNOW:
|
|     Documentation at the top of the hrPartition Table file "HRPARTIT.C"
|     might be of interest.
|
|     BUG: As of build 1515, (and indeed in earlier versions of NT) the
|     "PartitionNumber" returned in response to DeviceIoControl opcode
|     "IOCTL_DISK_GET_DRIVE_LAYOUT" comes back as garbage.  Whatever
|     comes back is reported as the value of "hrPartitionID" (garbage or
|     not).  However when trying to fetch the Volume Label, as part of a
|     workaround attempt, we use the index generated in the code below 
|     in an attempt to approximate the proper Partition Number.
*/
{
#define DL_SIZE 1024
CHAR            dl_buf[DL_SIZE];  /* Drive-Layout info comes back here   */
UINT            i;                /* Handy-Dandy loop index              */
ULONG           table_index=0;    /* hrPartition Table row index counter */
DWORD           bytes_out;        /* Exactly how big "dl_buf" got filled */
DRIVE_LAYOUT_INFORMATION
                *dl;              /* Drive-layout pointer                */


/*
| See if we can grab the Drive's partition layout info.
*/
if (DeviceIoControl(hdrv,                         // device handle
                    IOCTL_DISK_GET_DRIVE_LAYOUT,  // IoControlCode (op-code)
                    NULL,                         // "input buffer"
                    0,                            // "input buffer size"
                    dl_buf,                       // "output buffer"
                    DL_SIZE,                      // "output buffer size"
                    &bytes_out,                   // bytes written to part_info
                    NULL                          // no Overlapped I/o
                    )) {

    CACHEHEAD *ch;

    /*
    | OK, there's presumed to be at least one partition: instantiate the
    | new partition table.
    |
    | Do this by creating its cache list-head structure.
    */
    dv_row->attrib_list[HIDDEN_CTX].attrib_type = CA_CACHE;
    if ((dv_row->attrib_list[HIDDEN_CTX].u.cache
         = ch = (CACHEHEAD *) malloc( sizeof(CACHEHEAD) )) == NULL) {
        return ( FALSE );
        }

    /*
    | Now initialize the contents properly.
    | (This should match what macro CACHEHEAD_INSTANCE does to a static
    |  instance).
    */
    ch->list_count = 0;
    ch->list = NULL;

    #if defined(CACHE_DUMP)
        ch->print_row = debug_print_hrpartition;
    #else
        ch->print_row = NULL;
    #endif


    /* Grab a dereferencable pointer to the Drive Layout info */
    dl = (DRIVE_LAYOUT_INFORMATION *) dl_buf;

    /* For every Partition "slot" returned . . . */
    for (i = 0; i < dl->PartitionCount; i += 1) {

        PARTITION_INFORMATION
                        *p;       /* Partition info thats going to go . . */
        CACHEROW        *row;     /* . . .into this row in HrPartition    */
        CACHEROW        *fs_row;  /* Row ptr in HrFSEntry table           */
        ULONG           last_arc; /* Last OID arc to use as FS-Type       */


        /* Grab a simple pointer to the next PARTITION_INFO to consider */
        p = &(dl->PartitionEntry[i]);

        /*
        | Note: It may be that not all of the PartitionEntry elements are
        |       "live".
        */
        if (p->PartitionType == PARTITION_ENTRY_UNUSED) {
            continue;
            }

        /*
        |===============================
        | Create hrPartition Row entry
        |
        | Note: This table is also "indexed" by the hrDevice row index
        */
        if ((row = CreateTableRow( HRPT_ATTRIB_COUNT ) ) == NULL) {
            return ( FALSE );       // Out of memory
            }

        /* =========== hrPartitionIndex ==========*/
        row->attrib_list[HRPT_INDEX].attrib_type = CA_NUMBER;
        row->attrib_list[HRPT_INDEX].u.unumber_value = (table_index += 1);


        /* =========== hrPartitionLabel ==========*/
        row->attrib_list[HRPT_LABEL].attrib_type = CA_STRING;

        /*
        | If there is an MS-DOS logical device letter assigned to this
        | partition. . .
        */
        if ( p->RecognizedPartition ) {

            /*
            | Go get the label, copy it to malloc'ed storage and return it.
            |
            | NOTE: Due to what seems like a bug, we're passing in "i+1" here
            |       rather than "p->PartitionNumber" (which seems to come
            |       back as garbage).  Clearly "i" is not a proper substitute
            |       in the long run.  See "OTHER THINGS TO KNOW" in the docs
            |       above for this function.
            */
            row->attrib_list[HRPT_LABEL].u.string_value =
                FindPartitionLabel(pntdev, (i+1));
            }
        else {
            /* No label if no MS-DOS device */
            row->attrib_list[HRPT_LABEL].u.string_value = NULL;
            }


        /* =========== hrPartitionID ==========
        |
        | In build 1515, this number is returned as garbage.  See
        | "OTHER THINGS TO KNOW" above.
        */
        row->attrib_list[HRPT_ID].attrib_type = CA_NUMBER;
        row->attrib_list[HRPT_ID].u.unumber_value = p->PartitionNumber;


        /* =========== hrPartitionSize ==========*/
        row->attrib_list[HRPT_SIZE].attrib_type = CA_NUMBER;
        row->attrib_list[HRPT_SIZE].u.unumber_value =
            p->PartitionLength.LowPart / 1024;

        /* =========== hrPartitionFSIndex ==========*/
        row->attrib_list[HRPT_FSINDEX].attrib_type = CA_NUMBER;

        /* Assume no file system mounted (that we can find) */
        row->attrib_list[HRPT_FSINDEX].u.unumber_value = 0;

        /* Find the first Row (if any) in the hrFSTable */
        if ((fs_row = FindNextTableRow(0, &hrFSTable_cache)) == NULL) {

            /* No file systems listed at all.. we're done */
            continue;
            }

        /*
        | Convert the Partition-Type into the "last-arc" value we use
        | to indicate what kind of file-system it is.
        */
        last_arc = PartitionTypeToLastArc( p->PartitionType );

        do {    /* Walk the hrFSEntry table */

            /*
            | If we found that the hrFSTable entry "fs_row" specifies a
            | file-system TYPE (by arc number) that matches what the current
            | partition's type number translates to ... we're done.
            */
            if (fs_row->attrib_list[HRFS_TYPE].u.unumber_value == last_arc) {
                row->attrib_list[HRPT_FSINDEX].u.unumber_value = fs_row->index;
                break;
                }

            /* Step to the next row */
            fs_row = GetNextTableRow(fs_row);
            }
               while (fs_row != NULL);

        /*
        |===============================
        |Now add the row to the table
        */
        if (AddTableRow(row->attrib_list[HRPT_INDEX].u.unumber_value,/* Index */
                        row,                                         /* Row   */
                        ch                                           /* Cache */
                        ) == FALSE) {

            return ( FALSE );       /* Internal Logic Error! */
            }

        } /* For each partition */

    }  /* If DeviceIoControl succeeded */


/* Partition Table complete */
return ( TRUE ) ;
}


/* FindPartitionLabel - Find MS-DOS Device Label for a Fixed-Disk Partition */
/* FindPartitionLabel - Find MS-DOS Device Label for a Fixed-Disk Partition */
/* FindPartitionLabel - Find MS-DOS Device Label for a Fixed-Disk Partition */

static PCHAR
FindPartitionLabel(
                   CHAR   *pntdev, /* NT Device name for physical disk */
                   UINT   part_id  /* Partition Number (1-origined)    */
                   )
/*
|  EXPLICIT INPUTS:
|
|       "pntdev" - the NT Device Name (e.g. "\Device\Harddisk0\Partition0"
|       for the PHYSICAL device we're working on).
|
|       "part_id" - One-origined Partition number for which we hope to find
|       an MS-DOS Volume Label.
|
|  IMPLICIT INPUTS:
|
|       None.
|
|  OUTPUTS:
|
|     On Success:
|       Function returns a pointer to malloc'ed storage containing the
|       MS-DOS Device Volume label (as returned by "GetVolumeInformation()").
|
|     On any Failure:
|       Function returns NULL (indicating "some kind of failure").
|
|  THE BIG PICTURE:
|
|     This "helper" function attempts to map an NT Device Name and a
|     one-origined Partition Number into a Volume Label for return as
|     the value of "hrPartitionLabel".
|
|  OTHER THINGS TO KNOW:
|
|  The algorithm is based on studying the output from "QueryDosDevices()"
|  and blithely assuming that we can "back-map" the string
|  "\Device\HarddiskX\PartitionY" for any Partition "Y" to the associated
|  MS-DOS Device.  There is precious little documentation that sez we can,
|  but we try.
|
|  Here's the approach:
|
|  * Using the NT Device Name for the "PHYSICALDRIVEn", we scrape the
|    "\Partition0" off the end of the name and replace it with "\PartitionY"
|    where "Y" is the Partition number passed as input to this function.
|    This is the "Generated Partition Name".
|
|    ("PHYSICALDRIVE" NT Device Names all seem to have "\Partition0" as
|    the terminating part of their name, and since the Win32 docs say that
|    Partition Numbers are "1-origined", this seems like a safe approach.)
|
|  * We generate a list of all the MS-DOS device names (using QueryDosDevices).
|
|  * We take each MS-DOS Device name and ask for it's current underlying
|    NT Device name mapping.
|
|    + If the first name mapping for any MS-DOS Device matches our
|      "Generated Partition Name", then the MS-DOS Device name is submitted
|      to "GetVolumeInformation()" and the Volume Label returned is used as
|      the "Partition Label".
*/
{
#define BIG_BUFF 1024
CHAR    gen_part_name[MAX_PATH+16];     /* "pntdev" is at most MAX_PATH    */
CHAR   *partition;                      /* Where "\Partition0" starts      */
CHAR    MSDOS_devices[BIG_BUFF];        /* List of MS-DOS device names     */
CHAR    NT_device[BIG_BUFF];            /* Mapping of NT device names      */
CHAR   *devname;                        /* Index for MSDOS_devices         */


/* Copy the NT Device Name for the Physical Drive */
strcpy(gen_part_name, pntdev);

/* Obtain a pointer to the beginning of "\Partition0" in this string */
if ((partition = strstr(gen_part_name, "\\Partition")) != NULL) {

    /*
    | Replace "\Partition0" with "\PartitionY" where "Y" is the supplied
    | partition number:  We've got the "Generated Partition Name".
    */
    sprintf(partition, "\\Partition%d", part_id);

    /*
    | Now ask for a list of MS-DOS Device Names
    */
    if ( QueryDosDevice(NULL, MSDOS_devices, BIG_BUFF) != 0) {

        /*
        | Swing down the list of MS-DOS device names and get the NT Device
        | name mappings.
        */
        for (devname = MSDOS_devices;
             *devname != '\0';
             devname += (strlen(devname)+1)) {

            /* Obtain the mappings for device "devname" */
            QueryDosDevice(devname, NT_device, BIG_BUFF);

            /* If the first mapping matches the Generated Partition Name */
            if (strcmp(gen_part_name, NT_device) == 0) {

                #define VOL_LABEL_SIZE 128
                CHAR    MSDOS_root[64];            /* Root Path Name       */
                CHAR    v_label[VOL_LABEL_SIZE];   /* Volume Label         */
                CHAR    *ret_label;                /* --> Malloced storage */
                DWORD   comp_len;                  /* Filename length      */
                DWORD   flags;


                /*
                | We're obliged to add a root-directory "\" to the MS-DOS
                | device name.
                */
                sprintf(MSDOS_root, "%s\\", devname);

                /* Fetch the Volume Information for the MS-DOS Device */
                if (GetVolumeInformation(
                                         MSDOS_root,       // MS-DOS root name
                                         v_label,          // Vol. Label buf
                                         VOL_LABEL_SIZE,   // vol. label size
                                         NULL,             // Serial Number
                                         &comp_len,        // FileName length
                                         &flags,           // Flags
                                         NULL,             // File System name
                                         0                 // Name buff. len
                                         ) != 0) {
                    /*
                    | Allocate storage to hold a returnable copy
                    */
                    if ( (ret_label = (CHAR *) malloc(strlen(v_label) + 1))
                        != NULL) {

                        /* Copy the label to malloced storage */
                        strcpy(ret_label, v_label);

                        return (ret_label);
                        }
                    else {
                        /* Out of storage */
                        return (NULL);
                        }
                    }
                else {
                    /* "GetVolumeInformation" failed on MSDOS name */
                    return (NULL);
                    }
                }
            }                    
        }
    }

return (NULL);  /* Other Failure */
}

#if defined(CACHE_DUMP)

/* debug_print_hrdiskstorage - Prints a Row from HrDiskStorage sub-table */
/* debug_print_hrdiskstorage - Prints a Row from HrDiskStorage sub-table */
/* debug_print_hrdiskstorage - Prints a Row from HrDiskStorage sub-table */

static void
debug_print_hrdiskstorage(
                          CACHEROW     *row  /* Row in hrDiskStorage table */
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
    fprintf(OFILE, "=========================\n");
    fprintf(OFILE, "hrDiskStorage Table Cache\n");
    fprintf(OFILE, "=========================\n");
    return;
    }

fprintf(OFILE, "hrDiskStorageAccess. . . . %d ",
        row->attrib_list[HRDS_ACCESS].u.unumber_value);
switch (row->attrib_list[HRDS_ACCESS].u.unumber_value) {
    case 1: fprintf(OFILE, "(readWrite)\n");            break;
    case 2: fprintf(OFILE, "(readOnly)\n");             break;
    default:fprintf(OFILE, "(???)\n");                  break;
    }

fprintf(OFILE, "hrDiskStorageMedia . . . . %d ",
        row->attrib_list[HRDS_MEDIA].u.unumber_value);
switch (row->attrib_list[HRDS_MEDIA].u.unumber_value) {
    case 1: fprintf(OFILE, "(Other)\n");                break;
    case 2: fprintf(OFILE, "(Unknown)\n");              break;
    case 3: fprintf(OFILE, "(hardDisk)\n");             break;
    case 4: fprintf(OFILE, "(floppyDisk)\n");           break;
    case 5: fprintf(OFILE, "(opticalDiskROM)\n");       break;
    case 6: fprintf(OFILE, "(opticalDiskWORM)\n");      break;
    case 7: fprintf(OFILE, "(opticalDiskRW)\n");        break;
    case 8: fprintf(OFILE, "(ramDisk)\n");              break;
    default:fprintf(OFILE, "(???)\n");                  break;
    }

fprintf(OFILE, "hrDiskStorageRemovable . . %d ",
        row->attrib_list[HRDS_REMOVABLE].u.unumber_value);
switch (row->attrib_list[HRDS_REMOVABLE].u.unumber_value) {
    case 0: fprintf(OFILE, "(FALSE)\n"); break;
    case 1: fprintf(OFILE, "(TRUE)\n"); break;
    default: 
            fprintf(OFILE, "(???)\n"); break;
    }

fprintf(OFILE, "hrDiskStorageCapacity. . . %d (KBytes)\n",
        row->attrib_list[HRDS_CAPACITY].u.unumber_value);

}


/* debug_print_hrpartition - Prints a Row from HrPartition sub-table */
/* debug_print_hrpartition - Prints a Row from HrPartition sub-table */
/* debug_print_hrpartition - Prints a Row from HrPartition sub-table */

static void
debug_print_hrpartition(
                        CACHEROW     *row  /* Row in hrPartition table */
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
    fprintf(OFILE, "     =======================\n");
    fprintf(OFILE, "     hrPartition Table Cache\n");
    fprintf(OFILE, "     =======================\n");
    return;
    }


fprintf(OFILE, "     hrPartitionIndex . . . . . %d\n",
        row->attrib_list[HRPT_INDEX].u.unumber_value);

fprintf(OFILE, "     hrPartitionLabel . . . . . \"%s\"\n",
        row->attrib_list[HRPT_LABEL].u.string_value);

fprintf(OFILE, "     hrPartitionID. . . . . . . 0x%x\n",
        row->attrib_list[HRPT_ID].u.unumber_value);

fprintf(OFILE, "     hrPartitionSize. . . . . . %d\n",
        row->attrib_list[HRPT_SIZE].u.unumber_value);

fprintf(OFILE, "     hrPartitionFSIndex . . . . %d\n",
        row->attrib_list[HRPT_FSINDEX].u.unumber_value);
}
#endif
