/*
 * PROJECT:         ReactOS msgina.dll
 * FILE:            dll/win32/msgina/gui.c
 * PURPOSE:         ReactOS Logon GINA DLL
 * PROGRAMMER:      Eric Kohl
 */

#include "msgina.h"

BOOL
ConnectToLsa(
    PGINA_CONTEXT pgContext)
{
    LSA_STRING LogonProcessName;
    LSA_STRING PackageName;
    LSA_OPERATIONAL_MODE SecurityMode = 0;
    NTSTATUS Status;

    /* We are already connected to the LSA */
    if (pgContext->LsaHandle != NULL)
        return TRUE;

    /* Connect to the LSA server */
    RtlInitAnsiString((PANSI_STRING)&LogonProcessName,
                      "MSGINA");

    Status = LsaRegisterLogonProcess(&LogonProcessName,
                                     &pgContext->LsaHandle,
                                     &SecurityMode);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaRegisterLogonProcess failed (Status 0x%08lx)\n", Status);
        return FALSE;
    }

    /* Get the authentication package */
    RtlInitAnsiString((PANSI_STRING)&PackageName,
                      MSV1_0_PACKAGE_NAME);

    Status = LsaLookupAuthenticationPackage(pgContext->LsaHandle,
                                            &PackageName,
                                            &pgContext->AuthenticationPackage);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaLookupAuthenticationPackage failed (Status 0x%08lx)\n", Status);
        return FALSE;
    }

    return TRUE;
}

/* EOF */
