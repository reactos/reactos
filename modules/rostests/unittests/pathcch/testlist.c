/*
 * PROJECT:     ReactOS PathCch Library - Unit-tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     List of tests
 * COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#define STANDALONE
#include <apitest.h>

extern void func_PathCchCompileTest(void);

const struct test winetest_testlist[] =
{
    { "PathCchCompileTest", func_PathCchCompileTest },
    { 0, 0 }
};
