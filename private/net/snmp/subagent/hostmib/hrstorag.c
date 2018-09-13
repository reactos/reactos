/*
 *  HrStorage.c v0.10
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
 *    instance name routines for the HrStorage.  Actual instrumentation code is
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
 *    V1.00 - 04/17/97  D. D. Burns Original Creation: Thu Nov 07 16:39:42 1996
 *
 */


#include <windows.h>
#include <malloc.h>

#include <snmp.h>

#include "mib.h"
#include "smint.h"
#include "hostmsmi.h"
#include "user.h"         /* Developer supplied include file */


/*
 *  GetHrMemorySize
 *    The amount of physical main memory contained by the host.
 *
 *    Gets the value for HrMemorySize.
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
 | hrMemorySize
 |
 |  ACCESS         SYNTAX
 |  read-only      KBytes
 |
 | "The amount of physical main memory contained by the host."
 |
 | DISCUSSION:
 |
 | Win32 API function "GlobalMemoryStatus" is invoked and the value returned
 | in structure MEMORYSTATUS for cell "dwTotalPhys" is used to compute the value
 | returned for this attribute.
 |
 |============================================================================
 | 1.3.6.1.2.1.25.2.2.0
 |                | |
 |                | *-hrMemorySize
 |                *-hrStorage
 */

UINT
GetHrMemorySize(
        OUT KBytes *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
MEMORYSTATUS    mem_status;     /* For "GlobalMemoryStatus()" call */

    /* Get the "mem_status" structure filled in */
    GlobalMemoryStatus(&mem_status);

    /* Return total available physical bytes reduced to 1024 blocks */
    *outvalue = (KBytes)(mem_status.dwTotalPhys / 1024);

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrMemorySize() */


/*
 *  HrStorageFindInstance
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
HrStorageFindInstance( IN ObjectIdentifier *FullOid ,
                       IN OUT ObjectIdentifier *instance )
{
    UINT tmp_instance ;

    //
    //  Developer instrumentation code to find appropriate instance goes here.
    //  For non-tables, it is not necessary to modify this routine.
    //  However, if there is any context that needs to be set, it can be done
    //  here.

    if ( FullOid->idLength <= HRSTORAGE_VAR_INDEX )
	// No instance was specified
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else  if ( FullOid->idLength != HRSTORAGE_VAR_INDEX + 1 )
	// Instance length is more than 1
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else
	// The only valid instance for a non-table are instance 0.  If this
	// is a non-table, the following code validates the instances.  If this
	// is a table, developer modification is necessary below.

	tmp_instance = FullOid->ids[ HRSTORAGE_VAR_INDEX ] ;
	if ( tmp_instance )
	    return SNMP_ERRORSTATUS_NOSUCHNAME ;
	else
	{
	    // the instance is valid.  Create the instance portion of the OID
	    // to be returned from this call.
	    instance->ids[ 0 ] = tmp_instance ;
	    instance->idLength = 1 ;
	}

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of HrStorageFindInstance() */



/*
 *  HrStorageFindNextInstance
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
HrStorageFindNextInstance( IN ObjectIdentifier *FullOid ,
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
    //
    //  If this is a table, then the next instance is the one following the
    //  current instance.
    //
    //  If there are no more instances in the table, return NOSUCHNAME.
    //

    if ( FullOid->idLength <= HRSTORAGE_VAR_INDEX )
    {
	instance->ids[ 0 ] = 0 ;
	instance->idLength = 1 ;
    }
    else
	return SNMP_ERRORSTATUS_NOSUCHNAME ;

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of HrStorageFindNextInstance() */



/*
 *  HrStorageConvertInstance
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
HrStorageConvertInstance( IN ObjectIdentifier *oid_spec ,
                          IN OUT InstanceName *native_spec )
{
    //
    //  Developer supplied code to convert instance identifer to native
    //  specification of instance names goes here.
    //

    return SUCCESS ;

} /* end of HrStorageConvertInstance() */




/*
 *  HrStorageFreeInstance
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
HrStorageFreeInstance( IN OUT InstanceName *instance )
{

    //
    //  Developer supplied code to free native representation of instance name goes here.
    //

} /* end of HrStorageFreeInstance() */

