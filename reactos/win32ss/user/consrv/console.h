/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/console.h
 * PURPOSE:         Console Initialization Functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

VOID NTAPI
ConDrvInitConsoleSupport(VOID);

NTSTATUS WINAPI
ConSrvInitConsole(OUT struct _CONSOLE** /* PCONSOLE* */ NewConsole,
                  IN OUT PCONSOLE_START_INFO ConsoleStartInfo,
                  IN ULONG ConsoleLeaderProcessId);
VOID WINAPI ConSrvDeleteConsole(struct _CONSOLE* /* PCONSOLE */ Console);

NTSTATUS FASTCALL ConSrvGetConsole(PCONSOLE_PROCESS_DATA ProcessData,
                                   struct _CONSOLE** /* PCONSOLE* */ Console,
                                   BOOL LockConsole);
VOID FASTCALL ConSrvReleaseConsole(struct _CONSOLE* /* PCONSOLE */ Console,
                                   BOOL WasConsoleLocked);

/******************************************************************************/

NTSTATUS NTAPI
ConDrvGrabConsole(IN struct _CONSOLE* /* PCONSOLE */ Console,
                  IN BOOLEAN LockConsole);
VOID NTAPI
ConDrvReleaseConsole(IN struct _CONSOLE* /* PCONSOLE */ Console,
                     IN BOOLEAN WasConsoleLocked);

typedef struct _FRONTEND FRONTEND, *PFRONTEND;
typedef struct _CONSOLE_INFO CONSOLE_INFO, *PCONSOLE_INFO;

NTSTATUS NTAPI
ConDrvInitConsole(OUT struct _CONSOLE** /* PCONSOLE* */ NewConsole,
                  // IN OUT PCONSOLE_START_INFO ConsoleStartInfo,
                  IN PCONSOLE_INFO ConsoleInfo,
                  IN ULONG ConsoleLeaderProcessId);
NTSTATUS NTAPI
ConDrvRegisterFrontEnd(IN struct _CONSOLE* /* PCONSOLE */ Console,
                       IN PFRONTEND FrontEnd);
NTSTATUS NTAPI
ConDrvDeregisterFrontEnd(IN struct _CONSOLE* /* PCONSOLE */ Console);
VOID NTAPI
ConDrvDeleteConsole(IN struct _CONSOLE* /* PCONSOLE */ Console);

/* EOF */
