/*
 * STRSAFE.LIB
 *
 * Copyright 2007 Dmitry Chapyshev <dmitry@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <windows.h>
#include <strsafe.h>

HRESULT
StringCopyWorkerA(LPTSTR pszDest,
                  size_t cchDest,
                  LPCTSTR pszSrc);
HRESULT
StringCopyWorkerW(LPWSTR  pszDest,
                  size_t cchDest,
                  LPCWSTR pszSrc);
HRESULT
StringCopyExWorkerA(LPTSTR pszDest,
                    size_t cchDest,
                    size_t cbDest,
                    LPCTSTR pszSrc,
                    LPTSTR *ppszDestEnd,
                    size_t *pcchRemaining,
                    unsigned long dwFlags);
HRESULT
StringCopyExWorkerW(LPWSTR  pszDest,
                    size_t cchDest,
                    size_t cbDest,
                    LPCWSTR pszSrc,
                    LPWSTR *ppszDestEnd,
                    size_t *pcchRemaining,
                    unsigned long dwFlags);
HRESULT
StringCopyNWorkerA(LPTSTR pszDest,
                   size_t cchDest,
                   LPCTSTR pszSrc,
                   size_t cchSrc);
HRESULT
StringCopyNWorkerW(LPWSTR  pszDest,
                   size_t cchDest,
                   LPCWSTR pszSrc,
                   size_t cchSrc);
HRESULT
StringCopyNExWorkerA(LPTSTR pszDest,
                     size_t cchDest,
                     size_t cbDest,
                     LPCTSTR pszSrc,
                     size_t cchSrc,
                     LPTSTR *ppszDestEnd,
                     size_t *pcchRemaining,
                     unsigned long dwFlags);
HRESULT
StringCopyNExWorkerW(LPWSTR  pszDest,
                     size_t cchDest,
                     size_t cbDest,
                     LPCWSTR pszSrc,
                     size_t cchSrc,
                     LPWSTR *ppszDestEnd,
                     size_t *pcchRemaining,
                     unsigned long dwFlags);
HRESULT
StringCatWorkerA(LPTSTR pszDest,
                 size_t cchDest,
                 LPCTSTR pszSrc);
HRESULT
StringCatWorkerW(LPWSTR  pszDest,
                 size_t cchDest,
                 LPCWSTR pszSrc);
HRESULT
StringCatExWorkerA(LPTSTR pszDest,
                   size_t cchDest,
                   size_t cbDest,
                   LPCTSTR pszSrc,
                   LPTSTR *ppszDestEnd,
                   size_t *pcchRemaining,
                   unsigned long dwFlags);
HRESULT
StringCatExWorkerW(LPWSTR  pszDest,
                   size_t cchDest,
                   size_t cbDest,
                   LPCWSTR pszSrc,
                   LPWSTR *ppszDestEnd,
                   size_t *pcchRemaining,
                   unsigned long dwFlags);
HRESULT
StringCatNWorkerA(LPTSTR pszDest,
                  size_t cchDest,
                  LPCTSTR pszSrc,
                  size_t cchMaxAppend);
HRESULT
StringCatNWorkerW(LPWSTR  pszDest,
                  size_t cchDest,
                  LPCWSTR pszSrc,
                  size_t cchMaxAppend);
HRESULT
StringCatNExWorkerA(LPTSTR pszDest,
                    size_t cchDest,
                    size_t cbDest,
                    LPCTSTR pszSrc,
                    size_t cchMaxAppend,
                    LPTSTR *ppszDestEnd,
                    size_t *pcchRemaining,
                    unsigned long dwFlags);
HRESULT
StringCatNExWorkerW(LPWSTR  pszDest,
                    size_t cchDest,
                    size_t cbDest,
                    LPCWSTR pszSrc,
                    size_t cchMaxAppend,
                    LPWSTR *ppszDestEnd,
                    size_t *pcchRemaining,
                    unsigned long dwFlags);
HRESULT
StringVPrintfWorkerA(LPTSTR pszDest,
                     size_t cchDest,
                     LPCTSTR pszFormat,
                     va_list argList);
HRESULT
StringVPrintfWorkerW(LPWSTR  pszDest,
                     size_t cchDest,
                     LPCWSTR pszFormat,
                     va_list argList);
HRESULT
StringVPrintfExWorkerA(LPTSTR pszDest,
                       size_t cchDest,
                       size_t cbDest,
                       LPTSTR *ppszDestEnd,
                       size_t *pcchRemaining,
                       unsigned long dwFlags,
                       LPCTSTR pszFormat,
                       va_list argList);
HRESULT
StringVPrintfExWorkerW(LPWSTR  pszDest,
                       size_t cchDest,
                       size_t cbDest,
                       LPWSTR *ppszDestEnd,
                       size_t *pcchRemaining,
                       unsigned long dwFlags,
                       LPCWSTR pszFormat,
                       va_list argList);
HRESULT
StringLengthWorkerA(LPCTSTR psz,
                    size_t cchMax,
                    size_t *pcch);
HRESULT
StringLengthWorkerW(LPCWSTR psz,
                    size_t cchMax,
                    size_t *pcch);

/**************************************************************************/
// Worker's functions
/**************************************************************************/
HRESULT
StringCopyWorkerA(LPTSTR pszDest,
                  size_t cchDest,
                  LPCTSTR pszSrc)
{
    HRESULT hr = S_OK;

    if (cchDest == 0) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        while (cchDest && (*pszSrc != '\0'))
        {
            *pszDest++ = *pszSrc++;
            cchDest--;
        }
        if (cchDest == 0)
        {
            pszDest--;
            hr = STRSAFE_E_INSUFFICIENT_BUFFER;
        }
        *pszDest= '\0';
    }
    return hr;
}

HRESULT
StringCopyWorkerW(LPWSTR  pszDest,
                  size_t cchDest,
                  LPCWSTR pszSrc)
{
    HRESULT hr = S_OK;

    if (cchDest == 0) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        while (cchDest && (*pszSrc != L'\0'))
        {
            *pszDest++ = *pszSrc++;
            cchDest--;
        }
        if (cchDest == 0)
        {
            pszDest--;
            hr = STRSAFE_E_INSUFFICIENT_BUFFER;
        }
        *pszDest= L'\0';
    }
    return hr;
}

HRESULT
StringCopyExWorkerA(LPTSTR pszDest,
                    size_t cchDest,
                    size_t cbDest,
                    LPCTSTR pszSrc,
                    LPTSTR *ppszDestEnd,
                    size_t* pcchRemaining,
                    unsigned long dwFlags)
{
    HRESULT hr = S_OK;
    LPTSTR pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    if (dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                    hr = STRSAFE_E_INVALID_PARAMETER;
            }
            if (pszSrc == NULL) pszSrc = "";
        }
        if (SUCCEEDED(hr))
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL)
                        hr = STRSAFE_E_INVALID_PARAMETER;
                    else
                        hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                }
            }
            else
            {
                pszDestEnd = pszDest;
                cchRemaining = cchDest;

                while (cchRemaining && (*pszSrc != '\0'))
                {
                    *pszDestEnd++= *pszSrc++;
                    cchRemaining--;
                }

                if (cchRemaining > 0)
                {
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(char)) + (cbDest % sizeof(char)));
                }
                else
                {
                    pszDestEnd--;
                    cchRemaining++;
                    hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                }
                *pszDestEnd = '\0';
            }
        }
    }
    if (FAILED(hr))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;
                    *pszDestEnd = '\0';
                }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = '\0';
                }
            }
        }
    }
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd) *ppszDestEnd = pszDestEnd;
        if (pcchRemaining) *pcchRemaining = cchRemaining;
    }
    return hr;
}

