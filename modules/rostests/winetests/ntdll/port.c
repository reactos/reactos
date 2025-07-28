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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winnls.h"
#include "wine/test.h"
#include "winternl.h"

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
  ULONG_PTR MessageId;
  ULONG_PTR SectionSize;
  UCHAR Data[ANYSIZE_ARRAY];
} LPC_MESSAGE, *PLPC_MESSAGE;

#endif

/* on Wow64 we have to use the 64-bit layout */
typedef struct
{
  USHORT DataSize;
  USHORT MessageSize;
  USHORT MessageType;
  USHORT VirtualRangesOffset;
  ULONGLONG ClientId[2];
  ULONGLONG MessageId;
  ULONGLONG SectionSize;
  UCHAR Data[ANYSIZE_ARRAY];
} LPC_MESSAGE64;

union lpc_message
{
    LPC_MESSAGE   msg;
    LPC_MESSAGE64 msg64;
};

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

static UNICODE_STRING port;

/* Function pointers for ntdll calls */
static HMODULE hntdll = 0;
static NTSTATUS (WINAPI *pNtCompleteConnectPort)(HANDLE);
static NTSTATUS (WINAPI *pNtAcceptConnectPort)(PHANDLE,ULONG,PLPC_MESSAGE,ULONG,
                                               PLPC_SECTION_WRITE,PLPC_SECTION_READ);
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
static BOOL     (WINAPI *pIsWow64Process)(HANDLE, PBOOL);

static BOOL is_wow64;

