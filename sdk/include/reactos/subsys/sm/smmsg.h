/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            include/reactos/subsys/sm/smmsg.h
 * PURPOSE:         SMSS (SB and SM) Message Format
 * PROGRAMMERS:     Alex Ionescu
 */
#pragma once
#ifndef _SM_MSG_
#define _SM_MSG_

//
// There are the APIs that a Client (such as CSRSS) can send to the SMSS Server.
//
// These are called "SM" APIs.
//
// The exact names are not known, but we are basing them on the SmpApiName array
// in the checked build of SMSS, which is probably a close approximation. We add
// "p" to use the similar nomenclature seen/leaked out in the Base CSRSS APIs.
//
// The enumeration finishes with an enumeratee holding the maximum API number.
// Its name is based on BasepMaxApiNumber, UserpMaxApiNumber...
//
//
typedef enum _SMSRV_API_NUMBER
{
    SmpCreateForeignSessionApi,
    SmpSessionCompleteApi,
    SmpTerminateForeignSessionApi,
    SmpExecPgmApi,
    SmpLoadDeferedSubsystemApi,
    SmpStartCsrApi,
    SmpStopCsrApi,

    SmpMaxApiNumber
} SMSRV_API_NUMBER;

//
// These are the structures making up the SM_API_MSG packet structure defined
// below. Each one corresponds to an equivalent API from the list above.
//
typedef struct _SM_CREATE_FOREIGN_SESSION_MSG
{
    ULONG NotImplemented;
} SM_CREATE_FOREIGN_SESSION_MSG, *PSM_CREATE_FOREIGN_SESSION_MSG;

typedef struct _SM_SESSION_COMPLETE_MSG
{
    ULONG SessionId;
    NTSTATUS SessionStatus;
} SM_SESSION_COMPLETE_MSG, *PSM_SESSION_COMPLETE_MSG;

typedef struct _SM_TERMINATE_FOREIGN_SESSION_MSG
{
    ULONG NotImplemented;
} SM_TERMINATE_FOREIGN_SESSION_MSG, *PSM_TERMINATE_FOREIGN_SESSION_MSG;

typedef struct _SM_EXEC_PGM_MSG
{
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    BOOLEAN DebugFlag;
} SM_EXEC_PGM_MSG, *PSM_EXEC_PGM_MSG;
#ifndef _WIN64
C_ASSERT(sizeof(SM_EXEC_PGM_MSG) == 0x48);
#endif

typedef struct _SM_LOAD_DEFERED_SUBSYSTEM_MSG
{
    ULONG Length;
    WCHAR Buffer[32];
} SM_LOAD_DEFERED_SUBSYSTEM_MSG, *PSM_LOAD_DEFERED_SUBSYSTEM_MSG;

typedef struct _SM_START_CSR_MSG
{
    ULONG MuSessionId;
    ULONG Length;
    WCHAR Buffer[128];
    HANDLE WindowsSubSysProcessId;
    HANDLE SmpInitialCommandProcessId;
} SM_START_CSR_MSG, *PSM_START_CSR_MSG;

typedef struct _SM_STOP_CSR_MSG
{
    ULONG MuSessionId;
} SM_STOP_CSR_MSG, *PSM_STOP_CSR_MSG;

//
// This is the actual packet structure sent over LCP to the \SmApiPort
//
typedef struct _SM_API_MSG
{
    PORT_MESSAGE h;
    SMSRV_API_NUMBER ApiNumber;
    NTSTATUS ReturnValue;
    union
    {
        SM_CREATE_FOREIGN_SESSION_MSG CreateForeignSession;
        SM_SESSION_COMPLETE_MSG SessionComplete;
        SM_TERMINATE_FOREIGN_SESSION_MSG TerminateForeignComplete;
        SM_EXEC_PGM_MSG ExecPgm;
        SM_LOAD_DEFERED_SUBSYSTEM_MSG LoadDefered;
        SM_START_CSR_MSG StartCsr;
        SM_STOP_CSR_MSG StopCsr;
    } u;
} SM_API_MSG, *PSM_API_MSG;

//
// This is the size that Server 2003 SP1 SMSS expects, so make sure we conform.
//
#ifndef _WIN64
C_ASSERT(sizeof(SM_API_MSG) == 0x130);
#endif

