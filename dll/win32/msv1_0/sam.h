/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/sam.h
 * PURPOSE:     SAM releated functions - header file
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */
#ifndef _MSV1_0_SAM_H_
#define _MSV1_0_SAM_H_

typedef enum _LSA_SAM_NETLOGON_TYPE
{
    NetLogonAnonymous = 0,
    NetLogonLmKey,
    NetLogonNtKey
} LSA_SAM_NETLOGON_TYPE;

typedef struct _LSA_SAM_PWD_DATA
{
    /* TRUE: PlainPwd is filled,
       FALSE: LmPwd and NtPwd is filled */
    BOOL IsNetwork;
    PUNICODE_STRING PlainPwd;

    /* Input (IsNetwork = TRUE) */
    PMSV1_0_LM20_LOGON LogonInfo;
    PUNICODE_STRING ComputerName;
    /* Result (IsNetwork = TRUE) */
    LSA_SAM_NETLOGON_TYPE LogonType;
    LANMAN_SESSION_KEY LanmanSessionKey;
    USER_SESSION_KEY UserSessionKey;
} LSA_SAM_PWD_DATA, *PLSA_SAM_PWD_DATA;

/**
 * @brief Validates a normal user by checking if it exists in the sam database.
 *        Further some other checks are done.
 */
NTSTATUS
SamValidateUser(
    _In_ SECURITY_LOGON_TYPE LogonType,
    _In_ PUNICODE_STRING LogonUserName,
    _In_ PUNICODE_STRING LogonDomain,
    _In_ PLSA_SAM_PWD_DATA LogonPwdData,
    _In_ PUNICODE_STRING ComputerName,
    _Out_ PBOOL SpecialAccount,
    _Out_ PRPC_SID* AccountDomainSidPtr,
    _Out_ SAMPR_HANDLE* UserHandlePtr,
    _Out_ PSAMPR_USER_INFO_BUFFER* UserInfoPtr,
    _Out_ PNTSTATUS SubStatus);

#endif
