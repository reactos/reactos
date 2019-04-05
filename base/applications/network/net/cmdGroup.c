/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdGroup.c
 * PROGRAMMERS:     Eric Kohl <eric.kohl@reactos.org>
 */

#include "net.h"


static
int
CompareInfo(const void *a,
            const void *b)
{
    return _wcsicmp(((PGROUP_INFO_0)a)->grpi0_name,
                    ((PGROUP_INFO_0)b)->grpi0_name);
}


static
NET_API_STATUS
EnumerateGroups(VOID)
{
    PGROUP_INFO_0 pBuffer = NULL;
    PSERVER_INFO_100 pServer = NULL;
    DWORD dwRead = 0, dwTotal = 0;
    DWORD i;
    DWORD_PTR ResumeHandle = 0;
    NET_API_STATUS Status;

    Status = NetServerGetInfo(NULL,
                              100,
                              (LPBYTE*)&pServer);
    if (Status != NERR_Success)
        return Status;

    ConPuts(StdOut, L"\n");
    PrintMessageStringV(4400, pServer->sv100_name);
    ConPuts(StdOut, L"\n");
    PrintPadding(L'-', 79);
    ConPuts(StdOut, L"\n");

    NetApiBufferFree(pServer);

    Status = NetGroupEnum(NULL,
                          0,
                          (LPBYTE*)&pBuffer,
                          MAX_PREFERRED_LENGTH,
                          &dwRead,
                          &dwTotal,
                          &ResumeHandle);
    if (Status != NERR_Success)
        return Status;

    qsort(pBuffer,
          dwRead,
          sizeof(PGROUP_INFO_0),
          CompareInfo);

    for (i = 0; i < dwRead; i++)
    {
        if (pBuffer[i].grpi0_name)
            ConPrintf(StdOut, L"*%s\n", pBuffer[i].grpi0_name);
    }

    NetApiBufferFree(pBuffer);

    return NERR_Success;
}


static
NET_API_STATUS
DisplayGroup(LPWSTR lpGroupName)
{
    PGROUP_INFO_1 pGroupInfo = NULL;
    PGROUP_USERS_INFO_0 pUsers = NULL;
    LPWSTR *pNames = NULL;
    DWORD dwRead = 0;
    DWORD dwTotal = 0;
    DWORD_PTR ResumeHandle = 0;
    DWORD i;
    INT nPaddedLength = 15;
    NET_API_STATUS Status;

    Status = NetGroupGetInfo(NULL,
                             lpGroupName,
                             1,
                             (LPBYTE*)&pGroupInfo);
    if (Status != NERR_Success)
        return Status;

    Status = NetGroupGetUsers(NULL,
                              lpGroupName,
                              0,
                              (LPBYTE*)&pUsers,
                              MAX_PREFERRED_LENGTH,
                              &dwRead,
                              &dwTotal,
                              &ResumeHandle);
    if (Status != NERR_Success)
        goto done;

    pNames = RtlAllocateHeap(RtlGetProcessHeap(),
                             HEAP_ZERO_MEMORY,
                             dwRead * sizeof(LPWSTR));
    if (pNames == NULL)
    {
        Status = ERROR_OUTOFMEMORY;
        goto done;
    }

    for (i = 0; i < dwRead; i++)
    {
        pNames[i] = pUsers[i].grui0_name;
    }

    PrintPaddedMessageString(4401, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pGroupInfo->grpi1_name);

    PrintPaddedMessageString(4402, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pGroupInfo->grpi1_comment);

    ConPuts(StdOut, L"\n");

    PrintMessageString(4403);
    ConPuts(StdOut, L"\n");

    PrintPadding(L'-', 79);
    ConPuts(StdOut, L"\n");

    for (i = 0; i < dwRead; i++)
    {
        if (pNames[i])
            ConPrintf(StdOut, L"%s\n", pNames[i]);
    }

done:
    if (pNames != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pNames);

    if (pUsers != NULL)
        NetApiBufferFree(pUsers);

    if (pGroupInfo != NULL)
        NetApiBufferFree(pGroupInfo);

    return Status;
}


