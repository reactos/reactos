/*
 *  gennt.c v0.14  May 15, 1996 
 *
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
 *    SNMP Extension Agent
 *
 *  Abstract:
 *
 *    This module contains the code for dealing with the generic logic for
 *    processing the SNMP request.  It is table driven.  No user modification
 *    should be done.
 *
 *  Functions:
 *
 *    ResolveVarBind()
 *    FindClass()
 *    ResolveGetNext()
 *
 *  Author:
 *    Miriam Amos Nihart, Kathy Faust
 *
 *  Date:
 *      2/17/95
 *
 *  Revision History:
 * 	6/22/95  krw0001  FindClass - modify to stop checking for valid variable - we only care about valid
 *								class.
 *								Rewrite ResolveGetNext
 *  6/26/95  ags      FindClass - stop checking for valid variable
 *                              Rewrite ResolveGetNext
 *  7/31/95  ags      SNMP_oidfree works with CRTDLL.lib, hence use them.
 *  2/14/96  ags   v0.11    one fix for the getnext bug found by Cindy
 *  3/19/96  kff   v0.12    modified for trap support
 *  4/19/96  ags   v0.13    Modified to get rid of trap.c in case of no traps.
 *  5/15/96  cs    v0.14    Modified FindClass in the backward walkthru to tighten
 *                      up the verification
 */


#include <windows.h>
#include <malloc.h>
#include <stdio.h>

#include <snmp.h>

#include "mib.h"
#include "mib_xtrn.h"
#include "smint.h"

extern DWORD dwTimeZero ;

UINT
SnmpUtilOidMatch(AsnObjectIdentifier *pOid1, AsnObjectIdentifier *pOid2)
{
    unsigned long int nScan = min(pOid1->idLength, pOid2->idLength);
    unsigned long int i;

    for (i = 0; i < nScan; i++)
    {
        if (pOid1->ids[i] != pOid2->ids[i])
            break;
    }

    return i;
}



