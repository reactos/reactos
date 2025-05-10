/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IME menus
 * COPYRIGHT:   Copyright 1998 Patrik Stridvall
 *              Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
 *              Copyright 2017 James Tabor <james.tabor@reactos.org>
 *              Copyright 2018 Amine Khaldi <amine.khaldi@reactos.org>
 *              Copyright 2020-2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

/*
 * We will transport the IME menu items by using a flat memory block via
 * a file mapping object beyond the boundary of a process.
 */

#define IMEMENUINFO_BUFFER_SIZE 0x20000
#define IMEMENUINFO_MAGIC 0xBABEF00D

typedef struct tagIMEMENUINFO
{
    DWORD cbSize;
    DWORD cbCapacity; /* IMEMENUINFO_BUFFER_SIZE */
    DWORD dwMagic; /* IMEMENUINFO_MAGIC */
    DWORD dwFlags;
    DWORD dwType;
    DWORD dwItemCount;
    DWORD dwParentOffset;
    DWORD dwItemsOffset;
    DWORD dwBitmapsOffset;
} IMEMENUINFO, *PIMEMENUINFO;

typedef struct tagBITMAPINFOMAX
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[256];
} BITMAPINFOMAX, *PBITMAPINFOMAX;

typedef struct tagIMEMENUBITMAPHEADER
{
    DWORD cbSize;
    DWORD dwBitsOffset;
    HBITMAP hbm;
} IMEMENUBITMAPHEADER, *PIMEMENUBITMAPHEADER;

#define PTR_FROM_OFFSET(head, offset)  (PVOID)((PBYTE)(head) + (SIZE_T)(offset))

static
INT APIENTRY
Imm32ImeMenuAnsiToWide(
    _In_ const IMEMENUITEMINFOA *pItemA,
    _Out_ PIMEMENUITEMINFOW pItemW,
    _In_ UINT uCodePage,
    _In_ BOOL bBitmap)
{
    INT ret;
    pItemW->cbSize = pItemA->cbSize;
    pItemW->fType = pItemA->fType;
    pItemW->fState = pItemA->fState;
    pItemW->wID = pItemA->wID;
    if (bBitmap)
    {
        pItemW->hbmpChecked = pItemA->hbmpChecked;
        pItemW->hbmpUnchecked = pItemA->hbmpUnchecked;
        pItemW->hbmpItem = pItemA->hbmpItem;
    }
    pItemW->dwItemData = pItemA->dwItemData;
    ret = MultiByteToWideChar(uCodePage, 0, pItemA->szString, -1,
                              pItemW->szString, _countof(pItemW->szString));
    if (ret >= _countof(pItemW->szString))
    {
        ret = 0;
        pItemW->szString[0] = UNICODE_NULL;
    }
    return ret;
}

static
INT APIENTRY
Imm32ImeMenuWideToAnsi(
    _In_ const IMEMENUITEMINFOW *pItemW,
    _Out_ PIMEMENUITEMINFOA pItemA,
    _In_ UINT uCodePage)
{
    INT ret;
    pItemA->cbSize = pItemW->cbSize;
    pItemA->fType = pItemW->fType;
    pItemA->fState = pItemW->fState;
    pItemA->wID = pItemW->wID;
    pItemA->hbmpChecked = pItemW->hbmpChecked;
    pItemA->hbmpUnchecked = pItemW->hbmpUnchecked;
    pItemA->dwItemData = pItemW->dwItemData;
    pItemA->hbmpItem = pItemW->hbmpItem;
    ret = WideCharToMultiByte(uCodePage, 0, pItemW->szString, -1,
                              pItemA->szString, _countof(pItemA->szString), NULL, NULL);
    if (ret >= _countof(pItemA->szString))
    {
        ret = 0;
        pItemA->szString[0] = ANSI_NULL;
    }
    return ret;
}

