HRESULT CBufferedString::SetBSTR(BSTR bstr)
{
    int len = lstrlenW(bstr) + 1;
    if (len < ARRAYSIZE(m_szTmpBuf)) {
        if (m_fFree) {
            LocalFree(m_pBuf);
            m_pBuf = m_szTmpBuf;
            m_fFree = FALSE;
        }
    } else {
        if (m_fFree) {
            LPTSTR psz = (LPTSTR)LocalReAlloc(m_pBuf, len * SIZEOF(TCHAR), LMEM_MOVEABLE | LMEM_ZEROINIT);
            if (psz) {
                m_pBuf = psz;
            }
            else {
                return E_OUTOFMEMORY;
            }
        } else {
            LPTSTR psz = (LPTSTR)LocalAlloc(LPTR, len * SIZEOF(TCHAR));
            if (psz) {
                m_pBuf = psz;
                m_fFree = TRUE;
            }
            else {
                return E_OUTOFMEMORY;
            }
        }
    }
#ifdef UNICODE
    lstrcpy(m_pBuf, bstr);
#else
    WideCharToMultiByte(CP_ACP, 0, bstr, -1, m_pBuf, len, NULL, NULL);
#endif
    m_fSet = TRUE;
    return S_OK;
}

