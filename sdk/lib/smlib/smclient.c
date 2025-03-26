/*
 * PROJECT:     ReactOS NT-Compatible Session Manager
 * LICENSE:     BSD 2-Clause License (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     SMSS Client Library (SMLIB) Client Stubs
 * COPYRIGHT:   Copyright 2012-2013 Alex Ionescu <alex.ionescu@reactos.org>
 *              Copyright 2021 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2022 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/**
 * @brief
 * Connects to the SM API port for registering a session callback port (Sb)
 * associated to a subsystem, or for issuing API requests to the SM API port.
 *
 * There are only two ways to call this API:
 * a) subsystems willing to register with SM will use it
 *    with full parameters (the function checks them);
 * b) regular SM clients, will set to 0 the 1st, the 2nd,
 *    and the 3rd parameters.
 *
 * @param[in]   SbApiPortName
 * Name of the Sb port the calling subsystem server already
 * created in the system namespace.
 *
 * @param[in]   SbApiPort
 * LPC port handle (checked, but not used: the subsystem is
 * required to have already created the callback port before
 * it connects to the SM).
 *
 * @param[in]   ImageType
 * A valid IMAGE_SUBSYSTEM_xxx value. PE images having this
 * subsystem value will be handled by the current subsystem.
 *
 * @param[out]  SmApiPort
 * Pointer to a HANDLE, which will be filled with a valid
 * client-side LPC communication port.
 *
 * @return
 * If all three optional values are omitted, an LPC status.
 * STATUS_INVALID_PARAMETER_MIX if PortName is defined and
 * both SbApiPort and ImageType are 0.
 *
 * @remark
 * Exported on Vista+ by NTDLL and called RtlConnectToSm().
 **/
NTSTATUS
NTAPI
SmConnectToSm(
    _In_opt_ PUNICODE_STRING SbApiPortName,
    _In_opt_ HANDLE SbApiPort,
    _In_opt_ ULONG ImageType,
    _Out_ PHANDLE SmApiPort)
{
    NTSTATUS Status;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    UNICODE_STRING PortName;
    SB_CONNECTION_INFO ConnectInfo = {0};
    ULONG ConnectInfoLength = sizeof(ConnectInfo);

    /* Setup the QoS structure */
    SecurityQos.ImpersonationLevel = SecurityIdentification;
    SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQos.EffectiveOnly = TRUE;

    /* Set the SM API port name */
    RtlInitUnicodeString(&PortName, L"\\SmApiPort"); // SM_API_PORT_NAME

    /* Check if this is a client connecting to SMSS, or SMSS to itself */
    if (SbApiPortName)
    {
        /* A client SB port as well as an image type must be present */
        if (!SbApiPort || (ImageType == IMAGE_SUBSYSTEM_UNKNOWN))
            return STATUS_INVALID_PARAMETER_MIX;

        /* Validate SbApiPortName's length */
        if (SbApiPortName->Length >= sizeof(ConnectInfo.SbApiPortName))
            return STATUS_INVALID_PARAMETER;

        /* Copy the client port name, and NULL-terminate it */
        RtlCopyMemory(ConnectInfo.SbApiPortName,
                      SbApiPortName->Buffer,
                      SbApiPortName->Length);
        ConnectInfo.SbApiPortName[SbApiPortName->Length / sizeof(WCHAR)] = UNICODE_NULL;

        /* Save the subsystem type */
        ConnectInfo.SubsystemType = ImageType;
    }
    else
    {
        /* No client port, and the subsystem type is not set */
        ConnectInfo.SbApiPortName[0] = UNICODE_NULL;
        ConnectInfo.SubsystemType = IMAGE_SUBSYSTEM_UNKNOWN;
    }

    /* Connect to SMSS and exchange connection information */
    Status = NtConnectPort(SmApiPort,
                           &PortName,
                           &SecurityQos,
                           NULL,
                           NULL,
                           NULL,
                           &ConnectInfo,
                           &ConnectInfoLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SmConnectToSm: Connect to Sm failed %lx\n", Status);
    }
#if (NTDDI_VERSION < NTDDI_VISTA)
    else
    {
        /* Treat a warning or informational status as success */
        Status = STATUS_SUCCESS;
    }
#endif

    /* Return if the connection was successful or not */
    return Status;
}