HRESULT
StringCopyExWorkerW(LPWSTR pszDest,
                    size_t cchDest,
                    size_t cbDest,
                    LPCWSTR pszSrc,
                    LPWSTR *ppszDestEnd,
                    size_t* pcchRemaining,
                    unsigned long dwFlags)
{
    HRESULT hr = S_OK;
    LPWSTR  pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    if (dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                    hr = STRSAFE_E_INVALID_PARAMETER;
            }

            if (pszSrc == NULL) pszSrc = L"";
        }
        if (SUCCEEDED(hr))
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                        hr = STRSAFE_E_INVALID_PARAMETER;
                    else
                        hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                }
            }
            else
            {
                pszDestEnd = pszDest;
                cchRemaining = cchDest;
                while (cchRemaining && (*pszSrc != L'\0'))
                {
                    *pszDestEnd++= *pszSrc++;
                    cchRemaining--;
                }
                if (cchRemaining > 0)
                {
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));
                }
                else
                {
                    pszDestEnd--;
                    cchRemaining++;
                    hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                }
                *pszDestEnd = L'\0';
            }
        }
    }
    if (FAILED(hr))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;
                    *pszDestEnd = L'\0';
                }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = L'\0';
                }
            }
        }
    }
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd) *ppszDestEnd = pszDestEnd;
        if (pcchRemaining) *pcchRemaining = cchRemaining;
    }
    return hr;
}

HRESULT
StringCopyNWorkerA(LPTSTR pszDest,
                   size_t cchDest,
                   LPCTSTR pszSrc,
                   size_t cchSrc)
{
    HRESULT hr = S_OK;

    if (cchDest == 0) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        while (cchDest && cchSrc && (*pszSrc != '\0'))
        {
            *pszDest++= *pszSrc++;
            cchDest--;
            cchSrc--;
        }
        if (cchDest == 0)
        {
            pszDest--;
            hr = STRSAFE_E_INSUFFICIENT_BUFFER;
        }
        *pszDest= '\0';
    }
    return hr;
}

HRESULT
StringCopyNWorkerW(LPWSTR pszDest,
                   size_t cchDest,
                   LPCWSTR pszSrc,
                   size_t cchSrc)
{
    HRESULT hr = S_OK;

    if (cchDest == 0) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        while (cchDest && cchSrc && (*pszSrc != L'\0'))
        {
            *pszDest++= *pszSrc++;
            cchDest--;
            cchSrc--;
        }
        if (cchDest == 0)
        {
            pszDest--;
            hr = STRSAFE_E_INSUFFICIENT_BUFFER;
        }
        *pszDest= L'\0';
    }
    return hr;
}

HRESULT
StringCopyNExWorkerA(LPTSTR pszDest,
                     size_t cchDest,
                     size_t cbDest,
                     LPCTSTR pszSrc,
                     size_t cchSrc,
                     LPTSTR *ppszDestEnd,
                     size_t* pcchRemaining,
                     unsigned long dwFlags)
{
    HRESULT hr = S_OK;
    LPTSTR pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    if (dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0)) hr = STRSAFE_E_INVALID_PARAMETER;
            }

            if (pszSrc == NULL) pszSrc = "";
        }
        if (SUCCEEDED(hr))
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL) hr = STRSAFE_E_INVALID_PARAMETER;
                    else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                }
            }
            else
            {
                pszDestEnd = pszDest;
                cchRemaining = cchDest;
                while (cchRemaining && cchSrc && (*pszSrc != '\0'))
                {
                    *pszDestEnd++= *pszSrc++;
                    cchRemaining--;
                    cchSrc--;
                }
                if (cchRemaining > 0)
                {
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(char)) + (cbDest % sizeof(char)));
                }
                else
                {
                    pszDestEnd--;
                    cchRemaining++;
                    hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                }
                *pszDestEnd = '\0';
            }
        }
    }
    if (FAILED(hr))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;
                    *pszDestEnd = '\0';
                }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = '\0';
                }
            }
        }
    }
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd) *ppszDestEnd = pszDestEnd;
        if (pcchRemaining) *pcchRemaining = cchRemaining;
    }
    return hr;
}

HRESULT
StringCopyNExWorkerW(LPWSTR pszDest,
                     size_t cchDest,
                     size_t cbDest,
                     LPCWSTR pszSrc,
                     size_t cchSrc,
                     LPWSTR *ppszDestEnd,
                     size_t* pcchRemaining,
                     unsigned long dwFlags)
{
    HRESULT hr = S_OK;
    LPWSTR  pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    if (dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0)) hr = STRSAFE_E_INVALID_PARAMETER;
            }
            if (pszSrc == NULL) pszSrc = L"";
        }
        if (SUCCEEDED(hr))
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL) hr = STRSAFE_E_INVALID_PARAMETER;
                    else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                }
            }
            else
            {
                pszDestEnd = pszDest;
                cchRemaining = cchDest;
                while (cchRemaining && cchSrc && (*pszSrc != L'\0'))
                {
                    *pszDestEnd++= *pszSrc++;
                    cchRemaining--;
                    cchSrc--;
                }
                if (cchRemaining > 0)
                {
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));
                }
                else
                {
                    pszDestEnd--;
                    cchRemaining++;
                    hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                }
                *pszDestEnd = L'\0';
            }
        }
    }
    if (FAILED(hr))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;
                    *pszDestEnd = L'\0';
                }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = L'\0';
                }
            }
        }
    }
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd) *ppszDestEnd = pszDestEnd;
        if (pcchRemaining) *pcchRemaining = cchRemaining;
    }
    return hr;
}

HRESULT
StringCatWorkerA(LPTSTR pszDest,
                 size_t cchDest,
                 LPCTSTR pszSrc)
{
   HRESULT hr;
   size_t cchDestCurrent;

   hr = StringLengthWorkerA(pszDest, cchDest, &cchDestCurrent);
   if (SUCCEEDED(hr))
       hr = StringCopyWorkerA(pszDest + cchDestCurrent, cchDest - cchDestCurrent, pszSrc);
   return hr;
}

HRESULT
StringCatWorkerW(LPWSTR pszDest,
                 size_t cchDest,
                 LPCWSTR pszSrc)
{
   HRESULT hr;
   size_t cchDestCurrent;

   hr = StringLengthWorkerW(pszDest, cchDest, &cchDestCurrent);
   if (SUCCEEDED(hr))
       hr = StringCopyWorkerW(pszDest + cchDestCurrent, cchDest - cchDestCurrent, pszSrc);
   return hr;
}

HRESULT
StringCatExWorkerA(LPTSTR pszDest,
                   size_t cchDest,
                   size_t cbDest,
                   LPCTSTR pszSrc,
                   LPTSTR *ppszDestEnd,
                   size_t* pcchRemaining,
                   unsigned long dwFlags)
{
    HRESULT hr = S_OK;
    LPTSTR pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    if (dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cchDestCurrent;
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest == 0) && (cbDest == 0))
                    cchDestCurrent = 0;
                else
                    hr = STRSAFE_E_INVALID_PARAMETER;
            }
            else
            {
                hr = StringLengthWorkerA(pszDest, cchDest, &cchDestCurrent);
                if (SUCCEEDED(hr))
                {
                    pszDestEnd = pszDest + cchDestCurrent;
                    cchRemaining = cchDest - cchDestCurrent;
                }
            }
            if (pszSrc == NULL) pszSrc = "";
        }
        else
        {
            hr = StringLengthWorkerA(pszDest, cchDest, &cchDestCurrent);
            if (SUCCEEDED(hr))
            {
                pszDestEnd = pszDest + cchDestCurrent;
                cchRemaining = cchDest - cchDestCurrent;
            }
        }
        if (SUCCEEDED(hr))
        {
            if (cchDest == 0)
            {
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL) hr = STRSAFE_E_INVALID_PARAMETER;
                    else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                }
            }
            else
            {
                hr = StringCopyExWorkerA(pszDestEnd,
                                         cchRemaining,
                                         (cchRemaining * sizeof(char)) + (cbDest % sizeof(char)),
                                         pszSrc,
                                         &pszDestEnd,
                                         &cchRemaining,
                                         dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
            }
        }
    }
    if (FAILED(hr))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;
                    *pszDestEnd = '\0';
                }
            }
            if (dwFlags & STRSAFE_NULL_ON_FAILURE)
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = '\0';
                }
            }
        }
    }
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd) *ppszDestEnd = pszDestEnd;
        if (pcchRemaining) *pcchRemaining = cchRemaining;
    }
    return hr;
}

