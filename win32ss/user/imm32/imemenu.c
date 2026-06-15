/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IME menus
 * COPYRIGHT:   Copyright 1998 Patrik Stridvall
 *              Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
 *              Copyright 2017 James Tabor <james.tabor@reactos.org>
 *              Copyright 2018 Amine Khaldi <amine.khaldi@reactos.org>
 *              Copyright 2020-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

// See also: https://github.com/katahiromz/ImeMenuTest3

#define IMEMENUINFO_BUFFER_SIZE 0x20000
#define IMEMENUINFO_MAPPING_NAME L"ImmMenuInfo"

// IME menu info. 95% or more compatible to Windows
typedef struct tagIMEMENUINFO
{
    DWORD dwVersion;              // Must be 1
    DWORD dwBufferSize;           // Always 0x20000 (128KB)
    DWORD dwFlags;                // dwFlags of ImmGetImeMenuItems
    DWORD dwType;                 // dwType of ImmGetImeMenuItems
    ULONG_PTR dwParentOffset;     // Relative offset to parent menu data (or pointer)
    ULONG_PTR dwItemsOffset;      // Relative offset to menu items (or pointer)
    DWORD dwCount;                // # of items
    ULONG_PTR dwBitmapListOffset; // Relative offset to bitmap-node list head (or pointer)
    ULONG_PTR dwEndOffset;        // Offset to the bottom of this data (or pointer)
} IMEMENUINFO, *PIMEMENUINFO;

typedef struct tagBITMAPNODE
{
    ULONG_PTR dwNext;       // Relative offset to next node (or pointer)
    HBITMAP hbmpCached;     // Cached HBITMAP
    ULONG_PTR dibBitsPtr;   // Relative offset to DIB pixel data (or pointer)
    BITMAPINFOHEADER bmih;  // BITMAPINFOHEADER
} BITMAPNODE, *PBITMAPNODE;

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

static PVOID
Imm32WriteHBitmapToNode(
    _In_ HDC hDC,
    _In_ HBITMAP hbmp,
    _In_ PBITMAPNODE pNode,
    _In_ PIMEMENUINFO pView)
{
    BITMAPINFOHEADER *pBmih;
    DWORD nodeExtraSize;
    PVOID dibBitsPtr;
    HBITMAP hbmpTemp;
    HGDIOBJ hbmpOld;
    DWORD totalNeeded;
    int scanlines;

    if (!hbmp)
        return NULL;

    pBmih = &pNode->bmih;
    pBmih->biSize     = sizeof(BITMAPINFOHEADER);
    pBmih->biBitCount = 0;
    if (!GetDIBits(hDC, hbmp, 0, 0, NULL, (BITMAPINFO*)pBmih, DIB_RGB_COLORS))
    {
        ERR("Invalid hbmp: %p\n", hbmp);
        return NULL;
    }

    switch (pBmih->biBitCount)
    {
        case 16: case 32:
            nodeExtraSize = sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD);
            break;
        case 24:
            nodeExtraSize = sizeof(BITMAPINFOHEADER);
            break;
        default:
            nodeExtraSize = sizeof(BITMAPINFOHEADER);
            nodeExtraSize += (1 << pBmih->biBitCount) * sizeof(RGBQUAD);
            break;
    }

    totalNeeded = nodeExtraSize + pBmih->biSizeImage;
    if ((PBYTE)pBmih + totalNeeded + sizeof(ULONG_PTR) > (PBYTE)pView + pView->dwBufferSize)
    {
        ERR("No more space\n");
        return NULL;
    }

    dibBitsPtr = (PBYTE)pBmih + nodeExtraSize;
    pNode->dibBitsPtr = (ULONG_PTR)dibBitsPtr; // Absolute address; convert to relative later

    hbmpTemp = CreateCompatibleBitmap(hDC, pBmih->biWidth, labs(pBmih->biHeight));
    if (!hbmpTemp)
    {
        ERR("Out of memory\n");
        return NULL;
    }

    hbmpOld = SelectObject(hDC, hbmpTemp);
    scanlines = GetDIBits(hDC, hbmp, 0, labs(pBmih->biHeight), dibBitsPtr,
                          (PBITMAPINFO)pBmih, DIB_RGB_COLORS);
    SelectObject(hDC, hbmpOld);
    DeleteObject(hbmpTemp);

    if (!scanlines)
    {
        ERR("GetDIBits failed\n");
        return NULL;
    }

    return (PBYTE)pBmih + nodeExtraSize + pBmih->biSizeImage;
}