static DWORD
Imm32FindImeMenuBitmap(
    _In_ const IMEMENUINFO *pInfo,
    _In_ HBITMAP hbm)
{
    const BYTE *pb = (const BYTE *)pInfo;
    pb += pInfo->dwBitmapsOffset;
    const IMEMENUBITMAPHEADER *pBitmap = (const IMEMENUBITMAPHEADER *)pb;
    while (pBitmap->cbSize)
    {
        if (pBitmap->hbm == hbm)
            return (PBYTE)pBitmap - (PBYTE)pInfo;
        pBitmap = PTR_FROM_OFFSET(pBitmap, pBitmap->cbSize);
    }
    return 0;
}

static VOID
Imm32DeleteImeMenuBitmaps(_Inout_ PIMEMENUINFO pInfo)
{
    PBYTE pb = (PBYTE)pInfo;
    pb += pInfo->dwBitmapsOffset;
    PIMEMENUBITMAPHEADER pBitmap = (PIMEMENUBITMAPHEADER)pb;
    while (pBitmap->cbSize)
    {
        if (pBitmap->hbm)
        {
            DeleteObject(pBitmap->hbm);
            pBitmap->hbm = NULL;
        }
        pBitmap = PTR_FROM_OFFSET(pBitmap, pBitmap->cbSize);
    }
}

static DWORD
Imm32SerializeImeMenuBitmap(
    _In_ HDC hDC,
    _Inout_ PIMEMENUINFO pInfo,
    _In_ HBITMAP hbm)
{
    if (hbm == NULL)
        return 0;

    DWORD dwOffset = Imm32FindImeMenuBitmap(pInfo, hbm);
    if (dwOffset)
        return dwOffset;

    /* Get DIB info */
    BITMAPINFOMAX bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    if (!GetDIBits(hDC, hbm, 0, 0, NULL, (PBITMAPINFO)&bmi, DIB_RGB_COLORS))
    {
        ERR("!GetDIBits\n");
        return 0;
    }

    /* Calculate the possible color table size */
    DWORD colorTableSize = 0;
    if (bmi.bmiHeader.biBitCount <= 8)
        colorTableSize = (1 << bmi.bmiHeader.biBitCount) * sizeof(RGBQUAD);
    else if (bmi.bmiHeader.biBitCount == 16 || bmi.bmiHeader.biBitCount == 32)
        colorTableSize = 3 * sizeof(DWORD);

    DWORD cbBitmapHeader = sizeof(IMEMENUBITMAPHEADER);
    DWORD dibHeaderSize = sizeof(BITMAPINFOHEADER) + colorTableSize;
    DWORD cbData = cbBitmapHeader + dibHeaderSize + bmi.bmiHeader.biSizeImage;

    if (pInfo->cbSize + cbData + sizeof(DWORD) > pInfo->cbCapacity)
    {
        ERR("Boundary check\n");
        return 0;
    }

    /* Create a bitmap for getting bits */
    HBITMAP hbmTmp = CreateCompatibleBitmap(hDC, bmi.bmiHeader.biWidth, bmi.bmiHeader.biHeight);
    if (!hbmTmp)
    {
        ERR("Out of memory\n");
        return 0;
    }

    PBYTE pb = (PBYTE)pInfo + pInfo->cbSize;
    PIMEMENUBITMAPHEADER pBitmap = (PIMEMENUBITMAPHEADER)pb;

    pBitmap->cbSize = cbData;
    pBitmap->dwBitsOffset = cbBitmapHeader + dibHeaderSize;
    pBitmap->hbm = hbm;
    pb += cbBitmapHeader;
    PBITMAPINFO pbmi = (PBITMAPINFO)pb;
    CopyMemory(pbmi, &bmi, dibHeaderSize);
    pb += dibHeaderSize;

    HGDIOBJ hbmOld = SelectObject(hDC, hbmTmp);
    BOOL ret = GetDIBits(hDC, hbm, 0, bmi.bmiHeader.biHeight, pb, pbmi, DIB_RGB_COLORS);
    SelectObject(hDC, hbmOld);
    DeleteObject(hbmTmp);

    if (!ret)
    {
        ERR("!GetDIBits\n");
        pBitmap->cbSize = 0;
        return 0;
    }

    pInfo->cbSize += cbData;
    return (PBYTE)pBitmap - (PBYTE)pInfo;
}