/*
 *  ResolveVarBind
 *
 *    Resolves a single variable binding.  Modifies the variable value pair
 *    on a GET or a GET-NEXT.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    PduAction                  type of request - get, set, or getnext
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
ResolveVarBind( IN OUT RFC1157VarBind *VarBind , // Variable Binding to resolve
                IN UINT PduAction )              // Action specified in PDU

{
    unsigned long int cindex ;  // index into the class info table
    unsigned long int vindex ;  // index into the class's var table
    UINT instance_array[ MAX_STRING_LEN ] ;
    UINT status ;
    UINT result ;               // SNMP PDU error status
    AsnObjectIdentifier instance ;
    InstanceName native_instance ;

    instance.ids = instance_array ;

    // Determine which class the VarBind is for

    status = FindClass( VarBind, &cindex ) ;
    if ( status )
    {
        if ( PduAction != MIB_ACTION_GETNEXT )
        {
            // Check for valid variable as this is a get or set

            CHECK_VARIABLE( VarBind, cindex, vindex, status ) ;
            if ( !status )
                return SNMP_ERRORSTATUS_NOSUCHNAME ;

            // Check for valid instance

            status = ( *class_info[ cindex ].FindInstance )
                     ( (ObjectIdentifier *)&(VarBind->name) ,
                       (ObjectIdentifier *)&instance ) ;
            if ( status != SNMP_ERRORSTATUS_NOERROR )
                return status ;

            // Check for access

            CHECK_ACCESS( cindex, vindex, PduAction, status ) ;
            if ( !status )
            {
                if ( PduAction == MIB_ACTION_SET )
                    return SNMP_ERRORSTATUS_NOTWRITABLE ;
                else
                    return SNMP_ERRORSTATUS_GENERR ;
            }

            // Ok to do the get or set

            if ( PduAction == MIB_ACTION_GET )
            {
                status = ( *class_info[ cindex ].ConvertInstance )
                         ( (ObjectIdentifier *)&instance, &native_instance ) ;
                if ( status == FAILURE )
                    return SNMP_ERRORSTATUS_GENERR ;

                result = ( *class_info[ cindex ].variable[ vindex].SMIGet )
                         ( VarBind , cindex, vindex, &native_instance ) ;
            }
            else
            {
                if ( VarBind->value.asnType !=
                     class_info[ cindex ].variable[ vindex ].type )
                    return SNMP_ERRORSTATUS_BADVALUE ;

                status = ( *class_info[ cindex ].ConvertInstance )
                         ( (ObjectIdentifier *)&instance, &native_instance ) ;
                if ( status == FAILURE )
                    return SNMP_ERRORSTATUS_GENERR ;

                result = ( *class_info[ cindex ].variable[ vindex ].SMISet )
                         ( VarBind, cindex, vindex, &native_instance ) ;
            }
        }
        else  // This is a GETNEXT
        {
            //
            //  Call ResolveGetNext() to determine which class, variable, and
            //  instance to do a Get on.
            //

            status = ResolveGetNext( VarBind, &cindex, &vindex, &instance ) ;
            if ( status == SUCCESS )
            {
                status = ( *class_info[ cindex ].ConvertInstance )
                         ( (ObjectIdentifier *)&instance, &native_instance ) ;
                if ( status == FAILURE )
                    return SNMP_ERRORSTATUS_GENERR ;

                result = ( *class_info[ cindex ].variable[ vindex ].SMIGet )
                         ( VarBind, cindex, vindex, &native_instance ) ;
            }
            else
                return SNMP_ERRORSTATUS_NOSUCHNAME ;
        }

    }
    else
    {
        //
        // No class found, but its a GETNEXT.. we need to find the class that has the longest
        // with the requested oid and forward the request to it
        //

        if (PduAction == MIB_ACTION_GETNEXT)
        {
            unsigned long int ci;               // index into the class info table
            unsigned long int nLongestMatch;    // max number of ids that matched between names
            unsigned long int nCurrentMatch;    // matching number of IDs at the current iteration

            // scan the class_info table, relying on the fact that the table is ordered
            // ordered ascendingly on the class OID.
            for (ci = 0, nLongestMatch = 0; ci < CLASS_TABLE_MAX; ci++)
            {
                // pick up the number of matching ids between the VarBind and the class name..
                nCurrentMatch = SnmpUtilOidMatch(&VarBind->name, class_info[ci].oid);

                // store in cindex the first class with the highest match number
                if (nCurrentMatch > nLongestMatch)
                {
                    cindex = ci;
                    nLongestMatch = nCurrentMatch;
                }
            }


            // only if VarBind name is longer than the match number we need to look
            // for an even better match
            if (VarBind->name.idLength > nLongestMatch)
            {
                for (;cindex < CLASS_TABLE_MAX; cindex++)
                {
                    // make sure we don't go over the range with the longest Match
                    if (SnmpUtilOidMatch(&VarBind->name, class_info[cindex].oid) != nLongestMatch)
                        break;

                    // if the class matches entirely into the VarBind name, check if the first ID
                    // that follows in VarBind name is inside the range supported by the class
                    if (class_info[cindex].oid->idLength == nLongestMatch)
                    {
                        // this is a hack - we rely the var_index is always 1 more than number of ids in
                        // the class_info name. Since VarBind has already a name longer than nLongestMatch
                        // no buffer overrun happens here.
                        // if the VarBind name is in the right range, then we found the class - just break the loop
                        // (don't forget, var_index is '1' based)
                        if(VarBind->name.ids[class_info[cindex].var_index - 1] <= class_info[cindex].max_index)
                            break;
                    }
                    else
                    {
                        // the VarBind name is longer than the IDs that match, the class_info name is the same
                        // the first ID that follows in both names can't be equal, so we can break the loop if
                        // the VarBind name is just in front of it.
                        if (VarBind->name.ids[nLongestMatch] < class_info[cindex].oid->ids[nLongestMatch])
                            break;
                    }

                }
            }

            if (cindex < CLASS_TABLE_MAX )
                   vindex = class_info[cindex].min_index ;
            else
                   return SNMP_ERRORSTATUS_NOSUCHNAME ;

 			SNMP_oidfree( &VarBind->name ) ;
            SNMP_oidcpy( &VarBind->name ,
                         class_info[ cindex ].variable[ vindex ].oid ) ;
            status = ResolveGetNext( VarBind, &cindex, &vindex, &instance ) ;
            if ( status == SUCCESS )
            {
                status = ( *class_info[ cindex ].ConvertInstance )
                         ( (ObjectIdentifier *)&instance, &native_instance ) ;
                if ( status == FAILURE )
                    return SNMP_ERRORSTATUS_GENERR ;

                result = ( *class_info[ cindex ].variable[ vindex ].SMIGet )
                         ( VarBind, cindex, vindex, &native_instance ) ;
            }
        }
        else
            return SNMP_ERRORSTATUS_NOSUCHNAME ;
    }

    ( *class_info[ cindex ].FreeInstance )( &native_instance ) ;
    return result ;

} /* end of ResolveVarBind() */



