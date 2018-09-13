/*
 *  HrSWRunEntry.c v0.10
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
 *    instance name routines for the HrSWRunEntry.  Actual instrumentation code is
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
 *    V1.00 - 05/14/97  D. D. Burns     Genned: Thu Nov 07 16:47:29 1996
 *
 */


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <malloc.h>

#include <snmp.h>

#include "mib.h"
#include "smint.h"
#include "hostmsmi.h"
#include "user.h"         /* Developer supplied include file    */
#include "HMCACHE.H"      /* Cache-related definitions          */
#include <string.h>



/*
|==============================================================================
| Function prototypes for this module.
|
*/
/* AddHrSWRunRow - Generate another Row Entry in HrSWRun/Perf Table cache */
static BOOL
AddHrSWRunRow(PSYSTEM_PROCESS_INFORMATION ProcessInfo);

/* FetchProcessParams - Fetch Path & Parameter String from Process Cmd line */
void
FetchProcessParams(
PSYSTEM_PROCESS_INFORMATION ProcessInfo,   /* Process for parameters     */
CHAR                      **path_str,      /* Returned PATH string       */
CHAR                      **params_str     /* Returned Parameters string */
              );

#if defined(CACHE_DUMP)

/* debug_print_hrswrun - Prints a Row from HrSWRun(Perf) Table */
static void
debug_print_hrswrun(
                     CACHEROW     *row  /* Row in hrSWRun(Perf) table */
                     );
#endif


/*
|==============================================================================
| Cache Refresh Time
|
| The cache for the hrSWRun and hrSWRunPerf tables is refreshed automatically
| when a request arrives --AND-- the cache is older than CACHE_MAX_AGE
| in seconds.
|
*/
static
LARGE_INTEGER   cache_time;   // 100ns Timestamp of cache (when last refreshed)

#define CACHE_MAX_AGE 120     // Maximum age in seconds


/*
|==============================================================================
| Create the list-head for the HrSWRun(Perf) Table cache.
|
| This cache contains info for both the hrSWRun and hrSWRunPerf tables.
| (This macro is defined in "HMCACHE.H").
|
| This is global so code for the hrSWRunPerf table ("HRSWPREN.C") can
| reference it.
*/
CACHEHEAD_INSTANCE(hrSWRunTable_cache, debug_print_hrswrun);



/*
|==============================================================================
| Operating System Index
|
|  SNMP attribute "HrSWOSIndex" is the index into hrSWRun to the entry that
|  primary operating system running on the host.  This value is computed in
|  this module in function "AddHrSWRunRow()" and stored here for reference
|  by code in "HRSWRUN.C".
*/
ULONG   SWOSIndex;



/*
 *  GetHrSWRunIndex
 *    A unique value for each piece of software running on the host.  Wherever
 *    possible, this should be the system's native, unique id
 *
 *    Gets the value for HrSWRunIndex.
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
 | hrSWRunIndex
 |
 |  ACCESS         SYNTAX
 |  read-only      INTEGER (1..2147483647)
 |
 | "A unique value for each piece of software running on the host.  Wherever
 | possible, this should be the system's native, unique identification number."
 |
 | DISCUSSION:
 |
 | By using performance monitoring information from the Registry (using code
 | from "PVIEW") this attribute is given the value of the Process ID.
 |
 |============================================================================
 | 1.3.6.1.2.1.25.4.2.1.1.<instance>
 |                | | | |
 |                | | | *-hrSWRunIndex
 |                | | *-hrSWRunEntry
 |                | *-hrSWRunTable
 |                *-hrSWRun
 */