static BOOL
Imm32InitImeMenuView(
    PIMEMENUINFO pView,
    DWORD dwFlags,
    DWORD dwType,
    PIMEMENUITEMINFO lpImeParentMenu,
    PIMEMENUITEMINFO lpImeMenuItems,
    DWORD dwSize)
{
    ZeroMemory(pView, sizeof(*pView));
    pView->dwVersion    = 1;
    pView->dwBufferSize = IMEMENUINFO_BUFFER_SIZE;
    pView->dwFlags      = dwFlags;
    pView->dwType       = dwType;
    pView->dwCount      = dwSize / sizeof(IMEMENUITEMINFOW);

    if ((dwSize == 0 || !lpImeMenuItems) && !lpImeParentMenu)
    {
        // No parents nor items
    }
    else if (lpImeParentMenu)
    {
        // There is parent
        PIMEMENUITEMINFOW pParentSlot = (PIMEMENUITEMINFOW)(pView + 1);
        RtlCopyMemory(pParentSlot, lpImeParentMenu, sizeof(IMEMENUITEMINFOW));
        // Clear parent bitmaps (unused)
        pParentSlot->hbmpChecked   = NULL;
        pParentSlot->hbmpUnchecked = NULL;
        pParentSlot->hbmpItem      = NULL;
        // Offset (relative)
        pView->dwParentOffset = sizeof(*pView);
        pView->dwItemsOffset  = sizeof(*pView) + sizeof(IMEMENUITEMINFOW);
    }
    else
    {
        // No parent but some items
        pView->dwParentOffset = 0;
        pView->dwItemsOffset  = sizeof(*pView);
    }

    // SECURITY: Validate dwCount (ReactOS only)
    return pView->dwItemsOffset + pView->dwCount * sizeof(IMEMENUITEMINFOW) <= pView->dwBufferSize;
}

static PBITMAPNODE
Imm32SerializeBitmap(
    _In_ PIMEMENUINFO pView,
    _In_ HBITMAP      hbmp)
{
    PBITMAPNODE pListHead, pNode;
    PVOID pNewEnd;
    HWND hwndDesktop;
    HDC hDC, hMemDC;

    // Check bitmap caches
    pNode = (PBITMAPNODE)pView->dwBitmapListOffset;
    while (pNode)
    {
        if (pNode->hbmpCached == hbmp)
            return pNode; // Cache hit
        pNode = (PBITMAPNODE)pNode->dwNext;
    }

    // Boundary check
    pNode = (PBITMAPNODE)pView->dwEndOffset;
    if (!pNode ||
        (PBYTE)pNode < (PBYTE)pView ||
        (PBYTE)pNode >= (PBYTE)pView + pView->dwBufferSize ||
        // ReactOS only:
        (PBYTE)pNode + sizeof(BITMAPNODE) > (PBYTE)pView + pView->dwBufferSize)
    {
        ERR("Insufficient or incorrect space\n");
        return NULL;
    }

    hwndDesktop = GetDesktopWindow();
    hDC = GetDC(hwndDesktop);
    if (!hDC)
    {
        ERR("GetDC failed\n");
        return NULL;
    }

    hMemDC = CreateCompatibleDC(hDC);
    if (!hMemDC)
    {
        ERR("CreateCompatibleDC failed\n");
        ReleaseDC(hwndDesktop, hDC);
        return NULL;
    }
    pListHead = (PBITMAPNODE)pView->dwBitmapListOffset;
    pNode->dwNext = (ULONG_PTR)pListHead;

    pNewEnd = Imm32WriteHBitmapToNode(hMemDC, hbmp, pNode, pView);

    DeleteDC(hMemDC);
    ReleaseDC(hwndDesktop, hDC);

    if (!pNewEnd) // Failure
    {
        ERR("No more space\n");
        pView->dwEndOffset = (ULONG_PTR)pNode;
        pView->dwBitmapListOffset = pNode->dwNext;
        return NULL;
    }

    pNode->hbmpCached = hbmp;
    pNode->dwNext = (ULONG_PTR)pListHead;
    pView->dwBitmapListOffset = (ULONG_PTR)pNode;
    pView->dwEndOffset     = (ULONG_PTR)pNewEnd;

    return pNode;
}

