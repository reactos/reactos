/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for SetFocus/GetFocus/GetGUIThreadInfo
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <versionhelpers.h>

#define COPYIMAGE_VALID_FLAGS ( \
    LR_SHARED | LR_COPYFROMRESOURCE | LR_CREATEDIBSECTION | LR_LOADMAP3DCOLORS | 0x800 | \
    LR_VGACOLOR | LR_LOADREALSIZE | LR_DEFAULTSIZE | LR_LOADTRANSPARENT | LR_LOADFROMFILE | \
    LR_COPYDELETEORG | LR_COPYRETURNORG | LR_COLOR | LR_MONOCHROME \
)

static VOID
Test_CopyImage_Flags(VOID)
{
    HDC hDC = CreateCompatibleDC(NULL);
    HBITMAP hbm = CreateCompatibleBitmap(hDC, 10, 10);
    UINT iBit, uBit, uValidFlags = COPYIMAGE_VALID_FLAGS;
    HBITMAP hbmCopyed;

    if (IsWindowsVistaOrGreater())
        uValidFlags |= 0x10000;

    for (iBit = 0; iBit < sizeof(DWORD) * 8; ++iBit)
    {
        uBit = (1 << iBit);

        // We don't delete original at here
        if (uBit & LR_COPYDELETEORG)
            continue;

        if (uValidFlags & uBit) // Valid flag?
        {
            hbmCopyed = CopyImage(hbm, IMAGE_BITMAP, 0, 0, uBit);
            ok(hbmCopyed != NULL, "iBit %u: hbmCopyed was NULL\n", iBit);
            if (hbmCopyed)
                DeleteObject(hbmCopyed);
        }
        else
        {
            SetLastError(0xDEADFACE);
            hbmCopyed = CopyImage(hbm, IMAGE_BITMAP, 0, 0, uBit);
            ok(hbmCopyed == NULL, "iBit %u: hbmCopyed was %p\n", iBit, hbmCopyed);
            ok_err(ERROR_INVALID_PARAMETER);
            if (hbmCopyed)
                DeleteObject(hbmCopyed);
        }
    }

    DeleteDC(hDC);
}

START_TEST(CopyImage)
{
    Test_CopyImage_Flags();
}
