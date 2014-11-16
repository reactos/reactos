/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:
 * PURPOSE:
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "net.h"


static
int
CompareInfo(const void *a,
            const void *b)
{
    return _wcsicmp(((PUSER_INFO_0)a)->usri0_name,
                    ((PUSER_INFO_0)b)->usri0_name);
}


static
NET_API_STATUS
EnumerateUsers(VOID)
{
    PUSER_INFO_0 pBuffer = NULL;
    PSERVER_INFO_100 pServer = NULL;
    DWORD dwRead = 0, dwTotal = 0;
    DWORD i;
    DWORD_PTR ResumeHandle = 0;
    NET_API_STATUS Status;

    Status = NetServerGetInfo(NULL,
                              100,
                              (LPBYTE*)&pServer);
    if (Status != NERR_Success)
        return Status;

    PrintToConsole(L"\n");
    PrintResourceString(IDS_USER_ACCOUNTS, pServer->sv100_name);
    PrintToConsole(L"\n\n");
    PrintPadding(L'-', 79);
    PrintToConsole(L"\n");

    NetApiBufferFree(pServer);

    Status = NetUserEnum(NULL,
                         0,
                         0,
                         (LPBYTE*)&pBuffer,
                         MAX_PREFERRED_LENGTH,
                         &dwRead,
                         &dwTotal,
                         &ResumeHandle);
    if (Status != NERR_Success)
        return Status;

    qsort(pBuffer,
          dwRead,
          sizeof(PUSER_INFO_0),
          CompareInfo);

//    printf("dwRead: %lu  dwTotal: %lu\n", dwRead, dwTotal);
    for (i = 0; i < dwRead; i++)
    {
//        printf("%p\n", pBuffer[i].lgrpi0_name);
         if (pBuffer[i].usri0_name)
            PrintToConsole(L"%s\n", pBuffer[i].usri0_name);
    }

    NetApiBufferFree(pBuffer);

    return NERR_Success;
}


static
VOID
PrintDateTime(DWORD dwSeconds)
{
    LARGE_INTEGER Time;
    FILETIME FileTime;
    SYSTEMTIME SystemTime;
    WCHAR DateBuffer[80];
    WCHAR TimeBuffer[80];

    RtlSecondsSince1970ToTime(dwSeconds, &Time);
    FileTime.dwLowDateTime = Time.u.LowPart;
    FileTime.dwHighDateTime = Time.u.HighPart;
    FileTimeToLocalFileTime(&FileTime, &FileTime);
    FileTimeToSystemTime(&FileTime, &SystemTime);

    GetDateFormatW(LOCALE_USER_DEFAULT,
                   DATE_SHORTDATE,
                   &SystemTime,
                   NULL,
                   DateBuffer,
                   80);

    GetTimeFormatW(LOCALE_USER_DEFAULT,
                   TIME_NOSECONDS,
                   &SystemTime,
                   NULL,
                   TimeBuffer,
                   80);

    PrintToConsole(L"%s %s", DateBuffer, TimeBuffer);
}


static
DWORD
GetTimeInSeconds(VOID)
{
    LARGE_INTEGER Time;
    FILETIME FileTime;
    DWORD dwSeconds;

    GetSystemTimeAsFileTime(&FileTime);
    Time.u.LowPart = FileTime.dwLowDateTime;
    Time.u.HighPart = FileTime.dwHighDateTime;
    RtlTimeToSecondsSince1970(&Time, &dwSeconds);

    return dwSeconds;
}


