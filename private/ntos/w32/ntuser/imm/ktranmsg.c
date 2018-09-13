/**************************************************************************\
* Module Name: ktranmsg.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains all the code for the Korean translation subroutine.
*
* History:
* 15-Jul-1995
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

typedef struct tagMYIMESTRUCT {
    // This is the same as IMESTRUCT
    UINT        fnc;                // function code
    WPARAM      wParam;             // word parameter
    UINT        wCount;             // word counter
    UINT        dchSource;          // offset to pbKeyState
    UINT        dchDest;            // offset to pdwTransBuf
    LPARAM      lParam1;
    LPARAM      lParam2;
    LPARAM      lParam3;
    // My additional buffer
    BYTE        pbKeyState[256];
    DWORD       pdwTransBuf[257];
} MYIMESTRUCT;

typedef MYIMESTRUCT *LPMYIMESTRUCT;

MYIMESTRUCT myIME = { 0, 0, 0, sizeof(IMESTRUCT), sizeof(IMESTRUCT) + 256, 0};

#ifdef KILL_THIS
LRESULT CALLBACK KBHookProc(int iCode, WPARAM wParam, LPARAM lParam)
{
    HKL                 hKL;
    HWND                hWnd;
    HIMC                hIMC = NULL;
    LPINPUTCONTEXT      lpIMC = NULL;
    LPCOMPOSITIONSTRING lpCompStr;
    HTASK               hTask;
    int                 iLoop;

    if (!IsWindow(hWndSub))
        WinExec("wnlssub.exe",SW_HIDE);

    hWnd = GetFocus();
    hKL = GetKeyboardLayout(0);
    if ((hKL & 0xF000FFFFL) != 0xE0000412L || iCode < 0 || iCode == HC_NOREMOVE
        || (HIWORD(lParam) & KF_MENUMODE))
        goto CNH;
    hIMC = ImmGetContext(hWnd);
    if (hIMC == NULL || (lpIMC = ImmLockIMC(hIMC)) == NULL || !lpIMC->fOpen)
        goto CNH;

    if (wParam != VK_MENU && wParam != VK_F10)
        goto DoNext;

    // Menu is press with interim character
    if (HIWORD(lParam) & KF_UP)
        goto CNH;
    SendMsg:
    GetKeyboardState((LPBYTE)myIME.pbKeyState);
    myIME.wParam = wParam;
    myIME.lParam1 = lParam;
    myIME.pdwTransBuf[0] = 255/3;
    if (ImmEscape(hKL, hIMC, IME_ESC_AUTOMATA, (LPIMESTRUCT)&myIME) && myIME.wCount) {
        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        StartStrSvr(lpIMC, lpCompStr, myIME.wCount, myIME.pdwTransBuf, TRUE);
        ImmUnlockIMCC(lpIMC->hCompStr, lpCompStr);
    }
    if (wParam == VK_PROCESSKEY) {
        ImmUnlockIMC(hIMC, lpIMC);
        return 0;
    }
    goto CNH;

    DoNext:
    if ((HIWORD(lParam) & KF_ALTDOWN) && wParam != VK_JUNJA)
        goto CNH;

    if (lpIMC->fOpen != FALSE)
        goto DoHook;

    if ((wParam != VK_HANGEUL && wParam != VK_JUNJA) || (HIWORD(lParam) & KF_UP))
        goto CNH;

    DoHook:
    for (iLoop = 0; iLoop < iIndexOfLevel; iLoop++)
        if (stSaveLevel[iLoop].hLevel == hWnd && stSaveLevel[iLoop].uLevel == 3)
            break;

    if (iLoop >= iIndexOfLevel)
        goto CNH;

    if (wParam == VK_PROCESSKEY)
        goto SendMsg;

    GetKeyboardState((LPBYTE)myIME.pbKeyState);
    myIME.wParam = wParam;
    myIME.lParam1 = lParam;
    myIME.pdwTransBuf[0] = 255/3;
    if (ImmEscape(hKL, hIMC, IME_ESC_AUTOMATA, (LPIMESTRUCT)&myIME) && myIME.wCount) {
        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        StartStrSvr(lpIMC, lpCompStr, myIME.wCount, myIME.pdwTransBuf, FALSE);
        ImmUnlockIMCC(lpIMC->hCompStr, lpCompStr);
        ImmUnlockIMC(hIMC, lpIMC);
    }
    return 1;
    CNH:
    if (lpIMC)
        ImmUnlockIMC(hIMC, lpIMC);

    hTask = (hWnd)? GetWindowTask(hWnd): GetCurrentTask();
    for (iLoop = 0; iLoop < iIndexOfLevel; iLoop++)
        if (stSaveLevel[iLoop].hTask == hTask && IsTask(hTask))
            return CallNextHookEx(stSaveLevel[iLoop].hHook, iCode, wParam, lParam);
    return 0;
}
#endif

/**********************************************************************/
/* WINNLSTranslateMessageK()                                          */
/* translate messages for 3.1 apps
/* Return Value:                                                      */
/*      number of translated message                                  */
/**********************************************************************/
UINT WINNLSTranslateMessageK(int                 iNumMsg,
                             PTRANSMSG           pTransMsg,
                             LPINPUTCONTEXT      lpIMC,
                             LPCOMPOSITIONSTRING lpCompStr,
                             BOOL bAnsiIMC)
{
    HWND    hDefIMEWnd;
    int     i, j;
    static  BYTE bp1stInterim = 0;
    static  BYTE bp2ndInterim = 0;
    BOOL    bAnsiWnd;
    HWND    hWnd;
    WCHAR   wchUni;
    CHAR    chAnsi[2];
    BYTE    bchLow, bchHi, bCh;
    BOOL (WINAPI* fpPostMessage)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    LRESULT (WINAPI* fpSendMessage)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

    hWnd = (HWND)lpIMC->hWnd;

    hDefIMEWnd = ImmGetDefaultIMEWnd(hWnd);

    bAnsiWnd = !IsWindowUnicode(hWnd) ? TRUE : FALSE;
    if (bAnsiWnd) {
        fpPostMessage = PostMessageA;
        fpSendMessage = SendMessageA;
    } else {
        fpPostMessage = PostMessageW;
        fpSendMessage = SendMessageW;
    }

    for (i = 0; i < iNumMsg; i++) {

        switch (pTransMsg[i].message) {

        case WM_IME_COMPOSITION :

            if (pTransMsg[i].lParam & GCS_RESULTSTR) {

                fpPostMessage(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0L);

                for (j = 0; j < (int)lpCompStr->dwResultStrLen; j++) {
                    LPARAM  lParam;
                    bCh = 0;
                    if (bAnsiIMC) {
                        bCh = *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + j);
                        if (bAnsiWnd) {
                            if (IsDBCSLeadByte(bCh)) {
                                lParam = (bCh >= 0xB0 && bCh <= 0xC8)? 0xFFF10001L: 0xFFF20001L;
                                PostMessageA(hWnd, WM_CHAR, bCh, lParam);
                                bCh = *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + ++j);
                            } else
                                lParam = 1L;
                            PostMessageA(hWnd, WM_CHAR, bCh, lParam);
                        } else {
                            chAnsi[0] = bCh;
                            chAnsi[1] = *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + ++j);

                            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, chAnsi, 2, &wchUni, 1);

                            PostMessageW(hWnd, WM_CHAR, wchUni, lParam);
                        }
                    } else {    // !bAnsiIMC
                        bCh = *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + j * sizeof(WCHAR));
                        wchUni = bCh | ( *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset +
                                           (j * sizeof(WCHAR) + 1)) << 8);

                        if (bAnsiWnd) {
                            WideCharToMultiByte(CP_ACP, 0, &wchUni, 1, chAnsi, 2, NULL, NULL);

                            bchLow = chAnsi[0];
                            bchHi  = chAnsi[0]; //(BYTE)chAnsi;

                            if (IsDBCSLeadByte(bchLow)) {
                                lParam = (bchLow >= 0xB0 && bchLow <= 0xC8) ? 0xFFF10001L: 0xFFF20001L;
                                PostMessageA(hWnd, WM_CHAR, bchLow, lParam);
                                bchHi = chAnsi[1];
                            } else
                                lParam = 1L;

                            PostMessageA(hWnd, WM_CHAR, bchHi, lParam);
                        } else {
                            PostMessageW(hWnd, WM_CHAR, wchUni, lParam);
                        }
                    }
                }

                fpPostMessage(hWnd, WM_IME_REPORT, IR_STRINGEND, 0L);

            } else {    // !(pTransMsg[i].lParam & GCS_RESULTSTR)

                if (pTransMsg[i].wParam) {

                    fpPostMessage(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0L);

                    bp1stInterim = HIBYTE(LOWORD(pTransMsg[i].wParam));
                    bp2ndInterim = LOBYTE(LOWORD(pTransMsg[i].wParam));

                    if (bAnsiIMC) {
                        if (bAnsiWnd) {
                            PostMessageA(hWnd, WM_INTERIM, bp1stInterim, 0x00F00001L);
                            PostMessageA(hWnd, WM_INTERIM, bp2ndInterim, 0x00F00001L);
                            PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGEND, 0L);
                        } else {
                            chAnsi[0] = bp1stInterim;
                            chAnsi[1] = bp2ndInterim;

                            if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, chAnsi, 2, &wchUni, 1))
                                PostMessageW(hWnd, WM_INTERIM, wchUni, 0x00F00001L);
                            PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGEND, 0L);
                        }

                    } else {
                        if (bAnsiWnd) {
                            wchUni = (bp1stInterim << 8) | bp2ndInterim;  //(WORD)lpdwTransKey[i*3 + 1];
                            WideCharToMultiByte(CP_ACP, 0, &wchUni, 1, chAnsi, 2, NULL, NULL);

                            bchLow = chAnsi[0];
                            bchHi  = chAnsi[1];

                            PostMessageA(hWnd, WM_INTERIM, bchLow, 0x00F00001L);
                            PostMessageA(hWnd, WM_INTERIM, bchHi,  0x00F00001L);
                            PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGEND, 0L);
                        } else {
                            PostMessageW(hWnd, WM_INTERIM, pTransMsg[i].wParam, 0x00F00001L);
                            PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGEND, 0L);
                        }
                    }
                    fpSendMessage(hDefIMEWnd, WM_IME_ENDCOMPOSITION, 0, 0L);

                } else {    // !pTransMsg[i].wParam

                    fpPostMessage(hWnd, WM_IME_REPORT, IR_STRINGSTART, 0L);

                    if (bAnsiIMC) {
                        if (bAnsiWnd) {
                            PostMessageA(hWnd, WM_CHAR, bp1stInterim, 0xFFF10001L);
                            PostMessageA(hWnd, WM_CHAR, bp2ndInterim, 0xFFF10001L);
                            PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGEND, 0L);
                            PostMessageA(hWnd, WM_KEYDOWN, VK_BACK, 0x000E0001L);
                        } else {
                            chAnsi[0] = bp1stInterim;
                            chAnsi[1] = bp2ndInterim;

                            if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, chAnsi, 2, &wchUni, 1))
                                PostMessageW(hWnd, WM_CHAR, wchUni, 0xFFF10001L);

                            PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGEND, 0L);
                            PostMessageW(hWnd, WM_KEYDOWN, VK_BACK, 0x000E0001L);
                        }
                    } else {    // !bAnsiIMC
                        if (bAnsiWnd) {
                            wchUni = (bp1stInterim << 8 ) | bp2ndInterim;

                            WideCharToMultiByte(CP_ACP, 0, &wchUni, 1, chAnsi, 2, NULL, NULL);

                            bchLow = chAnsi[0];
                            bchHi  = chAnsi[1];

                            PostMessageA(hWnd, WM_CHAR, bchLow, 0xFFF10001L);
                            PostMessageA(hWnd, WM_CHAR, bchHi,  0xFFF10001L);
                            PostMessageA(hWnd, WM_IME_REPORT, IR_STRINGEND, 0L);
                            PostMessageA(hWnd, WM_KEYDOWN, VK_BACK, 0x000E0001L);
                        } else {
                            wchUni = bp1stInterim | (bp2ndInterim << 8);

                            PostMessageW(hWnd, WM_CHAR, wchUni, 0xFFF10001L);
                            PostMessageW(hWnd, WM_IME_REPORT, IR_STRINGEND, 0L);
                            PostMessageW(hWnd, WM_KEYDOWN, VK_BACK, 0x000E0001L);
                        }
                    }
                }
            }
            break;

        case WM_IME_STARTCOMPOSITION :
        case WM_IME_ENDCOMPOSITION :
            break;

        case WM_IME_KEYDOWN:
            fpPostMessage(hWnd, WM_KEYDOWN, LOWORD(pTransMsg[i].wParam),
                          pTransMsg[i].lParam);
            break;

        case WM_IME_KEYUP:
            fpPostMessage(hWnd, WM_KEYUP, LOWORD(pTransMsg[i].wParam),
                          pTransMsg[i].lParam);
            break;

        default :
            fpSendMessage(hDefIMEWnd, pTransMsg[i].message,
                          pTransMsg[i].wParam, pTransMsg[i].lParam);
            break;
        }
    }

    return 0;   // indicates all messages are post/sent within this function.
}

