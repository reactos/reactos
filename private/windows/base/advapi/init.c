/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    init.c

Abstract:

    AdvApi32.dll initialization

Author:

    Robert Reichel (RobertRe) 8-12-92

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <marta.h>
#include <winsvcp.h>
#include "advapi.h"
#include "tsappcmp.h"

extern CRITICAL_SECTION    FeClientLoadCritical;
extern CRITICAL_SECTION    SddlSidLookupCritical;

extern BOOL gbDllHasThreadState;

//
// Local prototypes for functions that seem to have no prototypes.
//

BOOLEAN
RegInitialize (
    IN HANDLE Handle,
    IN DWORD Reason,
    IN PVOID Reserved
    );

BOOLEAN
Sys003Initialize (
    IN HANDLE Handle,
    IN DWORD Reason,
    IN PVOID Reserved
    );

BOOLEAN
AppmgmtInitialize (
    IN HANDLE Handle,
    IN DWORD Reason,
    IN PVOID Reserved
    );

BOOLEAN
WmiDllInitialize (
    IN HANDLE Handle,
    IN DWORD Reason,
    IN PVOID Reserved
    );

#define ADVAPI_PROCESS_ATTACH   ( 1 << DLL_PROCESS_ATTACH )
#define ADVAPI_PROCESS_DETACH   ( 1 << DLL_PROCESS_DETACH )
#define ADVAPI_THREAD_ATTACH    ( 1 << DLL_THREAD_ATTACH )
#define ADVAPI_THREAD_DETACH    ( 1 << DLL_THREAD_DETACH )

typedef struct _ADVAPI_INIT_ROUTINE {
    PDLL_INIT_ROUTINE InitRoutine ;
    ULONG Flags ;
} ADVAPI_INIT_ROUTINE, * PADVAPI_INIT_ROUTINE ;


//
// Place all ADVAPI32 initialization hooks in this
// table.
//

ADVAPI_INIT_ROUTINE AdvapiInitRoutines[] = {

    { (PDLL_INIT_ROUTINE) RegInitialize, 
            ADVAPI_PROCESS_ATTACH | 
            ADVAPI_PROCESS_DETACH | 
            ADVAPI_THREAD_DETACH },

    { (PDLL_INIT_ROUTINE) Sys003Initialize,
            ADVAPI_PROCESS_ATTACH |
            ADVAPI_PROCESS_DETACH },

    { (PDLL_INIT_ROUTINE) MartaDllInitialize,
            ADVAPI_PROCESS_ATTACH |
            ADVAPI_PROCESS_DETACH },

    { (PDLL_INIT_ROUTINE) AppmgmtInitialize,
            ADVAPI_PROCESS_ATTACH |
            ADVAPI_PROCESS_DETACH },

    { (PDLL_INIT_ROUTINE) WmiDllInitialize,
            ADVAPI_PROCESS_ATTACH |
            ADVAPI_PROCESS_DETACH }

};



//
// Place all critical sections used in advapi32 here:
//

PRTL_CRITICAL_SECTION AdvapiCriticalSections[] = {
        &FeClientLoadCritical,
        &SddlSidLookupCritical,
        &Logon32Lock
};


BOOLEAN
DllInitialize(
    IN PVOID hmod,
    IN ULONG Reason,
    IN PCONTEXT Context
    )
{
    BOOLEAN Result = TRUE;
    int i ;
    NTSTATUS Status ;
    ULONG ReasonMask ;

    //
    // First, handle all the critical sections
    //


    if ( Reason == DLL_PROCESS_ATTACH ) {

        //
        // Note -- we no longer call DisableThreadLibraryCalls
        // because we need these so win32 local registry
        // can clean up its state
        //

        Result = TRUE ;

        for ( i = 0 ; 
              i < sizeof( AdvapiCriticalSections ) / sizeof ( PRTL_CRITICAL_SECTION ) ;
              i++ )
        {
            Status = RtlInitializeCriticalSection( AdvapiCriticalSections[ i ] );

            if ( !NT_SUCCESS( Status ) )
            {
#if DBG
                DbgPrint("ADVAPI:  Failed to initialize critical section %p\n",
                            AdvapiCriticalSections[ i ] );
#endif 

                Result = FALSE ;
                break;
            }
        }

        if ( !Result )
        {
            return FALSE ;
        }

        if (IsTerminalServer()) {
            InitializeTermsrvFpns();
        }

    }

    //
    // Now, run the subcomponents initialization routines
    //

    ReasonMask = 1 << Reason ;

    for ( i = 0 ;
          i < sizeof( AdvapiInitRoutines ) / sizeof( ADVAPI_INIT_ROUTINE ) ;
          i++ )
    {
        if ( ( AdvapiInitRoutines[i].Flags & ReasonMask ) != 0 )
        {
            Result = AdvapiInitRoutines[i].InitRoutine( hmod, Reason, Context );

            if ( !Result )
            {
#if DBG
                DbgPrint( "ADVAPI:  sub init routine %p failed\n",
                            AdvapiInitRoutines[ i ].InitRoutine );
#endif 
                break;
            }
        }
    }

    if ( !Result )
    {
        return Result ;
    }


    //
    // If this is the process detach, clean up all the critical sections
    // after the hooks are run.
    //

    if ( Reason == DLL_PROCESS_DETACH )
    {
        for ( i = 0 ; 
              i < sizeof( AdvapiCriticalSections ) / sizeof ( PRTL_CRITICAL_SECTION ) ;
              i++ )
        {
            Status = RtlDeleteCriticalSection( AdvapiCriticalSections[ i ] );

            if ( !NT_SUCCESS( Status ) )
            {
                Result = FALSE ;
                break;
            }
        }
    }

#if DBG
        SccInit(Reason);
#endif // DBG

    return( Result );
}





