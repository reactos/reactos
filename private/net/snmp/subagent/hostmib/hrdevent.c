/*
 *  HrDeviceEntry.c v0.10
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
 *    instance name routines for the HrDeviceEntry.  Actual instrumentation code is
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
 *    V1.00 - 04/27/97  D. D. Burns     Genned: Thu Nov 07 16:41:55 1996
 *
 *
 */


#include <windows.h>
#include <malloc.h>

#include <snmp.h>

#include "mib.h"
#include "smint.h"
#include "hostmsmi.h"
#include "user.h"         /* Developer supplied include file     */
#include "HMCACHE.H"      /* Cache-related definitions           */
#include "HRDEVENT.H"     /* HrDevice Table  related definitions */
#include <stdio.h>        /* for sprintf                         */


/*
|==============================================================================
| Function prototypes for this module.
|
*/
/* Gen_SingleDevices - Generate Single Device row entries in HrDevice */
BOOL
Gen_SingleDevices( void );


#if defined(CACHE_DUMP)

/* debug_print_hrdevice - Prints a Row from HrDevice */
static void
debug_print_hrdevice(
                     CACHEROW     *row  /* Row in hrDiskStorage table */
                     );
#endif


/*
|==============================================================================
| Create the list-head for the HrDevice table cache.
|
| This list-head is globally accessible so the logic that loads the "sub" 
| tables can scan this cache for matches (among other reasons).
|
| (This macro is defined in "HMCACHE.H").
*/
CACHEHEAD_INSTANCE(hrDevice_cache, debug_print_hrdevice);


/*
|==============================================================================
| Initial Load Device
|
| This number is the index into hrDevice table for the entry that corresponds
| to the disk from which the system was initially loaded.
|
| This static location serves as "cache" for the value of 
| "HrSystemInitialLoadDevice" (serviced by code in "HRSYSTEM.C").
|
| It is initialized by function "Gen_Fixed_disks()" in module "HRDISKST.C"
| which is called by way of "Gen_HrDiskStorage_Cache()" invoked from this
| module.  It is during the scan of the fixed disks that we discover from which
| one the system booted.
*/
ULONG   InitLoadDev_index=0;


/*
 *  GetHrDeviceIndex
 *    A unique value for each device contained by the host.  The value for 
 *    each device must remain constant at least from one re-initi
 *    
 *    Gets the value for HrDeviceIndex.
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
 | hrDeviceIndex
 | 
 |  ACCESS         SYNTAX
 |  read-only      INTEGER (1..2147483647)
 | 
 | "A unique value for each device contained by the host.  The value for each
 | device must remain constant at least from one re-initialization of the agent
 | to the next re-initialization."
 | 
 | DISCUSSION:
 | 
 | As mentioned in the discussion for this table, all entries for this table
 | are derived from a local cache built at start-up time.  As a consequence the
 | maximum value of this attribute is fixed at SNMP service start-time.
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.2.1.1.<instance>
 |                | | | |
 |                | | | *hrDeviceIndex
 |                | | *hrDeviceEntry
 |                | *hrDeviceTable
 |                *-hrDevice
 */

