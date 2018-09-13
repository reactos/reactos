/*
 *  HrSystem.c v0.10
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
 *    instance name routines for the HrSystem.  Actual instrumentation code is
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
 *    V1.00 - 04/17/97  D. D. Burns     Genned: Thu Nov 07 16:39:21 1996
 *
 *
 */


#include <windows.h>
#include <malloc.h>

#include <snmp.h>

#include <lmcons.h>       /* for NET_API_STATUS    */
#include <lmwksta.h>      /* For NetWkStaGetInfo() */
#include <lmapibuf.h>     /* For NetApiBufferFree()*/
#include <lmerr.h>        /* For NERR_Success      */
#include <winsock.h>      /* For htons()           */

#include "mib.h"
#include "smint.h"
#include "hostmsmi.h"
#include "user.h"         /* Developer supplied include file */
#include "HMCACHE.H"      /* For "hrSystemInitialLoadDevice" */


/*
 *  GetHrSystemUptime
 *    The amount of time since this host was last initialized.
 *    
 *    Gets the value for HrSystemUptime.
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
 | hrSystemUptime
 | 
 |  ACCESS         SYNTAX
 |  read-only      TimeTicks
 | 
 | "The amount of time since this host was last initialized.  Note that this is
 | different from sysUpTime in MIB-II [3] because sysUpTime is the uptime of 
 | the network management portion of the system."
 | 
 | DISCUSSION:
 | 
 | Win32 API function "GetTickCount" is used to obtain the value returned for a
 | GET on this variable.
 |
 |============================================================================
 | 1.3.6.1.2.1.25.1.1.0
 |                | |
 |                | *-hrSystemUptime
 |                *---hrSystem
 */

