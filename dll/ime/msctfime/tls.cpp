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
