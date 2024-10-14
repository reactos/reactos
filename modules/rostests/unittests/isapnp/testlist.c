/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test list for the ISA PnP bus driver
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

#define STANDALONE
#include <apitest.h>

extern void func_Resources(void);

const struct test winetest_testlist[] =
{
    { "Resources", func_Resources },
    { 0, 0 }
};
