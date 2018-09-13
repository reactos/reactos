/**************************************************************************\
* Module Name: ctxtinfo.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Get/set routines of various Input context information for imm32.dll
*
* History:
* 26-Feb-1996 wkwok
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

// Helper function:
// Converts RECONVERTSTRING structure between ANSI and UNICODE.
extern DWORD ImmReconversionWorker(LPRECONVERTSTRING lpRecTo, LPRECONVERTSTRING lpRecFrom, BOOL bToAnsi, DWORD dwCodePage);


int UnicodeToMultiByteSize(DWORD dwCodePage, LPCWSTR pwstr)
{
    char dummy[2], *lpszDummy = dummy;
    return WCSToMBEx((WORD)dwCodePage, pwstr, 1, &lpszDummy, sizeof(WCHAR), FALSE);
}


/***************************************************************************\
* ImmGetCompositionStringA
*
* Query composition string information specified by dwIndex.
*
* History:
* 28-Feb-1995   wkwok   Created
\***************************************************************************/

LONG WINAPI ImmGetCompositionStringA(
    HIMC   hImc,
    DWORD  dwIndex,
    LPVOID lpBuf,
    DWORD  dwBufLen)
{
    PCLIENTIMC     pClientImc;
    PINPUTCONTEXT  pInputContext;
    PCOMPOSITIONSTRING pCompStr;
    BOOL           fAnsi;
    LONG           lRet = 0;
    DWORD          dwCodePage;

    if (dwBufLen != 0 && lpBuf == NULL) {
        RIPMSG0(RIP_WARNING, "ImmGetCompositionStringW: NULL lpBuf.");
        return lRet;
    }

    pClientImc = ImmLockClientImc(hImc);
    if (pClientImc == NULL) {
        RIPMSG1(RIP_WARNING,
              "ImmGetCompositionStringA: Invalid hImc %lx.", hImc);
        return lRet;
    }

    fAnsi = !TestICF(pClientImc, IMCF_UNICODE);
    dwCodePage = CImcCodePage(pClientImc);

    ImmUnlockClientImc(pClientImc);

    pInputContext = ImmLockIMC(hImc);
    if (pInputContext == NULL) {
        RIPMSG1(RIP_WARNING, "ImmGetCompositionStringA: Lock hImc %lx failed.", hImc);
        return lRet;
    }

    pCompStr = (PCOMPOSITIONSTRING)ImmLockIMCC(pInputContext->hCompStr);
    if (pCompStr == NULL) {
        RIPMSG1(RIP_WARNING, "ImmGetCompositionStringA: Lock hCompStr %x failed",
              pInputContext->hCompStr);
        ImmUnlockIMC(hImc);
        return lRet;
    }

    lRet = InternalGetCompositionStringA(pCompStr, dwIndex,
                     lpBuf, dwBufLen, fAnsi, dwCodePage);

    ImmUnlockIMCC(pInputContext->hCompStr);
    ImmUnlockIMC(hImc);

    return lRet;
}


/***************************************************************************\
* ImmGetCompositionStringA
*
* Query composition string information specified by dwIndex.
*
* History:
* 28-Feb-1995   wkwok   Created
\***************************************************************************/

LONG WINAPI ImmGetCompositionStringW(
    HIMC   hImc,
    DWORD  dwIndex,
    LPVOID lpBuf,
    DWORD  dwBufLen)
{
    PCLIENTIMC     pClientImc;
    PINPUTCONTEXT  pInputContext;
    PCOMPOSITIONSTRING pCompStr;
    BOOL           fAnsi;
    LONG           lRet = 0;
    DWORD          dwCodePage;

    if (dwBufLen != 0 && lpBuf == NULL) {
        RIPMSG0(RIP_WARNING, "ImmGetCompositionStringW: NULL lpBuf.");
        return lRet;
    }

    pClientImc = ImmLockClientImc(hImc);
    if (pClientImc == NULL) {
        RIPMSG1(RIP_WARNING,
              "ImmGetCompositionStringW: Invalid hImc %lx.", hImc);
        return lRet;
    }

    fAnsi = !TestICF(pClientImc, IMCF_UNICODE);
    dwCodePage = CImcCodePage(pClientImc);

    ImmUnlockClientImc(pClientImc);

    pInputContext = ImmLockIMC(hImc);
    if (pInputContext == NULL) {
        RIPMSG1(RIP_WARNING, "ImmGetCompositionStringW: Lock hImc %lx failed.", hImc);
        return lRet;
    }

    pCompStr = (PCOMPOSITIONSTRING)ImmLockIMCC(pInputContext->hCompStr);
    if (pCompStr == NULL) {
        RIPMSG1(RIP_WARNING, "ImmGetCompositionStringA: Lock hCompStr %x failed",
              pInputContext->hCompStr);
        ImmUnlockIMC(hImc);
        return lRet;
    }

    lRet = InternalGetCompositionStringW(pCompStr, dwIndex,
                     lpBuf, dwBufLen, fAnsi, dwCodePage);

    ImmUnlockIMCC(pInputContext->hCompStr);
    ImmUnlockIMC(hImc);

    return lRet;
}


/***************************************************************************\
* ImmSetCompositionStringA
*
* Set composition string information specified by dwIndex.
*
* History:
* 28-Feb-1995   wkwok   Created
\***************************************************************************/

BOOL WINAPI ImmSetCompositionStringA(
    HIMC    hImc,
    DWORD   dwIndex,
    LPVOID lpComp,
    DWORD   dwCompLen,
    LPVOID lpRead,
    DWORD   dwReadLen)
{
    return ImmSetCompositionStringWorker(hImc, dwIndex, lpComp,
                                dwCompLen, lpRead, dwReadLen, TRUE);
}


/***************************************************************************\
* ImmSetCompositionStringW
*
* Set composition string information specified by dwIndex.
*
* History:
* 28-Feb-1995   wkwok   Created
\***************************************************************************/

BOOL WINAPI ImmSetCompositionStringW(
    HIMC    hImc,
    DWORD   dwIndex,
    LPVOID lpComp,
    DWORD   dwCompLen,
    LPVOID lpRead,
    DWORD   dwReadLen)
{
    return ImmSetCompositionStringWorker(hImc, dwIndex, lpComp,
                                dwCompLen, lpRead, dwReadLen, FALSE);
}


LONG CompositionString(
    HIMC               hImc,
    PINPUTCONTEXT      *ppInputContext,
    PCOMPOSITIONSTRING *ppCompStr,
    BOOL               fCheckSize)
{
    PINPUTCONTEXT      pInputContext;
    PCOMPOSITIONSTRING pCompStr;

    pInputContext = ImmLockIMC(hImc);
    if (!pInputContext) {
        RIPMSG1(RIP_WARNING, "CompositionString: Lock hImc %lx failed.", hImc);
        return (LONG)IMM_ERROR_GENERAL;
    }

    if (!pInputContext->hCompStr) {
        ImmUnlockIMC(hImc);
        return (LONG)IMM_ERROR_NODATA;
    }

    pCompStr = (PCOMPOSITIONSTRING)ImmLockIMCC(pInputContext->hCompStr);
    if (!pCompStr) {
        RIPMSG1(RIP_WARNING,
              "CompositionString: Lock hCompStr %lx failed.", pInputContext->hCompStr);
        ImmUnlockIMC(hImc);
        return (LONG)IMM_ERROR_GENERAL;
    }

    if (fCheckSize && pCompStr->dwSize < sizeof(COMPOSITIONSTRING)) {
        RIPMSG0(RIP_WARNING, "CompositionString: no composition string.");
        ImmUnlockIMCC(pInputContext->hCompStr);
        ImmUnlockIMC(hImc);
        return (LONG)IMM_ERROR_NODATA;
    }

    *ppInputContext = pInputContext;
    *ppCompStr = pCompStr;

    return (1);
}


BOOL CheckAttribute(
    LPBYTE  lpComp,        // the attr from apps
    DWORD   dwCompLen,     // the attr length from apps
    LPBYTE  lpAttr,        // the attr from IMC
    DWORD   dwAttrLen,     // the attr length from IMC
    LPDWORD lpClause,      // the clause from IMC
    DWORD   dwClauseLen)   // the clause length from IMC
{
    DWORD dwCnt;
    DWORD dwBound;
    BYTE bAttr;

    UNREFERENCED_PARAMETER(dwClauseLen);

    if (!lpClause) {
        RIPMSG0(RIP_WARNING, "CheckAttribute: no Clause. Pass it to IME.");
        return (TRUE);
    }

    if (!lpAttr) {
        RIPMSG0(RIP_WARNING, "CheckAttribute: no Attr. Not pass it to IME.");
        return (FALSE);
    }

    if (dwCompLen != dwAttrLen) {
        RIPMSG0(RIP_WARNING, "CheckAttribute: wrong length. Not pass it to IME.");
        return (FALSE);
    }

    /*
     * The attr. of chars of one clause have to be same.
     */
    while (*lpClause < dwCompLen) {
        dwBound = *(lpClause+1) - *lpClause;
        bAttr = *lpComp++;
        for (dwCnt = 1; dwCnt < dwBound; dwCnt++)
            if (bAttr != *lpComp++) {
                RIPMSG0(RIP_WARNING,
                      "CheckAttribute: mismatch clause att. Not Pass it to IME");
                return (FALSE);
            }
        lpClause++;
    }

    return (TRUE);
}


BOOL CheckClause(
    LPDWORD lpComp,        // the clause from apps
    DWORD   dwCompLen,     // the clause length from apps
    LPDWORD lpClause,      // the clause from IMC
    DWORD   dwClauseLen)   // the clause length from IMC
{
    UINT nCnt;
    INT  diff = 0;

    if (!dwClauseLen || !dwCompLen) {
        RIPMSG0(RIP_WARNING, "CheckClause: no Clause. Not Pass it to IME.");
        return (FALSE);
    }

    if (*lpComp || *lpClause) {
        RIPMSG0(RIP_WARNING, "CheckClause: lpClause[0] have to be ZERO.");
        return (FALSE);
    }

    for (nCnt = 0; nCnt < (UINT)(dwClauseLen/4); nCnt++)
    {
        if (*lpComp++ != *lpClause++)
        {
            diff++;
            if (dwCompLen > dwClauseLen)
                lpClause--;
            if (dwCompLen < dwClauseLen)
                lpComp--;
        }
        if (diff > 1)
            return (FALSE);
    }

    return (TRUE);
}


LPBYTE InternalSCS_SETSTR(
    LPCVOID  lpCompRead,
    DWORD    dwCompReadLen,
    LPVOID  *lplpNewCompRead,
    DWORD   *lpdwNewCompReadLen,
    BOOL     fAnsi,
    DWORD    dwCodePage)
{
    LPBYTE   lpBufRet;
    DWORD    dwBufSize;
    LPSTR    lpBufA;
    LPWSTR   lpBufW;
    INT      i;
    BOOL     bUDC;

    if (lpCompRead == NULL || dwCompReadLen == 0)
        return NULL;

    dwBufSize = dwCompReadLen * sizeof(WCHAR) * 2;

    lpBufRet = ImmLocalAlloc(0, dwBufSize);
    if (lpBufRet == NULL) {
        RIPMSG0(RIP_WARNING, "InternalSCS_SETSTR: memory failure.");
        return NULL;
    }

    lpBufW = (LPWSTR)lpBufRet;
    lpBufA = (LPSTR)(lpBufW + dwCompReadLen);

    if (fAnsi) {

        RtlCopyMemory(lpBufA, lpCompRead, dwCompReadLen);

        i = MultiByteToWideChar(dwCodePage,
                                (DWORD)MB_PRECOMPOSED,
                                (LPSTR)lpBufA,                  // src
                                (INT)dwCompReadLen,
                                (LPWSTR)lpBufW,                 // dest
                                (INT)dwCompReadLen);

        *lplpNewCompRead    = lpBufW;
        *lpdwNewCompReadLen = (DWORD)(i * sizeof(WCHAR));
    }
    else {

        RtlCopyMemory(lpBufW, lpCompRead, dwCompReadLen);

        i = WideCharToMultiByte(dwCodePage,
                                (DWORD)0,
                                lpBufW,                         // src
                                (INT)dwCompReadLen/sizeof(WCHAR),
                                (LPSTR)lpBufA,                  // dest
                                (INT)dwCompReadLen,
                                (LPSTR)NULL,
                                (LPBOOL)&bUDC);

        *lplpNewCompRead    = lpBufA;
        *lpdwNewCompReadLen = (DWORD)(i * sizeof(CHAR));
    }

    return lpBufRet;
}


LPBYTE InternalSCS_CHANGEATTR(
    HIMC     hImc,
    LPCVOID  lpCompRead,
    DWORD    dwCompReadLen,
    DWORD    dwIndex,
    LPVOID  *lplpNewCompRead,
    DWORD   *lpdwNewCompReadLen,
    BOOL     fAnsi,
    DWORD    dwCodePage)
{
    LPBYTE lpBufRet;
    LPBYTE lpAttr, lpAttrA, lpAttrW;
    DWORD  dwBufLenA, dwBufLenW;
    LPSTR  lpStrBufA, lpBufA;
    LPWSTR lpStrBufW, lpBufW;
    CHAR   c;
    WCHAR  wc;
    ULONG  MultiByteSize;

    if (lpCompRead == NULL || dwCompReadLen == 0)
        return NULL;

    if (fAnsi) {

        dwBufLenA = ImmGetCompositionStringA(hImc, dwIndex, NULL, 0);

        lpStrBufA = ImmLocalAlloc(0, dwBufLenA);
        if (lpStrBufA == NULL) {
            RIPMSG0(RIP_WARNING, "InternalSCS_CHANGEATTR: memory failure.");
            return NULL;
        }

        ImmGetCompositionStringA(hImc, dwIndex, lpStrBufA, dwBufLenA);

        lpBufRet = ImmLocalAlloc(0, dwBufLenA);
        if (lpBufRet == NULL) {
            RIPMSG0(RIP_WARNING, "InternalSCS_CHANGEATTR: memory failure.");
            ImmLocalFree(lpStrBufA);
            return NULL;
        }

        lpBufA  = lpStrBufA;
        lpAttrA = (LPBYTE)lpCompRead;
        lpAttr  = lpBufRet;

        while (dwBufLenA != 0 && (c=*lpBufA++) != 0) {
            if (IsDBCSLeadByteEx(dwCodePage, c)) {
                if (dwBufLenA >= 2) {
                    *lpAttr++ = *lpAttrA++;
                    dwBufLenA--;
                } else {
                    *lpAttr++ = *lpAttrA;
                }
                lpBufA++;
            } else {
                *lpAttr++ = *lpAttrA;
            }
            lpAttrA++;
            dwBufLenA--;
        }

        ImmLocalFree(lpStrBufA);
    }
    else {

        dwBufLenW = ImmGetCompositionStringW(hImc, dwIndex, NULL, 0);

        lpStrBufW = ImmLocalAlloc(0, dwBufLenW);
        if (lpStrBufW == NULL) {
            RIPMSG0(RIP_WARNING, "InternalSCS_CHANGEATTR: memory failure.");
            return NULL;
        }

        ImmGetCompositionStringW(hImc, dwIndex, lpStrBufW, dwBufLenW);

        lpBufRet = ImmLocalAlloc(0, dwBufLenW);
        if (lpBufRet == NULL) {
            RIPMSG0(RIP_WARNING, "InternalSCS_CHANGEATTR: memory failure.");
            ImmLocalFree(lpStrBufW);
            return NULL;
        }

        lpBufW  = lpStrBufW;
        lpAttrW = (LPBYTE)lpCompRead;
        lpAttr  = lpBufRet;

        while (dwBufLenW != 0 && (wc=*lpBufW++) != L'\0') {
            MultiByteSize = UnicodeToMultiByteSize(dwCodePage, &wc);
            if (MultiByteSize == 2) {
                *lpAttr++ = *lpAttrW;
            }
            *lpAttr++ = *lpAttrW++;
            dwBufLenW -= sizeof(WCHAR);
        }

        ImmLocalFree(lpStrBufW);
    }

    *lplpNewCompRead    = lpBufRet;
    *lpdwNewCompReadLen = (DWORD)(lpAttr - (PBYTE)lpBufRet);

    return lpBufRet;
}


