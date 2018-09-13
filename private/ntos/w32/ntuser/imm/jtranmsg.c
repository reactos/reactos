/**************************************************************************\
* Module Name: jtranmsg.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains all the code for the Japanese translation subroutine.
*
* History:
* 15-Aug-1995 kazum
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop


UINT
JTransCompositionA(
    LPINPUTCONTEXT lpIMC,
    LPCOMPOSITIONSTRING lpCompStrA,
    PTRANSMSG      pTransMsgSrc,
    PTRANSMSG      pTransMsgDest
    );

UINT
JTransCompositionW(
    LPINPUTCONTEXT lpIMC,
    LPCOMPOSITIONSTRING lpCompStrW,
    PTRANSMSG      pTransMsgSrc,
    PTRANSMSG      pTransMsgDest
    );

DWORD
CompStrWToUndetW(
    DWORD dwGCS,
    LPCOMPOSITIONSTRING lpCompStrW,
    LPUNDETERMINESTRUCT lpUndetW
    );

DWORD
CompStrWToUndetA(
    DWORD dwGCS,
    LPCOMPOSITIONSTRING lpCompStrW,
    LPUNDETERMINESTRUCT lpUndetA
    );

DWORD
CompStrWToStringExW(
    DWORD dwGCS,
    LPCOMPOSITIONSTRING lpCompStrW,
    LPSTRINGEXSTRUCT lpStringExW
    );

DWORD
CompStrWToStringExA(
    DWORD dwGCS,
    LPCOMPOSITIONSTRING lpCompStrW,
    LPSTRINGEXSTRUCT lpStringExA
    );

DWORD
CompStrWToStringW(
    LPCOMPOSITIONSTRING lpCompStrW,
    LPWSTR lpStringW
    );

DWORD
CompStrWToStringA(
    LPCOMPOSITIONSTRING lpCompStrW,
    LPSTR lpStringA
    );

VOID
CompStrWToCharW(
    HWND hWnd,
    LPCOMPOSITIONSTRING lpCompStrW
    );

VOID
CompStrWToCharA(
    HWND hWnd,
    LPCOMPOSITIONSTRING lpCompStrW
    );

DWORD
CompStrAToUndetA(
    DWORD dwGCS,
    LPCOMPOSITIONSTRING lpCompStrA,
    LPUNDETERMINESTRUCT lpUndetA
    );

DWORD
CompStrAToUndetW(
    DWORD dwGCS,
    LPCOMPOSITIONSTRING lpCompStrA,
    LPUNDETERMINESTRUCT lpUndetW
    );

DWORD
CompStrAToStringExA(
    DWORD dwGCS,
    LPCOMPOSITIONSTRING lpCompStrA,
    LPSTRINGEXSTRUCT lpStringExA
    );

DWORD
CompStrAToStringExW(
    DWORD dwGCS,
    LPCOMPOSITIONSTRING lpCompStrA,
    LPSTRINGEXSTRUCT lpStringExW
    );

DWORD
CompStrAToStringA(
    LPCOMPOSITIONSTRING lpCompStrA,
    LPSTR lpStringA
    );

DWORD
CompStrAToStringW(
    LPCOMPOSITIONSTRING lpCompStrA,
    LPWSTR lpStringW
    );

VOID
CompStrAToCharA(
    HWND hWnd,
    LPCOMPOSITIONSTRING lpCompStrA
    );

VOID
CompStrAToCharW(
    HWND hWnd,
    LPCOMPOSITIONSTRING lpCompStrA
    );


UINT
JTransCompositionA(
    LPINPUTCONTEXT lpIMC,
    LPCOMPOSITIONSTRING lpCompStrA,
    PTRANSMSG      pTransMsgSrc,
    PTRANSMSG      pTransMsgDest
    )
{
    UINT i = 0;
    int iNum = 0;
    DWORD dwSize;
    HWND hWnd;
    WORD wTextLen = 0;
    BOOL fDoneUndet = FALSE;
    BOOL fDoneDet = FALSE;
    DWORD dwGCS;
    BOOL bAnsiWnd;

    hWnd = (HWND)lpIMC->hWnd;

    if (!IsWindow(hWnd))
        return 0;

    bAnsiWnd = (! IsWindowUnicode(hWnd)) ? TRUE : FALSE;

    dwGCS = (DWORD)pTransMsgSrc->lParam;

    if (lpIMC->fdw31Compat & F31COMPAT_MCWHIDDEN)
    {
        if (bAnsiWnd) {
            dwSize = CompStrAToUndetA(dwGCS, lpCompStrA, NULL);
            if (dwSize != 0) {
                HGLOBAL hUndetA = NULL;
                LPUNDETERMINESTRUCT lpUndetA;

                if (hUndetA = GlobalAlloc(GHND | GMEM_SHARE, dwSize)) {
                    if (lpUndetA = (LPUNDETERMINESTRUCT)GlobalLock(hUndetA)) {
                        CompStrAToUndetA(dwGCS, lpCompStrA, lpUndetA);
                        fDoneUndet = TRUE;
                        GlobalUnlock(hUndetA);

                        if (SendMessageA(hWnd,WM_IME_REPORT,IR_UNDETERMINE, (LPARAM)hUndetA)) {
                            GlobalFree(hUndetA);
                            return 0;
                        }
                    }
                    GlobalFree(hUndetA);
                }
            }
        }
        else {
            dwSize = CompStrAToUndetW(dwGCS, lpCompStrA, NULL);
            if (dwSize != 0) {
                HGLOBAL hUndetW = NULL;
                LPUNDETERMINESTRUCT lpUndetW;

                if (hUndetW = GlobalAlloc(GHND | GMEM_SHARE, dwSize)) {
                    if (lpUndetW = (LPUNDETERMINESTRUCT)GlobalLock(hUndetW)) {
                        CompStrAToUndetW(dwGCS, lpCompStrA, lpUndetW);
                        fDoneUndet = TRUE;
                        GlobalUnlock(hUndetW);

                        if (SendMessageW(hWnd,WM_IME_REPORT,IR_UNDETERMINE, (LPARAM)hUndetW)) {
                            GlobalFree(hUndetW);
                            return 0;
                        }
                    }
                    GlobalFree(hUndetW);
                }
            }
        }
    }

    //
    // This is generate result string routine.
    // This should be same as WINNLSSendString of WIN3.1.
    //

    if (dwGCS & GCS_RESULTSTR)
    {
        //
        // Can we generate IR_STRINGEX ?
        //

        if (dwGCS & GCS_RESULTREADSTR)
        {
            if (bAnsiWnd) {
                dwSize = CompStrAToStringExA(dwGCS, lpCompStrA, NULL);
                if (dwSize != 0) {
                    HGLOBAL hStringExA = NULL;
                    LPSTRINGEXSTRUCT lpStringExA;

                    if (hStringExA = GlobalAlloc(GHND | GMEM_SHARE, dwSize)) {
                        if (lpStringExA = (LPSTRINGEXSTRUCT)GlobalLock(hStringExA)) {
                            CompStrAToStringExA(dwGCS, lpCompStrA, lpStringExA);
                            GlobalUnlock(hStringExA);

                            if (SendMessageA(hWnd,WM_IME_REPORT,IR_STRINGEX, (LPARAM)hStringExA))
                            {
                                GlobalFree(hStringExA);
                                fDoneDet = TRUE;
                                goto jtc_exit_30;
                            }
                        }
                        GlobalFree(hStringExA);
                    }
                }
            }
            else {
                dwSize = CompStrAToStringExW(dwGCS, lpCompStrA, NULL);
                if (dwSize != 0) {
                    HGLOBAL hStringExW = NULL;
                    LPSTRINGEXSTRUCT lpStringExW;

                    if (hStringExW = GlobalAlloc(GHND | GMEM_SHARE, dwSize)) {
                        if (lpStringExW = (LPSTRINGEXSTRUCT)GlobalLock(hStringExW)) {
                            CompStrAToStringExW(dwGCS, lpCompStrA, lpStringExW);
                            GlobalUnlock(hStringExW);

                            if (SendMessageW(hWnd,WM_IME_REPORT,IR_STRINGEX, (LPARAM)hStringExW))
                            {
                                GlobalFree(hStringExW);
                                fDoneDet = TRUE;
                                goto jtc_exit_30;
                            }
                        }
                        GlobalFree(hStringExW);
                    }
                }
            }
        }

        //
        // generate IR_STRING
        //

        if (bAnsiWnd) {
            dwSize = CompStrAToStringA(lpCompStrA, NULL);
            if (dwSize != 0) {
                HGLOBAL hStringA = NULL;
                LPSTR lpStringA;

                if (hStringA = GlobalAlloc(GHND | GMEM_SHARE, dwSize)) {
                    if (lpStringA = (LPSTR)GlobalLock(hStringA)) {
                        CompStrAToStringA(lpCompStrA, lpStringA);
                        GlobalUnlock(hStringA);

                        if (SendMessageA(hWnd,WM_IME_REPORT,IR_STRING, (LPARAM)hStringA))
                        {
                            GlobalFree(hStringA);
                            fDoneDet = TRUE;
                            goto jtc_exit_30;
                        }
                    }
                    GlobalFree(hStringA);
                }
            }
            else {
                return 0;
            }
        }
        else {
            dwSize = CompStrAToStringW(lpCompStrA, NULL);
            if (dwSize != 0) {
                HGLOBAL hStringW = NULL;
                LPWSTR lpStringW;

                if (hStringW = GlobalAlloc(GHND | GMEM_SHARE, dwSize)) {
                    if (lpStringW = (LPWSTR)GlobalLock(hStringW)) {
                        CompStrAToStringW(lpCompStrA, lpStringW);
                        GlobalUnlock(hStringW);

                        if (SendMessageW(hWnd,WM_IME_REPORT,IR_STRING, (LPARAM)hStringW))
                        {
                            GlobalFree(hStringW);
                            fDoneDet = TRUE;
                            goto jtc_exit_30;
                        }
                    }
                    GlobalFree(hStringW);
                }
            }
            else {
                return 0;
            }
        }

        //
        // generate IR_DBCSCHAR/IR_STRINGSTART/WM_CHAR/IR_STRINGEND
        //

        if (bAnsiWnd) {
            CompStrAToCharA(hWnd, lpCompStrA);
        }
        else {
            CompStrAToCharW(hWnd, lpCompStrA);
        }

        fDoneDet = TRUE;
    }


    if (!fDoneUndet && !fDoneDet)
    {
        *pTransMsgDest = *pTransMsgSrc;
        iNum++;
    }

jtc_exit_30:

    if (!fDoneUndet && fDoneDet && (dwGCS & GCS_COMPSTR))
    {
        pTransMsgDest->message = pTransMsgSrc->message;
        pTransMsgDest->wParam  = pTransMsgSrc->wParam;
        pTransMsgDest->lParam  = (LPARAM)(dwGCS & ~(GCS_RESULT | GCS_RESULTREAD));
        iNum++;
    }

    return iNum;
}


UINT
JTransCompositionW(
    LPINPUTCONTEXT lpIMC,
    LPCOMPOSITIONSTRING lpCompStrW,
    PTRANSMSG      pTransMsgSrc,
    PTRANSMSG      pTransMsgDest
    )
{
    UINT i = 0;
    int iNum = 0;
    DWORD dwSize;
    HWND hWnd;
    WORD wTextLen = 0;
    BOOL fDoneUndet = FALSE;
    BOOL fDoneDet = FALSE;
    DWORD dwGCS;
    BOOL bAnsiWnd;

    hWnd = (HWND)lpIMC->hWnd;

    if (!IsWindow(hWnd))
        return 0;

    bAnsiWnd = (! IsWindowUnicode(hWnd)) ? TRUE : FALSE;

    dwGCS = (DWORD)pTransMsgSrc->lParam;

    if (lpIMC->fdw31Compat & F31COMPAT_MCWHIDDEN)
    {
        if (bAnsiWnd) {
            dwSize = CompStrWToUndetA(dwGCS, lpCompStrW, NULL);
            if (dwSize != 0) {
                HGLOBAL hUndetA = NULL;
                LPUNDETERMINESTRUCT lpUndetA;

                if (hUndetA = GlobalAlloc(GHND | GMEM_SHARE, dwSize)) {
                    if (lpUndetA = (LPUNDETERMINESTRUCT)GlobalLock(hUndetA)) {
                        CompStrWToUndetA(dwGCS, lpCompStrW, lpUndetA);
                        fDoneUndet = TRUE;
                        GlobalUnlock(hUndetA);

                        if (SendMessageA(hWnd,WM_IME_REPORT,IR_UNDETERMINE, (LPARAM)hUndetA)) {
                            GlobalFree(hUndetA);
                            return 0;
                        }
                    }
                    GlobalFree(hUndetA);
                }
            }
        }
        else {
            dwSize = CompStrWToUndetW(dwGCS, lpCompStrW, NULL);
            if (dwSize != 0) {
                HGLOBAL hUndetW = NULL;
                LPUNDETERMINESTRUCT lpUndetW;

                if (hUndetW = GlobalAlloc(GHND | GMEM_SHARE, dwSize)) {
                    if (lpUndetW = (LPUNDETERMINESTRUCT)GlobalLock(hUndetW)) {
                        CompStrWToUndetW(dwGCS, lpCompStrW, lpUndetW);
                        fDoneUndet = TRUE;
                        GlobalUnlock(hUndetW);

                        if (SendMessageW(hWnd,WM_IME_REPORT,IR_UNDETERMINE, (LPARAM)hUndetW)) {
                            GlobalFree(hUndetW);
                            return 0;
                        }
                    }
                    GlobalFree(hUndetW);
                }
            }
        }
    }

    //
    // This is generate result string routine.
    // This should be same as WINNLSSendString of WIN3.1.
    //

    if (dwGCS & GCS_RESULTSTR)
    {
        //
        // Can we generate IR_STRINGEX ?
        //

        if (dwGCS & GCS_RESULTREADSTR)
        {
            if (bAnsiWnd) {
                dwSize = CompStrWToStringExA(dwGCS, lpCompStrW, NULL);
                if (dwSize != 0) {
                    HGLOBAL hStringExA = NULL;
                    LPSTRINGEXSTRUCT lpStringExA;

                    if (hStringExA = GlobalAlloc(GHND | GMEM_SHARE, dwSize)) {
                        if (lpStringExA = (LPSTRINGEXSTRUCT)GlobalLock(hStringExA)) {
                            CompStrWToStringExA(dwGCS, lpCompStrW, lpStringExA);
                            GlobalUnlock(hStringExA);

                            if (SendMessageA(hWnd,WM_IME_REPORT,IR_STRINGEX, (LPARAM)hStringExA))
                            {
                                GlobalFree(hStringExA);
                                fDoneDet = TRUE;
                                goto jtc_exit_30;
                            }
                        }
                        GlobalFree(hStringExA);
                    }
                }
            }
            else {
                dwSize = CompStrWToStringExW(dwGCS, lpCompStrW, NULL);
                if (dwSize != 0) {
                    HGLOBAL hStringExW = NULL;
                    LPSTRINGEXSTRUCT lpStringExW;

                    if (hStringExW = GlobalAlloc(GHND | GMEM_SHARE, dwSize)) {
                        if (lpStringExW = (LPSTRINGEXSTRUCT)GlobalLock(hStringExW)) {
                            CompStrWToStringExW(dwGCS, lpCompStrW, lpStringExW);
                            GlobalUnlock(hStringExW);

                            if (SendMessageW(hWnd,WM_IME_REPORT,IR_STRINGEX, (LPARAM)hStringExW))
                            {
                                GlobalFree(hStringExW);
                                fDoneDet = TRUE;
                                goto jtc_exit_30;
                            }
                        }
                        GlobalFree(hStringExW);
                    }
                }
            }
        }

        //
        // generate IR_STRING
        //

        if (bAnsiWnd) {
            dwSize = CompStrWToStringA(lpCompStrW, NULL);
            if (dwSize != 0) {
                HGLOBAL hStringA = NULL;
                LPSTR lpStringA;

                if (hStringA = GlobalAlloc(GHND | GMEM_SHARE, dwSize)) {
                    if (lpStringA = (LPSTR)GlobalLock(hStringA)) {
                        CompStrWToStringA(lpCompStrW, lpStringA);
                        GlobalUnlock(hStringA);

                        if (SendMessageA(hWnd,WM_IME_REPORT,IR_STRING, (LPARAM)hStringA))
                        {
                            GlobalFree(hStringA);
                            fDoneDet = TRUE;
                            goto jtc_exit_30;
                        }
                    }
                    GlobalFree(hStringA);
                }
            }
            else {
                return 0;
            }
        }
        else {
            dwSize = CompStrWToStringW(lpCompStrW, NULL);
            if (dwSize != 0) {
                HGLOBAL hStringW = NULL;
                LPWSTR lpStringW;

                if (hStringW = GlobalAlloc(GHND | GMEM_SHARE, dwSize)) {
                    if (lpStringW = (LPWSTR)GlobalLock(hStringW)) {
                        CompStrWToStringW(lpCompStrW, lpStringW);
                        GlobalUnlock(hStringW);

                        if (SendMessageW(hWnd,WM_IME_REPORT,IR_STRING, (LPARAM)hStringW))
                        {
                            GlobalFree(hStringW);
                            fDoneDet = TRUE;
                            goto jtc_exit_30;
                        }
                    }
                    GlobalFree(hStringW);
                }
            }
            else {
                return 0;
            }
        }

        //
        // generate IR_DBCSCHAR/IR_STRINGSTART/WM_CHAR/IR_STRINGEND
        //

        if (bAnsiWnd) {
            CompStrWToCharA(hWnd, lpCompStrW);
        }
        else {
            CompStrWToCharW(hWnd, lpCompStrW);
        }

        fDoneDet = TRUE;
    }


    if (!fDoneUndet && !fDoneDet)
    {
        *pTransMsgDest = *pTransMsgSrc;
        iNum++;
    }

jtc_exit_30:

    if (!fDoneUndet && fDoneDet && (dwGCS & GCS_COMPSTR))
    {
        pTransMsgDest->message = pTransMsgSrc->message;
        pTransMsgDest->wParam  = pTransMsgSrc->wParam;
        pTransMsgDest->lParam  = (LPARAM)(dwGCS & ~(GCS_RESULT | GCS_RESULTREAD));
        iNum++;
    }

    return iNum;
}



UINT
WINNLSTranslateMessageJ(
    UINT                uiNumMsg,
    PTRANSMSG           pTransMsg,
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    BOOL bAnsiIMC
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PTRANSMSG       pTransMsgBuf, pTransMsgTemp;
    UINT            uiNewNum = 0;
    UINT            uiTempNum;
    DWORD           dwTempSize;
    HWND            hDefIMEWnd;
    UINT            i;
    BOOL            bAnsiWnd;

    hDefIMEWnd = ImmGetDefaultIMEWnd((HWND)lpIMC->hWnd);

    bAnsiWnd = (! IsWindowUnicode(lpIMC->hWnd)) ? TRUE : FALSE;

    dwTempSize = uiNumMsg * sizeof(TRANSMSG);

    //
    // Allocate one more TRANSMSG and ZEROINIT the whole thing!
    //

    pTransMsgBuf = (PTRANSMSG)ImmLocalAlloc( HEAP_ZERO_MEMORY,
                                             dwTempSize + sizeof(TRANSMSG)
                                           );
    if ( pTransMsgBuf == NULL )
        goto wtmsg2_j_10;

    RtlCopyMemory(pTransMsgBuf, pTransMsg, dwTempSize);
    pTransMsgTemp = pTransMsgBuf;

    //
    // When MCW_HIDDEN mode, WM_IME_ENDCOMPOSITION will be translated to
    // IR_UNDETERMINE with 0 string. So that, this message have to be
    // generated after all messages.
    //

    if (lpIMC->fdw31Compat & F31COMPAT_MCWHIDDEN) {

        for (i = 0; i < uiNumMsg; i++, pTransMsgTemp++) {
            if (pTransMsgTemp->message == WM_IME_ENDCOMPOSITION) {
                break;
            }
        }

        if (pTransMsgTemp->message == WM_IME_ENDCOMPOSITION) {

            PTRANSMSG pTransMsgSrc = pTransMsgTemp + 1;

            while (pTransMsgSrc->message != 0) {
                *pTransMsgTemp++ = *pTransMsgSrc++;
            }

            pTransMsgTemp->message = WM_IME_ENDCOMPOSITION;
            pTransMsgTemp->wParam  = 0L;
            pTransMsgTemp->lParam  = 0L;

        }

        pTransMsgTemp = pTransMsgBuf;
    }

    while (pTransMsgTemp->message != 0) {

        switch (pTransMsgTemp->message)
        {
            case WM_IME_COMPOSITION:

                if (bAnsiIMC)
                    uiTempNum = JTransCompositionA(lpIMC,lpCompStr,pTransMsgTemp,pTransMsg);
                else
                    uiTempNum = JTransCompositionW(lpIMC,lpCompStr,pTransMsgTemp,pTransMsg);

                uiNewNum  += uiTempNum;
                pTransMsg += uiTempNum;

                if ( !(lpIMC->fdw31Compat & F31COMPAT_MCWHIDDEN) )
                {
                    if ( lpIMC->cfCompForm.dwStyle != CFS_DEFAULT ) {
                        SendMessage((HWND)lpIMC->hWnd, WM_IME_REPORT, IR_CHANGECONVERT, 0L);
                    }
                }

                break;

            case WM_IME_STARTCOMPOSITION:

                if ( !(lpIMC->fdw31Compat & F31COMPAT_MCWHIDDEN) )
                {
                    if ( lpIMC->cfCompForm.dwStyle != CFS_DEFAULT ) {
                        SendMessage((HWND)lpIMC->hWnd, WM_IME_REPORT, IR_OPENCONVERT, 0L);
                    }

                    *pTransMsg++ = *pTransMsgTemp;
                    uiNewNum++;
                }

                break;

            case WM_IME_ENDCOMPOSITION:

                if ( !(lpIMC->fdw31Compat & F31COMPAT_MCWHIDDEN) )
                {
                    if ( lpIMC->cfCompForm.dwStyle != CFS_DEFAULT ) {
                        SendMessage((HWND)lpIMC->hWnd, WM_IME_REPORT, IR_CLOSECONVERT, 0L);
                    }

                    *pTransMsg++ = *pTransMsgTemp;
                    uiNewNum++;

                }
                else {

                    HWND hWnd = (HWND)lpIMC->hWnd;
                    HGLOBAL hUndet = 0L;

                    if (hUndet = GlobalAlloc(GHND | GMEM_SHARE, sizeof(UNDETERMINESTRUCT)))
                    {
                        if (! IsWindowUnicode(lpIMC->hWnd))
                            SendMessageA(hWnd,WM_IME_REPORT, IR_UNDETERMINE, (LPARAM)hUndet);
                        else
                            SendMessageW(hWnd,WM_IME_REPORT, IR_UNDETERMINE, (LPARAM)hUndet);

                        GlobalFree(hUndet);
                    }
                }

                break;

            case WM_IME_NOTIFY:

                switch (pTransMsgTemp->wParam)
                {
                    case IMN_OPENCANDIDATE:

                        //
                        // When 3.1 Application want to set MCW_HIDDEN,
                        // the candidate window of Chicago IME go way
                        // from the restangle of the composition string
                        // that will be drawn by the application.
                        //

                        if (IsWindow((HWND)lpIMC->hWnd) &&
                            (lpIMC->fdw31Compat & F31COMPAT_MCWHIDDEN)) {
                            CANDIDATEFORM cfCandForm;
                            DWORD i;
                            DWORD dwNumCand = 0;

                            for (i = 0; i < 32; i++)
                            {
                                //
                                // Only the opened candidate should be updated.
                                //
                                if (!(pTransMsgTemp->lParam & (01L << i)))
                                    continue;

                                cfCandForm.dwIndex = i;
                                cfCandForm.dwStyle = CFS_EXCLUDE;
                                cfCandForm.ptCurrentPos = lpIMC->cfCompForm.ptCurrentPos;
                                cfCandForm.rcArea       = lpIMC->cfCompForm.rcArea;
                                SendMessage(hDefIMEWnd, WM_IME_CONTROL, IMC_SETCANDIDATEPOS,
                                               (LPARAM)(LPCANDIDATEFORM)&cfCandForm);
                            }
                        }

                        break;

                    default :
                        break;
                }

                if (!(lpIMC->fdw31Compat & F31COMPAT_MCWHIDDEN)) {
                    *pTransMsg++ = *pTransMsgTemp;
                    uiNewNum++;
                }
                else {
                    //
                    // For win31 apps who set MCW_HIDDEN, we won't give them
                    // IMN_OPENCANDIDATE here. Instead, send it directly to the
                    // default IME window.
                    //
                    SendMessage( hDefIMEWnd, 
                                 pTransMsgTemp->message,
                                 pTransMsgTemp->wParam,
                                 pTransMsgTemp->lParam
                               );
                }

                break;

            default :
                *pTransMsg++ = *pTransMsgTemp;
                uiNewNum++;
                break;
        }

        pTransMsgTemp++;
    }

    ImmLocalFree(pTransMsgBuf);

wtmsg2_j_10:
    return (uiNewNum);
}


DWORD
CompStrAToUndetA(
    DWORD dwGCS,
    LPCOMPOSITIONSTRING lpCompStrA,
    LPUNDETERMINESTRUCT lpUndetA
    )

/*++

Routine Description:

    Convert composition string (ANSI) to undetermine string (ANSI).

Arguments:

Return Value:

--*/