//
// There are the APIs that the SMSS Server can send to a client (such as CSRSS).
//
// These are called "SB" APIs.
//
// The exact names are unknown but we are basing them on the CsrServerSbApiName
// array in the checked build of CSRSRV which is probably a close approximation.
// We add "p" to use the similar nomenclature seen/leaked out in the Base CSRSS
// APIs.
//
// The enumeration finishes with an enumeratee holding the maximum API number.
// Its name is based on BasepMaxApiNumber, UserpMaxApiNumber...
//
//
typedef enum _SB_API_NUMBER
{
    SbpCreateSession,
    SbpTerminateSession,
    SbpForeignSessionComplete,
    SbpCreateProcess,

    SbpMaxApiNumber
} SB_API_NUMBER;

//
// These are the structures making up the SB_API_MSG packet structure defined
// below. Each one corresponds to an equivalent API from the list above.
//
typedef struct _SB_CREATE_SESSION_MSG
{
    ULONG SessionId;
    RTL_USER_PROCESS_INFORMATION ProcessInfo;
    ULONG Unknown;
    ULONG MuSessionId;
    CLIENT_ID ClientId;
} SB_CREATE_SESSION_MSG, *PSB_CREATE_SESSION_MSG;

typedef struct _SB_TERMINATE_SESSION_MSG
{
    ULONG SessionId;
} SB_TERMINATE_SESSION_MSG, *PSB_TERMINATE_SESSION_MSG;

typedef struct _SB_FOREIGN_SESSION_COMPLETE_MSG
{
    ULONG SessionId;
} SB_FOREIGN_SESSION_COMPLETE_MSG, *PSB_FOREIGN_SESSION_COMPLETE_MSG;

#define SB_PROCESS_FLAGS_DEBUG          0x1
#define SB_PROCESS_FLAGS_WAIT_ON_THREAD 0x2
#define SB_PROCESS_FLAGS_RESERVE_1MB    0x8
#define SB_PROCESS_FLAGS_SKIP_CHECKS    0x20
typedef struct _SB_CREATE_PROCESS_MSG
{
    union
    {
        struct
        {
            PUNICODE_STRING ImageName;
            PUNICODE_STRING CurrentDirectory;
            PUNICODE_STRING CommandLine;
            PUNICODE_STRING DllPath;
            ULONG Flags;
            ULONG DebugFlags;
        } In;
        struct
        {
            HANDLE ProcessHandle;
            HANDLE ThreadHandle;
            ULONG SubsystemType;
            CLIENT_ID ClientId;
        } Out;
    };
} SB_CREATE_PROCESS_MSG, *PSB_CREATE_PROCESS_MSG;

//
// When the server connects to a client, this structure is exchanged
//
typedef struct _SB_CONNECTION_INFO
{
    ULONG SubsystemType;
    WCHAR SbApiPortName[120];
} SB_CONNECTION_INFO, *PSB_CONNECTION_INFO;

//
// This is the actual packet structure sent over LCP to the \SbApiPort
//
typedef struct _SB_API_MSG
{
    PORT_MESSAGE h;
    union
    {
        SB_CONNECTION_INFO ConnectionInfo;
        struct
        {
            SB_API_NUMBER ApiNumber;
            NTSTATUS ReturnValue;
            union
            {
                SB_CREATE_SESSION_MSG CreateSession;
                SB_TERMINATE_SESSION_MSG TerminateSession;
                SB_FOREIGN_SESSION_COMPLETE_MSG ForeignSessionComplete;
                SB_CREATE_PROCESS_MSG CreateProcess;
            };
        };
    };
} SB_API_MSG, *PSB_API_MSG;

//
// This is the size that Server 2003 SP1 SMSS expects, so make sure we conform.
//
#ifndef _WIN64
C_ASSERT(sizeof(SB_CONNECTION_INFO) == 0xF4);
C_ASSERT(sizeof(SB_API_MSG) == 0x110);
#endif

//
// SB Message Handler
//
typedef
BOOLEAN
(NTAPI *PSB_API_ROUTINE)(
    IN PSB_API_MSG SbApiMsg
);

//
// The actual server functions that a client linking with smlib can call
//
NTSTATUS
NTAPI
SmConnectToSm(
    IN PUNICODE_STRING SbApiPortName,
    IN HANDLE SbApiPort,
    IN ULONG ImageType,
    OUT PHANDLE SmApiPort
);

NTSTATUS
NTAPI
SmExecPgm(
    IN HANDLE SmApiPort,
    IN PRTL_USER_PROCESS_INFORMATION ProcessInformation,
    IN BOOLEAN DebugFlag
);

NTSTATUS
NTAPI
SmSessionComplete(
    IN HANDLE SmApiPort,
    IN ULONG SessionId,
    IN NTSTATUS SessionStatus
);

#endif
