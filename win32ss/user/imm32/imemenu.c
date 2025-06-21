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

#define IMEMENUINFO_BUFFER_SIZE 0x20000
#define IMEMENUINFO_MAGIC 0xBABEF00D /* ReactOS-specific */

/* ReactOS-specific */
typedef struct tagIMEMENUINFO
{
    DWORD cbSize;
    DWORD cbCapacity; /* IMEMENUINFO_BUFFER_SIZE */
    DWORD dwMagic; /* IMEMENUINFO_MAGIC */
    DWORD dwFlags; /* ImmGetImeMenuItems.dwFlags */
    DWORD dwType; /* ImmGetImeMenuItems.dwType */
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

/* ReactOS-specific */
typedef struct tagIMEMENUBITMAPHEADER
{
    DWORD cbSize;
    DWORD dwBitsOffset;
    HBITMAP hbm;
} IMEMENUBITMAPHEADER, *PIMEMENUBITMAPHEADER;

#define PTR_FROM_OFFSET(head, offset)  (PVOID)((PBYTE)(head) + (SIZE_T)(offset))

/* Convert ANSI IME menu to Wide */
static BOOL
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
    pItemW->szString[_countof(pItemW->szString) - 1] = UNICODE_NULL; // Avoid buffer overrun
    return !!ret;
}

/* Convert Wide IME menu to ANSI */
static BOOL
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
    pItemA->szString[_countof(pItemA->szString) - 1] = ANSI_NULL; // Avoid buffer overrun
    return !!ret;
}

static DWORD
Imm32FindImeMenuBitmap(
    _In_ const IMEMENUINFO *pView,
    _In_ HBITMAP hbm)
{
    const BYTE *pb = (const BYTE *)pView;
    pb += pView->dwBitmapsOffset;
    const IMEMENUBITMAPHEADER *pBitmap = (const IMEMENUBITMAPHEADER *)pb;
    while (pBitmap->cbSize)
    {
        if (pBitmap->hbm == hbm)
            return (PBYTE)pBitmap - (PBYTE)pView; /* Byte offset from pView */
        pBitmap = PTR_FROM_OFFSET(pBitmap, pBitmap->cbSize);
    }
    return 0;
}

