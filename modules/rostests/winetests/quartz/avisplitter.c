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

static IUnknown *pAviSplitter = NULL;

static BOOL create_avisplitter(void)
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

#define TEST_INTERFACE(riid,expected) do { \
    hr = IUnknown_QueryInterface(pAviSplitter, &riid, (void**)&iface); \
    ok( hr == expected, #riid" should %s got %08X\n", expected==S_OK ? "exist" : "not be present", GetLastError() ); \
    if (hr == S_OK) { \
        ref = IUnknown_Release(iface); \
        ok(ref == 1, "Reference is %u, expected 1\n", ref); \
    } \
    iface = NULL; \
    } while(0)

    TEST_INTERFACE(IID_IBaseFilter,S_OK);
    TEST_INTERFACE(IID_IMediaSeeking,E_NOINTERFACE);
    TEST_INTERFACE(IID_IKsPropertySet,E_NOINTERFACE);
    TEST_INTERFACE(IID_IMediaPosition,E_NOINTERFACE);
    TEST_INTERFACE(IID_IQualityControl,E_NOINTERFACE);
    TEST_INTERFACE(IID_IQualProp,E_NOINTERFACE);
#undef TEST_INTERFACE
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

    IUnknown_QueryInterface(pAviSplitter, &IID_IBaseFilter, (void **)&base);
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

static void test_filesourcefilter(void)
{
    static const WCHAR prefix[] = {'w','i','n',0};
    static const struct
    {
        const char *label;
        const char *data;
        DWORD size;
        const GUID *subtype;
    }
    tests[] =
    {
        {
            "AVI",
            "\x52\x49\x46\x46xxxx\x41\x56\x49\x20",
            12,
            &MEDIASUBTYPE_Avi,
        },
        {
            "MPEG1 System",
            "\x00\x00\x01\xBA\x21\x00\x01\x00\x01\x80\x00\x01\x00\x00\x01\xBB",
            16,
            &MEDIASUBTYPE_MPEG1System,
        },
        {
            "MPEG1 Video",
            "\x00\x00\x01\xB3",
            4,
            &MEDIASUBTYPE_MPEG1Video,
        },
        {
            "MPEG1 Audio",
            "\xFF\xE0",
            2,
            &MEDIASUBTYPE_MPEG1Audio,
        },
        {
            "MPEG2 Program",
            "\x00\x00\x01\xBA\x40",
            5,
            &MEDIASUBTYPE_MPEG2_PROGRAM,
        },
        {
            "WAVE",
            "\x52\x49\x46\x46xxxx\x57\x41\x56\x45",
            12,
            &MEDIASUBTYPE_WAVE,
        },
        {
            "unknown format",
            "Hello World",
            11,
            NULL, /* FIXME: should be &MEDIASUBTYPE_NULL */
        },
    };
    WCHAR path[MAX_PATH], temp[MAX_PATH];
    IFileSourceFilter *filesource;
    DWORD ret, written;
    IBaseFilter *base;
    AM_MEDIA_TYPE mt;
    OLECHAR *olepath;
    BOOL success;
    HANDLE file;
    HRESULT hr;
    int i;

    ret = GetTempPathW(MAX_PATH, temp);
    ok(ret, "GetTempPathW failed with error %u\n", GetLastError());
    ret = GetTempFileNameW(temp, prefix, 0, path);
    ok(ret, "GetTempFileNameW failed with error %u\n", GetLastError());

    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        trace("Running test for %s\n", tests[i].label);

        file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        ok(file != INVALID_HANDLE_VALUE, "CreateFileW failed with error %u\n", GetLastError());
        success = WriteFile(file, tests[i].data, tests[i].size, &written, NULL);
        ok(success, "WriteFile failed with error %u\n", GetLastError());
        ok(written == tests[i].size, "could not write test data\n");
        CloseHandle(file);

        hr = CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER,
                              &IID_IBaseFilter, (void **)&base);
        ok(hr == S_OK, "CoCreateInstance failed with %08x\n", hr);
        hr = IBaseFilter_QueryInterface(base, &IID_IFileSourceFilter, (void **)&filesource);
        ok(hr == S_OK, "IBaseFilter_QueryInterface failed with %08x\n", hr);

        olepath = (void *)0xdeadbeef;
        hr = IFileSourceFilter_GetCurFile(filesource, &olepath, NULL);
        ok(hr == S_OK, "expected S_OK, got %08x\n", hr);
        ok(olepath == NULL, "expected NULL, got %p\n", olepath);

        hr = IFileSourceFilter_Load(filesource, NULL, NULL);
        ok(hr == E_POINTER, "expected E_POINTER, got %08x\n", hr);

        hr = IFileSourceFilter_Load(filesource, path, NULL);
        ok(hr == S_OK, "IFileSourceFilter_Load failed with %08x\n", hr);

        hr = IFileSourceFilter_GetCurFile(filesource, NULL, &mt);
        ok(hr == E_POINTER, "expected E_POINTER, got %08x\n", hr);

        olepath = NULL;
        hr = IFileSourceFilter_GetCurFile(filesource, &olepath, NULL);
        ok(hr == S_OK, "expected S_OK, got %08x\n", hr);
        CoTaskMemFree(olepath);

        olepath = NULL;
        memset(&mt, 0x11, sizeof(mt));
        hr = IFileSourceFilter_GetCurFile(filesource, &olepath, &mt);
        ok(hr == S_OK, "expected S_OK, got %08x\n", hr);
        ok(!lstrcmpW(olepath, path),
           "expected %s, got %s\n", wine_dbgstr_w(path), wine_dbgstr_w(olepath));
        if (tests[i].subtype)
        {
            ok(IsEqualGUID(&mt.majortype, &MEDIATYPE_Stream),
               "expected MEDIATYPE_Stream, got %s\n", wine_dbgstr_guid(&mt.majortype));
            ok(IsEqualGUID(&mt.subtype, tests[i].subtype),
               "expected %s, got %s\n", wine_dbgstr_guid(tests[i].subtype), wine_dbgstr_guid(&mt.subtype));
        }
        CoTaskMemFree(olepath);

        IFileSourceFilter_Release(filesource);
        IBaseFilter_Release(base);

        success = DeleteFileW(path);
        ok(success, "DeleteFileW failed with error %u\n", GetLastError());
    }
}

