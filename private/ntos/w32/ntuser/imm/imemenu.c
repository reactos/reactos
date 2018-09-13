//////////////////////////////////////////////////////////////////////////
// imemenu.c -     IME Menu APIs
//
// handles IME specific menu retrieval
//
// Copyright (c) 1985 - 1999, Microsoft Corporation
//
// History:
// 23-Mar-1997 hiroyama Created
//////////////////////////////////////////////////////////////////////////

#include "precomp.h"

#ifdef HIRO_DEBUG
#define D(x)    x
#else
#define D(x)
#endif

#define IME_MENU_FILE_NAME  L"ImmMenuInfo"
#define IME_MENU_MAXMEM     (128 * 1024)   // maximum size of mapped file

////////////////////////////////////////////////////////////
// private structurs for inter process communication
//
// NOTE for NT50: these are dedicated to internat.exe
//
// uses memory mapped file as shared buffer
// all strings expect UNICODE
// HBITMAP is de-compiled and then compiled again using
// internat.exe's context
//
////////////////////////////////////////////////////////////

typedef struct _IMEMENU_BMP_HEADER {
    struct _IMEMENU_BMP_HEADER* lpNext;
    HBITMAP hBitmap;
    LPBYTE pBits;
    BITMAPINFO bmi;
} IMEMENU_BMP_HEADER;

typedef struct {
    UINT cbSize;
    UINT fType;
    UINT fState;
    UINT wID;
    IMEMENU_BMP_HEADER* lpBmpChecked;
    IMEMENU_BMP_HEADER* lpBmpUnchecked;
    DWORD dwItemData;
    WCHAR szString[IMEMENUITEM_STRING_SIZE];    // menu string: always UNICODE.
    IMEMENU_BMP_HEADER* lpBmpItem;   // NULL means no bitmap in this menu
} IMEMENU_ITEM;

typedef struct {
    DWORD dwVersion;                // holds version of this memory chunk
    DWORD dwMemSize;                // size of memory buffer allocated
    DWORD dwFlags;                  // flags returned from IME
    DWORD dwType;
    IMEMENU_ITEM* lpImeParentMenu;  // parent menu's offset (if any) passed from requester
    IMEMENU_ITEM* lpImeMenu;        // offset to first menu item (will be set by IME side)
    DWORD dwSize;                   // number of menus to fill (not byte count)
    IMEMENU_BMP_HEADER* lpBmp;      // offset to first bitmap header
    IMEMENU_BMP_HEADER* lpBmpNext;  // points next available location for bmp buffer
} IMEMENU_HEADER;


// address conversion
#define CONVTO_OFFSET(x)    ((x) = (LPVOID)((x) ? ((LPBYTE)(x) - offset) : NULL))
#define CONVTO_PTR(x)       ((x) = (LPVOID)((x) ? ((LPBYTE)(x) + offset) : NULL))

#if DBG
#define CHK_OFFSET(x)       if ((ULONG_PTR)(x) >= pHeader->dwMemSize) { \
                                RIPMSG2(RIP_WARNING, "CHK_OFFSET(%s=%lx) is out of range.", #x, (ULONG_PTR)(x)); \
                            }
#define CHK_PTR(x)          if ((LPVOID)(x) < (LPVOID)pHeader || (LPBYTE)(x) > (LPBYTE)pHeader + pHeader->dwMemSize) { \
                                if ((x) != NULL) { \
                                    RIPMSG2(RIP_WARNING, "CHK_PTR(%s=%lx) is out of range.", #x, (ULONG_PTR)(x)); \
                                    DebugBreak(); \
                                } \
                            }
#else
#define CHK_OFFSET(x)
#define CHK_PTR(x)          if ((x) != NULL) { \
                                if ((LPVOID)(x) < (LPVOID)pHeader || (LPBYTE)(x) > (LPBYTE)pHeader + pHeader->dwMemSize) { \
                                    goto cleanup; \
                                } \
                            }
#endif

void ConvertImeMenuItemInfoAtoW(LPIMEMENUITEMINFOA lpA, LPIMEMENUITEMINFOW lpW, int nCP, BOOL copyBmp)
{
    int i;

    lpW->cbSize         = lpA->cbSize;
    lpW->fType          = lpA->fType;
    lpW->fState         = lpA->fState;
    lpW->wID            = lpA->wID;
    if (copyBmp) {
        lpW->hbmpChecked    = lpA->hbmpChecked;
        lpW->hbmpUnchecked  = lpA->hbmpUnchecked;
        lpW->hbmpItem       = lpA->hbmpItem;
    }
    lpW->dwItemData     = lpA->dwItemData;

    i = MultiByteToWideChar(nCP,
                            0,
                            lpA->szString,
                            lstrlenA(lpA->szString),
                            lpW->szString,
                            IMEMENUITEM_STRING_SIZE);

    lpW->szString[i] = L'\0';
}

