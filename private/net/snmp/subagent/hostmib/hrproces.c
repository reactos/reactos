/*
 *  HrProcessorEntry.c v0.10
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
 *    instance name routines for the HrProcessorEntry.  Actual instrumentation code is
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
 *    V1.00 - 04/28/97  D. D. Burns     Genned: Thu Nov 07 16:42:19 1996
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
#include "user.h"         /* Developer supplied include file */
#include "HMCACHE.H"      /* Cache-related definitions       */
#include "HRDEVENT.H"     /* HrDevice Table-related definitions */
#include <stdio.h>

/*
|==============================================================================
| "Processor-Information Buffer" Definition
|
| This definition defines a logical "Processor Information Block" where we
| can store all the information returned from an NtQuerySystemInformation()
| call that requests "SystemProcessorPerformanceInformation" for each running
| processor.
*/
typedef
    struct  pi_block {
        struct pi_block  *other;      // Associated "other" buffer

        LARGE_INTEGER     sys_time;   // Time when "pi_array" was last
                                      //      refreshed in 100ns ticks

        SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
                          *pi_array;   // Array: One element per processor
        DWORD              pi_size;    // Size in bytes of "pi_array" storage
                      }
                       PI_BLOCK;

/*
|==============================================================================
| "Processor-Information Buffer" Instances
|
| We create two instances of a Processor Information Buffer, one for the
| "oldest" and a second for "newest" samples of timer values.  Maintaining 
| two enables us to compute the average over time for processor loads.
|
| These blocks are initialized by code in "Gen_HrProcessor_Cache()" in
| this module.
|
| These buffers are refreshed in an alternating manner by function
| "hrProcessLoad_Refresh()" (in this module) that itself is invoked on a
| timer-driven basis.  (See source for the function).
*/
static
PI_BLOCK        pi_buf1;        // First Buffer
static
PI_BLOCK        pi_buf2;        // Second Buffer 


/*
|==============================================================================
| Oldest "Processor-Information Buffer"
|
| This cell points at one of the two PI_BLOCKs above.  It always points to
| the buffer block that has the "oldest" data in it.
*/
static
PI_BLOCK       *oldest_pi=NULL;

#if defined(PROC_CACHE)         // For debug cache dump only
static
int             processor_count;
#endif


/*
 *  GetHrProcessorFrwID
 *    The product ID of the firmware associated with the processor.
 *    
 *    Gets the value for HrProcessorFrwID.
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
 | hrProcessorFrwID
 | 
 |  ACCESS         SYNTAX
 |  read-only      ProductID
 | 
 | "The product ID of the firmware associated with the processor."
 | 
 | DISCUSSION:
 | 
 | <POA-11> The underlying syntax of this attribute is Object Identifier.  None
 | of the documented Win32 API functions seem capable of reporting this value.
 | We are allowed to report "unknownProductID"  ("0.0") in liew of the real
 | value, and this will be hardcoded unless an alternative is specified.
 | 
 | RESOLVED >>>>>>>>
 | <POA-11> Returning an unknown Product ID is acceptable.
 | RESOLVED >>>>>>>>
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.3.1.1.<instance>
 |                | | | |
 |                | | | *-hrProcessorFrwID
 |                | | *-hrProcessorEntry
 |                | *-hrProcessorTable
 |                *-hrDevice
 */

UINT
GetHrProcessorFrwID( 
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

} /* end of GetHrProcessorFrwID() */


/*
 *  GetHrProcessorLoad
 *    The average, over the last minute, of the percentage of time that this 
 *    processor was not idle.
 *    
 *    Gets the value for HrProcessorLoad.
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
 | hrProcessorLoad
 | 
 |  ACCESS         SYNTAX
 |  read-only      INTEGER (0..100)
 | 
 | "The average, over the last minute, of the percentage of time that this
 | processor was not idle."
 | 
 | DISCUSSION:
 | 
 | <POA-12> It seems likely to me that this performance statistic might be
 | maintained or be derivable from performance information maintained in the
 | Registry.  Please describe.
 | 
 | RESOLVED >>>>>>>>
 | <POA-12> I think we should just use the PerfMon code for this.
 | RESOLVED >>>>>>>>
 | 
 |============================================================================
 | We reference a continuously updated module-local cache of CPU time-usage
 | info maintained in the buffer-blocks "pi_buf1" and "pi_buf2" defined
 | at the beginning of this module.  In the code below, we reach into these
 | caches and compute the processor load for the processor specified.
 |============================================================================
 | 1.3.6.1.2.1.25.3.3.1.2.<instance>
 |                | | | |
 |                | | | *-hrProcessorLoad
 |                | | *-hrProcessorEntry
 |                | *-hrProcessorTable
 |                *-hrDevice
 */

