/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdConfig.c
 * PROGRAMMERS:     Eric Kohl <eric.kohl@reactos.org>
 */

#include "net.h"

static
INT
DisplayServerConfig(
    PSERVER_INFO_102 ServerInfo)
{
    PSERVER_TRANSPORT_INFO_0 TransportInfo = NULL;
    DWORD dwRead, dwTotal, i;
    INT nPaddedLength = 38;
    NET_API_STATUS Status;

    Status = NetServerTransportEnum(NULL, 0, (PBYTE*)&TransportInfo,
                                    MAX_PREFERRED_LENGTH,
                                    &dwRead,
                                    &dwTotal,
                                    NULL);
    if (Status != NERR_Success)
        goto done;

    PrintPaddedMessageString(4481, nPaddedLength);
    ConPrintf(StdOut, L"\\\\%s\n", ServerInfo->sv102_name);

    PrintPaddedMessageString(4482, nPaddedLength);
    ConPrintf(StdOut, L"%s\n\n", ServerInfo->sv102_comment);

    PrintPaddedMessageString(4484, nPaddedLength);
    ConPrintf(StdOut, L"%lu.%lu\n",
              ServerInfo->sv102_version_major,
              ServerInfo->sv102_version_minor);

    PrintPaddedMessageString(4489, nPaddedLength);
    ConPuts(StdOut, L"\n");
    for (i = 0; i < dwRead; i++)
    {
        ConPrintf(StdOut, L"      %s (%s)\n",
                  &TransportInfo[i].svti0_transportname[8],
                  TransportInfo[i].svti0_networkaddress);
    }
    ConPuts(StdOut, L"\n");

    PrintPaddedMessageString(4492, nPaddedLength);
    PrintMessageString((ServerInfo->sv102_hidden == SV_HIDDEN) ? 4300 : 4301);
    ConPuts(StdOut, L"\n");

    PrintPaddedMessageString(4506, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", ServerInfo->sv102_users);

    PrintPaddedMessageString(4511, nPaddedLength);
    ConPuts(StdOut, L"...\n\n");

    PrintPaddedMessageString(4520, nPaddedLength);
    if (ServerInfo->sv102_disc == SV_NODISC)
        PrintMessageString(4306);
    else
        ConPrintf(StdOut, L"%lu\n", ServerInfo->sv102_disc);

done:
    if (TransportInfo != NULL)
        NetApiBufferFree(TransportInfo);

    return 0;
}


static
INT
DisplayWorkstationConfig(VOID)
{
    PWKSTA_INFO_100 WorkstationInfo = NULL;
    PWKSTA_USER_INFO_1 UserInfo = NULL;
    PWKSTA_TRANSPORT_INFO_0 TransportInfo = NULL;
    DWORD dwRead = 0, dwTotal = 0, i;
    INT nPaddedLength = 38;
    NET_API_STATUS Status;

    Status = NetWkstaGetInfo(NULL, 100, (PBYTE*)&WorkstationInfo);
    if (Status != NERR_Success)
        goto done;

    Status = NetWkstaUserGetInfo(NULL, 1, (PBYTE*)&UserInfo);
    if (Status != NERR_Success)
        goto done;

    Status = NetWkstaTransportEnum(NULL,
                                   0,
                                   (PBYTE*)&TransportInfo,
                                   MAX_PREFERRED_LENGTH,
                                   &dwRead,
                                   &dwTotal,
                                   NULL);
    if (Status != NERR_Success)
        goto done;

    PrintPaddedMessageString(4450, nPaddedLength);
    ConPrintf(StdOut, L"\\\\%s\n", WorkstationInfo->wki100_computername);

    PrintPaddedMessageString(4468, nPaddedLength);
    ConPuts(StdOut, L"...\n");

    PrintPaddedMessageString(4451, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", UserInfo->wkui1_username);

    ConPuts(StdOut, L"\n");

    PrintPaddedMessageString(4453, nPaddedLength);
    ConPuts(StdOut, L"\n");
    for (i = 0; i < dwRead; i++)
    {
        ConPrintf(StdOut, L"      %s (%s)\n",
                  &TransportInfo[i].wkti0_transport_name[8],
                  TransportInfo[i].wkti0_transport_address);
    }
    ConPuts(StdOut, L"\n");

    PrintPaddedMessageString(4452, nPaddedLength);
    ConPrintf(StdOut, L"%lu.%lu\n",
              WorkstationInfo->wki100_ver_major,
              WorkstationInfo->wki100_ver_minor);

    ConPuts(StdOut, L"\n");

    PrintPaddedMessageString(4455, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", WorkstationInfo->wki100_langroup);

    PrintPaddedMessageString(4469, nPaddedLength);
    ConPuts(StdOut, L"...\n");

    PrintPaddedMessageString(4456, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", UserInfo->wkui1_logon_domain);

    ConPuts(StdOut, L"\n");

    PrintPaddedMessageString(4458, nPaddedLength);
    ConPuts(StdOut, L"...\n");

    PrintPaddedMessageString(4459, nPaddedLength);
    ConPuts(StdOut, L"...\n");

    PrintPaddedMessageString(4460, nPaddedLength);
    ConPuts(StdOut, L"...\n");

done:
    if (TransportInfo != NULL)
        NetApiBufferFree(TransportInfo);

    if (UserInfo != NULL)
        NetApiBufferFree(UserInfo);

    if (WorkstationInfo != NULL)
        NetApiBufferFree(WorkstationInfo);

    return 0;
}


INT
cmdConfig(
    INT argc,
    WCHAR **argv)
{
    INT i, result = 0;
    BOOL bServer = FALSE;
    BOOL bWorkstation = FALSE;
    PWSTR p, endptr;
    BOOL bModify = FALSE;
    LONG lValue;
    PSERVER_INFO_102 ServerInfo = NULL;
    NET_API_STATUS Status;

    for (i = 2; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"server") == 0)
        {
            if (bWorkstation == FALSE)
                bServer = TRUE;
            continue;
        }

        if (_wcsicmp(argv[i], L"workstation") == 0)
        {
            if (bServer == FALSE)
                bWorkstation = TRUE;
            continue;
        }

        if (_wcsicmp(argv[i], L"help") == 0)
        {
            /* Print short syntax help */
            if (bServer == TRUE)
            {
                PrintMessageString(4381);
                ConPuts(StdOut, L"\n");
                PrintNetMessage(MSG_CONFIG_SERVER_SYNTAX);
            }
            else
            {
                PrintMessageString(4381);
                ConPuts(StdOut, L"\n");
                PrintNetMessage(MSG_CONFIG_SYNTAX);
            }
            return 0;
        }

        if (_wcsicmp(argv[i], L"/help") == 0)
        {
            /* Print full help text*/
            if (bServer == TRUE)
            {
                PrintMessageString(4381);
                ConPuts(StdOut, L"\n");
                PrintNetMessage(MSG_CONFIG_SERVER_SYNTAX);
                PrintNetMessage(MSG_CONFIG_SERVER_HELP);
            }
            else
            {
                PrintMessageString(4381);
                ConPuts(StdOut, L"\n");
                PrintNetMessage(MSG_CONFIG_SYNTAX);
                PrintNetMessage(MSG_CONFIG_HELP);
            }
            return 0;
        }
    }

    if (bServer)
    {
        Status = NetServerGetInfo(NULL, 102, (PBYTE*)&ServerInfo);
        if (Status != NERR_Success)
            goto done;

        for (i = 2; i < argc; i++)
        {
            if (argv[i][0] != L'/')
                continue;

            if (_wcsnicmp(argv[i], L"/autodisconnect:", 16) == 0)
            {
                p = &argv[i][16];
                lValue = wcstol(p, &endptr, 10);
                if (*endptr != 0)
                {
                    PrintMessageStringV(3952, L"/AUTODISCONNECT");
                    result = 1;
                    goto done;
                }

                if (lValue < -1 || lValue > 65535)
                {
                    PrintMessageStringV(3952, L"/AUTODISCONNECT");
                    result = 1;
                    goto done;
                }

                ServerInfo->sv102_disc = lValue;
                bModify = TRUE;
            }
            else if (_wcsnicmp(argv[i], L"/srvcomment:", 12) == 0)
            {
                ServerInfo->sv102_comment = &argv[i][12];
                bModify = TRUE;
            }
            else if (_wcsnicmp(argv[i], L"/hidden:", 8) == 0)
            {
                p = &argv[i][8];
                if (_wcsicmp(p, L"yes") != 0 && _wcsicmp(p, L"no") != 0)
                {
                    PrintMessageStringV(3952, L"/HIDDEN");
                    result = 1;
                    goto done;
                }

                ServerInfo->sv102_hidden = (_wcsicmp(p, L"yes") == 0) ? TRUE : FALSE;
                bModify = TRUE;
            }
            else
            {
                PrintMessageString(4381);
                ConPuts(StdOut, L"\n");
                PrintNetMessage(MSG_CONFIG_SERVER_SYNTAX);
                result = 1;
                goto done;
            }
        }

        if (bModify)
        {
            Status = NetServerSetInfo(NULL, 102, (PBYTE)&ServerInfo, NULL);
            if (Status != NERR_Success)
                result = 1;
        }
        else
        {
            result = DisplayServerConfig(ServerInfo);
        }
    }
    else if (bWorkstation)
    {
        result = DisplayWorkstationConfig();
    }
    else
    {
        PrintMessageString(4378);
        ConPuts(StdOut, L"\n");
        ConPuts(StdOut, L"   Server\n");
        ConPuts(StdOut, L"   Workstation\n");
        ConPuts(StdOut, L"\n");
    }

done:
    if (ServerInfo != NULL)
        NetApiBufferFree(ServerInfo);

    if (result == 0)
        PrintErrorMessage(ERROR_SUCCESS);

    return result;
}
