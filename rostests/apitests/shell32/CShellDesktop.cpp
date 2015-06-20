/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CShellDesktop
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "shelltest.h"
#include <atlbase.h>
#include <atlcom.h>
#include <strsafe.h>

#define NDEBUG
#include <debug.h>
#include <shellutils.h>

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

    TestShellFolder(psf2);
}
