/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User API Server DLL
 * FILE:            include/reactos/subsys/win/winmsg.h
 * PURPOSE:         Public definitions for communication
 *                  between User-Mode API Clients and Servers
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _WINMSG_H
#define _WINMSG_H

#pragma once

#define USERSRV_SERVERDLL_INDEX     3
#define USERSRV_FIRST_API_NUMBER    1024

// Windows Server 2003 table from http://j00ru.vexillium.org/csrss_list/api_list.html#Windows_2k3
typedef enum _USERSRV_API_NUMBER
{
    UserpExitWindowsEx = USERSRV_FIRST_API_NUMBER,
    UserpEndTask,
    UserpLogon,
    UserpRegisterServicesProcess, // Not present in Win7
    UserpActivateDebugger,
    UserpGetThreadConsoleDesktop, // Not present in Win7
    UserpDeviceEvent,
    UserpRegisterLogonProcess,    // Not present in Win7
    UserpCreateSystemThreads,
    UserpRecordShutdownReason,
    // UserpCancelShutdown,              // Added in Vista
    // UserpConsoleHandleOperation,      // Added in Win7
    // UserpGetSetShutdownBlockReason,   // Added in Vista

    UserpMaxApiNumber
} USERSRV_API_NUMBER, *PUSERSRV_API_NUMBER;

/* The USERCONNECT structure is defined in win32ss/include/ntuser.h */
#define _USERSRV_API_CONNECTINFO    _USERCONNECT
#define  USERSRV_API_CONNECTINFO     USERCONNECT
#define PUSERSRV_API_CONNECTINFO    PUSERCONNECT

#if defined(_M_IX86)
C_ASSERT(sizeof(USERSRV_API_CONNECTINFO) == 0x124);
#endif


typedef struct _USER_EXIT_REACTOS
{
    DWORD LastError;
    UINT  Flags;
    BOOL  Success;
} USER_EXIT_REACTOS, *PUSER_EXIT_REACTOS;

typedef struct _USER_END_TASK
{
    DWORD LastError;
    HWND  WndHandle;
    BOOL  Force;
    BOOL  Success;
} USER_END_TASK, *PUSER_END_TASK;

typedef struct _USER_LOGON
{
    BOOL IsLogon;
} USER_LOGON, *PUSER_LOGON;

typedef struct _USER_GET_THREAD_CONSOLE_DESKTOP
{
    ULONG_PTR ThreadId;
    HANDLE ConsoleDesktop;
} USER_GET_THREAD_CONSOLE_DESKTOP, *PUSER_GET_THREAD_CONSOLE_DESKTOP;

typedef struct _USER_REGISTER_SERVICES_PROCESS
{
    ULONG_PTR ProcessId;
} USER_REGISTER_SERVICES_PROCESS, *PUSER_REGISTER_SERVICES_PROCESS;

typedef struct _USER_REGISTER_LOGON_PROCESS
{
    ULONG_PTR ProcessId;
    BOOL Register;
} USER_REGISTER_LOGON_PROCESS, *PUSER_REGISTER_LOGON_PROCESS;


typedef struct _USER_API_MESSAGE
{
    PORT_MESSAGE Header;

    PCSR_CAPTURE_BUFFER CsrCaptureData;
    CSR_API_NUMBER ApiNumber;
    NTSTATUS Status;
    ULONG Reserved;
    union
    {
        USER_EXIT_REACTOS ExitReactosRequest;
        USER_END_TASK EndTaskRequest;
        USER_LOGON LogonRequest;
        USER_GET_THREAD_CONSOLE_DESKTOP GetThreadConsoleDesktopRequest;
        USER_REGISTER_SERVICES_PROCESS RegisterServicesProcessRequest;
        USER_REGISTER_LOGON_PROCESS RegisterLogonProcessRequest;
    } Data;
} USER_API_MESSAGE, *PUSER_API_MESSAGE;

// Check that a USER_API_MESSAGE can hold in a CSR_API_MESSAGE.
CHECK_API_MSG_SIZE(USER_API_MESSAGE);

#endif // _WINMSG_H

/* EOF */
