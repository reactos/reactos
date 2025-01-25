/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SHIsBadInterfacePtr
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <undocshell.h>

typedef BOOL (WINAPI *FN_SHIsBadInterfacePtr)(LPCVOID, UINT_PTR);

static HRESULT STDMETHODCALLTYPE dummy_QueryInterface(REFIID riid, LPVOID *ppvObj) { return S_OK; }
static ULONG STDMETHODCALLTYPE dummy_AddRef() { return S_OK; }
static ULONG STDMETHODCALLTYPE dummy_Release() { return S_OK; }

START_TEST(SHIsBadInterfacePtr)
{
    struct CUnknownVtbl
    {
        HRESULT (STDMETHODCALLTYPE *QueryInterface)(REFIID riid, LPVOID *ppvObj);
        ULONG (STDMETHODCALLTYPE *AddRef)();
        ULONG (STDMETHODCALLTYPE *Release)();
    };
    struct CUnknown { CUnknownVtbl *lpVtbl; };

    BOOL ret;
    FN_SHIsBadInterfacePtr SHIsBadInterfacePtr =
        (FN_SHIsBadInterfacePtr)GetProcAddress(GetModuleHandleW(L"shell32"), MAKEINTRESOURCEA(84));

    if (!SHIsBadInterfacePtr)
    {
        skip("There is no SHIsBadInterfacePtr\n");
        return;
    }

    ret = SHIsBadInterfacePtr(NULL, 1);
    ok_int(ret, TRUE);

    CUnknown unk1 = { NULL };
    ret = SHIsBadInterfacePtr(&unk1, 1);
    ok_int(ret, TRUE);

    CUnknownVtbl vtbl1 = { dummy_QueryInterface, dummy_AddRef, NULL };
    CUnknown unk2 = { &vtbl1 };
    ret = SHIsBadInterfacePtr(&unk2, 1);
    ok_int(ret, TRUE);

    CUnknownVtbl vtbl2 = { dummy_QueryInterface, dummy_AddRef, dummy_Release };
    CUnknown unk3 = { &vtbl2 };
    ret = SHIsBadInterfacePtr(&unk3, 1);
    ok_int(ret, FALSE);
}
