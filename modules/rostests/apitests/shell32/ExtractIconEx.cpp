/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for ExtractIconEx routine
 * COPYRIGHT:   Copyright 2019 George Bișoc (george.bisoc@reactos.org)
 */

#include "shelltest.h"
#include <stdio.h>
#include <debug.h>

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
    {L"%SystemRoot%\\non-existent-file.sdf", 0},

    /* Multiple icons in the same EXE file (18 icons) */
    {L"%SystemRoot%\\explorer.exe", 18},

    /* Multiple icons in the same ICO file (6 icons)
     * Per MS: If the file is an .ico file the return value is 1." */
    {L"sysicon.ico", 1},

    /* ICO file with both normal and PNG icons */
    {L"ROS.ico", 1}
};

START_TEST(ExtractIconEx)
{
    UINT i, nReturnedIcons, nExtractedIcons, iSize;
    HRSRC hRes;
    HGLOBAL hData;
    LPVOID lpResLock;
    LPCSTR sData;
    FILE *fout;
    CHAR FileName[2][13] = { "ROS.ico", "sysicon.ico" };

    for (i = 2; i < 4; i++) // Point to resources 2 and 3
    {
        hRes = FindResourceW(NULL, MAKEINTRESOURCEW(i), MAKEINTRESOURCEW(RT_RCDATA));
        if (hRes == NULL)
        {
            skip("Could not locate resource (%d).\n", i);
            return;
        }
        iSize = SizeofResource(NULL, hRes);

        hData = LoadResource(NULL, hRes);
        if (hData == NULL)
        {
            skip("Could not load resource (%d).", i);
            return;
        }

        // Lock the resource into global memory.
        lpResLock = LockResource(hData);
        if (lpResLock == NULL)
        {
            skip("Could not lock resource (%d).", i);
            return;
        }

        // Get location of resource in memory
        sData = (LPSTR)lpResLock;

        fout = fopen(FileName[i - 2],"wb");
        fwrite(sData, iSize, 1, fout);
        fclose(fout);
    }
    /* Check count of icons returned */
    for (i = 0; i < _countof(IconTests); ++i)
    {
        nReturnedIcons = ExtractIconExW(IconTests[i].pszFilePath, -1, NULL, NULL, 0);
        ok(nReturnedIcons == IconTests[i].nIcons, "ExtractIconExW(%u): Expected %u icons, got %u\n", i, IconTests[i].nIcons, nReturnedIcons);
    }

    /* Check if the 0th icon can be extracted successfully */
    for (i = 0; i < _countof(IconTests); ++i)
    {
        nExtractedIcons = ExtractIconExW(IconTests[i].pszFilePath, 0, NULL, NULL, 1);
        ok(nExtractedIcons == IconTests[i].nIcons, "ExtractIconExW(%u): Expected %u icons, got %u\n", i, IconTests[i].nIcons, nExtractedIcons);
    }
}
