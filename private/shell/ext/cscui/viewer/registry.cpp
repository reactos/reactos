//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       registry.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "registry.h"


RegKey::RegKey(
    void
    ) : m_hkeyRoot(NULL),
        m_hkey(NULL),
        m_hChangeEvent(NULL)
{
    DBGTRACE((DM_REG, DL_MID, TEXT("RegKey::RegKey [default]")));
}


RegKey::RegKey(
    HKEY hkeyRoot,
    LPCTSTR pszSubKey
    ) : m_hkeyRoot(hkeyRoot),
        m_hkey(NULL),
        m_strSubKey(pszSubKey),
        m_hChangeEvent(NULL)
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

    DWORD dwResult     = ERROR_SUCCESS;
    HKEY hkeyRoot      = m_hkeyRoot; // Assume we'll use m_hkeyRoot;
    bool bCloseRootKey = false;
    
    Close();
    if (HKEY_CURRENT_USER == hkeyRoot)
    {
        //
        // Special-case HKEY_CURRENT_USER.
        // Since we're running from winlogon and need to open
        // the user hive during tricky times we need to
        // call RegOpenCurrentUser().  If it's successful all
        // we do is replace the hkeyRoot member with the
        // returned key.  From here on out things remain
        // unchanged.
        //
        if (ERROR_SUCCESS == RegOpenCurrentUser(samDesired, &hkeyRoot))
        {
            bCloseRootKey = true;
        }
    }

    dwResult = RegOpenKeyEx(hkeyRoot,
                            (LPCTSTR)m_strSubKey,
                            0,
                            samDesired,
                            &m_hkey);
                            
    if ((ERROR_FILE_NOT_FOUND == dwResult) && bCreate)
    {
        DWORD dwDisposition;
        dwResult = RegCreateKeyEx(hkeyRoot,
                                 (LPCTSTR)m_strSubKey,
                                 0,
                                 NULL,
                                 0,
                                 samDesired,
                                 NULL,
                                 &m_hkey,
                                 &dwDisposition);
    }
    if (bCloseRootKey)
    {
        RegCloseKey(hkeyRoot);
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

    if (NULL != m_hChangeEvent)
    {
        //
        // Once key is closed, the change-notification mechanism is 
        // disabled.  No need for the change event object.
        //
        CloseHandle(m_hChangeEvent);
        m_hChangeEvent = NULL;
    }

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
// Determine if a particular registry value exists.
//
HRESULT
RegKey::ValueExists(
    LPCTSTR pszValueName
    ) const
{
    DWORD dwResult = ERROR_SUCCESS;
    int iValue = 0;
    TCHAR szValue[MAX_PATH];
    DWORD cchValue;
    while(ERROR_SUCCESS == dwResult)
    {
        cchValue = ARRAYSIZE(szValue);
        dwResult = RegEnumValue(m_hkey, 
                                iValue++, 
                                szValue, 
                                &cchValue,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
        if (ERROR_SUCCESS == dwResult && (0 == lstrcmpi(pszValueName, szValue)))
        {
            DBGPRINT((DM_REG, DL_LOW, TEXT("Reg value \"%s\" exists."), pszValueName));
            return S_OK;
        }
    }
    if (ERROR_NO_MORE_ITEMS == dwResult)
    {
        DBGPRINT((DM_REG, DL_LOW, TEXT("Reg value \"%s\" doesn't exist."), pszValueName));
        return S_FALSE;
    }

    return HRESULT_FROM_WIN32(dwResult);
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
        LPTSTR psz = pstrDataOut->GetBuffer(cch);
        hr = GetValue(pszValueName, 
                      REG_SZ, 
                      (LPBYTE)psz, 
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

HRESULT 
RegKey::MonitorChanges(
    DWORD dwChangeFilter, 
    bool bWatchSubtree
    )
{
    DBGTRACE((DM_REG, DL_HIGH, TEXT("RegKey::MonitorChanges")));
    DWORD dwResult = 0;
    if (NULL == m_hChangeEvent)
    {
        //
        // Need a NULL DACL on this object because this code is run
        // from within winlogon.exe.
        //
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle       = FALSE;
        sa.nLength              = sizeof(sa);

        m_hChangeEvent = ::CreateEvent(&sa,
                                       FALSE,  // auto reset
                                       FALSE,  // initially reset
                                       NULL);  // unnamed
        if (NULL == m_hChangeEvent)
        {
            dwResult = GetLastError();
            DBGERROR((TEXT("CreateEvent failed with error %d"), dwResult));
        }
    }
    if (NULL != m_hChangeEvent)
    {
        dwResult = (DWORD)::RegNotifyChangeKeyValue(m_hkey,
                                                    bWatchSubtree,
                                                    dwChangeFilter,
                                                    m_hChangeEvent,
                                                    true);

        if (ERROR_SUCCESS != dwResult)
        {
            DBGERROR((TEXT("RegNotifyChangeKeyValue failed with error %d"), dwResult));
        }
    }
    return HRESULT_FROM_WIN32(dwResult);
}


bool
RegKey::HasChanged(
    void
    )
{
    return NULL != m_hChangeEvent && WAIT_OBJECT_0 == ::WaitForSingleObject(m_hChangeEvent, 0);
}


HRESULT
RegKey::DeleteAllValues(
    int *pcNotDeleted
    )
{
    HRESULT hr;
    KEYINFO ki;
    if (NULL != pcNotDeleted)
        *pcNotDeleted = 0;

    hr = QueryInfo(&ki, QIM_VALUENAMEMAXCHARCNT);
    if (FAILED(hr))
        return hr;

    CString strName;
    DWORD cchValueName;
    DWORD dwError = ERROR_SUCCESS;
    int cNotDeleted = 0;
    int iValue = 0;
    while(ERROR_SUCCESS == dwError)
    {
        cchValueName = ki.cchValueNameMax + 1;
        dwError = RegEnumValue(m_hkey,
                               iValue,
                               strName.GetBuffer(cchValueName),
                               &cchValueName,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        strName.ReleaseBuffer();
        if (ERROR_SUCCESS == dwError)
        {
            if (FAILED(hr = DeleteValue(strName)))
            {
                cNotDeleted++;
                iValue++;       // Skip to next value to avoid infinite loop.
                DBGERROR((TEXT("Error %d deleting reg value \"%s\""),
                         HRESULT_CODE(hr), strName.Cstr()));
            }
        }
    }
    if (ERROR_NO_MORE_ITEMS == dwError)
        dwError = ERROR_SUCCESS;

    if (pcNotDeleted)
        *pcNotDeleted = cNotDeleted;

    return HRESULT_FROM_WIN32(dwError);
}



HRESULT 
RegKey::DeleteValue(
    LPCTSTR pszValue
    )
{
    return HRESULT_FROM_WIN32(RegDeleteValue(m_hkey, pszValue));
}


HRESULT 
RegKey::QueryInfo(
    RegKey::KEYINFO *pInfo,
    DWORD fMask
    ) const
{
    LONG lResult = RegQueryInfoKey(m_hkey,
                                   NULL,
                                   NULL,
                                   NULL,
                                   (QIM_SUBKEYCNT & fMask) ? &pInfo->cSubKeys : NULL,
                                   (QIM_SUBKEYMAXCHARCNT & fMask) ? &pInfo->cchSubKeyMax : NULL,
                                   (QIM_CLASSMAXCHARCNT & fMask) ? &pInfo->cchClassMax : NULL,
                                   (QIM_VALUECNT & fMask) ? &pInfo->cValues : NULL,
                                   (QIM_VALUENAMEMAXCHARCNT & fMask) ? &pInfo->cchValueNameMax : NULL,
                                   (QIM_VALUEMAXBYTECNT & fMask) ? &pInfo->cbValueMax : NULL,
                                   (QIM_SECURITYBYTECNT & fMask) ? &pInfo->cbSecurityDesc : NULL,
                                   (QIM_LASTWRITETIME & fMask) ? &pInfo->LastWriteTime : NULL);

    return HRESULT_FROM_WIN32(lResult);
}


RegKey::ValueIterator::ValueIterator(
    RegKey& key
    ) : m_key(key),
        m_iValue(0),
        m_cchValueName(0)
{ 
    KEYINFO ki;
    if (SUCCEEDED(key.QueryInfo(&ki, QIM_VALUENAMEMAXCHARCNT)))
    {
        m_cchValueName = ki.cchValueNameMax + 1;
    }
}


//
// Returns:  S_OK    = More items.
//           S_FALSE = No more items.
HRESULT
RegKey::ValueIterator::Next(
    CString *pstrName, 
    LPDWORD pdwType, 
    LPBYTE pbData, 
    LPDWORD pcbData
    )
{
    HRESULT hr = S_OK;
    DWORD cchValueName = m_cchValueName;
    LONG lResult = RegEnumValue(m_key,
                                m_iValue,
                                pstrName->GetBuffer(m_cchValueName),
                                &cchValueName,
                                NULL,
                                pdwType,
                                pbData,
                                pcbData);

    pstrName->ReleaseBuffer();
    switch(lResult)
    {
        case ERROR_SUCCESS:
            m_iValue++;
            break;
        case ERROR_NO_MORE_ITEMS:
            hr = S_FALSE;
            break;
        default:
            DBGERROR((TEXT("Error %d enumerating reg value \"%s\""), 
                     lResult, m_key.SubKeyName().Cstr()));
            hr = HRESULT_FROM_WIN32(lResult);
            break;
    }
    return hr;
};


RegKeyCU::RegKeyCU(
    REGSAM samDesired
    ) : m_hkey(NULL)
{
    RegOpenCurrentUser(samDesired, &m_hkey);
}

RegKeyCU::~RegKeyCU(
    void
    )
{
    if (NULL != m_hkey)
        RegCloseKey(m_hkey);
}


