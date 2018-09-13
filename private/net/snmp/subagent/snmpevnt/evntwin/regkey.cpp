//****************************************************************************
//
//  Copyright (c) 1993,  Microsoft Corp.
//
//  File:  DOMDLGS.H
//
//  Implementation file for registry management classes
//
//  History:
//      Scott V. Walker 10/5/94
//
//****************************************************************************

#include "stdafx.h"

#include "portable.h"

#include "regkey.h"


BOOL g_bLostConnection = FALSE;


//****************************************************************************
//
//  CRegistryValue Implementation
//
//****************************************************************************

IMPLEMENT_DYNAMIC(CRegistryValue, CObject)

//****************************************************************************
//
//  CRegistryValue::CRegistryValue
//
//****************************************************************************
CRegistryValue::CRegistryValue()
{
    m_dwType = REG_NONE;
    m_dwDataLength = 0;
    m_pData = NULL;
}

//****************************************************************************
//
//  CRegistryValue::CRegistryValue
//
//****************************************************************************
CRegistryValue::CRegistryValue(LPCTSTR pszName, DWORD dwType,
    DWORD dwDataLength, LPBYTE pData)
{
    Set(pszName, dwType, dwDataLength, pData);
}

//****************************************************************************
//
//  CRegistryValue::~CRegistryValue
//
//****************************************************************************
CRegistryValue::~CRegistryValue()
{
    Empty();
}

//****************************************************************************
//
//  CRegistryValue::Set
//
//  Sets the value data fields.  The data pointed to by pData is COPIED!
//
//****************************************************************************
void CRegistryValue::Set(LPCTSTR pszName, DWORD dwType,
    DWORD dwDataLength, LPBYTE pData)
{
    Empty();

    m_sName = pszName;
    m_dwType = dwType;
    m_dwDataLength = dwDataLength;
    if (dwDataLength == 0 || pData == NULL)
        m_pData = NULL;
    else
    {
        m_pData = new BYTE[dwDataLength];
        memcpy(m_pData, pData, dwDataLength);
    }
}

//****************************************************************************
//
//  CRegistryValue::Get
//
//  Gets the value data fields.  The data pointed to by m_pData is COPIED
//  into the buffer pointed to by pData... this buffer better be big enough!
//  If pData is NULL, no copy is performed.
//
//****************************************************************************
void CRegistryValue::Get(CString &sName, DWORD &dwType,
    DWORD &dwDataLength, LPBYTE pData)
{
    sName = m_sName;
    dwType = m_dwType;
    dwDataLength = m_dwDataLength;
    if (dwDataLength != 0 && pData != NULL)
        memcpy(pData, m_pData, m_dwDataLength);
}

//****************************************************************************
//
//  CRegistryValue::Empty
//
//  Clear the value data and deletes its data buffer.
//
//****************************************************************************
void CRegistryValue::Empty()
{
    m_sName.Empty();
    m_dwType = REG_NONE;
    m_dwDataLength = 0;
    if (m_pData != NULL)
        delete m_pData;
    m_pData = NULL;
}

//****************************************************************************
//
//  CRegistryValue::operator=
//
//  Assignment operator.  Copies CRegistryValue object.
//
//****************************************************************************
const CRegistryValue& CRegistryValue::operator=(CRegistryValue &other)
{
    Set(other.m_sName, other.m_dwType, other.m_dwDataLength, other.m_pData);

    return *this;
}

//****************************************************************************
//
//  CRegistryKey Implementation
//
//****************************************************************************

IMPLEMENT_DYNAMIC(CRegistryKey, CObject)

//****************************************************************************
//
//  CRegistryKey::CRegistryKey
//
//****************************************************************************
CRegistryKey::CRegistryKey()
{
    // The lost connection status is initialized only once so that if a connection
    // is ever lost we won't waste any time trying to close keys.

    Initialize();
}

