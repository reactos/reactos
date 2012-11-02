/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Interface to csrss
 * FILE:             subsys/win32k/include/csr.h
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 */

#pragma once

extern PEPROCESS CsrProcess;

NTSTATUS FASTCALL CsrInit(void);

/* EOF */
