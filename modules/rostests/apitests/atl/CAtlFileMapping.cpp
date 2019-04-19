/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for CAtlFileMapping
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include <atlfile.h>

#ifdef HAVE_APITEST
    #include <apitest.h>
#else
    #include <stdlib.h>
    #include <stdio.h>
    #include <stdarg.h>
    #include <windows.h>
    int g_tests_executed = 0;
    int g_tests_failed = 0;
    int g_tests_skipped = 0;
    const char *g_file = NULL;
    int g_line = 0;
    void set_location(const char *file, int line)
    {
        g_file = file;
        g_line = line;
    }
    void ok_func(int value, const char *fmt, ...)
    {
        va_list va;
        va_start(va, fmt);
        if (!value)
        {
            printf("%s (%d): ", g_file, g_line);
            vprintf(fmt, va);
            g_tests_failed++;
        }
        g_tests_executed++;
        va_end(va);
    }
    void skip_func(const char *fmt, ...)
    {
        va_list va;
        va_start(va, fmt);
        printf("%s (%d): test skipped: ", g_file, g_line);
        vprintf(fmt, va);
        g_tests_skipped++;
        va_end(va);
    }
    #undef ok
    #define ok(value, ...) do { \
        set_location(__FILE__, __LINE__); \
        ok_func(value, __VA_ARGS__); \
    } while (0)
    #define ok_(x1,x2) set_location(x1,x2); ok_func
    #define skip(...) do { \
        set_location(__FILE__, __LINE__); \
        skip_func(__VA_ARGS__); \
    } while (0)
    #define START_TEST(x)   int main()
    char *wine_dbgstr_w(const wchar_t *wstr)
    {
        static char buf[512];
        WideCharToMultiByte(CP_ACP, 0, wstr, -1, buf, _countof(buf), NULL, NULL);
        return buf;
    }
#endif

struct TestData
{
    int data[4];
};