//****************************************************************************
//
//  CRegistryKey::Initialize
//
//****************************************************************************
void CRegistryKey::Initialize()
{
    m_bConnected = FALSE;
    m_bOpen = FALSE;
    m_bLocal = TRUE;
    m_bDirty = FALSE;

    m_hkeyConnect = NULL;
    m_hkeyRemote = NULL;
    m_hkeyOpen = NULL;
    m_Sam = 0;

    m_dwSubKeys = 0;
    m_dwMaxSubKey = 0;
    m_dwMaxClass = 0;
    m_dwValues = 0;
    m_dwMaxValueName = 0;
    m_dwMaxValueData = 0;
    m_dwSecurityDescriptor = 0;

    m_ftLastWriteTime.dwLowDateTime = 0;
    m_ftLastWriteTime.dwHighDateTime = 0;

    m_lResult = ERROR_SUCCESS;
}

//****************************************************************************
//
//  CRegistryKey::~CRegistryKey
//
//  Destructor.
//
//****************************************************************************
CRegistryKey::~CRegistryKey()
{
    if (g_bLostConnection) {
        // If we lost the registry connection, it will be useless to do anything.
        return;
    }

    // If we're currently open, then close.
    if (m_bOpen)
        Close(TRUE);

    // If we're currently connected, then disconnect.
    if (m_bConnected)
        Disconnect(TRUE);
}

//****************************************************************************
//
//  CRegistryKey::Connect
//
//****************************************************************************
LONG CRegistryKey::Connect(LPCTSTR pszComputer, HKEY hkey)
{
    if (g_bLostConnection) {
        return RPC_S_SERVER_UNAVAILABLE;
    }

    TCHAR szName[MAX_COMPUTERNAME_LENGTH + 1];
    CString sComputer;
    HKEY hkeyRemote;
    DWORD dwNumChars;

    m_lResult = ERROR_SUCCESS;

    if (m_bConnected)
    {
        m_lResult = Disconnect(TRUE);
        if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
            g_bLostConnection = TRUE;
        }
        if (m_lResult != ERROR_SUCCESS)
            return m_lResult;
    }

    // Is this the local machine?

    dwNumChars = MAX_COMPUTERNAME_LENGTH + 1;

    sComputer = pszComputer;
    GetComputerName(szName, &dwNumChars);
    if (sComputer.IsEmpty() || !lstrcmpi(pszComputer, szName))
    {
        // Local

        m_bLocal = TRUE;
        hkeyRemote = NULL;
    }
    else
    {
        // Remote

        m_bLocal = FALSE;
        m_lResult = RegConnectRegistry(pszComputer, hkey, &hkeyRemote);

        if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
            g_bLostConnection = TRUE;
        }
        if (m_lResult != ERROR_SUCCESS)
            return m_lResult;
        lstrcpy(szName, pszComputer);
    }

    m_bConnected = TRUE;
    m_hkeyConnect = hkey;
    m_hkeyRemote = hkeyRemote;
    m_sComputer = szName;

    return ERROR_SUCCESS;
}

//****************************************************************************
//
//  CRegistryKey::Disconnect
//
//****************************************************************************
LONG CRegistryKey::Disconnect(BOOL bForce)
{
    m_lResult = ERROR_SUCCESS;

    if (m_bConnected)
    {
        // Close the open key
        if (m_bOpen)
        {
            m_lResult = Close(bForce);
            if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
                g_bLostConnection = TRUE;                
            }

            if (!bForce && m_lResult != ERROR_SUCCESS)
                return m_lResult;
        }

        // Close the remote connection
        if (!g_bLostConnection) {
            if (!m_bLocal)
            {
                m_lResult = RegCloseKey(m_hkeyRemote);
                if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
                    g_bLostConnection = TRUE;
                }
                if (!bForce && m_lResult != ERROR_SUCCESS)
                    return m_lResult;
            }
        }
    }
    


    
    Initialize();

    return ERROR_SUCCESS;
}

