/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Utility functions
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/*************************************************************************
 *                SHCreatePropertyBag (SHELL32.715)
 */
EXTERN_C HRESULT
WINAPI
SHCreatePropertyBag(REFIID riid, void **ppvObj)
{
    return SHCreatePropertyBagOnMemory(STGM_READWRITE, riid, ppvObj);
}

/*************************************************************************
 *                SheRemoveQuotesA (SHELL32.@)
 */
EXTERN_C LPSTR
WINAPI
SheRemoveQuotesA(LPSTR psz)
{
    PCHAR pch;

    if (*psz == '"')
    {
        for (pch = psz + 1; *pch && *pch != '"'; ++pch)
        {
            *(pch - 1) = *pch;
        }

        if (*pch == '"')
            *(pch - 1) = ANSI_NULL;
    }

    return psz;
}

/*************************************************************************
 *                SheRemoveQuotesW (SHELL32.@)
 *
 * ExtractAssociatedIconExW uses this function.
 */
EXTERN_C LPWSTR
WINAPI
SheRemoveQuotesW(LPWSTR psz)
{
    PWCHAR pch;

    if (*psz == L'"')
    {
        for (pch = psz + 1; *pch && *pch != L'"'; ++pch)
        {
            *(pch - 1) = *pch;
        }

        if (*pch == L'"')
            *(pch - 1) = UNICODE_NULL;
    }

    return psz;
}