/*
 *  FindClass
 *
 *    This routine determines the class by walking the class_info table
 *    backwards and comparing the class oids.  The table is walked
 *    backwards because it assumes that the classes are listed in
 *    increasing order.  For example,
 *
 *    Group Name              Group Identifier
 *
 *    group1                  1.3.6.1.4.1.36.2.78
 *    table1                  1.3.6.1.4.1.36.2.78.9
 *    table2                  1.3.6.1.4.1.36.2.78.10
 *
 *    We need to look for the longest exact match on the oid thus we
 *    walk the table backwards.
 *
 *  Arguments:
 *
 *     VarBind                 Variable value pair
 *     class                   Index into the class_info
 *
 *  Return Codes:
 *
 *     SUCCESS                 Class is valid, return index into class_info
 *     FAILURE                 Invalid class
 *
 */

UINT
FindClass( IN RFC1157VarBind *VarBind ,
           IN OUT UINT *cindex )
{
    int index ;
    UINT status, vindex ;
    UINT length ;

    for ( index = CLASS_TABLE_MAX - 1 ; index >= 0 ; index-- )
    {
        if ( class_info[ index ].table )
            // skip over the entry code -- kkf, why?
//            length = class_info[ index ].var_index - 2 ;
            length = class_info[ index ].var_index - 1 ;
        else
            length = class_info[ index ].var_index - 1 ;
        status = SNMP_oidncmp( &VarBind->name ,
                               class_info[ index ].oid ,
                               length ) ;

        // if the oid don't match the class or it is shorter than the
        // class go on to the next one.
        // If the oid requested is shorter than the class we can't stop
        // otherwise we'll point to a wrong (longest match) class.
        if (status != 0 ||
            VarBind->name.idLength < class_info[ index ].var_index)
            continue;

        vindex = VarBind->name.ids[ class_info[ index ].var_index - 1 ] ;
        // cs - added the vindex verification to make sure that the varbind
        // oid definitely belongs in this class (fixed partial table oids)
		
        if ( vindex >= class_info[ index ].min_index &&
             vindex <= class_info[ index ].max_index)
        {
            *cindex = index ;
            return SUCCESS ;
        }
    }

	//  Failed to match by walking list backwards (longest match)
	//  so OID supplied is shorter than expected (e.g., partial OID supplied)
	//  Try matching by forward walking...
	for (index = 0; index < CLASS_TABLE_MAX; index++ ) {
		status = SNMP_oidncmp( &VarBind->name ,
		                       class_info[ index ].oid ,
							   VarBind->name.idLength ) ;
		if ( status == 0 ) {
			*cindex = index ;
			return SUCCESS ;
		}
	}

    return FAILURE ;

} /* end of FindClass() */



