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

#ifndef __REACTOS__
#define UNICODE
#endif
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
#include <string>
#include <sstream>

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

    *ppObj = NULL;
    return E_NOINTERFACE;
}

HRESULT __stdcall BtrfsContextMenu::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID) {
    HANDLE h;
    IO_STATUS_BLOCK iosb;
    btrfs_get_file_ids bgfi;
    NTSTATUS Status;

    if (!pidlFolder) {
        FORMATETC format = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        UINT num_files, i;
        WCHAR fn[MAX_PATH];
        HDROP hdrop;

        if (!pdtobj)
            return E_FAIL;

        stgm.tymed = TYMED_HGLOBAL;

        if (FAILED(pdtobj->GetData(&format, &stgm)))
            return E_INVALIDARG;

        stgm_set = TRUE;

        hdrop = (HDROP)GlobalLock(stgm.hGlobal);

        if (!hdrop) {
            ReleaseStgMedium(&stgm);
            stgm_set = FALSE;
            return E_INVALIDARG;
        }

        num_files = DragQueryFileW((HDROP)stgm.hGlobal, 0xFFFFFFFF, NULL, 0);

        for (i = 0; i < num_files; i++) {
            if (DragQueryFileW((HDROP)stgm.hGlobal, i, fn, sizeof(fn) / sizeof(WCHAR))) {
                h = CreateFileW(fn, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

                if (h != INVALID_HANDLE_VALUE) {
                    Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_FILE_IDS, NULL, 0, &bgfi, sizeof(btrfs_get_file_ids));

                    if (NT_SUCCESS(Status) && bgfi.inode == 0x100 && !bgfi.top) {
                        WCHAR parpath[MAX_PATH];
                        HANDLE h2;

                        StringCchCopyW(parpath, sizeof(parpath) / sizeof(WCHAR), fn);

                        PathRemoveFileSpecW(parpath);

                        h2 = CreateFileW(parpath, FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

                        if (h2 != INVALID_HANDLE_VALUE)
                            allow_snapshot = TRUE;

                        CloseHandle(h2);

                        ignore = FALSE;
                        bg = FALSE;

                        CloseHandle(h);
                        GlobalUnlock(hdrop);
                        return S_OK;
                    }

                    CloseHandle(h);
                }
            }
        }

        GlobalUnlock(hdrop);

        return S_OK;
    }

    if (!SHGetPathFromIDListW(pidlFolder, path))
        return E_FAIL;

    // check we have permissions to create new subdirectory

    h = CreateFileW(path, FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (h == INVALID_HANDLE_VALUE)
        return E_FAIL;

    // check is Btrfs volume

    Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_FILE_IDS, NULL, 0, &bgfi, sizeof(btrfs_get_file_ids));

    if (!NT_SUCCESS(Status)) {
        CloseHandle(h);
        return E_FAIL;
    }

    CloseHandle(h);

    ignore = FALSE;
    bg = TRUE;

    return S_OK;
}

static BOOL get_volume_path_parent(const WCHAR* fn, WCHAR* volpath, ULONG volpathlen) {
    WCHAR *f, *p;
    BOOL b;

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

static BOOL show_reflink_paste(WCHAR* path) {
    HDROP hdrop;
    HANDLE lh;
    ULONG num_files;
    WCHAR fn[MAX_PATH], volpath1[255], volpath2[255];

    if (!IsClipboardFormatAvailable(CF_HDROP))
        return FALSE;

    if (!GetVolumePathNameW(path, volpath1, sizeof(volpath1) / sizeof(WCHAR)))
        return FALSE;

    if (!OpenClipboard(NULL))
        return FALSE;

    hdrop = (HDROP)GetClipboardData(CF_HDROP);

    if (!hdrop) {
        CloseClipboard();
        return FALSE;
    }

    lh = GlobalLock(hdrop);

    if (!lh) {
        CloseClipboard();
        return FALSE;
    }

    num_files = DragQueryFileW(hdrop, 0xFFFFFFFF, NULL, 0);

    if (num_files == 0) {
        GlobalUnlock(lh);
        CloseClipboard();
        return FALSE;
    }

    if (!DragQueryFileW(hdrop, 0, fn, sizeof(fn) / sizeof(WCHAR))) {
        GlobalUnlock(lh);
        CloseClipboard();
        return FALSE;
    }

    if (!get_volume_path_parent(fn, volpath2, sizeof(volpath2) / sizeof(WCHAR))) {
        GlobalUnlock(lh);
        CloseClipboard();
        return FALSE;
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

    *phBmp = NULL;

    InitBitmapInfo(&bmi, sizeof(bmi), psize->cx, psize->cy, 32);

    hdcUsed = hdc ? hdc : GetDC(NULL);

    if (hdcUsed) {
        *phBmp = CreateDIBSection(hdcUsed, &bmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
        if (hdc != hdcUsed)
            ReleaseDC(NULL, hdcUsed);
    }

    return !*phBmp ? E_OUTOFMEMORY : S_OK;
}

void BtrfsContextMenu::get_uac_icon() {
    IWICImagingFactory* factory = NULL;
    IWICBitmap* bitmap;
    HRESULT hr;

#ifdef __REACTOS__
    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (void **)&factory);
#else
    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
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

                hr = Create32BitHBITMAP(NULL, &sz, (void**)&buf, &uacicon);
                if (SUCCEEDED(hr)) {
                    UINT stride = cx * sizeof(DWORD);
                    UINT buflen = cy * stride;
                    bitmap->CopyPixels(NULL, stride, buflen, buf);
                }
            }

            bitmap->Release();
        }

        factory->Release();
    }
}