void ConvertImeMenuItemInfoWtoA(LPIMEMENUITEMINFOW lpW, LPIMEMENUITEMINFOA lpA, int nCP)
{
    int i;
    BOOL bUDC;

    lpA->cbSize         = lpW->cbSize;
    lpA->fType          = lpW->fType;
    lpA->fState         = lpW->fState;
    lpA->wID            = lpW->wID;
    lpA->hbmpChecked    = lpW->hbmpChecked;
    lpA->hbmpUnchecked  = lpW->hbmpUnchecked;
    lpA->dwItemData     = lpW->dwItemData;
    lpA->hbmpItem       = lpW->hbmpItem;


    i = WideCharToMultiByte(nCP,
                            0,
                            lpW->szString,
                            wcslen(lpW->szString),
                            lpA->szString,
                            IMEMENUITEM_STRING_SIZE,
                            (LPSTR)NULL,
                            &bUDC);

    lpA->szString[i] = '\0';
}


#if DBG
void DumpBytes(LPBYTE pb, UINT size)
{
    UINT i;
    TRACE(("\npbmi dump:"));
    for (i = 0; i < size; ++i) {
        TRACE(("%02X ", pb[i] & 0xff));
    }
    TRACE(("\n"));
    UNREFERENCED_PARAMETER(pb); // just in case
}
#else
#define DumpBytes(a,b)
#endif

////////////////////////////////////////////////////////////////////
// SaveBitmapToMemory

IMEMENU_BMP_HEADER* SaveBitmapToMemory(HDC hDC, HBITMAP hBmp, IMEMENU_BMP_HEADER* lpBH, IMEMENU_HEADER* pHeader)
{
    HBITMAP hTmpBmp, hBmpOld;
    IMEMENU_BMP_HEADER* lpNext = NULL;
    PBITMAPINFO pbmi = &lpBH->bmi;
    ULONG sizBMI;


    if (!hBmp) {
        RIPMSG0(RIP_WARNING, "SaveBitmapToMemory: hBmp == NULL");
        return NULL;
    }
    UserAssert(lpBH != NULL);

    //
    // Let the graphics engine to retrieve the dimension of the bitmap for us
    // GetDIBits uses the size to determine if it's BITMAPCOREINFO or BITMAPINFO
    // if BitCount != 0, color table will be retrieved
    //
    pbmi->bmiHeader.biSize = sizeof pbmi->bmiHeader;
    pbmi->bmiHeader.biBitCount = 0;             // don't get the color table
    if ((GetDIBits(hDC, hBmp, 0, 0, (LPSTR)NULL, pbmi, DIB_RGB_COLORS)) == 0) {
        RIPMSG0(RIP_WARNING, "SaveBitmapToMemory: failed to GetDIBits(NULL)");
       return NULL;
    }


    //
    // Note: 24 bits per pixel has no color table.  So, we don't have to
    // allocate memory for retrieving that.  Otherwise, we do.
    //
    switch (pbmi->bmiHeader.biBitCount) {
        case 24:                                      // has color table
            sizBMI = sizeof(BITMAPINFOHEADER);
            break;
        case 16:
        case 32:
            sizBMI = sizeof(BITMAPINFOHEADER) + sizeof(DWORD) * 3;
            break;
        default:
            sizBMI = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << pbmi->bmiHeader.biBitCount);
            break;

    }

    //
    // check if the buffer has enough space to put bitmap
    //
    if ((LPBYTE)pHeader + pHeader->dwMemSize < (LPBYTE)lpBH + sizeof lpBH + sizBMI + pbmi->bmiHeader.biSizeImage) {
        RIPMSG0(RIP_WARNING, "SaveBitmapToMemory: size of bmp image(s) exceed limit ");
        return FALSE;
    }

    //
    // Now that we know the size of the image, let pBits point the given buffer
    //
    lpBH->pBits = (LPBYTE)pbmi + sizBMI;

    //
    // Bitmap can't be selected into a DC when calling GetDIBits
    // Assume that the hDC is the DC where the bitmap would have been selected
    // if indeed it has been selected
    //
    if (hTmpBmp = CreateCompatibleBitmap(hDC, pbmi->bmiHeader.biWidth, pbmi->bmiHeader.biHeight)) {
        hBmpOld = SelectObject(hDC, hTmpBmp);
        if (GetDIBits(hDC, hBmp, 0, pbmi->bmiHeader.biHeight, (LPSTR)lpBH->pBits, pbmi, DIB_RGB_COLORS) == 0){
            SelectObject(hDC, hBmpOld);
            RIPMSG0(RIP_WARNING, "SaveBitmapToMemory: GetDIBits() failed.");
            return NULL;
        }
        lpNext = (IMEMENU_BMP_HEADER*)((LPBYTE)pbmi + sizBMI + pbmi->bmiHeader.biSizeImage);

        DumpBytes((LPBYTE)pbmi, sizeof *pbmi);
    } else {
        RIPMSG0(RIP_WARNING, "SaveBitmapToMemory: CreateCompatibleBitmap() failed.");
        return NULL;
    }

    SelectObject(hDC, hBmpOld);
    DeleteObject(hTmpBmp);
    return lpNext;
}

