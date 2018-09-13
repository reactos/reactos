//****************************************************************************
//
//  Copyright (c) 1995,  Microsoft Corp.
//
//  File:  REGKEY.H
//
//  Definitions for registry management classes
//
//  History:
//      Scott V. Walker, SEA  10/5/94
//
//****************************************************************************
#ifndef _REGKEY_H_
#define _REGKEY_H_

#include <tchar.h>



//****************************************************************************
//
//  CLASS:  CRegistryValue
//
//****************************************************************************
class CRegistryValue : public CObject
{
    DECLARE_DYNAMIC(CRegistryValue)

public:
    CString m_sName;
    DWORD m_dwType;
    DWORD m_dwDataLength;
    LPBYTE m_pData;

public:
    CRegistryValue();
    CRegistryValue(LPCTSTR pszName, DWORD dwType, DWORD dwDataLength,
        LPBYTE pData);
    ~CRegistryValue();

    void Set(LPCTSTR pszName, DWORD dwType, DWORD dwDataLength,
        LPBYTE pData);
    void Get(CString &sName, DWORD &dwType, DWORD &dwDataLength,
        LPBYTE pData = NULL);
    void Empty();
    const CRegistryValue& operator=(CRegistryValue &other);
};

//****************************************************************************
//
//  CLASS:  CRegistryKey
//
//****************************************************************************
class CRegistryKey : public CObject
{
    DECLARE_DYNAMIC(CRegistryKey)

public:
    CString m_sComputer;    // Name of computer we're connected to
    HKEY m_hkeyConnect;     // Handle to current connection key (or NULL)
    HKEY m_hkeyRemote;      // Handle to remote connection key (or NULL)
    BOOL m_bConnected;      // TRUE if currently connected
    BOOL m_bLocal;          // TRUE if connected to the local computer

    HKEY m_hkeyOpen;        // Handle to currently open key (or NULL)
    BOOL m_bOpen;           // TRUE if currently open
    CString m_sFullName;    // Full path name of currently open key
    CString m_sKeyName;     // Name of currently open key
    REGSAM m_Sam;           // Security access mask we opened with

    BOOL m_bDirty;          // TRUE if there are changes pending in this key

    CString m_sClass;       // Class name of key
    DWORD m_dwSubKeys;      // Number of subkeys in this key
    DWORD m_dwMaxSubKey;    // Longest subkey name length
    DWORD m_dwMaxClass;     // Longest class string length
    DWORD m_dwValues;       // Number of value entries in current key
    DWORD m_dwMaxValueName; // Longest value name length
    DWORD m_dwMaxValueData; // Longest value data length
    DWORD m_dwSecurityDescriptor;   // Security descriptor length

    FILETIME m_ftLastWriteTime; // Last modification date for key or values

    LONG m_lResult;         // Last return value from a registry API

public:
    CRegistryKey();
    ~CRegistryKey();

    void Initialize();
    LONG Connect(LPCTSTR pszComputer = NULL,
        HKEY hkey = HKEY_LOCAL_MACHINE);
    LONG Disconnect(BOOL bForce = FALSE);
    LONG Open(LPCTSTR pszKeyName, REGSAM samDesired = KEY_ALL_ACCESS);
    LONG Create(LPCTSTR pszKeyName, DWORD &dwDisposition,
        LPCTSTR pszClass = NULL, REGSAM samDesired = KEY_ALL_ACCESS,
        LPSECURITY_ATTRIBUTES lpSecAttr = NULL);
    LONG Close(BOOL bForce = FALSE);
    CStringArray* EnumValues();
    CStringArray* EnumSubKeys();
    BOOL GetValue(LPCTSTR pszValue, CRegistryValue &regval);
    BOOL SetValue(CRegistryValue &regval);
    BOOL GetSubKey(LPCTSTR pszSubKey, CRegistryKey &regkey);
    BOOL CreateSubKey(LPCTSTR pszSubKey, CRegistryKey &regkey,
        LPCTSTR pszClass = NULL, LPSECURITY_ATTRIBUTES lpSecAttr = NULL, BOOL bIsVolatile = FALSE);
    BOOL DeleteSubKey(LPCTSTR pszSubKey);	
};



class CEventTrapRegistry
{
public:
	CEventTrapRegistry();
	~CEventTrapRegistry();
};

extern BOOL g_bLostConnection;

#endif // _REGKEY_H_