static void test_SharedMem()
{
    CAtlFileMapping<TestData> test1, test2;
    CAtlFileMappingBase test3;
    BOOL bAlreadyExisted;
    HRESULT hr;

    ok(test1.GetData() == NULL, "Expected NULL, got %p\n", test1.GetData());
    ok(test3.GetData() == NULL, "Expected NULL, got %p\n", test3.GetData());
    ok(test1.GetHandle() == NULL, "Expected NULL, got %p\n", test1.GetHandle());
    ok(test3.GetHandle() == NULL, "Expected NULL, got %p\n", test3.GetHandle());
    ok(test1.GetMappingSize() == 0, "Expected 0, got %lu\n", test1.GetMappingSize());
    ok(test3.GetMappingSize() == 0, "Expected 0, got %lu\n", test3.GetMappingSize());

    test1 = test1;
    //test1 = test2;  // Asserts on orig.m_pData != NULL, same with CopyFrom
    //test1 = test3;  // does not compile
    hr = test1.Unmap();
    ok(hr == S_OK, "Expected S_OK, got 0x%lx\n", hr);

    // Asserts on name == NULL
    hr = test1.OpenMapping(_T("TEST_MAPPING"), 123, 0, FILE_MAP_ALL_ACCESS);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got 0x%lx\n", hr);
    ok(test1.GetData() == NULL, "Expected NULL, got %p\n", test1.GetData());
    ok(test1.GetHandle() == NULL, "Expected NULL, got %p\n", test1.GetHandle());
    ok(test1.GetMappingSize() == 123, "Expected 123, got %lu\n", test1.GetMappingSize());

    hr = test1.Unmap();
    ok(hr == S_OK, "Expected S_OK, got 0x%lx\n", hr);
    ok(test1.GetData() == NULL, "Expected NULL, got %p\n", test1.GetData());
    ok(test1.GetHandle() == NULL, "Expected NULL, got %p\n", test1.GetHandle());
    ok(test1.GetMappingSize() == 123, "Expected 123, got %lu\n", test1.GetMappingSize());

    bAlreadyExisted = 123;
    hr = test1.MapSharedMem(sizeof(TestData), _T("TEST_MAPPING"), &bAlreadyExisted, (LPSECURITY_ATTRIBUTES)0, PAGE_READWRITE, FILE_MAP_ALL_ACCESS);
    ok(hr == S_OK, "Expected S_OK, got 0x%lx\n", hr);
    ok(test1.GetData() != NULL, "Expected ptr, got %p\n", test1.GetData());
    ok(test1.GetHandle() != NULL, "Expected handle, got %p\n", test1.GetHandle());
    ok(test1.GetMappingSize() == sizeof(TestData), "Expected sizeof(TestData), got %lu\n", test1.GetMappingSize());
    ok(bAlreadyExisted == FALSE, "Expected FALSE, got %u\n", bAlreadyExisted);

    if (test1.GetData())
    {
        memset(test1.GetData(), 0x35, sizeof(TestData));
    }

    hr = test2.CopyFrom(test1);
    ok(hr == S_OK, "Expected S_OK, got 0x%lx\n", hr);
    ok(test2.GetData() != NULL, "Expected ptr, got %p\n", test2.GetData());
    ok(test2.GetHandle() != NULL, "Expected handle, got %p\n", test2.GetHandle());
    ok(test2.GetMappingSize() == sizeof(TestData), "Expected sizeof(TestData), got %lu\n", test2.GetMappingSize());

    // test1 is not closed:
    ok(test1.GetData() != NULL, "Expected ptr, got %p\n", test1.GetData());
    ok(test1.GetHandle() != NULL, "Expected handle, got %p\n", test1.GetHandle());
    ok(test1.GetMappingSize() == sizeof(TestData), "Expected sizeof(TestData), got %lu\n", test1.GetMappingSize());

    // test2 does not equal test1
    ok(test1.GetData() != test2.GetData(), "Expected different ptrs\n");
    ok(test1.GetHandle() != test2.GetHandle(), "Expected different handles\n");

    TestData* t1 = test1;
    TestData* t2 = test2;
    if (t1 && t2)
    {
        ok(t1->data[0] == 0x35353535, "Expected 0x35353535, got 0x%x\n", t1->data[0]);
        ok(t2->data[0] == 0x35353535, "Expected 0x35353535, got 0x%x\n", t2->data[0]);

        t1->data[0] = 0xbeefbeef;
        ok(t1->data[0] == (int)0xbeefbeef, "Expected 0xbeefbeef, got 0x%x\n", t1->data[0]);
        ok(t2->data[0] == (int)0xbeefbeef, "Expected 0xbeefbeef, got 0x%x\n", t2->data[0]);
    }

    hr = test3.OpenMapping(_T("TEST_MAPPING"), sizeof(TestData), offsetof(TestData, data[1]));
    ok(hr == HRESULT_FROM_WIN32(ERROR_MAPPED_ALIGNMENT), "Expected HRESULT_FROM_WIN32(ERROR_MAPPED_ALIGNMENT), got 0x%lx\n", hr);
    ok(test3.GetData() == NULL, "Expected NULL, got %p\n", test3.GetData());
    ok(test3.GetHandle() == NULL, "Expected NULL, got %p\n", test3.GetHandle());
    ok(test3.GetMappingSize() == sizeof(TestData), "Expected sizeof(TestData), got %lu\n", test3.GetMappingSize());

    hr = test2.Unmap();
    ok(hr == S_OK, "Expected S_OK, got 0x%lx\n", hr);
    ok(test2.GetData() == NULL, "Expected NULL, got %p\n", test2.GetData());
    ok(test2.GetHandle() == NULL, "Expected NULL, got %p\n", test2.GetHandle());
    ok(test2.GetMappingSize() == sizeof(TestData), "Expected sizeof(TestData), got %lu\n", test2.GetMappingSize());

    bAlreadyExisted = 123;
    // Wrong access flag
    hr = test2.MapSharedMem(sizeof(TestData), _T("TEST_MAPPING"), &bAlreadyExisted, (LPSECURITY_ATTRIBUTES)0, PAGE_EXECUTE_READ, FILE_MAP_ALL_ACCESS);
    ok(hr == E_ACCESSDENIED, "Expected E_ACCESSDENIED, got 0x%lx\n", hr);
    ok(test2.GetData() == NULL, "Expected NULL, got %p\n", test2.GetData());
    ok(test2.GetHandle() == NULL, "Expected NULL, got %p\n", test2.GetHandle());
    ok(test2.GetMappingSize() == sizeof(TestData), "Expected sizeof(TestData), got %lu\n", test2.GetMappingSize());
    ok(bAlreadyExisted == TRUE, "Expected TRUE, got %u\n", bAlreadyExisted);

    bAlreadyExisted = 123;
    hr = test2.MapSharedMem(sizeof(TestData), _T("TEST_MAPPING"), &bAlreadyExisted, (LPSECURITY_ATTRIBUTES)0, PAGE_READWRITE, FILE_MAP_ALL_ACCESS);
    ok(hr == S_OK, "Expected S_OK, got 0x%lx\n", hr);
    ok(test2.GetData() != NULL, "Expected ptr, got %p\n", test2.GetData());
    ok(test2.GetHandle() != NULL, "Expected handle, got %p\n", test2.GetHandle());
    ok(test2.GetMappingSize() == sizeof(TestData), "Expected sizeof(TestData), got %lu\n", test2.GetMappingSize());
    ok(bAlreadyExisted == TRUE, "Expected TRUE, got %u\n", bAlreadyExisted);

    // test2 does not equal test1
    ok(test1.GetData() != test2.GetData(), "Expected different ptrs\n");
    ok(test1.GetHandle() != test2.GetHandle(), "Expected different handles\n");

    t2 = test2;
    if (t1 && t2)
    {
        ok(t1->data[0] == (int)0xbeefbeef, "Expected 0xbeefbeef, got 0x%x\n", t1->data[0]);
        ok(t2->data[0] == (int)0xbeefbeef, "Expected 0xbeefbeef, got 0x%x\n", t2->data[0]);

        t1->data[0] = 0xdeaddead;
        ok(t1->data[0] == (int)0xdeaddead, "Expected 0xdeaddead, got 0x%x\n", t1->data[0]);
        ok(t2->data[0] == (int)0xdeaddead, "Expected 0xdeaddead, got 0x%x\n", t2->data[0]);
    }
}

