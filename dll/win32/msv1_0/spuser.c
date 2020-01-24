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
    //NtlmInit(NtlmUserMode);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
UsrSpInitUserModeContext(
    LSA_SEC_HANDLE p1,
    PSecBuffer p2)
{
    fdTRACE("UsrSpInitUserModeContext(%p %p)\n",
          p1, p2);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
UsrSpMakeSignature(
    LSA_SEC_HANDLE p1,
    ULONG p2,
    PSecBufferDesc p3,
    ULONG p4)
{
    fdTRACE("UsrSpMakeSignature(%p 0x%x %p 0x%x)\n",
          p1, p2, p3, p4);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
UsrSpVerifySignature(
    LSA_SEC_HANDLE p1,
    PSecBufferDesc p2,
    ULONG p3,
    PULONG p4)
{
    fdTRACE("UsrSpVerifySignature(%p 0x%x %p 0x%x)\n",
          p1, p2, p3, p4);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
UsrSpSealMessage(
    LSA_SEC_HANDLE p1,
    ULONG p2,
    PSecBufferDesc p3,
    ULONG p4)
{
    fdTRACE("UsrSpSealMessage(%p 0x%x %p 0x%x)\n",
          p1, p2, p3, p4);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
UsrSpUnsealMessage(
    LSA_SEC_HANDLE p1,
    PSecBufferDesc p2,
    ULONG p3,
    PULONG p4)
{
    fdTRACE("UsrSpUnsealMessage(%p 0x%x %p 0x%x)\n",
          p1, p2, p3, p4);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
UsrSpGetContextToken(
    LSA_SEC_HANDLE p1,
    PHANDLE p2)
{
    fdTRACE("UsrSpGetContextToken(%p %p)\n",
          p1, p2);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
UsrSpQueryContextAttributes(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextAttribute,
    IN OUT PVOID Buffer)
{
    SECURITY_STATUS status;

    fdTRACE("UsrSpQueryContextAttributes(%p 0x%x %p)\n",
            ContextHandle, ContextAttribute, Buffer);

    status = NtlmQueryContextAttributesAW(ContextHandle, ContextAttribute, Buffer, TRUE);

    fdTRACE("Status %x\n", status);
    return status;
}

NTSTATUS NTAPI
UsrSpCompleteAuthToken(
    LSA_SEC_HANDLE p1,
    PSecBufferDesc p2)
{
    fdTRACE("UsrSpGetContextToken(%p %p)\n",
          p1, p2);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
UsrSpDeleteUserModeContext(
    LSA_SEC_HANDLE p1)
{
    fdTRACE("UsrSpDeleteUserModeContext(%p)\n",
          p1);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
UsrSpFormatCredentials(
    PSecBuffer p1,
    PSecBuffer p2)
{
    fdTRACE("UsrSpFormatCredentials(%p %p)\n",
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
    fdTRACE("UsrSpMarshallSupplementalCreds(0x%x %p %p %p)\n",
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
    fdTRACE("UsrSpExportSecurityContext(%p 0x%x %p 0x%x)\n",
          p1, p2, p3, p4);

    return ERROR_NOT_SUPPORTED;
}

NTSTATUS NTAPI
UsrSpImportSecurityContext(
    PSecBuffer p1,
    HANDLE p2,
    PLSA_SEC_HANDLE p3)
{
    fdTRACE("UsrSpImportSecurityContext(%p 0x%x %p)\n",
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
