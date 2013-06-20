/*
 * Unit tests for the avi splitter functions
 *
 * Copyright (C) 2007 Google (Lei Zhang)
 * Copyright (C) 2008 Google (Maarten Lankhorst)
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

#include "wine/test.h"
#include "dshow.h"
#include "tlhelp32.h"

static HANDLE (WINAPI *pCreateToolhelp32Snapshot)(DWORD, DWORD);
static BOOL (WINAPI *pThread32First)(HANDLE, LPTHREADENTRY32);
static BOOL (WINAPI *pThread32Next)(HANDLE, LPTHREADENTRY32);

static IUnknown *pAviSplitter = NULL;

static int count_threads(void)
{
    THREADENTRY32 te;
    int threads;
    HANDLE h;

    h = pCreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    te.dwSize = sizeof(te);

    if (h == INVALID_HANDLE_VALUE)
        return -1;

    pThread32First(h, &te);
    if (te.th32OwnerProcessID == GetCurrentProcessId())
        threads = 1;
    else
        threads = 0;

    while (pThread32Next(h, &te))
        if (te.th32OwnerProcessID == GetCurrentProcessId())
            ++threads;

    CloseHandle(h);
    return threads;
}

static int create_avisplitter(void)
{
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_AviSplitter, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IUnknown, (LPVOID*)&pAviSplitter);
    return (hr == S_OK && pAviSplitter != NULL);
}

static void release_avisplitter(void)
{
    HRESULT hr;

    Sleep(1000);
    hr = IUnknown_Release(pAviSplitter);

    /* Looks like wine has a reference leak somewhere on test_threads tests,
     * it passes in windows
     */
    ok(hr == 0, "IUnknown_Release failed with %d\n", (INT)hr);

    while (hr > 0)
        hr = IUnknown_Release(pAviSplitter);
    pAviSplitter = NULL;
}

static void test_query_interface(void)
{
    HRESULT hr;
    ULONG ref;
    IUnknown *iface= NULL;

    hr = IUnknown_QueryInterface(pAviSplitter, &IID_IBaseFilter,
        (void**)&iface);

    ok(hr == S_OK,
        "IID_IBaseFilter should exist, got %08x!\n", GetLastError());
    if (hr == S_OK)
    {
        ref = IUnknown_Release(iface);
        iface = NULL;
        ok(ref == 1, "Reference is %u, expected 1\n", ref);
    }

    hr = IUnknown_QueryInterface(pAviSplitter, &IID_IMediaSeeking,
        (void**)&iface);
    if (hr == S_OK)
        ref = IUnknown_Release(iface);
    iface = NULL;
    todo_wine ok(hr == E_NOINTERFACE,
        "Query for IMediaSeeking returned: %08x\n", hr);

/* These interfaces should not be present:
    IID_IKsPropertySet, IID_IMediaPosition, IID_IQualityControl, IID_IQualProp
*/
}

static void test_pin(IPin *pin)
{
    IMemInputPin *mpin = NULL;

    IPin_QueryInterface(pin, &IID_IMemInputPin, (void **)&mpin);

    ok(mpin == NULL, "IMemInputPin found!\n");
    if (mpin)
        IMemInputPin_Release(mpin);
    /* TODO */
}

static void test_basefilter(void)
{
    IEnumPins *pin_enum = NULL;
    IBaseFilter *base = NULL;
    IPin *pins[2];
    ULONG ref;
    HRESULT hr;

    IUnknown_QueryInterface(pAviSplitter, &IID_IBaseFilter, (void *)&base);
    if (base == NULL)
    {
        /* test_query_interface handles this case */
        skip("No IBaseFilter\n");
        return;
    }

    hr = IBaseFilter_EnumPins(base, NULL);
    ok(hr == E_POINTER, "hr = %08x and not E_POINTER\n", hr);

    hr= IBaseFilter_EnumPins(base, &pin_enum);
    ok(hr == S_OK, "hr = %08x and not S_OK\n", hr);

    hr = IEnumPins_Next(pin_enum, 1, NULL, NULL);
    ok(hr == E_POINTER, "hr = %08x and not E_POINTER\n", hr);

    hr = IEnumPins_Next(pin_enum, 2, pins, NULL);
    ok(hr == E_INVALIDARG, "hr = %08x and not E_INVALIDARG\n", hr);

    pins[0] = (void *)0xdead;
    pins[1] = (void *)0xdeed;

    hr = IEnumPins_Next(pin_enum, 2, pins, &ref);
    ok(hr == S_FALSE, "hr = %08x instead of S_FALSE\n", hr);
    ok(pins[0] != (void *)0xdead && pins[0] != NULL,
        "pins[0] = %p\n", pins[0]);
    if (pins[0] != (void *)0xdead && pins[0] != NULL)
    {
        test_pin(pins[0]);
        IPin_Release(pins[0]);
    }

    ok(pins[1] == (void *)0xdeed, "pins[1] = %p\n", pins[1]);

    ref = IEnumPins_Release(pin_enum);
    ok(ref == 0, "ref is %u and not 0!\n", ref);

    IBaseFilter_Release(base);
}

