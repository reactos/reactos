/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    snmpctrl.c

Abstract:

    Provides service control functionality for Proxy Agent.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <windows.h>

#include <process.h>
#include <stdio.h>
#include <string.h>

#include <winsvc.h>


//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "snmpctrl.h"


//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------


// OPENISSUE - danhi says that it was not known that os/2 allows user
// OPENISSUE - defined service controls, they are now not functional in
// OPENISSUE - NT (they were functional in build 258 and earlier).


INT __cdecl main(
    IN int  argc,     //argument count
    IN char *argv[])  //argument vector
    {
    SC_HANDLE hSCManager;
    SC_HANDLE hService;
    DWORD     dwControl;
    SERVICE_STATUS ServiceStatus;
    INT       loglevel = -1;
    INT       logtype  = -1;
    INT       stdctrl  = -1;

    if      (argc == 1)
        {
        printf("Error:  No arguments specified.\n", *argv);
        printf("\nusage:  snmpctrl [/loglevel:<level>] [/logtype:<type>]\n");

        return 1;
        }

    while(--argc)
        {
        INT temp;

        argv++;

        if      (1 == sscanf(*argv, "/loglevel:%d", &temp))
            {
            loglevel = temp;

            if (loglevel < SNMP_SERVICE_LOGLEVEL_MIN ||
                SNMP_SERVICE_LOGLEVEL_MAX < loglevel)
                {
                printf("Error:  LogLevel must be %d thru %d.\n",
                    SNMP_SERVICE_LOGLEVEL_MIN, SNMP_SERVICE_LOGLEVEL_MAX);
                exit(1);
                }
            }
        else if (1 == sscanf(*argv, "/logtype:%d", &temp))
            {
            logtype = temp;

            if (logtype < SNMP_SERVICE_LOGTYPE_MIN ||
                SNMP_SERVICE_LOGTYPE_MAX < logtype)
                {
                printf("Error:  LogType must be %d thru %d.\n",
                    SNMP_SERVICE_LOGTYPE_MIN, SNMP_SERVICE_LOGTYPE_MAX);
                exit(1);
                }
            }
        else if (1 == sscanf(*argv, "/service_control:%d", &temp))
            {
            stdctrl = temp;

            if (stdctrl != SERVICE_CONTROL_STOP &&
                stdctrl != SERVICE_CONTROL_PAUSE &&
                stdctrl != SERVICE_CONTROL_CONTINUE &&
                stdctrl != SERVICE_CONTROL_INTERROGATE)
                {
                printf("Error:  Service_Control invalid.\n");
                exit(1);
                }
            }
        else
            {
            printf("Error:  Argument %s is invalid.\n", *argv);
            printf("\nusage:  snmpctrl [/loglevel:<level>] [/logtype:<type>]\n");

            return 1;
            }
        } // end while()

    if      ((hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT))
             == NULL)
        {
        printf("error on ConnectSCManager %d\n", GetLastError());

        return 1;
        }
    else if ((hService = OpenService(hSCManager, "snmp",
                                     SERVICE_ALL_ACCESS))
        == NULL)
        {
        printf("error on OpenService %d\n", GetLastError());

        return 1;
        }

    if (loglevel >= 0)
        {
        dwControl = loglevel + SNMP_SERVICE_LOGLEVEL_BASE;

        if (!ControlService(hService, dwControl, &ServiceStatus))
            {
            printf("error on ControlService %d\n", GetLastError());

            return 1;
            }
        else
            printf("Service LogLevel changed to %d.\n", loglevel);
        }

    if (logtype >= 0)
        {
        dwControl = logtype  + SNMP_SERVICE_LOGTYPE_BASE;

        if (!ControlService(hService, dwControl, &ServiceStatus))
            {
            printf("error on ControlService %d\n", GetLastError());

            return 1;
            }
        else
            printf("Service LogType changed to %d.\n", logtype);
        }

    if (stdctrl >= 0)
        {
        dwControl = stdctrl;

        if (!ControlService(hService, dwControl, &ServiceStatus))
            {
            printf("error on ControlService %d\n", GetLastError());

            return 1;
            }
        else
            printf("Service control %d issued.\n", stdctrl);
        }

    return 0;

    } // end main()


//-------------------------------- END --------------------------------------