UINT
GetHrDeviceIndex( 
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
| Return the "hrDeviceIndex" value from this entry
*/
*outvalue = row->attrib_list[HRDV_INDEX].u.unumber_value;


return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrDeviceIndex() */


/*
 *  GetHrDeviceType
 *    An indication of the type of device.
 *    
 *    Gets the value for HrDeviceType.
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
 | hrDeviceType
 | 
 |  ACCESS         SYNTAX
 |  read-only      OBJECT IDENTIFIER
 | 
 |   "An indication of the type of device.
 | 
 |   If this value is `hrDeviceProcessor { hrDeviceTypes3 }'
 |   then an entry exists in the hrProcessorTable
 |   which corresponds to this device.
 | 
 |   If this value is `hrDeviceNetwork { hrDeviceTypes 4}',
 |   then an entry exists in the hrNetworkTable
 |   which corresponds to this device.
 | 
 |   If this value is `hrDevicePrinter { hrDeviceTypes 5}',
 |   then an entry exists in the hrPrinterTable
 |   which corresponds to this device.
 | 
 |   If this value is `hrDeviceDiskStorage {hrDeviceTypes 6 }',
 |   then an entry exists in the
 |   hrDiskStorageTable which corresponds to this
 |   device."
 | 
 | DISCUSSION:
 | 
 | The list of registered device types (i.e. values that can be used in the
 | hrDeviceType attribute) are:
 | 
 |    hrDeviceOther             OBJECT IDENTIFIER ::= { hrDeviceTypes 1 }
 |    hrDeviceUnknown           OBJECT IDENTIFIER ::= { hrDeviceTypes 2 }
 |    hrDeviceProcessor         OBJECT IDENTIFIER ::= { hrDeviceTypes 3 }
 |    hrDeviceNetwork           OBJECT IDENTIFIER ::= { hrDeviceTypes 4 }
 |    hrDevicePrinter           OBJECT IDENTIFIER ::= { hrDeviceTypes 5 }
 |    hrDeviceDiskStorage       OBJECT IDENTIFIER ::= { hrDeviceTypes 6 }
 |    hrDeviceVideo             OBJECT IDENTIFIER ::= { hrDeviceTypes 10 }
 |    hrDeviceAudio             OBJECT IDENTIFIER ::= { hrDeviceTypes 11 }
 |    hrDeviceCoprocessor       OBJECT IDENTIFIER ::= { hrDeviceTypes 12 }
 |    hrDeviceKeyboard          OBJECT IDENTIFIER ::= { hrDeviceTypes 13 }
 |    hrDeviceModem             OBJECT IDENTIFIER ::= { hrDeviceTypes 14 }
 |    hrDeviceParallelPort      OBJECT IDENTIFIER ::= { hrDeviceTypes 15 }
 |    hrDevicePointing          OBJECT IDENTIFIER ::= { hrDeviceTypes 16 }
 |    hrDeviceSerialPort        OBJECT IDENTIFIER ::= { hrDeviceTypes 17 }
 |    hrDeviceTape              OBJECT IDENTIFIER ::= { hrDeviceTypes 18 }
 |    hrDeviceClock             OBJECT IDENTIFIER ::= { hrDeviceTypes 19 }
 |    hrDeviceVolatileMemory    OBJECT IDENTIFIER ::= { hrDeviceTypes 20 }
 |    hrDeviceNonVolatileMemory OBJECT IDENTIFIER ::= { hrDeviceTypes 21 }
 | 
 | (See discussion above for hrDeviceTable).
 |============================================================================
 | 1.3.6.1.2.1.25.3.1.n
 |                | | |
 |                | | * Identifying arc for type
 |                | *-hrDeviceTypes (OIDs specifying device types)
 |                *-hrDevice
 |
 | 1.3.6.1.2.1.25.3.2.1.2.<instance>
 |                | | | |
 |                | | | *-hrDeviceType
 |                | | *-hrDeviceEntry
 |                | *-hrDeviceTable
 |                *-hrDevice
 */

UINT
GetHrDeviceType( 
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
if ((row = FindTableRow(index, &hrDevice_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| By convention with the cache-building function "Gen_HrDevice_Cache()",
| and it's minions the cached value is the right-most arc we must return
| as the value.
|
| Hence whatever cache entry we retrieve, we tack the number retrieved
| from the cache for this attribute onto { hrDeviceType ... }.
*/
if ( (outvalue->ids = SNMP_malloc(10 * sizeof( UINT ))) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }
outvalue->idLength = 10;


/*
| Load in the full hrDeviceType OID:
|
| 1.3.6.1.2.1.25.3.1.n
|                | | |
|                | | * Identifying arc for type
|                | *-hrDeviceTypes (OIDs specifying device types)
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
outvalue->ids[8] = 1;

/* Cached Device Type indicator */
outvalue->ids[9] = row->attrib_list[HRDV_TYPE].u.unumber_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrDeviceType() */


/*
 *  GetHrDeviceDesc
 *    A textual description of this device, including the device's 
 *    manufacturer and revision, and optionally, its serial number.
 *    
 *    Gets the value for HrDeviceDesc.
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
 | hrDeviceDescr
 | 
 |  ACCESS         SYNTAX
 |  read-only      DisplayString (SIZE (0..64))
 | 
 | "A textual description of this device, including the device's manufacturer and
 | revision, and optionally, its serial number."
 | 
 | DISCUSSION:
 | 
 | (See discussion above for hrDeviceTable, the information source for this
 |  attribute depends on the device type).
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.2.1.3.<instance>
 |                | | | |
 |                | | | *-hrDeviceDescr
 |                | | *-hrDeviceEntry
 |                | *-hrDeviceTable
 |                *-hrDevice
 */

UINT
GetHrDeviceDesc( 
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
if ((row = FindTableRow(index, &hrDevice_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

outvalue->string = row->attrib_list[HRDV_DESCR].u.string_value;

/* "Truncate" here to meet RFC as needed*/
if ((outvalue->length = strlen(outvalue->string)) > 64) {
    outvalue->length = 64;
    }


return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrDeviceDesc() */


/*
 *  GetHrDeviceID
 *    The product ID for this device.
 *    
 *    Gets the value for HrDeviceID.
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
 | hrDeviceID
 | 
 |  ACCESS         SYNTAX
 |  read-only      ProductID
 | 
 | "The product ID for this device."
 | 
 |    ProductID ::= OBJECT IDENTIFIER
 | 
 |    "unknownProduct will be used for any unknown ProductID:
 |     unknownProduct OBJECT IDENTIFIER ::= { 0 0 }
 | 
 | DISCUSSION:
 | 
 | <POA-10> I anticipate always using "unknownProduct" as the value for this
 | attribute, as I can envision no systematic means of acquiring a registered
 | OID for all devices to be used as the value for this attribute.
 | 
 | RESOLVED >>>>>>>>
 | <POA-10> Returning an unknown Product ID is acceptable.
 | RESOLVED >>>>>>>>
 |============================================================================
 | 1.3.6.1.2.1.25.3.2.1.4.<instance>
 |                | | | |
 |                | | | *-hrDeviceID
 |                | | *-hrDeviceEntry
 |                | *-hrDeviceTable
 |                *-hrDevice
 */

UINT
GetHrDeviceID( 
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

} /* end of GetHrDeviceID() */


/*
 *  GetHrDeviceStatus
 *    The current operational state of the device described by this row of the 
 *    table.  A value unknown (1) indicates that the current 
 *    
 *    Gets the value for HrDeviceStatus.
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
 | hrDeviceStatus
 | 
 |  ACCESS         SYNTAX
 |  read-only      INTEGER {unknown(1),running(2),warning(3),testing(4),
 |                          down(5)}
 | 
 | "The current operational state of the device described by this row of the
 | table.  A value unknown(1) indicates that the current state of the device is
 | unknown.  running(2) indicates that the device is up and running and that no
 | unusual error conditions are known.  The warning(3) state indicates that agent
 | has been informed of an unusual error condition by the operational software
 | (e.g., a disk device driver) but that the device is still 'operational'.  An
 | example would be high number of soft errors on a disk.  A value of testing(4),
 | indicates that the device is not available for use because it is in the
 | testing state.  The state of down(5) is used only when the agent has been
 | informed that the device is not available for any use."
 | 
 | DISCUSSION:
 | 
 | For those devices for which a driver can be queried for the device status,
 | this is done.  For all other circumstances, "unknown(1)" is returned.
 | 
 | (See discussion above for hrDeviceTable, the information source for this
 |  attribute depends on the device type).
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.2.1.5.<instance>
 |                | | | |
 |                | | | *-hrDeviceStatus
 |                | | *-hrDeviceEntry
 |                | *-hrDeviceTable
 |                *-hrDevice
 */

UINT
GetHrDeviceStatus( 
        OUT INThrDeviceStatus *outvalue ,
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
| By convention with the cache-building function "Gen_HrDevice_Cache()",
| and its minions the cached value in the row just fetched above for
| the "hrDeviceType" attribute (indexed by our symbol "HRDV_TYPE") is the
| last arc in the OID that specifies the type of device for which we need
| to return status/errors.
|
| The scheme for returning status depends on the value of the type to
| dispatch to the correct code to handle that kind of device.
|
| Also, code that initializes the hrDevice cache for any given device
| has the option of storing in the "hidden context" attribute (accessed via 
| our symbol "HIDDEN_CTX") information needed to access that device.
| 
| For instance, the code that enters Printer devices into hrDevice (in
| function "Gen_HrPrinter_Cache()" in "HRPRINTE.C") stores a string in
| "HIDDEN_CTX" that is the printer name thereby allowing logic below to 
| re-open that printer to obtain status/errors.
|
*/
switch ( row->attrib_list[HRDV_TYPE].u.unumber_value ) {

    case HRDV_TYPE_LASTARC_PRINTER:
        /*  (In "HRPRINTE.C") */
        if (!COMPUTE_hrPrinter_status(row, (UINT *) outvalue)) {
            return SNMP_ERRORSTATUS_GENERR;
            }
        break;


    case HRDV_TYPE_LASTARC_PROCESSOR:
        /* Any processor in the table is running */
        *outvalue = 2;
        break;


    case HRDV_TYPE_LASTARC_DISKSTORAGE:
        /* Stored by Gen_hrDiskStorage_cache() */
        *outvalue =  row->attrib_list[HRDV_STATUS].u.unumber_value;
        break;


    case HRDV_TYPE_LASTARC_KEYBOARD:
    case HRDV_TYPE_LASTARC_POINTING:
        /* Any Keyboard or Mouse in the table is reasably presumed "running" */
        *outvalue = 2;  // "running"
        break;


    case HRDV_TYPE_LASTARC_PARALLELPORT:
    case HRDV_TYPE_LASTARC_SERIALPORT:
        *outvalue = 1;  // "Unknown"
        break;


    case HRDV_TYPE_LASTARC_OTHER:
    case HRDV_TYPE_LASTARC_UNKNOWN:
    case HRDV_TYPE_LASTARC_NETWORK:
    case HRDV_TYPE_LASTARC_VIDEO:
    case HRDV_TYPE_LASTARC_AUDIO:
    case HRDV_TYPE_LASTARC_COPROCESSOR:
    case HRDV_TYPE_LASTARC_MODEM:
    case HRDV_TYPE_LASTARC_TAPE:
    case HRDV_TYPE_LASTARC_CLOCK:
    case HRDV_TYPE_LASTARC_VOLMEMORY:
    case HRDV_TYPE_LASTARC_NONVOLMEMORY:

        *outvalue = 1;  // "Unknown"
        break;


    default:
        return SNMP_ERRORSTATUS_GENERR;

    }

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrDeviceStatus() */


/*
 *  GetHrDeviceErrors
 *    The number of errors detected on this device.  It should be noted that as 
 *    this object has a SYNTAX of Counter, that it does not 
 *    
 *    Gets the value for HrDeviceErrors.
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
 | hrDeviceErrors
 |  ACCESS         SYNTAX
 |  read-only      Counter
 | 
 | "The number of errors detected on this device.  It should be noted that as
 | this object has a SYNTAX of Counter, that it does not have a defined initial
 | value.  However, it is recommended that this object be initialized to zero."
 | 
 | 
 | DISCUSSION:
 | 
 | For those devices for which a driver can be queried for the device errors,
 | this is done.  For all other circumstances, "0" is returned.
 | 
 | (See discussion above for hrDeviceTable, the information source for this
 |  attribute depends on the device type).
 | 
 |============================================================================
 | 1.3.6.1.2.1.25.3.2.1.6.<instance>
 |                | | | |
 |                | | | *-hrDeviceErrors
 |                | | *-hrDeviceEntry
 |                | *-hrDeviceTable
 |                *-hrDevice
 */

UINT
GetHrDeviceErrors( 
        OUT Counter *outvalue ,
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
| By convention with the cache-building function "Gen_HrDevice_Cache()",
| and its minions the cached value in the row just fetched above for
| the "hrDeviceType" attribute (indexed by our symbol "HRDV_TYPE") is the
| last arc in the OID that specifies the type of device for which we need
| to return status/errors.
|
| The scheme for returning status depends on the value of the type to
| dispatch to the correct code to handle that kind of device.
|
| Also, code that initializes the hrDevice cache for any given device
| has the option of storing in the "hidden context" attribute (accessed via 
| our symbol "HIDDEN_CTX") information needed to access that device.
| 
| For instance, the code that enters Printer devices into hrDevice (in
| function "Gen_HrPrinter_Cache()" in "HRPRINTE.C") stores a string in
| "HIDDEN_CTX" that is the printer name thereby allowing logic below to 
| re-open that printer to obtain status/errors.
|
*/
switch ( row->attrib_list[HRDV_TYPE].u.unumber_value ) {

    case HRDV_TYPE_LASTARC_PRINTER:

        /*  (In "HRPRINTE.C") */
        if (!COMPUTE_hrPrinter_errors(row, outvalue)) {
            return SNMP_ERRORSTATUS_GENERR;
            }
        break;


    case HRDV_TYPE_LASTARC_PROCESSOR:

        /* If 'errors' ain't 0, odds are low you're gonna find out via SNMP */
        *outvalue = 0;
        break;


    case HRDV_TYPE_LASTARC_POINTING:
    case HRDV_TYPE_LASTARC_KEYBOARD:
    case HRDV_TYPE_LASTARC_PARALLELPORT:
    case HRDV_TYPE_LASTARC_SERIALPORT:
        /* 'errors' presumed 0 */
        *outvalue = 0;
        break;


    case HRDV_TYPE_LASTARC_OTHER:
    case HRDV_TYPE_LASTARC_UNKNOWN:
    case HRDV_TYPE_LASTARC_NETWORK:
    case HRDV_TYPE_LASTARC_DISKSTORAGE:
    case HRDV_TYPE_LASTARC_VIDEO:
    case HRDV_TYPE_LASTARC_AUDIO:
    case HRDV_TYPE_LASTARC_COPROCESSOR:
    case HRDV_TYPE_LASTARC_MODEM:
    case HRDV_TYPE_LASTARC_TAPE:
    case HRDV_TYPE_LASTARC_CLOCK:
    case HRDV_TYPE_LASTARC_VOLMEMORY:
    case HRDV_TYPE_LASTARC_NONVOLMEMORY:
        *outvalue = 0;
        break;

    default:
        return SNMP_ERRORSTATUS_GENERR;

    }

return SNMP_ERRORSTATUS_NOERROR ;
} /* end of GetHrDeviceErrors() */


/*
 *  HrDeviceEntryFindInstance
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
HrDeviceEntryFindInstance( IN ObjectIdentifier *FullOid ,
                       IN OUT ObjectIdentifier *instance )
{
    UINT tmp_instance ;

    //
    //  Developer instrumentation code to find appropriate instance goes here.
    //  For non-tables, it is not necessary to modify this routine.  However, if
    //  there is any context that needs to be set, it can be done here.
    //

    if ( FullOid->idLength <= HRDEVICEENTRY_VAR_INDEX )
	// No instance was specified
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else  if ( FullOid->idLength != HRDEVICEENTRY_VAR_INDEX + 1 )
	// Instance length is more than 1
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else
	// The only valid instance for a non-table are instance 0.  If this
	// is a non-table, the following code validates the instances.  If this
	// is a table, developer modification is necessary below.

	tmp_instance = FullOid->ids[ HRDEVICEENTRY_VAR_INDEX ] ;

        /*
        | For hrDeviceTable, the instance arc(s) is a single arc, and it must
        | correctly select an entry in the hrDeviceTable cache.
        | Check that here.
        */
	if ( FindTableRow(tmp_instance, &hrDevice_cache) == NULL ) {
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

} /* end of HrDeviceEntryFindInstance() */



/*
 *  HrDeviceEntryFindNextInstance
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
HrDeviceEntryFindNextInstance( IN ObjectIdentifier *FullOid ,
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


    if ( FullOid->idLength <= HRDEVICEENTRY_VAR_INDEX )
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
        tmp_instance = FullOid->ids[ HRDEVICEENTRY_VAR_INDEX ] ;
        }

    /* Now go off and try to find the next instance in the table */
    if ((row = FindNextTableRow(tmp_instance, &hrDevice_cache)) == NULL) {
        return SNMP_ERRORSTATUS_NOSUCHNAME ;
        }

    instance->ids[ 0 ] = row->index ;
    instance->idLength = 1 ;

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of HrDeviceEntryFindNextInstance() */



/*
 *  HrDeviceEntryConvertInstance
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
HrDeviceEntryConvertInstance( IN ObjectIdentifier *oid_spec ,
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

} /* end of HrDeviceEntryConvertInstance() */




/*
 *  HrDeviceEntryFreeInstance
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
HrDeviceEntryFreeInstance( IN OUT InstanceName *instance )
{

  /* No action needed for hrDevice Table */
} /* end of HrDeviceEntryFreeInstance() */


/*
| End of Generated Code
*/

/* Gen_HrDevice_Cache - Generate a initial cache for HrDevice Table */
/* Gen_HrDevice_Cache - Generate a initial cache for HrDevice Table */
/* Gen_HrDevice_Cache - Generate a initial cache for HrDevice Table */

BOOL
Gen_HrDevice_Cache(
                    void
                    )

/*
|  EXPLICIT INPUTS:
|
|       None.
|
|  IMPLICIT INPUTS:
|
|       The module-local head of the cache for the HrDevice table,
|       "HrDevice_cache".
|
|  OUTPUTS:
|
|     On Success:
|       Function returns TRUE indicating that the cache has been fully
|       populated with all "static" cache-able values.
|
|     On any Failure:
|       Function returns FALSE (indicating "not enough storage" or other
|       internal logic error).
|
|  THE BIG PICTURE:
|
|       At subagent startup time, the cache for each table in the MIB is
|       populated with rows for each row in the table.  This function is
|       invoked by the start-up code in "UserMibInit()" ("MIB.C") to 
|       populate the cache for the HrDevice table.
|
|  OTHER THINGS TO KNOW:
|
|       There is one of these function for every table that has a cache.
|       Each is found in the respective source file.
|
|=============== From WebEnable Design Spec Rev 3 04/11/97==================
| A Row in the hrDeviceTable
| 
| "A (conceptual) entry for one device contained by the host.  As an example, an
| instance of the hrDeviceType object might be named hrDeviceType.3"
| 
|    HrDeviceEntry ::= SEQUENCE {
|            hrDeviceIndex           INTEGER,
|            hrDeviceType            OBJECT IDENTIFIER,
|            hrDeviceDescr           DisplayString,
|            hrDeviceID              ProductID,
|            hrDeviceStatus          INTEGER,
|            hrDeviceErrors          Counter
|        }
| 
| DISCUSSION:
| 
| This is the largest and most complicated table to populate.  The strategy for
| populating entries in this table is to execute a slug of code for each device
| type (in the list below) in an attempt to find all instances of that device
| type.  For some devices, the code uses standard Win32 API functions, for
| others it is clear that special-purpose code is needed to extract the relevant
| information from the "behind the scenes" (direct NT kernel inquiries).
| 
| This table is fully populated with respect to the other tables in the hrDevice
| group. The other tables are sparse tables augmenting only selected entries in
| hrDeviceTable.
| 
| The list of registered device types (i.e. values that can be used in the
| hrDeviceType attribute) are:
| 
|    hrDeviceOther             OBJECT IDENTIFIER ::= { hrDeviceTypes 1 }
|    hrDeviceUnknown           OBJECT IDENTIFIER ::= { hrDeviceTypes 2 }
|    hrDeviceProcessor         OBJECT IDENTIFIER ::= { hrDeviceTypes 3 }
|    hrDeviceNetwork           OBJECT IDENTIFIER ::= { hrDeviceTypes 4 }
|    hrDevicePrinter           OBJECT IDENTIFIER ::= { hrDeviceTypes 5 }
|    hrDeviceDiskStorage       OBJECT IDENTIFIER ::= { hrDeviceTypes 6 }
|    hrDeviceVideo             OBJECT IDENTIFIER ::= { hrDeviceTypes 10 }
|    hrDeviceAudio             OBJECT IDENTIFIER ::= { hrDeviceTypes 11 }
|    hrDeviceCoprocessor       OBJECT IDENTIFIER ::= { hrDeviceTypes 12 }
|    hrDeviceKeyboard          OBJECT IDENTIFIER ::= { hrDeviceTypes 13 }
|    hrDeviceModem             OBJECT IDENTIFIER ::= { hrDeviceTypes 14 }
|    hrDeviceParallelPort      OBJECT IDENTIFIER ::= { hrDeviceTypes 15 }
|    hrDevicePointing          OBJECT IDENTIFIER ::= { hrDeviceTypes 16 }
|    hrDeviceSerialPort        OBJECT IDENTIFIER ::= { hrDeviceTypes 17 }
|    hrDeviceTape              OBJECT IDENTIFIER ::= { hrDeviceTypes 18 }
|    hrDeviceClock             OBJECT IDENTIFIER ::= { hrDeviceTypes 19 }
|    hrDeviceVolatileMemory    OBJECT IDENTIFIER ::= { hrDeviceTypes 20 }
|    hrDeviceNonVolatileMemory OBJECT IDENTIFIER ::= { hrDeviceTypes 21 }
| 
| All of the foregoing types can be divided into two groups based on the
| approach needed to acquire information about them.  Information for the first
| group can be queried using Win32 API functions while the second group 
| requires special inquiry-code.
| 
| (1) Win32 Device-Types  Win32 Function Used
| ----------------------  -------------------
|    hrDeviceOther
|         Partitions      DeviceIoControl (IOCTL_GET_PARTITION_INFO)
| 
|    hrDeviceProcessor    GetSystemInfo
|    hrDevicePrinter      EnumPrinterDrivers
|    hrDeviceDiskStorage  QueryDosDevice/CreateFile (using physical drive access)
|    hrDeviceKeyboard     GetKeyboardType
|    hrDevicePointing     (Win32 function provides pointer-device button-count)
|
| 
| (2) Special-Inquiry Device-Types
| ---------------------------------
|    hrDeviceNetwork      Access is provided via special "mib2util" DLL
| 
|    hrDeviceParallelPort NtQuerySystemInformation(SYSTEM_DEVICE_INFORMATION)
|    hrDeviceSerialPort
| 
|    hrDeviceVideo        ??? NtQuerySystemInformation(SYSTEM_GDI_DRIVER_INFORMATION)
|    hrDeviceAudio        ???
|    hrDeviceTape         ???
| 
| 
| The following "devices" do not readily fall into either of the above groups
| and no attempt is made to recognize them:
| 
|    hrDeviceModem             OBJECT IDENTIFIER ::= { hrDeviceTypes 14 }
|    hrDeviceVolatileMemory    OBJECT IDENTIFIER ::= { hrDeviceTypes 20 }
|    hrDeviceNonVolatileMemory OBJECT IDENTIFIER ::= { hrDeviceTypes 21 }
|    hrDeviceCoprocessor       OBJECT IDENTIFIER ::= { hrDeviceTypes 12 }
|    hrDeviceClock             OBJECT IDENTIFIER ::= { hrDeviceTypes 19 }
| 
| 
| Other Implementation Details
| ----------------------------
| The bulk of the information for this table (and the associated sparse tables
| hrProcessorTable, hrNetworkTable, hrPrinterTable, hrDiskStorageTable,
| hrPartitionTable and hrFSTable) is acquired and stored in a local cache at the
| time the SNMP agent enrolls the DLL for the Host Resources MIB.  This strategy
| is designed to reduce the hit on system resources when requests are processed.
| The only information that is acquired dynamically (on a per SNMP request) is
| information for variables likely to be dynamic: Status and Error status.
| 
| One consequence of this strategy is that user-implemented changes to the
| system configuration (including changing printer drivers, or disk partition
| layout) will not be reported until the SNMP service is restarted.
| 
| 
|============================================================================
| 1.3.6.1.2.1.25.3.1.n
|                | | |
|                | | * Identifying arc for type
|                | *-hrDeviceTypes (OIDs specifying device types)
|                *-hrDevice
|
| 1.3.6.1.2.1.25.3.2.1....
|                | | |
|                | | *hrDeviceEntry
|                | *hrDeviceTable
|                *-hrDevice
|
*/
{


/*
|============================================================================
|
| Call the "Gen_*_cache()" functions for each of the "sub-tables" within
| the hrDevice table.
|
| Each of these functions is responsible for:
|
|       + populating the hrDevice cache with however many rows are called for
|         given the device(s) available
|
|       + creating and populating their own cache for the sub-table if
|         the sub-table needs a cache.  (If all the sub-table attributes
|         are "computed" on request, then there is no need for a separate 
|         sub-table cache).
*/

if (Gen_HrPrinter_Cache(HRDV_TYPE_LASTARC_PRINTER) == FALSE) {
    return ( FALSE );
    }

if (Gen_HrProcessor_Cache(HRDV_TYPE_LASTARC_PROCESSOR) == FALSE) {
    return ( FALSE );
    }

if (Gen_HrNetwork_Cache(HRDV_TYPE_LASTARC_NETWORK) == FALSE) {
    return ( FALSE );
    }

if (Gen_HrDiskStorage_Cache(HRDV_TYPE_LASTARC_DISKSTORAGE) == FALSE) {
    return ( FALSE );
    }


/*
|============================================================================
|
| Now handle the odd "one-off" devices for which potentially just a single 
| entry is made into the existing hrDevice table cache.
*/
if (Gen_SingleDevices() == FALSE) {
    return ( FALSE );
    }

#if defined(CACHE_DUMP)
PrintCache(&hrDevice_cache);
PrintCache(&hrDiskStorage_cache);
#endif


/*
| HrDevice cache generation complete.
*/
return ( TRUE );
}


/* Gen_SingleDevices - Generate Single Device row entries in HrDevice */
/* Gen_SingleDevices - Generate Single Device row entries in HrDevice */
/* Gen_SingleDevices - Generate Single Device row entries in HrDevice */

BOOL
Gen_SingleDevices( void )

/*
|  EXPLICIT INPUTS:
|
|       None.
|
|  IMPLICIT INPUTS:
|
|       None.
|
|  OUTPUTS:
|
|     On Success:
|       Function creates a new row entry populated with all "static" cache-able
|       values for HrDevice table for each "one-off" device and returns TRUE.
|
|     On any Failure:
|       Function returns FALSE (indicating "not enough storage" or other
|       internal logic error).
|
|
|  THE BIG PICTURE:
|
|       At subagent startup time, the cache for each table in the MIB is
|       being populated.  This function handles populating hrDevice with
|       a row for each "single-type" device (such as keyboard).
|
|  OTHER THINGS TO KNOW:
|
|       Devices being added by this function are not associated with
|       sub-tables.
|
|       We handle:
|         + Keyboard "device"
|         + Pointing "device"
|         + Parallel and Serial Port "devices"
|
*/
{
UINT    key_status;             /* Value returned from GetKeyboardType()    */
UINT    button_count;           /* Mouse button count from GetSystemMetrics */
CHAR    msg[256];               /* (Big enough for constant strings below)  */
UINT    dev_number;

#define PHYS_SIZE 64
CHAR    phys_name[PHYS_SIZE];   /* Buffer where a string like "\\.C:" (for */
                                /*  example) is built for drive access.    */
HANDLE                  hdrv;   /* Handle to device                        */


/*
|==============================================================================
| Keyboard Device
|
| If we can get keyboard type... */
if ((key_status = GetKeyboardType(0)) != 0 ) {

    PCHAR       key_type;              /* Description string             */

    /* Select initial part of description string */
    switch (key_status) {
        case 1: key_type = "IBM PC/XT or compatible (83-key) keyboard"; break;
        case 2: key_type = "Olivetti \"ICO\" (102-key) keyboard"; break;
        case 3: key_type = "IBM PC/AT (84-key) or similar keyboard"; break;
        case 4: key_type = "IBM enhanced (101- or 102-key) keyboard"; break;
        case 5: key_type = "Nokia 1050 and similar keyboards"; break;
        case 6: key_type = "Nokia 9140 and similar keyboards"; break;
        case 7: key_type = "Japanese keyboard"; break;
        default: key_type = "Unknown keyboard"; break;
        }

    /* Build the full description string */
    sprintf(msg, "%s, Subtype=(%d)", key_type, GetKeyboardType(1));

    if (AddHrDeviceRow(HRDV_TYPE_LASTARC_KEYBOARD,      // Last Type OID arc
                       msg,                             // Description string
                       NULL,                            // No hidden context
                       CA_UNKNOWN) == NULL) {

        return ( FALSE );       /* Something blew */
        }
    }

/*
|==============================================================================
| Pointing Device
|
| If we can get Mouse Button count... */
if ((button_count = GetSystemMetrics(SM_CMOUSEBUTTONS)) != 0 ) {

    sprintf(msg, "%d-Buttons %s",
            button_count,
            (GetSystemMetrics(SM_MOUSEWHEELPRESENT)) ? " (with wheel)" : "");

 
    if (AddHrDeviceRow(HRDV_TYPE_LASTARC_POINTING,      // Last Type OID arc
                       msg,                             // Description string
                       NULL,                            // No hidden context
                       CA_UNKNOWN) == NULL) {

        return ( FALSE );       /* Something blew */
        }
    }


/*
|==============================================================================
| LPT Devices
|
| For every LPTx device we can open successfully. . .
*/
for (dev_number = 1; dev_number < 4; dev_number += 1) {

    /* Build it for device n: */
    sprintf(phys_name, "LPT%d:", dev_number);

    /*
    | Suppress any attempt by the system to talk to the user
    */
    SetErrorMode(SEM_FAILCRITICALERRORS);

    /* Attempt to get a handle using this physical name string */
    if ((hdrv = CreateFile(phys_name,           // Device
                               GENERIC_READ,    // Access mode
                               FILE_SHARE_READ, // Share Mode
                               NULL,            // Security
                               OPEN_EXISTING,   // CreationDistribution
                               FILE_ATTRIBUTE_NORMAL,  // FlagsandAttributes
                               NULL             // Template file
                           )) != INVALID_HANDLE_VALUE) {

        /*
        | Ok, we managed to get ahold of it, we'll put it in the table.
        */
        if (AddHrDeviceRow(HRDV_TYPE_LASTARC_PARALLELPORT, // Last Type OID arc
                           phys_name,                      // Descr string
                           NULL,                           // No hidden context
                           CA_UNKNOWN) == NULL) {

            return ( FALSE );       /* Something blew */
            }

        CloseHandle(hdrv);
        }   /* if (we managed to "CreateFile" the device) */
    else {
        /*
        | Keep trucking if we couldn't open the device, but quit when we
        | hit this error.
        */
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            break;
            }
        }

    SetErrorMode(0);        /* Turn error suppression mode off */
    }   /* For each device */

/*
|==============================================================================
| COM Devices
|
| For every COMx device we can open successfully. . .
*/
for (dev_number = 1; dev_number <= 4; dev_number += 1) {

    /* Build it for device n: */
    sprintf(phys_name, "COM%d:", dev_number);

    /*
    | Suppress any attempt by the system to talk to the user
    */
    SetErrorMode(SEM_FAILCRITICALERRORS);

    /* Attempt to get a handle using this physical name string */
    if ((hdrv = CreateFile(phys_name,           // Device
                               GENERIC_READ,    // Access mode
                               FILE_SHARE_READ, // Share Mode
                               NULL,            // Security
                               OPEN_EXISTING,   // CreationDistribution
                               FILE_ATTRIBUTE_NORMAL,  // FlagsandAttributes
                               NULL             // Template file
                           )) != INVALID_HANDLE_VALUE) {

        /*
        | Ok, we managed to get ahold of it, we'll put it in the table.
        */
        if (AddHrDeviceRow(HRDV_TYPE_LASTARC_SERIALPORT,   // Last Type OID arc
                           phys_name,                      // Descr string
                           NULL,                           // No hidden context
                           CA_UNKNOWN) == NULL) {

            return ( FALSE );       /* Something blew */
            }

        CloseHandle(hdrv);
        }   /* if (we managed to "CreateFile" the device) */
    else {
        /*
        | Keep trucking if we couldn't open the device, but quit when we
        | hit this error (skip share failures).
        */
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            break;
            }
        }

    SetErrorMode(0);        /* Turn error suppression mode off */
    }   /* For each device */
    return ( TRUE );    //--ft:09/28-- sundown cleanup
}

/* AddHrDeviceRow - Generate another Row Entry in HrDevice Table */
/* AddHrDeviceRow - Generate another Row Entry in HrDevice Table */
/* AddHrDeviceRow - Generate another Row Entry in HrDevice Table */

CACHEROW *
AddHrDeviceRow(
               ULONG   type_arc,       /* Last Arc value for OID for Type   */
               LPSTR   descr,          /* Description string                */
               void   *hidden_ctx,     /* If non-NULL: Hidden-context value */
               ATTRIB_TYPE  hc_type    /* Type of "hidden_ctx"              */
               )

/*
|  EXPLICIT INPUTS:
|
|       "type_arc" is the number that is inserted as the right-most arc in
|       Object Identifier that is the cache entry for the hrDevicetype of the
|       device.
|
|       "descr" is a string pointer that is to be the cached value of
|       the hrDeviceDesc attribute.
|
|       "hidden_ctx" - If non-null, this is a pointer to the value to be
|       stored as the "Hidden Context" attribute in the new row.
|
|       "hc_type" is the type of "hidden_ctx" if hidden_ctx is non-null.
|
|
|  IMPLICIT INPUTS:
|
|       The module-local head of the cache for the HrDevice table,
|       "HrDevice_cache".
|
|  OUTPUTS:
|
|     On Success:
|       Function creates a new row entry populated with all "static" cache-able
|       values for HrDevice table and returns a pointer to the new row entry.
|
|     On any Failure:
|       Function returns NULL (indicating "not enough storage" or other
|       internal logic error).
|
|
|  THE BIG PICTURE:
|
|       At subagent startup time, the cache for each table in the MIB is
|       populated with rows for each row in the table.  This function is
|       invoked by the start-up code in each of the "sub-hrDevicetable"
|       files to populate the cache for the HrDevice table for the rows
|       pertaining to a particular sub-table (hrProcessorTable, hrPrinterTable
|       etc).
|
|  OTHER THINGS TO KNOW:
|
|  The "hidden_ctx" argument provides an easy way for the caller to stash
|  a value useful for later run-time reference.  For instance, "GET" functions
|  for Printer devices may need a string that identifies the printer (for
|  a given row-entry) in order to lookup the current value of an SNMP 
|  attribute (like the current status).  So the "Hidden Context" attribute
|  may be set to a string that can be submitted to a Win32 function to obtain
|  the current status for the printer.
*/
{
static                          /* NOTE: "static" is a 'must'              */
ULONG     table_index=0;        /* HrDeviceTable index counter             */

CACHEROW *row;                  /* --> Cache structure for row-being built */


/*
| OK, the caller wants another row in the table, get a row-entry created.
*/
if ((row = CreateTableRow( HRDV_ATTRIB_COUNT ) ) == NULL) {
    return ( NULL );       // Out of memory
    }

/*
| Set up the standard-hrDevice attributes in the new row
*/

/* =========== hrDeviceIndex ==========*/
row->attrib_list[HRDV_INDEX].attrib_type = CA_NUMBER;
row->attrib_list[HRDV_INDEX].u.unumber_value = (table_index += 1) ;


/* =========== hrDeviceType ==========
|
| Some GET functions for "computed" attributes expect to be able to use
| the value of the "hrDeviceType" cache value stored below to dispatch
| to appropriate code based on the device type (using the last type-OID arc
| as the "switch" value).
*/
row->attrib_list[HRDV_TYPE].attrib_type = CA_NUMBER;
row->attrib_list[HRDV_TYPE].u.unumber_value = type_arc ;


/* =========== hrDeviceDescr ==========*/
row->attrib_list[HRDV_DESCR].attrib_type = CA_STRING;
if ( (row->attrib_list[HRDV_DESCR].u.string_value
      = ( LPSTR ) malloc(strlen(descr) + 1)) == NULL) {
    return ( NULL );       /* out of memory */
    }
strcpy(row->attrib_list[HRDV_DESCR].u.string_value, descr);


/*
| The rest of the standard hrDevice attributes are "computed" at run time
*/

/* =========== hrDeviceStatus ==========*/
row->attrib_list[HRDV_STATUS].attrib_type = CA_COMPUTED;


/* =========== hrDeviceErrors ==========*/
row->attrib_list[HRDV_ERRORS].attrib_type = CA_COMPUTED;

/*
|================================================================
| If they gave us a hidden-context attribute string, store it now.
*/
if (hidden_ctx != NULL) {

    switch (hc_type) {

        case CA_STRING:
            row->attrib_list[HIDDEN_CTX].attrib_type = CA_STRING;
            if ( (row->attrib_list[HIDDEN_CTX].u.string_value
                  = ( LPSTR ) malloc(strlen((LPSTR)hidden_ctx) + 1)) == NULL) {
                return ( NULL );       /* out of memory */
                }
            strcpy(row->attrib_list[HIDDEN_CTX].u.string_value, hidden_ctx);
            break;

        case CA_NUMBER:
            row->attrib_list[HIDDEN_CTX].attrib_type = CA_NUMBER;
            row->attrib_list[HIDDEN_CTX].u.unumber_value =
                                                      *((ULONG *) hidden_ctx);
            break;

        case CA_CACHE:
            row->attrib_list[HIDDEN_CTX].attrib_type = CA_CACHE;
            row->attrib_list[HIDDEN_CTX].u.cache = (CACHEHEAD *) hidden_ctx;
            break;

        case CA_UNKNOWN:
            row->attrib_list[HIDDEN_CTX].attrib_type = CA_UNKNOWN;
            break;

        default:
            return ( NULL );    /* Something wrong */
        }
    }
else {
    /* Show no "Hidden-Context" attribute for this row */
    row->attrib_list[HIDDEN_CTX].attrib_type = CA_UNKNOWN;
    row->attrib_list[HIDDEN_CTX].u.string_value = NULL;
    }

/*
| Now insert the filled-in CACHEROW structure into the
| cache-list for the hrDeviceTable.
*/
if (AddTableRow(row->attrib_list[HRDV_INDEX].u.unumber_value,  /* Index */
                row,                                           /* Row   */
                &hrDevice_cache                                /* Cache */
                ) == FALSE) {
    return ( NULL );       /* Internal Logic Error! */
    }

/*
| Meet caller's expectation of receiving a pointer to the new row.
*/
return ( row );
}

#if defined(CACHE_DUMP)

/* debug_print_hrdevice - Prints a Row from HrDevice */
/* debug_print_hrdevice - Prints a Row from HrDevice */
/* debug_print_hrdevice - Prints a Row from HrDevice */

static void
debug_print_hrdevice(
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
char    *type;          /* String representation of device type */

if (row == NULL) {
    fprintf(OFILE, "====================\n");
    fprintf(OFILE, "hrDevice Table Cache\n");
    fprintf(OFILE, "====================\n");
    return;
    }

fprintf(OFILE, "hrDeviceIndex. . . . . . . %d\n",
        row->attrib_list[HRDV_INDEX].u.unumber_value);

switch (row->attrib_list[HRDV_TYPE].u.unumber_value) {

    case 1: type = "Other"; break;
    case 2: type = "Unknown"; break;
    case 3: type = "Processor"; break;
    case 4: type = "Network"; break;
    case 5: type = "Printer"; break;
    case 6: type = "DiskStorage"; break;
    case 10: type = "Video"; break;
    case 11: type = "Audio"; break;
    case 12: type = "Coprocessor"; break;
    case 13: type = "Keyboard"; break;
    case 14: type = "Modem"; break;
    case 15: type = "ParallelPort"; break;
    case 16: type = "Pointing"; break;
    case 17: type = "SerialPort"; break;
    case 18: type = "Tape"; break;
    case 19: type = "Clock"; break;
    case 20: type = "VolatileMemory"; break;
    case 21: type = "NonVolatileMemory"; break;
    default: type = "<Unknown!>"; break;
    }

fprintf(OFILE, "hrDeviceType . . . . . . . %d (%s)\n",
        row->attrib_list[HRDV_TYPE].u.unumber_value, type);

fprintf(OFILE, "hrDeviceDescr. . . . . . . %s\n",
        row->attrib_list[HRDV_DESCR].u.string_value);

fprintf(OFILE, "hrDeviceStatus . . . . . . ");
switch (row->attrib_list[HRDV_STATUS].attrib_type) {

    case CA_STRING:
        fprintf(OFILE, "CA_STRING: \"%s\"\n",
                row->attrib_list[HRDV_STATUS].u.string_value);
        break;

    case CA_NUMBER:
        fprintf(OFILE, "CA_NUMBER: %d\n",
                row->attrib_list[HRDV_STATUS].u.unumber_value);
        break;

    case CA_UNKNOWN:
        fprintf(OFILE, "CA_UNKNOWN\n");
        break;

    case CA_COMPUTED:
        fprintf(OFILE, "CA_COMPUTED\n");
        break;

    default:
        fprintf(OFILE, "(INCORRECT)\n");
        break;
    }

fprintf(OFILE, "hrDeviceErrors . . . . . . ");
switch (row->attrib_list[HRDV_ERRORS].attrib_type) {

    case CA_STRING:
        fprintf(OFILE, "CA_STRING: \"%s\"\n",
                row->attrib_list[HRDV_ERRORS].u.string_value);
        break;

    case CA_NUMBER:
        fprintf(OFILE, "CA_NUMBER: %d\n",
                row->attrib_list[HRDV_ERRORS].u.unumber_value);
        break;

    case CA_UNKNOWN:
        fprintf(OFILE, "CA_UNKNOWN\n");
        break;

    case CA_COMPUTED:
        fprintf(OFILE, "CA_COMPUTED\n");
        break;

    default:
        fprintf(OFILE, "(INCORRECT)\n");
        break;
    }


/* Hidden Context */
fprintf(OFILE, "(HIDDEN CONTEXT) . . . . . ");

switch (row->attrib_list[HRDV_TYPE].u.unumber_value) {

    /*
    | What is stored in HIDDEN_CTX is hardwired for these types
    | of cache entries:
    */
    case 3: // "Processor"
        fprintf(OFILE, "CA_NUMBER: %d (Processor Number)\n",
                row->attrib_list[HIDDEN_CTX].u.unumber_value);
        break;


    case 4: // "Network"
        fprintf(OFILE, "CA_NUMBER: %d ( \"hrNetworkIfIndex\" value)\n",
                row->attrib_list[HIDDEN_CTX].u.unumber_value);
        break;


    case 5: // "Printer"
        fprintf(OFILE, "CA_STRING: \"%s\" ( \"OpenPrinter\" string)\n",
                row->attrib_list[HIDDEN_CTX].u.string_value);
        break;


    /* For this type, it varies */
    case 6: // "DiskStorage"

        switch (row->attrib_list[HIDDEN_CTX].attrib_type) {

            case CA_STRING:
                fprintf(OFILE, "CA_STRING: \"%s\"\n",
                        row->attrib_list[HIDDEN_CTX].u.string_value);
                break;

            case CA_NUMBER:
                fprintf(OFILE, "CA_NUMBER: %d\n",
                        row->attrib_list[HIDDEN_CTX].u.unumber_value);
                break;

            case CA_UNKNOWN:
                fprintf(OFILE, "CA_UNKNOWN\n");
                break;

            case CA_COMPUTED:
                fprintf(OFILE, "CA_COMPUTED\n");
                break;

            case CA_CACHE:
                fprintf(OFILE, "CA_CACHE @ 0x%x\n",
                        row->attrib_list[HIDDEN_CTX].u.cache);
                if (row->attrib_list[HIDDEN_CTX].u.cache != NULL) {
                    PrintCache(row->attrib_list[HIDDEN_CTX].u.cache);
                    }
                break;
            }

        break;


    case 10: // "Video"
    case 11: // "Audio"
    case 12: // "Coprocessor"
    case 13: // "Keyboard"
    case 14: // "Modem"
    case 15: // "ParallelPort"
    case 16: // "Pointing"
    case 17: // "SerialPort"
    case 18: // "Tape"
    case 19: // "Clock"
    case 20: // "VolatileMemory"
    case 21: // "NonVolatileMemory"
    case 2: // "Unknown"
    case 1: // "Other"
    default:
        fprintf(OFILE, "<None>\n");
        break;

    }
}
#endif
