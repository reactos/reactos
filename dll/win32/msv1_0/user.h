/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/user.c
 * PURPOSE:     header for user.c
 * COPYRIGHT:   Copyright 2019-2020 Andreas Maier <staubim@quantentunnel.de>
 */
#ifndef _MSV1_0_USER_H_
#define _MSV1_0_USER_H_


// functions provided by LSA in SpInitialize (user mode)
extern PSECPKG_DLL_FUNCTIONS UsrFunctionTable;
// functions we provide to LSA in SpLsaModeInitialize (user mode)
extern SECPKG_USER_FUNCTION_TABLE UsrTables[1];

NTSTATUS NTAPI
SpInstanceInit(
    ULONG Version,
    PSECPKG_DLL_FUNCTIONS FunctionTable,
    PVOID *UserFunctions);

NTSTATUS NTAPI
UsrSpMakeSignature(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG QualityOfProtection,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber);

NTSTATUS NTAPI
UsrSpVerifySignature(
    LSA_SEC_HANDLE p1,
    PSecBufferDesc p2,
    ULONG p3,
    PULONG p4);

NTSTATUS NTAPI
UsrSpSealMessage(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG QualityOfProtection,
    IN OUT PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber);

NTSTATUS NTAPI
UsrSpUnsealMessage(
    LSA_SEC_HANDLE ContextHandle,
    PSecBufferDesc MessageBuffers,
    ULONG MessageSequenceNumber,
    PULONG QualityOfProtection);

NTSTATUS NTAPI
UsrSpGetContextToken(
    LSA_SEC_HANDLE ContextHandle,
    PHANDLE ImpersonationToken);

NTSTATUS NTAPI
UsrSpQueryContextAttributes(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextAttribute,
    IN OUT PVOID Buffer);

NTSTATUS NTAPI
UsrSpCompleteAuthToken(
    LSA_SEC_HANDLE ContextHandle,
    PSecBufferDesc InputBuffer);

NTSTATUS NTAPI
UsrSpDeleteUserModeContext(
    LSA_SEC_HANDLE p1);

NTSTATUS NTAPI
UsrSpFormatCredentials(
    PSecBuffer p1,
    PSecBuffer p2);

NTSTATUS NTAPI
UsrSpMarshallSupplementalCreds(
    ULONG p1,
    PUCHAR p2,
    PULONG p3,
    PVOID *p4);

NTSTATUS NTAPI
UsrSpExportSecurityContext(
    LSA_SEC_HANDLE p1,
    ULONG p2,
    PSecBuffer p3,
    PHANDLE p4);

NTSTATUS NTAPI
UsrSpImportSecurityContext(
    PSecBuffer p1,
    HANDLE p2,
    PLSA_SEC_HANDLE p3);

void _fdTRACE(
    IN const char *s,
    IN const char *file,
    IN const int line, ...);
#define fdTRACE(a,...) _fdTRACE(a,__FILE__,__LINE__ ,##__VA_ARGS__)

#endif /* _MSV1_0_USER_H_ */
