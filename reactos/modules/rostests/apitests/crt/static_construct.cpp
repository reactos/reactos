/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for static C++ object construction
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 *                  Mark Jansen
 */

#include <apitest.h>
#include <strsafe.h>
#include "dll_startup.h"

extern "C"
{
extern int static_init_counter;

static int static_init_counter_at_startup;
static int static_construct_counter_at_startup;
static int m_uninit_at_startup;

int static_construct_counter = 789;
}

static struct init_static
{
    int m_uninit;
    int m_counter;

    init_static() :
        m_counter(2)
    {
        static_init_counter_at_startup = static_init_counter;
        static_construct_counter_at_startup = static_construct_counter;
        m_uninit_at_startup = m_uninit;
        static_construct_counter++;
        m_uninit++;
    }
} init_static;

static
VOID
TestInitStatic(VOID)
{
    ok(static_init_counter_at_startup == 123, "static_init_counter at startup: %d\n", static_init_counter_at_startup);
    ok(static_construct_counter_at_startup == 789, "static_construct_counter at startup: %d\n", static_construct_counter_at_startup);
    ok(m_uninit_at_startup == 0, "init_static.m_uninit at startup: %d\n", m_uninit_at_startup);

    ok(static_init_counter == 123, "static_init_counter: %d\n", static_init_counter);

    ok(static_construct_counter == 790, "static_construct_counter: %d\n", static_construct_counter);
    ok(init_static.m_counter == 2, "init_static.m_counter: %d\n", init_static.m_counter);
    ok(init_static.m_uninit == 1, "init_static.m_uninit: %d\n", init_static.m_uninit);
}

static
VOID
TestDllStartup(VOID)
{
#if defined(TEST_MSVCRT)
    const PCWSTR DllName = L"msvcrt_crt_dll_startup.dll";
#elif defined(TEST_STATIC_CRT)
    const PCWSTR DllName = L"static_crt_dll_startup.dll";
#else
#error This test only makes sense for static CRT and msvcrt.dll
#endif
    WCHAR DllPath[MAX_PATH];
    GetModuleFileNameW(NULL, DllPath, _countof(DllPath));
    wcsrchr(DllPath, L'\\')[1] = UNICODE_NULL;
    StringCchCatW(DllPath, _countof(DllPath), DllName);

    HMODULE hDll = LoadLibraryW(DllPath);
    if (hDll == NULL)
    {
        skip("Helper dll not found\n");
        return;
    }
    SET_COUNTER_VALUES_POINTER *pSetCounterValuesPointer = reinterpret_cast<SET_COUNTER_VALUES_POINTER*>(GetProcAddress(hDll, "SetCounterValuesPointer"));
    if (pSetCounterValuesPointer == NULL)
    {
        skip("Helper function not found\n");
        FreeLibrary(hDll);
        return;
    }
    counter_values values;
    pSetCounterValuesPointer(&values);
    ok(values.m_uninit_at_startup == 0, "m_uninit_at_startup = %d\n", values.m_uninit_at_startup);
    ok(values.m_uninit == 1, "m_uninit = %d\n", values.m_uninit);
    ok(values.m_counter == 2, "m_counter = %d\n", values.m_counter);
    ok(values.static_construct_counter_at_startup == 5656, "static_construct_counter_at_startup = %d\n", values.static_construct_counter_at_startup);
    ok(values.static_construct_counter == 5657, "static_construct_counter = %d\n", values.static_construct_counter);
    ok(values.dtor_counter_at_detach == 0, "dtor_counter_at_detach = %d\n", values.dtor_counter_at_detach);
    ok(values.dtor_counter == 0, "dtor_counter = %d\n", values.dtor_counter);
    values.dtor_counter_at_detach = 78789;
    values.dtor_counter = 7878;
    FreeLibrary(hDll);
    ok(values.m_uninit_at_startup == 0, "m_uninit_at_startup = %d\n", values.m_uninit_at_startup);
    ok(values.m_uninit == 1, "m_uninit = %d\n", values.m_uninit);
    ok(values.m_counter == 2, "m_counter = %d\n", values.m_counter);
    ok(values.static_construct_counter_at_startup == 5656, "static_construct_counter_at_startup = %d\n", values.static_construct_counter_at_startup);
    ok(values.static_construct_counter == 5657, "static_construct_counter = %d\n", values.static_construct_counter);
    ok(values.dtor_counter_at_detach == 7878, "dtor_counter_at_detach = %d\n", values.dtor_counter_at_detach);
    ok(values.dtor_counter == 7879, "dtor_counter = %d\n", values.dtor_counter);
}

