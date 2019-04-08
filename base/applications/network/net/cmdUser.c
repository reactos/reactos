/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdUser.c
 * PURPOSE:
 *
 * PROGRAMMERS:     Eric Kohl
 *                  Curtis Wilson
 */

#include "net.h"

typedef struct _COUNTY_TABLE
{
    DWORD dwCountryCode;
    DWORD dwMessageId;
} COUNTRY_TABLE, *PCOUNTRY_TABLE;


static WCHAR szPasswordChars[] = L"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@#$%_-+:";
static COUNTRY_TABLE CountryTable[] =
{ {  0, 5080},   // System Default
  {  1, 5081},   // United States
  {  2, 5082},   // Canada (French)
  {  3, 5083},   // Latin America
  { 31, 5084},   // Netherlands
  { 32, 5085},   // Belgium
  { 33, 5086},   // France
  { 34, 5090},   // Spain
  { 39, 5087},   // Italy
  { 41, 5088},   // Switzerland
  { 44, 5089},   // United Kingdom
  { 45, 5091},   // Denmark
  { 46, 5092},   // Sweden
  { 47, 5093},   // Norway
  { 49, 5094},   // Germany
  { 61, 5095},   // Australia
  { 81, 5096},   // Japan
  { 82, 5097},   // Korea
  { 86, 5098},   // China (PRC)
  { 88, 5099},   // Taiwan
  { 99, 5100},   // Asia
  {351, 5101},   // Portugal
  {358, 5102},   // Finland
  {785, 5103},   // Arabic
  {972, 5104} }; // Hebrew


