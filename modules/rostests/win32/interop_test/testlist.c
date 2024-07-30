/*
 * PROJECT:     ReactOS interoperability tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     List of tests
 * COPYRIGHT:   Copyright 2024 Stanislav Motylkov <x86corez@gmail.com>
 */

#define STANDALONE
#include <wine/test.h>

extern void func_LocaleTests(void);

const struct test winetest_testlist[] =
{
    { "LocaleTests", func_LocaleTests },

    { 0, 0 }
};
