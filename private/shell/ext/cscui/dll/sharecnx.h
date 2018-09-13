//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       sharecnx.h
//
//--------------------------------------------------------------------------

#ifndef _WINDOWS_
#   include <windows.h>
#endif
#ifndef _INC_COMMCTRL_
#   include <commctrl.h>
#endif
#ifndef _INC_COMCTRLP
#   include <comctrlp.h>
#endif

class CShareCnxStatusCache
{
    public:
        CShareCnxStatusCache(void);
        ~CShareCnxStatusCache(void);

        HRESULT IsOpenConnectionShare(LPCTSTR pszShare, bool bRefresh = false);
        HRESULT IsOpenConnectionPathUNC(LPCTSTR pszPathUNC, bool bRefresh = false);

    private:
        class Entry
        {
            public:
                enum { StatusOpenCnx = 0x00000001 };     // 1 == Open connection.

                explicit Entry(LPCTSTR pszShare, DWORD dwStatus = 0);
                ~Entry(void);
                //
                // Refresh cached data in m_dwStatus member.
                //
                HRESULT Refresh(void);
                //
                // Returns the status DWORD.
                //
                DWORD Status(void) const
                    { return m_dwStatus; }
                //
                // Returns true if requested status bits are set.
                //
                bool CheckStatus(DWORD dwMask) const
                    { return boolify(dwMask == (m_dwStatus & dwMask)); }
                //
                // Returns error bits from m_dwStatus member.
                //
                HRESULT LastResult(void) const
                    { return m_hrLastResult; }
                //
                // Returns address of share name.  Can be NULL.
                //
                LPCTSTR Share(void) const
                    { return m_pszShare; }
                //
                // Returns true if share name ptr is non-null and all error
                // bits in m_dwStatus member are clear.
                //
                bool IsValid(void) const
                    { return NULL != Share() && SUCCEEDED(m_hrLastResult); }
                //
                // Static function for obtaining share's system status.  Result goes
                // in m_dwStatus member.  Static so that creator of an Entry object
                // can use the status value in the Entry ctor.
                //
                static HRESULT QueryShareStatus(LPCTSTR pszShare, DWORD *pdwStatus);

            private:
                LPTSTR m_pszShare;     // Name of the share associated with entry.
                DWORD  m_dwStatus;     // Share status and error bits.
                DWORD  m_hrLastResult; // Last query result.
                //
                // Trivial copy-a-string helper.
                //
                LPTSTR StrDup(LPCTSTR psz);
                //
                // Prevent copy.
                //
                Entry(const Entry& rhs);
                Entry& operator = (const Entry& rhs);
                //
                // We don't want folks creating blank entries.
                // The error reporting structure assumes that a NULL
                // m_pszShare member indicates an allocation failure.
                //
                Entry(void)
                    : m_pszShare(NULL),
                      m_dwStatus(0),
                      m_hrLastResult(E_FAIL) { }
        };

        HDPA m_hdpa;  // Dynamic array of Entry object ptrs.

        //
        // Number of entries in cache.
        //
        int Count(void) const;
        //
        // Add an entry to the cache.
        //
        Entry *AddEntry(LPCTSTR pszShare, DWORD dwStatus);
        //
        // Find an entry in the cache.
        //
        Entry *FindEntry(LPCTSTR pszShare) const;
        //
        // Retrieve a given entry at a given DPA index.
        //
        Entry *GetEntry(int iEntry) const
            { return (Entry *)DPA_GetPtr(m_hdpa, iEntry); }
        //
        // Same as GetEntry but won't AV if m_hdpa is NULL.
        //
        Entry *SafeGetEntry(int iEntry) const
            { return NULL != m_hdpa ? GetEntry(iEntry) : NULL; }
        //
        // Get the status information for a share.
        //
        HRESULT GetShareStatus(LPCTSTR pszShare, DWORD *pdwStatus, bool bRefresh);
        //
        // Prevent copy.
        //
        CShareCnxStatusCache(const CShareCnxStatusCache& rhs);
        CShareCnxStatusCache& operator = (const CShareCnxStatusCache& rhs);
};