/**
 * @brief
 * Sends a message to the SM via the SM API port.
 *
 * @param[in]   SmApiPort
 * Port handle returned by SmConnectToSm().
 *
 * @param[in,out]   SmApiMsg
 * Message to send to the SM. The API-specific data must be initialized,
 * and the SmApiMsg->ApiNumber must be specified accordingly.
 *
 * @return
 * Success status as handed by the SM reply; otherwise a failure
 * status code.
 *
 * @remark
 * Exported on Vista+ by NTDLL and called RtlSendMsgToSm().
 **/
NTSTATUS
NTAPI
SmSendMsgToSm(
    _In_ HANDLE SmApiPort,
    _Inout_ PSM_API_MSG SmApiMsg)
{
    static ULONG RtlpSmMessageInfo[SmpMaxApiNumber] =
    {
        0 /*sizeof(SM_CREATE_FOREIGN_SESSION_MSG)*/,
        sizeof(SM_SESSION_COMPLETE_MSG),
        0 /*sizeof(SM_TERMINATE_FOREIGN_SESSION_MSG)*/,
        sizeof(SM_EXEC_PGM_MSG),
        sizeof(SM_LOAD_DEFERED_SUBSYSTEM_MSG),
        sizeof(SM_START_CSR_MSG),
        sizeof(SM_STOP_CSR_MSG),
    };

    NTSTATUS Status;
    ULONG DataLength;

    if (SmApiMsg->ApiNumber >= SmpMaxApiNumber)
        return STATUS_NOT_IMPLEMENTED;

    /* Obtain the necessary data length for this API */
    DataLength = RtlpSmMessageInfo[SmApiMsg->ApiNumber];

    /* Fill out the Port Message Header */
    // RtlZeroMemory(&SmApiMsg->h, sizeof(SmApiMsg->h));
    SmApiMsg->h.u2.ZeroInit = 0;
    /* DataLength = user_data_size + anything between
     * header and data, including intermediate padding */
    SmApiMsg->h.u1.s1.DataLength = (CSHORT)DataLength +
        FIELD_OFFSET(SM_API_MSG, u) - sizeof(SmApiMsg->h);
    /* TotalLength = sizeof(*SmApiMsg) on <= NT5.2, otherwise:
     * DataLength + header_size == user_data_size + FIELD_OFFSET(SM_API_MSG, u)
     * without structure trailing padding */
    SmApiMsg->h.u1.s1.TotalLength = SmApiMsg->h.u1.s1.DataLength + sizeof(SmApiMsg->h);

    /* Send the LPC message and wait for a reply */
    Status = NtRequestWaitReplyPort(SmApiPort, &SmApiMsg->h, &SmApiMsg->h);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SmSendMsgToSm: NtRequestWaitReplyPort failed, Status: 0x%08lx\n", Status);
    }
    else
    {
        /* Return the real status */
        Status = SmApiMsg->ReturnValue;
    }

    return Status;
}

/**
 * @brief
 * This function is called by an environment subsystem server
 * to tell the SM it has terminated the session it managed.
 *
 * @param[in]   SmApiPort
 * Port handle returned by SmConnectToSm().
 *
 * @param[in]   SessionId
 * The session ID of the session being terminated.
 *
 * @param[in]   SessionStatus
 * An NT status code for the termination.
 *
 * @return
 * Success status as handed by the SM reply; otherwise a failure
 * status code.
 **/