LPBYTE InternalSCS_CHANGECLAUSE(
    HIMC     hImc,
    LPCVOID  lpCompRead,
    DWORD    dwCompReadLen,
    DWORD    dwIndex,
    LPDWORD *lplpNewCompRead,
    DWORD   *lpdwNewCompReadLen,
    BOOL     fAnsi,
    DWORD    dwCodePage)
{
    LPDWORD lpdw, lpNewdw, lpBufRet;
    DWORD   dwBufLenA, dwBufLenW;
    LPSTR   lpStrBufA;
    LPWSTR  lpStrBufW;
    INT     i;

    if (lpCompRead == NULL || dwCompReadLen == 0)
        return NULL;

    lpdw = (LPDWORD)lpCompRead;

    lpBufRet = ImmLocalAlloc(0, dwCompReadLen);
    if (lpBufRet == NULL) {
        RIPMSG0(RIP_WARNING, "InternalSCS_CHANGECLAUSE: memory failure.");
        return NULL;
    }

    if (fAnsi) {

        dwBufLenA = ImmGetCompositionStringA(hImc, dwIndex, NULL, 0);

        lpStrBufA = ImmLocalAlloc(0, dwBufLenA);
        if (lpStrBufA == NULL) {
            RIPMSG0(RIP_WARNING, "InternalSCS_CHANGECLAUSE: memory failure.");
            ImmLocalFree(lpBufRet);
            return NULL;
        }

        ImmGetCompositionStringA(hImc, dwIndex, lpStrBufA, dwBufLenA);
    }
    else {

        dwBufLenW = ImmGetCompositionStringW(hImc, dwIndex, NULL, 0);

        lpStrBufW = ImmLocalAlloc(0, dwBufLenW);
        if (lpStrBufW == NULL) {
            RIPMSG0(RIP_WARNING, "InternalSCS_CHANGECLAUSE: memory failure.");
            ImmLocalFree(lpBufRet);
            return NULL;
        }

        ImmGetCompositionStringW(hImc, dwIndex, lpStrBufW, dwBufLenW);
    }

    *lplpNewCompRead = lpNewdw = lpBufRet;
    *lpdwNewCompReadLen = dwCompReadLen;

    for (i = 0; i < (INT)(dwCompReadLen / sizeof(DWORD)); i++) {
        *lpNewdw++ = fAnsi ? CalcCharacterPositionAtoW(*lpdw++, lpStrBufA, dwCodePage)
                           : CalcCharacterPositionWtoA(*lpdw++, lpStrBufW, dwCodePage);
    }

    return (LPBYTE)lpBufRet;
}


LPBYTE InternalSCS_RECONVERTSTRING(
    LPRECONVERTSTRING  lpReconv,
    DWORD              dwReconvLen,
    LPRECONVERTSTRING *lplpNewReconv,
    DWORD             *lpdwNewReconvLen,
    BOOL               fAnsi,
    DWORD              dwCodePage)
{
    LPRECONVERTSTRING lpNewReconv;
    DWORD dwBufSize;

    if (lpReconv == NULL || dwReconvLen == 0)
        return NULL;

    if (fAnsi) {
        // AtoW
        dwBufSize = (lpReconv->dwSize - sizeof *lpReconv + 1) * sizeof(WCHAR) + sizeof *lpReconv;
    }
    else {
        dwBufSize = lpReconv->dwSize + sizeof(BYTE);
    }
    lpNewReconv = ImmLocalAlloc(0, dwBufSize);
    if (lpNewReconv == NULL) {
        RIPMSG0(RIP_WARNING, "InternalSCS_RECONVERTSTRING: memory failure.");
        return NULL;
    }

    lpNewReconv->dwVersion = 0;
    lpNewReconv->dwSize=  dwBufSize;

    lpNewReconv->dwSize = ImmReconversionWorker(lpNewReconv, lpReconv, !fAnsi, dwCodePage);
    if (lpNewReconv->dwSize == 0) {
        ImmLocalFree(lpNewReconv);
        return NULL;;
    }
    *lpdwNewReconvLen = lpNewReconv->dwSize;
    *lplpNewReconv = lpNewReconv;

    return (LPBYTE)lpNewReconv;
}


/***************************************************************************\
* ImmSetCompositionStringWorker
*
* Worker function of ImmSetCompositionStringA/ImmSetCompositionStringW
*
* History:
* 28-Feb-1995   wkwok   Created
\***************************************************************************/

BOOL ImmSetCompositionStringWorker(
    HIMC    hImc,
    DWORD   dwIndex,
    LPVOID lpComp,
    DWORD   dwCompLen,
    LPVOID lpRead,
    DWORD   dwReadLen,
    BOOL    fAnsi)
{
    PINPUTCONTEXT      pInputContext;
    PCOMPOSITIONSTRING pCompStr;
    DWORD              dwThreadId;
    PIMEDPI            pImeDpi;
    LPBYTE             lpCompBuf, lpReadBuf;
    LPBYTE             lpNewComp = NULL, lpNewRead = NULL;
    DWORD              dwNewCompLen, dwNewReadLen;
    BOOL               fRet = FALSE;
    BOOL               fCheckSize = TRUE;
    BOOL               fNeedAWConversion;
    LPBYTE             lpOrgComp, lpOrgRead;
    DWORD              dwOrgCompLen, dwOrgReadLen;

    dwThreadId = GetInputContextThread(hImc);
    if (dwThreadId != GetCurrentThreadId()) {
        RIPMSG1(RIP_WARNING,
              "ImmSetCompositionString: Invalid input context access %lx.", hImc);
        return FALSE;
    }

    pImeDpi = ImmLockImeDpi(GetKeyboardLayout(dwThreadId));
    if (pImeDpi == NULL)
        return FALSE;

    lpCompBuf = lpReadBuf = NULL;

    // Backup original pointers to copyback for QUERY.
    lpOrgComp = lpComp;
    lpOrgRead = lpRead;
    dwOrgCompLen = dwCompLen;
    dwOrgReadLen = dwReadLen;

    /*
     * Check if we need ANSI/Unicode conversion
     */
    if (( fAnsi && !(pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE)) ||
        (!fAnsi &&  (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE))) {
        /*
         * No A/W conversion needed.
         */
        fNeedAWConversion = FALSE;
        goto start_scs;
    }
    fNeedAWConversion = TRUE;

    switch (dwIndex) {
    case SCS_SETSTR:
        if ( lpComp &&
            (lpCompBuf = InternalSCS_SETSTR(lpComp, dwCompLen,
                                &lpNewComp, &dwNewCompLen, fAnsi, IMECodePage(pImeDpi))) == NULL)
            goto callime_scs;
        if ( lpRead &&
            (lpReadBuf = InternalSCS_SETSTR(lpRead, dwReadLen,
                                &lpNewRead, &dwNewReadLen, fAnsi, IMECodePage(pImeDpi))) == NULL)
            goto callime_scs;

        fCheckSize = FALSE;
        break;

    case SCS_CHANGEATTR:
        if ( lpComp &&
            (lpCompBuf = InternalSCS_CHANGEATTR(
                                hImc, lpComp, dwCompLen, GCS_COMPSTR,
                                &lpNewComp, &dwNewCompLen, fAnsi, IMECodePage(pImeDpi))) == NULL)
            goto callime_scs;
        if ( lpRead &&
            (lpReadBuf = InternalSCS_CHANGEATTR(
                                hImc, lpRead, dwReadLen, GCS_COMPREADSTR,
                                &lpNewRead, &dwNewReadLen, fAnsi, IMECodePage(pImeDpi))) == NULL)
            goto callime_scs;
        break;

    case SCS_CHANGECLAUSE:
       if ( lpComp &&
           (lpCompBuf = InternalSCS_CHANGECLAUSE(
                                hImc, lpComp, dwCompLen, GCS_COMPSTR,
                                (LPDWORD *)&lpNewComp, &dwNewCompLen, fAnsi, IMECodePage(pImeDpi))) == NULL)
            goto callime_scs;
        if ( lpRead &&
            (lpReadBuf = InternalSCS_CHANGECLAUSE(
                                hImc, lpRead, dwReadLen, GCS_COMPREADSTR,
                                (LPDWORD *)&lpNewRead, &dwNewReadLen, fAnsi, IMECodePage(pImeDpi))) == NULL)
            goto callime_scs;
        break;

    case SCS_SETRECONVERTSTRING:
    case SCS_QUERYRECONVERTSTRING:
        if (lpComp &&
            (lpCompBuf = InternalSCS_RECONVERTSTRING((LPRECONVERTSTRING)lpComp, dwCompLen,
                                (LPRECONVERTSTRING *)&lpNewComp, &dwNewCompLen,
                                fAnsi, IMECodePage(pImeDpi))) == NULL)
            goto callime_scs;
        if (lpRead &&
            (lpReadBuf = InternalSCS_RECONVERTSTRING((LPRECONVERTSTRING)lpRead, dwReadLen,
                                (LPRECONVERTSTRING *)&lpNewRead, &dwNewReadLen,
                                fAnsi, IMECodePage(pImeDpi))) == NULL)
            goto callime_scs;

        fCheckSize = FALSE;
        break;

    default:
        goto callime_scs;
    }

    if (lpCompBuf != NULL) {
        lpComp    = lpNewComp;
        dwCompLen = dwNewCompLen;
    }

    if (lpReadBuf != NULL) {
        lpRead    = lpNewRead;
        dwReadLen = dwNewReadLen;
    }

start_scs:

    if (CompositionString(hImc, &pInputContext, &pCompStr, fCheckSize) <= 0)
        goto callime_scs;

    switch (dwIndex)
    {
    case SCS_SETSTR:
        fRet = TRUE;
        break;

    case SCS_CHANGEATTR:
        if ( lpComp &&
            !CheckAttribute((LPBYTE)lpComp, dwCompLen,
                    (LPBYTE)((LPBYTE)pCompStr + pCompStr->dwCompAttrOffset),
                    pCompStr->dwCompAttrLen,
                    (LPDWORD)((LPBYTE)pCompStr + pCompStr->dwCompClauseOffset),
                    pCompStr->dwCompClauseLen)) break;

        if ( lpRead &&
            !CheckAttribute((LPBYTE)lpRead, dwReadLen,
                    (LPBYTE)((LPBYTE)pCompStr + pCompStr->dwCompReadAttrOffset),
                    pCompStr->dwCompReadAttrLen,
                    (LPDWORD)((LPBYTE)pCompStr + pCompStr->dwCompReadClauseOffset),
                    pCompStr->dwCompReadClauseLen)) break;
        fRet = TRUE;
        break;

    case SCS_CHANGECLAUSE:
        if ( lpComp &&
            !CheckClause((LPDWORD)lpComp, dwCompLen,
                         (LPDWORD)((LPBYTE)pCompStr + pCompStr->dwCompClauseOffset),
                         pCompStr->dwCompClauseLen)) break;
        if ( lpRead &&
            !CheckClause((LPDWORD)lpRead, dwReadLen,
                         (LPDWORD)((LPBYTE)pCompStr + pCompStr->dwCompReadClauseOffset),
                         pCompStr->dwCompReadClauseLen)) break;
        fRet = TRUE;
        break;

    case SCS_SETRECONVERTSTRING:
    case SCS_QUERYRECONVERTSTRING:
        if (pImeDpi->ImeInfo.fdwSCSCaps & SCS_CAP_SETRECONVERTSTRING) {
            fRet = TRUE;
        }
        break;

    default:
        break;
    }

    ImmUnlockIMCC(pInputContext->hCompStr);
    ImmUnlockIMC(hImc);

callime_scs:

    if (fRet) {
        fRet = (*pImeDpi->pfn.ImeSetCompositionString)(hImc, dwIndex,
                                    lpComp, dwCompLen, lpRead, dwReadLen);
    }

    /*
     * Check if we need ANSI/Unicode back conversion
     */
    if (fNeedAWConversion) {
        LPBYTE lpCompBufBack = NULL, lpReadBufBack = NULL;
        /*
         * A/W back conversion needed.
         */
        switch (dwIndex) {
        case SCS_QUERYRECONVERTSTRING:
            if (lpOrgComp &&
                (lpCompBufBack = InternalSCS_RECONVERTSTRING((LPRECONVERTSTRING)lpComp, dwCompLen,
                                    (LPRECONVERTSTRING *)&lpNewComp, &dwNewCompLen,
                                    !fAnsi, IMECodePage(pImeDpi)))) {
                RtlCopyMemory(lpOrgComp, lpNewComp, dwNewCompLen);
            }
            if (lpOrgRead &&
                (lpReadBufBack = InternalSCS_RECONVERTSTRING(
                                    (LPRECONVERTSTRING)lpRead, dwReadLen,
                                    (LPRECONVERTSTRING *)&lpNewRead, &dwNewReadLen,
                                    !fAnsi, IMECodePage(pImeDpi)))) {
                RtlCopyMemory(lpOrgRead, lpNewRead, dwNewReadLen);
            }
        }
        if (lpCompBufBack != NULL)
            LocalFree(lpCompBufBack);
        if (lpReadBufBack != NULL)
            LocalFree(lpReadBufBack);
    }

    if (lpCompBuf != NULL)
        ImmLocalFree(lpCompBuf);
    if (lpReadBuf != NULL)
        ImmLocalFree(lpReadBuf);

    ImmUnlockImeDpi(pImeDpi);

    return fRet;
}


/***************************************************************************\
* ImmGetCandidateListCountA
*
* Query the byte count and list count to receive all candidate list.
*
* History:
* 27-Feb-1995   wkwok   Created
\***************************************************************************/

DWORD WINAPI ImmGetCandidateListCountA(
    HIMC    hImc,
    LPDWORD lpdwListCount)      // the buffer pointer for list count
{
    return ImmGetCandidateListCountWorker(hImc, lpdwListCount, TRUE);
}


/***************************************************************************\
* ImmGetCandidateListCountW
*
* Query the byte count and list count to receive all candidate list.
*
* History:
* 27-Feb-1995   wkwok   Created
\***************************************************************************/

DWORD WINAPI ImmGetCandidateListCountW(
    HIMC    hImc,
    LPDWORD lpdwListCount)      // the buffer pointer for list count
{
    return ImmGetCandidateListCountWorker(hImc, lpdwListCount, FALSE);
}


/***************************************************************************\
* ImmGetCandidateListCountWorker
*
* Worker function of ImmGetCandidateListCountA/ImmGetCandidateListCountW.
*
* History:
* 27-Feb-1995   wkwok   Created
\***************************************************************************/