//////////////////////////////////////////////////////////////////////////////
// DecompileBitmap()
//
// decompile given hBitmap into IMEMENU_BMP_HEADER
// manupilate IMEMENU_BMP_HEADER links in IMEMENU_HEADER
//
// History:
// 23-Mar-1997 HiroYama Created
//////////////////////////////////////////////////////////////////////////////

IMEMENU_BMP_HEADER* DecompileBitmap(IMEMENU_HEADER* pHeader, HBITMAP hBitmap)
{
    IMEMENU_BMP_HEADER* pBmp = pHeader->lpBmp;
    HDC hDC;

    // first search handled bitmap
    while (pBmp) {
        if (pBmp->hBitmap == hBitmap) {
            // if hBitmap is already de-compiled, return it
            return pBmp;
        }
        pBmp = pBmp->lpNext;
    }

    // not yet allocated, so prepare memory buffer
    pBmp = pHeader->lpBmpNext;
    UserAssert(pBmp != NULL);
    CHK_PTR(pBmp);
    if (pBmp == NULL) {
        RIPMSG1(RIP_WARNING, "DecompileBitmap: pBmp == NULL in L%d", __LINE__);
        return NULL;
    }

    // use desktop's DC
    hDC = GetDC(GetDesktopWindow());
    if (hDC == NULL) {
        RIPMSG1(RIP_WARNING, "DecompileBitmap: hDC == NULL in L%d", __LINE__);
        return NULL;
    }

    //
    // decompile hBitmap
    //
    pBmp->lpNext = pHeader->lpBmp;
    pHeader->lpBmpNext = SaveBitmapToMemory(hDC, hBitmap, pBmp, pHeader);
    if (pHeader->lpBmpNext == NULL) {
        RIPMSG1(RIP_WARNING, "DecompileBitmap: pHeader->lpBmpNext == NULL in L%d", __LINE__);
        // error case. restore bmp link, then returns NULL
        pHeader->lpBmpNext = pBmp;
        pHeader->lpBmp = pBmp->lpNext;
        pBmp = NULL;
        goto cleanup;
    }

    // if succeeded, mark this BITMAP_HEADER with hBitmap
    pBmp->hBitmap = hBitmap;

    //
    // put this BITMAP_HEADER in linked list
    //
    pHeader->lpBmp = pBmp;

cleanup:
    if (hDC)
        ReleaseDC(GetDesktopWindow(), hDC);
    return pBmp;
}

//////////////////////////////////////////////////////////////////////////////
// ImmPutImeMenuItemsIntoMappedFile()
//
// Interprocess IME Menu handler
//
// called from ImeSystemHandler() in user32.dll
//
// handler of WM_IME_SYSTEM:IMS_MENU_ITEM
//
// History:
// 23-Mar-1997 HiroYama Created
//////////////////////////////////////////////////////////////////////////////

LRESULT ImmPutImeMenuItemsIntoMappedFile(HIMC hImc)
{
    HANDLE hMap = NULL;
    LPVOID lpMap = NULL;
    IMEMENU_HEADER* pHeader;
    LPIMEMENUITEMINFO lpBuf = NULL;
    IMEMENU_ITEM* pMenu;
    IMEMENU_BMP_HEADER* pBmp;
    LRESULT lRet = 0;
    ULONG_PTR offset;
    DWORD i;

    // Open memory mapped file
    hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, IME_MENU_FILE_NAME);
    if (hMap == NULL) {
        RIPMSG0(RIP_WARNING, "ImmPutImeMenuItemsIntoMappedFile: cannot open mapped file.");
        return 0L;
    }

    // Map entire view of the file into the process memory space
    lpMap = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (lpMap == NULL) {
        RIPMSG0(RIP_WARNING, "ImmPutImeMenuItemsIntoMappedFile: cannot map view of file.");
        goto cleanup;
        // I wish if I could use C++...
    }

    pHeader = (IMEMENU_HEADER*)lpMap;

    ///////////////////
    // Version check
    ///////////////////
    if (pHeader->dwVersion != 1) {
        RIPMSG1(RIP_WARNING, "ImmPutImeMenuItemsIntoMappedFile: dwVersion(%d) does not match.",
                pHeader->dwVersion);
        goto cleanup;
    }

    //////////////////////////////
    // convert offset to pointer
    offset = (ULONG_PTR)pHeader;
    CONVTO_PTR(pHeader->lpImeParentMenu);
    CHK_PTR(pHeader->lpImeParentMenu);
    pMenu = CONVTO_PTR(pHeader->lpImeMenu);
    CHK_PTR(pHeader->lpImeMenu);
    if (pHeader->dwSize) {
        UserAssert(pHeader->lpImeMenu);    // if dwSize is specified, we need real buffer here
        lpBuf = ImmLocalAlloc(HEAP_ZERO_MEMORY, pHeader->dwSize * sizeof(IMEMENUITEMINFOW));
        if (lpBuf == NULL) {
            RIPMSG0(RIP_WARNING, "ImmPutImeMenuItemsIntoMappedFile: not enough memory for receiver's buffer.");
            goto cleanup;
        }
    }


    // preparation
