//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       registry.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCVIEW_REGISTRY_H
#define _INC_CSCVIEW_REGISTRY_H

#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef _INC_CSCVIEW_STRCLASS_H
#   include "strclass.h"
#endif

#ifndef _INC_CSCVIEW_CARRAY_H
#   include "carray.h"
#endif
//
// Represents a single registry key.  Provides basic functions for 
// opening and closing the key as well as setting and querying for
// values in that key.  Closure of the key handle is ensured through
// the destructor.
//
class RegKey
{
    public:
        //
        // Class for iterating values in the key.
        //
        class ValueIterator
        {
            public:
                explicit ValueIterator(RegKey& key);

                HRESULT Next(CString *pstrName, LPDWORD pdwType, LPBYTE pbData, LPDWORD pcbData);

            private:
                RegKey& m_key;
                int     m_iValue;
                int     m_cchValueName;
        };


        //
        // Flags used by QueryInfo().
        //
        enum
        {
            QIM_SUBKEYCNT           = 0x00000001,
            QIM_SUBKEYMAXCHARCNT    = 0x00000002,
            QIM_CLASSMAXCHARCNT     = 0x00000004,
            QIM_VALUECNT            = 0x00000008,
            QIM_VALUENAMEMAXCHARCNT = 0x00000010,
            QIM_VALUEMAXBYTECNT     = 0x00000020,
            QIM_LASTWRITETIME       = 0x00000040,
            QIM_SECURITYBYTECNT     = 0x00000080,
            QIM_ALL                 = 0xFFFFFFFF
        };

        //
        // Data returned by QueryInfo().
        //
        struct KEYINFO
        {
            DWORD    cSubKeys;
            DWORD    cchSubKeyMax;
            DWORD    cchClassMax;
            DWORD    cValues;
            DWORD    cchValueNameMax;
            DWORD    cbValueMax;
            DWORD    cbSecurityDesc;
            FILETIME LastWriteTime;
        };

        RegKey(void);
        RegKey(HKEY hkeyRoot, LPCTSTR pszSubKey);
        virtual ~RegKey(void);

        operator HKEY(void) const
            { return m_hkey; }

        HKEY GetHandle(void) const
            { return m_hkey; }

        bool IsOpen(void) const
            { return NULL != m_hkey; }

        bool IsMonitoringChanges(void) const
            { return NULL != m_hChangeEvent; }

        HRESULT Open(REGSAM samDesired, bool bCreate = false) const;
        void Attach(HKEY hkey);
        void Detach(void);
        void Close(void) const;

        const CString& SubKeyName(void) const
            { return m_strSubKey; }

        int GetValueBufferSize(
            LPCTSTR pszValueName) const;

        HRESULT ValueExists(
            LPCTSTR pszValueName) const;

        //
        // Retrieve REG_DWORD
        //
        HRESULT GetValue(
            LPCTSTR pszValueName,
            DWORD *pdwDataOut) const;
        //
        // Retrieve REG_BINARY
        //
        HRESULT GetValue(
            LPCTSTR pszValueName,
            LPBYTE pbDataOut,
            int cbDataOut) const;
        //
        // Retrieve REG_SZ
        //
        HRESULT GetValue(
            LPCTSTR pszValueName,
            CString *pstrDataOut) const;
        //
        // Retrieve REG_MULTI_SZ
        //
        HRESULT GetValue(
            LPCTSTR pszValueName,
            CArray<CString> *prgstrOut) const;
        //
        // Set REG_DWORD
        //
        HRESULT SetValue(
            LPCTSTR pszValueName,
            DWORD dwData);
        //
        // Set REG_BINARY
        //
        HRESULT SetValue(
            LPCTSTR pszValueName,
            const LPBYTE pbData,
            int cbData);
        //
        // Set REG_SZ
        //
        HRESULT SetValue(
            LPCTSTR pszValueName,
            LPCTSTR pszData);
        //
        // Set REG_MULTI_SZ
        //
        HRESULT SetValue(
            LPCTSTR pszValueName,
            const CArray<CString>& rgstrData);

        HRESULT MonitorChanges(
            DWORD dwChangeFilter, 
            bool bWatchSubtree);

        HRESULT DeleteValue(
            LPCTSTR pszValue);

        HRESULT DeleteAllValues(
            int *pcNotDeleted);

        HRESULT QueryInfo(RegKey::KEYINFO *pInfo, DWORD fMask) const;

        RegKey::ValueIterator CreateValueIterator(void)
            { return ValueIterator(*this); }

        bool HasChanged(void);


    private:
        HKEY           m_hkeyRoot;
        mutable HKEY   m_hkey;
        mutable HANDLE m_hChangeEvent;
        CString        m_strSubKey;

        HRESULT SetValue(
            LPCTSTR pszValueName,
            DWORD dwValueType,
            const LPBYTE pbData, 
            int cbData);

        HRESULT GetValue(
            LPCTSTR pszValueName,
            DWORD dwTypeExpected,
            LPBYTE pbData,
            int cbData) const;

        LPTSTR CreateDoubleNulTermList(
            const CArray<CString>& rgstrSrc) const;

        //
        // Prevent copy.
        //
        RegKey(const RegKey& rhs);
        RegKey& operator = (const RegKey& rhs);
};


//
// Use this object when you can't trust HKEY_CURRENT_USER to be correct.
// It uses the new NT5 API RegOpenCurrentUser to open the key.
//
class RegKeyCU
{
    public:
        RegKeyCU(REGSAM samDesired);
        ~RegKeyCU(void);

        operator HKEY(void) const
            { return m_hkey; }

    private:
        HKEY m_hkey;
};


#endif // _INC_CSCVIEW_REGISTRY_H