//****************************************************************************
//
//  CRegistryKey::Create
//
//****************************************************************************
LONG CRegistryKey::Create(LPCTSTR pszKeyName, DWORD &dwDisposition,
    LPCTSTR pszClass, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecAttr)
{
    if (g_bLostConnection) {
        return RPC_S_SERVER_UNAVAILABLE;
    }
    
    HKEY hkeyOpen, hkey;

    m_lResult = ERROR_SUCCESS;
    dwDisposition = 0;

    if (m_bOpen)
    {
        m_lResult = Close(TRUE);
        if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
            g_bLostConnection = TRUE;
        }
        if (m_lResult != ERROR_SUCCESS)
            return m_lResult;
    }

    // If not connected, default to \\Local_Machine\HKEY_LOCAL_MACHINE
    if (!m_bConnected)
    {
        m_lResult = Connect();
        if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
            g_bLostConnection = TRUE;
        }
        if (m_lResult != ERROR_SUCCESS)
            return m_lResult;
    }

    // Attempt to create the specified subkey
    if (m_bLocal)
        hkey = m_hkeyConnect;
    else
        hkey = m_hkeyRemote;

    m_lResult = RegCreateKeyEx(hkey, pszKeyName, 0, (LPTSTR)pszClass,
        REG_OPTION_NON_VOLATILE, samDesired, lpSecAttr, &hkeyOpen,
        &dwDisposition);
    if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
        g_bLostConnection = TRUE;
    }
    if (m_lResult != ERROR_SUCCESS)
        return m_lResult;

    m_lResult = RegCloseKey(hkeyOpen);
    if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
        g_bLostConnection = TRUE;
        return m_lResult;
    }

    return Open(pszKeyName, samDesired);
}

//****************************************************************************
//
//  CRegistryKey::Open
//
//****************************************************************************
LONG CRegistryKey::Open(LPCTSTR pszKeyName, REGSAM samDesired)
{
    if (g_bLostConnection) {
        return RPC_S_SERVER_UNAVAILABLE;
    }


    HKEY hkeyOpen, hkey;
    CString sWork;
    int nPos;

    m_lResult = ERROR_SUCCESS;

    if (m_bOpen)
    {
        m_lResult = Close(TRUE);
        if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
            g_bLostConnection = TRUE;
        }
        if (m_lResult != ERROR_SUCCESS)
            return m_lResult;
    }

    // If not connected, default to \\Local_Machine\HKEY_LOCAL_MACHINE
    if (!m_bConnected)
    {
        m_lResult = Connect();
        if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
            g_bLostConnection = TRUE;
        }
        if (m_lResult != ERROR_SUCCESS)
            return m_lResult;
    }

    // Attempt to open the specified subkey
    if (m_bLocal)
        hkey = m_hkeyConnect;
    else
        hkey = m_hkeyRemote;
    m_lResult = RegOpenKeyEx(hkey, pszKeyName, 0, samDesired, &hkeyOpen);
    if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
        g_bLostConnection = TRUE;
    }
    if (m_lResult != ERROR_SUCCESS)
        return m_lResult;

    // Attempt to get info about this key.

    TCHAR szBuffer[1024 + 1];
    DWORD dwClass, dwSubKeys, dwMaxSubKey, dwMaxClass, dwValues;
    DWORD dwMaxValueName, dwMaxValueData, dwSecurityDescriptor;
    FILETIME ftLastWriteTime;

    dwClass = 1024 + 1;
    m_lResult = RegQueryInfoKey(hkeyOpen, szBuffer, &dwClass, 0, &dwSubKeys, 
        &dwMaxSubKey, &dwMaxClass, &dwValues, &dwMaxValueName,
        &dwMaxValueData, &dwSecurityDescriptor, &ftLastWriteTime);
    
    if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
        g_bLostConnection = TRUE;
        return m_lResult;
    }

    if (m_lResult != ERROR_SUCCESS)
    {
        RegCloseKey(hkeyOpen);
        return m_lResult;
    }

    // Success! save all the data.

    m_sFullName = pszKeyName;
    nPos = m_sFullName.ReverseFind('\\');
    if (nPos >= 0)
        m_sKeyName = m_sFullName.Mid(nPos + 1);
    else
        m_sKeyName = m_sFullName;

    m_hkeyOpen = hkeyOpen;
    m_bOpen = TRUE;
    m_Sam = samDesired;
    m_sClass = szBuffer;
    m_dwSubKeys = dwSubKeys;
    m_dwMaxSubKey = dwMaxSubKey;
    m_dwMaxClass = dwMaxClass;
    m_dwValues = dwValues;
    m_dwMaxValueName = dwMaxValueName;
    m_dwMaxValueData = dwMaxValueData;
    m_dwSecurityDescriptor = dwSecurityDescriptor;
    m_ftLastWriteTime = ftLastWriteTime;

    return ERROR_SUCCESS;
}

