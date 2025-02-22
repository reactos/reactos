/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for TCP connections owner functions
 * COPYRIGHT:   Copyright 2018 Pierre Schweitzer
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <iphlpapi.h>
#include <winsock2.h>

static DWORD GetExtendedTcpTableWithAlloc(PVOID *TcpTable, BOOL Order, DWORD Family, TCP_TABLE_CLASS Class)
{
    DWORD ret;
    DWORD Size = 0;

    *TcpTable = NULL;

    ret = GetExtendedTcpTable(*TcpTable, &Size, Order, Family, Class, 0);
    if (ret == ERROR_INSUFFICIENT_BUFFER)
    {
        *TcpTable = HeapAlloc(GetProcessHeap(), 0, Size);
        if (*TcpTable == NULL)
        {
            return ERROR_OUTOFMEMORY;
        }

        ret = GetExtendedTcpTable(*TcpTable, &Size, Order, Family, Class, 0);
        if (ret != NO_ERROR)
        {
            HeapFree(GetProcessHeap(), 0, *TcpTable);
            *TcpTable = NULL;
        }
    }

    return ret;
}

START_TEST(GetOwnerModuleFromTcpEntry)
{
    WSADATA wsaData;
    SOCKET sock;
    SOCKADDR_IN server;
    PMIB_TCPTABLE_OWNER_MODULE TcpTableOwnerMod;
    DWORD i;
    BOOLEAN Found;
    FILETIME Creation;
    LARGE_INTEGER CreationTime;
    DWORD Pid = GetCurrentProcessId();

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        skip("Failed to init WS2\n");
        return;
    }

    GetSystemTimeAsFileTime(&Creation);
    CreationTime.LowPart = Creation.dwLowDateTime;
    CreationTime.HighPart = Creation.dwHighDateTime;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        skip("Cannot create socket\n");
        goto quit;
    }

    ZeroMemory(&server, sizeof(SOCKADDR_IN));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(9876);

    if (bind(sock, (SOCKADDR*)&server, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
    {
        skip("Cannot bind socket\n");
        goto quit2;
    }

    if (listen(sock, SOMAXCONN) == SOCKET_ERROR)
    {
        skip("Cannot listen on socket\n");
        goto quit2;
    }

    if (GetExtendedTcpTableWithAlloc((PVOID *)&TcpTableOwnerMod, TRUE, AF_INET, TCP_TABLE_OWNER_MODULE_LISTENER) == ERROR_SUCCESS)
    {
        ok(TcpTableOwnerMod->dwNumEntries > 0, "No TCP connections?!\n");

        Found = FALSE;
        for (i = 0; i < TcpTableOwnerMod->dwNumEntries; ++i)
        {
            if (TcpTableOwnerMod->table[i].dwState == MIB_TCP_STATE_LISTEN &&
                TcpTableOwnerMod->table[i].dwLocalAddr == 0 &&
                TcpTableOwnerMod->table[i].dwLocalPort == htons(9876) &&
                TcpTableOwnerMod->table[i].dwRemoteAddr == 0)
            {
                Found = TRUE;
                break;
            }
        }

        if (!Found)
        {
            skip("Our socket wasn't found!\n");
        }
        else
        {
            DWORD Size = 0;
            PTCPIP_OWNER_MODULE_BASIC_INFO BasicInfo = NULL;

            ok(TcpTableOwnerMod->table[i].dwOwningPid == Pid, "Invalid owner\n");

            ok(TcpTableOwnerMod->table[i].liCreateTimestamp.QuadPart >= CreationTime.QuadPart, "Invalid time\n");
            ok(TcpTableOwnerMod->table[i].liCreateTimestamp.QuadPart <= CreationTime.QuadPart + 60000000000LL, "Invalid time\n");

            if (GetOwnerModuleFromTcpEntry(&TcpTableOwnerMod->table[i], TCPIP_OWNER_MODULE_INFO_BASIC, BasicInfo, &Size) == ERROR_INSUFFICIENT_BUFFER)
            {
                BasicInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
                ok(BasicInfo != NULL, "HeapAlloc failed\n");

                if (GetOwnerModuleFromTcpEntry(&TcpTableOwnerMod->table[i], TCPIP_OWNER_MODULE_INFO_BASIC, BasicInfo, &Size) == ERROR_SUCCESS)
                {
                    WCHAR CurrentModule[MAX_PATH];
                    PWSTR FileName;

                    if (GetModuleFileNameW(NULL, CurrentModule, MAX_PATH) != 0)
                    {
                        FileName = wcsrchr(CurrentModule, L'\\');
                        ++FileName;

                        ok(_wcsicmp(CurrentModule, BasicInfo->pModulePath) == 0, "Mismatching names (%S, %S)\n", CurrentModule, BasicInfo->pModulePath);
                        ok(_wcsicmp(FileName, BasicInfo->pModuleName) == 0, "Mismatching names (%S, %S)\n", FileName, BasicInfo->pModuleName);
                    }
                    else
                    {
                        skip("GetModuleFileNameW failed\n");
                    }
                }
                else
                {
                    skip("GetOwnerModuleFromTcpEntry failed\n");
                }
            }
            else
            {
                skip("GetOwnerModuleFromTcpEntry failed\n");
            }
        }

        HeapFree(GetProcessHeap(), 0, TcpTableOwnerMod);
    }
    else
    {
        skip("GetExtendedTcpTableWithAlloc failure\n");
    }

quit2:
    closesocket(sock);
quit:
    WSACleanup();
}