static void test_FileMapping()
{
    WCHAR Buf[MAX_PATH] = {0};
    ULARGE_INTEGER FileSize;

    GetModuleFileNameW(NULL, Buf, _countof(Buf));

    CAtlFileMapping<IMAGE_DOS_HEADER> test1, test2;
    HRESULT hr;

    {
        CHandle hFile(CreateFileW(Buf, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL));
        ok(hFile != INVALID_HANDLE_VALUE, "Could not open %S, aborting test\n", Buf);
        if (hFile == INVALID_HANDLE_VALUE)
            return;

        FileSize.LowPart = GetFileSize(hFile, &FileSize.HighPart);

        hr = test1.MapFile(hFile, 0, 0, PAGE_READONLY, FILE_MAP_READ);
        ok(hr == S_OK, "Expected S_OK, got 0x%lx\n", hr);
        ok(test1.GetData() != NULL, "Expected ptr, got %p\n", test1.GetData());
        ok(test1.GetHandle() != NULL, "Expected handle, got %p\n", test1.GetHandle());
        ok(test1.GetMappingSize() == FileSize.LowPart, "Expected %lu, got %lu\n", FileSize.LowPart, test1.GetMappingSize());

        hr = test2.MapFile(hFile, 0, 0x10000, PAGE_READONLY, FILE_MAP_READ);
        ok(hr == S_OK, "Expected S_OK, got 0x%lx\n", hr);
        ok(test2.GetData() != NULL, "Expected ptr, got %p\n", test2.GetData());
        ok(test2.GetHandle() != NULL, "Expected handle, got %p\n", test2.GetHandle());
        // Offset is subtracted
        ok(test2.GetMappingSize() == FileSize.LowPart - 0x10000, "Expected %lu, got %lu\n", FileSize.LowPart-0x10000, test2.GetMappingSize());

        hr = test1.Unmap();
        ok(hr == S_OK, "Expected S_OK, got 0x%lx\n", hr);
        ok(test1.GetData() == NULL, "Expected NULL, got %p\n", test1.GetData());
        ok(test1.GetHandle() == NULL, "Expected NULL, got %p\n", test1.GetHandle());
        ok(test1.GetMappingSize() == FileSize.LowPart, "Expected %lu, got %lu\n", FileSize.LowPart, test1.GetMappingSize());

        hr = test1.MapFile(hFile, 0x1000);
        ok(hr == S_OK, "Expected S_OK, got 0x%lx\n", hr);
        ok(test1.GetData() != NULL, "Expected ptr, got %p\n", test1.GetData());
        ok(test1.GetHandle() != NULL, "Expected handle, got %p\n", test1.GetHandle());
        ok(test1.GetMappingSize() == 0x1000, "Expected 0x1000, got %lu\n", test1.GetMappingSize());
    }

    // We can still access it after the file is closed
    PIMAGE_DOS_HEADER dos = test1;
    if (dos)
    {
        ok(dos->e_magic == IMAGE_DOS_SIGNATURE, "Expected IMAGE_DOS_SIGNATURE, got 0x%x\n", dos->e_magic);
    }
}


START_TEST(CAtlFileMapping)
{
    test_SharedMem();
    test_FileMapping();

#ifndef HAVE_APITEST
    printf("CAtlFile: %i tests executed (0 marked as todo, %i failures), %i skipped.\n", g_tests_executed, g_tests_failed, g_tests_skipped);
    return g_tests_failed;
#endif
}
