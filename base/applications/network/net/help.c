/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:
 * PURPOSE:
 *
 * PROGRAMMERS:     Magnus Olsen (greatlord@reactos.org)
 */

#include "net.h"

INT cmdHelp(INT argc, WCHAR **argv)
{
    if (argc > 3)
    {
      return 0;
    }

    if (_wcsicmp(argv[2],L"ACCOUNTS")==0)
    {
        puts("ACCOUNTS");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"COMPUTER")==0)
    {
        puts("COMPUTER");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"CONFIG")==0)
    {
        puts("CONFIG");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"CONTINUE")==0)
    {
        puts("CONTINUE");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"FILE")==0)
    {
        puts("FILE");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"GROUP")==0)
    {
        puts("GROUP");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"HELP")==0)
    {
        puts("HELP");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"HELPMSG")==0)
    {
        puts("HELPMSG");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"LOCALGROUP")==0)
    {
        puts("LOCALGROUP");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"NAME")==0)
    {
        puts("NAME");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"PRINT")==0)
    {
        puts("PRINT");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"SEND")==0)
    {
        puts("SEND");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"SESSION")==0)
    {
        puts("SESSION");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"SHARE")==0)
    {
        puts("SHARE");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"START")==0)
    {
        puts("START");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"STATISTICS")==0)
    {
        puts("STATISTICS");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"STOP")==0)
    {
        puts("STOP");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"TIME")==0)
    {
        puts("TIME");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"USE")==0)
    {
        puts("USE");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"USER")==0)
    {
        puts("USER");
        puts("help text");
        return 0;
    }

    if (_wcsicmp(argv[2],L"VIEW")==0)
    {
        puts("VIEW");
        puts("help text");
        return 0;
    }

    help();
    return 0;
}


VOID help(VOID)
{
    puts("NET ACCOUNTS");
    puts("NET COMPUTER");
    puts("NET CONFIG");
    puts("NET CONFIG SERVER");
    puts("NET CONFIG WORKSTATION");
    puts("NET CONTINUE");
    puts("NET FILE");
    puts("NET GROUP");

    puts("NET HELP");
    puts("NET HELPMSG");
    puts("NET LOCALGROUP");
    puts("NET NAME");
    puts("NET PAUSE");
    puts("NET PRINT");
    puts("NET SEND");
    puts("NET SESSION");

    puts("NET SHARE");
    puts("NET START");
    puts("NET STATISTICS");
    puts("NET STOP");
    puts("NET TIME");
    puts("NET USE");
    puts("NET USER");
    puts("NET VIEW");
}