static HBITMAP
Imm32DeserializeBitmap(
    _Inout_ PBITMAPNODE pNode)
{
    if (!pNode || !pNode->dibBitsPtr)
        return NULL;

    if (pNode->hbmpCached)
        return pNode->hbmpCached;

    HDC hDC = GetDC(GetDesktopWindow());
    if (!hDC)
        return NULL;

    HDC hCompatDC = CreateCompatibleDC(hDC);
    if (!hCompatDC)
    {
        ReleaseDC(GetDesktopWindow(), hDC);
        return NULL;
    }

    HBITMAP hbmp = CreateDIBitmap(hCompatDC, &pNode->bmih, CBM_INIT, (PVOID)pNode->dibBitsPtr,
                                  (PBITMAPINFO)&pNode->bmih, DIB_RGB_COLORS);
    pNode->hbmpCached = hbmp;

    DeleteDC(hCompatDC);
    ReleaseDC(GetDesktopWindow(), hDC);

    return hbmp;
}

static BOOL
Imm32SerializeImeMenu(HIMC hIMC, PIMEMENUINFO pView)
{
    PIMEMENUITEMINFOW pParent, pItems, pTempBuf;
    DWORD i, dwCount, dwTempBufSize;
    PBITMAPNODE pNode;
    BOOL ret = FALSE;

    if (pView->dwVersion != 1)
    {
        ERR("dwVersion mismatch: 0x%08lX\n", pView->dwVersion);
        return FALSE;
    }

    if (pView->dwParentOffset)
    {
        // Convert relative to absolute
        pParent = (PIMEMENUITEMINFOW)((PBYTE)pView + pView->dwParentOffset);
        if ((PBYTE)pParent < (PBYTE)pView || (PBYTE)pParent >= (PBYTE)pView + pView->dwBufferSize)
        {
            ERR("Invalid dwParentOffset\n");
            return FALSE;
        }
        pView->dwParentOffset = (ULONG_PTR)pParent;
    }
    else
    {
        pParent = NULL;
    }

    if (pView->dwItemsOffset)
    {
        // Convert relative to absolute
        pItems = (PIMEMENUITEMINFOW)((PBYTE)pView + pView->dwItemsOffset);
        if ((PBYTE)pItems < (PBYTE)pView || (PBYTE)pItems >= (PBYTE)pView + pView->dwBufferSize)
        {
            ERR("Invalid dwItemsOffset\n");
            return FALSE;
        }
        pView->dwItemsOffset = (ULONG_PTR)pItems;
    }
    else
    {
        pItems = NULL;
    }

    // Allocate items buffer
    if (pView->dwCount > 0)
    {
        dwTempBufSize = pView->dwCount * sizeof(IMEMENUITEMINFOW);
        pTempBuf = ImmLocalAlloc(0, dwTempBufSize);
        if (!pTempBuf)
        {
            ERR("Out of memory\n");
            return FALSE;
        }
    }
    else
    {
        pTempBuf = NULL;
        dwTempBufSize = 0;
    }

    dwCount = ImmGetImeMenuItemsW(hIMC, pView->dwFlags, pView->dwType, pParent,
                                  pTempBuf, dwTempBufSize);
    pView->dwCount = dwCount;

    if (dwCount == 0) // No items?
    {
        if (pTempBuf)
            ImmLocalFree(pTempBuf);

        ret = TRUE;
        goto ConvertBack;
    }

    if (!pTempBuf)
    {
        ret = TRUE;
        goto ConvertBack;
    }

    pView->dwBitmapListOffset = 0;

    // Compute end of items buffer and ensure it stays within the mapped buffer
    {
        PBYTE pBufferEnd = (PBYTE)pView + pView->dwBufferSize;
        PBYTE pItemsEnd  = (PBYTE)pItems + dwCount * sizeof(IMEMENUITEMINFOW);
        if (pItemsEnd > pBufferEnd)
        {
            ret = FALSE;
            goto FreeAndCleanup;
        }
        pView->dwEndOffset = (ULONG_PTR)pItemsEnd;
    }

    // Serialize items
    for (i = 0; i < dwCount; i++)
    {
        PIMEMENUITEMINFOW pSrc  = &pTempBuf[i];
        PIMEMENUITEMINFOW pDest = pItems + i;
        *pDest = *pSrc;

        // Serialize hbmpChecked
        if (pSrc->hbmpChecked)
        {
            pNode = Imm32SerializeBitmap(pView, pSrc->hbmpChecked);
            if (!pNode)
                goto FreeAndCleanup;
            pDest->hbmpChecked = (HBITMAP)(ULONG_PTR)pNode; // Absolute pointer
        }
        else
        {
            pDest->hbmpChecked = NULL;
        }

        // Serialize hbmpUnchecked
        if (pSrc->hbmpUnchecked)
        {
            pNode = Imm32SerializeBitmap(pView, pSrc->hbmpUnchecked);
            if (!pNode)
                goto FreeAndCleanup;
            pDest->hbmpUnchecked = (HBITMAP)(ULONG_PTR)pNode;
        }
        else
        {
            pDest->hbmpUnchecked = NULL;
        }

        // Serialize hbmpItem
        if (pSrc->hbmpItem)
        {
            pNode = Imm32SerializeBitmap(pView, pSrc->hbmpItem);
            if (!pNode)
                goto FreeAndCleanup;
            pDest->hbmpItem = (HBITMAP)(ULONG_PTR)pNode;
        }
        else
        {
            pDest->hbmpItem = NULL;
        }
    }

    ret = TRUE;

FreeAndCleanup:
    ImmLocalFree(pTempBuf);

ConvertBack:
    // Convert dwItemsOffset to relative
    if (pView->dwItemsOffset)
        pView->dwItemsOffset = pView->dwItemsOffset - (ULONG_PTR)pView;

    if (pView->dwCount > 0 && pItems)
    {
        // Convert bitmaps to relative
        for (i = 0; i < pView->dwCount; i++)
        {
            PIMEMENUITEMINFOW pItem = pItems + i;
            if (pItem->hbmpChecked)
                pItem->hbmpChecked = (HBITMAP)((PBYTE)pItem->hbmpChecked - (PBYTE)pView);
            if (pItem->hbmpUnchecked)
                pItem->hbmpUnchecked = (HBITMAP)((PBYTE)pItem->hbmpUnchecked - (PBYTE)pView);
            if (pItem->hbmpItem)
                pItem->hbmpItem = (HBITMAP)((PBYTE)pItem->hbmpItem - (PBYTE)pView);
        }
    }

    // Convert dwBitmapListOffset to relative
    if (pView->dwBitmapListOffset)
    {
        PBITMAPNODE pCur = (PBITMAPNODE)pView->dwBitmapListOffset;
        pView->dwBitmapListOffset = (ULONG_PTR)pCur - (ULONG_PTR)pView;

        while (pCur)
        {
            PBITMAPNODE pNext = (PBITMAPNODE)pCur->dwNext;

            if (pCur->dibBitsPtr)
                pCur->dibBitsPtr = pCur->dibBitsPtr - (ULONG_PTR)pView;
            if (pCur->dwNext)
                pCur->dwNext = (ULONG_PTR)pNext - (ULONG_PTR)pView;

            pCur = pNext;
        }
    }

    // Convert dwParentOffset to relative
    if (pView->dwParentOffset && (PBYTE)pView->dwParentOffset >= (PBYTE)pView)
        pView->dwParentOffset = pView->dwParentOffset - (ULONG_PTR)pView;

    return ret;
}

