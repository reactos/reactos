/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/msv1_0.c
 * PURPOSE:     Functions needed to fill PSECPKG_USER_FUNCTION_TABLE
 *              (SpUserModeInitialize)
 * COPYRIGHT:   Copyright 2019 Andreas Maier <staubim@quantentunnel.de>
 */
#ifndef _CONTEXT_H_
#define _CONTEXT_H_

SECURITY_STATUS
SEC_ENTRY
NtlmInitializeSecurityContext(
    IN OPTIONAL LSA_SEC_HANDLE hCredential,
    IN OPTIONAL LSA_SEC_HANDLE hContext,
    IN OPTIONAL SEC_WCHAR *pszTargetName,
    IN ULONG fContextReq,
    IN ULONG Reserved1,
    IN ULONG TargetDataRep,
    IN OPTIONAL PSecBufferDesc pInput,
    IN ULONG Reserved2,
    IN OUT OPTIONAL PLSA_SEC_HANDLE phNewContext,
    IN OUT OPTIONAL PSecBufferDesc pOutput,
    OUT ULONG *pfContextAttr,
    OUT OPTIONAL PTimeStamp ptsExpiry);

SECURITY_STATUS
SEC_ENTRY
NtlmAcceptSecurityContext(
    IN LSA_SEC_HANDLE hCredential,
    IN LSA_SEC_HANDLE hContext,
    IN PSecBufferDesc pInput,
    IN ULONG fContextReq,
    IN ULONG TargetDataRep,
    IN OUT PLSA_SEC_HANDLE phNewContext,
    IN OUT PSecBufferDesc pOutput,
    OUT ULONG *pfContextAttr,
    OUT PTimeStamp ptsExpiry);

SECURITY_STATUS
SEC_ENTRY
NtlmQueryContextAttributesAW(
    IN LSA_SEC_HANDLE hContext,
    IN ULONG ulAttribute,
    OUT PVOID pBuffer,
    IN BOOL isUnicode);

#endif