HRESULT __stdcall BtrfsContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) {
    WCHAR str[256];
    ULONG entries = 0;

    if (ignore)
        return E_INVALIDARG;

    if (uFlags & CMF_DEFAULTONLY)
        return S_OK;

    if (!bg) {
        if (allow_snapshot) {
            if (LoadStringW(module, IDS_CREATE_SNAPSHOT, str, sizeof(str) / sizeof(WCHAR)) == 0)
                return E_FAIL;

            if (!InsertMenuW(hmenu, indexMenu, MF_BYPOSITION, idCmdFirst, str))
                return E_FAIL;

            entries = 1;
        }

        if (idCmdFirst + entries <= idCmdLast) {
            MENUITEMINFOW mii;

            if (LoadStringW(module, IDS_SEND_SUBVOL, str, sizeof(str) / sizeof(WCHAR)) == 0)
                return E_FAIL;

            if (!uacicon)
                get_uac_icon();

            memset(&mii, 0, sizeof(MENUITEMINFOW));
            mii.cbSize = sizeof(MENUITEMINFOW);
            mii.fMask = MIIM_STRING | MIIM_ID | MIIM_BITMAP;
            mii.dwTypeData = str;
            mii.wID = idCmdFirst + entries;
            mii.hbmpItem = uacicon;

            if (!InsertMenuItemW(hmenu, indexMenu + entries, TRUE, &mii))
                return E_FAIL;

            entries++;
        }
    } else {
        if (LoadStringW(module, IDS_NEW_SUBVOL, str, sizeof(str) / sizeof(WCHAR)) == 0)
            return E_FAIL;

        if (!InsertMenuW(hmenu, indexMenu, MF_BYPOSITION, idCmdFirst, str))
            return E_FAIL;

        entries = 1;

        if (idCmdFirst + 1 <= idCmdLast) {
            MENUITEMINFOW mii;

            if (LoadStringW(module, IDS_RECV_SUBVOL, str, sizeof(str) / sizeof(WCHAR)) == 0)
                return E_FAIL;

            if (!uacicon)
                get_uac_icon();

            memset(&mii, 0, sizeof(MENUITEMINFOW));
            mii.cbSize = sizeof(MENUITEMINFOW);
            mii.fMask = MIIM_STRING | MIIM_ID | MIIM_BITMAP;
            mii.dwTypeData = str;
            mii.wID = idCmdFirst + 1;
            mii.hbmpItem = uacicon;

            if (!InsertMenuItemW(hmenu, indexMenu + 1, TRUE, &mii))
                return E_FAIL;

            entries++;
        }

        if (idCmdFirst + 2 <= idCmdLast && show_reflink_paste(path)) {
            if (LoadStringW(module, IDS_REFLINK_PASTE, str, sizeof(str) / sizeof(WCHAR)) == 0)
                return E_FAIL;

            if (!InsertMenuW(hmenu, indexMenu + 2, MF_BYPOSITION, idCmdFirst + 2, str))
                return E_FAIL;

            entries++;
        }
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, entries);
}

