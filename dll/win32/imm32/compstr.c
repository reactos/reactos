/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing composition strings of IMM32
 * COPYRIGHT:   Copyright 1998 Patrik Stridvall
 *              Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
 *              Copyright 2017 James Tabor <james.tabor@reactos.org>
 *              Copyright 2018 Amine Khaldi <amine.khaldi@reactos.org>
 *              Copyright 2020 Oleg Dubinskiy <oleg.dubinskij2013@yandex.ua>
 *              Copyright 2020-2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

BOOL APIENTRY
Imm32OpenICAndCS(HIMC hIMC, LPINPUTCONTEXT *ppIC, LPCOMPOSITIONSTRING *ppCS)
{
    LPINPUTCONTEXT pIC;
    LPCOMPOSITIONSTRING pCS;

    *ppIC = NULL;
    *ppCS = NULL;

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return FALSE;

    pCS = ImmLockIMCC(pIC->hCompStr);
    if (!pCS)
    {
        ImmUnlockIMC(hIMC);
        return FALSE;
    }

    *ppIC = pIC;
    *ppCS = pCS;
    return TRUE;
}

static inline LONG APIENTRY
Imm32CompStrAnsiToWide(LPCSTR psz, DWORD cb, LPWSTR lpBuf, DWORD dwBufLen, UINT uCodePage)
{
    DWORD ret = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED, psz, cb / sizeof(CHAR),
                                    lpBuf, dwBufLen / sizeof(WCHAR));
    if (lpBuf && (ret + 1) * sizeof(WCHAR) <= dwBufLen)
        lpBuf[ret] = 0;
    return ret * sizeof(WCHAR);
}

static inline LONG APIENTRY
Imm32CompStrWideToAnsi(LPCWSTR psz, DWORD cb, LPSTR lpBuf, DWORD dwBufLen, UINT uCodePage)
{
    DWORD ret = WideCharToMultiByte(uCodePage, 0, psz, cb / sizeof(WCHAR),
                                    lpBuf, dwBufLen / sizeof(CHAR), NULL, NULL);
    if (lpBuf && (ret + 1) * sizeof(CHAR) <= dwBufLen)
        lpBuf[ret] = 0;
    return ret * sizeof(CHAR);
}

static INT APIENTRY
Imm32CompAttrWideToAnsi(const BYTE *src, INT src_len, LPCWSTR text,
                        INT str_len, LPBYTE dst, INT dst_len, UINT uCodePage)
{
    INT rc;
    INT i, j = 0, k = 0, len;

    if (!src_len)
        return 0;

    str_len /= sizeof(WCHAR);
    rc = WideCharToMultiByte(uCodePage, 0, text, str_len, NULL, 0, NULL, NULL);

    if (dst_len)
    {
        if (dst_len > rc)
            dst_len = rc;

        for (i = 0; i < str_len; ++i, ++k)
        {
            len = WideCharToMultiByte(uCodePage, 0, &text[i], 1, NULL, 0, NULL, NULL);
            for (; len > 0; --len)
            {
                dst[j++] = src[k];

                if (dst_len <= j)
                    goto end;
            }
        }
end:
        rc = j;
    }

    return rc * sizeof(BYTE);
}

static INT APIENTRY
Imm32CompAttrAnsiToWide(const BYTE *src, INT src_len, LPCSTR text,
                        INT str_len, LPBYTE dst, INT dst_len, UINT uCodePage)
{
    INT rc;
    INT i, j = 0;

    if (!src_len)
        return 0;

    str_len /= sizeof(CHAR);
    rc = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED, text, str_len, NULL, 0);

    if (dst_len)
    {
        if (dst_len > rc)
            dst_len = rc;

        for (i = 0; i < str_len; ++i)
        {
            if (IsDBCSLeadByteEx(uCodePage, text[i]) && text[i + 1])
                continue;

            dst[j++] = src[i];

            if (dst_len <= j)
                break;
        }

        rc = j;
    }

    return rc * sizeof(BYTE);
}

