/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdUse.c
 * PURPOSE:
 *
 * PROGRAMMERS:     Pierre Schweitzer
 */

#include "net.h"

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

    ConPrintf(StdOut, L"%s\t\t\t%s\t\t\t\t%s\n", L"Local", L"Remote", L"Provider");

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
                if (!Local || _wcsicmp(lpCur->lpLocalName, Local) == 0)
                {
                    ConPrintf(StdOut, L"%s\t\t\t%s\t\t%s\n", lpCur->lpLocalName, lpCur->lpRemoteName, lpCur->lpProvider);
                }

                lpCur++;
            }
        }
    } while (dRet != WN_NO_MORE_ENTRIES);

    HeapFree(GetProcessHeap(), 0, lpRes);
    WNetCloseEnum(hEnum);

    return 0;
}

static
VOID
PrintError(DWORD Status)
{
    WCHAR szStatusBuffer[16];
    LPWSTR Buffer;

    swprintf(szStatusBuffer, L"%lu", Status);
    PrintMessageStringV(3502, szStatusBuffer);

    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, Status, 0, (LPWSTR)&Buffer, 0, NULL))
    {
        ConPrintf(StdErr, L"\n%s", Buffer);
        LocalFree(Buffer);
    }
}

static
BOOL
ValidateDeviceName(PWSTR DevName)
{
    DWORD Len;

    Len = wcslen(DevName);
    if (Len != 2)
    {
        return FALSE;
    }

    if (!iswalpha(DevName[0]) || DevName[1] != L':')
    {
        return FALSE;
    }

    return TRUE;
}

INT
cmdUse(
    INT argc,
    WCHAR **argv)
{
    DWORD Status, Len, Delete;

    if (argc == 2)
    {
        Status = EnumerateConnections(NULL);
        if (Status == NO_ERROR)
            PrintErrorMessage(ERROR_SUCCESS);
        else
            PrintError(Status);

        return 0;
    }
    else if (argc == 3)
    {
        if (!ValidateDeviceName(argv[2]))
        {
            PrintMessageStringV(3952, L"DeviceName");
            return 1;
        }

        Status = EnumerateConnections(argv[2]);
        if (Status == NO_ERROR)
            PrintErrorMessage(ERROR_SUCCESS);
        else
            PrintError(Status);

        return 0;
    }

    Delete = 0;
    if (_wcsicmp(argv[2], L"/DELETE") == 0)
    {
        Delete = 3;
    }
    else
    {
        if ((argv[2][0] != '*' && argv[2][1] != 0) &&
            !ValidateDeviceName(argv[2]))
        {
            PrintMessageStringV(3952, L"DeviceName");
            return 1;
        }
    }

    if (_wcsicmp(argv[3], L"/DELETE") == 0)
    {
        Delete = 2;
    }

    if (Delete != 0)
    {
        if (!ValidateDeviceName(argv[Delete]) || argv[Delete][0] == L'*')
        {
            PrintMessageStringV(3952, L"DeviceName");
            return 1;
        }

        Status = WNetCancelConnection2(argv[Delete], CONNECT_UPDATE_PROFILE, FALSE);
        if (Status != NO_ERROR)
            PrintError(Status);

        return Status;
    }
    else
    {
        BOOL Persist = FALSE;
        NETRESOURCE lpNet;
        WCHAR Access[256];
        DWORD OutFlags = 0, Size = ARRAYSIZE(Access);

        Len = wcslen(argv[3]);
        if (Len < 4)
        {
            PrintMessageStringV(3952, L"Name");
            return 1;
        }

        if (argv[3][0] != L'\\' || argv[3][1] != L'\\')
        {
            PrintMessageStringV(3952, L"Name");
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
                            PrintMessageStringV(3952, L"Persistent");
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
            PrintMessageStringV(3919, argv[3], Access);
        else if (Status != NO_ERROR)
            PrintError(Status);

        return Status;
    }
}