static HBITMAP
Imm32DeserializeImeMenuBitmap(_Inout_ const IMEMENUBITMAPHEADER *pBitmap)
{
    const BYTE *pb = (const BYTE *)pBitmap;
    const BITMAPINFO *pbmi = (const BITMAPINFO *)(pb + sizeof(*pBitmap));

    HDC hDC = GetDC(NULL);
    HBITMAP hbm = CreateDIBitmap(hDC,
                                 &pbmi->bmiHeader,
                                 CBM_INIT,
                                 pb + pBitmap->dwBitsOffset,
                                 pbmi,
                                 DIB_RGB_COLORS);
    if (!hbm)
        ERR("!hbm\n");
    ReleaseDC(NULL, hDC);
    return hbm;
}

static DWORD
Imm32SerializeImeMenu(
    _Out_ PBYTE pb,
    _In_ DWORD cbCapacity,
    _In_ HIMC hIMC,
    _In_ DWORD dwFlags,
    _In_ DWORD dwType,
    _Inout_opt_ PIMEMENUITEMINFO lpImeParentMenu,
    _In_ BOOL bCountOnly)
{
    PIMEMENUINFO pInfo = (PIMEMENUINFO)pb;
    ZeroMemory(pInfo, cbCapacity);
    pInfo->cbSize = sizeof(*pInfo);
    pInfo->cbCapacity = cbCapacity;
    pInfo->dwMagic = IMEMENUINFO_MAGIC;
    pInfo->dwFlags = dwFlags;
    pInfo->dwType = dwType;
    pb += sizeof(*pInfo);

    DWORD dwItemCount = ImmGetImeMenuItemsW(hIMC, dwFlags, dwType, lpImeParentMenu, NULL, 0);
    pInfo->dwItemCount = dwItemCount;
    if (bCountOnly)
        return dwItemCount;

    if (!dwItemCount)
        return 0;

    if (lpImeParentMenu)
    {
        pInfo->dwParentOffset = pb - (PBYTE)pInfo;
        pInfo->cbSize += sizeof(*lpImeParentMenu);
        CopyMemory(pb, lpImeParentMenu, sizeof(*lpImeParentMenu));
        pb += sizeof(*lpImeParentMenu);
    }

    SIZE_T cbItems = dwItemCount * sizeof(IMEMENUITEMINFO);
    PIMEMENUITEMINFO pItems = ImmLocalAlloc(LPTR, cbItems);
    if (!pItems)
        return 0;

    dwItemCount = ImmGetImeMenuItemsW(hIMC, dwFlags, dwType, lpImeParentMenu, pItems, cbItems);
    pInfo->dwItemCount = dwItemCount;

    pInfo->dwItemsOffset = pb - (PBYTE)pInfo;
    pInfo->dwBitmapsOffset = pInfo->dwItemsOffset + cbItems;
    if (pInfo->dwBitmapsOffset + sizeof(DWORD) > pInfo->cbCapacity)
    {
        for (DWORD iItem = 0; iItem < dwItemCount; ++iItem)
        {
            if (pItems[iItem].hbmpChecked)
                DeleteObject(pItems[iItem].hbmpChecked);
            if (pItems[iItem].hbmpUnchecked)
                DeleteObject(pItems[iItem].hbmpUnchecked);
            if (pItems[iItem].hbmpItem)
                DeleteObject(pItems[iItem].hbmpItem);
        }
        ImmLocalFree(pItems);
        return 0;
    }
    CopyMemory(pb, pItems, cbItems);
    pInfo->cbSize += cbItems;

    ImmLocalFree(pItems);
    pItems = PTR_FROM_OFFSET(pInfo, pInfo->dwItemsOffset);

    DWORD dwOffset;
    HDC hDC = CreateCompatibleDC(NULL);
    for (DWORD iItem = 0; iItem < dwItemCount; ++iItem)
    {
        PIMEMENUITEMINFO pItem = &pItems[iItem];

        dwOffset = Imm32SerializeImeMenuBitmap(hDC, pInfo, pItem->hbmpChecked);
        if (dwOffset)
            pItem->hbmpChecked = UlongToHandle(dwOffset);

        dwOffset = Imm32SerializeImeMenuBitmap(hDC, pInfo, pItem->hbmpUnchecked);
        if (dwOffset)
            pItem->hbmpUnchecked = UlongToHandle(dwOffset);

        dwOffset = Imm32SerializeImeMenuBitmap(hDC, pInfo, pItem->hbmpItem);
        if (dwOffset)
            pItem->hbmpItem = UlongToHandle(dwOffset);
    }
    DeleteDC(hDC);

    Imm32DeleteImeMenuBitmaps(pInfo);

    return dwItemCount;
}