NTSTATUS
NTAPI
SmSessionComplete(
    _In_ HANDLE SmApiPort,
    _In_ ULONG SessionId,
    _In_ NTSTATUS SessionStatus)
{
    SM_API_MSG SmApiMsg = {0};
    PSM_SESSION_COMPLETE_MSG SessionComplete = &SmApiMsg.u.SessionComplete;

#if 0 //def _WIN64
    /* 64-bit SMSS needs to talk to 32-bit processes so do the LPC conversion */
    if (SmpIsWow64Process())
    {
        return SmpWow64SessionComplete(SmApiPort, SessionId, SessionStatus);
    }
#endif

    /* Set the message data */
    SessionComplete->SessionId = SessionId;
    SessionComplete->SessionStatus = SessionStatus;

    /* Send the message and wait for a reply */
    SmApiMsg.ApiNumber = SmpSessionCompleteApi;
    return SmSendMsgToSm(SmApiPort, &SmApiMsg);
}

/**
 * @brief
 * Requests the SM to start a process under a new environment session.
 *
 * @param[in]   SmApiPort
 * Port handle returned by SmConnectToSm().
 *
 * @param[in]   ProcessInformation
 * A process description as returned by RtlCreateUserProcess().
 *
 * @param[in]   DebugFlag
 * If set, indicates that the caller wants to debug this process
 * and act as its debug user interface.
 *
 * @return
 * Success status as handed by the SM reply; otherwise a failure
 * status code.
 **/
NTSTATUS
NTAPI
SmExecPgm(
    _In_ HANDLE SmApiPort,
    _In_ PRTL_USER_PROCESS_INFORMATION ProcessInformation,
    _In_ BOOLEAN DebugFlag)
{
    NTSTATUS Status;
    SM_API_MSG SmApiMsg = {0};
    PSM_EXEC_PGM_MSG ExecPgm = &SmApiMsg.u.ExecPgm;

#if 0 //def _WIN64
    /* 64-bit SMSS needs to talk to 32-bit processes so do the LPC conversion */
    if (SmpIsWow64Process())
    {
        return SmpWow64ExecPgm(SmApiPort, ProcessInformation, DebugFlag);
    }
#endif

    /* Set the message data */
    ExecPgm->ProcessInformation = *ProcessInformation;
    ExecPgm->DebugFlag = DebugFlag;

    /* Send the message and wait for a reply */
    SmApiMsg.ApiNumber = SmpExecPgmApi;
    Status = SmSendMsgToSm(SmApiPort, &SmApiMsg);

    /* Close the handles that the parent passed in and return status */
    NtClose(ProcessInformation->ProcessHandle);
    NtClose(ProcessInformation->ThreadHandle);
    return Status;
}

/**
 * @brief
 * This function is used to make the SM start an environment
 * subsystem server process.
 *
 * @param[in]   SmApiPort
 * Port handle returned by SmConnectToSm().
 *
 * @param[in]   DeferedSubsystem
 * Name of the subsystem to start. This must be one of the subsystems
 * listed by value's name in the SM registry key
 * \Registry\SYSTEM\CurrentControlSet\Control\Session Manager\SubSystems
 * (used by the SM to lookup the corresponding image name).
 * Default valid names are: "Debug", "Windows", "Posix", "Os2".
 *
 * @return
 * Success status as handed by the SM reply; otherwise a failure
 * status code.
 **/
NTSTATUS
NTAPI
SmLoadDeferedSubsystem(
    _In_ HANDLE SmApiPort,
    _In_ PUNICODE_STRING DeferedSubsystem)
{
    SM_API_MSG SmApiMsg = {0};
    PSM_LOAD_DEFERED_SUBSYSTEM_MSG LoadDefered = &SmApiMsg.u.LoadDefered;

#if 0 //def _WIN64
    /* 64-bit SMSS needs to talk to 32-bit processes so do the LPC conversion */
    if (SmpIsWow64Process())
    {
        return SmpWow64LoadDeferedSubsystem(SmApiPort, DeferedSubsystem);
    }
#endif

    /* Validate DeferedSubsystem's length */
    if (DeferedSubsystem->Length > sizeof(LoadDefered->Buffer))
        return STATUS_INVALID_PARAMETER;

    /* Set the message data */
    /* Buffer stores a counted non-NULL-terminated UNICODE string */
    LoadDefered->Length = DeferedSubsystem->Length;
    RtlCopyMemory(LoadDefered->Buffer,
                  DeferedSubsystem->Buffer,
                  DeferedSubsystem->Length);

    /* Send the message and wait for a reply */
    SmApiMsg.ApiNumber = SmpLoadDeferedSubsystemApi;
    return SmSendMsgToSm(SmApiPort, &SmApiMsg);
}

