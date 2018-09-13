// MLSWalk.cpp : Implementation of CMLStrWalkW
#include "private.h"
#include "mlswalk.h"

/////////////////////////////////////////////////////////////////////////////
// CMLStrWalkW

BOOL CMLStrWalkW::Lock(HRESULT& rhr)
{
    if (m_pszBuf)
        rhr = E_FAIL; // Already locked

    if (SUCCEEDED(rhr) &&
        m_lLen > 0 &&
        FAILED(rhr = m_pMLStr->LockWStr(m_lPos, m_lLen, m_lFlags, 0, &m_pszBuf, &m_cchBuf, &m_lLockLen)))
    {
        m_pszBuf = NULL; // Mark as it's not locked
    }

    if (m_fCanStopAtMiddle && FAILED(rhr) && m_lDoneLen > 0)
    {
        rhr = S_OK;
        return FALSE; // Stop it, but not fail
    }
    else
    {
        return (SUCCEEDED(rhr) && m_lLen > 0);
    }
}

void CMLStrWalkW::Unlock(HRESULT& rhr, long lActualLen)
{
    HRESULT hr = S_OK;

    if (!m_pszBuf)
        hr = E_FAIL; // Not locked yet

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = m_pMLStr->UnlockWStr(m_pszBuf, 0, NULL, NULL))) // Unlock even if rhr is already failed
    {
        if (!lActualLen)
            lActualLen = m_lLockLen;
        else
            ASSERT(lActualLen > 0 && lActualLen <= m_lLockLen);

        m_lPos += lActualLen;
        m_lLen -= lActualLen;
        m_lDoneLen += lActualLen;
    }

    m_pszBuf = NULL; // Unlock anyway

    if (SUCCEEDED(rhr))
        rhr = hr; // if rhr is failed before UnlockBuf, use it
}
