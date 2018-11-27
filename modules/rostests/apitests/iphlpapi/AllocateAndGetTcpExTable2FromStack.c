/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for TCP connections enumeration functions
 * COPYRIGHT:   Copyright 2018 Pierre Schweitzer
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <iphlpapi.h>
#include <winsock2.h>

static DWORD (WINAPI * pAllocateAndGetTcpExTable2FromStack)(PVOID*,BOOL,HANDLE,DWORD,DWORD,TCP_TABLE_CLASS);

START_TEST(AllocateAndGetTcpExTable2FromStack)
{
    WSADATA wsaData;
    SOCKET sock;
    SOCKADDR_IN server;
    PMIB_TCPTABLE TcpTable;
    PMIB_TCPTABLE_OWNER_PID TcpTableOwner;
    PMIB_TCPTABLE_OWNER_MODULE TcpTableOwnerMod;
    DWORD i;
    BOOLEAN Found;
    HINSTANCE hIpHlpApi;
    SYSTEMTIME Creation;
    DWORD Pid = GetCurrentProcessId();

    hIpHlpApi = GetModuleHandleW(L"iphlpapi.dll");
    if (!hIpHlpApi)
    {
        skip("Failed to load iphlpapi.dll\n");
        return;
    }

    pAllocateAndGetTcpExTable2FromStack = (void *)GetProcAddress(hIpHlpApi, "AllocateAndGetTcpExTable2FromStack");
    if (pAllocateAndGetTcpExTable2FromStack == NULL)
    {
        skip("AllocateAndGetTcpExTable2FromStack not found\n");
        return;
    }

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        skip("Failed to init WS2\n");
        return;
    }

    GetSystemTime(&Creation);

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

    if (pAllocateAndGetTcpExTable2FromStack((PVOID *)&TcpTable, TRUE, GetProcessHeap(), 0, AF_INET, TCP_TABLE_BASIC_ALL) == ERROR_SUCCESS)
    {
        ok(TcpTable->dwNumEntries > 0, "No TCP connections?!\n");

        Found = FALSE;
        for (i = 0; i < TcpTable->dwNumEntries; ++i)
        {
            if (TcpTable->table[i].dwState == MIB_TCP_STATE_LISTEN &&
                TcpTable->table[i].dwLocalAddr == 0 &&
                TcpTable->table[i].dwLocalPort == htons(9876) &&
                TcpTable->table[i].dwRemoteAddr == 0)
            {
                Found = TRUE;
                break;
            }
        }
        ok(Found, "Our socket wasn't found!\n");

        HeapFree(GetProcessHeap(), 0, TcpTable);
    }
    else
    {
        skip("AllocateAndGetTcpExTable2FromStack failure\n");
    }

    if (pAllocateAndGetTcpExTable2FromStack((PVOID *)&TcpTable, TRUE, GetProcessHeap(), 0, AF_INET, TCP_TABLE_BASIC_CONNECTIONS) == ERROR_SUCCESS)
    {
        Found = FALSE;
        for (i = 0; i < TcpTable->dwNumEntries; ++i)
        {
            if (TcpTable->table[i].dwState == MIB_TCP_STATE_LISTEN &&
                TcpTable->table[i].dwLocalAddr == 0 &&
                TcpTable->table[i].dwLocalPort == htons(9876) &&
                TcpTable->table[i].dwRemoteAddr == 0)
            {
                Found = TRUE;
                break;
            }
        }
        ok(Found == FALSE, "Our socket was found!\n");

        HeapFree(GetProcessHeap(), 0, TcpTable);
    }
    else
    {
        skip("AllocateAndGetTcpExTable2FromStack failure\n");
    }

    if (pAllocateAndGetTcpExTable2FromStack((PVOID *)&TcpTable, TRUE, GetProcessHeap(), 0, AF_INET, TCP_TABLE_BASIC_LISTENER) == ERROR_SUCCESS)
    {
        ok(TcpTable->dwNumEntries > 0, "No TCP connections?!\n");

        Found = FALSE;
        for (i = 0; i < TcpTable->dwNumEntries; ++i)
        {
            if (TcpTable->table[i].dwState == MIB_TCP_STATE_LISTEN &&
                TcpTable->table[i].dwLocalAddr == 0 &&
                TcpTable->table[i].dwLocalPort == htons(9876) &&
                TcpTable->table[i].dwRemoteAddr == 0)
            {
                Found = TRUE;
                break;
            }
        }
        ok(Found, "Our socket wasn't found!\n");

        HeapFree(GetProcessHeap(), 0, TcpTable);
    }
    else
    {
        skip("AllocateAndGetTcpExTable2FromStack failure\n");
    }

    if (pAllocateAndGetTcpExTable2FromStack((PVOID *)&TcpTableOwner, TRUE, GetProcessHeap(), 0, AF_INET, TCP_TABLE_OWNER_PID_ALL) == ERROR_SUCCESS)
    {
        ok(TcpTableOwner->dwNumEntries > 0, "No TCP connections?!\n");

        Found = FALSE;
        for (i = 0; i < TcpTableOwner->dwNumEntries; ++i)
        {
            if (TcpTableOwner->table[i].dwState == MIB_TCP_STATE_LISTEN &&
                TcpTableOwner->table[i].dwLocalAddr == 0 &&
                TcpTableOwner->table[i].dwLocalPort == htons(9876) &&
                TcpTableOwner->table[i].dwRemoteAddr == 0)
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
            ok(TcpTableOwner->table[i].dwOwningPid == Pid, "Invalid owner\n");
        }

        HeapFree(GetProcessHeap(), 0, TcpTableOwner);
    }
    else
    {
        skip("AllocateAndGetTcpExTable2FromStack failure\n");
    }

    if (pAllocateAndGetTcpExTable2FromStack((PVOID *)&TcpTableOwner, TRUE, GetProcessHeap(), 0, AF_INET, TCP_TABLE_OWNER_PID_CONNECTIONS) == ERROR_SUCCESS)
    {
        Found = FALSE;
        for (i = 0; i < TcpTableOwner->dwNumEntries; ++i)
        {
            if (TcpTableOwner->table[i].dwState == MIB_TCP_STATE_LISTEN &&
                TcpTableOwner->table[i].dwLocalAddr == 0 &&
                TcpTableOwner->table[i].dwLocalPort == htons(9876) &&
                TcpTableOwner->table[i].dwRemoteAddr == 0)
            {
                Found = TRUE;
                break;
            }
        }
        ok(Found == FALSE, "Our socket was found!\n");

        HeapFree(GetProcessHeap(), 0, TcpTableOwner);
    }
    else
    {
        skip("AllocateAndGetTcpExTable2FromStack failure\n");
    }

    if (pAllocateAndGetTcpExTable2FromStack((PVOID *)&TcpTableOwner, TRUE, GetProcessHeap(), 0, AF_INET, TCP_TABLE_OWNER_PID_LISTENER) == ERROR_SUCCESS)
    {
        ok(TcpTableOwner->dwNumEntries > 0, "No TCP connections?!\n");

        Found = FALSE;
        for (i = 0; i < TcpTableOwner->dwNumEntries; ++i)
        {
            if (TcpTableOwner->table[i].dwState == MIB_TCP_STATE_LISTEN &&
                TcpTableOwner->table[i].dwLocalAddr == 0 &&
                TcpTableOwner->table[i].dwLocalPort == htons(9876) &&
                TcpTableOwner->table[i].dwRemoteAddr == 0)
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
            ok(TcpTableOwner->table[i].dwOwningPid == Pid, "Invalid owner\n");
        }

        HeapFree(GetProcessHeap(), 0, TcpTableOwner);
    }
    else
    {
        skip("AllocateAndGetTcpExTable2FromStack failure\n");
    }

    if (pAllocateAndGetTcpExTable2FromStack((PVOID *)&TcpTableOwnerMod, TRUE, GetProcessHeap(), 0, AF_INET, TCP_TABLE_OWNER_MODULE_ALL) == ERROR_SUCCESS)
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
            SYSTEMTIME SockTime;

            ok(TcpTableOwnerMod->table[i].dwOwningPid == Pid, "Invalid owner\n");

            CopyMemory(&SockTime, &TcpTableOwnerMod->table[i].liCreateTimestamp, sizeof(SYSTEMTIME));
            ok(Creation.wYear == SockTime.wYear, "Invalid year\n");
            ok(Creation.wMonth == SockTime.wMonth, "Invalid month\n");
            ok(Creation.wDayOfWeek == SockTime.wDayOfWeek, "Invalid day of week\n");
            ok(Creation.wDay == SockTime.wDay, "Invalid day\n");
            ok(Creation.wHour == SockTime.wHour, "Invalid hour\n");
            ok(Creation.wMinute == SockTime.wMinute, "Invalid minute\n");
        }

        HeapFree(GetProcessHeap(), 0, TcpTableOwnerMod);
    }
    else
    {
        skip("AllocateAndGetTcpExTable2FromStack failure\n");
    }

    if (pAllocateAndGetTcpExTable2FromStack((PVOID *)&TcpTableOwnerMod, TRUE, GetProcessHeap(), 0, AF_INET, TCP_TABLE_OWNER_MODULE_CONNECTIONS) == ERROR_SUCCESS)
    {
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
        ok(Found == FALSE, "Our socket was found!\n");

        HeapFree(GetProcessHeap(), 0, TcpTableOwnerMod);
    }
    else
    {
        skip("AllocateAndGetTcpExTable2FromStack failure\n");
    }

    if (pAllocateAndGetTcpExTable2FromStack((PVOID *)&TcpTableOwnerMod, TRUE, GetProcessHeap(), 0, AF_INET, TCP_TABLE_OWNER_MODULE_LISTENER) == ERROR_SUCCESS)
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
            SYSTEMTIME SockTime;

            ok(TcpTableOwnerMod->table[i].dwOwningPid == Pid, "Invalid owner\n");

            CopyMemory(&SockTime, &TcpTableOwnerMod->table[i].liCreateTimestamp, sizeof(SYSTEMTIME));
            ok(Creation.wYear == SockTime.wYear, "Invalid year\n");
            ok(Creation.wMonth == SockTime.wMonth, "Invalid month\n");
            ok(Creation.wDayOfWeek == SockTime.wDayOfWeek, "Invalid day of week\n");
            ok(Creation.wDay == SockTime.wDay, "Invalid day\n");
            ok(Creation.wHour == SockTime.wHour, "Invalid hour\n");
            ok(Creation.wMinute == SockTime.wMinute, "Invalid minute\n");
        }

        HeapFree(GetProcessHeap(), 0, TcpTableOwnerMod);
    }
    else
    {
        skip("AllocateAndGetTcpExTable2FromStack failure\n");
    }

quit2:
    closesocket(sock);
quit:
    WSACleanup();
}