DWORD ImmGetCandidateListCountWorker(
    HIMC    hImc,
    LPDWORD lpdwListCount,
    BOOL    fAnsi)
{
    PCLIENTIMC      pClientImc;
    PINPUTCONTEXT   pInputContext;
    LPCANDIDATEINFO lpCandInfo;
    DWORD           dwRet = 0;
    INT             i;
    DWORD           dwCodePage;

    if (lpdwListCount) {
        *lpdwListCount = 0;
    } else {
        RIPMSG0(RIP_WARNING, "ImmGetCandidateListCount: NULL lpdwListCount.");
        return dwRet;
    }

    pClientImc = ImmLockClientImc(hImc);
    if (pClientImc == NULL) {
        RIPMSG1(RIP_WARNING, "ImmGetCandidateListCount: Invalid hImc %lx.", hImc);
        goto GetCandListCntExit;
    }
    dwCodePage = CImcCodePage(pClientImc);

    pInputContext = ImmLockIMC(hImc);
    if (pInputContext == NULL) {
        RIPMSG1(RIP_WARNING, "ImmGetCandidateListCount: Lock hImc %lx failed.", hImc);
        goto GetCandListCntUnlockClientImc;
    }

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(pInputContext->hCandInfo);
    if (!lpCandInfo) {
        RIPMSG1(RIP_WARNING,
              "ImmGetCandidateListCount: Lock hCandInfo %x failed.",
              pInputContext->hCandInfo);
        goto GetCandListCntUnlockIMC;
    }

    if (lpCandInfo->dwSize < sizeof(CANDIDATEINFO)) {
        RIPMSG0(RIP_WARNING, "ImmGetCandidateListCount: no candidate list.");
        goto GetCandListCntUnlockIMC;
    }

    *lpdwListCount = lpCandInfo->dwCount;

    if (fAnsi && TestICF(pClientImc, IMCF_UNICODE)) {
        LPCANDIDATELIST lpCandListW;

        dwRet = DWORD_ALIGN(sizeof(CANDIDATEINFO))
              + DWORD_ALIGN(lpCandInfo->dwPrivateSize);

        for (i = 0; i < (INT)lpCandInfo->dwCount; i++) {
            lpCandListW = (LPCANDIDATELIST)((LPBYTE)lpCandInfo + lpCandInfo->dwOffset[i]);
            dwRet += InternalGetCandidateListWtoA(lpCandListW, NULL, 0, dwCodePage);
        }
    }
    else if (!fAnsi && !TestICF(pClientImc, IMCF_UNICODE)) {
        LPCANDIDATELIST lpCandListA;

        dwRet = DWORD_ALIGN(sizeof(CANDIDATEINFO))
              + DWORD_ALIGN(lpCandInfo->dwPrivateSize);

        for (i = 0; i < (INT)lpCandInfo->dwCount; i++) {
            lpCandListA = (LPCANDIDATELIST)((LPBYTE)lpCandInfo + lpCandInfo->dwOffset[i]);
            dwRet += InternalGetCandidateListAtoW(lpCandListA, NULL, 0, dwCodePage);
        }
    }
    else {
        dwRet = lpCandInfo->dwSize;
    }

    ImmUnlockIMCC(pInputContext->hCandInfo);

GetCandListCntUnlockIMC:
    ImmUnlockIMC(hImc);

GetCandListCntUnlockClientImc:
    ImmUnlockClientImc(pClientImc);

GetCandListCntExit:
    return dwRet;
}


/***************************************************************************\
* ImmGetCandidateListA
*
* Gets the candidate list information specified by dwIndex.
*
* History:
* 27-Feb-1995   wkwok   Created
\***************************************************************************/

DWORD WINAPI ImmGetCandidateListA(
    HIMC            hImc,
    DWORD           dwIndex,
    LPCANDIDATELIST lpCandList,
    DWORD           dwBufLen)
{
    return ImmGetCandidateListWorker(hImc, dwIndex,
                                     lpCandList, dwBufLen, TRUE);
}


/***************************************************************************\
* ImmGetCandidateListW
*
* Gets the candidate list information specified by dwIndex.
*
* History:
* 27-Feb-1995   wkwok   Created
\***************************************************************************/

DWORD WINAPI ImmGetCandidateListW(
    HIMC            hImc,
    DWORD           dwIndex,
    LPCANDIDATELIST lpCandList,
    DWORD           dwBufLen)
{
    return ImmGetCandidateListWorker(hImc, dwIndex,
                                     lpCandList, dwBufLen, FALSE);
}


/***************************************************************************\
* ImmGetCandidateListWorker
*
* Worker function of ImmGetCandidateListA/ImmGetCandidateListW.
*
* History:
* 27-Feb-1995   wkwok   Created
\***************************************************************************/

DWORD ImmGetCandidateListWorker(
    HIMC            hImc,
    DWORD           dwIndex,
    LPCANDIDATELIST lpCandList,
    DWORD           dwBufLen,
    BOOL            fAnsi)
{
    PCLIENTIMC      pClientImc;
    PINPUTCONTEXT   pInputContext;
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandListTemp;
    DWORD           dwBufLenTemp;
    DWORD           dwRet = 0;
    DWORD           dwCodePage;

    pClientImc = ImmLockClientImc(hImc);
    if (pClientImc == NULL) {
        RIPMSG1(RIP_WARNING, "ImmGetCandidateList: Invalid hImc %lx.", hImc);
        goto GetCandListExit;

    }

    dwCodePage = CImcCodePage(pClientImc);

    pInputContext = ImmLockIMC(hImc);
    if (pInputContext == NULL) {
        RIPMSG1(RIP_WARNING, "ImmGetCandidateList: Lock hImc %lx failed.", hImc);
        goto GetCandListUnlockClientImc;
    }

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(pInputContext->hCandInfo);
    if (!lpCandInfo) {
        RIPMSG1(RIP_WARNING, "ImmGetCandidateList: Lock hCandInfo %x failed",
              pInputContext->hCandInfo);
        goto GetCandListUnlockIMC;
    }

    if (lpCandInfo->dwSize < sizeof(CANDIDATEINFO)) {
        RIPMSG0(RIP_WARNING, "ImmGetCandidateList: no candidate list.");
        goto GetCandListUnlockIMCC;
    }

    /*
     * invalid access
     */
    if (dwIndex >= lpCandInfo->dwCount) {
        RIPMSG0(RIP_WARNING, "ImmGetCandidateList: dwIndex >= lpCandInfo->dwCount.");
        goto GetCandListUnlockIMCC;
    }

    lpCandListTemp = (LPCANDIDATELIST)((LPBYTE)lpCandInfo + lpCandInfo->dwOffset[dwIndex]);

    if (fAnsi && TestICF(pClientImc, IMCF_UNICODE)) {
        /*
         * ANSI Caller with an Unicode hImc.
         */
        dwBufLenTemp = InternalGetCandidateListWtoA(lpCandListTemp, NULL, 0, dwCodePage);
    }
    else if (!fAnsi && !TestICF(pClientImc, IMCF_UNICODE)) {
        /*
         * Unicode Caller with an ANSI hImc.
         */
        dwBufLenTemp = InternalGetCandidateListAtoW(lpCandListTemp, NULL, 0, dwCodePage);
    }
    else {
        /*
         * No conversion required.
         */
        dwBufLenTemp = lpCandListTemp->dwSize;
    }

    /*
     * Query buffer size or early exit on error
     */
    if (dwBufLen == 0 || dwBufLenTemp == 0) {
        dwRet = dwBufLenTemp;
    }
    else if (!lpCandList) {
        RIPMSG0(RIP_WARNING, "ImmGetCandidateList: Null lpCandList.");
    }
    else if (dwBufLen < dwBufLenTemp) {
        RIPMSG2(RIP_WARNING, "ImmGetCandidateList: dwBufLen = %d too small, require = %d.",
              dwBufLen, dwBufLenTemp);
    } else {
        if (fAnsi && TestICF(pClientImc, IMCF_UNICODE)) {
            dwRet = InternalGetCandidateListWtoA(lpCandListTemp, lpCandList, dwBufLenTemp, dwCodePage);
        }
        else if (!fAnsi && !TestICF(pClientImc, IMCF_UNICODE)) {
            dwRet = InternalGetCandidateListAtoW(lpCandListTemp, lpCandList, dwBufLenTemp, dwCodePage);
        }
        else {
            RtlCopyMemory((LPBYTE)lpCandList, (LPBYTE)lpCandListTemp, dwBufLenTemp);
            dwRet = dwBufLenTemp;
        }
    }

GetCandListUnlockIMCC:
    ImmUnlockIMCC(pInputContext->hCandInfo);

GetCandListUnlockIMC:
    ImmUnlockIMC(hImc);

GetCandListUnlockClientImc:
    ImmUnlockClientImc(pClientImc);

GetCandListExit:
    return dwRet;
}


/***************************************************************************\
* ImmGetGuideLineA
*
* Gets the guide line information reported by the IME.
*
* History:
* 26-Feb-1995   wkwok   Created
\***************************************************************************/

DWORD WINAPI ImmGetGuideLineA(
    HIMC    hImc,
    DWORD   dwIndex,
    LPSTR   lpszBuf,
    DWORD   dwBufLen)
{
    return ImmGetGuideLineWorker(hImc, dwIndex,
                                 (LPBYTE)lpszBuf, dwBufLen, TRUE);
}


/***************************************************************************\
* ImmGetGuideLineW
*
* Gets the guide line information reported by the IME.
*
* History:
* 26-Feb-1995   wkwok   Created
\***************************************************************************/

DWORD WINAPI ImmGetGuideLineW(
    HIMC    hImc,
    DWORD   dwIndex,
    LPWSTR  lpwszBuf,
    DWORD   dwBufLen)
{
    return ImmGetGuideLineWorker(hImc, dwIndex,
                                 (LPBYTE)lpwszBuf, dwBufLen, FALSE);
}


/***************************************************************************\
* ImmGetGuideLineWorker
*
* Worker function of ImmGetGuideLineA/ImmGetGuideLineW.
*
* History:
* 26-Feb-1995   wkwok   Created
\***************************************************************************/

DWORD ImmGetGuideLineWorker(
    HIMC    hImc,
    DWORD   dwIndex,
    LPBYTE  lpBuf,
    DWORD   dwBufLen,
    BOOL    fAnsi)
{
    PCLIENTIMC    pClientImc;
    PINPUTCONTEXT pInputContext;
    LPGUIDELINE   lpGuideLine;
    LPBYTE        lpBufTemp;
    DWORD         dwRet = 0;
    DWORD         dwBufLenNeeded;
    BOOL          bUDC;
    DWORD         dwCodePage;

    pClientImc = ImmLockClientImc(hImc);
    if (pClientImc == NULL) {
        RIPMSG1(RIP_WARNING, "ImmGetGuideLine: Invalid hImc %lx.", hImc);
        goto GetGuideLineExit;
    }
    dwCodePage = CImcCodePage(pClientImc);

    pInputContext = ImmLockIMC(hImc);
    if (pInputContext == NULL) {
        RIPMSG1(RIP_WARNING, "ImmGetGuideLine: Lock hImc %lx failed.", hImc);
        goto GetGuideLineUnlockClientImc;
    }

    lpGuideLine = (LPGUIDELINE)ImmLockIMCC(pInputContext->hGuideLine);
    if (!lpGuideLine) {
        RIPMSG1(RIP_WARNING, "ImmGetGuideLine: Lock hGuideLine %lx failed.",
              pInputContext->hGuideLine);
        goto GetGuideLineUnlockIMC;
    }

    switch (dwIndex) {
    case GGL_LEVEL:
        dwRet = lpGuideLine->dwLevel;
        break;

    case GGL_INDEX:
        dwRet = lpGuideLine->dwIndex;
        break;

    case GGL_STRING:

        lpBufTemp = (LPBYTE)lpGuideLine + lpGuideLine->dwStrOffset;

        /*
         * Calculate the required buffer length.
         */
        if (fAnsi && TestICF(pClientImc, IMCF_UNICODE)) {
            dwBufLenNeeded = WideCharToMultiByte(dwCodePage,
                                                 (DWORD)0,
                                                 (LPWSTR)lpBufTemp,
                                                 (INT)lpGuideLine->dwStrLen,
                                                 (LPSTR)NULL,
                                                 (INT)0,
                                                 (LPSTR)NULL,
                                                 (LPBOOL)&bUDC);
        }
        else if (!fAnsi && !TestICF(pClientImc, IMCF_UNICODE)) {
            dwBufLenNeeded = MultiByteToWideChar(dwCodePage,
                                                 (DWORD)MB_PRECOMPOSED,
                                                 (LPSTR)lpBufTemp,
                                                 (INT)lpGuideLine->dwStrLen,
                                                 (LPWSTR)NULL,
                                                 (INT)0);
            dwBufLenNeeded *= sizeof(WCHAR);
        }
        else {
            dwBufLenNeeded = lpGuideLine->dwStrLen;
            /*
             * The dwStrLen records the strlen and not the byte count.
             */
            if (TestICF(pClientImc, IMCF_UNICODE))
                dwBufLenNeeded *= sizeof(WCHAR);
        }

        /*
         * Query GuideLine string size only or early exit on error
         */
        if (dwBufLen == 0 || dwBufLenNeeded == 0) {
            dwRet = dwBufLenNeeded;
            goto GetGuideLineUnlockIMCC;
        }

        if (lpBuf == NULL || dwBufLen < dwBufLenNeeded)
            goto GetGuideLineUnlockIMCC;

        if (fAnsi && TestICF(pClientImc, IMCF_UNICODE)) {
            dwRet = WideCharToMultiByte(dwCodePage,
                                        (DWORD)0,
                                        (LPWSTR)lpBufTemp,
                                        (INT)lpGuideLine->dwStrLen,
                                        (LPSTR)lpBuf,
                                        (INT)dwBufLen,
                                        (LPSTR)NULL,
                                        (LPBOOL)&bUDC);
        }
        else if (!fAnsi && !TestICF(pClientImc, IMCF_UNICODE)) {
            dwRet = MultiByteToWideChar(dwCodePage,
                                        (DWORD)MB_PRECOMPOSED,
                                        (LPSTR)lpBufTemp,
                                        (INT)lpGuideLine->dwStrLen,
                                        (LPWSTR)lpBuf,
                                        (INT)dwBufLen/sizeof(WCHAR));
            dwRet *= sizeof(WCHAR);
        }
        else {
            RtlCopyMemory(lpBuf, lpBufTemp, dwBufLenNeeded);
            dwRet = dwBufLenNeeded;
        }

        break;

    case GGL_PRIVATE:

        lpBufTemp = (LPBYTE)lpGuideLine + lpGuideLine->dwPrivateOffset;

        /*
         * The dwPrivateOffset is an offset to a CANDIDATELIST when
         * lpGuideLine->dwIndex == GL_ID_REVERSECONVERSION. Do conversion
         * for this case only.
         */
        if (fAnsi && TestICF(pClientImc, IMCF_UNICODE) &&
                lpGuideLine->dwIndex == GL_ID_REVERSECONVERSION) {
            dwBufLenNeeded = InternalGetCandidateListWtoA(
                        (LPCANDIDATELIST)lpBufTemp, (LPCANDIDATELIST)NULL, 0, dwCodePage);
        }
        else if (!fAnsi && !TestICF(pClientImc, IMCF_UNICODE) &&
                lpGuideLine->dwIndex == GL_ID_REVERSECONVERSION) {
            dwBufLenNeeded = InternalGetCandidateListAtoW(
                        (LPCANDIDATELIST)lpBufTemp, (LPCANDIDATELIST)NULL, 0, dwCodePage);
        }
        else {
            dwBufLenNeeded = lpGuideLine->dwPrivateSize;
        }

        /*
         * Query dwPrivateSize size only or early exit on error
         */
        if (dwBufLen == 0 || dwBufLenNeeded == 0) {
            dwRet = dwBufLenNeeded;
            goto GetGuideLineUnlockIMCC;
        }

        if (lpBuf == NULL || dwBufLen < dwBufLenNeeded)
            goto GetGuideLineUnlockIMCC;

        if (fAnsi && TestICF(pClientImc, IMCF_UNICODE) &&
                lpGuideLine->dwIndex == GL_ID_REVERSECONVERSION) {
            dwRet = InternalGetCandidateListWtoA(
                    (LPCANDIDATELIST)lpBufTemp, (LPCANDIDATELIST)lpBuf, dwBufLenNeeded, dwCodePage);
        }
        else if (!fAnsi && !TestICF(pClientImc, IMCF_UNICODE) &&
                lpGuideLine->dwIndex == GL_ID_REVERSECONVERSION) {
            dwRet = InternalGetCandidateListAtoW(
                    (LPCANDIDATELIST)lpBufTemp, (LPCANDIDATELIST)lpBuf, dwBufLenNeeded, dwCodePage);
        }
        else {
            RtlCopyMemory(lpBuf, lpBufTemp, dwBufLenNeeded);
            dwRet = dwBufLenNeeded;
        }

        break;

    default:
        break;
    }

GetGuideLineUnlockIMCC:
    ImmUnlockIMCC(pInputContext->hGuideLine);

GetGuideLineUnlockIMC:
    ImmUnlockIMC(hImc);

GetGuideLineUnlockClientImc:
    ImmUnlockClientImc(pClientImc);

GetGuideLineExit:
    return dwRet;
}