static DWORD
Imm32DeserializeImeMenu(
    _Inout_ PBYTE pb,
    _Out_opt_ PIMEMENUITEMINFO lpImeMenu,
    _In_ DWORD dwSize)
{
    PIMEMENUINFO pInfo = (PIMEMENUINFO)pb;
    if (pInfo->dwMagic != IMEMENUINFO_MAGIC || pInfo->cbSize > pInfo->cbCapacity)
        return 0;

    DWORD dwItemCount = pInfo->dwItemCount;
    if (!lpImeMenu)
        return dwItemCount;

    if (dwItemCount > dwSize / sizeof(IMEMENUITEMINFO))
        dwItemCount = dwSize / sizeof(IMEMENUITEMINFO);

    pb += pInfo->dwItemsOffset;
    PIMEMENUITEMINFO pItems = (PIMEMENUITEMINFO)pb;

    PIMEMENUBITMAPHEADER pBitmap;
    for (DWORD iItem = 0; iItem < dwItemCount; ++iItem)
    {
        PIMEMENUITEMINFO pItem = &pItems[iItem];
        if (pItem->hbmpChecked)
        {
            pBitmap = PTR_FROM_OFFSET(pInfo, pItem->hbmpChecked);
            pItem->hbmpChecked = Imm32DeserializeImeMenuBitmap(pBitmap);
        }
        if (pItem->hbmpUnchecked)
        {
            pBitmap = PTR_FROM_OFFSET(pInfo, pItem->hbmpUnchecked);
            pItem->hbmpUnchecked = Imm32DeserializeImeMenuBitmap(pBitmap);
        }
        if (pItem->hbmpItem)
        {
            pBitmap = PTR_FROM_OFFSET(pInfo, pItem->hbmpItem);
            pItem->hbmpItem = Imm32DeserializeImeMenuBitmap(pBitmap);
        }
        lpImeMenu[iItem] = *pItem;
    }

    return dwItemCount;
}

/***********************************************************************
 *		ImmPutImeMenuItemsIntoMappedFile (IMM32.@)
 *
 * Called from user32.dll to transport the IME menu items by using a
 * file mapping object. This function is provided for WM_IME_SYSTEM:IMS_GETIMEMENU
 * handling.
 */
LRESULT WINAPI
ImmPutImeMenuItemsIntoMappedFile(_In_ HIMC hIMC)
{
    /* Open the existing file mapping*/
    HANDLE hMapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"ImmMenuInfo");
    if (!hMapping)
    {
        ERR("!hMapping\n");
        return 0;
    }

    /* Map the view */
    PIMEMENUINFO pView = MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!pView)
    {
        ERR("!pView\n");
        CloseHandle(hMapping);
        return 0;
    }

    /* Get parent menu info */
    PVOID lpImeParentMenu = NULL;
    if (pView->dwParentOffset)
        lpImeParentMenu = PTR_FROM_OFFSET(pView, pView->dwParentOffset);

    /* Serialize the IME menu */
    DWORD dwItemCount = Imm32SerializeImeMenu((PBYTE)pView, pView->cbCapacity,
                                              hIMC, pView->dwFlags, pView->dwType,
                                              lpImeParentMenu, !pView->dwItemCount);

    /* Clean up */
    UnmapViewOfFile(pView);
    CloseHandle(hMapping);

    return dwItemCount;
}

