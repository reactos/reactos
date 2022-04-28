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
#include <strsafe.h>
#include <stddef.h>
#include <winternl.h>
#else
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <strsafe.h>
#include <shellapi.h>
#include <winioctl.h>
#include <ndk/iofuncs.h>
#undef DeleteFile
#endif
#include <wincodec.h>
#include <sstream>
#include <iostream>

#define NO_SHLWAPI_STRFCNS
#include <shlwapi.h>

#include "contextmenu.h"
#include "resource.h"
#ifndef __REACTOS__
#include "../btrfsioctl.h"
#else
#include "btrfsioctl.h"
#endif

#define NEW_SUBVOL_VERBA "newsubvol"
#define NEW_SUBVOL_VERBW L"newsubvol"
#define SNAPSHOT_VERBA "snapshot"
#define SNAPSHOT_VERBW L"snapshot"
#define REFLINK_VERBA "reflink"
#define REFLINK_VERBW L"reflink"
#define RECV_VERBA "recvsubvol"
#define RECV_VERBW L"recvsubvol"
#define SEND_VERBA "sendsubvol"
#define SEND_VERBW L"sendsubvol"

typedef struct {
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
} reparse_header;

static void path_remove_file(wstring& path);

// FIXME - don't assume subvol's top inode is 0x100

HRESULT __stdcall BtrfsContextMenu::QueryInterface(REFIID riid, void **ppObj) {
    if (riid == IID_IUnknown || riid == IID_IContextMenu) {
        *ppObj = static_cast<IContextMenu*>(this);
        AddRef();
        return S_OK;
    } else if (riid == IID_IShellExtInit) {
        *ppObj = static_cast<IShellExtInit*>(this);
        AddRef();
        return S_OK;
    }

    *ppObj = nullptr;
    return E_NOINTERFACE;
}

HRESULT __stdcall BtrfsContextMenu::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID) {
    IO_STATUS_BLOCK iosb;
    btrfs_get_file_ids bgfi;
    NTSTATUS Status;

    if (!pidlFolder) {
        FORMATETC format = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        UINT num_files, i;
        WCHAR fn[MAX_PATH];
        HDROP hdrop;

        if (!pdtobj)
            return E_FAIL;

        stgm.tymed = TYMED_HGLOBAL;

        if (FAILED(pdtobj->GetData(&format, &stgm)))
            return E_INVALIDARG;

        stgm_set = true;

        hdrop = (HDROP)GlobalLock(stgm.hGlobal);

        if (!hdrop) {
            ReleaseStgMedium(&stgm);
            stgm_set = false;
            return E_INVALIDARG;
        }

        num_files = DragQueryFileW((HDROP)stgm.hGlobal, 0xFFFFFFFF, nullptr, 0);

        for (i = 0; i < num_files; i++) {
            if (DragQueryFileW((HDROP)stgm.hGlobal, i, fn, sizeof(fn) / sizeof(WCHAR))) {
                win_handle h = CreateFileW(fn, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

                if (h != INVALID_HANDLE_VALUE) {
                    Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_FILE_IDS, nullptr, 0, &bgfi, sizeof(btrfs_get_file_ids));

                    if (NT_SUCCESS(Status) && bgfi.inode == 0x100 && !bgfi.top) {
                        wstring parpath;

                        {
                            win_handle h2;

                            parpath = fn;
                            path_remove_file(parpath);

                            h2 = CreateFileW(parpath.c_str(), FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

                            if (h2 != INVALID_HANDLE_VALUE)
                                allow_snapshot = true;
                        }

                        ignore = false;
                        bg = false;

                        GlobalUnlock(hdrop);
                        return S_OK;
                    }
                }
            }
        }

        GlobalUnlock(hdrop);

        return S_OK;
    }

    {
        WCHAR pathbuf[MAX_PATH];

        if (!SHGetPathFromIDListW(pidlFolder, pathbuf))
            return E_FAIL;

        path = pathbuf;
    }

    {
        // check we have permissions to create new subdirectory

        win_handle h = CreateFileW(path.c_str(), FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

        if (h == INVALID_HANDLE_VALUE)
            return E_FAIL;

        // check is Btrfs volume

        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_FILE_IDS, nullptr, 0, &bgfi, sizeof(btrfs_get_file_ids));

        if (!NT_SUCCESS(Status))
            return E_FAIL;
    }

    ignore = false;
    bg = true;

    return S_OK;
}

static bool get_volume_path_parent(const WCHAR* fn, WCHAR* volpath, ULONG volpathlen) {
    WCHAR *f, *p;
    bool b;

    f = PathFindFileNameW(fn);

    if (f == fn)
        return GetVolumePathNameW(fn, volpath, volpathlen);

    p = (WCHAR*)malloc((f - fn + 1) * sizeof(WCHAR));
    memcpy(p, fn, (f - fn) * sizeof(WCHAR));
    p[f - fn] = 0;

    b = GetVolumePathNameW(p, volpath, volpathlen);

    free(p);

    return b;
}

static bool show_reflink_paste(const wstring& path) {
    HDROP hdrop;
    HANDLE lh;
    ULONG num_files;
    WCHAR fn[MAX_PATH], volpath1[255], volpath2[255];

    if (!IsClipboardFormatAvailable(CF_HDROP))
        return false;

    if (!GetVolumePathNameW(path.c_str(), volpath1, sizeof(volpath1) / sizeof(WCHAR)))
        return false;

    if (!OpenClipboard(nullptr))
        return false;

    hdrop = (HDROP)GetClipboardData(CF_HDROP);

    if (!hdrop) {
        CloseClipboard();
        return false;
    }

    lh = GlobalLock(hdrop);

    if (!lh) {
        CloseClipboard();
        return false;
    }

    num_files = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);

    if (num_files == 0) {
        GlobalUnlock(lh);
        CloseClipboard();
        return false;
    }

    if (!DragQueryFileW(hdrop, 0, fn, sizeof(fn) / sizeof(WCHAR))) {
        GlobalUnlock(lh);
        CloseClipboard();
        return false;
    }

    if (!get_volume_path_parent(fn, volpath2, sizeof(volpath2) / sizeof(WCHAR))) {
        GlobalUnlock(lh);
        CloseClipboard();
        return false;
    }

    GlobalUnlock(lh);

    CloseClipboard();

    return !wcscmp(volpath1, volpath2);
}

// The code for putting an icon against a menu item comes from:
// http://web.archive.org/web/20070208005514/http://shellrevealed.com/blogs/shellblog/archive/2007/02/06/Vista-Style-Menus_2C00_-Part-1-_2D00_-Adding-icons-to-standard-menus.aspx

static void InitBitmapInfo(BITMAPINFO* pbmi, ULONG cbInfo, LONG cx, LONG cy, WORD bpp) {
    ZeroMemory(pbmi, cbInfo);
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biCompression = BI_RGB;

    pbmi->bmiHeader.biWidth = cx;
    pbmi->bmiHeader.biHeight = cy;
    pbmi->bmiHeader.biBitCount = bpp;
}

