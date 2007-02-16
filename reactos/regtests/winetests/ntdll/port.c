/* Unit test suite for Ntdll Port API functions
 *
 * Copyright 2006 James Hawkins
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include <stdarg.h>

#include "ntdll_test.h"
#include "windows.h"

#ifndef __WINE_WINTERNL_H

typedef struct _CLIENT_ID
{
   HANDLE UniqueProcess;
   HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _LPC_SECTION_WRITE
{
  ULONG Length;
  HANDLE SectionHandle;
  ULONG SectionOffset;
  ULONG ViewSize;
  PVOID ViewBase;
  PVOID TargetViewBase;
} LPC_SECTION_WRITE, *PLPC_SECTION_WRITE;

typedef struct _LPC_SECTION_READ
{
  ULONG Length;
  ULONG ViewSize;
  PVOID ViewBase;
} LPC_SECTION_READ, *PLPC_SECTION_READ;

typedef struct _LPC_MESSAGE
{
  USHORT DataSize;
  USHORT MessageSize;
  USHORT MessageType;
  USHORT VirtualRangesOffset;
  CLIENT_ID ClientId;
  ULONG MessageId;
  ULONG SectionSize;
  UCHAR Data[ANYSIZE_ARRAY];
} LPC_MESSAGE, *PLPC_MESSAGE;

#endif

/* Types of LPC messages */
#define UNUSED_MSG_TYPE                 0
#define LPC_REQUEST                     1
#define LPC_REPLY                       2
#define LPC_DATAGRAM                    3
#define LPC_LOST_REPLY                  4
#define LPC_PORT_CLOSED                 5
#define LPC_CLIENT_DIED                 6
#define LPC_EXCEPTION                   7
#define LPC_DEBUG_EVENT                 8
#define LPC_ERROR_EVENT                 9
#define LPC_CONNECTION_REQUEST         10

static const WCHAR PORTNAME[] = {'\\','M','y','P','o','r','t',0};

#define REQUEST1    "Request1"
#define REQUEST2    "Request2"
#define REPLY       "Reply"

#define MAX_MESSAGE_LEN    30

UNICODE_STRING  port;
static char     selfname[MAX_PATH];
static int      myARGC;
static char**   myARGV;

/* Function pointers for ntdll calls */
static HMODULE hntdll = 0;
static NTSTATUS (WINAPI *pNtCompleteConnectPort)(HANDLE);
static NTSTATUS (WINAPI *pNtAcceptConnectPort)(PHANDLE,ULONG,PLPC_MESSAGE,ULONG,
                                               ULONG,PLPC_SECTION_READ);
static NTSTATUS (WINAPI *pNtReplyPort)(HANDLE,PLPC_MESSAGE);
static NTSTATUS (WINAPI *pNtReplyWaitReceivePort)(PHANDLE,PULONG,PLPC_MESSAGE,
                                                  PLPC_MESSAGE);
static NTSTATUS (WINAPI *pNtCreatePort)(PHANDLE,POBJECT_ATTRIBUTES,ULONG,ULONG,ULONG);
static NTSTATUS (WINAPI *pNtRequestWaitReplyPort)(HANDLE,PLPC_MESSAGE,PLPC_MESSAGE);
static NTSTATUS (WINAPI *pNtRequestPort)(HANDLE,PLPC_MESSAGE);
static NTSTATUS (WINAPI *pNtRegisterThreadTerminatePort)(HANDLE);
static NTSTATUS (WINAPI *pNtConnectPort)(PHANDLE,PUNICODE_STRING,
                                         PSECURITY_QUALITY_OF_SERVICE,
                                         PLPC_SECTION_WRITE,PLPC_SECTION_READ,
                                         PVOID,PVOID,PULONG);
static NTSTATUS (WINAPI *pRtlInitUnicodeString)(PUNICODE_STRING,LPCWSTR);
static NTSTATUS (WINAPI *pNtWaitForSingleObject)(HANDLE,BOOLEAN,PLARGE_INTEGER);

