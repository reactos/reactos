/*
 *  gendll.c v0.11   March 14, 1996
 *
 ****************************************************************************
 *                                                                          *
 *      (C) Copyright 1995, 1996 DIGITAL EQUIPMENT CORPORATION              *
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
 *    SNMP Extension Agent
 *
 *  Abstract:
 *
 *    This module contains the code for plugging into the Windows NT Extendible
 *    Agent.  It contains the dll's main routine and the three exported SNMP
 *    routines.
 *
 *  Functions:
 *
 *    DllMain()
 *    SnmpExtensionInit()
 *    SnmpExtensionTrap()
 *    SnmpExtensionQuery()
 *
 *  Author:
 *    Miriam Amos Nihart, Kathy Faust
 *
 *  Date:
 *    2/17/95
 *
 *  Revision History:
 *    6/26/95 KKF, register using Subroot_oid
 *    7/31/95 ags  readded above code + uncommented calls to SNMP_oidfree
 *    3/14/96 kkf, modified SNMPExtensionTrap
 */



// General notes:
//
// Microsoft's Extendible Agent for Windows NT is implemented by dynamically
// linking to Extension Agent DLLs that implement portions of the MIB.  These
// Extension Agents are configured in the Windows NT Registration Database.
// When the Extendible Agent Service is started, it queries the registry to
// determine which Extension Agent DLLs have been installed and need to be
// loaded and initialized.  The Extendible Agent invokes various DLL entry
// points (examples follow in this file) to request MIB queries and obtain
// Extension Agent generated traps.


// Necessary includes.

#include <windows.h>
#include <malloc.h>
#include <stdio.h>

#include <snmp.h>

//
// The file mib.h is a user supplied header file (could be from a code
// generator).  This file should contain the definition, macros, forward
// declarations, etc.  The file mib_xtrn.h contains the external
// declarations for the variable tables and classes composing this MIB.
//

#include "mib.h"
#include "mib_xtrn.h"


// Extension Agent DLLs need access to the elapsed time that the agent has
// been active.  This is implemented by initializing the Extension Agent
// with a time zero reference, and allowing the agent to compute elapsed
// time by subtracting the time zero reference from the current system time.
// This example Extension Agent implements this reference with dwTimeZero.

DWORD dwTimeZero = 0 ;
extern DWORD dwTimeZero ;

//
// Trap Queue
//

q_hdr_t trap_q = { NULL, NULL } ;

extern q_hdr_t trap_q ;  // make it global
HANDLE hTrapQMutex ;
extern HANDLE hTrapQMutex ;
HANDLE hEnabledTraps ;
extern HANDLE hEnabledTraps ;



/*
 *  DllMain
 *
 *    This is a standard Win32 DLL entry point.  See the Win32 SDK for more
 *    information on its arguments and their meanings.  This example DLL does
 *    not perform any special actions using this mechanism.
 *
 *  Arguments:
 *
 *  Results:
 *
 *  Side Effects:
 *
 */

BOOL WINAPI
DllMain( HANDLE hDll ,
         DWORD  dwReason ,
         LPVOID lpReserved )
{

    switch( dwReason )
    {
        case DLL_PROCESS_ATTACH :
        case DLL_PROCESS_DETACH :
        case DLL_THREAD_ATTACH :
        case DLL_THREAD_DETACH :
        default :
            break ;

    }

    return TRUE ;

} /* end of DllMain() (the DllEntryPoint) */



/*
 *  SnmpExtensionInit
 *
 *    Extension Agent DLLs provide the following entry point to coordinate the
 *    initializations of the Extension Agent and the Extendible Agent.  The
 *    Extendible Agent provides the Extension Agent with a time zero reference;
 *    and the Extension Agent provides the Extendible Agent with an Event
 *    handle for communicating occurence of traps, and an object identifier
 *    representing the root of the MIB subtree that the Extension Agent
 *    supports.
 *
 *    Traps support is determined by the user in the UserMibInit() routine.
 *    If a valid handle from CreateEvent() is returned in the argument
 *    hPollForTrapEvent, then traps have been implemented and the SNMP
 *    Extendible Agent will poll this Extension Agent to retrieve the traps
 *    upon notification through the SetEvent() routine.  Polling is done
 *    through the SnmpExtensionTrap() routine.  If NULL is returned in the
 *    argument hPollForTrapEvent, there are no traps.
 *
 *  Arguments:
 *
 *  Results:
 *
 */

BOOL WINAPI
SnmpExtensionInit( IN DWORD dwTimeZeroReference ,
                   OUT HANDLE *hPollForTrapEvent ,
                   OUT AsnObjectIdentifier *supportedView )
{

    // Record the time reference provided by the Extendible Agent.

    dwTimeZero = dwTimeZeroReference ;


    // Indicate the MIB view supported by this Extension Agent, an object
    // identifier representing the sub root of the MIB that is supported.

    *supportedView = Subroot_oid ; // NOTE! structure copy
//    *supportedView = *(class_info[ 0 ].oid) ; // NOTE! structure copy

    // Call the User's initialization routine

    if ( !UserMibInit( hPollForTrapEvent ) )
        return FALSE ;

    // Indicate that Extension Agent initialization was successful.

    return TRUE ;

} /* end of SnmpExtensionInit() */