/***************************************************************************\
* ImmGetConversionStatus
*
* Gets current conversion status.
*
* History:
* 26-Feb-1995   wkwok   Created
\***************************************************************************/

BOOL WINAPI ImmGetConversionStatus(     // Get the conversion status
    HIMC    hImc,
    LPDWORD lpfdwConversion,
    LPDWORD lpfdwSentence)
{
    PINPUTCONTEXT pInputContext;

    pInputContext = ImmLockIMC(hImc);
    if (pInputContext == NULL) {
        RIPMSG1(RIP_WARNING, "ImmGetConversionStatus: Lock hImc %lx failed", hImc);
        return FALSE;
    }

    if (lpfdwConversion != NULL)
        *lpfdwConversion = pInputContext->fdwConversion;

    if (lpfdwSentence != NULL)
        *lpfdwSentence = pInputContext->fdwSentence;

    ImmUnlockIMC(hImc);

    return TRUE;
}


/***************************************************************************\
* ImmSetConversionStatus
*
* Sets current conversion status.
*
* History:
* 26-Feb-1995   wkwok   Created
\***************************************************************************/

BOOL WINAPI ImmSetConversionStatus(
    HIMC  hImc,
    DWORD fdwConversion,
    DWORD fdwSentence)
{
    PINPUTCONTEXT pInputContext;
    DWORD         fdwOldConversion;
    DWORD         fdwOldSentence;
    BOOL          fConvModeChg;
    BOOL          fSentenceChg;
    HWND          hWnd;
    DWORD         dwOpenStatus;
    DWORD         dwConversion;

    if (GetInputContextThread(hImc) != GetCurrentThreadId()) {
        RIPMSG1(RIP_WARNING,
              "ImmSetConversionStatus: Invalid input context access %lx.", hImc);
        return FALSE;
    }

    pInputContext = ImmLockIMC(hImc);
    if (pInputContext == NULL) {
        RIPMSG1(RIP_WARNING,
              "ImmSetConversionStatus: Lock hImc %lx failed", hImc);
        return FALSE;
    }

    fConvModeChg = FALSE;
    fSentenceChg = FALSE;

    if (pInputContext->fdwConversion != fdwConversion) {
        if ((fdwConversion & IME_CMODE_LANGUAGE) == IME_CMODE_KATAKANA) {
            RIPMSG0(RIP_WARNING, "ImmSetConversionStatus: wrong fdwConversion");
        }
        fdwOldConversion = pInputContext->fdwConversion;
        pInputContext->fdwConversion = fdwConversion;
        fConvModeChg = TRUE;
    }

    if (pInputContext->fdwSentence != fdwSentence) {
        fdwOldSentence = pInputContext->fdwSentence;
        pInputContext->fdwSentence = fdwSentence;
        fSentenceChg = TRUE;
    }

    hWnd = pInputContext->hWnd;
    if ( fConvModeChg ) {

        dwOpenStatus = (DWORD)pInputContext->fOpen;
        dwConversion = pInputContext->fdwConversion;
    }

    ImmUnlockIMC(hImc);

#ifdef LATER
    // Do uNumLangVKey and uNumVKey checking later.
#endif

    /*
     * inform IME and UI about the conversion mode changes.
     */
    if (fConvModeChg) {
        MakeIMENotify(hImc, hWnd, NI_CONTEXTUPDATED, fdwOldConversion,
                IMC_SETCONVERSIONMODE, IMN_SETCONVERSIONMODE, 0L);

        /*
         * notify shell and keyboard the conversion mode change
         */
        NtUserNotifyIMEStatus( hWnd, dwOpenStatus, dwConversion );
    }

    /*
     * inform IME and UI about the sentence mode changes.
     */
    if (fSentenceChg) {
        MakeIMENotify(hImc, hWnd, NI_CONTEXTUPDATED, fdwOldSentence,
                IMC_SETSENTENCEMODE, IMN_SETSENTENCEMODE, 0L);
    }

    return TRUE;
}


/***************************************************************************\
* ImmGetOpenStatus
*
* Gets the open or close status of the IME.
*
* History:
* 26-Feb-1995   wkwok   Created
\***************************************************************************/

BOOL WINAPI ImmGetOpenStatus(
    HIMC hImc)
{
    PINPUTCONTEXT pInputContext;
    BOOL          fOpen;

    if (hImc == NULL_HIMC)
        return FALSE;

    pInputContext = ImmLockIMC(hImc);
    if (!pInputContext) {
        RIPMSG1(RIP_WARNING, "ImmGetOpenStatus: Lock hImc %lx failed", hImc);
        return FALSE;
    }

    fOpen = pInputContext->fOpen;
    ImmUnlockIMC(hImc);

    return (fOpen);
}


/***************************************************************************\
* ImmSetOpenStatus
*
* Opens or closes the IME.
*
* History:
* 26-Feb-1995   wkwok   Created
\***************************************************************************/

BOOL WINAPI ImmSetOpenStatus(
    HIMC hImc,
    BOOL fOpen)
{
    PINPUTCONTEXT pInputContext;
    HWND          hWnd;
    DWORD         dwOpenStatus;
    DWORD         dwConversion;
    BOOL          fOpenChg = FALSE;

    if (GetInputContextThread(hImc) != GetCurrentThreadId()) {
        RIPMSG1(RIP_WARNING,
              "ImmSetOpenStatus: Invalid input context access %lx.", hImc);
        return FALSE;
    }

    pInputContext = ImmLockIMC(hImc);
    if (!pInputContext) {
        RIPMSG1(RIP_WARNING, "ImmSetOpenStatus: Lock hImc %lx failed", hImc);
        return FALSE;
    }

    if (pInputContext->fOpen != fOpen) {
        fOpenChg = TRUE;
        pInputContext->fOpen = fOpen;
    }

    if ( fOpenChg ) {
        hWnd = (HWND)pInputContext->hWnd;
        dwOpenStatus = (DWORD)pInputContext->fOpen;
        dwConversion = (DWORD)pInputContext->fdwConversion;
    }

    ImmUnlockIMC(hImc);

    /*
     * inform IME and UI about the conversion mode changes.
     */
    if (fOpenChg) {
        MakeIMENotify(hImc, hWnd, NI_CONTEXTUPDATED, (DWORD)0,
                IMC_SETOPENSTATUS, IMN_SETOPENSTATUS, 0L);

        NtUserNotifyIMEStatus( hWnd, dwOpenStatus, dwConversion );
    }

    return TRUE;
}


/***************************************************************************\
* ImmGetCompositionFontA
*
* Opens or closes the IME.
*
* History:
* 27-Feb-1995   wkwok   Created
\***************************************************************************/

BOOL WINAPI ImmGetCompositionFontA(
    HIMC       hImc,
    LPLOGFONTA lpLogFontA)
{
    PCLIENTIMC    pClientImc;
    PINPUTCONTEXT pInputContext;
    LOGFONTW      LogFontW;
    BOOL          fUnicode, fRet;

    pClientImc = ImmLockClientImc(hImc);
    if (pClientImc == NULL) {
        RIPMSG1(RIP_WARNING, "ImmGetCompositionFontA: Invalid hImc %lx.", hImc);
        return FALSE;
    }

    fUnicode = TestICF(pClientImc, IMCF_UNICODE);

    ImmUnlockClientImc(pClientImc);

    pInputContext = ImmLockIMC(hImc);
    if (pInputContext == NULL) {
        RIPMSG1(RIP_WARNING, "ImmGetCompositionFontA: Lock hImc %lx failed.", hImc);
        return FALSE;
    }

    if (fUnicode) {

        ImmUnlockIMC(hImc);

        if (ImmGetCompositionFontW(hImc, &LogFontW)) {
            LFontWtoLFontA(&LogFontW, lpLogFontA);
            return (TRUE);
        }

        return FALSE;
    }

    if ((pInputContext->fdwInit & INIT_LOGFONT) == INIT_LOGFONT) {
        *lpLogFontA = pInputContext->lfFont.A;
        fRet = TRUE;
    }
    else {
        fRet = FALSE;
    }

    ImmUnlockIMC(hImc);

    return fRet;
}


/***************************************************************************\
* ImmGetCompositionFontW
*
* Opens or closes the IME.
*
* History:
* 27-Feb-1995   wkwok   Created
\***************************************************************************/

BOOL WINAPI ImmGetCompositionFontW(
    HIMC       hImc,
    LPLOGFONTW lpLogFontW)
{
    PCLIENTIMC    pClientImc;
    PINPUTCONTEXT pInputContext;
    LOGFONTA      LogFontA;
    BOOL          fUnicode, fRet;

    pClientImc = ImmLockClientImc(hImc);
    if (pClientImc == NULL) {
        RIPMSG1(RIP_WARNING, "ImmGetCompositionFontW: Invalid hImc %lx.", hImc);
        return FALSE;
    }

    fUnicode = TestICF(pClientImc, IMCF_UNICODE);

    ImmUnlockClientImc(pClientImc);

    pInputContext = ImmLockIMC(hImc);
    if (!pInputContext) {
        RIPMSG1(RIP_WARNING, "ImmGetCompositionFontW: Lock hImc %lx failed.", hImc);
        return (FALSE);
    }

    if (!fUnicode) {

        ImmUnlockIMC(hImc);

        if (ImmGetCompositionFontA(hImc, &LogFontA)) {
            LFontAtoLFontW(&LogFontA, lpLogFontW);
            return (TRUE);
        }

        return FALSE;
    }

    if ((pInputContext->fdwInit & INIT_LOGFONT) == INIT_LOGFONT) {
        *lpLogFontW = pInputContext->lfFont.W;
        fRet = TRUE;
    }
    else {
        fRet = FALSE;
    }

    ImmUnlockIMC(hImc);

    return fRet;
}


BOOL WINAPI ImmSetCompositionFontA(
    HIMC       hImc,
    LPLOGFONTA lpLogFontA)
{
    PCLIENTIMC    pClientImc;
    PINPUTCONTEXT pInputContext;
    LOGFONTW      LogFontW;
    HWND          hWnd;
    BOOL          fUnicode;

    if (GetInputContextThread(hImc) != GetCurrentThreadId()) {
        RIPMSG1(RIP_WARNING,
              "ImmSetCompositionFontA: Invalid input context access %lx.", hImc);
        return FALSE;
    }

    pClientImc = ImmLockClientImc(hImc);
    if (pClientImc == NULL) {
        RIPMSG1(RIP_WARNING, "ImmSetCompositionFontA: Invalid hImc %lx.", hImc);
        return FALSE;
    }

    fUnicode = TestICF(pClientImc, IMCF_UNICODE);

    ImmUnlockClientImc(pClientImc);

    pInputContext = ImmLockIMC(hImc);
    if (!pInputContext) {
        RIPMSG1(RIP_WARNING, "ImmSetCompositionFontA: Lock hImc %lx failed.", hImc);
        return (FALSE);
    }

    if (fUnicode) {

        ImmUnlockIMC(hImc);

        LFontAtoLFontW(lpLogFontA, &LogFontW);

        return ImmSetCompositionFontW(hImc, &LogFontW);
    }

    /*
     * Japanese 3.x applications need to receive 3.x compatible notification message.
     *
     */
    if ( (GetClientInfo()->dwExpWinVer < VER40) &&
         (PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID())) == LANG_JAPANESE)   &&
         ! (pInputContext->fdw31Compat & F31COMPAT_MCWHIDDEN) &&
         (pInputContext->cfCompForm.dwStyle != CFS_DEFAULT) ) {

        PostMessageA( pInputContext->hWnd, WM_IME_REPORT, IR_CHANGECONVERT, (LPARAM)NULL);
    }

    pInputContext->lfFont.A = *lpLogFontA;
    pInputContext->fdwInit |= INIT_LOGFONT;
    hWnd = pInputContext->hWnd;

    ImmUnlockIMC(hImc);

    /*
     * inform IME and UI about the change of composition font.
     */
    MakeIMENotify(hImc, hWnd, NI_CONTEXTUPDATED, 0L,
            IMC_SETCOMPOSITIONFONT, IMN_SETCOMPOSITIONFONT, 0L);


    return TRUE;
}


BOOL WINAPI ImmSetCompositionFontW(
    HIMC       hImc,
    LPLOGFONTW lpLogFontW)
{
    PCLIENTIMC    pClientImc;
    PINPUTCONTEXT pInputContext;
    LOGFONTA      LogFontA;
    HWND          hWnd;
    BOOL          fUnicode;

    if (GetInputContextThread(hImc) != GetCurrentThreadId()) {
        RIPMSG1(RIP_WARNING,
              "ImmSetCompositionFontW: Invalid input context access %lx.", hImc);
        return FALSE;
    }

    pClientImc = ImmLockClientImc(hImc);
    if (pClientImc == NULL) {
        RIPMSG1(RIP_WARNING, "ImmSetCompositionFontW: Invalid hImc %lx.", hImc);
        return (FALSE);
    }

    fUnicode = TestICF(pClientImc, IMCF_UNICODE);

    ImmUnlockClientImc(pClientImc);

    pInputContext = ImmLockIMC(hImc);
    if (!pInputContext) {
        RIPMSG1(RIP_WARNING, "ImmSetCompositionFontW: Lock hImc %lx failed.", hImc);
        return (FALSE);
    }

    if (!fUnicode) {

        ImmUnlockIMC(hImc);

        LFontWtoLFontA(lpLogFontW, &LogFontA);

        return ImmSetCompositionFontA(hImc, &LogFontA);
    }

    /*
     * Japanese 3.x applications need to receive 3.x compatible notification message.
     *
     */
    if ( (GetClientInfo()->dwExpWinVer < VER40) &&
         (PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID())) == LANG_JAPANESE)   &&
         ! (pInputContext->fdw31Compat & F31COMPAT_MCWHIDDEN) &&
         (pInputContext->cfCompForm.dwStyle != CFS_DEFAULT) ) {

        PostMessageW( pInputContext->hWnd, WM_IME_REPORT, IR_CHANGECONVERT, (LPARAM)NULL);
    }
    pInputContext->lfFont.W = *lpLogFontW;
    pInputContext->fdwInit |= INIT_LOGFONT;
    hWnd = pInputContext->hWnd;

    ImmUnlockIMC(hImc);

    /*
     * inform IME and UI about the change of composition font.
     */
    MakeIMENotify(hImc, hWnd, NI_CONTEXTUPDATED, 0L,
            IMC_SETCOMPOSITIONFONT, IMN_SETCOMPOSITIONFONT, 0L);

    return TRUE;
}


/***************************************************************************\
* ImmGetConversionListA
*
* Obtains the list of FE character or word from one character or word.
*
* History:
* 27-Feb-1995   wkwok   Created
\***************************************************************************/

