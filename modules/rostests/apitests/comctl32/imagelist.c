/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for imagelist
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#include "wine/test.h"
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <comctl32_undoc.h>

#define WinVerMajor() LOBYTE(GetVersion())

#define ILC_COLORMASK 0xfe
#define IL_IMGSIZE 16

static BOOL IL_IsValid(HIMAGELIST himl)
{
    int w = -42, h;
    if (!himl || IsBadReadPtr(himl, sizeof(void*)))
        return FALSE;
    return ImageList_GetIconSize(himl, &w, &h) && w != -42;
}

static HRESULT IL_Destroy(HIMAGELIST himl)
{
    if (himl && !IL_IsValid(himl))
        return E_INVALIDARG;
    return ImageList_Destroy(himl) ? S_OK : S_FALSE;
}

static inline HIMAGELIST IL_Create(UINT flags)
{
    return ImageList_Create(IL_IMGSIZE, IL_IMGSIZE, flags, 1, 0);
}

static UINT IL_CalculateOtherBpp(UINT ilc)
{
    UINT bpp = (ilc & ILC_COLORMASK) == ILC_COLOR32 ? ILC_COLOR16 : ILC_COLOR32;
    return (ilc & ~ILC_COLORMASK) | bpp;
}

static BOOL IL_AddImagesForTest(HIMAGELIST himl)
{
    int idx = -1;
    HINSTANCE hInst = LoadLibraryW(L"USER32");
    if (!hInst)
        return FALSE;
    HICON hIco = (HICON)LoadImage(hInst, MAKEINTRESOURCE(100),
                                  IMAGE_ICON, IL_IMGSIZE, IL_IMGSIZE, 0);
    if (hIco)
    {
        idx = ImageList_AddIcon(himl, hIco);
        DestroyIcon(hIco);
    }
    FreeLibrary(hInst);
    return idx != -1;
}

static void Test_SystemIL(void)
{
    const UINT flags = ILC_COLOR16 | ILC_MASK;
    HIMAGELIST himl;

    himl = IL_Create(flags);
    ok(IL_Destroy(himl) == S_OK && !IL_IsValid(himl), "Can destroy normal\n");

    /* You can (sometimes) destroy a system imagelist!
     * On Win9x it destroys it for all processes according to
     * https://sporks.space/2021/09/18/notes-on-the-system-image-list/ and
     * https://www.catch22.net/tuts/win32/system-image-list/
     */
    himl = IL_Create(flags | ILC_SYSTEM);
    if (WinVerMajor() >= 6)
        ok(IL_Destroy(himl) == S_FALSE && IL_IsValid(himl), "Can't destroy system\n");
    else
        ok(IL_Destroy(himl) == S_OK && !IL_IsValid(himl), "Can destroy system\n");
}

static void Test_Flags(void)
{
    const UINT flags = ILC_COLOR16 | ILC_MASK;
    UINT flagsIn, flagsOut;
    HIMAGELIST himl;

    himl = IL_Create(flagsIn = flags);
    flagsOut = ImageList_GetFlags(himl);
    if (himl ? TRUE : (skip("Could not initialize\n"), FALSE))
    {
        ok((flagsOut & ILC_COLORMASK) == (flagsIn & ILC_COLORMASK), "ILC_COLOR\n");
        ok(!(flagsOut & ILC_SYSTEM), "!ILC_SYSTEM\n");

        ok(IL_AddImagesForTest(himl), "Initialize\n");
        flagsIn = IL_CalculateOtherBpp(flagsIn);
        ok(ImageList_SetFlags(himl, flagsIn), "Can change BPP\n");
        ok(ImageList_GetImageCount(himl) == 0, "SetFlags deletes all images\n");

        ok(IL_AddImagesForTest(himl), "Initialize\n");
        ok(ImageList_SetFlags(himl, ImageList_GetFlags(himl)), "Can set same flags\n");
        if (WinVerMajor() >= 6)
        {
            ok(ImageList_GetImageCount(himl) != 0, "SetFlags does not delete with same flags\n");
            ok(ImageList_SetFlags(himl, flagsIn ^ ILC_SYSTEM), "Can change ILC_SYSTEM\n");
        }
        else
        {
            ok(ImageList_GetImageCount(himl) == 0, "SetFlags deletes all images even with same flags\n");
            ok(!ImageList_SetFlags(himl, flagsIn ^ ILC_SYSTEM), "Can't change ILC_SYSTEM\n");
        }

        IL_Destroy(himl);
    }

    himl = IL_Create(flagsIn = flags | ILC_SYSTEM);
    flagsOut = ImageList_GetFlags(himl);
    if (himl ? TRUE : (skip("Could not initialize\n"), FALSE))
    {
        ok((flagsOut & ILC_SYSTEM), "ILC_SYSTEM\n"); /* Flag is not hidden */

        ok(IL_AddImagesForTest(himl), "Initialize\n");

        flagsIn = IL_CalculateOtherBpp(flagsIn);
        ok(ImageList_SetFlags(himl, flagsIn), "Can change BPP\n");
        ok(ImageList_GetImageCount(himl) == 0, "SetFlags deletes all images\n");

        ok(IL_AddImagesForTest(himl), "Initialize\n");
        ok(ImageList_SetFlags(himl, ImageList_GetFlags(himl)), "Can set same flags\n");
        if (WinVerMajor() >= 6)
        {
            ok(ImageList_SetFlags(himl, flagsIn ^ ILC_SYSTEM), "Can change ILC_SYSTEM\n");
        }
        else
        {
            ok(!ImageList_SetFlags(himl, flagsIn ^ ILC_SYSTEM), "Can't change ILC_SYSTEM\n");
        }

        IL_Destroy(himl);
    }
}

START_TEST(ImageListApi)
{
    InitCommonControls();
    Test_SystemIL();
    Test_Flags();
}