{
    DWORD dwPos;
    DWORD dwSize;
    UINT  i;

    dwSize = DWORD_ALIGN((sizeof(UNDETERMINESTRUCT)+1)) +
             DWORD_ALIGN(((lpCompStrA->dwResultStrLen+1) * sizeof(CHAR))) +
             DWORD_ALIGN((lpCompStrA->dwResultClauseLen+1)) +
             DWORD_ALIGN(((lpCompStrA->dwResultReadStrLen+1) * sizeof(CHAR))) +
             DWORD_ALIGN((lpCompStrA->dwResultReadClauseLen+1)) +
             DWORD_ALIGN((lpCompStrA->dwCompAttrLen+1)) +
             DWORD_ALIGN(((lpCompStrA->dwCompStrLen+1) * sizeof(CHAR)));

    if (lpUndetA == NULL) {
        return dwSize;
    }

    dwPos = DWORD_ALIGN((sizeof(UNDETERMINESTRUCT) + 1));
    lpUndetA->dwSize = dwSize;

    if (dwGCS & GCS_COMPSTR)
    {
        lpUndetA->uUndetTextLen = lpCompStrA->dwCompStrLen;
        lpUndetA->uUndetTextPos = dwPos;
        memcpy((PBYTE)lpUndetA   + dwPos,
               (PBYTE)lpCompStrA + lpCompStrA->dwCompStrOffset,
               lpCompStrA->dwCompStrLen * sizeof(CHAR)
              );
        *(LPSTR)((PBYTE)lpUndetA + dwPos + lpCompStrA->dwCompStrLen*sizeof(CHAR)) = '\0';
        dwPos += DWORD_ALIGN(((lpUndetA->uUndetTextLen+1)*sizeof(CHAR)));

        // Sometime Chicago IME does not generate GCS_COMPATTR
        // with GCS_COMPSTR. But uUndetAttrPos should be filled
        // when the UndetText is updated.
        if (lpCompStrA->dwCompAttrLen && !(dwGCS & GCS_COMPATTR))
            dwGCS |= GCS_COMPATTR;
    }

    if (dwGCS & GCS_COMPATTR)
    {
        lpUndetA->uUndetAttrPos = dwPos;
        memcpy((PBYTE)lpUndetA   + dwPos,
               (PBYTE)lpCompStrA + lpCompStrA->dwCompAttrOffset,
               lpCompStrA->dwCompAttrLen
              );
        dwPos += DWORD_ALIGN((lpUndetA->uUndetTextLen + 1));
    }

    if (dwGCS & GCS_CURSORPOS)
    {
        lpUndetA->uCursorPos = lpCompStrA->dwCursorPos;
    }

    if (dwGCS & GCS_DELTASTART)
    {
        lpUndetA->uDeltaStart = lpCompStrA->dwDeltaStart;
    }

    if (dwGCS & GCS_RESULTSTR)
    {
        lpUndetA->uDetermineTextLen = lpCompStrA->dwResultStrLen;
        lpUndetA->uDetermineTextPos = dwPos;
        memcpy((PBYTE)lpUndetA   + dwPos,
               (PBYTE)lpCompStrA + lpCompStrA->dwResultStrOffset,
               lpCompStrA->dwResultStrLen * sizeof(CHAR)
              );
        *(LPSTR)((PBYTE)lpUndetA + dwPos + lpCompStrA->dwResultStrLen*sizeof(CHAR)) = '\0';
        dwPos += DWORD_ALIGN(((lpUndetA->uDetermineTextLen + 1)*sizeof(CHAR)));
    }

    if ( (dwGCS & GCS_RESULTCLAUSE) &&
         (lpCompStrA->dwResultClauseLen > 0) )
    {
        LPDWORD lpw;
        LPDWORD lpdw;

        lpUndetA->uDetermineDelimPos = dwPos;

        lpw  = (LPDWORD)((PBYTE)lpUndetA + dwPos);
        lpdw = (LPDWORD)((PBYTE)lpCompStrA + lpCompStrA->dwResultClauseOffset);
        for (i = 0; i < (lpCompStrA->dwResultClauseLen / sizeof(DWORD)); i++)
            *lpw++ = *lpdw++;

        dwPos += DWORD_ALIGN((lpCompStrA->dwResultClauseLen + 1));
    }

    if (dwGCS & GCS_RESULTREADSTR)
    {
        lpUndetA->uYomiTextLen = lpCompStrA->dwResultReadStrLen;
        lpUndetA->uYomiTextPos = dwPos;
        memcpy((PBYTE)lpUndetA   + dwPos,
               (PBYTE)lpCompStrA + lpCompStrA->dwResultReadStrOffset,
               lpCompStrA->dwResultReadStrLen * sizeof(CHAR)
              );
        *(LPSTR)((PBYTE)lpUndetA + dwPos + lpCompStrA->dwResultReadStrLen*sizeof(CHAR)) = '\0';
        dwPos += DWORD_ALIGN(((lpUndetA->uYomiTextLen + 1)*sizeof(CHAR)));
    }

    if ( (dwGCS & GCS_RESULTREADCLAUSE) &&
         (lpCompStrA->dwResultReadClauseLen > 0) )
    {
        LPDWORD lpw;
        LPDWORD lpdw;

        lpUndetA->uYomiDelimPos = dwPos;

        lpw  = (LPDWORD)((PBYTE)lpUndetA + dwPos);
        lpdw = (LPDWORD)((PBYTE)lpCompStrA + lpCompStrA->dwResultReadClauseOffset);
        for (i = 0; i < (lpCompStrA->dwResultReadClauseLen / sizeof(DWORD)); i++)
            *lpw++ = *lpdw++;

        dwPos += DWORD_ALIGN((lpCompStrA->dwResultReadClauseLen + 1));
    }

    return dwSize;
}

