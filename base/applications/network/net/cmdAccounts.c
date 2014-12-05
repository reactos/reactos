/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:
 * PURPOSE:
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "net.h"

INT
cmdAccounts(
    INT argc,
    WCHAR **argv)
{
    PUSER_MODALS_INFO_0 Info0 = NULL;
    PUSER_MODALS_INFO_1 Info1 = NULL;
    PUSER_MODALS_INFO_3 Info3 = NULL;
    NT_PRODUCT_TYPE ProductType;
    LPWSTR p;
    LPWSTR endptr;
    DWORD ParamErr;
    ULONG value;
    INT i;
    BOOL Modified = FALSE;
#if 0
    BOOL Domain = FALSE;
#endif
    INT nPaddedLength = 58;
    NET_API_STATUS Status;
    INT result = 0;

    for (i = 2; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"help") == 0)
        {
            /* Print short syntax help */
            PrintResourceString(IDS_ACCOUNTS_SYNTAX);
            return 0;
        }

        if (_wcsicmp(argv[i], L"/help") == 0)
        {
            /* Print full help text*/
            PrintResourceString(IDS_ACCOUNTS_HELP);
            return 0;
        }

        if (_wcsicmp(argv[i], L"/domain") == 0)
        {
            PrintResourceString(IDS_ERROR_OPTION_NOT_SUPPORTED, L"/DOMAIN");
#if 0
            Domain = TRUE;
#endif
        }
    }

    Status = NetUserModalsGet(NULL, 0, (LPBYTE*)&Info0);
    if (Status != NERR_Success)
        goto done;

    for (i = 2; i < argc; i++)
    {
        if (_wcsnicmp(argv[i], L"/forcelogoff:", 13) == 0)
        {
            p = &argv[i][13];
            if (wcsicmp(p, L"no"))
            {
                Info0->usrmod0_force_logoff = TIMEQ_FOREVER;
                Modified = TRUE;
            }
            else
            {
                value = wcstoul(p, &endptr, 10);
                if (*endptr != 0)
                {
                    PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"/FORCELOGOFF");
                    result = 1;
                    goto done;
                }

                Info0->usrmod0_force_logoff = value * 60;
                Modified = TRUE;
            }
        }
        else if (_wcsnicmp(argv[i], L"/minpwlen:", 10) == 0)
        {
            p = &argv[i][10];
            value = wcstoul(p, &endptr, 10);
            if (*endptr != 0)
            {
                    PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"/MINPWLEN");
                    result = 1;
                    goto done;
            }

            Info0->usrmod0_min_passwd_len = value;
            Modified = TRUE;
        }
        else if (_wcsnicmp(argv[i], L"/maxpwage:", 10) == 0)
        {
            p = &argv[i][10];

            if (wcsicmp(p, L"unlimited"))
            {
                Info0->usrmod0_max_passwd_age = ULONG_MAX;
                Modified = TRUE;
            }
            else
            {
                value = wcstoul(p, &endptr, 10);
                if (*endptr != 0)
                {
                    PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"/MAXPWLEN");
                    result = 1;
                    goto done;
                }

                Info0->usrmod0_max_passwd_age = value * 86400;
                Modified = TRUE;
            }
        }
        else if (_wcsnicmp(argv[i], L"/minpwage:", 10) == 0)
        {
            p = &argv[i][10];
            value = wcstoul(p, &endptr, 10);
            if (*endptr != 0)
            {
                PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"/MINPWAGE");
                result = 1;
                goto done;
            }

            Info0->usrmod0_min_passwd_age = value * 86400;
            Modified = TRUE;
        }
        else if (_wcsnicmp(argv[i], L"/uniquepw:", 10) == 0)
        {
            p = &argv[i][10];
            value = wcstoul(p, &endptr, 10);
            if (*endptr != 0)
            {
                PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"/UNIQUEPW");
                result = 1;
                goto done;
            }

            Info0->usrmod0_password_hist_len = value;
            Modified = TRUE;
        }
    }

    if (Modified == TRUE)
    {
        Status = NetUserModalsSet(NULL, 0, (LPBYTE)Info0, &ParamErr);
        if (Status != NERR_Success)
            goto done;
    }
    else
    {
        Status = NetUserModalsGet(NULL, 1, (LPBYTE*)&Info1);
        if (Status != NERR_Success)
            goto done;

        Status = NetUserModalsGet(NULL, 3, (LPBYTE*)&Info3);
        if (Status != NERR_Success)
            goto done;

        RtlGetNtProductType(&ProductType);

        PrintPaddedResourceString(IDS_ACCOUNTS_FORCE_LOGOFF, nPaddedLength);
        if (Info0->usrmod0_force_logoff == TIMEQ_FOREVER)
            PrintResourceString(IDS_GENERIC_NEVER);
        else
            PrintResourceString(IDS_ACCOUNTS_LOGOFF_SECONDS, Info0->usrmod0_force_logoff);
        PrintToConsole(L"\n");

        PrintPaddedResourceString(IDS_ACCOUNTS_MIN_PW_AGE, nPaddedLength);
        PrintToConsole(L"%lu\n", Info0->usrmod0_min_passwd_age / 86400);

        PrintPaddedResourceString(IDS_ACCOUNTS_MAX_PW_AGE, nPaddedLength);
        PrintToConsole(L"%lu\n", Info0->usrmod0_max_passwd_age / 86400);

        PrintPaddedResourceString(IDS_ACCOUNTS_MIN_PW_LENGTH, nPaddedLength);
        PrintToConsole(L"%lu\n", Info0->usrmod0_min_passwd_len);

        PrintPaddedResourceString(IDS_ACCOUNTS_PW_HIST_LENGTH, nPaddedLength);
        if (Info0->usrmod0_password_hist_len == 0)
            PrintResourceString(IDS_GENERIC_NONE);
        else
            PrintToConsole(L"%lu", Info0->usrmod0_password_hist_len);
        PrintToConsole(L"\n");

        PrintPaddedResourceString(IDS_ACCOUNTS_LOCKOUT_THRESHOLD, nPaddedLength);
        if (Info3->usrmod3_lockout_threshold == 0)
            PrintResourceString(IDS_GENERIC_NEVER);
        else
            PrintToConsole(L"%lu", Info3->usrmod3_lockout_threshold);
        PrintToConsole(L"\n");

        PrintPaddedResourceString(IDS_ACCOUNTS_LOCKOUT_DURATION, nPaddedLength);
        PrintToConsole(L"%lu\n", Info3->usrmod3_lockout_duration / 60);

        PrintPaddedResourceString(IDS_ACCOUNTS_LOCKOUT_WINDOW, nPaddedLength);
        PrintToConsole(L"%lu\n", Info3->usrmod3_lockout_observation_window / 60);

        PrintPaddedResourceString(IDS_ACCOUNTS_COMPUTER_ROLE, nPaddedLength);
        if (Info1->usrmod1_role == UAS_ROLE_PRIMARY)
        {
            if (ProductType == NtProductLanManNt)
            {
                PrintResourceString(IDS_ACCOUNTS_PRIMARY_SERVER);
            }
            else if (ProductType == NtProductServer)
            {
                PrintResourceString(IDS_ACCOUNTS_STANDALONE_SERVER);
            }
            else
            {
                PrintResourceString(IDS_ACCOUNTS_WORKSTATION);
            }
        }
        else
        {
            PrintResourceString(IDS_ACCOUNTS_BACKUP_SERVER);
        }
        PrintToConsole(L"\n");
    }

done:
    if (Info3 != NULL)
        NetApiBufferFree(Info3);

    if (Info1 != NULL)
        NetApiBufferFree(Info1);

    if (Info0 != NULL)
        NetApiBufferFree(Info0);

    return result;
}

/* EOF */
