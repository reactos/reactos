/*
 * isnewshk.cpp - INewShortcutHook implementation for URL class.
 */


#include "priv.h"
#include "ishcut.h"

#include "resource.h"

#include <mluisupp.h>

#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

STDMETHODIMP
Intshcut::SetReferent(
    LPCTSTR pcszReferent,
    HWND hwndParent)
{
    HRESULT hr;
    TCHAR szURL[MAX_URL_STRING];

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_STRING_PTR(pcszReferent, -1));
    ASSERT(IS_VALID_HANDLE(hwndParent, WND));

    hr = IURLQualify(pcszReferent, UQF_IGNORE_FILEPATHS | UQF_GUESS_PROTOCOL, szURL, NULL, NULL);
    if (SUCCEEDED(hr))
    {
        hr = ValidateURL(szURL);

        if (hr == S_OK)
            hr = SetURL(szURL, 0);
    }

    if (S_OK != hr)
    {
        ASSERT(FAILED(hr));

        // Massage result
        switch (hr)
        {
            case URL_E_INVALID_SYNTAX:
            case URL_E_UNREGISTERED_PROTOCOL:
                hr = S_FALSE;
                break;

            default:
                break;
        }

        TraceMsg(TF_INTSHCUT, "Intshcut::SetReferent(): Failed to set referent to %s.",
                   pcszReferent);
    }

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));

    return(hr);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


STDMETHODIMP Intshcut::GetReferent(PTSTR pszReferent, int cchReferent)
{
    HRESULT hr = InitProp();
    if (SUCCEEDED(hr))
    {
        TCHAR szURL[INTERNET_MAX_URL_LENGTH];
        hr = m_pprop->GetProp(PID_IS_URL, szURL, SIZECHARS(szURL));
        if (S_OK == hr)
        {
            if (lstrlen(szURL) < cchReferent)
            {
                StrCpyN(pszReferent, szURL, cchReferent);
                hr = S_OK;
            }
            else
                hr = E_FAIL;
        }
        else
            hr = S_FALSE;

        if (hr != S_OK)
        {
            if (cchReferent > 0)
                *pszReferent = '\0';
        }
    }
    return hr;
}


STDMETHODIMP Intshcut::SetFolder(LPCTSTR pcszFolder)
{
    HRESULT hr;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(PathIsDirectory(pcszFolder));

    if (Str_SetPtr(&m_pszFolder, pcszFolder))
    {
        hr = S_OK;

        TraceMsg(TF_INTSHCUT, "Intshcut::SetFolder(): Set folder to %s.",
                   m_pszFolder);
    }
    else
        hr = E_OUTOFMEMORY;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));

    return(hr);
}


STDMETHODIMP
Intshcut::GetFolder(
    LPTSTR pszFolder,
    int cchFolder)
{
    HRESULT hr;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_WRITE_BUFFER(pszFolder, TCHAR, cchFolder));

    if (m_pszFolder)
    {
        if (lstrlen(m_pszFolder) < cchFolder)
        {
            StrCpyN(pszFolder, m_pszFolder, cchFolder);

            hr = S_OK;

            TraceMsg(TF_INTSHCUT, "Intshcut::GetFolder(): Returning folder %s.",
                     pszFolder);
        }
        else
            hr = E_FAIL;
    }
    else
        hr = S_FALSE;

    if (hr != S_OK)
    {
        if (cchFolder > 0)
            *pszFolder = '\0';
    }

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT((hr == S_OK &&
            IS_VALID_STRING_PTR(pszFolder, -1) &&
            EVAL(lstrlen(pszFolder) < cchFolder)) ||
           ((hr == S_FALSE ||
             hr == E_FAIL) &&
            EVAL(! cchFolder ||
                 ! *pszFolder)));

    return(hr);
}


STDMETHODIMP
Intshcut::GetName(
    LPTSTR pszName,
    int cchBuf)
{
    HRESULT hr = E_FAIL;
    TCHAR rgchShortName[MAX_PATH];

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_WRITE_BUFFER(pszName, TCHAR, cchBuf));

    hr = E_FAIL;

    if (MLLoadString(IDS_SHORT_NEW_INTSHCUT, rgchShortName, SIZECHARS(rgchShortName)))
    {
        TCHAR rgchLongName[MAX_PATH];

        if (MLLoadString(IDS_NEW_INTSHCUT, rgchLongName, SIZECHARS(rgchLongName)))
        {
            TCHAR rgchCurDir[MAX_PATH];
            LPCTSTR pcszFolderToUse;

            // Use current directory if m_pszFolder has not been set.

            pcszFolderToUse = m_pszFolder;

            if (! pcszFolderToUse)
            {
                if (GetCurrentDirectory(SIZECHARS(rgchCurDir), rgchCurDir) > 0)
                    pcszFolderToUse = rgchCurDir;
            }

            if (pcszFolderToUse)
            {
                TCHAR rgchUniqueName[MAX_PATH];

                if (PathYetAnotherMakeUniqueName(rgchUniqueName, pcszFolderToUse,
                                                 rgchShortName, rgchLongName))
                {
                    PTSTR pszFileName;
                    PTSTR pszRemoveExt;

                    pszFileName = (PTSTR)PathFindFileName(rgchUniqueName);
                    pszRemoveExt = (PTSTR)PathFindExtension(pszFileName);
                    *pszRemoveExt = '\0';

                    if (lstrlen(pszFileName) < cchBuf)
                    {
                        StrCpyN(pszName, pszFileName, cchBuf);
                        hr = S_OK;
                    }
                }
            }
        }
    }

    if (hr == S_OK)
        TraceMsg(TF_INTSHCUT, "Intshcut::GetName(): Returning %s.", pszName);

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT((hr == S_OK &&
            IS_VALID_STRING_PTR(pszName, -1) &&
            EVAL(lstrlen(pszName) < cchBuf)) ||
           (hr == E_FAIL &&
            (! cchBuf ||
             ! *pszName)));

    return(hr);
}


