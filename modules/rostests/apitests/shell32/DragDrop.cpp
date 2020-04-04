/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for Drag & Drop
 * PROGRAMMER:      Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "shelltest.h"
#include <shlwapi.h>

#define NDEBUG
#include <debug.h>
#include <stdio.h>

#define TESTFILENAME L"DragDropTest.txt"
#define DROPPED_ON_FILE L"DragDroppedOn.lnk"

static CComPtr<IShellFolder> s_pDesktop;

static WCHAR s_szSrcTestFile[MAX_PATH];
static WCHAR s_szDestFolder[MAX_PATH];
static WCHAR s_szDestTestFile[MAX_PATH];
static WCHAR s_szDestLinkSpec[MAX_PATH];
static WCHAR s_szDroppedToItem[MAX_PATH];

enum OP
{
    OP_NONE,
    OP_COPY,
    OP_MOVE,
    OP_LINK,
    OP_NONE_OR_COPY,
    OP_NONE_OR_MOVE,
    OP_NONE_OR_LINK
};

#define D_NONE DROPEFFECT_NONE
#define D_COPY DROPEFFECT_COPY
#define D_MOVE DROPEFFECT_MOVE
#define D_LINK DROPEFFECT_LINK
#define D_NONE_OR_COPY 0xAABBCCDD
#define D_NONE_OR_MOVE 0x11223344
#define D_NONE_OR_LINK 0x55667788

struct TEST_ENTRY
{
    int line;
    OP op;
    HRESULT hr1;
    HRESULT hr2;
    DWORD dwKeyState;
    DWORD dwEffects1;
    DWORD dwEffects2;
    DWORD dwEffects3;
};