HRESULT
StringCatExWorkerW(LPWSTR pszDest,
                   size_t cchDest,
                   size_t cbDest,
                   LPCWSTR pszSrc,
                   LPWSTR *ppszDestEnd,
                   size_t* pcchRemaining,
                   unsigned long dwFlags)
{
    HRESULT hr = S_OK;
    LPWSTR  pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    if (dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cchDestCurrent;
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest == 0) && (cbDest == 0)) cchDestCurrent = 0;
                else hr = STRSAFE_E_INVALID_PARAMETER;
            }
            else
            {
                hr = StringLengthWorkerW(pszDest, cchDest, &cchDestCurrent);
                if (SUCCEEDED(hr))
                {
                    pszDestEnd = pszDest + cchDestCurrent;
                    cchRemaining = cchDest - cchDestCurrent;
                }
            }
            if (pszSrc == NULL) pszSrc = L"";
        }
        else
        {
            hr = StringLengthWorkerW(pszDest, cchDest, &cchDestCurrent);
            if (SUCCEEDED(hr))
            {
                pszDestEnd = pszDest + cchDestCurrent;
                cchRemaining = cchDest - cchDestCurrent;
            }
        }
        if (SUCCEEDED(hr))
        {
            if (cchDest == 0)
            {
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL) hr = STRSAFE_E_INVALID_PARAMETER;
                    else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                }
            }
            else
            {
                hr = StringCopyExWorkerW(pszDestEnd,
                                         cchRemaining,
                                         (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)),
                                         pszSrc,
                                         &pszDestEnd,
                                         &cchRemaining,
                                         dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
            }
        }
    }
    if (FAILED(hr))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;
                    *pszDestEnd = L'\0';
                }
            }
            if (dwFlags & STRSAFE_NULL_ON_FAILURE)
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = L'\0';
                }
            }
        }
    }
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd) *ppszDestEnd = pszDestEnd;
        if (pcchRemaining) *pcchRemaining = cchRemaining;
    }
    return hr;
}

HRESULT
StringCatNWorkerA(LPTSTR pszDest,
                  size_t cchDest,
                  LPCTSTR pszSrc,
                  size_t cchMaxAppend)
{
    HRESULT hr;
    size_t cchDestCurrent;

    hr = StringLengthWorkerA(pszDest, cchDest, &cchDestCurrent);
    if (SUCCEEDED(hr))
        hr = StringCopyNWorkerA(pszDest + cchDestCurrent, cchDest - cchDestCurrent, pszSrc, cchMaxAppend);
    return hr;
}

HRESULT
StringCatNWorkerW(LPWSTR pszDest,
                  size_t cchDest,
                  LPCWSTR pszSrc,
                  size_t cchMaxAppend)
{
    HRESULT hr;
    size_t cchDestCurrent;

    hr = StringLengthWorkerW(pszDest, cchDest, &cchDestCurrent);
    if (SUCCEEDED(hr))
        hr = StringCopyNWorkerW(pszDest + cchDestCurrent, cchDest - cchDestCurrent, pszSrc, cchMaxAppend);
    return hr;
}

HRESULT
StringCatNExWorkerA(LPTSTR pszDest,
                    size_t cchDest,
                    size_t cbDest,
                    LPCTSTR pszSrc,
                    size_t cchMaxAppend,
                    LPTSTR *ppszDestEnd,
                    size_t* pcchRemaining,
                    unsigned long dwFlags)
{
    HRESULT hr = S_OK;
    LPTSTR pszDestEnd = pszDest;
    size_t cchRemaining = 0;
    size_t cchDestCurrent = 0;

    if (dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest == 0) && (cbDest == 0)) cchDestCurrent = 0;
                else hr = STRSAFE_E_INVALID_PARAMETER;
            }
            else
            {
                hr = StringLengthWorkerA(pszDest, cchDest, &cchDestCurrent);
                if (SUCCEEDED(hr))
                {
                    pszDestEnd = pszDest + cchDestCurrent;
                    cchRemaining = cchDest - cchDestCurrent;
                }
            }
            if (pszSrc == NULL) pszSrc = "";
        }
        else
        {
            hr = StringLengthWorkerA(pszDest, cchDest, &cchDestCurrent);
            if (SUCCEEDED(hr))
            {
                pszDestEnd = pszDest + cchDestCurrent;
                cchRemaining = cchDest - cchDestCurrent;
            }
        }
        if (SUCCEEDED(hr))
        {
            if (cchDest == 0)
            {
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL) hr = STRSAFE_E_INVALID_PARAMETER;
                    else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                }
            }
            else
            {
                hr = StringCopyNExWorkerA(pszDestEnd,
                                          cchRemaining,
                                          (cchRemaining * sizeof(char)) + (cbDest % sizeof(char)),
                                          pszSrc,
                                          cchMaxAppend,
                                          &pszDestEnd,
                                          &cchRemaining,
                                          dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
            }
        }
    }

    if (FAILED(hr))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;
                    *pszDestEnd = '\0';
                }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = '\0';
                }
            }
        }
    }
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd) *ppszDestEnd = pszDestEnd;
        if (pcchRemaining) *pcchRemaining = cchRemaining;
    }
    return hr;
}

HRESULT
StringCatNExWorkerW(LPWSTR pszDest,
                    size_t cchDest,
                    size_t cbDest,
                    LPCWSTR pszSrc,
                    size_t cchMaxAppend,
                    LPWSTR *ppszDestEnd,
                    size_t* pcchRemaining,
                    unsigned long dwFlags)
{
    HRESULT hr = S_OK;
    LPWSTR  pszDestEnd = pszDest;
    size_t cchRemaining = 0;
    size_t cchDestCurrent = 0;

    if (dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest == 0) && (cbDest == 0)) cchDestCurrent = 0;
                else hr = STRSAFE_E_INVALID_PARAMETER;
            }
            else
            {
                hr = StringLengthWorkerW(pszDest, cchDest, &cchDestCurrent);
                if (SUCCEEDED(hr))
                {
                    pszDestEnd = pszDest + cchDestCurrent;
                    cchRemaining = cchDest - cchDestCurrent;
                }
            }
            if (pszSrc == NULL) pszSrc = L"";
        }
        else
        {
            hr = StringLengthWorkerW(pszDest, cchDest, &cchDestCurrent);
            if (SUCCEEDED(hr))
            {
                pszDestEnd = pszDest + cchDestCurrent;
                cchRemaining = cchDest - cchDestCurrent;
            }
        }
        if (SUCCEEDED(hr))
        {
            if (cchDest == 0)
            {
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL) hr = STRSAFE_E_INVALID_PARAMETER;
                    else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                }
            }
            else
            {
                hr = StringCopyNExWorkerW(pszDestEnd,
                                          cchRemaining,
                                          (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)),
                                          pszSrc,
                                          cchMaxAppend,
                                          &pszDestEnd,
                                          &cchRemaining,
                                          dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
            }
        }
    }
    if (FAILED(hr))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;
                    *pszDestEnd = L'\0';
                }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = L'\0';
                }
            }
        }
    }
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd) *ppszDestEnd = pszDestEnd;
        if (pcchRemaining) *pcchRemaining = cchRemaining;
    }
    return hr;
}

