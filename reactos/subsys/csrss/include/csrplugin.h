/* $Id: csrplugin.h,v 1.2 2004/05/28 21:33:41 gvg Exp $
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

typedef NTSTATUS (STDCALL *CSRSS_INSERT_OBJECT_PROC)(PCSRSS_PROCESS_DATA ProcessData,
                                                     PHANDLE Handle,
                                                     Object_t *Object);
typedef NTSTATUS (STDCALL *CSRSS_GET_OBJECT_PROC)(PCSRSS_PROCESS_DATA ProcessData,
                                                  HANDLE Handle,
                                                  Object_t **Object);
typedef NTSTATUS (STDCALL *CSRSS_RELEASE_OBJECT_PROC)(PCSRSS_PROCESS_DATA ProcessData,
                                                      HANDLE Object );

typedef struct tagCSRSS_EXPORTED_FUNCS
{
  CSRSS_INSERT_OBJECT_PROC CsrInsertObjectProc;
  CSRSS_GET_OBJECT_PROC CsrGetObjectProc;
  CSRSS_RELEASE_OBJECT_PROC CsrReleaseObjectProc;
} CSRSS_EXPORTED_FUNCS, *PCSRSS_EXPORTED_FUNCS;

typedef BOOL (STDCALL *CSRPLUGIN_INIT_COMPLETE_PROC)(void);

typedef BOOL (STDCALL *CSRPLUGIN_INITIALIZE_PROC)(PCSRSS_API_DEFINITION *ApiDefinitions,
                                                  PCSRSS_OBJECT_DEFINITION *ObjectDefinitions,
                                                  CSRPLUGIN_INIT_COMPLETE_PROC *InitCompleteProc,
                                                  PCSRSS_EXPORTED_FUNCS Exports,
                                                  HANDLE CsrssApiHeap);

#endif /* CSRPLUGIN_H_INCLUDED */

/* EOF */