#if DBG
    if (pHeader->lpImeParentMenu) {
        UserAssert(!pHeader->lpImeParentMenu->lpBmpChecked &&
                   !pHeader->lpImeParentMenu->lpBmpUnchecked &&
                   !pHeader->lpImeParentMenu->lpBmpItem);
    }
#endif

    //////////////////////////////////
    // Get IME menus
    pHeader->dwSize = ImmGetImeMenuItemsW(hImc, pHeader->dwFlags, pHeader->dwType,
                                 (LPIMEMENUITEMINFOW)pHeader->lpImeParentMenu, lpBuf,
                                  pHeader->dwSize * sizeof(IMEMENUITEMINFOW));
    // now, pHeader->dwSize contains number of menu items rather than byte size
    if (pHeader->dwSize == 0) {
        goto cleanup;
    }
    //////////////////////////////////

    //
    // Copy back the information
    //
    // if lpBuf != NULL, we need to copy back information
    //
    if (lpBuf) {
        LPIMEMENUITEMINFO lpMenuW = lpBuf;

        pHeader->lpBmp = NULL;
        // lpBmpNext will point first possible memory for bmp de-compile
        pHeader->lpBmpNext = (LPVOID)((LPBYTE)pHeader + (pHeader->dwSize + 1) * sizeof(IMEMENUITEMINFOW));

        // copy menuinfo
        for (i = 0; i < pHeader->dwSize; ++i, ++pMenu, ++lpMenuW) {
            RtlCopyMemory(pMenu, lpMenuW, sizeof *lpMenuW);
            // decompile hbitmap
            if (lpMenuW->hbmpChecked) {
                if ((pMenu->lpBmpChecked = DecompileBitmap(pHeader, lpMenuW->hbmpChecked)) == NULL) {
                    RIPMSG1(RIP_WARNING, "ImmPutImeMenuItemsIntoMappedFile: DecompileBitmap Failed in L%d", __LINE__);
                    goto cleanup;
                }
            }
            if (lpMenuW->hbmpUnchecked) {
                if ((pMenu->lpBmpUnchecked = DecompileBitmap(pHeader, lpMenuW->hbmpUnchecked)) == NULL) {
                    RIPMSG1(RIP_WARNING, "ImmPutImeMenuItemsIntoMappedFile: DecompileBitmap Failed in L%d", __LINE__);
                    goto cleanup;
                }
            }
            if (lpMenuW->hbmpItem) {
                if ((pMenu->lpBmpItem = DecompileBitmap(pHeader, lpMenuW->hbmpItem)) == NULL) {
                    RIPMSG1(RIP_WARNING, "ImmPutImeMenuItemsIntoMappedFile: DecompileBitmap Failed in L%d", __LINE__);
                    goto cleanup;
                }
            }
        }

        //////////////////////////////////////////////////////////////////////
        //
        // convert pointer to offset
        //

        pMenu = pHeader->lpImeMenu;
        CONVTO_OFFSET(pHeader->lpImeMenu);
        // no need to convert parent menu, so let it be NULL
        D(pHeader->lpImeParentMenu = NULL);

        // pointer to BITMAP_HEADER in each menu
        for (i = 0; i < pHeader->dwSize; ++i, ++pMenu) {
            TRACE(("ImmPutImeMenuItemsIntoMappedFile: convertiong '%S'\n", pMenu->szString));
            CONVTO_OFFSET(pMenu->lpBmpChecked);
            CONVTO_OFFSET(pMenu->lpBmpUnchecked);
            TRACE(("ImmPutImeMenuItemsIntoMappedFile: before conversion (%#lx)\n", pMenu->lpBmpItem));
            CONVTO_OFFSET(pMenu->lpBmpItem);
            TRACE(("ImmPutImeMenuItemsIntoMappedFile: after  conversion (%#lx)\n", pMenu->lpBmpItem));

            // check them
            CHK_OFFSET(pMenu->lpBmpChecked);
            CHK_OFFSET(pMenu->lpBmpChecked);
            CHK_OFFSET(pMenu->lpBmpItem);
        }

        //
        // first pointer to BITMAP_HEADER linked list
        //
        pBmp = pHeader->lpBmp;
        CONVTO_OFFSET(pHeader->lpBmp);
        CHK_OFFSET(pHeader->lpBmp);
        // pHeader->lpBmpNext will not be used, so let it be NULL
        D(pHeader->lpBmpNext = NULL);

        //
        // pointers in BITMAP_HEADER linked list
        //
        while (pBmp) {
            IMEMENU_BMP_HEADER* ptBmp = pBmp->lpNext;
            CONVTO_OFFSET(pBmp->pBits);
            CONVTO_OFFSET(pBmp->lpNext);
            CHK_OFFSET(pBmp->lpNext);
            pBmp = ptBmp;
        }
        //
        // pointer conversion finished
        //
        //////////////////////////////////////////////////////////////////////
    } // end if (lpBuf)

    //
    // everything went OK
    //
    lRet = 1;

cleanup:
    if (lpBuf)
        ImmLocalFree(lpBuf);
    if (lpMap)
        UnmapViewOfFile(lpMap);
    if (hMap)
        CloseHandle(hMap);
    return lRet;
}