//****************************************************************************
//
//  CRegistryKey::Close
//
//****************************************************************************
LONG CRegistryKey::Close(BOOL bForce)
{
    if (!g_bLostConnection) {

        m_lResult = ERROR_SUCCESS;

        if (!m_bOpen)
            return ERROR_SUCCESS;


        if (m_bDirty)
        {
            m_lResult = RegFlushKey(m_hkeyOpen);
            if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
                g_bLostConnection = TRUE;
            }

            if (!bForce && m_lResult != ERROR_SUCCESS)
                return m_lResult;
        }

        if (!g_bLostConnection) {
            m_lResult = RegCloseKey(m_hkeyOpen);
            if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
                g_bLostConnection = TRUE;
            }

            if (!bForce && m_lResult != ERROR_SUCCESS)
                return m_lResult;
        }
    }

    m_bDirty = FALSE;

    m_hkeyOpen = NULL;
    m_bOpen = FALSE;
    m_sFullName.Empty();
    m_sClass.Empty();
    m_Sam = 0;

    m_dwSubKeys = 0;
    m_dwMaxSubKey = 0;
    m_dwMaxClass = 0;
    m_dwValues = 0;
    m_dwMaxValueName = 0;
    m_dwMaxValueData = 0;
    m_dwSecurityDescriptor = 0;

    m_ftLastWriteTime.dwLowDateTime = 0;
    m_ftLastWriteTime.dwHighDateTime = 0;

    if (g_bLostConnection) {
        m_lResult = RPC_S_SERVER_UNAVAILABLE;
        return RPC_S_SERVER_UNAVAILABLE;
    }
    else {
        m_lResult = ERROR_SUCCESS;
        return ERROR_SUCCESS;
    }
}

//****************************************************************************
//
//  CRegistryKey::EnumValues
//
//  Returns NULL if unsuccessful, returns empty array if successful but open
//  key has no values.
//  NOTE: Caller is responsible for deleting returned string array.
//
//****************************************************************************
CStringArray* CRegistryKey::EnumValues()
{
    if (g_bLostConnection) {
        m_lResult = RPC_S_SERVER_UNAVAILABLE;
        return NULL;
    }
         
    TCHAR szBuffer[1024 + 1];
    DWORD dwLength;
    CStringArray *pArr;
    int i;

    m_lResult = ERROR_SUCCESS;

    if (!m_bOpen || g_bLostConnection)
        return NULL;

    // Enumerate all the values into a string array
    pArr = new CStringArray;
    i = 0;
    m_lResult = ERROR_SUCCESS;
    while (TRUE)
    {
        dwLength = 1024 + 1;
        m_lResult = RegEnumValue(m_hkeyOpen, i, szBuffer, &dwLength, NULL,
            NULL, NULL, NULL);

        if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
            g_bLostConnection = TRUE;
        }

        if (m_lResult != ERROR_SUCCESS)
            break;
        if (dwLength > 0)
            pArr->Add(szBuffer);
        i++;
    }

    // Did we find a normal end condition?
    if (m_lResult == ERROR_NO_MORE_ITEMS)
        return pArr;

    delete pArr;
    return NULL;
}

//****************************************************************************
//
//  CRegistryKey::EnumSubKeys
//
//  Returns NULL if unsuccessful, returns empty array if successful but open
//  key has no values.
//  NOTE: Caller is responsible for deleting returned string array.
//
//****************************************************************************
CStringArray* CRegistryKey::EnumSubKeys()
{
    if (g_bLostConnection) {
        m_lResult = RPC_S_SERVER_UNAVAILABLE;
        return NULL;
    }



    TCHAR szBuffer[1024 + 1];
    DWORD dwLength;
    CStringArray *pArr;
    int i;

    m_lResult = ERROR_SUCCESS;

    if (!m_bOpen)
        return NULL;

    // Enumerate all the subkeys into a string array
    pArr = new CStringArray;
    i = 0;

    while (TRUE)
    {
        dwLength = 1024 + 1;
        m_lResult = RegEnumKeyEx(m_hkeyOpen, i, szBuffer, &dwLength, NULL,
            NULL, NULL, NULL);

        if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
            g_bLostConnection = TRUE;
        }

        if (m_lResult != ERROR_SUCCESS)
            break;
        if (dwLength > 0)
            pArr->Add(szBuffer);
        i++;
    }

    // Did we find a normal end condition?
    if (m_lResult == ERROR_NO_MORE_ITEMS)
        return pArr;

    delete pArr;
    return NULL;
}