INT
cmdGroup(
    INT argc,
    WCHAR **argv)
{
    INT i, j;
    INT result = 0;
    ULONG dwUserCount = 0;
    BOOL bAdd = FALSE;
    BOOL bDelete = FALSE;
#if 0
    BOOL bDomain = FALSE;
#endif
    PWSTR pGroupName = NULL;
    PWSTR pComment = NULL;
    PWSTR *pUsers = NULL;
    GROUP_INFO_0 Info0;
    GROUP_INFO_1 Info1;
    GROUP_INFO_1002 Info1002;
    NET_API_STATUS Status;

    if (argc == 2)
    {
        Status = EnumerateGroups();
        ConPrintf(StdOut, L"Status: %lu\n", Status);
        return 0;
    }
    else if (argc == 3)
    {
        Status = DisplayGroup(argv[2]);
        ConPrintf(StdOut, L"Status: %lu\n", Status);
        return 0;
    }

    i = 2;
    if (argv[i][0] != L'/')
    {
        pGroupName = argv[i];
        i++;
    }

    for (j = i; j < argc; j++)
    {
        if (argv[j][0] == L'/')
            break;

        dwUserCount++;
    }

    if (dwUserCount > 0)
    {
        pUsers = RtlAllocateHeap(RtlGetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 dwUserCount * sizeof(PGROUP_USERS_INFO_0));
        if (pUsers == NULL)
            return 0;
    }

    j = 0;
    for (; i < argc; i++)
    {
        if (argv[i][0] == L'/')
            break;

        pUsers[j] = argv[i];
        j++;
    }

    for (; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"/help") == 0)
        {
            ConResPuts(StdOut, IDS_GENERIC_SYNTAX);
            PrintNetMessage(MSG_GROUP_SYNTAX);
            PrintNetMessage(MSG_GROUP_HELP);
            return 0;
        }
        else if (_wcsicmp(argv[i], L"/add") == 0)
        {
            bAdd = TRUE;
        }
        else if (_wcsicmp(argv[i], L"/delete") == 0)
        {
            bDelete = TRUE;
        }
        else if (_wcsnicmp(argv[i], L"/comment:", 9) == 0)
        {
            pComment = &argv[i][9];
        }
        else if (_wcsicmp(argv[i], L"/domain") == 0)
        {
            ConResPrintf(StdErr, IDS_ERROR_OPTION_NOT_SUPPORTED, L"/DOMAIN");
#if 0
            bDomain = TRUE;
#endif
        }
        else
        {
            PrintErrorMessage(3506/*, argv[i]*/);
            result = 1;
            goto done;
        }
    }

    if (pGroupName == NULL)
    {
        result = 1;
        goto done;
    }

    if (bAdd && bDelete)
    {
        result = 1;
        goto done;
    }

    if (pUsers == NULL)
    {
        if (!bAdd && !bDelete && pComment != NULL)
        {
            /* Set group comment */
            Info1002.grpi1002_comment = pComment;
            Status = NetGroupSetInfo(NULL,
                                     pGroupName,
                                     1002,
                                     (LPBYTE)&Info1002,
                                     NULL);
            ConPrintf(StdOut, L"Status: %lu\n", Status);
        }
        else if (bAdd && !bDelete)
        {
            /* Add the group */
            if (pComment == NULL)
            {
                Info0.grpi0_name = pGroupName;
            }
            else
            {
                Info1.grpi1_name = pGroupName;
                Info1.grpi1_comment = pComment;
            }

            Status = NetGroupAdd(NULL,
                                 (pComment == NULL) ? 0 : 1,
                                 (pComment == NULL) ? (LPBYTE)&Info0 : (LPBYTE)&Info1,
                                 NULL);
            ConPrintf(StdOut, L"Status: %lu\n", Status);
        }
        else if (!bAdd && bDelete && pComment == NULL)
        {
            /* Delete the group */
            Status = NetGroupDel(NULL,
                                 pGroupName);
            ConPrintf(StdOut, L"Status: %lu\n", Status);
        }
        else
        {
            result = 1;
        }
    }
    else
    {
        if (bAdd && !bDelete && pComment == NULL)
        {
            /* Add group user */
            for (i = 0; i < dwUserCount; i++)
            {
                Status = NetGroupAddUser(NULL,
                                         pGroupName,
                                         pUsers[i]);
                if (Status != NERR_Success)
                    break;
            }
            ConPrintf(StdOut, L"Status: %lu\n", Status);
        }
        else if (!bAdd && bDelete && pComment == NULL)
        {
            /* Delete group members */
            for (i = 0; i < dwUserCount; i++)
            {
                Status = NetGroupDelUser(NULL,
                                         pGroupName,
                                         pUsers[i]);
                if (Status != NERR_Success)
                    break;
            }
            ConPrintf(StdOut, L"Status: %lu\n", Status);
        }
        else
        {
            result = 1;
        }
    }

done:
    if (pUsers != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pUsers);

    if (result != 0)
    {
        ConResPuts(StdOut, IDS_GENERIC_SYNTAX);
        PrintNetMessage(MSG_GROUP_SYNTAX);
    }

    return result;
}

/* EOF */
