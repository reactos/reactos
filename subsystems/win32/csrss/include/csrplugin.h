/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/include/csrplugin.h
 * PURPOSE:         CSRSS plugin interface
 */

/*
 * CSRSS is a native application and can only implicitly link against native
 * DLLs. Since e.g. user32.dll and gdi32.dll are win32 DLLs and not native
 * DLLs it is not possible to call functions in those DLLs directly from
 * CSRSS.
 * However, it is possible to explicitly load a non-native DLL. Such a DLL
 * can then in turn be implicitly linked against other DLLs in its own
 * subsystem.
 */

#ifndef CSRPLUGIN_H_INCLUDED
#define CSRPLUGIN_H_INCLUDED

#include <windows.h>
#include "api.h"

typedef NTSTATUS (WINAPI *CSRSS_INSERT_OBJECT_PROC)(PCSRSS_PROCESS_DATA ProcessData,
                                                     PHANDLE Handle,
                                                     Object_t *Object,
                                                     DWORD Access,
                                                     BOOL Inheritable);
typedef NTSTATUS (WINAPI *CSRSS_GET_OBJECT_PROC)(PCSRSS_PROCESS_DATA ProcessData,
                                                  HANDLE Handle,
                                                  Object_t **Object,
                                                  DWORD Access);
typedef NTSTATUS (WINAPI *CSRSS_RELEASE_OBJECT_BY_POINTER_PROC)(Object_t *Object);
typedef NTSTATUS (WINAPI *CSRSS_RELEASE_OBJECT_PROC)(PCSRSS_PROCESS_DATA ProcessData,
                                                      HANDLE Object );
typedef NTSTATUS (WINAPI *CSRSS_ENUM_PROCESSES_PROC)(CSRSS_ENUM_PROCESS_PROC EnumProc,
                                                      PVOID Context);

typedef struct tagCSRSS_EXPORTED_FUNCS
{
  CSRSS_INSERT_OBJECT_PROC CsrInsertObjectProc;
  CSRSS_GET_OBJECT_PROC CsrGetObjectProc;
  CSRSS_RELEASE_OBJECT_BY_POINTER_PROC CsrReleaseObjectByPointerProc;
  CSRSS_RELEASE_OBJECT_PROC CsrReleaseObjectProc;
  CSRSS_ENUM_PROCESSES_PROC CsrEnumProcessesProc;
} CSRSS_EXPORTED_FUNCS, *PCSRSS_EXPORTED_FUNCS;

typedef BOOL (WINAPI *CSRPLUGIN_INIT_COMPLETE_PROC)(void);

typedef BOOL (WINAPI *CSRPLUGIN_HARDERROR_PROC)(IN PCSRSS_PROCESS_DATA ProcessData,
                                                 IN PHARDERROR_MSG HardErrorMessage);

typedef BOOL (WINAPI *CSRPLUGIN_INITIALIZE_PROC)(PCSRSS_API_DEFINITION *ApiDefinitions,
                                                  PCSRSS_OBJECT_DEFINITION *ObjectDefinitions,
                                                  CSRPLUGIN_INIT_COMPLETE_PROC *InitCompleteProc,
                                                  CSRPLUGIN_HARDERROR_PROC *HardErrorProc,
                                                  PCSRSS_EXPORTED_FUNCS Exports,
                                                  HANDLE CsrssApiHeap);

#endif /* CSRPLUGIN_H_INCLUDED */

/* EOF */
