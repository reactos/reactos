/*
 * PROJECT:     ReactOS lsass_apitest
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     test functions from PSECPKG_FUNCTION_TABLE
 *              (returned by SpLsaModeInitialize)
 * COPYRIGHT:   Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 */

#ifndef _LSAFNTABLE_H_
#define _LSAFNTABLE_H_

void LsaFnInit();
void LsaFnFini();

NTSTATUS NTAPI
LsaFnCreateLogonSession(
    _In_ PLUID LogonId);

NTSTATUS NTAPI
LsaFnDeleteLogonSession(PLUID p1);

NTSTATUS NTAPI
LsaFnAddCredential(
    PLUID p1,
    ULONG p2,
    PLSA_STRING p3,
    PLSA_STRING p4);

NTSTATUS NTAPI
LsaFnGetCredentials(
    PLUID p1,
    ULONG p2,
    PULONG p3,
    BOOLEAN p4,
    PLSA_STRING p5,
    PULONG p6,
    PLSA_STRING p7);


NTSTATUS NTAPI
LsaFnDeleteCredential(
    PLUID p1,
    ULONG p2,
    PLSA_STRING p3);

PVOID NTAPI
LsaFnAllocateLsaHeap(
    ULONG Length);

VOID NTAPI
LsaFnFreeLsaHeap(
    PVOID Base);

NTSTATUS NTAPI
LsaFnAllocateClientBuffer(
    PLSA_CLIENT_REQUEST p1,
    ULONG p2,
    PVOID *p3);

NTSTATUS NTAPI
LsaFnFreeClientBuffer(
    PLSA_CLIENT_REQUEST p1,
    PVOID p2);

NTSTATUS NTAPI
LsaFnCopyToClientBuffer(
    PLSA_CLIENT_REQUEST p1,
    ULONG p2,
    PVOID p3,
    PVOID p4);

NTSTATUS NTAPI
LsaFnCopyFromClientBuffer(
    PLSA_CLIENT_REQUEST p1,
    ULONG p2,
    PVOID p3,
    PVOID p4);

NTSTATUS NTAPI
LsaFnImpersonateClient(void);

NTSTATUS NTAPI LsaFnUnloadPackage(void);

NTSTATUS NTAPI
LsaFnDuplicateHandle(
    HANDLE p1,
    PHANDLE p2);

NTSTATUS NTAPI
LsaFnSaveSupplementalCredentials(
    PLUID p1,
    ULONG p2,
    PVOID p3,
    BOOLEAN p4);

HANDLE NTAPI
LsaFnCreateThread(
    SEC_ATTRS p1,
    ULONG p2,
    SEC_THREAD_START p3,
    PVOID p4, ULONG p5,
    PULONG p6);

NTSTATUS NTAPI
LsaFnGetClientInfo(
    PSECPKG_CLIENT_INFO ClientInfo);

HANDLE NTAPI
LsaFnRegisterNotification(
    SEC_THREAD_START p1,
    PVOID p2,
    ULONG p3,
    ULONG p4,
    ULONG p5,
    ULONG p6,
    HANDLE p7);

NTSTATUS NTAPI
LsaFnCancelNotification(
    HANDLE p1);

NTSTATUS NTAPI
LsaFnMapBuffer(
    PSecBuffer p1,
    PSecBuffer p2);

NTSTATUS NTAPI
LsaFnCreateToken(
    PLUID p1,
    PTOKEN_SOURCE p2,
    SECURITY_LOGON_TYPE p3,
    SECURITY_IMPERSONATION_LEVEL p4,
    LSA_TOKEN_INFORMATION_TYPE p5,
    PVOID p6,
    PTOKEN_GROUPS p7,
    PUNICODE_STRING p8,
    PUNICODE_STRING p9,
    PUNICODE_STRING p10,
    PUNICODE_STRING p11,
    PHANDLE p12,
    PNTSTATUS p13);

VOID NTAPI LsaFnAuditLogon(
    NTSTATUS p1,
    NTSTATUS p2,
    PUNICODE_STRING p3,
    PUNICODE_STRING p4,
    PUNICODE_STRING p5,
    OPTIONAL PSID p6,
    SECURITY_LOGON_TYPE p7,
    PTOKEN_SOURCE p8,
    PLUID p9);

NTSTATUS NTAPI
LsaFnCallPackage(
    PUNICODE_STRING p1,
    PVOID p2,
    ULONG p3,
    PVOID *p4,
    PULONG p5,
    PNTSTATUS p6);

VOID NTAPI
LsaFnFreeReturnBuffer(
    PVOID p1);