static HRESULT Create32BitHBITMAP(HDC hdc, const SIZE *psize, void **ppvBits, HBITMAP* phBmp) {
    BITMAPINFO bmi;
    HDC hdcUsed;

    *phBmp = nullptr;

    InitBitmapInfo(&bmi, sizeof(bmi), psize->cx, psize->cy, 32);

    hdcUsed = hdc ? hdc : GetDC(nullptr);

    if (hdcUsed) {
        *phBmp = CreateDIBSection(hdcUsed, &bmi, DIB_RGB_COLORS, ppvBits, nullptr, 0);
        if (hdc != hdcUsed)
            ReleaseDC(nullptr, hdcUsed);
    }

    return !*phBmp ? E_OUTOFMEMORY : S_OK;
}

void BtrfsContextMenu::get_uac_icon() {
    IWICImagingFactory* factory = nullptr;
    IWICBitmap* bitmap;
    HRESULT hr;

#ifdef __REACTOS__
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (void **)&factory);
#else
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
#endif

    if (SUCCEEDED(hr)) {
        HANDLE icon;

        // We can't use IDI_SHIELD, as that will only give us the full-size icon
        icon = LoadImageW(GetModuleHandleW(L"user32.dll"), MAKEINTRESOURCEW(106)/* UAC shield */, IMAGE_ICON,
                          GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

        hr = factory->CreateBitmapFromHICON((HICON)icon, &bitmap);
        if (SUCCEEDED(hr)) {
            UINT cx, cy;

            hr = bitmap->GetSize(&cx, &cy);
            if (SUCCEEDED(hr)) {
                SIZE sz;
                BYTE* buf;

                sz.cx = (int)cx;
                sz.cy = -(int)cy;

                hr = Create32BitHBITMAP(nullptr, &sz, (void**)&buf, &uacicon);
                if (SUCCEEDED(hr)) {
                    UINT stride = (UINT)(cx * sizeof(DWORD));
                    UINT buflen = cy * stride;
                    bitmap->CopyPixels(nullptr, stride, buflen, buf);
                }
            }

            bitmap->Release();
        }

        factory->Release();
    }
}

HRESULT __stdcall BtrfsContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) {
    wstring str;
    ULONG entries = 0;

    if (ignore)
        return E_INVALIDARG;

    if (uFlags & CMF_DEFAULTONLY)
        return S_OK;

    if (!bg) {
        if (allow_snapshot) {
            if (load_string(module, IDS_CREATE_SNAPSHOT, str) == 0)
                return E_FAIL;

            if (!InsertMenuW(hmenu, indexMenu, MF_BYPOSITION, idCmdFirst, str.c_str()))
                return E_FAIL;

            entries = 1;
        }

        if (idCmdFirst + entries <= idCmdLast) {
            MENUITEMINFOW mii;

            if (load_string(module, IDS_SEND_SUBVOL, str) == 0)
                return E_FAIL;

            if (!uacicon)
                get_uac_icon();

            memset(&mii, 0, sizeof(MENUITEMINFOW));
            mii.cbSize = sizeof(MENUITEMINFOW);
            mii.fMask = MIIM_STRING | MIIM_ID | MIIM_BITMAP;
            mii.dwTypeData = (WCHAR*)str.c_str();
            mii.wID = idCmdFirst + entries;
            mii.hbmpItem = uacicon;

            if (!InsertMenuItemW(hmenu, indexMenu + entries, true, &mii))
                return E_FAIL;

            entries++;
        }
    } else {
        if (load_string(module, IDS_NEW_SUBVOL, str) == 0)
            return E_FAIL;

        if (!InsertMenuW(hmenu, indexMenu, MF_BYPOSITION, idCmdFirst, str.c_str()))
            return E_FAIL;

        entries = 1;

        if (idCmdFirst + 1 <= idCmdLast) {
            MENUITEMINFOW mii;

            if (load_string(module, IDS_RECV_SUBVOL, str) == 0)
                return E_FAIL;

            if (!uacicon)
                get_uac_icon();

            memset(&mii, 0, sizeof(MENUITEMINFOW));
            mii.cbSize = sizeof(MENUITEMINFOW);
            mii.fMask = MIIM_STRING | MIIM_ID | MIIM_BITMAP;
            mii.dwTypeData = (WCHAR*)str.c_str();
            mii.wID = idCmdFirst + 1;
            mii.hbmpItem = uacicon;

            if (!InsertMenuItemW(hmenu, indexMenu + 1, true, &mii))
                return E_FAIL;

            entries++;
        }

        if (idCmdFirst + 2 <= idCmdLast && show_reflink_paste(path)) {
            if (load_string(module, IDS_REFLINK_PASTE, str) == 0)
                return E_FAIL;

            if (!InsertMenuW(hmenu, indexMenu + 2, MF_BYPOSITION, idCmdFirst + 2, str.c_str()))
                return E_FAIL;

            entries++;
        }
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, entries);
}

static void path_remove_file(wstring& path) {
    size_t bs = path.rfind(L"\\");

    if (bs == string::npos)
        return;

    if (bs == path.find(L"\\")) { // only one backslash
        path = path.substr(0, bs + 1);
        return;
    }

    path = path.substr(0, bs);
}

static void path_strip_path(wstring& path) {
    size_t bs = path.rfind(L"\\");

    if (bs == string::npos) {
        path = L"";
        return;
    }

    path = path.substr(bs + 1);
}

