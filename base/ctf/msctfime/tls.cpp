/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Thread-local storage
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "msctfime.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctfime);

DWORD TLS::s_dwTlsIndex = (DWORD)-1;

/// @implemented
BOOL TLS::Initialize()
{
    s_dwTlsIndex = ::TlsAlloc();
    return s_dwTlsIndex != (DWORD)-1;
}

/// @implemented
VOID TLS::Uninitialize()
{
    if (s_dwTlsIndex != (DWORD)-1)
    {
        ::TlsFree(s_dwTlsIndex);
        s_dwTlsIndex = (DWORD)-1;
    }
}

/// @implemented
TLS* TLS::GetTLS()
{
    if (s_dwTlsIndex == (DWORD)-1)
        return NULL;

    return InternalAllocateTLS();
}

/// @implemented
TLS* TLS::PeekTLS()
{
    return (TLS*)::TlsGetValue(TLS::s_dwTlsIndex);
}

/// @implemented
TLS* TLS::InternalAllocateTLS()
{
    TLS *pTLS = TLS::PeekTLS();
    if (pTLS)
        return pTLS;

    if (DllShutdownInProgress())
        return NULL;

    pTLS = (TLS *)cicMemAllocClear(sizeof(TLS));
    if (!pTLS)
        return NULL;

    if (!::TlsSetValue(s_dwTlsIndex, pTLS))
    {
        cicMemFree(pTLS);
        return NULL;
    }

    pTLS->m_dwFlags1 |= 1;
    pTLS->m_dwUnknown2 |= 1;
    return pTLS;
}

/// @implemented
BOOL TLS::InternalDestroyTLS()
{
    TLS *pTLS = TLS::PeekTLS();
    if (!pTLS)
        return FALSE;

    if (pTLS->m_pBridge)
        pTLS->m_pBridge->Release();
    if (pTLS->m_pProfile)
        pTLS->m_pProfile->Release();
    if (pTLS->m_pThreadMgr)
        pTLS->m_pThreadMgr->Release();

    cicMemFree(pTLS);
    ::TlsSetValue(s_dwTlsIndex, NULL);
    return TRUE;
}

/// @implemented
BOOL TLS::NonEACompositionEnabled()
{
    if (!m_NonEAComposition)
    {
        DWORD dwValue = 1;

        CicRegKey regKey;
        LSTATUS error = regKey.Open(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\CTF\\CUAS"));
        if (error == ERROR_SUCCESS)
        {
            error = regKey.QueryDword(TEXT("NonEAComposition"), &dwValue);
            if (error != ERROR_SUCCESS)
                dwValue = 1;
        }

        m_NonEAComposition = dwValue;
    }

    return (m_NonEAComposition == 2);
}
