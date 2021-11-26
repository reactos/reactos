/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for NtOpenThreadToken[Ex]
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

START_TEST(NtOpenThreadToken)
{
    NTSTATUS Status;
    HANDLE TokenHandle;
    BOOLEAN OpenAsSelf;
    ULONG HandleAttributes[] = { 0, OBJ_KERNEL_HANDLE };
    ULONG i;

    TokenHandle = (HANDLE)0x55555555;
    Status = NtOpenThreadToken(NtCurrentThread(),
                               TOKEN_READ,
                               TRUE,
                               &TokenHandle);
    ok(Status == STATUS_NO_TOKEN, "Status = %lx\n", Status);
    ok(TokenHandle == (HANDLE)0x55555555 /* 2003 */ ||
       TokenHandle == NULL /* Win7 */, "TokenHandle = %p\n", TokenHandle);

    Status = RtlImpersonateSelf(SecurityImpersonation);
    ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
    for (OpenAsSelf = FALSE; OpenAsSelf <= TRUE; OpenAsSelf++)
    {
        Status = NtOpenThreadToken(NtCurrentThread(),
                                   TOKEN_READ,
                                   OpenAsSelf,
                                   &TokenHandle);
        ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
        if (NT_SUCCESS(Status))
        {
            ok((LONG_PTR)TokenHandle > 0, "TokenHandle = %p\n", TokenHandle);
            Status = NtClose(TokenHandle);
            ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
        }

        for (i = 0; i < RTL_NUMBER_OF(HandleAttributes); i++)
        {
            Status = NtOpenThreadTokenEx(NtCurrentThread(),
                                         TOKEN_READ,
                                         OpenAsSelf,
                                         HandleAttributes[i],
                                         &TokenHandle);
            ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
            if (NT_SUCCESS(Status))
            {
                ok((LONG_PTR)TokenHandle > 0, "TokenHandle = %p\n", TokenHandle);
                Status = NtClose(TokenHandle);
                ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
            }
        }
    }

    TokenHandle = NULL;
    Status = NtSetInformationThread(NtCurrentThread(),
                                    ThreadImpersonationToken,
                                    &TokenHandle,
                                    sizeof(TokenHandle));
    ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
}
