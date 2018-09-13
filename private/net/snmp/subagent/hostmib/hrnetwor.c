/*
 *  HrNetworkEntry.c v0.10
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
 *    instance name routines for the HrNetworkEntry.  Actual instrumentation code is
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
 *    V1.00 - 04/28/97  D. D. Burns     Genned: Thu Nov 07 16:42:33 1996
 *
 */


#include <nt.h>
#include <windef.h>
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
#include "iphlpapi.h"     /* Access to MIB2 Utility function */



/*
 *  GetHrNetworkIfIndex
 *    The value of the ifIndex which corresponds to this network device.
 *    
 *    Gets the value for HrNetworkIfIndex.
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
 | hrNetworkIfIndex
 | 
 |  ACCESS         SYNTAX
 |  read-only      INTEGER
 | 
 | "The value of ifIndex which corresponds to this network device."
 | 
 | DISCUSSION:
 | 
 | <POA-13> A mechanism by which I can map a network interface device (as found
 | in the course of populating the hrDeviceTable) to the "ifIndex" value in
 | MIB-II needs to be described to me.
 | 
 | RESOLVED >>>>>>>>
 | <POA-13> We expose this info via MIB2UTIL.DLL.
 | RESOLVED >>>>>>>>
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.4.1.1.<instance>
 |                | | | |
 |                | | | *-HrNetworkIfIndex
 |                | | *-HrNetworkEntry
 |                | *-HrNetworkTable
 |                *-hrDevice
 */

