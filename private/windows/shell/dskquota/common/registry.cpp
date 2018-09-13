#include "pch.h"
#pragma hdrstop

#include "registry.h"

RegKey::RegKey(
    void
    ) : m_hkeyRoot(NULL),
        m_hkey(NULL),
        m_hChangeEvent(NULL),
        m_bWatchSubtree(false),
        m_dwChangeFilter(0)
{
    DBGTRACE((DM_REG, DL_MID, TEXT("RegKey::RegKey [default]")));
}


RegKey::RegKey(
    HKEY hkeyRoot,
    LPCTSTR pszSubKey
    ) : m_hkeyRoot(hkeyRoot),
        m_hkey(NULL),
        m_strSubKey(pszSubKey),
        m_hChangeEvent(NULL),
        m_bWatchSubtree(false),
        m_dwChangeFilter(0)
{
    DBGTRACE((DM_REG, DL_MID, TEXT("RegKey::RegKey")));
    //
    // Nothing to do.
    //
}


RegKey::~RegKey(
    void
    )
{
    DBGTRACE((DM_REG, DL_MID, TEXT("RegKey::~RegKey")));
    Close();
    if (NULL != m_hChangeEvent)
        CloseHandle(m_hChangeEvent);
}


HRESULT
RegKey::Open(
    REGSAM samDesired,  // Access mask (i.e. KEY_READ, KEY_WRITE etc.)
    bool bCreate        // Create key if it doesn't exist?
    ) const
{
    DBGTRACE((DM_REG, DL_HIGH, TEXT("RegKey::Open")));
    DBGPRINT((DM_REG, DL_HIGH, TEXT("\thkeyRoot = 0x%08X, SubKey = \"%s\""),
                      m_hkeyRoot, m_strSubKey.Cstr()));

    DWORD dwResult = ERROR_SUCCESS;
    Close();
    if (bCreate)
    {
        DWORD dwDisposition;
        dwResult = RegCreateKeyEx(m_hkeyRoot,
                                 (LPCTSTR)m_strSubKey,
                                 0,
                                 NULL,
                                 0,
                                 samDesired,
                                 NULL,
                                 &m_hkey,
                                 &dwDisposition);
    }
    else
    {
        dwResult = RegOpenKeyEx(m_hkeyRoot,
                                (LPCTSTR)m_strSubKey,
                                0,
                                samDesired,
                                &m_hkey);
    }
    return HRESULT_FROM_WIN32(dwResult);
}


void 
RegKey::Attach(
    HKEY hkey
    )
{
    Close();
    m_strSubKey.Empty();
    m_hkeyRoot = NULL;
    m_hkey     = hkey;
}

void 
RegKey::Detach(
    void
    )
{
    m_hkey = NULL;
}


void
RegKey::Close(
    void
    ) const
{
    DBGTRACE((DM_REG, DL_HIGH, TEXT("RegKey::Close")));
    DBGPRINT((DM_REG, DL_HIGH, TEXT("\thkeyRoot = 0x%08X, SubKey = \"%s\""),
                      m_hkeyRoot, m_strSubKey.Cstr()));

    if (NULL != m_hkey)
    {
        //
        // Do this little swap so that the m_hkey member is NULL
        // when the actual key is being closed.  This lets the async
        // change proc determine if it was signaled because of a true
        // change or because the key was being closed.
        //
        HKEY hkeyTemp = m_hkey;
        m_hkey = NULL;
        RegCloseKey(hkeyTemp);
    }
}


//
// This is the basic form of GetValue.  All other forms of 
// GetValue() call into this one.
//
HRESULT
RegKey::GetValue(
    LPCTSTR pszValueName,
    DWORD dwTypeExpected,
    LPBYTE pbData,
    int cbData
    ) const
{
    DWORD dwType;
    DWORD dwResult = RegQueryValueEx(m_hkey,
                                     pszValueName,
                                     0,
                                     &dwType,
                                     pbData,
                                     (LPDWORD)&cbData);

    if (ERROR_SUCCESS == dwResult && dwType != dwTypeExpected)
        dwResult = ERROR_INVALID_DATATYPE;

    return HRESULT_FROM_WIN32(dwResult);
}