STDMETHODIMP
Intshcut::GetExtension(
    LPTSTR pszExtension,
    int cchBufMax)
{
    HRESULT hr;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_WRITE_BUFFER(pszExtension, TCHAR, cchBufMax));

    if (SIZECHARS(TEXT(".url")) < cchBufMax)
    {
        StrCpyN(pszExtension, TEXT(".url"), cchBufMax);

        hr = S_OK;

        TraceMsg(TF_INTSHCUT, "Intshcut::GetExtension(): Returning extension %s.",
                   pszExtension);
    }
    else
    {
        if (cchBufMax > 0)
            *pszExtension = '\0';

        hr = E_FAIL;
    }

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT((hr == S_OK &&
            IS_VALID_STRING_PTR(pszExtension, -1) &&
            EVAL(lstrlen(pszExtension) < cchBufMax)) ||
           (hr == E_FAIL &&
            EVAL(! cchBufMax ||
                 ! *pszExtension)));

    return(hr);
}


// Ansi versions.  Needed for W9x

STDMETHODIMP
Intshcut::SetReferent(
    LPCSTR pcszReferent,
    HWND hwndParent)
{
    HRESULT hr;

    WCHAR szReferent[MAX_URL_STRING];
    ASSERT(lstrlenA(pcszReferent) + 1 < ARRAYSIZE(szReferent));

    SHAnsiToUnicode(pcszReferent, szReferent, ARRAYSIZE(szReferent));

    hr = SetReferent(szReferent, hwndParent);

    return hr;
}


STDMETHODIMP Intshcut::GetReferent(PSTR pszReferent, int cchReferent)
{
    HRESULT hr;

    WCHAR szReferent[MAX_URL_STRING];

    ASSERT(cchReferent <= ARRAYSIZE(szReferent));

    hr = GetReferent(szReferent, ARRAYSIZE(szReferent));

    if (SUCCEEDED(hr))
        SHUnicodeToAnsi(szReferent, pszReferent, cchReferent);

    return hr;
}


STDMETHODIMP Intshcut::SetFolder(LPCSTR pcszFolder)
{
    HRESULT hr;

    WCHAR szFolder[MAX_PATH];
    ASSERT(lstrlenA(pcszFolder) + 1 < ARRAYSIZE(szFolder))

    SHAnsiToUnicode(pcszFolder, szFolder, ARRAYSIZE(szFolder));

    hr = SetFolder(szFolder);
    
    return(hr);
}


STDMETHODIMP
Intshcut::GetFolder(
    LPSTR pszFolder,
    int cchFolder)
{
    HRESULT hr;

    WCHAR szFolder[MAX_PATH];
    ASSERT(cchFolder <= ARRAYSIZE(szFolder));

    hr = GetFolder(szFolder, ARRAYSIZE(szFolder));

    if (SUCCEEDED(hr))
        SHUnicodeToAnsi(szFolder, pszFolder, cchFolder);

    return hr;
}


STDMETHODIMP
Intshcut::GetName(
    LPSTR pszName,
    int cchBuf)
{
    HRESULT hr;

    WCHAR szName[MAX_PATH];
    ASSERT(cchBuf <= ARRAYSIZE(szName));

    hr = GetName(szName, ARRAYSIZE(szName));

    if (SUCCEEDED(hr))
        SHUnicodeToAnsi(szName, pszName, cchBuf);

    return hr;
}


STDMETHODIMP
Intshcut::GetExtension(
    LPSTR pszExtension,
    int cchBufMax)
{
    HRESULT hr;

    WCHAR szExtension[MAX_PATH];
    ASSERT(cchBufMax<= ARRAYSIZE(szExtension));

    hr = GetExtension(szExtension, ARRAYSIZE(szExtension));

    if (SUCCEEDED(hr))
        SHUnicodeToAnsi(szExtension, pszExtension, cchBufMax);

    return hr;
}

