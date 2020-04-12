/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/sam.c
 * PURPOSE:     SAM releated functions
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

#include "precomp.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(msv1_0);

NTSTATUS
GetAccountDomainSid(PRPC_SID *Sid)
{
    LSAPR_HANDLE PolicyHandle = NULL;
    PLSAPR_POLICY_INFORMATION PolicyInfo = NULL;
    ULONG Length = 0;
    NTSTATUS Status;

    Status = LsaIOpenPolicyTrusted(&PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LsaIOpenPolicyTrusted() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    Status = LsarQueryInformationPolicy(PolicyHandle,
                                        PolicyAccountDomainInformation,
                                        &PolicyInfo);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LsarQueryInformationPolicy() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    Length = RtlLengthSid(PolicyInfo->PolicyAccountDomainInfo.Sid);

    *Sid = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
    if (*Sid == NULL)
    {
        ERR("Failed to allocate SID\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    memcpy(*Sid, PolicyInfo->PolicyAccountDomainInfo.Sid, Length);

done:
    if (PolicyInfo != NULL)
        LsaIFree_LSAPR_POLICY_INFORMATION(PolicyAccountDomainInformation,
                                          PolicyInfo);

    if (PolicyHandle != NULL)
        LsarClose(&PolicyHandle);

    return Status;
}

static
NTSTATUS
MsvpCheckPassword(
    IN PLSA_SAM_PWD_DATA UserPwdData,
    IN PSAMPR_USER_INFO_BUFFER UserInfo)
{
    ENCRYPTED_NT_OWF_PASSWORD UserNtPassword;
    ENCRYPTED_LM_OWF_PASSWORD UserLmPassword;
    BOOLEAN UserLmPasswordPresent = FALSE;
    BOOLEAN UserNtPasswordPresent = FALSE;
    OEM_STRING LmPwdString;
    CHAR LmPwdBuffer[15];
    NTSTATUS Status;

    TRACE("(%p %p)\n", UserPwdData, UserInfo);

    /* Calculate the LM password and hash for the users password */
    LmPwdString.Length = 15;
    LmPwdString.MaximumLength = 15;
    LmPwdString.Buffer = LmPwdBuffer;
    ZeroMemory(LmPwdString.Buffer, LmPwdString.MaximumLength);

    Status = RtlUpcaseUnicodeStringToOemString(&LmPwdString,
                                               UserPwdData->PlainPwd,
                                               FALSE);
    if (NT_SUCCESS(Status))
    {
        /* Calculate the LM hash value of the users password */
        Status = SystemFunction006(LmPwdString.Buffer,
                                   (LPSTR)&UserLmPassword);
        if (NT_SUCCESS(Status))
        {
            UserLmPasswordPresent = TRUE;
        }
    }

    /* Calculate the NT hash of the users password */
    Status = SystemFunction007(UserPwdData->PlainPwd,
                               (LPBYTE)&UserNtPassword);
    if (NT_SUCCESS(Status))
    {
        UserNtPasswordPresent = TRUE;
    }

    Status = STATUS_WRONG_PASSWORD;

    /* Succeed, if no password has been set */
    if (UserInfo->All.NtPasswordPresent == FALSE &&
        UserInfo->All.LmPasswordPresent == FALSE)
    {
        TRACE("No password check!\n");
        Status = STATUS_SUCCESS;
        goto done;
    }

    /* Succeed, if NT password matches */
    if (UserNtPasswordPresent && UserInfo->All.NtPasswordPresent)
    {
        TRACE("Check NT password hashes:\n");
        if (RtlEqualMemory(&UserNtPassword,
                           UserInfo->All.NtOwfPassword.Buffer,
                           sizeof(ENCRYPTED_NT_OWF_PASSWORD)))
        {
            TRACE("  success!\n");
            Status = STATUS_SUCCESS;
            goto done;
        }

        TRACE("  failed!\n");
    }

    /* Succeed, if LM password matches */
    if (UserLmPasswordPresent && UserInfo->All.LmPasswordPresent)
    {
        TRACE("Check LM password hashes:\n");
        if (RtlEqualMemory(&UserLmPassword,
                           UserInfo->All.LmOwfPassword.Buffer,
                           sizeof(ENCRYPTED_LM_OWF_PASSWORD)))
        {
            TRACE("  success!\n");
            Status = STATUS_SUCCESS;
            goto done;
        }
        TRACE("  failed!\n");
    }

done:
    return Status;
}

/* TODO
 * context_cli_NegFlg
 * context_serverchallenge
 *
 */
static
NTSTATUS
MsvpCheckNetworkPassword(
    IN PLSA_SAM_PWD_DATA UserPwdData,
    IN PSAMPR_USER_INFO_BUFFER UserInfo)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PMSV1_0_LM20_LOGON LogonInfo = UserPwdData->LogonInfo;
    ULONG context_cli_NegFlg = 0;
    BOOL UseNTLMv2;
    UCHAR *ResponseKeyLm;
    UCHAR *ResponseKeyNt;
    UCHAR *ChallengeFromClient;//[MSV1_0_CHALLENGE_LENGTH];
    UCHAR ZeroBytes16[16];
        //PNTLMSSP_CONTEXT_SVR Context = NULL; // FIXME - maybe from CLIENT_REQUEST ...
        //UCHAR ExportedSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    USER_SESSION_KEY SessionBaseKey;
        //UNICODE_STRING ServerName;
    // Shortcuts
    PUNICODE_STRING ad_DomainName = &LogonInfo->LogonDomainName;
    PSTRING NtChallengeResponse = &LogonInfo->CaseSensitiveChallengeResponse;
    PSTRING LmChallengeResponse = &LogonInfo->CaseInsensitiveChallengeResponse;
        //PNTLMSSP_CONTEXT_SVR context = Context;
    EXT_STRING_W XServerName;
    EXT_STRING_W Xad_DomainName;
    ULONGLONG ChallengeTimestamp;
    //UCHAR context_ServerChallenge[MSV1_0_CHALLENGE_LENGTH];
    UCHAR *ServerChallenge = LogonInfo->ChallengeToClient;
    STRING ExpectedNtChallengeResponse;
    STRING ExpectedLmChallengeResponse;

    // Zurückgeben??
    UCHAR KeyExchangeKey[MSV1_0_USER_SESSION_KEY_LENGTH];

    TRACE("(%p %p)\n", UserPwdData, UserInfo);

    //FIXME: context_cli_NegFlg
    // relevante flags aus response ermitteln ..
    //FIXME: context_ServerChallenge
    // serverchallenge ... ist das ermittelbar?
    //RtlZeroBytes(context_ServerChallenge, MSV1_0_CHALLENGE_LENGTH);
    RtlZeroBytes(ZeroBytes16, MSV1_0_CHALLENGE_LENGTH);
    RtlInitString(&ExpectedLmChallengeResponse, NULL);
    RtlInitString(&ExpectedNtChallengeResponse, NULL);

        //ExtDataInit(&SessionBaseKey, NULL, 0);
        //ExtDataSetLength(&SessionBaseKey, MSV1_0_USER_SESSION_KEY_LENGTH, TRUE);

            /* Servername is NetBIOS Name or DNS Hostname */
            //RtlInitUnicodeString(&ServerName, (WCHAR*)gsvr->NbMachineName.Buffer);

    // Hacks
    XServerName.Buffer = (PBYTE)UserPwdData->ComputerName->Buffer;
    XServerName.bUsed = UserPwdData->ComputerName->Length;
    XServerName.bAllocated = UserPwdData->ComputerName->MaximumLength;
    Xad_DomainName.Buffer = (PBYTE)ad_DomainName->Buffer;
    Xad_DomainName.bUsed = ad_DomainName->Length;
    Xad_DomainName.bAllocated = ad_DomainName->MaximumLength;

    // guessing cli_negFlg
    if ((LmChallengeResponse->Length == 0x18) &&
        (NtChallengeResponse->Length == 0x18))
    {
        context_cli_NegFlg |= NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY;
    }
    //If AUTHENTICATE_MESSAGE.NtChallengeResponseFields.NtChallengeResponseLen > 0x0018
    //Set ChallengeFromClient to NTLMv2_RESPONSE.NTLMv2_CLIENT_CHALLENGE.ChallengeFromClient
    //ElseIf NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY is set in NegFlg
    //Set ChallengeFromClient to LM_RESPONSE.Response[0..7]
    //Else
    //Set ChallengeFromClient to NIL
    //EndIf
    UseNTLMv2 = FALSE;
    ChallengeTimestamp = 0;
    if (NtChallengeResponse->Length > 0x18)
    {
        PMSV1_0_NTLM3_RESPONSE ntResp = (PMSV1_0_NTLM3_RESPONSE)NtChallengeResponse->Buffer;
        ChallengeFromClient = ntResp->ChallengeFromClient;
        //Time.dwHighDateTime = ntResp->TimeStamp >> 32;
        //Time.dwLowDateTime = ntResp->TimeStamp && 0xffffffff;
        ChallengeTimestamp = ntResp->TimeStamp;
        UseNTLMv2 = TRUE;
    }
    else if (context_cli_NegFlg & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY)
        ChallengeFromClient = (UCHAR*)LmChallengeResponse->Buffer;
    else
        ChallengeFromClient = ZeroBytes16;
/*
            // FIXME: Fill ad_EncryptedRandomSessionKey
            // FIXME: I think it should be consist in CaseSensitiveChallengeResponse or
            //        CaseInsensitiveChallengeResponse ... i guess the first
            //        bytes of it ... but when ... could we get this information
            //        if length is x bytes? ...?
*/


    TRACE("context->cli_NegFlg\n");
    NtlmPrintNegotiateFlags(context_cli_NegFlg);

    // Berücksichtigen und ggf. NULL oder so übergeben ..
    //FIXME: UserInfo->All.NtPasswordPresent
    //FIXME: UserInfo->All.LmPasswordPresent
    ResponseKeyNt = ZeroBytes16;
    if (UserInfo->All.NtPasswordPresent &
        (UserInfo->All.NtOwfPassword.Length == MSV1_0_NT_OWF_PASSWORD_LENGTH))
        ResponseKeyNt = (PUCHAR)UserInfo->All.NtOwfPassword.Buffer;

    ResponseKeyLm = ZeroBytes16;
    if (UserInfo->All.LmPasswordPresent &
        (UserInfo->All.LmOwfPassword.Length == MSV1_0_NT_OWF_PASSWORD_LENGTH))
        ResponseKeyLm = (PUCHAR)UserInfo->All.LmOwfPassword.Buffer;

    if (!ComputeResponse(
        context_cli_NegFlg,//FIXME: context->cli_NegFlg,
        UseNTLMv2,
        FALSE,
        &Xad_DomainName,
        ResponseKeyLm,
        ResponseKeyNt,
        &XServerName,
        ChallengeFromClient,
        ServerChallenge,//context_ServerChallenge,
        ChallengeTimestamp,
        /*HACK*/(PEXT_DATA)&ExpectedNtChallengeResponse,
        /*HACK*/(PEXT_DATA)&ExpectedLmChallengeResponse,
        &SessionBaseKey))
    {
        Status = STATUS_INTERNAL_ERROR;
        ERR("ComputeResponse failed!\n");
        goto done;
    }

    // cli_neg_flags
    //FIXME: Need to fiure out if we
    // need one of the following flags:
    // (needed for KXKEY)
    //   NTLMSSP_NEGOTIATE_LM_KEY
    //   ... wenn es nicht der basekey ist...
    //   NTLMSSP_REQUEST_NON_NT_SESSION_KEY
    //   ... wenn Byte 8-15 alle 0 sind ...

    TRACE("NTChallengeResponse\n");
    NtlmPrintHexDump((PVOID)NtChallengeResponse->Buffer, NtChallengeResponse->Length);
    TRACE("NTChallengeResponse (expected)\n");
    NtlmPrintHexDump((PVOID)ExpectedNtChallengeResponse.Buffer, ExpectedNtChallengeResponse.Length);

    TRACE("LmChallengeResponse\n");
    NtlmPrintHexDump((PVOID)LmChallengeResponse->Buffer, LmChallengeResponse->Length);
    TRACE("LmChallengeResponse (expected)\n");
    NtlmPrintHexDump((PVOID)ExpectedLmChallengeResponse.Buffer, ExpectedLmChallengeResponse.Length);

    // If (AUTHENTICATE_MESSAGE.NtChallengeResponse !=
    // ExpectedNtChallengeResponse)
    // If (AUTHENTICATE_MESSAGE.LmChallengeResponse !=
    // ExpectedLmChallengeResponse)

    //FIXME: should we try here a second login
    //       wihout domain ... or should the caller
    //       call a 2nd time ApLogonUserEx2?
    /*
    if (!RtlEqualString(ad_NtChallengeResponse, &ExpectedNtChallengeResponse, FALSE) &&
       ((ad_LmChallengeResponse->Length != 0) &&
        (!RtlEqualString(ad_LmChallengeResponse, &ExpectedLmChallengeResponse, FALSE))))
    {
        // Retry using NIL for the domain name: Retrieve the ResponseKeyNT
        // and ResponseKeyLM from the local user account database using
        // the UserName specified in the AUTHENTICATE_MESSAGE and
        // NIL for the DomainName.
        FIXME("2nd try not implemented (DomainName = NIL).\n");
        //Set ExpectedNtChallengeResponse, ExpectedLmChallengeResponse,
        //SessionBaseKey to ComputeResponse(NegFlg, ResponseKeyNT,
        //ResponseKeyLM, CHALLENGE_MESSAGE.ServerChallenge,
        //ChallengeFromClient, Time, ServerName)
        //Set KeyExchangeKey to KXKEY(SessionBaseKey,
        //AUTHENTICATE_MESSAGE.LmChallengeResponse,
        //CHALLENGE_MESSAGE.ServerChallenge)
        //If (AUTHENTICATE_MESSAGE.NtChallengeResponse !=
        //ExpectedNtChallengeResponse)
        //If (AUTHENTICATE_MESSAGE.LmChallengeResponse !=
        //ExpectedLmChallengeResponse)
        {
            //Return INVALID message error
            ret = SEC_E_INVALID_TOKEN;
            goto quit;
            //EndIf
            //EndIf
        //EndIf
        //EndIf
        }
    //EndIf
    }*/

    UserPwdData->LogonType = NetLogonAnonymouse;

    /* Succeed, if NT password matches */
    if (NtChallengeResponse->Length > 0 && UserInfo->All.NtPasswordPresent)
    {
        TRACE("Check NT challenge response:\n");
        if ((NtChallengeResponse->Length != ExpectedNtChallengeResponse.Length) ||
            (RtlEqualMemory(NtChallengeResponse->Buffer,
                            ExpectedNtChallengeResponse.Buffer,
                            ExpectedNtChallengeResponse.Length)))
        {
            TRACE("  success!\n");
            UserPwdData->LogonType = NetLogonNtKey;
            Status = STATUS_SUCCESS;
            goto makekxkey;
        }

        TRACE("  failed!\n");
    }

    /* Succeed, if LM password matches */
    if (LmChallengeResponse->Length > 0 && UserInfo->All.LmPasswordPresent)
    {
        TRACE("Check NT challenge response:\n");
        if ((LmChallengeResponse->Length != ExpectedLmChallengeResponse.Length) ||
            (RtlEqualMemory(LmChallengeResponse->Buffer,
                            ExpectedLmChallengeResponse.Buffer,
                            ExpectedLmChallengeResponse.Length)))
        {
            TRACE("  success!\n");
            UserPwdData->LogonType = NetLogonLmKey;
            Status = STATUS_SUCCESS;
            goto makekxkey;
        }
        TRACE("  failed!\n");
    }

    // both keys - lm and nt - did not fit
    Status = STATUS_WRONG_PASSWORD;
    goto done;

makekxkey:
    // make keys and fill result info
    // Set KeyExchangeKey to KXKEY(SessionBaseKey,
    // AUTHENTICATE_MESSAGE.LmChallengeResponse, CHALLENGE_MESSAGE.ServerChallenge)
    KXKEY(context_cli_NegFlg, (PUCHAR)&SessionBaseKey,
          /*HACK*/(PEXT_DATA)&LmChallengeResponse,
          /*HACK*/(PEXT_DATA)&NtChallengeResponse,
          ServerChallenge/*context_ServerChallenge*/, ResponseKeyLm,
          KeyExchangeKey);
    TRACE("KeyExchangeKey\n");
    NtlmPrintHexDump(KeyExchangeKey, 16);

    if (!((context_cli_NegFlg & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY) &&
          /* using NTLMv1? */
          (LmChallengeResponse->Length != 0x18) &&
          (NtChallengeResponse->Length != 0x18)) &&
       (context_cli_NegFlg == NTLMSSP_REQUEST_NON_NT_SESSION_KEY))
    {
        RtlCopyMemory(&UserPwdData->LmSessionKey, KeyExchangeKey, MSV1_0_LANMAN_SESSION_KEY_LENGTH);
        RtlZeroBytes(&UserPwdData->UserSessionKey, MSV1_0_USER_SESSION_KEY_LENGTH);
    }
    else
    {
        RtlCopyMemory(&UserPwdData->UserSessionKey, KeyExchangeKey, MSV1_0_USER_SESSION_KEY_LENGTH);
        RtlZeroBytes(&UserPwdData->LmSessionKey, MSV1_0_LANMAN_SESSION_KEY_LENGTH);
    }
    /*-> hier nicht mehr nötig -> LM2_LOGON_PRFOFILE füllen!*/

    //Set MessageMIC to AUTHENTICATE_MESSAGE.MIC
    /* MessageMIC = ad->authMessage->MIC; unused should compared with?? */
    //Set AUTHENTICATE_MESSAGE.MIC to Z(16)

    //If (NTLMSSP_NEGOTIATE_KEY_EXCH flag is set in NegFlg
    //AND (NTLMSSP_NEGOTIATE_SIGN OR NTLMSSP_NEGOTIATE_SEAL are set in NegFlg) )
    /*if ((context_cli_NegFlg & NTLMSSP_NEGOTIATE_KEY_EXCH) &&
        (context_cli_NegFlg & (NTLMSSP_NEGOTIATE_SIGN |
                               NTLMSSP_NEGOTIATE_SEAL)))
    {
        //Set ExportedSessionKey to RC4K(KeyExchangeKey,
        //AUTHENTICATE_MESSAGE.EncryptedRandomSessionKey)
        TRACE("EncryptedRandomSessionKey...\n");
        NtlmPrintHexDump((PBYTE)ad_EncryptedRandomSessionKey.Buffer, ad_EncryptedRandomSessionKey.Length);
        // Assert nötig, da ExportedSessionKey auch 16 Bytes ist ...
        //ASSERT(ad->authMessage->EncryptedRandomSessionKey.Length == MSV1_0_USER_SESSION_KEY_LENGTH);
        RC4K(KeyExchangeKey, ARRAYSIZE(KeyExchangeKey),
             (PUCHAR)ad_EncryptedRandomSessionKey.Buffer,
             ad_EncryptedRandomSessionKey.Length,
             ExportedSessionKey);
    }
    else
    {
        //Set ExportedSessionKey to KeyExchangeKey
        memcpy(ExportedSessionKey, KeyExchangeKey, MSV1_0_USER_SESSION_KEY_LENGTH);
    }
    TRACE("ExportedSessionKey\n");
    NtlmPrintHexDump(ExportedSessionKey, 16);
    //Set MIC to HMAC_MD5(ExportedSessionKey, ConcatenationOf(
    //NEGOTIATE_MESSAGE, CHALLENGE_MESSAGE,
    //AUTHENTICATE_MESSAGE))
    FIXME("need NEGO/CHALLENGE and Auth-Message for MIC!\n");
    //memset(&MIC, 0, 16);
    //Set ClientSigningKey to SIGNKEY(NegFlg, ExportedSessionKey , "Client")
    SIGNKEY(ExportedSessionKey, TRUE, context->cli_msg.ClientSigningKey);
    //Set ServerSigningKey to SIGNKEY(NegFlg, ExportedSessionKey , "Server")
    SIGNKEY(ExportedSessionKey, FALSE, context->cli_msg.ServerSigningKey);
    //Set ClientSealingKey to SEALKEY(NegFlg, ExportedSessionKey , "Client")
    SEALKEY(context->cli_NegFlg, ExportedSessionKey, TRUE, context->cli_msg.ClientSealingKey);
    //Set ServerSealingKey to SEALKEY(NegFlg, ExportedSessionKey , "Server")
    SEALKEY(context->cli_NegFlg, ExportedSessionKey, FALSE, context->cli_msg.ServerSealingKey);
    //RC4Init(ClientHandle, ClientSealingKey)
    RC4Init(&context->cli_msg.ClientHandle, context->cli_msg.ClientSealingKey, 16);//sizeof(context->cli_msg.ClientSealingKey));
    //RC4Init(ServerHandle, ServerSealingKey)
    RC4Init(&context->cli_msg.ServerHandle, context->cli_msg.ServerSealingKey, 16);//sizeof(context->cli_msg.ServerSealingKey));

    PrintSignSealKeyInfo(&context->cli_msg);*/

done:
    NtlmAStrFree(&ExpectedLmChallengeResponse);
    NtlmAStrFree(&ExpectedNtChallengeResponse);
    return Status;
}

static
BOOL
MsvpCheckLogonHours(
    _In_ PSAMPR_LOGON_HOURS LogonHours,
    _In_ PLARGE_INTEGER LogonTime)
{
#if 0
    LARGE_INTEGER LocalLogonTime;
    TIME_FIELDS TimeFields;
    USHORT MinutesPerUnit, Offset;
    BOOL bFound;

    FIXME("MsvpCheckLogonHours(%p %p)\n", LogonHours, LogonTime);

    if (LogonHours->UnitsPerWeek == 0 || LogonHours->LogonHours == NULL)
    {
        FIXME("No logon hours!\n");
        return TRUE;
    }

    RtlSystemTimeToLocalTime(LogonTime, &LocalLogonTime);
    RtlTimeToTimeFields(&LocalLogonTime, &TimeFields);

    FIXME("UnitsPerWeek: %u\n", LogonHours->UnitsPerWeek);
    MinutesPerUnit = 10080 / LogonHours->UnitsPerWeek;

    Offset = ((TimeFields.Weekday * 24 + TimeFields.Hour) * 60 + TimeFields.Minute) / MinutesPerUnit;
    FIXME("Offset: %us\n", Offset);

    bFound = (BOOL)(LogonHours->LogonHours[Offset / 8] & (1 << (Offset % 8)));
    FIXME("Logon permitted: %s\n", bFound ? "Yes" : "No");

    return bFound;
#endif
    return TRUE;
}

static
BOOL
MsvpCheckWorkstations(
    _In_ PRPC_UNICODE_STRING WorkStations,
    _In_ PWSTR ComputerName)
{
    PWSTR pStart, pEnd;
    BOOL bFound = FALSE;

    TRACE("MsvpCheckWorkstations(%p %S)\n", WorkStations, ComputerName);

    if (WorkStations->Length == 0 || WorkStations->Buffer == NULL)
    {
        TRACE("No workstations!\n");
        return TRUE;
    }

    TRACE("Workstations: %wZ\n", WorkStations);

    pStart = WorkStations->Buffer;
    for (;;)
    {
        pEnd = wcschr(pStart, L',');
        if (pEnd != NULL)
            *pEnd = UNICODE_NULL;

        TRACE("Comparing '%S' and '%S'\n", ComputerName, pStart);
        if (_wcsicmp(ComputerName, pStart) == 0)
        {
            bFound = TRUE;
            if (pEnd != NULL)
                *pEnd = L',';
            break;
        }

        if (pEnd == NULL)
            break;

        *pEnd = L',';
        pStart = pEnd + 1;
    }

    TRACE("Found allowed workstation: %s\n", (bFound) ? "Yes" : "No");

    return bFound;
}

NTSTATUS
SamValidateNormalUser(
    IN PUNICODE_STRING UserName,
    IN PLSA_SAM_PWD_DATA PwdData,
    IN PUNICODE_STRING ComputerName,
    OUT PRPC_SID* AccountDomainSidPtr,
    OUT SAMPR_HANDLE* UserHandlePtr,
    OUT PSAMPR_USER_INFO_BUFFER* UserInfoPtr,
    OUT PNTSTATUS SubStatus)
{
    NTSTATUS Status;
    SAMPR_HANDLE ServerHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;
    PRPC_SID AccountDomainSid;
    RPC_UNICODE_STRING Names[1];
    SAMPR_HANDLE UserHandle = NULL;
    SAMPR_ULONG_ARRAY RelativeIds = {0, NULL};
    SAMPR_ULONG_ARRAY Use = {0, NULL};
    PSAMPR_USER_INFO_BUFFER UserInfo = NULL;
    LARGE_INTEGER LogonTime;
    LARGE_INTEGER AccountExpires;
    LARGE_INTEGER PasswordMustChange;
    LARGE_INTEGER PasswordLastSet;

    /* Get the account domain SID */
    Status = GetAccountDomainSid(&AccountDomainSid);
    if (!NT_SUCCESS(Status))
    {
        ERR("GetAccountDomainSid() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Connect to the SAM server */
    Status = SamIConnect(NULL,
                         &ServerHandle,
                         SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                         TRUE);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamIConnect() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Open the account domain */
    Status = SamrOpenDomain(ServerHandle,
                            DOMAIN_LOOKUP,
                            AccountDomainSid,
                            &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamrOpenDomain failed (Status %08lx)\n", Status);
        goto done;
    }

    Names[0].Length = UserName->Length;
    Names[0].MaximumLength = UserName->MaximumLength;
    Names[0].Buffer = UserName->Buffer;

    /* Try to get the RID for the user name */
    Status = SamrLookupNamesInDomain(DomainHandle,
                                     1,
                                     Names,
                                     &RelativeIds,
                                     &Use);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamrLookupNamesInDomain failed (Status %08lx)\n", Status);
        Status = STATUS_NO_SUCH_USER;
        goto done;
    }

    /* Fail, if it is not a user account */
    if (Use.Element[0] != SidTypeUser)
    {
        ERR("Account is not a user account!\n");
        Status = STATUS_NO_SUCH_USER;
        goto done;
    }

    /* Open the user object */
    Status = SamrOpenUser(DomainHandle,
                          USER_READ_GENERAL | USER_READ_LOGON |
                          USER_READ_ACCOUNT | USER_READ_PREFERENCES, /* FIXME */
                          RelativeIds.Element[0],
                          &UserHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamrOpenUser failed (Status %08lx)\n", Status);
        goto done;
    }

    Status = SamrQueryInformationUser(UserHandle,
                                      UserAllInformation,
                                      &UserInfo);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamrQueryInformationUser failed (Status %08lx)\n", Status);
        goto done;
    }

    TRACE("UserName: %wZ\n", &UserInfo->All.UserName);

    /* Check the password */
    if ((UserInfo->All.UserAccountControl & USER_PASSWORD_NOT_REQUIRED) == 0)
    {
        if (!PwdData->IsNetwork)
            Status = MsvpCheckPassword(PwdData, UserInfo);
        else
            Status = MsvpCheckNetworkPassword(PwdData, UserInfo);

        if (!NT_SUCCESS(Status))
        {
            ERR("MsvpCheckPassword failed (Status %08lx)\n", Status);
            goto done;
        }
    }

    /* Check account restrictions for non-administrator accounts */
    if (RelativeIds.Element[0] != DOMAIN_USER_RID_ADMIN)
    {
        /* Check if the account has been disabled */
        if (UserInfo->All.UserAccountControl & USER_ACCOUNT_DISABLED)
        {
            ERR("Account disabled!\n");
            *SubStatus = STATUS_ACCOUNT_DISABLED;
            Status = STATUS_ACCOUNT_RESTRICTION;
            goto done;
        }

        /* Check if the account has been locked */
        if (UserInfo->All.UserAccountControl & USER_ACCOUNT_AUTO_LOCKED)
        {
            ERR("Account locked!\n");
            *SubStatus = STATUS_ACCOUNT_LOCKED_OUT;
            Status = STATUS_ACCOUNT_RESTRICTION;
            goto done;
        }

        /* Get the logon time */
        Status = NtQuerySystemTime(&LogonTime);
        if (!NT_SUCCESS(Status))
        {
            ERR("NtQuerySystemTime failed 0x%lx.\n", Status);
            goto done;
        }

        /* Check if the account expired */
        if (!(UserInfo->All.UserAccountControl & USER_DONT_EXPIRE_PASSWORD))
        {
            AccountExpires.LowPart = UserInfo->All.AccountExpires.LowPart;
            AccountExpires.HighPart = UserInfo->All.AccountExpires.HighPart;
            if (LogonTime.QuadPart >= AccountExpires.QuadPart)
            {
                ERR("Account expired!\n");
                *SubStatus = STATUS_ACCOUNT_EXPIRED;
                Status = STATUS_ACCOUNT_RESTRICTION;
                goto done;
            }

            /* Check if the password expired */
            PasswordMustChange.LowPart = UserInfo->All.PasswordMustChange.LowPart;
            PasswordMustChange.HighPart = UserInfo->All.PasswordMustChange.HighPart;
            PasswordLastSet.LowPart = UserInfo->All.PasswordLastSet.LowPart;
            PasswordLastSet.HighPart = UserInfo->All.PasswordLastSet.HighPart;

            if (LogonTime.QuadPart >= PasswordMustChange.QuadPart)
            {
                ERR("Password expired!\n");
                if (PasswordLastSet.QuadPart == 0)
                    *SubStatus = STATUS_PASSWORD_MUST_CHANGE;
                else
                    *SubStatus = STATUS_PASSWORD_EXPIRED;

                Status = STATUS_ACCOUNT_RESTRICTION;
                goto done;
            }
        }

        /* Check logon hours */
        if (!MsvpCheckLogonHours(&UserInfo->All.LogonHours, &LogonTime))
        {
            ERR("Invalid logon hours!\n");
            *SubStatus = STATUS_INVALID_LOGON_HOURS;
            Status = STATUS_ACCOUNT_RESTRICTION;
            goto done;
        }

        /* Check workstations */
        if (!MsvpCheckWorkstations(&UserInfo->All.WorkStations, ComputerName->Buffer))
        {
            ERR("Invalid workstation!\n");
            *SubStatus = STATUS_INVALID_WORKSTATION;
            Status = STATUS_ACCOUNT_RESTRICTION;
            goto done;
        }
    }
done:
    if (NT_SUCCESS(Status))
    {
        *UserHandlePtr = UserHandle;
        *AccountDomainSidPtr = AccountDomainSid;
        *UserInfoPtr = UserInfo;
    }
    else
    {
        if (AccountDomainSid != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, AccountDomainSid);

        if (UserHandle != NULL)
            SamrCloseHandle(&UserHandle);

        SamIFree_SAMPR_USER_INFO_BUFFER(UserInfo,
                                        UserAllInformation);
    }

    SamIFree_SAMPR_ULONG_ARRAY(&RelativeIds);
    SamIFree_SAMPR_ULONG_ARRAY(&Use);

    if (DomainHandle != NULL)
        SamrCloseHandle(&DomainHandle);

    if (ServerHandle != NULL)
        SamrCloseHandle(&ServerHandle);

    return Status;
}

static
NTSTATUS
GetNtAuthorityDomainSid(PRPC_SID *Sid)
{
    SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
    ULONG Length = 0;

    Length = RtlLengthRequiredSid(0);
    *Sid = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
    if (*Sid == NULL)
    {
        ERR("Failed to allocate SID\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitializeSid(*Sid,&NtAuthority, 0);

    return STATUS_SUCCESS;
}

NTSTATUS
SamValidateUser(
    IN SECURITY_LOGON_TYPE LogonType,
    IN PUNICODE_STRING LogonUserName,
    IN PUNICODE_STRING LogonDomain,
    IN PLSA_SAM_PWD_DATA LogonPwdData,
    IN PUNICODE_STRING ComputerName,
    OUT PBOOL SpecialAccount,
    OUT PRPC_SID* AccountDomainSidPtr,
    OUT SAMPR_HANDLE* UserHandlePtr,
    OUT PSAMPR_USER_INFO_BUFFER* UserInfoPtr,
    OUT PNTSTATUS SubStatus)
{
    static const UNICODE_STRING NtAuthorityU = RTL_CONSTANT_STRING(L"NT AUTHORITY");
    static const UNICODE_STRING LocalServiceU = RTL_CONSTANT_STRING(L"LocalService");
    static const UNICODE_STRING NetworkServiceU = RTL_CONSTANT_STRING(L"NetworkService");

    NTSTATUS Status = STATUS_SUCCESS;

    *SpecialAccount = FALSE;
    //ULONG ComputerNameSize;
    //WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];

    /* Get the computer name */
    //ComputerNameSize = ARRAYSIZE(ComputerName);
    //GetComputerNameW(ComputerName, &ComputerNameSize);

    /* Check for special accounts */
    // FIXME: Windows does not do this that way!! (msv1_0 does not contain these hardcoded values)
    // FIXME: Is this only for local logins needed?
    if (RtlEqualUnicodeString(LogonDomain, &NtAuthorityU, TRUE))
    {
        *SpecialAccount = TRUE;

        /* Get the authority domain SID */
        Status = GetNtAuthorityDomainSid(AccountDomainSidPtr);
        if (!NT_SUCCESS(Status))
        {
            ERR("GetNtAuthorityDomainSid() failed (Status 0x%08lx)\n", Status);
            return Status;
        }

        if (RtlEqualUnicodeString(LogonUserName, &LocalServiceU, TRUE))
        {
            TRACE("SpecialAccount: LocalService\n");

            if (LogonType != Service)
                return STATUS_LOGON_FAILURE;

            *UserInfoPtr = RtlAllocateHeap(RtlGetProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       sizeof(SAMPR_USER_ALL_INFORMATION));
            if (*UserInfoPtr == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            (*UserInfoPtr)->All.UserId = SECURITY_LOCAL_SERVICE_RID;
            (*UserInfoPtr)->All.PrimaryGroupId = SECURITY_LOCAL_SERVICE_RID;
        }
        else if (RtlEqualUnicodeString(LogonUserName, &NetworkServiceU, TRUE))
        {
            TRACE("SpecialAccount: NetworkService\n");

            if (LogonType != Service)
                return STATUS_LOGON_FAILURE;

            *UserInfoPtr = RtlAllocateHeap(RtlGetProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       sizeof(SAMPR_USER_ALL_INFORMATION));
            if (*UserInfoPtr == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            (*UserInfoPtr)->All.UserId = SECURITY_NETWORK_SERVICE_RID;
            (*UserInfoPtr)->All.PrimaryGroupId = SECURITY_NETWORK_SERVICE_RID;
        }
        else
        {
            Status = STATUS_NO_SUCH_USER;
            goto done;
        }
    }
    else
    {
        TRACE("NormalAccount\n");
        #if 1 /* use extracted function */
        Status = SamValidateNormalUser(
            LogonUserName,
            LogonPwdData,
            ComputerName,
            AccountDomainSidPtr,
            UserHandlePtr,
            UserInfoPtr,
            SubStatus);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamValidateUser() failed (Status 0x%08lx)\n", Status);
            return Status;
        }
        #else
        /* Get the account domain SID */
        Status = GetAccountDomainSid(&AccountDomainSid);
        if (!NT_SUCCESS(Status))
        {
            ERR("GetAccountDomainSid() failed (Status 0x%08lx)\n", Status);
            return Status;
        }

        /* Connect to the SAM server */
        Status = SamIConnect(NULL,
                             &ServerHandle,
                             SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                             TRUE);
        if (!NT_SUCCESS(Status))
        {
            TRACE("SamIConnect() failed (Status 0x%08lx)\n", Status);
            goto done;
        }

        /* Open the account domain */
        Status = SamrOpenDomain(ServerHandle,
                                DOMAIN_LOOKUP,
                                AccountDomainSid,
                                &DomainHandle);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamrOpenDomain failed (Status %08lx)\n", Status);
            goto done;
        }

        Names[0].Length = LogonInfo->UserName.Length;
        Names[0].MaximumLength = LogonInfo->UserName.MaximumLength;
        Names[0].Buffer = LogonInfo->UserName.Buffer;

        /* Try to get the RID for the user name */
        Status = SamrLookupNamesInDomain(DomainHandle,
                                         1,
                                         Names,
                                         &RelativeIds,
                                         &Use);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamrLookupNamesInDomain failed (Status %08lx)\n", Status);
            Status = STATUS_NO_SUCH_USER;
            goto done;
        }

        /* Fail, if it is not a user account */
        if (Use.Element[0] != SidTypeUser)
        {
            ERR("Account is not a user account!\n");
            Status = STATUS_NO_SUCH_USER;
            goto done;
        }

        /* Open the user object */
        Status = SamrOpenUser(DomainHandle,
                              USER_READ_GENERAL | USER_READ_LOGON |
                              USER_READ_ACCOUNT | USER_READ_PREFERENCES, /* FIXME */
                              RelativeIds.Element[0],
                              &UserHandle);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamrOpenUser failed (Status %08lx)\n", Status);
            goto done;
        }

        Status = SamrQueryInformationUser(UserHandle,
                                          UserAllInformation,
                                          &UserInfo);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamrQueryInformationUser failed (Status %08lx)\n", Status);
            goto done;
        }

        TRACE("UserName: %wZ\n", &UserInfo->All.UserName);

        /* Check the password */
        if ((UserInfo->All.UserAccountControl & USER_PASSWORD_NOT_REQUIRED) == 0)
        {
            Status = MsvpCheckPassword(&LogonInfo->Password,
                                       UserInfo);
            if (!NT_SUCCESS(Status))
            {
                ERR("MsvpCheckPassword failed (Status %08lx)\n", Status);
                goto done;
            }
        }

        /* Check account restrictions for non-administrator accounts */
        if (RelativeIds.Element[0] != DOMAIN_USER_RID_ADMIN)
        {
            /* Check if the account has been disabled */
            if (UserInfo->All.UserAccountControl & USER_ACCOUNT_DISABLED)
            {
                ERR("Account disabled!\n");
                *SubStatus = STATUS_ACCOUNT_DISABLED;
                Status = STATUS_ACCOUNT_RESTRICTION;
                goto done;
            }

            /* Check if the account has been locked */
            if (UserInfo->All.UserAccountControl & USER_ACCOUNT_AUTO_LOCKED)
            {
                ERR("Account locked!\n");
                *SubStatus = STATUS_ACCOUNT_LOCKED_OUT;
                Status = STATUS_ACCOUNT_RESTRICTION;
                goto done;
            }

            /* Check if the account expired */
            AccountExpires.LowPart = UserInfo->All.AccountExpires.LowPart;
            AccountExpires.HighPart = UserInfo->All.AccountExpires.HighPart;
            if (LogonTime.QuadPart >= AccountExpires.QuadPart)
            {
                ERR("Account expired!\n");
                *SubStatus = STATUS_ACCOUNT_EXPIRED;
                Status = STATUS_ACCOUNT_RESTRICTION;
                goto done;
            }

            /* Check if the password expired */
            PasswordMustChange.LowPart = UserInfo->All.PasswordMustChange.LowPart;
            PasswordMustChange.HighPart = UserInfo->All.PasswordMustChange.HighPart;
            PasswordLastSet.LowPart = UserInfo->All.PasswordLastSet.LowPart;
            PasswordLastSet.HighPart = UserInfo->All.PasswordLastSet.HighPart;

            if (LogonTime.QuadPart >= PasswordMustChange.QuadPart)
            {
                ERR("Password expired!\n");
                if (PasswordLastSet.QuadPart == 0)
                    *SubStatus = STATUS_PASSWORD_MUST_CHANGE;
                else
                    *SubStatus = STATUS_PASSWORD_EXPIRED;

                Status = STATUS_ACCOUNT_RESTRICTION;
                goto done;
            }

            /* Check logon hours */
            if (!MsvpCheckLogonHours(&UserInfo->All.LogonHours, &LogonTime))
            {
                ERR("Invalid logon hours!\n");
                *SubStatus = STATUS_INVALID_LOGON_HOURS;
                Status = STATUS_ACCOUNT_RESTRICTION;
                goto done;
            }

            /* Check workstations */
            if (!MsvpCheckWorkstations(&UserInfo->All.WorkStations, ComputerName))
            {
                ERR("Invalid workstation!\n");
                *SubStatus = STATUS_INVALID_WORKSTATION;
                Status = STATUS_ACCOUNT_RESTRICTION;
                goto done;
            }
        }
        #endif
    }
done:
    return Status;
}