static
DWORD APIENTRY
Imm32GetImeMenuItemWInterProcess(
    _In_ HIMC hIMC,
    _In_ DWORD dwFlags,
    _In_ DWORD dwType,
    _Inout_opt_ PVOID lpImeParentMenu,
    _Out_opt_ PVOID lpImeMenu,
    _In_ DWORD dwSize)
{
    /* Get IME window */
    HWND hwndIme = (HWND)NtUserQueryInputContext(hIMC, QIC_DEFAULTWINDOWIME);
    if (!hwndIme || !IsWindow(hwndIme))
    {
        ERR("\n");
        return 0;
    }

    /* Lock */
    RtlEnterCriticalSection(&gcsImeDpi);

    /* Create a file mapping */
    HANDLE hMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                         0, IMEMENUINFO_BUFFER_SIZE, L"ImmMenuInfo");
    if (!hMapping)
    {
        ERR("!pView\n");
        RtlLeaveCriticalSection(&gcsImeDpi);
        return 0;
    }

    /* Map the view */
    PIMEMENUINFO pView = MapViewOfFile(hMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    if (!pView)
    {
        ERR("!pView\n");
        CloseHandle(hMapping);
        RtlLeaveCriticalSection(&gcsImeDpi);
        return 0;
    }

    /* Send WM_IME_SYSTEM.IMS_GETIMEMENU message. It will call ImmPutImeMenuItemsIntoMappedFile */
    DWORD ret = 0;
    if (SendMessageW(hwndIme, WM_IME_SYSTEM, IMS_GETIMEMENU, (LPARAM)hIMC))
    {
        /* De-serialize the IME menu */
        ret = Imm32DeserializeImeMenu((PBYTE)pView, lpImeMenu, dwSize);
    }

    /* Cleanup */
    UnmapViewOfFile(pView); /* Unmap */
    CloseHandle(hMapping); /* Close the file mapping */
    RtlLeaveCriticalSection(&gcsImeDpi); /* Unlock */
    return ret;
}

static DWORD
ImmGetImeMenuItemsAW(
    _In_ HIMC hIMC,
    _In_ DWORD dwFlags,
    _In_ DWORD dwType,
    _Inout_opt_ PVOID lpImeParentMenu,
    _Out_opt_ PVOID lpImeMenu,
    _In_ DWORD dwSize,
    _In_ BOOL bTargetIsAnsi)
{
    DWORD ret = 0, cbTotal, dwProcessId, dwThreadId, iItem;
    PINPUTCONTEXT pIC;
    PIMEDPI pImeDpi = NULL;
    IMEMENUITEMINFOA ParentA;
    IMEMENUITEMINFOW ParentW;
    PIMEMENUITEMINFOA pItemA;
    PIMEMENUITEMINFOW pItemW;
    PVOID pNewItems = NULL, pNewParent = NULL;
    BOOL bImcIsAnsi;
    HKL hKL;

    if (!hIMC)
    {
        ERR("!hIMC\n");
        return 0;
    }

    dwProcessId = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTPROCESSID);
    if (!dwProcessId)
    {
        ERR("!dwProcessId\n");
        return 0;
    }

    if (dwProcessId != GetCurrentProcessId())
    {
        if (bTargetIsAnsi)
            return 0;
        return Imm32GetImeMenuItemWInterProcess(hIMC, dwFlags, dwType, lpImeParentMenu,
                                                lpImeMenu, dwSize);
    }

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
    {
        ERR("!pIC\n");
        return 0;
    }

    dwThreadId = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTTHREADID);
    if (!dwThreadId)
    {
        ERR("!dwThreadId\n");
        ImmUnlockIMC(hIMC);
        return 0;
    }

    hKL = GetKeyboardLayout(dwThreadId);
    pImeDpi = ImmLockImeDpi(hKL);
    if (!pImeDpi)
    {
        ERR("!pImeDpi\n");
        ImmUnlockIMC(hIMC);
        return 0;
    }

    bImcIsAnsi = Imm32IsImcAnsi(hIMC);

    if (bImcIsAnsi != bTargetIsAnsi)
    {
        if (bTargetIsAnsi)
        {
            if (lpImeParentMenu)
                pNewParent = &ParentW;

            if (lpImeMenu)
            {
                cbTotal = ((dwSize / sizeof(IMEMENUITEMINFOA)) * sizeof(IMEMENUITEMINFOW));
                pNewItems = ImmLocalAlloc(LPTR, cbTotal);
                if (!pNewItems)
                {
                    ERR("!pNewItems\n");
                    goto Quit;
                }
            }
        }
        else
        {
            if (lpImeParentMenu)
                pNewParent = &ParentA;

            if (lpImeMenu)
            {
                cbTotal = ((dwSize / sizeof(IMEMENUITEMINFOW)) * sizeof(IMEMENUITEMINFOA));
                pNewItems = ImmLocalAlloc(LPTR, cbTotal);
                if (!pNewItems)
                {
                    ERR("!pNewItems\n");
                    goto Quit;
                }
            }
        }
    }
    else
    {
        pNewItems = lpImeMenu;
        pNewParent = lpImeParentMenu;
    }

    ret = pImeDpi->ImeGetImeMenuItems(hIMC, dwFlags, dwType, pNewParent, pNewItems, dwSize);
    if (!ret || !lpImeMenu)
    {
        ERR("%d, %p\n", ret, lpImeMenu);
        goto Quit;
    }

    if (bImcIsAnsi != bTargetIsAnsi)
    {
        if (bTargetIsAnsi)
        {
            if (pNewParent)
                Imm32ImeMenuWideToAnsi(pNewParent, lpImeParentMenu, pImeDpi->uCodePage);

            pItemW = pNewItems;
            pItemA = lpImeMenu;
            for (iItem = 0; iItem < ret; ++iItem, ++pItemW, ++pItemA)
            {
                if (!Imm32ImeMenuWideToAnsi(pItemW, pItemA, pImeDpi->uCodePage))
                {
                    ERR("\n");
                    ret = 0;
                    break;
                }
            }
        }
        else
        {
            if (pNewParent)
                Imm32ImeMenuAnsiToWide(pNewParent, lpImeParentMenu, pImeDpi->uCodePage, TRUE);

            pItemA = pNewItems;
            pItemW = lpImeMenu;
            for (iItem = 0; iItem < dwSize; ++iItem, ++pItemA, ++pItemW)
            {
                if (!Imm32ImeMenuAnsiToWide(pItemA, pItemW, pImeDpi->uCodePage, TRUE))
                {
                    ERR("\n");
                    ret = 0;
                    break;
                }
            }
        }
    }

