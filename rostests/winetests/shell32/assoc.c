/* Unit test suite for various shell Association objects
 *
 * Copyright 2012 Detlef Riekenberg
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS

#include <stdarg.h>

#include "shlwapi.h"
#include "shlguid.h"
#include "shobjidl.h"

#include "wine/test.h"


static void test_IQueryAssociations_QueryInterface(void)
{
    IQueryAssociations *qa;
    IQueryAssociations *qa2;
    IUnknown *unk;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_QueryAssociations, NULL, CLSCTX_INPROC_SERVER, &IID_IQueryAssociations, (void*)&qa);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IQueryAssociations_QueryInterface(qa, &IID_IQueryAssociations, (void**)&qa2);
    ok(hr == S_OK, "QueryInterface (IQueryAssociations) returned 0x%x\n", hr);
    if (SUCCEEDED(hr)) {
        IQueryAssociations_Release(qa2);
    }

    hr = IQueryAssociations_QueryInterface(qa, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface (IUnknown) returned 0x%x\n", hr);
    if (SUCCEEDED(hr)) {
        IUnknown_Release(unk);
    }

    hr = IQueryAssociations_QueryInterface(qa, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "got 0x%x (expected E_POINTER)\n", hr);

    IQueryAssociations_Release(qa);
}


static void test_IApplicationAssociationRegistration_QueryInterface(IApplicationAssociationRegistration *appreg)
{
    IApplicationAssociationRegistration *appreg2;
    IUnknown *unk;
    HRESULT hr;

    hr = IApplicationAssociationRegistration_QueryInterface(appreg, &IID_IApplicationAssociationRegistration,
       (void**)&appreg2);
    ok(hr == S_OK, "QueryInterface (IApplicationAssociationRegistration) returned 0x%x\n", hr);
    if (SUCCEEDED(hr)) {
        IApplicationAssociationRegistration_Release(appreg2);
    }

    hr = IApplicationAssociationRegistration_QueryInterface(appreg, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface (IUnknown) returned 0x%x\n", hr);
    if (SUCCEEDED(hr)) {
        IUnknown_Release(unk);
    }

    hr = IApplicationAssociationRegistration_QueryInterface(appreg, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "got 0x%x (expected E_POINTER)\n", hr);
}

struct assoc_getstring_test
{
    const WCHAR *key;
    ASSOCF       flags;
    ASSOCSTR     str;
    DWORD        len;
    HRESULT      hr;
    HRESULT      brokenhr;
};

static const WCHAR httpW[] = {'h','t','t','p',0};
static const WCHAR badW[] = {'b','a','d','b','a','d',0};

static struct assoc_getstring_test getstring_tests[] =
{
    { httpW, 0, ASSOCSTR_EXECUTABLE, 2, 0x8007007a /* E_NOT_SUFFICIENT_BUFFER */, S_OK },
    { httpW, ASSOCF_NOTRUNCATE, ASSOCSTR_EXECUTABLE, 2, E_POINTER },
    { NULL }
};

static void test_IQueryAssociations_GetString(void)
{
    struct assoc_getstring_test *ptr = getstring_tests;
    IQueryAssociations *assoc;
    HRESULT hr;
    DWORD len;
    int i = 0;

    hr = CoCreateInstance(&CLSID_QueryAssociations, NULL, CLSCTX_INPROC_SERVER, &IID_IQueryAssociations, (void*)&assoc);
    ok(hr == S_OK, "failed to create object, 0x%x\n", hr);

    hr = IQueryAssociations_Init(assoc, 0, httpW, NULL, NULL);
    ok(hr == S_OK, "Init failed, 0x%x\n", hr);

    len = 0;
    hr = IQueryAssociations_GetString(assoc, 0, ASSOCSTR_EXECUTABLE, NULL, NULL, &len);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);
    ok(len > 0, "got wrong needed length, %d\n", len);

    while (ptr->key)
    {
        WCHAR buffW[MAX_PATH];
        DWORD len;

        hr = IQueryAssociations_Init(assoc, 0, ptr->key, NULL, NULL);
        ok(hr == S_OK, "%d: Init failed, 0x%x\n", i, hr);

        len = ptr->len;
        buffW[0] = ptr->flags & ASSOCF_NOTRUNCATE ? 0x1 : 0;
        hr = IQueryAssociations_GetString(assoc, ptr->flags, ptr->str, NULL, buffW, &len);
        if (hr != ptr->hr)
            ok(broken(hr == ptr->brokenhr), "%d: GetString failed, 0x%08x\n", i, hr);
        else
        {
            ok(hr == ptr->hr, "%d: GetString failed, 0x%08x\n", i, hr);
            ok(len > ptr->len, "%d: got needed length %d\n", i, len);
        }

        /* even with ASSOCF_NOTRUNCATE it's null terminated */
        if ((ptr->flags & ASSOCF_NOTRUNCATE) && (ptr->len > 0))
            ok(buffW[0] == 0 || broken(buffW[0] == 0x1) /* pre win7 */, "%d: got %x\n", i, buffW[0]);

        if (!(ptr->flags & ASSOCF_NOTRUNCATE) && ptr->len && FAILED(ptr->hr))
            ok(buffW[0] != 0, "%d: got %x\n", i, buffW[0]);

        i++;
        ptr++;
    }

    IQueryAssociations_Release(assoc);
}

