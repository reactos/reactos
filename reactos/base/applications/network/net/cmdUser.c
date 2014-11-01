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

    PrintToConsole(L"\nUser accounts for \\\\%s\n\n", pServer->sv100_name);

    NetApiBufferFree(pServer);

    PrintToConsole(L"------------------------------------------\n");

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

    PrintToConsole(L"%s %s\n", DateBuffer, TimeBuffer);
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
    DWORD dwLastSet;
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

    PrintToConsole(L"User name                    %s\n", pUserInfo->usri4_name);
    PrintToConsole(L"Full name                    %s\n", pUserInfo->usri4_full_name);
    PrintToConsole(L"Comment                      %s\n", pUserInfo->usri4_comment);
    PrintToConsole(L"User comment                 %s\n", pUserInfo->usri4_usr_comment);
    PrintToConsole(L"Country code                 %03ld ()\n", pUserInfo->usri4_country_code);
    PrintToConsole(L"Account active               %s\n", (pUserInfo->usri4_flags & UF_ACCOUNTDISABLE)? L"No" : ((pUserInfo->usri4_flags & UF_LOCKOUT) ? L"Locked" : L"Yes"));
    PrintToConsole(L"Account expires              ");
    if (pUserInfo->usri4_acct_expires == TIMEQ_FOREVER)
        PrintToConsole(L"Never\n");
    else
        PrintDateTime(pUserInfo->usri4_acct_expires);

    PrintToConsole(L"\n");

    PrintToConsole(L"Password last set            ");
    dwLastSet = GetTimeInSeconds() - pUserInfo->usri4_password_age;
    PrintDateTime(dwLastSet);

    PrintToConsole(L"Password expires             ");
    if ((pUserInfo->usri4_flags & UF_DONT_EXPIRE_PASSWD) || pUserModals->usrmod0_max_passwd_age == TIMEQ_FOREVER)
        PrintToConsole(L"Never\n");
    else
        PrintDateTime(dwLastSet + pUserModals->usrmod0_max_passwd_age);

    PrintToConsole(L"Password changeable          ");
    PrintDateTime(dwLastSet + pUserModals->usrmod0_min_passwd_age);

    PrintToConsole(L"Password required            %s\n", (pUserInfo->usri4_flags & UF_PASSWD_NOTREQD) ? L"No" : L"Yes");
    PrintToConsole(L"User may change password     %s\n", (pUserInfo->usri4_flags & UF_PASSWD_CANT_CHANGE) ? L"No" : L"Yes");

    PrintToConsole(L"\n");
    PrintToConsole(L"Workstations allowed         %s\n", (pUserInfo->usri4_workstations == NULL || wcslen(pUserInfo->usri4_workstations) == 0) ? L"All" : pUserInfo->usri4_workstations);
    PrintToConsole(L"Logon script                 %s\n", pUserInfo->usri4_script_path);
    PrintToConsole(L"User profile                 %s\n", pUserInfo->usri4_profile);
    PrintToConsole(L"Home directory               %s\n", pUserInfo->usri4_home_dir);
    PrintToConsole(L"Last logon                   ");
    if (pUserInfo->usri4_last_logon == 0)
        PrintToConsole(L"Never\n");
    else
        PrintDateTime(pUserInfo->usri4_last_logon);
    PrintToConsole(L"\n");
    PrintToConsole(L"Logon hours allowed          \n");
    PrintToConsole(L"\n");
    PrintToConsole(L"Local group memberships      \n");
    PrintToConsole(L"Global group memberships     \n");

done:
    if (pUserModals != NULL)
        NetApiBufferFree(pUserModals);

    if (pUserInfo != NULL)
        NetApiBufferFree(pUserInfo);

    return NERR_Success;
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
        printf("User: %S\n", lpUserName);
        i++;
    }

    if (argv[i][0] != L'/')
    {
        lpPassword = argv[i];
        printf("Password: %S\n", lpPassword);
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
            printf("The /DOMAIN option is not supported yet!\n");
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
                UserInfo.usri4_flags &= ~UF_ACCOUNTDISABLE;
            }
            else if (_wcsicmp(p, L"no") == 0)
            {
                UserInfo.usri4_flags |= UF_ACCOUNTDISABLE;
            }
            else
            {
                PrintToConsole(L"You entered an invalid value for the /ACTIVE option.\n");
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
        }
        else if (_wcsnicmp(argv[j], L"/expires:", 9) == 0)
        {
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
        }
        else if (_wcsnicmp(argv[j], L"/passwordreq:", 13) == 0)
        {
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
        }
        else if (_wcsnicmp(argv[j], L"/usercomment:", 13) == 0)
        {
            pUserInfo->usri4_usr_comment = &argv[j][13];
        }
        else if (_wcsnicmp(argv[j], L"/workstations:", 14) == 0)
        {
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
    if (!bAdd && !bDelete && pUserInfo != NULL)
        NetApiBufferFree(pUserInfo);

    if (result != 0)
        PrintResourceString(IDS_USER_SYNTAX);

    return result;
}

/* EOF */