static
int
CompareInfo(const void *a, const void *b)
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
    DWORD ResumeHandle = 0;
    NET_API_STATUS Status;

    Status = NetServerGetInfo(NULL,
                              100,
                              (LPBYTE*)&pServer);
    if (Status != NERR_Success)
        return Status;

    ConPuts(StdOut, L"\n");
    PrintMessageStringV(4410, pServer->sv100_name);
    ConPuts(StdOut, L"\n");
    PrintPadding(L'-', 79);
    ConPuts(StdOut, L"\n");

    NetApiBufferFree(pServer);

    do
    {
        Status = NetUserEnum(NULL,
                             0,
                             0,
                             (LPBYTE*)&pBuffer,
                             MAX_PREFERRED_LENGTH,
                             &dwRead,
                             &dwTotal,
                             &ResumeHandle);
        if ((Status != NERR_Success) && (Status != ERROR_MORE_DATA))
            return Status;

        qsort(pBuffer,
              dwRead,
              sizeof(PUSER_INFO_0),
              CompareInfo);

        for (i = 0; i < dwRead; i++)
        {
            if (pBuffer[i].usri0_name)
                ConPrintf(StdOut, L"%s\n", pBuffer[i].usri0_name);
        }

        NetApiBufferFree(pBuffer);
        pBuffer = NULL;
    }
    while (Status == ERROR_MORE_DATA);

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

    ConPrintf(StdOut, L"%s %s", DateBuffer, TimeBuffer);
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
BOOL
GetCountryFromCountryCode(
    _In_ DWORD dwCountryCode,
    _In_ DWORD dwCountryLength,
    _Out_ PWSTR szCountryBuffer)
{
    DWORD i;

    for (i = 0; i < ARRAYSIZE(CountryTable); i++)
    {
        if (CountryTable[i].dwCountryCode == dwCountryCode)
        {
            if (szCountryBuffer != NULL && dwCountryLength > 0)
            {
                FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                               hModuleNetMsg,
                               CountryTable[i].dwMessageId,
                               LANG_USER_DEFAULT,
                               szCountryBuffer,
                               dwCountryLength,
                               NULL);
            }

            return TRUE;
        }
    }

    return FALSE;
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
    WCHAR szCountry[40];
    INT nPaddedLength = 36;
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

    PrintPaddedMessageString(4411, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pUserInfo->usri4_name);

    PrintPaddedMessageString(4412, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pUserInfo->usri4_full_name);

    PrintPaddedMessageString(4413, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pUserInfo->usri4_comment);

    PrintPaddedMessageString(4414, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pUserInfo->usri4_usr_comment);

    PrintPaddedMessageString(4416, nPaddedLength);
    GetCountryFromCountryCode(pUserInfo->usri4_country_code,
                              ARRAYSIZE(szCountry), szCountry);
    ConPrintf(StdOut, L"%03ld (%s)\n", pUserInfo->usri4_country_code, szCountry);

    PrintPaddedMessageString(4419, nPaddedLength);
    if (pUserInfo->usri4_flags & UF_ACCOUNTDISABLE)
        PrintMessageString(4301);
    else if (pUserInfo->usri4_flags & UF_LOCKOUT)
        PrintMessageString(4440);
    else
        PrintMessageString(4300);
    ConPuts(StdOut, L"\n");

    PrintPaddedMessageString(4420, nPaddedLength);
    if (pUserInfo->usri4_acct_expires == TIMEQ_FOREVER)
        PrintMessageString(4305);
    else
        PrintDateTime(pUserInfo->usri4_acct_expires);
    ConPuts(StdOut, L"\n\n");

    PrintPaddedMessageString(4421, nPaddedLength);
    dwLastSet = GetTimeInSeconds() - pUserInfo->usri4_password_age;
    PrintDateTime(dwLastSet);
    ConPuts(StdOut, L"\n");

    PrintPaddedMessageString(4422, nPaddedLength);
    if ((pUserInfo->usri4_flags & UF_DONT_EXPIRE_PASSWD) || pUserModals->usrmod0_max_passwd_age == TIMEQ_FOREVER)
        PrintMessageString(4305);
    else
        PrintDateTime(dwLastSet + pUserModals->usrmod0_max_passwd_age);
    ConPuts(StdOut, L"\n");

    PrintPaddedMessageString(4423, nPaddedLength);
    PrintDateTime(dwLastSet + pUserModals->usrmod0_min_passwd_age);
    ConPuts(StdOut, L"\n");

    PrintPaddedMessageString(4437, nPaddedLength);
    PrintMessageString((pUserInfo->usri4_flags & UF_PASSWD_NOTREQD) ? 4301 : 4300);
    ConPuts(StdOut, L"\n");

    PrintPaddedMessageString(4438, nPaddedLength);
    PrintMessageString((pUserInfo->usri4_flags & UF_PASSWD_CANT_CHANGE) ? 4301 : 4300);
    ConPuts(StdOut, L"\n\n");

    PrintPaddedMessageString(4424, nPaddedLength);
    if (pUserInfo->usri4_workstations == NULL || wcslen(pUserInfo->usri4_workstations) == 0)
        PrintMessageString(4302);
    else
        ConPrintf(StdOut, L"%s", pUserInfo->usri4_workstations);
    ConPuts(StdOut, L"\n");

    PrintPaddedMessageString(4429, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pUserInfo->usri4_script_path);

    PrintPaddedMessageString(4439, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pUserInfo->usri4_profile);

    PrintPaddedMessageString(4436, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pUserInfo->usri4_home_dir);

    PrintPaddedMessageString(4430, nPaddedLength);
    if (pUserInfo->usri4_last_logon == 0)
        PrintMessageString(4305);
    else
        PrintDateTime(pUserInfo->usri4_last_logon);
    ConPuts(StdOut, L"\n\n");

    PrintPaddedMessageString(4432, nPaddedLength);
    if (pUserInfo->usri4_logon_hours == NULL)
        PrintMessageString(4302);
    ConPuts(StdOut, L"\n\n");

    ConPuts(StdOut, L"\n");
    PrintPaddedMessageString(4427, nPaddedLength);
    if (dwLocalGroupTotal != 0 && pLocalGroupInfo != NULL)
    {
        for (i = 0; i < dwLocalGroupTotal; i++)
        {
            if (i != 0)
                PrintPadding(L' ', nPaddedLength);
            ConPrintf(StdOut, L"*%s\n", pLocalGroupInfo[i].lgrui0_name);
        }
    }
    else
    {
        ConPuts(StdOut, L"\n");
    }

    PrintPaddedMessageString(4431, nPaddedLength);
    if (dwGroupTotal != 0 && pGroupInfo != NULL)
    {
        for (i = 0; i < dwGroupTotal; i++)
        {
            if (i != 0)
                PrintPadding(L' ', nPaddedLength);
            ConPrintf(StdOut, L"*%s\n", pGroupInfo[i].grui0_name);
        }
    }
    else
    {
        ConPuts(StdOut, L"\n");
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
        PrintMessageString(4358);
        ReadFromConsole(szPassword1, PWLEN + 1, FALSE);
        ConPuts(StdOut, L"\n");

        PrintMessageString(4361);
        ReadFromConsole(szPassword2, PWLEN + 1, FALSE);
        ConPuts(StdOut, L"\n");

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
            ConPuts(StdOut, L"\n");
            PrintMessageString(3728);
            *lpPassword = NULL;
        }
    }
}


