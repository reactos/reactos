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
    INT i, result = 0;
    BOOL bAdd = FALSE;
    BOOL bDelete = FALSE;
    PWSTR pComputerName = NULL;
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
    if (argc > 2 && argv[i][0] != L'\\' && argv[i][1] != L'\\')
    {
        pComputerName = argv[i];
        i++;
    }

    for (; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"help") == 0)
        {
            /* Print short syntax help */
            ConResPuts(StdOut, IDS_GENERIC_SYNTAX);
            PrintNetMessage(MSG_COMPUTER_SYNTAX);
            return 0;
        }

        if (_wcsicmp(argv[i], L"/help") == 0)
        {
            /* Print full help text*/
            ConResPuts(StdOut, IDS_GENERIC_SYNTAX);
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
        ConResPuts(StdOut, IDS_GENERIC_SYNTAX);
        PrintNetMessage(MSG_COMPUTER_SYNTAX);
        return 1;
    }

    if (bAdd)
    {
        printf("Add %S (not implemented yet)\n", pComputerName);
    }
    else if (bDelete)
    {
        printf("Delete %S (not implemented yet)\n", pComputerName);
    }

    if (result == 0)
        PrintErrorMessage(ERROR_SUCCESS);

    return result;
}

/* EOF */
