/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            dll/win32/kernel32/include/console.h
 * PURPOSE:         Console API Client Definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

/* CONSTANTS ******************************************************************/

#define HANDLE_DETACHED_PROCESS     (HANDLE)-1
#define HANDLE_CREATE_NEW_CONSOLE   (HANDLE)-2
#define HANDLE_CREATE_NO_WINDOW     (HANDLE)-3


/* FUNCTION PROTOTYPES ********************************************************/

BOOLEAN
WINAPI
ConDllInitialize(IN ULONG Reason,
                 IN PWSTR SessionDir);

VOID
InitializeCtrlHandling(VOID);

DWORD
WINAPI
ConsoleControlDispatcher(IN LPVOID lpThreadParameter);

DWORD
WINAPI
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

HANDLE
TranslateStdHandle(HANDLE hHandle);

#define SetTEBLangID(p) (p)

VOID
SetUpConsoleInfo(IN BOOLEAN CaptureTitle,
                 IN OUT LPDWORD pTitleLength,
                 IN OUT LPWSTR* lpTitle OPTIONAL,
                 IN OUT LPDWORD pDesktopLength,
                 IN OUT LPWSTR* lpDesktop OPTIONAL,
                 IN OUT PCONSOLE_START_INFO ConsoleStartInfo);

VOID
SetUpHandles(IN PCONSOLE_START_INFO ConsoleStartInfo);

VOID
InitExeName(VOID);

VOID
SetUpAppName(IN BOOLEAN CaptureStrings,
             IN OUT LPDWORD CurDirLength,
             IN OUT LPWSTR* CurDir,
             IN OUT LPDWORD AppNameLength,
             IN OUT LPWSTR* AppName);

USHORT
GetCurrentExeName(OUT PWCHAR ExeName,
                  IN USHORT BufferSize);

LPCWSTR
IntCheckForConsoleFileName(IN LPCWSTR pszName,
                           IN DWORD dwDesiredAccess);

HANDLE WINAPI
OpenConsoleW(LPCWSTR wsName,
             DWORD   dwDesiredAccess,
             BOOL    bInheritHandle,
             DWORD   dwShareMode);

/* EOF */
