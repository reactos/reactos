/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/console.h
 * PURPOSE:         Consoles Management
 * PROGRAMMERS:     Hermes Belusca-Maito
 */

#pragma once

#define ConSrvLockConsoleListExclusive()    \
    RtlAcquireResourceExclusive(&ListLock, TRUE)

#define ConSrvLockConsoleListShared()       \
    RtlAcquireResourceShared(&ListLock, TRUE)

#define ConSrvUnlockConsoleList()           \
    RtlReleaseResource(&ListLock)

extern LIST_ENTRY ConsoleList;
extern RTL_RESOURCE ListLock;

#if 0
/*
 * WARNING: Change the state of the console ONLY when the console is locked !
 */
typedef enum _CONSOLE_STATE
{
    CONSOLE_INITIALIZING,   /* Console is initializing */
    CONSOLE_RUNNING     ,   /* Console running */
    CONSOLE_TERMINATING ,   /* Console about to be destroyed (but still not) */
    CONSOLE_IN_DESTRUCTION  /* Console in destruction */
} CONSOLE_STATE, *PCONSOLE_STATE;
#endif


VOID WINAPI ConSrvInitConsoleSupport(VOID);
NTSTATUS WINAPI ConSrvInitConsole(OUT PCONSOLE* NewConsole,
                                  IN OUT PCONSOLE_START_INFO ConsoleStartInfo,
                                  IN PCSR_PROCESS ConsoleLeaderProcess);
VOID WINAPI ConSrvDeleteConsole(PCONSOLE Console);
BOOL FASTCALL ConSrvValidatePointer(PCONSOLE Console);
BOOL FASTCALL ConSrvValidateConsoleState(PCONSOLE Console,
                                         CONSOLE_STATE ExpectedState);
BOOL FASTCALL ConSrvValidateConsoleUnsafe(PCONSOLE Console,
                                          CONSOLE_STATE ExpectedState,
                                          BOOL LockConsole);
BOOL FASTCALL ConSrvValidateConsole(PCONSOLE Console,
                                    CONSOLE_STATE ExpectedState,
                                    BOOL LockConsole);

/* EOF */