UINT
GetHrNetworkIfIndex( 
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
if ((row = FindTableRow(index, &hrDevice_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }


/*
| By convention with "Gen_HrNetwork_Cache()", the "Hidden Context" attribute
| for the selected row is the value to be returned by hrNetworkIfIndex.
*/
*outvalue = row->attrib_list[HIDDEN_CTX].u.unumber_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrNetworkIfIndex() */


/*
 *  HrNetworkEntryFindInstance
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
HrNetworkEntryFindInstance( IN ObjectIdentifier *FullOid ,
                       IN OUT ObjectIdentifier *instance )
{
    UINT             tmp_instance;  /* Instance arc value                 */
    CACHEROW        *row;           /* Row entry fetched from cache       */

    //
    //  Developer instrumentation code to find appropriate instance goes here.
    //  For non-tables, it is not necessary to modify this routine.  However, if
    //  there is any context that needs to be set, it can be done here.
    //

    if ( FullOid->idLength <= HRNETWORKENTRY_VAR_INDEX )
	// No instance was specified
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else  if ( FullOid->idLength != HRNETWORKENTRY_VAR_INDEX + 1 )
	// Instance length is more than 1
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else
	// The only valid instance for a non-table are instance 0.  If this
	// is a non-table, the following code validates the instances.  If this
	// is a table, developer modification is necessary below.

	tmp_instance = FullOid->ids[ HRNETWORKENTRY_VAR_INDEX ] ;

        /*
        | For hrNetworkTable, the instance arc(s) is a single arc, and it must
        | correctly select an entry in the hrDeviceTable cache.
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
            | entry actually for a device of type "Network"?
            |
            | (We examine the last arc of the OID that specifies the device
            |  type in the row entry selected by the instance arc).
            */
            if (row->attrib_list[HRDV_TYPE].u.unumber_value !=
                HRDV_TYPE_LASTARC_NETWORK) {

                return SNMP_ERRORSTATUS_NOSUCHNAME;
                }

	    // the instance is valid.  Create the instance portion of the OID
	    // to be returned from this call.

	    instance->ids[ 0 ] = tmp_instance ;
	    instance->idLength = 1 ;
	}

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of HrNetworkEntryFindInstance() */



/*
 *  HrNetworkEntryFindNextInstance
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
HrNetworkEntryFindNextInstance( IN ObjectIdentifier *FullOid ,
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


    if ( FullOid->idLength <= HRNETWORKENTRY_VAR_INDEX )
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
        tmp_instance = FullOid->ids[ HRNETWORKENTRY_VAR_INDEX ] ;
        }

    /* Now go off and try to find the next instance in the table */
    if ((row = FindNextTableRow(tmp_instance, &hrDevice_cache)) == NULL) {
        return SNMP_ERRORSTATUS_NOSUCHNAME ;
        }

    /*
    | The instance arc selects an hrDeviceTable row entry, but is that
    | entry actually for a device of type "Network"?
    |
    | (We examine the last arc of the OID that specifies the device
    |  type in the row entry selected by the instance arc).
    */
    do {
        if (row->attrib_list[HRDV_TYPE].u.unumber_value ==
            HRDV_TYPE_LASTARC_NETWORK) {

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

} /* end of HrNetworkEntryFindNextInstance() */



/*
 *  HrNetworkEntryConvertInstance
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
HrNetworkEntryConvertInstance( IN ObjectIdentifier *oid_spec ,
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

} /* end of HrNetworkEntryConvertInstance() */




/*
 *  HrNetworkEntryFreeInstance
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
HrNetworkEntryFreeInstance( IN OUT InstanceName *instance )
{

  /* No action needed for hrNetwork Table */
} /* end of HrNetworkEntryFreeInstance() */

/*
| End of Generated Code
*/

/* Gen_HrNetwork_Cache - Gen. a initial cache for HrDevice PROCESSOR Table */
/* Gen_HrNetwork_Cache - Gen. a initial cache for HrDevice PROCESSOR Table */
/* Gen_HrNetwork_Cache - Gen. a initial cache for HrDevice PROCESSOR Table */

BOOL
Gen_HrNetwork_Cache(
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
|       populated with all rows required for Network devices.
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
|       populate the cache for the HrDevice table with Network-device 
|       specific entries.
|
|  OTHER THINGS TO KNOW:
|
|       Since all the attributes in the HrNetwork "sub" table are computed
|       upon request (based on cached information in a selected row in the
|       HrDevice table) there is no need to build a cache specifically for
|       this sub-table.  (This routine is only loading the HrDevice cache.
|                                                          --------
|
|       This function holds a convention with the GET routines earlier in
|       this module that the "HIDDEN_CTX" attribute for Network-devices 
|       contains a string that is the value of "hrNetworkIfIndex".
|============================================================================
| 1.3.6.1.2.1.25.3.4.1...
|                | | |
|                | | *-HrNetworkEntry
|                | *-HrNetworkTable
|                *-hrDevice
*/
{
DWORD           dwBytesRequired;    
MIB_IFTABLE    *iftable;        /* --> Heap storage containing table     */
UINT            i;              /* iftable index                         */


/*
| We fetch the IfTable from the Mib2 subsection of the agent and for each
| network interface (ie for every entry in the table) we create a row in
| the hrDeviceTable.
|
| The HIDDEN_CTX attribute value in the row will simply be the "dwIndex"
| entry in the from the iftable entry from which the row was generated.
| This becomes the value of "hrNetworkIfIndex".
*/

/* Initialize */
dwBytesRequired = 0;
iftable = NULL;

/* Ask for the size of the table from "iphlpapi" */
if (GetIfTable(iftable, &dwBytesRequired, TRUE) != ERROR_INSUFFICIENT_BUFFER) {
    return ( FALSE );
    }

/* Allocate necessary memory */
if ((iftable = (MIB_IFTABLE *)malloc(dwBytesRequired)) == NULL) {
    return ( FALSE );
    }

/* Ask for the table information from "iphlpapi" */
if (GetIfTable(iftable, &dwBytesRequired, TRUE) != NO_ERROR ) {

    /* Release */
    free(iftable);

    /* Something blew */
    return ( FALSE );
    }

/* Sweep thru any table creating hrDevice rows */ 
for (i = 0; i < iftable->dwNumEntries; i += 1) {

    /*
    | "Hidden Context" is the ifTable index passed in from the GetIfTable()
    |
    | It will be returned as the value of "hrNetworkIfIndex" by the Get
    | function.
    */

    if (AddHrDeviceRow(type_arc,                     // DeviceType OID Last-Arc
        (unsigned char *) &iftable->table[i].bDescr, // Device Description
                       &iftable->table[i].dwIndex,   // Hidden Ctx "index"
                       CA_NUMBER                     // Hidden Ctx "type"
                       ) == NULL ) {

        /* Release */
        free(iftable);

        /* Something blew */
        return ( FALSE );
        }
    }

/* Release */
free(iftable);

return ( TRUE );
}