DWORD
CompStrAToUndetW(
    DWORD dwGCS,
    LPCOMPOSITIONSTRING lpCompStrA,
    LPUNDETERMINESTRUCT lpUndetW
    )

/*++

Routine Description:

    Convert composition string (ANSI) to undetermine string (Unicode).

Arguments:

Return Value:

--*/

{
    DWORD dwPos;
    DWORD dwSize;
    UINT  i;

    dwSize = DWORD_ALIGN((sizeof(UNDETERMINESTRUCT)+1)) +
             DWORD_ALIGN(((lpCompStrA->dwResultStrLen+1) * sizeof(WCHAR))) +
             DWORD_ALIGN((lpCompStrA->dwResultClauseLen+1)) +
             DWORD_ALIGN(((lpCompStrA->dwResultReadStrLen+1) * sizeof(WCHAR))) +
             DWORD_ALIGN((lpCompStrA->dwResultReadClauseLen+1)) +
             DWORD_ALIGN((lpCompStrA->dwCompAttrLen * 2)) +
             DWORD_ALIGN(((lpCompStrA->dwCompStrLen+1) * sizeof(WCHAR)));

    if (lpUndetW == NULL) {
        return dwSize;
    }

    dwPos = DWORD_ALIGN((sizeof(UNDETERMINESTRUCT) + 1));
    lpUndetW->dwSize = dwSize;

    if (dwGCS & GCS_COMPSTR)
    {
        i = MultiByteToWideChar( CP_ACP,
                                (DWORD)MB_PRECOMPOSED,
                                (LPSTR)((PBYTE)lpCompStrA + lpCompStrA->dwCompStrOffset),  // src
                                (INT)lpCompStrA->dwCompStrLen,
                                (LPWSTR)((PBYTE)lpUndetW + dwPos),                         // dest
                                (INT)dwSize - dwPos);
        ((LPWSTR)((PBYTE)lpUndetW + dwPos))[i] = L'\0';
        lpUndetW->uUndetTextLen = i;
        lpUndetW->uUndetTextPos = dwPos;
        dwPos += DWORD_ALIGN(((lpUndetW->uUndetTextLen + 1)*sizeof(WCHAR)));

        // Sometime Chicago IME does not generate GCS_COMPATTR
        // with GCS_COMPSTR. But uUndetAttrPos should be filled
        // when the UndetText is updated.
        if (lpCompStrA->dwCompAttrLen && !(dwGCS & GCS_COMPATTR))
            dwGCS |= GCS_COMPATTR;
    }

    if (dwGCS & GCS_COMPATTR)
    {
        if (lpUndetW->uUndetTextLen != 0) {
            LPWSTR lpwszUndetText;
            PBYTE  lpAttrW;
            PBYTE  lpAttrA;
            WCHAR  wc;
            ULONG  MultiByteSize;

            lpwszUndetText = (LPWSTR)((PBYTE)lpUndetW + lpUndetW->uUndetTextPos);
            lpAttrA = (PBYTE)lpCompStrA + lpCompStrA->dwCompAttrOffset;
            lpAttrW = (PBYTE)lpUndetW   + dwPos;

            while (wc=*lpwszUndetText++) {
                RtlUnicodeToMultiByteSize(&MultiByteSize, &wc, sizeof(WCHAR));
                if (MultiByteSize == 2) {
                    *lpAttrW++ = *lpAttrA;
                    lpAttrA += 2;
                }
                else {
                    *lpAttrW++ = *lpAttrA++;
                }
            }

            lpUndetW->uUndetAttrPos = dwPos;
            dwPos += DWORD_ALIGN((lpUndetW->uUndetTextLen + 1));
        }
    }

    if (dwGCS & GCS_CURSORPOS)
    {
        if (lpCompStrA->dwCursorPos != -1) {
            lpUndetW->uCursorPos = CalcCharacterPositionAtoW(lpCompStrA->dwCursorPos,
                                                             (LPSTR)((PBYTE)lpCompStrA + lpCompStrA->dwCompStrOffset),
                                                             CP_ACP
                                                            );
        }
        else {
            lpUndetW->uCursorPos = lpCompStrA->dwCursorPos;
        }
    }

    if (dwGCS & GCS_DELTASTART)
    {
        if (lpCompStrA->dwDeltaStart != -1) {
            lpUndetW->uDeltaStart = CalcCharacterPositionAtoW(lpCompStrA->dwDeltaStart,
                                                              (LPSTR)((PBYTE)lpCompStrA + lpCompStrA->dwCompStrOffset),
                                                              CP_ACP
                                                             );
        }
        else {
            lpUndetW->uDeltaStart = lpCompStrA->dwDeltaStart;
        }
    }

    if (dwGCS & GCS_RESULTSTR)
    {
        i = MultiByteToWideChar( CP_ACP,
                                (DWORD)MB_PRECOMPOSED,
                                (LPSTR)((PBYTE)lpCompStrA + lpCompStrA->dwResultStrOffset),  // src
                                (INT)lpCompStrA->dwResultStrLen,
                                (LPWSTR)((PBYTE)lpUndetW + dwPos),                           // dest
                                (INT)dwSize - dwPos);
        ((LPWSTR)((PBYTE)lpUndetW + dwPos))[i] = L'\0';
        lpUndetW->uDetermineTextLen = i;
        lpUndetW->uDetermineTextPos = dwPos;
        dwPos += DWORD_ALIGN(((lpUndetW->uDetermineTextLen + 1)*sizeof(WCHAR)));
    }

    if ( (dwGCS & GCS_RESULTCLAUSE) &&
         (lpCompStrA->dwResultClauseLen > 0) )
    {
        if (lpUndetW->uDetermineTextLen != 0) {
            LPDWORD lpw;
            LPDWORD lpdw;

            lpw  = (LPDWORD)((PBYTE)lpUndetW + dwPos);
            lpdw = (LPDWORD)((PBYTE)lpCompStrA + lpCompStrA->dwResultClauseOffset);

            for (i = 0; i < (lpCompStrA->dwResultClauseLen / sizeof(DWORD)); i++) {
                *lpw++ = CalcCharacterPositionAtoW(*lpdw++,
                                                   (LPSTR)((PBYTE)lpCompStrA + lpCompStrA->dwResultStrOffset),
                                                   CP_ACP
                                                  );
            }

            lpUndetW->uDetermineDelimPos = dwPos;
            dwPos += DWORD_ALIGN((lpCompStrA->dwResultClauseLen + 1));
        }
    }

    if (dwGCS & GCS_RESULTREADSTR)
    {
        i = MultiByteToWideChar( CP_ACP,
                                (DWORD)MB_PRECOMPOSED,
                                (LPSTR)((PBYTE)lpCompStrA + lpCompStrA->dwResultReadStrOffset),  // src
                                (INT)lpCompStrA->dwResultReadStrLen,
                                (LPWSTR)((PBYTE)lpUndetW + dwPos),                               // dest
                                (INT)dwSize - dwPos);
        ((LPWSTR)((PBYTE)lpUndetW + dwPos))[i] = L'\0';
        lpUndetW->uYomiTextLen = i;
        lpUndetW->uYomiTextPos = dwPos;
        dwPos += DWORD_ALIGN(((lpUndetW->uYomiTextLen + 1)*sizeof(WCHAR)));
    }

    if ( (dwGCS & GCS_RESULTREADCLAUSE) &&
         (lpCompStrA->dwResultReadClauseLen > 0) )
    {
        if (lpUndetW->uYomiTextLen != 0) {
            LPDWORD lpw;
            LPDWORD lpdw;

            lpw  = (LPDWORD)((PBYTE)lpUndetW + dwPos);
            lpdw = (LPDWORD)((PBYTE)lpCompStrA + lpCompStrA->dwResultReadClauseOffset);

            for (i = 0; i < (lpCompStrA->dwResultReadClauseLen / sizeof(DWORD)); i++) {
                *lpw++ = CalcCharacterPositionAtoW(*lpdw++,
                                                   (LPSTR)((PBYTE)lpCompStrA + lpCompStrA->dwResultReadStrOffset),
                                                   CP_ACP
                                                  );
            }

            lpUndetW->uYomiDelimPos = dwPos;
            dwPos += DWORD_ALIGN((lpCompStrA->dwResultReadClauseLen + 1));
        }
    }

    return dwSize;
}


