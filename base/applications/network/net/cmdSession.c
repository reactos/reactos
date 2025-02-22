/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdSession.c
 * PROGRAMMERS:     Eric Kohl <eric.kohl@reactos.org>
 */

#include "net.h"

static
VOID
SecondsToDurationString(
    _Out_ PWSTR DurationString,
    _In_ size_t DurationStringSize,
    _In_ DWORD dwDuration)
{
    DWORD dwHours, dwRemainingSeconds, dwMinutes, dwSeconds;

    dwHours = dwDuration / 3600;
    dwRemainingSeconds = dwDuration % 3600;
    dwMinutes = dwRemainingSeconds / 60;
    dwSeconds = dwRemainingSeconds % 60;

    StringCchPrintfW(DurationString, DurationStringSize, L"%02lu:%02lu:%02lu", dwHours, dwMinutes, dwSeconds);
}


NET_API_STATUS
EnumSessions(
    _In_ PWSTR pszComputerName,
    _In_ BOOL bList)
{
    PSESSION_INFO_2 pBuffer = NULL;
    WCHAR DurationBuffer[10];
    DWORD dwRead = 0, dwTotal = 0, i;
    DWORD ResumeHandle = 0;
    NET_API_STATUS Status;

    Status = NetSessionEnum(pszComputerName,
                            NULL,
                            NULL,
                            2,
                            (LPBYTE*)&pBuffer,
                            MAX_PREFERRED_LENGTH,
                            &dwRead,
                            &dwTotal,
                            &ResumeHandle);
    if ((Status != NERR_Success) && (Status != ERROR_MORE_DATA))
    {
//        PrintMessageStringV(3502, Status);
        ConPrintf(StdOut, L"System error %lu has occurred.\n\n", Status);
        return Status;
    }

    if (dwTotal == 0)
    {
        PrintMessageString(3683);
    }
    else
    {
        ConPuts(StdOut, L"\n");
        PrintMessageString(4750);
        PrintPadding(L'-', 79);
        ConPuts(StdOut, L"\n");

        for (i = 0; i < dwRead; i++)
        {
            if (pBuffer[i].sesi2_cname)
            {
                SecondsToDurationString(DurationBuffer,
                                        ARRAYSIZE(DurationBuffer),
                                        pBuffer[i].sesi2_idle_time);

                ConPrintf(StdOut, L"%-22.22s %-20.20s %-17.17s %-5lu %-8.8s\n",
                          pBuffer[i].sesi2_cname,
                          pBuffer[i].sesi2_username,
                          pBuffer[i].sesi2_cltype_name,
                          pBuffer[i].sesi2_num_opens,
                          DurationBuffer);
            }
        }
    }

    NetApiBufferFree(pBuffer);

    return NERR_Success;
}


INT
cmdSession(
    _In_ INT argc,
    _In_ WCHAR **argv)
{
    PWSTR pszComputerName = NULL;
    BOOL bList = FALSE;
    BOOL bDelete = FALSE;
    INT i = 0;
    NET_API_STATUS Status;
    INT result = 0;

    for (i = 2; i < argc; i++)
    {
        if (argv[i][0] == L'\\' && argv[i][1] == L'\\' && pszComputerName == NULL)
        {
            pszComputerName = argv[i];
            i++;
        }
        else if (_wcsicmp(argv[i], L"/list") == 0)
        {
            bList = TRUE;
            continue;
        }
        else if (_wcsicmp(argv[i], L"/delete") == 0)
        {
            bDelete = TRUE;
            continue;
        }
        else if (_wcsicmp(argv[i], L"/help") == 0)
        {
            PrintMessageString(4381);
            ConPuts(StdOut, L"\n");
            PrintNetMessage(MSG_SESSION_SYNTAX);
            PrintNetMessage(MSG_SESSION_HELP);
            return 0;
        }
        else
        {
            PrintMessageString(4381);
            ConPuts(StdOut, L"\n");
            PrintNetMessage(MSG_SESSION_SYNTAX);
            return 1;
        }
    }

    if (bDelete)
        Status = NetSessionDel(pszComputerName, NULL, NULL);
    else
        Status = EnumSessions(pszComputerName, bList);

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
