#ifndef _INC_DSKQUOTA_REGISTRY_H
#define _INC_DSKQUOTA_REGISTRY_H

#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef _INC_DSKQUOTA_STRCLASS_H
#   include "strclass.h"
#endif

#ifndef _INC_DSKQUOTA_CARRAY_H
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
        RegKey(void);
        RegKey(HKEY hkeyRoot, LPCTSTR pszSubKey);
        virtual ~RegKey(void);

        operator HKEY(void) const
            { return m_hkey; }

        HKEY GetHandle(void) const
            { return m_hkey; }

        HRESULT Open(REGSAM samDesired, bool bCreate = false) const;
        void Attach(HKEY hkey);
        void Detach(void);
        void Close(void) const;

        int GetValueBufferSize(
            LPCTSTR pszValueName) const;

        bool IsOpen(void) const
            { return NULL != m_hkey; }

        HRESULT WatchForChange(DWORD dwChangeFilter, bool bWatchSubtree = false);
        HRESULT WaitForChange(DWORD dwChangeFilter, bool bWatchSubtree = false);

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

    protected:
        virtual void OnChange(HKEY hkey);

    private:
        HKEY         m_hkeyRoot;
        mutable HKEY m_hkey;
        HANDLE       m_hChangeEvent;
        DWORD        m_dwChangeFilter;
        CString      m_strSubKey;
        bool         m_bWatchSubtree;

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

        static DWORD NotifyWaitThreadProc(LPVOID pvParam);

        //
        // Prevent copy.
        //
        RegKey(const RegKey& rhs);
        RegKey& operator = (const RegKey& rhs);
};


#if DBG
class RegKeyChg : public RegKey
{
    public:
        RegKeyChg(HKEY hkeyRoot, LPCTSTR pszSubKey);
        ~RegKeyChg(void);

    protected:
        virtual void OnChange(HKEY hkey);
};

#endif // #if DBG

#endif // _INC_DSKQUOTA_REGISTRY_H