DWORD
CompStrAToStringExA(
    DWORD dwGCS,
    LPCOMPOSITIONSTRING lpCompStrA,
    LPSTRINGEXSTRUCT lpStringExA
    )

/*++

Routine Description:

    Convert composition string (ANSI) to StringEx (ANSI).

Arguments:

Return Value:

--*/

{
    DWORD dwPos;
    DWORD dwSize;
    UINT  i;

    dwSize = DWORD_ALIGN((sizeof(STRINGEXSTRUCT)+1)) +
             DWORD_ALIGN(((lpCompStrA->dwResultStrLen+1) * sizeof(CHAR))) +
             DWORD_ALIGN(lpCompStrA->dwResultClauseLen+1) +
             DWORD_ALIGN(((lpCompStrA->dwResultReadStrLen+1) * sizeof(CHAR))) +
             DWORD_ALIGN(lpCompStrA->dwResultReadClauseLen+1);

    if (lpStringExA == NULL) {
        return dwSize;
    }

    dwPos = DWORD_ALIGN(sizeof(STRINGEXSTRUCT) + 1);
    lpStringExA->dwSize = dwSize;

    lpStringExA->uDeterminePos = dwPos;
    memcpy((PBYTE)lpStringExA + dwPos,
           (PBYTE)lpCompStrA  + lpCompStrA->dwResultStrOffset,
           lpCompStrA->dwResultStrLen * sizeof(CHAR)
          );
    *(LPSTR)((PBYTE)lpStringExA + dwPos + lpCompStrA->dwResultStrLen*sizeof(CHAR)) = '\0';
    dwPos += DWORD_ALIGN(((lpCompStrA->dwResultStrLen + 1)*sizeof(CHAR)));

    if ( (dwGCS & GCS_RESULTCLAUSE) &&
         (lpCompStrA->dwResultClauseLen > 0) )
    {
        LPDWORD lpw;
        LPDWORD lpdw;

        lpStringExA->uDetermineDelimPos = dwPos;

        lpw  = (LPDWORD)((PBYTE)lpStringExA + dwPos);
        lpdw = (LPDWORD)((PBYTE)lpCompStrA + lpCompStrA->dwResultClauseOffset);
        for (i = 0; i < (lpCompStrA->dwResultClauseLen / sizeof(DWORD)); i++)
            *lpw++ = *lpdw++;

        dwPos += DWORD_ALIGN((lpCompStrA->dwResultClauseLen + 1));
    }

    lpStringExA->uYomiPos = dwPos;
    memcpy((PBYTE)lpStringExA + dwPos,
           (PBYTE)lpCompStrA  + lpCompStrA->dwResultReadStrOffset,
           lpCompStrA->dwResultReadStrLen * sizeof(CHAR)
          );
    *(LPSTR)((PBYTE)lpStringExA + dwPos + lpCompStrA->dwResultReadStrLen*sizeof(CHAR)) = '\0';
    dwPos += DWORD_ALIGN(((lpCompStrA->dwResultReadStrLen + 1)*sizeof(CHAR)));

    if ( (dwGCS & GCS_RESULTREADCLAUSE) &&
         (lpCompStrA->dwResultReadClauseLen > 0) )
    {
        LPDWORD lpw;
        LPDWORD lpdw;

        lpStringExA->uYomiDelimPos = dwPos;

        lpw  = (LPDWORD)((PBYTE)lpStringExA + dwPos);
        lpdw = (LPDWORD)((PBYTE)lpCompStrA + lpCompStrA->dwResultReadClauseOffset);
        for (i = 0; i < (lpCompStrA->dwResultReadClauseLen / sizeof(DWORD)); i++)
            *lpw++ = *lpdw++;
    }

    return dwSize;
}