//****************************************************************************
//
//  CRegistryKey::GetValue
//
//  Note: regval is always emptied regardless of success/failure
//
//****************************************************************************
BOOL CRegistryKey::GetValue(LPCTSTR pszValue, CRegistryValue &regval)
{

    DWORD dwLength, dwType;
    BYTE *pBuffer;

    regval.Empty();

    if (g_bLostConnection) {
        m_lResult = RPC_S_SERVER_UNAVAILABLE;
        return FALSE;
    }

    if (!m_bOpen)
    {
        m_lResult = ERROR_INVALID_FUNCTION;
        return FALSE;
    }

    // Find out how big the data is
    m_lResult = RegQueryValueEx(m_hkeyOpen, (LPTSTR)pszValue, NULL, NULL,
        NULL, &dwLength);

    if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
        g_bLostConnection = TRUE;
    }

    if (m_lResult != ERROR_SUCCESS)
        return FALSE;

    if (dwLength == 0)
        return TRUE;

    // Now make a buffer big enough for it.
    pBuffer = new BYTE[dwLength];
    if (pBuffer == NULL)
        return FALSE;

    m_lResult = RegQueryValueEx(m_hkeyOpen, (LPTSTR)pszValue, NULL, &dwType,
        pBuffer, &dwLength);


    if (m_lResult == ERROR_SUCCESS)
        regval.Set(pszValue, dwType, dwLength, pBuffer);

    delete pBuffer;

    if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
        g_bLostConnection = TRUE;
    }

    if (m_lResult != ERROR_SUCCESS)
        return FALSE;

    return TRUE;
}

//****************************************************************************
//
//  CRegistryKey::SetValue
//
//****************************************************************************
BOOL CRegistryKey::SetValue(CRegistryValue &regval)
{
    if (g_bLostConnection) {
        m_lResult = RPC_S_SERVER_UNAVAILABLE;
        return FALSE;
    }

    if (!m_bOpen)
    {
        m_lResult = ERROR_INVALID_FUNCTION;
        return FALSE;
    }
    
    if (regval.m_sName.IsEmpty())
    {
        m_lResult = ERROR_INVALID_DATA;
        return FALSE;
    }

    m_lResult = RegSetValueEx(m_hkeyOpen, regval.m_sName, 0, regval.m_dwType,
        regval.m_pData, regval.m_dwDataLength);

    if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
        g_bLostConnection = TRUE;
    }

    if (m_lResult != ERROR_SUCCESS)
        return FALSE;

    m_bDirty = TRUE;

    return TRUE;
}

//****************************************************************************
//
//  CRegistryKey::GetSubKey
//
//  Note: If successful, regkey is returned connected and open on the
//  specified key.  If failure, regkey is returned disconnected.
//
//****************************************************************************
BOOL CRegistryKey::GetSubKey(LPCTSTR pszSubKey, CRegistryKey &regkey)
{
    if (g_bLostConnection) {
        m_lResult = RPC_S_SERVER_UNAVAILABLE;
        return FALSE;
    }


    CString sSubKey;

    m_lResult = ERROR_SUCCESS;

    if (!m_bOpen)
        return FALSE;

    m_lResult = regkey.Disconnect(TRUE);

    if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
        g_bLostConnection = TRUE;
    }

    if (m_lResult != ERROR_SUCCESS)
        return FALSE;

    // Try to connect and open same key
    m_lResult = regkey.Connect(m_sComputer, m_hkeyConnect);

    if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
        g_bLostConnection = TRUE;
    }

    if (m_lResult != ERROR_SUCCESS)
        return FALSE;
    sSubKey = pszSubKey;
    m_lResult = regkey.Open(m_sFullName + "\\" + sSubKey, m_Sam);

    if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
        g_bLostConnection = TRUE;
    }

    if (m_lResult != ERROR_SUCCESS)
    {
        regkey.Disconnect(TRUE);
        return FALSE;
    }

    return TRUE;
}

