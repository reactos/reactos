/*
 * COPYRIGHT:       See COPYING in the top level directory
 * WINE COPYRIGHT:
 * Copyright 1999, 2000 Juergen Schmied <juergen.schmied@debitel.net>
 * Copyright 2003 CodeWeavers Inc. (Ulrich Czekalla)
 * Copyright 2006 Robert Reif
 *
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/advapi32/sec/misc.c
 * PURPOSE:         Miscellaneous security functions (some ported from Wine)
 */

#include <advapi32.h>

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

/* Interface to ntmarta.dll ***************************************************/

NTMARTA NtMartaStatic = { 0 };
static PNTMARTA NtMarta = NULL;

#define FindNtMartaProc(Name)                                                  \
    NtMartaStatic.Name = (PVOID)GetProcAddress(NtMartaStatic.hDllInstance,     \
                                               "Acc" # Name );                 \
    if (NtMartaStatic.Name == NULL)                                            \
    {                                                                          \
        return GetLastError();                                                 \
    }


static DWORD
LoadAndInitializeNtMarta(VOID)
{
    /* this code may be executed simultaneously by multiple threads in case they're
       trying to initialize the interface at the same time, but that's no problem
       because the pointers returned by GetProcAddress will be the same. However,
       only one of the threads will change the NtMarta pointer to the NtMartaStatic
       structure, the others threads will detect that there were other threads
       initializing the structure faster and will release the reference to the
       DLL */

    NtMartaStatic.hDllInstance = LoadLibraryW(L"ntmarta.dll");
    if (NtMartaStatic.hDllInstance == NULL)
    {
        return GetLastError();
    }

#if 0
    FindNtMartaProc(LookupAccountTrustee);
    FindNtMartaProc(LookupAccountName);
    FindNtMartaProc(LookupAccountSid);
    FindNtMartaProc(SetEntriesInAList);
    FindNtMartaProc(ConvertAccessToSecurityDescriptor);
    FindNtMartaProc(ConvertSDToAccess);
    FindNtMartaProc(ConvertAclToAccess);
    FindNtMartaProc(GetAccessForTrustee);
    FindNtMartaProc(GetExplicitEntries);
#endif
    FindNtMartaProc(RewriteGetNamedRights);
    FindNtMartaProc(RewriteSetNamedRights);
    FindNtMartaProc(RewriteGetHandleRights);
    FindNtMartaProc(RewriteSetHandleRights);
    FindNtMartaProc(RewriteSetEntriesInAcl);
    FindNtMartaProc(RewriteGetExplicitEntriesFromAcl);
    FindNtMartaProc(TreeResetNamedSecurityInfo);
    FindNtMartaProc(GetInheritanceSource);
    FindNtMartaProc(FreeIndexArray);

    return ERROR_SUCCESS;
}


DWORD
CheckNtMartaPresent(VOID)
{
    DWORD ErrorCode;

    if (InterlockedCompareExchangePointer((PVOID)&NtMarta,
                                          NULL,
                                          NULL) == NULL)
    {
        /* we're the first one trying to use ntmarta, initialize it and change
           the pointer after initialization */
        ErrorCode = LoadAndInitializeNtMarta();

        if (ErrorCode == ERROR_SUCCESS)
        {
            /* try change the NtMarta pointer */
            if (InterlockedCompareExchangePointer((PVOID)&NtMarta,
                                                  &NtMartaStatic,
                                                  NULL) != NULL)
            {
                /* another thread initialized ntmarta in the meanwhile, release
                   the reference of the dll loaded. */
                FreeLibrary(NtMartaStatic.hDllInstance);
            }
        }
#if DBG
        else
        {
            ERR("Failed to initialize ntmarta.dll! Error: 0x%x\n", ErrorCode);
        }
#endif
    }
    else
    {
        /* ntmarta was already initialized */
        ErrorCode = ERROR_SUCCESS;
    }

    return ErrorCode;
}


VOID
UnloadNtMarta(VOID)
{
    if (InterlockedExchangePointer((PVOID)&NtMarta,
                                   NULL) != NULL)
    {
        FreeLibrary(NtMartaStatic.hDllInstance);
    }
}


/******************************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
ImpersonateAnonymousToken(IN HANDLE ThreadHandle)
{
    NTSTATUS Status;

    Status = NtImpersonateAnonymousToken(ThreadHandle);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
ImpersonateLoggedOnUser(HANDLE hToken)
{
    SECURITY_QUALITY_OF_SERVICE Qos;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE NewToken;
    TOKEN_TYPE Type;
    ULONG ReturnLength;
    BOOL Duplicated;
    NTSTATUS Status;

    /* Get the token type */
    Status = NtQueryInformationToken(hToken,
                                     TokenType,
                                     &Type,
                                     sizeof(TOKEN_TYPE),
                                     &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    if (Type == TokenPrimary)
    {
        /* Create a duplicate impersonation token */
        Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        Qos.ImpersonationLevel = SecurityImpersonation;
        Qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        Qos.EffectiveOnly = FALSE;

        ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
        ObjectAttributes.RootDirectory = NULL;
        ObjectAttributes.ObjectName = NULL;
        ObjectAttributes.Attributes = 0;
        ObjectAttributes.SecurityDescriptor = NULL;
        ObjectAttributes.SecurityQualityOfService = &Qos;

        Status = NtDuplicateToken(hToken,
                                  TOKEN_IMPERSONATE | TOKEN_QUERY,
                                  &ObjectAttributes,
                                  FALSE,
                                  TokenImpersonation,
                                  &NewToken);
        if (!NT_SUCCESS(Status))
        {
            ERR("NtDuplicateToken failed: Status %08x\n", Status);
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }

        Duplicated = TRUE;
    }
    else
    {
        /* User the original impersonation token */
        NewToken = hToken;
        Duplicated = FALSE;
    }

    /* Impersonate the the current thread */
    Status = NtSetInformationThread(NtCurrentThread(),
                                    ThreadImpersonationToken,
                                    &NewToken,
                                    sizeof(HANDLE));

    if (Duplicated != FALSE)
    {
        NtClose(NewToken);
    }

    if (!NT_SUCCESS(Status))
    {
        ERR("NtSetInformationThread failed: Status %08x\n", Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * GetUserNameA [ADVAPI32.@]
 *
 * Get the current user name.
 *
 * PARAMS
 *  lpszName [O]   Destination for the user name.
 *  lpSize   [I/O] Size of lpszName.
 *
 *
 * @implemented
 */
BOOL
WINAPI
GetUserNameA(LPSTR lpszName,
             LPDWORD lpSize)
{
    UNICODE_STRING NameW;
    ANSI_STRING NameA;
    BOOL Ret;

    /* apparently Win doesn't check whether lpSize is valid at all! */

    NameW.MaximumLength = (*lpSize) * sizeof(WCHAR);
    NameW.Buffer = LocalAlloc(LMEM_FIXED, NameW.MaximumLength);
    if(NameW.Buffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    NameA.Length = 0;
    NameA.MaximumLength = ((*lpSize) < 0xFFFF ? (USHORT)(*lpSize) : 0xFFFF);
    NameA.Buffer = lpszName;

    Ret = GetUserNameW(NameW.Buffer,
                       lpSize);
    if(Ret)
    {
        NameW.Length = (*lpSize - 1) * sizeof(WCHAR);
        RtlUnicodeStringToAnsiString(&NameA, &NameW, FALSE);

        *lpSize = NameA.Length + 1;
    }

    LocalFree(NameW.Buffer);

    return Ret;
}

/******************************************************************************
 * GetUserNameW [ADVAPI32.@]
 *
 * See GetUserNameA.
 *
 * @implemented
 */
BOOL
WINAPI
GetUserNameW(LPWSTR lpszName,
             LPDWORD lpSize)
{
    HANDLE hToken = INVALID_HANDLE_VALUE;
    DWORD tu_len = 0;
    char* tu_buf = NULL;
    TOKEN_USER* token_user = NULL;
    DWORD an_len = 0;
    SID_NAME_USE snu = SidTypeUser;
    WCHAR* domain_name = NULL;
    DWORD dn_len = 0;

    if (!OpenThreadToken (GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken))
    {
        DWORD dwLastError = GetLastError();
        if (dwLastError != ERROR_NO_TOKEN
            && dwLastError != ERROR_NO_IMPERSONATION_TOKEN)
        {
            /* don't call SetLastError(),
               as OpenThreadToken() ought to have set one */
            return FALSE;
        }

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            /* don't call SetLastError(),
               as OpenProcessToken() ought to have set one */
            return FALSE;
        }
    }

    tu_buf = LocalAlloc(LMEM_FIXED, 36);
    if (!tu_buf)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        CloseHandle(hToken);
        return FALSE;
    }

    if (!GetTokenInformation(hToken, TokenUser, tu_buf, 36, &tu_len) || tu_len > 36)
    {
        LocalFree(tu_buf);
        tu_buf = LocalAlloc(LMEM_FIXED, tu_len);
        if (!tu_buf)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            CloseHandle(hToken);
            return FALSE;
        }

        if (!GetTokenInformation(hToken, TokenUser, tu_buf, tu_len, &tu_len))
        {
            /* don't call SetLastError(),
               as GetTokenInformation() ought to have set one */
            LocalFree(tu_buf);
            CloseHandle(hToken);
            return FALSE;
        }
    }

    CloseHandle(hToken);
    token_user = (TOKEN_USER*)tu_buf;

    an_len = *lpSize;
    dn_len = 32;
    domain_name = LocalAlloc(LMEM_FIXED, dn_len * sizeof(WCHAR));
    if (!domain_name)
    {
        LocalFree(tu_buf);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (!LookupAccountSidW(NULL, token_user->User.Sid, lpszName, &an_len, domain_name, &dn_len, &snu)
        || dn_len > 32)
    {
        if (dn_len > 32)
        {
            LocalFree(domain_name);
            domain_name = LocalAlloc(LMEM_FIXED, dn_len * sizeof(WCHAR));
            if (!domain_name)
            {
                LocalFree(tu_buf);
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
        }

        an_len = *lpSize;
        if (!LookupAccountSidW(NULL, token_user->User.Sid, lpszName, &an_len, domain_name, &dn_len, &snu))
        {
            /* don't call SetLastError(),
               as LookupAccountSid() ought to have set one */
            LocalFree(domain_name);
            LocalFree(tu_buf);
            *lpSize = an_len;
            return FALSE;
        }
    }

    LocalFree(domain_name);
    LocalFree(tu_buf);
    *lpSize = an_len + 1;
    return TRUE;
}


/******************************************************************************
 * LookupAccountSidA [ADVAPI32.@]
 *
 * @implemented
 */
BOOL
WINAPI
LookupAccountSidA(LPCSTR lpSystemName,
                  PSID lpSid,
                  LPSTR lpName,
                  LPDWORD cchName,
                  LPSTR lpReferencedDomainName,
                  LPDWORD cchReferencedDomainName,
                  PSID_NAME_USE peUse)
{
    UNICODE_STRING NameW, ReferencedDomainNameW, SystemNameW;
    LPWSTR NameBuffer = NULL;
    LPWSTR ReferencedDomainNameBuffer = NULL;
    DWORD dwName, dwReferencedDomainName;
    BOOL Ret;

    /*
     * save the buffer sizes the caller passed to us, as they may get modified and
     * we require the original values when converting back to ansi
     */
    dwName = *cchName;
    dwReferencedDomainName = *cchReferencedDomainName;

    /* allocate buffers for the unicode strings to receive */
    if (dwName > 0)
    {
        NameBuffer = LocalAlloc(LMEM_FIXED, dwName * sizeof(WCHAR));
        if (NameBuffer == NULL)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
    }
    else
        NameBuffer = NULL;

    if (dwReferencedDomainName > 0)
    {
        ReferencedDomainNameBuffer = LocalAlloc(LMEM_FIXED, dwReferencedDomainName * sizeof(WCHAR));
        if (ReferencedDomainNameBuffer == NULL)
        {
            if (dwName > 0)
            {
                LocalFree(NameBuffer);
            }

            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
    }
    else
        ReferencedDomainNameBuffer = NULL;


    /* convert the system name to unicode - if present */
    if (lpSystemName != NULL)
    {
        ANSI_STRING SystemNameA;

        RtlInitAnsiString(&SystemNameA, lpSystemName);
        RtlAnsiStringToUnicodeString(&SystemNameW, &SystemNameA, TRUE);
    }
    else
        SystemNameW.Buffer = NULL;

    /* it's time to call the unicode version */
    Ret = LookupAccountSidW(SystemNameW.Buffer,
                            lpSid,
                            NameBuffer,
                            cchName,
                            ReferencedDomainNameBuffer,
                            cchReferencedDomainName,
                            peUse);
    if (Ret)
    {
        /*
         * convert unicode strings back to ansi, don't forget that we can't convert
         * more than 0xFFFF (USHORT) characters! Also don't forget to explicitly
         * terminate the converted string, the Rtl functions don't do that!
         */
        if (lpName != NULL)
        {
            ANSI_STRING NameA;

            NameA.Length = 0;
            NameA.MaximumLength = ((dwName <= 0xFFFF) ? (USHORT)dwName : 0xFFFF);
            NameA.Buffer = lpName;

            RtlInitUnicodeString(&NameW, NameBuffer);
            RtlUnicodeStringToAnsiString(&NameA, &NameW, FALSE);
            NameA.Buffer[NameA.Length] = '\0';
        }

        if (lpReferencedDomainName != NULL)
        {
            ANSI_STRING ReferencedDomainNameA;

            ReferencedDomainNameA.Length = 0;
            ReferencedDomainNameA.MaximumLength = ((dwReferencedDomainName <= 0xFFFF) ?
                                                   (USHORT)dwReferencedDomainName : 0xFFFF);
            ReferencedDomainNameA.Buffer = lpReferencedDomainName;

            RtlInitUnicodeString(&ReferencedDomainNameW, ReferencedDomainNameBuffer);
            RtlUnicodeStringToAnsiString(&ReferencedDomainNameA, &ReferencedDomainNameW, FALSE);
            ReferencedDomainNameA.Buffer[ReferencedDomainNameA.Length] = '\0';
        }
    }

    /* free previously allocated buffers */
    if (SystemNameW.Buffer != NULL)
    {
        RtlFreeUnicodeString(&SystemNameW);
    }

    if (NameBuffer != NULL)
    {
        LocalFree(NameBuffer);
    }

    if (ReferencedDomainNameBuffer != NULL)
    {
        LocalFree(ReferencedDomainNameBuffer);
    }

    return Ret;
}


/******************************************************************************
 * LookupAccountSidW [ADVAPI32.@]
 *
 * @implemented
 */
BOOL WINAPI
LookupAccountSidW(LPCWSTR pSystemName,
                  PSID pSid,
                  LPWSTR pAccountName,
                  LPDWORD pdwAccountName,
                  LPWSTR pDomainName,
                  LPDWORD pdwDomainName,
                  PSID_NAME_USE peUse)
{
    LSA_UNICODE_STRING SystemName;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes = {0};
    LSA_HANDLE PolicyHandle = NULL;
    NTSTATUS Status;
    PLSA_REFERENCED_DOMAIN_LIST ReferencedDomain = NULL;
    PLSA_TRANSLATED_NAME TranslatedName = NULL;
    BOOL ret;
    DWORD dwAccountName, dwDomainName;

    RtlInitUnicodeString(&SystemName, pSystemName);
    Status = LsaOpenPolicy(&SystemName, &ObjectAttributes, POLICY_LOOKUP_NAMES, &PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(LsaNtStatusToWinError(Status));
        return FALSE;
    }

    Status = LsaLookupSids(PolicyHandle, 1, &pSid, &ReferencedDomain, &TranslatedName);

    LsaClose(PolicyHandle);

    if (!NT_SUCCESS(Status) || Status == STATUS_SOME_NOT_MAPPED)
    {
        SetLastError(LsaNtStatusToWinError(Status));
        ret = FALSE;
    }
    else
    {
        ret = TRUE;

        dwAccountName = TranslatedName->Name.Length / sizeof(WCHAR);
        if (ReferencedDomain && ReferencedDomain->Entries > 0)
            dwDomainName = ReferencedDomain->Domains[0].Name.Length / sizeof(WCHAR);
        else
            dwDomainName = 0;

        if (*pdwAccountName <= dwAccountName || *pdwDomainName <= dwDomainName)
        {
            /* One or two buffers are insufficient, add up a char for NULL termination */
            *pdwAccountName = dwAccountName + 1;
            *pdwDomainName = dwDomainName + 1;
            ret = FALSE;
        }
        else
        {
            /* Lengths are sufficient, copy the data */
            if (dwAccountName)
                RtlCopyMemory(pAccountName, TranslatedName->Name.Buffer, dwAccountName * sizeof(WCHAR));
            pAccountName[dwAccountName] = L'\0';

            if (dwDomainName)
                RtlCopyMemory(pDomainName, ReferencedDomain->Domains[0].Name.Buffer, dwDomainName * sizeof(WCHAR));
            pDomainName[dwDomainName] = L'\0';

            *pdwAccountName = dwAccountName;
            *pdwDomainName = dwDomainName;

            if (peUse)
                *peUse = TranslatedName->Use;
        }

        if (!ret)
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

    if (ReferencedDomain)
        LsaFreeMemory(ReferencedDomain);

    if (TranslatedName)
        LsaFreeMemory(TranslatedName);

    return ret;
}

/******************************************************************************
 * LookupAccountNameW [ADVAPI32.@]
 *
 * @implemented
 */
BOOL
WINAPI
LookupAccountNameW(LPCWSTR lpSystemName,
                   LPCWSTR lpAccountName,
                   PSID Sid,
                   LPDWORD cbSid,
                   LPWSTR ReferencedDomainName,
                   LPDWORD cchReferencedDomainName,
                   PSID_NAME_USE peUse)
{
    OBJECT_ATTRIBUTES ObjectAttributes = {0};
    UNICODE_STRING SystemName;
    UNICODE_STRING AccountName;
    LSA_HANDLE PolicyHandle = NULL;
    PLSA_REFERENCED_DOMAIN_LIST ReferencedDomains = NULL;
    PLSA_TRANSLATED_SID TranslatedSid = NULL;
    PSID pDomainSid;
    DWORD dwDomainNameLength;
    DWORD dwSidLength;
    UCHAR nSubAuthorities;
    BOOL bResult;
    NTSTATUS Status;

    TRACE("%s %s %p %p %p %p %p\n", lpSystemName, lpAccountName,
          Sid, cbSid, ReferencedDomainName, cchReferencedDomainName, peUse);

    RtlInitUnicodeString(&SystemName,
                         lpSystemName);

    Status = LsaOpenPolicy(lpSystemName ? &SystemName : NULL,
                           &ObjectAttributes,
                           POLICY_LOOKUP_NAMES,
                           &PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(LsaNtStatusToWinError(Status));
        return FALSE;
    }

    RtlInitUnicodeString(&AccountName,
                         lpAccountName);

    Status = LsaLookupNames(PolicyHandle,
                            1,
                            &AccountName,
                            &ReferencedDomains,
                            &TranslatedSid);

    LsaClose(PolicyHandle);

    if (!NT_SUCCESS(Status) || Status == STATUS_SOME_NOT_MAPPED)
    {
        SetLastError(LsaNtStatusToWinError(Status));
        bResult = FALSE;
    }
    else
    {
        pDomainSid = ReferencedDomains->Domains[TranslatedSid->DomainIndex].Sid;
        nSubAuthorities = *GetSidSubAuthorityCount(pDomainSid);
        dwSidLength = GetSidLengthRequired(nSubAuthorities + 1);

        dwDomainNameLength = ReferencedDomains->Domains->Name.Length / sizeof(WCHAR);

        if (*cbSid < dwSidLength ||
            *cchReferencedDomainName < dwDomainNameLength + 1)
        {
            *cbSid = dwSidLength;
            *cchReferencedDomainName = dwDomainNameLength + 1;

            bResult = FALSE;
        }
        else
        {
            CopySid(*cbSid, Sid, pDomainSid);
            *GetSidSubAuthorityCount(Sid) = nSubAuthorities + 1;
            *GetSidSubAuthority(Sid, (DWORD)nSubAuthorities) = TranslatedSid->RelativeId;

            RtlCopyMemory(ReferencedDomainName, ReferencedDomains->Domains->Name.Buffer, dwDomainNameLength * sizeof(WCHAR));
            ReferencedDomainName[dwDomainNameLength] = L'\0';

            *cchReferencedDomainName = dwDomainNameLength;

            *peUse = TranslatedSid->Use;

            bResult = TRUE;
        }

        if (bResult == FALSE)
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

    if (ReferencedDomains != NULL)
        LsaFreeMemory(ReferencedDomains);

    if (TranslatedSid != NULL)
        LsaFreeMemory(TranslatedSid);

    return bResult;
}


/**********************************************************************
 * LookupPrivilegeValueA				EXPORTED
 *
 * @implemented
 */
BOOL
WINAPI
LookupPrivilegeValueA(LPCSTR lpSystemName,
                      LPCSTR lpName,
                      PLUID lpLuid)
{
    UNICODE_STRING SystemName;
    UNICODE_STRING Name;
    BOOL Result;

    /* Remote system? */
    if (lpSystemName != NULL)
    {
        RtlCreateUnicodeStringFromAsciiz(&SystemName,
                                         (LPSTR)lpSystemName);
    }
    else
        SystemName.Buffer = NULL;

    /* Check the privilege name is not NULL */
    if (lpName == NULL)
    {
        SetLastError(ERROR_NO_SUCH_PRIVILEGE);
        return FALSE;
    }

    RtlCreateUnicodeStringFromAsciiz(&Name,
                                     (LPSTR)lpName);

    Result = LookupPrivilegeValueW(SystemName.Buffer,
                                   Name.Buffer,
                                   lpLuid);

    RtlFreeUnicodeString(&Name);

    /* Remote system? */
    if (SystemName.Buffer != NULL)
    {
        RtlFreeUnicodeString(&SystemName);
    }

    return Result;
}


/**********************************************************************
 * LookupPrivilegeValueW
 *
 * @implemented
 */
BOOL
WINAPI
LookupPrivilegeValueW(LPCWSTR lpSystemName,
                      LPCWSTR lpPrivilegeName,
                      PLUID lpLuid)
{
    OBJECT_ATTRIBUTES ObjectAttributes = {0};
    UNICODE_STRING SystemName;
    UNICODE_STRING PrivilegeName;
    LSA_HANDLE PolicyHandle = NULL;
    NTSTATUS Status;

    TRACE("%S,%S,%p\n", lpSystemName, lpPrivilegeName, lpLuid);

    RtlInitUnicodeString(&SystemName,
                         lpSystemName);

    Status = LsaOpenPolicy(lpSystemName ? &SystemName : NULL,
                           &ObjectAttributes,
                           POLICY_LOOKUP_NAMES,
                           &PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(LsaNtStatusToWinError(Status));
        return FALSE;
    }

    RtlInitUnicodeString(&PrivilegeName,
                         lpPrivilegeName);

    Status = LsaLookupPrivilegeValue(PolicyHandle,
                                     &PrivilegeName,
                                     lpLuid);

    LsaClose(PolicyHandle);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(LsaNtStatusToWinError(Status));
        return FALSE;
    }

    return TRUE;
}

/**********************************************************************
 * LookupPrivilegeNameW				EXPORTED
 *
 * @implemented
 */
BOOL
WINAPI
LookupPrivilegeNameW(LPCWSTR lpSystemName,
                     PLUID lpLuid,
                     LPWSTR lpName,
                     LPDWORD cchName)
{
    OBJECT_ATTRIBUTES ObjectAttributes = {0};
    UNICODE_STRING SystemName;
    PUNICODE_STRING PrivilegeName = NULL;
    LSA_HANDLE PolicyHandle = NULL;
    NTSTATUS Status;

    TRACE("%S,%p,%p,%p\n", lpSystemName, lpLuid, lpName, cchName);

    RtlInitUnicodeString(&SystemName,
                         lpSystemName);

    Status = LsaOpenPolicy(lpSystemName ? &SystemName : NULL,
                           &ObjectAttributes,
                           POLICY_LOOKUP_NAMES,
                           &PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(LsaNtStatusToWinError(Status));
        return FALSE;
    }

    Status = LsaLookupPrivilegeName(PolicyHandle,
                                    lpLuid,
                                    &PrivilegeName);
    if (NT_SUCCESS(Status))
    {
        if (PrivilegeName->Length + sizeof(WCHAR) > *cchName * sizeof(WCHAR))
        {
            Status = STATUS_BUFFER_TOO_SMALL;

            *cchName = (PrivilegeName->Length + sizeof(WCHAR)) / sizeof(WCHAR);
        }
        else
        {
            RtlMoveMemory(lpName,
                          PrivilegeName->Buffer,
                          PrivilegeName->Length);
            lpName[PrivilegeName->Length / sizeof(WCHAR)] = 0;

            *cchName = PrivilegeName->Length / sizeof(WCHAR);
        }

        LsaFreeMemory(PrivilegeName->Buffer);
        LsaFreeMemory(PrivilegeName);
    }

    LsaClose(PolicyHandle);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(LsaNtStatusToWinError(Status));
        return FALSE;
    }

    return TRUE;
}

/**********************************************************************
 * LookupPrivilegeDisplayNameW			EXPORTED
 *
 * @unimplemented
 */
BOOL
WINAPI
LookupPrivilegeDisplayNameW(LPCWSTR lpSystemName,
                            LPCWSTR lpName,
                            LPWSTR lpDisplayName,
                            LPDWORD cchDisplayName,
                            LPDWORD lpLanguageId)
{
    OBJECT_ATTRIBUTES ObjectAttributes = {0};
    UNICODE_STRING SystemName, Name;
    PUNICODE_STRING DisplayName;
    LSA_HANDLE PolicyHandle = NULL;
    USHORT LanguageId;
    NTSTATUS Status;

    TRACE("%S,%S,%p,%p,%p\n", lpSystemName, lpName, lpDisplayName, cchDisplayName, lpLanguageId);

    RtlInitUnicodeString(&SystemName, lpSystemName);
    RtlInitUnicodeString(&Name, lpName);

    Status = LsaOpenPolicy(lpSystemName ? &SystemName : NULL,
                           &ObjectAttributes,
                           POLICY_LOOKUP_NAMES,
                           &PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(LsaNtStatusToWinError(Status));
        return FALSE;
    }

    Status = LsaLookupPrivilegeDisplayName(PolicyHandle, &Name, &DisplayName, &LanguageId);
    if (NT_SUCCESS(Status))
    {
        *lpLanguageId = LanguageId;
        if (DisplayName->Length + sizeof(WCHAR) > *cchDisplayName * sizeof(WCHAR))
        {
            Status = STATUS_BUFFER_TOO_SMALL;

            *cchDisplayName = (DisplayName->Length + sizeof(WCHAR)) / sizeof(WCHAR);
        }
        else
        {
            RtlMoveMemory(lpDisplayName,
                          DisplayName->Buffer,
                          DisplayName->Length);
            lpDisplayName[DisplayName->Length / sizeof(WCHAR)] = 0;

            *cchDisplayName = DisplayName->Length / sizeof(WCHAR);
        }

        LsaFreeMemory(DisplayName->Buffer);
        LsaFreeMemory(DisplayName);
    }

    LsaClose(PolicyHandle);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(LsaNtStatusToWinError(Status));
        return FALSE;
    }

    return TRUE;
}

static DWORD
pGetSecurityInfoCheck(SECURITY_INFORMATION SecurityInfo,
                      PSID *ppsidOwner,
                      PSID *ppsidGroup,
                      PACL *ppDacl,
                      PACL *ppSacl,
                      PSECURITY_DESCRIPTOR* ppSecurityDescriptor)
{
    if ((SecurityInfo & (OWNER_SECURITY_INFORMATION |
                         GROUP_SECURITY_INFORMATION |
                         DACL_SECURITY_INFORMATION |
                         SACL_SECURITY_INFORMATION)) &&
        ppSecurityDescriptor == NULL)
    {
        /* if one of the SIDs or ACLs are present, the security descriptor
           most not be NULL */
        return ERROR_INVALID_PARAMETER;
    }
    else
    {
        /* reset the pointers unless they're ignored */
        if ((SecurityInfo & OWNER_SECURITY_INFORMATION) &&
            ppsidOwner != NULL)
        {
            *ppsidOwner = NULL;
        }
        if ((SecurityInfo & GROUP_SECURITY_INFORMATION) &&
            ppsidGroup != NULL)
        {
            *ppsidGroup = NULL;
        }
        if ((SecurityInfo & DACL_SECURITY_INFORMATION) &&
            ppDacl != NULL)
        {
            *ppDacl = NULL;
        }
        if ((SecurityInfo & SACL_SECURITY_INFORMATION) &&
            ppSacl != NULL)
        {
            *ppSacl = NULL;
        }

        if (SecurityInfo & (OWNER_SECURITY_INFORMATION |
                            GROUP_SECURITY_INFORMATION |
                            DACL_SECURITY_INFORMATION |
                            SACL_SECURITY_INFORMATION))
        {
            *ppSecurityDescriptor = NULL;
        }

        return ERROR_SUCCESS;
    }
}


static DWORD
pSetSecurityInfoCheck(PSECURITY_DESCRIPTOR pSecurityDescriptor,
                      SECURITY_INFORMATION SecurityInfo,
                      PSID psidOwner,
                      PSID psidGroup,
                      PACL pDacl,
                      PACL pSacl)
{
    /* initialize a security descriptor on the stack */
    if (!InitializeSecurityDescriptor(pSecurityDescriptor,
                                      SECURITY_DESCRIPTOR_REVISION))
    {
        return GetLastError();
    }

    if (SecurityInfo & OWNER_SECURITY_INFORMATION)
    {
        if (RtlValidSid(psidOwner))
        {
            if (!SetSecurityDescriptorOwner(pSecurityDescriptor,
                                            psidOwner,
                                            FALSE))
            {
                return GetLastError();
            }
        }
        else
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    if (SecurityInfo & GROUP_SECURITY_INFORMATION)
    {
        if (RtlValidSid(psidGroup))
        {
            if (!SetSecurityDescriptorGroup(pSecurityDescriptor,
                                            psidGroup,
                                            FALSE))
            {
                return GetLastError();
            }
        }
        else
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    if (SecurityInfo & DACL_SECURITY_INFORMATION)
    {
        if (pDacl != NULL)
        {
            if (SetSecurityDescriptorDacl(pSecurityDescriptor,
                                          TRUE,
                                          pDacl,
                                          FALSE))
            {
                /* check if the DACL needs to be protected from being
                   modified by inheritable ACEs */
                if (SecurityInfo & PROTECTED_DACL_SECURITY_INFORMATION)
                {
                    goto ProtectDacl;
                }
            }
            else
            {
                return GetLastError();
            }
        }
        else
        {
ProtectDacl:
            /* protect the DACL from being modified by inheritable ACEs */
            if (!SetSecurityDescriptorControl(pSecurityDescriptor,
                                              SE_DACL_PROTECTED,
                                              SE_DACL_PROTECTED))
            {
                return GetLastError();
            }
        }
    }

    if (SecurityInfo & SACL_SECURITY_INFORMATION)
    {
        if (pSacl != NULL)
        {
            if (SetSecurityDescriptorSacl(pSecurityDescriptor,
                                          TRUE,
                                          pSacl,
                                          FALSE))
            {
                /* check if the SACL needs to be protected from being
                   modified by inheritable ACEs */
                if (SecurityInfo & PROTECTED_SACL_SECURITY_INFORMATION)
                {
                    goto ProtectSacl;
                }
            }
            else
            {
                return GetLastError();
            }
        }
        else
        {
ProtectSacl:
            /* protect the SACL from being modified by inheritable ACEs */
            if (!SetSecurityDescriptorControl(pSecurityDescriptor,
                                              SE_SACL_PROTECTED,
                                              SE_SACL_PROTECTED))
            {
                return GetLastError();
            }
        }
    }

    return ERROR_SUCCESS;
}


/**********************************************************************
 * GetNamedSecurityInfoW			EXPORTED
 *
 * @implemented
 */
DWORD
WINAPI
GetNamedSecurityInfoW(LPWSTR pObjectName,
                      SE_OBJECT_TYPE ObjectType,
                      SECURITY_INFORMATION SecurityInfo,
                      PSID *ppsidOwner,
                      PSID *ppsidGroup,
                      PACL *ppDacl,
                      PACL *ppSacl,
                      PSECURITY_DESCRIPTOR *ppSecurityDescriptor)
{
    DWORD ErrorCode;

    if (pObjectName != NULL)
    {
        ErrorCode = CheckNtMartaPresent();
        if (ErrorCode == ERROR_SUCCESS)
        {
            ErrorCode = pGetSecurityInfoCheck(SecurityInfo,
                                              ppsidOwner,
                                              ppsidGroup,
                                              ppDacl,
                                              ppSacl,
                                              ppSecurityDescriptor);

            if (ErrorCode == ERROR_SUCCESS)
            {
                /* call the MARTA provider */
                ErrorCode = AccRewriteGetNamedRights(pObjectName,
                                                     ObjectType,
                                                     SecurityInfo,
                                                     ppsidOwner,
                                                     ppsidGroup,
                                                     ppDacl,
                                                     ppSacl,
                                                     ppSecurityDescriptor);
            }
        }
    }
    else
        ErrorCode = ERROR_INVALID_PARAMETER;

    return ErrorCode;
}

/**********************************************************************
 * SetNamedSecurityInfoW			EXPORTED
 *
 * @implemented
 */
DWORD
WINAPI
SetNamedSecurityInfoW(LPWSTR pObjectName,
                      SE_OBJECT_TYPE ObjectType,
                      SECURITY_INFORMATION SecurityInfo,
                      PSID psidOwner,
                      PSID psidGroup,
                      PACL pDacl,
                      PACL pSacl)
{
    DWORD ErrorCode;

    if (pObjectName != NULL)
    {
        ErrorCode = CheckNtMartaPresent();
        if (ErrorCode == ERROR_SUCCESS)
        {
            SECURITY_DESCRIPTOR SecurityDescriptor;

            ErrorCode = pSetSecurityInfoCheck(&SecurityDescriptor,
                                              SecurityInfo,
                                              psidOwner,
                                              psidGroup,
                                              pDacl,
                                              pSacl);

            if (ErrorCode == ERROR_SUCCESS)
            {
                /* call the MARTA provider */
                ErrorCode = AccRewriteSetNamedRights(pObjectName,
                                                     ObjectType,
                                                     SecurityInfo,
                                                     &SecurityDescriptor);
            }
        }
    }
    else
        ErrorCode = ERROR_INVALID_PARAMETER;

    return ErrorCode;
}

/**********************************************************************
 * GetSecurityInfo				EXPORTED
 *
 * @implemented
 */
DWORD
WINAPI
GetSecurityInfo(HANDLE handle,
                SE_OBJECT_TYPE ObjectType,
                SECURITY_INFORMATION SecurityInfo,
                PSID *ppsidOwner,
                PSID *ppsidGroup,
                PACL *ppDacl,
                PACL *ppSacl,
                PSECURITY_DESCRIPTOR *ppSecurityDescriptor)
{
    DWORD ErrorCode;

    if (handle != NULL)
    {
        ErrorCode = CheckNtMartaPresent();
        if (ErrorCode == ERROR_SUCCESS)
        {
            ErrorCode = pGetSecurityInfoCheck(SecurityInfo,
                                              ppsidOwner,
                                              ppsidGroup,
                                              ppDacl,
                                              ppSacl,
                                              ppSecurityDescriptor);

            if (ErrorCode == ERROR_SUCCESS)
            {
                /* call the MARTA provider */
                ErrorCode = AccRewriteGetHandleRights(handle,
                                                      ObjectType,
                                                      SecurityInfo,
                                                      ppsidOwner,
                                                      ppsidGroup,
                                                      ppDacl,
                                                      ppSacl,
                                                      ppSecurityDescriptor);
            }
        }
    }
    else
        ErrorCode = ERROR_INVALID_HANDLE;

    return ErrorCode;
}


/**********************************************************************
 * SetSecurityInfo				EXPORTED
 *
 * @implemented
 */
DWORD
WINAPI
SetSecurityInfo(HANDLE handle,
                SE_OBJECT_TYPE ObjectType,
                SECURITY_INFORMATION SecurityInfo,
                PSID psidOwner,
                PSID psidGroup,
                PACL pDacl,
                PACL pSacl)
{
    DWORD ErrorCode;

    if (handle != NULL)
    {
        ErrorCode = CheckNtMartaPresent();
        if (ErrorCode == ERROR_SUCCESS)
        {
            SECURITY_DESCRIPTOR SecurityDescriptor;

            ErrorCode = pSetSecurityInfoCheck(&SecurityDescriptor,
                                              SecurityInfo,
                                              psidOwner,
                                              psidGroup,
                                              pDacl,
                                              pSacl);

            if (ErrorCode == ERROR_SUCCESS)
            {
                /* call the MARTA provider */
                ErrorCode = AccRewriteSetHandleRights(handle,
                                                      ObjectType,
                                                      SecurityInfo,
                                                      &SecurityDescriptor);
            }
        }
    }
    else
        ErrorCode = ERROR_INVALID_HANDLE;

    return ErrorCode;
}

/*
 * @implemented
 */
BOOL
WINAPI
CreatePrivateObjectSecurity(PSECURITY_DESCRIPTOR ParentDescriptor,
                            PSECURITY_DESCRIPTOR CreatorDescriptor,
                            PSECURITY_DESCRIPTOR *NewDescriptor,
                            BOOL IsDirectoryObject,
                            HANDLE Token,
                            PGENERIC_MAPPING GenericMapping)
{
    NTSTATUS Status;

    Status = RtlNewSecurityObject(ParentDescriptor,
                                  CreatorDescriptor,
                                  NewDescriptor,
                                  IsDirectoryObject,
                                  Token,
                                  GenericMapping);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
CreatePrivateObjectSecurityEx(PSECURITY_DESCRIPTOR ParentDescriptor,
                              PSECURITY_DESCRIPTOR CreatorDescriptor,
                              PSECURITY_DESCRIPTOR* NewDescriptor,
                              GUID* ObjectType,
                              BOOL IsContainerObject,
                              ULONG AutoInheritFlags,
                              HANDLE Token,
                              PGENERIC_MAPPING GenericMapping)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
CreatePrivateObjectSecurityWithMultipleInheritance(PSECURITY_DESCRIPTOR ParentDescriptor,
                                                   PSECURITY_DESCRIPTOR CreatorDescriptor,
                                                   PSECURITY_DESCRIPTOR* NewDescriptor,
                                                   GUID** ObjectTypes,
                                                   ULONG GuidCount,
                                                   BOOL IsContainerObject,
                                                   ULONG AutoInheritFlags,
                                                   HANDLE Token,
                                                   PGENERIC_MAPPING GenericMapping)
{
    FIXME("%s() semi-stub\n", __FUNCTION__);
    return CreatePrivateObjectSecurity(ParentDescriptor, CreatorDescriptor, NewDescriptor, IsContainerObject, Token, GenericMapping);
}


/*
 * @implemented
 */
BOOL
WINAPI
DestroyPrivateObjectSecurity(PSECURITY_DESCRIPTOR *ObjectDescriptor)
{
    NTSTATUS Status;

    Status = RtlDeleteSecurityObject(ObjectDescriptor);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetPrivateObjectSecurity(IN PSECURITY_DESCRIPTOR ObjectDescriptor,
                         IN SECURITY_INFORMATION SecurityInformation,
                         OUT PSECURITY_DESCRIPTOR ResultantDescriptor OPTIONAL,
                         IN DWORD DescriptorLength,
                         OUT PDWORD ReturnLength)
{
    NTSTATUS Status;

    /* Call RTL */
    Status = RtlQuerySecurityObject(ObjectDescriptor,
                                    SecurityInformation,
                                    ResultantDescriptor,
                                    DescriptorLength,
                                    ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /* Success */
    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetPrivateObjectSecurity(SECURITY_INFORMATION SecurityInformation,
                         PSECURITY_DESCRIPTOR ModificationDescriptor,
                         PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
                         PGENERIC_MAPPING GenericMapping,
                         HANDLE Token)
{
    NTSTATUS Status;

    Status = RtlSetSecurityObject(SecurityInformation,
                                  ModificationDescriptor,
                                  ObjectsSecurityDescriptor,
                                  GenericMapping,
                                  Token);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
DWORD
WINAPI
TreeResetNamedSecurityInfoW(LPWSTR pObjectName,
                            SE_OBJECT_TYPE ObjectType,
                            SECURITY_INFORMATION SecurityInfo,
                            PSID pOwner,
                            PSID pGroup,
                            PACL pDacl,
                            PACL pSacl,
                            BOOL KeepExplicit,
                            FN_PROGRESSW fnProgress,
                            PROG_INVOKE_SETTING ProgressInvokeSetting,
                            PVOID Args)
{
    DWORD ErrorCode;

    if (pObjectName != NULL)
    {
        ErrorCode = CheckNtMartaPresent();
        if (ErrorCode == ERROR_SUCCESS)
        {
            switch (ObjectType)
            {
                case SE_FILE_OBJECT:
                case SE_REGISTRY_KEY:
                {
                    /* check the SecurityInfo flags for sanity (both, the protected
                       and unprotected dacl/sacl flag must not be passed together) */
                    if (((SecurityInfo & DACL_SECURITY_INFORMATION) &&
                         (SecurityInfo & (PROTECTED_DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION)) ==
                             (PROTECTED_DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION))

                        ||

                        ((SecurityInfo & SACL_SECURITY_INFORMATION) &&
                         (SecurityInfo & (PROTECTED_SACL_SECURITY_INFORMATION | UNPROTECTED_SACL_SECURITY_INFORMATION)) ==
                             (PROTECTED_SACL_SECURITY_INFORMATION | UNPROTECTED_SACL_SECURITY_INFORMATION)))
                    {
                        ErrorCode = ERROR_INVALID_PARAMETER;
                        break;
                    }

                    /* call the MARTA provider */
                    ErrorCode = AccTreeResetNamedSecurityInfo(pObjectName,
                                                              ObjectType,
                                                              SecurityInfo,
                                                              pOwner,
                                                              pGroup,
                                                              pDacl,
                                                              pSacl,
                                                              KeepExplicit,
                                                              fnProgress,
                                                              ProgressInvokeSetting,
                                                              Args);
                    break;
                }

                default:
                    /* object type not supported */
                    ErrorCode = ERROR_INVALID_PARAMETER;
                    break;
            }
        }
    }
    else
        ErrorCode = ERROR_INVALID_PARAMETER;

    return ErrorCode;
}

#ifdef HAS_FN_PROGRESSW

typedef struct _INTERNAL_FNPROGRESSW_DATA
{
    FN_PROGRESSA fnProgress;
    PVOID Args;
} INTERNAL_FNPROGRESSW_DATA, *PINTERNAL_FNPROGRESSW_DATA;

static VOID WINAPI
InternalfnProgressW(LPWSTR pObjectName,
                    DWORD Status,
                    PPROG_INVOKE_SETTING pInvokeSetting,
                    PVOID Args,
                    BOOL SecuritySet)
{
    PINTERNAL_FNPROGRESSW_DATA pifnProgressData = (PINTERNAL_FNPROGRESSW_DATA)Args;
    INT ObjectNameSize;
    LPSTR pObjectNameA;

    ObjectNameSize = WideCharToMultiByte(CP_ACP,
                                         0,
                                         pObjectName,
                                         -1,
                                         NULL,
                                         0,
                                         NULL,
                                         NULL);

    if (ObjectNameSize > 0)
    {
        pObjectNameA = RtlAllocateHeap(RtlGetProcessHeap(),
                                       0,
                                       ObjectNameSize);
        if (pObjectNameA != NULL)
        {
            pObjectNameA[0] = '\0';
            WideCharToMultiByte(CP_ACP,
                                0,
                                pObjectName,
                                -1,
                                pObjectNameA,
                                ObjectNameSize,
                                NULL,
                                NULL);

            pifnProgressData->fnProgress((LPWSTR)pObjectNameA, /* FIXME: wrong cast!! */
                                         Status,
                                         pInvokeSetting,
                                         pifnProgressData->Args,
                                         SecuritySet);

            RtlFreeHeap(RtlGetProcessHeap(),
                        0,
                        pObjectNameA);
        }
    }
}
#endif


/*
 * @implemented
 */
DWORD
WINAPI
TreeResetNamedSecurityInfoA(LPSTR pObjectName,
                            SE_OBJECT_TYPE ObjectType,
                            SECURITY_INFORMATION SecurityInfo,
                            PSID pOwner,
                            PSID pGroup,
                            PACL pDacl,
                            PACL pSacl,
                            BOOL KeepExplicit,
                            FN_PROGRESSA fnProgress,
                            PROG_INVOKE_SETTING ProgressInvokeSetting,
                            PVOID Args)
{
#ifndef HAS_FN_PROGRESSW
    /* That's all this function does, at least up to w2k3... Even MS was too
       lazy to implement it... */
    return ERROR_CALL_NOT_IMPLEMENTED;
#else
    INTERNAL_FNPROGRESSW_DATA ifnProgressData;
    UNICODE_STRING ObjectName;
    DWORD Ret;

    if (!RtlCreateUnicodeStringFromAsciiz(&ObjectName, pObjectName))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ifnProgressData.fnProgress = fnProgress;
    ifnProgressData.Args = Args;

    Ret = TreeResetNamedSecurityInfoW(ObjectName.Buffer,
                                      ObjectType,
                                      SecurityInfo,
                                      pOwner,
                                      pGroup,
                                      pDacl,
                                      pSacl,
                                      KeepExplicit,
                                      (fnProgress != NULL ? InternalfnProgressW : NULL),
                                      ProgressInvokeSetting,
                                      &ifnProgressData);

    RtlFreeUnicodeString(&ObjectName);

    return Ret;
#endif
}

/* EOF */