//
// Get a DWORD value (REG_DWORD).
//
HRESULT
RegKey::GetValue(
    LPCTSTR pszValueName,
    DWORD *pdwDataOut
    ) const
{
    return GetValue(pszValueName, REG_DWORD, (LPBYTE)pdwDataOut, sizeof(DWORD));
}

//
// Get a byte buffer value (REG_BINARY).
//
HRESULT
RegKey::GetValue(
    LPCTSTR pszValueName,
    LPBYTE pbDataOut,
    int cbDataOut
    ) const
{
    return GetValue(pszValueName, REG_BINARY, pbDataOut, cbDataOut);
}

//
// Get a text string value (REG_SZ) and write it to a CString object.
//
HRESULT
RegKey::GetValue(
    LPCTSTR pszValueName,
    CString *pstrDataOut
    ) const
{
    HRESULT hr = E_FAIL;
    int cch = GetValueBufferSize(pszValueName) / sizeof(TCHAR);
    if (NULL != pstrDataOut && 0 < cch)
    {
        hr = GetValue(pszValueName, 
                      REG_SZ, 
                      (LPBYTE)pstrDataOut->GetBuffer(cch), 
                      pstrDataOut->SizeBytes());

        pstrDataOut->ReleaseBuffer();
    }
    return hr;
}


//
// Get a multi-text string value (REG_MULTI_SZ) and write it to a CArray<CString> object.
//
HRESULT
RegKey::GetValue(
    LPCTSTR pszValueName,
    CArray<CString> *prgstrOut
    ) const
{
    HRESULT hr = E_FAIL;
    int cb = GetValueBufferSize(pszValueName);
    if (NULL != prgstrOut && 0 < cb)
    {
        array_autoptr<TCHAR> ptrTemp(new TCHAR[cb / sizeof(TCHAR)]);
        LPCTSTR psz = ptrTemp.get();
        hr = GetValue(pszValueName, REG_MULTI_SZ, (LPBYTE)psz, cb);
        if (SUCCEEDED(hr))
        {
            while(psz && TEXT('\0') != *psz)
            {
                prgstrOut->Append(CString(psz));
                psz += lstrlen(psz) + 1;
            }
        }
    }
    return hr;
}


//
// Return the required buffer size for a given registry value.
//
int
RegKey::GetValueBufferSize(
    LPCTSTR pszValueName
    ) const
{
    DWORD dwType;
    int cbData = 0;
    DWORD dwDummy;
    DWORD dwResult = RegQueryValueEx(m_hkey,
                                     pszValueName,
                                     0,
                                     &dwType,
                                     (LPBYTE)&dwDummy,
                                     (LPDWORD)&cbData);
    if (ERROR_MORE_DATA != dwResult)
        cbData = 0;

    return cbData;
}


//
// This is the basic form of SetValue.  All other forms of 
// SetValue() call into this one.
//
HRESULT
RegKey::SetValue(
    LPCTSTR pszValueName,
    DWORD dwValueType,
    const LPBYTE pbData, 
    int cbData
    )
{
    DWORD dwResult = RegSetValueEx(m_hkey,
                                   pszValueName,
                                   0,
                                   dwValueType,
                                   pbData,
                                   cbData);

    return HRESULT_FROM_WIN32(dwResult);
}
      
//
// Set a DWORD value (REG_DWORD).
//
HRESULT
RegKey::SetValue(
    LPCTSTR pszValueName,
    DWORD dwData
    )
{
    return SetValue(pszValueName, REG_DWORD, (const LPBYTE)&dwData, sizeof(dwData));
}


//
// Set a byte buffer value (REG_BINARY).
//
HRESULT
RegKey::SetValue(
    LPCTSTR pszValueName,
    const LPBYTE pbData,
    int cbData
    )
{
    return SetValue(pszValueName, REG_BINARY, pbData, cbData);
}


