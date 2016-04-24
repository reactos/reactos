/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User API Server DLL
 * FILE:            win32ss/user/winsrv/usersrv/api.h
 * PURPOSE:         Public server APIs definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

/* init.c */
BOOL NTAPI _UserSoundSentry(VOID);
CSR_API(SrvCreateSystemThreads);
CSR_API(SrvActivateDebugger);
CSR_API(SrvGetThreadConsoleDesktop);
CSR_API(SrvDeviceEvent);

/* harderror.c */
VOID
NTAPI
UserServerHardError(IN PCSR_THREAD ThreadData,
                    IN PHARDERROR_MSG Message);

/* register.c */
CSR_API(SrvRegisterServicesProcess);
CSR_API(SrvRegisterLogonProcess);

/* shutdown.c */
ULONG
NTAPI
UserClientShutdown(IN PCSR_PROCESS CsrProcess,
                   IN ULONG Flags,
                   IN BOOLEAN FirstPhase);

CSR_API(SrvExitWindowsEx);
CSR_API(SrvEndTask);
CSR_API(SrvLogon);
CSR_API(SrvRecordShutdownReason);
