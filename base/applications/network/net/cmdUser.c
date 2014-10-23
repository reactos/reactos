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

    printf("\nUser accounts for \\\\%S\n\n", pServer->sv100_name);

    NetApiBufferFree(pServer);

    printf("------------------------------------------\n");

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
            printf("%S\n", pBuffer[i].usri0_name);
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

    printf("%S %S\n", DateBuffer, TimeBuffer);
}


static
NET_API_STATUS
DisplayUser(LPWSTR lpUserName)
{
    PUSER_INFO_4 pUserInfo = NULL;
    NET_API_STATUS Status;

    /* Modify the user */
    Status = NetUserGetInfo(NULL,
                            lpUserName,
                            4,
                            (LPBYTE*)&pUserInfo);
    if (Status != NERR_Success)
        return Status;

    printf("User name                    %S\n", pUserInfo->usri4_name);
    printf("Full name                    %S\n", pUserInfo->usri4_full_name);
    printf("Comment                      %S\n", pUserInfo->usri4_comment);
    printf("User comment                 %S\n", pUserInfo->usri4_usr_comment);
    printf("Country code                 %03ld ()\n", pUserInfo->usri4_country_code);
    printf("Account active               %S\n", (pUserInfo->usri4_flags & UF_ACCOUNTDISABLE)? L"No" : ((pUserInfo->usri4_flags & UF_LOCKOUT) ? L"Locked" : L"Yes"));
    printf("Account expires              ");
    if (pUserInfo->usri4_acct_expires == TIMEQ_FOREVER)
        printf("Never\n");
    else
        PrintDateTime(pUserInfo->usri4_acct_expires);

    printf("\n");
    printf("Password last set            \n");
    printf("Password expires             \n");
    printf("Password changeable          \n");
    printf("Password required            \n");
    printf("User may change password     \n");
    printf("\n");
    printf("Workstation allowed          %S\n", pUserInfo->usri4_workstations);
    printf("Logon script                 %S\n", pUserInfo->usri4_script_path);
    printf("User profile                 %S\n", pUserInfo->usri4_profile);
    printf("Home directory               %S\n", pUserInfo->usri4_home_dir);
    printf("Last logon                   ");
    if (pUserInfo->usri4_last_logon == 0)
        printf("Never\n");
    else
        PrintDateTime(pUserInfo->usri4_last_logon);
    printf("\n");
    printf("Logon hours allowed          \n");
    printf("\n");
    printf("Local group memberships      \n");
    printf("Global group memberships     \n");

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
        printf("Status: %lu\n", Status);
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