/*
 *   ResolveGetNext
 *
 *     Determines the class, the variable and the instance that the
 *     GetNext request is to be performed on.  This is a recursive
 *     routine.  The input arguments VarBind and class may be modified
 *     as part of the resolution.
 *
 *     The rules for getnext are:
 *       1. No instance and no variable specified so return the first
 *          variable for the first instance.
 *       2. No instance specified but a variable is specified so return
 *          the variable for the first instance.
 *       3. An instance and a variable are specified
 *              Follow 3a,4b for  Non Tables
 *              Follow 3b, 4b, 5b for  Tables
 *
 *       3a.Return the next	variable for the instance.
 *       4a.An instance and a variable are specified but the variable is the
 *          last variable in the group so return the first variable for the
 *          next group.
 *          If there is no next group return FAILURE.
 *
 *       3b. Return the variable for the next instance ( walk down the column).
 *       4b. Reached the bottom of the column, start at the top of next column.
 *       5b. An instance and a variable are specified but it is the last 
 *          variable and the last instace so roll to the next group (class).
 *          If there is no next group return FAILURE.
 *
 *  Arguments:
 *
 *     VarBind                 Variable value pair
 *     cindex                  Index into the class_info
 *     vindex                  address to specify variable for the get
 *
 *  Return Codes:
 *
 *     SUCCESS                 Able to resolve the request to a class, variable
 *                             and instance
 *     FAILURE                 Unable to resolve the request within this  MIB
 *
 */