static
NET_API_STATUS
DisplayUser(LPWSTR lpUserName)
{
    PUSER_MODALS_INFO_0 pUserModals = NULL;
    PUSER_INFO_4 pUserInfo = NULL;
    PLOCALGROUP_USERS_INFO_0 pLocalGroupInfo = NULL;
    PGROUP_USERS_INFO_0 pGroupInfo = NULL;
    DWORD dwLocalGroupRead, dwLocalGroupTotal;
    DWORD dwGroupRead, dwGroupTotal;
    DWORD dwLastSet;
    DWORD i;
    INT nPaddedLength = 29;
    NET_API_STATUS Status;

    /* Modify the user */
    Status = NetUserGetInfo(NULL,
                            lpUserName,
                            4,
                            (LPBYTE*)&pUserInfo);
    if (Status != NERR_Success)
        return Status;

    Status = NetUserModalsGet(NULL,
                              0,
                              (LPBYTE*)&pUserModals);
    if (Status != NERR_Success)
        goto done;

    Status = NetUserGetLocalGroups(NULL,
                                   lpUserName,
                                   0,
                                   0,
                                   (LPBYTE*)&pLocalGroupInfo,
                                   MAX_PREFERRED_LENGTH,
                                   &dwLocalGroupRead,
                                   &dwLocalGroupTotal);
    if (Status != NERR_Success)
        goto done;

    Status = NetUserGetGroups(NULL,
                              lpUserName,
                              0,
                              (LPBYTE*)&pGroupInfo,
                              MAX_PREFERRED_LENGTH,
                              &dwGroupRead,
                              &dwGroupTotal);
    if (Status != NERR_Success)
        goto done;

    PrintPaddedResourceString(IDS_USER_NAME, nPaddedLength);
    PrintToConsole(L"%s\n", pUserInfo->usri4_name);

    PrintPaddedResourceString(IDS_USER_FULL_NAME, nPaddedLength);
    PrintToConsole(L"%s\n", pUserInfo->usri4_full_name);

    PrintPaddedResourceString(IDS_USER_COMMENT, nPaddedLength);
    PrintToConsole(L"%s\n", pUserInfo->usri4_comment);

    PrintPaddedResourceString(IDS_USER_USER_COMMENT, nPaddedLength);
    PrintToConsole(L"%s\n", pUserInfo->usri4_usr_comment);

    PrintPaddedResourceString(IDS_USER_COUNTRY_CODE, nPaddedLength);
    PrintToConsole(L"%03ld ()\n", pUserInfo->usri4_country_code);

    PrintPaddedResourceString(IDS_USER_ACCOUNT_ACTIVE, nPaddedLength);
    if (pUserInfo->usri4_flags & UF_ACCOUNTDISABLE)
        PrintResourceString(IDS_GENERIC_NO);
    else if (pUserInfo->usri4_flags & UF_LOCKOUT)
        PrintResourceString(IDS_GENERIC_LOCKED);
    else
        PrintResourceString(IDS_GENERIC_YES);
    PrintToConsole(L"\n");

    PrintPaddedResourceString(IDS_USER_ACCOUNT_EXPIRES, nPaddedLength);
    if (pUserInfo->usri4_acct_expires == TIMEQ_FOREVER)
        PrintResourceString(IDS_GENERIC_NEVER);
    else
        PrintDateTime(pUserInfo->usri4_acct_expires);
    PrintToConsole(L"\n\n");

    PrintPaddedResourceString(IDS_USER_PW_LAST_SET, nPaddedLength);
    dwLastSet = GetTimeInSeconds() - pUserInfo->usri4_password_age;
    PrintDateTime(dwLastSet);

    PrintPaddedResourceString(IDS_USER_PW_EXPIRES, nPaddedLength);
    if ((pUserInfo->usri4_flags & UF_DONT_EXPIRE_PASSWD) || pUserModals->usrmod0_max_passwd_age == TIMEQ_FOREVER)
        PrintResourceString(IDS_GENERIC_NEVER);
    else
        PrintDateTime(dwLastSet + pUserModals->usrmod0_max_passwd_age);
    PrintToConsole(L"\n");

    PrintPaddedResourceString(IDS_USER_PW_CHANGEABLE, nPaddedLength);
    PrintDateTime(dwLastSet + pUserModals->usrmod0_min_passwd_age);

    PrintPaddedResourceString(IDS_USER_PW_REQUIRED, nPaddedLength);
    PrintResourceString((pUserInfo->usri4_flags & UF_PASSWD_NOTREQD) ? IDS_GENERIC_NO : IDS_GENERIC_YES);
    PrintToConsole(L"\n");

    PrintPaddedResourceString(IDS_USER_CHANGE_PW, nPaddedLength);
    PrintResourceString((pUserInfo->usri4_flags & UF_PASSWD_CANT_CHANGE) ? IDS_GENERIC_NO : IDS_GENERIC_YES);
    PrintToConsole(L"\n\n");

    PrintPaddedResourceString(IDS_USER_WORKSTATIONS, nPaddedLength);
    if (pUserInfo->usri4_workstations == NULL || wcslen(pUserInfo->usri4_workstations) == 0)
        PrintResourceString(IDS_GENERIC_ALL);
    else
        PrintToConsole(L"%s", pUserInfo->usri4_workstations);
    PrintToConsole(L"\n");

    PrintPaddedResourceString(IDS_USER_LOGON_SCRIPT, nPaddedLength);
    PrintToConsole(L"%s\n", pUserInfo->usri4_script_path);

    PrintPaddedResourceString(IDS_USER_PROFILE, nPaddedLength);
    PrintToConsole(L"%s\n", pUserInfo->usri4_profile);

    PrintPaddedResourceString(IDS_USER_HOME_DIR, nPaddedLength);
    PrintToConsole(L"%s\n", pUserInfo->usri4_home_dir);

    PrintPaddedResourceString(IDS_USER_LAST_LOGON, nPaddedLength);
    if (pUserInfo->usri4_last_logon == 0)
        PrintResourceString(IDS_GENERIC_NEVER);
    else
        PrintDateTime(pUserInfo->usri4_last_logon);
    PrintToConsole(L"\n\n");

    PrintPaddedResourceString(IDS_USER_LOGON_HOURS, nPaddedLength);
    if (pUserInfo->usri4_logon_hours == NULL)
        PrintResourceString(IDS_GENERIC_ALL);
    PrintToConsole(L"\n\n");

    PrintToConsole(L"\n");
    PrintPaddedResourceString(IDS_USER_LOCAL_GROUPS, nPaddedLength);
    if (dwLocalGroupTotal != 0 && pLocalGroupInfo != NULL)
    {
        for (i = 0; i < dwLocalGroupTotal; i++)
        {
            if (i != 0)
                PrintPadding(L' ', nPaddedLength);
            PrintToConsole(L"*%s\n", pLocalGroupInfo[i].lgrui0_name);
        }
    }
    else
    {
        PrintToConsole(L"\n");
    }

    PrintPaddedResourceString(IDS_USER_GLOBAL_GROUPS, nPaddedLength);
    if (dwGroupTotal != 0 && pGroupInfo != NULL)
    {
        for (i = 0; i < dwGroupTotal; i++)
        {
            if (i != 0)
                PrintPadding(L' ', nPaddedLength);
            PrintToConsole(L"*%s\n", pGroupInfo[i].grui0_name);
        }
    }
    else
    {
        PrintToConsole(L"\n");
    }

done:
    if (pGroupInfo != NULL)
        NetApiBufferFree(pGroupInfo);

    if (pLocalGroupInfo != NULL)
        NetApiBufferFree(pLocalGroupInfo);

    if (pUserModals != NULL)
        NetApiBufferFree(pUserModals);

    if (pUserInfo != NULL)
        NetApiBufferFree(pUserInfo);

    return NERR_Success;
}


