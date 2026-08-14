/*
 * Copyright 2020 Nikolay Sivov for CodeWeavers
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

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "rtworkq.h"

#include "wine/test.h"

static void test_platform_init(void)
{
    APTTYPEQUALIFIER qualifier;
    APTTYPE apttype;
    HRESULT hr;

    /* Startup initializes MTA. */
    hr = CoGetApartmentType(&apttype, &qualifier);
    ok(hr == CO_E_NOTINITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = RtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    hr = CoGetApartmentType(&apttype, &qualifier);
    ok(hr == S_OK || broken(FAILED(hr)) /* Win8 */, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        ok(apttype == APTTYPE_MTA && qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA,
                "Unexpected apartment type %d, qualifier %d.\n", apttype, qualifier);

    hr = RtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = CoGetApartmentType(&apttype, &qualifier);
    ok(hr == CO_E_NOTINITIALIZED, "Unexpected hr %#lx.\n", hr);

    /* Try with STA initialized before startup. */
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    hr = CoGetApartmentType(&apttype, &qualifier);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(apttype == APTTYPE_MAINSTA && qualifier == APTTYPEQUALIFIER_NONE,
            "Unexpected apartment type %d, qualifier %d.\n", apttype, qualifier);

    hr = RtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    hr = CoGetApartmentType(&apttype, &qualifier);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(apttype == APTTYPE_MAINSTA && qualifier == APTTYPEQUALIFIER_NONE,
            "Unexpected apartment type %d, qualifier %d.\n", apttype, qualifier);

    hr = RtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    CoUninitialize();

    /* Startup -> init main STA -> uninitialize -> shutdown */
    hr = RtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    hr = CoGetApartmentType(&apttype, &qualifier);
    ok(hr == S_OK || broken(FAILED(hr)) /* Win8 */, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        ok(apttype == APTTYPE_MTA && qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA,
                "Unexpected apartment type %d, qualifier %d.\n", apttype, qualifier);

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hr == S_OK, "Failed to initialize, hr %#lx.\n", hr);

    hr = CoGetApartmentType(&apttype, &qualifier);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(apttype == APTTYPE_MAINSTA && qualifier == APTTYPEQUALIFIER_NONE,
            "Unexpected apartment type %d, qualifier %d.\n", apttype, qualifier);

    CoUninitialize();

    hr = CoGetApartmentType(&apttype, &qualifier);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(apttype == APTTYPE_MTA && qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA,
            "Unexpected apartment type %d, qualifier %d.\n", apttype, qualifier);

    hr = RtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
}

START_TEST(rtworkq)
{
    test_platform_init();
}