static BOOL init_function_ptrs(void)
{
    hntdll = LoadLibraryA("ntdll.dll");

    if (!hntdll)
        return FALSE;

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

    if (!pNtCompleteConnectPort || !pNtAcceptConnectPort ||
        !pNtReplyWaitReceivePort || !pNtCreatePort || !pNtRequestWaitReplyPort ||
        !pNtRequestPort || !pNtRegisterThreadTerminatePort ||
        !pNtConnectPort || !pRtlInitUnicodeString)
    {
        todo_wine win_skip("Needed port functions are not available\n");
        FreeLibrary(hntdll);
        return FALSE;
    }

    pIsWow64Process = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process");
    if (!pIsWow64Process || !pIsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;
    return TRUE;
}

static void ProcessConnectionRequest(union lpc_message *LpcMessage, PHANDLE pAcceptPortHandle)
{
    NTSTATUS status;

    if (is_wow64)
    {
        ok(LpcMessage->msg64.MessageType == LPC_CONNECTION_REQUEST,
           "Expected LPC_CONNECTION_REQUEST, got %d\n", LpcMessage->msg64.MessageType);
        ok(!*LpcMessage->msg64.Data, "Expected empty string!\n");
    }
    else
    {
        ok(LpcMessage->msg.MessageType == LPC_CONNECTION_REQUEST,
           "Expected LPC_CONNECTION_REQUEST, got %d\n", LpcMessage->msg.MessageType);
        ok(!*LpcMessage->msg.Data, "Expected empty string!\n");
    }

    status = pNtAcceptConnectPort(pAcceptPortHandle, 0, &LpcMessage->msg, 1, NULL, NULL);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %lx\n", status);
    
    status = pNtCompleteConnectPort(*pAcceptPortHandle);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %lx\n", status);
}

static void ProcessLpcRequest(HANDLE PortHandle, union lpc_message *LpcMessage)
{
    NTSTATUS status;

    if (is_wow64)
    {
        ok(LpcMessage->msg64.MessageType == LPC_REQUEST,
           "Expected LPC_REQUEST, got %d\n", LpcMessage->msg64.MessageType);
        ok(!strcmp((LPSTR)LpcMessage->msg64.Data, REQUEST2),
           "Expected %s, got %s\n", REQUEST2, LpcMessage->msg64.Data);
        strcpy((LPSTR)LpcMessage->msg64.Data, REPLY);

        status = pNtReplyPort(PortHandle, &LpcMessage->msg);
        ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %lx\n", status);
        ok(LpcMessage->msg64.MessageType == LPC_REQUEST,
           "Expected LPC_REQUEST, got %d\n", LpcMessage->msg64.MessageType);
        ok(!strcmp((LPSTR)LpcMessage->msg64.Data, REPLY),
           "Expected %s, got %s\n", REPLY, LpcMessage->msg64.Data);
    }
    else
    {
        ok(LpcMessage->msg.MessageType == LPC_REQUEST,
           "Expected LPC_REQUEST, got %d\n", LpcMessage->msg.MessageType);
        ok(!strcmp((LPSTR)LpcMessage->msg.Data, REQUEST2),
           "Expected %s, got %s\n", REQUEST2, LpcMessage->msg.Data);
        strcpy((LPSTR)LpcMessage->msg.Data, REPLY);

        status = pNtReplyPort(PortHandle, &LpcMessage->msg);
        ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %lx\n", status);
        ok(LpcMessage->msg.MessageType == LPC_REQUEST,
           "Expected LPC_REQUEST, got %d\n", LpcMessage->msg.MessageType);
        ok(!strcmp((LPSTR)LpcMessage->msg.Data, REPLY),
           "Expected %s, got %s\n", REPLY, LpcMessage->msg.Data);
    }
}

static DWORD WINAPI test_ports_client(LPVOID arg)
{
    SECURITY_QUALITY_OF_SERVICE sqos;
    union lpc_message *LpcMessage, *out;
    HANDLE PortHandle;
    ULONG len, size;
    NTSTATUS status;

    sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    sqos.ImpersonationLevel = SecurityImpersonation;
    sqos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    sqos.EffectiveOnly = TRUE;

    status = pNtConnectPort(&PortHandle, &port, &sqos, 0, 0, &len, NULL, NULL);
    todo_wine ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %lx\n", status);
    if (status != STATUS_SUCCESS) return 1;

    status = pNtRegisterThreadTerminatePort(PortHandle);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %lx\n", status);

    if (is_wow64)
    {
        size = FIELD_OFFSET(LPC_MESSAGE64, Data[MAX_MESSAGE_LEN]);
        LpcMessage = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        out = HeapAlloc(GetProcessHeap(), 0, size);

        LpcMessage->msg64.DataSize = strlen(REQUEST1) + 1;
        LpcMessage->msg64.MessageSize = FIELD_OFFSET(LPC_MESSAGE64, Data[LpcMessage->msg64.DataSize]);
        strcpy((LPSTR)LpcMessage->msg64.Data, REQUEST1);

        status = pNtRequestPort(PortHandle, &LpcMessage->msg);
        ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %lx\n", status);
        ok(LpcMessage->msg64.MessageType == 0, "Expected 0, got %d\n", LpcMessage->msg64.MessageType);
        ok(!strcmp((LPSTR)LpcMessage->msg64.Data, REQUEST1),
           "Expected %s, got %s\n", REQUEST1, LpcMessage->msg64.Data);

        /* Fill in the message */
        memset(LpcMessage, 0, size);
        LpcMessage->msg64.DataSize = strlen(REQUEST2) + 1;
        LpcMessage->msg64.MessageSize = FIELD_OFFSET(LPC_MESSAGE64, Data[LpcMessage->msg64.DataSize]);
        strcpy((LPSTR)LpcMessage->msg64.Data, REQUEST2);

        /* Send the message and wait for the reply */
        status = pNtRequestWaitReplyPort(PortHandle, &LpcMessage->msg, &out->msg);
        ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %lx\n", status);
        ok(!strcmp((LPSTR)out->msg64.Data, REPLY), "Expected %s, got %s\n", REPLY, out->msg64.Data);
        ok(out->msg64.MessageType == LPC_REPLY, "Expected LPC_REPLY, got %d\n", out->msg64.MessageType);
    }
    else
    {
        size = FIELD_OFFSET(LPC_MESSAGE, Data[MAX_MESSAGE_LEN]);
        LpcMessage = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        out = HeapAlloc(GetProcessHeap(), 0, size);

        LpcMessage->msg.DataSize = strlen(REQUEST1) + 1;
        LpcMessage->msg.MessageSize = FIELD_OFFSET(LPC_MESSAGE, Data[LpcMessage->msg.DataSize]);
        strcpy((LPSTR)LpcMessage->msg.Data, REQUEST1);

        status = pNtRequestPort(PortHandle, &LpcMessage->msg);
        ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %lx\n", status);
        ok(LpcMessage->msg.MessageType == 0, "Expected 0, got %d\n", LpcMessage->msg.MessageType);
        ok(!strcmp((LPSTR)LpcMessage->msg.Data, REQUEST1),
           "Expected %s, got %s\n", REQUEST1, LpcMessage->msg.Data);

        /* Fill in the message */
        memset(LpcMessage, 0, size);
        LpcMessage->msg.DataSize = strlen(REQUEST2) + 1;
        LpcMessage->msg.MessageSize = FIELD_OFFSET(LPC_MESSAGE, Data[LpcMessage->msg.DataSize]);
        strcpy((LPSTR)LpcMessage->msg.Data, REQUEST2);

        /* Send the message and wait for the reply */
        status = pNtRequestWaitReplyPort(PortHandle, &LpcMessage->msg, &out->msg);
        ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %lx\n", status);
        ok(!strcmp((LPSTR)out->msg.Data, REPLY), "Expected %s, got %s\n", REPLY, out->msg.Data);
        ok(out->msg.MessageType == LPC_REPLY, "Expected LPC_REPLY, got %d\n", out->msg.MessageType);
    }

    HeapFree(GetProcessHeap(), 0, out);
    HeapFree(GetProcessHeap(), 0, LpcMessage);

    return 0;
}

static void test_ports_server( HANDLE PortHandle )
{
    HANDLE AcceptPortHandle;
    union lpc_message *LpcMessage;
    ULONG size;
    NTSTATUS status;
    BOOL done = FALSE;

    size = FIELD_OFFSET(LPC_MESSAGE, Data) + MAX_MESSAGE_LEN;
    LpcMessage = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);

    while (TRUE)
    {
        status = pNtReplyWaitReceivePort(PortHandle, NULL, NULL, &LpcMessage->msg);
        todo_wine
        {
            ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %ld(%lx)\n", status, status);
        }
        /* STATUS_INVALID_HANDLE: win2k without admin rights will perform an
         *                        endless loop here
         */
        if ((status == STATUS_NOT_IMPLEMENTED) ||
            (status == STATUS_INVALID_HANDLE)) return;

        switch (is_wow64 ? LpcMessage->msg64.MessageType : LpcMessage->msg.MessageType)
        {
            case LPC_CONNECTION_REQUEST:
                ProcessConnectionRequest(LpcMessage, &AcceptPortHandle);
                break;

            case LPC_REQUEST:
                ProcessLpcRequest(PortHandle, LpcMessage);
                done = TRUE;
                break;

            case LPC_DATAGRAM:
                if (is_wow64)
                    ok(!strcmp((LPSTR)LpcMessage->msg64.Data, REQUEST1),
                       "Expected %s, got %s\n", REQUEST1, LpcMessage->msg64.Data);
                else
                    ok(!strcmp((LPSTR)LpcMessage->msg.Data, REQUEST1),
                       "Expected %s, got %s\n", REQUEST1, LpcMessage->msg.Data);
                break;

            case LPC_CLIENT_DIED:
                ok(done, "Expected LPC request to be completed!\n");
                HeapFree(GetProcessHeap(), 0, LpcMessage);
                return;

            default:
                ok(FALSE, "Unexpected message: %d\n",
                   is_wow64 ? LpcMessage->msg64.MessageType : LpcMessage->msg.MessageType);
                break;
        }
    }

    HeapFree(GetProcessHeap(), 0, LpcMessage);
}

START_TEST(port)
{
    OBJECT_ATTRIBUTES obj;
    HANDLE port_handle;
    NTSTATUS status;

    if (!init_function_ptrs())
        return;

    pRtlInitUnicodeString(&port, PORTNAME);

    memset(&obj, 0, sizeof(OBJECT_ATTRIBUTES));
    obj.Length = sizeof(OBJECT_ATTRIBUTES);
    obj.ObjectName = &port;

    status = pNtCreatePort(&port_handle, &obj, 100, 100, 0);
    if (status == STATUS_ACCESS_DENIED) skip("Not enough rights\n");
    else ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %ld\n", status);

    if (status == STATUS_SUCCESS)
    {
        DWORD id;
        HANDLE thread = CreateThread(NULL, 0, test_ports_client, NULL, 0, &id);
        ok(thread != NULL, "Expected non-NULL thread handle!\n");

        test_ports_server( port_handle );
        ok( WaitForSingleObject( thread, 10000 ) == 0, "thread didn't exit\n" );
        CloseHandle(thread);
    }
    FreeLibrary(hntdll);
}
