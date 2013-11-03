/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/virtualmemory.hpp
 * PURPOSE:         Virtual memory header
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

#pragma once

class VIRTUAL_MEMORY
{
public:
    NTSTATUS Allocate(IN HANDLE ProcessHandle, IN OUT PVOID *BaseAddress, IN ULONG ZeroBits, IN OUT PSIZE_T RegionSize, IN ULONG AllocationType, IN ULONG Protect);
    NTSTATUS Free(IN HANDLE ProcessHandle, IN OUT PVOID *BaseAddress, IN OUT PSIZE_T RegionSize, IN ULONG FreeType);
    NTSTATUS Lock(IN HANDLE ProcessHandle, IN OUT PVOID *BaseAddress, IN OUT PSIZE_T RegionSize, IN ULONG MapType);
    NTSTATUS Unlock(IN HANDLE ProcessHandle, IN OUT PVOID *BaseAddress, IN OUT PSIZE_T RegionSize, IN ULONG MapType);

private:
};