/*
 *  SnmpExtensionTrap
 *
 *    Extension Agent DLLs provide the following entry point to communicate
 *    traps to the Extendible Agent.  The Extendible Agent will query this
 *    entry point when the trap Event (supplied at initialization time) is
 *    asserted, which indicates that zero or more traps may have occured.
 *    The Extendible Agent will repetedly call this entry point until FALSE
 *    is returned, indicating that all outstanding traps have been processed.
 *
 *  Arguments:
 *
 *  Results:
 *
 |=========================================================================
 | There are no Traps associated with the HostMIB.  Consequently this
 | routine is taken over and used to refresh the cached information
 | associated with SNMP attribute "hrProcessorLoad" (in "HRPROCES.C") thru
 | a call to function "hrProcessLoad_Refresh()" (also in "HRPROCES.C").
 |
 | This function is being entered because a timer has expired that was
 | initialized by code in "TrapInit()" (in "GENNT.C").  The timer automatically
 | resets itself.
 |
 | All the standard generated code is subsumed by a simple call to
 | "hrProcessLoad_Refresh()".
 */

BOOL WINAPI
SnmpExtensionTrap( OUT AsnObjectIdentifier *enterprise ,
                   OUT AsnInteger *genericTrap ,
                   OUT AsnInteger *specificTrap ,
                   OUT AsnTimeticks *timeStamp ,
                   OUT RFC1157VarBindList *variableBindings )
{
#if 0
    tcb_t *entry ;

    // Traps are process by processing the traps on the trap queue.
    // The Extendible Agent will call this routine upon the receipt of
    // the event on the handle passed back in the SnmpExtensionInit routine.
    // The Extendible Agent calls this return back as long as this routine
    // returns true.

    // acquire mutex for trap_q
    WaitForSingleObject( hTrapQMutex, INFINITE ) ;

    // Dequeue a trap entry

    QUEUE_REMOVE( trap_q, entry ) ;

    // release the mutex for trap_q
    ReleaseMutex( hTrapQMutex ) ;

    if (entry == NULL)
       return FALSE ;

    *enterprise = entry->enterprise ;  // note structure copy
    *genericTrap = entry->genericTrap ;
    *specificTrap = entry->specificTrap ;
    *timeStamp = entry->timeStamp ;
    *variableBindings = entry->varBindList ; // note structure copy
    free(entry) ;
    return TRUE ;
#endif

/*
|========================
| Special HostMIB code:
*/
    /* Re-fetch CPU statistics from kernel */
    hrProcessLoad_Refresh();

    /* Don't call again until the timer goes off again */
    return FALSE;

} /* end of SnmpExtensionTrap() */



/*
 *  SnmpExtensionQuery
 *
 *    Extension Agent DLLs provide the following entry point to resolve queries
 *    for MIB variables in their supported MIB view (supplied at initialization
 *    time).  The requestType is Get/GetNext/Set.
 *
 *  Arguments:
 *
 *  Results:
 *
 */


BOOL WINAPI
SnmpExtensionQuery( IN BYTE requestType ,
                    IN OUT RFC1157VarBindList *variableBindings ,
                    OUT AsnInteger *errorStatus ,
                    OUT AsnInteger *errorIndex )
{
    UINT index ;
    UINT *tmp ;


    // Iterate through the variable bindings list to resolve individual
    // variable bindings.

    for ( index = 0 ; index < variableBindings->len ; index++ )
    {
        *errorStatus = ResolveVarBind( &variableBindings->list[ index ] ,
                                       requestType ) ;


        // Test and handle case where Get Next past end of MIB view supported
        // by this Extension Agent occurs.  Special processing is required to
        // communicate this situation to the Extendible Agent so it can take
        // appropriate action, possibly querying other Extension Agents.

        if ( *errorStatus == SNMP_ERRORSTATUS_NOSUCHNAME &&
             requestType == MIB_ACTION_GETNEXT )
        {
            *errorStatus = SNMP_ERRORSTATUS_NOERROR ;


            // Modify variable binding of such variables so the OID points
            // just outside the MIB view supported by this Extension Agent.
            // The Extendible Agent tests for this, and takes appropriate
            // action.

            SNMP_oidfree( &variableBindings->list[ index ].name ) ;
            SNMP_oidcpy( &variableBindings->list[ index ].name, &Subroot_oid  ) ;
            tmp = variableBindings->list[ index ].name.ids ;
            (tmp[ SUBROOT_LENGTH - 1 ])++ ;
        }


        // If an error was indicated, communicate error status and error
        // index to the Extendible Agent.  The Extendible Agent will ensure
        // that the origional variable bindings are returned in the response
        // packet.

        if ( *errorStatus != SNMP_ERRORSTATUS_NOERROR )
        {
            *errorIndex = index + 1 ;
            goto Exit ;
        }
    }

Exit:

    // Indicate that Extension Agent processing was sucessfull.

    return SNMPAPI_NOERROR ;

} /* end of SnmpExtensionQuery() */

/* end of gendll.c */
