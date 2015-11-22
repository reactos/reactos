/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for static C++ object construction
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
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

START_TEST(static_construct)
{
    TestInitStatic();
    TestDllStartup();
}