UINT
GetHrSWRunIndex(
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

*outvalue = row->attrib_list[HRSR_INDEX].u.number_value;
return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSWRunIndex() */


/*
 *  GetHrSWRunName
 *    A textual description of this running piece of software, including the
 *    manufacturer, revision, and the name by which it is commo
 *
 *    Gets the value for HrSWRunName.
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
 | hrSWRunName
 |
 |  ACCESS         SYNTAX
 |  read-only      InternationalDisplayString (SIZE (0..64))
 |
 | "A textual description of this running piece of software, including the
 | manufacturer, revision, and the name by which it is commonly known.  If this
 | software was installed locally, this should be the same string as used in the
 | corresponding hrSWInstalledName."
 |
 | DISCUSSION:
 |
 | By using performance monitoring information from the Registry (using code
 | from "PVIEW") this attribute is given the value of the Process name.
 |
 |============================================================================
 | 1.3.6.1.2.1.25.4.2.1.2.<instance>
 |                | | | |
 |                | | | *-hrSWRunName
 |                | | *-hrSWRunEntry
 |                | *-hrSWRunTable
 |                *-hrSWRun
 */

UINT
GetHrSWRunName(
        OUT InternationalDisplayString *outvalue ,
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

/* Return the name that was computed at cache-build time */
outvalue->length = strlen(row->attrib_list[HRSR_NAME].u.string_value);
outvalue->string = row->attrib_list[HRSR_NAME].u.string_value;
if (outvalue->length > 64) {
    outvalue->length = 64;      /* Truncate */
    }

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSWRunName() */


/*
 *  GetHrSWRunID
 *    The product ID of this running piece of software.
 *
 *    Gets the value for HrSWRunID.
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
 | hrSWRunID
 |
 |  ACCESS         SYNTAX
 |  read-only      ProductID
 |
 | "The product ID of this running piece of software."
 |
 | DISCUSSION:
 |
 | <POA-16> I anticipate always using "unknownProduct" as the value for this
 | attribute, as I can envision no systematic means of acquiring a registered
 | OID for all process software to be used as the value for this attribute.
 |
 | RESOLVED >>>>>>>
 | <POA-16> Returning an unknown Product ID is acceptable.
 | RESOLVED >>>>>>>
 |
 |============================================================================
 | 1.3.6.1.2.1.25.4.2.1.3.<instance>
 |                | | | |
 |                | | | *-hrSWRunID
 |                | | *-hrSWRunEntry
 |                | *-hrSWRunTable
 |                *-hrSWRun
 */

UINT
GetHrSWRunID(
        OUT ProductID *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{

/*
| The deal on this attribute is that we'll never have a valid OID value
| for this attribute.  Consequently, we always return the standard
| "unknown" OID value ("0.0") regardless of the instance value (which
| by now in the calling sequence of things has been validated anyway).
*/

if ( (outvalue->ids = SNMP_malloc(2 * sizeof( UINT ))) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }
outvalue->idLength = 2;

/*
| Load in the OID value for "unknown" for ProductID: "0.0"
*/
outvalue->ids[0] = 0;
outvalue->ids[1] = 0;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSWRunID() */


/*
 *  GetHrSWRunPath
 *    A description of the location on long-term storage (e.g. a disk drive)
 *    from which this software was loaded.
 *
 *    Gets the value for HrSWRunPath.
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
 | hrSWRunPath
 |
 |  ACCESS         SYNTAX
 |  read-only      InternationalDisplayString (SIZE(0..128))
 |
 | "A description of the location on long-term storage (e.g. a disk drive) from
 | which this software was loaded."
 |
 | DISCUSSION:
 |
 | <POA-17> This information is not extracted by the sample PVIEW code from the
 | performance monitoring statistics kept in the Registry.  If this information
 | is available from the Registry or some other source, I need to acquire the
 | description of how to get it.
 |
 | RESOLVED >>>>>>>>
 | <POA-17> This is obtained using PerfMon code pointers provided by Bob Watson.
 | RESOLVED >>>>>>>>
 |
 |============================================================================
 | 1.3.6.1.2.1.25.4.2.1.4.<instance>
 |                | | | |
 |                | | | *-hrSWRunPath
 |                | | *-hrSWRunEntry
 |                | *-hrSWRunTable
 |                *-hrSWRun
 */

UINT
GetHrSWRunPath(
        OUT InternationalDisplayString *outvalue ,
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

/*
| Return the Path string that was computed at cache-build time.
| NOTE: This string might be NULL.
*/
if (row->attrib_list[HRSR_PATH].u.string_value == NULL) {
    outvalue->length = 0;
    }
else {
    outvalue->length = strlen(row->attrib_list[HRSR_PATH].u.string_value);
    outvalue->string = row->attrib_list[HRSR_PATH].u.string_value;
    if (outvalue->length > 128) {
        outvalue->length = 128;      /* Truncate */
        }
    }

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSWRunPath() */

/*
 *  GetHrSWRunParameters
 *
 *    A description of the parameters supplied to this software when it was
 *    initially loaded."
 *
 *    Gets the value for HrSWRunParameters.
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
 | hrSWRunParameters
 |
 |  ACCESS         SYNTAX
 |  read-only      InternationalDisplayString (SIZE(0..128))
 |
 | "A description of the parameters supplied to this software when it was
 | initially loaded."
 |
 | DISCUSSION:
 |
 | <POA-18> This information is not extracted by the sample PVIEW code from the
 | performance monitoring statistics kept in the Registry.  If this information
 | is available from the Registry or some other source, I need to acquire the
 | description of how to get it.
 |
 | RESOLVED >>>>>>>>
 | <POA-18> See discussion for "hrSWRunPath" above.
 | RESOLVED >>>>>>>>
 |
 |============================================================================
 | NOTE: This function edited in by hand, as it was not originally generated.
 |============================================================================
 | 1.3.6.1.2.1.25.4.2.1.5.<instance>
 |                | | | |
 |                | | | *-hrSWRunParameters
 |                | | *-hrSWRunEntry
 |                | *-hrSWRunTable
 |                *-hrSWRun
 */

UINT
GetHrSWRunParameters(
        OUT InternationalDisplayString *outvalue ,
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

/*
| Return the Parameter string that was computed at cache-build time.
| NOTE: This string might be NULL.
*/
if (row->attrib_list[HRSR_PARAM].u.string_value == NULL) {
    outvalue->length = 0;
    }
else {
    outvalue->length = strlen(row->attrib_list[HRSR_PARAM].u.string_value);
    outvalue->string = row->attrib_list[HRSR_PARAM].u.string_value;
    if (outvalue->length > 128) {
        outvalue->length = 128;      /* Truncate */
        }
    }

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSWRunParameters() */


/*
 *  GetHrSWRunType
 *    The type of this software.
 *
 *    Gets the value for HrSWRunType.
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
 | hrSWRunType
 |
 |  ACCESS         SYNTAX
 |  read-only      INTEGER {unknown(1),operatingSystem(2),deviceDriver(3),
 |                           application(4)}
 |
 | "The type of this software."
 |
 | DISCUSSION:
 |
 | <POA-19> This information is not extracted by the sample PVIEW code from the
 | performance monitoring statistics kept in the Registry.  If this information
 | is available from the Registry or some other source, I need to acquire the
 | description of how to get it.
 |
 | >>>>>>>>
 | <POA-19>  I am not sure whether this information is included in the perfmon
 | data block. I will investigate further.
 | >>>>>>>>
 |
 |============================================================================
 | 1.3.6.1.2.1.25.4.2.1.6.<instance>
 |                | | | |
 |                | | | *-hrSWRunType
 |                | | *-hrSWRunEntry
 |                | *-hrSWRunTable
 |                *-hrSWRun
 */

UINT
GetHrSWRunType(
        OUT INTSWType *outvalue ,
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

*outvalue = row->attrib_list[HRSR_TYPE].u.number_value;
return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSWRunType() */


/*
 *  GetHrSWRunStatus
 *    The status of this running piece of software.  Setting this value to
 *    invalid(4) shall cause this software to stop running and be
 *
 *    Gets the value for HrSWRunStatus.
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
 | hrSWRunStatus
 |
 |  ACCESS         SYNTAX
 |  read-write     INTEGER {
 |                         running(1),
 |                         runnable(2), -- waiting for resource (CPU, memory, IO)
 |                         notRunnable(3), -- loaded but waiting for event
 |                         invalid(4)      -- not loaded
 |                         }
 |
 | "The status of this running piece of software.  Setting this value to
 | invalid(4) shall cause this software to stop running and to be unloaded."
 |
 | DISCUSSION:
 |
 | <POA-20> For an SNMP "GET" on this attribute, this information is not extracted
 | by the sample PVIEW code from the performance monitoring statistics kept in
 | the Registry.  If this information is available from the Registry or some
 | other source, I need to acquire the description of how to get it.
 |
 | RESOLVED >>>>>>>
 | <POA-20>  I think running and notRunnable will be all that are applicable
 | here (that latter being returned in situations which are currently labeled
 | "not responding").
 | RESOLVED >>>>>>>
 |
 |============================================================================
 | 1.3.6.1.2.1.25.4.2.1.7.<instance>
 |                | | | |
 |                | | | *-hrSWRunStatus
 |                | | *-hrSWRunEntry
 |                | *-hrSWRunTable
 |                *-hrSWRun
 */

UINT
GetHrSWRunStatus(
        OUT INThrSWRunStatus *outvalue ,
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

*outvalue = row->attrib_list[HRSR_STATUS].u.number_value;
return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSWRunStatus() */


/*
 *  SetHrSWRunStatus
 *    The status of this running piece of software.  Setting this value to
 *    invalid(4) shall cause this software to stop running and be
 *
 *    Sets the HrSWRunStatus value.
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
SetHrSWRunStatus(
        IN INThrSWRunStatus *invalue ,
        OUT INThrSWRunStatus *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{

    return SNMP_ERRORSTATUS_NOSUCHNAME ;

} /* end of SetHrSWRunStatus() */


/*
 *  HrSWRunEntryFindInstance
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
HrSWRunEntryFindInstance( IN ObjectIdentifier *FullOid ,
                       IN OUT ObjectIdentifier *instance )
{
    UINT tmp_instance ;

    //
    //  Developer instrumentation code to find appropriate instance goes here.
    //  For non-tables, it is not necessary to modify this routine.  However, if
    //  there is any context that needs to be set, it can be done here.
    //

    if ( FullOid->idLength <= HRSWRUNENTRY_VAR_INDEX )
	// No instance was specified
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else  if ( FullOid->idLength != HRSWRUNENTRY_VAR_INDEX + 1 )
	// Instance length is more than 1
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else
	// The only valid instance for a non-table are instance 0.  If this
	// is a non-table, the following code validates the instances.  If this
	// is a table, developer modification is necessary below.

	tmp_instance = FullOid->ids[ HRSWRUNENTRY_VAR_INDEX ] ;

        /*
        | Check for age-out and possibly refresh the entire cache for the
        | hrSWRun table before we check to see if the instance is there.
        */
        if (hrSWRunCache_Refresh() == FALSE) {
            return SNMP_ERRORSTATUS_GENERR;
            }

        /*
        | For hrSWRun, the instance arc(s) is a single arc, and it must
        | correctly select an entry in the hrSWRun Table cache.
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

} /* end of HrSWRunEntryFindInstance() */



/*
 *  HrSWRunEntryFindNextInstance
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
HrSWRunEntryFindNextInstance( IN ObjectIdentifier *FullOid ,
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


    if ( FullOid->idLength <= HRSWRUNENTRY_VAR_INDEX )
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
        tmp_instance = FullOid->ids[ HRSWRUNENTRY_VAR_INDEX ] ;
        }

    /*
    | Check for age-out and possibly refresh the entire cache for the
    | hrSWRun table before we check to see if the instance is there.
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

} /* end of HrSWRunEntryFindNextInstance() */



/*
 *  HrSWRunEntryConvertInstance
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
HrSWRunEntryConvertInstance( IN ObjectIdentifier *oid_spec ,
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

} /* end of HrSWRunEntryConvertInstance() */




/*
 *  HrSWRunEntryFreeInstance
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
HrSWRunEntryFreeInstance( IN OUT InstanceName *instance )
{

    //
    //  Developer supplied code to free native representation of instance name goes here.
    //

} /* end of HrSWRunEntryFreeInstance() */

/*
| End of Generated Code
*/

/* Gen_HrSWRun_Cache - Generate a initial cache for HrSWRun(Perf) Table */
/* Gen_HrSWRun_Cache - Generate a initial cache for HrSWRun(Perf) Table */
/* Gen_HrSWRun_Cache - Generate a initial cache for HrSWRun(Perf) Table */

BOOL
Gen_HrSWRun_Cache(
                  void
                  )

/*
|  EXPLICIT INPUTS:
|
|       None.
|
|  IMPLICIT INPUTS:
|
|       The module-local head of the cache for the HrSWRun table,
|       "hrSWRunTable_cache".
|
|  OUTPUTS:
|
|     On Success:
|       Function returns TRUE indicating that the cache has been fully
|       populated with all "static" cache-able values.  This function populates
|       the hrSWRun Table cache, but this cache also includes the two
|       attributes for the hrSWRunPerf Table.  So in effect, one cache serves
|       two tables, but the hrSWRunPerf table is a "one-to-one" extension
|       of hrSWRun table.. that is a row in hrSWRun always has a corresponding
|       "two-entry" row in hrSWRunPerf.
|
|     On any Failure:
|       Function returns FALSE (indicating "not enough storage").
|
|  THE BIG PICTURE:
|
|       At subagent startup time, the cache for each table in the MIB is
|       populated with rows for each row in the table.  This function is
|       invoked by the start-up code in  "UserMibInit()" ("MIB.C")  to
|       populate the cache for the HrSWRun table (which also serves the
|       hrSWRunPerf Table).
|
|       It is also re-entered whenever a request for information from this
|       cache comes in and the cache is older than a certain age (symbol
|       "CACHE_MAX_AGE" defined at the beginning of this module).  In this
|       case the cache is rebuilt, and in this way this function is different
|       from all the other "Gen_*_Cache()" functions which only build their
|       caches once (in the initial release).
|
|
|  OTHER THINGS TO KNOW:
|
|       There is one of these function for every table that has a cache.
|       Each cachehead is found in the respective table's source file.
|
|       The strategy on getting running software enumerated revolves around
|       NtQuerySystemInformation(SystemProcessInformation...) invocation.
|
|       Once we have a list of processes, additional information (such as
|       the parameters on the command-line) are fetched by opening the
|       process (if possible) and reading process memory.
|
|       Note that unlike the other cache's in the initial release, this cache
|       for hrSWRun and hrSWRunPerf is updated before it is read if it is
|       older than a specified period of time (set by #define at the beginning
|       of this file).
|
|============================================================================
| 1.3.6.1.2.1.25.4.1.0
|                | |
|                | *-hrSWOSIndex
|                *-hrSWRun
|
| 1.3.6.1.2.1.25.4.2.1..
|                | | |
|                | | *-hrSWRunEntry
|                | *-hrSWRunTable
|                *-hrSWRun
*/
#define LARGE_BUFFER_SIZE       (4096*8)
#define INCREMENT_BUFFER_SIZE   (4096*2)
{
static                             /* Initial ProcessBuffer size        */
DWORD      ProcessBufSize = LARGE_BUFFER_SIZE;
static
LPBYTE     pProcessBuffer = NULL;  /* Re-used and re-expanded as needed */

PSYSTEM_PROCESS_INFORMATION
           ProcessInfo;            /* --> Next process to process       */
ULONG      ProcessBufferOffset=0;  /* Accumulating offset cell          */
NTSTATUS   ntstatus;               /* Generic return status             */
DWORD      dwReturnedBufferSize;   /* From NtQuerySystemInformation()   */


/*
| Blow away any old copy of the cache
*/
DestroyTable( &hrSWRunTable_cache );


/*
| Grab an initial buffer for Process Information
*/
if (pProcessBuffer == NULL) {
    /* allocate a new block */
    if ((pProcessBuffer = malloc ( ProcessBufSize )) == NULL) {
        return ( FALSE );
        }
    }


/*
| Go for a (new/refreshed) buffer of current Process Info
*/
while( (ntstatus = NtQuerySystemInformation(
                                            SystemProcessInformation,
                                            pProcessBuffer,
                                            ProcessBufSize,
                                            &dwReturnedBufferSize
                                            )
        ) == STATUS_INFO_LENGTH_MISMATCH ) {

    /* expand buffer & retry */
    ProcessBufSize += INCREMENT_BUFFER_SIZE;

    if ( !(pProcessBuffer = realloc(pProcessBuffer,ProcessBufSize)) ) {
        return ( FALSE );
        }
    }

/*
| Freshen the time on the cache
|
| Get the current system-time in 100ns intervals . . . */
ntstatus = NtQuerySystemTime (&cache_time);


/*
| Loop over each instance of Process Information in the ProcessBuffer
| and build a row in the cache for hrSWRun and hrSWRunPerf tables.
*/
for (ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) pProcessBuffer;
     ;  /* Exit check below */
     ProcessBufferOffset += ProcessInfo->NextEntryOffset,
     ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
                          &pProcessBuffer[ProcessBufferOffset]
     ) {

    /* Add a Row to the cache */
    if (AddHrSWRunRow(ProcessInfo) != TRUE) {
        return ( FALSE );       // Out of memory
        }

    /* If this is the last process, bag it */
    if (ProcessInfo->NextEntryOffset == 0) {
        break;
        }
    }

#if defined(CACHE_DUMP)
PrintCache(&hrSWRunTable_cache);
#endif

/* Cache (re)-build successful */
return ( TRUE );
}

/* AddHrSWRunRow - Generate another Row Entry in HrSWRun/Perf Table */
/* AddHrSWRunRow - Generate another Row Entry in HrSWRun/Perf Table */
/* AddHrSWRunRow - Generate another Row Entry in HrSWRun/Perf Table */

static BOOL
AddHrSWRunRow(

PSYSTEM_PROCESS_INFORMATION ProcessInfo   /* --> Next process to process */

              )

/*
|  EXPLICIT INPUTS:
|
|       "ProcessInfo" points to the next process (as described by a
|       SYSTEM_PROCESS_INFORMATION structure) for which a row is to be
|       inserted into the HrSWRun(Perf) table cache.
|
|  IMPLICIT INPUTS:
|
|       The module-local head of the cache for the HrDevice table,
|       "hrSWRunTable_cache".
|
|  OUTPUTS:
|
|     On Success:
|       Function creates a new row entry populated with all "static" cache-able
|       values for HrSWRun(Perf) table and returns TRUE.  Note that if the
|       process is the "System Process", the row entry index is stored in
|       module cell "SWOSIndex" for reference by code in "HRSWRUN.C".
|
|     On any Failure:
|       Function returns FALSE (indicating "not enough storage" or other
|       internal logic error).
|
|
|  THE BIG PICTURE:
|
|       At subagent startup time, the cache for each table in the MIB is
|       populated with rows for each row in the table.  This function is
|       invoked by the cache-build code in "Gen_HrSWRun_Cache()" above.
|
|  OTHER THINGS TO KNOW:
|
|    The cache being (re)built by this function serves two tables, hrSWRun
|    and hrSWRunPerf.
|
|    In general, we use the Process's "ProcessID" as the index in the
|    hrSWRun(Perf) table.  However special handling is done for the Idle
|    Process because it's Process ID is zero.   We convert it to "1" to meet
|    the SNMP requirement that indexes be greater than zero.  We note that
|    as of this writing, no process id of 1 is seen in build 1515 (the "System"
|    process has a processID of 2).
|
|    The "type" of software can be unknown(1), operatingSystem(2),
|    deviceDriver(3) and application(4).  We only detect the Idle and System
|    processes (by their names) as "operatingSystem(2)", everything else
|    is presumed "application(4)".
|
|    As for "status", it can be running(1), runnable(2), notRunnable(3) or
|    invalid(4).  If the number of threads is greater than 0, it is presumed
|    "running(1)", otherwise "invalid(4)".
*/
#define ANSI_PNAME_LEN 256
{
ANSI_STRING     pname;          /* ANSI version of UNICODE process name    */
CHAR            pbuf[ANSI_PNAME_LEN+1];    /* Buffer for "pname"           */
CHAR           *pname_str;      /* Pointer to our final process name       */
CHAR           *path_str=NULL;  /* Pointer to our Path name                */
CHAR           *params=NULL;    /* Pointer to any parameters fnd on cmdline*/
UINT            type;           /* SNMP code for type of software          */
UINT            status;         /* SNMP code for the status of software    */
CACHEROW        *row;           /* --> Cache structure for row-being built */


/*
| OK, the caller wants another row in the table, get a row-entry created.
*/
if ((row = CreateTableRow( HRSR_ATTRIB_COUNT ) ) == NULL) {
    return ( FALSE );       // Out of memory
    }

/*
| Set up the standard-hrSWRun(Perf) attributes in the new row
*/

type = 4;       /* Presume "application(4)" type software */

if (ProcessInfo->NumberOfThreads > 0) {
    status = 1;     /* Presume "running(1)" for software status */
    }
else {
    status = 4;     /* "invalid(4)", process on the way out */
    }

/* =========== HrSWRunIndex ==========*/
row->attrib_list[HRSR_INDEX].attrib_type = CA_NUMBER;
row->attrib_list[HRSR_INDEX].u.unumber_value =
                                   HandleToUlong(ProcessInfo->UniqueProcessId) ;

/* Special check for system idle process, roll it from 0 to 1 */
if (ProcessInfo->UniqueProcessId == 0) {
    row->attrib_list[HRSR_INDEX].u.unumber_value = 1;
    }



/* =========== HrSWRunName ==========*/
row->attrib_list[HRSR_NAME].attrib_type = CA_STRING;

/* If we actually have a process name for this process . . . */
if (ProcessInfo->ImageName.Buffer != NULL) {

    /* Prep the STRING structure */
    pname.Buffer = pbuf;
    pname.MaximumLength = ANSI_PNAME_LEN;

    /* Convert from Unicode */
    RtlUnicodeStringToAnsiString(&pname,               // Target string
                                 (PUNICODE_STRING)&ProcessInfo->ImageName,//Src
                                 FALSE);               // = Don't Allocate buf
    /*
    | Here we parse not only the process name but any path that may be
    | prepended to it.  (We make no attempt to eliminate any ".EXE" that
    | may be on the end of the image name).
    |
    | NOTE: If you are going to rip off this code, be aware that as-of
    |       build 1515, we NEVER seem to get an image name that has the
    |       path prepended on the front... so most of this code to skip
    |       the possibly-present path is almost certainly superfluous.
    */

    /* Try to "backup" until we hit any "\" */
    if ( (pname_str = strrchr(pname.Buffer,'\\')) != NULL) {
        pname_str++;                     /* Pop to first char after "\" */
        }
    else {
        pname_str = pname.Buffer;        /* Use entire string, no "\" found */

        /*
        | A piece of software with no path means it could be the "System"
        | process.  Check for that here.
        */
        if (strcmp(pname_str, "System") == 0) {
            type = 2;   /* Mark the software as "operatingSystem(2)" type */

            /*
            | We're processing the main System Process, so record it's index
            | in module-level cell for reference from "HRSWRUN.C".
            */
            SWOSIndex = row->attrib_list[HRSR_INDEX].u.unumber_value;
            }
        }
    }
else {
    /* The system idle process has no name */
    pname_str = "System Idle Process";
    type = 2;             /* Mark the software as "operatingSystem(2)" type */
    }

/* Allocate cache storage and copy the process name to it */
if ( (row->attrib_list[HRSR_NAME].u.string_value
      = ( LPSTR ) malloc(strlen(pname_str) + 1)) == NULL) {
    return ( FALSE );       /* out of memory */
    }
strcpy(row->attrib_list[HRSR_NAME].u.string_value, pname_str);

/*
| We bother to do the overhead of trying to extract path & parameters from
| the command-line that started the process by reading process memory
| only if the type of the software is "application(4)" and status is
| "runnable(2)".
*/
if (status == 2 && type == 4) {  /* If it is a runnable application . . . */

    FetchProcessParams(ProcessInfo, &path_str, &params);
    }


/* =========== HrSWRunPath ==========*/
row->attrib_list[HRSR_PATH].attrib_type = CA_STRING;
row->attrib_list[HRSR_PATH].u.string_value = NULL;

/* If we did detect a path . . . */
if (path_str != NULL) {

    /* Allocate cache storage and copy the path string to it */
    if ( (row->attrib_list[HRSR_PATH].u.string_value
          = ( LPSTR ) malloc(strlen(path_str) + 1)) == NULL) {
        return ( FALSE );       /* out of memory */
        }
    strcpy(row->attrib_list[HRSR_PATH].u.string_value, path_str);
    }


/* =========== HrSWRunParameters ==========
row->attrib_list[HRSR_PARAM].attrib_type = CA_STRING;
row->attrib_list[HRSR_PARAM].u.string_value = NULL;    /* In case of none */

/* If we did find parameters . . . */
if (params != NULL) {

    /* Allocate cache storage and copy the parameter string to it */
    if ( (row->attrib_list[HRSR_PARAM].u.string_value
          = ( LPSTR ) malloc(strlen(params) + 1)) == NULL) {
        return ( FALSE );       /* out of memory */
        }
    strcpy(row->attrib_list[HRSR_PARAM].u.string_value, params);
    }


/* =========== HrSWRunType ========== */
row->attrib_list[HRSR_TYPE].attrib_type = CA_NUMBER;
row->attrib_list[HRSR_TYPE].u.unumber_value = type;


/* =========== HrSWRunStatus ========== */
row->attrib_list[HRSR_STATUS].attrib_type = CA_NUMBER;
row->attrib_list[HRSR_STATUS].u.unumber_value = status;

/*
| For hrSWRunPerf Table:
*/

/* =========== HrSWRunPerfCPU ==========
| UserTime + KernelTime are in 100ns (1/10th of a millionth of a second)
| units and HrSWRunPerfCPU is supposed to be in 1/100th of a second units.
|
| So .01        - second intervals
| is .010 000 0 - 100nanoseconds intervals,
|
| so dividing 100ns intervals by 100,000 gives centi-seconds.
*/

row->attrib_list[HRSP_CPU].attrib_type = CA_NUMBER;
row->attrib_list[HRSP_CPU].u.unumber_value = (ULONG)
((ProcessInfo->UserTime.QuadPart + ProcessInfo->KernelTime.QuadPart) / 100000);


/* =========== HrSWRunPerfMem ========== */
row->attrib_list[HRSP_MEM].attrib_type = CA_NUMBER;
row->attrib_list[HRSP_MEM].u.unumber_value =
                                           ProcessInfo->WorkingSetSize / 1024;


/*
| Now insert the filled-in CACHEROW structure into the
| cache-list for the hrSWRun(Perf) Table.
*/
if (AddTableRow(row->attrib_list[HRSR_INDEX].u.unumber_value,  /* Index */
                row,                                           /* Row   */
                &hrSWRunTable_cache                            /* Cache */
                ) == FALSE) {
    return ( FALSE );       /* Internal Logic Error! */
    }

return ( TRUE );
}

/* hrSWRunCache_Refresh - hrSWRun(Perf) Cache Refresh-Check Routine */
/* hrSWRunCache_Refresh - hrSWRun(Perf) Cache Refresh-Check Routine */
/* hrSWRunCache_Refresh - hrSWRun(Perf) Cache Refresh-Check Routine */

BOOL
hrSWRunCache_Refresh(
                     void
                     )

/*
|  EXPLICIT INPUTS:
|
|       None.
|
|  IMPLICIT INPUTS:
|
|       The "hrSWRunTable_cache" CACHEHEAD structure and the time when
|       it was last refreshed in module-local cell "cache_time".
|
|  OUTPUTS:
|
|     On Success/Failure:
|       The function returns TRUE.  Only if the cache-time has aged-out
|       is the cache actually rebuilt.
|
|     On any Failure:
|       If during a rebuild there is an error, this function returns FALSE.
|       The state of the cache is indeterminate.
|
|  THE BIG PICTURE:
|
|       This function is invoked before any reference is made to any SNMP
|       variable in the hrSWRun or hrSWRunPerf table.  It checks to see
|       if the cache needs to be rebuilt based on the last time it was built.
|
|       The calls to this function are strategically located in the
|       "FindInstance" and "FindNextInstance" functions in "HRSWRUNE.C"
|       (this module) and "HRSWPREN.C" (for the RunPerf table) as well
|       as in "HRSWRUN.C" for the stand-alone attribute "hrSWOSIndex".
|
|  OTHER THINGS TO KNOW:
|
*/
{
LARGE_INTEGER   now_time;       /* Current System time in 100 ns ticks */


/* Get the current time in 100 ns ticks*/
NtQuerySystemTime (&now_time);

/* If the cache is older than the maximum allowed time (in ticks) . . .  */
if ( (now_time.QuadPart - cache_time.QuadPart) > (CACHE_MAX_AGE * 10000000) ){
    return ( Gen_HrSWRun_Cache() );
    }

return ( TRUE );        /* No Error (because no refresh) */
}

/* FetchProcessParams - Fetch Path & Parameter String from Process Cmd line */
/* FetchProcessParams - Fetch Path & Parameter String from Process Cmd line */
/* FetchProcessParams - Fetch Path & Parameter String from Process Cmd line */

void
FetchProcessParams(

PSYSTEM_PROCESS_INFORMATION ProcessInfo,    /* Process for parameters     */
CHAR                      **path_str,       /* Returned PATH string       */
CHAR                      **params_str      /* Returned Parameters string */
              )
/*
|  EXPLICIT INPUTS:
|
|       "ProcessInfo" points to the process (as described by a
|       SYSTEM_PROCESS_INFORMATION structure) for which the path & parameters
|       (from the command-line) are desired.
|
|       "path_str" is the address of a pointer to be set to any "path" string.
|       "params" is the address of a pointer to be set to any "parameters"
|        string.
|
|  IMPLICIT INPUTS:
|
|       None.
|
|  OUTPUTS:
|
|     On Success:
|       Function returns pointers to a static buffer containing the
|       path & parameters section of the command line.  There may be nothing
|       in the buffer (ie just the null-termination).
|
|     On any Failure:
|       Function returns NULLS indicating a problem was encountered
|       attempting to obtain the command-line image from which the
|       path & parameter portion is to be extracted, or indicating that
|       one or both were not present.
|
|  THE BIG PICTURE:
|
|       Called from "AddHrSWRunRow()" above, this is a helper function
|       that serves to isolate the code lifted from "TLIST" from
|       the rest of the subagent.
|
|  OTHER THINGS TO KNOW:
|
|    The black magic here was lifted from sections of "TLIST".
*/
{
HANDLE                      hProcess;
PEB                         Peb;
NTSTATUS                    Status;
PROCESS_BASIC_INFORMATION   BasicInfo;
WCHAR                       szT[MAX_PATH * 2];
UNICODE_STRING              u_param;
RTL_USER_PROCESS_PARAMETERS ProcessParameters;

#define ANSI_PARAM_LEN (MAX_PATH * 2)
ANSI_STRING     param;          /* ANSI version of UNICODE command line    */
static
CHAR            pbuf[ANSI_PARAM_LEN+1];    /* Buffer for "parameters"      */
CHAR           *param_str;      /* Pointer to our final parameter string   */
DWORD           dwbytesret;     /* Count of bytes read from process memory */


/* Presume failure/nothing obtained */
*path_str = NULL;
*params_str = NULL;

/* get a handle to the process */
hProcess = OpenProcess(PROCESS_ALL_ACCESS,
                       FALSE,
                       HandleToUlong(ProcessInfo->UniqueProcessId));
if (!hProcess) {
    return;
    }


Status = NtQueryInformationProcess(hProcess, ProcessBasicInformation,
    &BasicInfo, sizeof(BasicInfo), NULL);
if (!NT_SUCCESS(Status)) {
    CloseHandle(hProcess);
    return;
    }


// get the PEB
if (ReadProcessMemory(hProcess, BasicInfo.PebBaseAddress, &Peb,
    sizeof(PEB), NULL)) {

    // get the processparameters
    if (ReadProcessMemory(hProcess, Peb.ProcessParameters,
        &ProcessParameters, sizeof(ProcessParameters), NULL)) {

        // get cmdline
        if (ReadProcessMemory(hProcess,
                              ProcessParameters.CommandLine.Buffer,
                              szT,
                              sizeof(szT),
                              &dwbytesret
                              )
            ) {

            CHAR        *scanner;       /* Used for parsing the command-line */

            /* Prep the STRING structure */
            param.Buffer = pbuf;
            param.MaximumLength = ANSI_PARAM_LEN;
            u_param.Length = (USHORT) dwbytesret;
            u_param.Buffer = szT;

            /* Convert from Unicode */
            RtlUnicodeStringToAnsiString(&param,    /* Target string        */
                                         &u_param,  /* Src                  */
                                         FALSE);    /* = Don't Allocate buf */

            /*
            | OK, we can have the following situations:
            |
            | 1)   "\system\system32\smss.exe -parameter1 -parameter2"
            |       --------path-----         -------parameters------
            |
            | 2)   "\system\system32\smss.exe"
            |       --------path-----
            |
            | 3)   "smss.exe -parameter1 -parameter2"
            |                -------parameters------
            |
            | and we want to handle this by returning "path" and "parameter"
            | as shown, where:
            |
            | 1) both path and parameters are present
            | 2) only path is present
            | 3) only parameters are present
            |
            | We do this:
            |
            |  - Scan forward for a blank.
            |    If we get one:
            |           + return the address following it as "parameters"
            |           + set the blank to a null byte (cutting off parameters)
            |    If not:
            |           + return NULL as "parameters"
            |
            |    ----Parameters are done.
            |
            |  - Perform a reverse search for "\" on whatever is now in the
            |    buffer
            |    If we find a "\":
            |           + Step forward one character and turn it into a null
            |             byte (turning buffer into string containing path).
            |           + Return the buffer address as "path"
            |    If not:
            |           + return NULL as "path"
            */
            /* Parameter */
            if ((scanner = strchr(pbuf, ' ')) != NULL) {

                /* Return address of char after blank as start of parameters */
                *params_str = (scanner + 1);
                *scanner = '\0';             /* Terminate base string */
                }
            else {
                /* No parameters */
                *params_str = NULL;
                }

            /* Path */
            if ((scanner = strrchr(pbuf, '\\')) != NULL) {
                /* Terminate the path */
                *(scanner+1) = '\0';

                /* Return start of buffer as path */
                *path_str = pbuf;
                }
            else {
                /* No path */
                *path_str = NULL;
                }

            CloseHandle(hProcess);

            /* Return address of static ANSI string buffer */
            return;
            }
        }
    }

CloseHandle(hProcess);

/* Nothing back */
return;
}

#if defined(CACHE_DUMP)

/* debug_print_hrswrun - Prints a Row from HrSWRun(Perf) Table */
/* debug_print_hrswrun - Prints a Row from HrSWRun(Perf) Table */
/* debug_print_hrswrun - Prints a Row from HrSWRun(Perf) Table */

static void
debug_print_hrswrun(
                    CACHEROW     *row  /* Row in hrSWRun(Perf) table */
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
    fprintf(OFILE, "=================================\n");
    fprintf(OFILE, "hrSWRun & hrSWRunPerf Table Cache\n");
    fprintf(OFILE, "=================================\n");
    return;
    }


fprintf(OFILE, "HrSWRunIndex . . . . . . %d\n",
        row->attrib_list[HRSR_INDEX].u.unumber_value);

fprintf(OFILE, "HrSWRunName. . . . . . . \"%s\"\n",
        row->attrib_list[HRSR_NAME].u.string_value);

fprintf(OFILE, "HrSWRunPath. . . . . . . \"%s\"\n",
        row->attrib_list[HRSR_PATH].u.string_value);

fprintf(OFILE, "HRSWRunParameters. . . . \"%s\"\n",
        row->attrib_list[HRSR_PARAM].u.string_value);

fprintf(OFILE, "HrSWRunType. . . . . . . %d ",
        row->attrib_list[HRSR_TYPE].u.unumber_value);

switch (row->attrib_list[HRSR_TYPE].u.unumber_value) {
    case 1: fprintf(OFILE, "(unknown)\n");        break;
    case 2: fprintf(OFILE, "(operatingSystem)\n");        break;
    case 3: fprintf(OFILE, "(deviceDriver)\n");        break;
    case 4: fprintf(OFILE, "(application)\n");        break;
    default:
            fprintf(OFILE, "(???)\n");
    }

fprintf(OFILE, "HrSWRunStatus. . . . . . %d ",
        row->attrib_list[HRSR_STATUS].u.unumber_value);

switch (row->attrib_list[HRSR_STATUS].u.unumber_value) {
    case 1: fprintf(OFILE, "(running)\n");        break;
    case 2: fprintf(OFILE, "(runnable)\n");        break;
    case 3: fprintf(OFILE, "(notRunnable)\n");        break;
    case 4: fprintf(OFILE, "(invalid)\n");        break;
    default:
            fprintf(OFILE, "(???)\n");
    }

fprintf(OFILE, "HrSWRunPerfCpu . . . . . %d (Centi-seconds)\n",
        row->attrib_list[HRSP_CPU].u.unumber_value);

fprintf(OFILE, "HrSWRunPerfMem . . . . . %d (Kbytes)\n",
        row->attrib_list[HRSP_MEM].u.unumber_value);
}
#endif