static VOID
Imm32DeleteImeMenuBitmaps(_Inout_ PIMEMENUINFO pView)
{
    PBYTE pb = (PBYTE)pView;
    pb += pView->dwBitmapsOffset;
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
    _Inout_ PIMEMENUINFO pView,
    _In_ HBITMAP hbm)
{
    if (hbm == NULL)
        return 0;

    DWORD dwOffset = Imm32FindImeMenuBitmap(pView, hbm);
    if (dwOffset)
        return dwOffset; /* Already serialized */

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

    /* Calculate the data sizes and validate them */
    DWORD cbBitmapHeader = sizeof(IMEMENUBITMAPHEADER);
    DWORD dibHeaderSize = sizeof(BITMAPINFOHEADER) + colorTableSize;
    DWORD cbData = cbBitmapHeader + dibHeaderSize + bmi.bmiHeader.biSizeImage;
    if (pView->cbSize + cbData + sizeof(DWORD) > pView->cbCapacity)
    {
        ERR("Too large IME menu (0x%X, 0x%X)\n", pView->cbSize, cbData);
        return 0;
    }

    /* Create a bitmap for getting bits */
    HBITMAP hbmTmp = CreateCompatibleBitmap(hDC, bmi.bmiHeader.biWidth, bmi.bmiHeader.biHeight);
    if (!hbmTmp)
    {
        ERR("Out of memory\n");
        return 0;
    }

    /* Store the IMEMENUBITMAPHEADER */
    PBYTE pb = (PBYTE)pView + pView->cbSize;
    PIMEMENUBITMAPHEADER pBitmap = (PIMEMENUBITMAPHEADER)pb;
    pBitmap->cbSize = cbData;
    pBitmap->dwBitsOffset = cbBitmapHeader + dibHeaderSize;
    pBitmap->hbm = hbm;
    pb += cbBitmapHeader;

    /* Store the BITMAPINFO */
    PBITMAPINFO pbmi = (PBITMAPINFO)pb;
    CopyMemory(pbmi, &bmi, dibHeaderSize);
    pb += dibHeaderSize;

    /* Get the bits */
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

    pView->cbSize += cbData;
    return (PBYTE)pBitmap - (PBYTE)pView; /* Byte offset from pView */
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

/*
 * We transport the IME menu items by using a flat memory block via
 * a file mapping object beyond boundary of process.
 */
static DWORD
Imm32SerializeImeMenu(
    _Inout_ PIMEMENUINFO pView,
    _In_ HIMC hIMC,
    _Inout_opt_ PIMEMENUITEMINFOW lpImeParentMenu,
    _In_ BOOL bCountOnly)
{
    /* Sanity check */
    if (pView->dwMagic != IMEMENUINFO_MAGIC || pView->cbSize > pView->cbCapacity)
    {
        ERR("Invalid pView\n");
        return 0;
    }

    /* Get the count of menu items */
    DWORD dwFlags = pView->dwFlags;
    DWORD dwType = pView->dwType;
    DWORD dwItemCount = ImmGetImeMenuItemsW(hIMC, dwFlags, dwType, lpImeParentMenu, NULL, 0);
    pView->dwItemCount = dwItemCount;
    if (bCountOnly)
        return dwItemCount;

    if (!dwItemCount)
        return 0;

    /* Start of serialization */
    PBYTE pb = (PBYTE)pView;
    pb += sizeof(*pView);

    /* Store the parent menu data */
    if (lpImeParentMenu)
    {
        pView->dwParentOffset = pb - (PBYTE)pView;
        pView->cbSize += sizeof(*lpImeParentMenu);
        CopyMemory(pb, lpImeParentMenu, sizeof(*lpImeParentMenu));
        pb += sizeof(*lpImeParentMenu);
    }

    /* The byte size of items */
    SIZE_T cbItems = dwItemCount * sizeof(IMEMENUITEMINFOW);

    /* Update the offset info */
    pView->dwItemsOffset = pb - (PBYTE)pView;
    pView->dwBitmapsOffset = pView->dwItemsOffset + cbItems;
    if (pView->dwItemsOffset + sizeof(DWORD) > pView->cbCapacity ||
        pView->dwBitmapsOffset + sizeof(DWORD) > pView->cbCapacity)
    {
        ERR("Too large IME menu (0x%X, 0x%X)\n", pView->dwItemsOffset, pView->dwBitmapsOffset);
        return 0;
    }

    /* Actually get the items */
    PIMEMENUITEMINFOW pItems = (PIMEMENUITEMINFOW)pb;
    dwItemCount = ImmGetImeMenuItemsW(hIMC, dwFlags, dwType, lpImeParentMenu, pItems, cbItems);
    pView->dwItemCount = dwItemCount;
    pView->cbSize += cbItems;

    /* Serialize the bitmaps */
    DWORD dwOffset;
    HDC hDC = CreateCompatibleDC(NULL);
    for (DWORD iItem = 0; iItem < dwItemCount; ++iItem)
    {
        PIMEMENUITEMINFOW pItem = &pItems[iItem];

        dwOffset = Imm32SerializeImeMenuBitmap(hDC, pView, pItem->hbmpChecked);
        if (dwOffset)
            pItem->hbmpChecked = UlongToHandle(dwOffset);

        dwOffset = Imm32SerializeImeMenuBitmap(hDC, pView, pItem->hbmpUnchecked);
        if (dwOffset)
            pItem->hbmpUnchecked = UlongToHandle(dwOffset);

        dwOffset = Imm32SerializeImeMenuBitmap(hDC, pView, pItem->hbmpItem);
        if (dwOffset)
            pItem->hbmpItem = UlongToHandle(dwOffset);
    }
    DeleteDC(hDC);

    TRACE("pView->cbSize: 0x%X\n", pView->cbSize);

    /* Clean up */
    Imm32DeleteImeMenuBitmaps(pView);

    return dwItemCount;
}

static DWORD
Imm32DeserializeImeMenu(
    _Inout_ PIMEMENUINFO pView,
    _Out_writes_bytes_opt_(dwSize) PIMEMENUITEMINFOW lpImeMenuItems,
    _In_ DWORD dwSize)
{
    /* Sanity check */
    if (pView->dwMagic != IMEMENUINFO_MAGIC || pView->cbSize > pView->cbCapacity)
    {
        ERR("Invalid pView\n");
        return 0;
    }

    DWORD dwItemCount = pView->dwItemCount;
    if (lpImeMenuItems == NULL)
        return dwItemCount; /* Count only */

    /* Limit the item count for dwSize */
    if (dwItemCount > dwSize / sizeof(IMEMENUITEMINFOW))
        dwItemCount = dwSize / sizeof(IMEMENUITEMINFOW);

    /* Get the items pointer */
    PIMEMENUITEMINFOW pItems = PTR_FROM_OFFSET(pView, pView->dwItemsOffset);

    /* Copy the items and de-serialize the bitmaps */
    PIMEMENUBITMAPHEADER pBitmap;
    for (DWORD iItem = 0; iItem < dwItemCount; ++iItem)
    {
        PIMEMENUITEMINFOW pItem = &pItems[iItem];
        if (pItem->hbmpChecked)
        {
            pBitmap = PTR_FROM_OFFSET(pView, pItem->hbmpChecked);
            pItem->hbmpChecked = Imm32DeserializeImeMenuBitmap(pBitmap);
        }
        if (pItem->hbmpUnchecked)
        {
            pBitmap = PTR_FROM_OFFSET(pView, pItem->hbmpUnchecked);
            pItem->hbmpUnchecked = Imm32DeserializeImeMenuBitmap(pBitmap);
        }
        if (pItem->hbmpItem)
        {
            pBitmap = PTR_FROM_OFFSET(pView, pItem->hbmpItem);
            pItem->hbmpItem = Imm32DeserializeImeMenuBitmap(pBitmap);
        }
        lpImeMenuItems[iItem] = *pItem;
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
    /* Open the existing file mapping */
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
    DWORD dwItemCount = Imm32SerializeImeMenu(pView, hIMC, lpImeParentMenu, !pView->dwItemCount);

    /* Clean up */
    UnmapViewOfFile(pView);
    CloseHandle(hMapping);

    return dwItemCount;
}

static DWORD
Imm32GetImeMenuItemWInterProcess(
    _In_ HIMC hIMC,
    _In_ DWORD dwFlags,
    _In_ DWORD dwType,
    _Inout_opt_ PVOID lpImeParentMenu,
    _Out_writes_bytes_opt_(dwSize) PVOID lpImeMenuItems,
    _In_ DWORD dwSize)
{
    /* Get IME window */
    HWND hwndIme = (HWND)NtUserQueryInputContext(hIMC, QIC_DEFAULTWINDOWIME);
    if (!hwndIme || !IsWindow(hwndIme))
    {
        ERR("!hwndIme\n");
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

    /* Initialize view header */
    ZeroMemory(pView, IMEMENUINFO_BUFFER_SIZE);
    pView->cbSize = sizeof(*pView);
    pView->cbCapacity = IMEMENUINFO_BUFFER_SIZE;
    pView->dwMagic = IMEMENUINFO_MAGIC;
    pView->dwFlags = dwFlags;
    pView->dwType = dwType;
    pView->dwItemCount = lpImeMenuItems ? (dwSize / sizeof(IMEMENUITEMINFOW)) : 0;

    /* Send WM_IME_SYSTEM.IMS_GETIMEMENU message. It will call ImmPutImeMenuItemsIntoMappedFile */
    DWORD ret = 0;
    if (SendMessageW(hwndIme, WM_IME_SYSTEM, IMS_GETIMEMENU, (LPARAM)hIMC))
    {
        /* De-serialize the IME menu */
        ret = Imm32DeserializeImeMenu(pView, lpImeMenuItems, dwSize);
    }

    /* Clean up */
    UnmapViewOfFile(pView); /* Unmap */
    CloseHandle(hMapping); /* Close the file mapping */
    RtlLeaveCriticalSection(&gcsImeDpi); /* Unlock */
    return ret;
}

/* Absorbs the differences between ANSI and Wide */
static DWORD
ImmGetImeMenuItemsAW(
    _In_ HIMC hIMC,
    _In_ DWORD dwFlags,
    _In_ DWORD dwType,
    _Inout_opt_ PVOID lpImeParentMenu,
    _Out_writes_bytes_opt_(dwSize) PVOID lpImeMenuItems,
    _In_ DWORD dwSize,
    _In_ BOOL bTargetIsAnsi)
{
    DWORD ret = 0, iItem;

    if (!hIMC)
    {
        ERR("!hIMC\n");
        return 0;
    }

    /* Get input process ID */
    DWORD dwProcessId = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTPROCESSID);
    if (!dwProcessId)
    {
        ERR("!dwProcessId\n");
        return 0;
    }

    if (dwProcessId != GetCurrentProcessId()) /* Cross process? */
    {
        if (bTargetIsAnsi)
        {
            ERR("ImmGetImeMenuItemsA cannot cross process boundary\n");
            return 0;
        }

        /* Transport the IME menu items, using file mapping */
        return Imm32GetImeMenuItemWInterProcess(hIMC, dwFlags, dwType, lpImeParentMenu,
                                                lpImeMenuItems, dwSize);
    }

    PINPUTCONTEXT pIC = ImmLockIMC(hIMC);
    if (!pIC)
    {
        ERR("!pIC\n");
        return 0;
    }

    /* Get input thread ID */
    DWORD dwThreadId = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTTHREADID);
    if (!dwThreadId)
    {
        ERR("!dwThreadId\n");
        ImmUnlockIMC(hIMC);
        return 0;
    }

    /* Get IME interface */
    HKL hKL = GetKeyboardLayout(dwThreadId);
    PIMEDPI pImeDpi = ImmLockImeDpi(hKL);
    if (!pImeDpi)
    {
        ERR("!pImeDpi\n");
        ImmUnlockIMC(hIMC);
        return 0;
    }

    /* ImeGetImeMenuItems is optional */
    if (!pImeDpi->ImeGetImeMenuItems)
    {
        WARN("ImeGetImeMenuItems is not available (optional).\n");
        ImmUnlockImeDpi(pImeDpi);
        ImmUnlockIMC(hIMC);
        return 0;
    }

    /* Is the IME ANSI? */
    BOOL bImcIsAnsi = Imm32IsImcAnsi(hIMC);

    IMEMENUITEMINFOA ParentA, *pItemA;
    IMEMENUITEMINFOW ParentW, *pItemW;
    PVOID pNewItems = NULL, pNewParent = NULL;

    /* Are text types (ANSI/Wide) different between IME and target? */
    if (bImcIsAnsi != bTargetIsAnsi)
    {
        DWORD cbTotal;
        if (bTargetIsAnsi)
        {
            /* Convert the parent */
            if (lpImeParentMenu)
            {
                Imm32ImeMenuAnsiToWide(lpImeParentMenu, &ParentW, pImeDpi->uCodePage, TRUE);
                pNewParent = &ParentW;
            }

            /* Allocate buffer for new items */
            if (lpImeMenuItems)
            {
                cbTotal = ((dwSize / sizeof(IMEMENUITEMINFOA)) * sizeof(IMEMENUITEMINFOW));
                pNewItems = ImmLocalAlloc(0, cbTotal);
                if (!pNewItems)
                {
                    ERR("!pNewItems\n");
                    goto Quit;
                }
            }
        }
        else
        {
            /* Convert the parent */
            if (lpImeParentMenu)
            {
                Imm32ImeMenuWideToAnsi(lpImeParentMenu, &ParentA, pImeDpi->uCodePage);
                pNewParent = &ParentA;
            }

            /* Allocate buffer for new items */
            if (lpImeMenuItems)
            {
                cbTotal = ((dwSize / sizeof(IMEMENUITEMINFOW)) * sizeof(IMEMENUITEMINFOA));
                pNewItems = ImmLocalAlloc(0, cbTotal);
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
        /* Get the items directly */
        pNewItems = lpImeMenuItems;
        pNewParent = lpImeParentMenu;
    }

    /* Get IME menu items from the IME */
    ret = pImeDpi->ImeGetImeMenuItems(hIMC, dwFlags, dwType, pNewParent, pNewItems, dwSize);
    if (!ret)
    {
        ERR("ImeGetImeMenuItems failed\n");
        goto Quit;
    }

    if (!lpImeMenuItems)
        goto Quit;

    if (bImcIsAnsi != bTargetIsAnsi) /* Are text types different? */
    {
        if (bTargetIsAnsi)
        {
            /* Convert the parent */
            if (pNewParent)
                Imm32ImeMenuWideToAnsi(pNewParent, lpImeParentMenu, pImeDpi->uCodePage);

            /* Convert the items */
            pItemW = pNewItems;
            pItemA = lpImeMenuItems;
            for (iItem = 0; iItem < ret; ++iItem, ++pItemW, ++pItemA)
            {
                Imm32ImeMenuWideToAnsi(pItemW, pItemA, pImeDpi->uCodePage);
            }
        }
        else
        {
            /* Convert the parent */
            if (pNewParent)
                Imm32ImeMenuAnsiToWide(pNewParent, lpImeParentMenu, pImeDpi->uCodePage, TRUE);

            /* Convert the items */
            pItemA = pNewItems;
            pItemW = lpImeMenuItems;
            for (iItem = 0; iItem < dwSize; ++iItem, ++pItemA, ++pItemW)
            {
                Imm32ImeMenuAnsiToWide(pItemA, pItemW, pImeDpi->uCodePage, TRUE);
            }
        }
    }

Quit:
    if (pNewItems != lpImeMenuItems)
        ImmLocalFree(pNewItems);
    ImmUnlockImeDpi(pImeDpi);
    ImmUnlockIMC(hIMC);
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
    _Out_writes_bytes_opt_(dwSize) PIMEMENUITEMINFOA lpImeMenu,
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
    _Out_writes_bytes_opt_(dwSize) PIMEMENUITEMINFOW lpImeMenu,
    _In_ DWORD dwSize)
{
    TRACE("(%p, 0x%lX, 0x%lX, %p, %p, 0x%lX)\n",
          hIMC, dwFlags, dwType, lpImeParentMenu, lpImeMenu, dwSize);
    return ImmGetImeMenuItemsAW(hIMC, dwFlags, dwType, lpImeParentMenu, lpImeMenu, dwSize, FALSE);
}
