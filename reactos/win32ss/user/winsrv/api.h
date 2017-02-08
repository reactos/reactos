/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User API Server DLL
 * FILE:            win32ss/user/winsrv/api.h
 * PURPOSE:         Public server APIs definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

/* init.c */
BOOL WINAPI _UserSoundSentry(VOID);

/* harderror.c */
VOID WINAPI UserServerHardError(IN PCSR_THREAD ThreadData,
                                IN PHARDERROR_MSG Message);

/* register.c */
CSR_API(SrvRegisterServicesProcess);
CSR_API(SrvRegisterLogonProcess);

/* shutdown.c */
CSR_API(SrvExitWindowsEx);

/* EOF */
