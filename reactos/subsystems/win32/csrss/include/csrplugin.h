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

#pragma once

#include <windows.h>
#include "api.h"

typedef NTSTATUS (WINAPI *CSRSS_ENUM_PROCESSES_PROC)(CSRSS_ENUM_PROCESS_PROC EnumProc,
                                                      PVOID Context);

typedef struct tagCSRSS_EXPORTED_FUNCS
{
  CSRSS_ENUM_PROCESSES_PROC CsrEnumProcessesProc;
} CSRSS_EXPORTED_FUNCS, *PCSRSS_EXPORTED_FUNCS;

typedef BOOL (WINAPI *CSRPLUGIN_INIT_COMPLETE_PROC)(void);

typedef BOOL (WINAPI *CSRPLUGIN_HARDERROR_PROC)(IN PCSRSS_PROCESS_DATA ProcessData,
                                                 IN PHARDERROR_MSG HardErrorMessage);

typedef NTSTATUS (WINAPI *CSRPLUGIN_PROCESS_INHERIT_PROC)(IN PCSRSS_PROCESS_DATA SourceProcessData,
                                                          IN PCSRSS_PROCESS_DATA TargetProcessData);

typedef NTSTATUS (WINAPI *CSRPLUGIN_PROCESS_DELETED_PROC)(IN PCSRSS_PROCESS_DATA ProcessData);

typedef struct tagCSRSS_SERVER_PROCS
{
  CSRPLUGIN_INIT_COMPLETE_PROC InitCompleteProc;
  CSRPLUGIN_HARDERROR_PROC HardErrorProc;
  CSRPLUGIN_PROCESS_INHERIT_PROC ProcessInheritProc;
  CSRPLUGIN_PROCESS_DELETED_PROC ProcessDeletedProc;
} CSRPLUGIN_SERVER_PROCS, *PCSRPLUGIN_SERVER_PROCS;

typedef BOOL (WINAPI *CSRPLUGIN_INITIALIZE_PROC)(PCSRSS_API_DEFINITION *ApiDefinitions,
                                                  PCSRPLUGIN_SERVER_PROCS ServerProcs,
                                                  PCSRSS_EXPORTED_FUNCS Exports,
                                                  HANDLE CsrssApiHeap);

/* EOF */