UINT
GetHrProcessorLoad( 
        OUT Integer *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
ULONG           index;          /* As fetched from instance structure   */
CACHEROW        *row;           /* Row entry fetched from cache         */
ULONG           p;              /* Selected Processor (number from 0)   */
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
               *oldest, *newest;/* --> CPU data for "n" and "n+1minute" */
LONGLONG        llDendiff;      /* Difference Denominator               */
LONGLONG        llNewNum;       /* Numerator of Newest Time-count       */
LONGLONG        llOldNum;       /* Numerator of Oldest Time-count       */
double          fNum,fDen;      /* Floated versions of LONGLONGs        */
double          fload;          /* Percentage Load                      */


/*
| Grab the instance information
*/
index = GET_INSTANCE(0);

/*
| Use it to find the right entry in the cache
*/
if ((row = FindTableRow(index, &hrDevice_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| By convention with "Gen_HrProcessor_Cache()" the cache initialization
| routine, the "hidden context" for devices which are "Processors" is
| the Processor Number, starting with 0.
*/
p = row->attrib_list[HIDDEN_CTX].u.unumber_value;


/*
| We compute the load using "SystemProcessorPerformanceInformation" that
| has been gathered for all processors in buffers maintained in "pi_buf1"
| and "pi_buf2".
|
| Obtain pointers to the "newest" and "oldest" slug of information for
| the specified processor out of "pi_buf1/2".
*/
oldest = &(oldest_pi->pi_array[p]);
newest = &(oldest_pi->other->pi_array[p]);


/*
| The performance info (as of this writing) we need comes from:
|
| typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
|     LARGE_INTEGER IdleTime;
|     LARGE_INTEGER KernelTime;
|     LARGE_INTEGER UserTime;
|     LARGE_INTEGER DpcTime;          // DEVL only
|     LARGE_INTEGER InterruptTime;    // DEVL only
|     ULONG InterruptCount;
| } ....
|
| where "IdleTime" is taken to be time spent by this processor in its
|                  idlethread.
|
|       "KernelTime" is taken to be total time spent by processor in kernel
|                  mode code (including the idle thread).
|
|       "UserTime" is taken to be total time spent by processor in user mode
|                  code.
|
| all in ticks of 100ns (one tenth of a millionth of a second).
|
| So total "Not-Idle" time is "(KernelTime-IdleTime) + UserTime" in ticks.
| Total time for the interval is the difference in the "sys_time" that is
| associated with each buffer ("oldest_pi->" and "oldest_pi->other->").
|
*/
llNewNum = (newest->KernelTime.QuadPart - newest->IdleTime.QuadPart)
                  + newest->UserTime.QuadPart;
llOldNum = (oldest->KernelTime.QuadPart - oldest->IdleTime.QuadPart)
                  + oldest->UserTime.QuadPart;

            /* (Newest System-Time)             -     (Oldest System-Time)  */
llDendiff = oldest_pi->other->sys_time.QuadPart - oldest_pi->sys_time.QuadPart;

/* If there will be no divide by 0 */
if ( llDendiff != 0 ) {

    /*
    | Now float these guys and convert to a percentage.
    */
    fNum = (double) (llNewNum - llOldNum);
    fDen = (double) llDendiff;

    fload = (fNum / fDen) * 100.0;
    }
else {
    fload = 0.0;
    }
 
*outvalue = (int) fload;      // Truncate to integer

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrProcessorLoad() */


/*
 *  HrProcessorEntryFindInstance
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
HrProcessorEntryFindInstance( IN ObjectIdentifier *FullOid ,
                       IN OUT ObjectIdentifier *instance )
{
    UINT            tmp_instance ;  /* Instance arc value                 */
    CACHEROW        *row;           /* Row entry fetched from cache       */

    //
    //  Developer instrumentation code to find appropriate instance goes here.
    //  For non-tables, it is not necessary to modify this routine.  However, if
    //  there is any context that needs to be set, it can be done here.
    //

    if ( FullOid->idLength <= HRPROCESSORENTRY_VAR_INDEX )
	// No instance was specified
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else  if ( FullOid->idLength != HRPROCESSORENTRY_VAR_INDEX + 1 )
	// Instance length is more than 1
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else
	// The only valid instance for a non-table are instance 0.  If this
	// is a non-table, the following code validates the instances.  If this
	// is a table, developer modification is necessary below.

	tmp_instance = FullOid->ids[ HRPROCESSORENTRY_VAR_INDEX ] ;

        /*
        | For hrProcessorTable, the instance arc(s) is a single arc, and 
        | it must correctly select an entry in the hrDeviceTable cache.
        |
        | Check that here.
        */
	if ( (row = FindTableRow(tmp_instance, &hrDevice_cache)) == NULL ) {
	    return SNMP_ERRORSTATUS_NOSUCHNAME ;
            }
	else
	{
            /*
            | The instance arc selects an hrDeviceTable row entry, but is that
            | entry actually for a device of type "Processor"?
            |
            | (We examine the last arc of the OID that specifies the device
            |  type in the row entry selected by the instance arc).
            */
            if (row->attrib_list[HRDV_TYPE].u.unumber_value !=
                HRDV_TYPE_LASTARC_PROCESSOR) {

                return SNMP_ERRORSTATUS_NOSUCHNAME;
                }

	    // the instance is valid.  Create the instance portion of the OID
	    // to be returned from this call.
	    instance->ids[ 0 ] = tmp_instance ;
	    instance->idLength = 1 ;
	}

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of HrProcessorEntryFindInstance() */



/*
 *  HrProcessorEntryFindNextInstance
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
HrProcessorEntryFindNextInstance( IN ObjectIdentifier *FullOid ,
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


    if ( FullOid->idLength <= HRPROCESSORENTRY_VAR_INDEX )
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
        tmp_instance = FullOid->ids[ HRPROCESSORENTRY_VAR_INDEX ] ;
        }

    /* Now go off and try to find the next instance in the table */
    if ((row = FindNextTableRow(tmp_instance, &hrDevice_cache)) == NULL) {
        return SNMP_ERRORSTATUS_NOSUCHNAME ;
        }

    /*
    | The instance arc selects an hrDeviceTable row entry, but is that
    | entry actually for a device of type "Processor"?
    |
    | (We examine the last arc of the OID that specifies the device
    |  type in the row entry selected by the instance arc).
    */
    do {
        if (row->attrib_list[HRDV_TYPE].u.unumber_value ==
            HRDV_TYPE_LASTARC_PROCESSOR) {

            /* Found an hrDeviceTable entry for the right device type */
            break;
            }

        /* Step to the next row in the table */
        row = GetNextTableRow( row );
        }
        while ( row != NULL );

    /* If we found a proper device-type row . . . */
    if ( row != NULL) {
        instance->ids[ 0 ] = row->index ;
        instance->idLength = 1 ;
        }
    else {

        /*
        | Fell off the end of the hrDeviceTable without finding a row
        | entry that had the right device type.
        */
        return SNMP_ERRORSTATUS_NOSUCHNAME ;
        }

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of HrProcessorEntryFindNextInstance() */



/*
 *  HrProcessorEntryConvertInstance
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
HrProcessorEntryConvertInstance( IN ObjectIdentifier *oid_spec ,
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

} /* end of HrProcessorEntryConvertInstance() */




/*
 *  HrProcessorEntryFreeInstance
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
HrProcessorEntryFreeInstance( IN OUT InstanceName *instance )
{

  /* No action needed for hrProcessor Table */
} /* end of HrProcessorEntryFreeInstance() */

/*
| End of Generated Code
*/

/* Gen_HrProcessor_Cache - Gen. a initial cache for HrDevice PROCESSOR Table */
/* Gen_HrProcessor_Cache - Gen. a initial cache for HrDevice PROCESSOR Table */
/* Gen_HrProcessor_Cache - Gen. a initial cache for HrDevice PROCESSOR Table */

BOOL
Gen_HrProcessor_Cache(
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
|       None.
|
|  OUTPUTS:
|
|     On Success:
|       Function returns TRUE indicating that the HrDevice cache has been fully
|       populated with all rows required for Processor devices.
|
|     On any Failure:
|       Function returns FALSE (indicating "not enough storage" or other
|       internal logic error).
|
|  THE BIG PICTURE:
|
|       At subagent startup time, the cache for each table in the MIB is
|       populated with rows for each row in the table.  This function is
|       invoked by the start-up code in "Gen_HrDevice_Cache()" to
|       populate the cache for the HrDevice table with processor-specific
|       entries.
|
|  OTHER THINGS TO KNOW:
|
|       This function is loading entries into the existing HrDevice cache
|       for devices of type "processor" as well as setting up what logically
|       amounts to a "cache" of information used to compute the value of
|       hrProcessorLoad.
|
|       Specifically, this function initializes module-local cells that
|       describe buffers containing processor-time information for each
|       processor.
|
|       This function holds a convention with the GET routines earlier in
|       this module that the "HIDDEN_CTX" attribute for processors contains
|       a number that can be used to select which processor information
|       is to be returned.  We generate this number here.
|============================================================================
| 1.3.6.1.2.1.25.3.3.1...
|                | | |
|                | | *hrProcessorEntry
|                | *hrProcessorTable
|                *-hrDevice
|
*/
{
SYSTEM_INFO     sys_info;       /* Filled in by GetSystemInfo for processors */
UINT            i;              /* Handy-Dandy loop index                    */
char           *descr;          /* Selected description string               */


/* Acquire system information statistics */
GetSystemInfo(&sys_info);

/*
| Build a description based on the system info.  We presume all processors
| are identical.
*/
switch (sys_info.wProcessorArchitecture) {


    case PROCESSOR_ARCHITECTURE_INTEL:
        switch (sys_info.wProcessorLevel) {
            case 3:     descr = "Intel 80386";     break;
            case 4:     descr = "Intel 80486";     break;
            case 5:     descr = "Intel Pentium";   break;
            default:    descr = "Intel";           break;
            }
        break;
                
    case PROCESSOR_ARCHITECTURE_ALPHA:
        switch (sys_info.wProcessorLevel) {
            case 21064: descr = "Alpha 21064";     break;
            case 21066: descr = "Alpha 21066";     break;
            case 21164: descr = "Alpha 21164";     break;
            default:    descr = "DEC Alpha";       break;
            }
        break;

    case PROCESSOR_ARCHITECTURE_MIPS:
        switch (sys_info.wProcessorLevel) {
            case 4:     descr = "MIPS R4000";     break;
            default:    descr = "MIPS";           break;
            }
        break;

    case PROCESSOR_ARCHITECTURE_PPC:
        switch (sys_info.wProcessorLevel) {
            case 1:     descr = "PPC 601";       break;
            case 3:     descr = "PPC 603";       break;
            case 4:     descr = "PPC 604";       break;
            case 6:     descr = "PPC 603+";      break;
            case 9:     descr = "PPC 604+";      break;
            case 20:    descr = "PPC 620";       break;
            default:    descr = "PPC";           break;
            }
        break;
    
    case PROCESSOR_ARCHITECTURE_UNKNOWN:
    default:
        descr = "Unknown Processor Type";
    }

/*
| For every processor we have in the system, fill in a row in the hrDevice
| table.
*/
for (i = 0; i < sys_info.dwNumberOfProcessors; i += 1) {

    /*
    | "Hidden Context" is a generated number starting at 0 which we'll
    | consider to be the processor number..
    */

    if (AddHrDeviceRow(type_arc,        // DeviceType OID Last-Arc
                       descr,           // Device Description
                       &i,              // Hidden Ctx "Processor #"
                       CA_NUMBER        // Hidden Ctx type
                       ) == NULL ) {

        /* Something blew */
        return ( FALSE );
        }
    }

/*
| Now initialize the PI_BLOCK instances needed to compute the hrProcessorLoad
| and the pointer to the PI_BLOCK instance that is to be considered the
| "oldest".
*/

/*
| Storage for both buffers. . . .*/
pi_buf1.pi_size = sys_info.dwNumberOfProcessors *
                  sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);
if ((pi_buf1.pi_array = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *)
                        malloc(pi_buf1.pi_size)) == NULL) {
    return ( FALSE );  // Out of Memory
    }

pi_buf2.pi_size = sys_info.dwNumberOfProcessors *
                  sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);
if ((pi_buf2.pi_array = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *)
                        malloc(pi_buf2.pi_size)) == NULL) {
    return ( FALSE );  // Out of Memory
    }

/*
| Now "hook" the two buffer blocks together so we can switch between them
| symmetrically.
*/
pi_buf1.other = &pi_buf2;
pi_buf2.other = &pi_buf1;

#if defined(PROC_CACHE)         // For debug cache dump only
processor_count = sys_info.dwNumberOfProcessors;
#endif

/*
| Pretend the first is the oldest and get it refreshed.
*/
oldest_pi = &pi_buf1;     // Select it
hrProcessLoad_Refresh();  // Refresh it, and select the other as "oldest"

SleepEx(1, FALSE);        // Pause one millisecond to avoid divide by 0
hrProcessLoad_Refresh();  // Refresh the second and select other as "oldest"

/*
| Now each Processor Information Block contains full information (about
| all processors) separated in time by 1 millisecond.  The "oldest" will
| be refreshed periodically every minute by the timer which is initialized
| via a call to "TrapInit()" made from source "mib.c" after the initialization
| of caches is complete.  Once the timer begins ticking regularly, the time
| samples in these two PI_BLOCK buffers will differ by one minute, (the period
| of the timer) which is the period required by the definition of
| "hrProcessorLoad".
*/


return ( TRUE );
}