Quit:
    if (pNewItems != lpImeMenu)
        ImmLocalFree(pNewItems);
    ImmUnlockImeDpi(pImeDpi);
    ImmUnlockIMC(hIMC);
    TRACE("ret: 0x%X\n", ret);
    return ret;
}

/***********************************************************************
 *		ImmGetImeMenuItemsA (IMM32.@)
 */
DWORD WINAPI
ImmGetImeMenuItemsA(
    _In_ HIMC hIMC,
    _In_ DWORD dwFlags,
    _In_ DWORD dwType,
    _Inout_opt_ PIMEMENUITEMINFOA lpImeParentMenu,
    _Out_opt_ PIMEMENUITEMINFOA lpImeMenu,
    _In_ DWORD dwSize)
{
    TRACE("(%p, 0x%lX, 0x%lX, %p, %p, 0x%lX)\n",
          hIMC, dwFlags, dwType, lpImeParentMenu, lpImeMenu, dwSize);
    return ImmGetImeMenuItemsAW(hIMC, dwFlags, dwType, lpImeParentMenu, lpImeMenu, dwSize, TRUE);
}

/***********************************************************************
 *		ImmGetImeMenuItemsW (IMM32.@)
 */
DWORD WINAPI
ImmGetImeMenuItemsW(
    _In_ HIMC hIMC,
    _In_ DWORD dwFlags,
    _In_ DWORD dwType,
    _Inout_opt_ PIMEMENUITEMINFOW lpImeParentMenu,
    _Out_opt_ PIMEMENUITEMINFOW lpImeMenu,
    _In_ DWORD dwSize)
{
    TRACE("(%p, 0x%lX, 0x%lX, %p, %p, 0x%lX)\n",
          hIMC, dwFlags, dwType, lpImeParentMenu, lpImeMenu, dwSize);
    return ImmGetImeMenuItemsAW(hIMC, dwFlags, dwType, lpImeParentMenu, lpImeMenu, dwSize, FALSE);
}