//****************************************************************************
//
//  CRegistryKey::CreateSubKey
//
//  Note: If successful, regkey is returned connected and open on the
//  new key; if the key already existed, it is simply opened.  If failure,
//  regkey is returned disconnected.
//
//****************************************************************************
BOOL CRegistryKey::CreateSubKey(
    LPCTSTR pszSubKey, 
    CRegistryKey &regkey,
    LPCTSTR pszClass, 
    LPSECURITY_ATTRIBUTES lpSecAttr,
    BOOL bIsVolatile)
{
    if (g_bLostConnection) {
        m_lResult = RPC_S_SERVER_UNAVAILABLE;
        return FALSE;
    }
 
 
    CString sSubKey, sClass;
    HKEY hkeyOpen;
    DWORD dwDisposition;

    m_lResult = ERROR_SUCCESS;

    if (!m_bOpen)
        return FALSE;

    m_lResult = regkey.Disconnect(TRUE);

    if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
        g_bLostConnection = TRUE;
    }

    if (m_lResult != ERROR_SUCCESS)
        return FALSE;

    // Try to connect and open same key
    m_lResult = regkey.Connect(m_sComputer, m_hkeyConnect);

    if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
        g_bLostConnection = TRUE;
    }

    if (m_lResult != ERROR_SUCCESS)
        return FALSE;
    sSubKey = pszSubKey;
    sClass = pszClass;
    DWORD dwRegOptions = bIsVolatile ? REG_OPTION_VOLATILE : REG_OPTION_NON_VOLATILE;
    m_lResult = RegCreateKeyEx(m_hkeyOpen, sSubKey, 0, (LPTSTR)(LPCTSTR)sClass,
        dwRegOptions, m_Sam, lpSecAttr, &hkeyOpen, &dwDisposition);

    if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
        g_bLostConnection = TRUE;
    }

    if (m_lResult != ERROR_SUCCESS)
    {
        regkey.Disconnect(TRUE);
        return FALSE;
    }
    m_lResult = RegCloseKey(hkeyOpen);

    if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
        g_bLostConnection = TRUE;
    }

    if (m_lResult != ERROR_SUCCESS)
        return FALSE;

    m_lResult = regkey.Open(m_sFullName + "\\" + sSubKey, m_Sam);

    if (m_lResult != ERROR_SUCCESS)
    {
        if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
            g_bLostConnection = TRUE;
        }

        regkey.Disconnect(TRUE);
        return FALSE;
    }

    m_bDirty = TRUE;
    if (dwDisposition == REG_CREATED_NEW_KEY)
        regkey.m_bDirty = TRUE;

    return TRUE;
}

//****************************************************************************
//
//  CRegistryKey::DeleteSubKey
//
//****************************************************************************
BOOL CRegistryKey::DeleteSubKey(LPCTSTR pszSubKey)
{
    if (g_bLostConnection) {
        return FALSE;
    }

    CString sSubKey;
    CRegistryKey subkey;
    int i;

    m_lResult = ERROR_SUCCESS;
    sSubKey = pszSubKey;

    if (!m_bOpen)
        return FALSE;

    if (!GetSubKey(sSubKey, subkey))
        return FALSE;

    // Delete all subkeys of the specified subkey (RegDeleteKey limitation)
    CStringArray *parr = subkey.EnumSubKeys();
    for (i=0; i<parr->GetSize(); i++)
    {
        if (!subkey.DeleteSubKey(parr->GetAt(i)))
            return FALSE;
    }
    delete parr;

    subkey.Close(TRUE);

    m_lResult = RegDeleteKey(m_hkeyOpen, sSubKey);
    if (m_lResult != ERROR_SUCCESS) {
        if ((m_lResult == RPC_S_SERVER_UNAVAILABLE) || (m_lResult == RPC_S_CALL_FAILED)) {
            g_bLostConnection = TRUE;
        }
        return FALSE;
    }

    return TRUE;
}