static BOOL init_function_ptrs(void)
{
    hntdll = LoadLibraryA("ntdll.dll");

    if (hntdll)
    {
        pNtCompleteConnectPort = (void *)GetProcAddress(hntdll, "NtCompleteConnectPort");
        pNtAcceptConnectPort = (void *)GetProcAddress(hntdll, "NtAcceptConnectPort");
        pNtReplyPort = (void *)GetProcAddress(hntdll, "NtReplyPort");
        pNtReplyWaitReceivePort = (void *)GetProcAddress(hntdll, "NtReplyWaitReceivePort");
        pNtCreatePort = (void *)GetProcAddress(hntdll, "NtCreatePort");
        pNtRequestWaitReplyPort = (void *)GetProcAddress(hntdll, "NtRequestWaitReplyPort");
        pNtRequestPort = (void *)GetProcAddress(hntdll, "NtRequestPort");
        pNtRegisterThreadTerminatePort = (void *)GetProcAddress(hntdll, "NtRegisterThreadTerminatePort");
        pNtConnectPort = (void *)GetProcAddress(hntdll, "NtConnectPort");
        pRtlInitUnicodeString = (void *)GetProcAddress(hntdll, "RtlInitUnicodeString");
        pNtWaitForSingleObject = (void *)GetProcAddress(hntdll, "NtWaitForSingleObject");
    }

    if (!pNtCompleteConnectPort || !pNtAcceptConnectPort ||
        !pNtReplyWaitReceivePort || !pNtCreatePort || !pNtRequestWaitReplyPort ||
        !pNtRequestPort || !pNtRegisterThreadTerminatePort ||
        !pNtConnectPort || !pRtlInitUnicodeString)
    {
        return FALSE;
    }

    return TRUE;
}

static void ProcessConnectionRequest(PLPC_MESSAGE LpcMessage, PHANDLE pAcceptPortHandle)
{
    NTSTATUS status;

    ok(LpcMessage->MessageType == LPC_CONNECTION_REQUEST,
       "Expected LPC_CONNECTION_REQUEST, got %d\n", LpcMessage->MessageType);
    ok(!*LpcMessage->Data, "Expected empty string!\n");

    status = pNtAcceptConnectPort(pAcceptPortHandle, 0, LpcMessage, 1, 0, NULL);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %ld\n", status);
    
    status = pNtCompleteConnectPort(*pAcceptPortHandle);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %ld\n", status);
}

static void ProcessLpcRequest(HANDLE PortHandle, PLPC_MESSAGE LpcMessage)
{
    NTSTATUS status;

    ok(LpcMessage->MessageType == LPC_REQUEST,
       "Expected LPC_REQUEST, got %d\n", LpcMessage->MessageType);
    ok(!lstrcmp((LPSTR)LpcMessage->Data, REQUEST2),
       "Expected %s, got %s\n", REQUEST2, LpcMessage->Data);

    lstrcpy((LPSTR)LpcMessage->Data, REPLY);

    status = pNtReplyPort(PortHandle, LpcMessage);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %ld\n", status);
    ok(LpcMessage->MessageType == LPC_REQUEST,
       "Expected LPC_REQUEST, got %d\n", LpcMessage->MessageType);
    ok(!lstrcmp((LPSTR)LpcMessage->Data, REPLY),
       "Expected %s, got %s\n", REPLY, LpcMessage->Data);
}

