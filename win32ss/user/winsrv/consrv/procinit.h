/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/procinit.h
 * PURPOSE:         Functions for console processes initialization
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

NTSTATUS
ConSrvAllocateConsole(
    IN OUT PCONSOLE_PROCESS_DATA ProcessData,
    OUT PHANDLE pInputHandle,
    OUT PHANDLE pOutputHandle,
    OUT PHANDLE pErrorHandle,
    IN OUT PCONSOLE_INIT_INFO ConsoleInitInfo);

NTSTATUS
ConSrvInheritConsole(
    IN OUT PCONSOLE_PROCESS_DATA ProcessData,
    IN HANDLE ConsoleHandle,
    IN BOOLEAN CreateNewHandleTable,
    OUT PHANDLE pInputHandle,
    OUT PHANDLE pOutputHandle,
    OUT PHANDLE pErrorHandle,
    IN OUT PCONSOLE_START_INFO ConsoleStartInfo);

NTSTATUS
ConSrvRemoveConsole(
    IN OUT PCONSOLE_PROCESS_DATA ProcessData);
