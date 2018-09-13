/*
 *  HrSWRunPerfEntry.c v0.10
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
 *    instance name routines for the HrSWRunPerfEntry.  Actual instrumentation code is
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
 *    V1.00 - 05/14/97  D. D. Burns     Genned: Thu Nov 07 16:48:05 1996
 *
 */


#include <windows.h>
#include <malloc.h>

#include <snmp.h>

#include "mib.h"
#include "smint.h"
#include "hostmsmi.h"
#include "user.h"         /* Developer supplied include file */
#include "HMCACHE.H"      /* Cache-related definitions       */

/*
| NOTE:
|    The cache used by this table is the same one created for hrSWRun.  The
|    CACHEHEAD structure for it is located in "HRSWRUNE.C".
*/



/*
 *  GetHrSWRunPerfCPU
 *    The number of centi-seconds of the total system's CPU resources consumed 
 *    by this process.  Note that  on a multi-processor syste
 *    
 *    Gets the value for HrSWRunPerfCPU.
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
 | hrSWRunPerfCPU
 | 
 |  ACCESS         SYNTAX
 |  read-only      INTEGER
 | 
 | "The number of centi-seconds of the total system's CPU resources consumed by
 | this process.  Note that on a multi-processor system, this value may increment
 | by more than one centi-second in one centi-second of real (wall clock) time."
 | 
 | DISCUSSION:
 | 
 | <POA-22> Given the performance monitoring counters available in the Registry,
 | how do we compute this SNMP attribute value for a given process?
 | 
 | RESOLVED >>>>>>>>
 | <POA-22> I think we should just use the PerfMon code for this.
 | RESOLVED >>>>>>>>
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.5.1.1.1.<instance>
 |                | | | |
 |                | | | *-hrSWRunPerfCPU
 |                | | *-hrSWRunPerfEntry
 |                | *-hrSWRunPerfTable
 |                *-hrSWRunPerf
 */

UINT
GetHrSWRunPerfCPU( 
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
if ((row = FindTableRow(index, &hrSWRunTable_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

*outvalue = row->attrib_list[HRSP_CPU].u.number_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSWRunPerfCPU() */


/*
 *  GetHrSWRunPerfMem
 *    The total amount of real system memory allocated to this process.
 *    
 *    Gets the value for HrSWRunPerfMem.
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
 | hrSWRunPerfMem
 | 
 |  ACCESS         SYNTAX
 |  read-only      KBytes
 | 
 | "The total amount of real system memory allocated to this process."
 | 
 | DISCUSSION:
 | 
 | <POA-23> Given the performance monitoring counters available in the Registry,
 | how do we compute this SNMP attribute value for a given process?
 | 
 | RESOLVED >>>>>>>>
 | <POA-22> I think we should just use the PerfMon code for this.
 | RESOLVED >>>>>>>>
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.5.1.1.2.<instance>
 |                | | | |
 |                | | | *-hrSWRunPerfMem
 |                | | *-hrSWRunPerfEntry
 |                | *-hrSWRunPerfTable
 |                *-hrSWRunPerf
 */

UINT
GetHrSWRunPerfMem( 
        OUT KBytes *outvalue ,
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
if ((row = FindTableRow(index, &hrSWRunTable_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

*outvalue = row->attrib_list[HRSP_MEM].u.number_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSWRunPerfMem() */


/*
 *  HrSWRunPerfEntryFindInstance
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
HrSWRunPerfEntryFindInstance( IN ObjectIdentifier *FullOid ,
                       IN OUT ObjectIdentifier *instance )
{
    UINT tmp_instance ;

    //
    //  Developer instrumentation code to find appropriate instance goes here.
    //  For non-tables, it is not necessary to modify this routine.  However, if
    //  there is any context that needs to be set, it can be done here.
    //

    if ( FullOid->idLength <= HRSWRUNPERFENTRY_VAR_INDEX )
	// No instance was specified
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else  if ( FullOid->idLength != HRSWRUNPERFENTRY_VAR_INDEX + 1 )
	// Instance length is more than 1
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else
	// The only valid instance for a non-table are instance 0.  If this
	// is a non-table, the following code validates the instances.  If this
	// is a table, developer modification is necessary below.

	tmp_instance = FullOid->ids[ HRSWRUNPERFENTRY_VAR_INDEX ] ;

        /*
        | Check for age-out and possibly refresh the entire cache for the 
        | hrSWRun(Perf) table before we check to see if the instance is there.
        */
        if (hrSWRunCache_Refresh() == FALSE) {
            return SNMP_ERRORSTATUS_GENERR;
            }

        /*
        | For hrSWRunPerf, the instance arc(s) is a single arc, and it must
        | correctly select an entry in the hrSWRun(Perf) Table cache.
        | Check that here.
        */
	if ( FindTableRow(tmp_instance, &hrSWRunTable_cache) == NULL ) {
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

} /* end of HrSWRunPerfEntryFindInstance() */



/*
 *  HrSWRunPerfEntryFindNextInstance
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
HrSWRunPerfEntryFindNextInstance( IN ObjectIdentifier *FullOid ,
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


    if ( FullOid->idLength <= HRSWRUNPERFENTRY_VAR_INDEX )
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
        tmp_instance = FullOid->ids[ HRSWRUNPERFENTRY_VAR_INDEX ] ;
        }

    /*
    | Check for age-out and possibly refresh the entire cache for the 
    | hrSWRun(Perf) table before we check to see if the instance is there.
    */
    if (hrSWRunCache_Refresh() == FALSE) {
        return SNMP_ERRORSTATUS_GENERR;
        }

    /* Now go off and try to find the next instance in the table */
    if ((row = FindNextTableRow(tmp_instance, &hrSWRunTable_cache)) == NULL) {
        return SNMP_ERRORSTATUS_NOSUCHNAME ;
        }

    instance->ids[ 0 ] = row->index ;
    instance->idLength = 1 ;

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of HrSWRunPerfEntryFindNextInstance() */



/*
 *  HrSWRunPerfEntryConvertInstance
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
HrSWRunPerfEntryConvertInstance( IN ObjectIdentifier *oid_spec ,
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

} /* end of HrSWRunPerfEntryConvertInstance() */




/*
 *  HrSWRunPerfEntryFreeInstance
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
HrSWRunPerfEntryFreeInstance( IN OUT InstanceName *instance )
{

    //
    //  Developer supplied code to free native representation of instance name goes here.
    //

} /* end of HrSWRunPerfEntryFreeInstance() */

