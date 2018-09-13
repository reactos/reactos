/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    snmptst4.c

Abstract:

    Driver routine to invoke an test the Extension Agent DLL.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>


//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <snmputil.h>
#include <authapi.h>

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

typedef AsnObjectIdentifier View; // temp until view is defined...

int __cdecl main(
    IN int  argumentCount,
    IN char *argumentVector[])
    {
    HANDLE  hExtension;
    FARPROC initAddr;
    FARPROC queryAddr;
    FARPROC trapAddr;

    DWORD  timeZeroReference;
    HANDLE hPollForTrapEvent;
    View   supportedView;

    INT i;
    INT numQueries = 10;
    UINT typ;

    extern INT nLogLevel;
    extern INT nLogType;

    nLogLevel = 15;
    nLogType  = 1;

    // avoid compiler warning...
    UNREFERENCED_PARAMETER(argumentCount);
    UNREFERENCED_PARAMETER(argumentVector);

    timeZeroReference = GetCurrentTime()/10;

    // load the extension agent dll and resolve the entry points...
    if (GetModuleHandle("lmmib2.dll") == NULL)
        {
        if ((hExtension = LoadLibrary("lmmib2.dll")) == NULL)
            {
            dbgprintf(1, "error on LoadLibrary %d\n", GetLastError());

            }
        else if ((initAddr = GetProcAddress(hExtension,
                 "SnmpExtensionInit")) == NULL)
            {
            dbgprintf(1, "error on GetProcAddress %d\n", GetLastError());
            }
        else if ((queryAddr = GetProcAddress(hExtension,
                 "SnmpExtensionQuery")) == NULL)
            {
            dbgprintf(1, "error on GetProcAddress %d\n",
                              GetLastError());

            }
        else if ((trapAddr = GetProcAddress(hExtension,
                 "SnmpExtensionTrap")) == NULL)
            {
            dbgprintf(1, "error on GetProcAddress %d\n",
                      GetLastError());

            }
        else
            {
            // initialize the extension agent via its init entry point...
            (*initAddr)(
                timeZeroReference,
                &hPollForTrapEvent,
                &supportedView);
            }
        } // end if (Already loaded)

    // create a trap thread to respond to traps from the extension agent...

    //rather than oomplicate this test routine, will poll for these events
    //below.  normally this would be done by another thread in the extendible
    //agent.


    // loop here doing repetitive extension agent get queries...
    // poll for potential traps each iteration (see note above)...

    //block...
         {
         RFC1157VarBindList varBinds;
         AsnInteger         errorStatus;
         AsnInteger         errorIndex;
         UINT OID_Prefix[] = { 1, 3, 6, 1, 4, 1, 77 };
         AsnObjectIdentifier MIB_OidPrefix = { 7, OID_Prefix };


	 errorStatus = 0;
	 errorIndex  = 0;
         varBinds.list = (RFC1157VarBind *)SnmpUtilMemAlloc( sizeof(RFC1157VarBind) );
         varBinds.len = 1;
         varBinds.list[0].name.idLength = MIB_OidPrefix.idLength;
         varBinds.list[0].name.ids = (UINT *)SnmpUtilMemAlloc( sizeof(UINT) *
                                               varBinds.list[0].name.idLength );
         memcpy( varBinds.list[0].name.ids, MIB_OidPrefix.ids,
                 sizeof(UINT)*varBinds.list[0].name.idLength );
         varBinds.list[0].value.asnType = ASN_NULL;

         do
            {
	    printf( "GET-NEXT of:  " ); SnmpUtilPrintOid( &varBinds.list[0].name );
                                        printf( "   " );
            (*queryAddr)( (AsnInteger)ASN_RFC1157_GETNEXTREQUEST,
                          &varBinds,
		          &errorStatus,
		          &errorIndex
                          );
            printf( "\n  is  " ); SnmpUtilPrintOid( &varBinds.list[0].name );
	    if ( errorStatus )
	       {
               printf( "\nErrorstatus:  %lu\n\n", errorStatus );
	       }
	    else
	       {
               printf( "\n  =  " ); SnmpUtilPrintAsnAny( &varBinds.list[0].value );
	       }
            putchar( '\n' );
            }
         while ( varBinds.list[0].name.ids[7-1] != 78 );

         // Free the memory
         SnmpUtilVarBindListFree( &varBinds );


#if 0

            // query potential traps (see notes above)
            if (hPollForTrapEvent != NULL)
                {
                DWORD dwResult;

                if      ((dwResult = WaitForSingleObject(hPollForTrapEvent,
                         0/*immediate*/)) == 0xffffffff)
                    {
                    dbgprintf(1, "error on WaitForSingleObject %d\n",
                        GetLastError());
                    }
                else if (dwResult == 0 /*signaled*/)
                    {
                    AsnObjectIdentifier enterprise;
                    AsnInteger          genericTrap;
                    AsnInteger          specificTrap;
                    AsnTimeticks        timeStamp;
                    RFC1157VarBindList  variableBindings;

                    while(
                        (*trapAddr)(&enterprise, &genericTrap, &specificTrap,
                                    &timeStamp, &variableBindings)
                        )
                        {
                        printf("trap: gen=%d spec=%d time=%d\n",
                            genericTrap, specificTrap, timeStamp);

                        //also print data

                        } // end while ()

                    } // end if (trap ready)

                } // end if (handling traps)
#endif


         } // block


    return 0;

    } // end main()


//-------------------------------- END --------------------------------------

