/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    servtrap.c

Abstract:

    Provides trap functionality for Proxy Agent.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <windows.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <snmputil.h>

#include "regconf.h"


//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

HANDLE g_hTerminationEvent;


//--------------------------- PRIVATE CONSTANTS -----------------------------

#define TTWFMOTimeout ((DWORD)300000)


//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

VOID trapThread(VOID *threadParam)
    {
    INT    eventListSize      = 0;
    HANDLE *eventList         = NULL;
    INT    *eventListRegIndex = NULL;
    DWORD  status;
    INT    i;

    AsnObjectIdentifier enterprise;
    AsnInteger          genericTrap;
    AsnInteger          specificTrap;
    AsnInteger          timeStamp;
    RFC1157VarBindList  variableBindings;

    UNREFERENCED_PARAMETER(threadParam);

    // add this thread's terminate event to the list...

    if ((eventList = (HANDLE *)SnmpUtilMemReAlloc(eventList, sizeof(HANDLE))) == NULL)
        {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: TRAP: out of memory.\n"));

        goto longBreak;
        }

    if ((eventListRegIndex = (INT *)SnmpUtilMemReAlloc(eventListRegIndex, sizeof(INT)))
        == NULL)
        {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: TRAP: out of memory.\n"));

        goto longBreak;
        }

    eventList[eventListSize]           = g_hTerminationEvent;
    eventListRegIndex[eventListSize++] = -1; // not really used


    // add trap events for extension agents that have provided an event...

    for (i=0; i<extAgentsLen; i++)
        {
        if (extAgents[i].hPollForTrapEvent != NULL &&
            extAgents[i].fInitedOk)
            {
            if ((eventList = (HANDLE *)SnmpUtilMemReAlloc(eventList, (eventListSize+1)*sizeof(HANDLE)))
                == NULL)
                {
                SNMPDBG((SNMP_LOG_ERROR, "SNMP: TRAP: out of memory.\n"));

                goto longBreak;
                }

            if ((eventListRegIndex = (INT *)SnmpUtilMemReAlloc(eventListRegIndex,
                (eventListSize+1)*sizeof(INT))) == NULL)
                {
                SNMPDBG((SNMP_LOG_ERROR, "SNMP: TRAP: out of memory.\n"));

                goto longBreak;
                }

            eventList[eventListSize]           = extAgents[i].hPollForTrapEvent;
            eventListRegIndex[eventListSize++] = i;
            }
        } // end for()


    // perform normal processing...

    while(1)
        {
        if      ((status = WaitForMultipleObjects(eventListSize, eventList,
                 FALSE, TTWFMOTimeout)) == 0xffffffff)
            {
            SNMPDBG((SNMP_LOG_ERROR, "SNMP: TRAP: error %d waiting for trap event list.\n", GetLastError()));

            goto longBreak;
            }
        else if (status == WAIT_TIMEOUT)
            {
            SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: timeout waiting for trap event list.\n"));

            continue;
            }

        // the service will set event 0 in the event list when it wants
        // this thread to terminate.

        if (status == 0)
            {
            SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: termination event set.\n"));
            break; // if g_hTerminationEvent, then exit
            }

        SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: polling %s for traps.\n", extAgents[eventListRegIndex[status]].pathName));

        // call snmpextensiontrap entry of appropriate extension dll...

        while ((*extAgents[eventListRegIndex[status]].trapAddr)(&enterprise,
                &genericTrap, &specificTrap, &timeStamp, &variableBindings))
            {
            if (!SnmpSvcGenerateTrap(&enterprise, genericTrap, specificTrap,
                                         timeStamp, &variableBindings))
                {
                SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: error %d generating trap.\n", GetLastError()));

                //not a serious error.
                }
            }

        } // end while()

longBreak:

    if (eventList)         SnmpUtilMemFree(eventList);
    if (eventListRegIndex) SnmpUtilMemFree(eventListRegIndex);

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: TRAP: agentTrapThread exiting.\n"));

    } // end trapThread()


//-------------------------------- END --------------------------------------