static
VOID
GenerateRandomPassword(
    LPWSTR *lpPassword,
    LPBOOL lpAllocated)
{
    LPWSTR pPassword = NULL;
    INT nCharsLen, i, nLength = 8;

    srand(GetTickCount());

    pPassword = HeapAlloc(GetProcessHeap(),
                          HEAP_ZERO_MEMORY,
                          (nLength + 1) * sizeof(WCHAR));
    if (pPassword == NULL)
        return;

    nCharsLen = wcslen(szPasswordChars);

    for (i = 0; i < nLength; i++)
    {
        pPassword[i] = szPasswordChars[rand() % nCharsLen];
    }

    *lpPassword = pPassword;
    *lpAllocated = TRUE;
}


static
NET_API_STATUS
BuildWorkstationsList(
    _Out_ PWSTR *pWorkstationsList,
    _In_ PWSTR pRaw)
{
    BOOL isLastSep, isSep;
    INT i, j;
    WCHAR c;
    INT nLength = 0;
    INT nArgs = 0;
    INT nRawLength;
    PWSTR pList;

    /* Check for invalid characters in the raw string */
    if (wcspbrk(pRaw, L"/[]=?\\+:.") != NULL)
        return 3952;

    /* Count the number of workstations in the list and
     * the required buffer size */
    isLastSep = FALSE;
    isSep = FALSE;
    nRawLength = wcslen(pRaw);
    for (i = 0; i < nRawLength; i++)
    {
        c = pRaw[i];
        if (c == L',' || c == L';')
            isSep = TRUE;

        if (isSep == TRUE)
        {
            if ((isLastSep == FALSE) && (i != 0) && (i != nRawLength - 1))
                nLength++;
        }
        else
        {
            nLength++;

            if (isLastSep == TRUE || (isLastSep == FALSE && i == 0))
                nArgs++;
        }

        isLastSep = isSep;
        isSep = FALSE;
    }

    nLength++;

    /* Leave, if there are no workstations in the list */
    if (nArgs == 0)
    {
        pWorkstationsList = NULL;
        return NERR_Success;
    }

    /* Fail if there are more than eight workstations in the list */
    if (nArgs > 8)
        return 3951;

    /* Allocate the buffer for the clean workstation list */
    pList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nLength * sizeof(WCHAR));
    if (pList == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Build the clean workstation list */
    isLastSep = FALSE;
    isSep = FALSE;
    nRawLength = wcslen(pRaw);
    for (i = 0, j = 0; i < nRawLength; i++)
    {
        c = pRaw[i];
        if (c == L',' || c == L';')
            isSep = TRUE;

        if (isSep == TRUE)
        {
            if ((isLastSep == FALSE) && (i != 0) && (i != nRawLength - 1))
            {
                pList[j] = L',';
                j++;
            }
        }
        else
        {
            pList[j] = c;
            j++;

            if (isLastSep == TRUE || (isLastSep == FALSE && i == 0))
                nArgs++;
        }

        isLastSep = isSep;
        isSep = FALSE;
    }

    *pWorkstationsList = pList;

    return NERR_Success;
}


