/* $Id: csr.h,v 1.1 2004/05/28 21:33:41 gvg Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Interface to csrss
 * FILE:             subsys/win32k/include/csr.h
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 */

#ifndef CSR_H_INCLUDED
#define CSR_H_INCLUDED

extern NTSTATUS FASTCALL CsrInit(void);
extern NTSTATUS FASTCALL CsrNotify(PCSRSS_API_REQUEST Request, PCSRSS_API_REPLY Reply);

#endif /* CSR_H_INCLUDED */

/* EOF */
