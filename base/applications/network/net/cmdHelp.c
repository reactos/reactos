/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdHelp.c
 * PURPOSE:
 *
 * PROGRAMMERS:     Magnus Olsen (greatlord@reactos.org)
 */

#include "net.h"

INT cmdHelp(INT argc, WCHAR **argv)
{
    PrintMessageString(4381);
    ConPuts(StdOut, L"\n");

    if (argc != 3)
    {
        PrintNetMessage(MSG_HELP_SYNTAX);
        PrintNetMessage(MSG_HELP_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"ACCOUNTS") == 0)
    {
        PrintNetMessage(MSG_ACCOUNTS_SYNTAX);
        PrintNetMessage(MSG_ACCOUNTS_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"COMPUTER") == 0)
    {
        PrintNetMessage(MSG_COMPUTER_SYNTAX);
        PrintNetMessage(MSG_COMPUTER_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"CONFIG") == 0)
    {
        if ((argc > 3) && (_wcsicmp(argv[3], L"SERVER") == 0))
        {
            PrintNetMessage(MSG_CONFIG_SERVER_SYNTAX);
            PrintNetMessage(MSG_CONFIG_SERVER_HELP);
            return 0;
        }
        else
        {
            PrintNetMessage(MSG_CONFIG_SYNTAX);
            PrintNetMessage(MSG_CONFIG_HELP);
            return 0;
        }
    }

    if (_wcsicmp(argv[2], L"CONTINUE") == 0)
    {
        PrintNetMessage(MSG_CONTINUE_SYNTAX);
        PrintNetMessage(MSG_CONTINUE_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"FILE") == 0)
    {
        PrintNetMessage(MSG_FILE_SYNTAX);
        PrintNetMessage(MSG_FILE_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"GROUP") == 0)
    {
        PrintNetMessage(MSG_GROUP_SYNTAX);
        PrintNetMessage(MSG_GROUP_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"HELPMSG") == 0)
    {
        PrintNetMessage(MSG_HELPMSG_SYNTAX);
        PrintNetMessage(MSG_HELPMSG_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"LOCALGROUP") == 0)
    {
        PrintNetMessage(MSG_LOCALGROUP_SYNTAX);
        PrintNetMessage(MSG_LOCALGROUP_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"NAME") == 0)
    {
        PrintNetMessage(MSG_NAME_SYNTAX);
        PrintNetMessage(MSG_NAME_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"PAUSE") == 0)
    {
        PrintNetMessage(MSG_PAUSE_SYNTAX);
        PrintNetMessage(MSG_PAUSE_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"PRINT") == 0)
    {
        PrintNetMessage(MSG_PRINT_SYNTAX);
        PrintNetMessage(MSG_PRINT_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"SEND") == 0)
    {
        PrintNetMessage(MSG_SEND_SYNTAX);
        PrintNetMessage(MSG_SEND_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"SESSION") == 0)
    {
        PrintNetMessage(MSG_SESSION_SYNTAX);
        PrintNetMessage(MSG_SESSION_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"SHARE") == 0)
    {
        PrintNetMessage(MSG_SHARE_SYNTAX);
        PrintNetMessage(MSG_SHARE_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"START") == 0)
    {
        PrintNetMessage(MSG_START_SYNTAX);
        PrintNetMessage(MSG_START_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"STATISTICS") == 0)
    {
        PrintNetMessage(MSG_STATISTICS_SYNTAX);
        PrintNetMessage(MSG_STATISTICS_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"STOP") == 0)
    {
        PrintNetMessage(MSG_STOP_SYNTAX);
        PrintNetMessage(MSG_STOP_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"TIME") == 0)
    {
        PrintNetMessage(MSG_TIME_SYNTAX);
        PrintNetMessage(MSG_TIME_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"USE") == 0)
    {
        PrintNetMessage(MSG_USE_SYNTAX);
        PrintNetMessage(MSG_USE_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"USER") == 0)
    {
        PrintNetMessage(MSG_USER_SYNTAX);
        PrintNetMessage(MSG_USER_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2], L"VIEW") == 0)
    {
        PrintNetMessage(MSG_VIEW_SYNTAX);
        PrintNetMessage(MSG_VIEW_HELP);
        return 0;
    }

#if 0
    if (_wcsicmp(argv[2], L"SERVICES") == 0)
    {
        return 0;
    }
#endif

    if (_wcsicmp(argv[2], L"SYNTAX") == 0)
    {
        PrintNetMessage(MSG_SYNTAX_HELP);
        return 0;
    }

    PrintNetMessage(MSG_HELP_SYNTAX);
    PrintNetMessage(MSG_HELP_HELP);

    return 0;
}


INT
cmdSyntax(INT argc, WCHAR **argv)
{
    PrintMessageString(4381);
    ConPuts(StdOut, L"\n");
    PrintNetMessage(MSG_SYNTAX_HELP);
    return 0;
}
