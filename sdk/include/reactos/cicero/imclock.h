/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Locking and Unlocking IMC and IMCC handles
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

// class IMCCLOCK<T_DATA>;
// class IMCCLock<T_DATA>;
// class _IMCLock;
// class IMCLock;

template <typename T_DATA>
class IMCCLOCK
{
protected:
    T_DATA *m_pIMCC;

public:
    HIMCC m_hIMCC;
    HRESULT m_hr;

    IMCCLOCK(HIMCC hIMCC)
    {
        m_pIMCC = NULL;
        m_hr = S_OK;
        m_hIMCC = hIMCC;
    }
};

template <typename T_DATA>
class IMCCLock : public IMCCLOCK<T_DATA>
{
public:
    IMCCLock(HIMCC hIMCC) : IMCCLOCK<T_DATA>(hIMCC)
    {
        if (hIMCC)
            _LockIMCC(this->m_hIMCC, &this->m_pIMCC);
    }
    ~IMCCLock()
    {
        unlock();
    }

    void unlock()
    {
        if (this->m_pIMCC)
        {
            _UnlockIMCC(this->m_hIMCC);
            this->m_pIMCC = NULL;
        }
    }

    operator T_DATA*() const
    {
        return this->m_pIMCC;
    }
    T_DATA& get() const
    {
        return *this->m_pIMCC;
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

class IMCLOCK
{
protected:
    LPINPUTCONTEXTDX m_pIC;

public:
    HIMC m_hIMC;
    HRESULT m_hr;
    DWORD m_dw3;

    IMCLOCK(HIMC hIMC)
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

class IMCLock : public IMCLOCK
{
public:
    IMCLock(HIMC hIMC) : IMCLOCK(hIMC)
    {
        m_hr = _LockIMC(hIMC, &m_pIC);
    }
    ~IMCLock()
    {
        unlock();
    }

    void unlock()
    {
        if (m_pIC)
        {
            _UnlockIMC(m_hIMC);
            m_pIC = NULL;
        }
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

        IMCCLock<COMPOSITIONSTRING> imccLock(m_pIC->hCompStr);
        if (!imccLock)
            return FALSE;

        return imccLock.get().dwCompStrLen > 0;
    }

    BOOL UseVerticalCompWindow() const
    {
        return m_pIC->cfCompForm.dwStyle && ((m_pIC->lfFont.A.lfEscapement / 900) % 4 == 3);
    }

    operator INPUTCONTEXTDX*() const
    {
        return m_pIC;
    }
    INPUTCONTEXTDX& get() const
    {
        return *m_pIC;
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