static INT APIENTRY
Imm32CompClauseAnsiToWide(const DWORD *source, INT slen, LPCSTR text,
                          LPDWORD target, INT tlen, UINT uCodePage)
{
    INT rc, i;

    if (!slen)
        return 0;

    if (tlen)
    {
        if (tlen > slen)
            tlen = slen;

        tlen /= sizeof(DWORD);

        for (i = 0; i < tlen; ++i)
        {
            target[i] = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED, text, source[i], NULL, 0);
        }

        rc = sizeof(DWORD) * i;
    }
    else
    {
        rc = slen;
    }

    return rc;
}

static INT APIENTRY
Imm32CompClauseWideToAnsi(const DWORD *source, INT slen, LPCWSTR text,
                          LPDWORD target, INT tlen, UINT uCodePage)
{
    INT rc, i;

    if (!slen)
        return 0;

    if (tlen)
    {
        if (tlen > slen)
            tlen = slen;

        tlen /= sizeof(DWORD);

        for (i = 0; i < tlen; ++i)
        {
            target[i] = WideCharToMultiByte(uCodePage, 0, text, source[i], NULL, 0, NULL, NULL);
        }

        rc = sizeof(DWORD) * i;
    }
    else
    {
        rc = slen;
    }

    return rc;
}

#define CS_StrA(pCS, Name)      ((LPCSTR)(pCS) + (pCS)->dw##Name##Offset)
#define CS_StrW(pCS, Name)      ((LPCWSTR)CS_StrA(pCS, Name))
#define CS_Attr(pCS, Name)      ((const BYTE *)CS_StrA(pCS, Name))
#define CS_Clause(pCS, Name)    ((const DWORD *)CS_StrA(pCS, Name))
#define CS_Size(pCS, Name)      ((pCS)->dw##Name##Len)
#define CS_SizeA(pCS, Name)     (CS_Size(pCS, Name) * sizeof(CHAR))
#define CS_SizeW(pCS, Name)     (CS_Size(pCS, Name) * sizeof(WCHAR))