static const TEST_ENTRY s_TestEntries[] =
{
    // MK_LBUTTON
    { __LINE__, OP_NONE, S_OK, S_OK, MK_LBUTTON, D_NONE, D_NONE, D_NONE },
    { __LINE__, OP_COPY, S_OK, S_OK, MK_LBUTTON, D_COPY, D_COPY, D_COPY },
    { __LINE__, OP_MOVE, S_OK, S_OK, MK_LBUTTON, D_COPY | D_MOVE, D_MOVE, D_NONE },
    { __LINE__, OP_MOVE, S_OK, S_OK, MK_LBUTTON, D_COPY | D_MOVE | D_LINK, D_MOVE, D_NONE },
    { __LINE__, OP_COPY, S_OK, S_OK, MK_LBUTTON, D_COPY | D_LINK, D_COPY, D_COPY },
    { __LINE__, OP_MOVE, S_OK, S_OK, MK_LBUTTON, D_MOVE, D_MOVE, D_NONE },
    { __LINE__, OP_MOVE, S_OK, S_OK, MK_LBUTTON, D_MOVE | D_LINK, D_MOVE, D_NONE },
    { __LINE__, OP_LINK, S_OK, S_OK, MK_LBUTTON, D_LINK, D_LINK, D_LINK },

    // MK_LBUTTON | MK_SHIFT
    { __LINE__, OP_NONE, S_OK, S_OK, MK_LBUTTON | MK_SHIFT, D_NONE, D_NONE, D_NONE },
    { __LINE__, OP_NONE_OR_COPY, S_OK, S_OK, MK_LBUTTON | MK_SHIFT, D_COPY, D_NONE_OR_COPY, D_NONE_OR_COPY },
    { __LINE__, OP_MOVE, S_OK, S_OK, MK_LBUTTON | MK_SHIFT, D_COPY | D_MOVE, D_MOVE, D_NONE },
    { __LINE__, OP_MOVE, S_OK, S_OK, MK_LBUTTON | MK_SHIFT, D_COPY | D_MOVE | D_LINK, D_MOVE, D_NONE },
    { __LINE__, OP_NONE_OR_COPY, S_OK, S_OK, MK_LBUTTON | MK_SHIFT, D_COPY | D_LINK, D_NONE_OR_COPY, D_NONE_OR_COPY },
    { __LINE__, OP_MOVE, S_OK, S_OK, MK_LBUTTON | MK_SHIFT, D_MOVE, D_MOVE, D_NONE },
    { __LINE__, OP_MOVE, S_OK, S_OK, MK_LBUTTON | MK_SHIFT, D_MOVE | D_LINK, D_MOVE, D_NONE },
    { __LINE__, OP_NONE_OR_LINK, S_OK, S_OK, MK_LBUTTON | MK_SHIFT, D_LINK, D_NONE_OR_LINK, D_NONE_OR_LINK },

    // MK_LBUTTON | MK_SHIFT | MK_CONTROL
#define MK_LBUTTON_SHIFT_CTRL (MK_LBUTTON | MK_SHIFT | MK_CONTROL)
    { __LINE__, OP_NONE, S_OK, S_OK, MK_LBUTTON_SHIFT_CTRL, D_NONE, D_NONE, D_NONE },
    { __LINE__, OP_NONE_OR_COPY, S_OK, S_OK, MK_LBUTTON_SHIFT_CTRL, D_COPY, D_NONE_OR_COPY, D_NONE_OR_COPY },
    { __LINE__, OP_NONE_OR_COPY, S_OK, S_OK, MK_LBUTTON_SHIFT_CTRL, D_COPY | D_MOVE, D_NONE_OR_COPY, D_NONE_OR_COPY },
    { __LINE__, OP_LINK, S_OK, S_OK, MK_LBUTTON_SHIFT_CTRL, D_COPY | D_MOVE | D_LINK, D_LINK, D_LINK },
    { __LINE__, OP_LINK, S_OK, S_OK, MK_LBUTTON_SHIFT_CTRL, D_COPY | D_LINK, D_LINK, D_LINK },
    { __LINE__, OP_NONE_OR_MOVE, S_OK, S_OK, MK_LBUTTON_SHIFT_CTRL, D_MOVE, D_NONE_OR_MOVE, D_NONE },
    { __LINE__, OP_LINK, S_OK, S_OK, MK_LBUTTON_SHIFT_CTRL, D_MOVE | D_LINK, D_LINK, D_LINK },
    { __LINE__, OP_LINK, S_OK, S_OK, MK_LBUTTON_SHIFT_CTRL, D_LINK, D_LINK, D_LINK },
#undef MK_LBUTTON_SHIFT_CTRL

    // MK_LBUTTON | MK_CONTROL
    { __LINE__, OP_NONE, S_OK, S_OK, MK_LBUTTON | MK_CONTROL, D_NONE, D_NONE, D_NONE },
    { __LINE__, OP_COPY, S_OK, S_OK, MK_LBUTTON | MK_CONTROL, D_COPY, D_COPY, D_COPY },
    { __LINE__, OP_COPY, S_OK, S_OK, MK_LBUTTON | MK_CONTROL, D_COPY | D_MOVE, D_COPY, D_COPY },
    { __LINE__, OP_COPY, S_OK, S_OK, MK_LBUTTON | MK_CONTROL, D_COPY | D_MOVE | D_LINK, D_COPY, D_COPY },
    { __LINE__, OP_COPY, S_OK, S_OK, MK_LBUTTON | MK_CONTROL, D_COPY | D_LINK, D_COPY, D_COPY },
    { __LINE__, OP_NONE_OR_MOVE, S_OK, S_OK, MK_LBUTTON | MK_CONTROL, D_MOVE, D_NONE_OR_MOVE, D_NONE },
    { __LINE__, OP_NONE_OR_MOVE, S_OK, S_OK, MK_LBUTTON | MK_CONTROL, D_MOVE | D_LINK, D_NONE_OR_MOVE, D_NONE },
    { __LINE__, OP_NONE_OR_LINK, S_OK, S_OK, MK_LBUTTON | MK_CONTROL, D_LINK, D_NONE_OR_LINK, D_NONE_OR_LINK },
};