HRESULT
StringVPrintfWorkerA(LPTSTR pszDest,
                     size_t cchDest,
                     LPCTSTR pszFormat,
                     va_list argList)
{
    HRESULT hr = S_OK;

    if (cchDest == 0) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        int iRet;
        size_t cchMax;
        cchMax = cchDest - 1;
        iRet = _vsnprintf(pszDest, cchMax, pszFormat, argList);
        if ((iRet < 0) || (((size_t)iRet) > cchMax))
        {
            pszDest += cchMax;
            *pszDest = '\0';
            hr = STRSAFE_E_INSUFFICIENT_BUFFER;
        }
        else if (((size_t)iRet) == cchMax)
        {
            pszDest += cchMax;
            *pszDest = '\0';
        }
    }
    return hr;
}

HRESULT
StringVPrintfWorkerW(LPWSTR pszDest,
                     size_t cchDest,
                     LPCWSTR pszFormat,
                     va_list argList)
{
    HRESULT hr = S_OK;

    if (cchDest == 0) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        int iRet;
        size_t cchMax;
        cchMax = cchDest - 1;
        iRet = _vsnwprintf(pszDest, cchMax, pszFormat, argList);
        if ((iRet < 0) || (((size_t)iRet) > cchMax))
        {
            pszDest += cchMax;
            *pszDest = L'\0';
            hr = STRSAFE_E_INSUFFICIENT_BUFFER;
        }
        else if (((size_t)iRet) == cchMax)
        {
            pszDest += cchMax;
            *pszDest = L'\0';
        }
    }
    return hr;
}

HRESULT
StringVPrintfExWorkerA(LPTSTR pszDest,
                       size_t cchDest,
                       size_t cbDest,
                       LPTSTR *ppszDestEnd,
                       size_t* pcchRemaining,
                       unsigned long dwFlags,
                       LPCTSTR pszFormat,
                       va_list argList)
{
    HRESULT hr = S_OK;
    LPTSTR pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    if (dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                    hr = STRSAFE_E_INVALID_PARAMETER;
            }
            if (pszFormat == NULL) pszFormat = "";
        }
        if (SUCCEEDED(hr))
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;
                if (*pszFormat != '\0')
                {
                    if (pszDest == NULL) hr = STRSAFE_E_INVALID_PARAMETER;
                    else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                }
            }
            else
            {
                int iRet;
                size_t cchMax;
                cchMax = cchDest - 1;
                iRet = _vsnprintf(pszDest, cchMax, pszFormat, argList);
                if ((iRet < 0) || (((size_t)iRet) > cchMax))
                {
                    pszDestEnd = pszDest + cchMax;
                    cchRemaining = 1;
                    *pszDestEnd = '\0';
                    hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                }
                else if (((size_t)iRet) == cchMax)
                {
                    pszDestEnd = pszDest + cchMax;
                    cchRemaining = 1;
                    *pszDestEnd = '\0';
                }
                else if (((size_t)iRet) < cchMax)
                {
                    pszDestEnd = pszDest + iRet;
                    cchRemaining = cchDest - iRet;
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(char)) + (cbDest % sizeof(char)));
                }
            }
        }
    }
    if (FAILED(hr))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;
                    *pszDestEnd = '\0';
                }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = '\0';
                }
            }
        }
    }
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd) *ppszDestEnd = pszDestEnd;
        if (pcchRemaining) *pcchRemaining = cchRemaining;
    }
    return hr;
}

HRESULT
StringVPrintfExWorkerW(LPWSTR pszDest,
                       size_t cchDest,
                       size_t cbDest,
                       LPWSTR *ppszDestEnd,
                       size_t* pcchRemaining,
                       unsigned long dwFlags,
                       LPCWSTR pszFormat,
                       va_list argList)
{
    HRESULT hr = S_OK;
    LPWSTR  pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    if (dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                    hr = STRSAFE_E_INVALID_PARAMETER;
            }

            if (pszFormat == NULL) pszFormat = L"";
        }
        if (SUCCEEDED(hr))
        {
            if (cchDest == 0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;
                if (*pszFormat != L'\0')
                {
                    if (pszDest == NULL) hr = STRSAFE_E_INVALID_PARAMETER;
                    else hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                }
            }
            else
            {
                int iRet;
                size_t cchMax;
                cchMax = cchDest - 1;
                iRet = _vsnwprintf(pszDest, cchMax, pszFormat, argList);
                if ((iRet < 0) || (((size_t)iRet) > cchMax))
                {
                    pszDestEnd = pszDest + cchMax;
                    cchRemaining = 1;
                    *pszDestEnd = L'\0';
                    hr = STRSAFE_E_INSUFFICIENT_BUFFER;
                }
                else if (((size_t)iRet) == cchMax)
                {
                    pszDestEnd = pszDest + cchMax;
                    cchRemaining = 1;
                    *pszDestEnd = L'\0';
                }
                else if (((size_t)iRet) < cchMax)
                {
                    pszDestEnd = pszDest + iRet;
                    cchRemaining = cchDest - iRet;
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));
                }
            }
        }
    }
    if (FAILED(hr))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;
                    *pszDestEnd = L'\0';
                }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = L'\0';
                }
            }
        }
    }
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (ppszDestEnd) *ppszDestEnd = pszDestEnd;
        if (pcchRemaining) *pcchRemaining = cchRemaining;
    }
    return hr;
}

HRESULT
StringLengthWorkerA(LPCTSTR psz,
                    size_t cchMax,
                    size_t* pcch)
{
    HRESULT hr = S_OK;
    size_t cchMaxPrev = cchMax;

    while (cchMax && (*psz != '\0')) psz++, cchMax--;
    if (cchMax == 0) hr = STRSAFE_E_INVALID_PARAMETER;
    if (SUCCEEDED(hr) && pcch) *pcch = cchMaxPrev - cchMax;
    return hr;
}

HRESULT
StringLengthWorkerW(LPCWSTR psz,
                    size_t cchMax,
                    size_t* pcch)
{
    HRESULT hr = S_OK;
    size_t cchMaxPrev = cchMax;

    while (cchMax && (*psz != L'\0')) psz++, cchMax--;
    if (cchMax == 0) hr = STRSAFE_E_INVALID_PARAMETER;
    if (SUCCEEDED(hr) && pcch) *pcch = cchMaxPrev - cchMax;
    return hr;
}

STRSAFE_INLINE_API
StringGetsExWorkerA(LPTSTR pszDest,
                    size_t cchDest,
                    size_t cbDest,
                    LPTSTR *ppszDestEnd,
                    size_t* pcchRemaining,
                    unsigned long dwFlags)
{
    HRESULT hr = S_OK;
    LPTSTR pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    if (dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                    hr = STRSAFE_E_INVALID_PARAMETER;
            }
        }
        if (SUCCEEDED(hr))
        {
            if (cchDest <= 1)
            {
                pszDestEnd = pszDest;
                cchRemaining = cchDest;
                if (cchDest == 1) *pszDestEnd = '\0';
                hr = STRSAFE_E_INSUFFICIENT_BUFFER;
            }
            else
            {
                char ch;
                pszDestEnd = pszDest;
                cchRemaining = cchDest;
                while ((cchRemaining > 1) && (ch = (char)getc(stdin)) != '\n')
                {
                    if (ch == EOF)
                    {
                        if (pszDestEnd == pszDest)
                            hr = STRSAFE_E_END_OF_FILE;
                        break;
                    }
                    *pszDestEnd = ch;
                    pszDestEnd++;
                    cchRemaining--;
                }
                if (cchRemaining > 0)
                {
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(char)) + (cbDest % sizeof(char)));
                }
                *pszDestEnd = '\0';
            }
        }
    }
    if (FAILED(hr))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;
                    *pszDestEnd = '\0';
                }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = '\0';
                }
            }
        }
    }
    if (SUCCEEDED(hr) ||
        (hr == STRSAFE_E_INSUFFICIENT_BUFFER) ||
        (hr == STRSAFE_E_END_OF_FILE))
    {
        if (ppszDestEnd) *ppszDestEnd = pszDestEnd;
        if (pcchRemaining) *pcchRemaining = cchRemaining;
    }
    return hr;
}