#define CS_DoStr(pCS, Name, AorW) do { \
    if (dwBufLen == 0) { \
        dwBufLen = CS_Size##AorW((pCS), Name); \
    } else { \
        if (dwBufLen > CS_Size##AorW((pCS), Name)) \
            dwBufLen = CS_Size##AorW((pCS), Name); \
        RtlCopyMemory(lpBuf, CS_Str##AorW((pCS), Name), dwBufLen); \
    } \
} while (0)

#define CS_DoStrA(pCS, Name) CS_DoStr(pCS, Name, A)
#define CS_DoStrW(pCS, Name) CS_DoStr(pCS, Name, W)
#define CS_DoAttr CS_DoStrA
#define CS_DoClause CS_DoStrA

LONG APIENTRY
Imm32GetCompStrA(HIMC hIMC, const COMPOSITIONSTRING *pCS, DWORD dwIndex,
                 LPVOID lpBuf, DWORD dwBufLen, BOOL bAnsiClient, UINT uCodePage)
{
    if (bAnsiClient)
    {
        switch (dwIndex)
        {
            case GCS_COMPREADSTR:
                CS_DoStrA(pCS, CompReadStr);
                break;

            case GCS_COMPREADATTR:
                CS_DoAttr(pCS, CompReadAttr);
                break;

            case GCS_COMPREADCLAUSE:
                CS_DoClause(pCS, CompReadClause);
                break;

            case GCS_COMPSTR:
                CS_DoStrA(pCS, CompStr);
                break;

            case GCS_COMPATTR:
                CS_DoAttr(pCS, CompAttr);
                break;

            case GCS_COMPCLAUSE:
                CS_DoClause(pCS, CompClause);
                break;

            case GCS_CURSORPOS:
                dwBufLen = pCS->dwCursorPos;
                break;

            case GCS_DELTASTART:
                dwBufLen = pCS->dwDeltaStart;
                break;

            case GCS_RESULTREADSTR:
                CS_DoStrA(pCS, ResultReadStr);
                break;

            case GCS_RESULTREADCLAUSE:
                CS_DoClause(pCS, ResultReadClause);
                break;

            case GCS_RESULTSTR:
                CS_DoStrA(pCS, ResultStr);
                break;

            case GCS_RESULTCLAUSE:
                CS_DoClause(pCS, ResultClause);
                break;

            default:
                FIXME("TODO:\n");
                return IMM_ERROR_GENERAL;
        }
    }
    else /* !bAnsiClient */
    {
        switch (dwIndex)
        {
            case GCS_COMPREADSTR:
                dwBufLen = Imm32CompStrWideToAnsi(CS_StrW(pCS, CompReadStr),
                                                  CS_SizeW(pCS, CompReadStr),
                                                  lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_COMPREADATTR:
                dwBufLen = Imm32CompAttrWideToAnsi(CS_Attr(pCS, CompReadAttr),
                                                   CS_Size(pCS, CompReadAttr),
                                                   CS_StrW(pCS, CompStr),
                                                   CS_SizeW(pCS, CompStr),
                                                   lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_COMPREADCLAUSE:
                dwBufLen = Imm32CompClauseWideToAnsi(CS_Clause(pCS, CompReadClause),
                                                     CS_Size(pCS, CompReadClause),
                                                     CS_StrW(pCS, CompStr),
                                                     lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_COMPSTR:
                dwBufLen = Imm32CompStrWideToAnsi(CS_StrW(pCS, CompStr),
                                                  CS_SizeW(pCS, CompStr),
                                                  lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_COMPATTR:
                dwBufLen = Imm32CompAttrWideToAnsi(CS_Attr(pCS, CompAttr),
                                                   CS_Size(pCS, CompAttr),
                                                   CS_StrW(pCS, CompStr),
                                                   CS_SizeW(pCS, CompStr),
                                                   lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_COMPCLAUSE:
                dwBufLen = Imm32CompClauseWideToAnsi(CS_Clause(pCS, CompClause),
                                                     CS_Size(pCS, CompClause),
                                                     CS_StrW(pCS, CompStr),
                                                     lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_CURSORPOS:
                dwBufLen = IchAnsiFromWide(pCS->dwCursorPos, CS_StrW(pCS, CompStr), uCodePage);
                break;

            case GCS_DELTASTART:
                dwBufLen = IchAnsiFromWide(pCS->dwDeltaStart, CS_StrW(pCS, CompStr), uCodePage);
                break;

            case GCS_RESULTREADSTR:
                dwBufLen = Imm32CompStrWideToAnsi(CS_StrW(pCS, ResultReadStr),
                                                  CS_SizeW(pCS, ResultReadStr),
                                                  lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_RESULTREADCLAUSE:
                dwBufLen = Imm32CompClauseWideToAnsi(CS_Clause(pCS, ResultReadClause),
                                                     CS_Size(pCS, ResultReadClause),
                                                     CS_StrW(pCS, CompStr),
                                                     lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_RESULTSTR:
                dwBufLen = Imm32CompStrWideToAnsi(CS_StrW(pCS, ResultStr),
                                                  CS_SizeW(pCS, ResultStr),
                                                  lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_RESULTCLAUSE:
                dwBufLen = Imm32CompClauseWideToAnsi(CS_Clause(pCS, ResultClause),
                                                     CS_Size(pCS, ResultClause),
                                                     CS_StrW(pCS, CompStr),
                                                     lpBuf, dwBufLen, uCodePage);
                break;

            default:
                FIXME("TODO:\n");
                return IMM_ERROR_GENERAL;
        }
    }

    return dwBufLen;
}

LONG APIENTRY
Imm32GetCompStrW(HIMC hIMC, const COMPOSITIONSTRING *pCS, DWORD dwIndex,
                 LPVOID lpBuf, DWORD dwBufLen, BOOL bAnsiClient, UINT uCodePage)
{
    if (bAnsiClient)
    {
        switch (dwIndex)
        {
            case GCS_COMPREADSTR:
                dwBufLen = Imm32CompStrAnsiToWide(CS_StrA(pCS, CompReadStr),
                                                  CS_SizeA(pCS, CompReadStr),
                                                  lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_COMPREADATTR:
                dwBufLen = Imm32CompAttrAnsiToWide(CS_Attr(pCS, CompReadAttr),
                                                   CS_Size(pCS, CompReadAttr),
                                                   CS_StrA(pCS, CompStr), CS_SizeA(pCS, CompStr),
                                                   lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_COMPREADCLAUSE:
                dwBufLen = Imm32CompClauseAnsiToWide(CS_Clause(pCS, CompReadClause),
                                                     CS_Size(pCS, CompReadClause),
                                                     CS_StrA(pCS, CompStr),
                                                     lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_COMPSTR:
                dwBufLen = Imm32CompStrAnsiToWide(CS_StrA(pCS, CompStr),
                                                  CS_SizeA(pCS, CompStr),
                                                  lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_COMPATTR:
                dwBufLen = Imm32CompAttrAnsiToWide(CS_Attr(pCS, CompAttr),
                                                   CS_Size(pCS, CompAttr),
                                                   CS_StrA(pCS, CompStr), CS_SizeA(pCS, CompStr),
                                                   lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_COMPCLAUSE:
                dwBufLen = Imm32CompClauseAnsiToWide(CS_Clause(pCS, CompClause),
                                                     CS_Size(pCS, CompClause),
                                                     CS_StrA(pCS, CompStr),
                                                     lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_CURSORPOS:
                dwBufLen = IchWideFromAnsi(pCS->dwCursorPos, CS_StrA(pCS, CompStr), uCodePage);
                break;

            case GCS_DELTASTART:
                dwBufLen = IchWideFromAnsi(pCS->dwDeltaStart, CS_StrA(pCS, CompStr), uCodePage);
                break;

            case GCS_RESULTREADSTR:
                dwBufLen = Imm32CompStrAnsiToWide(CS_StrA(pCS, ResultReadStr),
                                                  CS_SizeA(pCS, ResultReadStr),
                                                  lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_RESULTREADCLAUSE:
                dwBufLen = Imm32CompClauseAnsiToWide(CS_Clause(pCS, ResultReadClause),
                                                     CS_Size(pCS, ResultReadClause),
                                                     CS_StrA(pCS, CompStr),
                                                     lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_RESULTSTR:
                dwBufLen = Imm32CompStrAnsiToWide(CS_StrA(pCS, ResultStr),
                                                  CS_SizeA(pCS, ResultStr),
                                                  lpBuf, dwBufLen, uCodePage);
                break;

            case GCS_RESULTCLAUSE:
                dwBufLen = Imm32CompClauseAnsiToWide(CS_Clause(pCS, ResultClause),
                                                     CS_Size(pCS, ResultClause),
                                                     CS_StrA(pCS, CompStr),
                                                     lpBuf, dwBufLen, uCodePage);
                break;

            default:
                FIXME("TODO:\n");
                return IMM_ERROR_GENERAL;
        }
    }
    else /* !bAnsiClient */
    {
        switch (dwIndex)
        {
            case GCS_COMPREADSTR:
                CS_DoStrW(pCS, CompReadStr);
                break;

            case GCS_COMPREADATTR:
                CS_DoAttr(pCS, CompReadAttr);
                break;

            case GCS_COMPREADCLAUSE:
                CS_DoClause(pCS, CompReadClause);
                break;

            case GCS_COMPSTR:
                CS_DoStrW(pCS, CompStr);
                break;

            case GCS_COMPATTR:
                CS_DoAttr(pCS, CompAttr);
                break;

            case GCS_COMPCLAUSE:
                CS_DoClause(pCS, CompClause);
                break;

            case GCS_CURSORPOS:
                dwBufLen = pCS->dwCursorPos;
                break;

            case GCS_DELTASTART:
                dwBufLen = pCS->dwDeltaStart;
                break;

            case GCS_RESULTREADSTR:
                CS_DoStrW(pCS, ResultReadStr);
                break;

            case GCS_RESULTREADCLAUSE:
                CS_DoClause(pCS, ResultReadClause);
                break;

            case GCS_RESULTSTR:
                CS_DoStrW(pCS, ResultStr);
                break;

            case GCS_RESULTCLAUSE:
                CS_DoClause(pCS, ResultClause);
                break;

            default:
                FIXME("TODO:\n");
                return IMM_ERROR_GENERAL;
        }
    }

    return dwBufLen;
}

BOOL APIENTRY
Imm32SetCompositionStringAW(HIMC hIMC, DWORD dwIndex, LPVOID pComp, DWORD dwCompLen,
                            LPVOID pRead, DWORD dwReadLen, BOOL bAnsiAPI)
{
    BOOL ret = FALSE, bAnsiClient;
    LPVOID pCompNew = NULL, pReadNew = NULL;
    DWORD dwThreadId, cbCompNew = 0, cbReadNew = 0;
    LPINPUTCONTEXT pIC;
    LPCOMPOSITIONSTRING pCS;
    HKL hKL;
    PIMEDPI pImeDpi;
    UINT uCodePage;
    LPRECONVERTSTRING pRS;

    dwThreadId = (DWORD)NtUserQueryInputContext(hIMC, QIC_INPUTTHREADID);
    if (dwThreadId != GetCurrentThreadId())
        return FALSE;

    hKL = GetKeyboardLayout(dwThreadId);
    pImeDpi = ImmLockImeDpi(hKL);
    if (!pImeDpi)
        return FALSE;

    uCodePage = pImeDpi->uCodePage;
    bAnsiClient = !ImeDpi_IsUnicode(pImeDpi);

    switch (dwIndex)
    {
        case SCS_SETSTR: case SCS_CHANGEATTR: case SCS_CHANGECLAUSE:
            break;

        case SCS_SETRECONVERTSTRING: case SCS_QUERYRECONVERTSTRING:
            if (pImeDpi->ImeInfo.fdwSCSCaps & SCS_CAP_SETRECONVERTSTRING)
                break;
            /* FALL THROUGH */
        default:
            ImmUnlockImeDpi(pImeDpi);
            return FALSE;
    }

    if (bAnsiAPI == bAnsiClient || (!pComp && !pRead))
    {
        ret = pImeDpi->ImeSetCompositionString(hIMC, dwIndex, pComp, dwCompLen,
                                               pRead, dwReadLen);
        ImmUnlockImeDpi(pImeDpi);
        return ret;
    }

    if (!Imm32OpenICAndCS(hIMC, &pIC, &pCS))
    {
        ImmUnlockImeDpi(pImeDpi);
        return FALSE;
    }

    /*
     * This code is really too complicated. But I cannot simplify.
     * It converts like (pComp, dwCompLen) --> (pCompNew, cbCompNew) and
     * (pRead, dwReadLen) --> (pReadNew, cbReadNew).
     * (1) Check bAnsiClient, (2) Get the size, (3) Allocate a buffer for conversion,
     * (4) Store converted data into the buffer.
     */
    switch (dwIndex)
    {
        case SCS_SETSTR:
            if (pComp)
            {
                if (bAnsiClient)
                {
                    cbCompNew = Imm32CompStrWideToAnsi(pComp, dwCompLen, NULL, 0, uCodePage);
                    pCompNew = Imm32HeapAlloc(0, cbCompNew);
                    if (!pCompNew)
                        goto Quit;

                    Imm32CompStrWideToAnsi(pComp, dwCompLen, pCompNew, cbCompNew, uCodePage);
                }
                else
                {
                    cbCompNew = Imm32CompStrAnsiToWide(pComp, dwCompLen, NULL, 0, uCodePage);
                    pCompNew = Imm32HeapAlloc(0, cbCompNew);
                    if (!pCompNew)
                        goto Quit;

                    Imm32CompStrAnsiToWide(pComp, dwCompLen, pCompNew, cbCompNew, uCodePage);
                }
            }

            if (pRead)
            {
                if (bAnsiClient)
                {
                    cbReadNew = Imm32CompStrWideToAnsi(pRead, dwReadLen, NULL, 0, uCodePage);
                    pReadNew = Imm32HeapAlloc(0, cbReadNew);
                    if (!pReadNew)
                        goto Quit;

                    Imm32CompStrWideToAnsi(pRead, dwReadLen, pReadNew, cbReadNew, uCodePage);
                }
                else
                {
                    cbReadNew = Imm32CompStrAnsiToWide(pRead, dwReadLen, NULL, 0, uCodePage);
                    pReadNew = Imm32HeapAlloc(0, cbReadNew);
                    if (!pReadNew)
                        goto Quit;

                    Imm32CompStrAnsiToWide(pRead, dwReadLen, pReadNew, cbReadNew, uCodePage);
                }
            }
            break;

        case SCS_CHANGEATTR:
            if (pComp)
            {
                if (bAnsiClient)
                {
                    cbCompNew = Imm32CompAttrWideToAnsi(pComp, dwCompLen,
                                                        CS_StrW(pCS, CompStr),
                                                        CS_SizeW(pCS, CompStr),
                                                        NULL, 0, uCodePage);
                    pCompNew = Imm32HeapAlloc(0, cbCompNew);
                    if (!pCompNew)
                        goto Quit;

                    Imm32CompAttrWideToAnsi(pComp, dwCompLen,
                                            CS_StrW(pCS, CompStr), CS_SizeW(pCS, CompStr),
                                            pCompNew, cbCompNew, uCodePage);
                }
                else
                {
                    cbCompNew = Imm32CompAttrAnsiToWide(pComp, dwCompLen,
                                                        CS_StrA(pCS, CompStr),
                                                        CS_SizeA(pCS, CompStr),
                                                        NULL, 0, uCodePage);
                    pCompNew = Imm32HeapAlloc(0, cbCompNew);
                    if (!pCompNew)
                        goto Quit;

                    Imm32CompAttrAnsiToWide(pComp, dwCompLen,
                                            CS_StrA(pCS, CompStr), CS_SizeA(pCS, CompStr),
                                            pCompNew, cbCompNew, uCodePage);
                }
            }

            if (pRead)
            {
                if (bAnsiClient)
                {
                    cbReadNew = Imm32CompAttrWideToAnsi(pRead, dwReadLen,
                                                        CS_StrW(pCS, CompReadStr),
                                                        CS_SizeW(pCS, CompReadStr),
                                                        NULL, 0, uCodePage);
                    pReadNew = Imm32HeapAlloc(0, cbReadNew);
                    if (!pReadNew)
                        goto Quit;

                    Imm32CompAttrWideToAnsi(pRead, dwReadLen,
                                            CS_StrW(pCS, CompReadStr), CS_SizeW(pCS, CompReadStr),
                                            pReadNew, cbReadNew, uCodePage);
                }
                else
                {
                    cbReadNew = Imm32CompAttrAnsiToWide(pRead, dwReadLen,
                                                        CS_StrA(pCS, CompReadStr),
                                                        CS_SizeA(pCS, CompReadStr),
                                                        NULL, 0, uCodePage);
                    pReadNew = Imm32HeapAlloc(0, cbReadNew);
                    if (!pReadNew)
                        goto Quit;

                    Imm32CompAttrAnsiToWide(pRead, dwReadLen,
                                            CS_StrA(pCS, CompReadStr), CS_SizeA(pCS, CompReadStr),
                                            pReadNew, cbReadNew, uCodePage);
                }
            }
            break;

        case SCS_CHANGECLAUSE:
            if (pComp)
            {
                if (bAnsiClient)
                {
                    cbCompNew = Imm32CompClauseWideToAnsi(pComp, dwCompLen, CS_StrW(pCS, CompStr),
                                                          NULL, 0, uCodePage);
                    pCompNew = Imm32HeapAlloc(0, cbCompNew);
                    if (!pCompNew)
                        goto Quit;

                    Imm32CompClauseWideToAnsi(pComp, dwCompLen, CS_StrW(pCS, CompStr),
                                              pCompNew, cbCompNew, uCodePage);
                }
                else
                {
                    cbCompNew = Imm32CompClauseAnsiToWide(pComp, dwCompLen, CS_StrA(pCS, CompStr),
                                                          NULL, 0, uCodePage);
                    pCompNew = Imm32HeapAlloc(0, cbCompNew);
                    if (!pCompNew)
                        goto Quit;

                    Imm32CompClauseAnsiToWide(pComp, dwCompLen, CS_StrA(pCS, CompStr),
                                              pCompNew, cbCompNew, uCodePage);
                }
            }

            if (pRead)
            {
                if (bAnsiClient)
                {
                    cbReadNew = Imm32CompClauseWideToAnsi(pRead, dwReadLen, CS_StrW(pCS, CompReadStr),
                                                          NULL, 0, uCodePage);
                    pReadNew = Imm32HeapAlloc(0, cbReadNew);
                    if (!pReadNew)
                        goto Quit;

                    Imm32CompClauseWideToAnsi(pRead, dwReadLen,
                                              CS_StrW(pCS, CompReadStr),
                                              pReadNew, cbReadNew, uCodePage);
                }
                else
                {
                    cbReadNew = Imm32CompClauseAnsiToWide(pRead, dwReadLen, CS_StrA(pCS, CompReadStr),
                                                          NULL, 0, uCodePage);
                    pReadNew = Imm32HeapAlloc(0, cbReadNew);
                    if (!pReadNew)
                        goto Quit;

                    Imm32CompClauseAnsiToWide(pRead, dwReadLen, CS_StrA(pCS, CompReadStr),
                                              pReadNew, cbReadNew, uCodePage);
                }
            }
            break;

        case SCS_SETRECONVERTSTRING: case SCS_QUERYRECONVERTSTRING:
        {
            if (pComp)
            {
                if (bAnsiClient)
                {
                    cbCompNew = Imm32ReconvertAnsiFromWide(NULL, pComp, uCodePage);
                    pCompNew = Imm32HeapAlloc(0, cbCompNew);
                    if (!pCompNew)
                        goto Quit;

                    pRS = pCompNew;
                    pRS->dwSize = cbCompNew;
                    pRS->dwVersion = 0;
                    Imm32ReconvertAnsiFromWide(pRS, pComp, uCodePage);
                }
                else
                {
                    cbCompNew = Imm32ReconvertWideFromAnsi(NULL, pComp, uCodePage);
                    pCompNew = Imm32HeapAlloc(0, cbCompNew);
                    if (!pCompNew)
                        goto Quit;

                    pRS = pCompNew;
                    pRS->dwSize = cbCompNew;
                    pRS->dwVersion = 0;
                    Imm32ReconvertWideFromAnsi(pRS, pComp, uCodePage);
                }
            }

            if (pRead)
            {
                if (bAnsiClient)
                {
                    cbReadNew = Imm32ReconvertAnsiFromWide(NULL, pRead, uCodePage);
                    pReadNew = Imm32HeapAlloc(0, cbReadNew);
                    if (!pReadNew)
                        goto Quit;

                    pRS = pReadNew;
                    pRS->dwSize = cbReadNew;
                    pRS->dwVersion = 0;
                    Imm32ReconvertAnsiFromWide(pRS, pRead, uCodePage);
                }
                else
                {
                    cbReadNew = Imm32ReconvertWideFromAnsi(NULL, pRead, uCodePage);
                    pReadNew = Imm32HeapAlloc(0, cbReadNew);
                    if (!pReadNew)
                        goto Quit;

                    pRS = pReadNew;
                    pRS->dwSize = cbReadNew;
                    pRS->dwVersion = 0;
                    Imm32ReconvertWideFromAnsi(pRS, pRead, uCodePage);
                }
            }
            break;
        }
    }

    ImmUnlockIMCC(pIC->hCompStr);
    pCS = NULL;
    ImmUnlockIMC(hIMC);
    pIC = NULL;

    ret = pImeDpi->ImeSetCompositionString(hIMC, dwIndex, pCompNew, cbCompNew,
                                           pReadNew, cbReadNew);

    if (dwIndex == SCS_QUERYRECONVERTSTRING)
    {
        if (pComp)
        {
            if (bAnsiClient)
                ret = Imm32ReconvertWideFromAnsi(pComp, pCompNew, uCodePage);
            else
                ret = Imm32ReconvertAnsiFromWide(pComp, pCompNew, uCodePage);
        }

        if (pRead)
        {
            if (bAnsiClient)
                ret = Imm32ReconvertWideFromAnsi(pRead, pReadNew, uCodePage);
            else
                ret = Imm32ReconvertAnsiFromWide(pRead, pReadNew, uCodePage);
        }
    }

Quit:
    if (pCS)
        ImmUnlockIMCC(pIC->hCompStr);
    if (pIC)
        ImmUnlockIMC(hIMC);
    Imm32HeapFree(pCompNew);
    Imm32HeapFree(pReadNew);
    ImmUnlockImeDpi(pImeDpi);
    return ret;
}

/***********************************************************************
 *		ImmGetCompositionStringA (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringA(HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
    LONG ret = 0;
    LPINPUTCONTEXT pIC;
    PCLIENTIMC pClientImc;
    LPCOMPOSITIONSTRING pCS;
    BOOL bAnsiClient;
    UINT uCodePage;

    TRACE("(%p, %lu, %p, %lu)\n", hIMC, dwIndex, lpBuf, dwBufLen);

    if (dwBufLen && !lpBuf)
        return 0;

    pClientImc = ImmLockClientImc(hIMC);
    if (!pClientImc)
        return 0;

    bAnsiClient = !(pClientImc->dwFlags & CLIENTIMC_WIDE);
    uCodePage = pClientImc->uCodePage;
    ImmUnlockClientImc(pClientImc);

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return 0;

    pCS = ImmLockIMCC(pIC->hCompStr);
    if (!pCS)
    {
        ImmUnlockIMC(hIMC);
        return 0;
    }

    ret = Imm32GetCompStrA(hIMC, pCS, dwIndex, lpBuf, dwBufLen, bAnsiClient, uCodePage);
    ImmUnlockIMCC(pIC->hCompStr);
    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmGetCompositionStringW (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringW(HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
    LONG ret = 0;
    LPINPUTCONTEXT pIC;
    PCLIENTIMC pClientImc;
    LPCOMPOSITIONSTRING pCS;
    BOOL bAnsiClient;
    UINT uCodePage;

    TRACE("(%p, %lu, %p, %lu)\n", hIMC, dwIndex, lpBuf, dwBufLen);

    if (dwBufLen && !lpBuf)
        return 0;

    pClientImc = ImmLockClientImc(hIMC);
    if (!pClientImc)
        return 0;

    bAnsiClient = !(pClientImc->dwFlags & CLIENTIMC_WIDE);
    uCodePage = pClientImc->uCodePage;
    ImmUnlockClientImc(pClientImc);

    pIC = ImmLockIMC(hIMC);
    if (!pIC)
        return 0;

    pCS = ImmLockIMCC(pIC->hCompStr);
    if (!pCS)
    {
        ImmUnlockIMC(hIMC);
        return 0;
    }

    ret = Imm32GetCompStrW(hIMC, pCS, dwIndex, lpBuf, dwBufLen, bAnsiClient, uCodePage);
    ImmUnlockIMCC(pIC->hCompStr);
    ImmUnlockIMC(hIMC);
    return ret;
}

/***********************************************************************
 *		ImmSetCompositionStringA (IMM32.@)
 */
BOOL WINAPI
ImmSetCompositionStringA(HIMC hIMC, DWORD dwIndex, LPVOID lpComp, DWORD dwCompLen,
                         LPVOID lpRead, DWORD dwReadLen)
{
    TRACE("(%p, %lu, %p, %lu, %p, %lu)\n",
          hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);
    return Imm32SetCompositionStringAW(hIMC, dwIndex, lpComp, dwCompLen,
                                       lpRead, dwReadLen, TRUE);
}

/***********************************************************************
 *		ImmSetCompositionStringW (IMM32.@)
 */
BOOL WINAPI
ImmSetCompositionStringW(HIMC hIMC, DWORD dwIndex, LPVOID lpComp, DWORD dwCompLen,
                         LPVOID lpRead, DWORD dwReadLen)
{
    TRACE("(%p, %lu, %p, %lu, %p, %lu)\n",
          hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);
    return Imm32SetCompositionStringAW(hIMC, dwIndex, lpComp, dwCompLen,
                                       lpRead, dwReadLen, FALSE);
}
