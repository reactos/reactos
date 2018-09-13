/*
 *  HrPartitionEntry.c v0.10
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
 *    instance name routines for the HrPartitionEntry.  Actual instrumentation code is
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
 *    V1.00 - 04/28/97  D. D. Burns     Genned: Thu Nov 07 16:43:52 1996
 *
 | 
 | The approach envisioned for generating the contents of this table consists 
 | of walking all physical drives (using CreateFile and the "physical drive" 
 | naming convention) and acquiring the partition information for each drive 
 | using Win32 API functions DeviceIoControl (IOCTL_DISK_GET_DRIVE_LAYOUT).
 | 
 */


#include <windows.h>
#include <malloc.h>

#include <snmp.h>

#include "mib.h"
#include "smint.h"
#include "hostmsmi.h"
#include "user.h"         /* Developer supplied include file */
#include "HMCACHE.H"      /* Cache-related definitions          */
#include "HRDEVENT.H"     /* HrDevice Table-related definitions */


/*
|==============================================================================
| A word about the cache for this sub-table.
|
| This is a unique sub-table within the hrDevice table in that it is doubly
| indexed.
|
| As a consequence, there are multiple instances of the cache-head,
| one for each simple instance of this table.  Consequently there is no
| single, static instance of a cache-head to be found (as is typically the
| case for sub-tables of hrDevice) here at the start of the module.
|
| The "value" of the "hidden-context" attribute for a hrDevice table
| row for this sub-table has as its value a pointer to a slug of malloced
| memory containing the cache-head for that instance of this sub-table.
| (See "HMCACHE.C" for storage picture).
|
| Initialization of this sub-table occurs as part of the initialization
| of the HrDiskStorage sub-table (HRDISKST.C) in function "ProcessPartitions".
*/

/*
|==============================================================================
| Function Prototypes for this module.
*/
/* RollToNextFixedDisk - Helper Routine for HrPartitionEntryFindNextInstance */
static UINT
RollToNextFixedDisk (
UINT       *dev_tmp_instance ,     /* ->Device Table Instance Arc ("1st")    */
UINT       *part_tmp_instance ,    /* ->Partition Table Instance Arc ("2nd") */
CACHEROW  **dev_row,               /* ->> Entry in hrDevice Table            */
CACHEHEAD **part_cache             /* ->> Cache-Header for Partition         */
                     );


/*
 *  GetHrPartitionIndex
 *    A unique value for each partition on this long term storage device.  The 
 *    value for each long-term storage device must remain con
 *    
 *    Gets the value for HrPartitionIndex.
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
 | hrPartitionIndex
 | 
 |  ACCESS         SYNTAX
 |  read-only      INTEGER (1..2147483647)
 | 
 | "A unique value for each partition on this long- term storage device.  The
 | value for each long-term storage device must remain constant at least from one
 | re-initialization of the agent to the next re- initialization."
 | 
 | DISCUSSION:
 | 
 | (See discussion above for the table as a whole).
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.7.1.1.<dev-instance>.<partition-instance>
 |                | | | |
 |                | | | *-hrPartitionIndex
 |                | | *-hrPartitionEntry
 |                | *-hrPartitionTable
 |                *-hrDevice
 */