//
// Set a text string value (REG_SZ).
//
HRESULT
RegKey::SetValue(
    LPCTSTR pszValueName,
    LPCTSTR pszData
    )
{
    return SetValue(pszValueName, REG_SZ, (const LPBYTE)pszData, (lstrlen(pszData) + 1) * sizeof(TCHAR));
}

//
// Set a text string value (REG_MULTI_SZ).
//
HRESULT
RegKey::SetValue(
    LPCTSTR pszValueName,
    const CArray<CString>& rgstrSrc
    )
{
    array_autoptr<TCHAR> ptrValues(CreateDoubleNulTermList(rgstrSrc));
    int cch = 1;
    int n = rgstrSrc.Count();

    for (int i = 0; i < n; i++)
        cch += rgstrSrc[i].Length() + 1;

    return SetValue(pszValueName, REG_MULTI_SZ, (const LPBYTE)ptrValues.get(), cch * sizeof(TCHAR));
}


LPTSTR
RegKey::CreateDoubleNulTermList(
    const CArray<CString>& rgstrSrc
    ) const
{
    int cEntries = rgstrSrc.Count();
    int cch = 1; // Account for 2nd nul term.
    int i;
    for (i = 0; i < cEntries; i++)
        cch += rgstrSrc[i].Length() + 1;

    LPTSTR pszBuf = new TCHAR[cch];
    LPTSTR pszWrite = pszBuf;

    for (i = 0; i < cEntries; i++)
    {
        CString& s = rgstrSrc[i];
        lstrcpy(pszWrite, s);
        pszWrite += s.Length() + 1;
    }
    *pszWrite = TEXT('\0'); // Double nul term.
    return pszBuf;
}


void 
RegKey::OnChange(
    HKEY hkey
    )
{
    DBGTRACE((DM_REG, DL_HIGH, TEXT("RegKey::OnChange")));
    //
    // Default does nothing.
    //
}


DWORD
RegKey::NotifyWaitThreadProc(
    LPVOID pvParam
    )
{
    DBGTRACE((DM_REG, DL_HIGH, TEXT("RegKey::NotifyWaitThreadProc")));
    RegKey *pThis = (RegKey *)pvParam;

    while(NULL != pThis->m_hkey)
    {
        DBGPRINT((DM_REG, DL_HIGH, TEXT("RegNotifyChangeKey(0x%08X, %d, 0x%08X, 0x%08X, %d)"),
                 pThis->m_hkey, pThis->m_bWatchSubtree, pThis->m_dwChangeFilter, pThis->m_hChangeEvent, true));
        LONG lResult = RegNotifyChangeKeyValue(pThis->m_hkey,
                                               pThis->m_bWatchSubtree,
                                               pThis->m_dwChangeFilter,
                                               pThis->m_hChangeEvent,
                                               true);
        if (ERROR_SUCCESS != lResult)
        {
            DBGERROR((TEXT("RegNotifyChangeKeyValue failed with error %d"), lResult));
            return 0;
        }
        else
        {
            DBGPRINT((DM_REG, DL_MID, TEXT("Waiting for reg change notification...")));
            switch(WaitForSingleObject(pThis->m_hChangeEvent, INFINITE))
            {
                case WAIT_OBJECT_0:
                    if (NULL != pThis->m_hkey)
                    {
                        DBGPRINT((DM_REG, DL_MID, TEXT("Rcv'd reg change notification")));
                        pThis->OnChange(pThis->m_hkey);
                    }
                    break;

                case WAIT_FAILED:
                    DBGERROR((TEXT("Registry chg wait failed with error %d"), GetLastError()));
                    break;

                default:
                    break;
            }
        }
    }
    return 0;
}