static void DoCreateTestFile(LPCWSTR pszFileName)
{
    FILE *fp = _wfopen(pszFileName, L"wb");
    ok(fp != NULL, "fp is NULL for '%S'\n", pszFileName);
    fclose(fp);
}

HRESULT DoCreateShortcut(
    LPCWSTR pszLnkFileName,
    LPCWSTR pszTargetPathName)
{
    CComPtr<IPersistFile> ppf;
    CComPtr<IShellLinkW> psl;
    HRESULT hr;

    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                          IID_IShellLinkW, (LPVOID *)&psl);
    if (SUCCEEDED(hr))
    {
        psl->SetPath(pszTargetPathName);

        hr = psl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);
        if (SUCCEEDED(hr))
        {
            hr = ppf->Save(pszLnkFileName, TRUE);
        }
    }

    return hr;
}

static HRESULT
GetUIObjectOfAbsPidl(PIDLIST_ABSOLUTE pidl, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;

    LPCITEMIDLIST pidlLast;
    CComPtr<IShellFolder> psf;
    HRESULT hr = SHBindToParent(pidl, IID_IShellFolder, (LPVOID *)&psf,
                                &pidlLast);
    if (FAILED(hr))
        return hr;

    hr = psf->GetUIObjectOf(NULL, 1, &pidlLast, riid, NULL, ppvOut);
    return hr;
}

static HRESULT
GetUIObjectOfPath(LPCWSTR pszPath, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;

    PIDLIST_ABSOLUTE pidl = ILCreateFromPathW(pszPath);
    if (!pidl)
        return E_FAIL;

    HRESULT hr = GetUIObjectOfAbsPidl(pidl, riid, ppvOut);

    CoTaskMemFree(pidl);

    return hr;
}

BOOL DoSpecExistsW(LPCWSTR pszSpec)
{
    WIN32_FIND_DATAW find;
    HANDLE hFind = FindFirstFileW(pszSpec, &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        FindClose(hFind);
        return TRUE;
    }
    return FALSE;
}

void DoDeleteSpecW(LPCWSTR pszSpec)
{
    WCHAR szPath[MAX_PATH], szFile[MAX_PATH];
    lstrcpyW(szPath, pszSpec);
    PathRemoveFileSpecW(szPath);

    WIN32_FIND_DATAW find;
    HANDLE hFind = FindFirstFileW(pszSpec, &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            lstrcpyW(szFile, szPath);
            PathAppendW(szFile, find.cFileName);
            DeleteFileW(szFile);
        } while (FindNextFileW(hFind, &find));

        FindClose(hFind);
    }
}