static void create_snapshot(HWND hwnd, const wstring& fn) {
    win_handle h;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_get_file_ids bgfi;

    h = CreateFileW(fn.c_str(), FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

    if (h != INVALID_HANDLE_VALUE) {
        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_FILE_IDS, nullptr, 0, &bgfi, sizeof(btrfs_get_file_ids));

        if (NT_SUCCESS(Status) && bgfi.inode == 0x100 && !bgfi.top) {
            wstring subvolname, parpath, searchpath, temp1, name, nameorig;
            win_handle h2;
            WIN32_FIND_DATAW wfd;
            SYSTEMTIME time;

            parpath = fn;
            path_remove_file(parpath);

            subvolname = fn;
            path_strip_path(subvolname);

            h2 = CreateFileW(parpath.c_str(), FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

            if (h2 == INVALID_HANDLE_VALUE)
                throw last_error(GetLastError());

            if (!load_string(module, IDS_SNAPSHOT_FILENAME, temp1))
                throw last_error(GetLastError());

            GetLocalTime(&time);

            wstring_sprintf(name, temp1, subvolname.c_str(), time.wYear, time.wMonth, time.wDay);
            nameorig = name;

            searchpath = parpath + L"\\" + name;

            fff_handle fff = FindFirstFileW(searchpath.c_str(), &wfd);

            if (fff != INVALID_HANDLE_VALUE) {
                ULONG num = 2;

                do {
#ifndef __REACTOS__
                    name = nameorig + L" (" + to_wstring(num) + L")";
#else
                    {
                        WCHAR buffer[32];

                        swprintf(buffer, L"%d", num);
                        name = nameorig + L" (" + buffer + L")";
                    }
#endif
                    searchpath = parpath + L"\\" + name;

                    fff = FindFirstFileW(searchpath.c_str(), &wfd);
                    num++;
                } while (fff != INVALID_HANDLE_VALUE);
            }

            size_t namelen = name.length() * sizeof(WCHAR);

            auto bcs = (btrfs_create_snapshot*)malloc(sizeof(btrfs_create_snapshot) - 1 + namelen);
            bcs->readonly = false;
            bcs->posix = false;
            bcs->subvol = h;
            bcs->namelen = (uint16_t)namelen;
            memcpy(bcs->name, name.c_str(), namelen);

            Status = NtFsControlFile(h2, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_CREATE_SNAPSHOT, bcs,
                                     (ULONG)(sizeof(btrfs_create_snapshot) - 1 + namelen), nullptr, 0);

            if (!NT_SUCCESS(Status))
                throw ntstatus_error(Status);
        }
    } else
        throw last_error(GetLastError());
}

static uint64_t __inline sector_align(uint64_t n, uint64_t a) {
    if (n & (a - 1))
        n = (n + a) & ~(a - 1);

    return n;
}

void BtrfsContextMenu::reflink_copy(HWND hwnd, const WCHAR* fn, const WCHAR* dir) {
    win_handle source, dest;
    WCHAR* name, volpath1[255], volpath2[255];
    wstring dirw, newpath;
    FILE_BASIC_INFO fbi;
    FILETIME atime, mtime;
    btrfs_inode_info bii;
    btrfs_set_inode_info bsii;
    ULONG bytesret;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_set_xattr bsxa;

    // Thanks to 0xbadfca11, whose version of reflink for Windows provided a few pointers on what
    // to do here - https://github.com/0xbadfca11/reflink

    name = PathFindFileNameW(fn);

    dirw = dir;

    if (dir[0] != 0 && dir[wcslen(dir) - 1] != '\\')
        dirw += L"\\";

    newpath = dirw;
    newpath += name;

    if (!get_volume_path_parent(fn, volpath1, sizeof(volpath1) / sizeof(WCHAR)))
        throw last_error(GetLastError());

    if (!GetVolumePathNameW(dir, volpath2, sizeof(volpath2) / sizeof(WCHAR)))
        throw last_error(GetLastError());

    if (wcscmp(volpath1, volpath2)) // different filesystems
        throw string_error(IDS_CANT_REFLINK_DIFFERENT_FS);

    source = CreateFileW(fn, GENERIC_READ | FILE_TRAVERSE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT, nullptr);
    if (source == INVALID_HANDLE_VALUE)
        throw last_error(GetLastError());

    Status = NtFsControlFile(source, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_INODE_INFO, nullptr, 0, &bii, sizeof(btrfs_inode_info));
    if (!NT_SUCCESS(Status))
        throw ntstatus_error(Status);

    // if subvol, do snapshot instead
    if (bii.inode == SUBVOL_ROOT_INODE) {
        btrfs_create_snapshot* bcs;
        win_handle dirh;
        wstring destname, search;
        WIN32_FIND_DATAW wfd;
        int num = 2;

        dirh = CreateFileW(dir, FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (dirh == INVALID_HANDLE_VALUE)
            throw last_error(GetLastError());

        search = dirw;
        search += name;
        destname = name;

        fff_handle fff = FindFirstFileW(search.c_str(), &wfd);

        if (fff != INVALID_HANDLE_VALUE) {
            do {
                wstringstream ss;

                ss << name;
                ss << L" (";
                ss << num;
                ss << L")";
                destname = ss.str();

                search = dirw + destname;

                fff = FindFirstFileW(search.c_str(), &wfd);
                num++;
            } while (fff != INVALID_HANDLE_VALUE);
        }

        bcs = (btrfs_create_snapshot*)malloc(sizeof(btrfs_create_snapshot) - sizeof(WCHAR) + (destname.length() * sizeof(WCHAR)));
        bcs->subvol = source;
        bcs->namelen = (uint16_t)(destname.length() * sizeof(WCHAR));
        memcpy(bcs->name, destname.c_str(), destname.length() * sizeof(WCHAR));

        Status = NtFsControlFile(dirh, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_CREATE_SNAPSHOT, bcs,
                                 (ULONG)(sizeof(btrfs_create_snapshot) - sizeof(WCHAR) + bcs->namelen), nullptr, 0);

        free(bcs);

        if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);

        return;
    }

    Status = NtQueryInformationFile(source, &iosb, &fbi, sizeof(FILE_BASIC_INFO), FileBasicInformation);
    if (!NT_SUCCESS(Status))
        throw ntstatus_error(Status);

    if (bii.type == BTRFS_TYPE_CHARDEV || bii.type == BTRFS_TYPE_BLOCKDEV || bii.type == BTRFS_TYPE_FIFO || bii.type == BTRFS_TYPE_SOCKET) {
        win_handle dirh;
        btrfs_mknod* bmn;

        dirh = CreateFileW(dir, FILE_ADD_FILE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (dirh == INVALID_HANDLE_VALUE)
            throw last_error(GetLastError());

        size_t bmnsize = offsetof(btrfs_mknod, name[0]) + (wcslen(name) * sizeof(WCHAR));
        bmn = (btrfs_mknod*)malloc(bmnsize);

        bmn->inode = 0;
        bmn->type = bii.type;
        bmn->st_rdev = bii.st_rdev;
        bmn->namelen = (uint16_t)(wcslen(name) * sizeof(WCHAR));
        memcpy(bmn->name, name, bmn->namelen);

        Status = NtFsControlFile(dirh, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_MKNOD, bmn, (ULONG)bmnsize, nullptr, 0);
        if (!NT_SUCCESS(Status)) {
            free(bmn);
            throw ntstatus_error(Status);
        }

        free(bmn);

        dest = CreateFileW(newpath.c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    } else if (fbi.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (CreateDirectoryExW(fn, newpath.c_str(), nullptr))
            dest = CreateFileW(newpath.c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        else
            dest = INVALID_HANDLE_VALUE;
    } else
        dest = CreateFileW(newpath.c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, nullptr, CREATE_NEW, 0, source);

    if (dest == INVALID_HANDLE_VALUE) {
        int num = 2;

        if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS && wcscmp(fn, newpath.c_str()))
            throw last_error(GetLastError());

        do {
            WCHAR* ext;
            wstringstream ss;

            ext = PathFindExtensionW(fn);

            ss << dirw;

            if (*ext == 0) {
                ss << name;
                ss << L" (";
                ss << num;
                ss << L")";
            } else {
                wstring namew = name;

                ss << namew.substr(0, ext - name);
                ss << L" (";
                ss << num;
                ss << L")";
                ss << ext;
            }

            newpath = ss.str();
            if (fbi.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (CreateDirectoryExW(fn, newpath.c_str(), nullptr))
                    dest = CreateFileW(newpath.c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
                else
                    dest = INVALID_HANDLE_VALUE;
            } else
                dest = CreateFileW(newpath.c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, nullptr, CREATE_NEW, 0, source);

            if (dest == INVALID_HANDLE_VALUE) {
                if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS)
                    throw last_error(GetLastError());

                num++;
            } else
                break;
        } while (true);
    }

    try {
        memset(&bsii, 0, sizeof(btrfs_set_inode_info));

        bsii.flags_changed = true;
        bsii.flags = bii.flags;

        if (bii.flags & BTRFS_INODE_COMPRESS) {
            bsii.compression_type_changed = true;
            bsii.compression_type = bii.compression_type;
        }

        Status = NtFsControlFile(dest, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_SET_INODE_INFO, &bsii, sizeof(btrfs_set_inode_info), nullptr, 0);
        if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);

        if (fbi.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!(fbi.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                fff_handle h;
                WIN32_FIND_DATAW fff;
                wstring qs;

                qs = fn;
                qs += L"\\*";

                h = FindFirstFileW(qs.c_str(), &fff);
                if (h != INVALID_HANDLE_VALUE) {
                    do {
                        wstring fn2;

                        if (fff.cFileName[0] == '.' && (fff.cFileName[1] == 0 || (fff.cFileName[1] == '.' && fff.cFileName[2] == 0)))
                            continue;

                        fn2 = fn;
                        fn2 += L"\\";
                        fn2 += fff.cFileName;

                        reflink_copy(hwnd, fn2.c_str(), newpath.c_str());
                    } while (FindNextFileW(h, &fff));
                }
            }

            // CreateDirectoryExW also copies streams, no need to do it here
        } else {
            if (fbi.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                reparse_header rh;
                uint8_t* rp;

                if (!DeviceIoControl(source, FSCTL_GET_REPARSE_POINT, nullptr, 0, &rh, sizeof(reparse_header), &bytesret, nullptr)) {
                    if (GetLastError() != ERROR_MORE_DATA)
                        throw last_error(GetLastError());
                }

                size_t rplen = sizeof(reparse_header) + rh.ReparseDataLength;
                rp = (uint8_t*)malloc(rplen);

                if (!DeviceIoControl(source, FSCTL_GET_REPARSE_POINT, nullptr, 0, rp, (ULONG)rplen, &bytesret, nullptr))
                    throw last_error(GetLastError());

                if (!DeviceIoControl(dest, FSCTL_SET_REPARSE_POINT, rp, (ULONG)rplen, nullptr, 0, &bytesret, nullptr))
                    throw last_error(GetLastError());

                free(rp);
            } else {
                FILE_STANDARD_INFO fsi;
                FILE_END_OF_FILE_INFO feofi;
                FSCTL_GET_INTEGRITY_INFORMATION_BUFFER fgiib;
                FSCTL_SET_INTEGRITY_INFORMATION_BUFFER fsiib;
                DUPLICATE_EXTENTS_DATA ded;
                uint64_t offset, alloc_size;
                ULONG maxdup;

                Status = NtQueryInformationFile(source, &iosb, &fsi, sizeof(FILE_STANDARD_INFO), FileStandardInformation);
                if (!NT_SUCCESS(Status))
                    throw ntstatus_error(Status);

                if (!DeviceIoControl(source, FSCTL_GET_INTEGRITY_INFORMATION, nullptr, 0, &fgiib, sizeof(FSCTL_GET_INTEGRITY_INFORMATION_BUFFER), &bytesret, nullptr))
                    throw last_error(GetLastError());

                if (fbi.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) {
                    if (!DeviceIoControl(dest, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &bytesret, nullptr))
                        throw last_error(GetLastError());
                }

                fsiib.ChecksumAlgorithm = fgiib.ChecksumAlgorithm;
                fsiib.Reserved = 0;
                fsiib.Flags = fgiib.Flags;
                if (!DeviceIoControl(dest, FSCTL_SET_INTEGRITY_INFORMATION, &fsiib, sizeof(FSCTL_SET_INTEGRITY_INFORMATION_BUFFER), nullptr, 0, &bytesret, nullptr))
                    throw last_error(GetLastError());

                feofi.EndOfFile = fsi.EndOfFile;
                Status = NtSetInformationFile(dest, &iosb, &feofi, sizeof(FILE_END_OF_FILE_INFO), FileEndOfFileInformation);
                if (!NT_SUCCESS(Status))
                    throw ntstatus_error(Status);

                ded.FileHandle = source;
                maxdup = 0xffffffff - fgiib.ClusterSizeInBytes + 1;

                alloc_size = sector_align(fsi.EndOfFile.QuadPart, fgiib.ClusterSizeInBytes);

                offset = 0;
                while (offset < alloc_size) {
                    ded.SourceFileOffset.QuadPart = ded.TargetFileOffset.QuadPart = offset;
                    ded.ByteCount.QuadPart = maxdup < (alloc_size - offset) ? maxdup : (alloc_size - offset);
                    if (!DeviceIoControl(dest, FSCTL_DUPLICATE_EXTENTS_TO_FILE, &ded, sizeof(DUPLICATE_EXTENTS_DATA), nullptr, 0, &bytesret, nullptr))
                        throw last_error(GetLastError());

                    offset += ded.ByteCount.QuadPart;
                }
            }

            ULONG streambufsize = 0;
            vector<char> streambuf;

            do {
                streambufsize += 0x1000;
                streambuf.resize(streambufsize);

                memset(streambuf.data(), 0, streambufsize);

                Status = NtQueryInformationFile(source, &iosb, streambuf.data(), streambufsize, FileStreamInformation);
            } while (Status == STATUS_BUFFER_OVERFLOW);

            if (!NT_SUCCESS(Status))
                throw ntstatus_error(Status);

            auto fsi = reinterpret_cast<FILE_STREAM_INFORMATION*>(streambuf.data());

            while (true) {
                if (fsi->StreamNameLength > 0) {
                    wstring sn = wstring(fsi->StreamName, fsi->StreamNameLength / sizeof(WCHAR));

                    if (sn != L"::$DATA" && sn.length() > 6 && sn.substr(sn.length() - 6, 6) == L":$DATA") {
                        win_handle stream;
                        uint8_t* data = nullptr;
                        auto stream_size = (uint16_t)fsi->StreamSize.QuadPart;


                        if (stream_size > 0) {
                            wstring fn2;

                            fn2 = fn;
                            fn2 += sn;

                            stream = CreateFileW(fn2.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);

                            if (stream == INVALID_HANDLE_VALUE)
                                throw last_error(GetLastError());

                            // We can get away with this because our streams are guaranteed to be below 64 KB -
                            // don't do this on NTFS!
                            data = (uint8_t*)malloc(stream_size);

                            if (!ReadFile(stream, data, stream_size, &bytesret, nullptr)) {
                                free(data);
                                throw last_error(GetLastError());
                            }
                        }

                        stream = CreateFileW((newpath + sn).c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, nullptr, CREATE_NEW, 0, nullptr);

                        if (stream == INVALID_HANDLE_VALUE) {
                            if (data) free(data);
                            throw last_error(GetLastError());
                        }

                        if (data) {
                            if (!WriteFile(stream, data, stream_size, &bytesret, nullptr)) {
                                free(data);
                                throw last_error(GetLastError());
                            }

                            free(data);
                        }
                    }
                }

                if (fsi->NextEntryOffset == 0)
                    break;

                fsi = reinterpret_cast<FILE_STREAM_INFORMATION*>(reinterpret_cast<char*>(fsi) + fsi->NextEntryOffset);
            }
        }

        atime.dwLowDateTime = fbi.LastAccessTime.LowPart;
        atime.dwHighDateTime = fbi.LastAccessTime.HighPart;
        mtime.dwLowDateTime = fbi.LastWriteTime.LowPart;
        mtime.dwHighDateTime = fbi.LastWriteTime.HighPart;
        SetFileTime(dest, nullptr, &atime, &mtime);

        Status = NtFsControlFile(source, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_XATTRS, nullptr, 0, &bsxa, sizeof(btrfs_set_xattr));

        if (Status == STATUS_BUFFER_OVERFLOW || (NT_SUCCESS(Status) && bsxa.valuelen > 0)) {
            ULONG xalen = 0;
            btrfs_set_xattr *xa = nullptr, *xa2;

            do {
                xalen += 1024;

                if (xa) free(xa);
                xa = (btrfs_set_xattr*)malloc(xalen);

                Status = NtFsControlFile(source, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_XATTRS, nullptr, 0, xa, xalen);
            } while (Status == STATUS_BUFFER_OVERFLOW);

            if (!NT_SUCCESS(Status)) {
                free(xa);
                throw ntstatus_error(Status);
            }

            xa2 = xa;
            while (xa2->valuelen > 0) {
                Status = NtFsControlFile(dest, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_SET_XATTR, xa2,
                                         (ULONG)(offsetof(btrfs_set_xattr, data[0]) + xa2->namelen + xa2->valuelen), nullptr, 0);
                if (!NT_SUCCESS(Status)) {
                    free(xa);
                    throw ntstatus_error(Status);
                }
                xa2 = (btrfs_set_xattr*)&xa2->data[xa2->namelen + xa2->valuelen];
            }

            free(xa);
        } else if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);
    } catch (...) {
        FILE_DISPOSITION_INFO fdi;

        fdi.DeleteFile = true;
        Status = NtSetInformationFile(dest, &iosb, &fdi, sizeof(FILE_DISPOSITION_INFO), FileDispositionInformation);
        if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);

        throw;
    }
}

