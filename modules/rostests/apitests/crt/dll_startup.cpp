/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for static C++ object construction/destruction in a DLL
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>
#include "dll_startup.h"

// we test the initial value of m_uninit variable here, so this is required
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

static struct counter_values counter_values =
{
    0, 0, 0, 0, 5656, 0, 0
};
static struct counter_values *p_counter_values;

static struct init_static
{
    int m_uninit;
    int m_counter;

    init_static() :
        m_counter(2)
    {
        counter_values.static_construct_counter_at_startup = counter_values.static_construct_counter;
        counter_values.m_uninit_at_startup = m_uninit;
        counter_values.static_construct_counter++;
        m_uninit++;
    }

    ~init_static()
    {
        p_counter_values->dtor_counter++;
    }
} init_static;

extern "C"
{
SET_COUNTER_VALUES_POINTER SetCounterValuesPointer;
void
WINAPI
SetCounterValuesPointer(
    _Out_ struct counter_values *pcv)
{
    p_counter_values = pcv;
    memcpy(pcv, &counter_values, sizeof(counter_values));
}

BOOL
WINAPI
DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD fdwReason,
    _In_ PVOID pvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        counter_values.m_uninit = init_static.m_uninit;
        counter_values.m_counter = init_static.m_counter;
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        p_counter_values->dtor_counter_at_detach = p_counter_values->dtor_counter;
    }
    return TRUE;
}

}