UINT
ResolveGetNext( IN OUT RFC1157VarBind *VarBind ,
                IN OUT UINT *cindex ,
                IN OUT UINT *vindex ,
                OUT AsnObjectIdentifier *instance )
                {
    UINT status ;
    access_mode_t tmpAccess ;

	
	/*
	 * We've come in with a pointer to the class, to start with
     * Do we have a variable specified?
     */
	
	*vindex = 0 ;
	if (VarBind->name.idLength < class_info[ *cindex ].var_index )  {
        /*
       	 * No variable specified. so pick the first variable (if it exists)
       	 * to start the search for a valid variable.
         * If not roll over to the next class.
    	 * Instnace is 0 for non Tables, and the first instance for Tables.
         */

	    if ( class_info[ *cindex ].min_index <= class_info[ *cindex ].max_index)   {

            *vindex = class_info[ *cindex ].min_index ;
            goto StartSearchAt;
        } else  {
            goto BumpClass;
        }

    } else {
        /*
         * Yes, a variable is specified.
         * If it is below min_index, start testing for a valid variable at the min_index.
         * If it is ablove max_index roll over to the next class.
         * If we change the variable, Instance is reset to the first (or the only) Instance.
         */
        *vindex = VarBind->name.ids[ class_info[ *cindex ].var_index - 1 ] ;
		
		if ( *vindex < class_info[ *cindex ].min_index) {
            *vindex = class_info[ *cindex ].min_index ;
            goto StartSearchAt;
		}
		
		if ( *vindex > class_info[ *cindex ].max_index)
            goto BumpClass;
        /*
         * A valid variable for this class is specified. Table & NonTable are treated
         * differently.
         * In case of Non Tables:
         *      if instance is specified, we start the serach for a valid variable at the
         *              next variable.
         *      if no instnace is specified, we start search at the specified variable.
         *
         * In case of Tables:
         * We may have
         *      a. No Instance              start at the 1st Instance
         *      b. Partial instance         start at the 1st Instance
         *      c. Invalid instance         start at the 1st Instance
         *      d. Valid Instance           start at the next Instance
		 * All these cases will be handled by the FindNextInstance
	     * Hence first check that access of the given vaiable, if it is readable
         * get the Next Instance. If not start the search for a valid variable at the next
         * variable.
         */

        if ( class_info[ *cindex ].table == NON_TABLE ) {
            /* test for the Instance */
			if ( VarBind->name.idLength > class_info[ *cindex ].var_index)
                (*vindex)++ ;

            goto StartSearchAt;
        } else {
            /* Start Table case */
            tmpAccess =  class_info[ *cindex ].variable[ *vindex ].access_mode ;
            if ( ( tmpAccess == NSM_READ_ONLY ) || (tmpAccess == NSM_READ_WRITE) ) {
                /*
                 * readable Variable,  walk down the column
                 */
                status = ( *class_info[ *cindex ].FindNextInstance )
					    ( (ObjectIdentifier *)&(VarBind->name) ,
					    (ObjectIdentifier *)instance ) ;
			
			    if (status == SNMP_ERRORSTATUS_NOERROR) { 		   	
                    SNMP_oidfree ( &VarBind->name ) ;
				    SNMP_oidcpy ( &VarBind->name,
				                class_info[*cindex ].variable[*vindex].oid ) ;
				    SNMP_oidappend ( &VarBind->name, instance );
				    return SUCCESS ;                   
                    /* we are all done   */
                }
            }
            /*
             * Either at end of the column, or variable specified is non Readable.
             * Hence we need to move to the next column,
             * This means we start at the 1st instnace.
             */
            (*vindex)++ ;
            goto StartSearchAt;
            /* End Table case */
        }
        /* end of variable specified case */
    }
StartSearchAt:
    /*
     * We have a start variable in *vindex.
     * At this point we are moving to the next column in case of a Table
     * Hence if we can't find an NextInstance ( empty Table), move to the
     * next class.
     */
     status = FAILURE;
     while ( *vindex <= class_info[ *cindex ].max_index)  {

        tmpAccess =  class_info[ *cindex ].variable[ *vindex ].access_mode ;
        if ( ( tmpAccess == NSM_READ_ONLY ) || (tmpAccess == NSM_READ_WRITE) ) {
            status = SUCCESS;
            break;
        } else  {
            (*vindex)++;
        }
     }

     if ( status == SUCCESS) {
        /*
         * we hava a valid variable, get the instance
         */
        SNMP_oidfree ( &VarBind->name ) ;
		SNMP_oidcpy ( &VarBind->name, class_info[ *cindex ].variable[*vindex ].oid );

		if ( class_info[ *cindex ].table == NON_TABLE) {
		
		    instance->ids[ 0 ] = 0 ;
			instance->idLength = 1 ;
			SNMP_oidappend ( &VarBind->name, instance );
			return SUCCESS ;

        } else {

            status = ( *class_info[ *cindex ].FindNextInstance )
					    ( (ObjectIdentifier *)&(VarBind->name) ,
					    (ObjectIdentifier *)instance ) ;
			
			if (status == SNMP_ERRORSTATUS_NOERROR) { 		   	
		        SNMP_oidappend ( &VarBind->name, instance );
				return SUCCESS ;
            }
        }
     }

/*
 * Come here to move on to the next class
 */

BumpClass:
    {
	    (*cindex)++ ;
		if ( *cindex >= CLASS_TABLE_MAX)
			return FAILURE ;
		SNMP_oidfree( &VarBind->name );
		SNMP_oidcpy ( &VarBind->name, class_info[ *cindex ].oid );
		status = ResolveGetNext( VarBind, cindex, vindex, instance) ;
		return status ;
	}
	
	// This oughtn't to happen
	return FAILURE ;
} /* end of ResolveGetNext() */


#ifndef TRAPS
//
// If there are no traps, TrapInit() is still needed. 
// If there are traps, all this code appears in 
// generated file trap.c
//

UINT number_of_traps = 0 ;

trap_t
    trap_info[] = {
        { NULL, 0, 0, 0, NULL }
} ;

extern
trap_t trap_info[] ;

extern
UINT number_of_traps ;

extern HANDLE hEnabledTraps ;
extern HANDLE hTrapQMutex ;