HRESULT __stdcall BtrfsContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO picia) {
    LPCMINVOKECOMMANDINFOEX pici = (LPCMINVOKECOMMANDINFOEX)picia;

    try {
        if (ignore)
            return E_INVALIDARG;

        if (!bg) {
            if ((IS_INTRESOURCE(pici->lpVerb) && allow_snapshot && pici->lpVerb == 0) || (!IS_INTRESOURCE(pici->lpVerb) && !strcmp(pici->lpVerb, SNAPSHOT_VERBA))) {
                UINT num_files, i;
                WCHAR fn[MAX_PATH];

                if (!stgm_set)
                    return E_FAIL;

                num_files = DragQueryFileW((HDROP)stgm.hGlobal, 0xFFFFFFFF, nullptr, 0);

                if (num_files == 0)
                    return E_FAIL;

                for (i = 0; i < num_files; i++) {
                    if (DragQueryFileW((HDROP)stgm.hGlobal, i, fn, sizeof(fn) / sizeof(WCHAR))) {
                        create_snapshot(pici->hwnd, fn);
                    }
                }

                return S_OK;
            } else if ((IS_INTRESOURCE(pici->lpVerb) && ((allow_snapshot && (ULONG_PTR)pici->lpVerb == 1) || (!allow_snapshot && (ULONG_PTR)pici->lpVerb == 0))) ||
                    (!IS_INTRESOURCE(pici->lpVerb) && !strcmp(pici->lpVerb, SEND_VERBA))) {
                UINT num_files, i;
                WCHAR dll[MAX_PATH], fn[MAX_PATH];
                wstring t;
                SHELLEXECUTEINFOW sei;

                GetModuleFileNameW(module, dll, sizeof(dll) / sizeof(WCHAR));

                if (!stgm_set)
                    return E_FAIL;

                num_files = DragQueryFileW((HDROP)stgm.hGlobal, 0xFFFFFFFF, nullptr, 0);

                if (num_files == 0)
                    return E_FAIL;

                for (i = 0; i < num_files; i++) {
                    if (DragQueryFileW((HDROP)stgm.hGlobal, i, fn, sizeof(fn) / sizeof(WCHAR))) {
                        t = L"\"";
                        t += dll;
                        t += L"\",SendSubvolGUI ";
                        t += fn;

                        RtlZeroMemory(&sei, sizeof(sei));

                        sei.cbSize = sizeof(sei);
                        sei.hwnd = pici->hwnd;
                        sei.lpVerb = L"runas";
                        sei.lpFile = L"rundll32.exe";
                        sei.lpParameters = t.c_str();
                        sei.nShow = SW_SHOW;
                        sei.fMask = SEE_MASK_NOCLOSEPROCESS;

                        if (!ShellExecuteExW(&sei))
                            throw last_error(GetLastError());

                        WaitForSingleObject(sei.hProcess, INFINITE);
                        CloseHandle(sei.hProcess);
                    }
                }

                return S_OK;
            }
        } else {
            if ((IS_INTRESOURCE(pici->lpVerb) && (ULONG_PTR)pici->lpVerb == 0) || (!IS_INTRESOURCE(pici->lpVerb) && !strcmp(pici->lpVerb, NEW_SUBVOL_VERBA))) {
                win_handle h;
                IO_STATUS_BLOCK iosb;
                NTSTATUS Status;
                wstring name, nameorig, searchpath;
                btrfs_create_subvol* bcs;
                WIN32_FIND_DATAW wfd;

                if (!load_string(module, IDS_NEW_SUBVOL_FILENAME, name))
                    throw last_error(GetLastError());

                h = CreateFileW(path.c_str(), FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

                if (h == INVALID_HANDLE_VALUE)
                    throw last_error(GetLastError());

                searchpath = path + L"\\" + name;
                nameorig = name;

                {
                    fff_handle fff = FindFirstFileW(searchpath.c_str(), &wfd);

                    if (fff != INVALID_HANDLE_VALUE) {
                        ULONG num = 2;

                        do {
#ifndef __REACTOS__
                            name = nameorig + L" (" + to_wstring(num) + L")";
#else
                            {
                                WCHAR buffer[32];

                                swprintf(buffer, L"%d", num);
                                name = nameorig + L" (" + buffer + L")";
                            }
#endif
                            searchpath = path + L"\\" + name;

                            fff = FindFirstFileW(searchpath.c_str(), &wfd);
                            num++;
                        } while (fff != INVALID_HANDLE_VALUE);
                    }
                }

                size_t bcslen = offsetof(btrfs_create_subvol, name[0]) + (name.length() * sizeof(WCHAR));
                bcs = (btrfs_create_subvol*)malloc(bcslen);

                bcs->readonly = false;
                bcs->posix = false;
                bcs->namelen = (uint16_t)(name.length() * sizeof(WCHAR));
                memcpy(bcs->name, name.c_str(), name.length() * sizeof(WCHAR));

                Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_CREATE_SUBVOL, bcs, (ULONG)bcslen, nullptr, 0);

                free(bcs);

                if (!NT_SUCCESS(Status))
                    throw ntstatus_error(Status);

                return S_OK;
            } else if ((IS_INTRESOURCE(pici->lpVerb) && (ULONG_PTR)pici->lpVerb == 1) || (!IS_INTRESOURCE(pici->lpVerb) && !strcmp(pici->lpVerb, RECV_VERBA))) {
                WCHAR dll[MAX_PATH];
                wstring t;
                SHELLEXECUTEINFOW sei;

                GetModuleFileNameW(module, dll, sizeof(dll) / sizeof(WCHAR));

                t = L"\"";
                t += dll;
                t += L"\",RecvSubvolGUI ";
                t += path;

                RtlZeroMemory(&sei, sizeof(sei));

                sei.cbSize = sizeof(sei);
                sei.hwnd = pici->hwnd;
                sei.lpVerb = L"runas";
                sei.lpFile = L"rundll32.exe";
                sei.lpParameters = t.c_str();
                sei.nShow = SW_SHOW;
                sei.fMask = SEE_MASK_NOCLOSEPROCESS;

                if (!ShellExecuteExW(&sei))
                    throw last_error(GetLastError());

                WaitForSingleObject(sei.hProcess, INFINITE);
                CloseHandle(sei.hProcess);

                return S_OK;
            } else if ((IS_INTRESOURCE(pici->lpVerb) && (ULONG_PTR)pici->lpVerb == 2) || (!IS_INTRESOURCE(pici->lpVerb) && !strcmp(pici->lpVerb, REFLINK_VERBA))) {
                HDROP hdrop;

                if (!IsClipboardFormatAvailable(CF_HDROP))
                    return S_OK;

                if (!OpenClipboard(pici->hwnd))
                    throw last_error(GetLastError());

                try {
                    hdrop = (HDROP)GetClipboardData(CF_HDROP);

                    if (hdrop) {
                        HANDLE lh;

                        lh = GlobalLock(hdrop);

                        if (lh) {
                            try {
                                ULONG num_files, i;
                                WCHAR fn[MAX_PATH];

                                num_files = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);

                                for (i = 0; i < num_files; i++) {
                                    if (DragQueryFileW(hdrop, i, fn, sizeof(fn) / sizeof(WCHAR))) {
                                        reflink_copy(pici->hwnd, fn, pici->lpDirectoryW);
                                    }
                                }
                            } catch (...) {
                                GlobalUnlock(lh);
                                throw;
                            }

                            GlobalUnlock(lh);
                        }
                    }
                } catch (...) {
                    CloseClipboard();
                    throw;
                }

                CloseClipboard();

                return S_OK;
            }
        }
    } catch (const exception& e) {
        error_message(pici->hwnd, e.what());
    }

    return E_FAIL;
}