DWORD WINAPI ImmGetConversionListA(
    HKL             hKL,
    HIMC            hImc,
    LPCSTR          lpszSrc,
    LPCANDIDATELIST lpCandListA,
    DWORD           dwBufLen,
    UINT            uFlag)
{
    PIMEDPI pImeDpi;
    DWORD   dwRet;
    LPWSTR  lpwszSrc;
    DWORD   dwBufTemp;
    LPCANDIDATELIST lpCandListW;
    INT     i;
    DWORD   dwCodePage;

    pImeDpi = FindOrLoadImeDpi(hKL);

    if (pImeDpi == NULL) {
        RIPMSG1(RIP_WARNING,
              "ImmGetConversionListA: cannot find DPI entry for hkl=%lx", hKL);
        return (0);
    }

    dwCodePage = IMECodePage(pImeDpi);

    if (!(pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE)) {
        /*
         * This is an ANSI call to an ANSI IME.
         */
        dwRet = (*pImeDpi->pfn.ImeConversionList.a)(hImc, lpszSrc,
                                        lpCandListA, dwBufLen, uFlag);
        ImmUnlockImeDpi(pImeDpi);
        return dwRet;
    }

    ImmUnlockImeDpi(pImeDpi);

    /*
     * This is an ANSI call to an Unicode IME.
     */
    if (lpszSrc != NULL) {

        dwBufTemp = (strlen(lpszSrc) + 1) * sizeof(WCHAR);

        lpwszSrc = ImmLocalAlloc(0, dwBufTemp);
        if (lpwszSrc == NULL)
            return (0);

        i = MultiByteToWideChar(dwCodePage,
                                (DWORD)MB_PRECOMPOSED,
                                (LPSTR)lpszSrc,              // src
                                (INT)strlen(lpszSrc),
                                (LPWSTR)lpwszSrc,            // dest
                                (INT)dwBufTemp/sizeof(WCHAR));

        lpwszSrc[i] = '\0';
    }
    else {
        lpwszSrc = NULL;
    }

    /*
     * Query the CandidateListW size required.
     */
    dwBufTemp = ImmGetConversionListW(hKL, hImc, lpwszSrc, NULL, 0, uFlag);

    if (dwBufTemp == 0 || (lpCandListW = ImmLocalAlloc(0, dwBufTemp)) == NULL) {
        if (lpwszSrc)
            ImmLocalFree(lpwszSrc);
        return (0);
    }

    /*
     * Now get the actual CandidateListW.
     */
    dwBufTemp = ImmGetConversionListW(hKL, hImc, lpwszSrc,
                                        lpCandListW, dwBufTemp, uFlag);

    /*
     * Query the CandidateListA size required.
     */
    if (dwBufTemp != 0) {
        dwBufTemp = InternalGetCandidateListWtoA(lpCandListW, NULL, 0, dwCodePage);
    }

    if (dwBufLen == 0 || dwBufTemp == 0) {
        /*
         * Query required buffer size or error has happened.
         */
        dwRet = dwBufTemp;
    }
    else if (dwBufLen < dwBufTemp) {
        /*
         * Not enough buffer area.
         */
        dwRet = 0;
    }
    else {
        /*
         * Get the actual CandidateListA
         */
        dwRet = InternalGetCandidateListWtoA(lpCandListW, lpCandListA, dwBufLen, dwCodePage);
    }

    if (lpwszSrc)
        ImmLocalFree(lpwszSrc);
    ImmLocalFree(lpCandListW);

    return dwRet;

}


/***************************************************************************\
* ImmGetConversionListW
*
* Obtains the list of FE character or word from one character or word.
*
* History:
* 27-Feb-1995   wkwok   Created
\***************************************************************************/

DWORD WINAPI ImmGetConversionListW(
    HKL             hKL,
    HIMC            hImc,
    LPCWSTR         lpwszSrc,
    LPCANDIDATELIST lpCandListW,
    DWORD           dwBufLen,
    UINT            uFlag)
{
    PIMEDPI pImeDpi;
    DWORD   dwRet;
    LPSTR   lpszSrc;
    DWORD   dwBufTemp;
    LPCANDIDATELIST lpCandListA;
    BOOL    bUDC;
    INT     i;
    DWORD   dwCodePage;

    pImeDpi = FindOrLoadImeDpi(hKL);

    if (pImeDpi == NULL) {
        RIPMSG1(RIP_WARNING,
              "ImmGetConversionListW: cannot find DPI entry for hkl=%lx", hKL);
        return (0);
    }

    dwCodePage = IMECodePage(pImeDpi);

    if (pImeDpi->ImeInfo.fdwProperty & IME_PROP_UNICODE) {
        /*
         * This is an Unicode call to an Unicode IME.
         */
        dwRet = (*pImeDpi->pfn.ImeConversionList.w)(hImc, lpwszSrc,
                                        lpCandListW, dwBufLen, uFlag);
        ImmUnlockImeDpi(pImeDpi);
        return dwRet;
    }

    ImmUnlockImeDpi(pImeDpi);

    /*
     * This is an Unicode call to an ANSI IME.
     */
    if (lpwszSrc != NULL) {

        dwBufTemp = (wcslen(lpwszSrc) + 1) * sizeof(WCHAR);

        lpszSrc = ImmLocalAlloc(0, dwBufTemp);
        if (lpszSrc == NULL)
            return (0);

        i = WideCharToMultiByte(dwCodePage,
                                (DWORD)0,
                                lpwszSrc,
                                (INT)wcslen(lpwszSrc),
                                (LPSTR)lpszSrc,
                                (INT)dwBufTemp,
                                (LPSTR)NULL,
                                (LPBOOL)&bUDC);

        lpszSrc[i] = '\0';
    }
    else {
        lpszSrc = NULL;
    }

    /*
     * Query the CandidateListA size required.
     */
    dwBufTemp = ImmGetConversionListA(hKL, hImc, lpszSrc, NULL, 0, uFlag);

    if (dwBufTemp == 0 || (lpCandListA = ImmLocalAlloc(0, dwBufTemp)) == NULL) {
        if (lpszSrc)
            ImmLocalFree(lpszSrc);

        return (0);
    }

    /*
     * Now get the actual CandidateListA.
     */
    dwBufTemp = ImmGetConversionListA(hKL, hImc, lpszSrc,
                                        lpCandListA, dwBufTemp, uFlag);

    /*
     * Query the CandidateListW size required.
     */
    if (dwBufTemp != 0) {
        dwBufTemp = InternalGetCandidateListAtoW(lpCandListA, NULL, 0, dwCodePage);
    }

    if (dwBufLen == 0 || dwBufTemp == 0) {
        /*
         * Query required buffer size or error has happened.
         */
        dwRet = dwBufTemp;
    }
    else if (dwBufLen < dwBufTemp) {
        /*
         * Not enough buffer area.
         */
        dwRet = 0;
    }
    else {
        /*
         * Get the actual CandidateListW
         */
        dwRet = InternalGetCandidateListAtoW(lpCandListA, lpCandListW, dwBufLen, dwCodePage);
    }

    if (lpszSrc)
        ImmLocalFree(lpszSrc);
    ImmLocalFree(lpCandListA);

    return dwRet;
}


/***************************************************************************\
* ImmGetStatusWindowPos
*
* Gets the position, in screen coordinates, of the status window.
*
* History:
* 27-Feb-1995   wkwok   Created
\***************************************************************************/

BOOL WINAPI ImmGetStatusWindowPos(
    HIMC    hImc,
    LPPOINT lpptPos)
{
    PINPUTCONTEXT pInputContext;
    BOOL fStatusWndPosInited;

    pInputContext = ImmLockIMC(hImc);
    if (!pInputContext) {
        RIPMSG1(RIP_WARNING, "ImmGetStatusWindowPos: Lock hImc %lx failed", hImc);
        return FALSE;
    }

    fStatusWndPosInited = ((pInputContext->fdwInit & INIT_STATUSWNDPOS) == INIT_STATUSWNDPOS);
    ImmUnlockIMC(hImc);

    if (fStatusWndPosInited) {
        *lpptPos = pInputContext->ptStatusWndPos;
        return TRUE;
    }

    return FALSE;
}


BOOL WINAPI ImmSetStatusWindowPos(
    HIMC     hImc,
    LPPOINT  lpptPos)
{
    PINPUTCONTEXT pInputContext;
    HWND          hWnd;

    if (GetInputContextThread(hImc) != GetCurrentThreadId()) {
        RIPMSG1(RIP_WARNING,
              "ImmSetStatusWindowPos: Invalid input context access %lx.", hImc);
        return FALSE;
    }

    pInputContext = ImmLockIMC(hImc);
    if (!pInputContext) {
        RIPMSG1(RIP_WARNING, "ImmSetStatusWindowPos: Lock hImc %lx failed", hImc);
        return (FALSE);
    }

    pInputContext->ptStatusWndPos = *lpptPos;
    pInputContext->fdwInit |= INIT_STATUSWNDPOS;

    hWnd = pInputContext->hWnd;

    ImmUnlockIMC(hImc);

    /*
     * inform IME and UI about the change of composition font.
     */
    MakeIMENotify(hImc, hWnd, NI_CONTEXTUPDATED, 0L,
            IMC_SETSTATUSWINDOWPOS, IMN_SETSTATUSWINDOWPOS, 0L);

    return TRUE;
}


/***************************************************************************\
* ImmGetCompositionWindow
*
* Gets the information of the composition window.
*
* History:
* 27-Feb-1995   wkwok   Created
\***************************************************************************/

BOOL WINAPI ImmGetCompositionWindow(
    HIMC              hImc,
    LPCOMPOSITIONFORM lpCompForm)
{
    PINPUTCONTEXT pInputContext;
    BOOL fCompFormInited;

    pInputContext = ImmLockIMC(hImc);
    if (!pInputContext) {
        RIPMSG1(RIP_WARNING, "ImmGetCompositionWindow: Lock hImc %lx failed", hImc);
        return FALSE;
    }

    fCompFormInited = ((pInputContext->fdwInit & INIT_COMPFORM) == INIT_COMPFORM);
    ImmUnlockIMC(hImc);

    if (fCompFormInited) {
        *lpCompForm = pInputContext->cfCompForm;
        return TRUE;
    }

    return FALSE;
}


BOOL WINAPI ImmSetCompositionWindow(
    HIMC              hImc,
    LPCOMPOSITIONFORM lpCompForm)
{
    PINPUTCONTEXT pInputContext;
    HWND          hWnd;

    if (GetInputContextThread(hImc) != GetCurrentThreadId()) {
        RIPMSG1(RIP_WARNING,
              "ImmSetCompositionWindow: Invalid input context access %lx.", hImc);
        return FALSE;
    }

    pInputContext = ImmLockIMC(hImc);
    if (!pInputContext) {
        RIPMSG1(RIP_WARNING, "ImmSetCompositionWindow: Lock hImc %lx failed", hImc);
        return FALSE;
    }

    pInputContext->cfCompForm = *lpCompForm;
    pInputContext->fdwInit |= INIT_COMPFORM;

    /*
     * Only WINNLS.DLL set F31COMPAT_MCWHIDDEN.
     * When the apps or edit control calls this API, we need to remove
     * F31COMPAT_MCWHIDDEN.
     */
    if (pInputContext->fdw31Compat & F31COMPAT_CALLFROMWINNLS)
       pInputContext->fdw31Compat &= ~F31COMPAT_CALLFROMWINNLS;
    else
       pInputContext->fdw31Compat &= ~F31COMPAT_MCWHIDDEN;

    hWnd = pInputContext->hWnd;

    ImmUnlockIMC(hImc);

    /*
     * inform IME and UI about the change of composition window.
     */
    MakeIMENotify(hImc, hWnd, NI_CONTEXTUPDATED, 0L,
            IMC_SETCOMPOSITIONWINDOW, IMN_SETCOMPOSITIONWINDOW, 0L);

    return TRUE;
}


/***************************************************************************\
* ImmGetCandidateWindow
*
* Gets the information of the candidate window specified by dwIndex.
*
* History:
* 27-Feb-1995   wkwok   Created
\***************************************************************************/

BOOL WINAPI ImmGetCandidateWindow(
    HIMC              hImc,
    DWORD             dwIndex,
    LPCANDIDATEFORM   lpCandForm)
{
    PINPUTCONTEXT pInputContext;

    pInputContext = ImmLockIMC(hImc);
    if (!pInputContext) {
        RIPMSG1(RIP_WARNING, "ImmGetCandidateWindow: Lock hImc %lx failed", hImc);
        return FALSE;
    }

    ImmUnlockIMC(hImc);

    if (pInputContext->cfCandForm[dwIndex].dwIndex == -1) {
        ImmUnlockIMC(hImc);
        return (FALSE);
    }

    *lpCandForm = pInputContext->cfCandForm[dwIndex];
    ImmUnlockIMC(hImc);
    return TRUE;
}


BOOL WINAPI ImmSetCandidateWindow(
    HIMC              hImc,
    LPCANDIDATEFORM   lpCandForm)
{
    PINPUTCONTEXT pInputContext;
    HWND          hWnd;

    if (lpCandForm->dwIndex >= 4)      // over flow candidate index
        return (FALSE);

    if (GetInputContextThread(hImc) != GetCurrentThreadId()) {
        RIPMSG1(RIP_WARNING,
              "ImmSetCandidateWindow: Invalid input context access %lx.", hImc);
        return FALSE;
    }

    pInputContext = ImmLockIMC(hImc);
    if (!pInputContext) {
        RIPMSG1(RIP_WARNING, "ImmSetCandidateWindow: Lock hImc %lx failed", hImc);
        return FALSE;
    }

    pInputContext->cfCandForm[lpCandForm->dwIndex] = *lpCandForm;

    hWnd = pInputContext->hWnd;

    ImmUnlockIMC(hImc);

    /*
     * inform IME and UI about the change of composition window.
     */
    MakeIMENotify(hImc, hWnd, NI_CONTEXTUPDATED, 0L, IMC_SETCANDIDATEPOS,
            IMN_SETCANDIDATEPOS, (LPARAM)(0x01 << lpCandForm->dwIndex));

    return TRUE;
}


