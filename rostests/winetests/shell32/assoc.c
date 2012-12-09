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

    /* this works since XP */
    hr = CoCreateInstance(&CLSID_QueryAssociations, NULL, CLSCTX_INPROC_SERVER, &IID_IQueryAssociations, (void*)&qa);

    if (FAILED(hr)) {
        win_skip("CoCreateInstance for IQueryAssociations returned 0x%x\n", hr);
        return;
    }

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


static void test_IApplicationAssociationRegistration_QueryInterface(void)
{
    IApplicationAssociationRegistration *appreg;
    IApplicationAssociationRegistration *appreg2;
    IUnknown *unk;
    HRESULT hr;

    /* this works since Vista */
    hr = CoCreateInstance(&CLSID_ApplicationAssociationRegistration, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IApplicationAssociationRegistration, (LPVOID*)&appreg);

    if (FAILED(hr)) {
        skip("IApplicationAssociationRegistration not created: 0x%x\n", hr);
        return;
    }

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

    IApplicationAssociationRegistration_Release(appreg);
}


START_TEST(assoc)
{
    CoInitialize(NULL);

    test_IQueryAssociations_QueryInterface();
    test_IApplicationAssociationRegistration_QueryInterface();

    CoUninitialize();
}
