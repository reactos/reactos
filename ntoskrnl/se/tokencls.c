/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Access token Query/Set information classes implementation
 * COPYRIGHT:       Copyright David Welch <welch@cwcom.net>
 *                  Copyright 2021-2022 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#include <ntlsa.h>

/* INFORMATION CLASSES ********************************************************/

static const INFORMATION_CLASS_INFO SeTokenInformationClass[] = {

    /* Class 0 not used, blame MS! */
    IQS_NONE,

    /* TokenUser */
    IQS_SAME(TOKEN_USER, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE),
    /* TokenGroups */
    IQS_SAME(TOKEN_GROUPS, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE),
    /* TokenPrivileges */
    IQS_SAME(TOKEN_PRIVILEGES, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE),
    /* TokenOwner */
    IQS_SAME(TOKEN_OWNER, ULONG, ICIF_QUERY | ICIF_SET | ICIF_SIZE_VARIABLE),
    /* TokenPrimaryGroup */
    IQS_SAME(TOKEN_PRIMARY_GROUP, ULONG, ICIF_QUERY | ICIF_SET | ICIF_SIZE_VARIABLE),
    /* TokenDefaultDacl */
    IQS_SAME(TOKEN_DEFAULT_DACL, ULONG, ICIF_QUERY | ICIF_SET | ICIF_SIZE_VARIABLE),
    /* TokenSource */
    IQS_SAME(TOKEN_SOURCE, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE),
    /* TokenType */
    IQS_SAME(TOKEN_TYPE, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE),
    /* TokenImpersonationLevel */
    IQS_SAME(SECURITY_IMPERSONATION_LEVEL, ULONG, ICIF_QUERY),
    /* TokenStatistics */
    IQS_SAME(TOKEN_STATISTICS, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE),
    /* TokenRestrictedSids */
    IQS_SAME(TOKEN_GROUPS, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE),
    /* TokenSessionId */
    IQS_SAME(ULONG, ULONG, ICIF_QUERY | ICIF_SET),
    /* TokenGroupsAndPrivileges */
    IQS_SAME(TOKEN_GROUPS_AND_PRIVILEGES, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE),
    /* TokenSessionReference */
    IQS_SAME(ULONG, ULONG, ICIF_SET),
    /* TokenSandBoxInert */
    IQS_SAME(ULONG, ULONG, ICIF_QUERY),
    /* TokenAuditPolicy */
    IQS_SAME(TOKEN_AUDIT_POLICY_INFORMATION, ULONG, ICIF_SET | ICIF_SET_SIZE_VARIABLE),
    /* TokenOrigin */
    IQS_SAME(TOKEN_ORIGIN, ULONG, ICIF_QUERY | ICIF_SET),
};

/* PUBLIC FUNCTIONS *****************************************************************/

/**
 * @brief
 * Queries information details about the given token to the call. The difference
 * between NtQueryInformationToken and this routine is that the system call has
 * user mode buffer data probing and additional protection checks whereas this
 * routine doesn't have any of these. The routine is used exclusively in kernel
 * mode.
 *
 * @param[in] AccessToken
 * An access token to be given.
 *
 * @param[in] TokenInformationClass
 * Token information class.
 *
 * @param[out] TokenInformation
 * Buffer with retrieved information. Such information is arbitrary, depending
 * on the requested information class.
 *
 * @return
 * Returns STATUS_SUCCESS if the operation to query the desired information
 * has completed successfully. STATUS_INSUFFICIENT_RESOURCES is returned if
 * pool memory allocation has failed to satisfy an operation. Otherwise
 * STATUS_INVALID_INFO_CLASS is returned indicating that the information
 * class provided is not supported by the routine.
 *
 * @remarks
 * Only certain information classes are not implemented in this function and
 * these are TokenOrigin, TokenGroupsAndPrivileges, TokenRestrictedSids and
 * TokenSandBoxInert. The following classes are implemented in NtQueryInformationToken
 * only.
 */