STRSAFE_INLINE_API
StringGetsExWorkerW(LPWSTR pszDest,
                    size_t cchDest,
                    size_t cbDest,
                    LPWSTR *ppszDestEnd,
                    size_t* pcchRemaining,
                    unsigned long dwFlags)
{
    HRESULT hr = S_OK;
    LPWSTR  pszDestEnd = pszDest;
    size_t cchRemaining = 0;

    if (dwFlags & (~STRSAFE_VALID_FLAGS)) hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (pszDest == NULL)
            {
                if ((cchDest != 0) || (cbDest != 0))
                    hr = STRSAFE_E_INVALID_PARAMETER;
            }
        }

        if (SUCCEEDED(hr))
        {
            if (cchDest <= 1)
            {
                pszDestEnd = pszDest;
                cchRemaining = cchDest;
                if (cchDest == 1) *pszDestEnd = L'\0';
                hr = STRSAFE_E_INSUFFICIENT_BUFFER;
            }
            else
            {
                wchar_t ch;
                pszDestEnd = pszDest;
                cchRemaining = cchDest;
                while ((cchRemaining > 1) && (ch = (wchar_t)getwc(stdin)) != L'\n')
                {
                    if (ch == (wchar_t)EOF)
                    {
                        if (pszDestEnd == pszDest)
                            hr = STRSAFE_E_END_OF_FILE;
                        break;
                    }
                    *pszDestEnd = ch;
                    pszDestEnd++;
                    cchRemaining--;
                }
                if (cchRemaining > 0)
                {
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), ((cchRemaining - 1) * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));
                }
                *pszDestEnd = L'\0';
            }
        }
    }
    if (FAILED(hr))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else if (cchDest > 0)
                {
                    pszDestEnd = pszDest + cchDest - 1;
                    cchRemaining = 1;
                    *pszDestEnd = L'\0';
                }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = L'\0';
                }
            }
        }
    }
    if (SUCCEEDED(hr) ||
        (hr == STRSAFE_E_INSUFFICIENT_BUFFER) ||
        (hr == STRSAFE_E_END_OF_FILE))
    {
        if (ppszDestEnd) *ppszDestEnd = pszDestEnd;
        if (pcchRemaining) *pcchRemaining = cchRemaining;
    }
    return hr;
}

STRSAFEAPI
StringCchCatA(LPTSTR pszDest,
             size_t cchDest,
             LPCTSTR pszSrc)
{
    if (cchDest > STRSAFE_MAX_CCH)
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringCatWorkerA(pszDest, cchDest, pszSrc);
}

STRSAFEAPI
StringCchCatW(LPWSTR pszDest,
             size_t cchDest,
             LPCWSTR pszSrc)
{
    if (cchDest > STRSAFE_MAX_CCH)
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringCatWorkerW(pszDest, cchDest, pszSrc);
}

STRSAFEAPI
StringCchCatExA(LPTSTR pszDest,
               size_t cchDest,
               LPCTSTR pszSrc,
               LPTSTR *ppszDestEnd,
               size_t *pcchRemaining,
               DWORD dwFlags)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cbDest;
        cbDest = cchDest * sizeof(char);
        hr = StringCatExWorkerA(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, pcchRemaining, dwFlags);
    }
    return hr;
}

STRSAFEAPI
StringCchCatExW(LPWSTR pszDest,
               size_t cchDest,
               LPCWSTR pszSrc,
               LPWSTR *ppszDestEnd,
               size_t *pcchRemaining,
               DWORD dwFlags)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cbDest;
        cbDest = cchDest * sizeof(wchar_t);
        hr = StringCatExWorkerW(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return hr;
}

STRSAFEAPI
StringCchCatNA(LPTSTR pszDest,
              size_t cchDest,
              LPCTSTR pszSrc,
              size_t cchMaxAppend)
{
    if (cchDest > STRSAFE_MAX_CCH)
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringCatNWorkerA(pszDest, cchDest, pszSrc, cchMaxAppend);
}

STRSAFEAPI
StringCchCatNW(LPWSTR pszDest,
              size_t cchDest,
              LPCWSTR pszSrc,
              size_t cchMaxAppend)
{
    if (cchDest > STRSAFE_MAX_CCH)
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringCatNWorkerW(pszDest, cchDest, pszSrc, cchMaxAppend);
}

STRSAFEAPI
StringCchCatNExA(LPTSTR pszDest,
                size_t cchDest,
                LPCTSTR pszSrc,
                size_t cchMaxAppend,
                LPTSTR *ppszDestEnd,
                size_t *pcchRemaining,
                DWORD dwFlags)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cbDest;
        cbDest = cchDest * sizeof(char);
        hr = StringCatNExWorkerA(pszDest, cchDest, cbDest, pszSrc, cchMaxAppend, ppszDestEnd, pcchRemaining, dwFlags);
    }
    return hr;
}

STRSAFEAPI
StringCchCatNExW(LPWSTR pszDest,
                size_t cchDest,
                LPCWSTR pszSrc,
                size_t cchMaxAppend,
                LPWSTR *ppszDestEnd,
                size_t *pcchRemaining,
                DWORD dwFlags)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cbDest;
        cbDest = cchDest * sizeof(wchar_t);
        hr = StringCatNExWorkerW(pszDest, cchDest, cbDest, pszSrc, cchMaxAppend, ppszDestEnd, pcchRemaining, dwFlags);
    }

    return hr;
}

STRSAFEAPI
StringCchCopyA(LPTSTR pszDest,
              size_t cchDest,
              LPCTSTR pszSrc)
{
    if (cchDest > STRSAFE_MAX_CCH)
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringCopyWorkerA(pszDest, cchDest, pszSrc);
}

STRSAFEAPI
StringCchCopyW(LPWSTR pszDest,
              size_t cchDest,
              LPCWSTR pszSrc)
{
    if (cchDest > STRSAFE_MAX_CCH)
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringCopyWorkerW(pszDest, cchDest, pszSrc);
}

STRSAFEAPI
StringCchCopyExA(LPTSTR pszDest,
                size_t cchDest,
                LPCTSTR pszSrc,
                LPTSTR *ppszDestEnd,
                size_t *pcchRemaining,
                DWORD dwFlags)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cbDest;
        cbDest = cchDest * sizeof(char);
        hr = StringCopyExWorkerA(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, pcchRemaining, dwFlags);
    }
    return hr;
}

STRSAFEAPI
StringCchCopyExW(LPWSTR pszDest,
                size_t cchDest,
                LPCWSTR pszSrc,
                LPWSTR *ppszDestEnd,
                size_t *pcchRemaining,
                DWORD dwFlags)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cbDest;
        cbDest = cchDest * sizeof(wchar_t);
        hr = StringCopyExWorkerW(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, pcchRemaining, dwFlags);
    }
    return hr;
}

STRSAFEAPI
StringCchCopyNA(LPTSTR pszDest,
               size_t cchDest,
               LPCTSTR pszSrc,
               size_t cchSrc)
{
    if ((cchDest > STRSAFE_MAX_CCH) || (cchSrc > STRSAFE_MAX_CCH))
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringCopyNWorkerA(pszDest, cchDest, pszSrc, cchSrc);
}

