/* 
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Interface to CSRSS / USERSRV
 * FILE:             subsystems/win32/win32k/ntuser/csr.h
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 */

#pragma once

extern PEPROCESS CsrProcess;

NTSTATUS FASTCALL CsrInit(void);
NTSTATUS FASTCALL co_CsrNotify(PCSR_API_MESSAGE Request);

/* EOF */
