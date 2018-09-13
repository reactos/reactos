/*
 *  HrSWInstalledEntry.c v0.10
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
 *    instance name routines for the HrSWInstalledEntry.  Actual instrumentation code is
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
 *    V1.00 - 04/27/97  D. D. Burns     Genned: Thu Nov 07 16:49:12 1996
 *
 */


#include <windows.h>
#include <malloc.h>

#include <snmp.h>
#include <winsock.h>      /* For htons()           */

#include "mib.h"
#include "smint.h"
#include "hostmsmi.h"
#include "user.h"         /* Developer supplied include file */
#include "HMCACHE.H"      /* Cache-related definitions           */
#include <regstr.h>       /* For Registry-lookup on software     */
#include <winreg.h>       /* For Registry-lookup on software     */
#include <objbase.h>      /* For CoFileTimeToDosDateTime()       */


/*
|==============================================================================
| Function prototypes for this module.
|
*/
/* Gen_SingleDevices - Generate Single Device row entries in HrDevice */
BOOL Gen_SingleDevices( void );


/* AddSWInstalled - Add a row to HrSWInstalled Table */
BOOL AddSWInstalled( HKEY sw_key,  FILETIME *ft);


#if defined(CACHE_DUMP)

/* debug_print_hrswinstalled - Prints a Row from HrSWInstalled table */
static void
debug_print_hrswinstalled(
                        CACHEROW     *row  /* Row in hrSWInstalled table */
                        );
#endif

/*
|==============================================================================
| Create the list-head for the HrSWInstalled table cache.
|
| (This macro is defined in "HMCACHE.H").
*/
static CACHEHEAD_INSTANCE(hrSWInstalled_cache, debug_print_hrswinstalled);


/*
|==============================================================================
| hrSWInstalledTable Attribute Defines
|
|    Each attribute defined for this table is associated with one of the
|    #defines below.  These symbols are used as C indices into the array of
|    attributes within a cached-row.
|
*/
#define HRIN_INDEX    0    // hrSWInstalledIndex
#define HRIN_NAME     1    // hrSWInstalledName
#define HRIN_DATE     2    // hrSWInstalledDate
                      //-->Add more here, change count below!
#define HRIN_ATTRIB_COUNT 3



/*
 *  GetHrSWInstalledIndex
 *    A unique value for each piece of software installed on the host.  This
 *    value shall be in the range from 1 to the number of piece
 *
 *    Gets the value for HrSWInstalledIndex.
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
 | hrSWInstalledIndex
 |
 |  ACCESS         SYNTAX
 |  read-only      INTEGER (1..2147483647)
 |
 | "A unique value for each piece of software installed on the host.  This value
 | shall be in the range from 1 to the number of pieces of software installed on
 | the host."
 |
 |============================================================================
 | 1.3.6.1.2.1.25.6.3.1.1.<instance>
 |                | | | |
 |                | | | *-hrSwInstalledIndex
 |                | | *-hrSWInstalledEntry
 |                | *-hrSWInstalledTable
 |                *-hrSWInstalled
 */

