/* $Id: win32csr.h,v 1.2 2004/01/11 17:31:15 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/win32csr.h
 * PURPOSE:         Interface to win32csr.dll
 */


#ifndef WIN32CSR_H_INCLUDED
#define WIN32CSR_H_INCLUDED

#include <windows.h>

extern HANDLE Win32CsrApiHeap;

NTSTATUS FASTCALL Win32CsrInsertObject(PCSRSS_PROCESS_DATA ProcessData,
                                       PHANDLE Handle,
                                       Object_t *Object);
NTSTATUS FASTCALL Win32CsrLockObject(PCSRSS_PROCESS_DATA ProcessData,
                                     HANDLE Handle,
                                     Object_t **Object,
                                     long Type);
VOID FASTCALL Win32CsrUnlockObject(Object_t *Object);

#ifndef TODO
NTSTATUS FASTCALL Win32CsrGetObject(PCSRSS_PROCESS_DATA ProcessData,
                                    HANDLE Handle,
                                    Object_t **Object);
NTSTATUS FASTCALL Win32CsrReleaseObject(PCSRSS_PROCESS_DATA ProcessData,
                                        HANDLE Object);
#endif

#endif /* WIN32CSR_H_INCLUDED */

/* EOF */
