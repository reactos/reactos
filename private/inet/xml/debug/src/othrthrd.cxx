//+---------------------------------------------------------------------------
//
//  Microsoft Windows
// Copyright (c) 1992 - 1999 Microsoft Corporation. All rights reserved.*///
//  File:       othrthrd.cxx
//
//  Contents:   Contains utilities that utilize the THREADSTATE data inside
//              MSHTML.DLL.
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

void EXPORT WINAPI
DbgSetTopUrl(LPWSTR pstrUrl)
{
    WideCharToMultiByte(CP_ACP,
                        0,
                        pstrUrl,
                        -1,
                        TLS(achTopUrl),
                        TOPURL_LENGTH,
                        NULL,
                        NULL);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetTopURLForThread
//
//  Synopsis:   Returns the URL of the document in the given thread. For
//              multiple document threads (like a frameset) the topmost one
//              is returned.
//
//  Arguments:  [tid] -- Thread ID desired
//              [psz] -- Pre-allocated buffer to put URL into.
//
//  Returns:    HRESULT, S_FALSE if url cannot be obtained.
//
//  Notes:      Will put "<not available>" in the output string if it cannot
//              be obtained. On an error, [psz] will be empty. Assumes
//              that psz is a buffer of size TOPURL_LENGTH
//
//----------------------------------------------------------------------------

char * szNotAvail = "<not available>";

HRESULT
GetTopURLForThread(DWORD tid, char * psz)
{
    char * pszTop = TLS(achTopUrl);

    if (*pszTop == '\0')
    {
        strcpy(psz, szNotAvail);
        return S_FALSE;
    }
    else
    {
        strcpy(psz, pszTop);
    }

    return S_OK;
}