static void DoTestEntry(const TEST_ENTRY *pEntry)
{
    int line = pEntry->line;
    HRESULT hr;
    PIDLIST_ABSOLUTE pidlDesktop = NULL;
    CComPtr<IDropTarget> pDropTarget;
    CComPtr<IDataObject> pDataObject;

    // get the desktop PIDL
    SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidlDesktop);
    ok(!!pidlDesktop, "pidlDesktop is NULL\n");

    // build paths
    //
    SHGetPathFromIDListW(pidlDesktop, s_szDroppedToItem);
    PathAppendW(s_szDroppedToItem, DROPPED_ON_FILE);

    GetModuleFileNameW(NULL, s_szSrcTestFile, _countof(s_szSrcTestFile));
    PathRemoveFileSpecW(s_szSrcTestFile);
    PathAppendW(s_szSrcTestFile, TESTFILENAME);

    lstrcpyW(s_szDestTestFile, s_szDestFolder);
    PathAppendW(s_szDestTestFile, TESTFILENAME);

    lstrcpyW(s_szDestLinkSpec, s_szDestFolder);
    PathAppendW(s_szDestLinkSpec, L"*DragDropTest*.lnk");

    //trace("s_szSrcTestFile: '%S'\n", s_szSrcTestFile);
    //trace("s_szDestTestFile: '%S'\n", s_szDestTestFile);
    //trace("s_szDestLinkSpec: '%S'\n", s_szDestLinkSpec);
    //trace("s_szDroppedToItem: '%S'\n", s_szDroppedToItem);

    // create or delete files
    //
    DoCreateTestFile(s_szSrcTestFile);
    DeleteFileW(s_szDestTestFile);
    DoDeleteSpecW(s_szDestLinkSpec);
    DeleteFileW(s_szDroppedToItem);
    DoCreateShortcut(s_szDroppedToItem, s_szDestFolder);

    // check file existence
    //
    ok(PathIsDirectoryW(s_szDestFolder), "s_szDestFolder is not directory\n");
    ok(PathFileExistsW(s_szSrcTestFile), "s_szSrcTestFile doesn't exist\n");
    ok(!DoSpecExistsW(s_szDestLinkSpec), "s_szDestLinkSpec doesn't exist\n");
    ok(!PathFileExistsW(s_szDestTestFile), "s_szDestTestFile exists\n");

    // get an IDataObject
    pDataObject = NULL;
    hr = GetUIObjectOfPath(s_szSrcTestFile, IID_IDataObject, (LPVOID *)&pDataObject);
    ok_long(hr, S_OK);

    // get an IDropTarget
    CComPtr<IEnumIDList> pEnumIDList;
    PIDLIST_ABSOLUTE pidl = NULL;
    hr = s_pDesktop->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS,
                                 &pEnumIDList);
    ok_long(hr, S_OK);
    while (pEnumIDList->Next(1, &pidl, NULL) == S_OK)
    {
        WCHAR szText[MAX_PATH];
        SHGetPathFromIDListW(pidl, szText);
        if (wcsstr(szText, DROPPED_ON_FILE) != NULL)
        {
            break;
        }
        CoTaskMemFree(pidl);
        pidl = NULL;
    }
    ok(pidl != NULL, "Line %d: pidl is NULL\n", line);
    pDropTarget = NULL;
    PITEMID_CHILD pidlLast = ILFindLastID(pidl);
    hr = s_pDesktop->GetUIObjectOf(NULL, 1, &pidlLast, IID_IDropTarget,
                                   NULL, (LPVOID *)&pDropTarget);
    CoTaskMemFree(pidl);
    ok_long(hr, S_OK);

    if (!pDropTarget)
    {
        skip("Line %d: pDropTarget was NULL\n", line);

        // clean up
        DeleteFileW(s_szSrcTestFile);
        DeleteFileW(s_szDestTestFile);
        DoDeleteSpecW(s_szDestLinkSpec);
        ILFree(pidlDesktop);

        return;
    }

    // DragEnter
    POINTL ptl = { 0, 0 };
    DWORD dwKeyState = pEntry->dwKeyState;
    DWORD dwEffects = pEntry->dwEffects1;
    hr = pDropTarget->DragEnter(pDataObject, dwKeyState, ptl, &dwEffects);

    ok(hr == pEntry->hr1, "Line %d: hr1 was %08lX\n", line, hr);

    switch (pEntry->dwEffects2)
    {
    case D_NONE_OR_COPY:
        ok((dwEffects == D_NONE || dwEffects == D_COPY),
           "Line %d: dwEffects2 was %08lX\n", line, dwEffects);
        break;
    case D_NONE_OR_MOVE:
        ok((dwEffects == D_NONE || dwEffects == D_MOVE),
           "Line %d: dwEffects2 was %08lX\n", line, dwEffects);
        break;
    case D_NONE_OR_LINK:
        ok((dwEffects == D_NONE || dwEffects == D_LINK),
           "Line %d: dwEffects2 was %08lX\n", line, dwEffects);
        break;
    default:
        ok(dwEffects == pEntry->dwEffects2,
           "Line %d: dwEffects2 was %08lX\n", line, dwEffects);
        break;
    }

    // Drop
    hr = pDropTarget->Drop(pDataObject, dwKeyState, ptl, &dwEffects);
    ok(hr == pEntry->hr2, "Line %d: hr2 was %08lX\n", line, hr);

    switch (pEntry->dwEffects3)
    {
    case D_NONE_OR_COPY:
        ok((dwEffects == D_NONE || dwEffects == D_COPY),
           "Line %d: dwEffects3 was %08lX\n", line, dwEffects);
        break;
    case D_NONE_OR_MOVE:
        ok((dwEffects == D_NONE || dwEffects == D_MOVE),
           "Line %d: dwEffects3 was %08lX\n", line, dwEffects);
        break;
    case D_NONE_OR_LINK:
        ok((dwEffects == D_NONE || dwEffects == D_LINK),
           "Line %d: dwEffects3 was %08lX\n", line, dwEffects);
        break;
    default:
        ok(dwEffects == pEntry->dwEffects3,
           "Line %d: dwEffects3 was %08lX\n", line, dwEffects);
        break;
    }

    // check file existence by pEntry->op
    switch (pEntry->op)
    {
    case OP_NONE:
        ok(PathFileExistsW(s_szSrcTestFile), "Line %d: src not exists\n", line);
        ok(!PathFileExistsW(s_szDestTestFile), "Line %d: dest exists\n", line);
        ok(!DoSpecExistsW(s_szDestLinkSpec), "Line %d: link exists\n", line);
        break;
    case OP_COPY:
        ok(PathFileExistsW(s_szSrcTestFile), "Line %d: src not exists\n", line);
        ok(PathFileExistsW(s_szDestTestFile), "Line %d: dest not exists\n", line);
        ok(!DoSpecExistsW(s_szDestLinkSpec), "Line %d: link exists\n", line);
        break;
    case OP_MOVE:
        ok(!PathFileExistsW(s_szSrcTestFile), "Line %d: src exists\n", line);
        ok(PathFileExistsW(s_szDestTestFile), "Line %d: dest not exists\n", line);
        ok(!DoSpecExistsW(s_szDestLinkSpec), "Line %d: link exists\n", line);
        break;
    case OP_LINK:
        ok(PathFileExistsW(s_szSrcTestFile), "Line %d: src not exists\n", line);
        ok(!PathFileExistsW(s_szDestTestFile), "Line %d: dest not exists\n", line);
        ok(DoSpecExistsW(s_szDestLinkSpec), "Line %d: link not exists\n", line);
        break;
    case OP_NONE_OR_COPY:
        ok(PathFileExistsW(s_szSrcTestFile), "Line %d: src not exists\n", line);
        ok(!DoSpecExistsW(s_szDestLinkSpec), "Line %d: link exists\n", line);
        break;
    case OP_NONE_OR_MOVE:
        ok(PathFileExistsW(s_szSrcTestFile) != PathFileExistsW(s_szDestTestFile),
           "Line %d: It must be either None or Move\n", line);
        break;
    case OP_NONE_OR_LINK:
        ok(PathFileExistsW(s_szSrcTestFile), "Line %d: src not exists\n", line);
        ok(!PathFileExistsW(s_szDestTestFile), "Line %d: dest not exists\n", line);
        break;
    }

    // clean up
    DeleteFileW(s_szSrcTestFile);
    DeleteFileW(s_szDestTestFile);
    DoDeleteSpecW(s_szDestLinkSpec);
    ILFree(pidlDesktop);
}

START_TEST(DragDrop)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok_int(SUCCEEDED(hr), TRUE);

    SHGetDesktopFolder(&s_pDesktop);
    ok(!!s_pDesktop, "s_pDesktop is NULL\n");

    BOOL ret = SHGetSpecialFolderPathW(NULL, s_szDestFolder, CSIDL_DESKTOP, FALSE);
    ok_int(ret, TRUE);

    for (size_t i = 0; i < _countof(s_TestEntries); ++i)
    {
        DoTestEntry(&s_TestEntries[i]);
    }

    DeleteFileW(s_szSrcTestFile);
    DeleteFileW(s_szDestTestFile);
    DoDeleteSpecW(s_szDestLinkSpec);
    DeleteFileW(s_szDroppedToItem);

    CoUninitialize();
}