static const WCHAR avifile[] = {'t','e','s','t','.','a','v','i',0};

static WCHAR *load_resource(const WCHAR *name)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(sizeof(pathW)/sizeof(WCHAR), pathW);
    lstrcatW(pathW, name);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %d\n", wine_dbgstr_w(pathW),
        GetLastError());

    res = FindResourceW(NULL, name, (LPCWSTR)RT_RCDATA);
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandleA(NULL), res ));
    WriteFile( file, ptr, SizeofResource( GetModuleHandleA(NULL), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleA(NULL), res ), "couldn't write resource\n" );
    CloseHandle( file );

    return pathW;
}

static void test_filter_graph(void)
{
    IFileSourceFilter *pfile = NULL;
    IBaseFilter *preader = NULL, *pavi = NULL;
    IEnumPins *enumpins = NULL;
    IPin *filepin = NULL, *avipin = NULL;
    HRESULT hr;
    HANDLE file = NULL;
    PIN_DIRECTION dir = PINDIR_OUTPUT;
    char buffer[13];
    DWORD readbytes;
    FILTER_STATE state;

    WCHAR *filename = load_resource(avifile);

    file = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        skip("Could not read test file \"%s\", skipping test\n", wine_dbgstr_w(filename));
        DeleteFileW(filename);
        return;
    }

    memset(buffer, 0, 13);
    readbytes = 12;
    ReadFile(file, buffer, readbytes, &readbytes, NULL);
    CloseHandle(file);
    if (strncmp(buffer, "RIFF", 4) || strcmp(buffer + 8, "AVI "))
    {
        skip("%s is not an avi riff file, not doing the avi splitter test\n",
            wine_dbgstr_w(filename));
        DeleteFileW(filename);
        return;
    }

    hr = IUnknown_QueryInterface(pAviSplitter, &IID_IFileSourceFilter,
        (void **)&pfile);
    ok(hr == E_NOINTERFACE,
        "Avi splitter returns unexpected error: %08x\n", hr);
    if (pfile)
        IFileSourceFilter_Release(pfile);
    pfile = NULL;

    hr = CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IBaseFilter, (LPVOID*)&preader);
    ok(hr == S_OK, "Could not create asynchronous reader: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    hr = IBaseFilter_QueryInterface(preader, &IID_IFileSourceFilter,
        (void**)&pfile);
    ok(hr == S_OK, "Could not get IFileSourceFilter: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    hr = IUnknown_QueryInterface(pAviSplitter, &IID_IBaseFilter,
        (void**)&pavi);
    ok(hr == S_OK, "Could not get base filter: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    hr = IFileSourceFilter_Load(pfile, filename, NULL);
    if (hr != S_OK)
    {
        trace("Could not load file: %08x\n", hr);
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

    IEnumPins_Release(enumpins);
    enumpins = NULL;

    hr = IBaseFilter_EnumPins(pavi, &enumpins);
    ok(hr == S_OK, "No enumpins: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    hr = IEnumPins_Next(enumpins, 1, &avipin, NULL);
    ok(hr == S_OK, "No pin: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    hr = IPin_Connect(filepin, avipin, NULL);
    ok(hr == S_OK, "Could not connect: %08x\n", hr);
    if (hr != S_OK)
        goto fail;

    IPin_Release(avipin);
    avipin = NULL;

    IEnumPins_Reset(enumpins);

    /* Windows puts the pins in the order: Outputpins - Inputpin,
     * wine does the reverse, just don't test it for now
     * Hate to admit it, but windows way makes more sense
     */
    while (IEnumPins_Next(enumpins, 1, &avipin, NULL) == S_OK)
    {
        IPin_QueryDirection(avipin, &dir);
        if (dir == PINDIR_OUTPUT)
        {
            /* Well, connect it to a null renderer! */
            IBaseFilter *pnull = NULL;
            IEnumPins *nullenum = NULL;
            IPin *nullpin = NULL;

            hr = CoCreateInstance(&CLSID_NullRenderer, NULL,
                CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID*)&pnull);
            if (hr == REGDB_E_CLASSNOTREG)
            {
                win_skip("Null renderer not registered, skipping\n");
                break;
            }
            ok(hr == S_OK, "Could not create null renderer: %08x\n", hr);

            hr = IBaseFilter_EnumPins(pnull, &nullenum);
            ok(hr == S_OK, "Failed to enum pins, hr %#x.\n", hr);
            hr = IEnumPins_Next(nullenum, 1, &nullpin, NULL);
            ok(hr == S_OK, "Failed to get next pin, hr %#x.\n", hr);
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
        }

        IPin_Release(avipin);
        avipin = NULL;
    }

    if (avipin)
        IPin_Release(avipin);
    avipin = NULL;

    if (hr != S_OK)
        goto fail2;
    /* At this point there is a minimalistic connected avi splitter that can
     * be used for all sorts of source filter tests. However that still needs
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

                hr = IPin_QueryPinInfo(to, &info);
                ok(hr == S_OK, "Failed to query pin info, hr %#x.\n", hr);

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
        IEnumPins_Release(enumpins);

    if (avipin)
        IPin_Release(avipin);
    if (filepin)
    {
        IPin *to = NULL;

        IPin_ConnectedTo(filepin, &to);
        if (to)
        {
            IPin_Disconnect(filepin);
            IPin_Disconnect(to);
        }
        IPin_Release(filepin);
    }

    if (preader)
        IBaseFilter_Release(preader);
    if (pavi)
        IBaseFilter_Release(pavi);
    if (pfile)
        IFileSourceFilter_Release(pfile);

    DeleteFileW(filename);
}

START_TEST(avisplitter)
{
    CoInitialize(NULL);

    if (!create_avisplitter())
    {
        skip("Could not create avisplitter\n");
        return;
    }

    test_query_interface();
    test_basefilter();
    test_filesourcefilter();
    test_filter_graph();

    release_avisplitter();

    CoUninitialize();
}
