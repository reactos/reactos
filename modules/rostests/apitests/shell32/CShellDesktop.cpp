/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CShellDesktop
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 *                  Mark Jansen
 */

#include "shelltest.h"

#include <ndk/rtlfuncs.h>
#include <stdio.h>
#include <shellutils.h>

// We would normally use S_LESSTHAN and S_GREATERTHAN, but w2k3 returns numbers like 3 and -3...
// So instead we check on the sign bit (compare result is the low word of the hresult).
#define SHORT_SIGN_BIT  0x8000

static
VOID
compare_imp(IShellFolder* psf, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, HRESULT expected)
{
    HRESULT hr;
    _SEH2_TRY
    {
        hr = psf->CompareIDs(0, pidl1, pidl2);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        winetest_ok(0, "Exception %lx!\n", _SEH2_GetExceptionCode());
        hr = HRESULT_FROM_WIN32(RtlNtStatusToDosError(_SEH2_GetExceptionCode()));
    }
    _SEH2_END;
    if (expected == S_LESSTHAN)
        winetest_ok(SUCCEEDED(hr) && (hr & SHORT_SIGN_BIT), "hr = %lx\n", hr);
    else if (expected == S_EQUAL)
        winetest_ok(hr == S_EQUAL, "hr = %lx\n", hr);
    else if (expected == S_GREATERTHAN)
        winetest_ok(SUCCEEDED(hr) && !(hr & SHORT_SIGN_BIT), "hr = %lx\n", hr);
    else
        winetest_ok(hr == expected, "hr = %lx\n", hr);
}

// make the winetest_ok look like it came from the line where the compare function was called, and not from inside the compare_imp function :)
#define compare         (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : compare_imp

static
VOID
TestCompareIDList(IShellFolder* psf)
{
    compare(psf, NULL, NULL, E_INVALIDARG);

    CComHeapPtr<ITEMIDLIST> desktop;
    HRESULT hr = SHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, NULL, &desktop);
    ok(hr == S_OK, "hr = %lx\n", hr);
    compare(psf, desktop, NULL, E_INVALIDARG);
    compare(psf, NULL, desktop, E_INVALIDARG);
    compare(psf, desktop, desktop, S_EQUAL);

    // First check the ordering of some special folders against eachother
    CComHeapPtr<ITEMIDLIST> internet;
    hr = SHGetFolderLocation(NULL, CSIDL_INTERNET, NULL, NULL, &internet);
    ok(hr == S_OK, "hr = %lx\n", hr);
    compare(psf, internet, desktop, S_LESSTHAN);
    compare(psf, desktop, internet, S_GREATERTHAN);

    CComHeapPtr<ITEMIDLIST> programs;
    hr = SHGetFolderLocation(NULL, CSIDL_PROGRAMS, NULL, NULL, &programs);
    ok(hr == S_OK, "hr = %lx\n", hr);
    compare(psf, programs, desktop, S_LESSTHAN);
    compare(psf, desktop, programs, S_GREATERTHAN);
    compare(psf, internet, programs, S_GREATERTHAN);
    compare(psf, programs, internet, S_LESSTHAN);

    // Verify that an idlist retrieved from GetCurFolder is equal to the original one.
    CComPtr<IPersistFolder2> persist;
    hr = psf->QueryInterface(IID_PPV_ARG(IPersistFolder2, &persist));
    ok(hr == S_OK, "hr = %lx\n", hr);
    if (hr == S_OK)
    {
        CComHeapPtr<ITEMIDLIST> cur;
        hr = persist->GetCurFolder(&cur);
        ok(hr == S_OK, "hr = %lx\n", hr);
        compare(psf, cur, desktop, S_EQUAL);
        compare(psf, desktop, cur, S_EQUAL);
    }

    // Compare special folders against full paths
    CComHeapPtr<ITEMIDLIST> dir1, dir2;
    PathToIDList(L"A:\\AAA.AAA", &dir1);
    PathToIDList(L"A:\\ZZZ.ZZZ", &dir2);

    compare(psf, dir1, desktop, S_LESSTHAN);
    compare(psf, desktop, dir1, S_GREATERTHAN);
    compare(psf, dir1, programs, S_LESSTHAN);
    compare(psf, programs, dir1, S_GREATERTHAN);
    compare(psf, dir1, dir1, S_EQUAL);

    compare(psf, dir2, desktop, S_LESSTHAN);
    compare(psf, desktop, dir2, S_GREATERTHAN);
    compare(psf, dir2, programs, S_LESSTHAN);
    compare(psf, programs, dir2, S_GREATERTHAN);
    compare(psf, dir2, dir2, S_EQUAL);

    CComHeapPtr<ITEMIDLIST> dir3, dir4;
    PathToIDList(L"Z:\\AAA.AAA", &dir3);
    PathToIDList(L"Z:\\ZZZ.ZZZ", &dir4);

    compare(psf, dir3, desktop, S_LESSTHAN);
    compare(psf, desktop, dir3, S_GREATERTHAN);
    compare(psf, dir3, programs, S_GREATERTHAN);
    compare(psf, programs, dir3, S_LESSTHAN);
    compare(psf, dir3, dir3, S_EQUAL);

    compare(psf, dir4, desktop, S_LESSTHAN);
    compare(psf, desktop, dir4, S_GREATERTHAN);
    compare(psf, dir4, programs, S_GREATERTHAN);
    compare(psf, programs, dir4, S_LESSTHAN);
    compare(psf, dir4, dir4, S_EQUAL);

    // Now compare the paths against eachother.
    compare(psf, dir1, dir2, S_LESSTHAN);
    compare(psf, dir2, dir1, S_GREATERTHAN);

    compare(psf, dir2, dir3, S_LESSTHAN);
    compare(psf, dir3, dir2, S_GREATERTHAN);

    compare(psf, dir3, dir4, S_LESSTHAN);
    compare(psf, dir4, dir3, S_GREATERTHAN);

    // Check that comparing desktop pidl with another one with another IShellFolder fails
    CComPtr<IShellFolder> psf2;
    hr = psf->BindToObject(programs, NULL, IID_IShellFolder, reinterpret_cast<void**>(&psf2));
    ok(hr == S_OK, "Impossible to bind to Programs pidl");
    if (hr == S_OK)
    {
        // Compare desktop pidl in programs scope should fail since it's relative pidl
        compare(psf2, desktop, programs, E_INVALIDARG);
        compare(psf2, programs, desktop, E_INVALIDARG);
        // For the same reasons, filesystem paths can't be compared with special shell
        // folders that don't have CFSFolder in children
        compare(psf2, dir1, dir2, E_INVALIDARG);
        compare(psf2, dir2, dir1, E_INVALIDARG);
    }
}