static DWORD
Imm32DeserializeImeMenu(
    _In_ PIMEMENUINFO pView,
    _Out_opt_ PIMEMENUITEMINFOW lpImeMenuItems,
    _In_ DWORD dwSize)
{
    DWORD i, dwCount;
    PBYTE pViewBase = (PBYTE)pView;
    PBITMAPNODE pNode;
    PIMEMENUITEMINFOW pItemsBase;

    dwCount = pView->dwCount;

    if (!lpImeMenuItems)
        return dwCount;

    if (dwCount == 0)
        return 0;

    // SECURITY: Validate dwSize (ReactOS only)
    dwCount = min(dwCount, dwSize / sizeof(IMEMENUITEMINFOW));

    if (pView->dwBitmapListOffset)
    {
        PBITMAPNODE pCur = (PBITMAPNODE)(pViewBase + pView->dwBitmapListOffset);
        pView->dwBitmapListOffset = (ULONG_PTR)pCur;

        while (pCur)
        {
            PBITMAPNODE pNextCur;

            pCur->hbmpCached = NULL;

            // dibBitsPtr: relative to absolute
            if (pCur->dibBitsPtr)
                pCur->dibBitsPtr = (ULONG_PTR)(pViewBase + pCur->dibBitsPtr);

            // dwNext: relative to absolute
            if (pCur->dwNext)
            {
                pNextCur = (PBITMAPNODE)(pViewBase + pCur->dwNext);
                if ((PBYTE)pNextCur < pViewBase || 
                    (PBYTE)pNextCur >= pViewBase + pView->dwBufferSize)
                {
                    ERR("Invalid dwNext\n");
                    return 0;
                }
                pCur->dwNext = (ULONG_PTR)pNextCur;
                pCur = pNextCur;
            }
            else
            {
                pCur->dwNext = 0;
                pCur = NULL;
            }
        }
    }

    // dwItemsOffset: relative to absolute
    if (pView->dwItemsOffset)
    {
        pItemsBase = (PIMEMENUITEMINFOW)(pViewBase + pView->dwItemsOffset);
        // Boundary check: base pointer must be inside the view
        if ((PBYTE)pItemsBase < pViewBase || (PBYTE)pItemsBase >= pViewBase + pView->dwBufferSize)
            return 0;

        // Boundary check (ReactOS only): ensure the whole items array fits inside the view
        if (dwCount > 0)
        {
            SIZE_T offsetFromBase = (SIZE_T)((PBYTE)pItemsBase - pViewBase);
            SIZE_T available      = (SIZE_T)pView->dwBufferSize - offsetFromBase;
            SIZE_T needed         = (SIZE_T)dwCount * sizeof(IMEMENUITEMINFOW);

            if (available < needed)
                return 0;
        }

        pView->dwItemsOffset = (ULONG_PTR)pItemsBase;
    }
    else
    {
        return 0;
    }

    // Bitmap offsets: relative to absolute
    for (i = 0; i < dwCount; i++)
    {
        PIMEMENUITEMINFOW pItem =
            (PIMEMENUITEMINFOW)((PBYTE)pView->dwItemsOffset + i * sizeof(IMEMENUITEMINFOW));

        // hbmpChecked
        if (pItem->hbmpChecked)
        {
            PBITMAPNODE pN = (PBITMAPNODE)(pViewBase + (ULONG_PTR)pItem->hbmpChecked);
            if ((PBYTE)pN < pViewBase || (PBYTE)pN >= pViewBase + pView->dwBufferSize)
            {
                ERR("Invalid hbmpChecked\n");
                return 0;
            }
            pItem->hbmpChecked = (HBITMAP)(ULONG_PTR)pN;
        }

        // hbmpUnchecked
        if (pItem->hbmpUnchecked)
        {
            PBITMAPNODE pN = (PBITMAPNODE)(pViewBase + (ULONG_PTR)pItem->hbmpUnchecked);
            if ((PBYTE)pN < pViewBase || (PBYTE)pN >= pViewBase + pView->dwBufferSize)
            {
                ERR("Invalid hbmpUnchecked\n");
                return 0;
            }
            pItem->hbmpUnchecked = (HBITMAP)(ULONG_PTR)pN;
        }

        // hbmpItem
        if (pItem->hbmpItem)
        {
            PBITMAPNODE pN = (PBITMAPNODE)(pViewBase + (ULONG_PTR)pItem->hbmpItem);
            if ((PBYTE)pN < pViewBase || (PBYTE)pN >= pViewBase + pView->dwBufferSize)
            {
                ERR("Invalid hbmpItem\n");
                return 0;
            }
            pItem->hbmpItem = (HBITMAP)(ULONG_PTR)pN;
        }
    }

    // De-serialize items
    for (i = 0; i < dwCount; i++)
    {
        PIMEMENUITEMINFOW pSrc =
            (PIMEMENUITEMINFOW)((PBYTE)pView->dwItemsOffset + i * sizeof(IMEMENUITEMINFOW));
        PIMEMENUITEMINFOW pDst = &lpImeMenuItems[i];

        // Copy scalar fields (excluding HBITMAP fields)
        pDst->cbSize     = pSrc->cbSize;
        pDst->fType      = pSrc->fType;
        pDst->fState     = pSrc->fState;
        pDst->wID        = pSrc->wID;
        pDst->dwItemData = pSrc->dwItemData;

        // Copy szString
        StringCbCopyW(pDst->szString, sizeof(pDst->szString), pSrc->szString);

        // De-serialize hbmpChecked
        pNode = (PBITMAPNODE)(ULONG_PTR)pSrc->hbmpChecked;
        pDst->hbmpChecked = Imm32DeserializeBitmap(pNode);

        // De-serialize hbmpUnchecked
        pNode = (PBITMAPNODE)(ULONG_PTR)pSrc->hbmpUnchecked;
        pDst->hbmpUnchecked = Imm32DeserializeBitmap(pNode);

        // De-serialize hbmpItem
        pNode = (PBITMAPNODE)(ULONG_PTR)pSrc->hbmpItem;
        pDst->hbmpItem = Imm32DeserializeBitmap(pNode);
    }

    return dwCount;
}

