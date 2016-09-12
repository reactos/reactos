/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdUse.c
 * PURPOSE:
 *
 * PROGRAMMERS:     Pierre Schweitzer
 */

#include "net.h"

#define COUNT_OF(a) (sizeof(a) / sizeof(a[0]))

static
DWORD
EnumerateConnections(LPCWSTR Local)
{
    DWORD dRet;
    HANDLE hEnum;
    LPNETRESOURCE lpRes;
    DWORD dSize = 0x1000;
    DWORD dCount = -1;
    LPNETRESOURCE lpCur;

    printf("%S\t\t\t%S\t\t\t\t%S\n", L"Local", L"Remote", L"Provider");

    dRet = WNetOpenEnum(RESOURCE_CONNECTED, RESOURCETYPE_DISK, 0, NULL, &hEnum);
    if (dRet != WN_SUCCESS)
    {
        return 1;
    }

    lpRes = HeapAlloc(GetProcessHeap(), 0, dSize);
    if (!lpRes)
    {
        WNetCloseEnum(hEnum);
        return 1;
    }

    do
    {
        dSize = 0x1000;
        dCount = -1;

        memset(lpRes, 0, dSize);
        dRet = WNetEnumResource(hEnum, &dCount, lpRes, &dSize);
        if (dRet == WN_SUCCESS || dRet == WN_MORE_DATA)
        {
            lpCur = lpRes;
            for (; dCount; dCount--)
            {
                if (!Local || wcsicmp(lpCur->lpLocalName, Local) == 0)
                {
                    printf("%S\t\t\t%S\t\t%S\n", lpCur->lpLocalName, lpCur->lpRemoteName, lpCur->lpProvider);
                }

                lpCur++;
            }
        }
    } while (dRet != WN_NO_MORE_ENTRIES);

    HeapFree(GetProcessHeap(), 0, lpRes);
    WNetCloseEnum(hEnum);

    return 0;
}

INT
cmdUse(
    INT argc,
    WCHAR **argv)
{
    DWORD Status, Len;

    if (argc == 2)
    {
        Status = EnumerateConnections(NULL);
        printf("Status: %lu\n", Status);
        return 0;
    }
    else if (argc == 3)
    {
        Len = wcslen(argv[2]);
        if (Len != 2)
        {
            PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"DeviceName");
            return 1;
        }

        if (!iswalpha(argv[2][0]) || argv[2][1] != L':')
        {
            PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"DeviceName");
            return 1;
        }

        Status = EnumerateConnections(argv[2]);
        printf("Status: %lu\n", Status);
        return 0;
    }

    Len = wcslen(argv[2]);
    if (Len != 1 && Len != 2)
    {
        PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"DeviceName");
        printf("Len: %lu\n", Len);
        return 1;
    }

    if (Len == 2 && argv[2][1] != L':')
    {
        PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"DeviceName");
        return 1;
    }

    if (argv[2][0] != L'*' && !iswalpha(argv[2][0]))
    {
        PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"DeviceName");
        return 1;
    }

    if (wcsicmp(argv[3], L"/DELETE") == 0)
    {
        if (argv[2][0] == L'*')
        {
            PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"DeviceName");
            return 1;
        }

        return WNetCancelConnection2(argv[2], CONNECT_UPDATE_PROFILE, FALSE);
    }
    else
    {
        BOOL Persist = FALSE;
        NETRESOURCE lpNet;
        WCHAR Access[256];
        DWORD OutFlags = 0, Size = COUNT_OF(Access);

        Len = wcslen(argv[3]);
        if (Len < 4)
        {
            PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"Name");
            return 1;
        }

        if (argv[3][0] != L'\\' || argv[3][1] != L'\\')
        {
            PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"Name");
            return 1;
        }

        if (argc > 4)
        {
            LPWSTR Cpy;
            Len = wcslen(argv[4]);
            if (Len > 12)
            {
                Cpy = HeapAlloc(GetProcessHeap(), 0, (Len + 1) * sizeof(WCHAR));
                if (Cpy)
                {
                    INT i;
                    for (i = 0; i < Len; ++i)
                        Cpy[i] = towupper(argv[4][i]);

                    if (wcsstr(Cpy, L"/PERSISTENT:") == Cpy)
                    {
                        LPWSTR Arg = Cpy + 12;
                        if (Len == 14 && Arg[0] == 'N' && Arg[1] == 'O')
                        {
                            Persist = FALSE;
                        }
                        else if (Len == 15 && Arg[0] == 'Y' && Arg[1] == 'E' && Arg[2] == 'S')
                        {
                            Persist = TRUE;
                        }
                        else
                        {
                            HeapFree(GetProcessHeap(), 0, Cpy);
                            PrintResourceString(IDS_ERROR_INVALID_OPTION_VALUE, L"Persistent");
                            return 1;
                        }
                    }
                    HeapFree(GetProcessHeap(), 0, Cpy);
                }
            }

        }

        lpNet.dwType = RESOURCETYPE_DISK;
        lpNet.lpLocalName = (argv[2][0] != L'*') ? argv[2] : NULL;
        lpNet.lpRemoteName = argv[3];
        lpNet.lpProvider = NULL;

        Status = WNetUseConnection(NULL, &lpNet, NULL, NULL, CONNECT_REDIRECT | (Persist ? CONNECT_UPDATE_PROFILE : 0), Access, &Size, &OutFlags);
        if (argv[2][0] == L'*' && Status == NO_ERROR && OutFlags == CONNECT_LOCALDRIVE)
            printf("%S is now connected to %S\n", argv[3], Access);

        return Status;
    }
}
