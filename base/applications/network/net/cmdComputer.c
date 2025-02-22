/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdComputer.c
 * PROGRAMMERS:     Eric Kohl <eric.kohl@reactos.org>
 */

#include "net.h"

INT
cmdComputer(
    INT argc,
    WCHAR **argv)
{
    WCHAR ComputerAccountName[MAX_PATH + 2];
    WCHAR ComputerPassword[LM20_PWLEN + 1];
    USER_INFO_1 UserInfo;
    INT i, result = 0;
    BOOL bAdd = FALSE;
    BOOL bDelete = FALSE;
    PWSTR pComputerName = NULL;
    NET_API_STATUS Status = NERR_Success;
/*
    OSVERSIONINFOEX VersionInfo;

    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
    if (!GetVersionEx((LPOSVERSIONINFO)&VersionInfo))
    {
        PrintErrorMessage(GetLastError());
        return 1;
    }

    if (VersionInfo.wProductType != VER_NT_DOMAIN_CONTROLLER)
    {
        PrintErrorMessage(3515);
        return 1;
    }
*/

    i = 2;
    if (argc > 2 && argv[i][0] == L'\\' && argv[i][1] == L'\\')
    {
        pComputerName = argv[i];
        i++;
    }

    for (; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"help") == 0)
        {
            /* Print short syntax help */
            PrintMessageString(4381);
            ConPuts(StdOut, L"\n");
            PrintNetMessage(MSG_COMPUTER_SYNTAX);
            return 0;
        }

        if (_wcsicmp(argv[i], L"/help") == 0)
        {
            /* Print full help text*/
            PrintMessageString(4381);
            ConPuts(StdOut, L"\n");
            PrintNetMessage(MSG_COMPUTER_SYNTAX);
            PrintNetMessage(MSG_COMPUTER_HELP);
            return 0;
        }

        if (_wcsicmp(argv[i], L"/add") == 0)
        {
            bAdd = TRUE;
            continue;
        }
        else if (_wcsicmp(argv[i], L"/del") == 0)
        {
            bDelete = TRUE;
            continue;
        }
        else
        {
            PrintErrorMessage(3506/*, argv[i]*/);
            return 1;
        }
    }

    if (pComputerName == NULL ||
        (bAdd == FALSE && bDelete == FALSE) ||
        (bAdd == TRUE && bDelete == TRUE))
    {
        PrintMessageString(4381);
        ConPuts(StdOut, L"\n");
        PrintNetMessage(MSG_COMPUTER_SYNTAX);
        return 1;
    }

    /*
     * Create the computer account name:
     *  Skip the leading '\\' and appand a '$'.
     */
    wcscpy(ComputerAccountName, &pComputerName[2]);
    wcscat(ComputerAccountName, L"$");

    if (bAdd)
    {
        /*
         * Create the computer password:
         *   Skip the leading '\\', shorten to a maximum of 14 characters
         *   and convert to lower case
         */
        wcsncpy(ComputerPassword, &pComputerName[2], LM20_PWLEN);
        ComputerPassword[LM20_PWLEN] = UNICODE_NULL;
        _wcslwr(ComputerPassword);

        /* Set the account data */
        UserInfo.usri1_name = ComputerAccountName;
        UserInfo.usri1_password = ComputerPassword;
        UserInfo.usri1_password_age = 0;
        UserInfo.usri1_priv = USER_PRIV_USER;
        UserInfo.usri1_home_dir = NULL;
        UserInfo.usri1_comment = NULL;
        UserInfo.usri1_flags = UF_SCRIPT | UF_WORKSTATION_TRUST_ACCOUNT;
        UserInfo.usri1_script_path = NULL;

        /* Add the computer account */
        Status = NetUserAdd(NULL,
                            1,
                            (LPBYTE)&UserInfo,
                            NULL);
    }
    else if (bDelete)
    {
        /* Delete the coputer account */
        Status = NetUserDel(NULL,
                            ComputerAccountName);
    }

    if (Status == NERR_Success)
    {
        PrintErrorMessage(ERROR_SUCCESS);
    }
    else
    {
        PrintErrorMessage(Status);
        result = 1;
    }

    return result;
}

/* EOF */