STRSAFEAPI
StringCchCopyNW(LPWSTR pszDest,
               size_t cchDest,
               LPCWSTR pszSrc,
               size_t cchSrc)
{
    if ((cchDest > STRSAFE_MAX_CCH) || (cchSrc > STRSAFE_MAX_CCH))
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringCopyNWorkerW(pszDest, cchDest, pszSrc, cchSrc);
}

STRSAFEAPI
StringCchCopyNExA(LPTSTR pszDest,
                 size_t cchDest,
                 LPCTSTR pszSrc,
                 size_t cchSrc,
                 LPTSTR *ppszDestEnd,
                 size_t *pcchRemaining,
                 DWORD dwFlags)
{
    HRESULT hr;

    if ((cchDest > STRSAFE_MAX_CCH) ||
        (cchSrc > STRSAFE_MAX_CCH))
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cbDest;
        cbDest = cchDest * sizeof(char);
        hr = StringCopyNExWorkerA(pszDest, cchDest, cbDest, pszSrc, cchSrc, ppszDestEnd, pcchRemaining, dwFlags);
    }
    return hr;
}

STRSAFEAPI
StringCchCopyNExW(LPWSTR pszDest,
                 size_t cchDest,
                 LPCWSTR pszSrc,
                 size_t cchSrc,
                 LPWSTR *ppszDestEnd,
                 size_t *pcchRemaining,
                 DWORD dwFlags)
{
    HRESULT hr;

    if ((cchDest > STRSAFE_MAX_CCH) ||
        (cchSrc > STRSAFE_MAX_CCH))
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cbDest;
        cbDest = cchDest * sizeof(wchar_t);
        hr = StringCopyNExWorkerW(pszDest, cchDest, cbDest, pszSrc, cchSrc, ppszDestEnd, pcchRemaining, dwFlags);
    }
    return hr;
}

STRSAFE_INLINE_API
StringCchGetsA(LPTSTR pszDest,
              size_t cchDest)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cbDest;
        cbDest = cchDest * sizeof(char);
        hr = StringGetsExWorkerA(pszDest, cchDest, cbDest, NULL, NULL, 0);
    }
    return hr;
}

STRSAFE_INLINE_API
StringCchGetsW(LPWSTR pszDest,
              size_t cchDest)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cbDest;
        cbDest = cchDest * sizeof(wchar_t);
        hr = StringGetsExWorkerW(pszDest, cchDest, cbDest, NULL, NULL, 0);
    }
    return hr;
}

STRSAFE_INLINE_API
StringCchGetsExA(LPTSTR pszDest,
                size_t cchDest,
                LPTSTR *ppszDestEnd,
                size_t *pcchRemaining,
                DWORD dwFlags)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cbDest;
        cbDest = cchDest * sizeof(char);
        hr = StringGetsExWorkerA(pszDest, cchDest, cbDest, ppszDestEnd, pcchRemaining, dwFlags);
    }
    return hr;
}

STRSAFE_INLINE_API
StringCchGetsExW(LPWSTR pszDest,
                size_t cchDest,
                LPWSTR *ppszDestEnd,
                size_t *pcchRemaining,
                DWORD dwFlags)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cbDest;
        cbDest = cchDest * sizeof(wchar_t);
        hr = StringGetsExWorkerW(pszDest, cchDest, cbDest, ppszDestEnd, pcchRemaining, dwFlags);
    }
    return hr;
}

STRSAFEAPI
StringCchPrintfA(LPTSTR pszDest,
                size_t cchDest,
                LPCTSTR pszFormat,
                ...)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        va_list argList;
        va_start(argList, pszFormat);
        hr = StringVPrintfWorkerA(pszDest, cchDest, pszFormat, argList);
        va_end(argList);
    }
    return hr;
}

STRSAFEAPI
StringCchPrintfW(LPWSTR pszDest,
                size_t cchDest,
                LPCWSTR pszFormat,
                ...)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        va_list argList;
        va_start(argList, pszFormat);
        hr = StringVPrintfWorkerW(pszDest, cchDest, pszFormat, argList);
        va_end(argList);
    }
    return hr;
}

STRSAFEAPI
StringCchPrintfExA(LPTSTR pszDest,
                  size_t cchDest,
                  LPTSTR *ppszDestEnd,
                  size_t *pcchRemaining,
                  DWORD dwFlags,
                  LPCTSTR pszFormat,
                 ...)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cbDest;
        va_list argList;
        cbDest = cchDest * sizeof(char);
        va_start(argList, pszFormat);
        hr = StringVPrintfExWorkerA(pszDest, cchDest, cbDest, ppszDestEnd, pcchRemaining, dwFlags, pszFormat, argList);
        va_end(argList);
    }
    return hr;
}

STRSAFEAPI
StringCchPrintfExW(LPWSTR pszDest,
                  size_t cchDest,
                  LPWSTR *ppszDestEnd,
                  size_t *pcchRemaining,
                  DWORD dwFlags,
                  LPCWSTR pszFormat,
                  ...)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cbDest;
        va_list argList;
        cbDest = cchDest * sizeof(wchar_t);
        va_start(argList, pszFormat);
        hr = StringVPrintfExWorkerW(pszDest, cchDest, cbDest, ppszDestEnd, pcchRemaining, dwFlags, pszFormat, argList);
        va_end(argList);
    }
    return hr;
}

STRSAFEAPI
StringCchVPrintfA(LPTSTR pszDest,
                 size_t cchDest,
                 LPCTSTR pszFormat,
                 va_list argList)
{
    if (cchDest > STRSAFE_MAX_CCH)
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringVPrintfWorkerA(pszDest, cchDest, pszFormat, argList);
}

STRSAFEAPI
StringCchVPrintfW(LPWSTR pszDest,
                 size_t cchDest,
                 LPCWSTR pszFormat,
                 va_list argList)
{
    if (cchDest > STRSAFE_MAX_CCH)
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringVPrintfWorkerW(pszDest, cchDest, pszFormat, argList);
}

STRSAFEAPI
StringCchVPrintfExA(LPTSTR pszDest,
                   size_t cchDest,
                   LPTSTR *ppszDestEnd,
                   size_t *pcchRemaining,
                   DWORD dwFlags,
                   LPCTSTR pszFormat,
                   va_list argList)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cbDest;
        cbDest = cchDest * sizeof(char);
        hr = StringVPrintfExWorkerA(pszDest, cchDest, cbDest, ppszDestEnd, pcchRemaining, dwFlags, pszFormat, argList);
    }
    return hr;
}

STRSAFEAPI
StringCchVPrintfExW(LPWSTR pszDest,
                   size_t cchDest,
                   LPWSTR *ppszDestEnd,
                   size_t *pcchRemaining,
                   DWORD dwFlags,
                   LPCWSTR pszFormat,
                   va_list argList)
{
    HRESULT hr;

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cbDest;
        cbDest = cchDest * sizeof(wchar_t);
        hr = StringVPrintfExWorkerW(pszDest, cchDest, cbDest, ppszDestEnd, pcchRemaining, dwFlags, pszFormat, argList);
    }
    return hr;
}

STRSAFEAPI
StringCchLengthA(LPCTSTR psz,
                size_t cchMax,
                size_t *pcch)
{
    if ((psz == NULL) || (cchMax > STRSAFE_MAX_CCH))
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringLengthWorkerA(psz, cchMax, pcch);
}

STRSAFEAPI
StringCchLengthW(LPCWSTR psz,
                size_t cchMax,
                size_t *pcch)
{
    if ((psz == NULL) || (cchMax > STRSAFE_MAX_CCH))
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringLengthWorkerW(psz, cchMax, pcch);
}

