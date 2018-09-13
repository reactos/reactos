/*
 *  File HRDEVENT.H
 *
 *  Facility:
 *
 *    Windows NT SNMP Extension Agent
 *
 *  Abstract:
 *
 *    This module is contains definitions pertaining to the HostMIB
 *    hrDevice table... definitions needed by the "sub-tables" within
 *    the hrDevice table and the functions that deal with these tables.
 *
 *  Author:
 *
 *    D. D. Burns @ WebEnable, Inc.
 *
 *
 *  Revision History:
 *
 *    V1.0 - 04/28/97  D. D. Burns     Original Creation
 */

#ifndef hrdevent_h
#define hrdevent_h


/*
|==============================================================================
| hrDevice Attribute Defines
|
|    Each attribute defined for this table is associated with one of the
|    #defines below (with the exception of "hrDeviceID" which is never cached
|    and handled exclusively by the GetHrDeviceID() function).
|
|    One special define is used in the same manner as the defines for the
|    real attributes to access a "hidden attribute" value which is never
|    returned as a consequence of an SNMP request, but is stored in the cache
|    to allow "computed" values to be obtained for some of the real attributes.
|
|    An example of a value in the HIDDEN_CTX attribute might be the string
|    needed to look up a value of hrDeviceStatus for this device or another
|    "computed" value in another associated table (such as hrPrintertable).
|
|    These symbols are used as C indices into the array of attributes within a
|    cached-row of the hrDevice Table.
|
*/
#define HRDV_INDEX    0    // hrDeviceIndex
#define HRDV_TYPE     1    // hrDeviceType
#define HRDV_DESCR    2    // hrDeviceDescr
                           // (hrDeviceID omitted)
#define HRDV_STATUS   3    // hrDeviceStatus
#define HRDV_ERRORS   4    // hrDeviceErrors
#define HIDDEN_CTX    5    // (Hidden Context Information).
                      //-->Add more here, change count below!
#define HRDV_ATTRIB_COUNT 6


/*
|==============================================================================
| hrPartition Attribute Defines
|
|    Each attribute defined for this table is associated with one of the
|    #defines below.  These symbols are used as C indices into the array of
|    attributes within a cached-row.
|
|    These symbols appear in this file so that code in HRDISKST.C can
|    properly initialize rows in the HrPartition table while code in
|    HRPARTIT.C can reference them.
*/
#define HRPT_INDEX     0    // hrPartitionIndex
#define HRPT_LABEL     1    // hrPartitionLabel
#define HRPT_ID        2    // hrPartitionID
#define HRPT_SIZE      3    // hrPartitionSize
#define HRPT_FSINDEX   4    // hrPartitionFSIndex
                      //-->Add more here, change count below!
#define HRPT_ATTRIB_COUNT 5


/*
|==============================================================================
| hrDiskStorage Attribute Defines
|
|    Each attribute defined for this table is associated with one of the
|    #defines below.  These symbols are used as C indices into the array of
|    attributes within a cached-row.
|
*/
#define HRDS_ACCESS    0    // hrDiskStorageAccess
#define HRDS_MEDIA     1    // hrDiskStorageMedia
#define HRDS_REMOVABLE 2    // hrDiskStorageRemovable
#define HRDS_CAPACITY  3    // hrDiskStorageCapacity
                      //-->Add more here, change count below!
#define HRDS_ATTRIB_COUNT 4


/*
|==============================================================================
| hrDevice Type OID Ending Arcs
|
|    RFC1514 specifies an object identifier "{ hrDeviceTypes }" to be used
|    as a prefix to the full OID that specifies a device's type in the
|    hrDevice table.  The symbols below specify the final arc "x" as in
|    "{ hrDeviceTypes x }" to be used for each device type.
|
|    You can't change these symbol values... we're just trying to be
|    mnemonic here.
*/
#define HRDV_TYPE_LASTARC_OTHER         1
#define HRDV_TYPE_LASTARC_UNKNOWN       2
#define HRDV_TYPE_LASTARC_PROCESSOR     3
#define HRDV_TYPE_LASTARC_NETWORK       4
#define HRDV_TYPE_LASTARC_PRINTER       5
#define HRDV_TYPE_LASTARC_DISKSTORAGE   6
#define HRDV_TYPE_LASTARC_VIDEO         10
#define HRDV_TYPE_LASTARC_AUDIO         11
#define HRDV_TYPE_LASTARC_COPROCESSOR   12
#define HRDV_TYPE_LASTARC_KEYBOARD      13
#define HRDV_TYPE_LASTARC_MODEM         14
#define HRDV_TYPE_LASTARC_PARALLELPORT  15
#define HRDV_TYPE_LASTARC_POINTING      16
#define HRDV_TYPE_LASTARC_SERIALPORT    17
#define HRDV_TYPE_LASTARC_TAPE          18
#define HRDV_TYPE_LASTARC_CLOCK         19
#define HRDV_TYPE_LASTARC_VOLMEMORY     20
#define HRDV_TYPE_LASTARC_NONVOLMEMORY  21


/*
|==============================================================================
| HRDEVICE-Related Function Prototypes
*/

/* Gen_HrPrinter_Cache - Generate a initial cache for HrDevice PRINTER Table */
BOOL Gen_HrPrinter_Cache( ULONG type_arc );       /* "HRPRINTE.C" */

/* COMPUTE_hrPrinter_status - Compute "hrDeviceStatus" for a Printer device */
BOOL COMPUTE_hrPrinter_status(
                         CACHEROW *row,
                         UINT     *outvalue
                         );                       /* "HRPRINTE.C" */

/* COMPUTE_hrPrinter_errors - Compute "hrDeviceErrors" for a Printer device */
BOOL COMPUTE_hrPrinter_errors(
                         CACHEROW *row,
                         UINT     *outvalue
                         );                       /* "HRPRINTE.C" */

/* Gen_HrProcessor_Cache - Gen. a initial cache for HrDevice PROCESSOR Table */
BOOL Gen_HrProcessor_Cache( ULONG type_arc );     /* "HRPROCES.C" */

/* Gen_HrNetwork_Cache - Gen. a initial cache for HrDevice NETWORK Table */
BOOL Gen_HrNetwork_Cache( ULONG type_arc );       /* "HRNETWOR.C" */

/* Gen_HrDiskStorage_Cache - Generate a initial cache for HrDiskStorage Table */
BOOL Gen_HrDiskStorage_Cache( ULONG type_arc );   /* "HRDISKST.C" */
extern CACHEHEAD hrDiskStorage_cache;    /* This cache is globally accessible */


/* AddrHrDeviceRow - Generate another Row Entry in HrDevice Table
|
|  Special purpose cache-row function just for hrDevice and related sub-tables.
|
|  Source is in "HRDEVENT.C".
*/
CACHEROW *
AddHrDeviceRow(
               ULONG   type_arc,       /* Last Arc value for OID for Type   */
               LPSTR   descr,          /* Description string                */
               void   *hidden_ctx,     /* If non-NULL: Hidden-context value */
               ATTRIB_TYPE  hc_type    /* Type of "hidden_ctx"              */
               );


#endif /* hrdevent_h */