static void test_IQueryAssociations_Init(void)
{
    IQueryAssociations *assoc;
    HRESULT hr;
    DWORD len;

    hr = CoCreateInstance(&CLSID_QueryAssociations, NULL, CLSCTX_INPROC_SERVER, &IID_IQueryAssociations, (void*)&assoc);
    ok(hr == S_OK, "failed to create object, 0x%x\n", hr);

    hr = IQueryAssociations_Init(assoc, 0, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Init failed, 0x%08x\n", hr);

    hr = IQueryAssociations_Init(assoc, 0, httpW, NULL, NULL);
    ok(hr == S_OK, "Init failed, 0x%08x\n", hr);

    hr = IQueryAssociations_Init(assoc, 0, badW, NULL, NULL);
    ok(hr == S_OK || broken(hr == S_FALSE) /* pre-vista */, "Init failed, 0x%08x\n", hr);

    len = 0;
    hr = IQueryAssociations_GetString(assoc, 0, ASSOCSTR_EXECUTABLE, NULL, NULL, &len);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION) || broken(hr == E_FAIL) /* pre-vista */, "got 0x%08x\n", hr);

    IQueryAssociations_Release(assoc);
}

static void test_IApplicationAssociationRegistration_QueryCurrentDefault(IApplicationAssociationRegistration *appreg)
{
    static const WCHAR emptyW[] = {0};
    static const WCHAR txtW[] = {'.','t','x','t',0};
    static const WCHAR spacetxtW[] = {' ','.','t','x','t',0};
    HRESULT hr;
    LPWSTR assocprog = NULL;

    hr = IApplicationAssociationRegistration_QueryCurrentDefault(appreg, emptyW, AT_URLPROTOCOL, AL_EFFECTIVE, &assocprog);
    ok(hr == E_INVALIDARG, "got 0x%x\n", hr);

    hr = IApplicationAssociationRegistration_QueryCurrentDefault(appreg, emptyW, AT_FILEEXTENSION, AL_EFFECTIVE, &assocprog);
    ok(hr == E_INVALIDARG, "got 0x%x\n", hr);

    hr = IApplicationAssociationRegistration_QueryCurrentDefault(appreg, spacetxtW, AT_FILEEXTENSION, AL_EFFECTIVE, &assocprog);
    ok(hr == E_INVALIDARG || hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION) /*Win8*/, "got 0x%x\n", hr);

    hr = IApplicationAssociationRegistration_QueryCurrentDefault(appreg, httpW, AT_URLPROTOCOL, AL_EFFECTIVE, NULL);
    ok(hr == E_INVALIDARG, "got 0x%x\n", hr);

    /* AT_FILEEXTENSION must start with a period */
    hr = IApplicationAssociationRegistration_QueryCurrentDefault(appreg, txtW, AT_FILEEXTENSION, AL_EFFECTIVE, &assocprog);
    ok(hr == S_OK, "got 0x%x\n", hr);
    trace("%s\n", wine_dbgstr_w(assocprog));
    CoTaskMemFree(assocprog);

    hr = IApplicationAssociationRegistration_QueryCurrentDefault(appreg, emptyW, AT_STARTMENUCLIENT, AL_EFFECTIVE, &assocprog);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION), "got 0x%x\n", hr);

    hr = IApplicationAssociationRegistration_QueryCurrentDefault(appreg, emptyW, AT_MIMETYPE, AL_EFFECTIVE, &assocprog);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION), "got 0x%x\n", hr);

    hr = IApplicationAssociationRegistration_QueryCurrentDefault(appreg, httpW, AT_URLPROTOCOL, AL_EFFECTIVE, &assocprog);
    todo_wine ok(hr == S_OK, "got 0x%x\n", hr);
    trace("%s\n", wine_dbgstr_w(assocprog));

    CoTaskMemFree(assocprog);
}

START_TEST(assoc)
{
    IQueryAssociations *qa;
    IApplicationAssociationRegistration *appreg = NULL;
    HRESULT hr;

    CoInitialize(NULL);

    /* this works since XP */
    hr = CoCreateInstance(&CLSID_QueryAssociations, NULL, CLSCTX_INPROC_SERVER, &IID_IQueryAssociations, (void*)&qa);
    if (hr == S_OK)
    {
        test_IQueryAssociations_QueryInterface();
        test_IQueryAssociations_GetString();
        test_IQueryAssociations_Init();

        IQueryAssociations_Release(qa);
    }
    else
        win_skip("IQueryAssociations not supported, 0x%x\n", hr);

    /* this works since Vista */
    hr = CoCreateInstance(&CLSID_ApplicationAssociationRegistration, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IApplicationAssociationRegistration, (LPVOID*)&appreg);
    if (hr == S_OK)
    {
        test_IApplicationAssociationRegistration_QueryInterface(appreg);
        test_IApplicationAssociationRegistration_QueryCurrentDefault(appreg);

        IApplicationAssociationRegistration_Release(appreg);
    }
    else
        win_skip("IApplicationAssociationRegistration not supported: 0x%x\n", hr);

    CoUninitialize();
}
