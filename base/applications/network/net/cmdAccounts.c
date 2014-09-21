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
//    BOOL Domain = FALSE;
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

/*
        if (_wcsicmp(argv[i], L"/domain") == 0)
        {
            Domain = TRUE;
        }
*/
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
                    printf("You entered an invalid value for the /FORCELOGOFF option.\n");
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
                    printf("You entered an invalid value for the /MINPWLEN option.\n");
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
                    printf("You entered an invalid value for the /MAXPWAGE option.\n");
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
                printf("You entered an invalid value for the /MINPWAGE option.\n");
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
                printf("You entered an invalid value for the /UNIQUEPW option.\n");
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

        printf("Force logoff after: ");
        if (Info0->usrmod0_force_logoff == TIMEQ_FOREVER)
            printf("Never\n");
        else
            printf("%lu seconds\n", Info0->usrmod0_force_logoff);

        printf("Minimum password age (in days): %lu\n", Info0->usrmod0_min_passwd_age / 86400);
        printf("Maximum password age (in days): %lu\n", Info0->usrmod0_max_passwd_age / 86400);
        printf("Minimum password length: %lu\n", Info0->usrmod0_min_passwd_len);

        printf("Password history length: ");
        if (Info0->usrmod0_password_hist_len == 0)
            printf("None\n");
        else
            printf("%lu\n", Info0->usrmod0_password_hist_len);

        printf("Lockout threshold: ");
        if (Info3->usrmod3_lockout_threshold == 0)
            printf("Never\n");
        else
            printf("%lu\n", Info3->usrmod3_lockout_threshold);

        printf("Lockout duration (in minutes): %lu\n", Info3->usrmod3_lockout_duration / 60);
        printf("Lockout observation window (in minutes): %lu\n", Info3->usrmod3_lockout_observation_window / 60);

        printf("Computer role: ");

        if (Info1->usrmod1_role == UAS_ROLE_PRIMARY)
        {
            if (ProductType == NtProductLanManNt)
            {
                printf("Primary server\n");
            }
            else if (ProductType == NtProductServer)
            {
                printf("Standalone server\n");
            }
            else
            {
                printf("Workstation\n");
            }
        }
        else
        {
            printf("Backup server\n");
        }
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