DWORD
CompStrAToStringExW(
    DWORD dwGCS,
    LPCOMPOSITIONSTRING lpCompStrA,
    LPSTRINGEXSTRUCT lpStringExW
    )

/*++

Routine Description:

    Convert composition string (ANSI) to StringEx (Unicode).

Arguments:

Return Value:

--*/

{
    DWORD dwPos;
    DWORD dwSize;
    DWORD dwLen;
    UINT  i;

    dwSize = DWORD_ALIGN((sizeof(STRINGEXSTRUCT)+1)) +
             (lpCompStrA->dwResultStrLen > 0 ? DWORD_ALIGN(((lpCompStrA->dwResultStrLen+1) * sizeof(WCHAR))) : 0) +
             (lpCompStrA->dwResultClauseLen > 0 ? DWORD_ALIGN(lpCompStrA->dwResultClauseLen+1) : 0) +
             (lpCompStrA->dwResultReadStrLen > 0 ? DWORD_ALIGN(((lpCompStrA->dwResultReadStrLen+1) * sizeof(WCHAR))) : 0)+
             (lpCompStrA->dwResultReadClauseLen > 0 ? DWORD_ALIGN(lpCompStrA->dwResultReadClauseLen+1) : 0);

    if (lpStringExW == NULL) {
        return dwSize;
    }

    dwPos = DWORD_ALIGN(sizeof(STRINGEXSTRUCT) + 1);
    lpStringExW->dwSize = dwSize;

    if (lpCompStrA->dwResultStrLen > 0) {
        i = MultiByteToWideChar( CP_ACP,
                                (DWORD)MB_PRECOMPOSED,
                                (LPSTR)((PBYTE)lpCompStrA + lpCompStrA->dwResultStrOffset),  // src
                                (INT)lpCompStrA->dwResultStrLen,
                                (LPWSTR)((PBYTE)lpStringExW + dwPos),                        // dest
                                (INT)dwSize - dwPos);
        ((LPWSTR)((PBYTE)lpStringExW + dwPos))[i] = L'\0';
        dwLen = i;
        lpStringExW->uDeterminePos = dwPos;
        dwPos += DWORD_ALIGN(((dwLen + 1)*sizeof(WCHAR)));
    } else {
        dwLen = 0;
        lpStringExW->uDeterminePos = 0;
    }

    if ( (dwGCS & GCS_RESULTCLAUSE) &&
         (lpCompStrA->dwResultClauseLen > 0) )
    {
        if (dwLen != 0 && lpCompStrA->dwResultClauseLen > 0) {
            LPDWORD lpw;
            LPDWORD lpdw;

            lpw  = (LPDWORD)((PBYTE)lpStringExW + dwPos);
            lpdw = (LPDWORD)((PBYTE)lpCompStrA + lpCompStrA->dwResultClauseOffset);

            for (i = 0; i < (lpCompStrA->dwResultClauseLen / sizeof(DWORD)); i++) {
                *lpw++ = CalcCharacterPositionAtoW(*lpdw++,
                                                   (LPSTR)((PBYTE)lpCompStrA + lpCompStrA->dwResultStrOffset),
                                                   CP_ACP
                                                  );
            }

            lpStringExW->uDetermineDelimPos = dwPos;
            dwPos += DWORD_ALIGN((lpCompStrA->dwResultClauseLen + 1));
        }
    }

    if (lpCompStrA->dwResultReadStrLen > 0) {
        i = MultiByteToWideChar( CP_ACP,
                                (DWORD)MB_PRECOMPOSED,
                                (LPSTR)((PBYTE)lpCompStrA + lpCompStrA->dwResultReadStrOffset),  // src
                                (INT)lpCompStrA->dwResultReadStrLen,
                                (LPWSTR)((PBYTE)lpStringExW + dwPos),                            // dest
                                (INT)dwSize - dwPos);
        ((LPWSTR)((PBYTE)lpStringExW + dwPos))[i] = L'\0';
        dwLen = i;
        lpStringExW->uYomiPos = dwPos;
        dwPos += DWORD_ALIGN(((dwLen + 1)*sizeof(WCHAR)));
    } else {
        dwLen = 0;
        lpStringExW->uYomiPos = 0;
    }

    if ( (dwGCS & GCS_RESULTREADCLAUSE) &&
         (lpCompStrA->dwResultReadClauseLen > 0) )
    {
        if (dwLen != 0 && lpCompStrA->dwResultReadClauseLen > 0) {
            LPDWORD lpw;
            LPDWORD lpdw;

            lpw  = (LPDWORD)((PBYTE)lpStringExW + dwPos);
            lpdw = (LPDWORD)((PBYTE)lpCompStrA + lpCompStrA->dwResultReadClauseOffset);

            for (i = 0; i < (lpCompStrA->dwResultReadClauseLen / sizeof(DWORD)); i++) {
                *lpw++ = CalcCharacterPositionAtoW(*lpdw++,
                                                   (LPSTR)((PBYTE)lpCompStrA + lpCompStrA->dwResultReadStrOffset),
                                                   CP_ACP
                                                  );
            }

            lpStringExW->uYomiDelimPos = dwPos;
            dwPos += DWORD_ALIGN((lpCompStrA->dwResultReadClauseLen + 1));
        }
    }

    return dwSize;
}

DWORD
CompStrAToStringA(
    LPCOMPOSITIONSTRING lpCompStrA,
    LPSTR lpStringA
    )

/*++

Routine Description:

    Convert composition string (ANSI) to String (ANSI).

Arguments:

Return Value:

--*/

{
    LPSTR lpszString;
    DWORD dwSize;

    lpszString = (LPSTR)((PBYTE)lpCompStrA + lpCompStrA->dwResultStrOffset);
    dwSize = lpCompStrA->dwResultStrLen;

    if (lpStringA == NULL) {
        return ((dwSize + 1) * sizeof(CHAR));
    }

    memcpy((PBYTE)lpStringA,
           (PBYTE)lpszString,
           (dwSize * sizeof(CHAR))
          );
    lpStringA[dwSize] = '\0';

    return ((dwSize + 1) * sizeof(CHAR));
}

DWORD
CompStrAToStringW(
    LPCOMPOSITIONSTRING lpCompStrA,
    LPWSTR lpStringW
    )

/*++

Routine Description:

    Convert composition string (ANSI) to String (Unicode).

Arguments:

Return Value:

--*/

{
    LPSTR lpszString;
    DWORD dwSize;
    UINT  i;

    lpszString = (LPSTR)((PBYTE)lpCompStrA + lpCompStrA->dwResultStrOffset);

    i = MultiByteToWideChar( CP_ACP,
                            (DWORD)MB_PRECOMPOSED,
                            (LPSTR)lpszString,              // src
                            (INT)lpCompStrA->dwResultStrLen,
                            (LPWSTR)lpStringW,              // dest
                            (INT)0);

    if (lpStringW == NULL) {
        dwSize = (i+1) * sizeof(WCHAR);
    }
    else {
        dwSize = (i+1) * sizeof(WCHAR);

        i = MultiByteToWideChar( CP_ACP,
                                (DWORD)MB_PRECOMPOSED,
                                (LPSTR)lpszString,              // src
                                (INT)lpCompStrA->dwResultStrLen,
                                (LPWSTR)lpStringW,              // dest
                                (INT)dwSize);

        lpStringW[i] = L'\0';
        dwSize = (i+1) * sizeof(WCHAR);
    }

    return dwSize;
}

VOID
CompStrAToCharA(
    HWND hWnd,
    LPCOMPOSITIONSTRING lpCompStrA
    )

/*++

Routine Description:

    Convert composition string (ANSI) to WM_CHAR (ANSI).

Arguments:

Return Value:

--*/

{
    LPSTR lpszString;
    BOOL  fDBCSWmChar = FALSE;
    WORD  wDBCSChar;
    BYTE  szAscii[3];

    lpszString = (LPSTR)((PBYTE)lpCompStrA + lpCompStrA->dwResultStrOffset);

    // IR_DBCSCHAR: If the app reply to this message with TRUE, we can
    // queue up double byte character in a WM_CHAR message.
    if ( GetClientInfo()->dwExpWinVer >= 0x030A ) {
        fDBCSWmChar = (BOOL)SendMessageA( hWnd,WM_IME_REPORT,IR_DBCSCHAR, 0L);
    }

    // Send IR_STRINGSTART prior to anything.
    PostMessageA( hWnd, WM_IME_REPORT, IR_STRINGSTART, 0L );


    while(szAscii[0]=*lpszString)
    {
        if( *CharNextA(lpszString) == 0 )
        {
            PostMessageA( hWnd, WM_IME_REPORT, IR_STRINGEND, 0L );
        }
        if( IsDBCSLeadByte( szAscii[0] ) )
        {
            szAscii[1] = *((PBYTE)(lpszString+1));

            //  If fDBCSWmChar==TRUE, The app can recieve WM_CHARs which
            // have double byte code in wParam.
            if ( fDBCSWmChar )
            {
                // It's necessary to swap bytes to put 1st byte into upper
                // part of wParam, and 2nd byte into lower part.
                wDBCSChar = MAKEWORD(szAscii[1], szAscii[0]);
                PostMessageA( hWnd, WM_CHAR, (WPARAM)wDBCSChar|WMCR_IR_DBCSCHAR, 1L );
            }
            else
            {
                // Send each byte on a WM_CHAR
                PostMessageA( hWnd, WM_CHAR, (WPARAM)(szAscii[0]), 1L);
                PostMessageA( hWnd, WM_CHAR, (WPARAM)(szAscii[1]), 1L);
            }
        }
        else
        {
            PostMessageA( hWnd, WM_CHAR, (WPARAM)(szAscii[0]), 1L);
        }
        lpszString = CharNextA(lpszString);
    }
}