STRSAFEAPI
StringCbCatA(LPTSTR pszDest,
            size_t cbDest,
            LPCTSTR pszSrc)
{
    size_t cchDest;

    cchDest = cbDest / sizeof(char);
    if (cchDest > STRSAFE_MAX_CCH)
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringCatWorkerA(pszDest, cchDest, pszSrc);
}

STRSAFEAPI
StringCbCatW(LPWSTR pszDest,
            size_t cbDest,
            LPCWSTR pszSrc)
{
    size_t cchDest;

    cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > STRSAFE_MAX_CCH)
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringCatWorkerW(pszDest, cchDest, pszSrc);
}

STRSAFEAPI
StringCbCatExA(LPTSTR pszDest,
              size_t cbDest,
              LPCTSTR pszSrc,
              LPTSTR *ppszDestEnd,
              size_t *pcbRemaining,
              DWORD dwFlags)
{
    HRESULT hr;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(char);
    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
        hr = StringCatExWorkerA(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, &cchRemaining, dwFlags);
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (pcbRemaining)
            *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
    }
    return hr;
}

STRSAFEAPI
StringCbCatExW(LPWSTR pszDest,
              size_t cbDest,
              LPCWSTR pszSrc,
              LPWSTR *ppszDestEnd,
              size_t *pcbRemaining,
              DWORD dwFlags)
{
    HRESULT hr;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
        hr = StringCatExWorkerW(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, &cchRemaining, dwFlags);
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (pcbRemaining)
            *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
    }
    return hr;
}

STRSAFEAPI
StringCbCatNA(LPTSTR pszDest,
             size_t cbDest,
             LPCTSTR pszSrc,
             size_t cbMaxAppend)
{
    HRESULT hr;
    size_t cchDest;

    cchDest = cbDest / sizeof(char);
    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cchMaxAppend;
        cchMaxAppend = cbMaxAppend / sizeof(char);
        hr = StringCatNWorkerA(pszDest, cchDest, pszSrc, cchMaxAppend);
    }
    return hr;
}

STRSAFEAPI
StringCbCatNW(LPWSTR pszDest,
             size_t cbDest,
             LPCWSTR pszSrc,
             size_t cbMaxAppend)
{
    HRESULT hr;
    size_t cchDest;

    cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cchMaxAppend;
        cchMaxAppend = cbMaxAppend / sizeof(wchar_t);
        hr = StringCatNWorkerW(pszDest, cchDest, pszSrc, cchMaxAppend);
    }
    return hr;
}

STRSAFEAPI
StringCbCatNExA(LPTSTR pszDest,
               size_t cbDest,
               LPCTSTR pszSrc,
               size_t cbMaxAppend,
               LPTSTR *ppszDestEnd,
               size_t *pcbRemaining,
               DWORD dwFlags)
{
    HRESULT hr;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(char);
    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cchMaxAppend;
        cchMaxAppend = cbMaxAppend / sizeof(char);
        hr = StringCatNExWorkerA(pszDest, cchDest, cbDest, pszSrc, cchMaxAppend, ppszDestEnd, &cchRemaining, dwFlags);
    }
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (pcbRemaining)
            *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
    }

    return hr;
}

STRSAFEAPI
StringCbCatNExW(LPWSTR pszDest,
               size_t cbDest,
               LPCWSTR pszSrc,
               size_t cbMaxAppend,
               LPWSTR *ppszDestEnd,
               size_t *pcbRemaining,
               DWORD dwFlags)
{
    HRESULT hr;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        size_t cchMaxAppend;
        cchMaxAppend = cbMaxAppend / sizeof(wchar_t);
        hr = StringCatNExWorkerW(pszDest, cchDest, cbDest, pszSrc, cchMaxAppend, ppszDestEnd, &cchRemaining, dwFlags);
    }
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (pcbRemaining)
            *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
    }
    return hr;
}

STRSAFEAPI
StringCbCopyA(LPTSTR pszDest,
             size_t cbDest,
             LPCTSTR pszSrc)
{
    size_t cchDest;

    cchDest = cbDest / sizeof(char);
    if (cchDest > STRSAFE_MAX_CCH)
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringCopyWorkerA(pszDest, cchDest, pszSrc);
}

STRSAFEAPI
StringCbCopyW(LPWSTR pszDest,
             size_t cbDest,
             LPCWSTR pszSrc)
{
    size_t cchDest;

    cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > STRSAFE_MAX_CCH)
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringCopyWorkerW(pszDest, cchDest, pszSrc);
}

STRSAFEAPI
StringCbCopyExA(LPTSTR pszDest,
               size_t cbDest,
               LPCTSTR pszSrc,
               LPTSTR *ppszDestEnd,
               size_t *pcbRemaining,
               DWORD dwFlags)
{
    HRESULT hr;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(char);

    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
        hr = StringCopyExWorkerA(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, &cchRemaining, dwFlags);
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (pcbRemaining)
            *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
    }

    return hr;
}

STRSAFEAPI
StringCbCopyExW(LPWSTR pszDest,
               size_t cbDest,
               LPCWSTR pszSrc,
               LPWSTR *ppszDestEnd,
               size_t *pcbRemaining,
               DWORD dwFlags)
{
    HRESULT hr;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
        hr = StringCopyExWorkerW(pszDest, cchDest, cbDest, pszSrc, ppszDestEnd, &cchRemaining, dwFlags);
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (pcbRemaining)
            *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
    }

    return hr;
}

STRSAFEAPI
StringCbCopyNA(LPTSTR pszDest,
              size_t cbDest,
              LPCTSTR pszSrc,
              size_t cbSrc)
{
    size_t cchDest;
    size_t cchSrc;

    cchDest = cbDest / sizeof(char);
    cchSrc = cbSrc / sizeof(char);
    if ((cchDest > STRSAFE_MAX_CCH) || (cchSrc > STRSAFE_MAX_CCH))
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringCopyNWorkerA(pszDest, cchDest, pszSrc, cchSrc);
}

STRSAFEAPI
StringCbCopyNW(LPWSTR pszDest,
              size_t cbDest,
              LPCWSTR pszSrc,
              size_t cbSrc)
{
    size_t cchDest;
    size_t cchSrc;

    cchDest = cbDest / sizeof(wchar_t);
    cchSrc = cbSrc / sizeof(wchar_t);
    if ((cchDest > STRSAFE_MAX_CCH) || (cchSrc > STRSAFE_MAX_CCH))
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringCopyNWorkerW(pszDest, cchDest, pszSrc, cchSrc);
}

STRSAFEAPI
StringCbCopyNExA(LPTSTR pszDest,
                size_t cbDest,
                LPCTSTR pszSrc,
                size_t cbSrc,
                LPTSTR *ppszDestEnd,
                size_t *pcbRemaining,
                DWORD dwFlags)
{
    HRESULT hr;
    size_t cchDest;
    size_t cchSrc;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(char);
    cchSrc = cbSrc / sizeof(char);
    if ((cchDest > STRSAFE_MAX_CCH) ||
        (cchSrc > STRSAFE_MAX_CCH))
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
        hr = StringCopyNExWorkerA(pszDest, cchDest, cbDest, pszSrc, cchSrc, ppszDestEnd, &cchRemaining, dwFlags);
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (pcbRemaining)
            *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
    }
    return hr;
}

STRSAFEAPI
StringCbCopyNExW(LPWSTR pszDest,
                size_t cbDest,
                LPCWSTR pszSrc,
                size_t cbSrc,
                LPWSTR *ppszDestEnd,
                size_t *pcbRemaining,
                DWORD dwFlags)
{
    HRESULT hr;
    size_t cchDest;
    size_t cchSrc;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(wchar_t);
    cchSrc = cbSrc / sizeof(wchar_t);
    if ((cchDest > STRSAFE_MAX_CCH) ||
        (cchSrc > STRSAFE_MAX_CCH))
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
        hr = StringCopyNExWorkerW(pszDest, cchDest, cbDest, pszSrc, cchSrc, ppszDestEnd, &cchRemaining, dwFlags);
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (pcbRemaining)
            *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
    }
    return hr;
}

