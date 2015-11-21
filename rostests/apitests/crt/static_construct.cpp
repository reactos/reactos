/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for static C++ object construction
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

extern "C"
{
extern int static_init_counter;

static int static_init_counter_at_startup;
static int static_construct_counter_at_startup;

int static_construct_counter = 789;
}

struct init_static
{
    int m_counter;

    init_static() :
        m_counter(2)
    {
        static_init_counter_at_startup = static_init_counter;
        static_construct_counter_at_startup = static_construct_counter;
        static_construct_counter++;
    }
} init_static;

START_TEST(static_construct)
{
    ok(static_init_counter_at_startup == 123, "static_init_counter at startup: %d\n", static_init_counter_at_startup);
    ok(static_construct_counter_at_startup == 789, "static_construct_counter at startup: %d\n", static_construct_counter_at_startup);

    ok(static_init_counter == 123, "static_init_counter: %d\n", static_init_counter);

    ok(static_construct_counter == 790, "static_construct_counter: %d\n", static_construct_counter);
    ok(init_static.m_counter == 2, "init_static.m_counter: %d\n", init_static.m_counter);
}
