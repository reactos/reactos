/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/win32csr.h
 * PURPOSE:         Interface to win32csr.dll
 */


#ifndef WIN32CSR_H_INCLUDED
#define WIN32CSR_H_INCLUDED

#include <windows.h>
#include <commctrl.h>

extern HANDLE Win32CsrApiHeap;
extern HINSTANCE Win32CsrDllHandle;

NTSTATUS FASTCALL Win32CsrInsertObject(PCSRSS_PROCESS_DATA ProcessData,
                                       PHANDLE Handle,
                                       Object_t *Object,
                                       DWORD Access,
                                       BOOL Inheritable);
NTSTATUS FASTCALL Win32CsrLockObject(PCSRSS_PROCESS_DATA ProcessData,
                                     HANDLE Handle,
                                     Object_t **Object,
                                     DWORD Access,
                                     long Type);
VOID FASTCALL Win32CsrUnlockObject(Object_t *Object);

NTSTATUS FASTCALL Win32CsrGetObject(PCSRSS_PROCESS_DATA ProcessData,
                                    HANDLE Handle,
                                    Object_t **Object,
                                    DWORD Access);
NTSTATUS FASTCALL Win32CsrReleaseObjectByPointer(Object_t *Object);
NTSTATUS FASTCALL Win32CsrReleaseObject(PCSRSS_PROCESS_DATA ProcessData,
                                        HANDLE Object);
NTSTATUS FASTCALL Win32CsrEnumProcesses(CSRSS_ENUM_PROCESS_PROC EnumProc,
                                        PVOID Context);

#endif /* WIN32CSR_H_INCLUDED */

/* EOF */
