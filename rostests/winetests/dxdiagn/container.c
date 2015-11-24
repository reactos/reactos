/*
 * Unit tests for IDxDiagContainer
 *
 * Copyright 2010 Andrew Nguyen
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

#include <stdio.h>
#include <wine/dxdiag.h>
#include <oleauto.h>
#include <wine/test.h>

struct property_test
{
    const WCHAR *prop;
    VARTYPE vt;
};

static IDxDiagProvider *pddp;
static IDxDiagContainer *pddc;

static const WCHAR DxDiag_SystemInfo[] = {'D','x','D','i','a','g','_','S','y','s','t','e','m','I','n','f','o',0};
static const WCHAR DxDiag_DisplayDevices[] = {'D','x','D','i','a','g','_','D','i','s','p','l','a','y','D','e','v','i','c','e','s',0};
static const WCHAR DxDiag_SoundDevices[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','S','o','u','n','d','.',
                                            'D','x','D','i','a','g','_','S','o','u','n','d','D','e','v','i','c','e','s',0};
static const WCHAR DxDiag_SoundCaptureDevices[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','S','o','u','n','d','.',
                                                   'D','x','D','i','a','g','_','S','o','u','n','d','C','a','p','t','u','r','e',
                                                   'D','e','v','i','c','e','s',0};

/* Based on debugstr_variant in dlls/jscript/jsutils.c. */
static const char *debugstr_variant(const VARIANT *var)
{
    static char buf[400];

    if (!var)
        return "(null)";

    switch (V_VT(var))
    {
    case VT_EMPTY:
        return "{VT_EMPTY}";
    case VT_BSTR:
        sprintf(buf, "{VT_BSTR: %s}", wine_dbgstr_w(V_BSTR(var)));
        break;
    case VT_BOOL:
        sprintf(buf, "{VT_BOOL: %x}", V_BOOL(var));
        break;
    case VT_UI4:
        sprintf(buf, "{VT_UI4: %u}", V_UI4(var));
        break;
    default:
        sprintf(buf, "{vt %d}", V_VT(var));
        break;
    }

    return buf;
}

static BOOL create_root_IDxDiagContainer(void)
{
    HRESULT hr;
    DXDIAG_INIT_PARAMS params;

    hr = CoCreateInstance(&CLSID_DxDiagProvider, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IDxDiagProvider, (LPVOID*)&pddp);
    if (SUCCEEDED(hr))
    {
        params.dwSize = sizeof(params);
        params.dwDxDiagHeaderVersion = DXDIAG_DX9_SDK_VERSION;
        params.bAllowWHQLChecks = FALSE;
        params.pReserved = NULL;
        hr = IDxDiagProvider_Initialize(pddp, &params);
        if (SUCCEEDED(hr))
        {
            hr = IDxDiagProvider_GetRootContainer(pddp, &pddc);
            if (SUCCEEDED(hr))
                return TRUE;
        }
        IDxDiagProvider_Release(pddp);
    }
    return FALSE;
}

static void test_GetNumberOfChildContainers(void)
{
    HRESULT hr;
    DWORD count;

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, NULL);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::GetNumberOfChildContainers to return E_INVALIDARG, got 0x%08x\n", hr);

    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, &count);
    ok(hr == S_OK,
       "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08x\n", hr);
    if (hr == S_OK)
        ok(count != 0, "Expected the number of child containers for the root container to be non-zero\n");

    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_GetNumberOfProps(void)
{
    HRESULT hr;
    DWORD count;

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    hr = IDxDiagContainer_GetNumberOfProps(pddc, NULL);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetNumberOfProps to return E_INVALIDARG, got 0x%08x\n", hr);

    hr = IDxDiagContainer_GetNumberOfProps(pddc, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfProps to return S_OK, got 0x%08x\n", hr);
    if (hr == S_OK)
        ok(count == 0, "Expected the number of properties for the root container to be zero\n");

    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_EnumChildContainerNames(void)
{
    HRESULT hr;
    WCHAR container[256];
    DWORD maxcount, index;
    static const WCHAR testW[] = {'t','e','s','t',0};
    static const WCHAR zerotestW[] = {0,'e','s','t',0};

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    /* Test various combinations of invalid parameters. */
    hr = IDxDiagContainer_EnumChildContainerNames(pddc, 0, NULL, 0);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG, got 0x%08x\n", hr);

    hr = IDxDiagContainer_EnumChildContainerNames(pddc, 0, NULL, sizeof(container)/sizeof(WCHAR));
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG, got 0x%08x\n", hr);

    /* Test the conditions in which the output buffer can be modified. */
    memcpy(container, testW, sizeof(testW));
    hr = IDxDiagContainer_EnumChildContainerNames(pddc, 0, container, 0);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(!memcmp(container, testW, sizeof(testW)),
       "Expected the container buffer to be untouched, got %s\n", wine_dbgstr_w(container));

    memcpy(container, testW, sizeof(testW));
    hr = IDxDiagContainer_EnumChildContainerNames(pddc, ~0, container, 0);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(!memcmp(container, testW, sizeof(testW)),
       "Expected the container buffer to be untouched, got %s\n", wine_dbgstr_w(container));

    memcpy(container, testW, sizeof(testW));
    hr = IDxDiagContainer_EnumChildContainerNames(pddc, ~0, container, sizeof(container)/sizeof(WCHAR));
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(!memcmp(container, zerotestW, sizeof(zerotestW)),
       "Expected the container buffer string to be empty, got %s\n", wine_dbgstr_w(container));

    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, &maxcount);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08x\n", hr);
    if (FAILED(hr))
    {
        skip("IDxDiagContainer::GetNumberOfChildContainers failed\n");
        goto cleanup;
    }

    trace("Starting child container enumeration of the root container:\n");

    /* We should be able to enumerate as many child containers as the value
     * that IDxDiagContainer::GetNumberOfChildContainers returns. */
    for (index = 0; index <= maxcount; index++)
    {
        /* A buffer size of 1 is unlikely to be valid, as only a null terminator
         * could be stored, and it is unlikely that a container name could be empty. */
        DWORD buffersize = 1;
        memcpy(container, testW, sizeof(testW));
        hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, container, buffersize);
        if (hr == E_INVALIDARG)
        {
            /* We should get here when index is one more than the maximum index value. */
            ok(maxcount == index,
               "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG "
               "on the last index %d, got 0x%08x\n", index, hr);
            ok(container[0] == '\0',
               "Expected the container buffer string to be empty, got %s\n", wine_dbgstr_w(container));
            break;
        }
        else if (hr == DXDIAG_E_INSUFFICIENT_BUFFER)
        {
            WCHAR temp[256];

            ok(container[0] == '\0',
               "Expected the container buffer string to be empty, got %s\n", wine_dbgstr_w(container));

            /* Get the container name to compare against. */
            hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, temp, sizeof(temp)/sizeof(WCHAR));
            ok(hr == S_OK,
               "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK, got 0x%08x\n", hr);

            /* Show that the DirectX SDK's stipulation that the buffer be at
             * least 256 characters long is a mere suggestion, and smaller sizes
             * can be acceptable also. IDxDiagContainer::EnumChildContainerNames
             * doesn't provide a way of getting the exact size required, so the
             * buffersize value will be iterated to at most 256 characters. */
            for (buffersize = 2; buffersize <= 256; buffersize++)
            {
                memcpy(container, testW, sizeof(testW));
                hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, container, buffersize);
                if (hr != DXDIAG_E_INSUFFICIENT_BUFFER)
                    break;

                ok(!memcmp(temp, container, sizeof(WCHAR)*(buffersize - 1)),
                   "Expected truncated container name string, got %s\n", wine_dbgstr_w(container));
            }

            ok(hr == S_OK,
               "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK, "
               "got hr = 0x%08x, buffersize = %d\n", hr, buffersize);
            if (hr == S_OK)
                trace("pddc[%d] = %s, length = %d\n", index, wine_dbgstr_w(container), buffersize);
        }
        else
        {
            ok(0, "IDxDiagContainer::EnumChildContainerNames unexpectedly returned 0x%08x\n", hr);
            break;
        }
    }

