/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CMyComputer
 * PROGRAMMER:      Giannis Adamopoulos
 */

#include "shelltest.h"

#define NDEBUG
#include <debug.h>
#include <stdio.h>
#include <shellutils.h>
#include <versionhelpers.h>

LPITEMIDLIST _CreateDummyPidl()
{
    /* Create a tiny pidl with no contents */
    LPITEMIDLIST testpidl = (LPITEMIDLIST)SHAlloc(3 * sizeof(WORD));
    testpidl->mkid.cb = 2 * sizeof(WORD);
    *(WORD*)((char*)testpidl + (int)(2 * sizeof(WORD))) = 0;

    return testpidl;
}

VOID TestUninitialized()
{
    CComPtr<IShellFolder> psf;
    CComPtr<IEnumIDList> penum;
    CComPtr<IDropTarget> pdt;
    CComPtr<IContextMenu> pcm;
    CComPtr<IShellView> psv;
    LPITEMIDLIST retrievedPidl;
    ULONG pceltFetched;
    HRESULT hr;

    /* Create a CFSFolder */
    hr = CoCreateInstance(CLSID_ShellFSFolder, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellFolder, &psf));
    ok(hr == S_OK, "hr = %lx\n", hr);

    /* An uninitialized CFSFolder doesn't contain any items */
    hr = psf->EnumObjects(NULL, 0, &penum);
    ok(hr == S_OK, "hr = %lx\n", hr);
    hr = penum->Next(0, &retrievedPidl, &pceltFetched);
    ok(hr == S_FALSE, "hr = %lx\n", hr);
    hr = penum->Next(1, &retrievedPidl, &pceltFetched);
    ok(hr == S_FALSE, "hr = %lx\n", hr);

    /* It supports viewing */
    hr = psf->CreateViewObject(NULL, IID_PPV_ARG(IDropTarget, &pdt));
    ok(hr == S_OK, "hr = %lx\n", hr);
    hr = psf->CreateViewObject(NULL, IID_PPV_ARG(IContextMenu, &pcm));
    ok(hr == S_OK, "hr = %lx\n", hr);
    hr = psf->CreateViewObject(NULL, IID_PPV_ARG(IShellView, &psv));
    ok(hr == S_OK, "hr = %lx\n", hr);

    /* And its display name is ... "C:\Documents and Settings\<username>\Desktop" */
    STRRET strretName;
    hr = psf->GetDisplayNameOf(NULL,SHGDN_FORPARSING,&strretName);
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(strretName.uType == STRRET_WSTR, "strretName.uType == %x\n", strretName.uType);
    ok((wcsncmp(strretName.pOleStr, L"C:\\Documents and Settings\\", 26) == 0) ||
       (wcsncmp(strretName.pOleStr, L"C:\\Users\\", 9) == 0),
       "wrong name, got: %S\n", strretName.pOleStr);
    ok(wcscmp(strretName.pOleStr + wcslen(strretName.pOleStr) - 8, L"\\Desktop") == NULL,
       "wrong name, got: %S\n", strretName.pOleStr);

    hr = psf->GetDisplayNameOf(NULL,SHGDN_FORPARSING|SHGDN_INFOLDER,&strretName);
    ok(hr == E_INVALIDARG, "hr = %lx\n", hr);




    /* Use Initialize method with  a dummy pidl and test the still non initialized CFSFolder */
    CComPtr<IPersistFolder2> ppf2;
    hr = psf->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2));
    ok(hr == S_OK, "hr = %lx\n", hr);

    LPITEMIDLIST testpidl = _CreateDummyPidl();

    hr = ppf2->Initialize(testpidl);
    ok(hr == S_OK, "hr = %lx\n", hr);

    CComHeapPtr<ITEMIDLIST> pidl;
    hr = ppf2->GetCurFolder(&pidl);
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(pidl->mkid.cb == 2 * sizeof(WORD), "got wrong pidl size, cb = %x\n", pidl->mkid.cb);

    /* methods that worked before, now fail */
    hr = psf->GetDisplayNameOf(NULL,SHGDN_FORPARSING,&strretName);
    ok(hr == (IsWindows7OrGreater() ? E_INVALIDARG : E_FAIL), "hr = %lx\n", hr);
    hr = psf->EnumObjects(NULL, 0, &penum);
    ok(hr == (IsWindows7OrGreater() ? E_INVALIDARG : HRESULT_FROM_WIN32(ERROR_CANCELLED)), "hr = %lx\n", hr);

    /* The following continue to work though */
    hr = psf->CreateViewObject(NULL, IID_PPV_ARG(IDropTarget, &pdt));
    ok(hr == S_OK, "hr = %lx\n", hr);
    hr = psf->CreateViewObject(NULL, IID_PPV_ARG(IContextMenu, &pcm));
    ok(hr == S_OK, "hr = %lx\n", hr);
    hr = psf->CreateViewObject(NULL, IID_PPV_ARG(IShellView, &psv));
    ok(hr == S_OK, "hr = %lx\n", hr);

}

