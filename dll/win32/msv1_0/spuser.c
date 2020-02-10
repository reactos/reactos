/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/msv1_0.c
 * PURPOSE:     Functions needed to fill PSECPKG_USER_FUNCTION_TABLE
 *              (SpUserModeInitialize)
 * COPYRIGHT:   Copyright 2019 Andreas Maier <staubim@quantentunnel.de>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(msv1_0);

#define LSA_VERSION 1

PSECPKG_DLL_FUNCTIONS UsrFunctionTable = NULL;
SECPKG_USER_FUNCTION_TABLE UsrTables[1];

ULONG x = 0;

NTSTATUS NTAPI
SpInstanceInit(
    ULONG Version,
    PSECPKG_DLL_FUNCTIONS FunctionTable,
    PVOID *UserFunctions)
{
    fdTRACE("SpInstanceInit(Version 0x%lx, %p, %p)\n",
          Version, FunctionTable, UserFunctions);

    /* I'm not sure if this can really happen. */
    if (UsrFunctionTable != NULL)
    {
        WARN("UsrSpInstanceInit already called!\n");
        //return STATUS_UNSUCCESSFUL;
    }

    if (Version != LSA_VERSION)
    {
        ERR("UsrSpInstanceInit - unsupported version!\n");
        return STATUS_NOT_SUPPORTED;
    }

    UsrFunctionTable = FunctionTable;
    NtlmInit(NtlmUserMode);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
UsrSpMakeSignature(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG QualityOfProtection,
    IN OUT PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber)
{
    PNTLMSSP_CONTEXT_USR UsrContext;
    NTSTATUS Status;

    fdTRACE("UsrSpMakeSignature(%p 0x%x %p 0x%x)\n",
          ContextHandle, QualityOfProtection,
          MessageBuffers, MessageSequenceNumber);

    UsrContext = NtlmUsrReferenceContext(ContextHandle, TRUE);
    if (UsrContext == NULL)
    {
        TRACE("Invalid context handle 0x%x\n", ContextHandle);
        return STATUS_INVALID_HANDLE;
    }
    Status = NtlmEncryptMessage(UsrContext->Hdr, QualityOfProtection,
                                MessageBuffers, MessageSequenceNumber, TRUE);
    NtlmUsrDereferenceContext(UsrContext);

    fdTRACE("Status %x\n", Status);
    return Status;
}

NTSTATUS NTAPI
UsrSpVerifySignature(
    LSA_SEC_HANDLE p1,
    PSecBufferDesc p2,
    ULONG p3,
    PULONG p4)
{
    fdTRACE("*** UNIMPLEMENTED *** UsrSpVerifySignature(%p 0x%x %p 0x%x)\n",
          p1, p2, p3, p4);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
UsrSpSealMessage(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG QualityOfProtection,
    IN OUT PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber)
{
    PNTLMSSP_CONTEXT_USR UsrContext;
    NTSTATUS Status;

    fdTRACE("UsrSpSealMessage(%p 0x%x %p 0x%x)\n",
          ContextHandle, QualityOfProtection,
          MessageBuffers, MessageSequenceNumber);

    UsrContext = NtlmUsrReferenceContext(ContextHandle, TRUE);
    if (UsrContext == NULL)
    {
        TRACE("Invalid context handle 0x%x\n", ContextHandle);
        return STATUS_INVALID_HANDLE;
    }
    Status = NtlmEncryptMessage(UsrContext->Hdr, QualityOfProtection,
                                MessageBuffers, MessageSequenceNumber, FALSE);
    NtlmUsrDereferenceContext(UsrContext);

    fdTRACE("Status %x\n", Status);
    return Status;
}

NTSTATUS NTAPI
UsrSpUnsealMessage(
    LSA_SEC_HANDLE p1,
    PSecBufferDesc p2,
    ULONG p3,
    PULONG p4)
{
    fdTRACE("*** UNIMPLEMENTED *** UsrSpUnsealMessage(%p 0x%x %p 0x%x)\n",
          p1, p2, p3, p4);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
UsrSpGetContextToken(
    LSA_SEC_HANDLE ContextHandle,
    PHANDLE ImpersonationToken)
{
    PNTLMSSP_CONTEXT_USR UsrContext;

    fdTRACE("UsrSpGetContextToken(%p %p)\n",
          ContextHandle, ImpersonationToken);

    if (ImpersonationToken == NULL)
    {
        ERR("ImpersonationToken is NULL.\n");
        return STATUS_INVALID_PARAMETER;
    }

    UsrContext = NtlmUsrReferenceContext(ContextHandle, TRUE);
    // TODO: UsrContext(?->Hdr) should have a ImpersonationToken
    //       If that Token = NULL return STATUS_INVALID_HANDLE
    if (UsrContext == NULL)
    {
        ERR("Invalid user context handle 0x%x.\n", ContextHandle);
        return STATUS_INVALID_HANDLE;
    }

    //FIXME: do something

    // I dont know where to create or for what this is
    // needed. So i give a handle that will easy to know.
    *ImpersonationToken = (HANDLE)0xBEEFFEEB;

    NtlmUsrDereferenceContext(UsrContext);
    return SEC_E_OK;
}

NTSTATUS NTAPI
UsrSpQueryContextAttributes(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextAttribute,
    IN OUT PVOID Buffer)
{
    SECURITY_STATUS Status;
    PNTLMSSP_CONTEXT_USR UsrContext;

    fdTRACE("UsrSpQueryContextAttributes(%p 0x%x %p)\n",
            ContextHandle, ContextAttribute, Buffer);

    UsrContext = NtlmUsrReferenceContext(ContextHandle, TRUE);
    if (UsrContext == NULL)
    {
        ERR("Invalid context handle 0x%x\n", ContextHandle);
        return STATUS_INVALID_HANDLE;
    }

    Status = NtlmQueryContextAttributes(UsrContext->Hdr, ContextAttribute, Buffer, TRUE);

    NtlmUsrDereferenceContext(UsrContext);

    fdTRACE("Status %x\n", Status);
    return Status;
}

NTSTATUS NTAPI
UsrSpCompleteAuthToken(
    LSA_SEC_HANDLE ContextHandle,
    PSecBufferDesc InputBuffer)
{
    fdTRACE("UsrSpCompleteAuthToken(%p %p)\n",
          ContextHandle, InputBuffer);

    UsrContext = NtlmUsrReferenceContext(ContextHandle, TRUE);
    if (UsrContext == NULL)
    {
        ERR("Invalid context handle 0x%x\n", ContextHandle);
        return STATUS_INVALID_HANDLE;
    }

    //FIXME: do something

    NtlmUsrDereferenceContext(UsrContext);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
UsrSpDeleteUserModeContext(
    LSA_SEC_HANDLE ContextHandle)
{
    PNTLMSSP_CONTEXT_USR UsrContext;

    fdTRACE("UsrSpDeleteUserModeContext(%p)\n",
          ContextHandle);

    // get user context without increasing ref count
    UsrContext = NtlmUsrReferenceContext(ContextHandle, FALSE);
    if (UsrContext == NULL)
    {
        TRACE("Invalid handle 0x%x\n", ContextHandle);
        return STATUS_INVALID_HANDLE;
    }
    // decrease refcount
    NtlmUsrDereferenceContext(UsrContext);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
UsrSpFormatCredentials(
    PSecBuffer p1,
    PSecBuffer p2)
{
    fdTRACE("*** UNIMPLEMENTED *** UsrSpFormatCredentials(%p %p)\n",
          p1, p2);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
UsrSpMarshallSupplementalCreds(
    ULONG p1,
    PUCHAR p2,
    PULONG p3,
    PVOID *p4)
{
    fdTRACE("*** UNIMPLEMENTED *** UsrSpMarshallSupplementalCreds(0x%x %p %p %p)\n",
          p1, p2, p3, p4);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
UsrSpExportSecurityContext(
    LSA_SEC_HANDLE p1,
    ULONG p2,
    PSecBuffer p3,
    PHANDLE p4)
{
    fdTRACE("*** UNIMPLEMENTED *** UsrSpExportSecurityContext(%p 0x%x %p 0x%x)\n",
          p1, p2, p3, p4);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
UsrSpImportSecurityContext(
    PSecBuffer p1,
    HANDLE p2,
    PLSA_SEC_HANDLE p3)
{
    fdTRACE("*** UNIMPLEMENTED *** UsrSpImportSecurityContext(%p 0x%x %p)\n",
          p1, p2, p3);

    return ERROR_NOT_SUPPORTED;
}

void _fdTRACE(
    IN const char *s,
    IN const char *file,
    IN const int line, ...)
{
    va_list ap;
    char buf[2048];
    char buf2[2048];
    char *filepart;

    va_start(ap, line);
    vsprintf(buf, s, ap);
    va_end(ap);

    filepart = strrchr(file, '\\');
    if (filepart == NULL)
        filepart = strrchr(file, '/');
    if (filepart == NULL)
        filepart = "\\";

    sprintf(buf2, "%s:%i [0x%04lx:0x%04lx] %s",
            filepart, line, GetCurrentProcessId(), GetCurrentThreadId(), buf);
    TRACE(buf2);
}