static
VOID
ReadPassword(
    LPWSTR *lpPassword,
    LPBOOL lpAllocated)
{
    WCHAR szPassword1[PWLEN + 1];
    WCHAR szPassword2[PWLEN + 1];
    LPWSTR ptr;

    *lpAllocated = FALSE;

    while (TRUE)
    {
        PrintResourceString(IDS_USER_ENTER_PASSWORD1);
        ReadFromConsole(szPassword1, PWLEN + 1, FALSE);
        PrintToConsole(L"\n");

        PrintResourceString(IDS_USER_ENTER_PASSWORD2);
        ReadFromConsole(szPassword2, PWLEN + 1, FALSE);
        PrintToConsole(L"\n");

        if (wcslen(szPassword1) == wcslen(szPassword2) &&
            wcscmp(szPassword1, szPassword2) == 0)
        {
            ptr = HeapAlloc(GetProcessHeap(),
                            0,
                            (wcslen(szPassword1) + 1) * sizeof(WCHAR));
            if (ptr != NULL)
            {
                wcscpy(ptr, szPassword1);
                *lpPassword = ptr;
                *lpAllocated = TRUE;
                return;
            }
        }
        else
        {
            PrintToConsole(L"\n");
            PrintResourceString(IDS_USER_NO_PASSWORD_MATCH);
            PrintToConsole(L"\n");
            *lpPassword = NULL;
        }
    }
}


