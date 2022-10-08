/*
 * PROJECT:     ReactOS headers
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Branding support for dialog boxes.
 * COPYRIGHT:   Copyright 2006 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2019 Stanislav Motylkov <x86corez@gmail.com>
 *              Copyright 2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *              Copyright 2022 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <wingdi.h>

typedef struct _BRAND
{
    HBITMAP hLogoBitmap;
    HBITMAP hBarBitmap;
    DWORD LogoWidth;
    DWORD LogoHeight;
    DWORD BarWidth;
    DWORD BarHeight;
} BRAND, *PBRAND;

VOID
Brand_LoadBitmaps(
    _In_ HINSTANCE hInstance,
    _Inout_ PBRAND pBrand);

VOID
Brand_Cleanup(
    _Inout_ PBRAND pBrand);


#if defined(BRANDING_LIB_IMPL) || !defined(BRANDING_LIB)

VOID
Brand_LoadBitmaps(
    _In_ HINSTANCE hInstance,
    _Inout_ PBRAND pBrand)
{
    BITMAP bm;

    if (!pBrand)
        return;

    ZeroMemory(pBrand, sizeof(*pBrand));

    pBrand->hLogoBitmap = LoadImageW(hInstance,
                                     MAKEINTRESOURCEW(IDI_ROSLOGO), IMAGE_BITMAP,
                                     0, 0, LR_DEFAULTCOLOR);
    if (pBrand->hLogoBitmap)
    {
        GetObjectW(pBrand->hLogoBitmap, sizeof(bm), &bm);
        pBrand->LogoWidth = bm.bmWidth;
        pBrand->LogoHeight = bm.bmHeight;
    }

    pBrand->hBarBitmap = LoadImageW(hInstance,
                                    MAKEINTRESOURCEW(IDI_BAR), IMAGE_BITMAP,
                                    0, 0, LR_DEFAULTCOLOR);
    if (pBrand->hBarBitmap)
    {
        GetObjectW(pBrand->hBarBitmap, sizeof(bm), &bm);
        pBrand->BarWidth = bm.bmWidth;
        pBrand->BarHeight = bm.bmHeight;
    }
}

VOID
Brand_Cleanup(
    _Inout_ PBRAND pBrand)
{
    if (pBrand->hBarBitmap)
        DeleteObject(pBrand->hBarBitmap);

    if (pBrand->hLogoBitmap)
        DeleteObject(pBrand->hLogoBitmap);

    ZeroMemory(pBrand, sizeof(*pBrand));
}

#endif // defined(BRANDING_LIB_IMPL) || !defined(BRANDING_LIB)


#ifdef __cplusplus
}
#endif

/* EOF */
