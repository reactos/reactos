/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Interface to csrss
 * FILE:             subsys/win32k/include/csr.h
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 */

#ifndef CSR_H_INCLUDED
#define CSR_H_INCLUDED

extern PEPROCESS CsrProcess;

extern NTSTATUS FASTCALL CsrInit(void);
extern NTSTATUS FASTCALL co_CsrNotify(PCSR_API_MESSAGE Request);
extern NTSTATUS FASTCALL CsrCloseHandle(HANDLE Handle);
NTSTATUS
STDCALL
CsrInsertObject(HANDLE ObjectHandle,
                ACCESS_MASK DesiredAccess,
                PHANDLE Handle);

#endif /* CSR_H_INCLUDED */

/* EOF */
