/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Interface between Win32k and USERSRV
 * FILE:             win32ss/user/ntuser/csr.h
 * PROGRAMER:        Hermes Belusca-Maito (hermes.belusca@sfr.fr), based on
 *                   the original code by Ge van Geldorp (ge@gse.nl) and by
 *                   the CSR code in NTDLL.
 */

#pragma once

/* NDK Headers */
#include <ndk/lpcfuncs.h>

/* CSRSS Header */
#include <csr/csr.h>
#include <win/winmsg.h>

extern PEPROCESS gpepCSRSS;
extern PVOID CsrApiPort;

VOID InitCsrProcess(VOID /*IN PEPROCESS CsrProcess*/);
VOID ResetCsrProcess(VOID);
NTSTATUS InitCsrApiPort(IN HANDLE CsrPortHandle);
VOID ResetCsrApiPort(VOID);

NTSTATUS
NTAPI
CsrClientCallServer(IN OUT PCSR_API_MESSAGE ApiMessage,
                    IN OUT PCSR_CAPTURE_BUFFER CaptureBuffer OPTIONAL,
                    IN CSR_API_NUMBER ApiNumber,
                    IN ULONG DataLength);

#define ST_RIT              (1<<0)
#define ST_DESKTOP_THREAD   (1<<1)
#define ST_GHOST_THREAD     (1<<2)

DWORD UserSystemThreadProc(BOOL bRemoteProcess);
BOOL UserCreateSystemThread(DWORD Type);

/* EOF */