static
BOOL
ReadNumber(
    PWSTR *s,
    PWORD pwValue)
{
    if (!iswdigit(**s))
        return FALSE;

    while (iswdigit(**s))
    {
        *pwValue = *pwValue * 10 + **s - L'0';
        (*s)++;
    }

    return TRUE;
}


static
BOOL
ReadSeparator(
    PWSTR *s)
{
    if (**s == L'/' || **s == L'.')
    {
        (*s)++;
        return TRUE;
    }

    return FALSE;
}


static
BOOL
ParseDate(
    PWSTR s,
    PULONG pSeconds)
{
    SYSTEMTIME SystemTime = {0};
    FILETIME LocalFileTime, FileTime;
    LARGE_INTEGER Time;
    INT nDateFormat = 0;
    PWSTR p = s;

    if (!*s)
        return FALSE;

    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_IDATE,
                   (PWSTR)&nDateFormat,
                   sizeof(INT));

    switch (nDateFormat)
    {
        case 0: /* mmddyy */
        default:
            if (!ReadNumber(&p, &SystemTime.wMonth))
                return FALSE;
            if (!ReadSeparator(&p))
                return FALSE;
            if (!ReadNumber(&p, &SystemTime.wDay))
                return FALSE;
            if (!ReadSeparator(&p))
                return FALSE;
            if (!ReadNumber(&p, &SystemTime.wYear))
                return FALSE;
            break;

        case 1: /* ddmmyy */
            if (!ReadNumber(&p, &SystemTime.wDay))
                return FALSE;
            if (!ReadSeparator(&p))
                return FALSE;
            if (!ReadNumber(&p, &SystemTime.wMonth))
                return FALSE;
            if (!ReadSeparator(&p))
                return FALSE;
            if (!ReadNumber(&p, &SystemTime.wYear))
                return FALSE;
            break;

        case 2: /* yymmdd */
            if (!ReadNumber(&p, &SystemTime.wYear))
                return FALSE;
            if (!ReadSeparator(&p))
                return FALSE;
            if (!ReadNumber(&p, &SystemTime.wMonth))
                return FALSE;
            if (!ReadSeparator(&p))
                return FALSE;
            if (!ReadNumber(&p, &SystemTime.wDay))
                return FALSE;
            break;
    }

    /* if only entered two digits: */
    /*   assume 2000's if value less than 80 */
    /*   assume 1900's if value greater or equal 80 */
    if (SystemTime.wYear <= 99)
    {
        if (SystemTime.wYear >= 80)
            SystemTime.wYear += 1900;
        else
            SystemTime.wYear += 2000;
    }

    if (!SystemTimeToFileTime(&SystemTime, &LocalFileTime))
        return FALSE;

    if (!LocalFileTimeToFileTime(&LocalFileTime, &FileTime))
        return FALSE;

    Time.u.LowPart = FileTime.dwLowDateTime;
    Time.u.HighPart = FileTime.dwHighDateTime;

    if (!RtlTimeToSecondsSince1970(&Time, pSeconds))
        return FALSE;

    return TRUE;
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
    BOOL bRandomPassword = FALSE;
    LPWSTR lpUserName = NULL;
    LPWSTR lpPassword = NULL;
    PUSER_INFO_4 pUserInfo = NULL;
    USER_INFO_4 UserInfo;
    LPWSTR pWorkstations = NULL;
    LPWSTR p;
    LPWSTR endptr;
    DWORD value;
    BOOL bPasswordAllocated = FALSE;
    NET_API_STATUS Status;

    i = 2;
    if ((i < argc) && (argv[i][0] != L'/'))
    {
        lpUserName = argv[i];
//        ConPrintf(StdOut, L"User: %s\n", lpUserName);
        i++;
    }

    if ((i < argc) && (argv[i][0] != L'/'))
    {
        lpPassword = argv[i];
//        ConPrintf(StdOut, L"Password: %s\n", lpPassword);
        i++;
    }

    for (j = i; j < argc; j++)
    {
        if (_wcsicmp(argv[j], L"/help") == 0)
        {
            PrintNetMessage(MSG_USER_HELP);
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
            ConResPrintf(StdErr, IDS_ERROR_OPTION_NOT_SUPPORTED, L"/DOMAIN");
#if 0
            bDomain = TRUE;
#endif
        }
        else if (_wcsicmp(argv[j], L"/random") == 0)
        {
            bRandomPassword = TRUE;
            GenerateRandomPassword(&lpPassword,
                                   &bPasswordAllocated);
        }
    }

    if (lpUserName == NULL && lpPassword == NULL)
    {
        Status = EnumerateUsers();
        ConPrintf(StdOut, L"Status: %lu\n", Status);
        return 0;
    }
    else if (lpUserName != NULL && lpPassword == NULL)
    {
        Status = DisplayUser(lpUserName);
        ConPrintf(StdOut, L"Status: %lu\n", Status);
        return 0;
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
            ConPrintf(StdOut, L"Status: %lu\n", Status);
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
        UserInfo.usri4_acct_expires = TIMEQ_FOREVER;
        UserInfo.usri4_primary_group_id = DOMAIN_GROUP_RID_USERS;

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
                ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"/ACTIVE");
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
                ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"/COUNTRYCODE");
                result = 1;
                goto done;
            }

            /* Verify the country code */
            if (GetCountryFromCountryCode(value, 0, NULL))
                pUserInfo->usri4_country_code = value;
        }
        else if (_wcsnicmp(argv[j], L"/expires:", 9) == 0)
        {
            p = &argv[i][9];
            if (_wcsicmp(p, L"never") == 0)
            {
                pUserInfo->usri4_acct_expires = TIMEQ_FOREVER;
            }
            else if (!ParseDate(p, &pUserInfo->usri4_acct_expires))
            {
                ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"/EXPIRES");
                result = 1;
                goto done;
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
                ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"/PASSWORDCHG");
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
                ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"/PASSWORDREQ");
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
            ConResPrintf(StdErr, IDS_ERROR_OPTION_NOT_SUPPORTED, L"/TIMES");
        }
        else if (_wcsnicmp(argv[j], L"/usercomment:", 13) == 0)
        {
            pUserInfo->usri4_usr_comment = &argv[j][13];
        }
        else if (_wcsnicmp(argv[j], L"/workstations:", 14) == 0)
        {
            p = &argv[i][14];
            if (wcscmp(p, L"*") == 0 || wcscmp(p, L"") == 0)
            {
                pUserInfo->usri4_workstations = NULL;
            }
            else
            {
                Status = BuildWorkstationsList(&pWorkstations, p);
                if (Status == NERR_Success)
                {
                    pUserInfo->usri4_workstations = pWorkstations;
                }
                else
                {
                    ConPrintf(StdOut, L"Status %lu\n\n", Status);
                    result = 1;
                    goto done;
                }
            }
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
        ConPrintf(StdOut, L"Status: %lu\n", Status);
    }
    else if (bAdd && !bDelete)
    {
        /* Add the user */
        Status = NetUserAdd(NULL,
                            4,
                            (LPBYTE)pUserInfo,
                            NULL);
        ConPrintf(StdOut, L"Status: %lu\n", Status);
    }
    else if (!bAdd && bDelete)
    {
        /* Delete the user */
        Status = NetUserDel(NULL,
                            lpUserName);
        ConPrintf(StdOut, L"Status: %lu\n", Status);
    }

    if (Status == NERR_Success &&
        lpPassword != NULL &&
        bRandomPassword == TRUE)
    {
        PrintMessageStringV(3968, lpUserName, lpPassword);
    }

done:
    if (pWorkstations != NULL)
        HeapFree(GetProcessHeap(), 0, pWorkstations);

    if ((bPasswordAllocated == TRUE) && (lpPassword != NULL))
        HeapFree(GetProcessHeap(), 0, lpPassword);

    if (!bAdd && !bDelete && pUserInfo != NULL)
        NetApiBufferFree(pUserInfo);

    if (result != 0)
    {
        ConResPuts(StdOut, IDS_GENERIC_SYNTAX);
        PrintNetMessage(MSG_USER_SYNTAX);
    }

    return result;
}

/* EOF */
