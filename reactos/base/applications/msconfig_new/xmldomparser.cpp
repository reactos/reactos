/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/xmldomparser.cpp
 * PURPOSE:     XML DOM Parser
 * COPYRIGHT:   Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#include "precomp.h"
#include "xmldomparser.hpp"
#include "utils.h"
#include "stringutils.h"

// UTF8 adapted version of ConvertStringToBSTR (see lib/sdk/comsupp/comsupp.c)
static BSTR
ConvertUTF8StringToBSTR(const char *pSrc)
{
    DWORD cwch;
    BSTR wsOut(NULL);

    if (!pSrc) return NULL;

    /* Compute the needed size with the NULL terminator */
    cwch = MultiByteToWideChar(CP_UTF8, 0, pSrc, -1, NULL, 0);
    if (cwch == 0) return NULL;

    /* Allocate the BSTR (without the NULL terminator) */
    wsOut = SysAllocStringLen(NULL, cwch - 1);
    if (!wsOut)
    {
        _com_issue_error(HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY));
        return NULL;
    }

    /* Convert the string */
    if (MultiByteToWideChar(CP_UTF8, 0, pSrc, -1, wsOut, cwch) == 0)
    {
        /* We failed, clean everything up */
        cwch = GetLastError();

        SysFreeString(wsOut);
        wsOut = NULL;

        _com_issue_error(!IS_ERROR(cwch) ? HRESULT_FROM_WIN32(cwch) : cwch);
    }

    return wsOut;
}

HRESULT
InitXMLDOMParser(VOID)
{
    return CoInitialize(NULL);
}

VOID
UninitXMLDOMParser(VOID)
{
    CoUninitialize();
    return;
}

HRESULT
CreateAndInitXMLDOMDocument(IXMLDOMDocument** ppDoc)
{
    HRESULT hr = CoCreateInstance(CLSID_DOMDocument30, // __uuidof(DOMDocument30), // NOTE: Do not use DOMDocument60 if you want MSConfig working by default on XP.
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARG(IXMLDOMDocument, ppDoc) /* IID_PPV_ARGS(ppDoc) */);
    if (!SUCCEEDED(hr))
        return hr;

    /* These methods should not fail so don't inspect result */
    (*ppDoc)->put_async(VARIANT_FALSE);  
    (*ppDoc)->put_validateOnParse(VARIANT_FALSE);
    (*ppDoc)->put_resolveExternals(VARIANT_FALSE);

    return hr;
}

BOOL
LoadXMLDocumentFromResource(IXMLDOMDocument* pDoc,
                            LPCWSTR lpszXMLResName)
{
    VARIANT_BOOL Success = VARIANT_FALSE;
    HRSRC   hRes;
    HGLOBAL handle;
    LPVOID  lpXMLRes;
    _bstr_t bstrXMLRes;

    if (!pDoc)
        return FALSE;

    // hRes = FindResourceExW(NULL, RT_HTML, lpszXMLResName, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    hRes = FindResourceW(NULL, lpszXMLResName, RT_HTML);
    if (hRes == NULL)
        return FALSE;

    handle = LoadResource(NULL, hRes);
    if (handle == NULL)
        return FALSE;

    lpXMLRes = LockResource(handle);
    if (lpXMLRes == NULL)
        goto Cleanup;

    /* Convert the resource to UNICODE if needed */
    if (!IsTextUnicode(lpXMLRes, SizeofResource(NULL, hRes), NULL))
        bstrXMLRes.Attach(ConvertUTF8StringToBSTR((LPCSTR)lpXMLRes));
    else
        bstrXMLRes = (LPCWSTR)lpXMLRes;

    if (SUCCEEDED(pDoc->loadXML(bstrXMLRes, &Success)) && (Success != VARIANT_TRUE))
    {
        IXMLDOMParseError* pXMLErr = NULL;
        _bstr_t            bstrErr;

        if (SUCCEEDED(pDoc->get_parseError(&pXMLErr)) &&
            SUCCEEDED(pXMLErr->get_reason(&bstrErr.GetBSTR())))
        {
            LPWSTR lpszStr = NULL;

            if (IS_INTRESOURCE((ULONG_PTR)lpszXMLResName))
                lpszStr = FormatString(L"Failed to load DOM from resource '#%u': %wS", lpszXMLResName, (wchar_t*)bstrErr);
            else
                lpszStr = FormatString(L"Failed to load DOM from resource '%wS': %wS", lpszXMLResName, (wchar_t*)bstrErr);

            MessageBoxW(NULL, lpszStr, L"Error", MB_ICONERROR | MB_OK);

            MemFree(lpszStr);
        }

        SAFE_RELEASE(pXMLErr);
    }

Cleanup:
    FreeResource(handle);

    return (Success == VARIANT_TRUE);
}

BOOL
LoadXMLDocumentFromFile(IXMLDOMDocument* pDoc,
                        LPCWSTR lpszFilename,
                        BOOL bIgnoreErrorsIfNonExistingFile)
{
    VARIANT_BOOL Success = VARIANT_FALSE;
    _variant_t varFileName(lpszFilename);

    if (!pDoc)
        return FALSE;

    if (SUCCEEDED(pDoc->load(varFileName, &Success)) && (Success != VARIANT_TRUE))
    {
        IXMLDOMParseError* pXMLErr    = NULL;
        LONG               lErrorCode = 0L;

        if (SUCCEEDED(pDoc->get_parseError(&pXMLErr)) &&
            SUCCEEDED(pXMLErr->get_errorCode(&lErrorCode)))
        {
            if ( !bIgnoreErrorsIfNonExistingFile ||
                ((lErrorCode != _HRESULT_TYPEDEF_(0x800C0006) /* INET_E_OBJECT_NOT_FOUND */) &&
                 (lErrorCode != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))) )
            {
                _bstr_t bstrErr;

                if (SUCCEEDED(pXMLErr->get_reason(&bstrErr.GetBSTR())))
                {
                    LPWSTR lpszStr = FormatString(L"Failed to load DOM from '%wS': %wS", lpszFilename, (wchar_t*)bstrErr);
                    MessageBoxW(NULL, lpszStr, L"Error", MB_ICONERROR | MB_OK);
                    MemFree(lpszStr);
                }
            }
        }

        SAFE_RELEASE(pXMLErr);
    }

    return (Success == VARIANT_TRUE);
}
