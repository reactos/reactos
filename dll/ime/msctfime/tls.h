/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Thread-local storage
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

class TLS;

class CicBridge;
class CicProfile;

class TLS
{
public:
    static DWORD s_dwTlsIndex;

    DWORD m_dwSystemInfoFlags;
    CicBridge *m_pBridge;
    CicProfile *m_pProfile;
    ITfThreadMgr_P *m_pThreadMgr;
    DWORD m_dwFlags1;
    DWORD m_dwFlags2;
    DWORD m_dwUnknown2;
    BOOL m_bDestroyed;
    BOOL m_bNowOpening;
    DWORD m_NonEAComposition;
    DWORD m_cWnds;

    /**
     * @implemented
     */
    static BOOL Initialize()
    {
        s_dwTlsIndex = ::TlsAlloc();
        return s_dwTlsIndex != (DWORD)-1;
    }

    /**
     * @implemented
     */
    static VOID Uninitialize()
    {
        if (s_dwTlsIndex != (DWORD)-1)
        {
            ::TlsFree(s_dwTlsIndex);
            s_dwTlsIndex = (DWORD)-1;
        }
    }

    /**
     * @implemented
     */
    static TLS* GetTLS()
    {
        if (s_dwTlsIndex == (DWORD)-1)
            return NULL;

        return InternalAllocateTLS();
    }

    /**
     * @implemented
     */
    static TLS* PeekTLS()
    {
        return (TLS*)::TlsGetValue(TLS::s_dwTlsIndex);
    }

    static TLS* InternalAllocateTLS();
    static BOOL InternalDestroyTLS();

    BOOL NonEACompositionEnabled();
};