HRESULT __stdcall BtrfsContextMenu::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT* pwReserved, LPSTR pszName, UINT cchMax) {
    if (ignore)
        return E_INVALIDARG;

    if (idCmd != 0)
        return E_INVALIDARG;

    if (!bg) {
        if (idCmd == 0) {
            switch (uFlags) {
                case GCS_HELPTEXTA:
                    if (LoadStringA(module, IDS_CREATE_SNAPSHOT_HELP_TEXT, pszName, cchMax))
                        return S_OK;
                    else
                        return E_FAIL;

                case GCS_HELPTEXTW:
                    if (LoadStringW(module, IDS_CREATE_SNAPSHOT_HELP_TEXT, (LPWSTR)pszName, cchMax))
                        return S_OK;
                    else
                        return E_FAIL;

                case GCS_VALIDATEA:
                case GCS_VALIDATEW:
                    return S_OK;

                case GCS_VERBA:
                    return StringCchCopyA(pszName, cchMax, SNAPSHOT_VERBA);

                case GCS_VERBW:
                    return StringCchCopyW((STRSAFE_LPWSTR)pszName, cchMax, SNAPSHOT_VERBW);

                default:
                    return E_INVALIDARG;
            }
        } else if (idCmd == 1) {
            switch (uFlags) {
                case GCS_HELPTEXTA:
                    if (LoadStringA(module, IDS_SEND_SUBVOL_HELP, pszName, cchMax))
                        return S_OK;
                    else
                        return E_FAIL;

                case GCS_HELPTEXTW:
                    if (LoadStringW(module, IDS_SEND_SUBVOL_HELP, (LPWSTR)pszName, cchMax))
                        return S_OK;
                    else
                        return E_FAIL;

                case GCS_VALIDATEA:
                case GCS_VALIDATEW:
                    return S_OK;

                case GCS_VERBA:
                    return StringCchCopyA(pszName, cchMax, SEND_VERBA);

                case GCS_VERBW:
                    return StringCchCopyW((STRSAFE_LPWSTR)pszName, cchMax, SEND_VERBW);

                default:
                    return E_INVALIDARG;
                }
        } else
            return E_INVALIDARG;
    } else {
        if (idCmd == 0) {
            switch (uFlags) {
                case GCS_HELPTEXTA:
                    if (LoadStringA(module, IDS_NEW_SUBVOL_HELP_TEXT, pszName, cchMax))
                        return S_OK;
                    else
                        return E_FAIL;

                case GCS_HELPTEXTW:
                    if (LoadStringW(module, IDS_NEW_SUBVOL_HELP_TEXT, (LPWSTR)pszName, cchMax))
                        return S_OK;
                    else
                        return E_FAIL;

                case GCS_VALIDATEA:
                case GCS_VALIDATEW:
                    return S_OK;

                case GCS_VERBA:
                    return StringCchCopyA(pszName, cchMax, NEW_SUBVOL_VERBA);

                case GCS_VERBW:
                    return StringCchCopyW((STRSAFE_LPWSTR)pszName, cchMax, NEW_SUBVOL_VERBW);

                default:
                    return E_INVALIDARG;
            }
        } else if (idCmd == 1) {
            switch (uFlags) {
                case GCS_HELPTEXTA:
                    if (LoadStringA(module, IDS_RECV_SUBVOL_HELP, pszName, cchMax))
                        return S_OK;
                    else
                        return E_FAIL;

                case GCS_HELPTEXTW:
                    if (LoadStringW(module, IDS_RECV_SUBVOL_HELP, (LPWSTR)pszName, cchMax))
                        return S_OK;
                    else
                        return E_FAIL;

                case GCS_VALIDATEA:
                case GCS_VALIDATEW:
                    return S_OK;

                case GCS_VERBA:
                    return StringCchCopyA(pszName, cchMax, RECV_VERBA);

                case GCS_VERBW:
                    return StringCchCopyW((STRSAFE_LPWSTR)pszName, cchMax, RECV_VERBW);

                default:
                    return E_INVALIDARG;
            }
        } else if (idCmd == 2) {
            switch (uFlags) {
                case GCS_HELPTEXTA:
                    if (LoadStringA(module, IDS_REFLINK_PASTE_HELP, pszName, cchMax))
                        return S_OK;
                    else
                        return E_FAIL;

                case GCS_HELPTEXTW:
                    if (LoadStringW(module, IDS_REFLINK_PASTE_HELP, (LPWSTR)pszName, cchMax))
                        return S_OK;
                    else
                        return E_FAIL;

                case GCS_VALIDATEA:
                case GCS_VALIDATEW:
                    return S_OK;

                case GCS_VERBA:
                    return StringCchCopyA(pszName, cchMax, REFLINK_VERBA);

                case GCS_VERBW:
                    return StringCchCopyW((STRSAFE_LPWSTR)pszName, cchMax, REFLINK_VERBW);

                default:
                    return E_INVALIDARG;
            }
        } else
            return E_INVALIDARG;
    }
}

