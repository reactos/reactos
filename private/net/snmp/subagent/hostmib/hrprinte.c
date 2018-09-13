/*
 *  HrPrinterEntry.c v0.10
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
 *    instance name routines for the HrPrinterEntry.  Actual instrumentation code is
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
 *    V1.00 - 04/27/97  D. D. Burns     Genned: Thu Nov 07 16:42:50 1996
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
#include "HRDEVENT.H"     /* HrDevice Table  related definitions */
#include <winspool.h>     /* Needed to acquire printer-status*/



/*
 *  GetHrPrinterStatus
 *    The current status of this printer device.
 *    
 *    Gets the value for HrPrinterStatus.
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
 | hrPrinterStatus
 | 
 |  ACCESS         SYNTAX
 |  read-only      INTEGER {other(1),unknown(2),idle(3),printing(4),warmup(5)}
 | 
 | "The current status of this printer device.  When in the idle(1), printing(2),
 | or warmup(3) state, the corresponding hrDeviceStatus should be running(2) or
 | warning(3).  When in the unknown state, the corresponding hrDeviceStatus
 | should be unknown(1)."
 | 
 | DISCUSSION:
 | 
 | <POA-14> The actual status and error state of a hardware printer is deeply
 | buried with respect to the application level.  Given that we can acquire
 | the name of the printer driver for a printer, some input on how best to
 | report the hardware status and error state would be appreciated.
 | 
 | LIMITED RESOLUTION >>>>>>>>
 | <POA-14> We report logical printers as though they were hardware printers.
 | This results in certain "undercount" and "overcount" situations when using
 | Host MIB values for inventory purposes.  For status purposes, the status
 | of the logical printers is returned.
 | LIMITED RESOLUTION >>>>>>>>
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.5.1.1.<instance>
 |                | | | |
 |                | | | *hrPrinterStatus
 |                | | *hrPrinterEntry
 |                | *hrPrinterTable
 |                *-hrDevice
 */

