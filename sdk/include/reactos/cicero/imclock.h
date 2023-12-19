/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Locking and Unlocking IMC and IMCC handles
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

// class _IMCCLock<T_DATA>;
// class InternalIMCCLock<T_DATA>;
// class _IMCLock;
// class IMCLock;

template <typename T_DATA>
class _IMCCLock
{
public:
    T_DATA *m_pIMCC;
    HIMCC m_hIMCC;
    HRESULT m_hr;

    _IMCCLock(HIMCC hIMCC)
    {
        m_pIMCC = NULL;
        m_hr = S_OK;
        m_hIMCC = hIMCC;
    }
};

template <typename T_DATA>
class InternalIMCCLock : public _IMCCLock<T_DATA>
{
public:
    InternalIMCCLock(HIMCC hIMCC) : _IMCCLock<T_DATA>(hIMCC)
    {
        if (hIMCC)
            _LockIMCC(this->m_hIMCC, &this->m_pIMCC);
    }
    ~InternalIMCCLock()
    {
        if (this->m_pIMCC)
            _UnlockIMCC(this->m_hIMCC);
    }
    operator T_DATA*() const
    {
        return this->m_pIMCC;
    }

protected:
    HRESULT _LockIMCC(HIMCC hIMCC, T_DATA **pptr)
    {
        if (!hIMCC)
            return E_INVALIDARG;
        *pptr = (T_DATA*)::ImmLockIMCC(hIMCC);
        return (*pptr ? S_OK : E_FAIL);
    }
    HRESULT _UnlockIMCC(HIMCC hIMCC)
    {
        if (!::ImmUnlockIMCC(hIMCC))
            return (::GetLastError() ? E_FAIL : S_OK);
        return S_OK;
    }
};

class _IMCLock
{
public:
    LPINPUTCONTEXTDX m_pIC;
    HIMC m_hIMC;
    HRESULT m_hr;
    DWORD m_dw3;

    _IMCLock(HIMC hIMC)
    {
        m_pIC = NULL;
        m_hIMC = hIMC;
        m_hr = S_OK;
        m_dw3 = 0;
    }

    BOOL Invalid() const
    {
        return (!m_pIC || m_hr != S_OK);
    }
};

class IMCLock : public _IMCLock
{
public:
    IMCLock(HIMC hIMC) : _IMCLock(hIMC)
    {
        m_hr = _LockIMC(hIMC, &m_pIC);
    }
    ~IMCLock()
    {
        if (m_pIC)
            _UnlockIMC(m_hIMC);
    }

    void InitContext()
    {
        if (!(m_pIC->fdwInit & INIT_COMPFORM))
            m_pIC->cfCompForm.dwStyle = 0;
        for (UINT i = 0; i < 4; ++i)
            m_pIC->cfCandForm[i].dwStyle = 0;
    }

    BOOL ValidCompositionString()
    {
        if (ImmGetIMCCSize(m_pIC->hCompStr) < sizeof(COMPOSITIONSTRING))
            return FALSE;

        InternalIMCCLock<COMPOSITIONSTRING> imccLock(m_pIC->hCompStr);
        if (!imccLock)
            return FALSE;

        return imccLock.m_pIMCC->dwCompStrLen > 0;
    }

    BOOL UseVerticalCompWindow() const
    {
        return m_pIC->cfCompForm.dwStyle && ((m_pIC->lfFont.A.lfEscapement / 900) % 4 == 3);
    }

    operator INPUTCONTEXTDX*() const
    {
        return m_pIC;
    }

protected:
    HRESULT _LockIMC(HIMC hIMC, LPINPUTCONTEXTDX *ppIC)
    {
        if (!hIMC)
            return E_INVALIDARG;

        LPINPUTCONTEXTDX pIC = (LPINPUTCONTEXTDX)ImmLockIMC(hIMC);
        *ppIC = pIC;
        return (pIC ? S_OK : E_FAIL);
    }
    HRESULT _UnlockIMC(HIMC hIMC)
    {
        return ::ImmUnlockIMC(hIMC) ? S_OK : E_FAIL;
    }
};