UINT
GetHrSystemUptime( 
        OUT TimeTicks *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{

*outvalue = GetTickCount();

return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSystemUptime() */


/*
 *  GetHrSystemDate
 *    The host's notion of the local date and time of day.
 *    
 *    Gets the value for HrSystemDate.
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
 | hrSystemDate
 | 
 |  ACCESS         SYNTAX
 |  read-write     DateAndTime
 | 
 | "The host's notion of the local date and time of day."
 | 
 | DISCUSSION:
 | 
 | Win32 API function "GetLocalTime" is used to obtain the value returned for a
 | GET on this variable.
 | 
 | Since this variable is "read-write", this implies that the system time can
 | be set by a SET request.  This is done with a Win32 API function
 | "SetLocalTime".
 | 
 | <POA-1> Issuing "SetLocalTime" requires that the issuing process have the
 | "SE_SYSTEMTIME_NAME" privilege which is not the default.  Are there any
 | security concerns about issuing the necessary Win32 API function call
 | "AdjustTokenPrivileges" from inside the SNMP agent to enable setting of
 | the system time?  Any other problems?
 | 
 | RESOLVED >>>>>>>
 | <POA-1> Let's leave this read-only.
 | RESOLVED >>>>>>>
 |
 |========================== From RFC1514 ====================================
 |
 |    DateAndTime ::= OCTET STRING (SIZE (8 | 11))
 |    --        A date-time specification for the local time of day.
 |    --        This data type is intended to provide a consistent
 |    --        method of reporting date information.
 |    --
 |    --            field  octets  contents                  range
 |    --            _____  ______  ________                  _____
 |    --              1      1-2   year                      0..65536
 |    --                           (in network byte order)
 |    --              2       3    month                     1..12
 |    --              3       4    day                       1..31
 |    --              4       5    hour                      0..23
 |    --              5       6    minutes                   0..59
 |    --              6       7    seconds                   0..60
 |    --                           (use 60 for leap-second)
 |    --              7       8    deci-seconds              0..9
 |    --              8       9    direction from UTC        "+" / "-"
 |    --                           (in ascii notation)
 |    --              9      10    hours from UTC            0..11
 |    --             10      11    minutes from UTC          0..59
 |    --
 |    --            Note that if only local time is known, then
 |    --            timezone information (fields 8-10) is not present.
 |
 |============================================================================
 | 1.3.6.1.2.1.25.1.2.0
 |                | |
 |                | *-hrSystemDate
 |                *---hrSystem
 */

UINT
GetHrSystemDate( 
        OUT DateAndTime *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
SYSTEMTIME      localtime;             // Where we retrieve current local time
USHORT          year_networkbyteorder; // Network byte order of year
static
char            octet_time[8];         // OCTET STRING format of "time"

    GetLocalTime(&localtime);

    year_networkbyteorder = htons(localtime.wYear);

    //
    // Format "dateandtime" according to RFC1514
    //
    octet_time[0] = (year_networkbyteorder & 0xFF);
    octet_time[1] = (year_networkbyteorder >> 8);
    octet_time[2] = (char) localtime.wMonth;
    octet_time[3] = (char) localtime.wDay;
    octet_time[4] = (char) localtime.wHour;
    octet_time[5] = (char) localtime.wMinute;
    octet_time[6] = (char) localtime.wSecond;
    octet_time[7] = localtime.wMilliseconds / 100;

    outvalue->length = 8;
    outvalue->string = octet_time;

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSystemDate() */


/*
 *  GetHrSystemInitialLoadDevice
 *    The index of the hrDeviceEntry for the device from which this host is 
 *    configured to load its initial operating system configurat
 *    
 *    Gets the value for HrSystemInitialLoadDevice.
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
 | hrSystemInitialLoadDevice
 | 
 |  ACCESS         SYNTAX
 |  read-write     INTEGER (1..2147483647)
 | 
 | "The index of the hrDeviceEntry for the device from which this host is
 | configured to load its initial operating system configuration."
 | 
 | DISCUSSION:
 | 
 | <POA-2> It seems that RFC1514 is attempting to allow the setting and resetting
 | of the default operating system to be booted thru the combination of this
 | attribute and "hrSystemInitialLoadParameters" (below).
 | 
 | While generic PC hardware typically boots from a hard disk partition that is
 | going to wind up being labelled as drive "C:", these two variables speak to a
 | more general situation in which a hardware bootstrap loader can be set to
 | point to almost any system image on a "permanent file system" (to use the unix
 | phrase).  It is apparent that we must attempt to approximate this in some way.
 | 
 | I note that a system file named "Boot.ini" resides on the bootable partition
 | of an Intel NT system.  The one on my system looks like:
 | 
 | >>>>>>>>
 | [boot loader]
 | timeout=30
 | default=multi(0)disk(0)rdisk(0)partition(1)\WINNT35
 | [operating systems]
 | multi(0)disk(0)rdisk(0)partition(1)\WINNT35="Windows NT Server Version 3.51"
 | multi(0)disk(0)rdisk(0)partition(1)\WINNT35="Windows NT Server Version 3.51 [VGA mode]" /basevideo /sos
 | <<<<<<<<
 | 
 | The contents of this file seems to be the source of the menu that appears
 | during the NT boot process.  Programming a selection from this menu is a close
 | approximation to the flexibility hinted at thru the combination of these
 | two SNMP attributes, "hrSystemInitialLoadDevice" and
 | "hrSystemInitialLoadParameters".
 | 
 | For the purposes of "GET", given the "default" shown in the "Boot.ini" file
 | above, it would seem that the string "multi(0)disk(0)rdisk(0)partition(1)"
 | constitutes a passing resemblance to "hrSystemInitialLoadDevice" while the
 | string "\WINNT35" seems to be a good candidate for the value of
 | "hrSystemInitialLoadParameters" (where both of these are drawn from the value
 | specified for "default" under "[boot loader]").
 | 
 | For the purposes of "SET", we would be required to modify this file according
 | to the values received.  I note that the file on my system is write-protected
 | and clearly this poses a problem if we allow the SNMP agent to attempt to
 | modify it.  Additionally, there is the problem of error-checking any value
 | provided from an SNMP SET request (I do not know the exact semantic
 | significance of "multi(0)disk(0)rdisk(0)partition(1)" and I do not know how
 | to error-check a received value).
 | 
 | RESOLVED >>>>>>>
 | <POA-2> Call GetWindowsDirectory and work backwards from that (get the 
 | device name, e.g. D, then do a QueryDosDevice to get the underlying 
 | partition information).  Much more accurate than trying to parse boot.ini, 
 | and portable to Alpha.  Leave read-only.
 | RESOLVED >>>>>>>
 |============================================================================
 | 1.3.6.1.2.1.25.1.3.0
 |                | |
 |                | *-hrSystemInitialLoadDevice
 |                *---hrSystem
 */

UINT
GetHrSystemInitialLoadDevice( 
        OUT Integer *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
    /*
    | Fetch this magic index from static storage in "HRDEVENT.C".
    */
    *outvalue = InitLoadDev_index;

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSystemInitialLoadDevice() */



/*
 *  SetHrSystemInitialLoadDevice
 *    The index of the hrDeviceEntry for the device from which this host is 
 *    configured to load its initial operating system configurat
 *    
 *    Sets the HrSystemInitialLoadDevice value.
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
SetHrSystemInitialLoadDevice( 
        IN Integer *invalue ,
        OUT Integer *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{

    return SNMP_ERRORSTATUS_NOSUCHNAME ;

} /* end of SetHrSystemInitialLoadDevice() */


/*
 *  GetHrSystemInitialLoadParameters
 *    This object contains the paramets (e.g. a pathname and parameter) 
 *    supplied to the load device when requesting the initial operat
 *    
 *    Gets the value for HrSystemInitialLoadParameters.
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
 | hrSystemInitialLoadParameters
 | 
 |   ACCESS        SYNTAX
 |   read-write    InternationalDisplayString (SIZE (0..128))
 | 
 | "This object contains the parameters (e.g. a pathname and parameter) supplied
 | to the load device when requesting the initial operating system configuration
 | from that device."
 | 
 | DISCUSSION:
 | 
 | (See discussion for "hrSystemInitialLoadDevice" above).
 | 
 | For initial version, we return a zero length string, and it is not SETable.
 |
 |============================================================================
 | 1.3.6.1.2.1.25.1.4.0
 |                | |
 |                | *-hrSystemInitialLoadParameters
 |                *---hrSystem
 */

UINT
GetHrSystemInitialLoadParameters( 
        OUT InternationalDisplayString *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
    // (No parameter-string returned  for initial release)
    outvalue->length = 0;
    outvalue->string = NULL;

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSystemInitialLoadParameters() */


/*
 *  SetHrSystemInitialLoadParameters
 *    This object contains the paramets (e.g. a pathname and parameter) 
 *    supplied to the load device when requesting the initial operat
 *    
 *    Sets the HrSystemInitialLoadParameters value.
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
SetHrSystemInitialLoadParameters( 
        IN InternationalDisplayString *invalue ,
        OUT InternationalDisplayString *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
    return SNMP_ERRORSTATUS_NOSUCHNAME ;

} /* end of SetHrSystemInitialLoadParameters() */

/*
 *  GetHrSystemNumUsers
 *    The number of user sessions for which this host is storing state 
 *    information.
 *    
 *    Gets the value for HrSystemNumUsers.
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
 | hrSystemNumUsers
 | 
 |  ACCESS         SYNTAX
 |  read-only      Gauge
 | 
 | "The number of user sessions for which this host is storing state information.
 | A session is a collection of processes requiring a single act of user
 | authentication and possibly subject to collective job control."
 | 
 | DISCUSSION:
 | 
 | <POA-3> This metric does not seem to be directly available thru a standard
 | Win32 API function.  I note that what appears to be logon information seems to
 | be stored in the registry under:
 | 
 |           "HKEY_LOCAL_MACHINE\microsoft\windows nt\winlogon".
 | 
 | + Should I use this as a source for this SNMP attribute?
 | + If this registry entry is the proper place to acquire this information, what
 |   is the full and proper way to parse this entry?  (By this I mean if more
 |   than one user is logged on, how does this single registry entry reflect
 |   multiple users?)
 | + Does it reflect other users logged in via a network connection?
 | 
 | RESOLVED >>>>>>>
 | <POA-3> There is going to be either one interactive user or none.  Use 
 | NetWkstaGetInfo to try to determine the name of the currently logged on 
 | interactive user and if this succeeds then it is the former.
 | RESOLVED >>>>>>>
 |
 |=============================================================================
 | 1.3.6.1.2.1.25.1.5.0
 |                | |
 |                | *-hrSystemNumUsers
 |                *---hrSystem
 */

UINT
GetHrSystemNumUsers( 
        OUT Gauge *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
    LPBYTE  info;     /* Where we get logged-on information */


    // Attempt to get the number of logged-on users . . . 
    if (NetWkstaGetInfo(NULL,           /* "Local" computer     */
                        102,            /* Info Level 102       */
                        &info) != NERR_Success) {

        return SNMP_ERRORSTATUS_GENERR ;
        }


    /* Just return it */
    *outvalue = ((LPWKSTA_INFO_102) info)->wki102_logged_on_users;

    /* Free the Net API Buffer */
    NetApiBufferFree(info);

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSystemNumUsers() */


/*
 *  GetHrSystemProcesses
 *    The number of process contexts currently loaded or running on this system.
 *    
 *    Gets the value for HrSystemProcesses.
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
 |=============== From WebEnable Design Spec Rev 3 04/11/97==================
 | hrSystemProcesses
 | 
 |  ACCESS         SYNTAX
 |  read-only      Gauge
 | 
 | "The number of process contexts currently loaded or running on this system."
 | 
 | DISCUSSION:
 | 
 | The sample-code "Process Viewer" (PVIEWER.EXE) that is part of the Win32 SDK
 | infers a list of active processes from performance information that is stored
 | in the registry.  We use this approach (and borrowed code) to obtaining a
 | count of active processes.
 |
 |-- (The foregoing approach is abandoned for a direct "beyond-the-veil" call
 |--  to NtQuerySystemInfo()).
 |=============================================================================
 | 1.3.6.1.2.1.25.1.6.0
 |                | |
 |                | *-hrSystemNumUsers
 |                *---hrSystem
 */

UINT
GetHrSystemProcesses( 
        OUT Gauge *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
    ULONG       process_count;

    // If we failed to get the Process Count successfully . . . 
    if ((process_count = Spt_GetProcessCount()) == 0) {
        return SNMP_ERRORSTATUS_GENERR ;
        }

    // Return the value
    *outvalue = process_count;

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSystemProcesses() */


/*
 *  GetHrSystemMaxProcesses
 *    The maximum number of process contexts this system can support.  If there 
 *    is no fixed maximum, the value should be zero.
 *    
 *    Gets the value for HrSystemMaxProcesses.
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
 |=============== From WebEnable Design Spec Rev 3 04/11/97==================
 | hrSystemMaxProcesses
 | 
 |  ACCESS         SYNTAX
 |  read-only      INTEGER (0..2147483647)
 | 
 | "The maximum number of process contexts this system can support.  If there is
 | no fixed maximum, the value should be zero.  On systems that have a fixed
 | maximum, this object can help diagnose failures that occur when this maximum
 | is reached."
 | 
 | DISCUSSION:
 | 
 | My understanding is there is no fixed maximum, as the effective maximum is
 | main-memory dependent.  We return 0 for the value of this attribute.
 | 
 |=============================================================================
 | 1.3.6.1.2.1.25.1.7.0
 |                | |
 |                | *-hrSystemNumUsers
 |                *---hrSystem
 */

UINT
GetHrSystemMaxProcesses( 
        OUT Integer *outvalue ,
        IN Access_Credential *access ,
        IN InstanceName *instance )

{
    *outvalue = 0;

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of GetHrSystemMaxProcesses() */


/*
 *  HrSystemFindInstance
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
HrSystemFindInstance( IN ObjectIdentifier *FullOid ,
                       IN OUT ObjectIdentifier *instance )
{
    UINT tmp_instance ;

    //
    //  Developer instrumentation code to find appropriate instance goes here.
    //  For non-tables, it is not necessary to modify this routine.  However, 
    //  if there is any context that needs to be set, it can be done here.
    //

    if ( FullOid->idLength <= HRSYSTEM_VAR_INDEX )
	// No instance was specified
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else  if ( FullOid->idLength != HRSYSTEM_VAR_INDEX + 1 )
	// Instance length is more than 1
	return SNMP_ERRORSTATUS_NOSUCHNAME ;
    else
	// The only valid instance for a non-table are instance 0.  If this
	// is a non-table, the following code validates the instances.  If this
	// is a table, developer modification is necessary below.

	tmp_instance = FullOid->ids[ HRSYSTEM_VAR_INDEX ] ;
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

} /* end of HrSystemFindInstance() */



/*
 *  HrSystemFindNextInstance
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
HrSystemFindNextInstance( IN ObjectIdentifier *FullOid ,
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

    if ( FullOid->idLength <= HRSYSTEM_VAR_INDEX )
    {
	instance->ids[ 0 ] = 0 ;
	instance->idLength = 1 ;
    }
    else
	return SNMP_ERRORSTATUS_NOSUCHNAME ;

    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of HrSystemFindNextInstance() */



/*
 *  HrSystemConvertInstance
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
HrSystemConvertInstance( IN ObjectIdentifier *oid_spec ,
                          IN OUT InstanceName *native_spec )
{
    //
    //  Developer supplied code to convert instance identifer to native
    //  specification of instance names goes here.
    //

    return SUCCESS ;

} /* end of HrSystemConvertInstance() */




/*
 *  HrSystemFreeInstance
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
HrSystemFreeInstance( IN OUT InstanceName *instance )
{

    //
    //  Developer supplied code to free native representation of instance name goes here.
    //

} /* end of HrSystemFreeInstance() */

