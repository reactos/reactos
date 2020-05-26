/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for PrivateExtractIcons
 * PROGRAMMER:      Hermes Belusca-Maito
 */

#include "precomp.h"

static struct
{
    PCWSTR FilePath;
    UINT cIcons;        // Return value from PrivateExtractIconsW
    BOOL bhIconValid;   // Whether or not the returned icon handle is not NULL.
} IconTests[] =
{
    /* Executables with icons */
    {L"notepad.exe", 1, TRUE},
    {L"%SystemRoot%\\System32\\cmd.exe", 1, TRUE},

    /* Executable without icon */
    {L"%SystemRoot%\\System32\\autochk.exe", 0, FALSE},

    /* Existing file */
    {L"%SystemRoot%\\System32\\shell32.dll", 1, TRUE},

    /* Non-existing files */
    {L"%SystemRoot%\\non-existent-file.sdf", 0xFFFFFFFF, FALSE},
};

START_TEST(PrivateExtractIcons)
{
    HICON ahIcon;
    UINT i, aIconId, cIcons;

    for (i = 0; i < _countof(IconTests); ++i)
    {
        /* Always test extraction of the FIRST icon (index 0) */
        ahIcon = (HICON)UlongToHandle(0xdeadbeef);
        aIconId = 0xdeadbeef;
        cIcons = PrivateExtractIconsW(IconTests[i].FilePath, 0, 16, 16, &ahIcon, &aIconId, 1, 0);
        ok(cIcons == IconTests[i].cIcons, "PrivateExtractIconsW(%u): got %u, expected %u\n", i, cIcons, IconTests[i].cIcons);
        ok(ahIcon != (HICON)UlongToHandle(0xdeadbeef), "PrivateExtractIconsW(%u): icon not set\n", i);
        ok((IconTests[i].bhIconValid && ahIcon) || (!IconTests[i].bhIconValid && !ahIcon),
            "PrivateExtractIconsW(%u): icon expected to be %s, but got 0x%p\n",
            i, IconTests[i].bhIconValid ? "valid" : "not valid", ahIcon);
        if (cIcons == 0xFFFFFFFF)
        {
            ok(aIconId == 0xdeadbeef, "PrivateExtractIconsW(%u): id should not be set\n", i);
        }
        else
        {
            ok(aIconId != 0xdeadbeef, "PrivateExtractIconsW(%u): id not set\n", i);
        }
        if (ahIcon && ahIcon != (HICON)UlongToHandle(0xdeadbeef))
            DestroyIcon(ahIcon);
    }
}