/*
 *  TrapInit
 *
 *    This routine initializes the trap handle.
 *
 *  Arguments:
 *
 *    hPollForTrapEvent    handle for traps - this is used to coordinate
 *                         between the Extendible Agent and this Extension
 *                         Agent.
 *                             - NULL indicates no traps
 *                             - value from CreateEvent() indicates traps
 *                               are implemented and the Extendible agent
 *                               must poll for them
 *
 *  Return Code:
 *
 *    SUCCESS     Successful initialization
 *    FAILURE     Unable to initialize
 *
 |=========================================================================
 | There are no Traps associated with the HostMIB.  Consequently this
 | routine is taken over and used to create a handle to a timer rather
 | than an event.
 |
 | We want to be entered at "SnmpExtensionTrap()" (in "HOSTMIB.C") on
 | a periodic interval.  When entered, we won't really do any trap processing,
 | instead we'll refresh the cached information associated with SNMP
 | attribute "hrProcessorLoad" (in "HRPROCES.C") thru a call to function
 | "hrProcessLoad_Refresh()" (also in "HRPROCES.C").
 |
 | So the contents of this standard function is replaced.  (Note that the
 | "hTrapQMutex" is no longer created).
 */

VOID
TrapInit( IN OUT HANDLE *hPollForTrapEvent )
{
#if 0
    // The default value for traps is NULL indicating NO traps.

    *hPollForTrapEvent = NULL ;
    hTrapQMutex = NULL ;

    // Call to CreateEvent uses the default security descriptor (therefore
    // the handle is not inheritable), flags auto reset (no call to ResetEvent()
    // required), flags no signal to be sent at the initial state, and does
    // not specify a name for this event.
    //
    // If the CreateEvent() fails the value returned is NULL so traps
    // are not enabled.  Otherwise the setting of this event with will cause
    // the Extendible Agent to call this Extension Agent's SnmpExtensionTrap
    // routine to collect any traps.

    *hPollForTrapEvent = CreateEvent( NULL ,   // Address of security attrib
                                      FALSE ,  // Flag for manual-reset event
                                      FALSE ,  // Flag for initial state 
                                      NULL ) ; // Address of event-object name

    //
    // Save the handle in a global variable for use later in setting a trap.
    //

    hEnabledTraps = *hPollForTrapEvent ;

    //
    //  Create Mutex for assuring single thread access to enque/dequeue on trap_q
    hTrapQMutex = CreateMutex( NULL,  // Address of security attrib
                               FALSE, // Mutex is not initially owned
			       NULL ) ; // Mutex is unnamed

    return ;
#endif
/*
|========================
| Special HostMIB code:
*/
LARGE_INTEGER   due_time;       /* When the timer first goes off */
LONG            period;         /* Frequency: every minute       */
BOOL            waitable;       /* Status from SetWaitable()     */


    *hPollForTrapEvent = NULL ;

    /* Attempt the creation of a waitable timer . . . */
    *hPollForTrapEvent = CreateWaitableTimer(NULL,      // Security
                                             FALSE,     // = Auto-resetting
                                             NULL       // = No name
                                             );

    /*
    | Set a negative due time to mean "relative": We want it to go off
    | in 30 seconds.  Ticks are 100 ns or 1/10th of a millionth of a second.
    |
    */
    due_time.QuadPart = 10000000 * (-30);

    /*
    | Set the period in milliseconds to 1 minute.
    */
    period = 1000 * 60;

    /* If we actually managed to create it, start it */
    if (*hPollForTrapEvent != NULL) {

        waitable = 
            SetWaitableTimer(*hPollForTrapEvent,    // Handle to timer
                             &due_time,             // "Due Time" to go off
                             period,                // Length of period in ms.
                             NULL,                  // no completion routine
                             NULL,                  // no arg to comp. routine
                             FALSE                  // no power-resume in NT
                             );
        }

} /* end of TrapInit() */

#endif /* #ifndef TRAPS */