STRSAFE_INLINE_API
StringCbGetsA(LPTSTR pszDest,
             size_t cbDest)
{
    size_t cchDest;

    cchDest = cbDest / sizeof(char);
    if (cchDest > STRSAFE_MAX_CCH)
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringGetsExWorkerA(pszDest, cchDest, cbDest, NULL, NULL, 0);
}

STRSAFE_INLINE_API
StringCbGetsW(LPWSTR pszDest,
             size_t cbDest)
{
    size_t cchDest;

    cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > STRSAFE_MAX_CCH)
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringGetsExWorkerW(pszDest, cchDest, cbDest, NULL, NULL, 0);
}

STRSAFE_INLINE_API
StringCbGetsExA(LPTSTR pszDest,
               size_t cbDest,
               LPTSTR *ppszDestEnd,
               size_t *pcbRemaining,
               DWORD dwFlags)
{
    HRESULT hr;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(char);
    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
        hr = StringGetsExWorkerA(pszDest, cchDest, cbDest, ppszDestEnd, &cchRemaining, dwFlags);
    if (SUCCEEDED(hr) ||
        (hr == STRSAFE_E_INSUFFICIENT_BUFFER) ||
        (hr == STRSAFE_E_END_OF_FILE))
    {
        if (pcbRemaining)
            *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
    }
    return hr;
}

STRSAFE_INLINE_API
StringCbGetsExW(LPWSTR pszDest,
               size_t cbDest,
               LPWSTR *ppszDestEnd,
               size_t *pcbRemaining,
               DWORD dwFlags)
{
    HRESULT hr;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
        hr = StringGetsExWorkerW(pszDest, cchDest, cbDest, ppszDestEnd, &cchRemaining, dwFlags);
    if (SUCCEEDED(hr) ||
        (hr == STRSAFE_E_INSUFFICIENT_BUFFER) ||
        (hr == STRSAFE_E_END_OF_FILE))
    {
        if (pcbRemaining)
            *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
    }
    return hr;
}

STRSAFEAPI
StringCbPrintfA(LPTSTR pszDest,
               size_t cbDest,
               LPCTSTR pszFormat,
               ...)
{
    HRESULT hr;
    size_t cchDest;

    cchDest = cbDest / sizeof(char);
    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        va_list argList;
        va_start(argList, pszFormat);
        hr = StringVPrintfWorkerA(pszDest, cchDest, pszFormat, argList);
        va_end(argList);
    }
    return hr;
}

STRSAFEAPI
StringCbPrintfW(LPWSTR pszDest,
               size_t cbDest,
               LPCWSTR pszFormat,
               ...)
{
    HRESULT hr;
    size_t cchDest;

    cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        va_list argList;
        va_start(argList, pszFormat);
        hr = StringVPrintfWorkerW(pszDest, cchDest, pszFormat, argList);
        va_end(argList);
    }
    return hr;
}

STRSAFEAPI
StringCbPrintfExA(LPTSTR pszDest,
                 size_t cbDest,
                 LPTSTR *ppszDestEnd,
                 size_t *pcbRemaining,
                 DWORD dwFlags,
                 LPCTSTR pszFormat,
                 ...)
{
    HRESULT hr;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(char);
    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        va_list argList;
        va_start(argList, pszFormat);
        hr = StringVPrintfExWorkerA(pszDest, cchDest, cbDest, ppszDestEnd, &cchRemaining, dwFlags, pszFormat, argList);
        va_end(argList);
    }
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (pcbRemaining)
            *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
    }
    return hr;
}

STRSAFEAPI
StringCbPrintfExW(LPWSTR pszDest,
                 size_t cbDest,
                 LPWSTR *ppszDestEnd,
                 size_t *pcbRemaining,
                 DWORD dwFlags,
                 LPCWSTR pszFormat,
                 ...)
{
    HRESULT hr;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
    {
        va_list argList;
        va_start(argList, pszFormat);
        hr = StringVPrintfExWorkerW(pszDest, cchDest, cbDest, ppszDestEnd, &cchRemaining, dwFlags, pszFormat, argList);
        va_end(argList);
    }
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (pcbRemaining)
            *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
    }
    return hr;
}

STRSAFEAPI
StringCbVPrintfA(LPTSTR pszDest,
                size_t cbDest,
                LPCTSTR pszFormat,
                va_list argList)
{
    size_t cchDest;

    cchDest = cbDest / sizeof(char);
    if (cchDest > STRSAFE_MAX_CCH)
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringVPrintfWorkerA(pszDest, cchDest, pszFormat, argList);
}

STRSAFEAPI
StringCbVPrintfW(LPWSTR pszDest,
                size_t cbDest,
                LPCWSTR pszFormat,
                va_list argList)
{
    size_t cchDest;

    cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > STRSAFE_MAX_CCH)
        return STRSAFE_E_INVALID_PARAMETER;
    else
        return StringVPrintfWorkerW(pszDest, cchDest, pszFormat, argList);
}

STRSAFEAPI
StringCbVPrintfExA(LPTSTR pszDest,
                  size_t cbDest,
                  LPTSTR *ppszDestEnd,
                  size_t *pcbRemaining,
                  DWORD dwFlags,
                  LPCTSTR pszFormat,
                  va_list argList)
{
    HRESULT hr;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(char);
    if (cchDest > STRSAFE_MAX_CCH)
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
        hr = StringVPrintfExWorkerA(pszDest, cchDest, cbDest, ppszDestEnd, &cchRemaining, dwFlags, pszFormat, argList);
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (pcbRemaining)
            *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
    }
    return hr;
}

STRSAFEAPI
StringCbVPrintfExW(LPWSTR pszDest,
                  size_t cbDest,
                  LPWSTR *ppszDestEnd,
                  size_t *pcbRemaining,
                  DWORD dwFlags,
                  LPCWSTR pszFormat,
                  va_list argList)
{
    HRESULT hr;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > STRSAFE_MAX_CCH)
	    hr = STRSAFE_E_INVALID_PARAMETER;
    else
        hr = StringVPrintfExWorkerW(pszDest, cchDest, cbDest, ppszDestEnd, &cchRemaining, dwFlags, pszFormat, argList);
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER))
    {
        if (pcbRemaining)
            *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
    }
    return hr;
}

STRSAFEAPI
StringCbLengthA(LPCTSTR psz,
               size_t cbMax,
               size_t *pcb)
{
    HRESULT hr;
    size_t cchMax;
    size_t cch = 0;

    cchMax = cbMax / sizeof(char);
    if ((psz == NULL) || (cchMax > STRSAFE_MAX_CCH))
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
        hr = StringLengthWorkerA(psz, cchMax, &cch);
    if (SUCCEEDED(hr) && pcb) *pcb = cch * sizeof(char);
    return hr;
}

STRSAFEAPI
StringCbLengthW(LPCWSTR psz,
               size_t cbMax,
               size_t *pcb)
{
    HRESULT hr;
    size_t cchMax;
    size_t cch = 0;

    cchMax = cbMax / sizeof(wchar_t);
    if ((psz == NULL) || (cchMax > STRSAFE_MAX_CCH))
        hr = STRSAFE_E_INVALID_PARAMETER;
    else
        hr = StringLengthWorkerW(psz, cchMax, &cch);
    if (SUCCEEDED(hr) && pcb) *pcb = cch * sizeof(wchar_t);
    return hr;
}
