/*
 * PROJECT:     ReactOS shell32
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Copy as Path Menu implementation
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static DWORD
SetClipboard(UINT cf, const void* data, SIZE_T size)
{
    BOOL succ = FALSE;
    HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, size);
    if (handle)
    {
        LPVOID clipdata = GlobalLock(handle);
        if (clipdata)
        {
            CopyMemory(clipdata, data, size);
            GlobalUnlock(handle);
            if (OpenClipboard(NULL))
            {
                EmptyClipboard();
                succ = SetClipboardData(cf, handle) != NULL;
                CloseClipboard();
            }
        }
        if (!succ)
        {
            GlobalFree(handle);
        }
    }
    return succ ? ERROR_SUCCESS : GetLastError();
}

static DWORD
SetClipboardFromString(LPCWSTR str)
{
    SIZE_T cch = lstrlenW(str) + 1, size = cch * sizeof(WCHAR);
    if (size > cch)
        return SetClipboard(CF_UNICODETEXT, str, size);
    else
        return ERROR_BUFFER_OVERFLOW;
}

static void
AppendToPathList(CStringW &paths, LPCWSTR path, DWORD index)
{
    if (index)
        paths += L"\r\n";
    LPCWSTR quote = StrChrW(path, L' ');
    if (quote)
        paths += L'\"';
    paths += path;
    if (quote)
        paths += L'\"';
}

STDMETHODIMP
CCopyAsPathMenu::Drop(IDataObject *pdto, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    CStringW paths;
    DWORD i, count;
#if 0
    CComPtr<IShellItemArray> array;
    HRESULT hr = SHCreateShellItemArrayFromDataObject(pdto, IID_PPV_ARG(IShellItemArray, &array));
    if (SUCCEEDED(hr))
    {
        for (i = 0, array->GetCount(&count); i < count && SUCCEEDED(hr); ++i)
        {
            CComPtr<IShellItem> item;
            hr = array->GetItemAt(i, &item);
            if (SUCCEEDED(hr))
            {
                CComHeapPtr<WCHAR> path;
                hr = item->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &path);
                if (SUCCEEDED(hr))
                {
                    AppendToPathList(paths, path, i);
                }
            }
        }
    }
#else
    FIXME("Implement and use SHCreateShellItemArrayFromDataObject\n");
    CDataObjectHIDA pCIDA(pdto);
    HRESULT hr = pCIDA.hr();
    if (SUCCEEDED(hr))
    {
        for (i = 0, count = pCIDA->cidl; i < count && SUCCEEDED(hr); ++i)
        {
            PCUIDLIST_ABSOLUTE folder = HIDA_GetPIDLFolder(pCIDA);
            PCUIDLIST_RELATIVE item = HIDA_GetPIDLItem(pCIDA, i);
            CComHeapPtr<ITEMIDLIST> full;
            hr = SHILCombine(folder, item, &full);
            if (SUCCEEDED(hr))
            {
                PCUITEMID_CHILD child;
                CComPtr<IShellFolder> sf;
                hr = SHBindToParent(full, IID_PPV_ARG(IShellFolder, &sf), &child);
                if (SUCCEEDED(hr))
                {
                    STRRET strret;
                    hr = sf->GetDisplayNameOf(child, SHGDN_FORPARSING, &strret);
                    if (SUCCEEDED(hr))
                    {
                        CComHeapPtr<WCHAR> path;
                        hr = StrRetToStrW(&strret, child, &path);
                        if (SUCCEEDED(hr))
                        {
                            AppendToPathList(paths, path, i);
                        }
                    }
                }
            }
        }
    }
    else
    {
        FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM stgm;
        hr = pdto->GetData(&fmte, &stgm);
        if (SUCCEEDED(hr))
        {
            for (i = 0, count = DragQueryFileW((HDROP)stgm.hGlobal, -1, NULL, 0); i < count && SUCCEEDED(hr); ++i)
            {
                WCHAR path[MAX_PATH];
                if (DragQueryFileW((HDROP)stgm.hGlobal, i, path, _countof(path)))
                {
                    AppendToPathList(paths, path, i);
                }
            }
            ReleaseStgMedium(&stgm);
        }
    }
#endif

    if (SUCCEEDED(hr))
    {
        DWORD err = SetClipboardFromString(paths);
        hr = HRESULT_FROM_WIN32(err);
    }

    if (SUCCEEDED(hr))
        *pdwEffect &= DROPEFFECT_COPY;
    else
        *pdwEffect &= DROPEFFECT_NONE;
    return hr;
}
