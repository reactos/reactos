/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            dll/win32/kernel32/include/console.h
 * PURPOSE:         Console API Client Definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

/* CONSTANTS ******************************************************************/

#define HANDLE_DETACHED_PROCESS    (HANDLE)-2
#define HANDLE_CREATE_NEW_CONSOLE  (HANDLE)-3
#define HANDLE_CREATE_NO_WINDOW    (HANDLE)-4


/* FUNCTION PROTOTYPES ********************************************************/

BOOL WINAPI
BasepInitConsole(VOID);

VOID WINAPI
BasepUninitConsole(VOID);

VOID WINAPI
InitConsoleCtrlHandling(VOID);

DWORD WINAPI
ConsoleControlDispatcher(IN LPVOID lpThreadParameter);

DWORD WINAPI
PropDialogHandler(IN LPVOID lpThreadParameter);

HANDLE WINAPI
DuplicateConsoleHandle(HANDLE hConsole,
                       DWORD  dwDesiredAccess,
                       BOOL   bInheritHandle,
                       DWORD  dwOptions);

BOOL WINAPI
GetConsoleHandleInformation(IN HANDLE hHandle,
                            OUT LPDWORD lpdwFlags);

BOOL WINAPI
SetConsoleHandleInformation(IN HANDLE hHandle,
                            IN DWORD dwMask,
                            IN DWORD dwFlags);

BOOL WINAPI
VerifyConsoleIoHandle(HANDLE Handle);

BOOL WINAPI
CloseConsoleHandle(HANDLE Handle);

HANDLE WINAPI
GetConsoleInputWaitHandle(VOID);

HANDLE FASTCALL
TranslateStdHandle(HANDLE hHandle);

VOID
InitConsoleInfo(IN OUT PCONSOLE_START_INFO ConsoleStartInfo,
                IN PUNICODE_STRING ImagePathName);

LPCWSTR
IntCheckForConsoleFileName(IN LPCWSTR pszName,
                           IN DWORD dwDesiredAccess);

HANDLE WINAPI
OpenConsoleW(LPCWSTR wsName,
             DWORD   dwDesiredAccess,
             BOOL    bInheritHandle,
             DWORD   dwShareMode);

BOOL WINAPI
SetConsoleInputExeNameW(LPCWSTR lpInputExeName);

/* EOF */
