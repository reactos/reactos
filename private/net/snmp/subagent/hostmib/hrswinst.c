/*
 *  HrSWInstalled.c v0.10
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
 *    instance name routines for the HrSWInstalled.  Actual instrumentation code is
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
 *    V1.00 - 04/27/97  D. D. Burns     Genned: Thu Nov 07 16:48:30 1996
 *
 */


#include <windows.h>
#include <malloc.h>

#include <snmp.h>

#include "mib.h"
#include "smint.h"
#include "hostmsmi.h"
#include "user.h"         /* Developer supplied include file     */


/*
 *  GetHrSWInstalledLastChange
 *    The value of sysUpTime when an entry in the hrSWInstalledTable was last 
 *    added, renamed, or deleted.  Because this table is likel
 *    
 *    Gets the value for HrSWInstalledLastChange.
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
 | hrSWInstalledLastChange
 | 
 |  ACCESS         SYNTAX
 |  read-only      TimeTicks
 | 
 | "The value of sysUpTime when an entry in the hrSWInstalledTable was last
 | added, renamed, or deleted.  Because this table is likely to contain many
 | entries, polling of this object allows a management station to determine when
 | re-downloading of the table might be useful."
 |
 |============================================================================
 | Decision is made to report the current value of sysUpTime as a means of
 | signalling the SNMP Manager that "whatever he's got now, he should ask
 | for the latest".  Our cache of installed software is never updated after
 | the agent comes up in the first release.
 |============================================================================
 | 1.3.6.1.2.1.25.6.1.0....
 |                | | 
 |                | *hrSWInstalledLastChange
 |                *-hrSWInstalled
 */

UINT
GetHrSWInstalledLastChange( 
        OUT TimeTicks *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{

    *outvalue = SnmpSvcGetUptime();

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSWInstalledLastChange() */


/*
 *  GetHrSWInstalledLastUpdateTime
 *    The value of sysUpTime when the hrSWInstalledTAble was last completely 
 *    updated.  Because caching of this data will be a popular 
 *    
 *    Gets the value for HrSWInstalledLastUpdateTime.
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
 | hrSWInstalledLastUpdateTime
 | 
 |  ACCESS         SYNTAX
 |  read-only      TimeTicks
 | 
 | "The value of sysUpTime when the hrSWInstalledTable was last completely
 | updated.  Because caching of this data will be a popular implementation
 | strategy, retrieval of this object allows a management station to obtain a
 | guarantee that no data in this table is older than the indicated time."
 |
 |============================================================================
 | Decision is made to report the current value of sysUpTime as a means of
 | signalling the SNMP Manager that "whatever he's got now, he should ask
 | for the latest".  Our cache of installed software is never updated after
 | the agent comes up in the first release.
 |============================================================================
 | 1.3.6.1.2.1.25.6.2.0....
 |                | | 
 |                | *hrSWInstalledLastUpdateTime
 |                *-hrSWInstalled
 */

UINT
GetHrSWInstalledLastUpdateTime( 
        OUT TimeTicks *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{

    *outvalue = SnmpSvcGetUptime();

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSWInstalledLastUpdateTime() */


/*
 *  HrSWInstalledFindInstance
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
HrSWInstalledFindInstance( IN ObjectIdentifier *FullOid ,
                       IN OUT ObjectIdentifier *instance )
{
    UINT tmp_instance ;

    //
    //  Developer instrumentation code to find appropriate instance goes here.
    //  For non-tables, it is not necessary to modify this routine.  However, if
    //  there is any context that needs to be set, it can be done here.
    //

    if ( FullOid->idLength <= HRSWINSTALLED_VAR_INDEX )
	// No instance was specified
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else  if ( FullOid->idLength != HRSWINSTALLED_VAR_INDEX + 1 )
	// Instance length is more than 1
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else
	// The only valid instance for a non-table are instance 0.  If this
	// is a non-table, the following code validates the instances.  If this
	// is a table, developer modification is necessary below.

	tmp_instance = FullOid->ids[ HRSWINSTALLED_VAR_INDEX ] ;
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

} /* end of HrSWInstalledFindInstance() */



/*
 *  HrSWInstalledFindNextInstance
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
HrSWInstalledFindNextInstance( IN ObjectIdentifier *FullOid ,
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

    if ( FullOid->idLength <= HRSWINSTALLED_VAR_INDEX )
    {
	instance->ids[ 0 ] = 0 ;
	instance->idLength = 1 ;
    }
    else
	return SNMP_ERRORSTATUS_NOSUCHNAME ;

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of HrSWInstalledFindNextInstance() */



/*
 *  HrSWInstalledConvertInstance
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
HrSWInstalledConvertInstance( IN ObjectIdentifier *oid_spec ,
                          IN OUT InstanceName *native_spec )
{
    //
    //  Developer supplied code to convert instance identifer to native
    //  specification of instance names goes here.
    //

    return SUCCESS ;

} /* end of HrSWInstalledConvertInstance() */




/*
 *  HrSWInstalledFreeInstance
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
HrSWInstalledFreeInstance( IN OUT InstanceName *instance )
{

    //
    //  Developer supplied code to free native representation of instance name goes here.
    //

} /* end of HrSWInstalledFreeInstance() */
