#include "private.h"

#define TF_THISMODULE TF_WEBCHECKCORE

//////////////////////////////////////////////////////////////////////////
//
// CMemStream implementation
//
//////////////////////////////////////////////////////////////////////////

CMemStream::CMemStream(BOOL fNewStream)
{
    // We don't start dirty
    m_fDirty = FALSE;
    m_pstm = NULL;
    m_fNewStream = fNewStream;
    m_fError = FALSE;

    if (fNewStream)
    {
        // Create new stream
        if (FAILED(CreateStreamOnHGlobal(NULL, TRUE, &m_pstm)))
        {
            m_pstm = NULL;
            m_fError = TRUE;
            DBG_WARN("CMemStream CreateStream fail");
        }
    }
}

CMemStream::~CMemStream()
{
    DWORD   dwSize = 0;

    // Free up stream
    if (m_pstm)
    {
        m_pstm->Release();
    }

#ifdef DEBUG
    if (m_fDirty)
        DBG_WARN("CMemStream destructor with dirty data");
    if (m_fError)
        DBG_WARN("CMemStream encountered read/write error");
#endif
}

HRESULT CMemStream::CopyToStream(IStream *pStm)
{
    char *  pData;
    DWORD   dwSize = 0;
    HLOCAL hGlobal=NULL;

    if (m_pstm)
    {
        // get global handle from stream
        GetHGlobalFromStream(m_pstm, &hGlobal);

        // find how much data we have
        STATSTG stats;
        m_pstm->Stat( &stats, STATFLAG_NONAME );
        dwSize = stats.cbSize.LowPart;

        // write out data
        ASSERT(hGlobal);

        pData = (char *)GlobalLock(hGlobal);
        if (pData)
            pStm->Write(pData, dwSize, NULL);

        GlobalUnlock(hGlobal);

        m_fDirty = FALSE;
    }

    return NOERROR;
}


HRESULT CMemStream::Read(void *pv, ULONG cb, ULONG *cbRead)
{
    HRESULT hr;

    // make sure we have a stream
    if(NULL == m_pstm)
        return ResultFromScode(E_OUTOFMEMORY);

    hr = m_pstm->Read(pv, cb, cbRead);
    if (FAILED(hr) || *cbRead!=cb)
        m_fError=TRUE;

    return hr;
}

HRESULT CMemStream::Write(void *pv, ULONG cb, ULONG *cbWritten)
{
    HRESULT hr;

    // make sure we have a stream
    if(NULL == m_pstm)
        return ResultFromScode(E_OUTOFMEMORY);

    m_fDirty = TRUE;

    hr = m_pstm->Write(pv, cb, cbWritten);
    if (FAILED(hr) || *cbWritten != cb)
        m_fError=TRUE;

    return hr;
}

HRESULT CMemStream::SaveToStream(IUnknown *punk)
{
    IPersistStream *    pips;
    HRESULT             hr;

    // make sure we have a stream
    if(NULL == m_pstm)
        return ResultFromScode(E_OUTOFMEMORY);

    // Get IPersistStream interface and save!
    hr = punk->QueryInterface(IID_IPersistStream, (void **)&pips);
    if(SUCCEEDED(hr))
        hr = OleSaveToStream(pips, m_pstm);

    if(pips)
        pips->Release();

    if (FAILED(hr))
        m_fError=TRUE;

    return hr;
}

HRESULT CMemStream::LoadFromStream(IUnknown **ppunk)
{
    HRESULT hr;

    // make sure we have a stream
    if(NULL == m_pstm)
        return ResultFromScode(E_OUTOFMEMORY);

    hr = OleLoadFromStream(m_pstm, IID_IUnknown, (void **)ppunk);

    if (FAILED(hr))
        m_fError=TRUE;

    return hr;
}

HRESULT CMemStream::Seek(long lMove, DWORD dwOrigin, DWORD *dwNewPos)
{
    LARGE_INTEGER   liDist;
    ULARGE_INTEGER  uliNew;
    HRESULT         hr;

    LISet32(liDist, lMove);
    hr = m_pstm->Seek(liDist, dwOrigin, &uliNew);
    if(dwNewPos)
        *dwNewPos = uliNew.LowPart;

    return hr;
}

//////////////////////////////////////////////////////////////////////////
//
// CRegStream implementation
//
//////////////////////////////////////////////////////////////////////////

CRegStream::CRegStream(HKEY hBaseKey, LPCTSTR pszRegKey, LPCTSTR pszSubKey, BOOL fForceNewStream) :
                CMemStream(fForceNewStream)
{
    long    lRes;
    DWORD   dwDisposition, dwType, dwSize;
    char *  pData;
    HLOCAL hGlobal;

    m_hKey = NULL;

    lstrcpyn(m_pszSubKey, pszSubKey, ARRAYSIZE(m_pszSubKey));//BUGBUG-FIXED-OVERFLOW

    lRes = RegCreateKeyEx(
                    hBaseKey,
                    pszRegKey,
                    0,
                    NULL,
                    0,
                    KEY_QUERY_VALUE | KEY_SET_VALUE,
                    NULL,
                    &m_hKey,
                    &dwDisposition
                    );

    if(lRes != ERROR_SUCCESS)
        return;

    if (!fForceNewStream)
    {
        // Initialize our stream from the registry
        m_fError=TRUE;  // assume failure

        // Find reg data size
        lRes = RegQueryValueEx(m_hKey, m_pszSubKey, NULL, &dwType, NULL, &dwSize);

        if(lRes != ERROR_SUCCESS) {
            // nothing in there at the moment - create new stream
            DBG("CRegStream creating new stream");
            m_fNewStream = TRUE;
            if(FAILED(CreateStreamOnHGlobal(NULL, TRUE, &m_pstm)))
            {
                m_pstm = NULL;
            }
            else
                m_fError=FALSE;
        }
        else
        {
            // Get global handle to data
            hGlobal = MemAlloc(LMEM_MOVEABLE, dwSize);
            if(NULL == hGlobal)
            {
                return;
            }

            pData = (char *)GlobalLock(hGlobal);

            lRes = RegQueryValueEx(m_hKey, m_pszSubKey, NULL, &dwType,
                        (BYTE *)pData, &dwSize);
            GlobalUnlock(hGlobal);

            if (lRes != ERROR_SUCCESS)
            {
                MemFree(hGlobal);
                return;
            }

            // make stream on global handle
            if (FAILED(CreateStreamOnHGlobal(hGlobal, TRUE, &m_pstm)))
            {
                m_pstm = NULL;
                MemFree(hGlobal);
            }
            else
                m_fError=FALSE;
        }
    }
}

CRegStream::~CRegStream()
{
    char *  pData;
    DWORD   dwSize = 0;
    HLOCAL hGlobal=NULL;

    // Save stream to registry if it's changed
    if (m_pstm && m_fDirty)
    {
        GetHGlobalFromStream(m_pstm, &hGlobal);

        STATSTG stats;
        m_pstm->Stat( &stats, STATFLAG_NONAME );
        dwSize = stats.cbSize.LowPart;

        pData = (char *)GlobalLock(hGlobal);
        if(pData)
            RegSetValueEx(m_hKey, m_pszSubKey, NULL, REG_BINARY,
                          (const BYTE *)pData, dwSize);
        GlobalUnlock(hGlobal);
        m_fDirty = FALSE;
    }

    // clean up
    if(m_hKey)
        RegCloseKey(m_hKey);
}