#define GetCompInfoA(Component)                                                \
        if (!dwBufLen) {    /* query required buffer size */                   \
                            /* not include \0             */                   \
            dwBufLen = pCompStr->dw ## Component ## Len * sizeof(CHAR);        \
        } else {                                                               \
            if (dwBufLen > pCompStr->dw ## Component ## Len * sizeof(CHAR)) {  \
                dwBufLen = pCompStr->dw ## Component ## Len * sizeof(CHAR);    \
            }                                                                  \
            /* don't copy \0, maybe there is actually none */                  \
            RtlCopyMemory((LPBYTE)lpBuf, (LPBYTE)pCompStr +                    \
                pCompStr->dw ## Component ## Offset, dwBufLen);                \
        }

#define GetCompInfoW(Component)                                                \
        if (!dwBufLen) {    /* query required buffer size */                   \
                            /* not include \0             */                   \
            dwBufLen = pCompStr->dw ## Component ## Len * sizeof(WCHAR);       \
        } else {                                                               \
            if (dwBufLen > pCompStr->dw ## Component ## Len * sizeof(WCHAR)) { \
                dwBufLen = pCompStr->dw ## Component ## Len * sizeof(WCHAR);   \
            }                                                                  \
            /* don't copy \0, maybe there is actually none */                  \
            RtlCopyMemory((LPBYTE)lpBuf, (LPBYTE)pCompStr +                    \
                pCompStr->dw ## Component ## Offset, dwBufLen);                \
        }

/***************************************************************************\
* InternalGetCompositionStringA
*
* Internal version of ImmGetCompositionStringA.
*
* History:
* 28-Feb-1995   wkwok   Created
\***************************************************************************/

LONG InternalGetCompositionStringA(
    PCOMPOSITIONSTRING pCompStr,
    DWORD              dwIndex,
    LPVOID             lpBuf,
    DWORD              dwBufLen,
    BOOL               fAnsiImc,
    DWORD              dwCodePage)
{
    if (fAnsiImc) {
        /*
         * Composition string in input context is of ANSI style.
         */
        switch (dwIndex) {
        case GCS_COMPSTR:
            GetCompInfoA(CompStr);
            break;
        case GCS_COMPATTR:
            GetCompInfoA(CompAttr);
            break;
        case GCS_COMPREADSTR:
            GetCompInfoA(CompReadStr);
            break;
        case GCS_COMPREADATTR:
            GetCompInfoA(CompReadAttr);
            break;
        case GCS_COMPREADCLAUSE:
            GetCompInfoA(CompReadClause);
            break;
        case GCS_CURSORPOS:
            dwBufLen = (LONG)pCompStr->dwCursorPos;
            break;
        case GCS_DELTASTART:
            dwBufLen = (LONG)pCompStr->dwDeltaStart;
            break;
        case GCS_RESULTSTR:
            GetCompInfoA(ResultStr);
            break;
        case GCS_RESULTCLAUSE:
            GetCompInfoA(ResultClause);
            break;
        case GCS_RESULTREADSTR:
            GetCompInfoA(ResultReadStr);
            break;
        case GCS_RESULTREADCLAUSE:
            GetCompInfoA(ResultReadClause);
            break;
        case GCS_COMPCLAUSE:
            GetCompInfoA(CompClause);
            break;
        default:
            dwBufLen = (DWORD)(LONG)IMM_ERROR_GENERAL;
            break;
        }

        return (LONG)dwBufLen;
    }

    /*
     * ANSI caller, Unicode input context/composition string.
     */
    switch (dwIndex) {
    case GCS_COMPSTR:
    case GCS_COMPREADSTR:
    case GCS_RESULTSTR:
    case GCS_RESULTREADSTR:
    {
        DWORD  dwStrSize;
        LPWSTR lpStrW;
        BOOL   bUDC;

        /*
         * Get ANSI string from Unicode composition string.
         */
        dwStrSize = InternalGetCompositionStringW(pCompStr, dwIndex,
                                            NULL, 0, fAnsiImc, dwCodePage);

        lpStrW = ImmLocalAlloc(HEAP_ZERO_MEMORY, dwStrSize + sizeof(WCHAR));
        if (lpStrW == NULL) {
            RIPMSG0(RIP_WARNING, "InternalGetCompositionStringA: memory failure.");
            return (LONG)IMM_ERROR_GENERAL;
        }

        dwStrSize = InternalGetCompositionStringW(pCompStr, dwIndex,
                                            lpStrW, dwStrSize, fAnsiImc, dwCodePage);

        dwBufLen = WideCharToMultiByte(dwCodePage,
                                       (DWORD)0,
                                       lpStrW,          // src
                                       wcslen(lpStrW),
                                       (LPSTR)lpBuf,    // dest
                                       dwBufLen,
                                       (LPSTR)NULL,
                                       (LPBOOL)&bUDC);

        ImmLocalFree(lpStrW);
        break;
    }

    case GCS_COMPATTR:
    case GCS_COMPREADATTR:
    {
        DWORD dwAttrLenW, dwIndexStr, dwStrSize;
        PBYTE lpAttrA, lpAttrW;
        LPSTR lpStrA, lpStrT;
        CHAR  c;

        /*
         * Get ANSI attribute from Unicode composition attribute.
         */
        switch (dwIndex) {
        case GCS_COMPATTR:
            lpAttrW = (PBYTE)pCompStr + pCompStr->dwCompAttrOffset;
            dwAttrLenW = pCompStr->dwCompAttrLen;
            dwIndexStr = GCS_COMPSTR;
            break;
        case GCS_COMPREADATTR:
            lpAttrW = (PBYTE)pCompStr + pCompStr->dwCompReadAttrOffset;
            dwAttrLenW = pCompStr->dwCompReadAttrLen;
            dwIndexStr = GCS_COMPREADSTR;
            break;
        }

        if (dwAttrLenW == 0) {
            /*
             * No CompAttr or CompReadAttr exists, do nothing.
             */
            return 0;
        }

        dwStrSize = InternalGetCompositionStringA(pCompStr,
                                        dwIndexStr, NULL, 0, fAnsiImc, dwCodePage);

        if (dwStrSize == (DWORD)(LONG)IMM_ERROR_GENERAL) {
            RIPMSG0(RIP_WARNING, "InternalGetCompositionStringA: IMM_ERROR_GENERAL.");
            return (LONG)IMM_ERROR_GENERAL;
        }

        /*
         * Query required size or early exit on error.
         */
        if (dwBufLen == 0 || dwStrSize == 0)
            return dwStrSize;

        lpStrA = ImmLocalAlloc(HEAP_ZERO_MEMORY, dwStrSize + sizeof(CHAR));
        if (lpStrA == NULL) {
            RIPMSG0(RIP_WARNING, "InternalGetCompositionStringA: memory failure.");
            return (LONG)IMM_ERROR_GENERAL;
        }

        dwStrSize = InternalGetCompositionStringA(pCompStr,
                                        dwIndexStr, lpStrA, dwStrSize, fAnsiImc, dwCodePage);

        if (dwStrSize == (LONG)IMM_ERROR_GENERAL) {
            RIPMSG0(RIP_WARNING, "InternalGetCompositionStringA: IMM_ERROR_GENERAL.");
            ImmLocalFree(lpStrA);
            return (LONG)IMM_ERROR_GENERAL;
        }

        lpStrT = lpStrA;
        lpAttrA = (PBYTE)lpBuf;

        while ((c=*lpStrT++) != '\0' && dwBufLen != 0 && dwAttrLenW-- != 0) {
            if (IsDBCSLeadByteEx(dwCodePage, c)) {
                if (dwBufLen >= 2) {
                    *lpAttrA++ = *lpAttrW;
                    *lpAttrA++ = *lpAttrW;
                    dwBufLen--;
                }
                else {
                    *lpAttrA++ = *lpAttrW;
                }
                lpStrT++;
            }
            else {
                *lpAttrA++ = *lpAttrW;
            }
            lpAttrW++;
            dwBufLen--;
        }

        dwBufLen = (DWORD)(lpAttrA - (PBYTE)lpBuf);

        ImmLocalFree(lpStrA);
        break;
    }

    case GCS_COMPCLAUSE:
    case GCS_COMPREADCLAUSE:
    case GCS_RESULTCLAUSE:
    case GCS_RESULTREADCLAUSE:
    {
        LPWSTR  lpStrW;
        DWORD   dwClauseLen, dwBufLenA;
        LPDWORD lpdwSrc, lpdwDst;
        UINT    i;

        /*
         * Get ANSI clause from Unicode composition clause.
         */
        switch (dwIndex) {
        case GCS_COMPCLAUSE:
            lpStrW = (LPWSTR)((PBYTE)pCompStr + pCompStr->dwCompStrOffset);
            lpdwSrc = (LPDWORD)((PBYTE)pCompStr + pCompStr->dwCompClauseOffset);
            dwClauseLen = pCompStr->dwCompClauseLen;
            break;
        case GCS_COMPREADCLAUSE:
            lpStrW = (LPWSTR)((PBYTE)pCompStr + pCompStr->dwCompReadStrOffset);
            lpdwSrc = (LPDWORD)((PBYTE)pCompStr + pCompStr->dwCompReadClauseOffset);
            dwClauseLen = pCompStr->dwCompReadClauseLen;
            break;
        case GCS_RESULTCLAUSE:
            lpStrW = (LPWSTR)((PBYTE)pCompStr + pCompStr->dwResultStrOffset);
            lpdwSrc = (LPDWORD)((PBYTE)pCompStr + pCompStr->dwResultClauseOffset);
            dwClauseLen = pCompStr->dwResultClauseLen;
            break;
        case GCS_RESULTREADCLAUSE:
            lpStrW = (LPWSTR)((PBYTE)pCompStr + pCompStr->dwResultReadStrOffset);
            lpdwSrc = (LPDWORD)((PBYTE)pCompStr + pCompStr->dwResultReadClauseOffset);
            dwClauseLen = pCompStr->dwResultReadClauseLen;
            break;
        }

        /*
         * Query clause length or early exit on error.
         */
        if (dwBufLen == 0 || (LONG)dwClauseLen < 0) {
            dwBufLen = dwClauseLen;
            break;
        }

        lpdwDst = (LPDWORD)lpBuf;
        dwBufLenA = dwBufLen / sizeof(DWORD);

        for (i = 0; i < dwClauseLen / sizeof(DWORD) && dwBufLenA != 0; i++) {
            *lpdwDst++ = CalcCharacterPositionWtoA(*lpdwSrc++, lpStrW, dwCodePage);
            dwBufLenA--;
        }

        dwBufLen = i * sizeof(DWORD);
        break;
    }

    case GCS_CURSORPOS:
    case GCS_DELTASTART:
        /*
         * Get ANSI cursor/delta start position from Unicode composition string.
         */
        switch (dwIndex) {
        case GCS_CURSORPOS:
            dwBufLen = pCompStr->dwCursorPos;
            break;
        case GCS_DELTASTART:
            dwBufLen = pCompStr->dwDeltaStart;
            break;
        }

        if ((LONG)dwBufLen > 0) {
            dwBufLen = CalcCharacterPositionWtoA(dwBufLen,
                            (LPWSTR)((PBYTE)pCompStr + pCompStr->dwCompStrOffset),
                            dwCodePage);
        }
        break;

    default:
        dwBufLen = (DWORD)(LONG)IMM_ERROR_GENERAL;
    }

    return (LONG)dwBufLen;
}


/***************************************************************************\
* InternalGetCompositionStringW
*
* Internal version of ImmGetCompositionStringW.
*
* History:
* 28-Feb-1995   wkwok   Created
\***************************************************************************/

LONG InternalGetCompositionStringW(
    PCOMPOSITIONSTRING pCompStr,
    DWORD              dwIndex,
    LPVOID             lpBuf,
    DWORD              dwBufLen,
    BOOL               fAnsiImc,
    DWORD              dwCodePage)
{
    if (!fAnsiImc) {
        /*
         * Composition string in input context is of Unicode style.
         */
        switch (dwIndex) {
        case GCS_COMPSTR:
            GetCompInfoW(CompStr);
            break;
        case GCS_COMPATTR:              // ANSI-only
            GetCompInfoA(CompAttr);
            break;
        case GCS_COMPREADSTR:
            GetCompInfoW(CompReadStr);
            break;
        case GCS_COMPREADATTR:          // ANSI-only
            GetCompInfoA(CompReadAttr);
            break;
        case GCS_COMPREADCLAUSE:        // ANSI-only
            GetCompInfoA(CompReadClause);
            break;
        case GCS_CURSORPOS:
            dwBufLen = (LONG)pCompStr->dwCursorPos;
            break;
        case GCS_DELTASTART:
            dwBufLen = (LONG)pCompStr->dwDeltaStart;
            break;
        case GCS_RESULTSTR:
            GetCompInfoW(ResultStr);
            break;
        case GCS_RESULTCLAUSE:          // ANSI-only
            GetCompInfoA(ResultClause);
            break;
        case GCS_RESULTREADSTR:
            GetCompInfoW(ResultReadStr);
            break;
        case GCS_RESULTREADCLAUSE:      // ANSI-only
            GetCompInfoA(ResultReadClause);
            break;
        case GCS_COMPCLAUSE:            // ANSI-only
            GetCompInfoA(CompClause);
            break;
        default:
            dwBufLen = (DWORD)IMM_ERROR_GENERAL;
            break;
        }

        return (LONG)dwBufLen;
    }

    /*
     * Unicode caller, ANSI input context/composition string.
     */
    switch (dwIndex) {
    case GCS_COMPSTR:
    case GCS_COMPREADSTR:
    case GCS_RESULTSTR:
    case GCS_RESULTREADSTR:
    {
        DWORD  dwStrSize;
        LPSTR lpStrA;

        /*
         * Get Unicode string from ANSI composition string.
         */
        dwStrSize = InternalGetCompositionStringA(pCompStr, dwIndex,
                                            NULL, 0, fAnsiImc, dwCodePage);

        lpStrA = ImmLocalAlloc(HEAP_ZERO_MEMORY, dwStrSize + sizeof(CHAR));
        if (lpStrA == NULL) {
            RIPMSG0(RIP_WARNING, "InternalGetCompositionStringW: memory failure.");
            return (LONG)IMM_ERROR_GENERAL;
        }

        dwStrSize = InternalGetCompositionStringA(pCompStr, dwIndex,
                                            lpStrA, dwStrSize, fAnsiImc, dwCodePage);

        dwBufLen = MultiByteToWideChar(dwCodePage,
                                       (DWORD)MB_PRECOMPOSED,
                                       lpStrA,              // src
                                       strlen(lpStrA),
                                       (LPWSTR)lpBuf,        // dest
                                       (INT)dwBufLen);

        dwBufLen *= sizeof(WCHAR);     // return number of bytes required.

        ImmLocalFree(lpStrA);
        break;
    }

    case GCS_COMPATTR:
    case GCS_COMPREADATTR:
    {
        DWORD  dwAttrLenA, dwIndexStr, dwStrSize;
        PBYTE  lpAttrA, lpAttrW;
        LPWSTR lpStrW, lpStrT;
        ULONG  MultiByteSize;
        WCHAR  wc;

        /*
         * Get Unicode attribute from ANSI composition attribute.
         */
        switch (dwIndex) {
        case GCS_COMPATTR:
            lpAttrA = (PBYTE)pCompStr + pCompStr->dwCompAttrOffset;
            dwAttrLenA = pCompStr->dwCompAttrLen;
            dwIndexStr = GCS_COMPSTR;
            break;
        case GCS_COMPREADATTR:
            lpAttrA = (PBYTE)pCompStr + pCompStr->dwCompReadAttrOffset;
            dwAttrLenA = pCompStr->dwCompReadAttrLen;
            dwIndexStr = GCS_COMPREADSTR;
            break;
        }

        if (dwAttrLenA == 0) {
            /*
             * No CompAttr or CompReadAttr exists, do nothing.
             */
            return 0;
        }

        dwStrSize = InternalGetCompositionStringW(pCompStr,
                                        dwIndexStr, NULL, 0, fAnsiImc, dwCodePage);

        if (dwStrSize == (DWORD)(LONG)IMM_ERROR_GENERAL) {
            RIPMSG0(RIP_WARNING, "InternalGetCompositionStringA: IMM_ERROR_GENERAL.");
            return (LONG)IMM_ERROR_GENERAL;
        }

        /*
         * Query required size or early exit on error.
         */
        if (dwBufLen == 0 || dwStrSize == 0)
            return dwStrSize / sizeof(WCHAR);

        lpStrW = ImmLocalAlloc(HEAP_ZERO_MEMORY, dwStrSize + sizeof(WCHAR));
        if (lpStrW == NULL) {
            RIPMSG0(RIP_WARNING, "InternalGetCompositionStringW: memory failure.");
            return (LONG)IMM_ERROR_GENERAL;
        }

        dwStrSize = InternalGetCompositionStringW(pCompStr,
                                        dwIndexStr, lpStrW, dwStrSize, fAnsiImc, dwCodePage);

        if (dwStrSize == (LONG)IMM_ERROR_GENERAL) {
            RIPMSG0(RIP_WARNING, "InternalGetCompositionStringA: IMM_ERROR_GENERAL.");
            ImmLocalFree(lpStrW);
            return (LONG)IMM_ERROR_GENERAL;
        }

        lpStrT = lpStrW;
        lpAttrW = (PBYTE)lpBuf;

        while ((wc=*lpStrT++) != L'\0' && dwBufLen != 0 && dwAttrLenA-- != 0) {
            MultiByteSize = UnicodeToMultiByteSize(dwCodePage, &wc);
            if (MultiByteSize == 2 && dwAttrLenA != 0) {
                *lpAttrW++ = *lpAttrA++;
                dwAttrLenA--;
            }
            else {
                *lpAttrW++ = *lpAttrA;
            }
            lpAttrA++;
            dwBufLen--;
        }

        dwBufLen = (DWORD)(lpAttrW - (PBYTE)lpBuf);

        ImmLocalFree(lpStrW);
        break;
    }

    case GCS_COMPCLAUSE:
    case GCS_COMPREADCLAUSE:
    case GCS_RESULTCLAUSE:
    case GCS_RESULTREADCLAUSE:
    {
        LPSTR   lpStrA;
        DWORD   dwClauseLen, dwBufLenW;
        LPDWORD lpdwSrc, lpdwDst;
        UINT    i;

        /*
         * Get Unicode clause from ANSI composition clause.
         */
        switch (dwIndex) {
        case GCS_COMPCLAUSE:
            lpStrA = (LPSTR)((PBYTE)pCompStr + pCompStr->dwCompStrOffset);
            lpdwSrc = (LPDWORD)((PBYTE)pCompStr + pCompStr->dwCompClauseOffset);
            dwClauseLen = pCompStr->dwCompClauseLen;
            break;
        case GCS_COMPREADCLAUSE:
            lpStrA = (LPSTR)((PBYTE)pCompStr + pCompStr->dwCompReadStrOffset);
            lpdwSrc = (LPDWORD)((PBYTE)pCompStr + pCompStr->dwCompReadClauseOffset);
            dwClauseLen = pCompStr->dwCompReadClauseLen;
            break;
        case GCS_RESULTCLAUSE:
            lpStrA = (LPSTR)((PBYTE)pCompStr + pCompStr->dwResultStrOffset);
            lpdwSrc = (LPDWORD)((PBYTE)pCompStr + pCompStr->dwResultClauseOffset);
            dwClauseLen = pCompStr->dwResultClauseLen;
            break;
        case GCS_RESULTREADCLAUSE:
            lpStrA = (LPSTR)((PBYTE)pCompStr + pCompStr->dwResultReadStrOffset);
            lpdwSrc = (LPDWORD)((PBYTE)pCompStr + pCompStr->dwResultReadClauseOffset);
            dwClauseLen = pCompStr->dwResultReadClauseLen;
            break;
        }


        /*
         * Query clause length or early exit on error.
         */
        if (dwBufLen == 0 || (LONG)dwClauseLen < 0) {
            dwBufLen = dwClauseLen;
            break;
        }

        lpdwDst = (LPDWORD)lpBuf;
        dwBufLenW = dwBufLen / sizeof(DWORD);

        for (i = 0; i < dwClauseLen / sizeof(DWORD) && dwBufLenW != 0; i++) {
            *lpdwDst++ = CalcCharacterPositionAtoW(*lpdwSrc++, lpStrA, dwCodePage);
            dwBufLenW--;
        }

        dwBufLen = i * sizeof(DWORD);
        break;
    }

    case GCS_CURSORPOS:
    case GCS_DELTASTART:
        /*
         * Get Unicode cursor/delta start position from ANSI composition string.
         */
        switch (dwIndex) {
        case GCS_CURSORPOS:
            dwBufLen = pCompStr->dwCursorPos;
            break;
        case GCS_DELTASTART:
            dwBufLen = pCompStr->dwDeltaStart;
            break;
        }

        if ((LONG)dwBufLen > 0) {
            dwBufLen = CalcCharacterPositionAtoW(dwBufLen,
                            (LPSTR)((PBYTE)pCompStr + pCompStr->dwCompStrOffset),
                            dwCodePage);
        }
        break;

    default:
        dwBufLen = (DWORD)(LONG)IMM_ERROR_GENERAL;
    }

    return (LONG)dwBufLen;
}


DWORD InternalGetCandidateListAtoW(
    LPCANDIDATELIST     lpCandListA,
    LPCANDIDATELIST     lpCandListW,
    DWORD               dwBufLen,
    DWORD               dwCodePage)
{
    LPWSTR lpCandStrW;
    LPSTR  lpCandStrA;
    INT    i, j;
    DWORD  dwCandListLen;

    dwCandListLen = sizeof(CANDIDATELIST);

    /*
     * CANDIDATELIST has already contained the dwOffset[0]
     */
    if (lpCandListA->dwCount > 0)
        dwCandListLen += sizeof(DWORD) * (lpCandListA->dwCount - 1);

    for (i = 0; i < (INT)lpCandListA->dwCount; i++) {

        lpCandStrA = (LPSTR)((LPBYTE)lpCandListA + lpCandListA->dwOffset[i]);

        j = MultiByteToWideChar(dwCodePage,
                                (DWORD)MB_PRECOMPOSED,
                                lpCandStrA,
                                -1,
                                (LPWSTR)NULL,
                                0);

        dwCandListLen += (j * sizeof(WCHAR));
    }

    dwCandListLen = DWORD_ALIGN(dwCandListLen);

    if (dwBufLen == 0)
        return dwCandListLen;

    if (dwBufLen < dwCandListLen) {
        RIPMSG0(RIP_WARNING, "InternalGetCandidateListAtoW: dwBufLen too small.");
        return 0;
    }

    lpCandListW->dwSize = dwBufLen;
    lpCandListW->dwStyle = lpCandListA->dwStyle;
    lpCandListW->dwCount = lpCandListA->dwCount;
    lpCandListW->dwSelection = lpCandListA->dwSelection;
    lpCandListW->dwPageStart = lpCandListA->dwPageStart;
    lpCandListW->dwPageSize = lpCandListA->dwPageSize;
    lpCandListW->dwOffset[0] = sizeof(CANDIDATELIST);
    if (lpCandListW->dwCount > 0)
        lpCandListW->dwOffset[0] += sizeof(DWORD) * (lpCandListW->dwCount - 1);

    dwCandListLen = dwBufLen - lpCandListW->dwOffset[0];

    for  (i = 0; i < (INT)lpCandListW->dwCount; i++) {

        lpCandStrA = (LPSTR) ((LPBYTE)lpCandListA + lpCandListA->dwOffset[i]);
        lpCandStrW = (LPWSTR)((LPBYTE)lpCandListW + lpCandListW->dwOffset[i]);

        j = MultiByteToWideChar(dwCodePage,
                                (DWORD)MB_PRECOMPOSED,
                                lpCandStrA,
                                -1,
                                lpCandStrW,
                                (INT)dwCandListLen/sizeof(WCHAR));

        dwCandListLen -= (j * sizeof(WCHAR));

        if (i < (INT)lpCandListW->dwCount - 1)
            lpCandListW->dwOffset[i+1] = lpCandListW->dwOffset[i] + j * sizeof(WCHAR);
    }

    return dwBufLen;
}


DWORD InternalGetCandidateListWtoA(
    LPCANDIDATELIST     lpCandListW,
    LPCANDIDATELIST     lpCandListA,
    DWORD               dwBufLen,
    DWORD               dwCodePage)
{
    LPWSTR lpCandStrW;
    LPSTR  lpCandStrA;
    INT    i, j;
    DWORD  dwCandListLen;
    BOOL   bUDC;

    dwCandListLen = sizeof(CANDIDATELIST);

    /*
     * CANDIDATELIST has already contained the dwOffset[0]
     */
    if (lpCandListW->dwCount > 0)
        dwCandListLen += sizeof(DWORD) * (lpCandListW->dwCount - 1);

    for (i = 0; i < (INT)lpCandListW->dwCount; i++) {

        lpCandStrW = (LPWSTR)((LPBYTE)lpCandListW + lpCandListW->dwOffset[i]);

        j = WideCharToMultiByte(dwCodePage,
                                (DWORD)0,
                                lpCandStrW,
                                -1,
                                (LPSTR)NULL,
                                (INT)0,
                                (LPSTR)NULL,
                                (LPBOOL)&bUDC);

        dwCandListLen += (j * sizeof(CHAR));
    }

    dwCandListLen = DWORD_ALIGN(dwCandListLen);

    if (dwBufLen == 0)
        return dwCandListLen;

    if (dwBufLen < dwCandListLen) {
        RIPMSG0(RIP_WARNING, "InternalGetCandidateListWtoA: dwBufLen too small.");
        return 0;
    }

    lpCandListA->dwSize = dwBufLen;
    lpCandListA->dwStyle = lpCandListW->dwStyle;
    lpCandListA->dwCount = lpCandListW->dwCount;
    lpCandListA->dwSelection = lpCandListW->dwSelection;
    lpCandListA->dwPageStart = lpCandListW->dwPageStart;
    lpCandListA->dwPageSize = lpCandListW->dwPageSize;
    lpCandListA->dwOffset[0] = sizeof(CANDIDATELIST);
    if (lpCandListA->dwCount > 0)
        lpCandListA->dwOffset[0] += sizeof(DWORD) * (lpCandListA->dwCount - 1);

    dwCandListLen = dwBufLen - lpCandListA->dwOffset[0];

    for  (i = 0; i < (INT)lpCandListA->dwCount; i++) {

        lpCandStrA = (LPSTR) ((LPBYTE)lpCandListA + lpCandListA->dwOffset[i]);
        lpCandStrW = (LPWSTR)((LPBYTE)lpCandListW + lpCandListW->dwOffset[i]);

        j = WideCharToMultiByte(dwCodePage,
                                (DWORD)0,
                                lpCandStrW,
                                -1,
                                (LPSTR)lpCandStrA,
                                (INT)dwCandListLen,
                                (LPSTR)NULL,
                                (LPBOOL)&bUDC);

        dwCandListLen -= (j * sizeof(CHAR));

        if (i < (INT)lpCandListA->dwCount - 1)
            lpCandListA->dwOffset[i+1] = lpCandListA->dwOffset[i] + j * sizeof(CHAR);
    }

    return dwBufLen;
}

/***************************************************************************\
* CalcCharacterPositionAtoW
*
* Calculate Unicode character position to ANSI character position.
*
* History:
* 28-Feb-1995   wkwok   Created
\***************************************************************************/

DWORD CalcCharacterPositionAtoW(
    DWORD dwCharPosA,
    LPSTR lpszCharStr,
    DWORD dwCodePage)
{
    DWORD dwCharPosW = 0;

    while (dwCharPosA != 0) {
        if (IsDBCSLeadByteEx(dwCodePage, *lpszCharStr)) {
            if (dwCharPosA >= 2) {
                dwCharPosA -= 2;
            }
            else {
                dwCharPosA--;
            }
            lpszCharStr += 2;
        }
        else {
            dwCharPosA--;
            lpszCharStr++;
        }
        dwCharPosW++;
    }

    return dwCharPosW;
}


/***************************************************************************\
* CalcCharacterPositionWtoA
*
* Calculate ANSI character position to Unicode character position.
*
* History:
* 28-Feb-1995   wkwok   Created
\***************************************************************************/

DWORD CalcCharacterPositionWtoA(
    DWORD dwCharPosW,
    LPWSTR lpwszCharStr,
    DWORD  dwCodePage)
{
    DWORD dwCharPosA = 0;
    ULONG MultiByteSize;

    while (dwCharPosW != 0) {
        MultiByteSize = UnicodeToMultiByteSize(dwCodePage, lpwszCharStr);
        if (MultiByteSize == 2) {
            dwCharPosA += 2;
        }
        else {
            dwCharPosA++;
        }
        dwCharPosW--;
        lpwszCharStr++;
    }

    return dwCharPosA;
}


VOID LFontAtoLFontW(
    LPLOGFONTA lpLogFontA,
    LPLOGFONTW lpLogFontW)
{
    INT i;

    RtlCopyMemory(lpLogFontW, lpLogFontA, sizeof(LOGFONTA)-LF_FACESIZE);

    i = MultiByteToWideChar(CP_ACP,     // Note: font face name should use ACP for A/W conversion.
                            MB_PRECOMPOSED,
                            lpLogFontA->lfFaceName,
                            strlen(lpLogFontA->lfFaceName),
                            lpLogFontW->lfFaceName,
                            LF_FACESIZE);

    lpLogFontW->lfFaceName[i] = L'\0';

    return;
}


VOID LFontWtoLFontA(
    LPLOGFONTW lpLogFontW,
    LPLOGFONTA lpLogFontA)
{
    INT  i;
    BOOL bUDC;

    RtlCopyMemory(lpLogFontA, lpLogFontW, sizeof(LOGFONTA)-LF_FACESIZE);

    i = WideCharToMultiByte(CP_ACP,     // Note: font face name should use ACP for A/W conversion.
                            0,
                            lpLogFontW->lfFaceName,
                            wcslen(lpLogFontW->lfFaceName),
                            lpLogFontA->lfFaceName,
                            LF_FACESIZE,
                            (LPSTR)NULL,
                            &bUDC);

    lpLogFontA->lfFaceName[i] = '\0';

    return;
}


BOOL MakeIMENotify(
    HIMC   hImc,
    HWND   hWnd,
    DWORD  dwAction,
    DWORD  dwIndex,
    DWORD  dwValue,
    WPARAM wParam,
    LPARAM lParam)
{
    PIMEDPI pImeDpi;
    DWORD   dwThreadId;

#ifdef LATER
    // implement MakeIMENotifyEvent() later
#endif

    if (dwAction != 0 && (dwThreadId = GetInputContextThread(hImc)) != 0) {

        pImeDpi = ImmLockImeDpi(GetKeyboardLayout(dwThreadId));

        if (pImeDpi != NULL) {
            (*pImeDpi->pfn.NotifyIME)(hImc, dwAction, dwIndex, dwValue);
            ImmUnlockImeDpi(pImeDpi);
        }
    }

    if (hWnd != NULL && wParam != 0)
        SendMessage(hWnd, WM_IME_NOTIFY, wParam, lParam);

    return TRUE;
}



//////////////////////////////////////////////////////////////////////
// Reconversion support
//////////////////////////////////////////////////////////////////////

typedef enum {FROM_IME, FROM_APP} REQ_CALLER;

///////////////////////////////////////////////////////////////////////////////////
// ImmGetReconvertTotalSize
//
// calculate the appropriate size of the buffer, based on caller/ansi information
//
// History:
// 28-Feb-1997   hiroyama   Created
///////////////////////////////////////////////////////////////////////////////////

DWORD ImmGetReconvertTotalSize(DWORD dwSize, REQ_CALLER eCaller, BOOL bAnsiTarget)
{
    if (dwSize < sizeof(RECONVERTSTRING)) {
        return 0;
    }
    if (bAnsiTarget) {
        dwSize -= sizeof(RECONVERTSTRING);
        if (eCaller == FROM_IME) {
            dwSize /= 2;
        } else {
            dwSize *= 2;
        }
        dwSize += sizeof(RECONVERTSTRING);
    }
    return dwSize;
}

DWORD ImmReconversionWorker(
        LPRECONVERTSTRING lpRecTo,
        LPRECONVERTSTRING lpRecFrom,
        BOOL bToAnsi,
        DWORD dwCodePage)
{
    INT i;
    DWORD dwSize = 0;

    UserAssert(lpRecTo);
    UserAssert(lpRecFrom);

    if (lpRecFrom->dwVersion != 0 || lpRecTo->dwVersion != 0) {
        RIPMSG0(RIP_WARNING, "ImmReconversionWorker: dwVersion in lpRecTo or lpRecFrom is incorrect.");
        return 0;
    }
    // Note:
    // In any IME related structures, use the following principal.
    // 1) xxxStrOffset is an actual offset, i.e. byte count.
    // 2) xxxStrLen is a number of characters, i.e. TCHAR count.
    //
    // CalcCharacterPositionXtoY() takes TCHAR count so that we
    // need to adjust xxxStrOffset if it's being converted. But you
    // should be careful, because the actual position of the string
    // is always at something like (LPBYTE)lpStruc + lpStruc->dwStrOffset.
    //
    if (bToAnsi) {
        // Convert W to A
        lpRecTo->dwStrOffset = sizeof *lpRecTo;
        i = WideCharToMultiByte(dwCodePage,
                                (DWORD)0,
                                (LPWSTR)((LPSTR)lpRecFrom + lpRecFrom->dwStrOffset), // src
                                (INT)lpRecFrom->dwStrLen,
                                (LPSTR)lpRecTo + lpRecTo->dwStrOffset,  // dest
                                (INT)lpRecFrom->dwStrLen * DBCS_CHARSIZE,
                                (LPSTR)NULL,
                                (LPBOOL)NULL);
        lpRecTo->dwCompStrOffset =
            CalcCharacterPositionWtoA(lpRecFrom->dwCompStrOffset / sizeof(WCHAR),
                                      (LPWSTR)((LPBYTE)lpRecFrom + lpRecFrom->dwStrOffset),
                                      dwCodePage)
                            * sizeof(CHAR);

        lpRecTo->dwCompStrLen =
            (CalcCharacterPositionWtoA(lpRecFrom->dwCompStrOffset / sizeof(WCHAR) +
                                      lpRecFrom->dwCompStrLen,
                                      (LPWSTR)((LPBYTE)lpRecFrom + lpRecFrom->dwStrOffset),
                                      dwCodePage)
                            * sizeof(CHAR))
            - lpRecTo->dwCompStrOffset;

        lpRecTo->dwTargetStrOffset =
            CalcCharacterPositionWtoA(lpRecFrom->dwTargetStrOffset / sizeof(WCHAR),
                                      (LPWSTR)((LPBYTE)lpRecFrom +
                                                lpRecFrom->dwStrOffset),
                                      dwCodePage)
                            * sizeof(CHAR);

        lpRecTo->dwTargetStrLen =
            (CalcCharacterPositionWtoA(lpRecFrom->dwTargetStrOffset / sizeof(WCHAR) +
                                      lpRecFrom->dwTargetStrLen,
                                      (LPWSTR)((LPBYTE)lpRecFrom + lpRecFrom->dwStrOffset),
                                       dwCodePage)
                            * sizeof(CHAR))
            - lpRecTo->dwTargetStrOffset;

        ((LPSTR)lpRecTo)[lpRecTo->dwStrOffset + i] = '\0';
        lpRecTo->dwStrLen = i * sizeof(CHAR);

        dwSize = sizeof(RECONVERTSTRING) + ((i + 1) * sizeof(CHAR));

    } else {

        // AtoW
        lpRecTo->dwStrOffset = sizeof *lpRecTo;
        i = MultiByteToWideChar(dwCodePage,
                                (DWORD)MB_PRECOMPOSED,
                                (LPSTR)lpRecFrom + lpRecFrom->dwStrOffset,  // src
                                (INT)lpRecFrom->dwStrLen,
                                (LPWSTR)((LPSTR)lpRecTo + lpRecTo->dwStrOffset), // dest
                                (INT)lpRecFrom->dwStrLen);

        lpRecTo->dwCompStrOffset =
            CalcCharacterPositionAtoW(lpRecFrom->dwCompStrOffset,
                                      (LPSTR)lpRecFrom + lpRecFrom->dwStrOffset,
                                      dwCodePage) * sizeof(WCHAR);

        lpRecTo->dwCompStrLen =
            ((CalcCharacterPositionAtoW(lpRecFrom->dwCompStrOffset +
                                       lpRecFrom->dwCompStrLen,
                                       (LPSTR)lpRecFrom + lpRecFrom->dwStrOffset,
                                        dwCodePage)  * sizeof(WCHAR))
            - lpRecTo->dwCompStrOffset) / sizeof(WCHAR);

        lpRecTo->dwTargetStrOffset =
            CalcCharacterPositionAtoW(lpRecFrom->dwTargetStrOffset,
                                      (LPSTR)lpRecFrom + lpRecFrom->dwStrOffset,
                                      dwCodePage) * sizeof(WCHAR);

        lpRecTo->dwTargetStrLen =
            ((CalcCharacterPositionAtoW(lpRecFrom->dwTargetStrOffset +
                                       lpRecFrom->dwTargetStrLen,
                                       (LPSTR)lpRecFrom + lpRecFrom->dwStrOffset,
                                       dwCodePage)  * sizeof(WCHAR))
            - lpRecTo->dwTargetStrOffset) / sizeof(WCHAR);

        lpRecTo->dwStrLen = i;  // Length is TCHAR count.
        if (lpRecTo->dwSize >= (DWORD)(lpRecTo->dwStrOffset + (i + 1)* sizeof(WCHAR))) {
            LPWSTR lpW = (LPWSTR)((LPSTR)lpRecTo + lpRecTo->dwStrOffset);
            lpW[i] = L'\0';
        }
        dwSize = sizeof(RECONVERTSTRING) + ((i + 1) * sizeof(WCHAR));
    }
    return dwSize;
}

///////////////////////////////////////////////////////////////////////////////////
// ImmRequestMessageWorker
//
// worker function for WM_IME_REQUEST message
//
// History:
// 30-Mar-1997   hiroyama   Created
///////////////////////////////////////////////////////////////////////////////////

LRESULT ImmRequestMessageWorker(HIMC hIMC, PWND pwnd, WPARAM wParam, LPARAM lParam, BOOL bAnsiOrigin)
{
    // the (least) size of the structure given in lParam: for valid pointer checking
    static CONST int nReqBufSize[][7] = {
        {   // sizes if IME is UNICODE
            sizeof(COMPOSITIONFORM),    // IMR_COMPOSITIONWINDOW
            sizeof(CANDIDATEFORM),      // IMR_CANDIDATEWINDOW
            sizeof(LOGFONTW),           // IMR_COMPOSITIONFONT
            sizeof(RECONVERTSTRING),    // IMR_RECONVERTSTRING
            sizeof(RECONVERTSTRING),    // IMR_CONFIRMRECONVERTSTRING
            sizeof(IMECHARPOSITION),    // IMR_QUERYCHARPOSITION
            sizeof(RECONVERTSTRING),    // IMR_DOCUMENTFEED
        },
        {   // sizes if IME is ANSI
            sizeof(COMPOSITIONFORM),    // IMR_COMPOSITIONWINDOW
            sizeof(CANDIDATEFORM),      // IMR_CANDIDATEWINDOW
            sizeof(LOGFONTA),           // IMR_COMPOSITIONFONT
            sizeof(RECONVERTSTRING),    // IMR_RECONVERTSTRING
            sizeof(RECONVERTSTRING),    // IMR_CONFIRMRECONVERTSTRING
            sizeof(IMECHARPOSITION),    // IMR_QUERYCHARPOSITION
            sizeof(RECONVERTSTRING),    // IMR_DOCUMENTFEED
        }
    };
    LRESULT lRet = 0L;
    CONST BOOLEAN bAnsiTarget = !!TestWF(pwnd, WFANSIPROC);    // TRUE if the target Window Proc is ANSI
    LPBYTE lpReq = (LPBYTE)lParam;                          // return buffer (maybe allocated buffer)
    LPBYTE lpNew = NULL;                                    // buffer allocated within this function
    DWORD dwSaveCharPos;
    PCLIENTIMC pClientImc;
    DWORD dwCodePage;

#define SEND_MESSAGE(bAnsi)   ((bAnsi) ? SendMessageA : SendMessageW)

    //////////////////////////////////////////////
    // Parameter checking

    // check wParam as sub messages
    if (wParam == 0 || wParam > IMR_DOCUMENTFEED) {  // wParam is not a proper sub message
        RIPMSG1(RIP_WARNING, "ImmRequestMessageWorker: wParam(%lx) out of range.", wParam);
        return 0L;
    }

    // Check if the pointer which is given through lParam points the proper memory block.
    UserAssert(bAnsiOrigin == 0 || bAnsiOrigin == 1);   // we'll use bAnsiOrigin as an index

    // The first sub message IMR_COMPOSITIONWINDOW is 1, so substract 1 from wParam
    if (lpReq && IsBadWritePtr(lpReq, nReqBufSize[bAnsiOrigin][wParam - 1])) {
        RIPMSG0(RIP_WARNING, "ImmRequestMessageWorker: Bad pointer passed from IME to write");
        return 0L;
    }

    // check the lpReq(==lParam): the spec does not allow lParam as NULL
    // except IMR_RECONVERTSTRING and IMR_DOCUMENTFEED
    if (wParam == IMR_RECONVERTSTRING || wParam == IMR_DOCUMENTFEED) {
        //
        // check version number
        //
        if (lpReq != NULL) {
            LPRECONVERTSTRING lpReconv = (LPRECONVERTSTRING)lParam;
            if (lpReconv->dwVersion != 0) {
                RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid version number: %d",
                        lpReconv->dwVersion);
                return 0L;
            }
            if (lpReconv->dwSize < sizeof(RECONVERTSTRING)) {
                RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid dwSize: %d",
                        lpReconv->dwSize);
                return 0L;
            }
        }
    } else if (wParam == IMR_CONFIRMRECONVERTSTRING) {
        // check if lParam is not NULL, and version of the structure is correct.
        if (lpReq == NULL || ((LPRECONVERTSTRING)lpReq)->dwVersion != 0) {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid argument or invalid version number");
            return 0L;
        }
    } else if (lpReq == NULL) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING,
                "ImmRequestMessageWorker: lParam should not be NULL with this wParam(%lx).",
                wParam);
        return 0L;
    }
    // end parameter checking
    ////////////////////////////////////////////

    pClientImc = ImmLockClientImc(hIMC);
    if (pClientImc != NULL) {
        dwCodePage = CImcCodePage(pClientImc);
        ImmUnlockClientImc(pClientImc);
    }
    else {
        dwCodePage = CP_ACP;
    }

    // allocate and prepare required buffer if we need A/W conversion
    switch (wParam) {
    case IMR_CONFIRMRECONVERTSTRING:
    case IMR_RECONVERTSTRING:
    case IMR_DOCUMENTFEED:
        if (bAnsiOrigin != bAnsiTarget) {
            if (lpReq != NULL) {
                // IME wants not only the buffer size but the real reconversion information
                DWORD dwSize = ImmGetReconvertTotalSize(((LPRECONVERTSTRING)lpReq)->dwSize, FROM_IME, bAnsiTarget);
                LPRECONVERTSTRING lpReconv;

                lpNew = ImmLocalAlloc(0, dwSize + sizeof(WCHAR));
                if (lpNew == NULL) {
                    RIPMSG0(RIP_WARNING, "ImmRequestMessageWorker: failed to allocate a buffer for reconversion.");
                    return 0L;
                }
                lpReconv = (LPRECONVERTSTRING)lpNew;
                // setup the information in the allocated structure
                lpReconv->dwVersion = 0;
                lpReconv->dwSize = dwSize;

                //
                // if it's confirmation message, we need to translate the contents
                //
                if (wParam == IMR_CONFIRMRECONVERTSTRING) {
                    ImmReconversionWorker(lpReconv, (LPRECONVERTSTRING)lParam, bAnsiTarget, dwCodePage);
                }
            }
        }
        break;

    case IMR_COMPOSITIONFONT:
        UserAssert(lpReq != NULL);      // has been checked so far
        if (bAnsiOrigin != bAnsiTarget) {
            if (bAnsiTarget) {
                lpNew = ImmLocalAlloc(0, sizeof(LOGFONTA));
            } else {
                lpNew = ImmLocalAlloc(0, sizeof(LOGFONTW));
            }
            if (lpNew == NULL) {
                RIPMSG0(RIP_WARNING, "ImmRequestMessageWorker: IMR_COMPOSITIONFONT: failed to allocate memory for A/W conversion.");
                return 0L;
            }
        }
        break;

    case IMR_QUERYCHARPOSITION:
        UserAssert(lpReq != NULL);
        if (bAnsiOrigin != bAnsiTarget) {
#define lpIMEPOS    ((LPIMECHARPOSITION)lParam)
            LPVOID lpstr;
            DWORD dwLen;

            dwSaveCharPos = lpIMEPOS->dwCharPos;

            dwLen = (!bAnsiOrigin ? ImmGetCompositionStringW : ImmGetCompositionStringA)(hIMC, GCS_COMPSTR, 0, 0);
            if (dwLen == 0) {
                RIPMSG0(RIP_WARNING, "ImmRequestMessageWorker: IMR_QUERYCHARPOSITION no compositiong string.");
                return 0L;
            }

            lpstr = ImmLocalAlloc(0, (dwLen + 1) * (!bAnsiOrigin ? sizeof(WCHAR) : sizeof(CHAR)));
            if (lpstr == NULL) {
                RIPMSG0(RIP_WARNING, "ImmRequestMessageWorker: IMR_QUERYCHARPOSITION: failed to allocate memory for A/W conversion.");
                return 0L;
            }

            (!bAnsiOrigin ? ImmGetCompositionStringW : ImmGetCompositionStringA)(hIMC, GCS_COMPSTR, lpstr, dwLen);
            if (bAnsiTarget) {
                lpIMEPOS->dwCharPos = CalcCharacterPositionWtoA(lpIMEPOS->dwCharPos, lpstr, dwCodePage);
            } else {
                lpIMEPOS->dwCharPos = CalcCharacterPositionAtoW(lpIMEPOS->dwCharPos, lpstr, dwCodePage);
            }

            ImmLocalFree(lpstr);
        }
        break;

    default:
        UserAssert(lpReq != NULL);      // has been checked so far
        break;
    }

    if (lpNew) {
        // if we allocated the buffer, let lpReq point it; lpNew is used later to free memory
        lpReq = lpNew;
    }

    //////////////////////////////////
    lRet = SEND_MESSAGE(bAnsiTarget)(HW(pwnd), WM_IME_REQUEST, wParam, (LPARAM)lpReq);
    //////////////////////////////////

    // copy back the results from WinProc to IME's buffer (only if conversion is needed)
    if (bAnsiOrigin != bAnsiTarget) {
        switch (wParam) {
        case IMR_RECONVERTSTRING:
        case IMR_DOCUMENTFEED:
            // Note: by definition, we don't have to do back-conversion for IMR_CONFIRMRECONVERTSTRING
            if (lRet != 0) {
                // IME wants the buffer size
                lRet = ImmGetReconvertTotalSize((DWORD)lRet, FROM_APP, bAnsiTarget);
                if (lRet < sizeof(RECONVERTSTRING)) {
                    RIPMSG1(RIP_WARNING, "ImmRequestMessageWorker: return value from application %d is invalid.", lRet);
                    lRet = 0;
                } else if (lpReq) {
                    // We need to perform the A/W conversion of the contents
                    if (!ImmReconversionWorker((LPRECONVERTSTRING)lParam, (LPRECONVERTSTRING)lpReq, bAnsiOrigin, dwCodePage)) {
                        lRet = 0;   // Error !
                    }
                }
            }
            break;
        case IMR_COMPOSITIONFONT:
            if (bAnsiOrigin) {
                LFontWtoLFontA((LPLOGFONTW)lpNew, (LPLOGFONTA)lParam);
            } else {
                LFontAtoLFontW((LPLOGFONTA)lpNew, (LPLOGFONTW)lParam);
            }
            break;
        case IMR_QUERYCHARPOSITION:
            UserAssert((LPVOID)lParam != NULL);
            lpIMEPOS->dwCharPos = dwSaveCharPos;
    #undef lpIMEPOS
            break;
        default:
            break;
        }

    }
    if (lpNew) {
        // buffer has been allocated, free it before returning
        ImmLocalFree(lpNew);
    }
    return lRet;
}

