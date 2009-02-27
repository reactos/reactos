/*
 * Unit tests for Direct Show functions - IReferenceClock
 *
 * Copyright (C) 2007 Alex Villacís Lasso
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

#include <assert.h>

#define COBJMACROS

#include "wine/test.h"
#include "uuids.h"
#include "dshow.h"
#include "control.h"

static void test_IReferenceClock_query_interface(const char * clockdesc, IReferenceClock * pClock)
{
    HRESULT hr;
    IUnknown *pF;

    hr = IReferenceClock_QueryInterface(pClock, &IID_IUnknown, (LPVOID *)&pF);
    ok(hr == S_OK, "IReferenceClock_QueryInterface returned %x\n", hr);
    ok(pF != NULL, "pF is NULL\n");

    hr = IReferenceClock_QueryInterface(pClock, &IID_IDirectDraw, (LPVOID *)&pF);
    ok(hr == E_NOINTERFACE, "IReferenceClock_QueryInterface returned %x\n", hr);
    ok(pF == NULL, "pF is not NULL\n");

    hr = IReferenceClock_QueryInterface(pClock, &IID_IReferenceClock, (LPVOID *)&pF);
    ok(hr == S_OK, "IReferenceClock_QueryInterface returned %x\n", hr);
    ok(pF != NULL, "pF is NULL\n");
}

/* The following method expects a reference clock that will keep ticking for
 * at least 5 seconds since its creation. This method assumes no other methods
 * were called on the IReferenceClock interface since its creation.
 */
static void test_IReferenceClock_methods(const char * clockdesc, IReferenceClock * pClock)
{
    HRESULT hr;
    REFERENCE_TIME time1;
    REFERENCE_TIME time2;
    signed long diff;

    /* Test response from invalid (NULL) argument */
    hr = IReferenceClock_GetTime(pClock, NULL);
    ok (hr == E_POINTER, "%s - Expected E_POINTER (0x%08x), got 0x%08x\n", clockdesc, E_POINTER, hr);

    /* Test response for valid value - try 1 */
    /* TODO: test whether Windows actually returns S_FALSE in its first invocation */
    time1 = (REFERENCE_TIME)0xdeadbeef;
    hr = IReferenceClock_GetTime(pClock, &time1);
    ok (hr == S_FALSE || hr == S_OK, "%s - Expected S_OK or S_FALSE, got 0x%08x\n", clockdesc, hr);
    ok (time1 != 0xdeadbeef, "%s - value was NOT changed on return!\n", clockdesc);

    /* Test response for valid value - try 2 */
    time2 = (REFERENCE_TIME)0xdeadbeef;
    hr = IReferenceClock_GetTime(pClock, &time2);
    ok (hr == S_FALSE || hr == S_OK, "%s - Expected S_OK or S_FALSE, got 0x%08x\n", clockdesc, hr);
    ok (time2 != 0xdeadbeef, "%s - value was NOT changed on return!\n", clockdesc);

    /* In case the second invocation managed to return S_FALSE, MSDN says the
       returned time is the same as the previous one. */
    ok ((hr != S_FALSE || time1 == time2), "%s - returned S_FALSE, but values not equal!\n", clockdesc);

    time1 = time2;
    Sleep(1000); /* Sleep for at least 1 second */
    hr = IReferenceClock_GetTime(pClock, &time2);
    /* After a 1-second sleep, there is no excuse to get S_FALSE (see TODO above) */
    ok (hr == S_OK, "%s - Expected S_OK, got 0x%08x\n", clockdesc, hr);

    /* FIXME: How much deviation should be allowed after a sleep? */
    /* 0.3% is common, and 0.4% is sometimes observed. */
    diff = time2 - time1;
    ok (9940000 <= diff && diff <= 10240000, "%s - Expected difference around 10000000, got %lu\n", clockdesc, diff);

}

static void test_IReferenceClock_SystemClock(void)
{
    IReferenceClock * pReferenceClock;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SystemClock, NULL, CLSCTX_INPROC_SERVER, &IID_IReferenceClock, (LPVOID*)&pReferenceClock);
    ok(hr == S_OK, "Unable to create reference clock from system clock %x\n", hr);
    if (hr == S_OK)
    {
        test_IReferenceClock_query_interface("SystemClock", pReferenceClock);
	test_IReferenceClock_methods("SystemClock", pReferenceClock);
	IReferenceClock_Release(pReferenceClock);
    }
}

START_TEST(referenceclock)
{
    CoInitialize(NULL);

    test_IReferenceClock_SystemClock();

    CoUninitialize();
}
