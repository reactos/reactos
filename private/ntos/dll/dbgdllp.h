/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dbgdllp.h

Abstract:

    Debug Subsystem Dll Private Types and Prototypes

Author:

    Mark Lucovsky (markl) 22-Jan-1990

Revision History:

--*/

#ifndef _DBGDLLP_
#define _DBGDLLP_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsm.h>
#define NOEXTAPI
#include <wdbgexts.h>
#include <ntdbg.h>



//
// DbgSs Private Dll Prototypes and variables
//

HANDLE DbgSspApiPort;
HANDLE DbgSspKmReplyPort;
PDBGSS_UI_LOOKUP DbgSspUiLookUpRoutine;
PDBGSS_SUBSYSTEMKEY_LOOKUP DbgSspSubsystemKeyLookupRoutine;
PDBGSS_DBGKM_APIMSG_FILTER DbgSspKmApiMsgFilter;

typedef struct _DBGSS_CONTINUE_KEY {
    DBGKM_APIMSG KmApiMsg;
    HANDLE ReplyEvent;
} DBGSS_CONTINUE_KEY, *PDBGSS_CONTINUE_KEY;


NTSTATUS
DbgSspConnectToDbg( VOID );

NTSTATUS
DbgSspSrvApiLoop(
    IN PVOID ThreadParameter
    );

NTSTATUS
DbgSspCreateProcess (
    IN PDBGSS_CONTINUE_KEY ContinueKey,
    IN PCLIENT_ID AppClientId,
    IN PCLIENT_ID DebugUiClientId,
    IN PDBGKM_CREATE_PROCESS NewProcess
    );

NTSTATUS
DbgSspCreateThread (
    IN PDBGSS_CONTINUE_KEY ContinueKey,
    IN PCLIENT_ID AppClientId,
    IN PDBGKM_CREATE_THREAD NewThread
    );

NTSTATUS
DbgSspExitThread (
    IN PDBGSS_CONTINUE_KEY ContinueKey,
    IN PCLIENT_ID AppClientId,
    IN PDBGKM_EXIT_THREAD ExitThread
    );

NTSTATUS
DbgSspExitProcess (
    IN PDBGSS_CONTINUE_KEY ContinueKey,
    IN PCLIENT_ID AppClientId,
    IN PDBGKM_EXIT_PROCESS ExitProcess
    );

NTSTATUS
DbgSspException (
    IN PDBGSS_CONTINUE_KEY ContinueKey,
    IN PCLIENT_ID AppClientId,
    IN PDBGKM_EXCEPTION Exception
    );

NTSTATUS
DbgSspLoadDll (
    IN PDBGSS_CONTINUE_KEY ContinueKey,
    IN PCLIENT_ID AppClientId,
    IN PDBGKM_LOAD_DLL LoadDll
    );

NTSTATUS
DbgSspUnloadDll (
    IN PDBGSS_CONTINUE_KEY ContinueKey,
    IN PCLIENT_ID AppClientId,
    IN PDBGKM_UNLOAD_DLL UnloadDll
    );

#endif // _DBGDLLP_
