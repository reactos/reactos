//
// Copyright (c) 1997-1999 Microsoft Corporation.
//
#include    <windows.h>
#include    <imm.h>
#include    "resource.h"
#include    "imeblink.h"

#define     UNICODE_CP      1200

/************************************************************/
/*  MatchImeName()                                          */
/************************************************************/
HKL MatchImeName(
    LPCTSTR szStr)
{
    TCHAR     szImeName[16];
    int       nLayout;
    HKL       hKL;
    HGLOBAL   hMem;
    HKL FAR * lpMem;
    int       i;

    nLayout = GetKeyboardLayoutList(0, NULL);

    // alloc temp buffer
    hMem = GlobalAlloc(GHND, sizeof(HKL) * nLayout);

    if (!hMem) {
        return (NULL);
    }

    lpMem = (HKL FAR *)GlobalLock(hMem);

    if (!lpMem) {
        GlobalFree(hMem);
        return (NULL);
    }

    // get all keyboard layouts, it includes all IMEs
    GetKeyboardLayoutList(nLayout, lpMem);

    for (i = 0; i < nLayout; i++) {
        LRESULT lRet;

        hKL = *(lpMem + i);

        lRet = ImmEscape(hKL, (HIMC)NULL, IME_ESC_IME_NAME, szImeName);

        if (!lRet) {                // this hKL can not ask name
            continue;
        }

        if (lstrcmp(szStr, szImeName) == 0) {
            goto MatchOvr;
        }
    }

    hKL = NULL;

MatchOvr:
    GlobalUnlock(hMem);
    GlobalFree(hMem);

    return (hKL);
}

/************************************************************/
/*  RegisterTable()                                         */
/************************************************************/
HKL RegisterTable(
    HWND          hWnd,
    LPUSRDICIMHDR lpIsvUsrDic,
    DWORD         dwFileSize,
    UINT          uCodePage)
{
    HKL    hKL;
   // HDC    hDC;
   // SIZE   lTextSize;
   // RECT   rcProcess;
    DWORD  i;
    LPBYTE lpCurr, lpEnd;
    BOOL   fRet;
    TCHAR  szStr[16];
   // TCHAR  szProcessFmt[32];
   // TCHAR  szResult[2][32];
   // TCHAR  szProcessInfo[48];
    WORD   wInternalCode[256];
    WORD   wAltInternalCode[256];

#ifdef UNICODE
    if (uCodePage == UNICODE_CP) {
        LPUNATSTR lpszMethodName;

        lpszMethodName = (LPUNATSTR)lpIsvUsrDic->achMethodName;

        for (i = 0; i < sizeof(lpIsvUsrDic->achMethodName) / sizeof(TCHAR); i++) {
            szStr[i] = *lpszMethodName++;
        }

        szStr[i] = '\0';
    } else {
        UINT uLen;

        uLen = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED,
            (LPCSTR)lpIsvUsrDic->achMethodName,
            sizeof(lpIsvUsrDic->achMethodName),
            szStr,
            sizeof(szStr) / sizeof(TCHAR));
        if (uLen == 0)
        {
            uCodePage = CP_ACP;
            uLen = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED,
                (LPCSTR)lpIsvUsrDic->achMethodName,
                sizeof(lpIsvUsrDic->achMethodName),
                szStr,
                sizeof(szStr) / sizeof(TCHAR));
        }

        szStr[uLen] = '\0';
    }
#else
    for (i = 0; i < sizeof(lpIsvUsrDic->achMethodName); i++) {
        szStr[i] = lpIsvUsrDic->achMethodName[i];
    }

    szStr[i] = '\0';