VOID TestInitialize()
{
    HRESULT hr;
    STRRET strretName;

    /* Create a CFSFolder */
    CComPtr<IShellFolder> psf;
    hr = CoCreateInstance(CLSID_ShellFSFolder, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellFolder, &psf));
    ok(hr == S_OK, "hr = %lx\n", hr);

    CComPtr<IPersistFolder3> ppf3;
    hr = psf->QueryInterface(IID_PPV_ARG(IPersistFolder3, &ppf3));
    ok(hr == S_OK, "hr = %lx\n", hr);

    LPITEMIDLIST testpidl = _CreateDummyPidl();
    PERSIST_FOLDER_TARGET_INFO pfti = {0};
    PERSIST_FOLDER_TARGET_INFO queriedPfti;
    hr = ppf3->InitializeEx(NULL, NULL, NULL);
    ok(hr == (IsWindows7OrGreater() ? E_INVALIDARG : E_OUTOFMEMORY), "hr = %lx\n", hr);

    hr = ppf3->InitializeEx(NULL, NULL, &pfti);
    ok(hr == (IsWindows7OrGreater() ? E_INVALIDARG : E_OUTOFMEMORY), "hr = %lx\n", hr);

    wcscpy(pfti.szTargetParsingName, L"C:\\");
    hr = ppf3->InitializeEx(NULL, NULL, &pfti);
    ok(hr == (IsWindows7OrGreater() ? E_INVALIDARG : E_OUTOFMEMORY), "hr = %lx\n", hr);

    hr = ppf3->InitializeEx(NULL, testpidl, NULL);
    ok(hr == S_OK, "hr = %lx\n", hr);

    hr = ppf3->GetFolderTargetInfo(&queriedPfti);
    ok(hr == (IsWindows7OrGreater() ? E_FAIL : S_OK), "hr = %lx\n", hr);

    hr = psf->GetDisplayNameOf(NULL,SHGDN_FORPARSING,&strretName);
    ok(hr == (IsWindows7OrGreater() ? E_INVALIDARG : E_FAIL), "hr = %lx\n", hr);

    pfti.szTargetParsingName[0] = 0;
    hr = ppf3->InitializeEx(NULL, testpidl, &pfti);
    ok(hr == S_OK, "hr = %lx\n", hr);

    hr = ppf3->GetFolderTargetInfo(&queriedPfti);
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(wcscmp(queriedPfti.szTargetParsingName, L"") == 0, "wrong name, got: %S\n", queriedPfti.szTargetParsingName);

    hr = psf->GetDisplayNameOf(NULL,SHGDN_FORPARSING,&strretName);
    ok(hr == (IsWindows7OrGreater() ? E_INVALIDARG : E_FAIL), "hr = %lx\n", hr);

    wcscpy(pfti.szTargetParsingName, L"C:\\");
    hr = ppf3->InitializeEx(NULL, testpidl, &pfti);
    ok(hr == S_OK, "hr = %lx\n", hr);

    hr = ppf3->GetFolderTargetInfo(&queriedPfti);
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(wcscmp(queriedPfti.szTargetParsingName, L"C:\\") == 0, "wrong name, got: %S\n", queriedPfti.szTargetParsingName);

    hr = psf->GetDisplayNameOf(NULL,SHGDN_FORPARSING,&strretName);
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(strretName.uType == STRRET_WSTR, "strretName.uType == %x\n", strretName.uType);
    ok(wcscmp(strretName.pOleStr, L"C:\\") == 0, "wrong name, got: %S\n", strretName.pOleStr);
}

VOID TestGetUIObjectOf()
{
    HRESULT hr;

    /* Create a CFSFolder */
    CComPtr<IShellFolder> psf;
    hr = CoCreateInstance(CLSID_ShellFSFolder, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellFolder, &psf));
    ok(hr == S_OK, "hr = %lx\n", hr);

    /* test 0 cidl for IDataObject */
    CComPtr<IDataObject> pdo;
    hr = psf->GetUIObjectOf(NULL, 0, NULL, IID_NULL_PPV_ARG(IDataObject, &pdo));
    ok(hr == E_INVALIDARG, "hr = %lx\n", hr);
}

START_TEST(CFSFolder)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    TestUninitialized();
    TestInitialize();
    TestGetUIObjectOf();
}