static void reflink_copy2(const wstring& srcfn, const wstring& destdir, const wstring& destname) {
    win_handle source, dest;
    FILE_BASIC_INFO fbi;
    FILETIME atime, mtime;
    btrfs_inode_info bii;
    btrfs_set_inode_info bsii;
    ULONG bytesret;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_set_xattr bsxa;

    source = CreateFileW(srcfn.c_str(), GENERIC_READ | FILE_TRAVERSE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT, nullptr);
    if (source == INVALID_HANDLE_VALUE)
        throw last_error(GetLastError());

    Status = NtFsControlFile(source, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_INODE_INFO, nullptr, 0, &bii, sizeof(btrfs_inode_info));
    if (!NT_SUCCESS(Status))
        throw ntstatus_error(Status);

    // if subvol, do snapshot instead
    if (bii.inode == SUBVOL_ROOT_INODE) {
        btrfs_create_snapshot* bcs;
        win_handle dirh;

        dirh = CreateFileW(destdir.c_str(), FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (dirh == INVALID_HANDLE_VALUE)
            throw last_error(GetLastError());

        size_t bcslen = offsetof(btrfs_create_snapshot, name[0]) + (destname.length() * sizeof(WCHAR));
        bcs = (btrfs_create_snapshot*)malloc(bcslen);
        bcs->subvol = source;
        bcs->namelen = (uint16_t)(destname.length() * sizeof(WCHAR));
        memcpy(bcs->name, destname.c_str(), destname.length() * sizeof(WCHAR));

        Status = NtFsControlFile(dirh, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_CREATE_SNAPSHOT, bcs, (ULONG)bcslen, nullptr, 0);
        if (!NT_SUCCESS(Status)) {
            free(bcs);
            throw ntstatus_error(Status);
        }

        free(bcs);

        return;
    }

    Status = NtQueryInformationFile(source, &iosb, &fbi, sizeof(FILE_BASIC_INFO), FileBasicInformation);
    if (!NT_SUCCESS(Status))
        throw ntstatus_error(Status);

    if (bii.type == BTRFS_TYPE_CHARDEV || bii.type == BTRFS_TYPE_BLOCKDEV || bii.type == BTRFS_TYPE_FIFO || bii.type == BTRFS_TYPE_SOCKET) {
        win_handle dirh;
        btrfs_mknod* bmn;

        dirh = CreateFileW(destdir.c_str(), FILE_ADD_FILE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (dirh == INVALID_HANDLE_VALUE)
            throw last_error(GetLastError());

        size_t bmnsize = offsetof(btrfs_mknod, name[0]) + (destname.length() * sizeof(WCHAR));
        bmn = (btrfs_mknod*)malloc(bmnsize);

        bmn->inode = 0;
        bmn->type = bii.type;
        bmn->st_rdev = bii.st_rdev;
        bmn->namelen = (uint16_t)(destname.length() * sizeof(WCHAR));
        memcpy(bmn->name, destname.c_str(), bmn->namelen);

        Status = NtFsControlFile(dirh, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_MKNOD, bmn, (ULONG)bmnsize, nullptr, 0);
        if (!NT_SUCCESS(Status)) {
            free(bmn);
            throw ntstatus_error(Status);
        }

        free(bmn);

        dest = CreateFileW((destdir + destname).c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    } else if (fbi.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (CreateDirectoryExW(srcfn.c_str(), (destdir + destname).c_str(), nullptr))
            dest = CreateFileW((destdir + destname).c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        else
            dest = INVALID_HANDLE_VALUE;
    } else
        dest = CreateFileW((destdir + destname).c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, nullptr, CREATE_NEW, 0, source);

    if (dest == INVALID_HANDLE_VALUE)
        throw last_error(GetLastError());

    memset(&bsii, 0, sizeof(btrfs_set_inode_info));

    bsii.flags_changed = true;
    bsii.flags = bii.flags;

    if (bii.flags & BTRFS_INODE_COMPRESS) {
        bsii.compression_type_changed = true;
        bsii.compression_type = bii.compression_type;
    }

    try {
        Status = NtFsControlFile(dest, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_SET_INODE_INFO, &bsii, sizeof(btrfs_set_inode_info), nullptr, 0);
        if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);

        if (fbi.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!(fbi.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                WIN32_FIND_DATAW fff;
                wstring qs;

                qs = srcfn;
                qs += L"\\*";

                fff_handle h = FindFirstFileW(qs.c_str(), &fff);
                if (h != INVALID_HANDLE_VALUE) {
                    do {
                        wstring fn2;

                        if (fff.cFileName[0] == '.' && (fff.cFileName[1] == 0 || (fff.cFileName[1] == '.' && fff.cFileName[2] == 0)))
                            continue;

                        fn2 = srcfn;
                        fn2 += L"\\";
                        fn2 += fff.cFileName;

                        reflink_copy2(fn2, destdir + destname + L"\\", fff.cFileName);
                    } while (FindNextFileW(h, &fff));
                }
            }

            // CreateDirectoryExW also copies streams, no need to do it here
        } else {
            if (fbi.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                reparse_header rh;
                uint8_t* rp;

                if (!DeviceIoControl(source, FSCTL_GET_REPARSE_POINT, nullptr, 0, &rh, sizeof(reparse_header), &bytesret, nullptr)) {
                    if (GetLastError() != ERROR_MORE_DATA)
                        throw last_error(GetLastError());
                }

                size_t rplen = sizeof(reparse_header) + rh.ReparseDataLength;
                rp = (uint8_t*)malloc(rplen);

                try {
                    if (!DeviceIoControl(source, FSCTL_GET_REPARSE_POINT, nullptr, 0, rp, (DWORD)rplen, &bytesret, nullptr))
                        throw last_error(GetLastError());

                    if (!DeviceIoControl(dest, FSCTL_SET_REPARSE_POINT, rp, (DWORD)rplen, nullptr, 0, &bytesret, nullptr))
                        throw last_error(GetLastError());
                } catch (...) {
                    free(rp);
                    throw;
                }

                free(rp);
            } else {
                FILE_STANDARD_INFO fsi;
                FILE_END_OF_FILE_INFO feofi;
                FSCTL_GET_INTEGRITY_INFORMATION_BUFFER fgiib;
                FSCTL_SET_INTEGRITY_INFORMATION_BUFFER fsiib;
                DUPLICATE_EXTENTS_DATA ded;
                uint64_t offset, alloc_size;
                ULONG maxdup;

                Status = NtQueryInformationFile(source, &iosb, &fsi, sizeof(FILE_STANDARD_INFO), FileStandardInformation);
                if (!NT_SUCCESS(Status))
                    throw ntstatus_error(Status);

                if (!DeviceIoControl(source, FSCTL_GET_INTEGRITY_INFORMATION, nullptr, 0, &fgiib, sizeof(FSCTL_GET_INTEGRITY_INFORMATION_BUFFER), &bytesret, nullptr))
                    throw last_error(GetLastError());

                if (fbi.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) {
                    if (!DeviceIoControl(dest, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &bytesret, nullptr))
                        throw last_error(GetLastError());
                }

                fsiib.ChecksumAlgorithm = fgiib.ChecksumAlgorithm;
                fsiib.Reserved = 0;
                fsiib.Flags = fgiib.Flags;
                if (!DeviceIoControl(dest, FSCTL_SET_INTEGRITY_INFORMATION, &fsiib, sizeof(FSCTL_SET_INTEGRITY_INFORMATION_BUFFER), nullptr, 0, &bytesret, nullptr))
                    throw last_error(GetLastError());

                feofi.EndOfFile = fsi.EndOfFile;
                Status = NtSetInformationFile(dest, &iosb, &feofi, sizeof(FILE_END_OF_FILE_INFO), FileEndOfFileInformation);
                if (!NT_SUCCESS(Status))
                    throw ntstatus_error(Status);

                ded.FileHandle = source;
                maxdup = 0xffffffff - fgiib.ClusterSizeInBytes + 1;

                alloc_size = sector_align(fsi.EndOfFile.QuadPart, fgiib.ClusterSizeInBytes);

                offset = 0;
                while (offset < alloc_size) {
                    ded.SourceFileOffset.QuadPart = ded.TargetFileOffset.QuadPart = offset;
                    ded.ByteCount.QuadPart = maxdup < (alloc_size - offset) ? maxdup : (alloc_size - offset);
                    if (!DeviceIoControl(dest, FSCTL_DUPLICATE_EXTENTS_TO_FILE, &ded, sizeof(DUPLICATE_EXTENTS_DATA), nullptr, 0, &bytesret, nullptr))
                        throw last_error(GetLastError());

                    offset += ded.ByteCount.QuadPart;
                }
            }

            ULONG streambufsize = 0;
            vector<char> streambuf;

            do {
                streambufsize += 0x1000;
                streambuf.resize(streambufsize);

                memset(streambuf.data(), 0, streambufsize);

                Status = NtQueryInformationFile(source, &iosb, streambuf.data(), streambufsize, FileStreamInformation);
            } while (Status == STATUS_BUFFER_OVERFLOW);

            if (!NT_SUCCESS(Status))
                throw ntstatus_error(Status);

            auto fsi = reinterpret_cast<FILE_STREAM_INFORMATION*>(streambuf.data());

            while (true) {
                if (fsi->StreamNameLength > 0) {
                    wstring sn = wstring(fsi->StreamName, fsi->StreamNameLength / sizeof(WCHAR));

                    if (sn != L"::$DATA" && sn.length() > 6 && sn.substr(sn.length() - 6, 6) == L":$DATA") {
                        win_handle stream;
                        uint8_t* data = nullptr;
                        auto stream_size = (uint16_t)fsi->StreamSize.QuadPart;

                        if (stream_size > 0) {
                            wstring fn2;

                            fn2 = srcfn;
                            fn2 += sn;

                            stream = CreateFileW(fn2.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);

                            if (stream == INVALID_HANDLE_VALUE)
                                throw last_error(GetLastError());

                            // We can get away with this because our streams are guaranteed to be below 64 KB -
                            // don't do this on NTFS!
                            data = (uint8_t*)malloc(stream_size);

                            if (!ReadFile(stream, data, stream_size, &bytesret, nullptr)) {
                                free(data);
                                throw last_error(GetLastError());
                            }
                        }

                        stream = CreateFileW((destdir + destname + sn).c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, nullptr, CREATE_NEW, 0, nullptr);

                        if (stream == INVALID_HANDLE_VALUE) {
                            if (data) free(data);
                            throw last_error(GetLastError());
                        }

                        if (data) {
                            if (!WriteFile(stream, data, stream_size, &bytesret, nullptr)) {
                                free(data);
                                throw last_error(GetLastError());
                            }

                            free(data);
                        }
                    }

                }

                if (fsi->NextEntryOffset == 0)
                    break;

                fsi = reinterpret_cast<FILE_STREAM_INFORMATION*>(reinterpret_cast<char*>(fsi) + fsi->NextEntryOffset);
            }
        }

        atime.dwLowDateTime = fbi.LastAccessTime.LowPart;
        atime.dwHighDateTime = fbi.LastAccessTime.HighPart;
        mtime.dwLowDateTime = fbi.LastWriteTime.LowPart;
        mtime.dwHighDateTime = fbi.LastWriteTime.HighPart;
        SetFileTime(dest, nullptr, &atime, &mtime);

        Status = NtFsControlFile(source, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_XATTRS, nullptr, 0, &bsxa, sizeof(btrfs_set_xattr));

        if (Status == STATUS_BUFFER_OVERFLOW || (NT_SUCCESS(Status) && bsxa.valuelen > 0)) {
            ULONG xalen = 0;
            btrfs_set_xattr *xa = nullptr, *xa2;

            do {
                xalen += 1024;

                if (xa) free(xa);
                xa = (btrfs_set_xattr*)malloc(xalen);

                Status = NtFsControlFile(source, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_XATTRS, nullptr, 0, xa, xalen);
            } while (Status == STATUS_BUFFER_OVERFLOW);

            if (!NT_SUCCESS(Status)) {
                free(xa);
                throw ntstatus_error(Status);
            }

            xa2 = xa;
            while (xa2->valuelen > 0) {
                Status = NtFsControlFile(dest, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_SET_XATTR, xa2,
                                         (ULONG)(offsetof(btrfs_set_xattr, data[0]) + xa2->namelen + xa2->valuelen), nullptr, 0);
                if (!NT_SUCCESS(Status)) {
                    free(xa);
                    throw ntstatus_error(Status);
                }
                xa2 = (btrfs_set_xattr*)&xa2->data[xa2->namelen + xa2->valuelen];
            }

            free(xa);
        } else if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);
    } catch (...) {
        FILE_DISPOSITION_INFO fdi;

        fdi.DeleteFile = true;
        Status = NtSetInformationFile(dest, &iosb, &fdi, sizeof(FILE_DISPOSITION_INFO), FileDispositionInformation);
        if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);

        throw;
    }
}

extern "C" void CALLBACK ReflinkCopyW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    vector<wstring> args;

    command_line_to_args(lpszCmdLine, args);

    if (args.size() >= 2) {
        bool dest_is_dir = false;
        wstring dest = args[args.size() - 1], destdir, destname;
        WCHAR volpath2[MAX_PATH];

        {
            win_handle destdirh = CreateFileW(dest.c_str(), FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                              nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

            if (destdirh != INVALID_HANDLE_VALUE) {
                BY_HANDLE_FILE_INFORMATION bhfi;

                if (GetFileInformationByHandle(destdirh, &bhfi) && bhfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    dest_is_dir = true;

                    destdir = dest;
                    if (destdir.substr(destdir.length() - 1, 1) != L"\\")
                        destdir += L"\\";
                }
            }
        }

        if (!dest_is_dir) {
            size_t found = dest.rfind(L"\\");

            if (found == wstring::npos) {
                destdir = L"";
                destname = dest;
            } else {
                destdir = dest.substr(0, found);
                destname = dest.substr(found + 1);
            }
        }

        if (!GetVolumePathNameW(dest.c_str(), volpath2, sizeof(volpath2) / sizeof(WCHAR)))
            return;

        for (unsigned int i = 0; i < args.size() - 1; i++) {
            WIN32_FIND_DATAW ffd;

            fff_handle h = FindFirstFileW(args[i].c_str(), &ffd);
            if (h != INVALID_HANDLE_VALUE) {
                WCHAR volpath1[MAX_PATH];
                wstring path = args[i];
                size_t found = path.rfind(L"\\");

                if (found == wstring::npos)
                    path = L"";
                else
                    path = path.substr(0, found);

                path += L"\\";

                if (get_volume_path_parent(path.c_str(), volpath1, sizeof(volpath1) / sizeof(WCHAR))) {
                    if (!wcscmp(volpath1, volpath2)) {
                        do {
                            try {
                                reflink_copy2(path + ffd.cFileName, destdir, dest_is_dir ? ffd.cFileName : destname);
                            } catch (const exception& e) {
                                cerr << "Error: " << e.what() << endl;
                            }
                        } while (FindNextFileW(h, &ffd));
                    }
                }
            }
        }
    }
}