/**
 * @brief
 * Requests the SM to create a new Terminal Services session
 * and start an initial command.
 *
 * @param[in]   SmApiPort
 * Port handle returned by SmConnectToSm().
 *
 * @param[out]  pMuSessionId
 * Pointer to a variable that receives the session ID of the new
 * Terminal Services session that has been created.
 *
 * @param[in]   CommandLine
 * Full path to the image to be used as the initial command.
 *
 * @param[out]  pWindowsSubSysProcessId
 * Pointer to a variable that receives the process ID of the environment
 * subsystem that has been started in the new session.
 *
 * @param[out]  pInitialCommandProcessId
 * Pointer to a variable that receives the process ID of the initial command.
 *
 * @return
 * Success status as handed by the SM reply; otherwise a failure
 * status code.
 **/
NTSTATUS
NTAPI
SmStartCsr(
    _In_ HANDLE SmApiPort,
    _Out_ PULONG pMuSessionId,
    _In_opt_ PUNICODE_STRING CommandLine,
    _Out_ PHANDLE pWindowsSubSysProcessId,
    _Out_ PHANDLE pInitialCommandProcessId)
{
    NTSTATUS Status;
    SM_API_MSG SmApiMsg = {0};
    PSM_START_CSR_MSG StartCsr = &SmApiMsg.u.StartCsr;

#if 0 //def _WIN64
    /* 64-bit SMSS needs to talk to 32-bit processes so do the LPC conversion */
    if (SmpIsWow64Process())
    {
        return SmpWow64StartCsr(SmApiPort,
                                pMuSessionId,
                                CommandLine,
                                pWindowsSubSysProcessId,
                                pInitialCommandProcessId);
    }
#endif

    /* Set the message data */
    if (CommandLine)
    {
        /* Validate CommandLine's length */
        if (CommandLine->Length > sizeof(StartCsr->Buffer))
            return STATUS_INVALID_PARAMETER;

        /* Buffer stores a counted non-NULL-terminated UNICODE string */
        StartCsr->Length = CommandLine->Length;
        RtlCopyMemory(StartCsr->Buffer,
                      CommandLine->Buffer,
                      CommandLine->Length);
    }
    else
    {
        StartCsr->Length = 0;
    }

    /* Send the message and wait for a reply */
    SmApiMsg.ApiNumber = SmpStartCsrApi;
    Status = SmSendMsgToSm(SmApiPort, &SmApiMsg);

    /* Give back information to caller */
    *pMuSessionId = StartCsr->MuSessionId;
    *pWindowsSubSysProcessId = StartCsr->WindowsSubSysProcessId;
    *pInitialCommandProcessId = StartCsr->SmpInitialCommandProcessId;

    return Status;
}

/**
 * @brief
 * Requests the SM to terminate a Terminal Services session.
 *
 * @param[in]   SmApiPort
 * Port handle returned by SmConnectToSm().
 *
 * @param[in]   MuSessionId
 * The Terminal Services session ID, returned by SmStartCsr().
 *
 * @return
 * Success status as handed by the SM reply; otherwise a failure
 * status code.
 **/
NTSTATUS
NTAPI
SmStopCsr(
    _In_ HANDLE SmApiPort,
    _In_ ULONG MuSessionId)
{
    SM_API_MSG SmApiMsg = {0};

#if 0 //def _WIN64
    /* 64-bit SMSS needs to talk to 32-bit processes so do the LPC conversion */
    if (SmpIsWow64Process())
    {
        return SmpWow64StopCsr(SmApiPort, MuSessionId);
    }
#endif

    /* Set the message data */
    SmApiMsg.u.StopCsr.MuSessionId = MuSessionId;

    /* Send the message and wait for a reply */
    SmApiMsg.ApiNumber = SmpStopCsrApi;
    return SmSendMsgToSm(SmApiPort, &SmApiMsg);
}