VOID
CompStrAToCharW(
    HWND hWnd,
    LPCOMPOSITIONSTRING lpCompStrA
    )

/*++

Routine Description:

    Convert composition string (ANSI) to WM_CHAR (Unicode).

Arguments:

Return Value:

--*/

{
    LPSTR lpszString;
    UINT  i;
    BYTE  c;
    WORD  wszUnicode[2];

    lpszString = (LPSTR)((PBYTE)lpCompStrA + lpCompStrA->dwResultStrOffset);

    // IR_DBCSCHAR: If the app reply to this message with TRUE, we can
    // queue up double byte character in a WM_CHAR message.
    // SendMessageW( hWnd,WM_IME_REPORT,IR_DBCSCHAR, 0L);

    // Send IR_STRINGSTART prior to anything.
    PostMessageW( hWnd, WM_IME_REPORT, IR_STRINGSTART, 0L );


    while(c=*lpszString)
    {
        if( *CharNextA(lpszString) == 0 )
        {
            PostMessageW( hWnd, WM_IME_REPORT, IR_STRINGEND, 0L );
        }
        if( IsDBCSLeadByte( c ) ) {
            i = MultiByteToWideChar( CP_ACP,
                                    (DWORD)MB_PRECOMPOSED,
                                    (LPSTR)lpszString,  // src
                                    (INT)2,
                                    (LPWSTR)wszUnicode, // dest
                                    (INT)sizeof(wszUnicode));
        }
        else {
            i = MultiByteToWideChar( CP_ACP,
                                    (DWORD)MB_PRECOMPOSED,
                                    (LPSTR)lpszString,  // src
                                    (INT)1,
                                    (LPWSTR)wszUnicode, // dest
                                    (INT)sizeof(wszUnicode));
        }
        if (i != 0) {
            PostMessageW( hWnd, WM_CHAR, (WPARAM)(wszUnicode[0]), 1L);
        }
        lpszString = CharNextA(lpszString);
    }
}

DWORD
CompStrWToUndetW(
    DWORD dwGCS,
    LPCOMPOSITIONSTRING lpCompStrW,
    LPUNDETERMINESTRUCT lpUndetW
    )

/*++

Routine Description:

    Convert composition string (Unicode) to undetermine string (Unicode).

Arguments:

Return Value:

--*/

{
    DWORD dwPos;
    DWORD dwSize;
    UINT  i;

    dwSize = DWORD_ALIGN((sizeof(UNDETERMINESTRUCT)+1)) +
             DWORD_ALIGN(((lpCompStrW->dwResultStrLen+1) * sizeof(WCHAR))) +
             DWORD_ALIGN((lpCompStrW->dwResultClauseLen+1)) +
             DWORD_ALIGN(((lpCompStrW->dwResultReadStrLen+1) * sizeof(WCHAR))) +
             DWORD_ALIGN((lpCompStrW->dwResultReadClauseLen+1)) +
             DWORD_ALIGN((lpCompStrW->dwCompAttrLen+1)) +
             DWORD_ALIGN(((lpCompStrW->dwCompStrLen+1) * sizeof(WCHAR)));

    if (lpUndetW == NULL) {
        return dwSize;
    }

    dwPos = DWORD_ALIGN((sizeof(UNDETERMINESTRUCT) + 1));
    lpUndetW->dwSize = dwSize;

    if (dwGCS & GCS_COMPSTR)
    {
        lpUndetW->uUndetTextLen = lpCompStrW->dwCompStrLen;
        lpUndetW->uUndetTextPos = dwPos;
        memcpy((PBYTE)lpUndetW   + dwPos,
               (PBYTE)lpCompStrW + lpCompStrW->dwCompStrOffset,
               lpCompStrW->dwCompStrLen * sizeof(WCHAR)
              );
        *(LPWSTR)((PBYTE)lpUndetW + dwPos + lpCompStrW->dwCompStrLen*sizeof(WCHAR)) = L'\0';
        dwPos += DWORD_ALIGN(((lpUndetW->uUndetTextLen+1)*sizeof(WCHAR)));

        // Sometime Chicago IME does not generate GCS_COMPATTR
        // with GCS_COMPSTR. But uUndetAttrPos should be filled
        // when the UndetText is updated.
        if (lpCompStrW->dwCompAttrLen && !(dwGCS & GCS_COMPATTR))
            dwGCS |= GCS_COMPATTR;
    }

    if (dwGCS & GCS_COMPATTR)
    {
        lpUndetW->uUndetAttrPos = dwPos;
        memcpy((PBYTE)lpUndetW   + dwPos,
               (PBYTE)lpCompStrW + lpCompStrW->dwCompAttrOffset,
               lpCompStrW->dwCompAttrLen
              );
        dwPos += DWORD_ALIGN((lpUndetW->uUndetTextLen + 1));
    }

    if (dwGCS & GCS_CURSORPOS)
    {
        lpUndetW->uCursorPos = lpCompStrW->dwCursorPos;
    }

    if (dwGCS & GCS_DELTASTART)
    {
        lpUndetW->uDeltaStart = lpCompStrW->dwDeltaStart;
    }

    if (dwGCS & GCS_RESULTSTR)
    {
        lpUndetW->uDetermineTextLen = lpCompStrW->dwResultStrLen;
        lpUndetW->uDetermineTextPos = dwPos;
        memcpy((PBYTE)lpUndetW   + dwPos,
               (PBYTE)lpCompStrW + lpCompStrW->dwResultStrOffset,
               lpCompStrW->dwResultStrLen * sizeof(WCHAR)
              );
        *(LPWSTR)((PBYTE)lpUndetW + dwPos + lpCompStrW->dwResultStrLen*sizeof(WCHAR)) = L'\0';
        dwPos += DWORD_ALIGN(((lpUndetW->uDetermineTextLen + 1)*sizeof(WCHAR)));
    }

    if ( (dwGCS & GCS_RESULTCLAUSE) &&
         (lpCompStrW->dwResultClauseLen > 0) )
    {
        LPDWORD lpw;
        LPDWORD lpdw;

        lpUndetW->uDetermineDelimPos = dwPos;

        lpw  = (LPDWORD)((PBYTE)lpUndetW + dwPos);
        lpdw = (LPDWORD)((PBYTE)lpCompStrW + lpCompStrW->dwResultClauseOffset);
        for (i = 0; i < (lpCompStrW->dwResultClauseLen / sizeof(DWORD)); i++)
            *lpw++ = *lpdw++;

        dwPos += DWORD_ALIGN((lpCompStrW->dwResultClauseLen + 1));
    }

    if (dwGCS & GCS_RESULTREADSTR)
    {
        lpUndetW->uYomiTextLen = lpCompStrW->dwResultReadStrLen;
        lpUndetW->uYomiTextPos = dwPos;
        memcpy((PBYTE)lpUndetW   + dwPos,
               (PBYTE)lpCompStrW + lpCompStrW->dwResultReadStrOffset,
               lpCompStrW->dwResultReadStrLen * sizeof(WCHAR)
              );
        *(LPWSTR)((PBYTE)lpUndetW + dwPos + lpCompStrW->dwResultReadStrLen*sizeof(WCHAR)) = L'\0';
        dwPos += DWORD_ALIGN(((lpUndetW->uYomiTextLen + 1)*sizeof(WCHAR)));
    }

    if ( (dwGCS & GCS_RESULTREADCLAUSE) &&
         (lpCompStrW->dwResultReadClauseLen > 0) )
    {
        LPDWORD lpw;
        LPDWORD lpdw;

        lpUndetW->uYomiDelimPos = dwPos;

        lpw  = (LPDWORD)((PBYTE)lpUndetW + dwPos);
        lpdw = (LPDWORD)((PBYTE)lpCompStrW + lpCompStrW->dwResultReadClauseOffset);
        for (i = 0; i < (lpCompStrW->dwResultReadClauseLen / sizeof(DWORD)); i++)
            *lpw++ = *lpdw++;

        dwPos += DWORD_ALIGN((lpCompStrW->dwResultReadClauseLen + 1));
    }

    return dwSize;
}

DWORD
CompStrWToUndetA(
    DWORD dwGCS,
    LPCOMPOSITIONSTRING lpCompStrW,
    LPUNDETERMINESTRUCT lpUndetA
    )

/*++

Routine Description:

    Convert composition string (Unicode) to undetermine string (ANSI).

Arguments:

Return Value:

--*/