//////////////////////////////////////////////////////////////////////////////
// InternalImeMenuCreateBitmap()
//
// create bitmap from IMEMENU_BMP_HEADER
//////////////////////////////////////////////////////////////////////////////

HBITMAP InternalImeMenuCreateBitmap(IMEMENU_BMP_HEADER* lpBH)
{
    HDC hDC;

    if (lpBH == NULL) {
        RIPMSG1(RIP_WARNING, "InternalImeMenuCreateBitmap: lpBH == NULL in L%d", __LINE__);
        return NULL;
    }
    if (lpBH->pBits == NULL) {
        RIPMSG1(RIP_WARNING, "InternalImeMenuCreateBitmap: lpBH->pBits == NULL in L%d", __LINE__);
        return NULL;
    }

    if (lpBH->hBitmap) {
        TRACE(("InternalImeMenuCreateBitmap: lpBH->hBitmap != NULL. will return it.\n"));
        return lpBH->hBitmap;
    }

    if (hDC = GetDC(GetDesktopWindow())) {
        HDC hMyDC = CreateCompatibleDC(hDC);
        if (hMyDC) {
            // (select palette) needed ?
            lpBH->hBitmap = CreateDIBitmap(hDC, &lpBH->bmi.bmiHeader, CBM_INIT,
                                                  lpBH->pBits, &lpBH->bmi, DIB_RGB_COLORS);
            if (lpBH->hBitmap == NULL) {
                DWORD dwErr = GetLastError();
                RIPMSG1(RIP_WARNING, "InternalImeMenuCreateBitmap: CreateDIBitmap Failed. Last error=%#x\n", dwErr);
            }
            DeleteDC(hMyDC);
        }
        else {
            RIPMSG0(RIP_WARNING, "InternalImeMenuCreateBitmap: CreateCompatibleDC failed.");
        }

        ReleaseDC(GetDesktopWindow(), hDC);
    }
    else {
        RIPMSG0(RIP_WARNING, "InternalImeMenuCreateBitmap: couldn't get Desktop DC.");
    }
    return lpBH->hBitmap;
}

//////////////////////////////////////////////////////////////////////////////
// ImmGetImeMenuItemsInterProcess()
//
// Inter process IME Menu handler
// sends WM_IME_SYSTEM:IMS_GETIMEMENU
//
// History:
// 23-Mar-1997 HiroYama Created
//////////////////////////////////////////////////////////////////////////////

