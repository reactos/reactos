/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdConfig.c
 * PROGRAMMERS:     Eric Kohl <eric.kohl@reactos.org>
 */

#include "net.h"

static
INT
DisplayServerConfig(VOID)
{
    PSERVER_INFO_102 ServerInfo = NULL;
    PSERVER_TRANSPORT_INFO_0 TransportInfo = NULL;
    DWORD dwRead, dwTotal, i;
    INT nPaddedLength = 38;
    NET_API_STATUS Status;

    Status = NetServerGetInfo(NULL, 102, (PBYTE*)&ServerInfo);
    if (Status != NERR_Success)
        goto done;

    Status = NetServerTransportEnum(NULL, 0, (PBYTE*)&TransportInfo,
                                    MAX_PREFERRED_LENGTH,
                                    &dwRead,
                                    &dwTotal,
                                    NULL);
    if (Status != NERR_Success)
        goto done;

    PrintPaddedResourceString(IDS_CONFIG_SERVER_NAME, nPaddedLength);
    ConPrintf(StdOut, L"\\\\%s\n", ServerInfo->sv102_name);

    PrintPaddedResourceString(IDS_CONFIG_SERVER_COMMENT, nPaddedLength);
    ConPrintf(StdOut, L"%s\n\n", ServerInfo->sv102_comment);

    PrintPaddedResourceString(IDS_CONFIG_SERVER_VERSION, nPaddedLength);
    ConPuts(StdOut, L"...\n");

    ConResPuts(StdOut, IDS_CONFIG_SERVER_ACTIVE);
    for (i = 0; i < dwTotal; i++)
    {
        ConPrintf(StdOut, L"      %s (%s)\n",
                  &TransportInfo[i].svti0_transportname[8],
                  TransportInfo[i].svti0_networkaddress);
    }
    ConPuts(StdOut, L"\n");

    PrintPaddedResourceString(IDS_CONFIG_SERVER_HIDDEN, nPaddedLength);
    ConResPuts(StdOut, (ServerInfo->sv102_hidden == SV_HIDDEN) ? IDS_GENERIC_YES : IDS_GENERIC_NO);
    ConPuts(StdOut, L"\n");

    PrintPaddedResourceString(IDS_CONFIG_SERVER_USERS, nPaddedLength);
    ConPrintf(StdOut, L"%lu\n", ServerInfo->sv102_users);

    PrintPaddedResourceString(IDS_CONFIG_SERVER_FILES, nPaddedLength);
    ConPuts(StdOut, L"...\n\n");

    PrintPaddedResourceString(IDS_CONFIG_SERVER_IDLE, nPaddedLength);
    if (ServerInfo->sv102_disc == SV_NODISC)
        ConResPuts(StdOut, IDS_GENERIC_UNLIMITED);
    else
        ConPrintf(StdOut, L"%lu\n", ServerInfo->sv102_disc);

done:
    if (TransportInfo != NULL)
        NetApiBufferFree(TransportInfo);

    if (ServerInfo != NULL)
        NetApiBufferFree(ServerInfo);

    return 0;
}


static
INT
DisplayWorkstationConfig(VOID)
{
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
                ConResPuts(StdOut, IDS_GENERIC_SYNTAX);
                ConResPuts(StdOut, IDS_CONFIG_SERVER_SYNTAX);
            }
            else
            {
                ConResPuts(StdOut, IDS_GENERIC_SYNTAX);
                ConResPuts(StdOut, IDS_CONFIG_SYNTAX);
            }
            return 0;
        }

        if (_wcsicmp(argv[i], L"/help") == 0)
        {
            /* Print full help text*/
            if (bServer == TRUE)
            {
                ConResPuts(StdOut, IDS_GENERIC_SYNTAX);
                ConResPuts(StdOut, IDS_CONFIG_SERVER_SYNTAX);
                ConResPuts(StdOut, IDS_CONFIG_SERVER_HELP_1);
                ConResPuts(StdOut, IDS_CONFIG_SERVER_HELP_2);
                ConResPuts(StdOut, IDS_CONFIG_SERVER_HELP_3);
                ConResPuts(StdOut, IDS_CONFIG_SERVER_HELP_4);
                ConResPuts(StdOut, IDS_CONFIG_SERVER_HELP_5);
                ConResPuts(StdOut, IDS_GENERIC_PAGE);
            }
            else
            {
                ConResPuts(StdOut, IDS_GENERIC_SYNTAX);
                ConResPuts(StdOut, IDS_CONFIG_SYNTAX);
                ConResPuts(StdOut, IDS_CONFIG_HELP_1);
                ConResPuts(StdOut, IDS_CONFIG_HELP_2);
            }
            return 0;
        }
    }

    if (bServer)
    {
        result = DisplayServerConfig();
    }
    else if (bWorkstation)
    {
        result = DisplayWorkstationConfig();
    }
    else
    {
        ConResPuts(StdOut, IDS_CONFIG_TEXT);
    }

    if (result == 0)
        ConResPuts(StdOut, IDS_ERROR_NO_ERROR);

    return result;
}