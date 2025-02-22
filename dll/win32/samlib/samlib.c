/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           SAM interface library
 * FILE:              lib/samlib/samlib.c
 * PROGRAMER:         Eric Kohl
 */

#include "precomp.h"

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include <ntsam.h>
#include <sam_c.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(samlib);

NTSTATUS
WINAPI
SystemFunction006(LPCSTR password,
                  LPSTR hash);

NTSTATUS
WINAPI
SystemFunction007(PUNICODE_STRING string,
                  LPBYTE hash);

NTSTATUS
WINAPI
SystemFunction012(const BYTE *in,
                  const BYTE *key,
                  LPBYTE out);

/* GLOBALS *******************************************************************/


/* FUNCTIONS *****************************************************************/

void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


handle_t __RPC_USER
PSAMPR_SERVER_NAME_bind(PSAMPR_SERVER_NAME pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("PSAMPR_SERVER_NAME_bind(%S)\n", pszSystemName);

    /* Check the server name prefix and server name length */
    if (pszSystemName != NULL)
    {
        int nLength = wcslen(pszSystemName);
        int nNameLength = nLength;

        if (nLength >= 1 && pszSystemName[0] == L'\\')
            nNameLength--;

        if (nLength >= 2 && pszSystemName[1] == L'\\')
            nNameLength--;

        if (((nLength - nNameLength != 0) &&
             (nLength - nNameLength != 2)) ||
            (nNameLength == 0))
        {
            WARN("Invalid server name %S\n", pszSystemName);
            RpcRaiseException(STATUS_OBJECT_NAME_INVALID);
        }
    }

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      pszSystemName,
                                      L"\\pipe\\samr",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        TRACE("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status)
    {
        TRACE("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
//        TRACE("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
PSAMPR_SERVER_NAME_unbind(PSAMPR_SERVER_NAME pszSystemName,
                          handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("PSAMPR_SERVER_NAME_unbind(%S)\n", pszSystemName);

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


NTSTATUS
SampCheckPassword(IN SAMPR_HANDLE UserHandle,
                  IN PUNICODE_STRING Password)
{
    USER_DOMAIN_PASSWORD_INFORMATION DomainPasswordInformation;
    LPWORD CharTypeBuffer = NULL;
    ULONG PasswordLength;
    ULONG i;
    ULONG Upper = 0, Lower = 0, Digit = 0, Punct = 0, Alpha = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("SampCheckPassword(%p %p)\n", UserHandle, Password);

    /* Get the domain password information */
    Status = SamrGetUserDomainPasswordInformation(UserHandle,
                                                  &DomainPasswordInformation);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrGetUserDomainPasswordInformation failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    PasswordLength = (ULONG)(Password->Length / sizeof(WCHAR));

    /* Fail if the password is too short or too long */
    if ((PasswordLength < DomainPasswordInformation.MinPasswordLength) ||
        (PasswordLength > 256))
        return STATUS_PASSWORD_RESTRICTION;

    /* Check the password complexity */
    if (DomainPasswordInformation.PasswordProperties & DOMAIN_PASSWORD_COMPLEX)
    {
        CharTypeBuffer = midl_user_allocate(PasswordLength * sizeof(WORD));
        if (CharTypeBuffer == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        GetStringTypeW(CT_CTYPE1,
                       Password->Buffer,
                       PasswordLength,
                       CharTypeBuffer);

        for (i = 0; i < PasswordLength; i++)
        {
            TRACE("%lu: %C %s %s %s %s\n", i, Password->Buffer[i],
                  (CharTypeBuffer[i] & C1_UPPER) ? "C1_UPPER" : "        ",
                  (CharTypeBuffer[i] & C1_LOWER) ? "C1_LOWER" : "        ",
                  (CharTypeBuffer[i] & C1_DIGIT) ? "C1_DIGIT" : "        ",
                  (CharTypeBuffer[i] & C1_PUNCT) ? "C1_PUNCT" : "        ",
                  (CharTypeBuffer[i] & C1_ALPHA) ? "C1_ALPHA" : "        ");

            if (CharTypeBuffer[i] & C1_UPPER)
                Upper = 1;

            if (CharTypeBuffer[i] & C1_LOWER)
                Lower = 1;

            if (CharTypeBuffer[i] & C1_DIGIT)
                Digit = 1;

            if (CharTypeBuffer[i] & C1_PUNCT)
                Punct = 1;

            if ((CharTypeBuffer[i] & C1_ALPHA) &&
                !(CharTypeBuffer[i] & C1_UPPER) &&
                !(CharTypeBuffer[i] & C1_LOWER))
                Alpha = 1;
        }

        TRACE("Upper: %lu\n", Upper);
        TRACE("Lower: %lu\n", Lower);
        TRACE("Digit: %lu\n", Digit);
        TRACE("Punct: %lu\n", Punct);
        TRACE("Alpha: %lu\n", Alpha);

        TRACE("Total: %lu\n", Upper + Lower + Digit + Punct + Alpha);
        if (Upper + Lower + Digit + Punct + Alpha < 3)
            Status = STATUS_PASSWORD_RESTRICTION;
    }

    if (CharTypeBuffer != NULL)
        midl_user_free(CharTypeBuffer);

    return Status;
}


NTSTATUS
NTAPI
SamAddMemberToAlias(IN SAM_HANDLE AliasHandle,
                    IN PSID MemberId)
{
    NTSTATUS Status;

    TRACE("SamAddMemberToAlias(%p %p)\n",
          AliasHandle, MemberId);

    RpcTryExcept
    {
        Status = SamrAddMemberToAlias((SAMPR_HANDLE)AliasHandle,
                                      (PRPC_SID)MemberId);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamAddMemberToGroup(IN SAM_HANDLE GroupHandle,
                    IN ULONG MemberId,
                    IN ULONG Attributes)
{
    NTSTATUS Status;

    TRACE("SamAddMemberToGroup(%p %lu %lx)\n",
          GroupHandle, MemberId, Attributes);

    RpcTryExcept
    {
        Status = SamrAddMemberToGroup((SAMPR_HANDLE)GroupHandle,
                                      MemberId,
                                      Attributes);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamAddMultipleMembersToAlias(IN SAM_HANDLE AliasHandle,
                             IN PSID *MemberIds,
                             IN ULONG MemberCount)
{
    SAMPR_PSID_ARRAY Buffer;
    NTSTATUS Status;

    TRACE("SamAddMultipleMembersToAlias(%p %p %lu)\n",
          AliasHandle, MemberIds, MemberCount);

    if (MemberIds == NULL)
        return STATUS_INVALID_PARAMETER_2;

    Buffer.Count = MemberCount;
    Buffer.Sids = (PSAMPR_SID_INFORMATION)MemberIds;

    RpcTryExcept
    {
        Status = SamrAddMultipleMembersToAlias((SAMPR_HANDLE)AliasHandle,
                                               &Buffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamChangePasswordUser(IN SAM_HANDLE UserHandle,
                      IN PUNICODE_STRING OldPassword,
                      IN PUNICODE_STRING NewPassword)
{
    NT_OWF_PASSWORD OldNtPassword;
    NT_OWF_PASSWORD NewNtPassword;
    LM_OWF_PASSWORD OldLmPassword;
    LM_OWF_PASSWORD NewLmPassword;
    OEM_STRING LmPwdString;
    CHAR LmPwdBuffer[15];
    BOOLEAN OldLmPasswordPresent = FALSE;
    BOOLEAN NewLmPasswordPresent = FALSE;
    NTSTATUS Status;

    ENCRYPTED_LM_OWF_PASSWORD OldLmEncryptedWithNewLm;
    ENCRYPTED_LM_OWF_PASSWORD NewLmEncryptedWithOldLm;
    ENCRYPTED_NT_OWF_PASSWORD OldNtEncryptedWithNewNt;
    ENCRYPTED_NT_OWF_PASSWORD NewNtEncryptedWithOldNt;
    PENCRYPTED_LM_OWF_PASSWORD pOldLmEncryptedWithNewLm = NULL;
    PENCRYPTED_LM_OWF_PASSWORD pNewLmEncryptedWithOldLm = NULL;

    /* Calculate the NT hash for the old password */
    Status = SystemFunction007(OldPassword,
                               (LPBYTE)&OldNtPassword);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SystemFunction007 failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Calculate the NT hash for the new password */
    Status = SystemFunction007(NewPassword,
                               (LPBYTE)&NewNtPassword);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SystemFunction007 failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Calculate the LM password and hash for the old password */
    LmPwdString.Length = 15;
    LmPwdString.MaximumLength = 15;
    LmPwdString.Buffer = LmPwdBuffer;
    ZeroMemory(LmPwdString.Buffer, LmPwdString.MaximumLength);

    Status = RtlUpcaseUnicodeStringToOemString(&LmPwdString,
                                               OldPassword,
                                               FALSE);
    if (NT_SUCCESS(Status))
    {
        /* Calculate the LM hash value of the password */
        Status = SystemFunction006(LmPwdString.Buffer,
                                   (LPSTR)&OldLmPassword);
        if (NT_SUCCESS(Status))
        {
            OldLmPasswordPresent = TRUE;
        }
    }

    /* Calculate the LM password and hash for the new password */
    LmPwdString.Length = 15;
    LmPwdString.MaximumLength = 15;
    LmPwdString.Buffer = LmPwdBuffer;
    ZeroMemory(LmPwdString.Buffer, LmPwdString.MaximumLength);

    Status = RtlUpcaseUnicodeStringToOemString(&LmPwdString,
                                               NewPassword,
                                               FALSE);
    if (NT_SUCCESS(Status))
    {
        /* Calculate the LM hash value of the password */
        Status = SystemFunction006(LmPwdString.Buffer,
                                   (LPSTR)&NewLmPassword);
        if (NT_SUCCESS(Status))
        {
            NewLmPasswordPresent = TRUE;
        }
    }

    if (OldLmPasswordPresent && NewLmPasswordPresent)
    {
        /* Encrypt the old LM hash with the new LM hash */
        Status = SystemFunction012((const BYTE *)&OldLmPassword,
                                   (const BYTE *)&NewLmPassword,
                                   (LPBYTE)&OldLmEncryptedWithNewLm);
        if (!NT_SUCCESS(Status))
        {
            TRACE("SystemFunction012 failed (Status 0x%08lx)\n", Status);
            return Status;
        }

        /* Encrypt the new LM hash with the old LM hash */
        Status = SystemFunction012((const BYTE *)&NewLmPassword,
                                   (const BYTE *)&OldLmPassword,
                                   (LPBYTE)&NewLmEncryptedWithOldLm);
        if (!NT_SUCCESS(Status))
        {
            TRACE("SystemFunction012 failed (Status 0x%08lx)\n", Status);
            return Status;
        }

        pOldLmEncryptedWithNewLm = &OldLmEncryptedWithNewLm;
        pNewLmEncryptedWithOldLm = &NewLmEncryptedWithOldLm;
    }

    /* Encrypt the old NT hash with the new NT hash */
    Status = SystemFunction012((const BYTE *)&OldNtPassword,
                               (const BYTE *)&NewNtPassword,
                               (LPBYTE)&OldNtEncryptedWithNewNt);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SystemFunction012 failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Encrypt the new NT hash with the old NT hash */
    Status = SystemFunction012((const BYTE *)&NewNtPassword,
                               (const BYTE *)&OldNtPassword,
                               (LPBYTE)&NewNtEncryptedWithOldNt);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SystemFunction012 failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    RpcTryExcept
    {
        Status = SamrChangePasswordUser((SAMPR_HANDLE)UserHandle,
                                        OldLmPasswordPresent && NewLmPasswordPresent,
                                        pOldLmEncryptedWithNewLm,
                                        pNewLmEncryptedWithOldLm,
                                        TRUE,
                                        &OldNtEncryptedWithNewNt,
                                        &NewNtEncryptedWithOldNt,
                                        FALSE,
                                        NULL,
                                        FALSE,
                                        NULL);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamChangePasswordUser2(IN PUNICODE_STRING ServerName,
                       IN PUNICODE_STRING UserName,
                       IN PUNICODE_STRING OldPassword,
                       IN PUNICODE_STRING NewPassword)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
SamChangePasswordUser3(IN PUNICODE_STRING ServerName,
                       IN PUNICODE_STRING UserName,
                       IN PUNICODE_STRING OldPassword,
                       IN PUNICODE_STRING NewPassword,
                       OUT PDOMAIN_PASSWORD_INFORMATION *EffectivePasswordPolicy,
                       OUT PUSER_PWD_CHANGE_FAILURE_INFORMATION *PasswordChangeFailureInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
SamCloseHandle(IN SAM_HANDLE SamHandle)
{
    NTSTATUS Status;

    TRACE("SamCloseHandle(%p)\n", SamHandle);

    RpcTryExcept
    {
        Status = SamrCloseHandle((SAMPR_HANDLE *)&SamHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamConnect(IN OUT PUNICODE_STRING ServerName OPTIONAL,
           OUT PSAM_HANDLE ServerHandle,
           IN ACCESS_MASK DesiredAccess,
           IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    PSAMPR_SERVER_NAME pServerName = NULL;
    NTSTATUS Status;

    TRACE("SamConnect(%p %p 0x%08x %p)\n",
          ServerName, ServerHandle, DesiredAccess, ObjectAttributes);

    if (ServerName != NULL && ServerName->Length > 0 && ServerName->Buffer != NULL)
    {
        /* Create a zero-terminated server name */
        pServerName = midl_user_allocate(ServerName->Length + sizeof(WCHAR));
        if (pServerName == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        CopyMemory(pServerName, ServerName->Buffer, ServerName->Length);
        pServerName[ServerName->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }

    RpcTryExcept
    {
        Status = SamrConnect(pServerName,
                             (SAMPR_HANDLE *)ServerHandle,
                             DesiredAccess);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (pServerName)
        midl_user_free(pServerName);

    return Status;
}


NTSTATUS
NTAPI
SamCreateAliasInDomain(IN SAM_HANDLE DomainHandle,
                       IN PUNICODE_STRING AccountName,
                       IN ACCESS_MASK DesiredAccess,
                       OUT PSAM_HANDLE AliasHandle,
                       OUT PULONG RelativeId)
{
    NTSTATUS Status;

    TRACE("SamCreateAliasInDomain(%p %p 0x%08x %p %p)\n",
          DomainHandle, AccountName, DesiredAccess, AliasHandle, RelativeId);

    *AliasHandle = NULL;
    *RelativeId = 0;

    RpcTryExcept
    {
        Status = SamrCreateAliasInDomain((SAMPR_HANDLE)DomainHandle,
                                         (PRPC_UNICODE_STRING)AccountName,
                                         DesiredAccess,
                                         (SAMPR_HANDLE *)AliasHandle,
                                         RelativeId);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamCreateGroupInDomain(IN SAM_HANDLE DomainHandle,
                       IN PUNICODE_STRING AccountName,
                       IN ACCESS_MASK DesiredAccess,
                       OUT PSAM_HANDLE GroupHandle,
                       OUT PULONG RelativeId)
{
    NTSTATUS Status;

    TRACE("SamCreateGroupInDomain(%p %p 0x%08x %p %p)\n",
          DomainHandle, AccountName, DesiredAccess, GroupHandle, RelativeId);

    *GroupHandle = NULL;
    *RelativeId = 0;

    RpcTryExcept
    {
        Status = SamrCreateGroupInDomain((SAMPR_HANDLE)DomainHandle,
                                         (PRPC_UNICODE_STRING)AccountName,
                                         DesiredAccess,
                                         (SAMPR_HANDLE *)GroupHandle,
                                         RelativeId);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamCreateUser2InDomain(IN SAM_HANDLE DomainHandle,
                       IN PUNICODE_STRING AccountName,
                       IN ULONG AccountType,
                       IN ACCESS_MASK DesiredAccess,
                       OUT PSAM_HANDLE UserHandle,
                       OUT PULONG GrantedAccess,
                       OUT PULONG RelativeId)
{
    NTSTATUS Status;

    TRACE("SamCreateUser2InDomain(%p %p %lu 0x%08x %p %p %p)\n",
          DomainHandle, AccountName, AccountType, DesiredAccess,
          UserHandle, GrantedAccess, RelativeId);

    *UserHandle = NULL;
    *RelativeId = 0;

    RpcTryExcept
    {
        Status = SamrCreateUser2InDomain((SAMPR_HANDLE)DomainHandle,
                                         (PRPC_UNICODE_STRING)AccountName,
                                         AccountType,
                                         DesiredAccess,
                                         (SAMPR_HANDLE *)UserHandle,
                                         GrantedAccess,
                                         RelativeId);

    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamCreateUserInDomain(IN SAM_HANDLE DomainHandle,
                      IN PUNICODE_STRING AccountName,
                      IN ACCESS_MASK DesiredAccess,
                      OUT PSAM_HANDLE UserHandle,
                      OUT PULONG RelativeId)
{
    NTSTATUS Status;

    TRACE("SamCreateUserInDomain(%p %p 0x%08x %p %p)\n",
          DomainHandle, AccountName, DesiredAccess, UserHandle, RelativeId);

    *UserHandle = NULL;
    *RelativeId = 0;

    RpcTryExcept
    {
        Status = SamrCreateUserInDomain((SAMPR_HANDLE)DomainHandle,
                                        (PRPC_UNICODE_STRING)AccountName,
                                        DesiredAccess,
                                        (SAMPR_HANDLE *)UserHandle,
                                        RelativeId);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamDeleteAlias(IN SAM_HANDLE AliasHandle)
{
    SAMPR_HANDLE LocalAliasHandle;
    NTSTATUS Status;

    TRACE("SamDeleteAlias(%p)\n", AliasHandle);

    LocalAliasHandle = (SAMPR_HANDLE)AliasHandle;

    if (LocalAliasHandle == NULL)
        return STATUS_INVALID_HANDLE;

    RpcTryExcept
    {
        Status = SamrDeleteAlias(&LocalAliasHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamDeleteGroup(IN SAM_HANDLE GroupHandle)
{
    SAMPR_HANDLE LocalGroupHandle;
    NTSTATUS Status;

    TRACE("SamDeleteGroup(%p)\n", GroupHandle);

    LocalGroupHandle = (SAMPR_HANDLE)GroupHandle;

    if (LocalGroupHandle == NULL)
        return STATUS_INVALID_HANDLE;

    RpcTryExcept
    {
        Status = SamrDeleteGroup(&LocalGroupHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamDeleteUser(IN SAM_HANDLE UserHandle)
{
    SAMPR_HANDLE LocalUserHandle;
    NTSTATUS Status;

    TRACE("SamDeleteUser(%p)\n", UserHandle);

    LocalUserHandle = (SAMPR_HANDLE)UserHandle;

    if (LocalUserHandle == NULL)
        return STATUS_INVALID_HANDLE;

    RpcTryExcept
    {
        Status = SamrDeleteUser(&LocalUserHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamEnumerateAliasesInDomain(IN SAM_HANDLE DomainHandle,
                            IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
                            OUT PVOID *Buffer,
                            IN ULONG PreferedMaximumLength,
                            OUT PULONG CountReturned)
{
    PSAMPR_ENUMERATION_BUFFER EnumBuffer = NULL;
    NTSTATUS Status;

    TRACE("SamEnumerateAliasesInDomain(%p %p %p %lu %p)\n",
          DomainHandle, EnumerationContext, Buffer, PreferedMaximumLength,
          CountReturned);

    if ((EnumerationContext == NULL) ||
        (Buffer == NULL) ||
        (CountReturned == NULL))
        return STATUS_INVALID_PARAMETER;

    *Buffer = NULL;

    RpcTryExcept
    {
        Status = SamrEnumerateAliasesInDomain((SAMPR_HANDLE)DomainHandle,
                                              EnumerationContext,
                                              &EnumBuffer,
                                              PreferedMaximumLength,
                                              CountReturned);

        if (EnumBuffer != NULL)
        {
            if (EnumBuffer->Buffer != NULL)
            {
                *Buffer = EnumBuffer->Buffer;
            }

            midl_user_free(EnumBuffer);
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamEnumerateDomainsInSamServer(IN SAM_HANDLE ServerHandle,
                               IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
                               OUT PVOID *Buffer,
                               IN ULONG PreferedMaximumLength,
                               OUT PULONG CountReturned)
{
    PSAMPR_ENUMERATION_BUFFER EnumBuffer = NULL;
    NTSTATUS Status;

    TRACE("SamEnumerateDomainsInSamServer(%p %p %p %lu %p)\n",
          ServerHandle, EnumerationContext, Buffer, PreferedMaximumLength,
          CountReturned);

    if ((EnumerationContext == NULL) ||
        (Buffer == NULL) ||
        (CountReturned == NULL))
        return STATUS_INVALID_PARAMETER;

    *Buffer = NULL;

    RpcTryExcept
    {
        Status = SamrEnumerateDomainsInSamServer((SAMPR_HANDLE)ServerHandle,
                                                 EnumerationContext,
                                                 &EnumBuffer,
                                                 PreferedMaximumLength,
                                                 CountReturned);

        if (EnumBuffer != NULL)
        {
            if (EnumBuffer->Buffer != NULL)
            {
                *Buffer = EnumBuffer->Buffer;
            }

            midl_user_free(EnumBuffer);
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamEnumerateGroupsInDomain(IN SAM_HANDLE DomainHandle,
                           IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
                           IN PVOID *Buffer,
                           IN ULONG PreferedMaximumLength,
                           OUT PULONG CountReturned)
{
    PSAMPR_ENUMERATION_BUFFER EnumBuffer = NULL;
    NTSTATUS Status;

    TRACE("SamEnumerateGroupsInDomain(%p %p %p %lu %p)\n",
          DomainHandle, EnumerationContext, Buffer,
          PreferedMaximumLength, CountReturned);

    if (EnumerationContext == NULL || Buffer == NULL || CountReturned == NULL)
        return STATUS_INVALID_PARAMETER;

    *Buffer = NULL;

    RpcTryExcept
    {
        Status = SamrEnumerateGroupsInDomain((SAMPR_HANDLE)DomainHandle,
                                             EnumerationContext,
                                             &EnumBuffer,
                                             PreferedMaximumLength,
                                             CountReturned);
        if (EnumBuffer != NULL)
        {
            if (EnumBuffer->Buffer != NULL)
                *Buffer = EnumBuffer->Buffer;

            midl_user_free(EnumBuffer);
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamEnumerateUsersInDomain(IN SAM_HANDLE DomainHandle,
                          IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
                          IN ULONG UserAccountControl,
                          OUT PVOID *Buffer,
                          IN ULONG PreferedMaximumLength,
                          OUT PULONG CountReturned)
{
    PSAMPR_ENUMERATION_BUFFER EnumBuffer = NULL;
    NTSTATUS Status;

    TRACE("SamEnumerateUsersInDomain(%p %p %lx %p %lu %p)\n",
          DomainHandle, EnumerationContext, UserAccountControl, Buffer,
          PreferedMaximumLength, CountReturned);

    if (EnumerationContext == NULL || Buffer == NULL || CountReturned == NULL)
        return STATUS_INVALID_PARAMETER;

    *Buffer = NULL;

    RpcTryExcept
    {
        Status = SamrEnumerateUsersInDomain((SAMPR_HANDLE)DomainHandle,
                                            EnumerationContext,
                                            UserAccountControl,
                                            &EnumBuffer,
                                            PreferedMaximumLength,
                                            CountReturned);
        if (EnumBuffer != NULL)
        {
            if (EnumBuffer->Buffer != NULL)
            {
                *Buffer = EnumBuffer->Buffer;
            }

            midl_user_free(EnumBuffer);
        }

    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamFreeMemory(IN PVOID Buffer)
{
    if (Buffer != NULL)
        midl_user_free(Buffer);

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
SamGetAliasMembership(IN SAM_HANDLE DomainHandle,
                      IN ULONG PassedCount,
                      IN PSID *Sids,
                      OUT PULONG MembershipCount,
                      OUT PULONG *Aliases)
{
    SAMPR_PSID_ARRAY SidArray;
    SAMPR_ULONG_ARRAY Membership;
    NTSTATUS Status;

    TRACE("SamAliasMembership(%p %lu %p %p %p)\n",
          DomainHandle, PassedCount, Sids, MembershipCount, Aliases);

    if (Sids == NULL ||
        MembershipCount == NULL ||
        Aliases == NULL)
        return STATUS_INVALID_PARAMETER;

    Membership.Element = NULL;

    RpcTryExcept
    {
        SidArray.Count = PassedCount;
        SidArray.Sids = (PSAMPR_SID_INFORMATION)Sids;

        Status = SamrGetAliasMembership((SAMPR_HANDLE)DomainHandle,
                                        &SidArray,
                                        &Membership);
        if (NT_SUCCESS(Status))
        {
            *MembershipCount = Membership.Count;
            *Aliases = Membership.Element;
        }
        else
        {
            if (Membership.Element != NULL)
                midl_user_free(Membership.Element);
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamGetCompatibilityMode(IN SAM_HANDLE ObjectHandle,
                        OUT PULONG Mode)
{
    TRACE("SamGetCompatibilityMode(%p %p)\n", ObjectHandle, Mode);

    if (Mode == NULL)
        return STATUS_INVALID_PARAMETER;

    *Mode = SAM_SID_COMPATIBILITY_ALL;

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
SamGetDisplayEnumerationIndex(IN SAM_HANDLE DomainHandle,
                              IN DOMAIN_DISPLAY_INFORMATION DisplayInformation,
                              IN PUNICODE_STRING Prefix,
                              OUT PULONG Index)
{
    NTSTATUS Status;

    TRACE("SamGetDisplayEnumerationIndex(%p %lu %wZ %p)\n",
           DomainHandle, DisplayInformation, Prefix, Index);

    if ((Prefix == NULL) ||
        (Index == NULL))
        return STATUS_INVALID_PARAMETER;

    RpcTryExcept
    {
        Status = SamrGetDisplayEnumerationIndex2((SAMPR_HANDLE)DomainHandle,
                                                 DisplayInformation,
                                                 (PRPC_UNICODE_STRING)Prefix,
                                                 Index);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamGetGroupsForUser(IN SAM_HANDLE UserHandle,
                    OUT PGROUP_MEMBERSHIP *Groups,
                    OUT PULONG MembershipCount)
{
    PSAMPR_GET_GROUPS_BUFFER GroupsBuffer = NULL;
    NTSTATUS Status;

    TRACE("SamGetGroupsForUser(%p %p %p)\n",
          UserHandle, Groups, MembershipCount);

    RpcTryExcept
    {
        Status = SamrGetGroupsForUser((SAMPR_HANDLE)UserHandle,
                                      &GroupsBuffer);
        if (NT_SUCCESS(Status))
        {
            *Groups = GroupsBuffer->Groups;
            *MembershipCount = GroupsBuffer->MembershipCount;

            MIDL_user_free(GroupsBuffer);
        }
        else
        {
            if (GroupsBuffer != NULL)
            {
                if (GroupsBuffer->Groups != NULL)
                    MIDL_user_free(GroupsBuffer->Groups);

                MIDL_user_free(GroupsBuffer);
            }
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamGetMembersInAlias(IN SAM_HANDLE AliasHandle,
                     OUT PSID **MemberIds,
                     OUT PULONG MemberCount)
{
    SAMPR_PSID_ARRAY_OUT SidArray;
    NTSTATUS Status;

    TRACE("SamGetMembersInAlias(%p %p %p)\n",
          AliasHandle, MemberIds, MemberCount);

    if ((MemberIds == NULL) ||
        (MemberCount == NULL))
        return STATUS_INVALID_PARAMETER;

    *MemberIds = NULL;
    *MemberCount = 0;

    SidArray.Sids = NULL;

    RpcTryExcept
    {
        Status = SamrGetMembersInAlias((SAMPR_HANDLE)AliasHandle,
                                       &SidArray);
        if (NT_SUCCESS(Status))
        {
            *MemberCount = SidArray.Count;
            *MemberIds = (PSID *)SidArray.Sids;
        }

    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamGetMembersInGroup(IN SAM_HANDLE GroupHandle,
                     OUT PULONG *MemberIds,
                     OUT PULONG *Attributes,
                     OUT PULONG MemberCount)
{
    PSAMPR_GET_MEMBERS_BUFFER MembersBuffer = NULL;
    NTSTATUS Status;

    TRACE("SamGetMembersInGroup(%p %p %p %p)\n",
          GroupHandle, MemberIds, Attributes, MemberCount);

    RpcTryExcept
    {
        Status = SamrGetMembersInGroup((SAMPR_HANDLE)GroupHandle,
                                       &MembersBuffer);
        if (NT_SUCCESS(Status))
        {
            *MemberIds = MembersBuffer->Members;
            *Attributes = MembersBuffer->Attributes;
            *MemberCount = MembersBuffer->MemberCount;

            MIDL_user_free(MembersBuffer);
        }
        else
        {
            if (MembersBuffer != NULL)
            {
                if (MembersBuffer->Members != NULL)
                    MIDL_user_free(MembersBuffer->Members);

                if (MembersBuffer->Attributes != NULL)
                    MIDL_user_free(MembersBuffer->Attributes);

                MIDL_user_free(MembersBuffer);
            }
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamLookupDomainInSamServer(IN SAM_HANDLE ServerHandle,
                           IN PUNICODE_STRING Name,
                           OUT PSID *DomainId)
{
    NTSTATUS Status;

    TRACE("SamLookupDomainInSamServer(%p %p %p)\n",
          ServerHandle, Name, DomainId);

    RpcTryExcept
    {
        Status = SamrLookupDomainInSamServer((SAMPR_HANDLE)ServerHandle,
                                             (PRPC_UNICODE_STRING)Name,
                                             (PRPC_SID *)DomainId);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamLookupIdsInDomain(IN SAM_HANDLE DomainHandle,
                     IN ULONG Count,
                     IN PULONG RelativeIds,
                     OUT PUNICODE_STRING *Names,
                     OUT PSID_NAME_USE *Use OPTIONAL)
{
    SAMPR_RETURNED_USTRING_ARRAY NamesBuffer = {0, NULL};
    SAMPR_ULONG_ARRAY UseBuffer = {0, NULL};
    ULONG i;
    NTSTATUS Status;

    TRACE("SamLookupIdsInDomain(%p %lu %p %p %p)\n",
          DomainHandle, Count, RelativeIds, Names, Use);

    *Names = NULL;

    if (Use != NULL)
        *Use = NULL;

    RpcTryExcept
    {
        Status = SamrLookupIdsInDomain((SAMPR_HANDLE)DomainHandle,
                                       Count,
                                       RelativeIds,
                                       &NamesBuffer,
                                       &UseBuffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (NT_SUCCESS(Status))
    {
        *Names = midl_user_allocate(Count * sizeof(RPC_UNICODE_STRING));
        if (*Names == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        for (i = 0; i < Count; i++)
        {
            (*Names)[i].Buffer = midl_user_allocate(NamesBuffer.Element[i].MaximumLength);
            if ((*Names)[i].Buffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }
        }

        for (i = 0; i < Count; i++)
        {
            (*Names)[i].Length = NamesBuffer.Element[i].Length;
            (*Names)[i].MaximumLength = NamesBuffer.Element[i].MaximumLength;

            RtlCopyMemory((*Names)[i].Buffer,
                          NamesBuffer.Element[i].Buffer,
                          NamesBuffer.Element[i].Length);
        }

        if (Use != NULL)
        {
            *Use = midl_user_allocate(Count * sizeof(SID_NAME_USE));
            if (*Use == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            RtlCopyMemory(*Use,
                          UseBuffer.Element,
                          Count * sizeof(SID_NAME_USE));
        }
    }

done:
    if (!NT_SUCCESS(Status))
    {
        if (*Names != NULL)
        {
            for (i = 0; i < Count; i++)
            {
                if ((*Names)[i].Buffer != NULL)
                    midl_user_free((*Names)[i].Buffer);
            }

            midl_user_free(*Names);
        }

        if (Use != NULL && *Use != NULL)
            midl_user_free(*Use);
    }

    if (NamesBuffer.Element != NULL)
    {
        for (i = 0; i < NamesBuffer.Count; i++)
        {
            if (NamesBuffer.Element[i].Buffer != NULL)
                midl_user_free(NamesBuffer.Element[i].Buffer);
        }

        midl_user_free(NamesBuffer.Element);
    }

    if (UseBuffer.Element != NULL)
        midl_user_free(UseBuffer.Element);

    return 0;
}


NTSTATUS
NTAPI
SamLookupNamesInDomain(IN SAM_HANDLE DomainHandle,
                       IN ULONG Count,
                       IN PUNICODE_STRING Names,
                       OUT PULONG *RelativeIds,
                       OUT PSID_NAME_USE *Use)
{
    SAMPR_ULONG_ARRAY RidBuffer = {0, NULL};
    SAMPR_ULONG_ARRAY UseBuffer = {0, NULL};
    NTSTATUS Status;

    TRACE("SamLookupNamesInDomain(%p %lu %p %p %p)\n",
          DomainHandle, Count, Names, RelativeIds, Use);

    *RelativeIds = NULL;
    *Use = NULL;

    RpcTryExcept
    {
        Status = SamrLookupNamesInDomain((SAMPR_HANDLE)DomainHandle,
                                         Count,
                                         (PRPC_UNICODE_STRING)Names,
                                         &RidBuffer,
                                         &UseBuffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    if (NT_SUCCESS(Status))
    {
        *RelativeIds = midl_user_allocate(Count * sizeof(ULONG));
        if (*RelativeIds == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        *Use = midl_user_allocate(Count * sizeof(SID_NAME_USE));
        if (*Use == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        RtlCopyMemory(*RelativeIds,
                      RidBuffer.Element,
                      Count * sizeof(ULONG));

        RtlCopyMemory(*Use,
                      UseBuffer.Element,
                      Count * sizeof(SID_NAME_USE));
    }

done:
    if (!NT_SUCCESS(Status))
    {
        if (*RelativeIds != NULL)
            midl_user_free(*RelativeIds);

        if (*Use != NULL)
            midl_user_free(*Use);
    }

    if (RidBuffer.Element != NULL)
        midl_user_free(RidBuffer.Element);

    if (UseBuffer.Element != NULL)
        midl_user_free(UseBuffer.Element);

    return Status;
}


NTSTATUS
NTAPI
SamOpenAlias(IN SAM_HANDLE DomainHandle,
             IN ACCESS_MASK DesiredAccess,
             IN ULONG AliasId,
             OUT PSAM_HANDLE AliasHandle)
{
    NTSTATUS Status;

    TRACE("SamOpenAlias(%p 0x%08x %lx %p)\n",
          DomainHandle, DesiredAccess, AliasId, AliasHandle);

    RpcTryExcept
    {
        Status = SamrOpenAlias((SAMPR_HANDLE)DomainHandle,
                               DesiredAccess,
                               AliasId,
                               (SAMPR_HANDLE *)AliasHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamOpenDomain(IN SAM_HANDLE ServerHandle,
              IN ACCESS_MASK DesiredAccess,
              IN PSID DomainId,
              OUT PSAM_HANDLE DomainHandle)
{
    NTSTATUS Status;

    TRACE("SamOpenDomain(%p 0x%08x %p %p)\n",
          ServerHandle, DesiredAccess, DomainId, DomainHandle);

    RpcTryExcept
    {
        Status = SamrOpenDomain((SAMPR_HANDLE)ServerHandle,
                                DesiredAccess,
                                (PRPC_SID)DomainId,
                                (SAMPR_HANDLE *)DomainHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamOpenGroup(IN SAM_HANDLE DomainHandle,
             IN ACCESS_MASK DesiredAccess,
             IN ULONG GroupId,
             OUT PSAM_HANDLE GroupHandle)
{
    NTSTATUS Status;

    TRACE("SamOpenGroup(%p 0x%08x %p %p)\n",
          DomainHandle, DesiredAccess, GroupId, GroupHandle);

    RpcTryExcept
    {
        Status = SamrOpenGroup((SAMPR_HANDLE)DomainHandle,
                               DesiredAccess,
                               GroupId,
                               (SAMPR_HANDLE *)GroupHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamOpenUser(IN SAM_HANDLE DomainHandle,
            IN ACCESS_MASK DesiredAccess,
            IN ULONG UserId,
            OUT PSAM_HANDLE UserHandle)
{
    NTSTATUS Status;

    TRACE("SamOpenUser(%p 0x%08x %lx %p)\n",
          DomainHandle, DesiredAccess, UserId, UserHandle);

    RpcTryExcept
    {
        Status = SamrOpenUser((SAMPR_HANDLE)DomainHandle,
                              DesiredAccess,
                              UserId,
                              (SAMPR_HANDLE *)UserHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamQueryDisplayInformation(IN SAM_HANDLE DomainHandle,
                           IN DOMAIN_DISPLAY_INFORMATION DisplayInformation,
                           IN ULONG Index,
                           IN ULONG EntryCount,
                           IN ULONG PreferredMaximumLength,
                           OUT PULONG TotalAvailable,
                           OUT PULONG TotalReturned,
                           OUT PULONG ReturnedEntryCount,
                           OUT PVOID *SortedBuffer)
{
    SAMPR_DISPLAY_INFO_BUFFER LocalBuffer;
    NTSTATUS Status;

    TRACE("SamQueryDisplayInformation(%p %lu %lu %lu %lu %p %p %p %p)\n",
          DomainHandle, DisplayInformation, Index, EntryCount,
          PreferredMaximumLength, TotalAvailable, TotalReturned,
          ReturnedEntryCount, SortedBuffer);

    if ((TotalAvailable == NULL) ||
        (TotalReturned == NULL) ||
        (ReturnedEntryCount == NULL) ||
        (SortedBuffer == NULL))
        return STATUS_INVALID_PARAMETER;

    RpcTryExcept
    {
        Status = SamrQueryDisplayInformation3((SAMPR_HANDLE)DomainHandle,
                                              DisplayInformation,
                                              Index,
                                              EntryCount,
                                              PreferredMaximumLength,
                                              TotalAvailable,
                                              TotalReturned,
                                              &LocalBuffer);
        if (NT_SUCCESS(Status))
        {
            switch (DisplayInformation)
            {
                case DomainDisplayUser:
                    *ReturnedEntryCount = LocalBuffer.UserInformation.EntriesRead;
                    *SortedBuffer = LocalBuffer.UserInformation.Buffer;
                    break;

                case DomainDisplayMachine:
                    *ReturnedEntryCount = LocalBuffer.MachineInformation.EntriesRead;
                    *SortedBuffer = LocalBuffer.MachineInformation.Buffer;
                    break;

                case DomainDisplayGroup:
                    *ReturnedEntryCount = LocalBuffer.GroupInformation.EntriesRead;
                    *SortedBuffer = LocalBuffer.GroupInformation.Buffer;
                    break;

                case DomainDisplayOemUser:
                    *ReturnedEntryCount = LocalBuffer.OemUserInformation.EntriesRead;
                    *SortedBuffer = LocalBuffer.OemUserInformation.Buffer;
                    break;

                case DomainDisplayOemGroup:
                    *ReturnedEntryCount = LocalBuffer.OemGroupInformation.EntriesRead;
                    *SortedBuffer = LocalBuffer.OemGroupInformation.Buffer;
                    break;

                case DomainDisplayServer:
                    /* FIXME */
                    break;
            }
        }
        else
        {
            *ReturnedEntryCount = 0;
            *SortedBuffer = NULL;
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamQueryInformationAlias(IN SAM_HANDLE AliasHandle,
                         IN ALIAS_INFORMATION_CLASS AliasInformationClass,
                         OUT PVOID *Buffer)
{
    NTSTATUS Status;

    TRACE("SamQueryInformationAlias(%p %lu %p)\n",
          AliasHandle, AliasInformationClass, Buffer);

    RpcTryExcept
    {
        Status = SamrQueryInformationAlias((SAMPR_HANDLE)AliasHandle,
                                           AliasInformationClass,
                                           (PSAMPR_ALIAS_INFO_BUFFER *)Buffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamQueryInformationDomain(IN SAM_HANDLE DomainHandle,
                          IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
                          OUT PVOID *Buffer)
{
    NTSTATUS Status;

    TRACE("SamQueryInformationDomain(%p %lu %p)\n",
          DomainHandle, DomainInformationClass, Buffer);

    RpcTryExcept
    {
        Status = SamrQueryInformationDomain((SAMPR_HANDLE)DomainHandle,
                                            DomainInformationClass,
                                            (PSAMPR_DOMAIN_INFO_BUFFER *)Buffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamQueryInformationGroup(IN SAM_HANDLE GroupHandle,
                         IN GROUP_INFORMATION_CLASS GroupInformationClass,
                         OUT PVOID *Buffer)
{
    NTSTATUS Status;

    TRACE("SamQueryInformationGroup(%p %lu %p)\n",
          GroupHandle, GroupInformationClass, Buffer);

    RpcTryExcept
    {
        Status = SamrQueryInformationGroup((SAMPR_HANDLE)GroupHandle,
                                           GroupInformationClass,
                                           (PSAMPR_GROUP_INFO_BUFFER *)Buffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamQueryInformationUser(IN SAM_HANDLE UserHandle,
                        IN USER_INFORMATION_CLASS UserInformationClass,
                        OUT PVOID *Buffer)
{
    NTSTATUS Status;

    TRACE("SamQueryInformationUser(%p %lu %p)\n",
          UserHandle, UserInformationClass, Buffer);

    RpcTryExcept
    {
        Status = SamrQueryInformationUser((SAMPR_HANDLE)UserHandle,
                                          UserInformationClass,
                                          (PSAMPR_USER_INFO_BUFFER *)Buffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamQuerySecurityObject(IN SAM_HANDLE ObjectHandle,
                       IN SECURITY_INFORMATION SecurityInformation,
                       OUT PSECURITY_DESCRIPTOR *SecurityDescriptor)
{
    PSAMPR_SR_SECURITY_DESCRIPTOR SamSecurityDescriptor = NULL;
    NTSTATUS Status;

    TRACE("SamQuerySecurityObject(%p %lu %p)\n",
          ObjectHandle, SecurityInformation, SecurityDescriptor);

    *SecurityDescriptor = NULL;

    RpcTryExcept
    {
        Status = SamrQuerySecurityObject((SAMPR_HANDLE)ObjectHandle,
                                         SecurityInformation,
                                         &SamSecurityDescriptor);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    TRACE("SamSecurityDescriptor: %p\n", SamSecurityDescriptor);

    if (SamSecurityDescriptor != NULL)
    {
        TRACE("SamSecurityDescriptor->Length: %lu\n", SamSecurityDescriptor->Length);
        TRACE("SamSecurityDescriptor->SecurityDescriptor: %p\n", SamSecurityDescriptor->SecurityDescriptor);

        *SecurityDescriptor = SamSecurityDescriptor->SecurityDescriptor;

        midl_user_free(SamSecurityDescriptor);
    }

    return Status;
}


NTSTATUS
NTAPI
SamRemoveMemberFromAlias(IN SAM_HANDLE AliasHandle,
                         IN PSID MemberId)
{
    NTSTATUS Status;

    TRACE("SamRemoveMemberFromAlias(%p %ul)\n",
          AliasHandle, MemberId);

    RpcTryExcept
    {
        Status = SamrRemoveMemberFromAlias((SAMPR_HANDLE)AliasHandle,
                                           MemberId);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamRemoveMemberFromForeignDomain(IN SAM_HANDLE DomainHandle,
                                 IN PSID MemberId)
{
    NTSTATUS Status;

    TRACE("SamRemoveMemberFromForeignDomain(%p %ul)\n",
          DomainHandle, MemberId);

    RpcTryExcept
    {
        Status = SamrRemoveMemberFromForeignDomain((SAMPR_HANDLE)DomainHandle,
                                                   MemberId);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamRemoveMemberFromGroup(IN SAM_HANDLE GroupHandle,
                         IN ULONG MemberId)
{
    NTSTATUS Status;

    TRACE("SamRemoveMemberFromGroup(%p %ul)\n",
          GroupHandle, MemberId);

    RpcTryExcept
    {
        Status = SamrRemoveMemberFromGroup((SAMPR_HANDLE)GroupHandle,
                                           MemberId);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamRemoveMultipleMembersFromAlias(IN SAM_HANDLE AliasHandle,
                                  IN PSID *MemberIds,
                                  IN ULONG MemberCount)
{
    SAMPR_PSID_ARRAY Buffer;
    NTSTATUS Status;

    TRACE("SamRemoveMultipleMembersFromAlias(%p %p %lu)\n",
          AliasHandle, MemberIds, MemberCount);

    if (MemberIds == NULL)
        return STATUS_INVALID_PARAMETER_2;

    Buffer.Count = MemberCount;
    Buffer.Sids = (PSAMPR_SID_INFORMATION)MemberIds;

    RpcTryExcept
    {
        Status = SamrRemoveMultipleMembersFromAlias((SAMPR_HANDLE)AliasHandle,
                                                    &Buffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamRidToSid(IN SAM_HANDLE ObjectHandle,
            IN ULONG Rid,
            OUT PSID *Sid)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
SamSetInformationAlias(IN SAM_HANDLE AliasHandle,
                       IN ALIAS_INFORMATION_CLASS AliasInformationClass,
                       IN PVOID Buffer)
{
    NTSTATUS Status;

    TRACE("SamSetInformationAlias(%p %lu %p)\n",
          AliasHandle, AliasInformationClass, Buffer);

    RpcTryExcept
    {
        Status = SamrSetInformationAlias((SAMPR_HANDLE)AliasHandle,
                                         AliasInformationClass,
                                         Buffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamSetInformationDomain(IN SAM_HANDLE DomainHandle,
                        IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
                        IN PVOID Buffer)
{
    NTSTATUS Status;

    TRACE("SamSetInformationDomain(%p %lu %p)\n",
          DomainHandle, DomainInformationClass, Buffer);

    RpcTryExcept
    {
        Status = SamrSetInformationDomain((SAMPR_HANDLE)DomainHandle,
                                          DomainInformationClass,
                                          Buffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamSetInformationGroup(IN SAM_HANDLE GroupHandle,
                       IN GROUP_INFORMATION_CLASS GroupInformationClass,
                       IN PVOID Buffer)
{
    NTSTATUS Status;

    TRACE("SamSetInformationGroup(%p %lu %p)\n",
          GroupHandle, GroupInformationClass, Buffer);

    RpcTryExcept
    {
        Status = SamrSetInformationGroup((SAMPR_HANDLE)GroupHandle,
                                         GroupInformationClass,
                                         Buffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamSetInformationUser(IN SAM_HANDLE UserHandle,
                      IN USER_INFORMATION_CLASS UserInformationClass,
                      IN PVOID Buffer)
{
    PSAMPR_USER_SET_PASSWORD_INFORMATION PasswordBuffer;
    SAMPR_USER_INTERNAL1_INFORMATION Internal1Buffer;
    USER_ALL_INFORMATION InternalAllBuffer;
    OEM_STRING LmPwdString;
    CHAR LmPwdBuffer[15];
    NTSTATUS Status;

    TRACE("SamSetInformationUser(%p %lu %p)\n",
          UserHandle, UserInformationClass, Buffer);

    if (UserInformationClass == UserSetPasswordInformation)
    {
        PasswordBuffer = (PSAMPR_USER_SET_PASSWORD_INFORMATION)Buffer;

        Status = SampCheckPassword(UserHandle,
                                   (PUNICODE_STRING)&PasswordBuffer->Password);
        if (!NT_SUCCESS(Status))
        {
            TRACE("SampCheckPassword failed (Status 0x%08lx)\n", Status);
            return Status;
        }

        /* Calculate the NT hash value of the password */
        Status = SystemFunction007((PUNICODE_STRING)&PasswordBuffer->Password,
                                   (LPBYTE)&Internal1Buffer.EncryptedNtOwfPassword);
        if (!NT_SUCCESS(Status))
        {
            TRACE("SystemFunction007 failed (Status 0x%08lx)\n", Status);
            return Status;
        }

        Internal1Buffer.NtPasswordPresent = TRUE;
        Internal1Buffer.LmPasswordPresent = FALSE;

        /* Build the LM password */
        LmPwdString.Length = 15;
        LmPwdString.MaximumLength = 15;
        LmPwdString.Buffer = LmPwdBuffer;
        ZeroMemory(LmPwdString.Buffer, LmPwdString.MaximumLength);

        Status = RtlUpcaseUnicodeStringToOemString(&LmPwdString,
                                                   (PUNICODE_STRING)&PasswordBuffer->Password,
                                                   FALSE);
        if (NT_SUCCESS(Status))
        {
            /* Calculate the LM hash value of the password */
            Status = SystemFunction006(LmPwdString.Buffer,
                                       (LPSTR)&Internal1Buffer.EncryptedLmOwfPassword);
            if (NT_SUCCESS(Status))
                Internal1Buffer.LmPasswordPresent = TRUE;
        }

        Internal1Buffer.PasswordExpired = PasswordBuffer->PasswordExpired;

        RpcTryExcept
        {
            Status = SamrSetInformationUser((SAMPR_HANDLE)UserHandle,
                                            UserInternal1Information,
                                            (PVOID)&Internal1Buffer);
        }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = I_RpcMapWin32Status(RpcExceptionCode());
        }
        RpcEndExcept;

        if (!NT_SUCCESS(Status))
        {
            TRACE("SamrSetInformation() failed (Status 0x%08lx)\n", Status);
        }

        return Status;
    }
    else if (UserInformationClass == UserAllInformation)
    {
        RtlCopyMemory(&InternalAllBuffer,
                      Buffer,
                      sizeof(USER_ALL_INFORMATION));

        if (InternalAllBuffer.WhichFields & (USER_ALL_LMPASSWORDPRESENT | USER_ALL_NTPASSWORDPRESENT))
        {
            if (InternalAllBuffer.WhichFields & USER_ALL_OWFPASSWORD)
            {
                /* Check NT password hash */
                if (InternalAllBuffer.WhichFields & USER_ALL_NTPASSWORDPRESENT)
                {
                    if (InternalAllBuffer.NtPassword.Length != sizeof(ENCRYPTED_NT_OWF_PASSWORD))
                        return STATUS_INVALID_PARAMETER;
                }

                /* Check LM password hash */
                if (InternalAllBuffer.WhichFields & USER_ALL_LMPASSWORDPRESENT)
                {
                    if (InternalAllBuffer.LmPassword.Length != sizeof(ENCRYPTED_LM_OWF_PASSWORD))
                        return STATUS_INVALID_PARAMETER;
                }
            }
            else
            {
                /*
                 * Only allow the NT password to be set.
                 * The LM password will be created here.
                 */
                if (InternalAllBuffer.WhichFields & USER_ALL_LMPASSWORDPRESENT)
                {
                    TRACE("Do not try to set a clear text LM password!\n");
                    return STATUS_INVALID_PARAMETER;
                }

                if (InternalAllBuffer.WhichFields & USER_ALL_NTPASSWORDPRESENT)
                {
                    Status = SampCheckPassword(UserHandle,
                                               &InternalAllBuffer.NtPassword);
                    if (!NT_SUCCESS(Status))
                    {
                        TRACE("SampCheckPassword failed (Status 0x%08lx)\n", Status);
                        return Status;
                    }

                    /* Calculate the NT password hash */
                    Status = SystemFunction007((PUNICODE_STRING)&InternalAllBuffer.NtPassword,
                                               (LPBYTE)&Internal1Buffer.EncryptedNtOwfPassword);
                    if (!NT_SUCCESS(Status))
                    {
                        TRACE("SystemFunction007 failed (Status 0x%08lx)\n", Status);
                        return Status;
                    }

                    InternalAllBuffer.NtPasswordPresent = TRUE;
                    InternalAllBuffer.LmPasswordPresent = FALSE;

                    InternalAllBuffer.NtPassword.Length = sizeof(ENCRYPTED_NT_OWF_PASSWORD);
                    InternalAllBuffer.NtPassword.MaximumLength = sizeof(ENCRYPTED_NT_OWF_PASSWORD);
                    InternalAllBuffer.NtPassword.Buffer = (LPWSTR)&Internal1Buffer.EncryptedNtOwfPassword;

                    /* Build the LM password */
                    LmPwdString.Length = 15;
                    LmPwdString.MaximumLength = 15;
                    LmPwdString.Buffer = LmPwdBuffer;
                    ZeroMemory(LmPwdString.Buffer, LmPwdString.MaximumLength);

                    Status = RtlUpcaseUnicodeStringToOemString(&LmPwdString,
                                                               (PUNICODE_STRING)&InternalAllBuffer.NtPassword,
                                                               FALSE);
                    if (NT_SUCCESS(Status))
                    {
                        /* Calculate the LM password hash */
                        Status = SystemFunction006(LmPwdString.Buffer,
                                                   (LPSTR)&Internal1Buffer.EncryptedLmOwfPassword);
                        if (NT_SUCCESS(Status))
                        {
                            InternalAllBuffer.WhichFields |= USER_ALL_LMPASSWORDPRESENT;
                            InternalAllBuffer.LmPasswordPresent = TRUE;

                            InternalAllBuffer.LmPassword.Length = sizeof(ENCRYPTED_LM_OWF_PASSWORD);
                            InternalAllBuffer.LmPassword.MaximumLength = sizeof(ENCRYPTED_LM_OWF_PASSWORD);
                            InternalAllBuffer.LmPassword.Buffer = (LPWSTR)&Internal1Buffer.EncryptedLmOwfPassword;
                        }
                    }
                }
            }
        }

        RpcTryExcept
        {
            Status = SamrSetInformationUser((SAMPR_HANDLE)UserHandle,
                                            UserAllInformation,
                                            (PVOID)&InternalAllBuffer);
        }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = I_RpcMapWin32Status(RpcExceptionCode());
        }
        RpcEndExcept;

        if (!NT_SUCCESS(Status))
        {
            TRACE("SamrSetInformation() failed (Status 0x%08lx)\n", Status);
        }

        return Status;
    }

    RpcTryExcept
    {
        Status = SamrSetInformationUser((SAMPR_HANDLE)UserHandle,
                                        UserInformationClass,
                                        Buffer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamSetMemberAttributesOfGroup(IN SAM_HANDLE GroupHandle,
                              IN ULONG MemberId,
                              IN ULONG Attributes)
{
    NTSTATUS Status;

    TRACE("SamSetMemberAttributesOfGroup(%p %lu 0x%lx)\n",
          GroupHandle, MemberId, Attributes);

    RpcTryExcept
    {
        Status = SamrSetMemberAttributesOfGroup((SAMPR_HANDLE)GroupHandle,
                                                MemberId,
                                                Attributes);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamSetSecurityObject(IN SAM_HANDLE ObjectHandle,
                     IN SECURITY_INFORMATION SecurityInformation,
                     IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    SAMPR_SR_SECURITY_DESCRIPTOR DescriptorToPass;
    ULONG Length;
    NTSTATUS Status;

    TRACE("SamSetSecurityObject(%p %lu %p)\n",
          ObjectHandle, SecurityInformation, SecurityDescriptor);

    /* Retrieve the length of the relative security descriptor */
    Length = 0;
    Status = RtlMakeSelfRelativeSD(SecurityDescriptor,
                                   NULL,
                                   &Length);
    if (Status != STATUS_BUFFER_TOO_SMALL)
        return STATUS_INVALID_PARAMETER;


    /* Allocate a buffer for the security descriptor */
    DescriptorToPass.Length = Length;
    DescriptorToPass.SecurityDescriptor = MIDL_user_allocate(Length);
    if (DescriptorToPass.SecurityDescriptor == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Convert the given security descriptor to a relative security descriptor */
    Status = RtlMakeSelfRelativeSD(SecurityDescriptor,
                                   (PSECURITY_DESCRIPTOR)DescriptorToPass.SecurityDescriptor,
                                   &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    RpcTryExcept
    {
        Status = SamrSetSecurityObject((SAMPR_HANDLE)ObjectHandle,
                                       SecurityInformation,
                                       &DescriptorToPass);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

done:
    if (DescriptorToPass.SecurityDescriptor != NULL)
        MIDL_user_free(DescriptorToPass.SecurityDescriptor);

    return Status;
}


NTSTATUS
NTAPI
SamShutdownSamServer(IN SAM_HANDLE ServerHandle)
{
    NTSTATUS Status;

    TRACE("SamShutdownSamServer(%p)\n", ServerHandle);

    RpcTryExcept
    {
        Status = SamrShutdownSamServer((SAMPR_HANDLE)ServerHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


NTSTATUS
NTAPI
SamTestPrivateFunctionsDomain(
    _In_ SAM_HANDLE DomainHandle)
{
    TRACE("SamTestPrivateFunctionsDomain(%p)\n",
          DomainHandle);
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
SamTestPrivateFunctionsUser(
    _In_ SAM_HANDLE UserHandle)
{
    TRACE("SamTestPrivateFunctionsUser(%p)\n",
          UserHandle);
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