UINT
GetHrPrinterStatus( 
        OUT INThrPrinterStatus *outvalue ,
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
| Ok, here's the skinny:  Just about all the status information that can
| be acquired about a printer under NT is acquired by
| "COMPUTE_hrPrinter_status()" that was written to service the need of
| reporting general status for printer devices out of "hrDeviceStatus".
|
| Since we can't gather any more information reliably than this function
| does, we simply call it and map the return codes it provides as values
| for "hrDeviceStatus" into codes appropriate for this attribute variable.
|
*/
if (COMPUTE_hrPrinter_status(row, (UINT *) outvalue) != TRUE) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/* We get back:
|               "unknown" = 1   If we can't open the printer at all.
|
|               "running" = 2   If we can open the printer and no status is
|                               showing on it.
|               "warning" = 3   If we can open the printer but PAUSED or
|                               PENDING_DELETION is showing on it.
*/
switch (*outvalue) {

    case 1:             // "unknown" for hrDeviceStatus
        *outvalue = 2;  // goes to-> "unknown" for hrPrinterStatus
        break;


    case 2:             // "running" for hrDeviceStatus
    case 3:             // "warning" for hrDeviceStatus
    default:
        *outvalue = 1;  // goes to-> "other" for hrPrinterStatus
        break;
    }

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrPrinterStatus() */


/*
 *  GetHrPrinterDetectedErrorState
 *    The error conditions as detected by the printer.
 *    
 *    Gets the value for HrPrinterDetectedErrorState.
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
 | hrPrinterDetectedErrorState
 | 
 |  ACCESS         SYNTAX
 |  read-only      OCTET STRING
 | 
 | "This object represents any error conditions detected by the printer.  The
 | error conditions are encoded as bits in an octet string, with the following
 | definitions:
 | 
 |     Condition         Bit #    hrDeviceStatus
 | 
 |     lowPaper          0        warning(3)
 |     noPaper           1        down(5)
 |     lowToner          2        warning(3)
 |     noToner           3        down(5)
 |     doorOpen          4        down(5)
 |     jammed            5        down(5)
 |     offline           6        down(5)
 |     serviceRequested  7        warning(3)
 | 
 | If multiple conditions are currently detected and the hrDeviceStatus would not
 | otherwise be unknown(1) or testing(4), the hrDeviceStatus shall correspond to
 | the worst state of those indicated, where down(5) is worse than warning(3)
 | which is worse than running(2).
 | 
 | Bits are numbered starting with the most significant bit of the first byte
 | being bit 0, the least significant bit of the first byte being bit 7, the most
 | significant bit of the second byte being bit 8, and so on.  A one bit encodes
 | that the condition was detected, while a zero bit encodes that the condition
 | was not detected.
 | 
 | This object is useful for alerting an operator to specific warning or error
 | conditions that may occur, especially those requiring human intervention."
 | 
 | DISCUSSION:
 | 
 | (See discussion above for "hrPrinterStatus").
 |
 |============================================================================
 | 1.3.6.1.2.1.25.3.5.1.2.<instance>
 |                | | | |
 |                | | | *hrPrinterDetectedErrorState
 |                | | *hrPrinterEntry
 |                | *hrPrinterTable
 |                *-hrDevice
 */

UINT
GetHrPrinterDetectedErrorState( 
        OUT OctetString *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{

/*
| The deal on this attribute is that under NT, you can barely tell
| if the printer is on, unlike 95, where you can tell if it is on its
| second bottle of toner for the day.
|
| Consequently we return a single all-bits zero octet regardless of 
| the instance value (which by now in the calling sequence of things
| has been validated anyway).
*/

outvalue->length = 1;
outvalue->string = "\0";

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrPrinterDetectedErrorState() */


/*
 *  HrPrinterEntryFindInstance
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
HrPrinterEntryFindInstance( IN ObjectIdentifier *FullOid ,
                       IN OUT ObjectIdentifier *instance )
{
    UINT             tmp_instance;  /* Instance arc value                 */
    CACHEROW        *row;           /* Row entry fetched from cache       */

    //
    //  Developer instrumentation code to find appropriate instance goes here.
    //  For non-tables, it is not necessary to modify this routine.  However, if
    //  there is any context that needs to be set, it can be done here.
    //

    if ( FullOid->idLength <= HRPRINTERENTRY_VAR_INDEX )
	// No instance was specified
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else  if ( FullOid->idLength != HRPRINTERENTRY_VAR_INDEX + 1 )
	// Instance length is more than 1
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else
	// The only valid instance for a non-table are instance 0.  If this
	// is a non-table, the following code validates the instances.  If this
	// is a table, developer modification is necessary below.

	tmp_instance = FullOid->ids[ HRPRINTERENTRY_VAR_INDEX ] ;

        /*
        | For hrPrinterTable, the instance arc(s) is a single arc, and it must
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
            | entry actually for a device of type "Printer"?
            |
            | (We examine the last arc of the OID that specifies the device
            |  type in the row entry selected by the instance arc).
            */
            if (row->attrib_list[HRDV_TYPE].u.unumber_value !=
                HRDV_TYPE_LASTARC_PRINTER) {

                return SNMP_ERRORSTATUS_NOSUCHNAME;
                }

	    // the instance is valid.  Create the instance portion of the OID
	    // to be returned from this call.

	    instance->ids[ 0 ] = tmp_instance ;
	    instance->idLength = 1 ;
	}

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of HrPrinterEntryFindInstance() */



/*
 *  HrPrinterEntryFindNextInstance
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
HrPrinterEntryFindNextInstance( IN ObjectIdentifier *FullOid ,
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


    if ( FullOid->idLength <= HRPRINTERENTRY_VAR_INDEX )
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
        tmp_instance = FullOid->ids[ HRPRINTERENTRY_VAR_INDEX ] ;
        }

    /* Now go off and try to find the next instance in the table */
    if ((row = FindNextTableRow(tmp_instance, &hrDevice_cache)) == NULL) {
        return SNMP_ERRORSTATUS_NOSUCHNAME ;
        }

    /*
    | The instance arc selects an hrDeviceTable row entry, but is that
    | entry actually for a device of type "Printer"?
    |
    | (We examine the last arc of the OID that specifies the device
    |  type in the row entry selected by the instance arc).
    */
    do {
        if (row->attrib_list[HRDV_TYPE].u.unumber_value ==
            HRDV_TYPE_LASTARC_PRINTER) {

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

} /* end of HrPrinterEntryFindNextInstance() */



/*
 *  HrPrinterEntryConvertInstance
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
HrPrinterEntryConvertInstance( IN ObjectIdentifier *oid_spec ,
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

} /* end of HrPrinterEntryConvertInstance() */




/*
 *  HrPrinterEntryFreeInstance
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
HrPrinterEntryFreeInstance( IN OUT InstanceName *instance )
{

  /* No action needed for hrPrinter Table */
} /* end of HrPrinterEntryFreeInstance() */

/*
| End of Generated Code
*/

/* Gen_HrPrinter_Cache - Generate a initial cache for HrDevice PRINTER Table */
/* Gen_HrPrinter_Cache - Generate a initial cache for HrDevice PRINTER Table */
/* Gen_HrPrinter_Cache - Generate a initial cache for HrDevice PRINTER Table */

BOOL
Gen_HrPrinter_Cache(
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
|       populated with all rows required for Printer devices.
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
|       populate the cache for the HrDevice table with printer-specific
|       entries.
|
|  OTHER THINGS TO KNOW:
|
|       Since all the attributes in the HrPrinter "sub" table are computed
|       upon request (based on cached information in a selected row in the
|       HrDevice table) there is no need to build a cache specifically for
|       this sub-table.  (This routine is loading the HrDevice cache despite
|       it's name).                                   --------
|
|       This function holds a convention with the GET routines earlier in
|       this module that the "HIDDEN_CTX" attribute for printers contains
|       a string that can be used in OpenPrinter to get a handle to that
|       printer.
|============================================================================
| 1.3.6.1.2.1.25.3.5.1...
|                | | |
|                | | *hrPrinterEntry
|                | *hrPrinterTable
|                *-hrDevice
|
*/
{
CHAR    temp[8];                /* Temporary buffer for first call         */
DWORD   PI_request_len;         /* Printer Info: Storage actually needed   */
DWORD   PI_current_len;         /* Printer Info: Storage used on 2nd call  */
DWORD   PI_count;               /* Count of Printer Infos returned         */
UINT    i;                      /* Handy-Dandy loop index                  */
PRINTER_INFO_4 
        *PrinterInfo;           /* --> allocated storage for drive strings */


/*
| We're going to call EnumPrinters() twice, once to get the proper
| buffer size, and the second time to actually get the printer info.
*/
EnumPrinters(PRINTER_ENUM_LOCAL,  // Flags
             NULL,                // Name (ignored)
             4,                   // Level
             temp,                // Buffer
             1,                   // "Too Small" Buffer size
             &PI_request_len,     // Required length... comes back.
             &PI_count
             );
/*
| Grab enough storage for the enumeration structures
*/
if ( (PrinterInfo = malloc(PI_request_len)) == NULL) {
    /* Storage Request failed altogether, can't initialize */
    return ( FALSE );
    }

/* Now get the real stuff */
if (!EnumPrinters(PRINTER_ENUM_LOCAL,  // Flags
             NULL,                // Name (ignored)
             4,                   // Level
   (unsigned char *) PrinterInfo, // Buffer to receive enumeration
             PI_request_len,      // Actual buffer size
             &PI_request_len,     // Required length... comes back.
             &PI_count
             )) {

    /* Failed for some reason */
    free( PrinterInfo );
    return ( FALSE );
    }


/*
| Now swing down the list, and for every LOCAL printer,
|
|  + Fetch the description
|  + Make an hrDevice table row entry with the printer name & description
*/
for (i = 0; i < PI_count; i += 1) {

    /* If it is a Local printer ... */
    if (PrinterInfo[i].Attributes & PRINTER_ATTRIBUTE_LOCAL) {

        HANDLE  hprinter;       /* Handle to a printer */

        /* Open it to get a handle */
        if (OpenPrinter(PrinterInfo[i].pPrinterName,    // Printer Name
                        &hprinter,                      // Receive handle here
                        NULL                            // Security
                        ) == TRUE ) {

            PRINTER_INFO_2     *p2;
            DWORD              P2_request_len; /* Bytes-needed by GetPrinter */


            /*
            | Printer is Open, get a PRINTER_INFO_2 "slug-o-data"
            |
            | 1st call: Fails, but gets buffer size needed.
            */
            GetPrinter(hprinter,        // printer handle
                       2,               // Level 2
                       temp,            // Buffer for INFO_2
                       1,               // Buffer-too-small
                       &P2_request_len  // What we really need
                       );

            /*
            | Grab enough storage for the PRINTER_INFO_2 structure
            */
            if ( (p2 = malloc(P2_request_len)) == NULL) {

                /* Storage Request failed altogether, can't initialize */
                free( PrinterInfo );
                ClosePrinter( hprinter );
                return ( FALSE );
                }

            /*
            | 2nd call: Should succeed.
            */
            if (GetPrinter(hprinter,        // printer handle
                           2,               // Level 2
                     (unsigned char *) p2,  // Buffer for INFO_2
                           P2_request_len,  // Buffer-just-right
                           &P2_request_len  // What we really need
                           ) == TRUE) {


                /* Add a row to HrDevice Table
                |
                | We're using the printer-driver name as a "Poor Man's"
                | description: the driver "names" are quite descriptive with
                | version numbers yet!.
                |
                | The Hidden Context is the name needed to open the printer
                | to gain information about its status.
                */
                if (AddHrDeviceRow(type_arc,         // DeviceType OID Last-Arc
                                   p2->pDriverName,  // Used as description
                                   PrinterInfo[i].pPrinterName, // Hidden Ctx
                                   CA_STRING         // Hidden Ctx type
                                   ) == NULL ) {

                    /* Failure at a lower level: drop everything */
                    free( p2 );
                    free( PrinterInfo );
                    ClosePrinter( hprinter );
                    return ( FALSE );
                    }
                }

            /* Close up shop on this printer*/
            free( p2 );
            ClosePrinter( hprinter );
            }
        }
    }

free( PrinterInfo );

return ( TRUE );
}

/* COMPUTE_hrPrinter_errors - Compute "hrDeviceErrors" for a Printer device */
/* COMPUTE_hrPrinter_errors - Compute "hrDeviceErrors" for a Printer device */
/* COMPUTE_hrPrinter_errors - Compute "hrDeviceErrors" for a Printer device */

BOOL
COMPUTE_hrPrinter_errors(
                         CACHEROW *row,
                         UINT     *outvalue
                         )

/*
|  EXPLICIT INPUTS:
|
|       "row" points to the hrDevice cache row for the printer whose error
|       count is to be returned.
|
|       Attribute "HIDDEN_CTX" has a string value that is the name of the
|       printer by convention with "Gen_HrPrinter_Cache()" above.
|
|       "outvalue" is a pointer to an integer to receive the error count.
|
|  IMPLICIT INPUTS:
|
|       None.
|
|  OUTPUTS:
|
|     On Success:
|       Function returns TRUE and an error count for the specified printer.
|
|     On any Failure:
|       Function returns FALSE.
|
|  THE BIG PICTURE:
|
|       For an hrDevice attribute whose value is "computed", at the time of 
|       the request to see it is received, we dispatch to a "COMPUTE_" function
|       to get the value.  This is such a routine for printers.
|
|  OTHER THINGS TO KNOW:
|
|       This function holds a convention with the Gen_cache routines earlier in
|       this module that the "HIDDEN_CTX" attribute for printers contains
|       a string that can be used in OpenPrinter to get a handle to that
|       printer.
*/
{

/*
| No way to get any error counts under NT 
*/
*outvalue = 0;
return ( TRUE );

}

/* COMPUTE_hrPrinter_status - Compute "hrDeviceStatus" for a Printer device */
/* COMPUTE_hrPrinter_status - Compute "hrDeviceStatus" for a Printer device */
/* COMPUTE_hrPrinter_status - Compute "hrDeviceStatus" for a Printer device */

BOOL
COMPUTE_hrPrinter_status(
                         CACHEROW *row,
                         UINT     *outvalue
                         )

/*
|  EXPLICIT INPUTS:
|
|       "row" points to the hrDevice cache row for the printer whose status
|       is to be returned.
|
|       Attribute "HIDDEN_CTX" has a string value that is the name of the
|       printer by convention with "Gen_HrPrinter_Cache()" above.
|
|       "outvalue" is a pointer to an integer to receive the status.
|
|  IMPLICIT INPUTS:
|
|       None.
|
|  OUTPUTS:
|
|     On Success:
|       Function returns TRUE and a status for the specified printer:
|
|               "unknown" = 1   If we can't open the printer at all.
|
|               "running" = 2   If we can open the printer and no status is
|                               showing on it.
|               "warning" = 3   If we can open the printer but PAUSED or
|                               PENDING_DELETION is showing on it.
|
|
|     On any Failure:
|       Function returns FALSE.
|
|  THE BIG PICTURE:
|
|       For an hrDevice attribute whose value is "computed", at the time of 
|       the request to see it is received, we dispatch to a "COMPUTE_" function
|       to get the value.  This is such a routine for printers.
|
|  OTHER THINGS TO KNOW:
|
|       This function holds a convention with the Gen_cache routines earlier in
|       this module that the "HIDDEN_CTX" attribute for printers contains
|       a string that can be used in OpenPrinter to get a handle to that
|       printer.
*/
{
CHAR    temp[8];                /* Temporary buffer for first call         */
HANDLE  hprinter;               /* Handle to a printer */


/* Open Printer whose name is in "Hidden Context"  to get a handle */
if (OpenPrinter(row->attrib_list[HIDDEN_CTX].u.string_value,    // Printer Name
                &hprinter,                      // Receive handle here
                NULL                            // Security
                ) == TRUE ) {

    PRINTER_INFO_2     *p2;
    DWORD              P2_request_len; /* Bytes-needed by GetPrinter */


    /*
    | Printer is Open, get a PRINTER_INFO_2 "slug-o-data"
    |
    | 1st call: Fails, get buffer size needed.
    */
    GetPrinter(hprinter,        // printer handle
               2,               // Level 2
               temp,            // Buffer for INFO_2
               1,               // Buffer-too-small
               &P2_request_len  // What we really need
               );

    /*
    | Grab enough storage for the PRINTER_INFO_2 structure
    */
    if ( (p2 = malloc(P2_request_len)) == NULL) {

        /* Storage Request failed altogether */
        ClosePrinter( hprinter );
        return ( FALSE );
        }

    /*
    | 2nd call: Should succeed.
    */
    if (GetPrinter(hprinter,        // printer handle
                   2,               // Level 2
             (unsigned char *) p2,  // Buffer for INFO_2
                   P2_request_len,  // Buffer-just-right
                   &P2_request_len  // What we really need
                   ) == TRUE) {

        /*
        | As of this writing, only two status values are available
        | under NT:
        |
        |       PRINTER_STATUS_PAUSED
        |       PRINTER_STATUS_PENDING_DELETION
        |
        | Basically, if either of these is TRUE, we'll signal "warning".
        | If neither are TRUE, we'll signal "running" (on the basis that
        | we've managed to open the printer OK and it shows no status).
        */
        if (   (p2->Status & PRINTER_STATUS_PAUSED)
            || (p2->Status & PRINTER_STATUS_PENDING_DELETION)) {

            *outvalue = 3;      // "warning"
            }
        else {
            *outvalue = 2;      // "running"
            }
        }

    /* Free up and return */
    ClosePrinter( hprinter );
    free( p2 );
    }
else {
    *outvalue = 1;      // "unknown"
    }

return ( TRUE );
}