/**************************************************************************\
* ImmRequestMessage: Send WM_IME_REQUEST message to the given HIMC window
*
* IME function
*
* 27-Feb-1997 hiroyama      Created
\**************************************************************************/
LRESULT ImmRequestMessageAorW(HIMC hIMC, WPARAM wParam, LPARAM lParam, BOOL bAnsiOrigin)
{
    LPINPUTCONTEXT lpInputContext;
    PWND pwnd;
    LRESULT lRet = 0L;
    DWORD dwThreadId = GetInputContextThread(hIMC);

    if (dwThreadId != GetCurrentThreadId()) {
        RIPMSG1(RIP_WARNING,
              "ImmRequestMessageAorW:: Invalid input context access %lx.", hIMC);
        return lRet;
    }

    if (hIMC == NULL || (lpInputContext = ImmLockIMC(hIMC)) == NULL) {
        RIPMSG1(RIP_WARNING, "ImmRequestMessage: Invalid hImc %lx.", hIMC);
        return 0L;
    }

    // check if the window of the input context is valid
    if ((pwnd = ValidateHwnd(lpInputContext->hWnd)) == NULL) {
        RIPMSG1(RIP_WARNING, "ImmRequestMessage: Invalid hWnd %lx.", lpInputContext->hWnd);
    } else {
        // check if the message is being sent inter thread
        if (PtiCurrent() != GETPTI(pwnd)) {
            RIPMSG0(RIP_WARNING, "ImmRequestMessage: IME Attempt to send IMR_ message to different thread.");
        } else {
            lRet = ImmRequestMessageWorker(hIMC, pwnd, wParam, lParam, bAnsiOrigin);
        }
    }

    ImmUnlockIMC(hIMC);

    return lRet;
}

LRESULT WINAPI ImmRequestMessageA(HIMC hIMC, WPARAM wParam, LPARAM lParam)
{
    return ImmRequestMessageAorW(hIMC, wParam, lParam, TRUE /* ANSI */);
}

LRESULT WINAPI ImmRequestMessageW(HIMC hIMC, WPARAM wParam, LPARAM lParam)
{
    return ImmRequestMessageAorW(hIMC, wParam, lParam, FALSE /* not ANSI */);
}