cleanup:
    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_GetChildContainer(void)
{
    HRESULT hr;
    WCHAR container[256] = {0};
    IDxDiagContainer *child;

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    /* Test various combinations of invalid parameters. */
    hr = IDxDiagContainer_GetChildContainer(pddc, NULL, NULL);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::GetChildContainer to return E_INVALIDARG, got 0x%08x\n", hr);

    child = (void*)0xdeadbeef;
    hr = IDxDiagContainer_GetChildContainer(pddc, NULL, &child);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::GetChildContainer to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(child == (void*)0xdeadbeef, "Expected output pointer to be unchanged, got %p\n", child);

    hr = IDxDiagContainer_GetChildContainer(pddc, container, NULL);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::GetChildContainer to return E_INVALIDARG, got 0x%08x\n", hr);

    child = (void*)0xdeadbeef;
    hr = IDxDiagContainer_GetChildContainer(pddc, container, &child);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::GetChildContainer to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(child == NULL, "Expected output pointer to be NULL, got %p\n", child);

    /* Get the name of a suitable child container. */
    hr = IDxDiagContainer_EnumChildContainerNames(pddc, 0, container, sizeof(container)/sizeof(WCHAR));
    ok(hr == S_OK,
       "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK, got 0x%08x\n", hr);
    if (FAILED(hr))
    {
        skip("IDxDiagContainer::EnumChildContainerNames failed\n");
        goto cleanup;
    }

    child = (void*)0xdeadbeef;
    hr = IDxDiagContainer_GetChildContainer(pddc, container, &child);
    ok(hr == S_OK,
       "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);
    ok(child != NULL && child != (void*)0xdeadbeef, "Expected a valid output pointer, got %p\n", child);

    if (SUCCEEDED(hr))
    {
        IDxDiagContainer *ptr;

        /* Show that IDxDiagContainer::GetChildContainer returns a different pointer
         * for multiple calls for the same container name. */
        hr = IDxDiagContainer_GetChildContainer(pddc, container, &ptr);
        ok(hr == S_OK,
           "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);
        if (SUCCEEDED(hr))
            ok(ptr != child, "Expected the two pointers (%p vs. %p) to be unequal\n", child, ptr);

        IDxDiagContainer_Release(ptr);
        IDxDiagContainer_Release(child);
    }

cleanup:
    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_dot_parsing(void)
{
    HRESULT hr;
    WCHAR containerbufW[256] = {0}, childbufW[256] = {0};
    DWORD count, index;
    size_t i;
    static const struct
    {
        const char *format;
        const HRESULT expect;
    } test_strings[] = {
        { "%s.%s",   S_OK },
        { "%s.%s.",  S_OK },
        { ".%s.%s",  E_INVALIDARG },
        { "%s.%s..", E_INVALIDARG },
        { ".%s.%s.", E_INVALIDARG },
        { "..%s.%s", E_INVALIDARG },
    };

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    /* Find a container with a child container of its own. */
    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08x\n", hr);
    if (FAILED(hr))
    {
        skip("IDxDiagContainer::GetNumberOfChildContainers failed\n");
        goto cleanup;
    }

    for (index = 0; index < count; index++)
    {
        IDxDiagContainer *child;

        hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, containerbufW, sizeof(containerbufW)/sizeof(WCHAR));
        ok(hr == S_OK, "Expected IDxDiagContainer_EnumChildContainerNames to return S_OK, got 0x%08x\n", hr);
        if (FAILED(hr))
        {
            skip("IDxDiagContainer::EnumChildContainerNames failed\n");
            goto cleanup;
        }

        hr = IDxDiagContainer_GetChildContainer(pddc, containerbufW, &child);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IDxDiagContainer_EnumChildContainerNames(child, 0, childbufW, sizeof(childbufW)/sizeof(WCHAR));
            ok(hr == S_OK || hr == E_INVALIDARG,
               "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK or E_INVALIDARG, got 0x%08x\n", hr);
            IDxDiagContainer_Release(child);

            if (SUCCEEDED(hr))
                break;
        }
    }

    if (!*containerbufW || !*childbufW)
    {
        skip("Unable to find a suitable container\n");
        goto cleanup;
    }

    trace("Testing IDxDiagContainer::GetChildContainer dot parsing with container %s and child container %s.\n",
          wine_dbgstr_w(containerbufW), wine_dbgstr_w(childbufW));

    for (i = 0; i < sizeof(test_strings)/sizeof(test_strings[0]); i++)
    {
        IDxDiagContainer *child;
        char containerbufA[256];
        char childbufA[256];
        char dotbufferA[255 + 255 + 3 + 1];
        WCHAR dotbufferW[255 + 255 + 3 + 1]; /* containerbuf + childbuf + dots + null terminator */

        WideCharToMultiByte(CP_ACP, 0, containerbufW, -1, containerbufA, sizeof(containerbufA), NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, childbufW, -1, childbufA, sizeof(childbufA), NULL, NULL);
        sprintf(dotbufferA, test_strings[i].format, containerbufA, childbufA);
        MultiByteToWideChar(CP_ACP, 0, dotbufferA, -1, dotbufferW, sizeof(dotbufferW)/sizeof(WCHAR));

        trace("Trying container name %s\n", wine_dbgstr_w(dotbufferW));
        hr = IDxDiagContainer_GetChildContainer(pddc, dotbufferW, &child);
        ok(hr == test_strings[i].expect,
           "Expected IDxDiagContainer::GetChildContainer to return 0x%08x for %s, got 0x%08x\n",
           test_strings[i].expect, wine_dbgstr_w(dotbufferW), hr);
        if (SUCCEEDED(hr))
            IDxDiagContainer_Release(child);
    }

cleanup:
    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_EnumPropNames(void)
{
    HRESULT hr;
    WCHAR container[256], property[256];
    IDxDiagContainer *child = NULL;
    DWORD count, index, propcount;
    static const WCHAR testW[] = {'t','e','s','t',0};

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    /* Find a container with a non-zero number of properties. */
    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08x\n", hr);
    if (FAILED(hr))
    {
        skip("IDxDiagContainer::GetNumberOfChildContainers failed\n");
        goto cleanup;
    }

    for (index = 0; index < count; index++)
    {
        hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, container, sizeof(container)/sizeof(WCHAR));
        ok(hr == S_OK, "Expected IDxDiagContainer_EnumChildContainerNames to return S_OK, got 0x%08x\n", hr);
        if (FAILED(hr))
        {
            skip("IDxDiagContainer::EnumChildContainerNames failed\n");
            goto cleanup;
        }

        hr = IDxDiagContainer_GetChildContainer(pddc, container, &child);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IDxDiagContainer_GetNumberOfProps(child, &propcount);
            ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfProps to return S_OK, got 0x%08x\n", hr);

            if (!propcount)
            {
                IDxDiagContainer_Release(child);
                child = NULL;
            }
            else
                break;
        }
    }

    if (!child)
    {
        skip("Unable to find a container with non-zero property count\n");
        goto cleanup;
    }

    hr = IDxDiagContainer_EnumPropNames(child, ~0, NULL, 0);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::EnumPropNames to return E_INVALIDARG, got 0x%08x\n", hr);

    memcpy(property, testW, sizeof(testW));
    hr = IDxDiagContainer_EnumPropNames(child, ~0, property, 0);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::EnumPropNames to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(!memcmp(property, testW, sizeof(testW)),
       "Expected the property buffer to be unchanged, got %s\n", wine_dbgstr_w(property));

    memcpy(property, testW, sizeof(testW));
    hr = IDxDiagContainer_EnumPropNames(child, ~0, property, sizeof(property)/sizeof(WCHAR));
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::EnumPropNames to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(!memcmp(property, testW, sizeof(testW)),
       "Expected the property buffer to be unchanged, got %s\n", wine_dbgstr_w(property));

    trace("Starting property enumeration of the %s container:\n", wine_dbgstr_w(container));

    /* We should be able to enumerate as many properties as the value that
     * IDxDiagContainer::GetNumberOfProps returns. */
    for (index = 0; index <= propcount; index++)
    {
        /* A buffer size of 1 is unlikely to be valid, as only a null terminator
         * could be stored, and it is unlikely that a property name could be empty. */
        DWORD buffersize = 1;

        memcpy(property, testW, sizeof(testW));
        hr = IDxDiagContainer_EnumPropNames(child, index, property, buffersize);
        if (hr == E_INVALIDARG)
        {
            /* We should get here when index is one more than the maximum index value. */
            ok(propcount == index,
               "Expected IDxDiagContainer::EnumPropNames to return E_INVALIDARG "
               "on the last index %d, got 0x%08x\n", index, hr);
            ok(!memcmp(property, testW, sizeof(testW)),
               "Expected the property buffer to be unchanged, got %s\n", wine_dbgstr_w(property));
            break;
        }
        else if (hr == DXDIAG_E_INSUFFICIENT_BUFFER)
        {
            WCHAR temp[256];

            ok(property[0] == '\0',
               "Expected the property buffer string to be empty, got %s\n", wine_dbgstr_w(property));
            hr = IDxDiagContainer_EnumPropNames(child, index, temp, sizeof(temp)/sizeof(WCHAR));
            ok(hr == S_OK,
               "Expected IDxDiagContainer::EnumPropNames to return S_OK, got 0x%08x\n", hr);

            /* Show that the DirectX SDK's stipulation that the buffer be at
             * least 256 characters long is a mere suggestion, and smaller sizes
             * can be acceptable also. IDxDiagContainer::EnumPropNames doesn't
             * provide a way of getting the exact size required, so the buffersize
             * value will be iterated to at most 256 characters. */
            for (buffersize = 2; buffersize <= 256; buffersize++)
            {
                memcpy(property, testW, sizeof(testW));
                hr = IDxDiagContainer_EnumPropNames(child, index, property, buffersize);
                if (hr != DXDIAG_E_INSUFFICIENT_BUFFER)
                    break;

                ok(!memcmp(temp, property, sizeof(WCHAR)*(buffersize - 1)),
                   "Expected truncated property name string, got %s\n", wine_dbgstr_w(property));
            }

            ok(hr == S_OK,
               "Expected IDxDiagContainer::EnumPropNames to return S_OK, "
               "got hr = 0x%08x, buffersize = %d\n", hr, buffersize);
            if (hr == S_OK)
                trace("child[%d] = %s, length = %d\n", index, wine_dbgstr_w(property), buffersize);
        }
        else
        {
            ok(0, "IDxDiagContainer::EnumPropNames unexpectedly returned 0x%08x\n", hr);
            break;
        }
    }

    IDxDiagContainer_Release(child);

cleanup:
    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_GetProp(void)
{
    HRESULT hr;
    WCHAR container[256], property[256];
    IDxDiagContainer *child = NULL;
    DWORD count, index;
    VARIANT var;
    SAFEARRAY *sa;
    SAFEARRAYBOUND bound;
    ULONG ref;
    static const WCHAR emptyW[] = {0};
    static const WCHAR testW[] = {'t','e','s','t',0};

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    /* Find a container with a property. */
    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08x\n", hr);
    if (FAILED(hr))
    {
        skip("IDxDiagContainer::GetNumberOfChildContainers failed\n");
        goto cleanup;
    }

    for (index = 0; index < count; index++)
    {
        hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, container, sizeof(container)/sizeof(WCHAR));
        ok(hr == S_OK, "Expected IDxDiagContainer_EnumChildContainerNames to return S_OK, got 0x%08x\n", hr);
        if (FAILED(hr))
        {
            skip("IDxDiagContainer::EnumChildContainerNames failed\n");
            goto cleanup;
        }

        hr = IDxDiagContainer_GetChildContainer(pddc, container, &child);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IDxDiagContainer_EnumPropNames(child, 0, property, sizeof(property)/sizeof(WCHAR));
            ok(hr == S_OK || hr == E_INVALIDARG,
               "Expected IDxDiagContainer::EnumPropNames to return S_OK or E_INVALIDARG, got 0x%08x\n", hr);

            if (SUCCEEDED(hr))
                break;
            else
            {
                IDxDiagContainer_Release(child);
                child = NULL;
            }
        }
    }

    if (!child)
    {
        skip("Unable to find a suitable container\n");
        goto cleanup;
    }

    hr = IDxDiagContainer_GetProp(child, NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetProp to return E_INVALIDARG, got 0x%08x\n", hr);

    V_VT(&var) = 0xdead;
    hr = IDxDiagContainer_GetProp(child, NULL, &var);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetProp to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(V_VT(&var) == 0xdead, "Expected the variant to be untouched, got %u\n", V_VT(&var));

    hr = IDxDiagContainer_GetProp(child, emptyW, NULL);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetProp to return E_INVALIDARG, got 0x%08x\n", hr);

    V_VT(&var) = 0xdead;
    hr = IDxDiagContainer_GetProp(child, emptyW, &var);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetProp to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(V_VT(&var) == 0xdead, "Expected the variant to be untouched, got %u\n", V_VT(&var));

    hr = IDxDiagContainer_GetProp(child, testW, NULL);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetProp to return E_INVALIDARG, got 0x%08x\n", hr);

    V_VT(&var) = 0xdead;
    hr = IDxDiagContainer_GetProp(child, testW, &var);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetProp to return E_INVALIDARG, got 0x%08x\n", hr);
    ok(V_VT(&var) == 0xdead, "Expected the variant to be untouched, got %u\n", V_VT(&var));

    VariantInit(&var);
    hr = IDxDiagContainer_GetProp(child, property, &var);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetProp to return S_OK, got 0x%08x\n", hr);
    ok(V_VT(&var) != VT_EMPTY, "Expected the variant to be modified, got %d\n", V_VT(&var));

    /* Since the documentation for IDxDiagContainer::GetProp claims that the
     * function reports return values from VariantCopy, try to exercise failure
     * paths in handling the destination variant. */

    /* Try an invalid variant type. */
    V_VT(&var) = 0xdead;
    hr = IDxDiagContainer_GetProp(child, property, &var);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetProp to return S_OK, got 0x%08x\n", hr);
    ok(V_VT(&var) != 0xdead, "Expected the variant to be modified, got %d\n", V_VT(&var));

    /* Try passing a variant with a locked SAFEARRAY. */
    bound.cElements = 1;
    bound.lLbound = 0;
    sa = SafeArrayCreate(VT_UI1, 1, &bound);
    ok(sa != NULL, "Expected SafeArrayCreate to return a valid pointer\n");

    V_VT(&var) = (VT_ARRAY | VT_UI1);
    V_ARRAY(&var) = sa;

    hr = SafeArrayLock(sa);
    ok(hr == S_OK, "Expected SafeArrayLock to return S_OK, got 0x%08x\n", hr);

    hr = IDxDiagContainer_GetProp(child, property, &var);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetProp to return S_OK, got 0x%08x\n", hr);
    ok(V_VT(&var) != (VT_ARRAY | VT_UI1), "Expected the variant to be modified\n");

    hr = SafeArrayUnlock(sa);
    ok(hr == S_OK, "Expected SafeArrayUnlock to return S_OK, got 0x%08x\n", hr);
    hr = SafeArrayDestroy(sa);
    ok(hr == S_OK, "Expected SafeArrayDestroy to return S_OK, got 0x%08x\n", hr);

    /* Determine whether GetProp calls VariantClear on the passed variant. */
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = (IUnknown *)child;
    IDxDiagContainer_AddRef(child);

    hr = IDxDiagContainer_GetProp(child, property, &var);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetProp to return S_OK, got 0x%08x\n", hr);
    ok(V_VT(&var) != VT_UNKNOWN, "Expected the variant to be modified\n");

    IDxDiagContainer_AddRef(child);
    ref = IDxDiagContainer_Release(child);
    ok(ref == 2, "Expected reference count to be 2, got %u\n", ref);
    IDxDiagContainer_Release(child);

    IDxDiagContainer_Release(child);
cleanup:
    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_root_children(void)
{
    static const WCHAR DxDiag_DirectSound[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','S','o','u','n','d',0};
    static const WCHAR DxDiag_DirectMusic[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','M','u','s','i','c',0};
    static const WCHAR DxDiag_DirectInput[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','I','n','p','u','t',0};
    static const WCHAR DxDiag_DirectPlay[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','P','l','a','y',0};
    static const WCHAR DxDiag_SystemDevices[] = {'D','x','D','i','a','g','_','S','y','s','t','e','m','D','e','v','i','c','e','s',0};
    static const WCHAR DxDiag_DirectXFiles[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','X','F','i','l','e','s',0};
    static const WCHAR DxDiag_DirectShowFilters[] = {'D','x','D','i','a','g','_','D','i','r','e','c','t','S','h','o','w','F','i','l','t','e','r','s',0};
    static const WCHAR DxDiag_LogicalDisks[] = {'D','x','D','i','a','g','_','L','o','g','i','c','a','l','D','i','s','k','s',0};

    HRESULT hr;
    DWORD count, index;

    static const WCHAR *root_children[] = {
        DxDiag_SystemInfo, DxDiag_DisplayDevices, DxDiag_DirectSound,
        DxDiag_DirectMusic, DxDiag_DirectInput, DxDiag_DirectPlay,
        DxDiag_SystemDevices, DxDiag_DirectXFiles, DxDiag_DirectShowFilters,
        DxDiag_LogicalDisks
    };

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    /* Verify the identity and ordering of the root container's children. */
    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08x\n", hr);
    if (FAILED(hr))
    {
        skip("IDxDiagContainer::GetNumberOfChildContainers failed\n");
        goto cleanup;
    }

    ok(count == sizeof(root_children)/sizeof(root_children[0]),
       "Got unexpected count %u for the number of child containers\n", count);

    if (count != sizeof(root_children)/sizeof(root_children[0]))
    {
        skip("Received unexpected number of child containers\n");
        goto cleanup;
    }

    for (index = 0; index <= count; index++)
    {
        WCHAR container[256];

        hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, container, sizeof(container)/sizeof(WCHAR));
        if (hr == E_INVALIDARG)
        {
            ok(index == count,
               "Expected IDxDiagContainer::EnumChildContainerNames to return "
               "E_INVALIDARG on the last index %u\n", count);
            break;
        }
        else if (hr == S_OK)
        {
            ok(!lstrcmpW(container, root_children[index]),
               "Expected container %s for index %u, got %s\n",
               wine_dbgstr_w(root_children[index]), index, wine_dbgstr_w(container));
        }
        else
        {
            ok(0, "IDxDiagContainer::EnumChildContainerNames unexpectedly returned 0x%08x\n", hr);
            break;
        }
    }

cleanup:
    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_container_properties(IDxDiagContainer *container, const struct property_test *property_tests, size_t len)
{
    HRESULT hr;

    /* Check that the container has no properties if there are no properties to examine. */
    if (len == 0)
    {
        DWORD prop_count;

        hr = IDxDiagContainer_GetNumberOfProps(container, &prop_count);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfProps to return S_OK, got 0x%08x\n", hr);
        if (hr == S_OK)
            ok(prop_count == 0, "Expected container property count to be zero, got %u\n", prop_count);
    }
    else
    {
        VARIANT var;
        int i;

        VariantInit(&var);

        /* Examine the variant types of obtained property values. */
        for (i = 0; i < len; i++)
        {
            hr = IDxDiagContainer_GetProp(container, property_tests[i].prop, &var);
            ok(hr == S_OK, "[%d] Expected IDxDiagContainer::GetProp to return S_OK for %s, got 0x%08x\n",
               i, wine_dbgstr_w(property_tests[i].prop), hr);

            if (hr == S_OK)
            {
                ok(V_VT(&var) == property_tests[i].vt,
                   "[%d] Expected variant type %d, got %d\n", i, property_tests[i].vt, V_VT(&var));
                trace("%s = %s\n", wine_dbgstr_w(property_tests[i].prop), debugstr_variant(&var));
                VariantClear(&var);
            }
        }
    }
}

static void test_DxDiag_SystemInfo(void)
{
    static const WCHAR dwOSMajorVersion[] = {'d','w','O','S','M','a','j','o','r','V','e','r','s','i','o','n',0};
    static const WCHAR dwOSMinorVersion[] = {'d','w','O','S','M','i','n','o','r','V','e','r','s','i','o','n',0};
    static const WCHAR dwOSBuildNumber[] = {'d','w','O','S','B','u','i','l','d','N','u','m','b','e','r',0};
    static const WCHAR dwOSPlatformID[] = {'d','w','O','S','P','l','a','t','f','o','r','m','I','D',0};
    static const WCHAR dwDirectXVersionMajor[] = {'d','w','D','i','r','e','c','t','X','V','e','r','s','i','o','n','M','a','j','o','r',0};
    static const WCHAR dwDirectXVersionMinor[] = {'d','w','D','i','r','e','c','t','X','V','e','r','s','i','o','n','M','i','n','o','r',0};
    static const WCHAR szDirectXVersionLetter[] = {'s','z','D','i','r','e','c','t','X','V','e','r','s','i','o','n','L','e','t','t','e','r',0};
    static const WCHAR bDebug[] = {'b','D','e','b','u','g',0};
    static const WCHAR bNECPC98[] = {'b','N','E','C','P','C','9','8',0};
    static const WCHAR ullPhysicalMemory[] = {'u','l','l','P','h','y','s','i','c','a','l','M','e','m','o','r','y',0};
    static const WCHAR ullUsedPageFile[] = {'u','l','l','U','s','e','d','P','a','g','e','F','i','l','e',0};
    static const WCHAR ullAvailPageFile[] = {'u','l','l','A','v','a','i','l','P','a','g','e','F','i','l','e',0};
    static const WCHAR szWindowsDir[] = {'s','z','W','i','n','d','o','w','s','D','i','r',0};
    static const WCHAR szCSDVersion[] = {'s','z','C','S','D','V','e','r','s','i','o','n',0};
    static const WCHAR szDirectXVersionEnglish[] = {'s','z','D','i','r','e','c','t','X','V','e','r','s','i','o','n','E','n','g','l','i','s','h',0};
    static const WCHAR szDirectXVersionLongEnglish[] = {'s','z','D','i','r','e','c','t','X','V','e','r','s','i','o','n','L','o','n','g','E','n','g','l','i','s','h',0};
    static const WCHAR bNetMeetingRunning[] = {'b','N','e','t','M','e','e','t','i','n','g','R','u','n','n','i','n','g',0};
    static const WCHAR szMachineNameLocalized[] = {'s','z','M','a','c','h','i','n','e','N','a','m','e','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szMachineNameEnglish[] = {'s','z','M','a','c','h','i','n','e','N','a','m','e','E','n','g','l','i','s','h',0};
    static const WCHAR szLanguagesLocalized[] = {'s','z','L','a','n','g','u','a','g','e','s','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szLanguagesEnglish[] = {'s','z','L','a','n','g','u','a','g','e','s','E','n','g','l','i','s','h',0};
    static const WCHAR szTimeLocalized[] = {'s','z','T','i','m','e','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szTimeEnglish[] = {'s','z','T','i','m','e','E','n','g','l','i','s','h',0};
    static const WCHAR szPhysicalMemoryEnglish[] = {'s','z','P','h','y','s','i','c','a','l','M','e','m','o','r','y','E','n','g','l','i','s','h',0};
    static const WCHAR szPageFileLocalized[] = {'s','z','P','a','g','e','F','i','l','e','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szPageFileEnglish[] = {'s','z','P','a','g','e','F','i','l','e','E','n','g','l','i','s','h',0};
    static const WCHAR szOSLocalized[] = {'s','z','O','S','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szOSExLocalized[] = {'s','z','O','S','E','x','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szOSExLongLocalized[] = {'s','z','O','S','E','x','L','o','n','g','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szOSEnglish[] = {'s','z','O','S','E','n','g','l','i','s','h',0};
    static const WCHAR szOSExEnglish[] = {'s','z','O','S','E','x','E','n','g','l','i','s','h',0};
    static const WCHAR szOSExLongEnglish[] = {'s','z','O','S','E','x','L','o','n','g','E','n','g','l','i','s','h',0};
    static const WCHAR szProcessorEnglish[] = {'s','z','P','r','o','c','e','s','s','o','r','E','n','g','l','i','s','h',0};

    static const struct property_test property_tests[] =
    {
        {dwOSMajorVersion, VT_UI4},
        {dwOSMinorVersion, VT_UI4},
        {dwOSBuildNumber, VT_UI4},
        {dwOSPlatformID, VT_UI4},
        {dwDirectXVersionMajor, VT_UI4},
        {dwDirectXVersionMinor, VT_UI4},
        {szDirectXVersionLetter, VT_BSTR},
        {bDebug, VT_BOOL},
        {bNECPC98, VT_BOOL},
        {ullPhysicalMemory, VT_BSTR},
        {ullUsedPageFile, VT_BSTR},
        {ullAvailPageFile, VT_BSTR},
        {szWindowsDir, VT_BSTR},
        {szCSDVersion, VT_BSTR},
        {szDirectXVersionEnglish, VT_BSTR},
        {szDirectXVersionLongEnglish, VT_BSTR},
        {bNetMeetingRunning, VT_BOOL},
        {szMachineNameLocalized, VT_BSTR},
        {szMachineNameEnglish, VT_BSTR},
        {szLanguagesLocalized, VT_BSTR},
        {szLanguagesEnglish, VT_BSTR},
        {szTimeLocalized, VT_BSTR},
        {szTimeEnglish, VT_BSTR},
        {szPhysicalMemoryEnglish, VT_BSTR},
        {szPageFileLocalized, VT_BSTR},
        {szPageFileEnglish, VT_BSTR},
        {szOSLocalized, VT_BSTR},
        {szOSExLocalized, VT_BSTR},
        {szOSExLongLocalized, VT_BSTR},
        {szOSEnglish, VT_BSTR},
        {szOSExEnglish, VT_BSTR},
        {szOSExLongEnglish, VT_BSTR},
        {szProcessorEnglish, VT_BSTR},
    };

    IDxDiagContainer *container, *container2;
    static const WCHAR empty[] = {0};
    HRESULT hr;

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    hr = IDxDiagContainer_GetChildContainer(pddc, empty, &container2);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetChildContainer to return E_INVALIDARG, got 0x%08x\n", hr);

    hr = IDxDiagContainer_GetChildContainer(pddc, DxDiag_SystemInfo, &container);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);

    if (hr == S_OK)
    {
        trace("Testing container DxDiag_SystemInfo\n");
        test_container_properties(container, property_tests, sizeof(property_tests)/sizeof(property_tests[0]));

        container2 = NULL;
        hr = IDxDiagContainer_GetChildContainer(container, empty, &container2);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);
        ok(container2 != NULL, "Expected container2 != NULL\n");
        ok(container2 != container, "Expected container != container2\n");
        if (hr == S_OK) IDxDiagContainer_Release(container2);

        IDxDiagContainer_Release(container);
    }

    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_DxDiag_DisplayDevices(void)
{
    static const WCHAR szDescription[] = {'s','z','D','e','s','c','r','i','p','t','i','o','n',0};
    static const WCHAR szDeviceName[] = {'s','z','D','e','v','i','c','e','N','a','m','e',0};
    static const WCHAR szKeyDeviceID[] = {'s','z','K','e','y','D','e','v','i','c','e','I','D',0};
    static const WCHAR szKeyDeviceKey[] = {'s','z','K','e','y','D','e','v','i','c','e','K','e','y',0};
    static const WCHAR szVendorId[] = {'s','z','V','e','n','d','o','r','I','d',0};
    static const WCHAR szDeviceId[] = {'s','z','D','e','v','i','c','e','I','d',0};
    static const WCHAR szDeviceIdentifier[] = {'s','z','D','e','v','i','c','e','I','d','e','n','t','i','f','i','e','r',0};
    static const WCHAR dwWidth[] = {'d','w','W','i','d','t','h',0};
    static const WCHAR dwHeight[] = {'d','w','H','e','i','g','h','t',0};
    static const WCHAR dwBpp[] = {'d','w','B','p','p',0};
    static const WCHAR szDisplayMemoryLocalized[] = {'s','z','D','i','s','p','l','a','y','M','e','m','o','r','y','L','o','c','a','l','i','z','e','d',0};
    static const WCHAR szDisplayMemoryEnglish[] = {'s','z','D','i','s','p','l','a','y','M','e','m','o','r','y','E','n','g','l','i','s','h',0};
    static const WCHAR szDriverName[] = {'s','z','D','r','i','v','e','r','N','a','m','e',0};
    static const WCHAR szDriverVersion[] = {'s','z','D','r','i','v','e','r','V','e','r','s','i','o','n',0};
    static const WCHAR szSubSysId[] = {'s','z','S','u','b','S','y','s','I','d',0};
    static const WCHAR szRevisionId[] = {'s','z','R','e','v','i','s','i','o','n','I','d',0};
    static const WCHAR dwRefreshRate[] = {'d','w','R','e','f','r','e','s','h','R','a','t','e',0};
    static const WCHAR szManufacturer[] = {'s','z','M','a','n','u','f','a','c','t','u','r','e','r',0};
    static const WCHAR b3DAccelerationExists[] = {'b','3','D','A','c','c','e','l','e','r','a','t','i','o','n','E','x','i','s','t','s',0};
    static const WCHAR b3DAccelerationEnabled[] = {'b','3','D','A','c','c','e','l','e','r','a','t','i','o','n','E','n','a','b','l','e','d',0};
    static const WCHAR bDDAccelerationEnabled[] = {'b','D','D','A','c','c','e','l','e','r','a','t','i','o','n','E','n','a','b','l','e','d',0};

    static const struct property_test property_tests[] =
    {
        {szDescription, VT_BSTR},
        {szDeviceName, VT_BSTR},
        {szKeyDeviceID, VT_BSTR},
        {szKeyDeviceKey, VT_BSTR},
        {szVendorId, VT_BSTR},
        {szDeviceId, VT_BSTR},
        {szDeviceIdentifier, VT_BSTR},
        {dwWidth, VT_UI4},
        {dwHeight, VT_UI4},
        {dwBpp, VT_UI4},
        {szDisplayMemoryLocalized, VT_BSTR},
        {szDisplayMemoryEnglish, VT_BSTR},
        {szDriverName, VT_BSTR},
        {szDriverVersion, VT_BSTR},
        {szSubSysId, VT_BSTR},
        {szRevisionId, VT_BSTR},
        {dwRefreshRate, VT_UI4},
        {szManufacturer, VT_BSTR},
        {b3DAccelerationExists, VT_BOOL},
        {b3DAccelerationEnabled, VT_BOOL},
        {bDDAccelerationEnabled, VT_BOOL},
    };

    IDxDiagContainer *display_cont = NULL;
    DWORD count, i;
    HRESULT hr;

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    hr = IDxDiagContainer_GetChildContainer(pddc, DxDiag_DisplayDevices, &display_cont);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);

    if (hr != S_OK)
        goto cleanup;

    hr = IDxDiagContainer_GetNumberOfProps(display_cont, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfProps to return S_OK, got 0x%08x\n", hr);
    if (hr == S_OK)
        ok(count == 0, "Expected count to be 0, got %u\n", count);

    hr = IDxDiagContainer_GetNumberOfChildContainers(display_cont, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08x\n", hr);

    if (hr != S_OK)
        goto cleanup;

    for (i = 0; i < count; i++)
    {
        WCHAR child_container[256];
        IDxDiagContainer *child;

        hr = IDxDiagContainer_EnumChildContainerNames(display_cont, i, child_container, sizeof(child_container)/sizeof(WCHAR));
        ok(hr == S_OK, "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK, got 0x%08x\n", hr);

        hr = IDxDiagContainer_GetChildContainer(display_cont, child_container, &child);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);

        if (hr == S_OK)
        {
            trace("Testing container %s\n", wine_dbgstr_w(child_container));
            test_container_properties(child, property_tests, sizeof(property_tests)/sizeof(property_tests[0]));
        }
        IDxDiagContainer_Release(child);
    }

cleanup:
    if (display_cont) IDxDiagContainer_Release(display_cont);
    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_DxDiag_SoundDevices(void)
{
    static const WCHAR szDescription[] = {'s','z','D','e','s','c','r','i','p','t','i','o','n',0};
    static const WCHAR szGuidDeviceID[] = {'s','z','G','u','i','d','D','e','v','i','c','e','I','D',0};
    static const WCHAR szDriverPath[] = {'s','z','D','r','i','v','e','r','P','a','t','h',0};
    static const WCHAR szDriverName[] = {'s','z','D','r','i','v','e','r','N','a','m','e',0};
    static const WCHAR empty[] = {0};

    static const struct property_test property_tests[] =
    {
        {szDescription, VT_BSTR},
        {szGuidDeviceID, VT_BSTR},
        {szDriverName, VT_BSTR},
        {szDriverPath, VT_BSTR},
    };

    IDxDiagContainer *sound_cont = NULL;
    DWORD count, i;
    HRESULT hr;

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    hr = IDxDiagContainer_GetChildContainer(pddc, DxDiag_SoundDevices, &sound_cont);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);

    if (hr != S_OK)
        goto cleanup;

    hr = IDxDiagContainer_GetNumberOfProps(sound_cont, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfProps to return S_OK, got 0x%08x\n", hr);
    if (hr == S_OK)
        ok(count == 0, "Expected count to be 0, got %u\n", count);

    hr = IDxDiagContainer_GetNumberOfChildContainers(sound_cont, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08x\n", hr);

    if (hr != S_OK)
        goto cleanup;

    for (i = 0; i < count; i++)
    {
        WCHAR child_container[256];
        IDxDiagContainer *child, *child2;

        hr = IDxDiagContainer_EnumChildContainerNames(sound_cont, i, child_container, sizeof(child_container)/sizeof(WCHAR));
        ok(hr == S_OK, "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK, got 0x%08x\n", hr);

        hr = IDxDiagContainer_GetChildContainer(sound_cont, child_container, &child);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);

        if (hr == S_OK)
        {
            trace("Testing container %s\n", wine_dbgstr_w(child_container));
            test_container_properties(child, property_tests, sizeof(property_tests)/sizeof(property_tests[0]));
        }

        child2 = NULL;
        hr = IDxDiagContainer_GetChildContainer(child, empty, &child2);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);
        ok(child2 != NULL, "Expected child2 != NULL\n");
        ok(child2 != child, "Expected child != child2\n");
        if (hr == S_OK) IDxDiagContainer_Release(child2);

        IDxDiagContainer_Release(child);
    }

cleanup:
    if (sound_cont) IDxDiagContainer_Release(sound_cont);
    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_DxDiag_SoundCaptureDevices(void)
{
    static const WCHAR szDescription[] = {'s','z','D','e','s','c','r','i','p','t','i','o','n',0};
    static const WCHAR szGuidDeviceID[] = {'s','z','G','u','i','d','D','e','v','i','c','e','I','D',0};
    static const WCHAR szDriverPath[] = {'s','z','D','r','i','v','e','r','P','a','t','h',0};
    static const WCHAR szDriverName[] = {'s','z','D','r','i','v','e','r','N','a','m','e',0};

    static const struct property_test property_tests[] =
    {
        {szDescription, VT_BSTR},
        {szGuidDeviceID, VT_BSTR},
        {szDriverName, VT_BSTR},
        {szDriverPath, VT_BSTR},
    };

    IDxDiagContainer *sound_cont = NULL;
    DWORD count, i;
    HRESULT hr;

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    hr = IDxDiagContainer_GetChildContainer(pddc, DxDiag_SoundCaptureDevices, &sound_cont);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);

    if (hr != S_OK)
        goto cleanup;

    hr = IDxDiagContainer_GetNumberOfProps(sound_cont, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfProps to return S_OK, got 0x%08x\n", hr);
    if (hr == S_OK)
        ok(count == 0, "Expected count to be 0, got %u\n", count);

    hr = IDxDiagContainer_GetNumberOfChildContainers(sound_cont, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08x\n", hr);

    if (hr != S_OK)
        goto cleanup;

    for (i = 0; i < count; i++)
    {
        WCHAR child_container[256];
        IDxDiagContainer *child;

        hr = IDxDiagContainer_EnumChildContainerNames(sound_cont, i, child_container, sizeof(child_container)/sizeof(WCHAR));
        ok(hr == S_OK, "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK, got 0x%08x\n", hr);

        hr = IDxDiagContainer_GetChildContainer(sound_cont, child_container, &child);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08x\n", hr);

        if (hr == S_OK)
        {
            trace("Testing container %s\n", wine_dbgstr_w(child_container));
            test_container_properties(child, property_tests, sizeof(property_tests)/sizeof(property_tests[0]));
        }
        IDxDiagContainer_Release(child);
    }

cleanup:
    if (sound_cont) IDxDiagContainer_Release(sound_cont);
    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

START_TEST(container)
{
    CoInitialize(NULL);
    test_GetNumberOfChildContainers();
    test_GetNumberOfProps();
    test_EnumChildContainerNames();
    test_GetChildContainer();
    test_dot_parsing();
    test_EnumPropNames();
    test_GetProp();

    test_root_children();
    test_DxDiag_SystemInfo();
    test_DxDiag_DisplayDevices();
    test_DxDiag_SoundDevices();
    test_DxDiag_SoundCaptureDevices();
    CoUninitialize();
}
