/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            consrv/procinit.h
 * PURPOSE:         Functions for console processes initialization
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

NTSTATUS ConSrvAllocateConsole(PCONSOLE_PROCESS_DATA ProcessData,
                               PHANDLE pInputHandle,
                               PHANDLE pOutputHandle,
                               PHANDLE pErrorHandle,
                               PCONSOLE_INIT_INFO ConsoleInitInfo);
NTSTATUS ConSrvInheritConsole(PCONSOLE_PROCESS_DATA ProcessData,
                              HANDLE ConsoleHandle,
                              BOOLEAN CreateNewHandlesTable,
                              PHANDLE pInputHandle,
                              PHANDLE pOutputHandle,
                              PHANDLE pErrorHandle,
                              PCONSOLE_START_INFO ConsoleStartInfo);
NTSTATUS ConSrvRemoveConsole(PCONSOLE_PROCESS_DATA ProcessData);
