/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for imagelist
 * PROGRAMMERS:     Whindmar Saksit
 */

#include "wine/test.h"
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <comctl32_undoc.h>

#define WinVerMajor() LOBYTE(GetVersion())

#define ILC_CLRMASK (0xff & ~ILC_MASK)
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

static BOOL IL_AddImagesForTest(HIMAGELIST himl)
{
    int idx = -1;
    HINSTANCE hInst = LoadLibraryW(L"USER32");
    HICON hIco = (HICON)LoadImage(hInst, MAKEINTRESOURCE(100), /* Windows */
                                  IMAGE_ICON, IL_IMGSIZE, IL_IMGSIZE, 0);
    if (!hIco)
        hIco = (HICON)LoadImage(hInst, MAKEINTRESOURCE(32512), /* ReactOS */
                                IMAGE_ICON, IL_IMGSIZE, IL_IMGSIZE, 0);

    if (hIco)
    {
        idx = ImageList_AddIcon(himl, hIco);
        DestroyIcon(hIco);
    }
    return idx != -1;
}

static void Test_SystemIL()
{
    UINT flags = ILC_COLOR16 | ILC_MASK;
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

static void Test_Flags()
{
    UINT flags = ILC_COLOR16 | ILC_MASK, flagsIn, flagsOut;
    HIMAGELIST himl;

    flagsOut = ImageList_GetFlags(himl = IL_Create(flagsIn = flags));
    if (himl ? TRUE : (skip("Could not initialize\n"), FALSE))
    {
        ok((flagsOut & ILC_CLRMASK) == (flagsIn & ILC_CLRMASK), "ILC_COLOR\n");
        ok(!(flagsOut & ILC_SYSTEM), "!ILC_SYSTEM\n");

        ok(IL_AddImagesForTest(himl), "Initialize\n");
        ok(ImageList_SetFlags(himl, flagsIn ^= 0x30), "Can change BPP\n");
        ok(ImageList_GetImageCount(himl) == 0, "SetFlags deletes all images\n");

        ok(IL_AddImagesForTest(himl), "Initialize\n");
        ok(ImageList_SetFlags(himl, ImageList_GetFlags(himl)) && flagsOut, "Can set same flags\n");
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

    flagsOut = ImageList_GetFlags(himl = IL_Create(flagsIn = flags | ILC_SYSTEM));
    if (himl ? TRUE : (skip("Could not initialize\n"), FALSE))
    {
        ok((flagsOut & ILC_SYSTEM), "ILC_SYSTEM\n"); /* Flag is not hidden */

        ok(IL_AddImagesForTest(himl), "Initialize\n");
        ok(ImageList_SetFlags(himl, flagsIn ^= 0x30), "Can change BPP\n");
        ok(ImageList_GetImageCount(himl) == 0, "SetFlags deletes all images\n");

        ok(IL_AddImagesForTest(himl), "Initialize\n");
        ok(ImageList_SetFlags(himl, ImageList_GetFlags(himl)) && flagsOut, "Can set same flags\n");
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

START_TEST(imagelist)
{
    LoadLibraryW(L"comctl32.dll"); /* same as statically linking to comctl32 and doing InitCommonControls */
    Test_SystemIL();
    Test_Flags();
}
