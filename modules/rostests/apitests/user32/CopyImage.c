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

#define LR_UNKNOWN_0x10000 0x10000

static HANDLE CreateTestImage(UINT uType)
{
    HANDLE hImage;
    switch (uType)
    {
        case IMAGE_BITMAP:
        {
            HDC hDC = CreateCompatibleDC(NULL);
            hImage = (HANDLE)CreateCompatibleBitmap(hDC, 10, 10);
            DeleteDC(hDC);
            break;
        }
        case IMAGE_CURSOR:
            hImage = (HANDLE)LoadCursor(NULL, IDC_ARROW);
            break;
        case IMAGE_ICON:
            hImage = (HANDLE)LoadIcon(NULL, IDI_APPLICATION);
            break;
    }
    return hImage;
}

static VOID
Test_CopyImage_Flags(UINT uType)
{
    UINT iBit, uBit, uValidFlags = COPYIMAGE_VALID_FLAGS;
    HANDLE hImage, hCopiedImage;

    if (IsWindowsVistaOrGreater())
        uValidFlags |= LR_UNKNOWN_0x10000;

    hImage = CreateTestImage(uType);
    for (iBit = 0; iBit < sizeof(UINT) * CHAR_BIT; ++iBit)
    {
        uBit = (1 << iBit);

        SetLastError(0xDEADFACE);
        hCopiedImage = CopyImage(hImage, uType, 0, 0, uBit);

        if (uValidFlags & uBit) // Valid flag?
        {
            ok(hCopiedImage != NULL, "iBit %u: uType %u: hCopiedImage was NULL\n", iBit, uType);
        }
        else
        {
            ok(hCopiedImage == NULL, "iBit %u: uType %u: hCopiedImage was %p\n", iBit, uType, hCopiedImage);
            ok_err(ERROR_INVALID_PARAMETER);
        }

        if (hCopiedImage)
            DeleteObject(hCopiedImage);

        /* If the original image was deleted, re-create it */
        if (uBit & LR_COPYDELETEORG)
            hImage = CreateTestImage(uType);
    }

    DeleteObject(hImage);
}

START_TEST(CopyImage)
{
    Test_CopyImage_Flags(IMAGE_BITMAP);
    Test_CopyImage_Flags(IMAGE_CURSOR);
    Test_CopyImage_Flags(IMAGE_ICON);
}
