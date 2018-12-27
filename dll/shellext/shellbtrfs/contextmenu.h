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

#pragma once

#include <shlobj.h>

extern LONG objs_loaded;

class BtrfsContextMenu : public IShellExtInit, IContextMenu {
public:
    BtrfsContextMenu() {
        refcount = 0;
        ignore = true;
        stgm_set = false;
        uacicon = nullptr;
        allow_snapshot = false;
        InterlockedIncrement(&objs_loaded);
    }

    virtual ~BtrfsContextMenu() {
        if (stgm_set) {
            GlobalUnlock(stgm.hGlobal);
            ReleaseStgMedium(&stgm);
        }

        if (uacicon)
            DeleteObject(uacicon);

        InterlockedDecrement(&objs_loaded);
    }

    // IUnknown

    HRESULT __stdcall QueryInterface(REFIID riid, void **ppObj);

    ULONG __stdcall AddRef() {
        return InterlockedIncrement(&refcount);
    }

    ULONG __stdcall Release() {
        LONG rc = InterlockedDecrement(&refcount);

        if (rc == 0)
            delete this;

        return rc;
    }

    // IShellExtInit

    virtual HRESULT __stdcall Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID);

    // IContextMenu

    virtual HRESULT __stdcall QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    virtual HRESULT __stdcall InvokeCommand(LPCMINVOKECOMMANDINFO pici);
    virtual HRESULT __stdcall GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT* pwReserved, LPSTR pszName, UINT cchMax);

private:
    LONG refcount;
    bool ignore, allow_snapshot;
    bool bg;
    wstring path;
    STGMEDIUM stgm;
    bool stgm_set;
    HBITMAP uacicon;

    void reflink_copy(HWND hwnd, const WCHAR* fn, const WCHAR* dir);
    void get_uac_icon();
};
