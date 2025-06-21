/*
 * PROJECT:     ReactOS PathCch Library - Unit-tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     C compilation tests
 * COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"

#include "pathcch_compile.inc"

// See pathcch_compile.cpp
extern void test_CPP_PathCch(void);

START_TEST(PathCchCompileTest)
{
    test_C_PathCch();
    test_CPP_PathCch();
}

/* EOF */
