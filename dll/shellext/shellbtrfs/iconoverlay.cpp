/* Copyright (c) Mark Harmstone 2016
 * 
 * This file is part of WinBtrfs.
 * 
 * WinBtrfs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public Licence as published by
 * the Free Software Foundation, either version 3 of the Licence, or
 * (at your option) any later version.
 * 
 * WinBtrfs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public Licence for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public Licence
 * along with WinBtrfs.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef __REACTOS__
#include <windows.h>
#include <winternl.h>
#else
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <ndk/iofuncs.h>
#endif
#include "iconoverlay.h"
#ifndef __REACTOS__
#include "../btrfsioctl.h"
#else
#include "../../drivers/filesystems/btrfs/btrfsioctl.h"
#endif

#ifndef __REACTOS__
#ifdef __cplusplus
extern "C" {
#endif
NTSYSCALLAPI NTSTATUS NTAPI NtFsControlFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG FsControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);
#ifdef __cplusplus
}
#endif
#define STATUS_SUCCESS 0
#endif

extern HMODULE module;

HRESULT __stdcall BtrfsIconOverlay::QueryInterface(REFIID riid, void **ppObj) {
    if (riid == IID_IUnknown || riid == IID_IShellIconOverlayIdentifier) {
        *ppObj = static_cast<IShellIconOverlayIdentifier*>(this); 
        AddRef();
        return S_OK;
    }

    *ppObj = NULL;
    return E_NOINTERFACE;
}

HRESULT __stdcall BtrfsIconOverlay::GetOverlayInfo(PWSTR pwszIconFile, int cchMax, int* pIndex, DWORD* pdwFlags) {
    WCHAR dllpath[MAX_PATH];
    
    GetModuleFileNameW(module, dllpath, sizeof(dllpath));
    
    if (cchMax < (int)wcslen(dllpath))
        return E_INVALIDARG;
    
    if (!pIndex)
        return E_INVALIDARG;
    
    if (!pdwFlags)
        return E_INVALIDARG;
    
    wcscpy(pwszIconFile, dllpath);
    *pIndex = 0;
    *pdwFlags = ISIOI_ICONFILE | ISIOI_ICONINDEX;
    
    return S_OK;
}

HRESULT __stdcall BtrfsIconOverlay::GetPriority(int *pPriority) {
    if (!pPriority)
        return E_INVALIDARG;
    
    *pPriority = 0;
    
    return S_OK;
}

HRESULT __stdcall BtrfsIconOverlay::IsMemberOf(PCWSTR pwszPath, DWORD dwAttrib) {
    HANDLE h;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_get_file_ids bgfi;
    
    h = CreateFileW(pwszPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    
    if (h == INVALID_HANDLE_VALUE)
        return S_FALSE;
    
    Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_FILE_IDS, NULL, 0, &bgfi, sizeof(btrfs_get_file_ids));
    
    if (Status != STATUS_SUCCESS) {
        CloseHandle(h);
        return S_FALSE;
    }

    CloseHandle(h);
    
    return (bgfi.inode == 0x100 && !bgfi.top) ? S_OK : S_FALSE;
}
