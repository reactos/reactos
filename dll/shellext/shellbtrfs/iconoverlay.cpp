/* Copyright (c) Mark Harmstone 2016-17
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

#include "shellext.h"
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
#include "btrfsioctl.h"
#endif

HRESULT __stdcall BtrfsIconOverlay::QueryInterface(REFIID riid, void **ppObj) {
    if (riid == IID_IUnknown || riid == IID_IShellIconOverlayIdentifier) {
        *ppObj = static_cast<IShellIconOverlayIdentifier*>(this);
        AddRef();
        return S_OK;
    }

    *ppObj = nullptr;
    return E_NOINTERFACE;
}

HRESULT __stdcall BtrfsIconOverlay::GetOverlayInfo(PWSTR pwszIconFile, int cchMax, int* pIndex, DWORD* pdwFlags) noexcept {
    if (GetModuleFileNameW(module, pwszIconFile, cchMax) == 0)
        return E_FAIL;

    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        return E_FAIL;

    if (!pIndex)
        return E_INVALIDARG;

    if (!pdwFlags)
        return E_INVALIDARG;

    *pIndex = 0;
    *pdwFlags = ISIOI_ICONFILE | ISIOI_ICONINDEX;

    return S_OK;
}

HRESULT __stdcall BtrfsIconOverlay::GetPriority(int *pPriority) noexcept {
    if (!pPriority)
        return E_INVALIDARG;

    *pPriority = 0;

    return S_OK;
}

HRESULT __stdcall BtrfsIconOverlay::IsMemberOf(PCWSTR pwszPath, DWORD dwAttrib) noexcept {
    win_handle h;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_get_file_ids bgfi;

    h = CreateFileW(pwszPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

    if (h == INVALID_HANDLE_VALUE)
        return S_FALSE;

    Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_FILE_IDS, nullptr, 0, &bgfi, sizeof(btrfs_get_file_ids));

    if (!NT_SUCCESS(Status))
        return S_FALSE;

    return (bgfi.inode == 0x100 && !bgfi.top) ? S_OK : S_FALSE;
}