INT
cmdUser(
    INT argc,
    WCHAR **argv)
{
    INT i, j;
    INT result = 0;
    BOOL bAdd = FALSE;
    BOOL bDelete = FALSE;
#if 0
    BOOL bDomain = FALSE;
#endif
    LPWSTR lpUserName = NULL;
    LPWSTR lpPassword = NULL;
    PUSER_INFO_4 pUserInfo = NULL;
    USER_INFO_4 UserInfo;
    LPWSTR p;
    LPWSTR endptr;
    DWORD value;
    BOOL bPasswordAllocated = FALSE;
    NET_API_STATUS Status;

    if (argc == 2)
    {
        Status = EnumerateUsers();
        printf("Status: %lu\n", Status);
        return 0;
    }
    else if (argc == 3)
    {
        Status = DisplayUser(argv[2]);
        printf("Status: %lu\n", Status);
        return 0;
    }

    i = 2;
    if (argv[i][0] != L'/')
    {
        lpUserName = argv[i];
//        printf("User: %S\n", lpUserName);
        i++;
    }

    if (argv[i][0] != L'/')
    {
        lpPassword = argv[i];
//        printf("Password: %S\n", lpPassword);
        i++;
    }

    for (j = i; j < argc; j++)
    {
        if (_wcsicmp(argv[j], L"/help") == 0)
        {
            PrintResourceString(IDS_USER_HELP);
            return 0;
        }
        else if (_wcsicmp(argv[j], L"/add") == 0)
        {
            bAdd = TRUE;
        }
        else if (_wcsicmp(argv[j], L"/delete") == 0)
        {
            bDelete = TRUE;
        }
        else if (_wcsicmp(argv[j], L"/domain") == 0)
        {
            PrintResourceString(IDS_ERROR_OPTION_NOT_SUPPORTED, L"/DOMAIN");
#if 0
            bDomain = TRUE;
#endif
        }
    }

    if (bAdd && bDelete)
    {
        result = 1;
        goto done;
    }

    /* Interactive password input */
    if (lpPassword != NULL && wcscmp(lpPassword, L"*") == 0)
    {
        ReadPassword(&lpPassword,
                     &bPasswordAllocated);
    }

    if (!bAdd && !bDelete)
    {
        /* Modify the user */
        Status = NetUserGetInfo(NULL,
                                lpUserName,
                                4,
                                (LPBYTE*)&pUserInfo);
        if (Status != NERR_Success)
        {
            printf("Status: %lu\n", Status);
            result = 1;
            goto done;
        }
    }
    else if (bAdd && !bDelete)
    {
        /* Add the user */
        ZeroMemory(&UserInfo, sizeof(USER_INFO_4));

        UserInfo.usri4_name = lpUserName;
        UserInfo.usri4_password = lpPassword;
        UserInfo.usri4_flags = UF_SCRIPT | UF_NORMAL_ACCOUNT;

        pUserInfo = &UserInfo;
    }

    for (j = i; j < argc; j++)
    {
        if (_wcsnicmp(argv[j], L"/active:", 8) == 0)
        {
            p = &argv[i][8];
            if (_wcsicmp(p, L"yes") == 0)
            {
                pUserInfo->usri4_flags &= ~UF_ACCOUNTDISABLE;
            }
            else if (_wcsicmp(p, L"no") == 0)
            {
                pUserInfo->usri4_flags |= UF_ACCOUNTDISABLE;
            }
            else
            {
                PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"/ACTIVE");
                result = 1;
                goto done;
            }
        }
        else if (_wcsnicmp(argv[j], L"/comment:", 9) == 0)
        {
            pUserInfo->usri4_comment = &argv[j][9];
        }
        else if (_wcsnicmp(argv[j], L"/countrycode:", 13) == 0)
        {
            p = &argv[i][13];
            value = wcstoul(p, &endptr, 10);
            if (*endptr != 0)
            {
                PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"/COUNTRYCODE");
                result = 1;
                goto done;
            }

            /* FIXME: verify the country code */

            pUserInfo->usri4_country_code = value;
        }
        else if (_wcsnicmp(argv[j], L"/expires:", 9) == 0)
        {
            p = &argv[i][9];
            if (_wcsicmp(p, L"never") == 0)
            {
                pUserInfo->usri4_acct_expires = TIMEQ_FOREVER;
            }
            else
            {
                /* FIXME: Parse the date */
                PrintResourceString(IDS_ERROR_OPTION_NOT_SUPPORTED, L"/EXPIRES");
            }
        }
        else if (_wcsnicmp(argv[j], L"/fullname:", 10) == 0)
        {
            pUserInfo->usri4_full_name = &argv[j][10];
        }
        else if (_wcsnicmp(argv[j], L"/homedir:", 9) == 0)
        {
            pUserInfo->usri4_home_dir = &argv[j][9];
        }
        else if (_wcsnicmp(argv[j], L"/passwordchg:", 13) == 0)
        {
            p = &argv[i][13];
            if (_wcsicmp(p, L"yes") == 0)
            {
                pUserInfo->usri4_flags &= ~UF_PASSWD_CANT_CHANGE;
            }
            else if (_wcsicmp(p, L"no") == 0)
            {
                pUserInfo->usri4_flags |= UF_PASSWD_CANT_CHANGE;
            }
            else
            {
                PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"/PASSWORDCHG");
                result = 1;
                goto done;
            }
        }
        else if (_wcsnicmp(argv[j], L"/passwordreq:", 13) == 0)
        {
            p = &argv[i][13];
            if (_wcsicmp(p, L"yes") == 0)
            {
                pUserInfo->usri4_flags &= ~UF_PASSWD_NOTREQD;
            }
            else if (_wcsicmp(p, L"no") == 0)
            {
                pUserInfo->usri4_flags |= UF_PASSWD_NOTREQD;
            }
            else
            {
                PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"/PASSWORDREQ");
                result = 1;
                goto done;
            }
        }
        else if (_wcsnicmp(argv[j], L"/profilepath:", 13) == 0)
        {
            pUserInfo->usri4_profile = &argv[j][13];
        }
        else if (_wcsnicmp(argv[j], L"/scriptpath:", 12) == 0)
        {
            pUserInfo->usri4_script_path = &argv[j][12];
        }
        else if (_wcsnicmp(argv[j], L"/times:", 7) == 0)
        {
            /* FIXME */
            PrintResourceString(IDS_ERROR_OPTION_NOT_SUPPORTED, L"/TIMES");
        }
        else if (_wcsnicmp(argv[j], L"/usercomment:", 13) == 0)
        {
            pUserInfo->usri4_usr_comment = &argv[j][13];
        }
        else if (_wcsnicmp(argv[j], L"/workstations:", 14) == 0)
        {
            /* FIXME */
            PrintResourceString(IDS_ERROR_OPTION_NOT_SUPPORTED, L"/WORKSTATIONS");
        }
    }

    if (!bAdd && !bDelete)
    {
        /* Modify the user */
        Status = NetUserSetInfo(NULL,
                                lpUserName,
                                4,
                                (LPBYTE)pUserInfo,
                                NULL);
        printf("Status: %lu\n", Status);
    }
    else if (bAdd && !bDelete)
    {
        /* Add the user */
        Status = NetUserAdd(NULL,
                            4,
                            (LPBYTE)pUserInfo,
                            NULL);
        printf("Status: %lu\n", Status);
    }
    else if (!bAdd && bDelete)
    {
        /* Delete the user */
        Status = NetUserDel(NULL,
                            lpUserName);
        printf("Status: %lu\n", Status);
    }

done:
    if (bPasswordAllocated == TRUE && lpPassword != NULL)
        HeapFree(GetProcessHeap(), 0, lpPassword);

    if (!bAdd && !bDelete && pUserInfo != NULL)
        NetApiBufferFree(pUserInfo);

    if (result != 0)
        PrintResourceString(IDS_USER_SYNTAX);

    return result;
}

/* EOF */
