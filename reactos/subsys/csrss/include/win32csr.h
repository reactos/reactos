/* $Id: win32csr.h,v 1.1 2003/12/02 11:38:46 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/win32csr.h
 * PURPOSE:         Interface to win32csr.dll
 */

#include <windows.h>

extern HANDLE Win32CsrApiHeap;

NTSTATUS FASTCALL Win32CsrInsertObject(PCSRSS_PROCESS_DATA ProcessData,
                                       PHANDLE Handle,
                                       Object_t *Object);
NTSTATUS FASTCALL Win32CsrGetObject(PCSRSS_PROCESS_DATA ProcessData,
                                    HANDLE Handle,
                                    Object_t **Object);
NTSTATUS FASTCALL Win32CsrReleaseObject(PCSRSS_PROCESS_DATA ProcessData,
                                        HANDLE Object);
