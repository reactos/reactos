/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdShare.c
 * PROGRAMMERS:     Eric Kohl <eric.kohl@reactos.org>
 */

#include "net.h"


NET_API_STATUS
EnumerateShares(VOID)
{
    PSHARE_INFO_2 pBuffer = NULL;
    DWORD dwRead = 0, dwTotal = 0;
    DWORD ResumeHandle = 0, i;
    NET_API_STATUS Status;

    ConPuts(StdOut, L"\n");
    PrintMessageString(4730);
    ConPuts(StdOut, L"\n");
    PrintPadding(L'-', 79);
    ConPuts(StdOut, L"\n");

    do
    {
        Status = NetShareEnum(NULL,
                              2,
                              (LPBYTE*)&pBuffer,
                              MAX_PREFERRED_LENGTH,
                              &dwRead,
                              &dwTotal,
                              &ResumeHandle);
        if ((Status != NERR_Success) && (Status != ERROR_MORE_DATA))
            return Status;

        for (i = 0; i < dwRead; i++)
        {
            ConPrintf(StdOut, L"%-12s %-31s %s\n", pBuffer[i].shi2_netname, pBuffer[i].shi2_path, pBuffer[i].shi2_remark);
        }

        NetApiBufferFree(pBuffer);
        pBuffer = NULL;
    }
    while (Status == ERROR_MORE_DATA);

    return NERR_Success;
}


NET_API_STATUS
DisplayShare(
    PWSTR pShareName)
{
    PSHARE_INFO_2 pBuffer = NULL;
    INT nPaddedLength = 22;
    NET_API_STATUS Status;

    Status = NetShareGetInfo(NULL,
                             pShareName,
                             2,
                             (LPBYTE*)&pBuffer);
    if (Status != NERR_Success)
        return Status;

    PrintPaddedMessageString(4731, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pBuffer->shi2_netname);

    PrintPaddedMessageString(4339, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pBuffer->shi2_path);

    PrintPaddedMessageString(4334, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pBuffer->shi2_remark);

    PrintPaddedMessageString(4735, nPaddedLength);
    if (pBuffer->shi2_max_uses == (DWORD)-1)
        PrintMessageString(4736);
    else
        ConPrintf(StdOut, L"%lu", pBuffer->shi2_max_uses);
    ConPrintf(StdOut, L"\n");

    PrintPaddedMessageString(4737, nPaddedLength);
    if (pBuffer->shi2_current_uses > 0)
        ConPrintf(StdOut, L"%lu", pBuffer->shi2_current_uses);
    ConPrintf(StdOut, L"\n");

    NetApiBufferFree(pBuffer);

    return NERR_Success;
}


INT
cmdShare(
    INT argc,
    WCHAR **argv)
{
    PWSTR pShareName = NULL;
    INT i, result = 0;
    NET_API_STATUS Status;

    i = 2;
    if (argv[i][0] != L'/')
    {
        pShareName = argv[i];
        i++;
    }

    for (; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"/help") == 0)
        {
            /* Print full help text*/
            PrintMessageString(4381);
            ConPuts(StdOut, L"\n");
            PrintNetMessage(MSG_SHARE_SYNTAX);
            PrintNetMessage(MSG_SHARE_HELP);
            return 0;
        }
    }

    if (pShareName == NULL)
    {
        Status = EnumerateShares();
        ConPrintf(StdOut, L"Status: %lu\n", Status);
    }
    else
    {
        Status = DisplayShare(pShareName);
        ConPrintf(StdOut, L"Status: %lu\n", Status);
    }

    return result;
}

/* EOF */
