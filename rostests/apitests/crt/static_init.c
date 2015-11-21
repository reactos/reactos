/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for static variable initialization
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

extern int static_construct_counter;

int static_init_counter = 123;

START_TEST(static_init)
{    
    ok(static_init_counter == 123, "static_init_counter: %d\n", static_init_counter);
    ok(static_construct_counter == 790, "static_construct_counter: %d\n", static_construct_counter);
}