struct shared_memory
{
    int init_count;
    int uninit_count;
};

static HANDLE g_FileMapping = NULL;
static BOOL g_CreatedFileMapping = FALSE;
static shared_memory* g_Memory = NULL;

#define MAPPING_NAME L"crt_apitest_static_construct"

static void map_memory()
{
    if (g_FileMapping)
        return;

    g_FileMapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, MAPPING_NAME);
    if (g_FileMapping)
    {
        g_CreatedFileMapping = FALSE;
    }
    else
    {
        g_FileMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(shared_memory), MAPPING_NAME);
        g_CreatedFileMapping = TRUE;
    }
    if (g_FileMapping == NULL)
    {
        skip("Could not map shared memory\n");
        return;
    }
    g_Memory = static_cast<shared_memory*>(MapViewOfFile(g_FileMapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(shared_memory)));
    if (g_Memory == NULL)
    {
        skip("Could not map view of shared memory\n");
        CloseHandle(g_FileMapping);
        g_FileMapping = NULL;
    }
    if (g_CreatedFileMapping)
        ZeroMemory(g_Memory, sizeof(shared_memory));
}

static void unmap_memory()
{
    // we do not clean the mapping in the child, since we want to count all dtor's!
    if (g_FileMapping && g_CreatedFileMapping)
    {
        UnmapViewOfFile(g_Memory);
        CloseHandle(g_FileMapping);
        g_Memory = NULL;
        g_FileMapping = NULL;
    }
}

static struct shared_mem_static
{
    shared_mem_static()
    {
        map_memory();
        if (g_Memory)
            g_Memory->init_count++;
    }

    ~shared_mem_static()
    {
        if (g_Memory)
            g_Memory->uninit_count++;
        unmap_memory();
    }

} shared_mem_static;

static
VOID
TestStaticDestruct(VOID)
{
    ok(g_Memory != NULL, "Expected the mapping to be in place\n");
    ok(g_CreatedFileMapping == TRUE, "Expected to create a new shared section!\n");
    if (g_Memory == NULL)
    {
        skip("Can't proceed without file mapping\n");
        return;
    }
    ok(g_Memory->init_count == 1, "Expected init_count to be 1, was: %d\n", g_Memory->init_count);
    ok(g_Memory->uninit_count == 0, "Expected uninit_count to be 0, was: %d\n", g_Memory->uninit_count);

    WCHAR path[MAX_PATH];
    // we just need an extra argument to tell the test it's only running to increment the dtor count :)
    GetModuleFileNameW(NULL, path, _countof(path));
    WCHAR buf[MAX_PATH+40];
    StringCchPrintfW(buf, _countof(buf), L"\"%ls\" static_construct dummy", path);

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    BOOL created = CreateProcessW(NULL, buf, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(created, "Expected CreateProcess to succeed\n");
    if (created)
    {
        winetest_wait_child_process(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        ok(g_Memory->init_count == 2, "Expected init_count to be 2, was: %d\n", g_Memory->init_count);
        ok(g_Memory->uninit_count == 1, "Expected uninit_count to be 1, was: %d\n", g_Memory->uninit_count);
    }
}

START_TEST(static_construct)
{
    char **argv;
    int argc = winetest_get_mainargs(&argv);

    if (argc >= 3)
    {
        // we are just here to increment the reference count in the shared section!
        ok(g_Memory != NULL, "Expected the shared memory to be mapped!\n");
        ok(g_CreatedFileMapping == FALSE, "Expected the shared memory to be created by my parent!\n");
        return;
    }
    
    TestInitStatic();
    TestDllStartup();
    TestStaticDestruct();
}