DWORD ImmGetImeMenuItemsInterProcess(HIMC hImc,
                                     DWORD dwFlags,
                                     DWORD dwType,
                                     LPIMEMENUITEMINFOW lpParentMenu,
                                     LPIMEMENUITEMINFOW lpMenu,
                                     DWORD dwSize)
{
    HWND hwnd;
    HANDLE hMemFile = NULL;
    DWORD dwRet = 0;
    LPBYTE lpMap = NULL;
    IMEMENU_HEADER* pHeader;
    IMEMENU_ITEM* pMenuItem;
    IMEMENU_BMP_HEADER* pBmpHeader;
    DWORD i;
    ULONG_PTR offset;

    // Get default IME window
    //
    // Note: We do not consider user created HIMC here, because this inter-process call is intended to
    // support only internat.exe, and this message is passed as just a kick to IMM's def WinProc.
    hwnd = (HWND)NtUserQueryInputContext(hImc, InputContextDefaultImeWindow);
    if (hwnd == NULL || !IsWindow(hwnd)) {
        RIPMSG1(RIP_WARNING, "ImmGetImeMenuItemsInterProcess: hwnd(%lx) is not a valid window.", hwnd);
        return 0;
    }

    RtlEnterCriticalSection(&gcsImeDpi);

    // first, create memory mapped file
    hMemFile = CreateFileMapping((HANDLE)~0, NULL, PAGE_READWRITE,
                                 0, IME_MENU_MAXMEM, IME_MENU_FILE_NAME);
    if (hMemFile == NULL) {
        RIPMSG0(RIP_WARNING, "ImmGetImeMenuItemsInterProcess: cannot allocate memory mapped file.");
        goto cleanup;
    }
    // then get a view of the mapped file
    lpMap = (LPBYTE)MapViewOfFile(hMemFile, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    if (lpMap == NULL) {
        RIPMSG0(RIP_WARNING, "ImmGetImeMenuItemsInterProcess: cannot map view of memory mapped file.");
        goto cleanup;
    }

    //
    // shared buffer (memory mapped file) initialization
    //
    pHeader = (IMEMENU_HEADER*)lpMap;
    RtlZeroMemory(pHeader, sizeof *pHeader);
    pHeader->dwVersion = 1;
    pHeader->dwMemSize = IME_MENU_MAXMEM;
    pHeader->dwSize = dwSize / sizeof(IMEMENUITEMINFOW);    // CAUTION: dwSize could be 0.
    RIPMSG1(RIP_WARNING, "ImmGetImeMenuItemsInterProcess: pHeader->dwSize=%ld", pHeader->dwSize);
    pHeader->dwFlags = dwFlags;
    pHeader->dwType = dwType;

    //
    // 1) dwSize != 0 and lpMenu != NULL means, caller requests the given buffer filled
    // 2) if lpParentMenu is passed, we need to put its information in shared buffer
    //
    if ((dwSize && lpMenu) || lpParentMenu) {
        // if parent menu is specified, copy it here
        if (lpParentMenu) {
            IMEMENU_ITEM* pPMenu =
                pHeader->lpImeParentMenu = (IMEMENU_ITEM*)&pHeader[1];

            RtlCopyMemory(pPMenu, lpParentMenu, sizeof(IMEMENUITEMINFOW));

            // by design, IME will receive NULL hbmpItem in parent menu.
            // there is no way to guarantee the same hbmpItem is returned, thus NULL is passed.
            pPMenu->lpBmpChecked = pPMenu->lpBmpUnchecked = pPMenu->lpBmpItem = NULL;
            pHeader->lpImeMenu = pHeader->lpImeParentMenu + 1;
        }
        else {
            pHeader->lpImeParentMenu = NULL;
            pHeader->lpImeMenu = (LPVOID)&pHeader[1];
        }
        // convert pointer to offset
        offset = (ULONG_PTR)lpMap;
        CONVTO_OFFSET(pHeader->lpImeParentMenu);
        CONVTO_OFFSET(pHeader->lpImeMenu);
    }




    ///////////////////////////////////////////////////////////////////////
    if (!SendMessage(hwnd, WM_IME_SYSTEM, IMS_GETIMEMENU, (LPARAM)hImc)) {
        // if it fails
        goto cleanup;
    }
    ///////////////////////////////////////////////////////////////////////

    // NOTE: dwSize is maximum index of menu array. not a total byte size of array.
    dwSize = pHeader->dwSize;

    if (lpMenu) {
        ///////////////////////////////
        // convert offset to pointer
        ///////////////////////////////
        pMenuItem = CONVTO_PTR(pHeader->lpImeMenu);
        CHK_PTR(pMenuItem);
        // NOTE: we don't have to handle parent menu

        //
        // pointers to BITMAP_HEADER in each menu structure
        //
        for (i = 0; i < dwSize; ++i, ++pMenuItem) {
            CONVTO_PTR(pMenuItem->lpBmpChecked);
            CONVTO_PTR(pMenuItem->lpBmpUnchecked);
            CONVTO_PTR(pMenuItem->lpBmpItem);
            //
            // check the pointers
            //
            CHK_PTR(pMenuItem->lpBmpChecked);
            CHK_PTR(pMenuItem->lpBmpUnchecked);
            CHK_PTR(pMenuItem->lpBmpItem);
        }

        //
        // pointer to first BITMAP_HEADER
        //
        pBmpHeader = CONVTO_PTR(pHeader->lpBmp);

        //
        // each BITMAP_HEADER
        //
        while (pBmpHeader) {
            pBmpHeader->hBitmap = NULL;    // clear
            // pBits
            CONVTO_PTR(pBmpHeader->pBits);
            CHK_PTR(pBmpHeader->pBits);

            // next BITMAP_HEADER
            pBmpHeader = CONVTO_PTR(pBmpHeader->lpNext);
            CHK_PTR(pBmpHeader);
        }

        //
        // copy back the results
        //
        pMenuItem = pHeader->lpImeMenu;
        for (i = 0; i < dwSize; ++i, ++pMenuItem, ++lpMenu) {
            lpMenu->cbSize = pMenuItem->cbSize;
            lpMenu->fType = pMenuItem->fType;
            lpMenu->fState = pMenuItem->fState;
            lpMenu->wID = pMenuItem->wID;
            lpMenu->dwItemData = pMenuItem->dwItemData;
            wcscpy(lpMenu->szString, pMenuItem->szString);

            // Create bitmap from memory buffer
            // hbmp will be NULL if no bmp is specified.
            if (pMenuItem->lpBmpChecked) {
                lpMenu->hbmpChecked = InternalImeMenuCreateBitmap(pMenuItem->lpBmpChecked);
            }
            else {
                lpMenu->hbmpChecked = NULL;
            }
            if (pMenuItem->lpBmpUnchecked) {
                lpMenu->hbmpUnchecked = InternalImeMenuCreateBitmap(pMenuItem->lpBmpUnchecked);
            }
            else {
                lpMenu->hbmpUnchecked = NULL;
            }
            if (pMenuItem->lpBmpItem) {
                lpMenu->hbmpItem = InternalImeMenuCreateBitmap(pMenuItem->lpBmpItem);
            }
            else {
                lpMenu->hbmpItem = NULL;
            }
        }
    }


cleanup:
    if (lpMap) {
        UnmapViewOfFile(lpMap);
    }
    RtlLeaveCriticalSection(&gcsImeDpi);
    // destroy memory mapped file
    if (hMemFile) {
        CloseHandle(hMemFile);
    }

    return dwSize;
}

//////////////////////////////////////////////////////////////////////////////
// ImmGetImeMenuItemsWorker()
//
// Handler of IME Menu
//
// if specified HIMC belongs to other process, it calls
// ImmGetImeMenuItemsInterProcess()
//
// History:
// 23-Mar-1997 HiroYama Created
//////////////////////////////////////////////////////////////////////////////


DWORD ImmGetImeMenuItemsWorker(HIMC hIMC,
                               DWORD dwFlags,
                               DWORD dwType,
                               LPVOID lpImeParentMenu,
                               LPVOID lpImeMenu,
                               DWORD dwSize,
                               BOOL bAnsiOrigin)
{
    BOOL bAnsiIme = IsAnsiIMC(hIMC);
    DWORD dwRet = 0;
    LPINPUTCONTEXT lpInputContext;
    DWORD dwThreadId;
    PIMEDPI pImeDpi = NULL;
    LPVOID lpImePTemp = lpImeParentMenu;    // keeps parent menu
    LPVOID lpImeTemp = lpImeMenu;           // points menu buffer
    IMEMENUITEMINFOA imiiParentA;
    IMEMENUITEMINFOW imiiParentW;

    //
    // check if the call will be inter process
    //
    {
        DWORD dwProcessId = GetInputContextProcess(hIMC);
        if (dwProcessId == 0) {
            RIPMSG0(RIP_WARNING, "ImmGetImeMenuItemsWorker: dwProcessId == 0");
            return 0;
        }
        if (dwProcessId != GetCurrentProcessId()) {
            //
            // going to call another process' IME
            //
            TRACE(("ImmGetImeMenuItemsWorker: Inter process.\n"));
            if (bAnsiOrigin) {
                //
                // this inter-process thing is only allowed to internat.exe or equivalent
                //
                RIPMSG0(RIP_WARNING, "ImmGetImeMenuItemsWorker: interprocess getmenu is not allowed for ANSI caller.");
                return 0;
            }
            return ImmGetImeMenuItemsInterProcess(hIMC, dwFlags, dwType, lpImeParentMenu,
                                                  lpImeMenu, dwSize);
        }
    }

    //
    // within process
    //

    if (hIMC == NULL || (lpInputContext = ImmLockIMC(hIMC)) == NULL) {
        RIPMSG2(RIP_WARNING, "ImmGetImeMenuItemsWorker: illegal hIMC(%#lx) in L%d", hIMC, __LINE__);
        return 0;
    }

    dwThreadId = GetInputContextThread(hIMC);
    if (dwThreadId == 0) {
        RIPMSG1(RIP_WARNING, "ImmGetImeMenuItemsWorker: dwThreadId = 0 in L%d", __LINE__);
        goto cleanup;
    }
    if ((pImeDpi = ImmLockImeDpi(GetKeyboardLayout(dwThreadId))) == NULL) {
        RIPMSG1(RIP_WARNING, "ImmGetImeMenuItemWorker: pImeDpi == NULL in L%d.", __LINE__);
        goto cleanup;
    }

#if 0   // NT: we don't keep version info in ImeDpi
    if (pImeDpi->dwWinVersion <= IMEVER_0310) {
        RIPMSG1(RIP_WARNING, "GetImeMenuItems: OldIME does not support this. %lx", hIMC);
        goto cleanup;
    }
#endif

    //
    // if IME does not support IME Menu, do nothing
    //
    if (pImeDpi->pfn.ImeGetImeMenuItems) {
        LPVOID lpNewBuf = NULL;

        TRACE(("ImmGetImeMenuItemsWorker: IME has menu callback.\n"));

        if (bAnsiIme != bAnsiOrigin) {
            //
            // we need A/W translation before calling IME
            //
            if (bAnsiOrigin) {
                // ANSI API and UNICODE IME.
                // A to W conversion needed here
                if (lpImeParentMenu) {
                    // parent menu is specified. need conversion
                    lpImePTemp = (LPVOID)&imiiParentW;
                    ConvertImeMenuItemInfoAtoW((LPIMEMENUITEMINFOA)lpImeParentMenu,
                                               (LPIMEMENUITEMINFOW)lpImePTemp,
                                                CP_ACP, TRUE);  // ANSI app, UNICODE IME: let's use CP_ACP
                }
                if (lpImeMenu) {
                    // allocate memory block for temporary storage
                    DWORD dwNumBuffer = dwSize / sizeof(IMEMENUITEMINFOA);
                    dwSize = dwNumBuffer * sizeof(IMEMENUITEMINFOW);
                    if (dwSize == 0) {
                        RIPMSG0(RIP_WARNING, "ImmGetImeMenuItemsWorker: (AtoW) dwSize is 0.");
                        goto cleanup;
                    }
                    lpImeTemp = lpNewBuf = ImmLocalAlloc(0, dwSize);
                    TRACE(("ImmGetImeMenuItemsWorker: for UNICODE IME memory allocated %d bytes. lpNewBuf=%#x\n", dwSize, lpNewBuf));
                    if (lpNewBuf == NULL) {
                        RIPMSG1(RIP_WARNING, "ImmGetImeMenuItemsWorker: cannot alloc lpNewBuf in L%d", __LINE__);
                        goto cleanup;
                    }
                }
            }
            else {
                // UNICODE API and ANSI IME.
                // W to A conversion needed here
                if (lpImeParentMenu) {
                    // parent menu is speicified. need conversion
                    lpImePTemp = (LPVOID)&imiiParentA;
                    ConvertImeMenuItemInfoWtoA((LPIMEMENUITEMINFOW)lpImeParentMenu,
                                               (LPIMEMENUITEMINFOA)lpImePTemp,
                                                pImeDpi->dwCodePage);   // Note: hopefully in the future, this can be changed to IMECodePage(pImeDpi)
                }
                if (lpImeMenu) {
                    // allocate memory block for temporary storage
                    DWORD dwNumBuffer = dwSize / sizeof(IMEMENUITEMINFOW);
                    dwSize = dwNumBuffer / sizeof(IMEMENUITEMINFOA);
                    if (dwSize == 0) {
                        RIPMSG0(RIP_WARNING, "ImmGetImeMenuItemsWorker: (WtoA) dwSize is 0.");
                        goto cleanup;
                    }
                    lpImeTemp = lpNewBuf = ImmLocalAlloc(0, dwSize);
                    RIPMSG2(RIP_WARNING, "ImmGetImeMenuItemsWorker: for ANSI IME memory allocated %d bytes. lpNewBuf=%#x", dwSize, lpNewBuf);
                    if (lpNewBuf == NULL) {
                        RIPMSG1(RIP_WARNING, "ImmGetImeMenuItemsWorker: cannot alloc lpNewBuf in L%d", __LINE__);
                        goto cleanup;
                    }
                }
            }
        }

        ////////////////////////////////////////
        dwRet = pImeDpi->pfn.ImeGetImeMenuItems(hIMC, dwFlags, dwType, lpImePTemp, lpImeTemp, dwSize);
        ////////////////////////////////////////

        //
        // back-conversion needed if:
        // 1) IME returns menus, and
        // 2) A/W is different between caller and IME, and
        // 3) caller wants the buffer to be filled
        //
        if (dwRet && bAnsiIme != bAnsiOrigin && lpImeTemp) {
            if (bAnsiOrigin) {
                // ANSI API and UNICODE IME.
                // W to A conversion needed here
                LPIMEMENUITEMINFOW lpW = (LPIMEMENUITEMINFOW)lpImeTemp;
                LPIMEMENUITEMINFOA lpA = (LPIMEMENUITEMINFOA)lpImeMenu;
                DWORD i;

                for (i = 0; i < dwRet; ++i) {
                    ConvertImeMenuItemInfoWtoA((LPIMEMENUITEMINFOW)lpW++,
                                               (LPIMEMENUITEMINFOA)lpA++,
                                                CP_ACP);    // ANSI app and UNICODE IME: let's use CP_ACP
                }
            }
            else {
                // UNICODE API and ANSI IME.
                // A to W conversion needed here
                LPIMEMENUITEMINFOA lpA = (LPIMEMENUITEMINFOA)lpImeTemp;
                LPIMEMENUITEMINFOW lpW = (LPIMEMENUITEMINFOW)lpImeMenu;
                DWORD i;

                for (i = 0; i < dwSize; i++) {
                    ConvertImeMenuItemInfoAtoW((LPIMEMENUITEMINFOA)lpA++,
                                               (LPIMEMENUITEMINFOW)lpW++,
                                               pImeDpi->dwCodePage,     // Note: hopefully in the future, this can be changed to IMECodePage(pImeDpi)
                                               TRUE); // copy hbitmap also
                }
            }
        }

        // free temporary buffer if we've allocated it
        if (lpNewBuf)
            ImmLocalFree(lpNewBuf);
    }   // end if IME has menu callback

cleanup:
    if (pImeDpi) {
        ImmUnlockImeDpi(pImeDpi);
    }

    if (hIMC != NULL) {
        ImmUnlockIMC(hIMC);
    }

    return dwRet;
}


DWORD WINAPI ImmGetImeMenuItemsA(
    HIMC    hIMC,
    DWORD   dwFlags,
    DWORD   dwType,
    LPIMEMENUITEMINFOA lpImeParentMenu,
    LPIMEMENUITEMINFOA lpImeMenu,
    DWORD   dwSize)
{
    return ImmGetImeMenuItemsWorker(hIMC, dwFlags, dwType,
                                    (LPVOID)lpImeParentMenu,
                                    (LPVOID)lpImeMenu, dwSize, TRUE /* ANSI origin */);
}


DWORD WINAPI ImmGetImeMenuItemsW(
    HIMC    hIMC,
    DWORD   dwFlags,
    DWORD   dwType,
    LPIMEMENUITEMINFOW lpImeParentMenu,
    LPIMEMENUITEMINFOW lpImeMenu,
    DWORD   dwSize)
{
    return ImmGetImeMenuItemsWorker(hIMC, dwFlags, dwType,
                                    (LPVOID)lpImeParentMenu,
                                    (LPVOID)lpImeMenu, dwSize, FALSE /* UNICODE origin */);
}