#endif

    hKL = MatchImeName(szStr);

    if (!hKL) {
        return (hKL);
    }

    // convert sequence code to internal code
    for (i = 0; i < sizeof(wInternalCode) / sizeof(WORD); i++) {
        LRESULT lRet;

        lRet = ImmEscape(hKL, (HIMC)NULL,
            IME_ESC_SEQUENCE_TO_INTERNAL, &i);

        if (HIWORD(lRet) == 0xFFFF) {
            // This is caused by sign extent in Win9x in the return value of
            // ImmEscape, it causes an invalid internal code.
            wAltInternalCode[i] = 0;
        } else {
            wAltInternalCode[i] = HIWORD(lRet);
        }

        wInternalCode[i] = LOWORD(lRet);

#ifndef UNICODE
        if (wAltInternalCode[i] > 0xFF) {
            // convert to multi byte string
            wAltInternalCode[i] = LOBYTE(wAltInternalCode[i]) << 8 |
                HIBYTE(wAltInternalCode[i]);
        }

        if (wInternalCode[i] > 0xFF) {
            // convert to multi byte string
            wInternalCode[i] = LOBYTE(wInternalCode[i]) << 8 |
                HIBYTE(wInternalCode[i]);
        }
#endif
    }

    // check for each record and register it
    // get to the first record and skip the Bank ID
    lpCurr = (LPBYTE)(lpIsvUsrDic + 1) + sizeof(WORD);
    lpEnd = (LPBYTE)lpIsvUsrDic + dwFileSize;

    for (; lpCurr < lpEnd;
        // internal code + sequence code + Bank ID of next record
        lpCurr += sizeof(WORD) + lpIsvUsrDic->cMethodKeySize + sizeof(WORD)) {

        int j;

        // quick way to init \0 for the register string
        *(LPDWORD)szStr = 0;

#ifdef UNICODE
        if (uCodePage == UNICODE_CP) {
            szStr[0] = *(LPUNATSTR)lpCurr;
        } else {
            CHAR szMultiByte[4];

            szMultiByte[0] = HIBYTE(*(LPTSTR)lpCurr);
            szMultiByte[1] = LOBYTE(*(LPTSTR)lpCurr);

            MultiByteToWideChar(uCodePage, MB_PRECOMPOSED,
                szMultiByte, 2, szStr, 2);
        }
#else
        szStr[1] = *lpCurr;
        szStr[0] = *(lpCurr + 1);
#endif

        for (i = 0, j = 0; i < lpIsvUsrDic->cMethodKeySize; i++) {
            if (!wAltInternalCode[*(LPBYTE)(lpCurr + sizeof(WORD) + i)]) {
            } else if (wAltInternalCode[*(LPBYTE)(lpCurr + sizeof(WORD) + i)] < 0xFF) {
                *(LPTSTR)&szStr[4 + j] = (TCHAR)
                    wAltInternalCode[*(LPBYTE)(lpCurr + sizeof(WORD) + i)];
                j += sizeof(TCHAR) / sizeof(TCHAR);
            } else {
                *(LPWSTR)&szStr[4 + j] = (WCHAR)
                    wAltInternalCode[*(LPBYTE)(lpCurr + sizeof(WORD) + i)];
                j += sizeof(WCHAR) / sizeof(TCHAR);
            }

            if (wInternalCode[*(LPBYTE)(lpCurr + sizeof(WORD) + i)] < 0xFF) {
                *(LPTSTR)&szStr[4 + j] = (TCHAR)
                    wInternalCode[*(LPBYTE)(lpCurr + sizeof(WORD) + i)];
                j += sizeof(TCHAR) / sizeof(TCHAR);
            } else {
                *(LPWSTR)&szStr[4 + j] = (WCHAR)
                    wInternalCode[*(LPBYTE)(lpCurr + sizeof(WORD) + i)];
                j += sizeof(WCHAR) / sizeof(TCHAR);
            }
        }

        szStr[4 + j] = szStr[4 + j + 1] = szStr[4 + j + 2] = '\0';

        fRet = ImmRegisterWord(hKL, &szStr[4], IME_REGWORD_STYLE_EUDC,
            szStr);
    }

    return (hKL);
}