{
    DWORD dwPos;
    DWORD dwSize;
    UINT  i;
    BOOL bUDC;

    dwSize = DWORD_ALIGN((sizeof(UNDETERMINESTRUCT)+1)) +
             DWORD_ALIGN(((lpCompStrW->dwResultStrLen+1) * sizeof(WCHAR))) +
             DWORD_ALIGN((lpCompStrW->dwResultClauseLen+1)) +
             DWORD_ALIGN(((lpCompStrW->dwResultReadStrLen+1) * sizeof(WCHAR))) +
             DWORD_ALIGN((lpCompStrW->dwResultReadClauseLen+1)) +
             DWORD_ALIGN((lpCompStrW->dwCompAttrLen * 2)) +
             DWORD_ALIGN(((lpCompStrW->dwCompStrLen+1) * sizeof(WCHAR)));

    if (lpUndetA == NULL) {
        return dwSize;
    }

    dwPos = DWORD_ALIGN((sizeof(UNDETERMINESTRUCT) + 1));
    lpUndetA->dwSize = dwSize;

    if (dwGCS & GCS_COMPSTR)
    {
        i = WideCharToMultiByte( CP_ACP,
                                (DWORD)0,
                                (LPWSTR)((PBYTE)lpCompStrW + lpCompStrW->dwCompStrOffset),  // src
                                (INT)lpCompStrW->dwCompStrLen,
                                (LPSTR)((PBYTE)lpUndetA + dwPos),                           // dest
                                (INT)dwSize - dwPos,
                                (LPSTR)NULL,
                                (LPBOOL)&bUDC);
        ((LPSTR)((PBYTE)lpUndetA + dwPos))[i] = '\0';
        lpUndetA->uUndetTextLen = i;
        lpUndetA->uUndetTextPos = dwPos;
        dwPos += DWORD_ALIGN(((lpUndetA->uUndetTextLen + 1)*sizeof(CHAR)));

        // Sometime Chicago IME does not generate GCS_COMPATTR
        // with GCS_COMPSTR. But uUndetAttrPos should be filled
        // when the UndetText is updated.
        if (lpCompStrW->dwCompAttrLen && !(dwGCS & GCS_COMPATTR))
            dwGCS |= GCS_COMPATTR;
    }

    if (dwGCS & GCS_COMPATTR)
    {
        if (lpUndetA->uUndetTextLen != 0) {
            LPSTR  lpszUndetText;
            PBYTE  lpAttrW;
            PBYTE  lpAttrA;
            BYTE   c;

            lpszUndetText  = (LPSTR)((PBYTE)lpUndetA + lpUndetA->uUndetTextPos);
            lpAttrW = (PBYTE)lpCompStrW + lpCompStrW->dwCompAttrOffset;
            lpAttrA = (PBYTE)lpUndetA   + dwPos;

            while (c=*lpszUndetText++) {
                if (IsDBCSLeadByte(c)) {
                    *lpAttrA++ = *lpAttrW;
                    *lpAttrA++ = *lpAttrW;
                    lpszUndetText++;
                }
                else {
                    *lpAttrA++ = *lpAttrW;
                }
                lpAttrW++;
            }

            lpUndetA->uUndetAttrPos = dwPos;
            dwPos += DWORD_ALIGN((lpUndetA->uUndetTextLen + 1));
        }
    }

    if (dwGCS & GCS_CURSORPOS)
    {
        if (lpCompStrW->dwCursorPos != -1) {
            lpUndetA->uCursorPos = CalcCharacterPositionWtoA(lpCompStrW->dwCursorPos,
                                                             (LPWSTR)((PBYTE)lpCompStrW + lpCompStrW->dwCompStrOffset),
                                                             CP_ACP
                                                            );
        }
        else {
            lpUndetA->uCursorPos = lpCompStrW->dwCursorPos;
        }
    }

    if (dwGCS & GCS_DELTASTART)
    {
        if (lpCompStrW->dwDeltaStart != -1) {
            lpUndetA->uDeltaStart = CalcCharacterPositionWtoA(lpCompStrW->dwDeltaStart,
                                                              (LPWSTR)((PBYTE)lpCompStrW + lpCompStrW->dwCompStrOffset),
                                                              CP_ACP
                                                             );
        }
        else {
            lpUndetA->uDeltaStart = lpCompStrW->dwDeltaStart;
        }
    }

    if (dwGCS & GCS_RESULTSTR)
    {
        i = WideCharToMultiByte( CP_ACP,
                                (DWORD)0,
                                (LPWSTR)((PBYTE)lpCompStrW + lpCompStrW->dwResultStrOffset),  // src
                                (INT)lpCompStrW->dwResultStrLen,
                                (LPSTR)((PBYTE)lpUndetA + dwPos),                             // dest
                                (INT)dwSize - dwPos,
                                (LPSTR)NULL,
                                (LPBOOL)&bUDC);
        ((LPSTR)((PBYTE)lpUndetA + dwPos))[i] = '\0';
        lpUndetA->uDetermineTextLen = i;
        lpUndetA->uDetermineTextPos = dwPos;
        dwPos += DWORD_ALIGN(((lpUndetA->uDetermineTextLen + 1)*sizeof(CHAR)));
    }

    if ( (dwGCS & GCS_RESULTCLAUSE) &&
         (lpCompStrW->dwResultClauseLen > 0) )
    {
        if (lpUndetA->uDetermineTextLen != 0) {
            LPDWORD lpw;
            LPDWORD lpdw;

            lpw  = (LPDWORD)((PBYTE)lpUndetA + dwPos);
            lpdw = (LPDWORD)((PBYTE)lpCompStrW + lpCompStrW->dwResultClauseOffset);

            for (i = 0; i < (lpCompStrW->dwResultClauseLen / sizeof(DWORD)); i++) {
                *lpw++ = CalcCharacterPositionWtoA(*lpdw++,
                                                   (LPWSTR)((PBYTE)lpCompStrW + lpCompStrW->dwResultStrOffset),
                                                   CP_ACP
                                                  );
            }

            lpUndetA->uDetermineDelimPos = dwPos;
            dwPos += DWORD_ALIGN((lpCompStrW->dwResultClauseLen + 1));
        }
    }

    if (dwGCS & GCS_RESULTREADSTR)
    {
        i = WideCharToMultiByte( CP_ACP,
                                (DWORD)0,
                                (LPWSTR)((PBYTE)lpCompStrW + lpCompStrW->dwResultReadStrOffset),  // src
                                (INT)lpCompStrW->dwResultReadStrLen,
                                (LPSTR)((PBYTE)lpUndetA + dwPos),                                 // dest
                                (INT)dwSize - dwPos,
                                (LPSTR)NULL,
                                (LPBOOL)&bUDC);
        ((LPSTR)((PBYTE)lpUndetA + dwPos))[i] = '\0';
        lpUndetA->uYomiTextLen = i;
        lpUndetA->uYomiTextPos = dwPos;
        dwPos += DWORD_ALIGN(((lpUndetA->uYomiTextLen + 1)*sizeof(CHAR)));
    }

    if ( (dwGCS & GCS_RESULTREADCLAUSE) &&
         (lpCompStrW->dwResultReadClauseLen > 0) )
    {
        if (lpUndetA->uYomiTextLen != 0) {
            LPDWORD lpw;
            LPDWORD lpdw;

            lpw  = (LPDWORD)((PBYTE)lpUndetA + dwPos);
            lpdw = (LPDWORD)((PBYTE)lpCompStrW + lpCompStrW->dwResultReadClauseOffset);

            for (i = 0; i < (lpCompStrW->dwResultReadClauseLen / sizeof(DWORD)); i++) {
                *lpw++ = CalcCharacterPositionWtoA(*lpdw++,
                                                   (LPWSTR)((PBYTE)lpCompStrW + lpCompStrW->dwResultReadStrOffset),
                                                   CP_ACP
                                                  );
            }

            lpUndetA->uYomiDelimPos = dwPos;
            dwPos += DWORD_ALIGN((lpCompStrW->dwResultReadClauseLen + 1));
        }
    }

    return dwSize;
}


DWORD
CompStrWToStringExW(
    DWORD dwGCS,
    LPCOMPOSITIONSTRING lpCompStrW,
    LPSTRINGEXSTRUCT lpStringExW
    )

/*++

Routine Description:

    Convert composition string (Unicode) to StringEx (Unicode).

Arguments:

Return Value:

--*/

{
    DWORD dwPos;
    DWORD dwSize;
    UINT  i;

    dwSize = DWORD_ALIGN((sizeof(STRINGEXSTRUCT)+1)) +
             DWORD_ALIGN(((lpCompStrW->dwResultStrLen+1) * sizeof(WCHAR))) +
             DWORD_ALIGN(lpCompStrW->dwResultClauseLen+1) +
             DWORD_ALIGN(((lpCompStrW->dwResultReadStrLen+1) * sizeof(WCHAR))) +
             DWORD_ALIGN(lpCompStrW->dwResultReadClauseLen+1);

    if (lpStringExW == NULL) {
        return dwSize;
    }

    dwPos = DWORD_ALIGN(sizeof(STRINGEXSTRUCT) + 1);
    lpStringExW->dwSize = dwSize;

    lpStringExW->uDeterminePos = dwPos;
    memcpy((PBYTE)lpStringExW + dwPos,
           (PBYTE)lpCompStrW  + lpCompStrW->dwResultStrOffset,
           lpCompStrW->dwResultStrLen * sizeof(WCHAR)
          );
    *(LPWSTR)((PBYTE)lpStringExW + dwPos + lpCompStrW->dwResultStrLen*sizeof(WCHAR)) = L'\0';
    dwPos += DWORD_ALIGN(((lpCompStrW->dwResultStrLen + 1)*sizeof(WCHAR)));

    if ( (dwGCS & GCS_RESULTCLAUSE) &&
         (lpCompStrW->dwResultClauseLen > 0) )
    {
        LPDWORD lpw;
        LPDWORD lpdw;

        lpStringExW->uDetermineDelimPos = dwPos;

        lpw  = (LPDWORD)((PBYTE)lpStringExW + dwPos);
        lpdw = (LPDWORD)((PBYTE)lpCompStrW + lpCompStrW->dwResultClauseOffset);
        for (i = 0; i < (lpCompStrW->dwResultClauseLen / sizeof(DWORD)); i++)
            *lpw++ = *lpdw++;

        dwPos += DWORD_ALIGN((lpCompStrW->dwResultClauseLen + 1));
    }

    lpStringExW->uYomiPos = dwPos;
    memcpy((PBYTE)lpStringExW + dwPos,
           (PBYTE)lpCompStrW  + lpCompStrW->dwResultReadStrOffset,
           lpCompStrW->dwResultReadStrLen * sizeof(WCHAR)
          );
    *(LPWSTR)((PBYTE)lpStringExW + dwPos + lpCompStrW->dwResultReadStrLen*sizeof(WCHAR)) = L'\0';
    dwPos += DWORD_ALIGN(((lpCompStrW->dwResultReadStrLen + 1)*sizeof(WCHAR)));

    if ( (dwGCS & GCS_RESULTREADCLAUSE) &&
         (lpCompStrW->dwResultReadClauseLen > 0) )
    {
        LPDWORD lpw;
        LPDWORD lpdw;

        lpStringExW->uYomiDelimPos = dwPos;

        lpw  = (LPDWORD)((PBYTE)lpStringExW + dwPos);
        lpdw = (LPDWORD)((PBYTE)lpCompStrW + lpCompStrW->dwResultReadClauseOffset);
        for (i = 0; i < (lpCompStrW->dwResultReadClauseLen / sizeof(DWORD)); i++)
            *lpw++ = *lpdw++;
    }

    return dwSize;
}

DWORD
CompStrWToStringExA(
    DWORD dwGCS,
    LPCOMPOSITIONSTRING lpCompStrW,
    LPSTRINGEXSTRUCT lpStringExA
    )

/*++

Routine Description:

    Convert composition string (Unicode) to StringEx (ANSI).

Arguments:

Return Value:

--*/

