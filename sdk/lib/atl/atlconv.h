/*
 * PROJECT:     ReactOS ATL
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     String conversion
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#ifndef __ATLCONV_H__
#define __ATLCONV_H__

#pragma once

#include "atlbase.h"

namespace ATL
{

// This class does not own the string
template <int t_nBufferLength = 128>
class CA2CAEX
{
public:
    LPCSTR m_psz;

    CA2CAEX(_In_z_ LPCSTR psz) : m_psz(psz) { }

    CA2CAEX(_In_z_ LPCSTR psz, _In_ UINT nCodePage) : m_psz(psz)
    {
        UNREFERENCED_PARAMETER(nCodePage);
    }

    ~CA2CAEX() noexcept { } // There is nothing to free here

    _Ret_z_ operator LPCSTR() const noexcept { return m_psz; }

private:
    // CA2CAEX is not copyable
    CA2CAEX(_In_ const CA2CAEX&) noexcept = delete;
    CA2CAEX& operator=(_In_ const CA2CAEX&) noexcept = delete;
};

// This class does not own the string
template <int t_nBufferLength = 128>
class CW2CWEX
{
public:
    LPCWSTR m_psz;

    CW2CWEX(_In_z_ LPCWSTR psz) : m_psz(psz) { }

    CW2CWEX(_In_z_ LPCWSTR psz, _In_ UINT nCodePage) : m_psz(psz)
    {
        UNREFERENCED_PARAMETER(nCodePage);
    }

    ~CW2CWEX() noexcept { } // There is nothing to free here

    _Ret_z_ operator LPCWSTR() const noexcept { return m_psz; }

private:
    // CW2CWEX is not copyable
    CW2CWEX(_In_ const CW2CWEX&) noexcept = delete;
    CW2CWEX& operator=(_In_ const CW2CWEX&) noexcept = delete;
};

template <int t_nBufferLength = 128>
class CA2AEX
{
public:
    LPSTR m_psz;
    char m_szBuffer[t_nBufferLength];

    CA2AEX(_In_z_ LPCSTR psz)
    {
        Init(psz);
    }

    CA2AEX(_In_z_ LPCSTR psz, _In_ UINT nCodePage)
    {
        UNREFERENCED_PARAMETER(nCodePage);
        Init(psz);
    }

    ~CA2AEX() noexcept
    {
        if (m_psz != m_szBuffer)
            free(m_psz);
    }

    _Ret_z_ operator LPSTR() const noexcept
    {
        return m_psz;
    }

private:
    // CA2AEX is not copyable
    CA2AEX(_In_ const CA2AEX &) noexcept = delete;
    CA2AEX& operator=(_In_ const CA2AEX &) noexcept = delete;

    void Init(_In_z_ LPCSTR psz)
    {
        if (!psz)
        {
            m_psz = NULL;
            m_szBuffer[0] = 0;
            return;
        }
        int cchMax = lstrlenA(psz) + 1;
        if (cchMax <= t_nBufferLength)
        {
#ifdef _STRSAFE_H_INCLUDED_
            StringCchCopyA(m_szBuffer, _countof(m_szBuffer), psz);
#else
            lstrcpynA(m_szBuffer, psz, _countof(m_szBuffer));
#endif
            m_psz = m_szBuffer;
            return;
        }

        m_szBuffer[0] = 0;
        m_psz = _strdup(psz);
        if (!m_psz)
            AtlThrow(E_OUTOFMEMORY);
    }
};

template <int t_nBufferLength = 128>
class CW2WEX
{
public:
    LPWSTR m_psz;
    wchar_t m_szBuffer[t_nBufferLength];

    CW2WEX(_In_z_ LPCWSTR psz)
    {
        Init(psz);
    }

    CW2WEX(_In_z_ LPCWSTR psz, _In_ UINT nCodePage)
    {
        UNREFERENCED_PARAMETER(nCodePage);
        Init(psz);
    }

    ~CW2WEX() noexcept
    {
        if (m_psz != m_szBuffer)
            free(m_psz);
    }

    _Ret_z_ operator LPWSTR() const noexcept
    {
        return m_psz;
    }

private:
    // CW2WEX is not copyable
    CW2WEX(_In_ const CW2WEX&) noexcept = delete;
    CW2WEX& operator=(_In_ const CW2WEX&) noexcept = delete;

    void Init(_In_z_ LPCWSTR psz)
    {
        if (!psz)
        {
            m_psz = NULL;
            m_szBuffer[0] = 0;
            return;
        }
        int cchMax = lstrlenW(psz);
        if (cchMax <= t_nBufferLength)
        {
#ifdef _STRSAFE_H_INCLUDED_
            StringCchCopyW(m_szBuffer, _countof(m_szBuffer), psz);
#else
            lstrcpynW(m_szBuffer, psz, _countof(m_szBuffer));
#endif
            m_psz = m_szBuffer;
            return;
        }

        m_szBuffer[0] = 0;
        m_psz = _wcsdup(psz);
        if (!m_psz)
            AtlThrow(E_OUTOFMEMORY);
    }
};

template <int t_nBufferLength = 128>
class CA2WEX
{
public:
    LPWSTR m_psz;
    wchar_t m_szBuffer[t_nBufferLength];

    CA2WEX(_In_z_ LPCSTR psz)
    {
        Init(psz, CP_ACP);
    }

    CA2WEX(_In_z_ LPCSTR psz, _In_ UINT nCodePage)
    {
        Init(psz, nCodePage);
    }

    ~CA2WEX() noexcept
    {
        if (m_psz != m_szBuffer)
            free(m_psz);
    }

    _Ret_z_ operator LPWSTR() const noexcept
    {
        return m_psz;
    }

private:
    // CA2WEX is not copyable
    CA2WEX(_In_ const CA2WEX&) noexcept = delete;
    CA2WEX& operator=(_In_ const CA2WEX&) noexcept = delete;

    void Init(_In_z_ LPCSTR psz, _In_ UINT nCodePage)
    {
        if (!psz)
        {
            m_psz = NULL;
            m_szBuffer[0] = 0;
            return;
        }

#if 1
        int cchMax = lstrlenA(psz) + 1; // This is 3 times faster
#else
        int cchMax = MultiByteToWideChar(nCodePage, 0, psz, -1, NULL, 0); // It's slow
#endif
        if (cchMax <= (int)_countof(m_szBuffer))
        {
            // Use the static buffer
            m_psz = m_szBuffer;
            cchMax = _countof(m_szBuffer);
        }
        else
        {
            // Allocate a new buffer
            m_szBuffer[0] = 0;
            m_psz = (LPWSTR)malloc(cchMax * sizeof(WCHAR));
            if (!m_psz)
                AtlThrow(E_OUTOFMEMORY);
        }

        MultiByteToWideChar(nCodePage, 0, psz, -1, m_psz, cchMax);
        m_psz[cchMax - 1] = 0;
    }
};

template <int t_nBufferLength = 128>
class CW2AEX
{
public:
    LPSTR m_psz;
    char m_szBuffer[t_nBufferLength];

    CW2AEX(_In_z_ LPCWSTR psz)
    {
        Init(psz, CP_ACP);
    }

    CW2AEX(_In_z_ LPCWSTR psz, _In_ UINT nCodePage)
    {
        Init(psz, nCodePage);
    }

    ~CW2AEX() noexcept
    {
        if (m_psz != m_szBuffer)
            free(m_psz);
    }

    _Ret_z_ operator LPSTR() const noexcept
    {
        return m_psz;
    }

private:
    // CW2AEX is not copyable
    CW2AEX(_In_ const CW2AEX&) noexcept = delete;
    CW2AEX& operator=(_In_ const CW2AEX&) noexcept = delete;

    void Init(_In_z_ LPCWSTR psz, _In_ UINT nConvertCodePage)
    {
        if (!psz)
        {
            m_psz = NULL;
            m_szBuffer[0] = 0;
            return;
        }

        // NOTE: This has a failure.
        int cchMax = WideCharToMultiByte(nConvertCodePage, 0, psz, -1, NULL, 0, NULL, NULL);
        if (cchMax <= (int)_countof(m_szBuffer))
        {
            // Use the static buffer
            m_psz = m_szBuffer;
            cchMax = _countof(m_szBuffer);
        }
        else
        {
            // Allocate a new buffer
            m_szBuffer[0] = 0;
            m_psz = (LPSTR)malloc(cchMax * sizeof(CHAR));
            if (!m_psz)
                AtlThrow(E_OUTOFMEMORY);
        }

        WideCharToMultiByte(nConvertCodePage, 0, psz, -1, m_psz, cchMax, NULL, NULL);
        m_psz[cchMax - 1] = 0;
    }
};

typedef CA2AEX<> CA2A;
typedef CW2AEX<> CW2A;
typedef CA2WEX<> CA2W;
typedef CW2WEX<> CW2W;
typedef CA2CAEX<> CA2CA;
typedef CW2CWEX<> CW2CW;

#ifdef UNICODE
    #define CA2CTEX CA2WEX
    #define CA2TEX  CA2WEX
    #define CT2AEX  CW2AEX
    #define CT2CAEX CW2AEX
    #define CT2CWEX CW2CWEX
    #define CT2WEX  CW2WEX
    #define CW2CTEX CW2CWEX
    #define CW2CTEX CW2CWEX
#else
    #define CA2CTEX CA2CAEX
    #define CA2TEX  CA2AEX
    #define CT2AEX  CA2AEX
    #define CT2CAEX CA2CAEX
    #define CT2CWEX CA2WEX
    #define CT2WEX  CA2WEX
    #define CW2CTEX CW2AEX
    #define CW2TEX  CW2AEX
#endif

} // namespace ATL

#endif // ndef __ATLCONV_H__