NTSTATUS
NTAPI
SeQueryInformationToken(
    _In_ PACCESS_TOKEN AccessToken,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _Outptr_result_buffer_(_Inexpressible_(token-dependent)) PVOID *TokenInformation)
{
    NTSTATUS Status;
    PTOKEN Token = (PTOKEN)AccessToken;
    ULONG RequiredLength;
    union
    {
        PSID PSid;
        ULONG Ulong;
    } Unused;

    PAGED_CODE();

    /* Lock the token */
    SepAcquireTokenLockShared(Token);

    switch (TokenInformationClass)
    {
        case TokenUser:
        {
            PTOKEN_USER tu;

            DPRINT("SeQueryInformationToken(TokenUser)\n");
            RequiredLength = sizeof(TOKEN_USER) +
                RtlLengthSid(Token->UserAndGroups[0].Sid);

            /* Allocate the output buffer */
            tu = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (tu == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            Status = RtlCopySidAndAttributesArray(1,
                                                  &Token->UserAndGroups[0],
                                                  RequiredLength - sizeof(TOKEN_USER),
                                                  &tu->User,
                                                  (PSID)(tu + 1),
                                                  &Unused.PSid,
                                                  &Unused.Ulong);

            /* Return the structure */
            *TokenInformation = tu;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenGroups:
        {
            PTOKEN_GROUPS tg;
            ULONG SidLen;
            PSID Sid;

            DPRINT("SeQueryInformationToken(TokenGroups)\n");
            RequiredLength = sizeof(tg->GroupCount) +
                RtlLengthSidAndAttributes(Token->UserAndGroupCount - 1, &Token->UserAndGroups[1]);

            SidLen = RequiredLength - sizeof(tg->GroupCount) -
                ((Token->UserAndGroupCount - 1) * sizeof(SID_AND_ATTRIBUTES));

            /* Allocate the output buffer */
            tg = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (tg == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            Sid = (PSID)((ULONG_PTR)tg + sizeof(tg->GroupCount) +
                         ((Token->UserAndGroupCount - 1) * sizeof(SID_AND_ATTRIBUTES)));

            tg->GroupCount = Token->UserAndGroupCount - 1;
            Status = RtlCopySidAndAttributesArray(Token->UserAndGroupCount - 1,
                                                  &Token->UserAndGroups[1],
                                                  SidLen,
                                                  &tg->Groups[0],
                                                  Sid,
                                                  &Unused.PSid,
                                                  &Unused.Ulong);

            /* Return the structure */
            *TokenInformation = tg;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenPrivileges:
        {
            PTOKEN_PRIVILEGES tp;

            DPRINT("SeQueryInformationToken(TokenPrivileges)\n");
            RequiredLength = sizeof(tp->PrivilegeCount) +
                (Token->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));

            /* Allocate the output buffer */
            tp = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (tp == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            tp->PrivilegeCount = Token->PrivilegeCount;
            RtlCopyLuidAndAttributesArray(Token->PrivilegeCount,
                                          Token->Privileges,
                                          &tp->Privileges[0]);

            /* Return the structure */
            *TokenInformation = tp;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenOwner:
        {
            PTOKEN_OWNER to;
            ULONG SidLen;

            DPRINT("SeQueryInformationToken(TokenOwner)\n");
            SidLen = RtlLengthSid(Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
            RequiredLength = sizeof(TOKEN_OWNER) + SidLen;

            /* Allocate the output buffer */
            to = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (to == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            to->Owner = (PSID)(to + 1);
            Status = RtlCopySid(SidLen,
                                to->Owner,
                                Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);

            /* Return the structure */
            *TokenInformation = to;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenPrimaryGroup:
        {
            PTOKEN_PRIMARY_GROUP tpg;
            ULONG SidLen;

            DPRINT("SeQueryInformationToken(TokenPrimaryGroup)\n");
            SidLen = RtlLengthSid(Token->PrimaryGroup);
            RequiredLength = sizeof(TOKEN_PRIMARY_GROUP) + SidLen;

            /* Allocate the output buffer */
            tpg = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (tpg == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            tpg->PrimaryGroup = (PSID)(tpg + 1);
            Status = RtlCopySid(SidLen,
                                tpg->PrimaryGroup,
                                Token->PrimaryGroup);

            /* Return the structure */
            *TokenInformation = tpg;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenDefaultDacl:
        {
            PTOKEN_DEFAULT_DACL tdd;

            DPRINT("SeQueryInformationToken(TokenDefaultDacl)\n");
            RequiredLength = sizeof(TOKEN_DEFAULT_DACL);

            if (Token->DefaultDacl != NULL)
                RequiredLength += Token->DefaultDacl->AclSize;

            /* Allocate the output buffer */
            tdd = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (tdd == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            if (Token->DefaultDacl != NULL)
            {
                tdd->DefaultDacl = (PACL)(tdd + 1);
                RtlCopyMemory(tdd->DefaultDacl,
                              Token->DefaultDacl,
                              Token->DefaultDacl->AclSize);
            }
            else
            {
                tdd->DefaultDacl = NULL;
            }

            /* Return the structure */
            *TokenInformation = tdd;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenSource:
        {
            PTOKEN_SOURCE ts;

            DPRINT("SeQueryInformationToken(TokenSource)\n");
            RequiredLength = sizeof(TOKEN_SOURCE);

            /* Allocate the output buffer */
            ts = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (ts == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            *ts = Token->TokenSource;

            /* Return the structure */
            *TokenInformation = ts;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenType:
        {
            PTOKEN_TYPE tt;

            DPRINT("SeQueryInformationToken(TokenType)\n");
            RequiredLength = sizeof(TOKEN_TYPE);

            /* Allocate the output buffer */
            tt = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (tt == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            *tt = Token->TokenType;

            /* Return the structure */
            *TokenInformation = tt;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenImpersonationLevel:
        {
            PSECURITY_IMPERSONATION_LEVEL sil;

            DPRINT("SeQueryInformationToken(TokenImpersonationLevel)\n");
            RequiredLength = sizeof(SECURITY_IMPERSONATION_LEVEL);

            /* Fail if the token is not an impersonation token */
            if (Token->TokenType != TokenImpersonation)
            {
                Status = STATUS_INVALID_INFO_CLASS;
                break;
            }

            /* Allocate the output buffer */
            sil = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (sil == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            *sil = Token->ImpersonationLevel;

            /* Return the structure */
            *TokenInformation = sil;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenStatistics:
        {
            PTOKEN_STATISTICS ts;

            DPRINT("SeQueryInformationToken(TokenStatistics)\n");
            RequiredLength = sizeof(TOKEN_STATISTICS);

            /* Allocate the output buffer */
            ts = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (ts == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            ts->TokenId = Token->TokenId;
            ts->AuthenticationId = Token->AuthenticationId;
            ts->ExpirationTime = Token->ExpirationTime;
            ts->TokenType = Token->TokenType;
            ts->ImpersonationLevel = Token->ImpersonationLevel;
            ts->DynamicCharged = Token->DynamicCharged;
            ts->DynamicAvailable = SepComputeAvailableDynamicSpace(Token->DynamicCharged, Token->PrimaryGroup, Token->DefaultDacl);
            ts->GroupCount = Token->UserAndGroupCount - 1;
            ts->PrivilegeCount = Token->PrivilegeCount;
            ts->ModifiedId = Token->ModifiedId;

            /* Return the structure */
            *TokenInformation = ts;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenSessionId:
        {
            DPRINT("SeQueryInformationToken(TokenSessionId)\n");
            Status = SeQuerySessionIdToken(Token, (PULONG)TokenInformation);
            break;
        }

        default:
            DPRINT1("SeQueryInformationToken(%d) invalid information class\n", TokenInformationClass);
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    /* Release the lock of the token */
    SepReleaseTokenLock(Token);

    return Status;
}

/* SYSTEM CALLS ***************************************************************/

/**
 * @brief
 * Queries a specific type of information in regard of an access token based upon
 * the information class. The calling thread must have specific access rights in order
 * to obtain specific information about the token.
 *
 * @param[in] TokenHandle
 * A handle of a token where information is to be gathered.
 *
 * @param[in] TokenInformationClass
 * Token information class.
 *
 * @param[out] TokenInformation
 * A returned output buffer with token information, which information is arbitrarily upon
 * the information class chosen.
 *
 * @param[in] TokenInformationLength
 * Length of the token information buffer, in bytes.
 *
 * @param[out] ReturnLength
 * A pointer to a variable provided by the caller that receives the actual length
 * of the buffer pointed by TokenInformation, in bytes. If TokenInformation is NULL
 * and TokenInformationLength is 0, this parameter receives the required length
 * needed to store the buffer information in memory. This parameter must not
 * be NULL!
 *
 * @return
 * Returns STATUS_SUCCESS if information querying has completed successfully.
 * STATUS_BUFFER_TOO_SMALL is returned if the information length that represents
 * the token information buffer is not greater than the required length.
 * STATUS_INVALID_HANDLE is returned if the token handle is not a valid one.
 * STATUS_INVALID_INFO_CLASS is returned if the information class is not a valid
 * one (that is, the class doesn't belong to TOKEN_INFORMATION_CLASS).
 * STATUS_ACCESS_VIOLATION is returned if ReturnLength is NULL. A failure NTSTATUS
 * code is returned otherwise.
 */
_Must_inspect_result_
__kernel_entry
NTSTATUS
NTAPI
NtQueryInformationToken(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _Out_writes_bytes_to_opt_(TokenInformationLength, *ReturnLength)
        PVOID TokenInformation,
    _In_ ULONG TokenInformationLength,
    _Out_ PULONG ReturnLength)
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;
    PTOKEN Token;
    ULONG RequiredLength = 0;
    union
    {
        PSID PSid;
        ULONG Ulong;
    } Unused;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    /* Check buffers and class validity */
    Status = DefaultQueryInfoBufferCheck(TokenInformationClass,
                                         SeTokenInformationClass,
                                         RTL_NUMBER_OF(SeTokenInformationClass),
                                         ICIF_PROBE_READ_WRITE | ICIF_FORCE_RETURN_LENGTH_PROBE,
                                         TokenInformation,
                                         TokenInformationLength,
                                         ReturnLength,
                                         NULL,
                                         PreviousMode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtQueryInformationToken() failed, Status: 0x%x\n", Status);
        return Status;
    }

    Status = ObReferenceObjectByHandle(TokenHandle,
                                       (TokenInformationClass == TokenSource) ? TOKEN_QUERY_SOURCE : TOKEN_QUERY,
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Lock the token */
        SepAcquireTokenLockShared(Token);

        switch (TokenInformationClass)
        {
            case TokenUser:
            {
                PTOKEN_USER tu = (PTOKEN_USER)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenUser)\n");
                RequiredLength = sizeof(TOKEN_USER) +
                    RtlLengthSid(Token->UserAndGroups[0].Sid);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        Status = RtlCopySidAndAttributesArray(1,
                                                              &Token->UserAndGroups[0],
                                                              RequiredLength - sizeof(TOKEN_USER),
                                                              &tu->User,
                                                              (PSID)(tu + 1),
                                                              &Unused.PSid,
                                                              &Unused.Ulong);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    *ReturnLength = RequiredLength;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenGroups:
            {
                PTOKEN_GROUPS tg = (PTOKEN_GROUPS)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenGroups)\n");
                RequiredLength = sizeof(tg->GroupCount) +
                    RtlLengthSidAndAttributes(Token->UserAndGroupCount - 1, &Token->UserAndGroups[1]);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        ULONG SidLen = RequiredLength - sizeof(tg->GroupCount) -
                            ((Token->UserAndGroupCount - 1) * sizeof(SID_AND_ATTRIBUTES));
                        PSID Sid = (PSID_AND_ATTRIBUTES)((ULONG_PTR)tg + sizeof(tg->GroupCount) +
                                                         ((Token->UserAndGroupCount - 1) * sizeof(SID_AND_ATTRIBUTES)));

                        tg->GroupCount = Token->UserAndGroupCount - 1;
                        Status = RtlCopySidAndAttributesArray(Token->UserAndGroupCount - 1,
                                                              &Token->UserAndGroups[1],
                                                              SidLen,
                                                              &tg->Groups[0],
                                                              Sid,
                                                              &Unused.PSid,
                                                              &Unused.Ulong);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    *ReturnLength = RequiredLength;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenPrivileges:
            {
                PTOKEN_PRIVILEGES tp = (PTOKEN_PRIVILEGES)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenPrivileges)\n");
                RequiredLength = sizeof(tp->PrivilegeCount) +
                    (Token->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        tp->PrivilegeCount = Token->PrivilegeCount;
                        RtlCopyLuidAndAttributesArray(Token->PrivilegeCount,
                                                      Token->Privileges,
                                                      &tp->Privileges[0]);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    *ReturnLength = RequiredLength;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenOwner:
            {
                PTOKEN_OWNER to = (PTOKEN_OWNER)TokenInformation;
                ULONG SidLen;

                DPRINT("NtQueryInformationToken(TokenOwner)\n");
                SidLen = RtlLengthSid(Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
                RequiredLength = sizeof(TOKEN_OWNER) + SidLen;

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        to->Owner = (PSID)(to + 1);
                        Status = RtlCopySid(SidLen,
                                            to->Owner,
                                            Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                   *ReturnLength = RequiredLength;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenPrimaryGroup:
            {
                PTOKEN_PRIMARY_GROUP tpg = (PTOKEN_PRIMARY_GROUP)TokenInformation;
                ULONG SidLen;

                DPRINT("NtQueryInformationToken(TokenPrimaryGroup)\n");
                SidLen = RtlLengthSid(Token->PrimaryGroup);
                RequiredLength = sizeof(TOKEN_PRIMARY_GROUP) + SidLen;

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        tpg->PrimaryGroup = (PSID)(tpg + 1);
                        Status = RtlCopySid(SidLen,
                                            tpg->PrimaryGroup,
                                            Token->PrimaryGroup);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    *ReturnLength = RequiredLength;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenDefaultDacl:
            {
                PTOKEN_DEFAULT_DACL tdd = (PTOKEN_DEFAULT_DACL)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenDefaultDacl)\n");
                RequiredLength = sizeof(TOKEN_DEFAULT_DACL);

                if (Token->DefaultDacl != NULL)
                    RequiredLength += Token->DefaultDacl->AclSize;

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        if (Token->DefaultDacl != NULL)
                        {
                            tdd->DefaultDacl = (PACL)(tdd + 1);
                            RtlCopyMemory(tdd->DefaultDacl,
                                          Token->DefaultDacl,
                                          Token->DefaultDacl->AclSize);
                        }
                        else
                        {
                            tdd->DefaultDacl = NULL;
                        }
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    *ReturnLength = RequiredLength;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenSource:
            {
                PTOKEN_SOURCE ts = (PTOKEN_SOURCE)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenSource)\n");
                RequiredLength = sizeof(TOKEN_SOURCE);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        *ts = Token->TokenSource;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                   *ReturnLength = RequiredLength;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenType:
            {
                PTOKEN_TYPE tt = (PTOKEN_TYPE)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenType)\n");
                RequiredLength = sizeof(TOKEN_TYPE);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        *tt = Token->TokenType;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    *ReturnLength = RequiredLength;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenImpersonationLevel:
            {
                PSECURITY_IMPERSONATION_LEVEL sil = (PSECURITY_IMPERSONATION_LEVEL)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenImpersonationLevel)\n");

                /* Fail if the token is not an impersonation token */
                if (Token->TokenType != TokenImpersonation)
                {
                    Status = STATUS_INVALID_INFO_CLASS;
                    break;
                }

                RequiredLength = sizeof(SECURITY_IMPERSONATION_LEVEL);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        *sil = Token->ImpersonationLevel;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    *ReturnLength = RequiredLength;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenStatistics:
            {
                PTOKEN_STATISTICS ts = (PTOKEN_STATISTICS)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenStatistics)\n");
                RequiredLength = sizeof(TOKEN_STATISTICS);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        ts->TokenId = Token->TokenId;
                        ts->AuthenticationId = Token->AuthenticationId;
                        ts->ExpirationTime = Token->ExpirationTime;
                        ts->TokenType = Token->TokenType;
                        ts->ImpersonationLevel = Token->ImpersonationLevel;
                        ts->DynamicCharged = Token->DynamicCharged;
                        ts->DynamicAvailable = SepComputeAvailableDynamicSpace(Token->DynamicCharged, Token->PrimaryGroup, Token->DefaultDacl);
                        ts->GroupCount = Token->UserAndGroupCount - 1;
                        ts->PrivilegeCount = Token->PrivilegeCount;
                        ts->ModifiedId = Token->ModifiedId;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    *ReturnLength = RequiredLength;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenOrigin:
            {
                PTOKEN_ORIGIN to = (PTOKEN_ORIGIN)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenOrigin)\n");
                RequiredLength = sizeof(TOKEN_ORIGIN);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        to->OriginatingLogonSession = Token->AuthenticationId;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    *ReturnLength = RequiredLength;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenGroupsAndPrivileges:
            {
                PSID Sid, RestrictedSid;
                ULONG SidLen, RestrictedSidLen;
                ULONG UserGroupLength, RestrictedSidLength, PrivilegeLength;
                PTOKEN_GROUPS_AND_PRIVILEGES GroupsAndPrivs = (PTOKEN_GROUPS_AND_PRIVILEGES)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenGroupsAndPrivileges)\n");
                UserGroupLength = RtlLengthSidAndAttributes(Token->UserAndGroupCount, Token->UserAndGroups);
                RestrictedSidLength = RtlLengthSidAndAttributes(Token->RestrictedSidCount, Token->RestrictedSids);
                PrivilegeLength = Token->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);

                RequiredLength = sizeof(TOKEN_GROUPS_AND_PRIVILEGES) +
                                 UserGroupLength + RestrictedSidLength + PrivilegeLength;

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        GroupsAndPrivs->SidCount = Token->UserAndGroupCount;
                        GroupsAndPrivs->SidLength = UserGroupLength;
                        GroupsAndPrivs->Sids = (PSID_AND_ATTRIBUTES)(GroupsAndPrivs + 1);

                        Sid = (PSID)((ULONG_PTR)GroupsAndPrivs->Sids + (Token->UserAndGroupCount * sizeof(SID_AND_ATTRIBUTES)));
                        SidLen = UserGroupLength - (Token->UserAndGroupCount * sizeof(SID_AND_ATTRIBUTES));
                        Status = RtlCopySidAndAttributesArray(Token->UserAndGroupCount,
                                                              Token->UserAndGroups,
                                                              SidLen,
                                                              GroupsAndPrivs->Sids,
                                                              Sid,
                                                              &Unused.PSid,
                                                              &Unused.Ulong);
                        NT_ASSERT(NT_SUCCESS(Status));

                        GroupsAndPrivs->RestrictedSidCount = Token->RestrictedSidCount;
                        GroupsAndPrivs->RestrictedSidLength = RestrictedSidLength;
                        GroupsAndPrivs->RestrictedSids = NULL;
                        if (SeTokenIsRestricted(Token))
                        {
                            GroupsAndPrivs->RestrictedSids = (PSID_AND_ATTRIBUTES)((ULONG_PTR)GroupsAndPrivs->Sids + UserGroupLength);

                            RestrictedSid = (PSID)((ULONG_PTR)GroupsAndPrivs->RestrictedSids + (Token->RestrictedSidCount * sizeof(SID_AND_ATTRIBUTES)));
                            RestrictedSidLen = RestrictedSidLength - (Token->RestrictedSidCount * sizeof(SID_AND_ATTRIBUTES));
                            Status = RtlCopySidAndAttributesArray(Token->RestrictedSidCount,
                                                                  Token->RestrictedSids,
                                                                  RestrictedSidLen,
                                                                  GroupsAndPrivs->RestrictedSids,
                                                                  RestrictedSid,
                                                                  &Unused.PSid,
                                                                  &Unused.Ulong);
                            NT_ASSERT(NT_SUCCESS(Status));
                        }

                        GroupsAndPrivs->PrivilegeCount = Token->PrivilegeCount;
                        GroupsAndPrivs->PrivilegeLength = PrivilegeLength;
                        GroupsAndPrivs->Privileges = (PLUID_AND_ATTRIBUTES)((ULONG_PTR)(GroupsAndPrivs + 1) +
                                                     UserGroupLength + RestrictedSidLength);
                        RtlCopyLuidAndAttributesArray(Token->PrivilegeCount,
                                                      Token->Privileges,
                                                      GroupsAndPrivs->Privileges);

                        GroupsAndPrivs->AuthenticationId = Token->AuthenticationId;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    *ReturnLength = RequiredLength;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenRestrictedSids:
            {
                PTOKEN_GROUPS tg = (PTOKEN_GROUPS)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenRestrictedSids)\n");
                RequiredLength = sizeof(tg->GroupCount) +
                RtlLengthSidAndAttributes(Token->RestrictedSidCount, Token->RestrictedSids);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        ULONG SidLen = RequiredLength - sizeof(tg->GroupCount) -
                            (Token->RestrictedSidCount * sizeof(SID_AND_ATTRIBUTES));
                        PSID Sid = (PSID)((ULONG_PTR)tg + sizeof(tg->GroupCount) +
                                          (Token->RestrictedSidCount * sizeof(SID_AND_ATTRIBUTES)));

                        tg->GroupCount = Token->RestrictedSidCount;
                        Status = RtlCopySidAndAttributesArray(Token->RestrictedSidCount,
                                                              Token->RestrictedSids,
                                                              SidLen,
                                                              &tg->Groups[0],
                                                              Sid,
                                                              &Unused.PSid,
                                                              &Unused.Ulong);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    *ReturnLength = RequiredLength;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenSandBoxInert:
            {
                ULONG IsTokenSandBoxInert;

                DPRINT("NtQueryInformationToken(TokenSandBoxInert)\n");

                IsTokenSandBoxInert = SeTokenIsInert(Token);
                _SEH2_TRY
                {
                    /* Buffer size was already verified, no need to check here again */
                    *(PULONG)TokenInformation = IsTokenSandBoxInert;
                    *ReturnLength = sizeof(ULONG);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenSessionId:
            {
                ULONG SessionId = 0;

                DPRINT("NtQueryInformationToken(TokenSessionId)\n");

                Status = SeQuerySessionIdToken(Token, &SessionId);
                if (NT_SUCCESS(Status))
                {
                    _SEH2_TRY
                    {
                        /* Buffer size was already verified, no need to check here again */
                        *(PULONG)TokenInformation = SessionId;
                        *ReturnLength = RequiredLength;
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = _SEH2_GetExceptionCode();
                    }
                    _SEH2_END;
                }

                break;
            }

            default:
                DPRINT1("NtQueryInformationToken(%d) invalid information class\n", TokenInformationClass);
                Status = STATUS_INVALID_INFO_CLASS;
                break;
        }

        /* Unlock and dereference the token */
        SepReleaseTokenLock(Token);
        ObDereferenceObject(Token);
    }

    return Status;
}

/**
 * @unimplemented
 * @brief
 * Sets (modifies) some specific information in regard of an access token. The
 * calling thread must have specific access rights in order to modify token's
 * information data.
 *
 * @param[in] TokenHandle
 * A handle of a token where information is to be modified.
 *
 * @param[in] TokenInformationClass
 * Token information class.
 *
 * @param[in] TokenInformation
 * An arbitrary pointer to a buffer with token information to set. Such
 * arbitrary buffer depends on the information class chosen that the caller
 * wants to modify such information data of a token.
 *
 * @param[in] TokenInformationLength
 * Length of the token information buffer, in bytes.
 *
 * @return
 * Returns STATUS_SUCCESS if information setting has completed successfully.
 * STATUS_INFO_LENGTH_MISMATCH is returned if the information length of the
 * buffer is less than the required length. STATUS_INSUFFICIENT_RESOURCES is
 * returned if memory pool allocation has failed. STATUS_PRIVILEGE_NOT_HELD
 * is returned if the calling thread hasn't the required privileges to perform
 * the operation in question. A failure NTSTATUS code is returned otherwise.
 *
 * @remarks
 * The function is partly implemented, mainly TokenOrigin.
 */
_Must_inspect_result_
__kernel_entry
NTSTATUS
NTAPI
NtSetInformationToken(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _In_reads_bytes_(TokenInformationLength) PVOID TokenInformation,
    _In_ ULONG TokenInformationLength)
{
    NTSTATUS Status;
    PTOKEN Token;
    KPROCESSOR_MODE PreviousMode;
    ULONG NeededAccess = TOKEN_ADJUST_DEFAULT;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    Status = DefaultSetInfoBufferCheck(TokenInformationClass,
                                       SeTokenInformationClass,
                                       RTL_NUMBER_OF(SeTokenInformationClass),
                                       TokenInformation,
                                       TokenInformationLength,
                                       PreviousMode);
    if (!NT_SUCCESS(Status))
    {
        /* Invalid buffers */
        DPRINT("NtSetInformationToken() failed, Status: 0x%x\n", Status);
        return Status;
    }

    if (TokenInformationClass == TokenSessionId)
    {
        NeededAccess |= TOKEN_ADJUST_SESSIONID;
    }

    Status = ObReferenceObjectByHandle(TokenHandle,
                                       NeededAccess,
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        switch (TokenInformationClass)
        {
            case TokenOwner:
            {
                if (TokenInformationLength >= sizeof(TOKEN_OWNER))
                {
                    PTOKEN_OWNER to = (PTOKEN_OWNER)TokenInformation;
                    PSID InputSid = NULL, CapturedSid;
                    ULONG DefaultOwnerIndex;

                    _SEH2_TRY
                    {
                        InputSid = to->Owner;
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = _SEH2_GetExceptionCode();
                        _SEH2_YIELD(goto Cleanup);
                    }
                    _SEH2_END;

                    Status = SepCaptureSid(InputSid,
                                           PreviousMode,
                                           PagedPool,
                                           FALSE,
                                           &CapturedSid);
                    if (NT_SUCCESS(Status))
                    {
                        /* Lock the token */
                        SepAcquireTokenLockExclusive(Token);

                        /* Find the owner amongst the existing token user and groups */
                        Status = SepFindPrimaryGroupAndDefaultOwner(Token,
                                                                    NULL,
                                                                    CapturedSid,
                                                                    NULL,
                                                                    &DefaultOwnerIndex);
                        if (NT_SUCCESS(Status))
                        {
                            /* Found it */
                            Token->DefaultOwnerIndex = DefaultOwnerIndex;
                            ExAllocateLocallyUniqueId(&Token->ModifiedId);
                        }

                        /* Unlock the token */
                        SepReleaseTokenLock(Token);

                        SepReleaseSid(CapturedSid,
                                      PreviousMode,
                                      FALSE);
                    }
                }
                else
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                break;
            }

            case TokenPrimaryGroup:
            {
                if (TokenInformationLength >= sizeof(TOKEN_PRIMARY_GROUP))
                {
                    PTOKEN_PRIMARY_GROUP tpg = (PTOKEN_PRIMARY_GROUP)TokenInformation;
                    ULONG AclSize;
                    ULONG_PTR PrimaryGroup;
                    PSID InputSid = NULL, CapturedSid;
                    ULONG PrimaryGroupIndex, NewDynamicLength;

                    _SEH2_TRY
                    {
                        InputSid = tpg->PrimaryGroup;
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = _SEH2_GetExceptionCode();
                        _SEH2_YIELD(goto Cleanup);
                    }
                    _SEH2_END;

                    Status = SepCaptureSid(InputSid,
                                           PreviousMode,
                                           PagedPool,
                                           FALSE,
                                           &CapturedSid);
                    if (NT_SUCCESS(Status))
                    {
                        /* Lock the token */
                        SepAcquireTokenLockExclusive(Token);

                        /*
                         * We can whack the token's primary group only if
                         * the charged dynamic space boundary allows us
                         * to do so. Exceeding this boundary and we're
                         * busted out.
                         */
                        AclSize = Token->DefaultDacl ? Token->DefaultDacl->AclSize : 0;
                        NewDynamicLength = RtlLengthSid(CapturedSid) + AclSize;
                        if (NewDynamicLength > Token->DynamicCharged)
                        {
                            SepReleaseTokenLock(Token);
                            SepReleaseSid(CapturedSid, PreviousMode, FALSE);
                            Status = STATUS_ALLOTTED_SPACE_EXCEEDED;
                            DPRINT1("NtSetInformationToken(): Couldn't assign new primary group, space exceeded (current length %u, new length %lu)\n",
                                    Token->DynamicCharged, NewDynamicLength);
                            goto Cleanup;
                        }

                        /*
                         * The dynamic part of the token may require a rebuild
                         * if the current dynamic area is too small. If not then
                         * we're pretty much good as is.
                         */
                        Status = SepRebuildDynamicPartOfToken(Token, NewDynamicLength);
                        if (NT_SUCCESS(Status))
                        {
                            /* Find the primary group amongst the existing token user and groups */
                            Status = SepFindPrimaryGroupAndDefaultOwner(Token,
                                                                        CapturedSid,
                                                                        NULL,
                                                                        &PrimaryGroupIndex,
                                                                        NULL);
                            if (NT_SUCCESS(Status))
                            {
                                /*
                                 * We have found it. Add the length of
                                 * the previous primary group SID to the
                                 * available dynamic area.
                                 */
                                Token->DynamicAvailable += RtlLengthSid(Token->PrimaryGroup);

                                /*
                                 * Move the default DACL if it's not at the
                                 * head of the dynamic part.
                                 */
                                if ((Token->DefaultDacl) &&
                                    ((PULONG)(Token->DefaultDacl) != Token->DynamicPart))
                                {
                                    RtlMoveMemory(Token->DynamicPart,
                                                  Token->DefaultDacl,
                                                  RtlLengthSid(Token->PrimaryGroup));
                                    Token->DefaultDacl = (PACL)(Token->DynamicPart);
                                }

                                /* Take away available space from the dynamic area */
                                Token->DynamicAvailable -= RtlLengthSid(Token->UserAndGroups[PrimaryGroupIndex].Sid);

                                /*
                                 * And assign the new primary group. For that
                                 * we have to make sure where the primary group
                                 * is going to stay in memory, so if this token
                                 * has a default DACL then add up its size with
                                 * the address of the dynamic part.
                                 */
                                PrimaryGroup = (ULONG_PTR)(Token->DynamicPart) + AclSize;
                                RtlCopySid(RtlLengthSid(Token->UserAndGroups[PrimaryGroupIndex].Sid),
                                           (PVOID)PrimaryGroup,
                                           Token->UserAndGroups[PrimaryGroupIndex].Sid);
                                Token->PrimaryGroup = (PSID)PrimaryGroup;

                                ExAllocateLocallyUniqueId(&Token->ModifiedId);
                            }
                        }

                        /* Unlock the token */
                        SepReleaseTokenLock(Token);

                        SepReleaseSid(CapturedSid,
                                      PreviousMode,
                                      FALSE);
                    }
                }
                else
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                break;
            }

            case TokenDefaultDacl:
            {
                if (TokenInformationLength >= sizeof(TOKEN_DEFAULT_DACL))
                {
                    PTOKEN_DEFAULT_DACL tdd = (PTOKEN_DEFAULT_DACL)TokenInformation;
                    PACL InputAcl = NULL;

                    _SEH2_TRY
                    {
                        InputAcl = tdd->DefaultDacl;
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = _SEH2_GetExceptionCode();
                        _SEH2_YIELD(goto Cleanup);
                    }
                    _SEH2_END;

                    if (InputAcl != NULL)
                    {
                        PACL CapturedAcl;

                        /* Capture, validate, and copy the DACL */
                        Status = SepCaptureAcl(InputAcl,
                                               PreviousMode,
                                               PagedPool,
                                               TRUE,
                                               &CapturedAcl);
                        if (NT_SUCCESS(Status))
                        {
                            ULONG NewDynamicLength;
                            ULONG_PTR Acl;

                            /* Lock the token */
                            SepAcquireTokenLockExclusive(Token);

                            /*
                             * We can whack the token's default DACL only if
                             * the charged dynamic space boundary allows us
                             * to do so. Exceeding this boundary and we're
                             * busted out.
                             */
                            NewDynamicLength = CapturedAcl->AclSize + RtlLengthSid(Token->PrimaryGroup);
                            if (NewDynamicLength > Token->DynamicCharged)
                            {
                                SepReleaseTokenLock(Token);
                                SepReleaseAcl(CapturedAcl, PreviousMode, TRUE);
                                Status = STATUS_ALLOTTED_SPACE_EXCEEDED;
                                DPRINT1("NtSetInformationToken(): Couldn't assign new default DACL, space exceeded (current length %u, new length %lu)\n",
                                        Token->DynamicCharged, NewDynamicLength);
                                goto Cleanup;
                            }

                            /*
                             * The dynamic part of the token may require a rebuild
                             * if the current dynamic area is too small. If not then
                             * we're pretty much good as is.
                             */
                            Status = SepRebuildDynamicPartOfToken(Token, NewDynamicLength);
                            if (NT_SUCCESS(Status))
                            {
                                /*
                                 * Before setting up a new DACL for the
                                 * token object we add up the size of
                                 * the old DACL to the available dynamic
                                 * area
                                 */
                                if (Token->DefaultDacl)
                                {
                                    Token->DynamicAvailable += Token->DefaultDacl->AclSize;
                                }

                                /*
                                 * Move the primary group if it's not at the
                                 * head of the dynamic part.
                                 */
                                if ((PULONG)(Token->PrimaryGroup) != Token->DynamicPart)
                                {
                                    RtlMoveMemory(Token->DynamicPart,
                                                  Token->PrimaryGroup,
                                                  RtlLengthSid(Token->PrimaryGroup));
                                    Token->PrimaryGroup = (PSID)(Token->DynamicPart);
                                }

                                /* Take away available space from the dynamic area */
                                Token->DynamicAvailable -= CapturedAcl->AclSize;

                                /* Set the new dacl */
                                Acl = (ULONG_PTR)(Token->DynamicPart) + RtlLengthSid(Token->PrimaryGroup);
                                RtlCopyMemory((PVOID)Acl,
                                              CapturedAcl,
                                              CapturedAcl->AclSize);
                                Token->DefaultDacl = (PACL)Acl;

                                ExAllocateLocallyUniqueId(&Token->ModifiedId);
                            }

                            /* Unlock the token and release the ACL */
                            SepReleaseTokenLock(Token);
                            SepReleaseAcl(CapturedAcl, PreviousMode, TRUE);
                        }
                    }
                    else
                    {
                        /* Lock the token */
                        SepAcquireTokenLockExclusive(Token);

                        /* Clear the default dacl if present */
                        if (Token->DefaultDacl != NULL)
                        {
                            Token->DynamicAvailable += Token->DefaultDacl->AclSize;
                            RtlZeroMemory(Token->DefaultDacl, Token->DefaultDacl->AclSize);
                            Token->DefaultDacl = NULL;

                            ExAllocateLocallyUniqueId(&Token->ModifiedId);
                        }

                        /* Unlock the token */
                        SepReleaseTokenLock(Token);
                    }
                }
                else
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                break;
            }

            case TokenSessionId:
            {
                ULONG SessionId = 0;

                _SEH2_TRY
                {
                    /* Buffer size was already verified, no need to check here again */
                    SessionId = *(PULONG)TokenInformation;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                    _SEH2_YIELD(goto Cleanup);
                }
                _SEH2_END;

                /* Check for TCB privilege */
                if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
                {
                    Status = STATUS_PRIVILEGE_NOT_HELD;
                    break;
                }

                /* Lock the token */
                SepAcquireTokenLockExclusive(Token);

                Token->SessionId = SessionId;
                ExAllocateLocallyUniqueId(&Token->ModifiedId);

                /* Unlock the token */
                SepReleaseTokenLock(Token);

                break;
            }

            case TokenSessionReference:
            {
                ULONG SessionReference;

                _SEH2_TRY
                {
                    /* Buffer size was already verified, no need to check here again */
                    SessionReference = *(PULONG)TokenInformation;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                    _SEH2_YIELD(goto Cleanup);
                }
                _SEH2_END;

                /* Check for TCB privilege */
                if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
                {
                    Status = STATUS_PRIVILEGE_NOT_HELD;
                    goto Cleanup;
                }

                /* Check if it is 0 */
                if (SessionReference == 0)
                {
                    ULONG OldTokenFlags;

                    /* Lock the token */
                    SepAcquireTokenLockExclusive(Token);

                    /* Atomically set the flag in the token */
                    OldTokenFlags = RtlInterlockedSetBits(&Token->TokenFlags,
                                                          TOKEN_SESSION_NOT_REFERENCED);
                    /*
                     * If the flag was already set, do not dereference again
                     * the logon session. Use SessionReference as an indicator
                     * to know whether to really dereference the session.
                     */
                    if (OldTokenFlags == Token->TokenFlags)
                        SessionReference = ULONG_MAX;

                    /*
                     * Otherwise if the flag was never set but just for this first time then
                     * remove the referenced logon session data from the token and dereference
                     * the logon session when needed.
                     */
                    if (SessionReference == 0)
                    {
                        SepRmRemoveLogonSessionFromToken(Token);
                        SepRmDereferenceLogonSession(&Token->AuthenticationId);
                    }

                    /* Unlock the token */
                    SepReleaseTokenLock(Token);
                }
                break;
            }

            case TokenAuditPolicy:
            {
                PTOKEN_AUDIT_POLICY_INFORMATION PolicyInformation =
                    (PTOKEN_AUDIT_POLICY_INFORMATION)TokenInformation;
                SEP_AUDIT_POLICY AuditPolicy;
                ULONG i;

                _SEH2_TRY
                {
                    ProbeForRead(PolicyInformation,
                                 FIELD_OFFSET(TOKEN_AUDIT_POLICY_INFORMATION,
                                              Policies[PolicyInformation->PolicyCount]),
                                 sizeof(ULONG));

                    /* Loop all policies in the structure */
                    for (i = 0; i < PolicyInformation->PolicyCount; i++)
                    {
                        /* Set the corresponding bits in the packed structure */
                        switch (PolicyInformation->Policies[i].Category)
                        {
                            case AuditCategorySystem:
                                AuditPolicy.PolicyElements.System = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryLogon:
                                AuditPolicy.PolicyElements.Logon = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryObjectAccess:
                                AuditPolicy.PolicyElements.ObjectAccess = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryPrivilegeUse:
                                AuditPolicy.PolicyElements.PrivilegeUse = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryDetailedTracking:
                                AuditPolicy.PolicyElements.DetailedTracking = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryPolicyChange:
                                AuditPolicy.PolicyElements.PolicyChange = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryAccountManagement:
                                AuditPolicy.PolicyElements.AccountManagement = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryDirectoryServiceAccess:
                                AuditPolicy.PolicyElements.DirectoryServiceAccess = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryAccountLogon:
                                AuditPolicy.PolicyElements.AccountLogon = PolicyInformation->Policies[i].Value;
                                break;
                        }
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                    _SEH2_YIELD(goto Cleanup);
                }
                _SEH2_END;

                /* Check for TCB privilege */
                if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
                {
                    Status = STATUS_PRIVILEGE_NOT_HELD;
                    break;
                }

                /* Lock the token */
                SepAcquireTokenLockExclusive(Token);

                /* Set the new audit policy */
                Token->AuditPolicy = AuditPolicy;
                ExAllocateLocallyUniqueId(&Token->ModifiedId);

                /* Unlock the token */
                SepReleaseTokenLock(Token);

                break;
            }

            case TokenOrigin:
            {
                TOKEN_ORIGIN TokenOrigin;

                _SEH2_TRY
                {
                    /* Copy the token origin */
                    TokenOrigin = *(PTOKEN_ORIGIN)TokenInformation;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                    _SEH2_YIELD(goto Cleanup);
                }
                _SEH2_END;

                /* Check for TCB privilege */
                if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
                {
                    Status = STATUS_PRIVILEGE_NOT_HELD;
                    break;
                }

                /* Lock the token */
                SepAcquireTokenLockExclusive(Token);

                /* Check if there is no token origin set yet */
                if (RtlIsZeroLuid(&Token->OriginatingLogonSession))
                {
                    /* Set the token origin */
                    Token->OriginatingLogonSession =
                        TokenOrigin.OriginatingLogonSession;

                    ExAllocateLocallyUniqueId(&Token->ModifiedId);
                }

                /* Unlock the token */
                SepReleaseTokenLock(Token);

                break;
            }

            default:
            {
                DPRINT1("Invalid TokenInformationClass: 0x%lx\n",
                        TokenInformationClass);
                Status = STATUS_INVALID_INFO_CLASS;
                break;
            }
        }
Cleanup:
        ObDereferenceObject(Token);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetInformationToken failed with Status 0x%lx\n", Status);
    }

    return Status;
}

/* EOF */