UINT
GetHrPartitionIndex( 
        OUT Integer *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
ULONG           disk_index;     /* As fetched from instance structure */
ULONG           part_index;     /* As fetched from instance structure */
CACHEROW        *disk_row;      /* Row entry fetched from Disk cache  */
CACHEROW        *part_row;      /* Row entry fetched from Part. cache */
CACHEHEAD       *part_cache;    /* HrPartition Table cache to search  */


/*
| Grab the instance information
*/
disk_index = GET_INSTANCE(0);
part_index = GET_INSTANCE(1);


/*
|===========
| Index 1
| Use Disk-Index to find the right Disk-row entry in the hrDevice cache
*/
if ((disk_row = FindTableRow(disk_index, &hrDevice_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/* Grab a pointer to the hrPartition cache for this disk */
part_cache = disk_row->attrib_list[HIDDEN_CTX].u.cache;


/*
|===========
| Index 2
| Use Partition-Index to find the right row entry in the hrPartition cache
*/
if ((part_row = FindTableRow(part_index, part_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

*outvalue = part_row->attrib_list[HRPT_INDEX].u.unumber_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrPartitionIndex() */


/*
 *  GetHrPartitionLabel
 *    A textual description of this partition.
 *    
 *    Gets the value for HrPartitionLabel.
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
 | hrPartitionLabel
 |
 | ACCESS         SYNTAX
 | read-only      InternationalDisplayString (SIZE (0..128))
 |
 | "A textual description of this partition."
 |
 | DISCUSSION:
 |
 | This information for the entire drive is obtained using Win32 API CreateFile
 | to open the device and DeviceIoControl to retrieve the needed information.
 |
 |============================================================================
 | 1.3.6.1.2.1.25.3.7.1.2.<dev-instance>.<partition-instance>
 |                | | | |
 |                | | | *-hrPartitionLabel
 |                | | *-hrPartitionEntry
 |                | *-hrPartitionTable
 |                *-hrDevice
 */

UINT
GetHrPartitionLabel( 
        OUT InternationalDisplayString *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
ULONG           disk_index;     /* As fetched from instance structure */
ULONG           part_index;     /* As fetched from instance structure */
CACHEROW        *disk_row;      /* Row entry fetched from Disk cache  */
CACHEROW        *part_row;      /* Row entry fetched from Part. cache */
CACHEHEAD       *part_cache;    /* HrPartition Table cache to search  */


/*
| Grab the instance information
*/
disk_index = GET_INSTANCE(0);
part_index = GET_INSTANCE(1);


/*
|===========
| Index 1
| Use Disk-Index to find the right Disk-row entry in the hrDevice cache
*/
if ((disk_row = FindTableRow(disk_index, &hrDevice_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/* Grab a pointer to the hrPartition cache for this disk */
part_cache = disk_row->attrib_list[HIDDEN_CTX].u.cache;


/*
|===========
| Index 2
| Use Partition-Index to find the right row entry in the hrPartition cache
*/
if ((part_row = FindTableRow(part_index, part_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

outvalue->string = part_row->attrib_list[HRPT_LABEL].u.string_value;

if (outvalue->string == NULL) {
    outvalue->length = 0;
    }
else {
    outvalue->length = strlen(outvalue->string);

    /* "Truncate" here to meet RFC as needed*/
    if (outvalue->length > 128) {
        outvalue->length = 128;
        }
    }

return SNMP_ERRORSTATUS_NOERROR ;


} /* end of GetHrPartitionLabel() */


/*
 *  GetHrPartitionID
 *    A descriptor which uniquely represents this partition to the responsible 
 *    operating system.  On some systems, this might take on 
 *    
 *    Gets the value for HrPartitionID.
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
 | hrPartitionID
 | 
 |  ACCESS         SYNTAX
 |  read-only      OCTET STRING
 | 
 | "A descriptor which uniquely represents this partition to the responsible
 | operating system.  On some systems, this might take on a binary
 | representation."
 | 
 | DISCUSSION:
 | 
 | This information for the entire drive is obtained using Win32 API CreateFile
 | to open the device and DeviceIoControl to retrieve the needed information.
 | 
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.7.1.3.<dev-instance>.<partition-instance>
 |                | | | |
 |                | | | *-hrPartitionID
 |                | | *-hrPartitionEntry
 |                | *-hrPartitionTable
 |                *-hrDevice
 */

UINT
GetHrPartitionID( 
        OUT OctetString *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
ULONG           disk_index;     /* As fetched from instance structure */
ULONG           part_index;     /* As fetched from instance structure */
CACHEROW        *disk_row;      /* Row entry fetched from Disk cache  */
CACHEROW        *part_row;      /* Row entry fetched from Part. cache */
CACHEHEAD       *part_cache;    /* HrPartition Table cache to search  */


/*
| Grab the instance information
*/
disk_index = GET_INSTANCE(0);
part_index = GET_INSTANCE(1);


/*
|===========
| Index 1
| Use Disk-Index to find the right Disk-row entry in the hrDevice cache
*/
if ((disk_row = FindTableRow(disk_index, &hrDevice_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/* Grab a pointer to the hrPartition cache for this disk */
part_cache = disk_row->attrib_list[HIDDEN_CTX].u.cache;


/*
|===========
| Index 2
| Use Partition-Index to find the right row entry in the hrPartition cache
*/
if ((part_row = FindTableRow(part_index, part_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

outvalue->string = (char *) &(part_row->attrib_list[HRPT_ID].u.unumber_value);
outvalue->length = 4;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrPartitionID() */


/*
 *  GetHrPartitionSize
 *    The size of this partition.
 *    
 *    Gets the value for HrPartitionSize.
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
 | hrPartitionSize
 | 
 |  ACCESS         SYNTAX
 |  read-only      KBytes
 | 
 | "The size of this partition."
 | 
 | DISCUSSION:
 | 
 | This information for the entire drive is obtained using Win32 API CreateFile
 | to open the device and DeviceIoControl to retrieve the needed information.
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.7.1.4.<dev-instance>.<partition-instance>
 |                | | | |
 |                | | | *-hrPartitionSize
 |                | | *-hrPartitionEntry
 |                | *-hrPartitionTable
 |                *-hrDevice
 */

UINT
GetHrPartitionSize( 
        OUT KBytes *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
ULONG           disk_index;     /* As fetched from instance structure */
ULONG           part_index;     /* As fetched from instance structure */
CACHEROW        *disk_row;      /* Row entry fetched from Disk cache  */
CACHEROW        *part_row;      /* Row entry fetched from Part. cache */
CACHEHEAD       *part_cache;    /* HrPartition Table cache to search  */


/*
| Grab the instance information
*/
disk_index = GET_INSTANCE(0);
part_index = GET_INSTANCE(1);


/*
|===========
| Index 1
| Use Disk-Index to find the right Disk-row entry in the hrDevice cache
*/
if ((disk_row = FindTableRow(disk_index, &hrDevice_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/* Grab a pointer to the hrPartition cache for this disk */
part_cache = disk_row->attrib_list[HIDDEN_CTX].u.cache;


/*
|===========
| Index 2
| Use Partition-Index to find the right row entry in the hrPartition cache
*/
if ((part_row = FindTableRow(part_index, part_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

*outvalue = part_row->attrib_list[HRPT_SIZE].u.unumber_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrPartitionSize() */


/*
 *  GetHrPartitionFSIndex
 *    The index of the file system mounted on this partition.  If no file 
 *    system is mounted on this partition, then this value shall b
 *    
 *    Gets the value for HrPartitionFSIndex.
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
 | hrPartitionFSIndex
 | 
 |  ACCESS         SYNTAX
 |  read-only      INTEGER (0..2147483647)
 | 
 | "The index of the file system mounted on this partition.  If no file system is
 | mounted on this partition, then this value shall be zero.  Note that multiple
 | partitions may point to one file system, denoting that that file system
 | resides on those partitions.  Multiple file systems may not reside on one
 | partition."
 | 
 | DISCUSSION:
 | 
 | This information for the entire drive is obtained using Win32 API CreateFile
 | to open the device and DeviceIoControl to retrieve the needed information.
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.7.1.5.<dev-instance>.<partition-instance>
 |                | | | |
 |                | | | *-hrPartitionFSIndex
 |                | | *-hrPartitionEntry
 |                | *-hrPartitionTable
 |                *-hrDevice
 */

UINT
GetHrPartitionFSIndex( 
        OUT Integer *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
ULONG           disk_index;     /* As fetched from instance structure */
ULONG           part_index;     /* As fetched from instance structure */
CACHEROW        *disk_row;      /* Row entry fetched from Disk cache  */
CACHEROW        *part_row;      /* Row entry fetched from Part. cache */
CACHEHEAD       *part_cache;    /* HrPartition Table cache to search  */


/*
| Grab the instance information
*/
disk_index = GET_INSTANCE(0);
part_index = GET_INSTANCE(1);


/*
|===========
| Index 1
| Use Disk-Index to find the right Disk-row entry in the hrDevice cache
*/
if ((disk_row = FindTableRow(disk_index, &hrDevice_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/* Grab a pointer to the hrPartition cache for this disk */
part_cache = disk_row->attrib_list[HIDDEN_CTX].u.cache;


/*
|===========
| Index 2
| Use Partition-Index to find the right row entry in the hrPartition cache
*/
if ((part_row = FindTableRow(part_index, part_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

*outvalue = part_row->attrib_list[HRPT_FSINDEX].u.unumber_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrPartitionFSIndex() */


/*
 *  HrPartitionEntryFindInstance
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
HrPartitionEntryFindInstance( IN ObjectIdentifier *FullOid ,
                       IN OUT ObjectIdentifier *instance )
{
    UINT        dev_tmp_instance ;     /* Device Table Instance Arc     */
    UINT        part_tmp_instance ;    /* Partition Table Instance Arc  */
    CACHEROW    *dev_row;              /* --> Entry in hrDevice Table   */
    CACHEHEAD   *part_cache;           /* --> Cache-Header for Partition*/


    //
    //  Developer instrumentation code to find appropriate instance goes here.
    //  For non-tables, it is not necessary to modify this routine.  However, if
    //  there is any context that needs to be set, it can be done here.
    //

    if ( FullOid->idLength <= HRPARTITIONENTRY_VAR_INDEX )
	// No instance was specified
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
        // Instance is 2 arcs for this table:
    else  if ( FullOid->idLength != HRPARTITIONENTRY_VAR_INDEX + 2 )
	// Instance length is more than 2, or 1 exactly, either way: error
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else
	// The only valid instance for a non-table are instance 0.  If this
	// is a non-table, the following code validates the instances.  If this
	// is a table, developer modification is necessary below.

        /*
        | Check the first of two instance arcs here.  The first should
        | select a valid entry in hrDiskStorage cache.  This guarantees
        | that a disk is being selected.  If a valid entry is present in
        | hrDiskStorage cache, then a corresponding row should be in the
        | main hrDevice table (and having checked hrDiskStorage first, we
        | don't have to verify the type of the entry in hrDevice).
        |
        | The corresponding hrDevice row entry should have a hidden-context
        | that is a non-NULL pointer to a CACHEHEAD **if** the first instance
        | arc is truly selecting a Fixed-Disk (which is the only kind of disk
        | for which HrPartition sub-tables are constructed).
        */
	dev_tmp_instance = FullOid->ids[ HRPARTITIONENTRY_VAR_INDEX ] ;

	if ( FindTableRow(dev_tmp_instance, &hrDiskStorage_cache) == NULL ) {
	    return SNMP_ERRORSTATUS_NOSUCHNAME ;
            }

        /*
        | Ok, there is an entry in hrDiskStorage, now go get the corresponding
        | hrDevice entry... it **will be** for a Disk.
        */
        if ( (dev_row = FindTableRow(dev_tmp_instance, &hrDevice_cache))
            == NULL ) {
	    return SNMP_ERRORSTATUS_GENERR ;
            }

        /*
        | Check to be sure there is a cache tucked into the hidden-context,
        | this assures us it is a Fixed Disk and that there is something to
        | search given the second instance arc.  (The cache header may be
        | for an empty cache, but it will support a search).
        */
        if (dev_row->attrib_list[HIDDEN_CTX].attrib_type != CA_CACHE ||
            (part_cache = dev_row->attrib_list[HIDDEN_CTX].u.cache) == NULL) {
	    return SNMP_ERRORSTATUS_NOSUCHNAME ;
            }

        /*
        | First instance arc ("dev_tmp_instance") is kosher... check the second,
        | ("part_tmp_instance") it should select a valid entry in the cache 
        | whose header pointer is in HIDDEN_CTX.
        */
	part_tmp_instance = FullOid->ids[ HRPARTITIONENTRY_VAR_INDEX + 1] ;
        if ( FindTableRow(part_tmp_instance, part_cache) == NULL ) {
	    return SNMP_ERRORSTATUS_NOSUCHNAME ;
            }

	else
	{
	    // the both instances are valid.  Create the instance portion of
	    // the OID to be returned from this call.
	    instance->ids[ 0 ] = dev_tmp_instance ;
	    instance->ids[ 1 ] = part_tmp_instance ;
	    instance->idLength = 2 ;
	}

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of HrPartitionEntryFindInstance() */



/*
 *  HrPartitionEntryFindNextInstance
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
HrPartitionEntryFindNextInstance( IN ObjectIdentifier *FullOid ,
                           IN OUT ObjectIdentifier *instance )
{
    UINT        dev_tmp_instance=0;   /* Device Table Instance Arc ("1st")   */
    UINT        part_tmp_instance=0;  /* Partition Table Instance Arc ("2nd")*/
    CACHEROW    *hrDevice_row;        /* Looked-up row in hrDevice table     */
    CACHEROW    *dev_row = NULL;      /* --> Entry in hrDiskStorage Table    */
    CACHEROW    *part_row = NULL;     /* --> Entry in hrPartition Table      */
    CACHEHEAD   *part_cache=NULL;     /* --> Cache-Header for Partition      */
    UINT        ret_status;           /* Status to be returned by this func  */


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

    /*
    | With two numeric indices to deal with on this table, we're attempting
    | to cover the following situations:
    |
    | 1) No instance arcs provided at all.  In this case, we need to look up
    |    and return the instance arcs that select the first fixed-disk and
    |    the first partition within that disk.  We cover this case as a
    |    "standalone" case.
    |
    | 2) Only the hrdevice ("disk selecting") first instance arc is provided.
    |    In this case, we'll "assume" an initial partition-selecting instance
    |    arc of "0", and continue processing with the code that covers case 3.
    |
    | 3) Two or more instance arcs are provided, in which case we just use the
    |    first of two arcs as the hrdevice ("disk selecting") "first" instance 
    |    arc and the second arc as the hrPartition ("partition selecting") 
    |    "second" instance arc, ignoring any remaining arcs.
    |
    | The "Party Line" on this logic is that it works even if the "fixed-disk"
    | entries don't have monotonically increasing indices the way the
    | cache-population code currently creates them and even if the partition
    | cache (table) has no entries for a particular disk (it must, however
    | have a cache).
    */
    

    if ( FullOid->idLength <= HRPARTITIONENTRY_VAR_INDEX ) {

        /* CASE 1
        |
        | No instance arcs were provided, so we want to setup local instance
        | arcs that select the first hrDevice table "Fixed Disk" entry and the
        | first "Partition" entry within that disk (if any disk and if any
        | partitions within that disk).
        |
        | Entry into "RollToNextFixedDisk()" is special for this case as
        | "dev_row" is NULL, so it'll roll to the first legal Fixed-Disk.
        */
        ret_status =
            RollToNextFixedDisk(&dev_tmp_instance,     // hrDiskStorage arc
                                &part_tmp_instance,    // hrPartition arc
                                &dev_row,              // row in hrDiskStorage
                                &part_cache            // cache for hrPartition
                                );

        /* If we got a no-good return status */
        if (ret_status != SNMP_ERRORSTATUS_NOERROR) {
            return ( ret_status );
            }

        /*
        | Ok, all we need to do is roll into the hrPartition table using the
        | second instance arc returned above (which will be zero) to find
        | the true first entry in the hrPartition table cache so we can return
        | it's index as the second instance arc, (or keep rolling if the
        | partition table (cache) is empty (which it really shouldn't be, but
        | we have it covered if it is)).
        |
        | All this is performed by the "General Roll" code below.
        */           
        }

    else {  /* Some instance arcs provided */

        if ( FullOid->idLength == HRPARTITIONENTRY_VAR_INDEX + 1 ) {

            /* CASE 2
            |
            | Exactly one instance arc is provided, 
            | so use it and assume the second arc is 0.
            */
            dev_tmp_instance = FullOid->ids[ HRPARTITIONENTRY_VAR_INDEX ] ;
            part_tmp_instance = 0;
            }

        else {

            /* CASE 3
            |
            | Two or more instance arcs are provided, 
            | so use the first two arcs and ignore the rest.
            */
            dev_tmp_instance = FullOid->ids[ HRPARTITIONENTRY_VAR_INDEX ] ;
            part_tmp_instance = FullOid->ids[ HRPARTITIONENTRY_VAR_INDEX + 1] ;
            }

        /* Show "No HrPartition Cache Selected Yet" (at entry time: NULL) */

        /*
        | At this point, we need to know whether the first instance arc
        | actually selects a Fixed-Disk entry in hrDevice (and hrDiskStorage)
        | that has an hrPartition table that we can "roll" thru.
        |
        | Attempt a direct ("non-rolling") lookup on the hrDiskStorage table
        | cache with the first instance arc.
        */
        dev_row = FindTableRow(dev_tmp_instance, &hrDiskStorage_cache);

        /*
        | If (an entry is found)
        */
        if ( dev_row != NULL) {

            /* If (the entry is for a "Fixed-Disk") */
            if ( dev_row->attrib_list[HRDS_REMOVABLE].u.unumber_value
                == FALSE) {

                /*
                | Perform "FindTableRow" on hrdevice cache using first 
                | instance arc.
                */
                hrDevice_row = FindTableRow(dev_tmp_instance, &hrDevice_cache);
                
                /*
                | If (no hrdevice entry was found)
                */
                if ( hrDevice_row  == NULL) {
                    return SNMP_ERRORSTATUS_GENERR ;
                    }

                /*
                | If the hrdevice entry DOES NOT have a
                | cache associated with it . . .
                */
                if ( (hrDevice_row->attrib_list[HIDDEN_CTX].attrib_type
                      != CA_CACHE) ||

                     (hrDevice_row->attrib_list[HIDDEN_CTX].u.cache
                      == NULL)) {

                    return SNMP_ERRORSTATUS_GENERR ;
                    }

                /*
                | Select the cache from the hrdevice disk entry as the 
                | HrPartition table cache to be searched in the following
                | logic.
                */
                part_cache = hrDevice_row->attrib_list[HIDDEN_CTX].u.cache;

                } /* if entry was fixed-disk */

            } /* if entry was found */

        /*
        | At this point, if a hrpartition cache has been selected (!NULL),
        | the first instance arc has selected a fixed-disk entry
        | and there is no need to "roll" on the first index.
        |
        | Otherwise we've got to "reset" the second instance arc
        | to "0" and do a rolling lookup on the first arc for
        | another Fixed-disk entry (that should have a partition-cache).
        */

        /* if (an hrpartition cache has NOT been selected) */
        if (part_cache == NULL) {

            /* Perform "RollToNextFixedDisk" processing */
            ret_status =
                RollToNextFixedDisk(&dev_tmp_instance,  // hrDiskStorage arc
                                    &part_tmp_instance, // hrPartition arc
                                    &dev_row,           // row in hrDiskStorage
                                    &part_cache         // hrPartition cache
                                    );

            /* If we got a no-good return status */
            if (ret_status != SNMP_ERRORSTATUS_NOERROR) {
                return ( ret_status );  // (either NOSUCH or GENERR)
                }
            }

        }  /* else Some instance arcs provided */

    /*
    | At this point, we have:
    |
    |   + a valid hrPartition cache to search,
    |   + the second instance arc to search it with (by "rolling"),
    |   + the first instance arc that is valid, (but may need to be
    |       rolled again if there are no valid partitions found).
    |
    | We can now do a "General Roll" to land on the proper Partition entry.
    */

    while (1) {         /* "General Roll" */

        /*
        | Do a "FindNextTableRow" (rolling) lookup on the hrpartition
        | cache with the current value of the second instance arc.
        |
        | if (an entry was found)
        */
        if ((part_row = FindNextTableRow(part_tmp_instance, part_cache))
            != NULL ) {

            /*
            | Return the current first arc and the index from the returned
            | entry as the second instance arc and signal NOERROR.
            */
            instance->ids[ 0 ] = dev_tmp_instance ;
            instance->ids[ 1 ] = part_row->index;
            instance->idLength = 2 ;

            return SNMP_ERRORSTATUS_NOERROR ;
            }

        /*
        | Fell off the end of the current hrPartition cache, must
        | go get another hrPartition cache from the next fixed-disk entry.
        |
        | Perform "RollToNextFixedDisk" processing
        */
        ret_status =
            RollToNextFixedDisk(&dev_tmp_instance,  // hrDiskStorage arc
                                &part_tmp_instance, // hrPartition arc
                                &dev_row,           // row in hrDiskStorage
                                &part_cache         // hrPartition cache
                                );

        if (ret_status != SNMP_ERRORSTATUS_NOERROR) {
            return ( ret_status );   // (either NOSUCH or GENERR)
            }

        } /* while */


} /* end of HrPartitionEntryFindNextInstance() */

/* RollToNextFixedDisk - Helper Routine for HrPartitionEntryFindNextInstance */
/* RollToNextFixedDisk - Helper Routine for HrPartitionEntryFindNextInstance */
/* RollToNextFixedDisk - Helper Routine for HrPartitionEntryFindNextInstance */

static UINT
RollToNextFixedDisk (
                                                                  /* Index */
UINT       *dev_tmp_instance,     /* ->Device Table Instance Arc    ("1st") */
UINT       *part_tmp_instance,    /* ->Partition Table Instance Arc ("2nd") */
CACHEROW  **dev_row,              /* ->> Entry in hrDevice Table            */
CACHEHEAD **part_cache            /* ->> Cache-Header for Partition         */
                     )

/*
|  EXPLICIT INPUTS:
|
|       "dev_tmp_instance" & "part_tmp_instance" are pointers to the "current"
|       instance arcs for the hrDevice and hrPartition table respectively.
|
|       Note that "*dev_tmp_instance" is also implicitly the index into the 
|       hrDiskStorage table as well.
|
|       "dev_row" - is a pointer to the pointer to the hrDiskStorage
|       row currently selected by "*dev_tmp_instance".  It is the 
|       case that "*dev_row" might be null, indicating that no row
|       has been selected yet.
|
|       "part_cache" is a pointer to the pointer to any selected hrPartition
|       table (ie a cache taken from an hrDevice row for a fixed-disk).  If
|       no such partition cache has been selected yet, then "*part_cache" is
|       NULL.
|
|  IMPLICIT INPUTS:
|
|       "HrDiskStorage_cache" may be referenced if "*dev_row" is NULL.
|
|  OUTPUTS:
|
|     On Success:
|       Function returns SNMP_ERRORSTATUS_NOERROR when the roll to the next
|       Fixed-Disk hrDiskStorage entry has succeeded.  New "first" and "second"
|       instance arcs are returned as well as the cache for the hrPartition
|       table that needs to be searched using the "second" arc.
|       
|     On any Failure:
|       Function returns SNMP_ERRORSTATUS_GENERR if it bumps into what it
|       thinks should be a Fixed-Disk entry in hrDevice but the entry does
|       not have an hrPartition table cache as it should.
|
|       Function returns SNMP_ERRORSTATUS_NOSUCHNAME if it can't find another
|       hrDevice entry for a Fixed-Disk.
|
|  THE BIG PICTURE:
|
|     This function takes care of the details of "rolling" to the next
|     Fixed-Disk entry in hrDevice (& HrDiskStorage) on behalf of
|     "HrPartitionEntryFindNextInstance()".
|
|  OTHER THINGS TO KNOW:
|
|     Because we rolling at the device "level", we reset the "instance"
|     arc for the partition level back to zero so that a FindNextTableRow
|     using zero will select the first entry in the selected hrPartition
|     Table (cache) being returned by this function.
*/
{
CACHEROW        *hrDevice_row;          /* Looked-up row in hrDevice table */


/* Reset the second instance arc to "0" */
*part_tmp_instance = 0;

while (1) {   /* Forever . . . */

    /*
    | Try to get the "next" hrDiskStorage row 
    | given our pointer to the current row.
    |
    | If no "current" row, start with the first in hrDiskStorage
    | and riffle upward until we get the row "after" the row that
    | the current device instance arc would have selected.
    */

    /* if (there is no current hrDiskStorage row) */
    if (*dev_row == NULL) {

        /* If the cache is empty . . . */
        if ( (*dev_row =
              FindNextTableRow(*dev_tmp_instance, &hrDiskStorage_cache))
              == NULL) {
            return SNMP_ERRORSTATUS_NOSUCHNAME;
            }
        }
    else {

        /*
        | Perform "GetNextTableRow" on current hrdiskstorage entry
        |
        | if (processing returned no-next-entry)
        */
        if ( (*dev_row = GetNextTableRow((*dev_row))) == NULL) {
            return SNMP_ERRORSTATUS_NOSUCHNAME;
            }
        }

    /*
    | Ok, we've got a "next" row in hrDiskStorage.  If it isn't for a fixed
    | disk, we've got to go around again in hopes of finding one that is.
    */

    /* if (entry is not for Fixed-Disk (skip Removables)) */
    if ( (*dev_row)->attrib_list[HRDS_REMOVABLE].u.unumber_value == TRUE) {
      continue;         /* Skippity doo-dah */
      }

    /*
    | Set current first instance arc value to index of current entry:
    | it is the "next" row following the row for which the original
    | device instance arc selected.
    */
    *dev_tmp_instance = (*dev_row)->index;

    /*
    | Ok, now we've got to go over to the big hrDevice table and hope to
    | find the corresponding hrDevice row given the row index we're on in
    | hrDiskStorage.  As this is coded, I realize that we could have put
    | the HIDDEN_CTX attribute in the hrDiskStorage entry rather than
    | the hrDevice entry and saved this lookup, but it matters little.
    */

    /*
    | Do "FindTableRow" on hrdevice cache using first instance arc
    |
    | if (no hrdevice entry was found) */
    if ( (hrDevice_row = FindTableRow(*dev_tmp_instance, &hrDevice_cache))
        == NULL) {
        /*
        | There should be an hrDevice table entry for every entry in
        | hrDiskStorage.  This seems not to be the case for some reason.
        */
        return SNMP_ERRORSTATUS_GENERR ;
        }

    /* If (the hrdevice entry DOES NOT have a cache associated with it) */
    if ( (hrDevice_row->attrib_list[HIDDEN_CTX].attrib_type != CA_CACHE) ||
         (hrDevice_row->attrib_list[HIDDEN_CTX].u.cache == NULL) ) {
        /*
        | There should be a cache in the HIDDEN_CTX attribute for all fixed
        | disks.
        */
        return SNMP_ERRORSTATUS_GENERR ;
        }

    /*
    | Select and return the cache from the hrDevice fixed-disk row entry as 
    | the HrPartition table cache to be searched on return.
    */
    *part_cache = hrDevice_row->attrib_list[HIDDEN_CTX].u.cache;

    return SNMP_ERRORSTATUS_NOERROR;
    }
}


/*
 *  HrPartitionEntryConvertInstance
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
HrPartitionEntryConvertInstance( IN ObjectIdentifier *oid_spec ,
                          IN OUT InstanceName *native_spec )
{
static char    *array[2];/* The address of this (char *) is passed back     */
                         /* as an array of pointers to instance arc numbers */

static ULONG    inst1,   /* The addresses of these ULONGs are passed back  */
                inst2;   /* (Obviously, no "free()" action is needed) */

    /* We expect the two arcs in "oid_spec" */
    inst1 = oid_spec->ids[0];
    array[0] = (char *) &inst1;

    inst2 = oid_spec->ids[1];
    array[1] = (char *) &inst2;

    native_spec->count = 2;
    native_spec->array = array;

    return SUCCESS ;

} /* end of HrPartitionEntryConvertInstance() */




/*
 *  HrPartitionEntryFreeInstance
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
HrPartitionEntryFreeInstance( IN OUT InstanceName *instance )
{

    //
    //  Developer supplied code to free native representation of instance name goes here.
    //

} /* end of HrPartitionEntryFreeInstance() */
