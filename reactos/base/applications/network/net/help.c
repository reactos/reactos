/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/help.c
 * PURPOSE:
 *
 * PROGRAMMERS:     Magnus Olsen (greatlord@reactos.org)
 */

#include "net.h"

INT cmdHelp(INT argc, WCHAR **argv)
{
    if (argc != 3)
    {
        PrintResourceString(IDS_HELP_SYNTAX);
        return 0;
    }

    if (_wcsicmp(argv[2],L"ACCOUNTS") == 0)
    {
        PrintResourceString(IDS_ACCOUNTS_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"COMPUTER") == 0)
    {
        PrintResourceString(IDS_COMPUTER_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"CONFIG") == 0)
    {
        PrintResourceString(IDS_CONFIG_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"CONTINUE") == 0)
    {
        PrintResourceString(IDS_CONTINUE_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"FILE") == 0)
    {
        PrintResourceString(IDS_FILE_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"GROUP") == 0)
    {
        PrintResourceString(IDS_GROUP_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"HELPMSG") == 0)
    {
        PrintResourceString(IDS_HELPMSG_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"LOCALGROUP") == 0)
    {
        PrintResourceString(IDS_LOCALGROUP_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"NAME") == 0)
    {
        PrintResourceString(IDS_NAME_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"PAUSE") == 0)
    {
        PrintResourceString(IDS_PAUSE_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"PRINT") == 0)
    {
        PrintResourceString(IDS_PRINT_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"SEND") == 0)
    {
        PrintResourceString(IDS_SEND_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"SESSION") == 0)
    {
        PrintResourceString(IDS_SESSION_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"SHARE") == 0)
    {
        PrintResourceString(IDS_SHARE_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"START") == 0)
    {
        PrintResourceString(IDS_START_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"STATISTICS") == 0)
    {
        PrintResourceString(IDS_STATISTICS_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"STOP") == 0)
    {
        PrintResourceString(IDS_STOP_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"TIME") == 0)
    {
        PrintResourceString(IDS_TIME_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"USE") == 0)
    {
        PrintResourceString(IDS_USE_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"USER") == 0)
    {
        PrintResourceString(IDS_USER_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"VIEW") == 0)
    {
        PrintResourceString(IDS_VIEW_HELP);
        return 0;
    }

#if 0
    if (_wcsicmp(argv[2],L"SERVICES") == 0)
    {
        return 0;
    }

    if (_wcsicmp(argv[2],L"SYNTAX") == 0)
    {
        return 0;
    }
#endif

    PrintResourceString(IDS_HELP_SYNTAX);

    return 0;
}

