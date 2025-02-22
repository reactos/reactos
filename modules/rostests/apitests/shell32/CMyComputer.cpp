/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CMyComputer
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "shelltest.h"

#define NDEBUG
#include <debug.h>
#include <stdio.h>
#include <shellutils.h>

#define INVALID_POINTER ((PVOID)(ULONG_PTR)0xdeadbeefdeadbeefULL)

static
VOID
TestShellFolder(
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
}

VOID TestInitialize(_In_ IShellFolder2 *psf2)
{
    CComPtr<IPersistFolder2> ppf2;
    HRESULT hr = psf2->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2));
    ok(hr == S_OK, "hr = %lx\n", hr);

    hr = ppf2->Initialize(NULL);
    ok(hr == S_OK, "hr = %lx\n", hr);

    hr = ppf2->Initialize((LPCITEMIDLIST)INVALID_POINTER);
    ok(hr == S_OK, "hr = %lx\n", hr);

    //crashes in xp
    //hr = ppf2->GetCurFolder(NULL);
    //ok(hr == E_INVALIDARG, "hr = %lx\n", hr);

    CComHeapPtr<ITEMIDLIST> pidl;
    hr = ppf2->GetCurFolder(&pidl);
    ok(hr == S_OK, "hr = %lx\n", hr);
    // 0 in win10, 14 in xp
    ok(pidl->mkid.cb == 0x14, "expected empty pidl got cb = %x\n", pidl->mkid.cb);
}

START_TEST(CMyComputer)
{
    HRESULT hr;
    CComPtr<IShellFolder2> psf2;
    CComPtr<IShellFolder2> psf2_2;
    CComPtr<IShellFolder> psf;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(CLSID_MyComputer,
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
    hr = CoCreateInstance(CLSID_MyComputer,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IShellFolder2, &psf2_2));
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(psf2 == psf2_2, "Expected %p == %p\n", static_cast<PVOID>(psf2), static_cast<PVOID>(psf2_2));

    TestShellFolder(psf2);
    TestInitialize(psf2);
}