HRESULT 
RegKey::WatchForChange(
    DWORD dwChangeFilter, 
    bool bWatchSubtree
    )
{
    DBGTRACE((DM_REG, DL_HIGH, TEXT("RegKey::WatchForChange")));
    HRESULT hr = E_FAIL;
    
    if (NULL != m_hChangeEvent || NULL == m_hkey)
        return E_FAIL;

    m_hChangeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == m_hChangeEvent)
    {
        DBGERROR((TEXT("CreateEvent failed error %d"), GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    m_dwChangeFilter = dwChangeFilter;
    m_bWatchSubtree  = bWatchSubtree;

    DWORD dwThreadId = 0;
    HANDLE hThread = CreateThread(NULL,
                                  0,
                                  (LPTHREAD_START_ROUTINE)NotifyWaitThreadProc,
                                  this,
                                  0,
                                  &dwThreadId);
    if (INVALID_HANDLE_VALUE != hThread)
    {
        DBGPRINT((DM_REG, DL_MID, TEXT("Reg key chg watch thread ID = %d"), dwThreadId));
        CloseHandle(hThread);
        hr = NOERROR;
    }
    return hr;
}


HRESULT 
RegKey::WaitForChange(
    DWORD dwChangeFilter, 
    bool bWatchSubtree
    )
{
    DBGTRACE((DM_REG, DL_HIGH, TEXT("RegKey::WaitForChange")));
    HRESULT hr = NOERROR;
    LONG lResult = RegNotifyChangeKeyValue(m_hkey,
                                           bWatchSubtree,
                                           dwChangeFilter,
                                           NULL,
                                           false);

    if (ERROR_SUCCESS != lResult)
    {
        DBGERROR((TEXT("RegNotifyChangeKeyValue failed with error %d"), lResult));
        hr = HRESULT_FROM_WIN32(lResult);
    }
    return hr;
}
                 

#if DBG
//-----------------------------------------------------------------------------
//                        DEBUG ONLY
//-----------------------------------------------------------------------------
RegKeyChg::RegKeyChg(
    HKEY hkeyRoot, 
    LPCTSTR pszSubKey
    ) : RegKey(hkeyRoot, pszSubKey)
{
    DBGTRACE((DM_REG, DL_HIGH, TEXT("RegKeyChg::RegKeyChg")));
    HRESULT hr = Open(KEY_READ | KEY_NOTIFY);
    if (SUCCEEDED(hr))
    {
        hr = WatchForChange(REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_NAME, true); 
        if (FAILED(hr))
        {
            DBGERROR((TEXT("WatchForChange failed with error 0x%08X"), hr));
        }
    }
    else if (ERROR_FILE_NOT_FOUND != HRESULT_CODE(hr))
    {
        DBGERROR((TEXT("Error 0x%08X opening key 0x%08X \"%s\""), hr, hkeyRoot, pszSubKey));
    }
}


RegKeyChg::~RegKeyChg(
    void
    )
{ 
    DBGTRACE((DM_REG, DL_HIGH, TEXT("RegKeyChg::~RegKeyChg"))); 
}


void
RegKeyChg::OnChange(
    HKEY hkey
    )
{
    DBGTRACE((DM_REG, DL_HIGH, TEXT("RegKeyChg::OnChange")));
    RegKey key;
    key.Attach(hkey);
    DebugRegParams dp;
    HRESULT hr = key.GetValue(REGSTR_VAL_DEBUGPARAMS, (LPBYTE)&dp, sizeof(dp));
    if (SUCCEEDED(hr))
    {
        DBGPRINT((DM_REG, DL_HIGH, TEXT("Setting new debug parameters")));
        DBGPRINTMASK(dp.PrintMask);
        DBGPRINTLEVEL(dp.PrintLevel);
        DBGPRINTVERBOSE(dp.PrintVerbose);
        DBGTRACEMASK(dp.TraceMask);
        DBGTRACELEVEL(dp.TraceLevel);
        DBGTRACEVERBOSE(dp.TraceVerbose);
        DBGTRACEONEXIT(dp.TraceOnExit);
    }
    else
        DBGERROR((TEXT("GetValue failed with error 0x%08X"), hr));

    key.Detach();
}

#endif // #if DBG