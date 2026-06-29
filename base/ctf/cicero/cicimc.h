/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Locking and Unlocking IMC and IMCC handles
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

// struct CTFIMECONTEXT;
// class CIC_IMCC_LOCK<T_DATA>;
// class CicIMCCLock<T_DATA>;
// class CIC_IMC_LOCK;
// class CicIMCLock;

class CicInputContext;

typedef struct tagCTFIMECONTEXT
{
    CicInputContext *m_pCicIC;
    DWORD m_dwCicFlags;
} CTFIMECONTEXT, *PCTFIMECONTEXT;

template <typename T_DATA>
class CIC_IMCC_LOCK
{
protected:
    T_DATA *m_pIMCC;

public:
    HIMCC m_hIMCC;
    HRESULT m_hr;

    CIC_IMCC_LOCK(HIMCC hIMCC)
    {
        m_pIMCC = NULL;
        m_hr = S_OK;
        m_hIMCC = hIMCC;
    }
};

template <typename T_DATA>
class CicIMCCLock : public CIC_IMCC_LOCK<T_DATA>
{
public:
    CicIMCCLock(HIMCC hIMCC) : CIC_IMCC_LOCK<T_DATA>(hIMCC)
    {
        this->m_hr = _LockIMCC(this->m_hIMCC, &this->m_pIMCC);
    }
    ~CicIMCCLock()
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

class CIC_IMC_LOCK
{
protected:
    LPINPUTCONTEXTDX m_pIC;

public:
    HIMC m_hIMC;
    HRESULT m_hr;
    DWORD m_dw3;

    CIC_IMC_LOCK(HIMC hIMC)
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

class CicIMCLock : public CIC_IMC_LOCK
{
public:
    CicIMCLock(HIMC hIMC) : CIC_IMC_LOCK(hIMC)
    {
        m_hr = _LockIMC(hIMC, &m_pIC);
    }
    ~CicIMCLock()
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

        CicIMCCLock<COMPOSITIONSTRING> imccLock(m_pIC->hCompStr);
        if (!imccLock)
            return FALSE;

        return imccLock.get().dwCompStrLen > 0;
    }

    BOOL ClearCand();

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