UINT
GetHrSWInstalledIndex(
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
if ((row = FindTableRow(index, &hrSWInstalled_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| Return the "hrSWInstalledIndex" value from this entry
*/
*outvalue = row->attrib_list[HRIN_INDEX].u.unumber_value;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSWInstalledIndex() */


/*
 *  GetHrSWInstalledName
 *    A textual description of this installed piece of software, including the
 *    manufacturer, revision, the name by which it is commonl
 *
 *    Gets the value for HrSWInstalledName.
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
 | hrSWInstalledName
 |
 |  ACCESS         SYNTAX
 |  read-only      InternationalDisplayString (SIZE (0..64))
 |
 | "A textual description of this installed piece of software, including the
 | manufacturer, revision, the name by which it is commonly known, and
 | optionally, its serial number."
 |
 |============================================================================
 | DISCUSSION:
 |    For logo 95 programs, we use the Registry sub-key name.
 |
 | 1.3.6.1.2.1.25.6.3.1.2.<instance>
 |                | | | |
 |                | | | *-hrSwInstalledName
 |                | | *-hrSWInstalledEntry
 |                | *-hrSWInstalledTable
 |                *-hrSWInstalled
 */

UINT
GetHrSWInstalledName(
        OUT InternationalDisplayString *outvalue ,
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
if ((row = FindTableRow(index, &hrSWInstalled_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| Return the "hrSWInstalledName" value from this entry
*/
outvalue->string = row->attrib_list[HRIN_NAME].u.string_value;

/* "Truncate" here to meet RFC as needed*/
if ((outvalue->length = strlen(outvalue->string)) > 64) {
    outvalue->length = 64;
    }

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSWInstalledName() */


/*
 *  GetHrSWInstalledID
 *    The product ID of this installed piece of software.
 *
 *    Gets the value for HrSWInstalledID.
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
 | hrSWInstalledID
 |
 |  ACCESS         SYNTAX
 |  read-only      ProductID
 |
 | "The product ID of this installed piece of software."
 |
 |============================================================================
 | DISCUSSION:
 |    For logo 95 programs we don't know the ProductID as an OID, so we
 |    return the ProductID for "unknown": { 0.0 }
 |
 | 1.3.6.1.2.1.25.6.3.1.3.<instance>
 |                | | | |
 |                | | | *-hrSwInstalledID
 |                | | *-hrSWInstalledEntry
 |                | *-hrSWInstalledTable
 |                *-hrSWInstalled
 */

UINT
GetHrSWInstalledID(
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

} /* end of GetHrSWInstalledID() */


/*
 *  GetHrSWInstalledType
 *    The type of this software.
 *
 *    Gets the value for HrSWInstalledType.
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
 | hrSWInstalledType
 |
 |  ACCESS         SYNTAX
 | read-only       INTEGER {unknown(1),operatingSystem(2),deviceDriver(3),
 |                          application(4)}
 |
 | "The type of this software."
 |============================================================================
 | DISCUSSION:
 |    For logo 95 programs we presume that all uninstallable software is
 |    an application.  That is the only type we return.
 |
 | 1.3.6.1.2.1.25.6.3.1.4.<instance>
 |                | | | |
 |                | | | *-hrSwInstalledType
 |                | | *-hrSWInstalledEntry
 |                | *-hrSWInstalledTable
 |                *-hrSWInstalled
 */

UINT
GetHrSWInstalledType(
        OUT INTSWType *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{

*outvalue = 4;  // 4 = "application"

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSWInstalledType() */


/*
 *  GetHrSWInstalledDate
 *    The last-modification date of this application as it would appear in a
 *    directory listing.
 *
 *    Gets the value for HrSWInstalledDate.
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
 | hrSWInstalledDate
 |
 |  ACCESS         SYNTAX
 |  read-only      DateAndTime
 |
 | "The last-modification date of this application as it would appear in a
 |  directory listing."
 |============================================================================
 | DISCUSSION:
 |    For logo 95 programs we use the date of the last write into the
 |    Registry key associated with the application.
 |
 | 1.3.6.1.2.1.25.6.3.1.5.<instance>
 |                | | | |
 |                | | | *-hrSwInstalledDate
 |                | | *-hrSWInstalledEntry
 |                | *-hrSWInstalledTable
 |                *-hrSWInstalled
 */

UINT
GetHrSWInstalledDate(
        OUT DateAndTime *outvalue ,
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
if ((row = FindTableRow(index, &hrSWInstalled_cache)) == NULL) {
    return SNMP_ERRORSTATUS_GENERR;
    }

/*
| Return the "hrSWInstalledDate" value from this entry
*/
outvalue->string = row->attrib_list[HRIN_DATE].u.string_value;
outvalue->length = 8;

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSWInstalledDate() */


/*
 *  HrSWInstalledEntryFindInstance
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
HrSWInstalledEntryFindInstance( IN ObjectIdentifier *FullOid ,
                       IN OUT ObjectIdentifier *instance )
{
    UINT tmp_instance ;

    //
    //  Developer instrumentation code to find appropriate instance goes here.
    //  For non-tables, it is not necessary to modify this routine.  However, if
    //  there is any context that needs to be set, it can be done here.
    //

    if ( FullOid->idLength <= HRSWINSTALLEDENTRY_VAR_INDEX )
	// No instance was specified
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else  if ( FullOid->idLength != HRSWINSTALLEDENTRY_VAR_INDEX + 1 )
	// Instance length is more than 1
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else
	// The only valid instance for a non-table are instance 0.  If this
	// is a non-table, the following code validates the instances.  If this
	// is a table, developer modification is necessary below.

	tmp_instance = FullOid->ids[ HRSWINSTALLEDENTRY_VAR_INDEX ] ;

        /*
        | For hrSWInstalledTable, the instance arc(s) is a single arc, and
        | it must correctly select an entry in the hrSWInstalled Table cache.
        | Check that here.
        */
	if ( FindTableRow(tmp_instance, &hrSWInstalled_cache) == NULL ) {
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

} /* end of HrSWInstalledEntryFindInstance() */



/*
 *  HrSWInstalledEntryFindNextInstance
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
HrSWInstalledEntryFindNextInstance( IN ObjectIdentifier *FullOid ,
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


    if ( FullOid->idLength <= HRSWINSTALLEDENTRY_VAR_INDEX )
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
        tmp_instance = FullOid->ids[ HRSWINSTALLEDENTRY_VAR_INDEX ] ;
        }

    /* Now go off and try to find the next instance in the table */
    if ((row = FindNextTableRow(tmp_instance, &hrSWInstalled_cache)) == NULL) {
        return SNMP_ERRORSTATUS_NOSUCHNAME ;
        }

    instance->ids[ 0 ] = row->index ;
    instance->idLength = 1 ;

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of HrSWInstalledEntryFindNextInstance() */



/*
 *  HrSWInstalledEntryConvertInstance
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
HrSWInstalledEntryConvertInstance( IN ObjectIdentifier *oid_spec ,
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

} /* end of HrSWInstalledEntryConvertInstance() */




/*
 *  HrSWInstalledEntryFreeInstance
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
HrSWInstalledEntryFreeInstance( IN OUT InstanceName *instance )
{

  /* No action needed for hrSWInstalledTable */
} /* end of HrSWInstalledEntryFreeInstance() */

/*
| End of Generated Code
*/


/* Gen_HrSWInstalled_Cache - Generate a cache for HrSWInstalled Table */
/* Gen_HrSWInstalled_Cache - Generate a cache for HrSWInstalled Table */
/* Gen_HrSWInstalled_Cache - Generate a cache for HrSWInstalled Table */

BOOL
Gen_HrSWInstalled_Cache(
                        void
                        )

/*
|  EXPLICIT INPUTS:
|
|       None.
|
|  IMPLICIT INPUTS:
|
|       The module-local head of the cache for the HrSWInstalled table,
|       "hrSWInstalled_cache".
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
|       populate the cache for the HrSWInstalled table.
|
|  OTHER THINGS TO KNOW:
|
|       There is one of these function for every table that has a cache.
|       Each is found in the respective source file.
|
|=============== From WebEnable Design Spec Rev 3 04/11/97==================
| DISCUSSION:
|
| This implementation of this entire group is rather problematical without the
| creation of a standard.  It certainly appears that many software manufacturers
| dutifully register SOMETHING in the Registry when software is installed,
| however there appears to be no rhyme nor reason to the information put into
| the Registry in leaves below the manufacturers name.  Consequently, for
| installed application software, there is no easy, reliable way of mapping
| the Registry information into entries in this table.
|
| It is clear that some consistent scheme seems to be currently implemented for
| Microsoft software, however the details of extracting it from the Registry
| (and whether or not all information needed for full population of entries in
| this table is available) is not documented.
|
| Proper implementation of "hrSWInstalled" requires the creation and
| promulgation of a standard for registering ISD software (presumably in the
| Registry).  Information in "hrSWInstalled" includes attributes with
| values of Object Identifiers.  Webenable Inc. is prepared to work with
| Microsoft in establishing a standard for registering software in a fashion
| that allows proper implementation of the "hrSWInstalled" table.
|
| Resolution:
|      Report only logo 95 compliant software initially.
|
|============================================================================
|
| 1.3.6.1.2.1.25.6.1.0....
|                | |
|                | *hrSWInstalledLastChange
|                *-hrSWInstalled
|
| 1.3.6.1.2.1.25.6.2.0....
|                | |
|                | *hrSWInstalledLastUpdateTime
|                *-hrSWInstalled
|
| 1.3.6.1.2.1.25.6.3.1....
|                | | |
|                | | *-hrSWInstalledEntry
|                | *-hrSWInstalledTable
|                *-hrSWInstalled
|
*/

#define SUBKEY_LEN 64   // Long enough for short key-name of software
{
HKEY     subkey;                        /* Handle of subkey for uninstall software */
DWORD    index;                         /* Index counter for enumerating subkeys   */
LONG     enum_status=ERROR_SUCCESS;     /* Status from subkey enumeration          */
CHAR     subkey_name[SUBKEY_LEN];       /* Subkey name returned here               */
DWORD    subkey_len=SUBKEY_LEN;         /* Subkey name buffer size                 */
FILETIME keytime;                       /* Time subkey was last written to         */
BOOL     add_status;                    /* Status from add-row operation           */
HKEY     sw_key;                        /* Handle of software key for value enum   */


if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,    // hkey - currently open
                 REGSTR_PATH_UNINSTALL, // subkey to open
                 0,                     // options
                 KEY_READ,              // Security access mask
                 &subkey
                 ) == ERROR_SUCCESS) {

    /* Enumerate the keys using the Uninstall subkey */
    for (index = 0; enum_status != ERROR_NO_MORE_ITEMS; index += 1) {

        subkey_len=SUBKEY_LEN;

        enum_status = RegEnumKeyEx(subkey,      // Enumerating this key
                                   index,       // next subkey index
                                   subkey_name, // Buffer to rcv subkey name
                                   &subkey_len, // Buffer size
                                   NULL,        // Reserved
                                   NULL,        // Class name buffer
                                   NULL,        // Class buffer size
                                   &keytime     // Time of last write to subkey
                                   );

        /* Skip if we didn't open OK */
        if (enum_status != ERROR_SUCCESS) {
            continue;
            }


        /* Now try for the software itself */
        if (RegOpenKeyEx(subkey,                // hkey - currently open
                         subkey_name,           // subkey to open
                         0,                     // options
                         KEY_READ,              // Security access mask
                         &sw_key
                         ) == ERROR_SUCCESS) {

            /* Now Enumerate the Values of this key */
            add_status =
                AddSWInstalled(sw_key,   // Key to obtain DisplayName
                               &keytime  // Date and Time installed
                               );
            RegCloseKey(sw_key);

            /* If something blew down below, bail out */
            if (add_status == FALSE) {

                RegCloseKey(subkey);
                return ( FALSE );
                }
            }
        }
    }

RegCloseKey(subkey);

#if defined(CACHE_DUMP)
PrintCache(&hrSWInstalled_cache);
#endif

/* hrSWInstalled cache initialized */
return ( TRUE );
}


/* UTCDosDateTimeToLocalSysTime - converts UTC msdos date and time to local SYSTEMTIME structure */
/* UTCDosDateTimeToLocalSysTime - converts UTC msdos date and time to local SYSTEMTIME structure */
/* UTCDosDateTimeToLocalSysTime - converts UTC msdos date and time to local SYSTEMTIME structure */
void UTCDosDateTimeToLocalSysTime(WORD msdos_date, WORD msdos_time, LPSYSTEMTIME pSysTime)
{
    SYSTEMTIME utcSysTime;

    utcSysTime.wYear = (msdos_date >> 9) + 1980;
    utcSysTime.wMonth = ((msdos_date >> 5) & 0x0F);
    utcSysTime.wDay = (msdos_date & 0x1F);
    utcSysTime.wDayOfWeek = 0;
    utcSysTime.wHour = (msdos_time >> 11);
    utcSysTime.wMinute = ((msdos_time >> 5) & 0x3F);
    utcSysTime.wSecond = ((msdos_time & 0x1F) * 2);
    utcSysTime.wMilliseconds = 0;

    if (!SystemTimeToTzSpecificLocalTime(
             NULL,           // [in]  use active time zone
             &utcSysTime,    // [in]  utc system time
             pSysTime))      // [out] local time 
    {
        // if the utc time could not be converted to local time,
        // just return the utc time
        memcpy(&utcSysTime, pSysTime, sizeof(SYSTEMTIME));
    }
}


/* AddSWInstalled - Add a row to HrSWInstalled Table */
/* AddSWInstalled - Add a row to HrSWInstalled Table */
/* AddSWInstalled - Add a row to HrSWInstalled Table */

BOOL
AddSWInstalled(
               HKEY         sw_key,
               FILETIME    *ft
               )
/*
|  EXPLICIT INPUTS:
|
|       "sw_key" - an opened key for a piece of software whose Values we
|        must enumerate looking for "DisplayName"
|
|       "ft" - This is the time that the key was last written, and we
|       take it as the time the software was installed.
|
|  IMPLICIT INPUTS:
|
|       The module-local head of the cache for the HrSWInstalled table,
|       "hrSWInstalled_cache".
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
|     The Gen_SWInstalled_cache() function has been invoked to populate
|     the cache, and this function is called when it has found another
|     piece of software for which a row-entry needs to be placed into
|     the cache.
|
|  OTHER THINGS TO KNOW:
|
*/

#define VALUE_LEN 32         // Long enough for "UninstallString"
#define VALUE_DATA_LEN 128   // Long enough for a long "casual" name
{
DWORD    index;                         /* Index counter for enumerating subkeys   */
LONG     enum_status=ERROR_SUCCESS;     /* Status from subkey enumeration          */
CHAR     value_name[VALUE_LEN];         /* Value name returned here                */
DWORD    value_len;                     /* Value name buffer size                  */
DWORD    value_type;                    /* Type code for value string              */
CHAR     value_data[VALUE_DATA_LEN];    /* Value's value returned here          */
DWORD    value_data_len;                /* Value's value buffer length resides here*/

static                                  /* NOTE: "static" is a 'must'              */
ULONG    table_index=0;                 /* HrDeviceTable index counter             */
CACHEROW *row;                          /* --> Cache structure for row-being built */
WORD     msdos_date;                    /* Conversion area for software date/time  */
WORD     msdos_time;                    /* Conversion area for software date/time  */
char    *octet_string;                  /* Alias for building DateAndTime          */
UINT     i;                             /* Loop index                              */


/*
| Now go a-lookin' for "DisplayName", the value whose data will give us
| the name of the installed software.
|
| For each Value associated with this software's key . . .
*/
for (index = 0; enum_status != ERROR_NO_MORE_ITEMS; index += 1) {

    /* Make sure these cells continues to reflect the buffer size */
    value_len = VALUE_LEN;
    value_data_len = VALUE_DATA_LEN;

    enum_status = RegEnumValue(sw_key,        // Key whose values we're enuming
                               index,         // index of next value
                               value_name,    // Value name buffer
                               &value_len,    // length of Value name buffer
                               NULL,          // reserved
                               &value_type,   // type of value data
                               value_data,    // Buffer for value's data
                               &value_data_len// Length of data buffer
                               );

    /* Only if we managed to fetch this key do we try to recognize it */
    if (enum_status == ERROR_SUCCESS) {

        /* If the value we just read is for "DisplayName" */
        if ( strcmp(value_name, REGSTR_VAL_UNINSTALLER_DISPLAYNAME) == 0) {

            /*
            | Get a row-entry created.
            */
            if ((row = CreateTableRow( HRIN_ATTRIB_COUNT ) ) == NULL) {
                return ( FALSE );       // Out of memory
                }

            /*
            | Set up the cached hrSWInstalled attributes in the new row
            */

            /* =========== hrSWInstalledIndex ==========*/
            row->attrib_list[HRIN_INDEX].attrib_type = CA_NUMBER;
            row->attrib_list[HRIN_INDEX].u.unumber_value = (table_index += 1) ;


            /* =========== hrSWInstalledName ==========*/
            row->attrib_list[HRIN_NAME].attrib_type = CA_STRING;
            if ( (row->attrib_list[HRIN_NAME].u.string_value
                  = ( LPSTR ) malloc(value_data_len + 1)) == NULL) {
                return ( FALSE );       /* out of memory */
                }
            strcpy(row->attrib_list[HRIN_NAME].u.string_value, value_data);


            /* =========== hrSWInstalledDate ==========
            |
            | Here's the deal on this one.  We've got a 64-bit FILETIME
            | representation of when the Registry entry was made for the
            | software. We're taking this as the install time of the software.
            |
            | So we convert to MS-DOS time, then to DateAndTime (in the
            | 8-octet form) below:
            |========================== From RFC1514 ========================
            |
            |    DateAndTime ::= OCTET STRING (SIZE (8 | 11))
            |    --     A date-time specification for the local time of day.
            |    --     This data type is intended to provide a consistent
            |    --     method of reporting date information.
            |    --
            |    --         field  octets  contents                  range
            |    --         _____  ______  ________                  _____
            |    --           1      1-2   year                      0..65536
            |    --                           (in network byte order)
            |    --           2       3    month                     1..12
            |    --           3       4    day                       1..31
            |    --           4       5    hour                      0..23
            |    --           5       6    minutes                   0..59
            |    --           6       7    seconds                   0..60
            |    --                        (use 60 for leap-second)
            |    --           7       8    deci-seconds              0..9
            |    --           8       9    direction from UTC        "+" / "-"
            |    --                        (in ascii notation)
            |    --           9      10    hours from UTC            0..11
            |    --          10      11    minutes from UTC          0..59
            |    --
            |    --         Note that if only local time is known, then
            |    --         timezone information (fields 8-10) is not present.
            |
            |    MS-DOS records file dates and times as packed 16-bit values.
            |    An MS-DOS date has the following format:
            |    Bits	Contents
            |    ----   --------
            |    0-4	Days of the month (1-31).
            |    5-8	Months (1 = January, 2 = February, and so forth).
            |    9-15	Year offset from 1980 (add 1980 to get actual year).
            |
            |    An MS-DOS time has the following format:
            |    Bits	Contents
            |    ----   --------
            |    0-4	Seconds divided by 2.
            |    5-10	Minutes (0-59).
            |    11-15	Hours (0-23 on a 24-hour clock).
            |
            */
            row->attrib_list[HRIN_DATE].attrib_type = CA_STRING;
            if ( (row->attrib_list[HRIN_DATE].u.string_value
                        = octet_string = ( LPSTR ) malloc(8)) == NULL) {
                return ( FALSE );       /* out of memory */
                }
            for (i=0; i < 8; i += 1) octet_string[i] = '\0';

            if (CoFileTimeToDosDateTime(ft, &msdos_date, &msdos_time) == TRUE) {
                SYSTEMTIME localInstTime;
                USHORT year;

                UTCDosDateTimeToLocalSysTime(msdos_date, msdos_time, &localInstTime);

                year = htons(localInstTime.wYear);
                octet_string[0] = (year & 0xFF);
                octet_string[1] = (year >> 8);
                octet_string[2] = (char)localInstTime.wMonth;
                octet_string[3] = (char)localInstTime.wDay;
                octet_string[4] = (char)localInstTime.wHour;
                octet_string[5] = (char)localInstTime.wMinute;
                octet_string[6] = (char)localInstTime.wSecond;
                octet_string[7] = (char)localInstTime.wMilliseconds / 10;
                }

            /*
            | The other standard hrSWInstalled attributes are currently
            | "hardwired" in the Get functions.
            */

            /*
            | Now insert the filled-in CACHEROW structure into the
            | cache-list for the hrDeviceTable.
            */
            if (AddTableRow(row->attrib_list[HRIN_INDEX].u.unumber_value,  /* Index */
                            row,                                           /* Row   */
                            &hrSWInstalled_cache                           /* Cache */
                            ) == FALSE) {
                return ( FALSE );       /* Internal Logic Error! */
                }

            /*
            | Break from the Enumeration loop on the values, we've found
            | the one we want.
            */
            break;
            }
        }
    }  /* for */

/* Add succeeded */
return ( TRUE );
}


#if defined(CACHE_DUMP)


/* debug_print_hrswinstalled - Prints a Row from HrSWInstalled table */
/* debug_print_hrswinstalled - Prints a Row from HrSWInstalled table */
/* debug_print_hrswinstalled - Prints a Row from HrSWInstalled table */

static void
debug_print_hrswinstalled(
                        CACHEROW     *row  /* Row in hrSWInstalled table */
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
UINT    i;

if (row == NULL) {
    fprintf(OFILE, "=========================\n");
    fprintf(OFILE, "hrSWInstalled Table Cache\n");
    fprintf(OFILE, "=========================\n");
    return;
    }

fprintf(OFILE, "hrSWInstalledIndex . . . . %d\n",
        row->attrib_list[HRIN_INDEX].u.unumber_value);

fprintf(OFILE, "hrSWInstalledName  . . . . %s\n",
        row->attrib_list[HRIN_NAME].u.string_value);

fprintf(OFILE, "hrSWInstalledDate  . . . . ");
for (i = 0; i < 8; i += 1) {
    fprintf(OFILE, "%2.2x ",row->attrib_list[HRIN_DATE].u.string_value[i]);
    }
fprintf(OFILE, "\n");
}
#endif