static void create_snapshot(HWND hwnd, WCHAR* fn) {
    HANDLE h;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_get_file_ids bgfi;

    h = CreateFileW(fn, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (h != INVALID_HANDLE_VALUE) {
        Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_FILE_IDS, NULL, 0, &bgfi, sizeof(btrfs_get_file_ids));

        if (NT_SUCCESS(Status) && bgfi.inode == 0x100 && !bgfi.top) {
            WCHAR parpath[MAX_PATH], subvolname[MAX_PATH], templ[MAX_PATH], name[MAX_PATH], searchpath[MAX_PATH];
            HANDLE h2, fff;
            btrfs_create_snapshot* bcs;
            ULONG namelen, pathend;
            WIN32_FIND_DATAW wfd;
            SYSTEMTIME time;

            StringCchCopyW(parpath, sizeof(parpath) / sizeof(WCHAR), fn);
            PathRemoveFileSpecW(parpath);

            StringCchCopyW(subvolname, sizeof(subvolname) / sizeof(WCHAR), fn);
            PathStripPathW(subvolname);

            h2 = CreateFileW(parpath, FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

            if (h2 == INVALID_HANDLE_VALUE) {
                ShowError(hwnd, GetLastError());
                CloseHandle(h);
                return;
            }

            if (!LoadStringW(module, IDS_SNAPSHOT_FILENAME, templ, MAX_PATH)) {
                ShowError(hwnd, GetLastError());
                CloseHandle(h);
                CloseHandle(h2);
                return;
            }

            GetLocalTime(&time);

            if (StringCchPrintfW(name, sizeof(name) / sizeof(WCHAR), templ, subvolname, time.wYear, time.wMonth, time.wDay) == STRSAFE_E_INSUFFICIENT_BUFFER) {
                MessageBoxW(hwnd, L"Filename too long.\n", L"Error", MB_ICONERROR);
                CloseHandle(h);
                CloseHandle(h2);
                return;
            }

            StringCchCopyW(searchpath, sizeof(searchpath) / sizeof(WCHAR), parpath);
            StringCchCatW(searchpath, sizeof(searchpath) / sizeof(WCHAR), L"\\");
            pathend = wcslen(searchpath);

            StringCchCatW(searchpath, sizeof(searchpath) / sizeof(WCHAR), name);

            fff = FindFirstFileW(searchpath, &wfd);

            if (fff != INVALID_HANDLE_VALUE) {
                ULONG i = wcslen(searchpath), num = 2;

                do {
                    FindClose(fff);

                    searchpath[i] = 0;
                    if (StringCchPrintfW(searchpath, sizeof(searchpath) / sizeof(WCHAR), L"%s (%u)", searchpath, num) == STRSAFE_E_INSUFFICIENT_BUFFER) {
                        MessageBoxW(hwnd, L"Filename too long.\n", L"Error", MB_ICONERROR);
                        CloseHandle(h);
                        CloseHandle(h2);
                        return;
                    }

                    fff = FindFirstFileW(searchpath, &wfd);
                    num++;
                } while (fff != INVALID_HANDLE_VALUE);
            }

            namelen = wcslen(&searchpath[pathend]) * sizeof(WCHAR);

            bcs = (btrfs_create_snapshot*)malloc(sizeof(btrfs_create_snapshot) - 1 + namelen);
            bcs->readonly = FALSE;
            bcs->posix = FALSE;
            bcs->subvol = h;
            bcs->namelen = namelen;
            memcpy(bcs->name, &searchpath[pathend], namelen);

            Status = NtFsControlFile(h2, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_CREATE_SNAPSHOT, bcs, sizeof(btrfs_create_snapshot) - 1 + namelen, NULL, 0);

            if (!NT_SUCCESS(Status))
                ShowNtStatusError(hwnd, Status);

            CloseHandle(h2);
        }

        CloseHandle(h);
    } else
        ShowError(hwnd, GetLastError());
}

static UINT64 __inline sector_align(UINT64 n, UINT64 a) {
    if (n & (a - 1))
        n = (n + a) & ~(a - 1);

    return n;
}

BOOL BtrfsContextMenu::reflink_copy(HWND hwnd, const WCHAR* fn, const WCHAR* dir) {
    HANDLE source, dest;
    WCHAR* name, volpath1[255], volpath2[255];
    std::wstring dirw, newpath;
    BOOL ret = FALSE;
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

    if (!get_volume_path_parent(fn, volpath1, sizeof(volpath1) / sizeof(WCHAR))) {
        ShowError(hwnd, GetLastError());
        return FALSE;
    }

    if (!GetVolumePathNameW(dir, volpath2, sizeof(volpath2) / sizeof(WCHAR))) {
        ShowError(hwnd, GetLastError());
        return FALSE;
    }

    if (wcscmp(volpath1, volpath2)) // different filesystems
        return FALSE;

    source = CreateFileW(fn, GENERIC_READ | FILE_TRAVERSE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT, NULL);
    if (source == INVALID_HANDLE_VALUE) {
        ShowError(hwnd, GetLastError());
        return FALSE;
    }

    Status = NtFsControlFile(source, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_INODE_INFO, NULL, 0, &bii, sizeof(btrfs_inode_info));
    if (!NT_SUCCESS(Status)) {
        ShowNtStatusError(hwnd, Status);
        CloseHandle(source);
        return FALSE;
    }

    // if subvol, do snapshot instead
    if (bii.inode == SUBVOL_ROOT_INODE) {
        btrfs_create_snapshot* bcs;
        HANDLE dirh, fff;
        std::wstring destname, search;
        WIN32_FIND_DATAW wfd;
        int num = 2;

        dirh = CreateFileW(dir, FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (dirh == INVALID_HANDLE_VALUE) {
            ShowError(hwnd, GetLastError());
            CloseHandle(source);
            return FALSE;
        }

        search = dirw;
        search += name;
        destname = name;

        fff = FindFirstFileW(search.c_str(), &wfd);

        if (fff != INVALID_HANDLE_VALUE) {
            do {
                std::wstringstream ss;

                FindClose(fff);

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
        bcs->namelen = destname.length() * sizeof(WCHAR);
        memcpy(bcs->name, destname.c_str(), destname.length() * sizeof(WCHAR));

        Status = NtFsControlFile(dirh, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_CREATE_SNAPSHOT, bcs, sizeof(btrfs_create_snapshot) - sizeof(WCHAR) + bcs->namelen, NULL, 0);

        free(bcs);

        if (!NT_SUCCESS(Status)) {
            ShowNtStatusError(hwnd, Status);
            CloseHandle(source);
            CloseHandle(dirh);
            return FALSE;
        }

        CloseHandle(source);
        CloseHandle(dirh);
        return TRUE;
    }

    if (!GetFileInformationByHandleEx(source, FileBasicInfo, &fbi, sizeof(FILE_BASIC_INFO))) {
        ShowError(hwnd, GetLastError());
        CloseHandle(source);
        return FALSE;
    }

    if (bii.type == BTRFS_TYPE_CHARDEV || bii.type == BTRFS_TYPE_BLOCKDEV || bii.type == BTRFS_TYPE_FIFO || bii.type == BTRFS_TYPE_SOCKET) {
        HANDLE dirh;
        ULONG bmnsize;
        btrfs_mknod* bmn;

        dirh = CreateFileW(dir, FILE_ADD_FILE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (dirh == INVALID_HANDLE_VALUE) {
            ShowError(hwnd, GetLastError());
            CloseHandle(source);
            return FALSE;
        }

        bmnsize = offsetof(btrfs_mknod, name[0]) + (wcslen(name) * sizeof(WCHAR));
        bmn = (btrfs_mknod*)malloc(bmnsize);

        bmn->inode = 0;
        bmn->type = bii.type;
        bmn->st_rdev = bii.st_rdev;
        bmn->namelen = wcslen(name) * sizeof(WCHAR);
        memcpy(bmn->name, name, bmn->namelen);

        Status = NtFsControlFile(dirh, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_MKNOD, bmn, bmnsize, NULL, 0);
        if (!NT_SUCCESS(Status)) {
            ShowNtStatusError(hwnd, Status);
            CloseHandle(dirh);
            CloseHandle(source);
            free(bmn);
            return FALSE;
        }

        CloseHandle(dirh);
        free(bmn);

        dest = CreateFileW(newpath.c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, 0, NULL);
    } else if (fbi.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (CreateDirectoryExW(fn, newpath.c_str(), NULL))
            dest = CreateFileW(newpath.c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        else
            dest = INVALID_HANDLE_VALUE;
    } else
        dest = CreateFileW(newpath.c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_NEW, 0, source);

    if (dest == INVALID_HANDLE_VALUE) {
        int num = 2;

        if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS && wcscmp(fn, newpath.c_str())) {
            ShowError(hwnd, GetLastError());
            CloseHandle(source);
            return FALSE;
        }

        do {
            WCHAR* ext;
            std::wstringstream ss;

            ext = PathFindExtensionW(fn);

            ss << dirw;

            if (*ext == 0) {
                ss << name;
                ss << L" (";
                ss << num;
                ss << L")";
            } else {
                std::wstring namew = name;

                ss << namew.substr(0, ext - name);
                ss << L" (";
                ss << num;
                ss << L")";
                ss << ext;
            }

            newpath = ss.str();
            if (fbi.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (CreateDirectoryExW(fn, newpath.c_str(), NULL))
                    dest = CreateFileW(newpath.c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
                else
                    dest = INVALID_HANDLE_VALUE;
            } else
                dest = CreateFileW(newpath.c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_NEW, 0, source);

            if (dest == INVALID_HANDLE_VALUE) {
                if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS) {
                    ShowError(hwnd, GetLastError());
                    CloseHandle(source);
                    return FALSE;
                }

                num++;
            } else
                break;
        } while (TRUE);
    }

    memset(&bsii, 0, sizeof(btrfs_set_inode_info));

    bsii.flags_changed = TRUE;
    bsii.flags = bii.flags;

    if (bii.flags & BTRFS_INODE_COMPRESS) {
        bsii.compression_type_changed = TRUE;
        bsii.compression_type = bii.compression_type;
    }

    Status = NtFsControlFile(dest, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_SET_INODE_INFO, &bsii, sizeof(btrfs_set_inode_info), NULL, 0);
    if (!NT_SUCCESS(Status)) {
        ShowNtStatusError(hwnd, Status);
        goto end;
    }

    if (fbi.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (!(fbi.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
            HANDLE h;
            WIN32_FIND_DATAW fff;
            std::wstring qs;

            qs = fn;
            qs += L"\\*";

            h = FindFirstFileW(qs.c_str(), &fff);
            if (h != INVALID_HANDLE_VALUE) {
                do {
                    std::wstring fn2;

                    if (fff.cFileName[0] == '.' && (fff.cFileName[1] == 0 || (fff.cFileName[1] == '.' && fff.cFileName[2] == 0)))
                        continue;

                    fn2 = fn;
                    fn2 += L"\\";
                    fn2 += fff.cFileName;

                    if (!reflink_copy(hwnd, fn2.c_str(), newpath.c_str()))
                        goto end;
                } while (FindNextFileW(h, &fff));

                FindClose(h);
            }
        }

        // CreateDirectoryExW also copies streams, no need to do it here
    } else {
        HANDLE h;
        WIN32_FIND_STREAM_DATA fsd;

        if (fbi.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            reparse_header rh;
            ULONG rplen;
            UINT8* rp;

            if (!DeviceIoControl(source, FSCTL_GET_REPARSE_POINT, NULL, 0, &rh, sizeof(reparse_header), &bytesret, NULL)) {
                if (GetLastError() != ERROR_MORE_DATA) {
                    ShowError(hwnd, GetLastError());
                    goto end;
                }
            }

            rplen = sizeof(reparse_header) + rh.ReparseDataLength;
            rp = (UINT8*)malloc(rplen);

            if (!DeviceIoControl(source, FSCTL_GET_REPARSE_POINT, NULL, 0, rp, rplen, &bytesret, NULL)) {
                ShowError(hwnd, GetLastError());
                goto end;
            }

            if (!DeviceIoControl(dest, FSCTL_SET_REPARSE_POINT, rp, rplen, NULL, 0, &bytesret, NULL)) {
                ShowError(hwnd, GetLastError());
                goto end;
            }

            free(rp);
        } else {
            FILE_STANDARD_INFO fsi;
            FILE_END_OF_FILE_INFO feofi;
            FSCTL_GET_INTEGRITY_INFORMATION_BUFFER fgiib;
            FSCTL_SET_INTEGRITY_INFORMATION_BUFFER fsiib;
            DUPLICATE_EXTENTS_DATA ded;
            UINT64 offset, alloc_size;
            ULONG maxdup;

            if (!GetFileInformationByHandleEx(source, FileStandardInfo, &fsi, sizeof(FILE_STANDARD_INFO))) {
                ShowError(hwnd, GetLastError());
                goto end;
            }

            if (!DeviceIoControl(source, FSCTL_GET_INTEGRITY_INFORMATION, NULL, 0, &fgiib, sizeof(FSCTL_GET_INTEGRITY_INFORMATION_BUFFER), &bytesret, NULL)) {
                ShowError(hwnd, GetLastError());
                goto end;
            }

            if (fbi.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) {
                if (!DeviceIoControl(dest, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &bytesret, NULL)) {
                    ShowError(hwnd, GetLastError());
                    goto end;
                }
            }

            fsiib.ChecksumAlgorithm = fgiib.ChecksumAlgorithm;
            fsiib.Reserved = 0;
            fsiib.Flags = fgiib.Flags;
            if (!DeviceIoControl(dest, FSCTL_SET_INTEGRITY_INFORMATION, &fsiib, sizeof(FSCTL_SET_INTEGRITY_INFORMATION_BUFFER), NULL, 0, &bytesret, NULL)) {
                ShowError(hwnd, GetLastError());
                goto end;
            }

            feofi.EndOfFile = fsi.EndOfFile;
            if (!SetFileInformationByHandle(dest, FileEndOfFileInfo, &feofi, sizeof(FILE_END_OF_FILE_INFO))){
                ShowError(hwnd, GetLastError());
                goto end;
            }

            ded.FileHandle = source;
            maxdup = 0xffffffff - fgiib.ClusterSizeInBytes + 1;

            alloc_size = sector_align(fsi.EndOfFile.QuadPart, fgiib.ClusterSizeInBytes);

            offset = 0;
            while (offset < alloc_size) {
                ded.SourceFileOffset.QuadPart = ded.TargetFileOffset.QuadPart = offset;
                ded.ByteCount.QuadPart = maxdup < (alloc_size - offset) ? maxdup : (alloc_size - offset);
                if (!DeviceIoControl(dest, FSCTL_DUPLICATE_EXTENTS_TO_FILE, &ded, sizeof(DUPLICATE_EXTENTS_DATA), NULL, 0, &bytesret, NULL)) {
                    ShowError(hwnd, GetLastError());
                    goto end;
                }

                offset += ded.ByteCount.QuadPart;
            }
        }

        h = FindFirstStreamW(fn, FindStreamInfoStandard, &fsd, 0);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                std::wstring sn;

                sn = fsd.cStreamName;

                if (sn != L"::$DATA" && sn.length() > 6 && sn.substr(sn.length() - 6, 6) == L":$DATA") {
                    HANDLE stream;
                    UINT8* data = NULL;

                    if (fsd.StreamSize.QuadPart > 0) {
                        std::wstring fn2;

                        fn2 = fn;
                        fn2 += sn;

                        stream = CreateFileW(fn2.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

                        if (stream == INVALID_HANDLE_VALUE) {
                            ShowError(hwnd, GetLastError());
                            goto end;
                        }

                        // We can get away with this because our streams are guaranteed to be below 64 KB -
                        // don't do this on NTFS!
                        data = (UINT8*)malloc(fsd.StreamSize.QuadPart);

                        if (!ReadFile(stream, data, fsd.StreamSize.QuadPart, &bytesret, NULL)) {
                            ShowError(hwnd, GetLastError());
                            free(data);
                            CloseHandle(stream);
                            goto end;
                        }

                        CloseHandle(stream);
                    }

                    stream = CreateFileW((newpath + sn).c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_NEW, 0, NULL);

                    if (stream == INVALID_HANDLE_VALUE) {
                        ShowError(hwnd, GetLastError());

                        if (data) free(data);

                        goto end;
                    }

                    if (data) {
                        if (!WriteFile(stream, data, fsd.StreamSize.QuadPart, &bytesret, NULL)) {
                            ShowError(hwnd, GetLastError());
                            free(data);
                            CloseHandle(stream);
                            goto end;
                        }

                        free(data);
                    }

                    CloseHandle(stream);
                }
            } while (FindNextStreamW(h, &fsd));

            FindClose(h);
        }
    }

    atime.dwLowDateTime = fbi.LastAccessTime.LowPart;
    atime.dwHighDateTime = fbi.LastAccessTime.HighPart;
    mtime.dwLowDateTime = fbi.LastWriteTime.LowPart;
    mtime.dwHighDateTime = fbi.LastWriteTime.HighPart;
    SetFileTime(dest, NULL, &atime, &mtime);

    Status = NtFsControlFile(source, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_XATTRS, NULL, 0, &bsxa, sizeof(btrfs_set_xattr));

    if (Status == STATUS_BUFFER_OVERFLOW || (NT_SUCCESS(Status) && bsxa.valuelen > 0)) {
        ULONG xalen = 0;
        btrfs_set_xattr *xa = NULL, *xa2;

        do {
            xalen += 1024;

            if (xa) free(xa);
            xa = (btrfs_set_xattr*)malloc(xalen);

            Status = NtFsControlFile(source, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_XATTRS, NULL, 0, xa, xalen);
        } while (Status == STATUS_BUFFER_OVERFLOW);

        if (!NT_SUCCESS(Status)) {
            free(xa);
            ShowNtStatusError(hwnd, Status);
            goto end;
        }

        xa2 = xa;
        while (xa2->valuelen > 0) {
            Status = NtFsControlFile(dest, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_SET_XATTR, xa2,
                                     offsetof(btrfs_set_xattr, data[0]) + xa2->namelen + xa2->valuelen, NULL, 0);
            if (!NT_SUCCESS(Status)) {
                free(xa);
                ShowNtStatusError(hwnd, Status);
                goto end;
            }
            xa2 = (btrfs_set_xattr*)&xa2->data[xa2->namelen + xa2->valuelen];
        }

        free(xa);
    } else if (!NT_SUCCESS(Status)) {
        ShowNtStatusError(hwnd, Status);
        goto end;
    }

    ret = TRUE;

end:
    if (!ret) {
        FILE_DISPOSITION_INFO fdi;

        fdi.DeleteFile = TRUE;
        if (!SetFileInformationByHandle(dest, FileDispositionInfo, &fdi, sizeof(FILE_DISPOSITION_INFO)))
            ShowError(hwnd, GetLastError());
    }

    CloseHandle(dest);
    CloseHandle(source);

    return ret;
}

HRESULT __stdcall BtrfsContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO picia) {
    LPCMINVOKECOMMANDINFOEX pici = (LPCMINVOKECOMMANDINFOEX)picia;

    if (ignore)
        return E_INVALIDARG;

    if (!bg) {
        if ((IS_INTRESOURCE(pici->lpVerb) && allow_snapshot && pici->lpVerb == 0) || (!IS_INTRESOURCE(pici->lpVerb) && !strcmp(pici->lpVerb, SNAPSHOT_VERBA))) {
            UINT num_files, i;
            WCHAR fn[MAX_PATH];

            if (!stgm_set)
                return E_FAIL;

            num_files = DragQueryFileW((HDROP)stgm.hGlobal, 0xFFFFFFFF, NULL, 0);

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
            std::wstring t;
            SHELLEXECUTEINFOW sei;

            GetModuleFileNameW(module, dll, sizeof(dll) / sizeof(WCHAR));

            if (!stgm_set)
                return E_FAIL;

            num_files = DragQueryFileW((HDROP)stgm.hGlobal, 0xFFFFFFFF, NULL, 0);

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

                    if (!ShellExecuteExW(&sei)) {
                        ShowError(pici->hwnd, GetLastError());
                        return E_FAIL;
                    }

                    WaitForSingleObject(sei.hProcess, INFINITE);
                    CloseHandle(sei.hProcess);
                }
            }

            return S_OK;
        }
    } else {
        if ((IS_INTRESOURCE(pici->lpVerb) && (ULONG_PTR)pici->lpVerb == 0) || (!IS_INTRESOURCE(pici->lpVerb) && !strcmp(pici->lpVerb, NEW_SUBVOL_VERBA))) {
            HANDLE h;
            IO_STATUS_BLOCK iosb;
            NTSTATUS Status;
            ULONG pathlen, searchpathlen, pathend, bcslen;
            WCHAR name[MAX_PATH], *searchpath;
            btrfs_create_subvol* bcs;
            HANDLE fff;
            WIN32_FIND_DATAW wfd;

            if (!LoadStringW(module, IDS_NEW_SUBVOL_FILENAME, name, MAX_PATH)) {
                ShowError(pici->hwnd, GetLastError());
                return E_FAIL;
            }

            h = CreateFileW(path, FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

            if (h == INVALID_HANDLE_VALUE) {
                ShowError(pici->hwnd, GetLastError());
                return E_FAIL;
            }

            pathlen = wcslen(path);

            searchpathlen = pathlen + wcslen(name) + 10;
            searchpath = (WCHAR*)malloc(searchpathlen * sizeof(WCHAR));

            StringCchCopyW(searchpath, searchpathlen, path);
            StringCchCatW(searchpath, searchpathlen, L"\\");
            pathend = wcslen(searchpath);

            StringCchCatW(searchpath, searchpathlen, name);

            fff = FindFirstFileW(searchpath, &wfd);

            if (fff != INVALID_HANDLE_VALUE) {
                ULONG i = wcslen(searchpath), num = 2;

                do {
                    FindClose(fff);

                    searchpath[i] = 0;
                    if (StringCchPrintfW(searchpath, searchpathlen, L"%s (%u)", searchpath, num) == STRSAFE_E_INSUFFICIENT_BUFFER) {
                        MessageBoxW(pici->hwnd, L"Filename too long.\n", L"Error", MB_ICONERROR);
                        CloseHandle(h);
                        return E_FAIL;
                    }

                    fff = FindFirstFileW(searchpath, &wfd);
                    num++;
                } while (fff != INVALID_HANDLE_VALUE);
            }

            bcslen = offsetof(btrfs_create_subvol, name[0]) + (wcslen(&searchpath[pathend]) * sizeof(WCHAR));
            bcs = (btrfs_create_subvol*)malloc(bcslen);

            bcs->readonly = FALSE;
            bcs->posix = FALSE;
            bcs->namelen = wcslen(&searchpath[pathend]) * sizeof(WCHAR);
            memcpy(bcs->name, &searchpath[pathend], bcs->namelen);

            Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_CREATE_SUBVOL, bcs, bcslen, NULL, 0);

            free(searchpath);
            free(bcs);

            if (!NT_SUCCESS(Status)) {
                CloseHandle(h);
                ShowNtStatusError(pici->hwnd, Status);
                return E_FAIL;
            }

            CloseHandle(h);

            return S_OK;
        } else if ((IS_INTRESOURCE(pici->lpVerb) && (ULONG_PTR)pici->lpVerb == 1) || (!IS_INTRESOURCE(pici->lpVerb) && !strcmp(pici->lpVerb, RECV_VERBA))) {
            WCHAR dll[MAX_PATH];
            std::wstring t;
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

            if (!ShellExecuteExW(&sei)) {
                ShowError(pici->hwnd, GetLastError());
                return E_FAIL;
            }

            WaitForSingleObject(sei.hProcess, INFINITE);
            CloseHandle(sei.hProcess);

            return S_OK;
        } else if ((IS_INTRESOURCE(pici->lpVerb) && (ULONG_PTR)pici->lpVerb == 2) || (!IS_INTRESOURCE(pici->lpVerb) && !strcmp(pici->lpVerb, REFLINK_VERBA))) {
            HDROP hdrop;

            if (!IsClipboardFormatAvailable(CF_HDROP))
                return S_OK;

            if (!OpenClipboard(pici->hwnd)) {
                ShowError(pici->hwnd, GetLastError());
                return E_FAIL;
            }

            hdrop = (HDROP)GetClipboardData(CF_HDROP);

            if (hdrop) {
                HANDLE lh;

                lh = GlobalLock(hdrop);

                if (lh) {
                    ULONG num_files, i;
                    WCHAR fn[MAX_PATH];

                    num_files = DragQueryFileW(hdrop, 0xFFFFFFFF, NULL, 0);

                    for (i = 0; i < num_files; i++) {
                        if (DragQueryFileW(hdrop, i, fn, sizeof(fn) / sizeof(WCHAR))) {
                            if (!reflink_copy(pici->hwnd, fn, pici->lpDirectoryW)) {
                                GlobalUnlock(lh);
                                CloseClipboard();
                                return E_FAIL;
                            }
                        }
                    }

                    GlobalUnlock(lh);
                }
            }

            CloseClipboard();

            return S_OK;
        }
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

static void reflink_copy2(std::wstring srcfn, std::wstring destdir, std::wstring destname) {
    HANDLE source, dest;
    BOOL ret = FALSE;
    FILE_BASIC_INFO fbi;
    FILETIME atime, mtime;
    btrfs_inode_info bii;
    btrfs_set_inode_info bsii;
    ULONG bytesret;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_set_xattr bsxa;

    source = CreateFileW(srcfn.c_str(), GENERIC_READ | FILE_TRAVERSE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_OPEN_REPARSE_POINT, NULL);
    if (source == INVALID_HANDLE_VALUE)
        return;

    Status = NtFsControlFile(source, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_INODE_INFO, NULL, 0, &bii, sizeof(btrfs_inode_info));
    if (!NT_SUCCESS(Status)) {
        CloseHandle(source);
        return;
    }

    // if subvol, do snapshot instead
    if (bii.inode == SUBVOL_ROOT_INODE) {
        ULONG bcslen;
        btrfs_create_snapshot* bcs;
        HANDLE dirh;

        dirh = CreateFileW(destdir.c_str(), FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (dirh == INVALID_HANDLE_VALUE) {
            CloseHandle(source);
            return;
        }

        bcslen = offsetof(btrfs_create_snapshot, name[0]) + (destname.length() * sizeof(WCHAR));
        bcs = (btrfs_create_snapshot*)malloc(bcslen);
        bcs->subvol = source;
        bcs->namelen = destname.length() * sizeof(WCHAR);
        memcpy(bcs->name, destname.c_str(), destname.length() * sizeof(WCHAR));

        Status = NtFsControlFile(dirh, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_CREATE_SNAPSHOT, bcs, bcslen, NULL, 0);

        free(bcs);

        CloseHandle(source);
        CloseHandle(dirh);

        return;
    }

    if (!GetFileInformationByHandleEx(source, FileBasicInfo, &fbi, sizeof(FILE_BASIC_INFO))) {
        CloseHandle(source);
        return;
    }

    if (bii.type == BTRFS_TYPE_CHARDEV || bii.type == BTRFS_TYPE_BLOCKDEV || bii.type == BTRFS_TYPE_FIFO || bii.type == BTRFS_TYPE_SOCKET) {
        HANDLE dirh;
        ULONG bmnsize;
        btrfs_mknod* bmn;

        dirh = CreateFileW(destdir.c_str(), FILE_ADD_FILE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (dirh == INVALID_HANDLE_VALUE) {
            CloseHandle(source);
            return;
        }

        bmnsize = offsetof(btrfs_mknod, name[0]) + (destname.length() * sizeof(WCHAR));
        bmn = (btrfs_mknod*)malloc(bmnsize);

        bmn->inode = 0;
        bmn->type = bii.type;
        bmn->st_rdev = bii.st_rdev;
        bmn->namelen = destname.length() * sizeof(WCHAR);
        memcpy(bmn->name, destname.c_str(), bmn->namelen);

        Status = NtFsControlFile(dirh, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_MKNOD, bmn, bmnsize, NULL, 0);
        if (!NT_SUCCESS(Status)) {
            CloseHandle(dirh);
            CloseHandle(source);
            free(bmn);
            return;
        }

        CloseHandle(dirh);
        free(bmn);

        dest = CreateFileW((destdir + destname).c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, 0, NULL);
    } else if (fbi.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (CreateDirectoryExW(srcfn.c_str(), (destdir + destname).c_str(), NULL))
            dest = CreateFileW((destdir + destname).c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
            else
                dest = INVALID_HANDLE_VALUE;
    } else
        dest = CreateFileW((destdir + destname).c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_NEW, 0, source);

    if (dest == INVALID_HANDLE_VALUE) {
        CloseHandle(source);
        return;
    }

    memset(&bsii, 0, sizeof(btrfs_set_inode_info));

    bsii.flags_changed = TRUE;
    bsii.flags = bii.flags;

    if (bii.flags & BTRFS_INODE_COMPRESS) {
        bsii.compression_type_changed = TRUE;
        bsii.compression_type = bii.compression_type;
    }

    Status = NtFsControlFile(dest, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_SET_INODE_INFO, &bsii, sizeof(btrfs_set_inode_info), NULL, 0);
    if (!NT_SUCCESS(Status))
        goto end;

    if (fbi.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (!(fbi.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
            HANDLE h;
            WIN32_FIND_DATAW fff;
            std::wstring qs;

            qs = srcfn;
            qs += L"\\*";

            h = FindFirstFileW(qs.c_str(), &fff);
            if (h != INVALID_HANDLE_VALUE) {
                do {
                    std::wstring fn2;

                    if (fff.cFileName[0] == '.' && (fff.cFileName[1] == 0 || (fff.cFileName[1] == '.' && fff.cFileName[2] == 0)))
                        continue;

                    fn2 = srcfn;
                    fn2 += L"\\";
                    fn2 += fff.cFileName;

                    reflink_copy2(fn2, destdir + destname + L"\\", fff.cFileName);
                } while (FindNextFileW(h, &fff));

                FindClose(h);
            }
        }

        // CreateDirectoryExW also copies streams, no need to do it here
    } else {
        HANDLE h;
        WIN32_FIND_STREAM_DATA fsd;

        if (fbi.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            reparse_header rh;
            ULONG rplen;
            UINT8* rp;

            if (!DeviceIoControl(source, FSCTL_GET_REPARSE_POINT, NULL, 0, &rh, sizeof(reparse_header), &bytesret, NULL)) {
                if (GetLastError() != ERROR_MORE_DATA)
                    goto end;
            }

            rplen = sizeof(reparse_header) + rh.ReparseDataLength;
            rp = (UINT8*)malloc(rplen);

            if (!DeviceIoControl(source, FSCTL_GET_REPARSE_POINT, NULL, 0, rp, rplen, &bytesret, NULL))
                goto end;

            if (!DeviceIoControl(dest, FSCTL_SET_REPARSE_POINT, rp, rplen, NULL, 0, &bytesret, NULL))
                goto end;

            free(rp);
        } else {
            FILE_STANDARD_INFO fsi;
            FILE_END_OF_FILE_INFO feofi;
            FSCTL_GET_INTEGRITY_INFORMATION_BUFFER fgiib;
            FSCTL_SET_INTEGRITY_INFORMATION_BUFFER fsiib;
            DUPLICATE_EXTENTS_DATA ded;
            UINT64 offset, alloc_size;
            ULONG maxdup;

            if (!GetFileInformationByHandleEx(source, FileStandardInfo, &fsi, sizeof(FILE_STANDARD_INFO)))
                goto end;

            if (!DeviceIoControl(source, FSCTL_GET_INTEGRITY_INFORMATION, NULL, 0, &fgiib, sizeof(FSCTL_GET_INTEGRITY_INFORMATION_BUFFER), &bytesret, NULL))
                goto end;

            if (fbi.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) {
                if (!DeviceIoControl(dest, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &bytesret, NULL))
                    goto end;
            }

            fsiib.ChecksumAlgorithm = fgiib.ChecksumAlgorithm;
            fsiib.Reserved = 0;
            fsiib.Flags = fgiib.Flags;
            if (!DeviceIoControl(dest, FSCTL_SET_INTEGRITY_INFORMATION, &fsiib, sizeof(FSCTL_SET_INTEGRITY_INFORMATION_BUFFER), NULL, 0, &bytesret, NULL))
                goto end;

            feofi.EndOfFile = fsi.EndOfFile;
            if (!SetFileInformationByHandle(dest, FileEndOfFileInfo, &feofi, sizeof(FILE_END_OF_FILE_INFO)))
                goto end;

            ded.FileHandle = source;
            maxdup = 0xffffffff - fgiib.ClusterSizeInBytes + 1;

            alloc_size = sector_align(fsi.EndOfFile.QuadPart, fgiib.ClusterSizeInBytes);

            offset = 0;
            while (offset < alloc_size) {
                ded.SourceFileOffset.QuadPart = ded.TargetFileOffset.QuadPart = offset;
                ded.ByteCount.QuadPart = maxdup < (alloc_size - offset) ? maxdup : (alloc_size - offset);
                if (!DeviceIoControl(dest, FSCTL_DUPLICATE_EXTENTS_TO_FILE, &ded, sizeof(DUPLICATE_EXTENTS_DATA), NULL, 0, &bytesret, NULL))
                    goto end;

                offset += ded.ByteCount.QuadPart;
            }
        }

        h = FindFirstStreamW(srcfn.c_str(), FindStreamInfoStandard, &fsd, 0);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                std::wstring sn;

                sn = fsd.cStreamName;

                if (sn != L"::$DATA" && sn.length() > 6 && sn.substr(sn.length() - 6, 6) == L":$DATA") {
                    HANDLE stream;
                    UINT8* data = NULL;

                    if (fsd.StreamSize.QuadPart > 0) {
                        std::wstring fn2;

                        fn2 = srcfn;
                        fn2 += sn;

                        stream = CreateFileW(fn2.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

                        if (stream == INVALID_HANDLE_VALUE)
                            goto end;

                        // We can get away with this because our streams are guaranteed to be below 64 KB -
                        // don't do this on NTFS!
                        data = (UINT8*)malloc(fsd.StreamSize.QuadPart);

                        if (!ReadFile(stream, data, fsd.StreamSize.QuadPart, &bytesret, NULL)) {
                            free(data);
                            CloseHandle(stream);
                            goto end;
                        }

                        CloseHandle(stream);
                    }

                    stream = CreateFileW((destdir + destname + sn).c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_NEW, 0, NULL);

                    if (stream == INVALID_HANDLE_VALUE) {
                        if (data) free(data);
                        goto end;
                    }

                    if (data) {
                        if (!WriteFile(stream, data, fsd.StreamSize.QuadPart, &bytesret, NULL)) {
                            free(data);
                            CloseHandle(stream);
                            goto end;
                        }

                        free(data);
                    }

                    CloseHandle(stream);
                }
            } while (FindNextStreamW(h, &fsd));

            FindClose(h);
        }
    }

    atime.dwLowDateTime = fbi.LastAccessTime.LowPart;
    atime.dwHighDateTime = fbi.LastAccessTime.HighPart;
    mtime.dwLowDateTime = fbi.LastWriteTime.LowPart;
    mtime.dwHighDateTime = fbi.LastWriteTime.HighPart;
    SetFileTime(dest, NULL, &atime, &mtime);

    Status = NtFsControlFile(source, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_XATTRS, NULL, 0, &bsxa, sizeof(btrfs_set_xattr));

    if (Status == STATUS_BUFFER_OVERFLOW || (NT_SUCCESS(Status) && bsxa.valuelen > 0)) {
        ULONG xalen = 0;
        btrfs_set_xattr *xa = NULL, *xa2;

        do {
            xalen += 1024;

            if (xa) free(xa);
            xa = (btrfs_set_xattr*)malloc(xalen);

            Status = NtFsControlFile(source, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_XATTRS, NULL, 0, xa, xalen);
        } while (Status == STATUS_BUFFER_OVERFLOW);

        if (!NT_SUCCESS(Status)) {
            free(xa);
            goto end;
        }

        xa2 = xa;
        while (xa2->valuelen > 0) {
            Status = NtFsControlFile(dest, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_SET_XATTR, xa2,
                                     offsetof(btrfs_set_xattr, data[0]) + xa2->namelen + xa2->valuelen, NULL, 0);
            if (!NT_SUCCESS(Status)) {
                free(xa);
                goto end;
            }
            xa2 = (btrfs_set_xattr*)&xa2->data[xa2->namelen + xa2->valuelen];
        }

        free(xa);
    } else if (!NT_SUCCESS(Status))
        goto end;

    ret = TRUE;

end:
    if (!ret) {
        FILE_DISPOSITION_INFO fdi;

        fdi.DeleteFile = TRUE;
        SetFileInformationByHandle(dest, FileDispositionInfo, &fdi, sizeof(FILE_DISPOSITION_INFO));
    }

    CloseHandle(dest);
    CloseHandle(source);
}

void CALLBACK ReflinkCopyW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    LPWSTR* args;
    int num_args;

    args = CommandLineToArgvW(lpszCmdLine, &num_args);

    if (!args)
        return;

    if (num_args >= 2) {
        HANDLE destdirh;
        BOOL dest_is_dir = FALSE;
        std::wstring dest = args[num_args - 1], destdir, destname;
        WCHAR volpath2[MAX_PATH];
        int i;

        destdirh = CreateFileW(dest.c_str(), FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (destdirh != INVALID_HANDLE_VALUE) {
            BY_HANDLE_FILE_INFORMATION bhfi;

            if (GetFileInformationByHandle(destdirh, &bhfi) && bhfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                dest_is_dir = TRUE;

                destdir = dest;
                if (destdir.substr(destdir.length() - 1, 1) != L"\\")
                    destdir += L"\\";
            }
            CloseHandle(destdirh);
        }

        if (!dest_is_dir) {
            size_t found = dest.rfind(L"\\");

            if (found == std::wstring::npos) {
                destdir = L"";
                destname = dest;
            } else {
                destdir = dest.substr(0, found);
                destname = dest.substr(found + 1);
            }
        }

        if (!GetVolumePathNameW(dest.c_str(), volpath2, sizeof(volpath2) / sizeof(WCHAR)))
            goto end;

        for (i = 0; i < num_args - 1; i++) {
            WIN32_FIND_DATAW ffd;
            HANDLE h;

            h = FindFirstFileW(args[i], &ffd);
            if (h != INVALID_HANDLE_VALUE) {
                WCHAR volpath1[MAX_PATH];
                std::wstring path = args[i];
                size_t found = path.rfind(L"\\");

                if (found == std::wstring::npos)
                    path = L"";
                else
                    path = path.substr(0, found);

                path += L"\\";

                if (get_volume_path_parent(path.c_str(), volpath1, sizeof(volpath1) / sizeof(WCHAR))) {
                    if (!wcscmp(volpath1, volpath2)) {
                        do {
                            reflink_copy2(path + ffd.cFileName, destdir, dest_is_dir ? ffd.cFileName : destname);
                        } while (FindNextFileW(h, &ffd));
                    }
                }

                FindClose(h);
            }
        }
    }

end:
    LocalFree(args);
}
