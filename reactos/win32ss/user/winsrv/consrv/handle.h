/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/handle.h
 * PURPOSE:         Console I/O Handles functions
 * PROGRAMMERS:     David Welch
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

VOID
ConSrvInitObject(IN OUT PCONSOLE_IO_OBJECT Object,
                 IN CONSOLE_IO_OBJECT_TYPE Type,
                 IN PCONSOLE Console);
NTSTATUS
ConSrvInsertObject(IN PCONSOLE_PROCESS_DATA ProcessData,
                   OUT PHANDLE Handle,
                   IN PCONSOLE_IO_OBJECT Object,
                   IN ULONG Access,
                   IN BOOLEAN Inheritable,
                   IN ULONG ShareMode);
NTSTATUS
ConSrvRemoveObject(IN PCONSOLE_PROCESS_DATA ProcessData,
                   IN HANDLE Handle);
NTSTATUS
ConSrvGetObject(IN PCONSOLE_PROCESS_DATA ProcessData,
                IN HANDLE Handle,
                OUT PCONSOLE_IO_OBJECT* Object,
                OUT PVOID* Entry OPTIONAL,
                IN ULONG Access,
                IN BOOLEAN LockConsole,
                IN CONSOLE_IO_OBJECT_TYPE Type);
VOID
ConSrvReleaseObject(IN PCONSOLE_IO_OBJECT Object,
                    IN BOOLEAN IsConsoleLocked);
