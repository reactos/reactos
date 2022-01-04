/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for ExtractIconEx routine
 * COPYRIGHT:   Copyright 2019 George Bișoc (george.bisoc@reactos.org)
 */

#include "shelltest.h"

typedef struct
{
    PCWSTR pszFilePath;
    UINT nIcons;
} EXTRACTICONTESTS;

EXTRACTICONTESTS IconTests[] =
{
    /* Executable file with icon */
    {L"%SystemRoot%\\System32\\cmd.exe", 1},

    /* Executable file without icon */
    {L"%SystemRoot%\\System32\\autochk.exe", 0},

    /* Non-existing files */
    {L"%SystemRoot%\\non-existent-file.sdf", 0}
};

START_TEST(ExtractIconEx)
{
    UINT i, nReturnedIcons;

    for (i = 0; i < _countof(IconTests); ++i)
    {
        nReturnedIcons = ExtractIconExW(IconTests[i].pszFilePath, 0, NULL, NULL, IconTests[i].nIcons);
        ok(nReturnedIcons == IconTests[i].nIcons, "ExtractIconExW(%u): Expected %u icons, got %u\n", i, IconTests[i].nIcons, nReturnedIcons);
    }
}