{
    DWORD dwPos;
    DWORD dwSize;
    DWORD dwLen;
    UINT  i;
    BOOL bUDC;

    dwSize = DWORD_ALIGN((sizeof(STRINGEXSTRUCT)+1)) +
             (lpCompStrW->dwResultStrLen > 0 ? DWORD_ALIGN(((lpCompStrW->dwResultStrLen+1) * sizeof(WCHAR))) : 0) +
             (lpCompStrW->dwResultClauseLen > 0 ? DWORD_ALIGN(lpCompStrW->dwResultClauseLen+1) : 0) +
             (lpCompStrW->dwResultReadStrLen > 0 ? DWORD_ALIGN(((lpCompStrW->dwResultReadStrLen+1) * sizeof(WCHAR))) : 0)+
             (lpCompStrW->dwResultReadClauseLen > 0 ? DWORD_ALIGN(lpCompStrW->dwResultReadClauseLen+1) : 0);

    if (lpStringExA == NULL) {
        return dwSize;
    }

    dwPos = DWORD_ALIGN(sizeof(STRINGEXSTRUCT) + 1);
    lpStringExA->dwSize = dwSize;

    if (lpCompStrW->dwResultStrLen > 0) {
        i = WideCharToMultiByte( CP_ACP,
                                (DWORD)0,
                                (LPWSTR)((PBYTE)lpCompStrW + lpCompStrW->dwResultStrOffset),  // src
                                (INT)lpCompStrW->dwResultStrLen,
                                (LPSTR)((PBYTE)lpStringExA + dwPos),                          // dest
                                (INT)dwSize - dwPos,
                                (LPSTR)NULL,
                                (LPBOOL)&bUDC);
        ((LPSTR)((PBYTE)lpStringExA + dwPos))[i] = '\0';
        dwLen = i;
        lpStringExA->uDeterminePos = dwPos;
        dwPos += DWORD_ALIGN(((dwLen + 1)*sizeof(CHAR)));
    } else {
        dwLen = 0;
        lpStringExA->uDeterminePos = 0;
    }

    if ( (dwGCS & GCS_RESULTCLAUSE) &&
         (lpCompStrW->dwResultClauseLen > 0) )
    {
        if (dwLen != 0 && lpCompStrW->dwResultClauseLen > 0) {
            LPDWORD lpw;
            LPDWORD lpdw;

            lpw  = (LPDWORD)((PBYTE)lpStringExA + dwPos);
            lpdw = (LPDWORD)((PBYTE)lpCompStrW + lpCompStrW->dwResultClauseOffset);

            for (i = 0; i < (lpCompStrW->dwResultClauseLen / sizeof(DWORD)); i++) {
                *lpw++ = CalcCharacterPositionWtoA(*lpdw++,
                                                   (LPWSTR)((PBYTE)lpCompStrW + lpCompStrW->dwResultStrOffset),
                                                   CP_ACP
                                                  );
            }

            lpStringExA->uDetermineDelimPos = dwPos;
            dwPos += DWORD_ALIGN((lpCompStrW->dwResultClauseLen + 1));
        }
    }

    if (lpCompStrW->dwResultReadStrLen > 0) {
        i = WideCharToMultiByte( CP_ACP,
                                (DWORD)0,
                                (LPWSTR)((PBYTE)lpCompStrW + lpCompStrW->dwResultReadStrOffset),  // src
                                (INT)lpCompStrW->dwResultReadStrLen,
                                (LPSTR)((PBYTE)lpStringExA + dwPos),                              // dest
                                (INT)dwSize - dwPos,
                                (LPSTR)NULL,
                                (LPBOOL)&bUDC);
        ((LPSTR)((PBYTE)lpStringExA + dwPos))[i] = '\0';
        dwLen = i;
        lpStringExA->uYomiPos = dwPos;
        dwPos += DWORD_ALIGN(((dwLen + 1)*sizeof(CHAR)));
    } else {
        dwLen = 0;
        lpStringExA->uYomiPos = 0;
    }

    if ( (dwGCS & GCS_RESULTREADCLAUSE) &&
         (lpCompStrW->dwResultReadClauseLen > 0) )
    {
        if (dwLen != 0 && lpCompStrW->dwResultReadClauseLen > 0) {
            LPDWORD lpw;
            LPDWORD lpdw;

            lpw  = (LPDWORD)((PBYTE)lpStringExA + dwPos);
            lpdw = (LPDWORD)((PBYTE)lpCompStrW + lpCompStrW->dwResultReadClauseOffset);

            for (i = 0; i < (lpCompStrW->dwResultReadClauseLen / sizeof(DWORD)); i++) {
                *lpw++ = CalcCharacterPositionWtoA(*lpdw++,
                                                   (LPWSTR)((PBYTE)lpCompStrW + lpCompStrW->dwResultReadStrOffset),
                                                   CP_ACP
                                                  );
            }

            lpStringExA->uYomiDelimPos = dwPos;
            dwPos += DWORD_ALIGN((lpCompStrW->dwResultReadClauseLen + 1));
        }
    }

    return dwSize;
}

DWORD
CompStrWToStringW(
    LPCOMPOSITIONSTRING lpCompStrW,
    LPWSTR lpStringW
    )

/*++

Routine Description:

    Convert composition string (Unicode) to String (Unicode).

Arguments:

Return Value:

--*/

{
    LPWSTR lpwszString;
    DWORD dwSize;

    lpwszString = (LPWSTR)((PBYTE)lpCompStrW + lpCompStrW->dwResultStrOffset);
    dwSize = lpCompStrW->dwResultStrLen;

    if (lpStringW == NULL) {
        return ((dwSize + 1) * sizeof(WCHAR));
    }

    memcpy((PBYTE)lpStringW,
           (PBYTE)lpwszString,
           (dwSize * sizeof(WCHAR))
          );
    lpStringW[dwSize] = L'\0';

    return ((dwSize + 1) * sizeof(WCHAR));
}

DWORD
CompStrWToStringA(
    LPCOMPOSITIONSTRING lpCompStrW,
    LPSTR lpStringA
    )

/*++

Routine Description:

    Convert composition string (Unicode) to String (ANSI).

Arguments:

Return Value:

--*/

{
    LPWSTR lpwszString;
    DWORD dwSize;
    UINT  i;
    BOOL bUDC;

    lpwszString = (LPWSTR)((PBYTE)lpCompStrW + lpCompStrW->dwResultStrOffset);

    i = WideCharToMultiByte( CP_ACP,
                            (DWORD)0,
                            (LPWSTR)lpwszString,                             // src
                            (INT)lpCompStrW->dwResultStrLen,
                            (LPSTR)lpStringA,                                // dest
                            (INT)0,
                            (LPSTR)NULL,
                            (LPBOOL)&bUDC);

    if (lpStringA == NULL) {
        dwSize = (i+1) * sizeof(CHAR);
    }
    else {
        dwSize = (i+1) * sizeof(CHAR);

        i = WideCharToMultiByte( CP_ACP,
                                (DWORD)0,
                                (LPWSTR)lpwszString,                             // src
                                (INT)lpCompStrW->dwResultStrLen,
                                (LPSTR)lpStringA,                                // dest
                                (INT)dwSize,
                                (LPSTR)NULL,
                                (LPBOOL)&bUDC);

        lpStringA[i] = '\0';
        dwSize = (i+1) * sizeof(CHAR);
    }

    return dwSize;
}

VOID
CompStrWToCharW(
    HWND hWnd,
    LPCOMPOSITIONSTRING lpCompStrW
    )

/*++

Routine Description:

    Convert composition string (Unicode) to WM_CHAR (Unicode).

Arguments:

Return Value:

--*/

{
    LPWSTR lpwszString;

    lpwszString = (LPWSTR)((PBYTE)lpCompStrW + lpCompStrW->dwResultStrOffset);

    // IR_DBCSCHAR: If the app reply to this message with TRUE, we can
    // queue up double byte character in a WM_CHAR message.
    // SendMessageW( hWnd,WM_IME_REPORT,IR_DBCSCHAR, 0L);

    // Send IR_STRINGSTART prior to anything.
    PostMessageW( hWnd, WM_IME_REPORT, IR_STRINGSTART, 0L );


    while(*lpwszString)
    {
        if( *CharNextW(lpwszString) == 0 )
        {
            PostMessageW( hWnd, WM_IME_REPORT, IR_STRINGEND, 0L );
        }
        PostMessageW( hWnd, WM_CHAR, *lpwszString, 1L);
        lpwszString = CharNextW(lpwszString);
    }
}

VOID
CompStrWToCharA(
    HWND hWnd,
    LPCOMPOSITIONSTRING lpCompStrW
    )

/*++

Routine Description:

    Convert composition string (Unicode) to WM_CHAR (ANSI).

Arguments:

Return Value:

--*/

{
    LPWSTR lpwszString;
    BOOL   fDBCSWmChar = FALSE;
    WORD   wDBCSChar;
    UINT   i;
    BOOL   bUDC;
    BYTE   szAscii[3];

    lpwszString = (LPWSTR)((PBYTE)lpCompStrW + lpCompStrW->dwResultStrOffset);

    //
    // IR_DBCSCHAR: If the app reply to this message with TRUE, we can
    // queue up double byte character in a WM_CHAR message.
    //
    //

    if ( GetClientInfo()->dwExpWinVer >= 0x030A ) {
       fDBCSWmChar = (BOOL)SendMessageA( hWnd,WM_IME_REPORT,IR_DBCSCHAR, 0L);
    }

    //
    // Send IR_STRINGSTART prior to anything.
    //

    PostMessageA( hWnd, WM_IME_REPORT, IR_STRINGSTART, 0L );


    while (*lpwszString) {

       if ( *CharNextW(lpwszString) == 0 ) {
           PostMessageA( hWnd, WM_IME_REPORT, IR_STRINGEND, 0L );
       }

       i = WideCharToMultiByte( CP_ACP,
                                (DWORD)0,
                                (LPWSTR)lpwszString,  // src
                                (INT)1,
                                (LPSTR)szAscii,       // dest
                                (INT)sizeof(szAscii),
                                (LPSTR)NULL,
                                (LPBOOL)&bUDC);

        if (i != 0) {

            if ( IsDBCSLeadByte( szAscii[0] ) ) {

               //
               //  If fDBCSWmChar==TRUE, The app can recieve WM_CHARs which
               // have double byte code in wParam.
               //

               if ( fDBCSWmChar )
               {
                   //
                   // It's necessary to swap bytes to put 1st byte into upper
                   // part of wParam, and 2nd byte into lower part.
                   //

                   wDBCSChar = MAKEWORD(szAscii[1], szAscii[0]);
                   PostMessageA( hWnd, WM_CHAR, wDBCSChar|WMCR_IR_DBCSCHAR, 1L );
               }
               else
               {
                   //
                   // Send each byte on a WM_CHAR
                   //

                   PostMessageA( hWnd, WM_CHAR, (WPARAM)(szAscii[0]), 1L);
                   PostMessageA( hWnd, WM_CHAR, (WPARAM)(szAscii[1]), 1L);
               }
           }
           else
           {
               PostMessageA( hWnd, WM_CHAR, (WPARAM)(szAscii[0]), 1L);
           }
       }

       lpwszString = CharNextW(lpwszString);
   }

}