static DWORD WINAPI test_ports_client(LPVOID arg)
{
    SECURITY_QUALITY_OF_SERVICE sqos;
    LPC_MESSAGE *LpcMessage, *out;
    HANDLE PortHandle;
    ULONG len, size;
    NTSTATUS status;

    sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    sqos.ImpersonationLevel = SecurityImpersonation;
    sqos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    sqos.EffectiveOnly = TRUE;

    status = pNtConnectPort(&PortHandle, &port, &sqos, 0, 0, &len, NULL, NULL);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %ld\n", status);

    status = pNtRegisterThreadTerminatePort(PortHandle);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %ld\n", status);

    size = FIELD_OFFSET(LPC_MESSAGE, Data) + MAX_MESSAGE_LEN;
    LpcMessage = HeapAlloc(GetProcessHeap(), 0, size);
    out = HeapAlloc(GetProcessHeap(), 0, size);

    memset(LpcMessage, 0, size);
    LpcMessage->DataSize = lstrlen(REQUEST1) + 1;
    LpcMessage->MessageSize = FIELD_OFFSET(LPC_MESSAGE, Data) + LpcMessage->DataSize;
    lstrcpy((LPSTR)LpcMessage->Data, REQUEST1);

    status = pNtRequestPort(PortHandle, LpcMessage);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %ld\n", status);
    ok(LpcMessage->MessageType == 0, "Expected 0, got %d\n", LpcMessage->MessageType);
    ok(!lstrcmp((LPSTR)LpcMessage->Data, REQUEST1),
       "Expected %s, got %s\n", REQUEST1, LpcMessage->Data);

    /* Fill in the message */
    memset(LpcMessage, 0, size);
    LpcMessage->DataSize = lstrlen(REQUEST2) + 1;
    LpcMessage->MessageSize = FIELD_OFFSET(LPC_MESSAGE, Data) + LpcMessage->DataSize;
    lstrcpy((LPSTR)LpcMessage->Data, REQUEST2);

    /* Send the message and wait for the reply */
    status = pNtRequestWaitReplyPort(PortHandle, LpcMessage, out);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %ld\n", status);
    ok(!lstrcmp((LPSTR)out->Data, REPLY), "Expected %s, got %s\n", REPLY, out->Data);
    ok(out->MessageType == LPC_REPLY, "Expected LPC_REPLY, got %d\n", out->MessageType);

    return 0;
}

static void test_ports_server(void)
{
    OBJECT_ATTRIBUTES obj;
    HANDLE PortHandle;
    HANDLE AcceptPortHandle;
    PLPC_MESSAGE LpcMessage;
    ULONG size;
    NTSTATUS status;
    BOOL done = FALSE;

    pRtlInitUnicodeString(&port, PORTNAME);

    memset(&obj, 0, sizeof(OBJECT_ATTRIBUTES));
    obj.Length = sizeof(OBJECT_ATTRIBUTES);
    obj.ObjectName = &port;

    status = pNtCreatePort(&PortHandle, &obj, 100, 100, 0);
    todo_wine
    {
        ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %ld\n", status);
    }

    size = FIELD_OFFSET(LPC_MESSAGE, Data) + MAX_MESSAGE_LEN;
    LpcMessage = HeapAlloc(GetProcessHeap(), 0, size);
    memset(LpcMessage, 0, size);

    while (TRUE)
    {
        status = pNtReplyWaitReceivePort(PortHandle, NULL, NULL, LpcMessage);
        todo_wine
        {
            ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %ld(%lx)\n", status, status);
        }
        /* STATUS_INVALID_HANDLE: win2k without admin rights will perform an
         *                        endless loop here
         */
        if ((status == STATUS_NOT_IMPLEMENTED) ||
            (status == STATUS_INVALID_HANDLE)) return;

        switch (LpcMessage->MessageType)
        {
            case LPC_CONNECTION_REQUEST:
                ProcessConnectionRequest(LpcMessage, &AcceptPortHandle);
                break;

            case LPC_REQUEST:
                ProcessLpcRequest(PortHandle, LpcMessage);
                done = TRUE;
                break;

            case LPC_DATAGRAM:
                ok(!lstrcmp((LPSTR)LpcMessage->Data, REQUEST1),
                   "Expected %s, got %s\n", REQUEST1, LpcMessage->Data);
                break;

            case LPC_CLIENT_DIED:
                ok(done, "Expected LPC request to be completed!\n");
                return;

            default:
                ok(FALSE, "Unexpected message: %d\n", LpcMessage->MessageType);
                break;
        }
    }
}

START_TEST(port)
{
    HANDLE thread;
    DWORD id;

    if (!init_function_ptrs())
        return;

    myARGC = winetest_get_mainargs(&myARGV);
    strcpy(selfname, myARGV[0]);

    thread = CreateThread(NULL, 0, test_ports_client, NULL, 0, &id);
    ok(thread != NULL, "Expected non-NULL thread handle!\n");

    test_ports_server();
    CloseHandle(thread);
}