/* hrProcessLoad_Refresh - Processor Load Time-Information Refresh Routine */
/* hrProcessLoad_Refresh - Processor Load Time-Information Refresh Routine */
/* hrProcessLoad_Refresh - Processor Load Time-Information Refresh Routine */

void
hrProcessLoad_Refresh(
                      void
                      )

/*
|  EXPLICIT INPUTS:
|
|       None.
|
|  IMPLICIT INPUTS:
|
|       The "Processor-Information Buffer" specified by module-local cell
|       "oldest_pi" is refreshed with new time information fetched from
|       the kernel.
|
|  OUTPUTS:
|
|     On Success:
|       The PI_Block specified by "oldest_pi" is refreshed and "oldest_pi"
|       is set to point to the other PI_BLOCK.
|
|     On any Failure:
|       Function simply returns.
|
|  THE BIG PICTURE:
|
|       At subagent startup time, a timer is created by code in "TrapInit()"
|       set to "tick" at an interval of one minute.
|
|       Each time the timer goes off, the SNMP Agent calls the 
|       "SnmpExtensionTrap()" standard entry point for the sub agent.  Rather
|       than handle a trap, that function will invoke this function which
|       gathers CPU performance data so that the hrProcessLoad value can be
|       properly computed.
|
|  OTHER THINGS TO KNOW:
|
|       We alternate the buffer into which the newest CPU data is placed
|       by simply changing "oldest_pi" (each time we're invoked) to point
|       to the "other" buffer after we're done refreshing the oldest buffer.
|       In this manner, we always have two buffers of Processor Load info
|       allowing us to compute the load during the times associated with
|       those two buffers.
*/
{
NTSTATUS        ntstatus;
DWORD           bytesused;


/* Get the current system-time in 100ns intervals . . .*/
ntstatus = NtQuerySystemTime (&oldest_pi->sys_time);

/*
| . . . and as rapidly thereafter refresh the oldest buffer with information
|       on all processors */
ntstatus = NtQuerySystemInformation(SystemProcessorPerformanceInformation,
                                    oldest_pi->pi_array,
                                    oldest_pi->pi_size,
                                    &bytesused);

#if defined(PROC_CACHE)
/* =========================== DEBUG DUMP ================================== */
    {
    FILE            *pfile;                 /* Dump goes here        */
    time_t          ltime;                  /* For debug message     */
    int             i;                      /* Loop index            */
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
                    *oldest, *newest;/* --> CPU data for "n" and "n+1minute" */


    if ((pfile=fopen(PROC_FILE, "a+")) != NULL) {

        time( &ltime);
        fprintf(pfile, "\n=============== Open for appending: %s", ctime( &ltime ));

        fprintf(pfile, "Periodic Refresh of \"oldest_pi\" @ %x\n", oldest_pi);

        /* For each processor . . . */
        for (i=0; i < processor_count; i += 1) {

            fprintf(pfile, "For Processor %d:\n", i);

            oldest = &(oldest_pi->pi_array[i]);
            //newest = &(oldest_pi->other->pi_array[i]);

            fprintf(pfile, "  IdleTime   = (HI) %x  (LO) %x\n",
                    oldest->IdleTime.HighPart, oldest->IdleTime.LowPart);
            fprintf(pfile, "  KernelTime = (HI) %x  (LO) %x\n",
                    oldest->KernelTime.HighPart, oldest->KernelTime.LowPart);
            fprintf(pfile, "  UserTime   = (HI) %x  (LO) %x\n",
                    oldest->UserTime.HighPart, oldest->UserTime.LowPart);
            }
        }

    fclose(pfile);
    }
/* =========================== DEBUG DUMP ================================== */
#endif

/* Now the other buffer contains the "oldest" data, so change "oldest_pi" */
oldest_pi = oldest_pi->other;
}
