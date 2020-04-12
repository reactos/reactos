/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/sam.h
 * PURPOSE:     SAM releated functions - header file
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */
#ifndef _MSV1_0_SAM_H_
#define _MSV1_0_SAM_H_

typedef enum _LSA_SAM_NETLOGON_TYP
{
    NetLogonAnonymouse = 0,
    NetLogonLmKey,
    NetLogonNtKey
} LSA_SAM_NETLOGON_TYP;

//TODO Add IsSpecialAccount - variable ...
typedef struct _LSA_SAM_PWD_DATA
{
    /* TRUE: PlainPwd is filled, F
     * FALSE: LmPwd and NtPwd is filled  */
    BOOL IsNetwork;
    PUNICODE_STRING PlainPwd;
    //PSTRING LmPwd;
    //PSTRING NtPwd;

    // IsNetwork = TRUE
    // IsNetwork - input
    PMSV1_0_LM20_LOGON LogonInfo;
    PUNICODE_STRING ComputerName;
    // IsNetwork - result
    LSA_SAM_NETLOGON_TYP LogonType;
    LM_SESSION_KEY LmSessionKey;
    USER_SESSION_KEY UserSessionKey;
} LSA_SAM_PWD_DATA, *PLSA_SAM_PWD_DATA;

/**
 * @brief Validates a user by ...
 *        Returns the AccontDomainSid
 * @param PSID
 */
NTSTATUS
SamValidateNormalUser(
    IN PUNICODE_STRING UserName,
    IN PLSA_SAM_PWD_DATA PwdData,
    IN PUNICODE_STRING ComputerName,
    OUT PRPC_SID* AccountDomainSidPtr,
    OUT SAMPR_HANDLE* UserHandlePtr,
    OUT PSAMPR_USER_INFO_BUFFER* UserInfoPtr,
    OUT PNTSTATUS SubStatus);

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
    OUT PNTSTATUS SubStatus);

#endif