static DWORD
ImmGetImeMenuItemsInterProcess(
    _In_ HIMC hIMC,
    _In_ DWORD dwFlags,
    _In_ DWORD dwType,
    _Inout_opt_ PVOID lpImeParentMenu,
    _Out_writes_bytes_opt_(dwSize) PVOID lpImeMenuItems,
    _In_ DWORD dwSize)
{
    HWND         hwndIme;
    HANDLE       hMapping = NULL;
    PIMEMENUINFO pView    = NULL;
    DWORD        dwCount  = 0;

    // Get IME window
    hwndIme = (HWND)NtUserQueryInputContext(hIMC, QIC_DEFAULTWINDOWIME);
    if (!hwndIme || !IsWindow(hwndIme))
        return 0;

    RtlEnterCriticalSection(&gcsImeDpi);

    // Create a file mapping
    hMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                  0, IMEMENUINFO_BUFFER_SIZE, IMEMENUINFO_MAPPING_NAME);
    if (!hMapping)
        goto Cleanup;

    // Map view of file mapping
    pView = MapViewOfFile(hMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    if (!pView)
        goto Cleanup;

    if (!Imm32InitImeMenuView(pView, dwFlags, dwType, lpImeParentMenu, lpImeMenuItems, dwSize))
        goto Cleanup;

    // WM_IME_SYSTEM:IMS_GETIMEMENU will invoke ImmPutImeMenuItemsIntoMappedFile to serialize
    if (!SendMessageW(hwndIme, WM_IME_SYSTEM, IMS_GETIMEMENU, (LPARAM)hIMC))
        goto Cleanup;

    dwCount = pView->dwCount;

    if (!lpImeMenuItems)
        goto Cleanup;

    // De-serialize IME menu
    dwCount = Imm32DeserializeImeMenu(pView, lpImeMenuItems, dwSize);

Cleanup:
    if (pView)
        UnmapViewOfFile(pView);
    if (hMapping)
        CloseHandle(hMapping);

    RtlLeaveCriticalSection(&gcsImeDpi);

    return dwCount;
}

/***********************************************************************
 *		ImmPutImeMenuItemsIntoMappedFile (IMM32.@)
 *
 * Called from user32.dll to transport the IME menu items by using a
 * file mapping object. This function is provided for WM_IME_SYSTEM:IMS_GETIMEMENU
 * handling.
 */
BOOL WINAPI
ImmPutImeMenuItemsIntoMappedFile(_In_ HIMC hIMC)
{
    /* Open the existing file mapping */
    HANDLE hMapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, IMEMENUINFO_MAPPING_NAME);
    if (!hMapping)
    {
        ERR("OpenFileMappingW failed\n");
        return FALSE;
    }

    PIMEMENUINFO pView = MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!pView)
    {
        ERR("MapViewOfFile failed\n");
        CloseHandle(hMapping);
        return FALSE;
    }

    BOOL ret = Imm32SerializeImeMenu(hIMC, pView);
    UnmapViewOfFile(pView);
    CloseHandle(hMapping);
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
        return ImmGetImeMenuItemsInterProcess(hIMC, dwFlags, dwType, lpImeParentMenu,
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
