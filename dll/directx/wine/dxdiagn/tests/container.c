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
#include "dxdiag.h"
#include "oleauto.h"
#include "wine/test.h"

struct property_test
{
    const WCHAR *prop;
    VARTYPE vt;
};

static IDxDiagProvider *pddp;
static IDxDiagContainer *pddc;

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
       "Expected IDxDiagContainer::GetNumberOfChildContainers to return E_INVALIDARG, got 0x%08lx\n", hr);

    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, &count);
    ok(hr == S_OK,
       "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08lx\n", hr);
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
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetNumberOfProps to return E_INVALIDARG, got 0x%08lx\n", hr);

    hr = IDxDiagContainer_GetNumberOfProps(pddc, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfProps to return S_OK, got 0x%08lx\n", hr);
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
    static const WCHAR testW[] = L"test";

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    /* Test various combinations of invalid parameters. */
    hr = IDxDiagContainer_EnumChildContainerNames(pddc, 0, NULL, 0);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG, got 0x%08lx\n", hr);

    hr = IDxDiagContainer_EnumChildContainerNames(pddc, 0, NULL, ARRAY_SIZE(container));
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG, got 0x%08lx\n", hr);

    /* Test the conditions in which the output buffer can be modified. */
    memcpy(container, testW, sizeof(testW));
    hr = IDxDiagContainer_EnumChildContainerNames(pddc, 0, container, 0);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG, got 0x%08lx\n", hr);
    ok(!memcmp(container, testW, sizeof(testW)),
       "Expected the container buffer to be untouched, got %s\n", wine_dbgstr_w(container));

    memcpy(container, testW, sizeof(testW));
    hr = IDxDiagContainer_EnumChildContainerNames(pddc, ~0, container, 0);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG, got 0x%08lx\n", hr);
    ok(!memcmp(container, testW, sizeof(testW)),
       "Expected the container buffer to be untouched, got %s\n", wine_dbgstr_w(container));

    memcpy(container, testW, sizeof(testW));
    hr = IDxDiagContainer_EnumChildContainerNames(pddc, ~0, container, ARRAY_SIZE(container));
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::EnumChildContainerNames to return E_INVALIDARG, got 0x%08lx\n", hr);
    ok(!memcmp(container, L"\0est", sizeof(L"\0est")),
       "Expected the container buffer string to be empty, got %s\n", wine_dbgstr_w(container));

    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, &maxcount);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08lx\n", hr);
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
               "on the last index %ld, got 0x%08lx\n", index, hr);
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
            hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, temp, ARRAY_SIZE(temp));
            ok(hr == S_OK,
               "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK, got 0x%08lx\n", hr);

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
               "got hr = 0x%08lx, buffersize = %ld\n", hr, buffersize);
            if (hr == S_OK)
                trace("pddc[%ld] = %s, length = %ld\n", index, wine_dbgstr_w(container), buffersize);
        }
        else
        {
            ok(0, "IDxDiagContainer::EnumChildContainerNames unexpectedly returned 0x%08lx\n", hr);
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
       "Expected IDxDiagContainer::GetChildContainer to return E_INVALIDARG, got 0x%08lx\n", hr);

    child = (void*)0xdeadbeef;
    hr = IDxDiagContainer_GetChildContainer(pddc, NULL, &child);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::GetChildContainer to return E_INVALIDARG, got 0x%08lx\n", hr);
    ok(child == (void*)0xdeadbeef, "Expected output pointer to be unchanged, got %p\n", child);

    hr = IDxDiagContainer_GetChildContainer(pddc, container, NULL);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::GetChildContainer to return E_INVALIDARG, got 0x%08lx\n", hr);

    child = (void*)0xdeadbeef;
    hr = IDxDiagContainer_GetChildContainer(pddc, container, &child);
    ok(hr == E_INVALIDARG,
       "Expected IDxDiagContainer::GetChildContainer to return E_INVALIDARG, got 0x%08lx\n", hr);
    ok(child == NULL, "Expected output pointer to be NULL, got %p\n", child);

    /* Get the name of a suitable child container. */
    hr = IDxDiagContainer_EnumChildContainerNames(pddc, 0, container, ARRAY_SIZE(container));
    ok(hr == S_OK,
       "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK, got 0x%08lx\n", hr);
    if (FAILED(hr))
    {
        skip("IDxDiagContainer::EnumChildContainerNames failed\n");
        goto cleanup;
    }

    child = (void*)0xdeadbeef;
    hr = IDxDiagContainer_GetChildContainer(pddc, container, &child);
    ok(hr == S_OK,
       "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08lx\n", hr);
    ok(child != NULL && child != (void*)0xdeadbeef, "Expected a valid output pointer, got %p\n", child);

    if (SUCCEEDED(hr))
    {
        IDxDiagContainer *ptr;

        /* Show that IDxDiagContainer::GetChildContainer returns a different pointer
         * for multiple calls for the same container name. */
        hr = IDxDiagContainer_GetChildContainer(pddc, container, &ptr);
        ok(hr == S_OK,
           "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08lx\n", hr);
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
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08lx\n", hr);
    if (FAILED(hr))
    {
        skip("IDxDiagContainer::GetNumberOfChildContainers failed\n");
        goto cleanup;
    }

    for (index = 0; index < count; index++)
    {
        IDxDiagContainer *child;

        hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, containerbufW, ARRAY_SIZE(containerbufW));
        ok(hr == S_OK, "Expected IDxDiagContainer_EnumChildContainerNames to return S_OK, got 0x%08lx\n", hr);
        if (FAILED(hr))
        {
            skip("IDxDiagContainer::EnumChildContainerNames failed\n");
            goto cleanup;
        }

        hr = IDxDiagContainer_GetChildContainer(pddc, containerbufW, &child);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08lx\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IDxDiagContainer_EnumChildContainerNames(child, 0, childbufW, ARRAY_SIZE(childbufW));
            ok(hr == S_OK || hr == E_INVALIDARG,
               "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK or E_INVALIDARG, got 0x%08lx\n", hr);
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

    for (i = 0; i < ARRAY_SIZE(test_strings); i++)
    {
        IDxDiagContainer *child;
        char containerbufA[256];
        char childbufA[256];
        char dotbufferA[255 + 255 + 3 + 1];
        WCHAR dotbufferW[255 + 255 + 3 + 1]; /* containerbuf + childbuf + dots + null terminator */

        WideCharToMultiByte(CP_ACP, 0, containerbufW, -1, containerbufA, sizeof(containerbufA), NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, childbufW, -1, childbufA, sizeof(childbufA), NULL, NULL);
        sprintf(dotbufferA, test_strings[i].format, containerbufA, childbufA);
        MultiByteToWideChar(CP_ACP, 0, dotbufferA, -1, dotbufferW, ARRAY_SIZE(dotbufferW));

        trace("Trying container name %s\n", wine_dbgstr_w(dotbufferW));
        hr = IDxDiagContainer_GetChildContainer(pddc, dotbufferW, &child);
        ok(hr == test_strings[i].expect,
           "Expected IDxDiagContainer::GetChildContainer to return 0x%08lx for %s, got 0x%08lx\n",
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
    static const WCHAR testW[] = L"test";

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    /* Find a container with a non-zero number of properties. */
    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08lx\n", hr);
    if (FAILED(hr))
    {
        skip("IDxDiagContainer::GetNumberOfChildContainers failed\n");
        goto cleanup;
    }

    for (index = 0; index < count; index++)
    {
        hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, container, ARRAY_SIZE(container));
        ok(hr == S_OK, "Expected IDxDiagContainer_EnumChildContainerNames to return S_OK, got 0x%08lx\n", hr);
        if (FAILED(hr))
        {
            skip("IDxDiagContainer::EnumChildContainerNames failed\n");
            goto cleanup;
        }

        hr = IDxDiagContainer_GetChildContainer(pddc, container, &child);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08lx\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IDxDiagContainer_GetNumberOfProps(child, &propcount);
            ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfProps to return S_OK, got 0x%08lx\n", hr);

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
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::EnumPropNames to return E_INVALIDARG, got 0x%08lx\n", hr);

    memcpy(property, testW, sizeof(testW));
    hr = IDxDiagContainer_EnumPropNames(child, ~0, property, 0);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::EnumPropNames to return E_INVALIDARG, got 0x%08lx\n", hr);
    ok(!memcmp(property, testW, sizeof(testW)),
       "Expected the property buffer to be unchanged, got %s\n", wine_dbgstr_w(property));

    memcpy(property, testW, sizeof(testW));
    hr = IDxDiagContainer_EnumPropNames(child, ~0, property, ARRAY_SIZE(property));
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::EnumPropNames to return E_INVALIDARG, got 0x%08lx\n", hr);
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
               "on the last index %ld, got 0x%08lx\n", index, hr);
            ok(!memcmp(property, testW, sizeof(testW)),
               "Expected the property buffer to be unchanged, got %s\n", wine_dbgstr_w(property));
            break;
        }
        else if (hr == DXDIAG_E_INSUFFICIENT_BUFFER)
        {
            WCHAR temp[256];

            ok(property[0] == '\0',
               "Expected the property buffer string to be empty, got %s\n", wine_dbgstr_w(property));
            hr = IDxDiagContainer_EnumPropNames(child, index, temp, ARRAY_SIZE(temp));
            ok(hr == S_OK,
               "Expected IDxDiagContainer::EnumPropNames to return S_OK, got 0x%08lx\n", hr);

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
               "got hr = 0x%08lx, buffersize = %ld\n", hr, buffersize);
            if (hr == S_OK)
                trace("child[%ld] = %s, length = %ld\n", index, wine_dbgstr_w(property), buffersize);
        }
        else
        {
            ok(0, "IDxDiagContainer::EnumPropNames unexpectedly returned 0x%08lx\n", hr);
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
    static const WCHAR testW[] = L"test";

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    /* Find a container with a property. */
    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08lx\n", hr);
    if (FAILED(hr))
    {
        skip("IDxDiagContainer::GetNumberOfChildContainers failed\n");
        goto cleanup;
    }

    for (index = 0; index < count; index++)
    {
        hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, container, ARRAY_SIZE(container));
        ok(hr == S_OK, "Expected IDxDiagContainer_EnumChildContainerNames to return S_OK, got 0x%08lx\n", hr);
        if (FAILED(hr))
        {
            skip("IDxDiagContainer::EnumChildContainerNames failed\n");
            goto cleanup;
        }

        hr = IDxDiagContainer_GetChildContainer(pddc, container, &child);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08lx\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IDxDiagContainer_EnumPropNames(child, 0, property, ARRAY_SIZE(property));
            ok(hr == S_OK || hr == E_INVALIDARG,
               "Expected IDxDiagContainer::EnumPropNames to return S_OK or E_INVALIDARG, got 0x%08lx\n", hr);

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
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetProp to return E_INVALIDARG, got 0x%08lx\n", hr);

    V_VT(&var) = 0xdead;
    hr = IDxDiagContainer_GetProp(child, NULL, &var);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetProp to return E_INVALIDARG, got 0x%08lx\n", hr);
    ok(V_VT(&var) == 0xdead, "Expected the variant to be untouched, got %u\n", V_VT(&var));

    hr = IDxDiagContainer_GetProp(child, L"", NULL);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetProp to return E_INVALIDARG, got 0x%08lx\n", hr);

    V_VT(&var) = 0xdead;
    hr = IDxDiagContainer_GetProp(child, L"", &var);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetProp to return E_INVALIDARG, got 0x%08lx\n", hr);
    ok(V_VT(&var) == 0xdead, "Expected the variant to be untouched, got %u\n", V_VT(&var));

    hr = IDxDiagContainer_GetProp(child, testW, NULL);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetProp to return E_INVALIDARG, got 0x%08lx\n", hr);

    V_VT(&var) = 0xdead;
    hr = IDxDiagContainer_GetProp(child, testW, &var);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetProp to return E_INVALIDARG, got 0x%08lx\n", hr);
    ok(V_VT(&var) == 0xdead, "Expected the variant to be untouched, got %u\n", V_VT(&var));

    VariantInit(&var);
    hr = IDxDiagContainer_GetProp(child, property, &var);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetProp to return S_OK, got 0x%08lx\n", hr);
    ok(V_VT(&var) != VT_EMPTY, "Expected the variant to be modified, got %d\n", V_VT(&var));

    /* Since the documentation for IDxDiagContainer::GetProp claims that the
     * function reports return values from VariantCopy, try to exercise failure
     * paths in handling the destination variant. */

    /* Try an invalid variant type. */
    V_VT(&var) = 0xdead;
    hr = IDxDiagContainer_GetProp(child, property, &var);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetProp to return S_OK, got 0x%08lx\n", hr);
    ok(V_VT(&var) != 0xdead, "Expected the variant to be modified, got %d\n", V_VT(&var));

    /* Try passing a variant with a locked SAFEARRAY. */
    bound.cElements = 1;
    bound.lLbound = 0;
    sa = SafeArrayCreate(VT_UI1, 1, &bound);
    ok(sa != NULL, "Expected SafeArrayCreate to return a valid pointer\n");

    V_VT(&var) = (VT_ARRAY | VT_UI1);
    V_ARRAY(&var) = sa;

    hr = SafeArrayLock(sa);
    ok(hr == S_OK, "Expected SafeArrayLock to return S_OK, got 0x%08lx\n", hr);

    hr = IDxDiagContainer_GetProp(child, property, &var);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetProp to return S_OK, got 0x%08lx\n", hr);
    ok(V_VT(&var) != (VT_ARRAY | VT_UI1), "Expected the variant to be modified\n");

    hr = SafeArrayUnlock(sa);
    ok(hr == S_OK, "Expected SafeArrayUnlock to return S_OK, got 0x%08lx\n", hr);
    hr = SafeArrayDestroy(sa);
    ok(hr == S_OK, "Expected SafeArrayDestroy to return S_OK, got 0x%08lx\n", hr);

    /* Determine whether GetProp calls VariantClear on the passed variant. */
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = (IUnknown *)child;
    IDxDiagContainer_AddRef(child);

    hr = IDxDiagContainer_GetProp(child, property, &var);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetProp to return S_OK, got 0x%08lx\n", hr);
    ok(V_VT(&var) != VT_UNKNOWN, "Expected the variant to be modified\n");

    IDxDiagContainer_AddRef(child);
    ref = IDxDiagContainer_Release(child);
    ok(ref == 2, "Expected reference count to be 2, got %lu\n", ref);
    IDxDiagContainer_Release(child);

    IDxDiagContainer_Release(child);
cleanup:
    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_root_children(void)
{
    HRESULT hr;
    DWORD count, index;

    static const WCHAR *root_children[] = {
        L"DxDiag_SystemInfo", L"DxDiag_DisplayDevices", L"DxDiag_DirectSound",
        L"DxDiag_DirectMusic", L"DxDiag_DirectInput", L"DxDiag_DirectPlay",
        L"DxDiag_SystemDevices", L"DxDiag_DirectXFiles", L"DxDiag_DirectShowFilters",
        L"DxDiag_LogicalDisks"
    };

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    /* Verify the identity and ordering of the root container's children. */
    hr = IDxDiagContainer_GetNumberOfChildContainers(pddc, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08lx\n", hr);
    if (FAILED(hr))
    {
        skip("IDxDiagContainer::GetNumberOfChildContainers failed\n");
        goto cleanup;
    }

    ok(count == ARRAY_SIZE(root_children),
       "Got unexpected count %lu for the number of child containers\n", count);

    if (count != ARRAY_SIZE(root_children))
    {
        skip("Received unexpected number of child containers\n");
        goto cleanup;
    }

    for (index = 0; index <= count; index++)
    {
        WCHAR container[256];

        hr = IDxDiagContainer_EnumChildContainerNames(pddc, index, container, ARRAY_SIZE(container));
        if (hr == E_INVALIDARG)
        {
            ok(index == count,
               "Expected IDxDiagContainer::EnumChildContainerNames to return "
               "E_INVALIDARG on the last index %lu\n", count);
            break;
        }
        else if (hr == S_OK)
        {
            ok(!lstrcmpW(container, root_children[index]),
               "Expected container %s for index %lu, got %s\n",
               wine_dbgstr_w(root_children[index]), index, wine_dbgstr_w(container));
        }
        else
        {
            ok(0, "IDxDiagContainer::EnumChildContainerNames unexpectedly returned 0x%08lx\n", hr);
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
        ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfProps to return S_OK, got 0x%08lx\n", hr);
        if (hr == S_OK)
            ok(prop_count == 0, "Expected container property count to be zero, got %lu\n", prop_count);
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
            ok(hr == S_OK, "[%d] Expected IDxDiagContainer::GetProp to return S_OK for %s, got 0x%08lx\n",
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
    static const struct property_test property_tests[] =
    {
        {L"dwOSMajorVersion", VT_UI4},
        {L"dwOSMinorVersion", VT_UI4},
        {L"dwOSBuildNumber", VT_UI4},
        {L"dwOSPlatformID", VT_UI4},
        {L"dwDirectXVersionMajor", VT_UI4},
        {L"dwDirectXVersionMinor", VT_UI4},
        {L"szDirectXVersionLetter", VT_BSTR},
        {L"bDebug", VT_BOOL},
        {L"bIsD3DDebugRuntime", VT_BOOL},
        {L"bNECPC98", VT_BOOL},
        {L"ullPhysicalMemory", VT_BSTR},
        {L"ullUsedPageFile", VT_BSTR},
        {L"ullAvailPageFile", VT_BSTR},
        {L"szWindowsDir", VT_BSTR},
        {L"szCSDVersion", VT_BSTR},
        {L"szDirectXVersionEnglish", VT_BSTR},
        {L"szDirectXVersionLongEnglish", VT_BSTR},
        {L"bNetMeetingRunning", VT_BOOL},
        {L"szMachineNameLocalized", VT_BSTR},
        {L"szMachineNameEnglish", VT_BSTR},
        {L"szLanguagesLocalized", VT_BSTR},
        {L"szLanguagesEnglish", VT_BSTR},
        {L"szTimeLocalized", VT_BSTR},
        {L"szTimeEnglish", VT_BSTR},
        {L"szPhysicalMemoryEnglish", VT_BSTR},
        {L"szPageFileLocalized", VT_BSTR},
        {L"szPageFileEnglish", VT_BSTR},
        {L"szOSLocalized", VT_BSTR},
        {L"szOSExLocalized", VT_BSTR},
        {L"szOSExLongLocalized", VT_BSTR},
        {L"szOSEnglish", VT_BSTR},
        {L"szOSExEnglish", VT_BSTR},
        {L"szOSExLongEnglish", VT_BSTR},
        {L"szProcessorEnglish", VT_BSTR},
    };

    IDxDiagContainer *container, *container2;
    HRESULT hr;

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    hr = IDxDiagContainer_GetChildContainer(pddc, L"", &container2);
    ok(hr == E_INVALIDARG, "Expected IDxDiagContainer::GetChildContainer to return E_INVALIDARG, got 0x%08lx\n", hr);

    hr = IDxDiagContainer_GetChildContainer(pddc, L"DxDiag_SystemInfo", &container);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08lx\n", hr);

    if (hr == S_OK)
    {
        trace("Testing container DxDiag_SystemInfo\n");
        test_container_properties(container, property_tests, ARRAY_SIZE(property_tests));

        container2 = NULL;
        hr = IDxDiagContainer_GetChildContainer(container, L"", &container2);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08lx\n", hr);
        ok(container2 != NULL, "Expected container2 != NULL\n");
        ok(container2 != container, "Expected container != container2\n");

        IDxDiagContainer_Release(container2);
        IDxDiagContainer_Release(container);
    }

    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_DxDiag_DisplayDevices(void)
{
    static const struct property_test property_tests[] =
    {
        {L"szDescription", VT_BSTR},
        {L"szDeviceName", VT_BSTR},
        {L"szKeyDeviceID", VT_BSTR},
        {L"szKeyDeviceKey", VT_BSTR},
        {L"szVendorId", VT_BSTR},
        {L"szDeviceId", VT_BSTR},
        {L"szDeviceIdentifier", VT_BSTR},
        {L"dwWidth", VT_UI4},
        {L"dwHeight", VT_UI4},
        {L"dwBpp", VT_UI4},
        {L"szDisplayMemoryLocalized", VT_BSTR},
        {L"szDisplayMemoryEnglish", VT_BSTR},
        {L"szDriverName", VT_BSTR},
        {L"szDriverVersion", VT_BSTR},
        {L"szSubSysId", VT_BSTR},
        {L"szRevisionId", VT_BSTR},
        {L"dwRefreshRate", VT_UI4},
        {L"szManufacturer", VT_BSTR},
        {L"b3DAccelerationExists", VT_BOOL},
        {L"b3DAccelerationEnabled", VT_BOOL},
        {L"bAGPEnabled", VT_BOOL},
        {L"bAGPExistenceValid", VT_BOOL},
        {L"bAGPExists", VT_BOOL},
        {L"bDDAccelerationEnabled", VT_BOOL},
        {L"iAdapter", VT_UI4},
    };

    IDxDiagContainer *display_cont = NULL;
    DWORD count, i;
    HRESULT hr;

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    hr = IDxDiagContainer_GetChildContainer(pddc, L"DxDiag_DisplayDevices", &display_cont);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08lx\n", hr);

    if (hr != S_OK)
        goto cleanup;

    hr = IDxDiagContainer_GetNumberOfProps(display_cont, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfProps to return S_OK, got 0x%08lx\n", hr);
    if (hr == S_OK)
        ok(count == 0, "Expected count to be 0, got %lu\n", count);

    hr = IDxDiagContainer_GetNumberOfChildContainers(display_cont, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08lx\n", hr);

    if (hr != S_OK)
        goto cleanup;

    for (i = 0; i < count; i++)
    {
        WCHAR child_container[256];
        IDxDiagContainer *child;

        hr = IDxDiagContainer_EnumChildContainerNames(display_cont, i, child_container, ARRAY_SIZE(child_container));
        ok(hr == S_OK, "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK, got 0x%08lx\n", hr);

        hr = IDxDiagContainer_GetChildContainer(display_cont, child_container, &child);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08lx\n", hr);

        if (hr == S_OK)
        {
            trace("Testing container %s\n", wine_dbgstr_w(child_container));
            test_container_properties(child, property_tests, ARRAY_SIZE(property_tests));
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
    static const struct property_test property_tests[] =
    {
        {L"szDescription", VT_BSTR},
        {L"szGuidDeviceID", VT_BSTR},
        {L"szDriverName", VT_BSTR},
        {L"szDriverPath", VT_BSTR},
        {L"szHardwareID", VT_BSTR},
    };

    IDxDiagContainer *sound_cont = NULL;
    DWORD count, i;
    HRESULT hr;

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    hr = IDxDiagContainer_GetChildContainer(pddc, L"DxDiag_DirectSound.DxDiag_SoundDevices", &sound_cont);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08lx\n", hr);

    hr = IDxDiagContainer_GetNumberOfProps(sound_cont, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfProps to return S_OK, got 0x%08lx\n", hr);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    hr = IDxDiagContainer_GetNumberOfChildContainers(sound_cont, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08lx\n", hr);

    for (i = 0; i < count; i++)
    {
        IDxDiagContainer *child, *child2;
        WCHAR child_container[256];

        hr = IDxDiagContainer_EnumChildContainerNames(sound_cont, i, child_container, ARRAY_SIZE(child_container));
        ok(hr == S_OK, "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK, got 0x%08lx\n", hr);

        hr = IDxDiagContainer_GetChildContainer(sound_cont, child_container, &child);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08lx\n", hr);

        trace("Testing container %s\n", wine_dbgstr_w(child_container));
        test_container_properties(child, property_tests, ARRAY_SIZE(property_tests));

        child2 = NULL;
        hr = IDxDiagContainer_GetChildContainer(child, L"", &child2);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08lx\n", hr);
        ok(child2 != NULL, "Expected child2 != NULL\n");
        ok(child2 != child, "Expected child != child2\n");

        IDxDiagContainer_Release(child2);
        IDxDiagContainer_Release(child);
    }

    IDxDiagContainer_Release(sound_cont);
    IDxDiagContainer_Release(pddc);
    IDxDiagProvider_Release(pddp);
}

static void test_DxDiag_SoundCaptureDevices(void)
{
    static const struct property_test property_tests[] =
    {
        {L"szDescription", VT_BSTR},
        {L"szGuidDeviceID", VT_BSTR},
        {L"szDriverName", VT_BSTR},
        {L"szDriverPath", VT_BSTR},
    };

    IDxDiagContainer *sound_cont = NULL;
    DWORD count, i;
    HRESULT hr;

    if (!create_root_IDxDiagContainer())
    {
        skip("Unable to create the root IDxDiagContainer\n");
        return;
    }

    hr = IDxDiagContainer_GetChildContainer(pddc, L"DxDiag_DirectSound.DxDiag_SoundCaptureDevices", &sound_cont);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08lx\n", hr);

    hr = IDxDiagContainer_GetNumberOfProps(sound_cont, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfProps to return S_OK, got 0x%08lx\n", hr);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    hr = IDxDiagContainer_GetNumberOfChildContainers(sound_cont, &count);
    ok(hr == S_OK, "Expected IDxDiagContainer::GetNumberOfChildContainers to return S_OK, got 0x%08lx\n", hr);

    for (i = 0; i < count; i++)
    {
        WCHAR child_container[256];
        IDxDiagContainer *child;

        hr = IDxDiagContainer_EnumChildContainerNames(sound_cont, i, child_container, ARRAY_SIZE(child_container));
        ok(hr == S_OK, "Expected IDxDiagContainer::EnumChildContainerNames to return S_OK, got 0x%08lx\n", hr);

        hr = IDxDiagContainer_GetChildContainer(sound_cont, child_container, &child);
        ok(hr == S_OK, "Expected IDxDiagContainer::GetChildContainer to return S_OK, got 0x%08lx\n", hr);

        trace("Testing container %s\n", wine_dbgstr_w(child_container));
        test_container_properties(child, property_tests, ARRAY_SIZE(property_tests));

        IDxDiagContainer_Release(child);
    }

    IDxDiagContainer_Release(sound_cont);
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