static
VOID
TestDesktopFolder(
    _In_ IShellFolder2 *psf2)
{
    HRESULT hr;
    CComPtr<IDropTarget> pdt;
    CComPtr<IDropTarget> pdt_2;
    CComPtr<IContextMenu> pcm;
    CComPtr<IContextMenu> pcm_2;
    CComPtr<IShellView> psv;
    CComPtr<IShellView> psv_2;

    hr = psf2->CreateViewObject(NULL, IID_PPV_ARG(IDropTarget, &pdt));
    ok(hr == S_OK, "hr = %lx\n", hr);

    hr = psf2->CreateViewObject(NULL, IID_PPV_ARG(IDropTarget, &pdt_2));
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(pdt != pdt_2, "Expected %p != %p\n", static_cast<PVOID>(pdt), static_cast<PVOID>(pdt_2));

    hr = psf2->CreateViewObject(NULL, IID_PPV_ARG(IContextMenu, &pcm));
    ok(hr == S_OK, "hr = %lx\n", hr);

    hr = psf2->CreateViewObject(NULL, IID_PPV_ARG(IContextMenu, &pcm_2));
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(pcm != pcm_2, "Expected %p != %p\n", static_cast<PVOID>(pcm), static_cast<PVOID>(pcm_2));

    hr = psf2->CreateViewObject(NULL, IID_PPV_ARG(IShellView, &psv));
    ok(hr == S_OK, "hr = %lx\n", hr);

    hr = psf2->CreateViewObject(NULL, IID_PPV_ARG(IShellView, &psv_2));
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(psv != psv_2, "Expected %p != %p\n", static_cast<PVOID>(psv), static_cast<PVOID>(psv_2));

    STRRET strret;
    hr = psf2->GetDisplayNameOf(NULL, 0, &strret);
    ok(hr == S_OK, "hr = %lx\n", hr);
}

VOID TestInitialize(_In_ IShellFolder *psf)
{
    CComPtr<IPersistFolder2> ppf2;
    HRESULT hr = psf->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2));
    ok(hr == S_OK, "hr = %lx\n", hr);

    /* Create a tiny pidl with no contents */
    LPITEMIDLIST testpidl = (LPITEMIDLIST)SHAlloc(3 * sizeof(WORD));
    testpidl->mkid.cb = 2 * sizeof(WORD);
    *(WORD*)((char*)testpidl + (int)(2 * sizeof(WORD))) = 0;

    hr = ppf2->Initialize(testpidl);
    ok(hr == E_INVALIDARG, "hr = %lx\n", hr);

    //crashes in xp, works on win10
    //hr = ppf2->Initialize(NULL);
    //ok(hr == S_OK, "hr = %lx\n", hr);
    //hr = ppf2->Initialize((LPCITEMIDLIST)0xdeaddead);
    //ok(hr == S_OK, "hr = %lx\n", hr);
    //hr = ppf2->GetCurFolder(NULL);
    //ok(hr == E_INVALIDARG, "hr = %lx\n", hr);

    CComHeapPtr<ITEMIDLIST> pidl;
    hr = ppf2->GetCurFolder(&pidl);
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(pidl->mkid.cb == 0, "expected empty pidl got cb = %x\n", pidl->mkid.cb);
}

START_TEST(CShellDesktop)
{
    HRESULT hr;
    CComPtr<IShellFolder2> psf2;
    CComPtr<IShellFolder2> psf2_2;
    CComPtr<IShellFolder> psf;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(CLSID_ShellDesktop,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IShellFolder2, &psf2));
    ok(hr == S_OK, "hr = %lx\n", hr);
    if (FAILED(hr))
    {
        skip("Could not instantiate CShellDesktop\n");
        return;
    }

    /* second create should give us a pointer to the same object */
    hr = CoCreateInstance(CLSID_ShellDesktop,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IShellFolder2, &psf2_2));
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(psf2 == psf2_2, "Expected %p == %p\n", static_cast<PVOID>(psf2), static_cast<PVOID>(psf2_2));

    /* SHGetDesktopFolder should also give us the same pointer */
    hr = SHGetDesktopFolder(&psf);
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(psf == static_cast<IShellFolder *>(psf2), "Expected %p == %p\n", static_cast<PVOID>(psf), static_cast<PVOID>(psf2));

    TestDesktopFolder(psf2);
    TestCompareIDList(psf);
    TestInitialize(psf);
}