BOOLEAN NTAPI
LsaFnGetCallInfo(
    PSECPKG_CALL_INFO Info);

NTSTATUS NTAPI
LsaFnCallPackageEx(
    PUNICODE_STRING p1,
    PVOID p2,
    PVOID p3,
    ULONG p4,
    PVOID *p5,
    PULONG p6,
    PNTSTATUS p7);

PVOID NTAPI
LsaFnCreateSharedMemory(
    ULONG p1,
    ULONG p2);

PVOID NTAPI
LsaFnAllocateSharedMemory(
    PVOID p1,
    ULONG p2);

VOID NTAPI
LsaFnFreeSharedMemory(
    PVOID p1,
    PVOID p2);

BOOLEAN NTAPI
LsaFnDeleteSharedMemory(
    PVOID p1);

NTSTATUS NTAPI
LsaFnOpenSamUser(
    PUNICODE_STRING p1,
    SECPKG_NAME_TYPE p2,
    PUNICODE_STRING p3,
    BOOLEAN p4,
    ULONG p5,
    PVOID* p6);

NTSTATUS NTAPI
LsaFnGetUserCredentials(
    PVOID p1,
    PVOID *p2,
    PULONG p3,
    PVOID *p4,
    PULONG p5);

NTSTATUS NTAPI
LsaFnGetUserAuthData(
    PVOID p1,
    PUCHAR *p2,
    PULONG p3);

NTSTATUS NTAPI
LsaFnCloseSamUser(
    PVOID p1);

NTSTATUS NTAPI
LsaFnConvertAuthDataToToken(
    PVOID p1,
    ULONG p2,
    SECURITY_IMPERSONATION_LEVEL p3,
    PTOKEN_SOURCE p4,
    SECURITY_LOGON_TYPE p5,
    PUNICODE_STRING p6,
    PHANDLE p7,
    PLUID p8,
    PUNICODE_STRING p9,
    PNTSTATUS p10);

NTSTATUS NTAPI
LsaFnClientCallback(
    PCHAR p1,
    ULONG_PTR p2,
    ULONG_PTR p3,
    PSecBuffer p4,
    PSecBuffer p5);

NTSTATUS NTAPI
LsaFnUpdateCredentials(
    PSECPKG_PRIMARY_CRED p1,
    PSECPKG_SUPPLEMENTAL_CRED_ARRAY p2);

NTSTATUS NTAPI
LsaFnGetAuthDataForUser(
    PUNICODE_STRING p1,
    SECPKG_NAME_TYPE p2,
    PUNICODE_STRING p3,
    PUCHAR *p4,
    PULONG p5,
    PUNICODE_STRING p6);

NTSTATUS NTAPI
LsaFnCrackSingleName(
    ULONG p1,
    BOOLEAN p2,
    PUNICODE_STRING p3,
    PUNICODE_STRING p4,
    ULONG p5,
    PUNICODE_STRING p6,
    PUNICODE_STRING p7,
    PULONG p8);

NTSTATUS NTAPI
LsaFnAuditAccountLogon(
    ULONG p1,
    BOOLEAN p2,
    PUNICODE_STRING p3,
    PUNICODE_STRING p4,
    PUNICODE_STRING p5,
    NTSTATUS p6);

NTSTATUS NTAPI
LsaFnCallPackagePassthrough(
    PUNICODE_STRING p1,
    PVOID p2,
    PVOID p3,
    ULONG p4,
    PVOID*p5,
    PULONG p6,
    PNTSTATUS p7);

void NTAPI
LsaFnDummyFunctionX(
    PVOID p1,
    ULONG p2);

void NTAPI
LsaFnProtectMemory(
    PVOID Buffer,
    ULONG BufferSize);

void NTAPI
LsaFnUnprotectMemory(
    PVOID Buffer,
    ULONG BufferSize);

NTSTATUS NTAPI
LsaFnOpenTokenByLogonId(
    _In_  PLUID LogonId,
    _Out_ HANDLE *RetTokenHandle);

NTSTATUS NTAPI
LsaFnExpandAuthDataForDomain(
    _In_  PUCHAR UserAuthData,
    _In_  ULONG UserAuthDataSize,
    _In_  PVOID Reserved,
    _Out_ PUCHAR *ExpandedAuthData,
    _In_  PULONG ExpandedAuthDataSize);

PVOID NTAPI
LsaFnAllocatePrivateHeap(SIZE_T Length);

void NTAPI
LsaFnFreePrivateHeap(
    _In_ PVOID Base);

#endif
