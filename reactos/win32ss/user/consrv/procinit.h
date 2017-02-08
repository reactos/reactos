/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/procinit.h
 * PURPOSE:         Functions for console processes initialization
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

NTSTATUS FASTCALL ConSrvAllocateConsole(PCONSOLE_PROCESS_DATA ProcessData,
                                        PHANDLE pInputHandle,
                                        PHANDLE pOutputHandle,
                                        PHANDLE pErrorHandle,
                                        PCONSOLE_START_INFO ConsoleStartInfo);
NTSTATUS FASTCALL ConSrvInheritConsole(PCONSOLE_PROCESS_DATA ProcessData,
                                       struct _CONSOLE* /* PCONSOLE */ Console,
                                       BOOL CreateNewHandlesTable,
                                       PHANDLE pInputHandle,
                                       PHANDLE pOutputHandle,
                                       PHANDLE pErrorHandle);
VOID FASTCALL ConSrvRemoveConsole(PCONSOLE_PROCESS_DATA ProcessData);

/* EOF */
