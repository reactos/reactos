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
        ConResPuts(StdOut, IDS_HELP_SYNTAX);
        return 0;
    }

    if (_wcsicmp(argv[2],L"ACCOUNTS") == 0)
    {
        ConResPuts(StdOut, IDS_ACCOUNTS_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"COMPUTER") == 0)
    {
        ConResPuts(StdOut, IDS_COMPUTER_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"CONFIG") == 0)
    {
        ConResPuts(StdOut, IDS_CONFIG_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"CONTINUE") == 0)
    {
        ConResPuts(StdOut, IDS_CONTINUE_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"FILE") == 0)
    {
        ConResPuts(StdOut, IDS_FILE_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"GROUP") == 0)
    {
        ConResPuts(StdOut, IDS_GROUP_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"HELPMSG") == 0)
    {
        ConResPuts(StdOut, IDS_HELPMSG_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"LOCALGROUP") == 0)
    {
        ConResPuts(StdOut, IDS_LOCALGROUP_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"NAME") == 0)
    {
        ConResPuts(StdOut, IDS_NAME_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"PAUSE") == 0)
    {
        ConResPuts(StdOut, IDS_PAUSE_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"PRINT") == 0)
    {
        ConResPuts(StdOut, IDS_PRINT_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"SEND") == 0)
    {
        ConResPuts(StdOut, IDS_SEND_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"SESSION") == 0)
    {
        ConResPuts(StdOut, IDS_SESSION_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"SHARE") == 0)
    {
        ConResPuts(StdOut, IDS_SHARE_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"START") == 0)
    {
        ConResPuts(StdOut, IDS_START_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"STATISTICS") == 0)
    {
        ConResPuts(StdOut, IDS_STATISTICS_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"STOP") == 0)
    {
        ConResPuts(StdOut, IDS_STOP_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"TIME") == 0)
    {
        ConResPuts(StdOut, IDS_TIME_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"USE") == 0)
    {
        ConResPuts(StdOut, IDS_USE_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"USER") == 0)
    {
        ConResPuts(StdOut, IDS_USER_HELP);
        return 0;
    }

    if (_wcsicmp(argv[2],L"VIEW") == 0)
    {
        ConResPuts(StdOut, IDS_VIEW_HELP);
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

    ConResPuts(StdOut, IDS_HELP_SYNTAX);

    return 0;
}

