/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for PrivateExtractIcons
 * PROGRAMMER:      Hermes Belusca-Maito
 *                  Doug Lyons <douglyons@douglyons.com>
 */

#include "precomp.h"
#include <stdio.h>

BOOL FileExists(LPCSTR FileName)
{
    FILE *fp = NULL;

    fp = fopen(FileName, "r");
    if (fp != NULL)
    {
        fclose(fp);
        return TRUE;
    }
    return FALSE;
}

BOOL ResourceToFile(INT i, LPCSTR FileName)
{
    FILE *fout;
    HGLOBAL hData;
    HRSRC hRes;
    LPVOID lpResLock;
    UINT iSize;

    if (FileExists(FileName))
    {
        skip("'%s' already exists. Exiting now\n", FileName);
        return FALSE;
    }

    hRes = FindResourceW(NULL, MAKEINTRESOURCEW(i), MAKEINTRESOURCEW(RT_RCDATA));
    if (hRes == NULL)
    {
        skip("Could not locate resource (%d). Exiting now\n", i);
        return FALSE;
    }

    iSize = SizeofResource(NULL, hRes);

    hData = LoadResource(NULL, hRes);
    if (hData == NULL)
    {
        skip("Could not load resource (%d). Exiting now\n", i);
        return FALSE;
    }

    // Lock the resource into global memory.
    lpResLock = LockResource(hData);
    if (lpResLock == NULL)
    {
        skip("Could not lock resource (%d). Exiting now\n", i);
        return FALSE;
    }

    fout = fopen(FileName, "wb");
    fwrite(lpResLock, iSize, 1, fout);
    fclose(fout);
    return TRUE;
}

static struct
{
    PCWSTR FilePath;
    UINT cIcons;        // Return value from PrivateExtractIconsW
    UINT cTotIcons;     // Return value of Total Icons in File
    BOOL bhIconValid;   // Whether or not the returned icon handle is not NULL.
} IconTests[] =
{
    /* Executables with icons */
    {L"notepad.exe", 1, 1, TRUE},
    {L"%SystemRoot%\\System32\\cmd.exe", 1, 1, TRUE},

    /* Executable without icon */
    {L"%SystemRoot%\\System32\\autochk.exe", 0, 0, FALSE},

    /* Existing file 233 is ROS Only */
    {L"%SystemRoot%\\System32\\shell32.dll", 1, 233, TRUE},

    /* Non-existing files */
    {L"%SystemRoot%\\non-existent-file.sdf", 0xFFFFFFFF, 0, FALSE},
    /* Multiple icons in the same EXE file (18 icons) */
    {L"%SystemRoot%\\explorer.exe", 1, 18, TRUE},

    /* Multiple icons in the same ICO file (6 icons) */
    {L"%SystemRoot%\\bin\\sysicon.ico", 1, 1, TRUE},

    /* ICO file with both normal and PNG icons */
    {L"%SystemRoot%\\bin\\ROS.ico", 1, 1, TRUE},
};

START_TEST(PrivateExtractIcons)
{
    HICON ahIcon;
    UINT i, aIconId, cIcons, cTotIcons;
    CHAR FileName[2][13] = { "ROS.ico", "sysicon.ico" };

    if (!ResourceToFile(1, FileName[0]))
        return;
    if (!ResourceToFile(2, FileName[1]))
        return;

    for (i = 0; i < _countof(IconTests); ++i)
    {
        /* Always test extraction of the FIRST icon (index 0) */
        ahIcon = (HICON)UlongToHandle(0xdeadbeef);
        aIconId = 0xdeadbeef;

        /* Get total number of icons in file.
         * None of the hard numbers in the function matter since we have
         * the two NULL's for the Icon Handle and Count to be set. */
        cTotIcons = PrivateExtractIconsW(IconTests[i].FilePath, 0, 16, 16, NULL, NULL, 0, 0);
        if (i != 3) // not shell32.dll
            ok(cTotIcons == IconTests[i].cTotIcons, "PrivateExtractIconsW(%u): "
               "got %u, expected %u\n", i, cTotIcons, IconTests[i].cTotIcons);
        else /* ROS is 233, W2K2SP2 is 239 */
            ok(cTotIcons > 232 && cTotIcons < 240, "PrivateExtractIconsW(%u): "
               "got %u, expected %u\n", i, cTotIcons, IconTests[i].cTotIcons);

        /* Get count of icons requested  which is 1, unless error. */
        cIcons = PrivateExtractIconsW(IconTests[i].FilePath, 0, 16, 16, &ahIcon, &aIconId, 1, 0);
        ok(cIcons == IconTests[i].cIcons, "PrivateExtractIconsW(%u): got %u, expected %u\n", i, cIcons, IconTests[i].cIcons);
        ok(ahIcon != (HICON)UlongToHandle(0xdeadbeef), "PrivateExtractIconsW(%u): icon not set\n", i);
        ok((IconTests[i].bhIconValid && ahIcon) || (!IconTests[i].bhIconValid && !ahIcon),
            "PrivateExtractIconsW(%u): icon expected to be %s, but got 0x%p\n",
            i, IconTests[i].bhIconValid ? "valid" : "not valid", ahIcon);
        if (cIcons == 0xFFFFFFFF)
        {
            ok(aIconId == 0xdeadbeef,
               "PrivateExtractIconsW(%u): id should not be set to 0x%x\n",
               i, aIconId);
        }
        else
        {
            ok(aIconId != 0xdeadbeef, "PrivateExtractIconsW(%u): id not set\n", i);
        }
        if (ahIcon && ahIcon != (HICON)UlongToHandle(0xdeadbeef))
            DestroyIcon(ahIcon);
    }

    DeleteFileA(FileName[0]);
    DeleteFileA(FileName[1]);
}