static const WCHAR wfile[] = {'t','e','s','t','.','a','v','i',0};
static const char afile[] = "test.avi";

/* This test doesn't use the quartz filtergraph because it makes it impossible
 * to be certain that a thread is really one owned by the avi splitter
 * A lot of the decoder filters will also have their own thread, and windows'
 * filtergraph has a separate thread for start/stop/seeking requests.
 * By avoiding the filtergraph all together and connecting streams directly to
 * the null renderer I am sure that this is not the case here.
 */
static void test_threads(void)
{
    IFileSourceFilter *pfile = NULL;
    IBaseFilter *preader = NULL, *pavi = NULL;
    IEnumPins *enumpins = NULL;
    IPin *filepin = NULL, *avipin = NULL;
    HRESULT hr;
    int baselevel, curlevel, expected;
    HANDLE file = NULL;
    PIN_DIRECTION dir = PINDIR_OUTPUT;
    char buffer[13];
    DWORD readbytes;
    FILTER_STATE state;

    /* We need another way of counting threads on NT4. Skip these tests (for now) */
    if (!pCreateToolhelp32Snapshot || !pThread32First || !pThread32Next)
    {
        win_skip("Needed thread functions are not available (NT4)\n");
        return;
    }

    /* Before doing anything (number of threads at the start differs per OS) */
    baselevel = count_threads();

    file = CreateFileW(wfile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        skip("Could not read test file \"%s\", skipping test\n", afile);
        return;
    }

    memset(buffer, 0, 13);
    readbytes = 12;
    ReadFile(file, buffer, readbytes, &readbytes, NULL);
    CloseHandle(file);
    if (strncmp(buffer, "RIFF", 4) || strcmp(buffer + 8, "AVI "))
    {
        skip("%s is not an avi riff file, not doing the avi splitter test\n",
            afile);
        return;
    }

    hr = IUnknown_QueryInterface(pAviSplitter, &IID_IFileSourceFilter,
        (void **)&pfile);
    ok(hr == E_NOINTERFACE,
        "Avi splitter returns unexpected error: %08x\n", hr);
    if (pfile)
        IUnknown_Release(pfile);
    pfile = NULL;

    hr = CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IBaseFilter, (LPVOID*)&preader);
    ok(hr == S_OK, "Could not create asynchronous reader: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    hr = IUnknown_QueryInterface(preader, &IID_IFileSourceFilter,
        (void**)&pfile);
    ok(hr == S_OK, "Could not get IFileSourceFilter: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    hr = IUnknown_QueryInterface(pAviSplitter, &IID_IBaseFilter,
        (void**)&pavi);
    ok(hr == S_OK, "Could not get base filter: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    hr = IFileSourceFilter_Load(pfile, wfile, NULL);
    if (hr != S_OK)
    {
        trace("Could not load file\n");
        goto fail;
    }

    hr = IBaseFilter_EnumPins(preader, &enumpins);
    ok(hr == S_OK, "No enumpins: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    hr = IEnumPins_Next(enumpins, 1, &filepin, NULL);
    ok(hr == S_OK, "No pin: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    IUnknown_Release(enumpins);
    enumpins = NULL;

    hr = IBaseFilter_EnumPins(pavi, &enumpins);
    ok(hr == S_OK, "No enumpins: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    hr = IEnumPins_Next(enumpins, 1, &avipin, NULL);
    ok(hr == S_OK, "No pin: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    curlevel = count_threads();
    ok(curlevel == baselevel,
        "Amount of threads should be %d not %d\n", baselevel, curlevel);

    hr = IPin_Connect(filepin, avipin, NULL);
    ok(hr == S_OK, "Could not connect: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    expected = 1 + baselevel;
    curlevel = count_threads();
    ok(curlevel == expected,
        "Amount of threads should be %d not %d\n", expected, curlevel);

    IUnknown_Release(avipin);
    avipin = NULL;

    IEnumPins_Reset(enumpins);

    /* Windows puts the pins in the order: Outputpins - Inputpin,
     * wine does the reverse, just don't test it for now
     * Hate to admit it, but windows way makes more sense
     */
    while (IEnumPins_Next(enumpins, 1, &avipin, NULL) == S_OK)
    {
        ok(hr == S_OK, "hr: %08x\n", hr);
        IPin_QueryDirection(avipin, &dir);
        if (dir == PINDIR_OUTPUT)
        {
            /* Well, connect it to a null renderer! */
            IBaseFilter *pnull = NULL;
            IEnumPins *nullenum = NULL;
            IPin *nullpin = NULL;

            hr = CoCreateInstance(&CLSID_NullRenderer, NULL,
                CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID*)&pnull);
            ok(hr == S_OK, "Could not create null renderer: %08x\n", hr);
            if (hr != S_OK)
                break;

            IBaseFilter_EnumPins(pnull, &nullenum);
            IEnumPins_Next(nullenum, 1, &nullpin, NULL);
            IEnumPins_Release(nullenum);
            IPin_QueryDirection(nullpin, &dir);

            hr = IPin_Connect(avipin, nullpin, NULL);
            ok(hr == S_OK, "Failed to connect output pin: %08x\n", hr);
            IPin_Release(nullpin);
            if (hr != S_OK)
            {
                IBaseFilter_Release(pnull);
                break;
            }
            IBaseFilter_Run(pnull, 0);
            ++expected;
        }

        IUnknown_Release(avipin);
        avipin = NULL;
    }

    if (avipin)
        IUnknown_Release(avipin);
    avipin = NULL;

    if (hr != S_OK)
        goto fail2;
    /* At this point there is a minimalistic connected avi splitter that can
     * Be used for all sorts of source filter tests, however that still needs
     * to be written at a later time.
     *
     * Interesting tests:
     * - Can you disconnect an output pin while running?
     *   Expecting: Yes
     * - Can you disconnect the pullpin while running?
     *   Expecting: No
     * - Is the reference count incremented during playback or when connected?
     *   Does this happen once for every output pin? Or is there something else
     *   going on.
     *   Expecting: You tell me
     */

    IBaseFilter_Run(preader, 0);
    IBaseFilter_Run(pavi, 0);
    IBaseFilter_GetState(pavi, INFINITE, &state);

    curlevel = count_threads();
    ok(curlevel == expected,
        "Amount of threads should be %d not %d\n", expected, curlevel);

    IBaseFilter_Pause(pavi);
    IBaseFilter_Pause(preader);
    IBaseFilter_Stop(pavi);
    IBaseFilter_Stop(preader);
    IBaseFilter_GetState(pavi, INFINITE, &state);
    IBaseFilter_GetState(preader, INFINITE, &state);

fail2:
    IEnumPins_Reset(enumpins);
    while (IEnumPins_Next(enumpins, 1, &avipin, NULL) == S_OK)
    {
        IPin *to = NULL;

        IPin_QueryDirection(avipin, &dir);
        IPin_ConnectedTo(avipin, &to);
        if (to)
        {
            IPin_Release(to);

            if (dir == PINDIR_OUTPUT)
            {
                PIN_INFO info;
                IPin_QueryPinInfo(to, &info);

                /* Release twice: Once normal, second from the
                 * previous while loop
                 */
                IBaseFilter_Stop(info.pFilter);
                IPin_Disconnect(to);
                IPin_Disconnect(avipin);
                IBaseFilter_Release(info.pFilter);
                IBaseFilter_Release(info.pFilter);
            }
            else
            {
                IPin_Disconnect(to);
                IPin_Disconnect(avipin);
            }
        }
        IPin_Release(avipin);
        avipin = NULL;
    }

fail:
    if (hr != S_OK)
        skip("Prerequisites not matched, skipping remainder of test\n");
    if (enumpins)
        IUnknown_Release(enumpins);

    if (avipin)
        IUnknown_Release(avipin);
    if (filepin)
    {
        IPin *to = NULL;

        IPin_ConnectedTo(filepin, &to);
        if (to)
        {
            IPin_Disconnect(filepin);
            IPin_Disconnect(to);
        }
        IUnknown_Release(filepin);
    }

    if (preader)
        IUnknown_Release(preader);
    if (pavi)
        IUnknown_Release(pavi);
    if (pfile)
        IUnknown_Release(pfile);

    curlevel = count_threads();
    todo_wine
    ok(curlevel == baselevel,
        "Amount of threads should be %d not %d\n", baselevel, curlevel);
}

START_TEST(avisplitter)
{
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");

    pCreateToolhelp32Snapshot = (void*)GetProcAddress(kernel32, "CreateToolhelp32Snapshot");
    pThread32First = (void*)GetProcAddress(kernel32, "Thread32First");
    pThread32Next = (void*)GetProcAddress(kernel32, "Thread32Next");

    CoInitialize(NULL);

    if (!create_avisplitter())
    {
        skip("Could not create avisplitter\n");
        return;
    }

    test_query_interface();
    test_basefilter();
    test_threads();

    release_avisplitter();

    CoUninitialize();
}
